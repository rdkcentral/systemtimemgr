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
#define TIMER_STATUS_MSG    "TIMER_STATUS"
#define POWER_CHANGE_MSG    "POWER_CHANGE"
#define INTERNET_STATUS_MSG "INTERNET_STATUS"
#define cTIMER_STATUS_MESSAGE_LENGTH  80
#define cINTERNET_STATUS_LENGTH       32
#define cINTERFACE_NAME_LENGTH        16

/* Internet connectivity states (matches INetworkManager::InternetStatus enum) */
#define INTERNET_STATUS_NO_INTERNET     0
#define INTERNET_STATUS_LIMITED         1
#define INTERNET_STATUS_CAPTIVE_PORTAL  2
#define INTERNET_STATUS_FULLY_CONNECTED 3
#define INTERNET_STATUS_UNKNOWN         4

const int cTIMER_STATUS_UPDATE = 0;

typedef struct
{
	int event;
	int quality;
	char timerSrc[cTIMER_STATUS_MESSAGE_LENGTH];
	char currentTime[cTIMER_STATUS_MESSAGE_LENGTH];
	char  message[cTIMER_STATUS_MESSAGE_LENGTH];
}TimerMsg;

/*
 * Data structure carrying the onInternetStatusChange event payload from the
 * NetworkManager Thunder plugin.  Fields match the JSON params documented at:
 * https://github.com/rdkcentral/networkmanager/blob/develop/docs/NetworkManagerPlugin.md#event.onInternetStatusChange
 */
typedef struct
{
    int  prevState;                              /* previous internet connectivity state */
    char prevStatus[cINTERNET_STATUS_LENGTH];    /* e.g. "NO_INTERNET" */
    int  state;                                  /* current internet connectivity state */
    char status[cINTERNET_STATUS_LENGTH];        /* e.g. "FULLY_CONNECTED" */
    char interface[cINTERFACE_NAME_LENGTH];      /* default interface, e.g. "eth0" */
} InternetStatusData;


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
