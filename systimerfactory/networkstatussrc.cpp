/*
 * Copyright 2023 Comcast Cable Communications Management, LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string>
#include <algorithm>
#include <thread>
#include <chrono>
#include <cctype>
#include <unistd.h>

#include "networkstatussrc.h"
#include "irdklog.h"
#include "secure_wrapper.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "core/SystemInfo.h"
#include "websocket/JSONRPCLink.h"
using namespace WPEFramework;


using namespace std;
static std::string lastStatus;

/* Offset threshold in seconds above which makestep is triggered even if
 * chrony already has a selectable source (natural slewing would be too slow). */
static constexpr double OFFSET_STEP_THRESHOLD_S = 1.0;

/**
 * Return the absolute system-clock offset (seconds) as reported by
 * "chronyc tracking".  Positive means the local clock is fast relative to the
 * NTP reference; negative means it is slow.  Returns 0.0 on any parse error.
 *
 * Example line parsed:
 *   System time     : 2.345678901 seconds slow of NTP time
 */
static double getChronyOffset()
{
    FILE *fp = v_secure_popen("r", "chronyc tracking");
    if (!fp) {
        RDK_LOG(RDK_LOG_WARN, LOG_SYSTIME,
                "[%s:%d]: CHRONY: Failed to open chronyc tracking pipe\n",
                __FUNCTION__, __LINE__);
        return 0.0;
    }

    double offset = 0.0;
    char   line[256];
    while (fgets(line, static_cast<int>(sizeof(line)), fp)) {
        /* The line starts with "System time" (may have extra spaces before ':'). */
        if (strstr(line, "System time") == nullptr)
            continue;

        /* Format: "System time     : <value> seconds slow|fast of NTP time" */
        const char *colon = strchr(line, ':');
        if (!colon)
            break;

        double val = 0.0;
        char   direction[16] = {};
        if (sscanf(colon + 1, " %lf seconds %15s", &val, direction) == 2) {
            /* Positive offset  → clock is fast (ahead of NTP) */
            offset = (strncmp(direction, "fast", 4) == 0) ? val : -val;
        }
        break;
    }

    v_secure_pclose(fp);
    return offset;
}

const unsigned int ACTIVATION_RETRY_INTERVAL_MS = 1000;

const char* NETWORK_MANAGER_CALLSIGN = "org.rdk.NetworkManager";
const char* INTERNET_EVENT_NAME = "onInternetStatusChange";

static WPEFramework::JSONRPC::SmartLinkType<WPEFramework::Core::JSON::IElement>* thunder_client = nullptr;
static bool m_networkeventsubscribed = false;


