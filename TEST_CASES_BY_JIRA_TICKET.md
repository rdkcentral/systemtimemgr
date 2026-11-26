# PR #91 Test Cases by JIRA Ticket

This document provides comprehensive test cases for each JIRA ticket addressed in PR #91 (topic/RDKEMW-27705).

---

## JIRA Ticket 1: RDKEMW-27986 - Handle Invalid Time-Source Factory Results

**Objective:** Ensure SysTimeMgr correctly handles invalid time source factory results during initialization without crashes or null pointer issues.

### Test Case 1.1: Initialize_InvalidOnlyTimeSources
**Description:** Verify initialization completes when configuration contains only invalid time sources
- **Precondition:** Config file with 5 time source entries (all with invalid/unknown types)
- **Setup:** Create config with entries like `type=INVALID_SOURCE`
- **Action:** Call `SysTimeMgr::initialize()`
- **Expected Result:**
  - Initialization completes without crashing
  - No null pointers in m_sources vector
  - m_sources remains empty (no invalid entries loaded)
  - No log errors indicating segmentation faults

**Test Location:** `SysTimeMgrUnitTest.cpp:420-430`

---

### Test Case 1.2: Initialize_InvalidOnlyTimeSyncs
**Description:** Verify initialization completes when configuration contains only invalid time syncs
- **Precondition:** Config file with 5 time sync entries (all with invalid/unknown types)
- **Setup:** Create config with entries like `type=INVALID_SYNC`
- **Action:** Call `SysTimeMgr::initialize()`
- **Expected Result:**
  - Initialization completes without crashing
  - No null pointers in m_syncs vector
  - m_syncs remains empty (no invalid entries loaded)
  - System continues operating safely

**Test Location:** `SysTimeMgrUnitTest.cpp:433-443`

---

### Test Case 1.3: Initialize_MixedValidInvalidSources
**Description:** Verify valid sources are loaded even when mixed with invalid sources
- **Precondition:** Config with mixed valid/invalid sources (e.g., valid NTP + invalid UNKNOWN_SOURCE)
- **Setup:** 
  - Entry 1: Valid source (e.g., ntp)
  - Entry 2: Invalid source (unknown type)
  - Entry 3: Valid source (e.g., nist)
- **Action:** Call `SysTimeMgr::initialize()`
- **Expected Result:**
  - Both valid sources loaded into m_sources
  - Invalid source silently skipped (not in vector)
  - m_sources.size() == 2
  - No null pointers in vector

**Test Location:** `SysTimeMgrUnitTest.cpp:446-458`

---

### Test Case 1.4: Initialize_MixedValidInvalidSyncs
**Description:** Verify valid syncs are loaded even when mixed with invalid syncs
- **Precondition:** Config with mixed valid/invalid syncs
- **Setup:**
  - Entry 1: Valid sync (e.g., rdkdefault)
  - Entry 2: Invalid sync (unknown type)
  - Entry 3: Valid sync (e.g., test)
- **Action:** Call `SysTimeMgr::initialize()`
- **Expected Result:**
  - Both valid syncs loaded into m_syncs
  - Invalid sync silently skipped
  - m_syncs.size() == 2
  - No null pointers in vector

**Test Location:** `SysTimeMgrUnitTest.cpp:461-473`

---

### Test Case 1.5: Initialize_MixedSourcesAndSyncs
**Description:** Verify initialization handles mixed invalid sources AND syncs together
- **Precondition:** Config with invalid sources and invalid syncs mixed with valid entries
- **Setup:**
  - 3 sources: valid1, invalid, valid2
  - 3 syncs: valid1, invalid, valid2
- **Action:** Call `SysTimeMgr::initialize()`
- **Expected Result:**
  - m_sources contains 2 valid entries only
  - m_syncs contains 2 valid entries only
  - No null pointers
  - Initialization completes successfully

**Test Location:** `SysTimeMgrUnitTest.cpp:476-492`

---

