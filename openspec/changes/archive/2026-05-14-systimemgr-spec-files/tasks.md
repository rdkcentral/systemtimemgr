## 1. Spec files — initialization

- [x] 1.1 Create `openspec/changes/systimemgr-spec-files/specs/initialization/spec.md`
  - Single-instance enforcement (first start, second invocation rejected)
  - Log file generation
  - TimeSrc/TimeSync plugin loading from config
  - State machine enters `eSYSMGR_STATE_NTP_WAIT`

## 2. Spec files — bootup-flow

- [x] 2.1 Create `openspec/changes/systimemgr-spec-files/specs/bootup-flow/spec.md`
  - Last known good time recovery via `setInitialTime()`
  - `/tmp/systimeset` sentinel file creation
  - Initial `Poor` quality broadcast
  - `TIMER_STATUS_MSG` returns last known good time

## 3. Spec files — time-quality

- [x] 3.1 Create `openspec/changes/systimemgr-spec-files/specs/time-quality/spec.md`
  - Initial quality is Poor
  - NTP timeout → `NTP_FAIL` state → Poor broadcast
  - NTP acquired in `NTP_WAIT` or `NTP_FAIL` → Good broadcast
  - DTT acquired after NTP failure → Good broadcast
  - DRM secure time with NTP → Secure broadcast
  - Query response returns correct quality label

## 4. Spec files — secure-time

- [x] 4.1 Create `openspec/changes/systimemgr-spec-files/specs/secure-time/spec.md`
  - inotify detection of `/tmp/systimemgr/drm` creation/modification
  - Pre-existing DRM file triggers event on startup
  - Secure time from `NTP_WAIT` state
  - Secure time from `NTP_FAIL` state
  - Secure time with NTP acquired → Secure broadcast

## 5. Spec files — clock-file

- [x] 5.1 Create `openspec/changes/systimemgr-spec-files/specs/clock-file/spec.md`
  - Clock file updated on timer expiry in `RUNNING`/`NTP_ACQUIRED` state
  - Clock file updated after DTT acquisition
  - Reference time used when within 600s tolerance
  - Zero time from secondary source is ignored

## 6. Spec files — power-event-handling

- [x] 6.1 Create `openspec/changes/systimemgr-spec-files/specs/power-event-handling/spec.md`
  - PowerController subscription (success path, retry thread path)
  - Fallback to IARM PowerManager subscription
  - Deep sleep entry (DEEP_SLEEP_ON)
  - Deep sleep exit (DEEP_SLEEP_OFF): quality downgrade, re-broadcast, NTP/chronyd restart
  - Destructor releases PowerController resources

## 7. Spec files — iarm-communication

- [x] 7.1 Create `openspec/changes/systimemgr-spec-files/specs/iarm-communication/spec.md`
  - IARM bus init (new vs already-connected paths)
  - Time quality broadcast (`TimerMsg` structure: quality, message, timerSrc, currentTime)
  - `TIMER_STATUS_MSG` subscription and query response
  - `POWER_CHANGE_MSG` subscription
  - Bus name is `IARM_BUS_SYSTIME_MGR_NAME`

## 8. Design document

- [x] 8.1 Create `openspec/changes/systimemgr-spec-files/design.md`
  - Seven capability groupings and rationale
  - Observable behavior vs. implementation detail boundary
  - Both PowerController and IARM PowerManager paths documented
  - Source derivation notes

## 9. Sync specs to main specs directory

- [x] 9.1 Copy or promote all 7 spec files from `openspec/changes/systimemgr-spec-files/specs/` to `openspec/specs/`
  - `openspec/specs/initialization/spec.md`
  - `openspec/specs/bootup-flow/spec.md`
  - `openspec/specs/time-quality/spec.md`
  - `openspec/specs/secure-time/spec.md`
  - `openspec/specs/clock-file/spec.md`
  - `openspec/specs/power-event-handling/spec.md`
  - `openspec/specs/iarm-communication/spec.md`
