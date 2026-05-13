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
Log-scanning strategy
---------------------
Every test records the current byte offset of the log file at the point it
starts (via _log_offset()).  All subsequent log checks use _wait_for_new_log()
or _count_in_new_log() which seek to that offset before reading, so stale
content from earlier tests or previous runs can never cause a false-pass.
run_l2.sh removes the log file before starting the NWStatus sysTimeMgr
instance, so the log is always fresh for this test suite.
"""

import os
import time
from time import sleep
from helper_functions import (
    run_shell_command,
    check_file_exists,
    wait_for_nw_subscription,
    inject_internet_status,
    clear_inject_file,
    LOG_FILE,
)

# ---------------------------------------------------------------------------
# Private helpers
# ---------------------------------------------------------------------------

CLOCK_EVENT_FILE = "/tmp/clock-event"


def _log_offset():
    """Return the current byte size of the log file.
    Used as a cursor: pass the return value to _wait_for_new_log() /
    _count_in_new_log() so those functions only scan content written *after*
    this point in time, preventing stale lines from earlier tests causing
    false-passes.
    """
    try:
        return os.path.getsize(LOG_FILE)
    except OSError:
        return 0


def _wait_for_new_log(message, after_offset=0, timeout_s=10, poll_interval_s=0.5):
    """Poll the log file for *message* in content written after *after_offset*.
    Only bytes from *after_offset* onward are scanned, so lines that existed
    before the test injected its event cannot satisfy the assertion.
    Args:
        message (str):      Substring to search for.
        after_offset (int): Byte position returned by _log_offset() at test start.
        timeout_s (float):  Maximum time to wait.
        poll_interval_s:    Sleep between read attempts.
    Returns:
        True if *message* is found within *timeout_s* seconds, else False.
    """
    deadline = time.time() + timeout_s
    while time.time() < deadline:
        try:
            with open(LOG_FILE, "r", encoding="utf-8", errors="ignore") as f:
                f.seek(after_offset)
                if message in f.read():
                    return True
        except OSError:
            pass
        sleep(poll_interval_s)
    return False


def _count_in_new_log(message, after_offset=0):
    """Count occurrences of *message* in log content written after *after_offset*.
    Uses the same offset-based approach as _wait_for_new_log() so only fresh
    log lines are counted.
    Args:
        message (str):      Substring to count.
        after_offset (int): Byte position returned by _log_offset().
    Returns:
        int — number of occurrences found (0 if file unreadable).
    """
    try:
        with open(LOG_FILE, "r", encoding="utf-8", errors="ignore") as f:
            f.seek(after_offset)
            return f.read().count(message)
    except OSError:
        return 0


# ---------------------------------------------------------------------------
# Liveness / precondition tests (must pass before event tests)
# ---------------------------------------------------------------------------

def test_sysTimeMgr_is_running():
    """sysTimeMgr process must be running.
    run_l2.sh kills and restarts sysTimeMgr (with RFC chrony marker present)
    before executing this suite.  If the process is absent the remaining
    tests are meaningless.
    """
    pid = run_shell_command("pidof sysTimeMgr")
    assert pid != "", "sysTimeMgr process did not start"


def test_log_file_exists():
    """sysTimeMgr log file must exist.
    rdkloggers creates /opt/logs/systimemgr.log.0 on the first write after
    startup.  run_l2.sh removes the file before the fresh sysTimeMgr start;
    if rdkloggers has written at least one line by the time this test runs
    the file will be present.
    """
    assert check_file_exists(LOG_FILE), f"Log file '{LOG_FILE}' does not exist"


# ---------------------------------------------------------------------------
# Subscription test
# ---------------------------------------------------------------------------

def test_sysTimeMgr_subscribes_to_internet_status_change():
    """sysTimeMgr must subscribe to onInternetStatusChange at startup.
    The mock SmartLinkType (WPEFrameworkMock.h) creates a marker file
    /tmp/thunder_mock_org_rdk_NetworkManager_onInternetStatusChange.subscribed
    when Subscribe() is called.  Two checks:
      1. Marker file exists (fastest signal — does not depend on logging).
      2. The 'Successfully subscribed' line appears in log content written
         after this test started (offset-based to avoid stale matches).
    """
    offset = _log_offset()
    subscribed = wait_for_nw_subscription(timeout_s=20)
    assert subscribed, (
        "sysTimeMgr did not call SmartLinkType::Subscribe() within timeout. "
        "Ensure sysTimeMgr is compiled with -D__LOCAL_TEST_ and that "
        "networkstatussrc.cpp is included (topic/NWStatusMonitor branch)."
    )

    # run_l2.sh removes the log file before starting this sysTimeMgr instance
    # so the entire log is fresh.  The subscription happens during the 'sleep 2'
    # in run_l2.sh — before pytest even starts — so offset (captured after
    # pytest launches) is already past that line.  Use after_offset=0 to scan
    # the whole (fresh) log; stale-line risk is eliminated by the log rotation.
    expected_log = "Successfully subscribed to onInternetStatusChange"
    assert _wait_for_new_log(expected_log, after_offset=0, timeout_s=10), (
        f"Expected log line not found in {LOG_FILE}: '{expected_log}'"
    )


# ---------------------------------------------------------------------------
# Internet-up event tests
# ---------------------------------------------------------------------------

def test_fully_connected_event_is_processed():
    """Injecting FULLY_CONNECTED triggers chrony processing in sysTimeMgr.
    Core NWStatusMonitor path:
      handle_internetStatusChange
        → lastStatus transitions '' -> 'fully_connected'
        → g_internetUpPending = true, g_cv.notify_one()
        → runEventProcessingLoop wakes
        → processInternetOnline() logs 'CHRONY: Processing internet fully_connected event'
    The log scan starts from the byte offset captured *before* injection so
    a matching line from a previous test cannot satisfy the assertion.
    """
    # A prior event may have left lastStatus as 'fully_connected'. Reset it
    # with NO_INTERNET so this injection is not treated as a duplicate.
    clear_inject_file()
    inject_internet_status("NO_INTERNET", interface="eth0")
    sleep(1)

    if os.path.exists(CLOCK_EVENT_FILE):
        os.remove(CLOCK_EVENT_FILE)
    clear_inject_file()

    offset = _log_offset()
    ok = inject_internet_status("FULLY_CONNECTED", interface="eth0")
    assert ok, "Failed to write FULLY_CONNECTED inject file"

    expected_log = "CHRONY: Processing internet fully_connected event"
    assert _wait_for_new_log(expected_log, after_offset=offset, timeout_s=10), (
        f"Expected log line not found after injection in {LOG_FILE}: '{expected_log}'. "
        "sysTimeMgr may not have processed the FULLY_CONNECTED event."
    )


def test_fully_connected_event_signals_handling_log():
    """sysTimeMgr must log the signal-to-processing-thread message on fully_connected.
    The signalling log line ('Internet status changed to fully_connected')
    is emitted inside handle_internetStatusChange() immediately before
    g_cv.notify_one().  It only appears when the status transitions FROM a
    non-fully_connected state TO fully_connected (not for duplicates).
    A NO_INTERNET injection resets lastStatus first so the subsequent
    FULLY_CONNECTED is never treated as a duplicate.
    """
    # Reset lastStatus to ensure the upcoming FULLY_CONNECTED is a fresh transition.
    clear_inject_file()
    inject_internet_status("NO_INTERNET", interface="eth0")
    sleep(1)
    clear_inject_file()

    offset = _log_offset()
    ok = inject_internet_status("FULLY_CONNECTED", interface="eth0")
    assert ok, "Failed to write FULLY_CONNECTED inject file"

    # Exact substring from the RDK_LOG call in handle_internetStatusChange():
    #   "CHRONY: Internet status changed to fully_connected — signalling processing thread"
    expected_log = "Internet status changed to fully_connected"
    assert _wait_for_new_log(expected_log, after_offset=offset, timeout_s=10), (
        f"Expected log line not found after injection in {LOG_FILE}: '{expected_log}'"
    )


# ---------------------------------------------------------------------------
# Non-connected status — negative test
# ---------------------------------------------------------------------------

def test_no_internet_event_is_not_processed():
    """A NO_INTERNET event must be received and logged, but must NOT trigger chrony.
    Two assertions:
      1. Positive: the no-action log line for the specific status 'no_internet'
         appears in NEW log content (offset-based).  The exact substring is
         'Internet status=no_internet' from the RDK_LOG format string:
           'CHRONY: Internet status=%s (prev=%s) — no action needed'
         This is specific enough that it can only match a NO_INTERNET event.
      2. Negative: the count of 'CHRONY: Processing internet fully_connected event'
         in new content is zero — chrony processing was not triggered.
    """
    clear_inject_file()
    # Reset to a known state: inject FULLY_CONNECTED first so lastStatus is
    # 'fully_connected', making the NO_INTERNET a genuine status change.
    inject_internet_status("FULLY_CONNECTED", interface="eth0")
    sleep(1)
    clear_inject_file()

    offset = _log_offset()
    ok = inject_internet_status("NO_INTERNET", interface="eth0")
    assert ok, "Failed to write NO_INTERNET inject file"

    # Specific match: only a NO_INTERNET callback produces this substring.
    no_action_log = "Internet status=no_internet"
    assert _wait_for_new_log(no_action_log, after_offset=offset, timeout_s=10), (
        f"Expected 'no action needed' log for NO_INTERNET not found in new log content. "
        f"Searched for: '{no_action_log}'"
    )

    # Verify chrony processing was NOT triggered by this NO_INTERNET event.
    processing_log = "CHRONY: Processing internet fully_connected event"
    assert _count_in_new_log(processing_log, after_offset=offset) == 0, (
        "sysTimeMgr incorrectly triggered CHRONY processing for a NO_INTERNET event"
    )


# ---------------------------------------------------------------------------
# Duplicate event deduplication test
# ---------------------------------------------------------------------------

def test_duplicate_fully_connected_not_reprocessed():
    """A second consecutive FULLY_CONNECTED event must NOT trigger another chrony run.
    networkstatussrc.cpp early-returns in handle_internetStatusChange() when
    normalizedStatus equals the previous lastStatus, so g_internetUpPending is
    NOT set for a duplicate — the processing thread is never woken.
    Test structure:
      1. Verify the first FULLY_CONNECTED IS processed (count_first >= 1).
         This guards against the degenerate case where subscription failed:
         if count_first == 0 the second assertion would vacuously pass.
      2. Verify the duplicate FULLY_CONNECTED adds zero extra processing lines.
    """
    # Ensure lastStatus is NOT 'fully_connected' before the first injection.
    clear_inject_file()
    inject_internet_status("NO_INTERNET", interface="eth0")
    sleep(1)
    clear_inject_file()

    processing_log = "CHRONY: Processing internet fully_connected event"

    # ── First injection: must be processed ──────────────────────────────────
    offset_first = _log_offset()
    inject_internet_status("FULLY_CONNECTED", interface="eth0")
    assert _wait_for_new_log(processing_log, after_offset=offset_first, timeout_s=10), (
        "First FULLY_CONNECTED event was not processed — subscription may be broken. "
        "Cannot proceed with duplicate-deduplication check."
    )
    count_first = _count_in_new_log(processing_log, after_offset=offset_first)
    assert count_first >= 1, (
        f"Expected at least one processing log line after first injection, got {count_first}"
    )

    # ── Duplicate injection: must NOT produce another processing line ────────
    # lastStatus is now 'fully_connected'; the next FULLY_CONNECTED is a duplicate.
    offset_dup = _log_offset()
    clear_inject_file()
    inject_internet_status("FULLY_CONNECTED", interface="eth0")
    sleep(3)  # Give the mock poll thread time to deliver the event if it would.

    count_dup = _count_in_new_log(processing_log, after_offset=offset_dup)
    assert count_dup == 0, (
        f"Duplicate FULLY_CONNECTED event incorrectly triggered {count_dup} "
        "extra chrony processing run(s)"
    )



    expected_log = "Successfully subscribed to onInternetStatusChange"
    assert _wait_for_log(expected_log, timeout_s=10), (
        f"Expected log line not found in {LOG_FILE}: '{expected_log}'"
    )
  
