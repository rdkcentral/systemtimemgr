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
#ifndef _IARMSUBSCRIBER_H_
#define _IARMSUBSCRIBER_H_

#include <string>
#include <queue>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <unistd.h>
#include "isubscribe.h"
#include "power_controller.h"

//Although we could use  Lambda functions and higher order functions(which are available from c++11 onwards.)
//Since the registration functions typically c functions. 
//Perhaps this can be cosidered at a later point of time.

using namespace std;

/* System Time Mgr Power Controller State Data Structure to Pass Queue Data to the Event handler Thread */
typedef struct SysTimeMgr_Power_Event_State{
    PowerController_PowerState_t currentState;
    PowerController_PowerState_t newState;
    SysTimeMgr_Power_Event_State(PowerController_PowerState_t currSt, PowerController_PowerState_t newSt)
        : currentState(currSt), newState(newSt) {}
}SysTimeMgr_Power_Event_State_t;

/* power controller connect function retry delay of 300msec */
#define     PWR_CNTRL_CONNECT_WAIT_TIME_MS     (300000)

class IarmSubscriber:public ISubscribe
{
         private:
            static IarmSubscriber* pInstance;
            funcPtr m_powerHandler;
            /* Power Controller queue and mutex to guard it*/
            static std::queue<SysTimeMgr_Power_Event_State_t> m_pwrEvtQueue;
            static std::mutex m_pwrEvtQueueLock;
            /* Power Controller Event Handling Thread variables*/
            static std::thread m_sysTimeMgrPwrEvtHandlerThread;
            static std::mutex m_pwrEvtMutexLock;
            static std::condition_variable m_pwrEvtCondVar;
            static std::atomic<bool> m_sysTimeMgrPwrStopThread;

	public:
		IarmSubscriber(string sub);
		bool subscribe(string eventname,funcPtr fptr);
		static IarmSubscriber* getInstance() { return pInstance;}
		void invokepowerhandler(void* args){ if (m_powerHandler) (*m_powerHandler)(args);}
            static void sysTimeMgrPwrEventHandler(const PowerController_PowerState_t currentState,
                                                                         const PowerController_PowerState_t newState,
                                                                         void* userdata);
            static void sysTimeMgrHandlePwrEventData(const PowerController_PowerState_t currentState,
                            const PowerController_PowerState_t newState);
            static void sysTimeMgrPwrEventHandlingThreadFunc(void *arg);
            static void sysTimeMgrPwrConnectHandlingThreadFunc(void *arg);
            static void sysTimeMgrInitPwrEvt(void);
            static void sysTimeMgrDeinitPwrEvt(void);
};

#endif// _IARMSUBSCRIBER_H_
