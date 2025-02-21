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

#include "systimemgr.h"
#include <stdlib.h>
#include <vector>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "timerfactory.h"
#include <memory>
#include <sys/inotify.h>
#include <limits.h>
#include "irdklog.h"
#include "itimermsg.h"
#include <chrono>
#include "secure_wrapper.h"
using namespace std::chrono;


SysTimeMgr* SysTimeMgr::pInstance = NULL;
recursive_mutex SysTimeMgr::g_state_mutex;
mutex SysTimeMgr::g_instance_mutex;

SysTimeMgr* SysTimeMgr::get_instance()
{
    if(!pInstance)
    {
        std::lock_guard<std::mutex> guard(g_instance_mutex);
        if(!pInstance)
        {
           pInstance = new SysTimeMgr;
	   RDK_LOG(RDK_LOG_DEBUG,LOG_SYSTIME,"[%s:%d]:Created New Instance \n",__FUNCTION__,__LINE__);
        }
    }

    return pInstance;
}

SysTimeMgr::SysTimeMgr (string cfgfile):m_state(eSYSMGR_STATE_INIT),
	                  m_event(eSYSMGR_EVENT_UNKNOWN),
			  m_timerInterval(600000),
			  m_timequality(eTIMEQUALILTY_UNKNOWN),
			  m_timersrc("Last Known"),
                          m_publish(NULL),
			  m_subscriber(NULL),
			  m_cfgfile(cfgfile)
{
}

