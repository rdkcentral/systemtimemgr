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

#ifndef _ITIMESYNC_H_
#define _ITIMESYNC_H_

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

class ITimeSync
{
	public:
		ITimeSync() {}
		virtual ~ITimeSync(){}
		virtual void  updateTime(long long locTime) = 0;
		virtual long long getTime() = 0;
                virtual RdkDefaultTimeSync::TimeSource getTimeSource() const = 0;
};

#endif //_ITIMESYNC_H_
