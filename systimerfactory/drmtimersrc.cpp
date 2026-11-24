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
#include <exception>
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

	// Validate the JSON-RPC envelope before reading fields with typed accessors.
	
	 if (!result.isObject() || !result.isMember("success") || !result["success"].isBool())
	 {
	    RDK_LOG(RDK_LOG_ERROR,LOG_SYSTIME,"[%s:%d]:Malformed DRM reply (success). \n",__FUNCTION__,__LINE__);
	    return 0;
	 }
	 else if (result["success"].asBool())
	 {
	    if (!result.isMember("securetime") || !result["securetime"].isString())
	    {
	       RDK_LOG(RDK_LOG_ERROR,LOG_SYSTIME,"[%s:%d]:Malformed DRM reply (securetime). \n",__FUNCTION__,__LINE__);
	       return 0;
	    }

	    // Accept only a positive decimal timestamp; malformed values must not
	    // produce a secure-time marker or be used to advance the system clock.
	
	    const string secureTimeValue = result["securetime"].asString();
	    if (secureTimeValue.empty() ||
	        secureTimeValue.find_first_not_of("0123456789") != string::npos)
	    {
	       RDK_LOG(RDK_LOG_ERROR,LOG_SYSTIME,"[%s:%d]:Invalid DRM secure timestamp. \n",__FUNCTION__,__LINE__);
	       return 0;
	    }

	    const long long secureTime = stoll(secureTimeValue);
	    if (secureTime <= 0)
	    {
	       RDK_LOG(RDK_LOG_ERROR,LOG_SYSTIME,"[%s:%d]:DRM returned a non-positive secure timestamp. \n",__FUNCTION__,__LINE__);
	       return 0;
	    }

	    notifypathAvailable(m_path);
	    return secureTime;
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

   // Includes timestamp conversion failures (for example, values too large
   // for long long); report DRM time as unavailable rather than propagating.
	   
   catch (const std::exception& e) {
      RDK_LOG(RDK_LOG_ERROR,LOG_SYSTIME,"[%s:%d]:Exception while processing DRM reply. %s\n",__FUNCTION__,__LINE__,e.what());
      return 0;
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
