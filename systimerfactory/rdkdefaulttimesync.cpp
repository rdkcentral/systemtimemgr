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
#include "rdkdefaulttimesync.h"
#include "irdklog.h"
#include <string.h>
#include <chrono>
#ifdef T2_EVENT_ENABLED
#include <telemetry_busmessage_sender.h>
#endif

#ifdef T2_EVENT_ENABLED
void t2CountNotify(const char *marker, int val) {
    t2_event_d(marker, val);
}

void t2ValNotify( const char *marker, const char *val )
{
    t2_event_s(marker, val);
}
#endif

using namespace std::chrono;
map<string, string> RdkDefaultTimeSync::tokenize(string const& s,string token)
{
	map<string, string> m;

	string::size_type key_pos = 0;
	string::size_type key_end;
	string::size_type val_pos;
	string::size_type val_end;

	while((key_end = s.find(token.c_str(), key_pos)) != std::string::npos)
	{
		if((val_pos = s.find_first_not_of(token.c_str(), key_end)) == std::string::npos)
			break;

		val_end = s.find('\n', val_pos);
		m.emplace(s.substr(key_pos, key_end - key_pos), s.substr(val_pos, val_end - val_pos));

		key_pos = val_end;
		if(key_pos != std::string::npos)
			++key_pos;
	}
	return m;
}

long long RdkDefaultTimeSync::buildtime()
{
	long long ver_time = 0;
	//fstream verfile("/version.txt",std::ios_base::in);
	ifstream verfile("/version.txt");
	if (verfile.is_open())
	{
		std::string str((std::istreambuf_iterator<char>(verfile)),
				std::istreambuf_iterator<char>());

		auto tokens = tokenize(str,"=");
		std::string timestr = tokens["BUILD_TIME"];
		timestr.erase(std::remove(timestr.begin(), timestr.end(), '"'), timestr.end());

		struct tm tm;
		if (strptime(timestr.c_str(),"%Y-%m-%d %H:%M:%S",&tm) != NULL)
		{
			ver_time = timegm(&tm);
		}

	}

	return ver_time;
}

long long RdkDefaultTimeSync::getTime()
{
	//Brief Algorithm:
	//1. Look at file(clockTime) located in path. By Default Path is /opt/secure which is securely mounted.
	//2. Extract time from file if file is present.
	//3. extract build time from /version.txt.
	//4. if clocktime < buildtime, return build time
	//5. else return clock time.
	//6. update Current Time with what we are returning.        

	 #ifdef T2_EVENT_ENABLED
         t2_init(const_cast<char*>("SysTimeMgr"));
         #endif

	long long clock_time = 0,ver_time = 0;
	fstream myfile(m_path.c_str(), std::ios_base::in);
	if (myfile.is_open())
	{
		myfile>>clock_time; 
	}

 
	ver_time = buildtime();
	if (clock_time > ver_time)
	{
		char timeStr[100] = {0};
		time_t safe_clock_time = static_cast<time_t>(clock_time); // Explicit conversion to time_t
                strftime(timeStr, sizeof(timeStr), "%A %c", localtime(&safe_clock_time)); // Pass time_t pointer
		RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:Returning Last Known Good Time, time = %s \n",__FUNCTION__,__LINE__,timeStr);
		char buffer[128];
                snprintf(buffer, 128, "Returning Last Known Good Time, time = %s", timeStr);
		t2ValNotify(const_cast<char*>("SYST_INFO_SYSLKG"),const_cast<char*>(buffer));
                 t2_event_s("SYST_INFO_SYSLKG",buffer);
		m_currentTime = clock_time;
		return clock_time;
	}
	m_currentTime = ver_time;
	char timeStr[100] = {0};
	time_t safe_ver_time = static_cast<time_t>(ver_time); // Explicit conversion to time_t
        strftime(timeStr, sizeof(timeStr), "%A %c", localtime(&safe_ver_time)); // Pass time_t pointer
	RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:Returning build time, Time = %s\n",__FUNCTION__,__LINE__,timeStr);
	char buffer[128];
        snprintf(buffer, 128, "Returning build time, Time = %s", timeStr);
         t2ValNotify(const_cast<char*>("SYST_INFO_SYSBUILD"),const_cast<char*>(buffer));
	return ver_time;
}

void  RdkDefaultTimeSync::updateTime(long long locTime)
{
	//Breif Algorithm:
	//1. if we are called with 0, extract time saved and increment by 10 mins.
	//2. if there is no time saved, use build time + 10 mins.
	//3. if invoked with  a non zero time, save the time as -is.

	long long updatetime = locTime;
	char timeStr[100] = {0};
	time_t localTime = static_cast<time_t>(locTime);
        strftime(timeStr, sizeof(timeStr), "%A %c", localtime(&localTime));
	RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:Updating Time. Converted Time: %s , Actual value passed:%lld, \n",__FUNCTION__,__LINE__,timeStr,locTime);
	if (updatetime == 0)
	{
		//Since Time is not secure yet, try to see if we can set the current time.
		updatetime = static_cast<long long>(time(NULL));
		struct timespec ts;
		if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
			auto duration = seconds{ts.tv_sec} + nanoseconds{ts.tv_nsec};
			updatetime = static_cast<long long>(system_clock::to_time_t(time_point<system_clock, seconds>(duration_cast<seconds>(duration))));
			RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]: Clock_gettime Call succeeded with REAL time.\n",__FUNCTION__,__LINE__);
		}

	}

	if (updatetime >= m_currentTime)
	{
		//Time can be advanced since time is greater than previous time.
		m_currentTime = updatetime;
	}
	else
	{
		//Increment by 10 minutes.
		m_currentTime += 10*60;
	}

	memset(timeStr,0,sizeof(timeStr));
	time_t currentTime = static_cast<time_t>(m_currentTime);
        strftime(timeStr, sizeof(timeStr), "%A %c", localtime(&currentTime));
	RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:Updating Time in file. Converted Time: %s , Actual value passed:%lld, \n",__FUNCTION__,__LINE__,timeStr,m_currentTime);

	ofstream timefile(m_path.c_str(),ios::out);
	if (timefile.is_open())
	{
		timefile<<updatetime<<"\n";
		timefile.close();
	}
}
