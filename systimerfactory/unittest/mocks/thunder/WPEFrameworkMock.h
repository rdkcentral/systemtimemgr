/*
 * Copyright 2024 Comcast Cable Communications Management, LLC
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
 *
 * WPEFramework Thunder API stubs for __LOCAL_TEST__ builds.
 *
 * Provides the minimal WPEFramework namespace surface used by
 * networkstatussrc.cpp so the file can be compiled without a real
 * Thunder/WPEFramework installation.  Follows the same pattern as the
 * entservices-testframework Tests/mocks/thunder/ directory.
 *
 * ── Event injection for L2 tests ──────────────────────────────────────────
 *
 * When SmartLinkType::Subscribe() is called the mock:
 *   1. Creates /tmp/thunder_mock_<callsign>_<event>.subscribed   (marker)
 *   2. Spawns a polling thread watching
 *      /tmp/thunder_mock_<callsign>_<event>.inject
 *
 * L2 Python tests inject events by writing a one-line JSON object to that
 * inject file (atomic rename recommended):
 *
 *   echo '{"status":"FULLY_CONNECTED","interface":"eth0"}' \
 *       > /tmp/thunder_mock_org_rdk_NetworkManager_onInternetStatusChange.inject
 *
 * The polling thread picks it up, removes the file, parses the JSON, and
 * calls the registered C++ callback — exactly as real Thunder would after
 * receiving an onInternetStatusChange notification from the plugin.
 */

#pragma once

#include <string>
#include <map>
#include <memory>
#include <functional>
#include <thread>
#include <atomic>
#include <fstream>
#include <sstream>
#include <utility>
#include <cerrno>
#include <unistd.h>   /* usleep */
#include <sys/stat.h> /* stat   */
#include <cstdio>     /* remove, printf */

/* RDK logging stubs — irdklog.h is excluded from test builds (see the
 * #if GTEST_ENABLE / __LOCAL_TEST_ guard in networkstatussrc.cpp) so these
 * macros are the only RDK_LOG definitions in test builds.  Unconditional
 * defines ensure nothing else can leave them undefined. */
#undef  RDK_LOG
#define RDK_LOG(level, module, format, ...) do { printf("[" module "] " format, ##__VA_ARGS__); fflush(stdout); } while (0)
#undef  RDK_LOG_INFO
#define RDK_LOG_INFO  4
#undef  RDK_LOG_WARN
#define RDK_LOG_WARN  3
#undef  RDK_LOG_ERROR
#define RDK_LOG_ERROR 2
#undef  LOG_SYSTIME
#define LOG_SYSTIME "LOG.RDK.SYSTIME"

namespace WPEFramework {

// ── Core namespace ──────────────────────────────────────────────────────────

namespace Core {

/* Error codes — only ERROR_NONE is needed by networkstatussrc.cpp */
static constexpr int32_t ERROR_NONE = 0;

/* Core::SystemInfo::SetEnvironment — no-op stub (sets THUNDER_ACCESS) */
namespace SystemInfo {
    static inline void SetEnvironment(const std::string& /*key*/,
                                      const std::string& /*val*/) {}
} // namespace SystemInfo

namespace JSON {

/* Variant — holds a single string-valued JSON field */
class Variant {
public:
    Variant() = default;
    explicit Variant(const std::string& v) : value_(v) {}
    std::string String() const { return value_; }
private:
    std::string value_;
};

/**
 * VariantContainer — flat key→string JSON object.
 *
 * Supports the two operations used by handle_internetStatusChange():
 *   bool HasLabel(key)
 *   Variant operator[](key)
 *
 * FromJSON() parses a single-level JSON object of the form:
 *   {"status":"FULLY_CONNECTED","interface":"eth0"}
 */
class VariantContainer {
public:
    bool HasLabel(const std::string& key) const {
        return entries_.count(key) > 0;
    }

    Variant operator[](const std::string& key) const {
        auto it = entries_.find(key);
        return (it != entries_.end()) ? it->second : Variant{};
    }

