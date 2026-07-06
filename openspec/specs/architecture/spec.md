## Purpose

Describes the three-package structure of SystemTimeManager — what each package produces, what it depends on, and which package to modify for a given type of change.

---

## Packages

SystemTimeManager is split into three independently built packages. Each has its own `configure.ac` and is built as a separate bbclass in the meta layer.

### `interface` → header-only contract package

- **Produces**: header files installed to `/usr/include/systimerifc/`
- **Depends on**: nothing
- **Contains**: `ITimeSrc`, `ITimeSync`, `IPublish`, `ISubscribe`, `ITimerMsg`, `IRdkLog`

These headers define the vocabulary the whole system speaks. They contain no platform code, no library dependencies, and no implementation — only pure abstract classes and shared data types.

**Change this package only when the contract itself needs to change** — e.g., adding a new method all time sources must implement.

---

### `systimerfactory` → platform adapter library

- **Produces**: `libsystimerfactory.so`
- **Depends on**: `interface` headers + IARM + WPEFramework + chronyctl + optionally TEE/DTT
- **Contains**: all concrete implementations of the interfaces + factory functions

This is where all platform coupling lives. It knows about NTP, IARM, DRM, DTT, TEE, and power manager variants. Platform feature gates (`DTT_ENABLED`, `TEE_ENABLED`, `PWRMGRPLUGIN_ENABLED`) are resolved here at compile time.

The factory functions (`createTimeSrc`, `createTimeSync`, `createPublish`, `createSubscriber`) are the only entry points. They take a type string from the config file and return an abstract pointer. Callers never see the concrete type.

**Change this package when**: adding a new time source, adding a new platform IPC backend, or handling a new platform variant.

---

### `systimemgr` → core daemon

- **Produces**: `libsysTimeMgr.so` + `sysTimeMgr` binary
- **Depends on**: `interface` headers + `-lsystimerfactory`
- **Contains**: the state machine, time quality logic, all runtime decision-making

This package has **zero direct platform dependencies**. It never includes IARM, TEE, or WPEFramework headers. It only holds `ITimeSrc*`, `ITimeSync*`, `IPublish*`, and `ISubscribe*` pointers returned by the factory, and operates on them through the abstract interface.

**Change this package when**: changing state machine behavior, time quality transitions, or scheduling logic.

---

## Dependency Direction

```
systimemgr  ──▶  interface  (knows only the contract)
                    ▲
systimerfactory ───┘         (implements the contract)
       │
       ▼
  IARM, TEE, WPEFramework, DRM, DTT, chronyctl  (platform world)
```

The arrow never reverses. `systimemgr` does not reach into `systimerfactory` internals. `interface` knows nothing about either.

---

## Adding a New Time Source

1. Create `<name>timesrc.h/.cpp` in `systimerfactory/` implementing `ITimeSrc`
2. Add a case for it in `createTimeSrc()` in `timerfactory.cpp`
3. Add the new source entry to `/etc/systimemgr.conf` on the target
4. If it needs a compile gate, add the flag in `configure.ac` and guard the `.cpp` in `Makefile.am`

No changes needed in `systimemgr/` core or `interface/`.

---

## Adding a New Time Sync Backend

1. Create `<name>timesync.h/.cpp` in `systimerfactory/` implementing `ITimeSync`
2. Add a case for it in `createTimeSync()` in `timerfactory.cpp`
3. Reference it in the config file under `timesync`

No changes needed in `systimemgr/` core or `interface/`.
