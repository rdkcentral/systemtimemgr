/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 RDK Management
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
 */
#include "iarmsubscribe.h"
#include "power_controller.h"
#include "ipowercontrollersubscriber.h"
#include "irdklog.h"


IpowerControllerSubscriber::IpowerControllerSubscriber(string sub):IarmSubscriber(sub)
{
	RDK_LOG(RDK_LOG_DEBUG,LOG_SYSTIME,"[%s:%d]:Entry \n",__FUNCTION__,__LINE__);
	RDK_LOG(RDK_LOG_DEBUG,LOG_SYSTIME,"[%s:%d]:Exit \n",__FUNCTION__,__LINE__);
}

bool IpowerControllerSubscriber::subscribe(string eventname,funcPtr fptr)
{
	RDK_LOG(RDK_LOG_DEBUG,LOG_SYSTIME,"[%s:%d]:Entering Registering function for Event = %s \n",__FUNCTION__,__LINE__,eventname.c_str());
	bool retCode = false;
	uint32_t retValPwrCtrl=0;
	if (POWER_CHANGE_MSG == eventname)
	{
		RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:Registering function for the Event = %s \n",__FUNCTION__,__LINE__,eventname.c_str());

		/* This is for the initalization of refactored power controller client for IARM communication used in systemtime manager 
		    All the Power Controller functions are handled here */
		PowerController_Init();

		RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:PowerController_Init Completed \n",__FUNCTION__,__LINE__);
		/* This is for the systemtime manager event handler thread and mutex variable initalization */
		IpowerControllerSubscriber::sysTimeMgrInitPwrEvt();
		RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:Initializing sysTimeMgrInitPwrEvt Completed \n",__FUNCTION__,__LINE__);
		m_powerHandler = fptr;

		/* Check for Power Controller Connection and if failure, start a new thread and wait until connection established */
		if(POWER_CONTROLLER_ERROR_NONE == PowerController_Connect())
		{
			/* The registration function returns different values for different errors and err none value is 0 */
			retValPwrCtrl = PowerController_RegisterPowerModeChangedCallback(IpowerControllerSubscriber::sysTimeMgrPwrEventHandler, nullptr);
			if(POWER_CONTROLLER_ERROR_NONE != retValPwrCtrl)
			{
				/* The caller of the subscribe function does not check retCode, Only Error is Logged here for Failure */
				RDK_LOG(RDK_LOG_ERROR, LOG_SYSTIME, "[%s:%d]:Power Controller RegisterPowerModeChangedCallback Failed retValPwrCtrl=[%d]\n", __FUNCTION__, __LINE__,retValPwrCtrl);
			}
			else
			{
				RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:Power Controller Registration Success \n",__FUNCTION__,__LINE__);
				retCode = true;
			}
		}
		else
		{
			std::thread pwrConnectHandlerThread;
			pwrConnectHandlerThread = std::thread(&IpowerControllerSubscriber::sysTimeMgrPwrConnectHandlingThreadFunc, this);
			if (!pwrConnectHandlerThread.joinable())
			{
				/* The caller of the subscribe function does not check retCode, Only Error is Logged here for Failure */
				RDK_LOG(RDK_LOG_ERROR, LOG_SYSTIME, "[%s:%d]:Thread Creation Power Controller Connect HandlerThread Failed\n", __FUNCTION__, __LINE__);
			}
			else
			{
				RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:Power Controller Thread Creation Success \n",__FUNCTION__,__LINE__);
				retCode = true;
				pwrConnectHandlerThread.detach();
			}
		}
	}
	RDK_LOG(RDK_LOG_DEBUG,LOG_SYSTIME,"[%s:%d]:Exit retCode = %d\n",__FUNCTION__,__LINE__,retCode);
	return retCode;
}

IpowerControllerSubscriber::~IpowerControllerSubscriber()
{
	RDK_LOG(RDK_LOG_DEBUG,LOG_SYSTIME,"[%s:%d]:~IpowerControllerSubscriber function\n",__FUNCTION__,__LINE__);

	/* This is for the systemtime manager deInit */
	IpowerControllerSubscriber::sysTimeMgrDeinitPwrEvt();
	RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:DeInit Success \n",__FUNCTION__,__LINE__);
	
	/* This is for the Termination of systemtime manager's power controller client handler */
	PowerController_Term();
	RDK_LOG(RDK_LOG_DEBUG,LOG_SYSTIME,"[%s:%d]:Exit \n",__FUNCTION__,__LINE__);
}


