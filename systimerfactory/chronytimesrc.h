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
#ifndef _CHRONYTIMESRC_H_
#define _CHRONYTIMESRC_H_

#include "itimesrc.h"
#include "irdklog.h"
#include <time.h>

class ChronyTimeSrc : public ITimeSrc
{
	public:
		ChronyTimeSrc():ITimeSrc(){}

		bool isreference() { return true;}

		long long getTimeSec(){
			// NOTE: This is a placeholder implementation
			// In production, this should call the actual libchronyctl API
			// to communicate with chronyd daemon and get synchronized time.
			// The actual implementation would use functions from libchronyctl.so
			// Example: chronyctl_get_time() or similar API
			
			struct timespec ts;
			// Temporary fallback using clock_gettime
			// Replace this with actual libchronyctl API call when available
			if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
				RDK_LOG(RDK_LOG_DEBUG,LOG_SYSTIME,"[%s:%d]:Chrony Time Values, Time in Sec = %ld, Time in Nanosec = %ld\n",__FUNCTION__,__LINE__,ts.tv_sec,ts.tv_nsec);
				return ts.tv_sec;
			} else {
				RDK_LOG(RDK_LOG_ERROR,LOG_SYSTIME,"[%s:%d]:Failed to get chrony time\n",__FUNCTION__,__LINE__);
				return 0;
			}
		}

};

#endif// _CHRONYTIMESRC_H_
