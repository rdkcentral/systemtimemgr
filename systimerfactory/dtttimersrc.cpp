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
#include <memory>
#include <fstream>
#include <fcntl.h> 
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <jsonrpccpp/client.h>
#include <jsonrpccpp/client/connectors/httpclient.h>

#include "dtttimersrc.h"
#include "irdklog.h"
#ifdef WPEVGDRM_ENABLED
#include "core/SystemInfo.h"
#include "websocket/JSONRPCLink.h"
using namespace WPEFramework;
#endif //WPEVGDRM_ENABLED

using namespace jsonrpc;


#ifdef WPEVGDRM_ENABLED
void dttTimeavailable(const JsonObject& params)
{
   RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:DTTTime Notification Received. Value of UTC Time =  %lld \n",__FUNCTION__,__LINE__,params["timeUtc"].Number());

}
#endif//WPEVGDRM_ENABLED

long long DttTimeSrc::getTimeSec()
{
   HttpClient client("http://127.0.0.1:9998/jsonrpc");
   client.AddHeader("content-type","text/plain");

   Client c(client,JSONRPC_CLIENT_V2);
   Json::Value activeParams,dttTimeParams,result;
   string activemethod("Controller.1.activate");
   string dtttimeMethod("org.rdk.MediaSystem.1.getBroadcastTimeInfo");

   activeParams["id"] = "1";
   activeParams["callsign"] = "org.rdk.MediaSystem";

   dttTimeParams["id"] = "2";

   try {
          //1. Check activatetion status
          if (!m_pluginactivated) {
             result = c.CallMethod(activemethod,activeParams);
	     RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:JSON Data Received = %s \n",__FUNCTION__,__LINE__,result.toStyledString().c_str());
             m_pluginactivated = true;
          }

	  if ((m_pluginactivated) && (!m_dtteventsubscribed)) {
#ifdef WPEVGDRM_ENABLED
             Core::SystemInfo::SetEnvironment("THUNDER_ACCESS","127.0.0.1:9998");
	     WPEFramework::JSONRPC::LinkType<Core::JSON::IElement> wpeclient("org.rdk.MediaSystem.1");
	     if (wpeclient.Subscribe<JsonObject>(10,"onBroadcastTimeAvailable",std::bind(dttTimeavailable,std::placeholders::_1)) != 0) {
		     RDK_LOG(RDK_LOG_ERROR,LOG_SYSTIME,"[%s:%d]:Failed to register for onBroadcastTimeAvailable. \n",__FUNCTION__,__LINE__);
	     }
	     else {
		     RDK_LOG(RDK_LOG_ERROR,LOG_SYSTIME,"[%s:%d]:Successfully registered for onBroadcastTimeAvailable. \n",__FUNCTION__,__LINE__);
		     m_dtteventsubscribed = true;
	     }
#else
	     RDK_LOG(RDK_LOG_ERROR,LOG_SYSTIME,"[%s:%d]:onBroadcastTimeAvailable is not available. \n",__FUNCTION__,__LINE__);
	     m_dtteventsubscribed = true;
#endif //WPEVGDRM_ENABLED
	  }

          result = c.CallMethod(dtttimeMethod,dttTimeParams);
          RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:JSON Data Received = %s \n",__FUNCTION__,__LINE__,result.toStyledString().c_str());
          if (result["success"].asBool()) {
             Json::Value infoNode = result["info"];
	     notifypathAvailable(m_path);
	     return  stoll(infoNode["timeUtc"].asString());
          }
	  else {
	     RDK_LOG(RDK_LOG_ERROR,LOG_SYSTIME,"[%s:%d]:DTT Time Not Available. \n",__FUNCTION__,__LINE__);
	  }
   }
   catch (JsonRpcException& e) {
      RDK_LOG(RDK_LOG_ERROR,LOG_SYSTIME,"[%s:%d]:Exception Received from JSON Rpc Call. %s \n",__FUNCTION__,__LINE__,e.what());
   }

   // We will return the current time for now.
#ifdef __LOCAL_TEST_
   notifypathAvailable(m_path);
   return  time(NULL);
#else
   //We will have to return 0 since there is no DRM time available.
   return 0;
#endif //__LOCAL_TEST_
}
