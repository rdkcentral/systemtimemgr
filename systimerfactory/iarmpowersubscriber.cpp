/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 RDK Management
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
#include "iarmpowersubscriber.h"
#include "libIARM.h"
#include "libIBus.h"
#include "libIARMCore.h"
#include "pwrMgr.h"
#include "irdklog.h"

IarmPowerSubscriber::IarmPowerSubscriber(string sub):IarmSubscriber(sub),m_powerHandler(NULL)
{
	RDK_LOG(RDK_LOG_DEBUG,LOG_SYSTIME,"[%s:%d]:Entry\n",__FUNCTION__,__LINE__);
	int registered=0;
	if (IARM_RESULT_SUCCESS != IARM_Bus_IsConnected(m_subscriber.c_str(),&registered))
	{
		IARM_Bus_Init(m_subscriber.c_str());
		IARM_Bus_Connect();
		RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:IarmPowerSubscriber IARM_Bus_Init and IARM_Bus_Connect Invoked \n",__FUNCTION__,__LINE__);
	}
	else
	{
		RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:IarmPowerSubscriber IARM_Bus_IsConnected Success \n",__FUNCTION__,__LINE__);
	}
	RDK_LOG(RDK_LOG_DEBUG,LOG_SYSTIME,"[%s:%d]:Exit \n",__FUNCTION__,__LINE__);
}

bool IarmPowerSubscriber::subscribe(string eventname,funcPtr fptr)
{
   RDK_LOG(RDK_LOG_DEBUG,LOG_SYSTIME,"[%s:%d]:IARMBUS Registering function for Event = %s \n",__FUNCTION__,__LINE__,eventname.c_str());

   bool retCode = false;
   if (POWER_CHANGE_MSG == eventname) {
      m_powerHandler = fptr;
      retCode = IARM_Bus_RegisterEventHandler(IARM_BUS_PWRMGR_NAME,IARM_BUS_PWRMGR_EVENT_MODECHANGED,IarmPowerSubscriber::powereventHandler);
   }
   else
   {
      RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:IarmPowerSubscriber IARM_Bus_RegisterEventHandler Failed retCode = %d \n",__FUNCTION__,__LINE__,retCode);
   }
   RDK_LOG(RDK_LOG_DEBUG,LOG_SYSTIME,"[%s:%d]:Exit retCode=%d\n",__FUNCTION__,__LINE__,retCode);
   return retCode;
}

void IarmPowerSubscriber::powereventHandler(const char *owner, int eventId, void *data, size_t len)
{
   RDK_LOG(RDK_LOG_DEBUG,LOG_SYSTIME,"[%s:%d]:Power Change Entry Event Received\n",__FUNCTION__,__LINE__);
   if ( IARM_BUS_PWRMGR_EVENT_MODECHANGED != eventId ) {
      return;
   }

   IARM_Bus_PWRMgr_EventData_t *param = (IARM_Bus_PWRMgr_EventData_t *)data;
   RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:Power Change Event Received. New State = %d\n",__FUNCTION__,__LINE__,param->data.state.newState);

   IarmPowerSubscriber* instance = dynamic_cast<IarmPowerSubscriber*>(IarmSubscriber::getInstance());
   if (!instance) {
        RDK_LOG(RDK_LOG_ERROR, LOG_SYSTIME, "[%s:%d]: Error: IarmPowerSubscriber Get Instance Failed\n", __FUNCTION__, __LINE__);
        return;
    }

   if ((IARM_BUS_PWRMGR_POWERSTATE_OFF == param->data.state.newState) || 
       (IARM_BUS_PWRMGR_POWERSTATE_STANDBY_DEEP_SLEEP == param->data.state.newState)) {

      RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:Deep Sleep Event Received\n",__FUNCTION__,__LINE__);	
	  string powerstatus("DEEP_SLEEP_ON");
         instance->invokepowerhandler((void*)&powerstatus);
   }
   else if ((IARM_BUS_PWRMGR_POWERSTATE_STANDBY_DEEP_SLEEP == param->data.state.curState ) && 
            ((IARM_BUS_PWRMGR_POWERSTATE_ON == param->data.state.newState) || 
	     (IARM_BUS_PWRMGR_POWERSTATE_STANDBY_LIGHT_SLEEP == param->data.state.newState ) ||
	     (IARM_BUS_PWRMGR_POWERSTATE_STANDBY == param->data.state.newState ))) {
	 string powerstatus("DEEP_SLEEP_OFF");
         instance->invokepowerhandler((void*)&powerstatus);
   }
   RDK_LOG(RDK_LOG_DEBUG,LOG_SYSTIME,"[%s:%d]:Exit \n",__FUNCTION__,__LINE__);
}
