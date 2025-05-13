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

#ifndef _IPUBLISH_H_
#define _IPUBLISH_H_


using namespace std;
#include <string>
#include "itimermsg.h"


class IPublish
{
	protected:
		string m_publisher;
	public:
		IPublish(string pub):m_publisher(std::move(pub)) {}
		virtual void publish(int event, void* args) = 0;
};


#endif// _IPUBLISH_H_
