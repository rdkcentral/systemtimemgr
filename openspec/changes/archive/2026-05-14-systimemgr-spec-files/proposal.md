## Why

The `openspec/specs/` directory for the systemtimemgr repo is completely empty. The component implements rich, well-defined behavior — initialization, time quality state transitions (Poor → Good → Secure), power-event handling on deep sleep, and IARM bus communication — but none of it is captured as structured, reusable specifications. This makes requirements invisible during code review, onboarding, and future changes.

## What Changes

- Seven capability spec files are added under `openspec/specs/`
- Each spec covers a distinct behavioral area derived from the existing functional test feature files (`test/functional-tests/features/`) and the source code state machine in `systimemgr.cpp`
- No production code is modified

## Capabilities

### New Capabilities

- `initialization`: Startup sequence, single-instance enforcement, log file generation, and TimeSrc/TimeSync plugin loading from config
- `bootup-flow`: Initial time set from last known good time, `/tmp/systimeset` sentinel file, and initial `Poor` quality broadcast
- `time-quality`: State machine transitions — Poor (initial/NTP fail) → Good (NTP acquired, DTT acquired) → Secure (DRM acquired) — and the IARM time quality broadcast at each transition
- `secure-time`: Detection of DRM secure time availability via inotify path monitor on `/tmp/systimemgr/drm`, publishing `eSYSMGR_EVENT_SECURE_TIME_AVAILABLE`, and writing DRM state
- `clock-file`: Persistence of last known good time to `/opt/secure/clock.txt` via the TimeSync layer on each timer expiry
- `power-event-handling`: Subscribing to IARM/PowerController power mode change events, handling DEEP_SLEEP_ON (no-op) and DEEP_SLEEP_OFF (reset state, restart NTP service, re-publish time quality)
- `iarm-communication`: IARM bus publish/subscribe architecture — broadcasting `TimerMsg` time quality events, servicing `TIMER_STATUS_MSG` queries with current quality and time, and handling `POWER_CHANGE_MSG` from the power manager

### Modified Capabilities

<!-- No existing specs to modify — all capabilities are new -->

## Impact

- `openspec/specs/` — 7 new spec files created (no production code changes)
- Source files referenced: `systimemgr.cpp`, `systimemgr.h`, `systimerfactory/iarmpublish.cpp`, `systimerfactory/iarmsubscribe.cpp`, `systimerfactory/iarmpowersubscriber.cpp`, `systimerfactory/ipowercontrollersubscriber.cpp`
