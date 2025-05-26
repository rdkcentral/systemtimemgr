#
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

# Define the program name and the source files
bin_PROGRAMS = drmtest_gtest dtttest_gtest rdkDefaulttest_gtest timerfactory_gtest

# Define the include directories
COMMON_CPPFLAGS = -I../ -I../../ -I../../interface -I./mocks -I/usr/include

# Define the libraries to link against
COMMON_LDADD = -ljsoncpp -lgtest -lgtest_main -lgmock_main -lgmock 

# Define the compiler flags
COMMON_CXXFLAGS = -frtti

# Define the source files
drmtest_gtest_SOURCES = drmtimerUnitTest.cpp 
dtttest_gtest_SOURCES = dtttimerUnitTest.cpp
rdkDefaulttest_gtest_SOURCES = rdkDefaulttimesyncUnitTest.cpp
timerfactory_gtest_SOURCES = timerfactory_gtest.cpp

# Apply common properties to each program
drmtest_gtest_CPPFLAGS = $(COMMON_CPPFLAGS)
drmtest_gtest_LDADD = $(COMMON_LDADD)
drmtest_gtest_CXXFLAGS = $(COMMON_CXXFLAGS)

dtttest_gtest_CPPFLAGS = $(COMMON_CPPFLAGS)
dtttest_gtest_LDADD = $(COMMON_LDADD)
dtttest_gtest_CXXFLAGS = $(COMMON_CXXFLAGS)

rdkDefaulttest_gtest_CPPFLAGS = $(COMMON_CPPFLAGS)
rdkDefaulttest_gtest_LDADD = $(COMMON_LDADD)
rdkDefaulttest_gtest_CXXFLAGS = $(COMMON_CXXFLAGS)


timerfactory_gtest_CPPFLAGS = $(COMMON_CPPFLAGS)
timerfactory_gtest_LDADD = $(COMMON_LDADD)
timerfactory_gtest_CXXFLAGS = $(COMMON_CXXFLAGS)