void IpowerControllerSubscriber::sysTimeMgrPwrEventHandler(const PowerController_PowerState_t currentState,
										   const PowerController_PowerState_t newState,
										   void *userdata)
{
	RDK_LOG(RDK_LOG_DEBUG, LOG_SYSTIME, "[%s:%d]:Entering \n", __FUNCTION__, __LINE__);
	IpowerControllerSubscriber* instance = dynamic_cast<IpowerControllerSubscriber*>(IarmSubscriber::getInstance());

	if (instance)
	{
		std::lock_guard<std::mutex> lock(instance->m_pwrEvtQueueLock);
		instance->m_pwrEvtQueue.emplace(currentState, newState);
	}
	RDK_LOG(RDK_LOG_INFO, LOG_SYSTIME, "[%s:%d]:Sending Signal to Thread for Processing Callback Event\n", __FUNCTION__, __LINE__);
	instance->m_pwrEvtCondVar.notify_one();
	RDK_LOG(RDK_LOG_DEBUG,LOG_SYSTIME,"[%s:%d]:Exit \n",__FUNCTION__,__LINE__);
}


void IpowerControllerSubscriber::sysTimeMgrPwrConnectHandlingThreadFunc()
{
	RDK_LOG(RDK_LOG_DEBUG, LOG_SYSTIME, "[%s:%d]: Entered \n", __FUNCTION__, __LINE__);
	uint32_t retValPwrCtrl=0;

	/* Loop and check for Power controller connection and if fails sleep and retry */
	while(true)
	{
		if(POWER_CONTROLLER_ERROR_NONE == PowerController_Connect())
		{
			RDK_LOG(RDK_LOG_INFO, LOG_SYSTIME, "[%s:%d]: PowerController_Connect is Success\n", __FUNCTION__, __LINE__);
			break;
		}
		else
		{
			RDK_LOG(RDK_LOG_ERROR, LOG_SYSTIME, "[%s:%d]:PowerController_Connect Failed\n", __FUNCTION__, __LINE__);
			/* Sleep for 300 msec */
			usleep(PWR_CNTRL_CONNECT_WAIT_TIME_MS);
		}
	}
	retValPwrCtrl = PowerController_RegisterPowerModeChangedCallback(IpowerControllerSubscriber::sysTimeMgrPwrEventHandler, nullptr);
	if(POWER_CONTROLLER_ERROR_NONE != retValPwrCtrl)
	{
		/* Ideally Call back should not fail and failure can happen only if reregister same function is done.
		   Only Error is Logged here for Failure as this is a separate thread*/
		RDK_LOG(RDK_LOG_ERROR, LOG_SYSTIME, "[%s:%d]:Power Controller RegisterPowerModeChangedCallback Failed retValPwrCtrl=[%d]\n", __FUNCTION__, __LINE__,retValPwrCtrl);
	}
	RDK_LOG(RDK_LOG_DEBUG,LOG_SYSTIME,"[%s:%d]:Exit retValPwrCtrl = %d\n",__FUNCTION__,__LINE__,retValPwrCtrl);
}


void IpowerControllerSubscriber::sysTimeMgrPwrEventHandlingThreadFunc()
{
	RDK_LOG(RDK_LOG_DEBUG, LOG_SYSTIME, "[%s:%d]: Entered \n", __FUNCTION__, __LINE__);

	SysTimeMgr_Power_Event_State_t sysTimeMgrPwrEvent(POWER_STATE_UNKNOWN,POWER_STATE_UNKNOWN);
	while (true)
	{
		std::unique_lock<std::mutex> lkVar(m_pwrEvtMutexLock);
		RDK_LOG(RDK_LOG_INFO, LOG_SYSTIME, "[%s:%d]: Waiting for Events from Power Manager\n", __FUNCTION__, __LINE__); 
		/* Wait until the queue is not empty or stop thread is signaled       */
		m_pwrEvtCondVar.wait(lkVar, [this]
		{
			return !m_pwrEvtQueue.empty();
		});
		/* Unlock before processing event from the Queue */
		lkVar.unlock();		
		{
			/* Lock, extract element, unlock, process element, lock and check for element availability in queue.
			After all the elements are processed unlock the queue */
			std::unique_lock<std::mutex> queueLock(m_pwrEvtQueueLock);
			while(!m_pwrEvtQueue.empty())
			{
				sysTimeMgrPwrEvent = std::move(m_pwrEvtQueue.front());
				m_pwrEvtQueue.pop();
				queueLock.unlock();
				IpowerControllerSubscriber::sysTimeMgrHandlePwrEventData(sysTimeMgrPwrEvent.currentState, 
						sysTimeMgrPwrEvent.newState);
				queueLock.lock();
			}
			queueLock.unlock();
		}
	}
	RDK_LOG(RDK_LOG_DEBUG, LOG_SYSTIME, "[%s:%d]: Exit \n", __FUNCTION__, __LINE__);
}



