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

## Copy it to the board

On the Pico family, with the firmware [installed](./install-firmware.md)
(`jsbedded-pico-mquickjs.uf2` or `jsbedded-pico2-mquickjs.uf2`), the board shows up
as a USB drive named `JSBEDDED` next to its USB serial port:

1. Copy `examples/blink/main.js` to the `JSBEDDED` drive.
2. Eject the drive, so the PC finishes writing, then reset the board: unplug and
   plug it in again (without BOOTSEL), or press RUN where the board has one.
3. Open the USB serial port and watch `LED on` / `LED off` every half second.

At boot the firmware runs the first of `main.js`, `index.js`, `main.mjs`,
`index.mjs` it finds on the drive. The script is read once, at boot: after
editing it, reset again. If a script misbehaves, [dev mode](./recovery.md) starts
the board without running it.

## Or build it into the firmware

Boards without the drive (ESP32 for now), or an image that should always run
one script, take it built in:

```bash
make firmware BOARD=pico ENGINE=mquickjs SCRIPT=examples/blink/main.js
```

`jsbedded-pico-mquickjs-blink.uf2` / `jsbedded-pico2-mquickjs-blink.uf2` in the
`latest` release are built this way. A script on the drive still takes precedence.

Errors are printed with their position, e.g. `Uncaught TypeError: … at main.js:9:9`;
the event loop keeps running the other timers.
