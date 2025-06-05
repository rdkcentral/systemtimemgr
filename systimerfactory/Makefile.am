# Copyright 2023 Comcast Cable Communications Management, LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0
#


AM_CXXFLAGS = -Wall -std=c++1y  $(DEBUG_CXXFLAGS) -I${PKG_CONFIG_SYSROOT_DIR}$(includedir)/systimerifc $(IARM_CFLAGS) $(PWRMGR_CFLAGS)  $(CURL_CFLAGS) $(UNITTEST_CXXFLAGS) $(TEE_CXXFLAGS) $(JSONRPC_CFLAGS)  $(WPEFRAMEWORKCORE_CFLAGS) $(WPEFRAMEWORKWEBSOCKET_CFLAGS) $(WPEVGDRM_CXXFLAGS) $(DTT_CXXFLAGS)
ACLOCAL_AMFLAGS = -I m4


lib_LTLIBRARIES = libsystimerfactory.la

libsystimerfactory_la_SOURCES = timerfactory.cpp pubsubfactory.cpp rdkdefaulttimesync.cpp drmtimersrc.cpp

if IARM_ENABLED
libsystimerfactory_la_SOURCES += iarmpublish.cpp  iarmsubscribe.cpp iarmtimerstatussubscriber.cpp
if PWRMGRPLUGIN_ENABLED
libsystimerfactory_la_SOURCES += ipowercontrollersubscriber.cpp
else
libsystimerfactory_la_SOURCES += iarmpowersubscriber.cpp
endif
endif


if DTT_ENABLED
libsystimerfactory_la_SOURCES += dtttimersrc.cpp
endif

libsystimerfactory_la_LIBADD = $(IARM_LIBS) $(JSONRPC_LIBS) -lrdkloggers $(WPEFRAMEWORKCORE_LIBS) $(WPEFRAMEWORKWEBSOCKET_LIBS)


libsystimerfactory_la_includedir = ${includedir}
libsystimerfactory_la_include_HEADERS = timerfactory.h rdkdefaulttimesync.h
