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
    command_to_check = "ps aux | grep sysTimeMgr | grep -v grep"
    result = run_shell_command(command_to_check)
    assert result != ""

def test_check_systimemgr_log_file():
    log_file_path = LOG_FILE
    assert check_file_exists(log_file_path), f"Log File '{log_file_path}' does not exist."

def test_Initialization():
    INITIALIZATION_MSG = "Initializing Src and Syncs"
    assert INITIALIZATION_MSG in grep_sysTimeMgrlogs(INITIALIZATION_MSG)
