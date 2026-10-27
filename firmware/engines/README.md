# firmware/engines

One adapter per JavaScript engine; exactly one is linked into an image
(`make firmware ENGINE=<engine>`). Each folder contains:

- `engine.json` — declared facts: upstream, `submodules` CI must check out,
  `unsupported_boards` (left out of CI and releases), language level, minimum flash/RAM
- `engine.cmake` — sources and flags for the firmware build (see the variables
  listed in `../core/core.cmake`). An engine joins the CI matrix as soon as this file exists.
- adapter sources implementing [`include/jsb/engine.h`](include/jsb/engine.h)
- `sdkconfig.defaults` (optional) — ESP-IDF settings the engine needs, applied after
  `hal/esp-idf/sdkconfig.defaults`

The contract today: create a runtime with `console.log` installed, evaluate a
script, report uncaught errors on the console, destroy. Still to add: host
functions for the hardware APIs, value conversion, pending jobs,
interrupt/limit hooks, GC hooks and capability reporting.

`none/` is the placeholder with no JavaScript; it is the default `ENGINE` and
keeps a baseline image for every board.

Engines live in `third_party/` as submodules; local fixes, if any, are patch
files in `third_party/patches/<engine>/`, applied to a copy under `build/`.

Declared minimums stay `null` until measured — see `bench/`.