SysTimeMgr::~SysTimeMgr()
{
}
void SysTimeMgr::initialize()
{
    std::lock_guard<std::recursive_mutex> guard(g_state_mutex);

    //Create Timer Src and Syncs.
    ifstream cfgFile(m_cfgfile.c_str());

    if (cfgFile.is_open())
    {
	    string category,type,objargs;
	    while(cfgFile>>category>>type>>objargs)
	    {
	            RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:Initializing Src and Syncs. Category = %s, Type = %s, Args = %s.\n",__FUNCTION__,__LINE__,category.c_str(),type.c_str(),objargs.c_str());
		    if (category == "timesrc")
		    {
			    m_timerSrc.push_back(createTimeSrc(type,m_directory + objargs));
		    }
		    else if (category == "timesync")
		    {
			    m_timerSync.push_back(createTimeSync(type,m_directory + objargs));
		    }
	    }
	    cfgFile.close();
    }
    else
    {
	    RDK_LOG(RDK_LOG_ERROR,LOG_SYSTIME,"[%s:%d]:Failed to open Config file: %s , will run in degraded mode.\n",__FUNCTION__,__LINE__,m_cfgfile.c_str());
    }

    //m_timerSrc.push_back(createTimeSrc("regular","/tmp/clock.txt"));
    //m_timerSync.push_back(createTimeSync("test","/tmp/clock1.txt"));

    m_publish = createPublish("iarm",IARM_BUS_SYSTIME_MGR_NAME);
    m_subscriber = createSubscriber("iarm",IARM_BUS_SYSTIME_MGR_NAME);

    m_subscriber->subscribe(TIMER_STATUS_MSG,SysTimeMgr::getTimeStatus);
    m_subscriber->subscribe(POWER_CHANGE_MSG,SysTimeMgr::powerhandler);

    //Initialize Path Event Map
    m_pathEventMap.insert(pair<string,sysTimeMgrEvent>("ntp",eSYSMGR_EVENT_NTP_AVAILABLE));
    //Keeping the NTP available event for stt as well. Source is different but no need to have separate event.
    m_pathEventMap.insert(pair<string,sysTimeMgrEvent>("stt",eSYSMGR_EVENT_NTP_AVAILABLE));
    m_pathEventMap.insert(pair<string,sysTimeMgrEvent>("drm",eSYSMGR_EVENT_SECURE_TIME_AVAILABLE));
    m_pathEventMap.insert(pair<string,sysTimeMgrEvent>("dtt",eSYSMGR_EVENT_DTT_TIME_AVAILABLE));

    //Initialize time from the sync.
    setInitialTime();
    //Initialize state machine.
    stateMachine[eSYSMGR_STATE_RUNNING][eSYSMGR_EVENT_TIMER_EXPIRY] = &SysTimeMgr::timerExpiry;
    stateMachine[eSYSMGR_STATE_NTP_WAIT][eSYSMGR_EVENT_TIMER_EXPIRY] = &SysTimeMgr::ntpFailed;
    stateMachine[eSYSMGR_STATE_NTP_WAIT][eSYSMGR_EVENT_NTP_AVAILABLE] = &SysTimeMgr::ntpAquired;
    stateMachine[eSYSMGR_STATE_NTP_WAIT][eSYSMGR_EVENT_SECURE_TIME_AVAILABLE] = &SysTimeMgr::secureTimeAcquired;
    stateMachine[eSYSMGR_STATE_NTP_ACQUIRED][eSYSMGR_EVENT_SECURE_TIME_AVAILABLE] = &SysTimeMgr::updateSecureTime;
    stateMachine[eSYSMGR_STATE_NTP_ACQUIRED][eSYSMGR_EVENT_TIMER_EXPIRY] = &SysTimeMgr::updateTime;
    stateMachine[eSYSMGR_STATE_NTP_FAIL][eSYSMGR_EVENT_DTT_TIME_AVAILABLE] = &SysTimeMgr::dttAquired;
    stateMachine[eSYSMGR_STATE_NTP_FAIL][eSYSMGR_EVENT_TIMER_EXPIRY] = &SysTimeMgr::updateTime;
    stateMachine[eSYSMGR_STATE_NTP_FAIL][eSYSMGR_EVENT_NTP_AVAILABLE] = &SysTimeMgr::ntpAquired;
    stateMachine[eSYSMGR_STATE_NTP_FAIL][eSYSMGR_EVENT_SECURE_TIME_AVAILABLE] = &SysTimeMgr::secureTimeAcquired;
    stateMachine[eSYSMGR_STATE_DTT_ACQUIRED][eSYSMGR_EVENT_TIMER_EXPIRY] = &SysTimeMgr::updateClockRealTime;
    stateMachine[eSYSMGR_STATE_DTT_ACQUIRED][eSYSMGR_EVENT_SECURE_TIME_AVAILABLE] = &SysTimeMgr::secureTimeAcquired;
    stateMachine[eSYSMGR_STATE_DTT_ACQUIRED][eSYSMGR_EVENT_NTP_AVAILABLE] = &SysTimeMgr::ntpAquired;
    stateMachine[eSYSMGR_STATE_SECURE_TIME_ACQUIRED][eSYSMGR_EVENT_NTP_AVAILABLE] = &SysTimeMgr::updateSecureTime;
    stateMachine[eSYSMGR_STATE_SECURE_TIME_ACQUIRED][eSYSMGR_EVENT_TIMER_EXPIRY] = &SysTimeMgr::updateTime;
    m_state = eSYSMGR_STATE_NTP_WAIT;

}
void SysTimeMgr::runStateMachine(sysTimeMgrEvent event,void* args)
{
    RDK_LOG(RDK_LOG_DEBUG,LOG_SYSTIME,"[%s:%d]:Entered State Machine: Event = %d \n",__FUNCTION__,__LINE__,event);
    std::lock_guard<std::recursive_mutex> guard(g_state_mutex);
    map<sysTimeMgrState,map<sysTimeMgrEvent,memfunc> >::const_iterator iter = stateMachine.find(m_state);
    if ((iter != stateMachine.end()) && (iter->second.find(event) != iter->second.end()))
    {
       m_event = event;
       (this->*stateMachine[m_state][event])(args);
    }
    else
    {
       RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:There is no Event Processing available in this state. State = %d, Event = %d \n",__FUNCTION__,__LINE__,m_state,event);
    }

}

