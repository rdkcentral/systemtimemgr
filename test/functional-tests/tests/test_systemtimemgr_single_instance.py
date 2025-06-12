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

def test_check_systemtimemgr_is_starting():
    kill_sysTimeMgr()
    remove_logfile()
    print("Starting systemtimemgr process")
    command_to_start = "nohup /usr/local/bin/sysTimeMgr > /dev/null 2>&1 &"
    run_shell_silent(command_to_start)
    command_to_get_pid = "pidof sysTimeMgr"
    pid = run_shell_command(command_to_get_pid)
    assert pid != "", "sysTimeMgr process did not start"

def test_second_systemtimemgr_instance_is_not_started():
    command_to_get_pid = "pidof sysTimeMgr"
    pid1 = run_shell_command(command_to_get_pid)

    if is_systemtimemgr_running():
        print("systemtimemgr process is already running")
    else:
        command_to_start = "nohup /usr/local/bin/sysTimeMgr > /dev/null 2>&1 &"
        run_shell_silent(command_to_start)
        sleep(2)

    pid2 = run_shell_command(command_to_get_pid)
    assert pid1 == pid2, "A second instance of sysTimeMgr was started."

def test_tear_down():
    command_to_stop = "kill -9 `pidof sysTimeMgr`"
    run_shell_command(command_to_stop)
    command_to_get_pid = "pidof sysTimeMgr"
    remove_logfile()
    remove_outdir_contents(OUTPUT_DIR)
    pid = run_shell_command(command_to_get_pid)
    assert pid == ""
