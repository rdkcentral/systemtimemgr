## ADDED Requirements

### Requirement: Last Known Good Time Recovery

On startup, SystemTimeManager MUST restore time from the last known good time stored in the TimeSync layer before waiting for any time source.

#### Scenario: Last known time is available from TimeSync

- **WHEN** SystemTimeManager starts and calls `setInitialTime()`
- **AND** at least one ITimeSync returns a non-zero time
- **THEN** `clock_settime(CLOCK_REALTIME, ...)` is called with the recovered time
- **AND** the milestone `SYSTEM_TIME_SET` is logged
- **AND** time quality is set to `eTIMEQUALILTY_POOR`

#### Scenario: Current real-time clock is already ahead of TimeSync value

- **WHEN** SystemTimeManager starts and calls `setInitialTime()`
- **AND** the current `CLOCK_REALTIME` value is greater than the value returned by TimeSync
- **THEN** `clock_settime` is NOT called (existing time is preserved)
- **AND** a `Poor` quality status is published via IARM broadcast

#### Scenario: No TimeSync value available

- **WHEN** SystemTimeManager starts and calls `setInitialTime()`
- **AND** all ITimeSync objects return zero
- **THEN** `clock_settime` is NOT called
- **AND** SystemTimeManager proceeds without setting the clock

---

### Requirement: Sentinel File Creation on Startup

SystemTimeManager MUST create `/tmp/systimeset` on startup to signal that initial time setup has been attempted.

#### Scenario: Sentinel file is created

- **WHEN** `setInitialTime()` is called during startup
- **THEN** the file `/tmp/systimeset` is created
- **AND** the file exists on the filesystem after initialization completes

---

### Requirement: Initial Poor Quality Broadcast

After setting (or skipping) the initial time, SystemTimeManager MUST broadcast a `Poor` time quality event via IARM.

#### Scenario: Initial time quality is broadcast as Poor

- **WHEN** `setInitialTime()` completes
- **THEN** `publishStatus(ePUBLISH_TIME_INITIAL, "Poor")` is called
- **AND** an IARM broadcast event is emitted with quality `eTIMEQUALILTY_POOR` and message `"Poor"`

---

### Requirement: Returning Last Known Good Time on Query

When a `TIMER_STATUS_MSG` query arrives before any high-quality time source is acquired, SystemTimeManager MUST respond with the last known good time.

#### Scenario: Status query returns last known good time

- **WHEN** an IARM `TIMER_STATUS_MSG` event is received
- **THEN** `getTimeStatus()` returns the current `qualityOfTime` value and the current `CLOCK_REALTIME` time
- **AND** the log contains `"Returning Last Known Good Time"` or equivalent status message