void SysTimeMgr::run(bool forever)
{
   //1. create thread
   //2. if selected to run forever wait till thread dies.
   
   std::thread processThrd(SysTimeMgr::processThr,this);
   std::thread timerThrd(SysTimeMgr::timerThr,this);
   std::thread pathMonitorThrd(SysTimeMgr::pathThr,this);
   if (forever)
   {
       ofstream pidfile("/run/systimemgr.pid",ios::out);
       if (pidfile.is_open())
       {
           pidfile<<getpid()<<"\n";
           pidfile.close();
       }
       processThrd.join();
       timerThrd.join();
       pathMonitorThrd.join();
   }
   else
   {
       processThrd.detach();
       timerThrd.detach();
       pathMonitorThrd.detach();
   }
}

void SysTimeMgr::processThr(SysTimeMgr* instance)
{
	if (instance)
	{
		instance->processMsg();
	}
}
void SysTimeMgr::timerThr(SysTimeMgr* instance)
{
	if (instance)
	{
		instance->runTimer();
	}
}

void SysTimeMgr::pathThr(SysTimeMgr* instance)
{
	if (instance)
	{
		instance->runPathMonitor();
	}
}

void SysTimeMgr::processMsg()
{
	while (1)
	{
		unique_lock<std::mutex> lk(m_queue_mutex);
		while (m_queue.empty())
		{
			m_cond_var.wait(lk); // release lock and go join the waiting thread queue
		}
		//sysTimeMsg*  val = m_queue.front();
		unique_ptr<sysTimeMsg> val = move(m_queue.front());
		m_queue.pop();
		runStateMachine(val->m_event,val->m_args);
		//delete val;
	}
}
void SysTimeMgr::runTimer()
{
	while (1)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(m_timerInterval));
		sendMessage(eSYSMGR_EVENT_TIMER_EXPIRY,NULL);
	}
}

void SysTimeMgr::runPathMonitor()
{

	//Send out events oif the file already exists.
	//This  will take care of cases where the program has to restart.

	for (auto iter = m_pathEventMap.begin();iter != m_pathEventMap.end();++iter)
	{
		string fname = m_directory + "/" + iter->first;
		ifstream f(fname.c_str());
		if (f.good())
		{
			sendMessage(iter->second,NULL);
		}
		f.close();
	}
	int inotifyFd, wd; 
	inotifyFd = inotify_init();
	if (inotifyFd == -1)
	{
		RDK_LOG(RDK_LOG_ERROR,LOG_SYSTIME,"[%s:%d]: Unable to create inotifyfd. Exiting thread \n",__FUNCTION__,__LINE__);
		return;
	}
	
	wd = inotify_add_watch(inotifyFd, m_directory.c_str(), IN_DELETE|IN_MODIFY|IN_ATTRIB);
	if (wd == -1)
	{
		RDK_LOG(RDK_LOG_ERROR,LOG_SYSTIME,"[%s:%d]:Unable to create Watchi descriptor. Exiting thread \n",__FUNCTION__,__LINE__);
		return;
	}
	char buffer[sizeof(struct inotify_event) + NAME_MAX + 1];
	while (1)
	{
		ssize_t count = read(inotifyFd, buffer, sizeof(buffer));
		if (count < 0)
		{
			RDK_LOG(RDK_LOG_ERROR,LOG_SYSTIME,"[%s:%d]:Failed to read. Exiting \n",__FUNCTION__,__LINE__);
			return;
		}
		buffer[sizeof(buffer)- 1] = '\0';
		struct inotify_event * pevent = reinterpret_cast<struct inotify_event *>(buffer);
		if (pevent->mask & IN_ATTRIB)
		{
			//This is file changed or created.
			RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:File created/modified = %s \n",__FUNCTION__,__LINE__,pevent->name);
			string fName(pevent->name);
			auto iter = m_pathEventMap.find(fName);
			if (iter != m_pathEventMap.end())
			{
				RDK_LOG(RDK_LOG_DEBUG,LOG_SYSTIME,"[%s:%d]:FOUND EVENT = %d \n",__FUNCTION__,__LINE__,iter->second);
				sendMessage(iter->second,NULL);

			}


		}
	}
}
void SysTimeMgr::updateTime(void* args)
{
	updateTimeSync(0);
	for (auto const& it : m_timerSrc)
	{
		it->checkTime();
	}
}
void SysTimeMgr::ntpFailed(void* args)
{
	updateTimeSync(0);
	m_state = eSYSMGR_STATE_NTP_FAIL;
        publishStatus(ePUBLISH_NTP_FAIL,"Poor");
}
void SysTimeMgr::ntpAquired(void* args)
{
	updateTimeSync(0);
	
	//Publish NTP AVAILABLE Notification.
	m_timequality = eTIMEQUALILTY_GOOD;
	m_state = eSYSMGR_STATE_NTP_ACQUIRED;
	m_timersrc = "NTP";
	publishStatus(ePUBLISH_NTP_SUCCESS,"Good");
}
void SysTimeMgr::updateSecureTime(void* args)
{
	timerExpiry(args);
	m_state = eSYSMGR_STATE_RUNNING;
        m_timequality = eTIMEQUALILTY_SECURE;

	publishStatus(ePUBLISH_SECURE_TIME_SUCCESS,"Secure");
}
void SysTimeMgr::secureTimeAcquired(void* args)
{
	updateTimeSync(0);
	m_timequality = eTIMEQUALILTY_GOOD;
	m_state = eSYSMGR_STATE_SECURE_TIME_ACQUIRED;
}

