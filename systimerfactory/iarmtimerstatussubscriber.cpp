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
#include "iarmtimerstatussubscriber.h"
#include "libIARM.h"
#include "libIBus.h"
#include "libIARMCore.h"
#include "irdklog.h"

IarmTimerStatusSubscriber::IarmTimerStatusSubscriber(string sub):IarmSubscriber(sub)
{
   RDK_LOG(RDK_LOG_DEBUG,LOG_SYSTIME,"[%s:%d]:Entry\n",__FUNCTION__,__LINE__);
   int registered=0;
   if (IARM_Bus_IsConnected(m_subscriber.c_str(),&registered) != IARM_RESULT_SUCCESS) {
      IARM_Result_t init_result = IARM_Bus_Init(m_subscriber.c_str());
      if (init_result != IARM_RESULT_SUCCESS) {
         RDK_LOG(RDK_LOG_ERROR,LOG_SYSTIME,"[%s:%d]:IARM_Bus_Init failed with result %d\n",__FUNCTION__,__LINE__,init_result);
         return;
      }

      IARM_Result_t connect_result = IARM_Bus_Connect();
      if (connect_result != IARM_RESULT_SUCCESS) {
         RDK_LOG(RDK_LOG_ERROR,LOG_SYSTIME,"[%s:%d]:IARM_Bus_Connect failed with result %d\n",__FUNCTION__,__LINE__,connect_result);
         return;
      }

      RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:IARM bus initialized and connected successfully\n",__FUNCTION__,__LINE__);
   }
   else
   {
      RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:IarmTimerStatusSubscriber IARM_Bus_IsConnected Success \n",__FUNCTION__,__LINE__);
   }
   RDK_LOG(RDK_LOG_DEBUG,LOG_SYSTIME,"[%s:%d]:Exit\n",__FUNCTION__,__LINE__);
}

bool IarmTimerStatusSubscriber::subscribe(string eventname,funcPtr fptr)
{
   RDK_LOG(RDK_LOG_DEBUG,LOG_SYSTIME,"[%s:%d]:IARMBUS Registering function for Event = %s \n",__FUNCTION__,__LINE__,eventname.c_str());

   bool retCode = false;

   if (TIMER_STATUS_MSG == eventname) 
   {
      retCode = (IARM_Bus_RegisterCall(eventname.c_str(),(IARM_BusCall_t)fptr) == IARM_RESULT_SUCCESS);
   }
   else
   {
   	RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:Event name Matching Failed for TIMER STATUS\n",__FUNCTION__,__LINE__);
   }
   RDK_LOG(RDK_LOG_DEBUG,LOG_SYSTIME,"[%s:%d]:IarmTimerStatusSubscriber subscribe retCode = %d \n",__FUNCTION__,__LINE__,retCode);
   return retCode;
}


