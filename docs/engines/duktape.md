---
engine: duktape
---

# Duktape

Facts: `firmware/engines/duktape/engine.json`. Status: see the [matrix](../matrix.md).

Duktape (Sami Vaarala, MIT) is a small engine built for embedding. It runs
ES5.1 plus parts of ES2015+ (typed arrays, Proxy, Reflect, Symbol, the `**`
operator), but has no arrow functions, classes or Promise. It has a compact C
API and reference counting with a mark-and-sweep backup, so memory is freed as
soon as values die. Release 2.7.0.

```sh
make firmware BOARD=pico ENGINE=duktape
```

## In the firmware

- **Globals:** the standard Duktape set (`Duktape`, typed arrays, `Proxy`,
  `Reflect`, `Symbol`, `CBOR`, `TextEncoder`/`TextDecoder`, `performance`) plus
  `console.log`. Built-in functions are lightweight function values, which saves
  RAM; they have no own properties.
- **Errors:** uncaught exceptions print `Uncaught <Name>: <message>` and
  `at <file>:<line>`. There are no stack traces, which are turned off to save
  RAM.
- **Memory:** the JS heap is `malloc()`-backed with a per-board limit: 96 KB on
  pico, 80 KB on pico_w, 256 KB on pico2, 224 KB on pico2_w, 192 KB on ESP32-S3
  and 128 KB on ESP32-C3. Low-memory and shallow-C-stack options come from
  upstream's examples. An empty heap is estimated at 25–40 KB. On ESP32 the main
  task stack is raised to 16 KB.
- **Build:** compiled from the official release tarball, vendored with its
  checksum, because the git tree's generator needs Python 2. Building needs only
  CMake. Compiled with `-Os`.
- **Boards:** all six.

Details: [`firmware/engines/duktape/README.md`](../../firmware/engines/duktape/README.md).
