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
#ifndef _TESTSUBSCRIBER_H_
#define _TESTSUBSCRIBER_H_

#include <string>
#include "isubscribe.h"
#include "irdklog.h"

//Although we could use  Lambda functions and higher order functions(which are available from c++11 onwards.)
//Since the registration functions typically c functions. 
//Perhaps this can be cosidered at a later point of time.

using namespace std;
class TestSubscriber:public ISubscribe
{
	public:
		TestSubscriber(string sub):ISubscribe(std::move(sub)){}
		bool subscribe(string eventname,funcPtr fptr)
		{
			RDK_LOG(RDK_LOG_INFO,LOG_SYSTIME,"[%s:%d]:Subscribed for Event = %s \n",__FUNCTION__,__LINE__,eventname.c_str());
			return true;
		}
};


#endif// _TESTSUBSCRIBER_H_
