# Diagram: Chrony RFC Mode — Thread Startup and NTP Sync Monitor

Shows the RFC gate decision at construction time, which threads are started when Chrony mode is active, and the one-shot behavior of `ntpSyncMonitorThrd` once the kernel clock becomes NTP-synchronized.

**Related spec:** [specs/chrony-ntp-sync/spec.md](../specs/chrony-ntp-sync/spec.md)

---

```mermaid
sequenceDiagram
    participant main
    participant SysTimeMgr
    participant PathThread as pathThr\n(inotify /tmp/systimemgr/)
    participant MsgThread as processThr\n(state machine queue)
    participant NtpMon as ntpSyncMonitorThrd
    participant NwSub as nwEventSubscribeThrd
    participant NwProc as nwEventProcessThrd

    main->>SysTimeMgr: SysTimeMgr() constructor
    Note over SysTimeMgr: access('/opt/secure/RFC/chrony/chronyd_enabled')\nm_chronyRfcEnabled = true / false

    main->>SysTimeMgr: initialize()
    alt RFC enabled
        SysTimeMgr->>SysTimeMgr: chronyctl_init()
    else RFC disabled
        Note over SysTimeMgr: No chronyctl calls.\ntimesyncd manages NTP.
    end
    Note over SysTimeMgr: Loads plugins, IARM pub/sub,\nsetInitialTime(), builds stateMachine,\nm_state = NTP_WAIT

    main->>SysTimeMgr: run()
    SysTimeMgr->>PathThread: Start (always)
    SysTimeMgr->>MsgThread: Start (always)
    alt RFC enabled
        SysTimeMgr->>NtpMon: Start ntpSyncMonitorThrd
        SysTimeMgr->>NwSub: Start nwEventSubscribeThrd
        SysTimeMgr->>NwProc: Start nwEventProcessThrd
        Note over NwProc: Must start before NwSub\nso queue is ready before events arrive
    end

    loop adjtimex() poll — every 1 s
        NtpMon->>NtpMon: adjtimex(&tx)
        alt TIME_ERROR or STA_UNSYNC set
            NtpMon->>NtpMon: sleep 1 s, retry
        else adjtimex() syscall failed (< 0)
            NtpMon->>NtpMon: log error, sleep 1 s, retry
        end
    end

    Note over NtpMon: Kernel NTP synchronised —\nSTA_UNSYNC clear, not TIME_ERROR

    NtpMon->>NtpMon: open(/tmp/systimemgr/ntp, O_CREAT|O_NOFOLLOW|O_CLOEXEC)\nfutimens() → updates timestamps
    NtpMon-->>PathThread: inotify IN_ATTRIB fires on /tmp/systimemgr/ntp
    PathThread->>MsgThread: sendMessage(NTP_AVAILABLE)
    MsgThread->>SysTimeMgr: runStateMachine(NTP_AVAILABLE)

    NtpMon->>NtpMon: create /tmp/clock-event\n(O_CREAT|O_NOFOLLOW|O_CLOEXEC)\n→ "first boot NTP sync" sentinel

    NtpMon->>NtpMon: write "Synchronized\n" to /tmp/ntp_status\n(O_TRUNC|O_NOFOLLOW|O_CLOEXEC)

    NtpMon->>NtpMon: chronyctl_get_offset(&offset)\nT2 marker: SYST_INFO_NTP_DELTA_split

    Note over NtpMon: Thread exits — one-shot,\ndoes not loop after sync confirmed
```