void SysTimeMgr::dttAquired(void* args)
{
	updateTimeSync(0);
	m_timequality = eTIMEQUALILTY_GOOD;
	m_state = eSYSMGR_STATE_DTT_ACQUIRED;
	m_timersrc = "DTT";

	publishStatus(ePUBLISH_DTT_SUCCESS,"Good");
}
void SysTimeMgr::timerExpiry(void* args)
{
	RDK_LOG(RDK_LOG_DEBUG,LOG_SYSTIME,"[%s:%d]:Entered Function \n",__FUNCTION__,__LINE__);
	long long reftime = 0,filetime = 0,updateTime =0;

	for (auto const& it : m_timerSrc)
	{
		if (it->isreference())
		{
			reftime = it->getTimeSec();
		}
		else
		{
			long long tempTime = it->getTimeSec();
			if (tempTime != 0)
			{
				filetime = tempTime;
			}
			else
			{
				RDK_LOG(RDK_LOG_DEBUG,LOG_SYSTIME,"[%s:%d]:TimeSrc Returned Zero! \n",__FUNCTION__,__LINE__);
			}
		}
	}

	if ((reftime - filetime) <= 600)
	{
		updateTime = reftime;
	}
	
	updateTimeSync(updateTime);
}
void SysTimeMgr::updateTimeSync(long long updateTime)
{
	for (auto const& i : m_timerSync)
	{
		i->updateTime(updateTime);
	}
}
void SysTimeMgr::sendMessage(sysTimeMgrEvent event, void* args)
{
	unique_lock<std::mutex> lk(m_queue_mutex);
	//sysTimeMsg* ptr = new sysTimeMsg(event,args);
	unique_ptr<sysTimeMsg> ptr(new sysTimeMsg(event,args));
	m_queue.push(move(ptr));
	lk.unlock();
	m_cond_var.notify_one();
}

