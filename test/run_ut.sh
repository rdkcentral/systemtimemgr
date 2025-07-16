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

ENABLE_COV=true

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

find / -iname "jsonrpccpp" # This command might not be necessary for the build
./configure
make

mkdir -p /opt/secure/

# Create a directory to store Gtest XML reports
mkdir -p /tmp/gtest_reports/

fail=0

run_test() {
    test_binary="$1"
    report_file="/tmp/gtest_reports/${test_binary}.xml" # Define output path for XML report
    echo "Running $test_binary with XML output to $report_file..."
    # Execute the test binary with gtest_output flag
    ./$test_binary --gtest_output=xml:"${report_file}"
    status=$?
    if [ $status -ne 0 ]; then
        echo "Test $test_binary failed with exit code $status"
        fail=1
    else
        echo "Test $test_binary passed"
    fi
    echo "------------------------------------"
}

run_test drmtest_gtest
run_test dtttest_gtest
run_test rdkDefaulttest_gtest
run_test timerfactory_gtest
run_test pubsubfactory_gtest
run_test ipowercontrollersubscriber_gtest
run_test iarmtimerstatus_gtest
run_test iarmsubscribe_gtest
run_test iarmpublish_gtest
run_test iarmpowersubscribe_gtest
run_test systimemgr_gtest

if [ $fail -ne 0 ]; then
    echo "Some unit tests failed."
    exit 1
fi

echo "********************"
echo "**** CAPTURE SYSTEM TIMEMANAGER COVERAGE DATA ****"
echo "********************"
if [ "$ENABLE_COV" = true ]; then
    echo "Generating coverage report"

    lcov --capture --directory . --base-directory . --output-file raw_coverage.info
    lcov --extract raw_coverage.info '/__w/systemtimemgr/systemtimemgr*' --output-file systimer_coverage.info
    lcov --remove systimer_coverage.info '/usr/*'  '/__w/systemtimemgr/systimerfactory/unittest/*' '*/interface/*' '*/systemd_units/*' --output-file coverage.info
    lcov --list coverage.info

    #lcov --capture --directory . --output-file coverage.info
    #lcov --remove coverage.info '/usr/*' --output-file coverage.info
    #lcov --remove coverage.info './systimerfactory/unittest/*' --output-file coverage.info
    #lcov --list coverage.info
fi

cd "$TOP_DIR" # Use double quotes for robust path handling
