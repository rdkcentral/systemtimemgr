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

/*
 * NetworkMgrSubscriber
 *
 * Subscribes to the org.rdk.NetworkManager Thunder plugin's
 * onInternetStatusChange notification via the WPEFramework JSON-RPC WebSocket
 * endpoint.  Uses a minimal POSIX WebSocket client so no extra library
 * dependency (beyond libc/libpthread) is introduced.
 *
 * Protocol flow
 * -------------
 * 1. TCP-connect to 127.0.0.1:9998
 * 2. HTTP/1.1 Upgrade handshake (RFC 6455)
 * 3. Send masked JSON-RPC "register" call for onInternetStatusChange
 * 4. Loop: receive WebSocket frames, dispatch text payloads to handler
 * 5. On disconnect: wait RECONNECT_DELAY_SEC and retry from step 1
 */

#include "networkmgrsubscriber.h"
#include "irdklog.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <chrono>

/* ── Thunder endpoint configuration ─────────────────────────────────────── */
#define NM_THUNDER_HOST     "127.0.0.1"
#define NM_THUNDER_PORT     9998
#define NM_THUNDER_PATH     "/jsonrpc"

/*
 * JSON-RPC 2.0 subscription registration call sent to Thunder after the
 * WebSocket upgrade completes.  The "id" field "client.systimemgr.1" is the
 * client-side event namespace that Thunder will prefix on delivered events.
 */
#define NM_SUBSCRIBE_PAYLOAD \
    "{\"jsonrpc\":\"2.0\",\"id\":1," \
    "\"method\":\"org.rdk.NetworkManager.1.register\"," \
    "\"params\":{\"event\":\"onInternetStatusChange\"," \
               "\"id\":\"client.systimemgr.1\"}}"

/*
 * The method string Thunder uses when delivering the event back:
 *   "client.systimemgr.1.onInternetStatusChange"
 */
#define NM_EVENT_METHOD_SUBSTR "onInternetStatusChange"

/* WebSocket frame opcodes (RFC 6455 §5.2) */
#define WS_OP_CONTINUATION  0x00u
#define WS_OP_TEXT          0x01u
#define WS_OP_BINARY        0x02u
#define WS_OP_CLOSE         0x08u
#define WS_OP_PING          0x09u
#define WS_OP_PONG          0x0Au

/* Maximum plausible event payload size – protects against malformed frames */
#define WS_MAX_PAYLOAD_BYTES 4096u

/* Seconds to wait before reconnect attempt */
#define RECONNECT_DELAY_SEC  5

/* ── Static storage ──────────────────────────────────────────────────────── */
NetworkMgrSubscriber* NetworkMgrSubscriber::pInstance = nullptr;

/* ── Constructor / Destructor ────────────────────────────────────────────── */

NetworkMgrSubscriber::NetworkMgrSubscriber(string sub)
    : ISubscribe(std::move(sub))
    , m_internetStatusHandler(nullptr)
    , m_sockFd(-1)
    , m_running(false)
    , m_wsConnected(false)
{
    RDK_LOG(RDK_LOG_DEBUG, LOG_SYSTIME, "[%s:%d]: Entry\n", __FUNCTION__, __LINE__);
    pInstance = this;
    RDK_LOG(RDK_LOG_DEBUG, LOG_SYSTIME, "[%s:%d]: Exit\n", __FUNCTION__, __LINE__);
}

NetworkMgrSubscriber::~NetworkMgrSubscriber()
{
    RDK_LOG(RDK_LOG_DEBUG, LOG_SYSTIME, "[%s:%d]: Entry\n", __FUNCTION__, __LINE__);
    m_running = false;
    /* Closing the socket makes recv() return immediately, unblocking eventLoop */
    if (m_sockFd >= 0)
    {
        close(m_sockFd);
        m_sockFd = -1;
    }
    if (m_eventThread.joinable())
    {
        m_eventThread.join();
    }
    RDK_LOG(RDK_LOG_DEBUG, LOG_SYSTIME, "[%s:%d]: Exit\n", __FUNCTION__, __LINE__);
}

/* ── ISubscribe::subscribe ───────────────────────────────────────────────── */