void SysTimeMgr::setInitialTime()
{
	long long locTime = 0;
	struct timespec stime;
	for (auto const& i : m_timerSync)
	{
		locTime = i->getTime();
	}

	if (locTime == 0)
	{
		RDK_LOG(RDK_LOG_ERROR,LOG_SYSTIME,"[%s:%d]:Returning from Setting initial time since localtime returned from timersync is zero \n",__FUNCTION__,__LINE__);
		return;
	}
        struct timespec ts;
        long long currenttime = 0;
	m_timequality = eTIMEQUALILTY_POOR;
        if (clock_gettime(CLOCK_REALTIME, &ts) == 0) 
        {
            auto duration = seconds{ts.tv_sec} + nanoseconds{ts.tv_nsec};
            currenttime = static_cast<long long>(system_clock::to_time_t(time_point<system_clock, seconds>(duration_cast<seconds>(duration))));
            RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]: Clock_gettime Call succeeded with REAL time.\n",__FUNCTION__,__LINE__);
            if (currenttime > locTime)
            {
                RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]: Clock Time is greater that time provided by Timesync, leaving it alone.\n",__FUNCTION__,__LINE__);
                publishStatus(ePUBLISH_TIME_INITIAL,"Poor");
                return;
            }
        }
	stime.tv_sec = locTime;
	stime.tv_nsec = 0;
	if (clock_settime( CLOCK_REALTIME, &stime) != 0)
	{
		RDK_LOG(RDK_LOG_ERROR,LOG_SYSTIME,"[%s:%d]:Failed to set time \n",__FUNCTION__,__LINE__);
	}
	else
	{
		RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:Successfully to set time \n",__FUNCTION__,__LINE__);
	}

	publishStatus(ePUBLISH_TIME_INITIAL,"Poor");
}
void SysTimeMgr::publishStatus(publishEvent event,string message)
{
	TimerMsg msg;
	msg.event = event;
	msg.quality = m_timequality;
        memset(msg.message,'\0',10*cTIMER_STATUS_MESSAGE_LENGTH);
	snprintf(msg.message, cTIMER_STATUS_MESSAGE_LENGTH, "%s", message.c_str());        //CID:277715  Buffer not null terminated                  
	snprintf(msg.timerSrc, cTIMER_STATUS_MESSAGE_LENGTH, "%s", m_timersrc.c_str());    //CID:277715  Buffer not null terminated
	using namespace std::chrono;
	//time_t timeinSec = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	time_t timeinSec = time(NULL);
	struct timespec ts;
	if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
		auto duration = std::chrono::seconds{ts.tv_sec} + std::chrono::nanoseconds{ts.tv_nsec};
		timeinSec = system_clock::to_time_t(time_point<system_clock, seconds>(duration_cast<seconds>(duration)));
		RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]: clock_gettime Call succeeded with REAL time.\n",__FUNCTION__,__LINE__);
	}

	time_t monotimeinSec = time(NULL);
	char timeStr[100] = {0};
	char monotimeStr[100] = {0};
	strftime(timeStr, sizeof(timeStr), "%A %c", localtime(&timeinSec));
	strftime(monotimeStr, sizeof(monotimeStr), "%A %c", localtime(&monotimeinSec));
	RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:TIME Published = %ld, Converted Real Time(included in published time): %s, Converted Monotonic Time = %s \n",__FUNCTION__,__LINE__,timeinSec,timeStr,monotimeStr); 
	snprintf(msg.currentTime, cTIMER_STATUS_MESSAGE_LENGTH, "%s", std::to_string(timeinSec).c_str());   //CID:277715  Buffer not null terminated
	m_publish->publish(cTIMER_STATUS_UPDATE,&msg);
}
int SysTimeMgr::getTimeStatus(void* args)
{
	TimerMsg* pMsg = reinterpret_cast<TimerMsg*>(args);
	SysTimeMgr* pInstance = get_instance();

	if (pInstance != NULL)
	{
		pInstance->getTimeStatus(pMsg);
	}

	return 0;
}

