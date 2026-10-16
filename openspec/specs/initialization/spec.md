## Purpose

Defines the startup behavior of SystemTimeManager including single-instance enforcement, log file creation, TimeSrc/TimeSync plugin loading, and state machine initialization.
## Requirements
### Requirement: Single-Instance Enforcement

SystemTimeManager MUST ensure only one instance runs at a time. If a second invocation is attempted while an instance is already running, it MUST exit without starting.

#### Scenario: First invocation starts the process

- **WHEN** the SystemTimeManager binary is invoked and no instance is already running
- **THEN** a new SysTimeMgr singleton instance is created
- **AND** the process writes its PID to `/run/systimemgr.pid`

#### Scenario: Second invocation is rejected

- **WHEN** the SystemTimeManager binary is invoked while an instance is already running
- **THEN** the second invocation must exit immediately
- **AND** no additional SysTimeMgr instance is created

---

### Requirement: Log File Generation

SystemTimeManager MUST produce a log file during startup to confirm it is running.

#### Scenario: Log file is created on startup

- **WHEN** the SystemTimeManager binary is invoked
- **THEN** the SystemTimeManager log file must be generated
- **AND** the log file must be accessible for reading

---

### Requirement: TimeSrc and TimeSync Plugin Loading

SystemTimeManager MUST load time source and time sync plugins from its configuration file at startup.

#### Scenario: Plugins loaded from config file

- **WHEN** the SystemTimeManager is initializing
- **AND** the config file `/etc/systimemgr.conf` is present and readable
- **THEN** all `timesrc` entries in the config file are instantiated as ITimeSrc objects
- **AND** all `timesync` entries in the config file are instantiated as ITimeSync objects

#### Scenario: Degraded mode when config file is absent

- **WHEN** the SystemTimeManager is initializing
- **AND** the config file `/etc/systimemgr.conf` is not present or not readable
- **THEN** SystemTimeManager logs an error and continues in degraded mode
- **AND** the process does not crash or exit

---

### Requirement: State Machine Initialization

SystemTimeManager MUST enter the `NTP_WAIT` state after initialization completes.

#### Scenario: State machine starts in NTP_WAIT

- **WHEN** the SystemTimeManager completes initialization
- **THEN** the internal state is set to `eSYSMGR_STATE_NTP_WAIT`
- **AND** the processing thread, timer thread, and path monitor thread are all running

