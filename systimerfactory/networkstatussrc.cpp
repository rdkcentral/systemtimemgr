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

#include "networkstatussrc.h"
#include "irdklog.h"
#include "secure_wrapper.h"


#include "core/SystemInfo.h"
#include "websocket/JSONRPCLink.h"
using namespace WPEFramework;


using namespace std;
static std::string lastStatus;

const unsigned int ACTIVATION_RETRY_INTERVAL_MS = 1000;

const char* NETWORK_MANAGER_CALLSIGN = "org.rdk.NetworkManager";
const char* INTERNET_EVENT_NAME = "onInternetStatusChange";

static WPEFramework::JSONRPC::LinkType<WPEFramework::Core::JSON::IElement>* thunder_client = nullptr;
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

   RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]: CHRONY: Internet status change notification received. status = %s\n", __FUNCTION__,__LINE__,normalizedStatus.c_str());
   if (normalizedStatus == "fully_connected" && normalizedStatus != lastStatus) {
      RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]: CHRONY: Received First Internet status change event. status = %s\n", __FUNCTION__,__LINE__,normalizedStatus.c_str());
     int ret = v_secure_system("/usr/sbin/chronyc burst 3/4");
      if (ret != 0) {
                RDK_LOG(RDK_LOG_WARN,LOG_SYSTIME,"[%s:%d]: CHRONY: chronyc burst failed with code %d\n",__FUNCTION__,__LINE__, ret);
      } else {
      RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]: CHRONY: chronyc burst triggered for connected internet status.\n",__FUNCTION__,__LINE__);
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
            thunder_client = new WPEFramework::JSONRPC::LinkType<Core::JSON::IElement>(NETWORK_MANAGER_CALLSIGN, "", false);

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