void SysTimeMgr::getTimeStatus(TimerMsg* pMsg)
{
	std::lock_guard<std::recursive_mutex> guard(g_state_mutex);
	pMsg->quality = m_timequality;
	memset(pMsg->message,'\0',10*cTIMER_STATUS_MESSAGE_LENGTH);
	string populatedString;

	switch (m_timequality)
	{
		case  eTIMEQUALILTY_POOR:
		{
			populatedString = "Poor";
			break;
		}
		case eTIMEQUALILTY_GOOD:
		{
			populatedString = "Good";
			break;
		}
		case eTIMEQUALILTY_SECURE:
		{
			populatedString = "Secure";
			break;
		}
		default:
		{
			populatedString = "Unknown";
			break;
		}
	}
                      
	snprintf(pMsg->message, cTIMER_STATUS_MESSAGE_LENGTH, "%s", populatedString.c_str());      //CID:277708 Buffer not null terminated                   
        snprintf(pMsg->timerSrc, cTIMER_STATUS_MESSAGE_LENGTH, "%s", m_timersrc.c_str());          //CID:277708 Buffer not null terminated
	using namespace std::chrono;
	//time_t timeinSec = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	time_t timeinSec = time(NULL);
	struct timespec ts;
	if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
		auto duration = seconds{ts.tv_sec} + nanoseconds{ts.tv_nsec};
		timeinSec = system_clock::to_time_t(time_point<system_clock, seconds>(duration_cast<seconds>(duration)));
		RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]: Clock_gettime Call succeeded with REAL time.\n",__FUNCTION__,__LINE__);
	}
	time_t monotimeinSec = time(NULL);
	char timeStr[100] = {0};
	char monotimeStr[100] = {0};
	strftime(timeStr, sizeof(timeStr), "%A %c", localtime(&timeinSec));
	strftime(monotimeStr, sizeof(monotimeStr), "%A %c", localtime(&monotimeinSec));
	RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:TIME Returning for Query = %ld, Converted Real Time(included in TimeMsg): %s, Converted Monotonic Time = %s \n",__FUNCTION__,__LINE__,timeinSec,timeStr,monotimeStr);
        snprintf(pMsg->currentTime, cTIMER_STATUS_MESSAGE_LENGTH, "%s", std::to_string(timeinSec).c_str());   //CID:277708 Buffer not null terminated
}
int SysTimeMgr::powerhandler(void* args)
{
	RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:Entering Power Change Event \n",__FUNCTION__,__LINE__);
	string* message = reinterpret_cast<string*>(args);
	SysTimeMgr* pInstance = get_instance();

	if (pInstance == NULL) {
		return 0;
	}
	if (*message == "DEEP_SLEEP_ON") {
		pInstance->deepsleepon();
	}

	if (*message == "DEEP_SLEEP_OFF") {
		pInstance->deepsleepoff();
	}

	return 1;
}
void SysTimeMgr::deepsleepoff()
{
	//Reset State Machine
	RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:Deep Sleep is Turned off. Resetting State Machine and restarting ntp service. \n",__FUNCTION__,__LINE__);
	std::lock_guard<std::recursive_mutex> guard(g_state_mutex);
	if (m_timequality == eTIMEQUALILTY_SECURE) {
		m_timequality = eTIMEQUALILTY_GOOD;
	        m_state = eSYSMGR_STATE_NTP_ACQUIRED;
	}
	string message;
        m_timersrc = "Last Known";
  
	switch (m_timequality)
	{
		case  eTIMEQUALILTY_POOR:
		{
			message = "Poor";
			break;
		}
		case eTIMEQUALILTY_GOOD:
		{
			message = "Good";
			break;
		}
		case eTIMEQUALILTY_SECURE:
		{
			message = "Secure";
			break;
		}
		default:
		{
			message = "Unknown";
			break;
		}
	}

	publishStatus(ePUBLISH_DEEP_SLEEP_ON,message);

	//Turn on the NTP time sync.
        v_secure_system("/bin/systemctl reset-failed systemd-timesyncd.service");
	v_secure_system("/bin/systemctl restart systemd-timesyncd.service");
}

void SysTimeMgr::deepsleepon()
{
	//Turn on the NTP time sync.
	RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:Deep Sleep is turned on.\n",__FUNCTION__,__LINE__);
}

void SysTimeMgr::updateClockRealTime(void* args)
{
	long long locTime = 0;
	struct timespec stime;

	bool clock_acquired = false;

	for (auto const& i : m_timerSrc)
	{
		if (i->isclockProvider() && !clock_acquired) {
			locTime = i->getTimeSec();
			clock_acquired = true;
		}
		i->checkTime();
	}

	if (locTime != 0) {
		stime.tv_sec = locTime;
		stime.tv_nsec = 0;
		if (clock_settime( CLOCK_REALTIME, &stime) != 0) {
			RDK_LOG(RDK_LOG_ERROR,LOG_SYSTIME,"[%s:%d]:Failed to  set time \n",__FUNCTION__,__LINE__);
		}
		else {
			RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:Successfully to  set time \n",__FUNCTION__,__LINE__);
		}
	}
	else {
		RDK_LOG(RDK_LOG_ERROR,LOG_SYSTIME,"[%s:%d]:Time is not available to Set. Skipping Clock_settime. \n",__FUNCTION__,__LINE__);
	}
	updateTimeSync(0);
}
