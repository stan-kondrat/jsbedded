---
engine: jerryscript
---

# JerryScript

Facts: `firmware/engines/jerryscript/engine.json`. Status: see the [matrix](../matrix.md).

JerryScript (Samsung, now the JS Foundation, Apache-2.0) is a small engine built
for microcontrollers. Its `es.next` profile covers ES2015 and much of the later
language: classes, arrow functions, let/const, destructuring, generators,
async/await, Promise, Proxy, Reflect, Symbol, BigInt, typed arrays, Map/Set,
WeakRef. Kaluma and mcujs use it on the Pico, so it gives a direct comparison.
Pinned at release `v3.0.0`.

```sh
make firmware BOARD=pico ENGINE=jerryscript
```

## In the firmware

- **Globals:** the `es.next` set plus `console.log`. No module system
  (`import`/`export`) until there is a filesystem. No `print`, `setTimeout` or
  file I/O.
- **Errors:** uncaught exceptions print `Uncaught <Name>: <message>` and the
  stack (`at boot.js:1:7`); syntax errors give the position in the message.
  Promise jobs queued by the script run after it.
- **Memory:** a static JS heap per board: 128 KB on pico, 96 KB on pico_w,
  256 KB on pico2, 224 KB on pico2_w, 192 KB on ESP32-S3, 128 KB on ESP32-C3.
  Built-ins are created lazily, so an empty context takes only a few KB.
  **Running out of JS heap halts the firmware** (a JerryScript fatal error, not an
  exception). The C stack for scripts is capped at 8 KB on RP2040, 16 KB on
  RP2350 and 13 KB on ESP32 (main task stack 16 KB).
- **Build:** jerry-core is compiled straight from the submodule with `-Os`. No
  patches, no Python and no host tools are needed.
- **Boards:** all six (pico, pico_w, pico2, pico2_w, esp32s3_devkitc,
  esp32c3_devkitm).

Details: [`firmware/engines/jerryscript/README.md`](../../firmware/engines/jerryscript/README.md).
