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
#ifndef _IARMTIMERSTATUSSUBSCRIBER_H_
#define _IARMTIMERSTATUSSUBSCRIBER_H_

#include <string>
#include "isubscribe.h"
#include "iarmsubscribe.h"

//Although we could use  Lambda functions and higher order functions(which are available from c++11 onwards.)
//Since the registration functions typically c functions. 
//Perhaps this can be cosidered at a later point of time.

using namespace std;
class IarmTimerStatusSubscriber:public IarmSubscriber
{
	public:
		IarmTimerStatusSubscriber(string sub);
		bool subscribe(string eventname,funcPtr fptr);
};

#endif// _IARMTIMERSTATUSSUBSCRIBER_H_