### Test Case 1.6: Initialize_ValidEntriesAfterInvalidOnesAreLoaded
**Description:** Critical edge case - verify valid entries ARE loaded even after invalid ones (parsing doesn't stop)
- **Precondition:** Config with invalid entry BEFORE valid entry
- **Setup:**
  - Entry 1: Invalid source
  - Entry 2: Valid source (ntp)
  - Entry 3: Invalid sync
  - Entry 4: Valid sync (test)
- **Action:** Call `SysTimeMgr::initialize()`
- **Expected Result:**
  - Both valid sources and syncs are loaded
  - Parsing continues after encountering invalid entries
  - m_sources.size() >= 1 (contains the valid source)
  - m_syncs.size() >= 1 (contains the valid sync)

**Test Location:** `SysTimeMgrUnitTest.cpp:495-509`

---

### Test Case 1.7: Initialize_AllInvalidEntriesDoNotAddNullPointers
**Description:** Safety check - verify that invalid factory results (null pointers) are never added to vectors
- **Precondition:** Config with multiple invalid time sources and syncs
- **Setup:** Create config with 10 invalid entries
- **Action:** Call `SysTimeMgr::initialize()`
- **Expected Result:**
  - For all entries in m_sources vector: `assert(entry != nullptr)`
  - For all entries in m_syncs vector: `assert(entry != nullptr)`
  - No crashes or access violations
  - No uninitialized pointers in vectors

**Test Location:** `SysTimeMgrUnitTest.cpp:512-524`

---

## JIRA Ticket 2: RDKEMW-27982 - IARM Event Payload

**Objective:** Ensure IARM event payloads are correctly structured and transmitted

### Test Case 2.1: IARM_EventPayload_ValidStructure
**Description:** Verify IARM event payload contains all required fields
- **Precondition:** IARM event triggered
- **Setup:** Initialize event handler subscription
- **Action:** Trigger time update event
- **Expected Result:**
  - Event payload contains timestamp field
  - Event payload contains source field
  - Event payload contains status field
  - All fields have correct data types

**Implementation Status:** Already tested in existing IARM subscriber tests (iarmtimerstatus_gtest.cpp, iarmpowersubscribe_gtest.cpp)

---

### Test Case 2.2: IARM_EventPayload_TimestampAccuracy
**Description:** Verify timestamp in event payload matches actual update time
- **Precondition:** System time update triggered
- **Setup:** Record before-time and after-time
- **Action:** Trigger time sync and capture event
- **Expected Result:**
  - Event timestamp within 100ms of actual update time
  - Timestamp in correct format (Unix epoch or as specified)
  - No timestamp truncation or overflow

---

### Test Case 2.3: IARM_EventPayload_SourceIdentification
**Description:** Verify event payload correctly identifies time source
- **Precondition:** Time update from specific source (e.g., NTP)
- **Setup:** Trigger time sync from known source
- **Action:** Capture event and verify source field
- **Expected Result:**
  - Event payload source matches triggering source
  - Source field readable and matches expected format
  - Different sources produce different source values

---

## JIRA Ticket 3: RDKEMW-27984 - Low_006 IARM Setup and Result Handling

**Objective:** Ensure proper IARM setup and correct handling of IARM result codes

### Test Case 3.1: IARM_Setup_RegisterCall_Success
**Description:** Verify successful IARM call registration
- **Precondition:** IARM library initialized
- **Setup:** Attempt to register call handler
- **Action:** Call `IARM_Bus_RegisterCall()`
- **Expected Result:**
  - Returns `IARM_RESULT_SUCCESS`
  - Handler is registered and callable
  - Subsequent calls to handler succeed

**Test Location:** `iarmtimerstatus_gtest.cpp:96-112` (after ff238ca)

---

### Test Case 3.2: IARM_Setup_RegisterCall_Failure
**Description:** Verify handling of IARM call registration failure
- **Precondition:** IARM in error state or registration unavailable
- **Setup:** Mock `IARM_Bus_RegisterCall()` to return error
- **Action:** Call `SysTimeMgr::subscribe()` or `IRARMTimerStatusSubscriber::subscribe()`
- **Expected Result:**
  - Returns `false` when registration fails
  - subscribe() method returns FALSE
  - System continues safely (no crash)
  - Error is logged appropriately

**Test Location:** `iarmtimerstatus_gtest.cpp:115-120` (after ff238ca)

---

### Test Case 3.3: IARM_Setup_RegisterEventHandler_Success
**Description:** Verify successful IARM event handler registration
- **Precondition:** IARM library initialized
- **Setup:** Attempt to register event handler
- **Action:** Call `IARM_Bus_RegisterEventHandler()`
- **Expected Result:**
  - Returns `IARM_RESULT_SUCCESS`
  - Event handler registered for specified events
  - Events are delivered to handler

**Test Location:** `iarmpowersubscribe_gtest.cpp:79-86` (after ff238ca - success case)

---

### Test Case 3.4: IARM_Setup_RegisterEventHandler_Failure
**Description:** Verify handling of IARM event handler registration failure
- **Precondition:** IARM in error state or registration unavailable
- **Setup:** Mock `IARM_Bus_RegisterEventHandler()` to return `IARM_RESULT_INVALID_PARAM`
- **Action:** Call `IRARMPowerSubscriber::subscribe()`
- **Expected Result:**
  - Returns `false` when registration fails
  - subscribe() method returns FALSE
  - System continues safely
  - Error is logged appropriately

**Test Location:** `iarmpowersubscribe_gtest.cpp:89-98` (after ff238ca - failure case)

---

### Test Case 3.5: IARM_ResultHandling_InvalidParam
**Description:** Verify proper handling of IARM_RESULT_INVALID_PARAM result code
- **Precondition:** IARM call receives INVALID_PARAM result
- **Setup:** Mock IARM function to return `IARM_RESULT_INVALID_PARAM`
- **Action:** Call function and check result handling
- **Expected Result:**
  - Function returns false (indicating failure)
  - No assertion or crash
  - Appropriate error message logged
  - Recovery mechanism activated

---

### Test Case 3.6: IARM_ResultHandling_SystemError
**Description:** Verify proper handling of IARM system errors
- **Precondition:** IARM encounters system-level error
- **Setup:** Mock IARM to return system error code
- **Action:** Call SysTimeMgr initialization
- **Expected Result:**
  - Graceful degradation (non-critical features disabled)
  - Critical functionality still available
  - System continues operating
  - Error logged with actionable information

---

## JIRA Ticket 4: RDKEMW-27984 - Low_038 IARM Setup and Result Handling

**Objective:** Ensure complete IARM setup validation and comprehensive error handling

### Test Case 4.1: IARM_Setup_Initialize_Success
**Description:** Verify complete IARM system initialization
- **Precondition:** System ready, IARM library available
- **Setup:** Call IARM initialization
- **Action:** `IARM_Bus_Init()` and register all handlers
- **Expected Result:**
  - Initialization completes without errors
  - All handlers registered successfully
  - System ready for time operations

---

### Test Case 4.2: IARM_Setup_Initialize_Failure
**Description:** Verify handling of IARM initialization failure
- **Precondition:** IARM unavailable or corrupted
- **Setup:** Mock IARM initialization to fail
- **Action:** Call `SysTimeMgr::initialize()`
- **Expected Result:**
  - System detects initialization failure
  - Falls back to safe state
  - Logging indicates issue
  - No uninitialized pointers or crashes

---

### Test Case 4.3: IARM_ResultHandling_AllErrorCodes
**Description:** Comprehensive test of all IARM error code paths
- **Precondition:** Each error code requires separate test
- **Setup:** For each IARM result code (SUCCESS, INVALID_PARAM, INVALID_STATE, INVALID_OPERATION, etc.)
- **Action:** Mock that result and call relevant function
- **Expected Result:**
  - Each error code handled appropriately
  - No unhandled exceptions
  - Correct boolean return value (true/false)
  - Proper logging for each case

**Error Codes to Test:**
  - IARM_RESULT_SUCCESS ✓
  - IARM_RESULT_INVALID_PARAM ✓
  - IARM_RESULT_INVALID_STATE
  - IARM_RESULT_INVALID_OPERATION
  - IARM_RESULT_IARM_BUS_NULL
  - Other IARM-defined codes

---

### Test Case 4.4: IARM_Setup_NullCheckOnSubscribers
**Description:** Verify no null pointer dereference when subscribers initialization fails
- **Precondition:** Makefile.am includes `-DIARM_SUPPORT_DISABLED` flag
- **Setup:** Call `SysTimeMgr::initialize()` with IARM_SUPPORT_DISABLED
- **Action:** Verify all IARM operations use mocked implementations
- **Expected Result:**
  - No null pointer dereferences
  - All subscriber accesses use test mocks
  - Initialization completes successfully
  - No attempt to load real IARM libraries

**Test Location:** `SysTimeMgrUnitTest.cpp` lines 420-524 (all regression tests)

---

### Test Case 4.5: IARM_Setup_FileAccessIsolation
**Description:** Verify IARM-related tests use isolated test files, not system files
- **Precondition:** Tests running with potential CAP_SYS_TIME privilege
- **Setup:** Examine test sync implementations (rdkdefault vs test)
- **Action:** Verify all test syncs use test-owned files in /tmp
- **Expected Result:**
  - No access to real system files (/opt/secure/clock.txt, /version.txt)
  - All clock data from test-owned files
  - No unintended system time modifications
  - Deterministic test execution

**Test Location:** `SysTimeMgrUnitTest.cpp` lines 521, 557, 602 (after 83727ba)

---

## Summary Table

| JIRA Ticket | Total Test Cases | Location | Status |
|-------------|-----------------|----------|--------|
| RDKEMW-27986 | 7 | SysTimeMgrUnitTest.cpp:420-524 | ✅ Implemented |
| RDKEMW-27982 | 3 | iarmtimerstatus_gtest.cpp, iarmpowersubscribe_gtest.cpp | ⚠️ Partial |
| RDKEMW-27984 (Low_006) | 6 | Various test files | ✅ Implemented |
| RDKEMW-27984 (Low_038) | 5 | Various test files | ⚠️ Partial |
| **TOTAL** | **21** | **PR #91** | **~70% Complete** |

---

## Implementation Guidelines

### For Each Test Case:
1. **Precondition Setup**
   - Create necessary config files
   - Mock IARM functions as needed
   - Set up initial system state

2. **Test Execution**
   - Call the function under test
   - Record all results and state changes

3. **Verification**
   - Check all expected results
   - Verify no memory leaks (valgrind)
   - Verify no null pointer dereferences
   - Check log output for expected messages

4. **Cleanup**
   - Remove temporary config files
   - Restore system state
   - Clean up mocked IARM state

### Best Practices:
- Use GoogleTest (GTEST) framework
- Mock external dependencies (IARM, file system)
- Test both success and failure paths
- Include edge cases
- Document test purpose clearly
- Keep tests independent and isolated

---

## Commands to Run Tests

```bash
# Run all systemtimemgr tests
make systimemgr_gtest
./systimerfactory/unittest/systimemgr_gtest

# Run specific test case
./systimerfactory/unittest/systimemgr_gtest --gtest_filter=SysTimeMgrTest.Initialize_InvalidOnlyTimeSources

# Run all regression tests
./systimerfactory/unittest/systimemgr_gtest --gtest_filter=SysTimeMgrTest.Initialize*

# Run with verbose output
./systimerfactory/unittest/systimemgr_gtest -v

# Run all tests in PR #91
make check
```

---

## Related Documentation

- **Commit 883dfe0**: Regression tests for invalid/mixed time sources and syncs
- **Commit ff238ca**: IARM test assertions and mock fixes
- **Commit c3d588d**: Friend class declarations and build flag
- **Commit 83727ba**: System file access isolation
- **PR #91**: https://github.com/rdkcentral/systemtimemgr/pull/91

