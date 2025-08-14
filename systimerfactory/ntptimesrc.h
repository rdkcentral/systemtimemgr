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
#ifndef _NTPTIMESRC_H_
#define _NTPTIMESRC_H_

#include "itimesrc.h"
#include <sys/timex.h>
#include "irdklog.h"

class NtpTimeSrc : public ITimeSrc
{
	public:
		NtpTimeSrc():ITimeSrc(){}

		bool isreference() { return true;}

		long long getTimeSec(){
			struct ntptimeval tVal; 
			ntp_gettime(&tVal);
			RDK_LOG(RDK_LOG_DEBUG,LOG_SYSTIME,"[%s:%d]:NTP Time Values, MaxValue = %ld, Time in Sec = %ld, Time in Microsec = %ld, Estimated Error = %ld, TAI = %ld \n",__FUNCTION__,__LINE__,tVal.maxerror,tVal.time.tv_sec,tVal.time.tv_usec,tVal.esterror,tVal.tai);

			return tVal.time.tv_sec;
		}

};

#endif// _NTPTIMESRC_H_
