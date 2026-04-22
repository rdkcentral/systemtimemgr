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
#include <cstdlib>
#include <cctype>
#include <jsonrpccpp/client.h>
#include <jsonrpccpp/client/connectors/httpclient.h>

#include "networkstatussrc.h"
#include "irdklog.h"
#include "secure_wrapper.h"


#include "core/SystemInfo.h"
#include "websocket/JSONRPCLink.h"
using namespace WPEFramework;


using namespace std;
using namespace jsonrpc;
static std::string lastStatus;

#define NETWORK_RPC_TIMEOUT 5000

const unsigned int ACTIVATION_RETRY_COUNT = 5;
const unsigned int ACTIVATION_RETRY_INTERVAL_MS = 1000;

const char* ACTIVATION_METHOD = "Controller.1.activate";
const char* NETWORK_MANAGER_CALLSIGN = "org.rdk.NetworkManager";
const char* NETWORK_MANAGER_PLUGIN = "org.rdk.NetworkManager.1";
const char* INTERNET_EVENT_NAME = "onInternetStatusChange";
const unsigned int EVENT_SUBSCRIPTION_TIMEOUT_SEC = 5000;
int32_t thunder_ret = Core::ERROR_NONE;

// Definition & initialization outside class
WPEFramework::JSONRPC::LinkType<WPEFramework::Core::JSON::IElement>* NetworkStatusSrc::controller = nullptr;
WPEFramework::JSONRPC::LinkType<WPEFramework::Core::JSON::IElement>* NetworkStatusSrc::thunder_client = nullptr;
bool NetworkStatusSrc::m_networkeventsubscribed = false;


void handle_internetStatusChange(const JsonObject& params)
{
   string internetStatus;
  RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]- CHRONY TBR: Internet status change notification received. status = %s\n",__FUNCTION__,__LINE__,internetStatus.c_str());
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

   RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]: CHRONY: Internet status change notification received. status = %s\n",__FUNCTION__,__LINE__,normalizedStatus.c_str());
   if (normalizedStatus == "fully_connected" && normalizedStatus != lastStatus) {
     int ret = v_secure_system("/usr/sbin/chronyc burst 3/4");
      if (ret != 0) {
                RDK_LOG(RDK_LOG_WARN,LOG_SYSTIME,"[%s:%d]:chronyc burst failed with code %d\n",__FUNCTION__,__LINE__, ret);
      } else {
      RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:chronyc burst triggered for connected internet status.\n",__FUNCTION__,__LINE__);
      }
   } 
   lastStatus = normalizedStatus;
}

void NetworkStatusSrc::subscribeToInternetEvent()
{
    if (m_networkeventsubscribed) return;
    if (!thunder_client)
        thunder_client = new WPEFramework::JSONRPC::LinkType<Core::JSON::IElement>(NETWORK_MANAGER_CALLSIGN, "", false);

    if (thunder_client) {
        thunder_ret = thunder_client->Subscribe<JsonObject>(5000, "onInternetStatusChange", &handle_internetStatusChange);
        if (thunder_ret == Core::ERROR_NONE) {
            RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]: Successfully subscribed to onInternetStatusChange\n", __FUNCTION__,__LINE__);
            m_networkeventsubscribed = true;
        } else {
            RDK_LOG(RDK_LOG_ERROR,LOG_SYSTIME,"[%s:%d]: Failed to subscribe to onInternetStatusChange (%d)\n",__FUNCTION__,__LINE__,thunder_ret);
            delete thunder_client; thunder_client = nullptr;
        }
    }
}

void NetworkStatusSrc::unsubscribeFromInternetEvent()
{
    if (!m_networkeventsubscribed || !thunder_client) return;
    thunder_client->Unsubscribe(5000, "onInternetStatusChange");
    delete thunder_client;
    thunder_client = nullptr;
    m_networkeventsubscribed = false;
}

void NetworkStatusSrc::plugin_statechange(const JsonObject& parameters)
{
    if (!parameters.HasLabel("callsign") || !parameters.HasLabel("state"))
        return;

    std::string callsign = parameters["callsign"].String();
    std::string state = parameters["state"].String();

    if (callsign == NETWORK_MANAGER_CALLSIGN) {
        if (state == "Activated") {
            RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]: NetworkManager Activated\n", __FUNCTION__,__LINE__);
            NetworkStatusSrc::subscribeToInternetEvent();
        } else if (state == "Deactivated") {
            RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]: NetworkManager Deactivated\n", __FUNCTION__,__LINE__);
            NetworkStatusSrc::unsubscribeFromInternetEvent();
        }
    }
}

void NetworkStatusSrc::subscribeInternetStatusEvent()
{

    Core::SystemInfo::SetEnvironment("THUNDER_ACCESS", "127.0.0.1:9998");

    // Subscribe to plugin statechange once per lifetime
    if (!controller) {
        controller = new WPEFramework::JSONRPC::LinkType<Core::JSON::IElement>("", "", false);
        if (controller) {
            controller->Subscribe<JsonObject>(5000, "statechange", &NetworkStatusSrc::plugin_statechange);
            RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]: Subscribed to plugin statechange event\n", __FUNCTION__,__LINE__);
        }
    }
}

NetworkStatusSrc::~NetworkStatusSrc()
{

    if (controller) {
        controller->Unsubscribe(5000, "statechange");
        delete controller;
        controller = nullptr;
    }
    unsubscribeFromInternetEvent();
}
