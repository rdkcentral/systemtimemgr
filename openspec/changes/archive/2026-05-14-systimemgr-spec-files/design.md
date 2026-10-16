## Context

The `openspec/specs/` directory was empty. All behavioral definitions for the systemtimemgr component existed only in Gherkin feature files under `test/functional-tests/features/` and implicitly in the source code state machine. There were no canonical, structured requirement documents in the OpenSpec format.

## Goals / Non-Goals

**Goals:**
- Capture all significant capabilities of systemtimemgr as OpenSpec specs in `openspec/changes/systimemgr-spec-files/specs/`
- Cover the full behavioral surface: initialization, bootup, time quality transitions, secure time, clock file, power events, and IARM communication
- Use WHEN/THEN/AND requirement/scenario format for testability

**Non-Goals:**
- Modifying any production C++ source code
- Creating new unit or functional tests (that is a separate change)
- Speccing internal implementation details (thread management, mutex usage) that are not observable externally

## Decisions

### Decision 1: Seven capability groupings

The behavior was split into seven spec files matching seven distinct observable concerns:

| Capability | Core behavior |
|---|---|
| `initialization` | Single instance, log file, plugin loading, state machine entry |
| `bootup-flow` | Last known time recovery, sentinel file, initial Poor broadcast |
| `time-quality` | Poor→Good (NTP/DTT) and Good→Secure (DRM) transitions |
| `secure-time` | inotify DRM path monitor, secure time event flow |
| `clock-file` | `/opt/secure/clock.txt` persistence via TimeSync on timer expiry |
| `power-event-handling` | Deep sleep on/off, NTP restart, quality re-broadcast on wake |
| `iarm-communication` | IARM bus init, publish broadcast, TIMER_STATUS_MSG, POWER_CHANGE_MSG |

This grouping matches the existing feature file structure and the natural seams in the source code.

### Decision 2: Specs describe observable behavior, not implementation

Scenarios describe what external observers can verify (IARM events, file presence, quality strings, state transitions visible via logs) rather than internal mechanisms (mutex acquisition order, thread naming). This keeps specs stable when implementation details change.

### Decision 3: Both PowerController and IARM PowerManager paths are specced

The codebase supports two subscriber implementations (`IpowerControllerSubscriber` and `IarmPowerSubscriber`). Both paths are specced under `power-event-handling` since either may be active depending on build configuration. The spec uses behavior-level language that applies to both.

### Decision 4: Source for scenarios

Scenarios were derived from three primary sources:
1. `test/functional-tests/features/*.feature` — existing Gherkin scenarios
2. `systimemgr.cpp` — state machine transitions (`ntpAquired`, `ntpFailed`, `dttAquired`, `secureTimeAcquired`, `updateSecureTime`, `deepsleepoff`, `publishStatus`)
3. `systimerfactory/*.cpp` — IARM bus and PowerController subscriber logic
