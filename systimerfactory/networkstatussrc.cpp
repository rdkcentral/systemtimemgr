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

#ifdef WPEVGDRM_ENABLED
#include "core/SystemInfo.h"
#include "websocket/JSONRPCLink.h"
using namespace WPEFramework;
#endif //WPEVGDRM_ENABLED

using namespace std;
using namespace jsonrpc;

#define NETWORK_RPC_TIMEOUT 5000
#ifdef WPEVGDRM_ENABLED
namespace {
const unsigned int ACTIVATION_RETRY_COUNT = 5;
const unsigned int ACTIVATION_RETRY_INTERVAL_MS = 1000;

const char* ACTIVATION_METHOD = "Controller.1.activate";
const char* NETWORK_MANAGER_CALLSIGN = "org.rdk.NetworkManager.1";
const char* NETWORK_MANAGER_PLUGIN = "org.rdk.NetworkManager.1";
const char* INTERNET_EVENT_NAME = "onInternetStatusChange";
const unsigned int EVENT_SUBSCRIPTION_TIMEOUT_SEC = 5000;
int32_t thunder_ret = Core::ERROR_NONE;

void internetStatusChanged(const JsonObject& params)
{
   string internetStatus;
  RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]- CHRONY: Internet status change notification received. status = %s\n",__FUNCTION__,__LINE__,internetStatus.c_str());
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
   if (normalizedStatus == "fully_connected") {
     int ret = v_secure_system("/usr/sbin/chronyc burst 3/4");
      if (ret != 0) {
                RDK_LOG(RDK_LOG_WARN,LOG_SYSTIME,"[%s:%d]:chronyc burst failed with code %d\n",__FUNCTION__,__LINE__, ret);
      } else {
      RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:chronyc burst triggered for connected internet status.\n",__FUNCTION__,__LINE__);
      }
   }
}
} // namespace
#endif//WPEVGDRM_ENABLED

void NetworkStatusSrc::subscribeInternetStatusEvent()
{
#ifdef WPEVGDRM_ENABLED
   if (m_networkeventsubscribed) {
      return;
   }

   HttpClient client("http://127.0.0.1:9998/jsonrpc");
   client.AddHeader("content-type","text/plain");
   Client c(client,JSONRPC_CLIENT_V2);
   Json::Value activeParams,result;

   activeParams["id"] = "1";
   activeParams["callsign"] = NETWORK_MANAGER_CALLSIGN;
/*
   unsigned int retryCounter = 0;
   while ((!m_pluginactivated) && (retryCounter < ACTIVATION_RETRY_COUNT)) {
      try {
         result = c.CallMethod(ACTIVATION_METHOD,activeParams);
         RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:JSON Data Received = %s \n",__FUNCTION__,__LINE__,result.toStyledString().c_str());
         m_pluginactivated = true;
      }
      catch (JsonRpcException& e) {
         ++retryCounter;
         RDK_LOG(RDK_LOG_ERROR,LOG_SYSTIME,"[%s:%d]:NetworkManager plugin activation failed, attempt %u/%u. %s\n",__FUNCTION__,__LINE__,retryCounter,ACTIVATION_RETRY_COUNT,e.what());
         std::this_thread::sleep_for(std::chrono::milliseconds(ACTIVATION_RETRY_INTERVAL_MS));
      }
   }

   if (!m_pluginactivated) {
      RDK_LOG(RDK_LOG_ERROR,LOG_SYSTIME,"[%s:%d]:Failed to activate NetworkManager plugin, skipping subscription.\n",__FUNCTION__,__LINE__);
      return;
   }
*/
   //WPEFramework::JSONRPC::LinkType<WPEFramework::Core::JSON::IElement> *thunder_client = nullptr;
   Core::SystemInfo::SetEnvironment("THUNDER_ACCESS","127.0.0.1:9998");
   //thunder_client = new WPEFramework::JSONRPC::LinkType<Core::JSON::IElement>(NETWORK_MANAGER_CALLSIGN, "", false);
   WPEFramework::JSONRPC::LinkType<Core::JSON::IElement> wpeClient(NETWORK_MANAGER_CALLSIGN);
  //wpeClient.Subscribe<JsonObject>(EVENT_SUBSCRIPTION_TIMEOUT_SEC,_T("onInternetStatusChange"),std::bind(internetStatusChanged,std::placeholders::_1)
   thunder_ret = wpeClient.Subscribe<JsonObject>(5000,_T("onInternetStatusChange"),&internetStatusChanged);
   if (thunder_ret !=  Core::ERROR_NONE) {
      RDK_LOG(RDK_LOG_ERROR,LOG_SYSTIME,"[%s:%d]:Failed to register for onInternetStatusChange (%d) .\n",__FUNCTION__,__LINE__,thunder_ret);
   } else {
      RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:Successfully registered for onInternetStatusChange.\n",__FUNCTION__,__LINE__);
      m_networkeventsubscribed = true;
   }

      // ... after m_networkeventsubscribed = true;
   JsonObject inParam, outParamV4, outParamV6;

   /* Check Internet status for IPv4 */
   inParam.Set(_T("ipversion"), string("IPv4"));

   thunder_ret = wpeClient.Invoke<JsonObject, JsonObject>(NETWORK_RPC_TIMEOUT, _T("isConnectedToInternet"), inParam, outParamV4);
   
   if ( Core::ERROR_NONE == thunder_ret) {
       bool v4success = outParamV4.HasLabel("success") ? outParamV4["success"].Boolean() : false;
       bool v4connected = outParamV4.HasLabel("connectedToInternet") ? outParamV4["connectedToInternet"].Boolean() : false;
       RDK_LOG(RDK_LOG_INFO, LOG_SYSTIME,
           "[%s:%d]: IPv4 status - success: %d, connectedToInternet: %d\n",
           __FUNCTION__, __LINE__, v4success, v4connected);
   } else {
       RDK_LOG(RDK_LOG_ERROR, LOG_SYSTIME,
           "[%s:%d]: Failed to invoke isConnectedToInternet for IPv4 (%d).\n",
           __FUNCTION__, __LINE__, thunder_ret );
   }

   /* Check Internet status for IPv6 */
   inParam.Set(_T("ipversion"), string("IPv6"));

   thunder_ret = wpeClient.Invoke<JsonObject, JsonObject>(NETWORK_RPC_TIMEOUT, _T("isConnectedToInternet"), inParam, outParamV6);
   if (Core::ERROR_NONE == thunder_ret ) {
       bool v6success = outParamV6.HasLabel("success") ? outParamV6["success"].Boolean() : false;
       bool v6connected = outParamV6.HasLabel("connectedToInternet") ? outParamV6["connectedToInternet"].Boolean() : false;
       RDK_LOG(RDK_LOG_INFO, LOG_SYSTIME,
           "[%s:%d]: IPv6 status - success: %d, connectedToInternet: %d\n",
           __FUNCTION__, __LINE__, v6success, v6connected);
   } else {
       RDK_LOG(RDK_LOG_ERROR, LOG_SYSTIME,
           "[%s:%d]: Failed to invoke isConnectedToInternet for IPv6. (%d) \n",
           __FUNCTION__, __LINE__, thunder_ret);
   }
#else
   m_pluginactivated = true;
   m_networkeventsubscribed = true;
#endif //WPEVGDRM_ENABLED
}
