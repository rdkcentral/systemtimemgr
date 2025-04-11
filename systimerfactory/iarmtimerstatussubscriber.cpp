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

#include "iarmsubscribe.h"
#include "iarmtimerstatussubscriber.h"
#include "libIARM.h"
#include "libIBus.h"
#include "libIARMCore.h"
#include "pwrMgr.h"
#include "irdklog.h"

IarmTimerStatusSubscriber* IarmTimerStatusSubscriber::pInstance = NULL;
IarmTimerStatusSubscriber::IarmTimerStatusSubscriber(string sub):IarmSubscriber(sub)
{
   int registered=0;
   if (IARM_Bus_IsConnected(m_subscriber.c_str(),&registered) != IARM_RESULT_SUCCESS) {
      IARM_Bus_Init(m_subscriber.c_str());
      IARM_Bus_Connect();
   }
   IarmTimerStatusSubscriber::pInstance = this;
}

bool IarmTimerStatusSubscriber::subscribe(string eventname,funcPtr fptr)
{
   RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:IARMBUS Registering function for Event = %s \n",__FUNCTION__,__LINE__,eventname.c_str());

   bool retCode = false;

   if (TIMER_STATUS_MSG == eventname) 
   {
      retCode = IARM_Bus_RegisterCall(eventname.c_str(),(IARM_BusCall_t)fptr);
   }
   else
   {
   	RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:Event name Matching Failed for TIMER STATUS\n",__FUNCTION__,__LINE__);
   }
   RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:IarmTimerStatusSubscriber subscribe retCode = %d \n",__FUNCTION__,__LINE__,retCode);
   return retCode;
}


