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
#include "timerfactory.h"
#include "testpublish.h"
#include "testsubscribe.h"
#include <utility>

#ifdef ENABLE_IARM
#include "iarmpublish.h"
#include "iarmsubscribe.h"
#include "iarmtimerstatussubscriber.h"
#ifdef PWRMGRPLUGIN_ENABLED
#include "ipowercontrollersubscriber.h"
#else
#include "iarmpowersubscriber.h"
#endif //PWRMGRPLUGIN_ENABLED
#endif//ENABLE_IARM
#include "irdklog.h"

#
IPublish* createPublish(string type, string args)
{
	IPublish* ret = NULL;
	if (type == "test")
	{
		ret = new TestPublish(std::move(args));
	}
#ifdef ENABLE_IARM
	else if (type == "iarm") // CID 277711 : Resource leak (RESOURCE_LEAK)
	{
		ret = new IarmPublish(std::move(args));
	}
#endif//ENABLE_IARM


	return ret;
}
ISubscribe* createSubscriber(string type, string args, string subtype)
{
	ISubscribe* ret = NULL;
	if (type == "test")
	{
		ret = new TestSubscriber(std::move(args));
	}
#ifdef ENABLE_IARM
	else if (type == "iarm") // CID 277707 : Resource leak (RESOURCE_LEAK)
	{
		if(TIMER_STATUS_MSG == subtype)
		{
                        RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:createSubscriber for IarmTimerStatusSubscriber\n",__FUNCTION__,__LINE__);
			ret = new IarmTimerStatusSubscriber(std::move(args));
		}
		else if(POWER_CHANGE_MSG == subtype) 
		{
#ifdef PWRMGRPLUGIN_ENABLED
                        RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:createSubscriber for IPowerControllerSubscriber\n",__FUNCTION__,__LINE__);
			ret = new IpowerControllerSubscriber(std::move(args));
#else
                        RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:createSubscriber for IarmPowerSubscriber\n",__FUNCTION__,__LINE__);
			ret = new IarmPowerSubscriber(std::move(args));
#endif//PWRMGRPLUGIN_ENABLED
		}	
	}
#endif//ENABLE_IARM
	return ret;
}
