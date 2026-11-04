# firmware/api

Native side of the JSBedded hardware APIs. One folder per module.

The JS-facing contract is defined in the API typings in `js/` (`.d.ts` + TSDoc).
`make codegen` will generate the binding headers here; today module code is written
by hand against `jsb/engine.h` (`engines/include/`) and `jsb/hal.h` (`hal/include/`)
only. Each module has an install function, declared in
[`include/jsb/api.h`](include/jsb/api.h), that defines its global with
`jsb_engine_define()`; `core/src/main.c` calls them before the script runs.
Modules are listed in `core/core.cmake`.

| Module | Requires |
|---|---|
| `console/` | — |
| `timers/` | — (implemented in `core/src/timers.c`) |
| `gpio/` | `peripheral:gpio` |
| `uart/` | `peripheral:uart` |
| `i2c/` | `peripheral:i2c` |
| `spi/` | `peripheral:spi` |
| `net/` | `peripheral:wifi`, `engine:promises` (later) |
| `fs/` | `storage:fs` (later) |
