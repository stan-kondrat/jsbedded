# firmware/engines

One adapter per JavaScript engine. Each folder contains:

- `engine.json` — declared facts: version, language level, minimum flash/RAM
- `engine.mk` — sources and flags for the firmware build (later)
- adapter sources implementing `adapter.h` (later)

The adapter contract (`adapter.h`, not written yet) covers: create/destroy a
context, evaluate a script, register host functions, convert values, run
pending jobs, interrupt/limit hooks, GC hooks, and report capabilities.

Engines live in `third_party/` as submodules; local fixes, if any, are patch files in `third_party/patches/<engine>/`.

Declared minimums stay `null` until measured — see `bench/`.