void IpowerControllerSubscriber::sysTimeMgrHandlePwrEventData(const PowerController_PowerState_t currentState,
    const PowerController_PowerState_t newState)
{
	RDK_LOG(RDK_LOG_DEBUG,LOG_SYSTIME,"[%s:%d]:SysTimeMgrHandlePwrEventData currentState[%d] newState[%d]\n",__FUNCTION__,__LINE__,currentState,newState);
	string powerstatus = "UNKNOWN";
	IpowerControllerSubscriber* instance = dynamic_cast<IpowerControllerSubscriber*>(IarmSubscriber::getInstance());
	
	if(instance)
	{
		switch(newState)
		{
			case POWER_STATE_OFF:
			case POWER_STATE_STANDBY_DEEP_SLEEP:
				RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:Deep Sleep ON is Invoked\n",__FUNCTION__,__LINE__);
				powerstatus = "DEEP_SLEEP_ON";
				instance->invokepowerhandler((void*)&powerstatus);
				break;

			case POWER_STATE_ON:
			case POWER_STATE_STANDBY_LIGHT_SLEEP:
			case POWER_STATE_STANDBY:
			if(POWER_STATE_STANDBY_DEEP_SLEEP == currentState)
			{
					RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:Deep Sleep OFF is Invoked\n",__FUNCTION__,__LINE__);
					powerstatus = "DEEP_SLEEP_OFF";
					instance->invokepowerhandler((void*)&powerstatus);
			}
			break;

			default:
				RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:Err: default case Invalid State Value\n",__FUNCTION__,__LINE__);
				break;
		}	
	}
	else
	{
		RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:Err: Unable to getInstance\n",__FUNCTION__,__LINE__);
	}
	RDK_LOG(RDK_LOG_DEBUG,LOG_SYSTIME,"[%s:%d]:Exit \n",__FUNCTION__,__LINE__);
}


void IpowerControllerSubscriber::sysTimeMgrInitPwrEvt(void)
{
	RDK_LOG(RDK_LOG_DEBUG,LOG_SYSTIME,"[%s:%d]:Entering \n",__FUNCTION__,__LINE__);

	m_sysTimeMgrPwrEvtHandlerThread = std::thread(&IpowerControllerSubscriber::sysTimeMgrPwrEventHandlingThreadFunc, this);
	if (m_sysTimeMgrPwrEvtHandlerThread.joinable())
	{
		RDK_LOG(RDK_LOG_INFO, LOG_SYSTIME, "[%s:%d]:Thread Creation is Success\n", __FUNCTION__, __LINE__);
	}
	else
	{
		RDK_LOG(RDK_LOG_ERROR, LOG_SYSTIME, "[%s:%d]:Thread Creation Failed\n", __FUNCTION__, __LINE__);
	}
	RDK_LOG(RDK_LOG_DEBUG,LOG_SYSTIME,"[%s:%d]:Exit \n",__FUNCTION__,__LINE__);
}


void IpowerControllerSubscriber::sysTimeMgrDeinitPwrEvt(void)
{
	RDK_LOG(RDK_LOG_DEBUG,LOG_SYSTIME,"[%s:%d]:Entered \n",__FUNCTION__,__LINE__);

	{
		/* Notify the Event thread function to Exit, send signal with stop thread true so that the new CB thread exits,
		    lock Guard is unlocked after out of scope  after this block  */
		std::lock_guard<std::mutex> lock(m_pwrEvtMutexLock);
		m_pwrEvtCondVar.notify_one();
	}
	
	/* Clear the elements of the Queue  */
	{
		std::lock_guard<std::mutex> lock(m_pwrEvtQueueLock);
		m_pwrEvtQueue = std::queue<SysTimeMgr_Power_Event_State_t>();
	}

	/* Wait until the CB thread finishes and exits*/
	if (m_sysTimeMgrPwrEvtHandlerThread.joinable())
	{
		RDK_LOG(RDK_LOG_INFO, LOG_SYSTIME, "[%s:%d]:Deinit Joining the thread\n", __FUNCTION__, __LINE__);
		m_sysTimeMgrPwrEvtHandlerThread.join();
		RDK_LOG(RDK_LOG_INFO, LOG_SYSTIME, "[%s:%d]:Completed Deinit and Joined thread\n", __FUNCTION__, __LINE__);
	}

	if(POWER_CONTROLLER_ERROR_NONE != 
		PowerController_UnRegisterPowerModeChangedCallback(IpowerControllerSubscriber::sysTimeMgrPwrEventHandler))
	{
		RDK_LOG(RDK_LOG_ERROR, LOG_SYSTIME, "[%s:%d]:UnRegisterPowerModeChangedCallback Failed \n", __FUNCTION__, __LINE__);
	}
	RDK_LOG(RDK_LOG_DEBUG,LOG_SYSTIME,"[%s:%d]:Exit \n",__FUNCTION__,__LINE__);
}
