## ADDED Requirements

### Requirement: IARM Bus Initialization

SystemTimeManager MUST initialize and connect to the IARM bus at startup before publishing or subscribing to any events.

#### Scenario: IARM Bus connected if not already connected

- **WHEN** `IarmPublish` or `IarmSubscriber` is constructed
- **AND** `IARM_Bus_IsConnected()` returns a non-success result
- **THEN** `IARM_Bus_Init()` is called with the component name
- **AND** `IARM_Bus_Connect()` is called to join the bus

#### Scenario: IARM Bus already connected

- **WHEN** `IarmPublish` or `IarmSubscriber` is constructed
- **AND** `IARM_Bus_IsConnected()` returns success
- **THEN** `IARM_Bus_Init()` and `IARM_Bus_Connect()` are NOT called again

---

### Requirement: Time Quality Broadcast via IARM

SystemTimeManager MUST broadcast `TimerMsg` events over the IARM bus whenever time quality changes or a periodic update is due.

#### Scenario: Time quality event broadcast on state transition

- **WHEN** `publishStatus(event, message)` is called
- **THEN** a `TimerMsg` struct is populated with:
  - `event`: the publish event type (`ePUBLISH_*`)
  - `quality`: the current `qualityOfTime` enum value
  - `message`: the quality string (`"Poor"`, `"Good"`, or `"Secure"`)
  - `timerSrc`: the current time source name (`"NTP"`, `"DTT"`, `"Last Known"`, etc.)
  - `currentTime`: the current `CLOCK_REALTIME` timestamp as a string
- **AND** `IARM_Bus_BroadcastEvent(IARM_BUS_SYSTIME_MGR_NAME, cTIMER_STATUS_UPDATE, &msg, sizeof(TimerMsg))` is called

---

### Requirement: TIMER_STATUS_MSG Query Handling

SystemTimeManager MUST register as a subscriber for `TIMER_STATUS_MSG` events so external components can query the current time quality synchronously.

#### Scenario: Subscriber registered for TIMER_STATUS_MSG

- **WHEN** `initialize()` is called
- **THEN** `m_tmrsubscriber->subscribe(TIMER_STATUS_MSG, SysTimeMgr::getTimeStatus)` is called
- **AND** the callback `getTimeStatus` is registered for inbound `TIMER_STATUS_MSG` events on the IARM bus

#### Scenario: Query response populates TimerMsg with current state

- **WHEN** `SysTimeMgr::getTimeStatus(void* args)` is invoked by the IARM event handler
- **THEN** the `TimerMsg* pMsg` is updated with:
  - `quality`: the current `m_timequality` enum value
  - `message`: the matching quality string
  - `timerSrc`: the current `m_timersrc`
  - `currentTime`: the current `CLOCK_REALTIME` value as a string

---

### Requirement: POWER_CHANGE_MSG Subscription

SystemTimeManager MUST subscribe to power change messages so that deep sleep transitions are communicated from the power manager.

#### Scenario: Subscriber registered for POWER_CHANGE_MSG

- **WHEN** `initialize()` is called
- **THEN** `m_subscriber->subscribe(POWER_CHANGE_MSG, SysTimeMgr::powerhandler)` is called
- **AND** the callback `powerhandler` is registered to receive power mode change notifications

---

### Requirement: IARM Bus Name

SystemTimeManager MUST register on the IARM bus using the component name `IARM_BUS_SYSTIME_MGR_NAME` for both publishing and subscribing.

#### Scenario: Publisher and subscribers use the correct bus name

- **WHEN** `createPublish("iarm", IARM_BUS_SYSTIME_MGR_NAME)` is called
- **AND** `createSubscriber("iarm", IARM_BUS_SYSTIME_MGR_NAME, ...)` is called
- **THEN** all IARM operations use `IARM_BUS_SYSTIME_MGR_NAME` as the component/bus identifier
