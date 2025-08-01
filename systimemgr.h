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

#ifndef _SYSTIMEMGR_H_
#define _SYSTIMEMGR_H_

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
#include "itimesrc.h"
#include "itimesync.h"
#include "ipublish.h"
#include "isubscribe.h"
#include "itimermsg.h"
#include <memory>

#ifdef T2_EVENT_ENABLED
#include <telemetry_busmessage_sender.h>

void t2CountNotify(char *marker, int val);
void t2ValNotify(char *marker, char *val);
#endif

using namespace std;
typedef enum 
{
    eSYSMGR_STATE_INIT =0,
    eSYSMGR_STATE_NTP_WAIT,
    eSYSMGR_STATE_NTP_FAIL,
    eSYSMGR_STATE_NTP_ACQUIRED,
    eSYSMGR_STATE_DTT_ACQUIRED,
    eSYSMGR_STATE_SECURE_TIME_ACQUIRED,
    eSYSMGR_STATE_RUNNING
} sysTimeMgrState;

typedef enum
{
   eSYSMGR_EVENT_ADD_TIMESRC =0,
   eSYSMGR_EVENT_DELETE_TIMESRC,
   eSYSMGR_EVENT_ADD_TIMESYNC,
   eSYSMGR_EVENT_DELETE_TIMESSYNC,
   eSYSMGR_EVENT_TIMER_EXPIRY,
   eSYSMGR_EVENT_NTP_AVAILABLE,
   eSYSMGR_EVENT_DTT_TIME_AVAILABLE,
   eSYSMGR_EVENT_SECURE_TIME_AVAILABLE,
   eSYSMGR_EVENT_UNKNOWN,
} sysTimeMgrEvent;

typedef struct sysTimeMsg
{
	sysTimeMgrEvent m_event;
	void* m_args;
	sysTimeMsg(sysTimeMgrEvent event,void* args):m_event(event),m_args(args){}
	virtual ~sysTimeMsg(){ //CID 277706 : Deleting void pointer
        	if (m_args){
        		delete static_cast<TimerMsg*>(m_args);
        		m_args = nullptr;
                }
        }
}sysTimeMsg;



class SysTimeMgr
{
private: 
	typedef void (SysTimeMgr::*memfunc)(void* args);
	map<sysTimeMgrState,map<sysTimeMgrEvent,memfunc> > stateMachine;
	map<string,sysTimeMgrEvent> m_pathEventMap;
        sysTimeMgrState m_state;
        sysTimeMgrEvent m_event;

	unsigned long m_timerInterval;
	qualityOfTime m_timequality;
	string m_timersrc;

        const string m_directory = "/tmp/systimemgr";
	

        vector<ITimeSrc*> m_timerSrc;
	vector<ITimeSync*> m_timerSync;

	IPublish* m_publish;
        ISubscribe* m_subscriber;
        ISubscribe* m_tmrsubscriber;

	//Config file to load plugins.
	string m_cfgfile;


        SysTimeMgr (string cfgfile = "/etc/systimemgr.conf");
        void setInitialTime();

        static void processThr(SysTimeMgr* instance);
        static void timerThr(SysTimeMgr* instance);
        static void pathThr(SysTimeMgr* instance);



	void updateTimeSync(long long updateTime);
	void publishStatus(publishEvent event,string message);

	queue<unique_ptr<sysTimeMsg> > m_queue;
        mutex m_queue_mutex;
	condition_variable m_cond_var;

        static recursive_mutex g_state_mutex;
        static mutex g_instance_mutex;
        static SysTimeMgr* pInstance;


        // LIstening socket and its related addresses etc.
public:
        virtual ~SysTimeMgr();
        void runStateMachine(sysTimeMgrEvent event,void* args);

	void timerExpiry(void* args);
	void ntpFailed(void* args);
	void ntpAquired(void* args);
	void dttAquired(void* args);
	void updateSecureTime(void* args);
	void updateTime(void* args);
	void secureTimeAcquired(void* args);
	void updateClockRealTime(void* args);

        void initialize();
        static SysTimeMgr* get_instance();
        void run(bool forever=true);

	void sendMessage(sysTimeMgrEvent event, void* args);

	void processMsg();
	void runTimer();
	void runPathMonitor();

	void getTimeStatus(TimerMsg* pMsg);
	static int getTimeStatus(void* args);
	static int powerhandler(void* args);
	void deepsleepon();
	void deepsleepoff();

#ifdef GTEST_ENABLE 
friend class SysTimeMgrTest_RunStateMachine_HitsFunctionPointer_Test;
friend class SysTimeMgrTest_RunPathMonitorFileExistsAtStartup_Test;
friend class SysTimeMgrTest_RunPathMonitorCoversInotifyEvent_Test;
friend class SysTimeMgrTest_RunStateMachine_AllStatesEvents_Test;
friend class SysTimeMgrTest_SetInitialTime_NonZeroTime_Test;
friend class SysTimeMgrTest;
friend class SysTimeMgrTest_DestructorCovers_Test;
friend class SysTimeMgrTest_SetInitialTime_ZeroTime_Test;
friend class SysTimeMgrTest_UpdateTime_InvokesCheckTime_Test;
friend class SysTimeMgrTest_SetInitialTime_FileCreationFails_Test;
friend class SysTimeMgrTest_TimerExpiryUsesReference_Test;
friend class SysTimeMgrTest_TimerExpiryUsesFileTime_Test;
friend class SysTimeMgrTest_UpdateTimeSyncCallsSyncs_Test;
friend class SysTimeMgrTest_NtpAquiredPublishesStatusAndUpdatesState_Test;
friend class SysTimeMgrTest_NtpFailed_Test;
friend class SysTimeMgrTest_DttAcquiredPublishesStatusAndUpdatesState_Test;
friend class SysTimeMgrTest_SecureTimeAcquiredUpdatesState_Test;
friend class SysTimeMgrTest_UpdateSecureTimePublishesStatusAndUpdatesState_Test;
friend class SysTimeMgrTest_DeepSleepOffPublishesStatus_Test;
friend class SysTimeMgrTest_DeepSleepOffCoversPoorCase_Test;
friend class SysTimeMgrTest_UpdateClockRealTimeSetsTime_Test;
friend class SysTimeMgrTest_UpdateClockRealTimeAllBranches_Test;
friend class SysTimeMgrTest_GetTimeStatus_AllQualities_Test;
friend class SysTimeMgrTest_PublishStatusCoversAll_Test;
friend class SysTimeMgrTest_TimerExpiry_RefVsFileTime_Test;
friend class SysTimeMgrTest_RunPathMonitorCoversInotifyEvent_Test;
friend class SysTimeMgrTest_RunPathMonitorFileExistsAtStartup_Test;
friend class SysTimeMgrTest_RunPathMonitorInotifyAddWatchFails_Test;
friend class SysTimeMgrTest_RunPathMonitorCoversInotifyEvent_Test;
friend class SysTimeMgrTest_RunPathMonitorFileExistsAtStartup_Test;
friend class SysTimeMgrTest_RunPathMonitorInotifyAddWatchFails_Test;
friend class SysTimeMgrTest_UpdateClockRealTimeSetsTime_Test;
friend class SysTimeMgrTest_TimerExpiry_RefVsFileTime_Test;
friend class SysTimeMgrTest_ProcessThrCallsProcessMsgAndRunsOneIteration_Test;
friend class SysTimeMgrTest_TimerThrCallsRunTimerAndRunsOnce_Test;
friend class  SysTimeMgrTest_TimerThrAndProcessThrCoverage_Test;
#endif

};


#endif //_SYSTIMEMGR_H_
