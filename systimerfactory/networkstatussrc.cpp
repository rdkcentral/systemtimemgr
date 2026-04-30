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
    FILE *fp = v_secure_popen("r", "/usr/sbin/chronyc tracking");
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
          * this boot.  Three sub-cases:
          *
          *  A) chronyd not yet started: internet is already up; chronyd will
          *     reach its servers immediately via iburst on startup.  Nothing to do.
          *
          *  B) chronyd running, sources visible in 'chronyc sources' (any state,
          *     including '^?'): iburst is in progress or DNS resolved and polling
          *     has started.  Let chronyd finish naturally; 'makestep 1.0 4' in
          *     chrony.conf handles the initial step correction.
          *
          *  C) chronyd running, NO source entries in 'chronyc sources': sources
          *     are completely offline (device booted without internet and DNS may
          *     not have resolved yet).  Next natural poll could be many minutes
          *     away.  Call 'chronyc online' to mark sources reachable and trigger
          *     iburst immediately.
          *
          * Discriminators:
          *   1. 'systemctl is-active chronyd'  — non-zero → Case A.
          *   2. 'chronyc sources | grep -qE "^\^"' — matches ANY server line
          *      ('^?', '^*', '^+', '^-', '^x', '^~') meaning chronyd has at
          *      least one source entry it is actively polling. Exit 0 → Case B.
          *      Exit non-zero → Case C. */
         int chronyActive = v_secure_system("/bin/systemctl is-active --quiet chronyd.service");
         if (chronyActive != 0) {
            /* Case A */
            RDK_LOG(RDK_LOG_INFO, LOG_SYSTIME,
                    "[%s:%d]: CHRONY: First sync pending — chronyd not yet started."
                    " Internet is up; will sync via iburst on startup.\n",
                    __FUNCTION__, __LINE__);
         } else {
            /* chronyd is running — check whether any source entry exists */
            int srcPresent = v_secure_system("/usr/sbin/chronyc sources | grep -qE '^\\^'");
            if (srcPresent == 0) {
               /* Case B: at least one source entry visible (iburst running or
                * polling started).  Let chronyd complete the sync on its own. */
               RDK_LOG(RDK_LOG_INFO, LOG_SYSTIME,
                       "[%s:%d]: CHRONY: First sync pending — source(s) present in"
                       " chronyc sources (iburst/polling in progress). No action needed.\n",
                       __FUNCTION__, __LINE__);
            } else {
               /* Case C: no source entries — sources are offline; next poll
                * could be delayed by many minutes without intervention. */
               RDK_LOG(RDK_LOG_INFO, LOG_SYSTIME,
                       "[%s:%d]: CHRONY: First sync pending — no sources in chronyc sources."
                       " Device likely booted without internet. Calling chronyc online.\n",
                       __FUNCTION__, __LINE__);
               int ret = v_secure_system("/usr/sbin/chronyc online");
               if (ret == 0) {
                  RDK_LOG(RDK_LOG_INFO, LOG_SYSTIME,
                          "[%s:%d]: CHRONY: chronyc online succeeded."
                          " iburst will fire automatically.\n",
                          __FUNCTION__, __LINE__);
               } else {
                  RDK_LOG(RDK_LOG_ERROR, LOG_SYSTIME,
                          "[%s:%d]: CHRONY: chronyc online failed (ret=%d)."
                          " chronyd will sync on its own schedule.\n",
                          __FUNCTION__, __LINE__, ret);
               }
            }
         }
      } else {
         /* Check whether chrony already has a selectable (selected '*' or combined '+') source.
          * grep -qE '^\^[*+]' matches lines chronyc sources prints for selected/combined peers.
          * Exit 0  => at least one selectable source exists — safe to makestep immediately.
          * Exit !0 => no selectable source; burst + waitsync required first. */
         int srcRet = v_secure_system("/usr/sbin/chronyc sources | grep -qE '^\\^[*+]'");
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
