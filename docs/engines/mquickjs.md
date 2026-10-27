---
engine: mquickjs
---

# MQuickJS

Facts: `firmware/engines/mquickjs/engine.json`. Status: see the [matrix](../matrix.md).

MQuickJS (Micro QuickJS, by Fabrice Bellard and Charlie Gordon) runs a subset of
JavaScript close to ES5 in a "stricter mode": strict-mode semantics, no `with`,
no holes in arrays, globals must be declared. It is built for small devices. The
standard library sits in ROM, the engine allocates only from one fixed buffer,
and the interpreter doesn't use the C stack for JS calls. It is the first engine
for JSBedded v0.1.

```sh
make firmware BOARD=pico ENGINE=mquickjs
```

## In the firmware

- **Globals:** the upstream `mqjs` set, including `console.log`, `print`, `Math`,
  `JSON`, `Date`, typed arrays and `RegExp`. `load`, `setTimeout` and
  `clearTimeout` throw until the firmware has a file system and an event loop.
- **Errors:** uncaught exceptions print `Uncaught <Name>: <message>` and the stack
  (`at boot.js:1:7`).
- **Memory:** a static JS heap per board: 64 KB on RP2040, 128 KB on RP2350 and
  ESP32-S3, 96 KB on ESP32-C3. It holds every object and the JS stack.
- **Build:** the stdlib ROM tables are generated at configure time by a tool that
  is built with the host C compiler, so building needs `cc` (or `HOST_CC`) next to
  the cross toolchain.
- **Boards:** all six.

Details: [`firmware/engines/mquickjs/README.md`](../../firmware/engines/mquickjs/README.md).
