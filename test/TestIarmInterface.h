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
#endif//ENABLE_IARM
#include "irdklog.h"
#include "systimemgr.h"

#if defined(L2_ENABLED)
IPublish* createPublishTest(string type, string args);
ISubscribe* createSubscriberTest(string type, string args, string subtype);
void publishTest(int event, void* args);
#endif
