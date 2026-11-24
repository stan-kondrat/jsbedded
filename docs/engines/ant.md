---
engine: ant
---

# Ant

Facts: `firmware/engines/ant/engine.json`. Status: see the [matrix](../matrix.md).

**Builds for `pico` (RP2040); the blink example runs on hardware.** Upstream Ant
(`v16.5.c41757b4.0`) only has an interpreter-only embed profile for WebAssembly, sized
for megabytes of RAM. JSBedded applies `third_party/patches/ant/`
at build time: a generic embed profile, configurable heap, VM stack and table sizes,
single-threaded locks, and an embed source list that links without the OS modules.
Each patch is shaped as an upstream PR.

The blink image uses 1767 KB of the Pico's flash, so Ant images keep a 256 KB script
drive (1 MB on other engines). Building needs Clang ≥ 19 besides the pico-sdk toolchain.

Scripts get Ant's core (Symbol, iterators, generators, Math, BigInt, RegExp,
Map/Set, JSON, Promise, async/await) plus JSBedded's APIs. Not included: Date, typed
arrays, `import`, Intl, TypeScript.

Details, with file and line references, are in
[`firmware/engines/ant/README.md`](../../firmware/engines/ant/README.md).
