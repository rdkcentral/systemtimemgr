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

def test_is_systimemgr_running():
    command_to_check = "pidof sysTimeMgr"
    result = run_shell_command(command_to_check)
    assert result != "", "sysTimeMgr process did not start"

def test_check_systimemgr_log_file():
    log_file_path = LOG_FILE
    assert check_file_exists(log_file_path), f"Log File '{log_file_path}' does not exist."

def test_bootup_flow():
    CREATE_INSTANCE_MSG = "Created New Instance"
    assert CREATE_INSTANCE_MSG in grep_sysTimeMgrlogs(CREATE_INSTANCE_MSG)

    CHECK_INITIALIZATION_MSG_NTP = "Initializing Src and Syncs. Category = timesrc, Type = ntp, Args = /ntp"
    assert CHECK_INITIALIZATION_MSG_NTP in grep_sysTimeMgrlogs(CHECK_INITIALIZATION_MSG_NTP)

    CHECK_INITIALIZATION_MSG_DTT = "Initializing Src and Syncs. Category = timesrc, Type = dtt, Args = /dtt"
    assert CHECK_INITIALIZATION_MSG_DTT in grep_sysTimeMgrlogs(CHECK_INITIALIZATION_MSG_DTT)

    CHECK_INITIALIZATION_MSG_RDKDEFAULT = "Initializing Src and Syncs. Category = timesync, Type = rdkdefault, Args = /clock_time"
    assert CHECK_INITIALIZATION_MSG_RDKDEFAULT in grep_sysTimeMgrlogs(CHECK_INITIALIZATION_MSG_RDKDEFAULT)

    CHECK_LAST_GOOD_TIME_MSG = "Returning Last Known Good Time, time ="
    assert CHECK_LAST_GOOD_TIME_MSG in grep_sysTimeMgrlogs(CHECK_LAST_GOOD_TIME_MSG)

    CHECK_TIME_QUALITY_POOR_MSG = "Info: MsgType = 0, Quality = 0, Message = Poor"
    assert CHECK_TIME_QUALITY_POOR_MSG in grep_sysTimeMgrlogs(CHECK_TIME_QUALITY_POOR_MSG)

    CHECK_TIME_QUALITY_GOOD_MSG = "Info: MsgType = 2, Quality = 1, Message = Good"
    assert CHECK_TIME_QUALITY_GOOD_MSG in grep_sysTimeMgrlogs(CHECK_TIME_QUALITY_GOOD_MSG)
