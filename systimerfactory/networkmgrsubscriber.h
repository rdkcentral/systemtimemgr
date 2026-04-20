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

#ifndef _NETWORKMGRSUBSCRIBER_H_
#define _NETWORKMGRSUBSCRIBER_H_

#include <string>
#include <atomic>
#include <thread>
#include <vector>

#include "isubscribe.h"
#include "itimermsg.h"

using namespace std;

/*
 * NetworkMgrSubscriber
 *
 * Subscribes to the org.rdk.NetworkManager Thunder plugin's
 * onInternetStatusChange event over the WPEFramework JSON-RPC WebSocket
 * endpoint (ws://127.0.0.1:9998/jsonrpc).
 *
 * It is intentionally independent of the IARM bus; the connection uses a
 * plain POSIX TCP socket with a minimal WebSocket handshake so that no extra
 * compile-time library dependency beyond POSIX is required.
 *
 * Subscription registration JSON-RPC call:
 *   {"jsonrpc":"2.0","id":1,"method":"org.rdk.NetworkManager.1.register",
 *    "params":{"event":"onInternetStatusChange","id":"client.systimemgr.1"}}
 *
 * Event notification received:
 *   {"jsonrpc":"2.0",
 *    "method":"client.systimemgr.1.onInternetStatusChange",
 *    "params":{"prevState":<int>,"prevStatus":"<str>",
 *              "state":<int>,"status":"<str>","interface":"<str>"}}
 *
 * The subscriber's subscribe() method starts a background thread that
 * manages the WebSocket connection and auto-reconnects on failure.
 */
class NetworkMgrSubscriber : public ISubscribe
{
private:
    funcPtr          m_internetStatusHandler;
    int              m_sockFd;
    std::thread      m_eventThread;
    std::atomic<bool> m_running;
    std::atomic<bool> m_wsConnected;

    static NetworkMgrSubscriber* pInstance;

    /* Background thread: connect → upgrade → subscribe → receive loop */
    void eventLoop();

    /* Dispatch a received JSON payload to the registered handler */
    void handleJsonEvent(const std::string& payload);

    /* Build and send a masked WebSocket text frame */
    bool sendWsTextFrame(const std::string& payload);

    /* Send a WebSocket PONG control frame (response to PING) */
    void sendPong(const uint8_t* pingData, size_t dataLen);

    /* Perform a blocking recv loop until exactly 'len' bytes are read */
    bool recvAll(uint8_t* buf, size_t len);

public:
    explicit NetworkMgrSubscriber(string sub);
    ~NetworkMgrSubscriber();

    /* ISubscribe interface – registers for INTERNET_STATUS_MSG events */
    bool subscribe(string eventname, funcPtr fptr) override;

    /* Invoke the registered handler with a copy of the event data */
    void invokeInternetStatusHandler(const InternetStatusData& data);

    static NetworkMgrSubscriber* getInstance() { return pInstance; }

#ifdef GTEST_ENABLE
    friend class NetworkMgrSubscriberTest;
#endif
};

#endif /* _NETWORKMGRSUBSCRIBER_H_ */
