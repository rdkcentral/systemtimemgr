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

#ifndef _ITIMERMSG_H_
#define _ITIMERMSG_H_
#ifdef __cplusplus
extern "C" {
#endif


#define IARM_BUS_SYSTIME_MGR_NAME "SystemTimeMgr"
#define TIMER_STATUS_MSG  "TIMER_STATUS"
#define POWER_CHANGE_MSG  "POWER_CHANGE"
#define cTIMER_STATUS_MESSAGE_LENGTH  80

const int cTIMER_STATUS_UPDATE = 0;

typedef struct
{
	int event;
	int quality;
	char timerSrc[cTIMER_STATUS_MESSAGE_LENGTH];
	char currentTime[cTIMER_STATUS_MESSAGE_LENGTH];
	char  message[cTIMER_STATUS_MESSAGE_LENGTH];
}TimerMsg;


typedef enum
{
	ePUBLISH_TIME_INITIAL =0,
	ePUBLISH_NTP_FAIL,
	ePUBLISH_NTP_SUCCESS,
	ePUBLISH_DTT_SUCCESS,
	ePUBLISH_SECURE_TIME_SUCCESS,
	ePUBLISH_DEEP_SLEEP_ON,
}publishEvent;

typedef enum
{
   eTIMEQUALILTY_POOR =0,
   eTIMEQUALILTY_GOOD,
   eTIMEQUALILTY_SECURE,
   eTIMEQUALILTY_UNKNOWN,
} qualityOfTime;


#ifdef __cplusplus
}
#endif
#endif// _TIMERMSGIFC_H_
