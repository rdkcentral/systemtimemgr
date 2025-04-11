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

#ifdef ENABLE_IARM
#include "iarmpublish.h"
#include "iarmsubscribe.h"
#include "iarmtimerstatussubscriber.h"
#ifdef ENABLE_PWRMGRPLUGIN
#include "ipowercontrollersubscriber.h"
#else
#include "iarmpowersubscriber.h"
#endif //ENABLE_PWRMGRPLUGIN
#endif//ENABLE_IARM

#
IPublish* createPublish(string type, string args)
{
	IPublish* ret = NULL;
	if (type == "test")
	{
		ret = new TestPublish(args);
	}
#ifdef ENABLE_IARM
	else if (type == "iarm") // CID 277711 : Resource leak (RESOURCE_LEAK)
	{
		ret = new IarmPublish(args);
	}
#endif//ENABLE_IARM


	return ret;
}
ISubscribe* createSubscriber(string type, string args, string subtype)
{
	ISubscribe* ret = NULL;
	if (type == "test")
	{
		ret = new TestSubscriber(args);
	}
#ifdef ENABLE_IARM
	else if (type == "iarm") // CID 277707 : Resource leak (RESOURCE_LEAK)
	{
		if(TIMER_STATUS_MSG == subtype)
		{
			ret = new IarmTimerStatusSubscriber(args);
		}
		else if(POWER_CHANGE_MSG == subtype) 
		{
#ifdef ENABLE_PWRMGRPLUGIN
			ret = new IpowerControllerSubscriber(args);
#else
			ret = new IarmPowerSubscriber(args);
#endif//ENABLE_PWRMGRPLUGIN
		}	
	}
#endif//ENABLE_IARM
	return ret;
}
