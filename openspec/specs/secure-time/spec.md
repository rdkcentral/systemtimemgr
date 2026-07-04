## Purpose

Defines how SystemTimeManager detects DRM secure time availability via inotify on `/tmp/systimemgr/drm` and triggers the secure time acquisition state transitions.
## Requirements
### Requirement: Secure Time Detection via Path Monitor

SystemTimeManager MUST monitor `/tmp/systimemgr/drm` using inotify. When the file is created or modified, it MUST trigger the secure time acquisition flow.

#### Scenario: DRM file created or modified at runtime

- **WHEN** the path monitor detects an `IN_ATTRIB` event on `/tmp/systimemgr/drm`
- **THEN** a `eSYSMGR_EVENT_SECURE_TIME_AVAILABLE` message is sent to the processing queue
- **AND** the state machine processes the event in the current state

#### Scenario: DRM file already exists on startup

- **WHEN** SystemTimeManager starts and `/tmp/systimemgr/drm` already exists
- **THEN** a `eSYSMGR_EVENT_SECURE_TIME_AVAILABLE` event is immediately enqueued
- **AND** the system does not wait for an inotify event to trigger the secure time flow

---

### Requirement: Secure Time Acquisition from NTP_WAIT

If DRM secure time is detected while waiting for NTP, the `secureTimeAcquired` transition MUST be triggered.

#### Scenario: Secure time arrives before NTP

- **WHEN** the state is `eSYSMGR_STATE_NTP_WAIT`
- **AND** a `eSYSMGR_EVENT_SECURE_TIME_AVAILABLE` event is received
- **THEN** `secureTimeAcquired()` is called
- **AND** `m_timequality` is set to `eTIMEQUALILTY_GOOD`
- **AND** state transitions to `eSYSMGR_STATE_SECURE_TIME_ACQUIRED`

---

### Requirement: Secure Time Acquisition from NTP_FAIL

If DRM secure time is detected after NTP has failed, the secure time transition MUST still complete successfully.

#### Scenario: Secure time arrives after NTP failure

- **WHEN** the state is `eSYSMGR_STATE_NTP_FAIL`
- **AND** a `eSYSMGR_EVENT_SECURE_TIME_AVAILABLE` event is received
- **THEN** `secureTimeAcquired()` is called
- **AND** state transitions to `eSYSMGR_STATE_SECURE_TIME_ACQUIRED` with quality `eTIMEQUALILTY_GOOD`

---

### Requirement: Secure Time Event Published After NTP Acquisition

When both NTP and DRM are available, the final `Secure` quality broadcast MUST be emitted.

#### Scenario: NTP acquired then DRM secure time arrives

- **WHEN** the state is `eSYSMGR_STATE_NTP_ACQUIRED`
- **AND** a `eSYSMGR_EVENT_SECURE_TIME_AVAILABLE` event is received
- **THEN** `updateSecureTime()` is called
- **AND** `publishStatus(ePUBLISH_SECURE_TIME_SUCCESS, "Secure")` is called
- **AND** an IARM broadcast with message `"Secure"` and quality `eTIMEQUALILTY_SECURE` is emitted
- **AND** state transitions to `eSYSMGR_STATE_RUNNING`

