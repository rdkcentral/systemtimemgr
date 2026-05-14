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
import json

LOG_FILE = "/opt/logs/systimemgr.log.0"

# ---------------------------------------------------------------------------
# Network-status event injection helpers
#
# When sysTimeMgr is compiled with __LOCAL_TEST_ it uses WPEFrameworkMock.h
# instead of the real Thunder library.  The mock SmartLinkType polls a file:
#
#   /tmp/thunder_mock_<callsign>_<event>.inject
#
# Writing JSON to that file injects an event into sysTimeMgr — no Thunder
# daemon or WebSocket server required.  This mirrors the enservice
# entservices-testframework thunder mock approach.
# ---------------------------------------------------------------------------

# Callsign from networkstatussrc.cpp (sanitised: '.' → '_')
_NM_CALLSIGN_SANITISED = "org_rdk_NetworkManager"
_NM_EVENT              = "onInternetStatusChange"

INJECT_FILE    = f"/tmp/thunder_mock_{_NM_CALLSIGN_SANITISED}_{_NM_EVENT}.inject"
SUBSCRIBED_FILE = f"/tmp/thunder_mock_{_NM_CALLSIGN_SANITISED}_{_NM_EVENT}.subscribed"


def inject_internet_status(status, interface="eth0"):
    """Inject an onInternetStatusChange event into the running sysTimeMgr.
    Writes the JSON payload to the file that the mock SmartLinkType polls.
    Uses an atomic rename to avoid the mock reading a partial file.
    Args:
        status (str):    e.g. "FULLY_CONNECTED" or "NO_INTERNET"
        interface (str): network interface name (default "eth0")
    """
    payload = json.dumps({"status": status, "interface": interface})
    tmp = INJECT_FILE + ".tmp"
    try:
        with open(tmp, "w") as f:
            f.write(payload + "\n")
        os.rename(tmp, INJECT_FILE)
        return True
    except Exception as exc:
        print(f"[inject_internet_status] {exc}")
        return False


def wait_for_nw_subscription(timeout_s=15):
    """Wait until sysTimeMgr has subscribed to onInternetStatusChange.
    The mock SmartLinkType creates the .subscribed marker file when
    Subscribe() is called.  Returns True once the file exists.
    """
    deadline = time.time() + timeout_s
    while time.time() < deadline:
        if os.path.exists(SUBSCRIBED_FILE):
            return True
        sleep(0.2)
    return False


def clear_inject_file():
    """Remove any leftover inject file from a previous test run."""
    try:
        if os.path.exists(INJECT_FILE):
            os.remove(INJECT_FILE)
    except OSError:
        pass

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

def is_systemtimemgr_running():
    command_to_check = "ps aux | grep sysTimeMgr | grep -v grep"
    result = run_shell_command(command_to_check)
    return result != ""

def check_file_exists(file_path):
    return os.path.isfile(file_path)
