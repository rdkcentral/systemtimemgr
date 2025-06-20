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
from time import sleep
from helper_functions import *

def test_trigger_systimemgr_instance():
    kill_sysTimeMgr()
    print("Starting systemtimemgr process")
    command_to_start = "nohup /usr/local/bin/sysTimeMgr > /dev/null 2>&1 &"
    run_shell_silent(command_to_start)
    command_to_get_pid = "pidof sysTimeMgr"
    pid = run_shell_command(command_to_get_pid)
    assert pid != "", "sysTimeMgr process did not start"

def test_if_systimemgr_running():
    command_to_check = "ps aux | grep sysTimeMgr | grep -v grep"
    result = run_shell_command(command_to_check)
    assert result != ""

def test_check_systimemgr_log_file():
    log_file_path = LOG_FILE
    assert check_file_exists(log_file_path), f"Log File '{log_file_path}' does not exist."

def test_last_known_time():
    GET_TIME = "Returning Last Known Good Time"
    assert GET_TIME in grep_sysTimeMgrlogs(GET_TIME)

def parse_log_time(log_line):
    match = re.search(r'time = (.+)', log_line)
    if match:
        return match.group(1)
    return None

def test_parse_log_time():
    log_line = "Returning Last Known Good Time, time = "
    time_str = parse_log_time(log_line)