bool NetworkMgrSubscriber::subscribe(string eventname, funcPtr fptr)
{
    RDK_LOG(RDK_LOG_DEBUG, LOG_SYSTIME,
            "[%s:%d]: Registering handler for event=\"%s\"\n",
            __FUNCTION__, __LINE__, eventname.c_str());

    if (eventname != INTERNET_STATUS_MSG)
    {
        RDK_LOG(RDK_LOG_WARN, LOG_SYSTIME,
                "[%s:%d]: Unknown event \"%s\" – only %s is supported\n",
                __FUNCTION__, __LINE__, eventname.c_str(), INTERNET_STATUS_MSG);
        return false;
    }

    m_internetStatusHandler = fptr;
    m_running               = true;

    m_eventThread = std::thread(&NetworkMgrSubscriber::eventLoop, this);
    bool ok = m_eventThread.joinable();
    if (ok)
    {
        m_eventThread.detach();
        RDK_LOG(RDK_LOG_INFO, LOG_SYSTIME,
                "[%s:%d]: NetworkMgr event thread started\n",
                __FUNCTION__, __LINE__);
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_SYSTIME,
                "[%s:%d]: Failed to start NetworkMgr event thread\n",
                __FUNCTION__, __LINE__);
        m_running = false;
    }

    return ok;
}

/* ── Handler invocation ──────────────────────────────────────────────────── */

void NetworkMgrSubscriber::invokeInternetStatusHandler(const InternetStatusData& data)
{
    if (m_internetStatusHandler)
    {
        /* Pass a local copy so the handler sees a stable pointer */
        InternetStatusData copy = data;
        (*m_internetStatusHandler)(static_cast<void*>(&copy));
    }
}

/* ── Low-level socket helpers ────────────────────────────────────────────── */

/*
 * Blocking receive: reads exactly 'len' bytes into 'buf'.
 * Returns true on success, false if the connection was closed or an error
 * occurred.
 */
bool NetworkMgrSubscriber::recvAll(uint8_t* buf, size_t len)
{
    size_t total = 0;
    while (total < len)
    {
        ssize_t n = recv(m_sockFd, buf + total, len - total, 0);
        if (n <= 0)
            return false;
        total += static_cast<size_t>(n);
    }
    return true;
}

/*
 * Build and send a masked WebSocket text frame (client→server frames MUST be
 * masked per RFC 6455 §5.3).
 */
bool NetworkMgrSubscriber::sendWsTextFrame(const std::string& payload)
{
    size_t payloadLen = payload.size();

    /* Fixed masking key – statically chosen.  For a loopback connection this
     * is sufficient; the masking requirement exists to protect intermediary
     * proxies, not for cryptographic privacy. */
    static const uint8_t MASK[4] = {0xA1u, 0xB2u, 0xC3u, 0xD4u};

    std::vector<uint8_t> frame;
    frame.reserve(payloadLen + 12);

    /* Byte 0: FIN=1, opcode=text */
    frame.push_back(0x81u);

    /* Bytes 1-N: MASK bit + payload length */
    if (payloadLen < 126u)
    {
        frame.push_back(static_cast<uint8_t>(0x80u | payloadLen));
    }
    else if (payloadLen < 65536u)
    {
        frame.push_back(0xFEu);   /* 0x80 | 0x7E */
        frame.push_back(static_cast<uint8_t>((payloadLen >> 8u) & 0xFFu));
        frame.push_back(static_cast<uint8_t>(payloadLen & 0xFFu));
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_SYSTIME,
                "[%s:%d]: Payload too large (%zu bytes)\n",
                __FUNCTION__, __LINE__, payloadLen);
        return false;
    }

    /* Masking key */
    frame.push_back(MASK[0]);
    frame.push_back(MASK[1]);
    frame.push_back(MASK[2]);
    frame.push_back(MASK[3]);

    /* Masked payload */
    for (size_t i = 0; i < payloadLen; ++i)
    {
        frame.push_back(static_cast<uint8_t>(payload[i]) ^ MASK[i % 4u]);
    }

    ssize_t sent = send(m_sockFd, frame.data(), frame.size(), MSG_NOSIGNAL);
    return (sent == static_cast<ssize_t>(frame.size()));
}

/*
 * Send a masked WebSocket PONG control frame in response to a PING.
 * PING payloads are at most 125 bytes per RFC 6455 §5.5.
 */
void NetworkMgrSubscriber::sendPong(const uint8_t* pingData, size_t dataLen)
{
    if (dataLen > 125u)
        return;

    static const uint8_t MASK[4] = {0xCAu, 0xFEu, 0xBAu, 0xBEu};

    uint8_t frame[4 + 4 + 125];   /* header + mask + payload */
    frame[0] = 0x8Au;              /* FIN=1, opcode=PONG */
    frame[1] = static_cast<uint8_t>(0x80u | dataLen);
    frame[2] = MASK[0]; frame[3] = MASK[1];
    frame[4] = MASK[2]; frame[5] = MASK[3];

    for (size_t i = 0; i < dataLen; ++i)
        frame[6 + i] = pingData[i] ^ MASK[i % 4u];

    send(m_sockFd, frame, 6u + dataLen, MSG_NOSIGNAL);
}

