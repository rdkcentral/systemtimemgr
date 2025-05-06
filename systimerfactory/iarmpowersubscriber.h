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
#ifndef _IARMPOWERSUBSCRIBER_H_
#define _IARMPOWERSUBSCRIBER_H_

#include <string>
#include "isubscribe.h"

using namespace std;
class IarmPowerSubscriber:public IarmSubscriber
{
	private:
		funcPtr m_powerHandler;
	public:
		IarmPowerSubscriber(string sub);
		bool subscribe(string eventname,funcPtr fptr);
		void invokepowerhandler(void* args){ if (m_powerHandler) (*m_powerHandler)(args);}
		static void powereventHandler(const char *owner, int eventId, void *data, size_t len);
};

#endif// _IARMPOWERSUBSCRIBER_H_
