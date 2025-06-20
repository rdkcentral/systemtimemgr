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

import subprocess
import requests
import os
import time
import re
import signal
import shutil
from time import sleep

LOG_FILE = "/opt/logs/systimemgr.log.0"

def remove_logfile():
    try:
        if os.path.exists(LOG_FILE):
            os.remove(LOG_FILE)
            print(f"Log file {LOG_FILE} removed.")
        else:
            print(f"Log file {LOG_FILE} does not exist.")
    except Exception as e:
        print(f"Could not remove log file {LOG_FILE}: {e}")

def kill_sysTimeMgr(signal: int=9):
    print(f"Received Signal to kill systimemgr {signal} with pid {get_pid('sysTimeMgr')}")
    resp = subprocess.run(f"kill -{signal} {get_pid('sysTimeMgr')}", shell=True, capture_output=True)
    print(resp.stdout.decode('utf-8'))
    print(resp.stderr.decode('utf-8'))
    return ""

def grep_sysTimeMgrlogs(search: str):
    search_result = ""
    search_pattern = re.compile(re.escape(search), re.IGNORECASE)
    try:
        with open(LOG_FILE, 'r', encoding='utf-8', errors='ignore') as file:
            for line_number, line in enumerate(file, start=1):
                if search_pattern.search(line):
                    search_result = search_result + " \n" + line
    except Exception as e:
        print(f"Could not read file {LOG_FILE}: {e}")
    return search_result

def get_pid(name: str):
    return subprocess.run(f"pidof {name}", shell=True, capture_output=True).stdout.decode('utf-8').strip()

def run_shell_silent(command):
    subprocess.run(command, shell=True, capture_output=False, text=False)
    return

def run_shell_command(command):
    result = subprocess.run(command, shell=True, capture_output=True, text=True)
    return result.stdout.strip()

def check_file_exists(file_path):
    return os.path.isfile(file_path)
