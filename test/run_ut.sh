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

ENABLE_COV=false

if [ "x$1" = "x--enable-cov" ]; then
      echo "Enabling coverage options"
      export CXXFLAGS="-g -O0 -fprofile-arcs -ftest-coverage"
      export CFLAGS="-g -O0 -fprofile-arcs -ftest-coverage"
      export LDFLAGS="-lgcov --coverage"
      ENABLE_COV=true
fi
export TOP_DIR=`pwd`

apt-get update
apt-get -y install libjsoncpp-dev
apt-get install -y libjsonrpccpp-dev

export top_srcdir=`pwd`

cd ./systimerfactory/unittest/

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
./timerfactory_gtest

echo "********************"
echo "**** CAPTURE SYSTEM TIMEMANAGER COVERAGE DATA ****"
echo "********************"
if [ "$ENABLE_COV" = true ]; then
    echo "Generating coverage report"
    lcov --capture --directory . --output-file coverage.info
    lcov --remove coverage.info '/usr/*' --output-file coverage.info
    lcov --remove coverage.info './systimerfactory/unittest/*' --output-file coverage.info
    lcov --list coverage.info
fi

cd $TOP_DIR
