# Diagram: Network Event-Driven Chrony Sync

Shows the Thunder subscription retry loop (`nwEventSubscribeThrd`), how `fully_connected` events are filtered and handed off to `nwEventProcessThrd` via a condition variable, the 6 decision paths in `processInternetOnline()`, and the deep sleep wake path in `deepsleepoff()`.

**Related spec:** [specs/chrony-ntp-sync/spec.md](../specs/chrony-ntp-sync/spec.md)

---

```mermaid
flowchart TD
    subgraph SUB["nwEventSubscribeThrd — subscribeInternetStatusEvent()"]
        S1["Create Thunder SmartLinkType client\nfor org.rdk.NetworkManager"] --> S2["Subscribe&lt;JsonObject&gt;\nonInternetStatusChange"]
        S2 --> S3{Core::ERROR_NONE?}
        S3 -- "Yes" --> S4["m_networkeventsubscribed = true\nthread returns"]
        S3 -- "No" --> S5["delete client\nsleep up to 1 s\n(interruptible by shutdown cv)"]
        S5 --> S1
    end

    subgraph CB["Thunder I/O Thread — handle_internetStatusChange()"]
        T1["normalize status to lowercase"] --> T2{status ==\n'fully_connected'\nAND changed?}
        T2 -- "No" --> T3["update lastStatus only\nno signal"]
        T2 -- "Yes" --> T4["internetUpPending = true\ncv.notify_one()"]
    end

    subgraph PROC["nwEventProcessThrd — processInternetOnline()"]
        P0["wait on cv:\ninternetUpPending\nor stopProcessing"] --> P1["clear internetUpPending"]
        P1 --> P2{/tmp/clock-event\nexists?}

        P2 -- "No — first sync\npending this boot" --> PA{"systemctl\nis-active chronyd?"}
        PA -- "not active" --> A1["Case A: no-op\nInternet up, chronyd will\niburst on startup"]
        PA -- "active" --> PB{"chronyctl_get_source_count()\n> 0?"}
        PB -- "Yes" --> B1["Case B: no-op\niburst / polling\nalready in progress"]
        PB -- "No\n(booted offline)" --> C1["Case C:\nchronyctl_online(NULL,NULL)\nmark sources reachable\n→ iburst fires immediately"]

        P2 -- "Yes — post first\nsync already done" --> PD{"chronyctl_has_\nselectable_source()?"}
        PD -- "No selectable\nsource" --> D1["chronyctl_burst(NULL,NULL,4,6)\ngather fresh samples"]
        D1 --> D2["chronyctl_waitsync(20, 1s)\nwait up to 20 s for selection"]
        D2 --> D3{waitsync\nsucceeded?}
        D3 -- "Yes" --> D4["chronyctl_makestep()\nstep clock to reference"]
        D3 -- "No — timeout\nno valid reference" --> D5["skip makestep\n(no synced source)"]

        PD -- "Selectable source\npresent" --> PE["chronyctl_get_system_time_offset()"]
        PE --> PF{"fabs(offset) >\n1.0 s?"}
        PF -- "Yes — large offset" --> E1["chronyctl_makestep()\nimmediate step correction"]
        PF -- "No — small offset" --> F1["no-op\nnatural slew sufficient"]

        A1 & B1 & C1 & D4 & D5 & E1 & F1 --> P0
    end

    subgraph SLEEP["deepsleepoff() — Chrony wake path"]
        DS0{"systemd-timesyncd\nactive?"} -- "Yes" --> DS1["systemctl reset-failed timesyncd\nsystemctl restart timesyncd"]
        DS0 -- "No" --> DS2{"chronyd\nactive?"}
        DS2 -- "No" --> DS3["skip — neither service running"]
        DS2 -- "Yes" --> DS4["chronyctl_burst(NULL,NULL,4,6)"]
        DS4 --> DS5["chronyctl_waitsync(20,1s)"]
        DS5 --> DS6["chronyctl_makestep()\nbest-effort — called even if\nwaitsync timed out"]
    end

    T4 -.->|"signals cv"| P0
    SUB -.->|"subscription active\nbefore events can arrive"| CB
```

> **Key difference — makestep guard**: In `processInternetOnline()` (Scenario D) `makestep` is **skipped** when `waitsync` times out (no valid reference). In `deepsleepoff()` `makestep` is **always attempted** after `waitsync` (best-effort on wake).
