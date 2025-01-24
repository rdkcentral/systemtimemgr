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

#include "drmtimersrc.h"
#include "irdklog.h"

using namespace jsonrpc;



long long DrmTimeSrc::getTimeSec()
{
   HttpClient client("http://127.0.0.1:9998/jsonrpc");
   client.AddHeader("content-type","text/plain");

   Client c(client,JSONRPC_CLIENT_V2);
   Json::Value activeParams,secureTimeParams,result;
   string activemethod("DRMActivationService.1.GetActivationStatus");
   string securetime("DRMActivationService.1.GetSecureTime");

   activeParams["id"] = "1";

   secureTimeParams["id"] = "2";

   try {
      //1. Check activatetion status
      result = c.CallMethod(activemethod,activeParams);
      if (result["activationstatus"].asString() == "Active") {
         result = c.CallMethod(securetime,secureTimeParams);

	 RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:JSON Data Received = %s \n",__FUNCTION__,__LINE__,result.toStyledString().c_str());

	 if (result["success"].asBool())
	 {
	    notifypathAvailable(m_path);
	    return  stoll(result["securetime"].asString());
	 }
	 else {
	    RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:Success Should never return false, since we are checking Activation status before we extract time.\n",__FUNCTION__,__LINE__);
	 }
      }
      else {
	 RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:Activation status returned as Not Active. JSON Data received = %s \n",__FUNCTION__,__LINE__,result.toStyledString().c_str());
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
