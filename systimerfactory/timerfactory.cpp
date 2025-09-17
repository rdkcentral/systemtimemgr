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
#include "ntptimesrc.h"
#include "stttimesrc.h"
#include "regulartimesrc.h"
#include "testtimesync.h"
#include "rdkdefaulttimesync.h"
#include "drmtimersrc.h"
#ifdef DTT_ENABLED
#include "dtttimersrc.h"
#endif// DTT_ENABLED
#ifdef TEE_ENABLED
#include "teetimesync.h"
#endif //TEE_ENABLED

ITimeSrc* createTimeSrc(string type, string args)
{
	ITimeSrc* ret = NULL;
	if (type == "ntp")
	{
		ret = new NtpTimeSrc();
	}
	else if (type == "stt")
	{
		ret = new SttTimeSrc();
	}
	else if (type == "regular")
	{
		ret = new RegularTimeSrc(std::move(args));
	}
        else if (type == "drm")
        {
		ret = new DrmTimeSrc(std::move(args));
        }
#ifdef DTT_ENABLED
        else if (type == "dtt")
        {
		ret = new DttTimeSrc(std::move(args));
        }
#endif// DTT_ENABLED

	return ret;
}
ITimeSync* createTimeSync(string type, string args)
{
	ITimeSync* ret = NULL;
	if (type == "test")
	{
		ret = new TestTimeSync(std::move(args));
	}
	else if(type == "rdkdefault")
	{
		ret = new RdkDefaultTimeSync();
	}
#ifdef TEE_ENABLED
	else if (type == "tee")
	{
		ret = new TeeTimeSync();
	}
#endif	//TEE_ENABLED

	return ret;
}
