# API reference

The JS API is specified by the typings in `js/packages/<module>/index.d.ts`
(`.d.ts` + TSDoc), one npm package per module under `@jsbedded/`. The reference
pages under `reference/` are generated from them (TypeDoc +
`typedoc-plugin-markdown`) by `make docs-gen`. Do not edit them by hand.

All modules are globals today (no `import`). They need an engine that implements
the host-function contract: only `mquickjs` so far.

| Module | Global | Package | Status |
|---|---|---|---|
| console | `console.log` | `@jsbedded/console` | available (mquickjs) |
| timers | `setTimeout`, `setInterval`, `clearTimeout`, `clearInterval` | `@jsbedded/timers` | available (mquickjs) |
| gpio | `gpio.mode`, `gpio.write`, `gpio.read` | `@jsbedded/gpio` | available (mquickjs) |
| board | `board.name`, `board.LED` | `@jsbedded/board` | available (mquickjs) |
| uart | | | planned |
| i2c | | | planned |
| spi | | | planned |
| net | | | later |
| fs | | | later |

## At a glance

```ts
console.log("hello", 42, { a: 1 });     // hello 42 { a: 1 }

const id = setInterval(() => { /* … */ }, 500);
clearInterval(id);
setTimeout(() => console.log("once"), 1000);

gpio.mode(25, "output");               // "input" | "output"
gpio.write(25, 1);                     // 0 | 1 (or true / false)
gpio.read(25);                         // 0 | 1

board.name;                            // "pico"
board.LED;                             // 25 on pico / pico2, undefined on W and ESP32 boards
```

Limits: 16 timers at once; timer callbacks get no extra arguments; errors in
callbacks are printed (`Uncaught …`) and the event loop continues. Bad arguments
throw `TypeError` (wrong type) or `RangeError` (pin that doesn't exist, mode other
than `"input"`/`"output"`, level other than 0/1).
