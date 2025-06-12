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
from helper_functions import *

def test_check_and_start_systemtimemgr():
    kill_sysTimeMgr()
    remove_logfile()
    print("Starting systemtimemgr process")
    command_to_start = "nohup /usr/local/bin/sysTimeMgr > /dev/null 2>&1 &"
    run_shell_silent(command_to_start)
    command_to_get_pid = "pidof sysTimeMgr"
    pid = run_shell_command(command_to_get_pid)
    assert pid != "", "sysTimeMgr process did not start"


sleep(5)

QUERY_MSG = "Quality ="
assert QUERY_MSG in grep_sysTimeMgrlogs(QUERY_MSG)


