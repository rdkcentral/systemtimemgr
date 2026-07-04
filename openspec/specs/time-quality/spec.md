## Purpose

Defines the time quality state machine transitions — from Poor (initial/NTP fail) to Good (NTP or DTT acquired) to Secure (DRM acquired) — and the IARM broadcast emitted at each transition.

---

## State Machine Diagram

```mermaid
stateDiagram-v2
    [*] --> Initializing

    Initializing --> NTPWait : On Init\n(initialize() complete)

    NTPWait --> NTPFailed      : On TimerExpiry\n(ntpFailed → Poor broadcast)
    NTPWait --> NTPAcquired    : On NTPAcquired\n(ntpAquired → Good broadcast)
    NTPWait --> SecureTimeAcquired : On SecureTimeAcquired\n(secureTimeAcquired → quality=Good)

    NTPAcquired --> Running    : On SecureTimeAcquired\n(updateSecureTime → Secure broadcast)
    NTPAcquired --> NTPAcquired : On TimerExpiry\n(updateTime — clock file updated)

    NTPFailed --> NTPAcquired  : On NTPAcquired\n(ntpAquired → Good broadcast)
    NTPFailed --> DTTAcquired  : On DTTAcquired\n(dttAquired → Good broadcast)
    NTPFailed --> SecureTimeAcquired : On SecureTimeAcquired\n(secureTimeAcquired → quality=Good)
    NTPFailed --> NTPFailed    : On TimerExpiry\n(updateTime — clock file updated)

    DTTAcquired --> NTPAcquired    : On NTPAcquired\n(ntpAquired → Good broadcast)
    DTTAcquired --> SecureTimeAcquired : On SecureTimeAcquired\n(secureTimeAcquired → quality=Good)
    DTTAcquired --> DTTAcquired    : On TimerExpiry\n(updateClockRealTime)

    SecureTimeAcquired --> Running      : On NTPAcquired\n(updateSecureTime → Secure broadcast)
    SecureTimeAcquired --> SecureTimeAcquired : On TimerExpiry\n(updateTime — clock file updated)

    Running --> Running : On TimerExpiry\n(timerExpiry — clock file updated)

    note right of NTPWait
      Timer interval = 10 minutes (600 000 ms)
      In all states except Initializing,
      TimerExpiry is handled.
    end note

    note right of Running
      RUNNING is the terminal steady state.
      Quality = Secure.
      No further state transitions
      from the state machine map.
    end note
```

> **Wiki vs. code discrepancy**: The official Confluence state diagram shows a `Running → SecureTimeAcquired` transition on `SecureTimeAcquired` event. That transition does **not** exist in the code's `stateMachine` map and is a documentation artifact. The diagram above reflects the actual code.

---

## Sequence Diagram

The sequence diagram below shows the full thread model. Threads in parentheses are only started when `m_chronyRfcEnabled` is `true`.

```mermaid
sequenceDiagram
    participant main
    participant SysTimeMgr
    participant MsgProcessingThread
    participant TimerThread
    participant PathThread
    participant NwEventProcessThread as nwEventProcessThrd (RFC only)
    participant NwEventSubscribeThread as nwEventSubscribeThrd (RFC only)
    participant NtpSyncMonitorThread as ntpSyncMonitorThrd (RFC only)
    participant ITimeSrc as NtpTimeSrc / RegularTimeSrc
    participant ITimeSync as RdkDefaultTimeSync

    main->>SysTimeMgr: get_instance()
    main->>SysTimeMgr: initialize()
    Note over SysTimeMgr: Reads /etc/systimemgr.conf,<br/>creates ITimeSrc + ITimeSync plugins,<br/>chronyctl_init() if RFC enabled,<br/>creates IARM publish/subscribe,<br/>setInitialTime() → Poor broadcast,<br/>builds stateMachine map,<br/>m_state = NTP_WAIT
    SysTimeMgr-->>main: Initialization Done

    main->>SysTimeMgr: run()
    SysTimeMgr->>MsgProcessingThread: Start Thread (processThr)
    SysTimeMgr->>TimerThread: Start Thread (timerThr)
    SysTimeMgr->>PathThread: Start Thread (pathThr)
    SysTimeMgr->>NwEventProcessThread: Start Thread (RFC only)
    SysTimeMgr->>NwEventSubscribeThread: Start Thread (RFC only)
    SysTimeMgr->>NtpSyncMonitorThread: Start Thread (RFC only)

    loop Every 10 minutes
        TimerThread->>MsgProcessingThread: sendMessage(TIMER_EXPIRY)
    end

    Note over PathThread: inotify watches /tmp/systimemgr/<br/>On IN_ATTRIB: ntp / stt / drm / dtt
    PathThread->>MsgProcessingThread: sendMessage(NTP_AVAILABLE)
    PathThread->>MsgProcessingThread: sendMessage(SECURE_TIME_AVAILABLE)
    PathThread->>MsgProcessingThread: sendMessage(DTT_TIME_AVAILABLE)

    Note over NtpSyncMonitorThread: Polls adjtimex() every 1 s.<br/>On kernel NTP sync: touches /tmp/systimemgr/ntp<br/>(inotify fires NTP_AVAILABLE),<br/>creates /tmp/clock-event,<br/>writes /tmp/ntp_status = "Synchronized"
    NtpSyncMonitorThread->>PathThread: (inotify IN_ATTRIB on /tmp/systimemgr/ntp)

    Note over NwEventSubscribeThread: Subscribes to org.rdk.NetworkManager<br/>onInternetStatusChange via Thunder JSONRPC.<br/>Retries every 1 s until success.
    NwEventSubscribeThread->>NwEventProcessThread: (signals cv on fully_connected)

    Note over NwEventProcessThread: Runs processInternetOnline():<br/>chronyctl_online/burst/waitsync/makestep<br/>based on /tmp/clock-event sentinel<br/>and chrony source state.

    MsgProcessingThread->>SysTimeMgr: runStateMachine(event, args)
    SysTimeMgr->>ITimeSrc: getTimeSec() [on timerExpiry]
    SysTimeMgr->>ITimeSync: updateTime(reftime) [on timerExpiry]
    SysTimeMgr-->>MsgProcessingThread: IARM broadcast (Poor / Good / Secure)
```