/* ── JSON extraction helpers (avoids a jsoncpp/nlohmann dependency) ──────── */

/*
 * Extract the value of a JSON string field "key":"<value>".
 * Returns "" if the key is not found.
 */
static std::string jsonGetString(const std::string& json, const std::string& key)
{
    std::string needle = "\"" + key + "\":\"";
    size_t pos = json.find(needle);
    if (pos == std::string::npos)
        return "";
    pos += needle.size();
    size_t end = json.find('"', pos);
    return (end == std::string::npos) ? "" : json.substr(pos, end - pos);
}

/*
 * Extract the value of a JSON integer field "key":<value>.
 * Returns defVal if the key is not found or the value is not a number.
 */
static int jsonGetInt(const std::string& json, const std::string& key, int defVal = -1)
{
    std::string needle = "\"" + key + "\":";
    size_t pos = json.find(needle);
    if (pos == std::string::npos)
        return defVal;
    pos += needle.size();
    /* Skip optional whitespace */
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t'))
        ++pos;
    if (pos >= json.size())
        return defVal;
    bool negative = (json[pos] == '-');
    if (negative) ++pos;
    int val = 0;
    bool hasDigit = false;
    while (pos < json.size() && json[pos] >= '0' && json[pos] <= '9')
    {
        val = val * 10 + (json[pos] - '0');
        ++pos;
        hasDigit = true;
    }
    return hasDigit ? (negative ? -val : val) : defVal;
}

/* ── Event JSON dispatcher ───────────────────────────────────────────────── */

/*
 * Parse a JSON payload and, if it is an onInternetStatusChange notification,
 * fill an InternetStatusData struct and invoke the registered funcPtr handler.
 */
void NetworkMgrSubscriber::handleJsonEvent(const std::string& jsonStr)
{
    RDK_LOG(RDK_LOG_DEBUG, LOG_SYSTIME,
            "[%s:%d]: Received WS payload: %s\n",
            __FUNCTION__, __LINE__, jsonStr.c_str());

    /* Ignore subscribe-ack ("id":1 with "result" field) and other methods */
    if (jsonStr.find(NM_EVENT_METHOD_SUBSTR) == std::string::npos)
    {
        RDK_LOG(RDK_LOG_DEBUG, LOG_SYSTIME,
                "[%s:%d]: Not an onInternetStatusChange notification, ignoring\n",
                __FUNCTION__, __LINE__);
        return;
    }

    InternetStatusData data;
    memset(&data, 0, sizeof(data));

    data.prevState = jsonGetInt(jsonStr, "prevState");
    data.state     = jsonGetInt(jsonStr, "state");

    std::string prevStatus = jsonGetString(jsonStr, "prevStatus");
    std::string status     = jsonGetString(jsonStr, "status");
    std::string iface      = jsonGetString(jsonStr, "interface");

    snprintf(data.prevStatus, sizeof(data.prevStatus), "%s", prevStatus.c_str());
    snprintf(data.status,     sizeof(data.status),     "%s", status.c_str());
    snprintf(data.interface,  sizeof(data.interface),  "%s", iface.c_str());

    RDK_LOG(RDK_LOG_INFO, LOG_SYSTIME,
            "[%s:%d]: onInternetStatusChange: "
            "prevState=%d prevStatus=\"%s\" state=%d status=\"%s\" interface=\"%s\"\n",
            __FUNCTION__, __LINE__,
            data.prevState, data.prevStatus,
            data.state,     data.status,
            data.interface);

    invokeInternetStatusHandler(data);
}

/* ── WebSocket event loop ────────────────────────────────────────────────── */

/*
 * Background thread: establishes the WebSocket connection to Thunder, sends
 * the subscription registration, and enters a receive loop.  Automatically
 * reconnects on any failure.
 */
