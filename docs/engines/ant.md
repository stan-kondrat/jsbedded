---
engine: ant
---

# Ant

Facts: `firmware/engines/ant/engine.json`. Status: see the [matrix](../matrix.md).

**Not buildable for JSBedded boards yet. It is a research track, not in CI.** Ant's
smallest configuration (the interpreter-only Wasm embed: Silver parser,
interpreter and GC, no JIT) allocates more than 5 MB per isolate before running any code.
The cause is 1 MB GC arena chunks, an 8 MB arena floor, and a 1 MB+ VM stack reserved up front. Its
embed profile also only compiles for WebAssembly. Flash (an estimated 1.2–2 MB) would fit
the 4–8 MB boards. RAM (264–520 KB) would not fit on any of them.

What would make it feasible, as upstream changes:

- a generic embed profile separate from `__wasm__`
- configurable arena chunk size and minimum
- a configurable or growable VM stack
- embed stubs moved into `src/`
- a single-threaded option

Details, with file and line references, are in
[`firmware/engines/ant/README.md`](../../firmware/engines/ant/README.md).