void handle_internetStatusChange(const JsonObject& params)
{
   string internetStatus;
   if (params.HasLabel("status")) {
      internetStatus = params["status"].String();
   }
   else if (params.HasLabel("internetStatus")) {
      internetStatus = params["internetStatus"].String();
   }

   string normalizedStatus(internetStatus);
   transform(normalizedStatus.begin(), normalizedStatus.end(), normalizedStatus.begin(), [](unsigned char ch) {
      return static_cast<char>(std::tolower(ch));
   });

   RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]: CHRONY: Internet status change notification received. status = %s\n",
           __FUNCTION__,__LINE__,normalizedStatus.c_str());

   if (normalizedStatus == "fully_connected" && normalizedStatus != lastStatus) {
      bool firstSyncDone = (access("/tmp/clock-event", F_OK) == 0);

      if (!firstSyncDone) {
         /* /tmp/clock-event is absent — NTP has never successfully synced on
          * this boot.  Two sub-cases:
          *
          *  a) Device booted WITH internet: iburst is (or was) running and
          *     may already have selected a source.  Do nothing and let chronyd
          *     finish its iburst phase naturally.
          *
          *  b) Device booted WITHOUT internet: iburst fired but found no
          *     reachable server.  Now that connectivity is available we must
          *     kick off burst+waitsync+makestep ourselves — otherwise the
          *     device will never get an initial time correction.
          *
          * We distinguish (a) from (b) by checking whether chrony already
          * has a selectable source. */
         int noSrcCheck = v_secure_system("chronyc sources | grep -qE '^\\^[*+]'");
         if (noSrcCheck == 0) {
            /* Sub-case (a): iburst is working fine — leave it alone. */
            RDK_LOG(RDK_LOG_INFO, LOG_SYSTIME,
                    "[%s:%d]: CHRONY: /tmp/clock-event absent but iburst already"
                    " selected a source — letting chronyd complete naturally\n",
                    __FUNCTION__, __LINE__);
         } else {
            /* Sub-case (b): no clock-event AND no selectable source.
             * Device booted without internet; trigger sync now that
             * connectivity is available. */
            RDK_LOG(RDK_LOG_INFO, LOG_SYSTIME,
                    "[%s:%d]: CHRONY: /tmp/clock-event absent and no selectable"
                    " source — device booted without internet, initiating"
                    " burst+waitsync+makestep\n",
                    __FUNCTION__, __LINE__);

            int burstRet = v_secure_system("/usr/sbin/chronyc burst 3/4");
            if (burstRet != 0) {
               RDK_LOG(RDK_LOG_WARN, LOG_SYSTIME,
                       "[%s:%d]: CHRONY: chronyc burst failed with code %d\n",
                       __FUNCTION__, __LINE__, burstRet);
            } else {
               RDK_LOG(RDK_LOG_INFO, LOG_SYSTIME,
                       "[%s:%d]: CHRONY: burst triggered (boot-without-internet"
                       " recovery), waiting for source selection\n",
                       __FUNCTION__, __LINE__);
            }

            int waitRet = v_secure_system("/usr/sbin/chronyc waitsync 10 0 0 1");
            if (waitRet != 0) {
               RDK_LOG(RDK_LOG_ERROR, LOG_SYSTIME,
                       "[%s:%d]: CHRONY: waitsync timed out (code %d) during"
                       " boot-without-internet recovery. Skipping makestep.\n",
                       __FUNCTION__, __LINE__, waitRet);
            } else {
               RDK_LOG(RDK_LOG_INFO, LOG_SYSTIME,
                       "[%s:%d]: CHRONY: waitsync completed (boot-without-internet"
                       " recovery). Proceeding to makestep.\n",
                       __FUNCTION__, __LINE__);
               int stepRet = v_secure_system("/usr/sbin/chronyc makestep");
               if (stepRet != 0) {
                  RDK_LOG(RDK_LOG_ERROR, LOG_SYSTIME,
                          "[%s:%d]: CHRONY: makestep failed with code %d\n",
                          __FUNCTION__, __LINE__, stepRet);
               } else {
                  RDK_LOG(RDK_LOG_INFO, LOG_SYSTIME,
                          "[%s:%d]: CHRONY: makestep completed (boot-without-internet recovery)\n",
                          __FUNCTION__, __LINE__);
               }
            }
         }
      } else {
         /* Check whether chrony already has a selectable (selected '*' or combined '+') source.
          * grep -qE '^\^[*+]' matches lines chronyc sources prints for selected/combined peers.
          * Exit 0  => at least one selectable source exists — safe to makestep immediately.
          * Exit !0 => no selectable source; burst + waitsync required first. */
         int srcRet = v_secure_system("chronyc sources | grep -qE '^\\^[*+]'");
         bool hasSelectableSource = (srcRet == 0);

         if (!hasSelectableSource) {
            /* No selectable source — issue burst to gather fresh samples, then wait
             * up to 10 s for chrony to select one before stepping the clock.
             * waitsync args: max-tries=10, max-correction=0 (any), max-skew=0 (any), interval=1s */
            RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,
                    "[%s:%d]: CHRONY: No selectable source found, issuing burst 3/4\n",
                    __FUNCTION__,__LINE__);
            int burstRet = v_secure_system("/usr/sbin/chronyc burst 3/4");
            if (burstRet != 0) {
               RDK_LOG(RDK_LOG_WARN,LOG_SYSTIME,
                       "[%s:%d]: CHRONY: chronyc burst failed with code %d\n",
                       __FUNCTION__,__LINE__, burstRet);
            } else {
               RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,
                       "[%s:%d]: CHRONY: chronyc burst triggered, waiting for source selection\n",
                       __FUNCTION__,__LINE__);
            }

            /* waitsync 10 0 0 1 : poll every 1 s, up to 10 tries (~10 s).
             * makestep MUST NOT run if waitsync times out — chrony has no synced
             * reference at that point and makestep will fail/produce garbage. */
            RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,
                    "[%s:%d]: CHRONY: Running chronyc waitsync (max 10s)\n",
                    __FUNCTION__,__LINE__);
            int waitRet = v_secure_system("/usr/sbin/chronyc waitsync 10 0 0 1");
            if (waitRet != 0) {
               /* Timed out — still no synced source; skip makestep to avoid a
                * meaningless or erroneous clock step. */
               RDK_LOG(RDK_LOG_ERROR,LOG_SYSTIME,
                       "[%s:%d]: CHRONY: waitsync timed out (code %d), no synced source available."
                       " Skipping makestep.\n",
                       __FUNCTION__,__LINE__, waitRet);
            } else {
               RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,
                       "[%s:%d]: CHRONY: waitsync completed, source selected. Proceeding to makestep.\n",
                       __FUNCTION__,__LINE__);
               int stepRet = v_secure_system("/usr/sbin/chronyc makestep");
               if (stepRet != 0) {
                  RDK_LOG(RDK_LOG_ERROR,LOG_SYSTIME,
                          "[%s:%d]: CHRONY: chronyc makestep failed with code %d\n",
                          __FUNCTION__,__LINE__, stepRet);
               } else {
                  RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,
                          "[%s:%d]: CHRONY: chronyc makestep completed successfully\n",
                          __FUNCTION__,__LINE__);
               }
            }
         } else {
            /* Selectable source already present.
             * Only step the clock if the current offset exceeds the threshold;
             * smaller offsets are handled faster and safer by chrony's natural
             * frequency-slewing, so an explicit makestep is unnecessary. */
            double offset = getChronyOffset();
            double absOffset = std::fabs(offset);
            RDK_LOG(RDK_LOG_INFO, LOG_SYSTIME,
                    "[%s:%d]: CHRONY: Selectable source present, current offset = %.6f s"
                    " (threshold = %.1f s)\n",
                    __FUNCTION__, __LINE__, offset, OFFSET_STEP_THRESHOLD_S);

            if (absOffset > OFFSET_STEP_THRESHOLD_S) {
               RDK_LOG(RDK_LOG_INFO, LOG_SYSTIME,
                       "[%s:%d]: CHRONY: Offset exceeds threshold, issuing makestep\n",
                       __FUNCTION__, __LINE__);
               int stepRet = v_secure_system("/usr/sbin/chronyc makestep");
               if (stepRet != 0) {
                  RDK_LOG(RDK_LOG_ERROR, LOG_SYSTIME,
                          "[%s:%d]: CHRONY: chronyc makestep failed with code %d\n",
                          __FUNCTION__, __LINE__, stepRet);
               } else {
                  RDK_LOG(RDK_LOG_INFO, LOG_SYSTIME,
                          "[%s:%d]: CHRONY: chronyc makestep completed successfully\n",
                          __FUNCTION__, __LINE__);
               }
            } else {
               RDK_LOG(RDK_LOG_INFO, LOG_SYSTIME,
                       "[%s:%d]: CHRONY: Offset (%.6f s) within threshold — allowing"
                       " natural slew, no makestep needed\n",
                       __FUNCTION__, __LINE__, offset);
            }
         }
      }
   }
   lastStatus = normalizedStatus;
}