void NetworkMgrSubscriber::eventLoop()
{
    RDK_LOG(RDK_LOG_INFO, LOG_SYSTIME,
            "[%s:%d]: NetworkMgr subscriber event loop started\n",
            __FUNCTION__, __LINE__);

    while (m_running)
    {
        /* ── 1. Create TCP socket ─────────────────────────────────────── */
        m_sockFd = socket(AF_INET, SOCK_STREAM, 0);
        if (m_sockFd < 0)
        {
            RDK_LOG(RDK_LOG_ERROR, LOG_SYSTIME,
                    "[%s:%d]: socket() failed, retrying in %ds\n",
                    __FUNCTION__, __LINE__, RECONNECT_DELAY_SEC);
            std::this_thread::sleep_for(
                std::chrono::seconds(RECONNECT_DELAY_SEC));
            continue;
        }

        /* ── 2. Connect to Thunder ────────────────────────────────────── */
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port   = htons(NM_THUNDER_PORT);
        inet_pton(AF_INET, NM_THUNDER_HOST, &addr.sin_addr);

        if (connect(m_sockFd,
                    reinterpret_cast<struct sockaddr*>(&addr),
                    sizeof(addr)) < 0)
        {
            RDK_LOG(RDK_LOG_WARN, LOG_SYSTIME,
                    "[%s:%d]: connect() to Thunder failed, retrying in %ds\n",
                    __FUNCTION__, __LINE__, RECONNECT_DELAY_SEC);
            close(m_sockFd);
            m_sockFd = -1;
            std::this_thread::sleep_for(
                std::chrono::seconds(RECONNECT_DELAY_SEC));
            continue;
        }

        /* ── 3. HTTP/1.1 WebSocket upgrade handshake ─────────────────── */
        std::string upgradeReq =
            "GET " NM_THUNDER_PATH " HTTP/1.1\r\n"
            "Host: " NM_THUNDER_HOST ":" + std::to_string(NM_THUNDER_PORT) + "\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Key: c3lzVGltZU1nclN1YnNjcmliZQ==\r\n"
            "Sec-WebSocket-Version: 13\r\n"
            "\r\n";

        if (send(m_sockFd, upgradeReq.c_str(), upgradeReq.size(), MSG_NOSIGNAL) < 0)
        {
            RDK_LOG(RDK_LOG_ERROR, LOG_SYSTIME,
                    "[%s:%d]: Failed to send WebSocket upgrade request\n",
                    __FUNCTION__, __LINE__);
            close(m_sockFd);
            m_sockFd = -1;
            std::this_thread::sleep_for(
                std::chrono::seconds(RECONNECT_DELAY_SEC));
            continue;
        }

        /* Read HTTP response – look for "101 Switching Protocols" */
        char respBuf[1024] = {'\0'};
        ssize_t recvLen = recv(m_sockFd, respBuf, sizeof(respBuf) - 1u, 0);
        if (recvLen <= 0 || strstr(respBuf, "101") == nullptr)
        {
            RDK_LOG(RDK_LOG_ERROR, LOG_SYSTIME,
                    "[%s:%d]: WebSocket upgrade failed (response: %.120s)\n",
                    __FUNCTION__, __LINE__, respBuf);
            close(m_sockFd);
            m_sockFd = -1;
            std::this_thread::sleep_for(
                std::chrono::seconds(RECONNECT_DELAY_SEC));
            continue;
        }

        RDK_LOG(RDK_LOG_INFO, LOG_SYSTIME,
                "[%s:%d]: WebSocket connected to Thunder at %s:%d\n",
                __FUNCTION__, __LINE__, NM_THUNDER_HOST, NM_THUNDER_PORT);

        /* ── 4. Register for onInternetStatusChange event ────────────── */
        if (!sendWsTextFrame(NM_SUBSCRIBE_PAYLOAD))
        {
            RDK_LOG(RDK_LOG_ERROR, LOG_SYSTIME,
                    "[%s:%d]: Failed to send subscribe payload\n",
                    __FUNCTION__, __LINE__);
            close(m_sockFd);
            m_sockFd = -1;
            std::this_thread::sleep_for(
                std::chrono::seconds(RECONNECT_DELAY_SEC));
            continue;
        }

        m_wsConnected = true;
        RDK_LOG(RDK_LOG_INFO, LOG_SYSTIME,
                "[%s:%d]: Subscribed to onInternetStatusChange from NetworkManager\n",
                __FUNCTION__, __LINE__);

        /* ── 5. Receive loop ─────────────────────────────────────────── */
        bool connectionOk = true;
        while (m_running && connectionOk)
        {
            /* Read the 2-byte base header */
            uint8_t hdr[2];
            if (!recvAll(hdr, 2u))
            {
                RDK_LOG(RDK_LOG_WARN, LOG_SYSTIME,
                        "[%s:%d]: WebSocket connection closed by peer\n",
                        __FUNCTION__, __LINE__);
                connectionOk = false;
                break;
            }

            uint8_t  opcode     = hdr[0] & 0x0Fu;
            bool     fin        = (hdr[0] & 0x80u) != 0u;
            bool     masked     = (hdr[1] & 0x80u) != 0u;
            uint64_t payloadLen = hdr[1] & 0x7Fu;

            /* Extended payload length */
            if (payloadLen == 126u)
            {
                uint8_t lenBuf[2];
                if (!recvAll(lenBuf, 2u)) { connectionOk = false; break; }
                payloadLen = (static_cast<uint64_t>(lenBuf[0]) << 8u)
                           |  static_cast<uint64_t>(lenBuf[1]);
            }
            else if (payloadLen == 127u)
            {
                uint8_t lenBuf[8];
                if (!recvAll(lenBuf, 8u)) { connectionOk = false; break; }
                payloadLen = 0u;
                for (int i = 0; i < 8; ++i)
                    payloadLen = (payloadLen << 8u) | lenBuf[i];
            }

            /* Optional masking key (server→client frames should not be masked,
             * but handle gracefully just in case) */
            uint8_t maskKey[4] = {0u, 0u, 0u, 0u};
            if (masked)
            {
                if (!recvAll(maskKey, 4u)) { connectionOk = false; break; }
            }

            /* Guard against unreasonably large frames */
            if (payloadLen > WS_MAX_PAYLOAD_BYTES)
            {
                RDK_LOG(RDK_LOG_WARN, LOG_SYSTIME,
                        "[%s:%d]: Oversized WS frame (%llu bytes) – draining\n",
                        __FUNCTION__, __LINE__,
                        static_cast<unsigned long long>(payloadLen));
                /* Drain to stay in sync with the stream */
                uint64_t remaining = payloadLen;
                while (remaining > 0u && m_running)
                {
                    uint8_t drain[256];
                    size_t  chunk = (remaining < sizeof(drain))
                                        ? static_cast<size_t>(remaining)
                                        : sizeof(drain);
                    if (recv(m_sockFd, drain, chunk, 0) <= 0)
                    {
                        connectionOk = false;
                        break;
                    }
                    remaining -= chunk;
                }
                continue;
            }

            /* Read payload bytes */
            std::vector<uint8_t> payload(static_cast<size_t>(payloadLen));
            if (payloadLen > 0u)
            {
                if (!recvAll(payload.data(), static_cast<size_t>(payloadLen)))
                {
                    connectionOk = false;
                    break;
                }
                /* Unmask payload if the server (incorrectly) masked it */
                if (masked)
                {
                    for (size_t i = 0; i < static_cast<size_t>(payloadLen); ++i)
                        payload[i] ^= maskKey[i % 4u];
                }
            }

            /* Dispatch by opcode */
            switch (opcode)
            {
            case WS_OP_CLOSE:
                RDK_LOG(RDK_LOG_INFO, LOG_SYSTIME,
                        "[%s:%d]: Received WebSocket CLOSE frame\n",
                        __FUNCTION__, __LINE__);
                connectionOk = false;
                break;

            case WS_OP_PING:
                RDK_LOG(RDK_LOG_DEBUG, LOG_SYSTIME,
                        "[%s:%d]: Received WebSocket PING – sending PONG\n",
                        __FUNCTION__, __LINE__);
                sendPong(payload.data(), payload.size());
                break;

            case WS_OP_TEXT:
            case WS_OP_CONTINUATION:
                /* Only handle single-frame (FIN=1) text messages */
                if (fin)
                {
                    std::string jsonStr(payload.begin(), payload.end());
                    handleJsonEvent(jsonStr);
                }
                else
                {
                    RDK_LOG(RDK_LOG_WARN, LOG_SYSTIME,
                            "[%s:%d]: Fragmented WS frame – not supported, skipping\n",
                            __FUNCTION__, __LINE__);
                }
                break;

            default:
                /* Ignore binary frames and unknown opcodes */
                RDK_LOG(RDK_LOG_DEBUG, LOG_SYSTIME,
                        "[%s:%d]: Ignored WS frame opcode=0x%02x\n",
                        __FUNCTION__, __LINE__, opcode);
                break;
            }
        } /* receive loop */

        m_wsConnected = false;
        close(m_sockFd);
        m_sockFd = -1;

        if (m_running)
        {
            RDK_LOG(RDK_LOG_INFO, LOG_SYSTIME,
                    "[%s:%d]: Reconnecting to Thunder in %ds...\n",
                    __FUNCTION__, __LINE__, RECONNECT_DELAY_SEC);
            std::this_thread::sleep_for(
                std::chrono::seconds(RECONNECT_DELAY_SEC));
        }
    } /* reconnect loop */

    RDK_LOG(RDK_LOG_INFO, LOG_SYSTIME,
            "[%s:%d]: NetworkMgr subscriber event loop exited\n",
            __FUNCTION__, __LINE__);
}
