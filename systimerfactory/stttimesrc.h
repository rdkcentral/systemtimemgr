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
#ifndef _STTTIMESRC_H_
#define _STTTIMESRC_H_

#include "itimesrc.h"
#include <time.h>
#include "irdklog.h"

class SttTimeSrc : public ITimeSrc
{
	public:
		SttTimeSrc():ITimeSrc(){}

		bool isreference() { return true;}

		long long getTimeSec(){
			time_t timeinSec = time(NULL);
                        RDK_LOG(RDK_LOG_DEBUG,LOG_SYSTIME,"[%s:%d]:STT Time in seconds = %d \n",__FUNCTION__,__LINE__,timeinSec);

			return timeinSec;
		}

};

#endif// _STTTIMESRC_H_
