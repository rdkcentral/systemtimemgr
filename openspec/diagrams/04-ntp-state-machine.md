# Diagram: NTP Time Quality State Machine

Shows all states, the events that trigger transitions, and the IARM broadcast emitted at each transition. Each state label includes the resulting time quality level. Self-loops (TimerExpiry) in steady states update the clock file but do not change quality.

**Related spec:** [specs/time-quality/spec.md](../specs/time-quality/spec.md)

---

```mermaid
stateDiagram-v2
    direction LR

    state "Initializing" as Initializing
    state "NTPWait\n(Quality: Poor)" as NTPWait
    state "NTPFailed\n(Quality: Poor)" as NTPFailed
    state "NTPAcquired\n(Quality: Good  |  source: NTP)" as NTPAcquired
    state "DTTAcquired\n(Quality: Good  |  source: DTT)" as DTTAcquired
    state "SecureTimeAcquired\n(Quality: Good  |  source: Secure)" as SecureTimeAcquired
    state "Running\n(Quality: Secure  |  source: NTP + DRM)" as Running

    [*] --> Initializing
    Initializing --> NTPWait : initialize() complete

    NTPWait --> NTPFailed            : TimerExpiry / broadcast Poor
    NTPWait --> NTPAcquired          : NTPAvailable / broadcast Good
    NTPWait --> SecureTimeAcquired   : SecureTimeAvailable

    NTPFailed --> NTPAcquired        : NTPAvailable / broadcast Good
    NTPFailed --> DTTAcquired        : DTTAvailable / broadcast Good
    NTPFailed --> SecureTimeAcquired : SecureTimeAvailable
    NTPFailed --> NTPFailed          : TimerExpiry / update clock file

    NTPAcquired --> Running          : SecureTimeAvailable / broadcast Secure
    NTPAcquired --> NTPAcquired      : TimerExpiry / update clock file

    DTTAcquired --> NTPAcquired      : NTPAvailable / broadcast Good
    DTTAcquired --> SecureTimeAcquired : SecureTimeAvailable
    DTTAcquired --> DTTAcquired      : TimerExpiry / updateClockRealTime

    SecureTimeAcquired --> Running              : NTPAvailable / broadcast Secure
    SecureTimeAcquired --> SecureTimeAcquired   : TimerExpiry / update clock file

    Running --> Running : TimerExpiry / update clock file

    note right of NTPWait
        Timer interval: 10 min (600,000 ms).
        TimerExpiry fires in every state
        except Initializing.
    end note

    note right of Running
        Terminal steady state.
        Quality = Secure.
        No further transitions.
    end note
```

---

## Quality Levels Summary

| State | Quality | IARM broadcast |
|---|---|---|
| Initializing | Poor | Poor (from `setInitialTime()`) |
| NTPWait | Poor | — (waiting) |
| NTPFailed | Poor | Poor (on `TimerExpiry` from NTPWait) |
| NTPAcquired | Good | Good |
| DTTAcquired | Good | Good |
| SecureTimeAcquired | Good | — (quality set to Good; Secure broadcast deferred) |
| Running | Secure | Secure |

## Event Glossary

| Event | Trigger source |
|---|---|
| `NTPAvailable` | `pathThr` inotify on `/tmp/systimemgr/ntp` (touched by `ntpSyncMonitorThrd` or NtpTimeSrc plugin) |
| `DTTAvailable` | `pathThr` inotify on `/tmp/systimemgr/dtt` |
| `SecureTimeAvailable` | `pathThr` inotify on `/tmp/systimemgr/drm` |
| `TimerExpiry` | `timerThr` — fires every 10 minutes |

> **Wiki vs. code discrepancy**: The Confluence state diagram shows a `Running → SecureTimeAcquired` back-transition. That transition does **not** exist in the code's `stateMachine` map and is a documentation artifact. The diagram above reflects the actual implementation.
