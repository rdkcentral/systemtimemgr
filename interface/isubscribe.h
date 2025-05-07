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

#ifndef _ISUBSCRIBE_H_
#define _ISUBSCRIBE_H_

#include <string>
#include <map>
#include <functional>
#include "itimermsg.h"

using namespace std;

//Although we could use  Lambda functions and higher order functions(which are available from c++11 onwards.)
//Since the registration functions typically c functions. 
//Perhaps this can be cosidered at a later point of time.
typedef int(*funcPtr)(void*);


class ISubscribe
{
        protected:
		string m_subscriber;
	public:
		ISubscribe(string sub):m_subscriber(std::move(sub)){}
		virtual bool subscribe(string eventname,funcPtr fptr) = 0;
};


#endif// _ISUBSCRIBE_H_
