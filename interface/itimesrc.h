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

#ifndef _ITIMESRC_H_
#define _ITIMESRC_H_

#include <map>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>
#include <list>
#include <sys/ioctl.h>
#include <vector>
#include <algorithm>
#include "irdklog.h"
#include <fcntl.h> 
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>


using namespace std;
class ITimeSrc
{
	protected:
		void notifypathAvailable(string path)
		{
			int fd = open(path.c_str(), O_WRONLY|O_CREAT|O_CLOEXEC|O_NOCTTY,0644 );
			RDK_LOG(RDK_LOG_DEBUG,LOG_SYSTIME,"[%s:%d]:File Name %s, opened for writing \n",__FUNCTION__,__LINE__,path.c_str());
			if (fd >= 0)
			{
				RDK_LOG(RDK_LOG_DEBUG,LOG_SYSTIME,"[%s:%d]:File Name = %s is opened for writing \n",__FUNCTION__,__LINE__,path.c_str());
				struct timespec ts[2];
				timespec_get(&ts[0], TIME_UTC);
				ts[1] = ts[0];
				if (futimens(fd, ts) != 0)
				{
					RDK_LOG(RDK_LOG_ERROR,LOG_SYSTIME,"[%s:%d]:Failed to Set access times for %s \n",__FUNCTION__,__LINE__,path.c_str());
				}
				close(fd);
			}
		}
	public:
		ITimeSrc(){}
		virtual bool isreference() = 0;
		virtual long long getTimeSec() = 0;
		virtual bool checkTime() { return getTimeSec() == 0;}
		virtual bool isclockProvider() { return false;}
};


#endif// _ITIMESRC_H_
