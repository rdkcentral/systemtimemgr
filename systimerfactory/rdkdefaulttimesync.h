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
#ifndef _RDKDEFAULTTIMESYNC_H_
#define _RDKDEFAULTTIMESYNC_H_

#include <unistd.h>
#include "itimesync.h"
#include <fcntl.h>
#include <fstream>
#include <string>
#include <map>

#ifdef T2_EVENT_ENABLED
#include <telemetry_busmessage_sender.h>
#endif

#ifdef T2_EVENT_ENABLED
void t2CountNotify(char *marker, int val);
void t2ValNotify(char *marker, char *val);
#endif

using namespace std;
class RdkDefaultTimeSync: public ITimeSync
{
	private:
		string m_path;
                long long m_currentTime;
                map<string, string> tokenize(string const& s,string token);
                long long buildtime();
	public: 
		RdkDefaultTimeSync(string path = "/opt/secure/clock.txt"):ITimeSync(),m_path(std::move(path)),m_currentTime(0) {}
		~RdkDefaultTimeSync(){}
		virtual void  updateTime(long long locTime); 
		virtual long long getTime(); 
};

#endif// _RDKDEFAULTTIMESYNC_H_
