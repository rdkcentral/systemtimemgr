##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2018 RDK Management
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
##########################################################################

AM_CXXFLAGS = -Wall -std=c++1y  $(DEBUG_CXXFLAGS) $(TEE_CXXFLAGS) $(DTT_CXXFLAGS) -I${PKG_CONFIG_SYSROOT_DIR}$(includedir)/systimerifc
ACLOCAL_AMFLAGS = -I m4


bin_PROGRAMS = sysTimeMgr

lib_LTLIBRARIES = libsysTimeMgr.la

libsysTimeMgr_la_SOURCES = systimemgr.cpp
#if defined(L2_ENABLED)
libsysTimeMgr_la_SOURCES +=./test/TestIarmInterface.cpp
#endif
libsysTimeMgr_la_LDFLAGS = -lpthread  -lsystimerfactory -lrdkloggers -lsecure_wrapper


sysTimeMgr_SOURCES = main.cpp
sysTimeMgr_LDADD = libsysTimeMgr.la

#libsysTimeMgr_la_includedir = ${includedir}
#libsysTimeMgr_la_include_HEADERS = systimemgr.h