static void subscribeToInternetEvent()
{
    unsigned int attempt = 0;
    while (!m_networkeventsubscribed) {
        attempt++;
        if (!thunder_client)
            thunder_client = new WPEFramework::JSONRPC::SmartLinkType<Core::JSON::IElement>(NETWORK_MANAGER_CALLSIGN, "");

        if (thunder_client) {
            int32_t ret = thunder_client->Subscribe<JsonObject>(5000, "onInternetStatusChange", &handle_internetStatusChange);
            if (ret == Core::ERROR_NONE) {
                RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]: CHRONY: Successfully subscribed to onInternetStatusChange (attempt %u)\n", __FUNCTION__,__LINE__,attempt);
                m_networkeventsubscribed = true;
                return;
            }
            RDK_LOG(RDK_LOG_WARN,LOG_SYSTIME,"[%s:%d]: CHRONY: Subscribe to onInternetStatusChange failed (%d), attempt %u\n",__FUNCTION__,__LINE__,ret,attempt);
            delete thunder_client; thunder_client = nullptr;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(ACTIVATION_RETRY_INTERVAL_MS));
    }
}

void NetworkStatusSrc::subscribeInternetStatusEvent()
{
    Core::SystemInfo::SetEnvironment("THUNDER_ACCESS", "127.0.0.1:9998");
    // NetworkManager activates once and lives for the process lifetime.
    // Run the retry loop directly on the calling thread (runNetworkStatusMonitor).
    subscribeToInternetEvent();
}

NetworkStatusSrc::~NetworkStatusSrc()
{
    if (thunder_client) {
        thunder_client->Unsubscribe(5000, "onInternetStatusChange");
        delete thunder_client;
        thunder_client = nullptr;
    }
}
