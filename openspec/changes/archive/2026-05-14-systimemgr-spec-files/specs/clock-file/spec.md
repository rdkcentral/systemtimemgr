## ADDED Requirements

### Requirement: Clock File Updated on Timer Expiry

SystemTimeManager MUST persist the current time to `/opt/secure/clock.txt` (via the TimeSync layer) on each periodic timer expiry so that a last known good time is available on the next boot.

#### Scenario: Clock file updated during RUNNING state

- **WHEN** the state is `eSYSMGR_STATE_RUNNING` or `eSYSMGR_STATE_NTP_ACQUIRED`
- **AND** a `eSYSMGR_EVENT_TIMER_EXPIRY` event fires
- **THEN** `timerExpiry()` or `updateTime()` is called
- **AND** `updateTimeSync()` is called for all registered ITimeSync objects
- **AND** the ITimeSync implementation writes the current time to `/opt/secure/clock.txt`

---

### Requirement: Clock File Updated After DTT Acquisition

When DTT time is acquired, the clock file MUST be updated to reflect the new (Good-quality) time.

#### Scenario: Clock file updated after DTT timer expiry

- **WHEN** the state is `eSYSMGR_STATE_DTT_ACQUIRED`
- **AND** a `eSYSMGR_EVENT_TIMER_EXPIRY` event fires
- **THEN** `updateClockRealTime()` is called
- **AND** the ITimeSync layer updates `/opt/secure/clock.txt` with the current real time

---

### Requirement: Clock File Reflects Accurate Time

The value written to `/opt/secure/clock.txt` MUST be derived from the reference time source, validated against any secondary time sources.

#### Scenario: Reference time used when within tolerance

- **WHEN** a reference ITimeSrc (`isreference() == true`) provides time `reftime`
- **AND** a secondary ITimeSrc provides time `filetime`
- **AND** `(reftime - filetime) <= 600` seconds
- **THEN** `updateTimeSync(reftime)` is called with the reference time

#### Scenario: Zero time from secondary source is ignored

- **WHEN** a secondary ITimeSrc returns zero
- **THEN** only the reference time source value is used for `updateTimeSync`
- **AND** no write to `/opt/secure/clock.txt` occurs with a zero timestamp
