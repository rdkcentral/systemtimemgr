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
#ifndef _NETWORKSTATUSSRC_H_
#define _NETWORKSTATUSSRC_H_


#include <WPEFramework/core/core.h>
#include <WPEFramework/plugins/Service.h>

using namespace WPEFramework;

class NetworkStatusSrc
{

        public:
                   NetworkStatusSrc();

    // Declaration, NO initialization
    static WPEFramework::JSONRPC::LinkType<WPEFramework::Core::JSON::IElement>* controller;
    static WPEFramework::JSONRPC::LinkType<WPEFramework::Core::JSON::IElement>* thunder_client;

    static bool m_networkeventsubscribed;
                void subscribeInternetStatusEvent(); 
               ~NetworkStatusSrc();
                static void subscribeToInternetEvent();
                static void unsubscribeFromInternetEvent();
                static void plugin_statechange(const JsonObject& parameters);
};

#endif // _NETWORKSTATUSSRC_H_
