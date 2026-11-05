# blink

Toggle the on-board LED every 500 ms with `setInterval` and log each toggle.

- `main.ts` — the source.
- `main.js` — ES5 compiled from it with `tsc` (committed, built into the firmware):
  `cd js && npm install && npm run examples`. CI fails if it is stale.
- `tsconfig.json` — ES5 target, typings from `js/packages/{console,timers,gpio,board}`.
- `example.json` — boards and engines CI builds an image for.

## Run it

With the JSBedded firmware on a Pico or Pico 2 (`jsbedded-pico-mquickjs.uf2`), copy
`main.js` to the `JSBEDDED` USB drive, eject it and reset the board (unplug and plug
in). See [your first script](../../docs/guide/first-script.md).

Or download `jsbedded-pico-mquickjs-blink.uf2` (or `jsbedded-pico2-mquickjs-blink.uf2`)
from the [`latest` release](https://github.com/stan-kondrat/jsbedded/releases/tag/latest),
or build it:

```bash
make firmware BOARD=pico ENGINE=mquickjs SCRIPT=examples/blink/main.js
# -> build/pico-mquickjs-blink/jsbedded-pico-mquickjs-blink.uf2
```

Hold BOOTSEL, plug in the board, copy the `.uf2` to the `RPI-RP2` (Pico) or `RP2350` (Pico 2) drive. Open the
USB serial port (115200 is fine; USB ignores it):

```
JSBedded <version> | board=pico | engine=mquickjs | script=main.js (drive JSBEDDED)
LED on
LED off
LED on
…
```

The LED (GP25) toggles every 500 ms. On boards without a plain-GPIO LED
(Pico W, Pico 2 W, ESP32 dev boards) `board.LED` is undefined and the script
prints that instead.