    /**
     * Parse a flat JSON object.  Handles:
     *   - String values (double-quoted, basic backslash escape)
     *   - Number / bool / null values (stored as raw string)
     * Ignores nested objects/arrays.
     */
    void FromJSON(const std::string& json) {
        entries_.clear();
        std::string s = json;

        auto trimWS = [](std::string& str) {
            auto l = str.find_first_not_of(" \t\r\n");
            auto r = str.find_last_not_of(" \t\r\n");
            str = (l == std::string::npos) ? "" : str.substr(l, r - l + 1);
        };

        trimWS(s);
        if (!s.empty() && s.front() == '{') s.erase(0, 1);
        if (!s.empty() && s.back()  == '}') s.pop_back();

        size_t pos = 0;
        while (pos < s.size()) {
            /* ── key ── */
            size_t ks = s.find('"', pos);
            if (ks == std::string::npos) break;
            size_t ke = s.find('"', ks + 1);
            if (ke == std::string::npos) break;
            std::string key = s.substr(ks + 1, ke - ks - 1);

            /* ── colon ── */
            size_t colon = s.find(':', ke + 1);
            if (colon == std::string::npos) break;

            /* ── value ── */
            size_t vs = s.find_first_not_of(" \t", colon + 1);
            if (vs == std::string::npos) break;

            std::string val;
            if (s[vs] == '"') {
                /* String value */
                size_t ve = vs + 1;
                while (ve < s.size()) {
                    if (s[ve] == '\\' && ve + 1 < s.size()) {
                        ++ve; /* skip escaped character */
                    } else if (s[ve] == '"') {
                        break;
                    }
                    ++ve;
                }
                val = s.substr(vs + 1, ve - vs - 1);
                pos = ve + 1;
            } else {
                /* Number / bool / null */
                size_t ve = s.find_first_of(",}", vs);
                val = s.substr(vs, (ve == std::string::npos ? s.size() : ve) - vs);
                trimWS(val);
                pos = (ve == std::string::npos) ? s.size() : ve;
            }

            entries_[key] = Variant{val};
        }
    }

private:
    std::map<std::string, Variant> entries_;
};

/* IElement is the base template parameter used in SmartLinkType<IElement> */
using IElement  = VariantContainer;
using JsonObject = VariantContainer; /* convenience alias matching Thunder */

} // namespace JSON

} // namespace Core

/* JsonObject in the WPEFramework namespace — reachable after
 * "using namespace WPEFramework;" in networkstatussrc.cpp */
using JsonObject = Core::JSON::VariantContainer;

// ── JSONRPC namespace ───────────────────────────────────────────────────────

namespace JSONRPC {

/**
 * SmartLinkType<INTERFACE> — mock Thunder JSONRPC client.
 *
 * API surface matched to networkstatussrc.cpp usage:
 *
 *   SmartLinkType<Core::JSON::IElement> client(callsign, "");
 *   client.Subscribe<JsonObject>(timeout, "onInternetStatusChange", &handler);
 *   client.Unsubscribe(timeout, "onInternetStatusChange");
 *
 * Each Subscribe() call:
 *   - Creates /tmp/thunder_mock_<callsign>_<event>.subscribed
 *   - Spins a thread polling /tmp/thunder_mock_<callsign>_<event>.inject
 *     every 100 ms; when the file appears it is read, removed, and the
 *     registered callback is invoked.
 */
template<typename INTERFACE>
class SmartLinkType {
public:
    SmartLinkType(const std::string& callsign, const std::string& /*client*/)
        : callsign_(sanitise(callsign)) {}

    ~SmartLinkType() {
        /* Signal all polling threads to stop and wait for them */
        for (auto& kv : entries_) {
            kv.second->stop.store(true, std::memory_order_relaxed);
        }
        for (auto& kv : entries_) {
            if (kv.second->thread.joinable())
                kv.second->thread.join();
            removeFile(subscribedPath(kv.first));
        }
    }

    /* Non-copyable / non-movable (holds threads) */
    SmartLinkType(const SmartLinkType&)            = delete;
    SmartLinkType& operator=(const SmartLinkType&) = delete;

    /**
     * Subscribe<INBOUND>(timeout, eventName, callback)
     *
     * INBOUND must have:
     *   void FromJSON(const std::string&)   — populate from raw JSON line
     */
    template<typename INBOUND>
    int32_t Subscribe(uint32_t /*waitTime*/,
                      const std::string& eventName,
                      std::function<void(const INBOUND&)> handler)
    {
        /* Create subscription marker so Python tests can detect readiness */
        { std::ofstream marker(subscribedPath(eventName)); marker << "1\n"; }

        auto entry = std::unique_ptr<EventEntry>(new EventEntry());
        EventEntry* raw = entry.get();
        entries_[eventName] = std::move(entry);

        const std::string injectPath = injectFilePath(eventName);

        raw->thread = std::thread([raw,
                                   injectPath = std::move(injectPath),
                                   handler = std::move(handler)]() {
            while (!raw->stop.load(std::memory_order_relaxed)) {
                struct stat st{};
                if (::stat(injectPath.c_str(), &st) == 0 && st.st_size > 0) {
                    std::ifstream f(injectPath);
                    std::string   line;
                    if (std::getline(f, line) && !line.empty()) {
                        f.close();
                        removeFile(injectPath); /* consume the event */
                        INBOUND params;
                        params.FromJSON(line);
                        handler(params);
                    }
                }
                ::usleep(100000); /* 100 ms poll */
            }
        });

        return Core::ERROR_NONE;
    }

    int32_t Unsubscribe(uint32_t /*waitTime*/, const std::string& eventName) {
        auto it = entries_.find(eventName);
        if (it != entries_.end()) {
            it->second->stop.store(true, std::memory_order_relaxed);
            if (it->second->thread.joinable())
                it->second->thread.join();
            removeFile(subscribedPath(eventName));
            entries_.erase(it);
        }
        return Core::ERROR_NONE;
    }

private:
    /* Replace characters that are illegal in filenames */
    static std::string sanitise(const std::string& s) {
        std::string out = s;
        for (char& c : out) {
            if (c == '.' || c == '/' || c == ':' || c == ' ')
                c = '_';
        }
        return out;
    }

    std::string subscribedPath(const std::string& event) const {
        return "/tmp/thunder_mock_" + callsign_ + "_" + event + ".subscribed";
    }

    std::string injectFilePath(const std::string& event) const {
        return "/tmp/thunder_mock_" + callsign_ + "_" + event + ".inject";
    }

    static void removeFile(const std::string& path) {
        if (::remove(path.c_str()) != 0 && errno != ENOENT) {
            RDK_LOG(RDK_LOG_WARN, LOG_SYSTIME,
                    "[%s]: Failed to remove %s (errno=%d)\n",
                    __FUNCTION__, path.c_str(), errno);
        }
    }

    struct EventEntry {
        std::atomic<bool> stop{false};
        std::thread       thread;
        EventEntry() = default;
    };

    std::string callsign_;
    std::map<std::string, std::unique_ptr<EventEntry>> entries_;
};

} // namespace JSONRPC

} // namespace WPEFramework

/* Global alias — accessible even without "using namespace WPEFramework" */
using JsonObject = WPEFramework::Core::JSON::VariantContainer;