---

## Requirements
### Requirement: Initial Time Quality is Poor

When SystemTimeManager starts, time quality MUST be `Poor` until a reliable time source is acquired.

#### Scenario: Quality is Poor at startup

- **WHEN** SystemTimeManager completes `setInitialTime()`
- **THEN** `m_timequality` is set to `eTIMEQUALILTY_POOR`
- **AND** an IARM broadcast with message `"Poor"` is emitted

---

### Requirement: NTP Timeout Transitions Quality to Poor (NTP_FAIL)

If NTP is not acquired within the timer interval, the state machine MUST record NTP failure and broadcast `Poor` quality.

#### Scenario: NTP timer expires without sync

- **WHEN** the state is `eSYSMGR_STATE_NTP_WAIT`
- **AND** a `eSYSMGR_EVENT_TIMER_EXPIRY` event fires before NTP is acquired
- **THEN** `ntpFailed()` is called
- **AND** state transitions to `eSYSMGR_STATE_NTP_FAIL`
- **AND** `publishStatus(ePUBLISH_NTP_FAIL, "Poor")` is called
- **AND** an IARM broadcast with message `"Poor"` is emitted

---

### Requirement: NTP Acquisition Transitions Quality to Good

When the NTP time source signals availability, time quality MUST be upgraded to `Good`.

#### Scenario: NTP becomes available in NTP_WAIT state

- **WHEN** the state is `eSYSMGR_STATE_NTP_WAIT`
- **AND** a `eSYSMGR_EVENT_NTP_AVAILABLE` event is received
- **THEN** `ntpAquired()` is called
- **AND** `m_timequality` is set to `eTIMEQUALILTY_GOOD`
- **AND** state transitions to `eSYSMGR_STATE_NTP_ACQUIRED`
- **AND** `m_timersrc` is set to `"NTP"`
- **AND** `publishStatus(ePUBLISH_NTP_SUCCESS, "Good")` is called
- **AND** an IARM broadcast with message `"Good"` is emitted

#### Scenario: NTP becomes available after NTP_FAIL

- **WHEN** the state is `eSYSMGR_STATE_NTP_FAIL`
- **AND** a `eSYSMGR_EVENT_NTP_AVAILABLE` event is received
- **THEN** `ntpAquired()` is called
- **AND** state transitions to `eSYSMGR_STATE_NTP_ACQUIRED` with quality `eTIMEQUALILTY_GOOD`
- **AND** an IARM broadcast with message `"Good"` is emitted

---

### Requirement: DTT Acquisition Transitions Quality to Good

When DTT time becomes available (NTP having already failed), time quality MUST be upgraded to `Good` via the DTT path.

#### Scenario: DTT acquired after NTP failure

- **WHEN** the state is `eSYSMGR_STATE_NTP_FAIL`
- **AND** a `eSYSMGR_EVENT_DTT_TIME_AVAILABLE` event is received
- **THEN** `dttAquired()` is called
- **AND** `m_timequality` is set to `eTIMEQUALILTY_GOOD`
- **AND** state transitions to `eSYSMGR_STATE_DTT_ACQUIRED`
- **AND** `m_timersrc` is set to `"DTT"`
- **AND** `publishStatus(ePUBLISH_DTT_SUCCESS, "Good")` is called
- **AND** an IARM broadcast with message `"Good"` is emitted

---

### Requirement: DRM Secure Time Transitions Quality from Good to Secure

When DRM (secure time) becomes available after NTP or DTT has already been acquired, time quality MUST be upgraded to `Secure`.

#### Scenario: Secure time acquired after NTP

- **WHEN** the state is `eSYSMGR_STATE_NTP_ACQUIRED`
- **AND** a `eSYSMGR_EVENT_SECURE_TIME_AVAILABLE` event is received
- **THEN** `updateSecureTime()` is called
- **AND** `m_timequality` is set to `eTIMEQUALILTY_SECURE`
- **AND** state transitions to `eSYSMGR_STATE_RUNNING`
- **AND** `publishStatus(ePUBLISH_SECURE_TIME_SUCCESS, "Secure")` is called
- **AND** an IARM broadcast with message `"Secure"` is emitted

---

### Requirement: Time Quality Query Response

When any component queries the current time quality via `TIMER_STATUS_MSG`, SystemTimeManager MUST return the current quality enum value with a matching string label.

#### Scenario: Quality reported as Poor

- **WHEN** `getTimeStatus()` is called
- **AND** `m_timequality` is `eTIMEQUALILTY_POOR`
- **THEN** the `TimerMsg` is populated with quality `eTIMEQUALILTY_POOR` and message `"Poor"`

#### Scenario: Quality reported as Good

- **WHEN** `getTimeStatus()` is called
- **AND** `m_timequality` is `eTIMEQUALILTY_GOOD`
- **THEN** the `TimerMsg` is populated with quality `eTIMEQUALILTY_GOOD` and message `"Good"`

#### Scenario: Quality reported as Secure

- **WHEN** `getTimeStatus()` is called
- **AND** `m_timequality` is `eTIMEQUALILTY_SECURE`
- **THEN** the `TimerMsg` is populated with quality `eTIMEQUALILTY_SECURE` and message `"Secure"`

