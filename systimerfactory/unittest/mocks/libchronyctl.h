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

/*
 * Mock declarations for libchronyctl used in GTEST_ENABLE unit-test builds.
 *
 * This header shadows the installed libchronyctl.h so that networkstatussrc.cpp
 * can be compiled without the real library.  It provides:
 *   - The chronyctl_error enum and a minimal IPAddr type stub
 *   - extern "C" function declarations (implementations are in Client_Mock.h,
 *     compiled into the test binary's SysTimeMgrUnitTest.cpp translation unit)
 *   - A ChronyCtlMock GMock class for fine-grained test control via
 *     globalChronyCtlMock
 *
 * The pattern mirrors the v_secure_system mock in Client_Mock.h:
 *   networkstatussrc.cpp TU → includes this header (declarations only)
 *   SysTimeMgrUnitTest.cpp TU → includes Client_Mock.h (stub definitions)
 *   At link time the linker resolves the extern "C" symbols from the test TU.
 */

#ifndef LIBCHRONYCTL_H
#define LIBCHRONYCTL_H

#include <stdint.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CHRONYCTL_SUCCESS        =  0,
    CHRONYCTL_ERROR_INIT     = -1,
    CHRONYCTL_ERROR_NOT_INIT = -2,
    CHRONYCTL_ERROR_EXEC     = -3,
    CHRONYCTL_ERROR_PARSE    = -4,
    CHRONYCTL_ERROR_INVALID  = -5,
    CHRONYCTL_ERROR_MUTEX    = -6,
    CHRONYCTL_ERROR_NO_DATA  = -7,
    CHRONYCTL_ERROR_UNAUTH   = -8
} chronyctl_error;

/* Minimal IPAddr stub — layout matches addressing.h so function signatures
 * compile correctly.  NULL is passed for addr/mask in all current call sites,
 * so no field access is ever performed in tests. */
typedef struct {
    union { uint32_t in4; uint8_t in6[16]; uint32_t id; } addr;
    uint16_t family;
    uint16_t _pad;
} IPAddr;

int         chronyctl_init(void);
int         chronyctl_cleanup(void);
int         chronyctl_get_offset(double *offset_sec);
int         chronyctl_get_system_time_offset(double *offset_sec);
int         chronyctl_makestep(void);
int         chronyctl_online(const IPAddr *addr, const IPAddr *mask);
int         chronyctl_burst(const IPAddr *addr, const IPAddr *mask,
                            int n_good_samples, int n_total_samples);
int         chronyctl_has_selectable_source(int *has_selectable);
int         chronyctl_get_source_count(int *count);
int         chronyctl_waitsync(int max_tries, int interval_sec);
const char *chronyctl_strerror(int err);

#ifdef __cplusplus
}
#endif

/* -----------------------------------------------------------------------
 * GMock class — instantiate as globalChronyCtlMock in tests that need to
 * control chronyctl behaviour.  Stubs default to CHRONYCTL_SUCCESS / safe
 * values when globalChronyCtlMock is nullptr.
 * ----------------------------------------------------------------------- */
class ChronyCtlMock {
public:
    MOCK_METHOD(int, chronyctl_init, ());
    MOCK_METHOD(int, chronyctl_cleanup, ());
    MOCK_METHOD(int, chronyctl_get_offset, (double *offset_sec));
    MOCK_METHOD(int, chronyctl_makestep, ());
    MOCK_METHOD(int, chronyctl_online, (const IPAddr *addr, const IPAddr *mask));
    MOCK_METHOD(int, chronyctl_burst,
                (const IPAddr *addr, const IPAddr *mask,
                 int n_good_samples, int n_total_samples));
    MOCK_METHOD(int, chronyctl_has_selectable_source, (int *has_selectable));
    MOCK_METHOD(int, chronyctl_get_source_count, (int *count));
    MOCK_METHOD(int, chronyctl_waitsync, (int max_tries, int interval_sec));
    MOCK_METHOD(const char *, chronyctl_strerror, (int err));
};

/* Defined once in Client_Mock.h (SysTimeMgrUnitTest.cpp TU). */
extern ChronyCtlMock *globalChronyCtlMock;

#endif /* LIBCHRONYCTL_H */
