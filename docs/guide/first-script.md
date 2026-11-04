# Your first script

Blink the on-board LED. This is [`examples/blink/main.ts`](../../examples/blink/main.ts):

<<< @/../examples/blink/main.ts

- `board.LED` is the LED's GPIO number (25 on Pico and Pico 2), `undefined` where
  the LED isn't a plain GPIO.
- `gpio.mode` / `gpio.write` drive the pin; `setInterval` runs the callback every
  500 ms from the firmware's event loop.
- `console.log` writes to USB serial.

## Compile

MQuickJS runs ES5, so TypeScript is compiled with `tsc` (`target: ES5`), which also
type-checks it against the API typings (`@jsbedded/console`, `timers`, `gpio`, `board`):

```bash
cd js && npm install && npm run examples   # examples/blink/main.ts -> main.js
```

## Build it into the firmware

Until scripts can be uploaded (milestone 2), the script is built into the image:

```bash
make firmware BOARD=pico ENGINE=mquickjs SCRIPT=examples/blink/main.js
```

Or take `jsbedded-pico-mquickjs-blink.uf2` / `jsbedded-pico2-mquickjs-blink.uf2`
from the `latest` release. Copy it to the board in BOOTSEL mode, open its USB
serial port, and watch `LED on` / `LED off` every half second.

Errors are printed with their position, e.g. `Uncaught TypeError: … at main.js:9:9`;
the event loop keeps running the other timers.
