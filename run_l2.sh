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

RESULT_DIR="/tmp/l2_test_report"
mkdir -p "$RESULT_DIR"

apt-get update && apt-get install -y libjsonrpccpp-dev

touch /usr/local/bin/journalctl
chmod -R 777 /usr/local/bin/journalctl
ln -s /usr/local/bin/journalctl /usr/bin/journalctl

rm -rf /etc/systimemgr.conf
rm -rf /opt/secure/clock.txt
rm -rf /tmp/systimemgr

echo "timesrc  ntp /ntp" > /etc/systimemgr.conf
echo "timesrc dtt /dtt" >> /etc/systimemgr.conf
echo "timesync rdkdefault /clock_time" >> /etc/systimemgr.conf

echo "$(date +%s)" > /opt/secure/clock.txt

mkdir /tmp/systimemgr/
touch /tmp/systimemgr/ntp

rm -rf /opt/logs/systimemgr.log*

# Run L2 Test cases
pytest --json-report --json-report-summary --json-report-file $RESULT_DIR/systimemgr_single_instance.json test/functional-tests/tests/test_systimemgr_single_instance.py
pytest --json-report --json-report-summary --json-report-file $RESULT_DIR/systimemgr_initialisation.json test/functional-tests/tests/test_systimemgr_initialisation.py
pytest --json-report --json-report-summary --json-report-file $RESULT_DIR/systimemgr_get_time.json test/functional-tests/tests/test_systimemgr_get_time.py
pytest --json-report --json-report-summary --json-report-file $RESULT_DIR/systimemgr_check_file.json test/functional-tests/tests/test_systimemgr_check_file.py
pytest --json-report --json-report-summary --json-report-file $RESULT_DIR/systimemgr_time_quality.json test/functional-tests/tests/test_systimemgr_time_quality.py
pytest --json-report --json-report-summary --json-report-file $RESULT_DIR/systimemgr_bootup_flow.json test/functional-tests/tests/test_systimemgr_bootup_flow.py

#Secure Time Validation TestCases
rm -rf /etc/systimemgr.conf
rm -rf /opt/secure/clock.txt

echo "timesrc drm /drm" >> /etc/systimemgr.conf
echo "timesync rdkdefault /clock_time" >> /etc/systimemgr.conf

cat /opt/logs/systimemgr.log.0
echo "$(date +%s)" > /opt/secure/clock.txt
touch /tmp/systimemgr/drm

pytest --json-report --json-report-summary --json-report-file $RESULT_DIR/systimemgr_single_instance.json test/functional-tests/tests/test_systimemgr_single_instance.py
pytest --json-report --json-report-summary --json-report-file $RESULT_DIR/systimemgr_securetime_Initialisation.json test/functional-tests/tests/test_secureTime_initialisation.py
pytest --json-report --json-report-summary --json-report-file $RESULT_DIR/systimemgr_secureTime_checkEvent.json test/functional-tests/tests/test_secureTime_checkEvent.py
pytest --json-report --json-report-summary --json-report-file $RESULT_DIR/systimemgr_secureTime_Quality.json test/functional-tests/tests/test_secureTime_quality.py
