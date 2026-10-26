# firmware/api

Native side of the JSBedded hardware APIs. One folder per module.

The JS-facing contract is defined in the API typings in `js/` (`.d.ts` + TSDoc).
`make codegen` generates the binding headers here; module code implements them
against `engines/adapter.h` and `hal/hal.h` only.

| Module | Requires |
|---|---|
| `console/` | — |
| `timers/` | — |
| `gpio/` | `peripheral:gpio` |
| `uart/` | `peripheral:uart` |
| `i2c/` | `peripheral:i2c` |
| `spi/` | `peripheral:spi` |
| `net/` | `peripheral:wifi`, `engine:promises` (later) |
| `fs/` | `storage:fs` (later) |
