## ADDED Requirements

### Requirement: Subscribe to Power Mode Change Events

SystemTimeManager MUST register for power mode change notifications from either the IARM PowerManager bus or the PowerController API at startup.

#### Scenario: Power Controller connection succeeds at startup

- **WHEN** `IpowerControllerSubscriber::subscribe(POWER_CHANGE_MSG, ...)` is called
- **AND** `PowerController_Connect()` returns `POWER_CONTROLLER_ERROR_NONE`
- **THEN** `PowerController_RegisterPowerModeChangedCallback` is called with `sysTimeMgrPwrEventHandler`
- **AND** the callback is registered successfully

#### Scenario: Power Controller connection fails at startup — retry thread launched

- **WHEN** `IpowerControllerSubscriber::subscribe(POWER_CHANGE_MSG, ...)` is called
- **AND** `PowerController_Connect()` does not return `POWER_CONTROLLER_ERROR_NONE`
- **THEN** a background thread `sysTimeMgrPwrConnectHandlingThreadFunc` is started
- **AND** the thread retries `PowerController_Connect()` every 300 ms until success
- **AND** `PowerController_RegisterPowerModeChangedCallback` is called once connection is established

#### Scenario: Fallback to IARM PowerManager subscription

- **WHEN** IARM bus support is enabled and PowerController is not available
- **AND** `IarmPowerSubscriber::subscribe(POWER_CHANGE_MSG, ...)` is called
- **THEN** `IARM_Bus_RegisterEventHandler` is called for `IARM_BUS_PWRMGR_EVENT_MODECHANGED`

---

### Requirement: Deep Sleep Entry Handling

When the system enters deep sleep, SystemTimeManager MUST record the event but defer state changes until wake-up.

#### Scenario: Power state changes to OFF or STANDBY_DEEP_SLEEP

- **WHEN** a power mode change event is received
- **AND** the new state is `IARM_BUS_PWRMGR_POWERSTATE_OFF` or `IARM_BUS_PWRMGR_POWERSTATE_STANDBY_DEEP_SLEEP`
- **THEN** `powerhandler` is called with `"DEEP_SLEEP_ON"`
- **AND** `deepsleepon()` is invoked on the SysTimeMgr instance
- **AND** no time quality broadcast is emitted at this point

---

### Requirement: Deep Sleep Exit (Wake-up) Handling

When the system wakes from deep sleep, SystemTimeManager MUST reset its state, re-publish the current time quality, and restart the NTP synchronization service.

#### Scenario: Power state transitions from STANDBY_DEEP_SLEEP to ON or STANDBY

- **WHEN** a power mode change event is received
- **AND** the previous state is `IARM_BUS_PWRMGR_POWERSTATE_STANDBY_DEEP_SLEEP`
- **AND** the new state is `IARM_BUS_PWRMGR_POWERSTATE_ON`, `IARM_BUS_PWRMGR_POWERSTATE_STANDBY_LIGHT_SLEEP`, or `IARM_BUS_PWRMGR_POWERSTATE_STANDBY`
- **THEN** `powerhandler` is called with `"DEEP_SLEEP_OFF"`
- **AND** `deepsleepoff()` is invoked on the SysTimeMgr instance

#### Scenario: Secure quality downgraded to Good on wake

- **WHEN** `deepsleepoff()` executes
- **AND** `m_timequality` is `eTIMEQUALILTY_SECURE`
- **THEN** `m_timequality` is downgraded to `eTIMEQUALILTY_GOOD`
- **AND** state transitions to `eSYSMGR_STATE_NTP_ACQUIRED`
- **AND** `m_timersrc` is reset to `"Last Known"`

#### Scenario: Time quality re-broadcast on wake

- **WHEN** `deepsleepoff()` executes
- **THEN** `publishStatus(ePUBLISH_DEEP_SLEEP_ON, <current quality string>)` is called
- **AND** an IARM broadcast with the current quality (Poor, Good, or Secure) is emitted

#### Scenario: NTP service restarted on wake

- **WHEN** `deepsleepoff()` executes
- **AND** `systemd-timesyncd.service` is active
- **THEN** `systemctl reset-failed systemd-timesyncd.service` and `systemctl restart systemd-timesyncd.service` are invoked via `v_secure_system`

#### Scenario: Chronyd burst triggered on wake if chronyd is active

- **WHEN** `deepsleepoff()` executes
- **AND** `systemd-timesyncd.service` is not active
- **AND** `chronyd.service` is active
- **THEN** `chronyc burst 3/4` is executed via `v_secure_system`

---

### Requirement: Power Subscriber Cleanup on Destruction

When the power subscriber is destroyed, PowerController resources MUST be released.

#### Scenario: Destructor releases PowerController resources

- **WHEN** `IpowerControllerSubscriber` is destroyed
- **THEN** `sysTimeMgrDeinitPwrEvt()` is called to clean up the event queue and condition variable
- **AND** `PowerController_Term()` is called to release the Power Controller client
