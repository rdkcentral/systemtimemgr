##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2024 RDK Management
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
"""
L2 tests for the NWStatusMonitor feature (topic/NWStatusMonitor).

Verifies that sysTimeMgr:
  1. Subscribes to onInternetStatusChange from org.rdk.NetworkManager.
  2. Processes a FULLY_CONNECTED event and triggers the appropriate
     chrony sync logic.
  3. Ignores non-connected status events (no chrony action logged).
  4. Does not re-process a duplicate FULLY_CONNECTED event while
     lastStatus is already fully_connected.

When compiled with -D__LOCAL_TEST_ (the default for test builds) sysTimeMgr
uses WPEFrameworkMock.h instead of the real Thunder library.  Events are
injected by writing JSON to a file that the mock SmartLinkType polls:
  /tmp/thunder_mock_org_rdk_NetworkManager_onInternetStatusChange.inject

No Thunder daemon, WebSocket server, or network-mock.js is required.
This mirrors the enservice entservices-testframework thunder mock approach.
"""

import os
from time import sleep
from helper_functions import (
    run_shell_command,
    grep_sysTimeMgrlogs,
    check_file_exists,
    wait_for_nw_subscription,
    inject_internet_status,
    clear_inject_file,
    LOG_FILE,
)

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

CLOCK_EVENT_FILE = "/tmp/clock-event"


def _wait_for_log(message, timeout_s=10, poll_interval_s=0.5):
    """Poll sysTimeMgr log until *message* appears or timeout expires."""
    import time
    deadline = time.time() + timeout_s
    while time.time() < deadline:
        if message in grep_sysTimeMgrlogs(message):
            return True
        sleep(poll_interval_s)
    return False


# ---------------------------------------------------------------------------
# Liveness / precondition tests (must pass before event tests)
# ---------------------------------------------------------------------------

def test_sysTimeMgr_is_running():
    """sysTimeMgr process must be running."""
    pid = run_shell_command("pidof sysTimeMgr")
    assert pid != "", "sysTimeMgr process did not start"


def test_log_file_exists():
    """sysTimeMgr log file must exist."""
    assert check_file_exists(LOG_FILE), f"Log file '{LOG_FILE}' does not exist"


# ---------------------------------------------------------------------------
# Subscription test
# ---------------------------------------------------------------------------

def test_sysTimeMgr_subscribes_to_internet_status_change():
    """sysTimeMgr must subscribe to onInternetStatusChange at startup.

    The mock SmartLinkType (WPEFrameworkMock.h) creates a marker file
    /tmp/thunder_mock_org_rdk_NetworkManager_onInternetStatusChange.subscribed
    when Subscribe() is called.  Checks both the marker file and the log.
    """
    subscribed = wait_for_nw_subscription(timeout_s=20)
    assert subscribed, (
        "sysTimeMgr did not call SmartLinkType::Subscribe() within timeout. "
        "Ensure sysTimeMgr is compiled with -D__LOCAL_TEST_ and that "
        "networkstatussrc.cpp is included (topic/NWStatusMonitor branch)."
    )

    expected_log = "Successfully subscribed to onInternetStatusChange"
    assert _wait_for_log(expected_log, timeout_s=10), (
        f"Expected log line not found in {LOG_FILE}: '{expected_log}'"
    )


# ---------------------------------------------------------------------------
# Internet-up event tests
# ---------------------------------------------------------------------------

def test_fully_connected_event_is_processed():
    """Injecting FULLY_CONNECTED triggers chrony processing in sysTimeMgr.

    This covers the core NWStatusMonitor path:
      handle_internetStatusChange → g_internetUpPending=true →
      runEventProcessingLoop → processInternetOnline.
    """
    # Ensure /tmp/clock-event is absent so we exercise the first-sync path
    if os.path.exists(CLOCK_EVENT_FILE):
        os.remove(CLOCK_EVENT_FILE)
    clear_inject_file()

    ok = inject_internet_status("FULLY_CONNECTED", interface="eth0")
    assert ok, "Failed to write FULLY_CONNECTED inject file"

    # sysTimeMgr processes events on a dedicated thread; allow time for it
    expected_log = "CHRONY: Processing internet fully_connected event"
    assert _wait_for_log(expected_log, timeout_s=10), (
        f"Expected log line not found in {LOG_FILE}: '{expected_log}'. "
        "sysTimeMgr may not have processed the FULLY_CONNECTED event."
    )


def test_fully_connected_event_signals_handling_log():
    """sysTimeMgr must log the signal-to-processing-thread message on fully_connected."""
    clear_inject_file()
    ok = inject_internet_status("FULLY_CONNECTED", interface="eth0")
    assert ok, "Failed to write FULLY_CONNECTED inject file"

    expected_log = "Internet status changed to fully_connected"
    assert _wait_for_log(expected_log, timeout_s=10), (
        f"Expected log line not found in {LOG_FILE}: '{expected_log}'"
    )


# ---------------------------------------------------------------------------
# Non-connected status — negative test
# ---------------------------------------------------------------------------

def test_no_internet_event_is_not_processed():
    """A NO_INTERNET event must be logged as 'no action needed', not processed."""
    clear_inject_file()
    ok = inject_internet_status("NO_INTERNET", interface="eth0")
    assert ok, "Failed to write NO_INTERNET inject file"

    sleep(2)  # Give sysTimeMgr time to process the event if it mistakenly does

    # The code logs "no action needed" for non-connected states
    no_action_log = "no action needed"
    assert _wait_for_log(no_action_log, timeout_s=5), (
        f"Expected 'no action needed' log not found for NO_INTERNET event"
    )

    # The processing path must NOT have been triggered by this event
    processing_log = "CHRONY: Processing internet fully_connected event"
    log_hits_before = grep_sysTimeMgrlogs(processing_log).count(processing_log)
    sleep(1)
    log_hits_after = grep_sysTimeMgrlogs(processing_log).count(processing_log)
    assert log_hits_before == log_hits_after, (
        "sysTimeMgr incorrectly triggered CHRONY processing for NO_INTERNET event"
    )


# ---------------------------------------------------------------------------
# Duplicate event deduplication test
# ---------------------------------------------------------------------------

def test_duplicate_fully_connected_not_reprocessed():
    """A second consecutive FULLY_CONNECTED event must not trigger a second chrony run.

    networkstatussrc.cpp guards against duplicates: if lastStatus is already
    'fully_connected', a new fully_connected event sets g_internetUpPending again
    but the processing thread will only run processInternetOnline once per
    unique transition (duplicates are coalesced by the single-slot flag).

    This test verifies no *extra* processing log line appears for the duplicate.
    """
    clear_inject_file()
    inject_internet_status("FULLY_CONNECTED", interface="eth0")
    sleep(3)

    processing_log = "CHRONY: Processing internet fully_connected event"
    count_before = grep_sysTimeMgrlogs(processing_log).count(processing_log)

    clear_inject_file()
    inject_internet_status("FULLY_CONNECTED", interface="eth0")
    sleep(3)

    count_after = grep_sysTimeMgrlogs(processing_log).count(processing_log)
    assert count_after == count_before, (
        f"Duplicate FULLY_CONNECTED event triggered an extra chrony processing run "
        f"(before={count_before}, after={count_after})"
    )
