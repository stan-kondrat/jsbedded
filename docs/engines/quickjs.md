---
engine: quickjs
---

# QuickJS

Facts: `firmware/engines/quickjs/engine.json`. Status: see the [matrix](../matrix.md).

QuickJS (Fabrice Bellard) runs almost all of ES2025: classes, modules,
async/await, generators, Proxy, BigInt, typed arrays, RegExp with Unicode
properties. It is the full-language engine for the boards with the most RAM.
MQuickJS is the small engine for the rest. Pinned at release 2026-06-04.

```sh
make firmware BOARD=pico2 ENGINE=quickjs
```

## In the firmware

- **Globals:** the standard QuickJS set (`JS_NewContext()`) plus `console.log`.
  Atomics are compiled out (no threads). This release has no Intl or Temporal.
  No `quickjs-libc`, so no `std`/`os` modules, `print`, `setTimeout` or file I/O.
- **Errors:** uncaught exceptions print `Uncaught <Name>: <message>` and the
  stack (`at <eval> (boot.js:1:7)`). Promise jobs queued by the script run after
  it.
- **Memory:** the JS heap is `malloc()`-backed with a per-board limit: 384 KB on
  pico2, 320 KB on pico2_w, 256 KB on ESP32-S3. The C stack for the parser and
  interpreter is capped at 16 KB on RP2350 and at the main task stack minus 3 KB
  on ESP32 (which needs `CONFIG_ESP_MAIN_TASK_STACK_SIZE=16384`). An empty
  context is estimated at 150–200 KB.
- **Build:** the sources are copied to the build directory and one local patch is
  applied there (Atomics and `malloc_usable_size()` opt-outs), so building needs
  `git`. Compiled with `-Os`.
- **Boards:** pico2, pico2_w, esp32s3_devkitc. Not RP2040 (pico, pico_w) or
  ESP32-C3: once the SDK and networking take their share, an empty context would
  leave almost no room for scripts.

Details: [`firmware/engines/quickjs/README.md`](../../firmware/engines/quickjs/README.md).
