# Diagram: SystemTimeManager Architecture — Thread Model and Message Flow

Shows the full thread model, initialization sequence, and how events flow between threads into the state machine. Threads in parentheses are only started when `m_chronyRfcEnabled` is `true` (RFC feature flag present at `/opt/secure/RFC/chrony/chronyd_enabled`).

**Related spec:** [specs/time-quality/spec.md](../specs/time-quality/spec.md)

---

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
