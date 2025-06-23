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

#include "TestIarmInterface.h"

#if defined(L2_ENABLED)
IPublish* createPublishTest(string type, string args)
{
    IPublish* ret = NULL;
    RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]: Test Function for Publish Status Msg \n",__FUNCTION__,__LINE__);
    return ret;
}

ISubscribe* createSubscriberTest(string type, string args, string subtype)
{
        ISubscribe* ret = NULL;
        if (type == "test")
        {
                ret = new TestSubscriber(std::move(args));
        }
        else if (type == "iarm") // CID 277707 : Resource leak (RESOURCE_LEAK)
        {
                if(TIMER_STATUS_MSG == subtype)
                {
                        RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:createSubscriber for IarmTimerStatusSubscriber\n",__FUNCTION__,__LINE__);
                        ret = new IarmTimerStatusSubscriber(std::move(args));
                }
        }
        return ret;
}

IarmTimerStatusSubscriber::IarmTimerStatusSubscriber(string sub):IarmSubscriber(std::move(sub))
{
      RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:IarmTimerStatusSubscriber IARM_Bus_IsConnected Success \n",__FUNCTION__,__LINE__);
}

IarmSubscriber* IarmSubscriber::pInstance = NULL;
IarmSubscriber::IarmSubscriber(string sub):ISubscribe(std::move(sub))
{
        RDK_LOG(RDK_LOG_DEBUG,LOG_SYSTIME,"[%s:%d]:Entry\n",__FUNCTION__,__LINE__);
        IarmSubscriber::pInstance = this;
        RDK_LOG(RDK_LOG_DEBUG,LOG_SYSTIME,"[%s:%d]:Exit\n",__FUNCTION__,__LINE__);
}

bool IarmTimerStatusSubscriber::subscribe(string eventname,funcPtr fptr)
{
   RDK_LOG(RDK_LOG_DEBUG,LOG_SYSTIME,"[%s:%d]:IARMBUS Registering function for Event = %s \n",__FUNCTION__,__LINE__,eventname.c_str());
   return 0;
}

void publishTest(int event, void* args)
{
   std::lock_guard<std::mutex> guard(g_instance_mutex);

   TimerMsg* pMsg = reinterpret_cast<TimerMsg*>(args);
   RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:IARM Broadcast Info: MsgType = %d, Quality = %d, Message = %s \n",__FUNCTION__,__LINE__,pMsg->event,pMsg->quality,pMsg->message);
}
#endif
