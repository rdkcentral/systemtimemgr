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
#ifndef _TEETIMESYNC_H_
#define _TEETIMESYNC_H_

#include <unistd.h>
#include "itimesrc.h"
#include <fcntl.h>
#include <fstream>
#include "irdklog.h"
#include <ctime>
#ifdef ENABLE_IARM
#include "libIBus.h"
#endif //ENABLE_IARM
extern "C"
{
#ifdef ENABLE_IARM
#include "mfr/mfrMgr.h"
#else
#include "mfr/mfrTypes.h"
#endif //ENABLE_IARM
}

using namespace std;
class TeeTimeSync: public ITimeSync
{
	public: 
		TeeTimeSync():ITimeSync() {}
		~TeeTimeSync(){}
		void  updateTime(long long locTime) {
			char timeStr[100] = {0};
			strftime(timeStr, sizeof(timeStr), "%A %c", localtime((time_t*)&locTime));
			RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:Updating Time. Converted Time: %s , Actual value passed:%lld, \n",__FUNCTION__,__LINE__,timeStr,locTime);
#ifdef ENABLE_IARM
			IARM_Bus_MFRLib_SecureTime_Param param = (IARM_Bus_MFRLib_SecureTime_Param)locTime;
			IARM_Result_t ret;
			ret = IARM_Bus_Call(IARM_BUS_MFRLIB_NAME,IARM_BUS_MFRLIB_API_SetSecureTime,&param ,sizeof(IARM_Bus_MFRLib_SecureTime_Param) );
			if(ret != IARM_RESULT_SUCCESS)
			{
				RDK_LOG(RDK_LOG_ERROR,LOG_SYSTIME,"[%s:%d]:Failed to set time in MFR LIB \n",__FUNCTION__,__LINE__);
				getTime();
				return;
			}
#else
			if (mfrSetSecureTime((uint32_t*)&locTime) != mfrERR_NONE)
			{
				RDK_LOG(RDK_LOG_ERROR,LOG_SYSTIME,"[%s:%d]:Failed to set time in MFR LIB \n",__FUNCTION__,__LINE__);
				//print the current time in TEE.
				getTime();
				return;
			}
#endif //ENABLE_IARM
			RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:Successfully Set Time in MFR LIB \n",__FUNCTION__,__LINE__);

		}

		long long getTime() {
			long long ret = 0;
#ifdef ENABLE_IARM
			IARM_Bus_MFRLib_SecureTime_Param param;
			IARM_Result_t retCode;
			retCode = IARM_Bus_Call(IARM_BUS_MFRLIB_NAME,IARM_BUS_MFRLIB_API_GetSecureTime,&param ,sizeof(IARM_Bus_MFRLib_SecureTime_Param) );
			if(retCode != IARM_RESULT_SUCCESS)
			{
				RDK_LOG(RDK_LOG_ERROR,LOG_SYSTIME,"[%s:%d]:Failed to get time from MFR LIB \n",__FUNCTION__,__LINE__);
			}
			else
			{
				ret = (long long)param;
				RDK_LOG(RDK_LOG_ERROR,LOG_SYSTIME,"[%s:%d]:Successfully obtained time from MFR LIB. Time Returned = %lld \n",__FUNCTION__,__LINE__,ret);

			}

#else
			if (mfrGetSecureTime((uint32_t*)&ret) != mfrERR_NONE)
			{
				RDK_LOG(RDK_LOG_ERROR,LOG_SYSTIME,"[%s:%d]:Failed to get time from MFR LIB \n",__FUNCTION__,__LINE__);
			}
#endif //ENABLE_IARM
			char timeStr[100] = {0};
			strftime(timeStr, sizeof(timeStr), "%A %c", localtime((time_t*)&ret));
			RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:TIME Returning = %lld, Converted Current Time in TEE: %s \n",__FUNCTION__,__LINE__,ret,timeStr);
			return ret;
		}

        TimeSource getTimeSource() const override {
        return TIME_SOURCE_NONE; 
       }

};

#endif// _TEETIMESYNC_H_
