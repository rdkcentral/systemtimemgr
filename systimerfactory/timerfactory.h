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
#ifndef __TIMERFACTORY_H_
#define __TIMERFACTORY_H_

#include "itimesrc.h"
#include "itimesync.h"
#include "ipublish.h"
#include "isubscribe.h"
#include <string>

using namespace std;

ITimeSrc* createTimeSrc(string type, string args);
ITimeSync* createTimeSync(string type, string args);
IPublish* createPublish(string type, string args);
ISubscribe* createSubscriber(string type, string args);


#endif// __TIMERFACTORY_H_
