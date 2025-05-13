#!/bin/sh

####################################################################################
# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2023 RDK Management
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
####################################################################################

apt-get update
apt-get -y install libjsoncpp-dev
apt-get install -y libjsonrpccpp-dev

export top_srcdir=`pwd`

cd ./systimerfactory/unittest/

export CXXFLAGS="-g -O0 -fprofile-arcs -ftest-coverage"
export CFLAGS="-g -O0 -fprofile-arcs -ftest-coverage"
export LDFLAGS="-lgcov --coverage"

automake --add-missing
autoreconf --install

find / -iname "jsonrpccpp"
./configure
make

mkdir -p /opt/secure/
# Execute test suites for different sub-modules

./drmtest_gtest
./dtttest_gtest
./rdkDefaulttest_gtest

    lcov --capture --directory . --output-file coverage.info

    lcov --remove coverage.info '/usr/*' --output-file coverage.filtered.info

    genhtml coverage.filtered.info --output-directory out
