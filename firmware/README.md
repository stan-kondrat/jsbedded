# firmware

C code that runs on the microcontroller. Builds on its own:

```bash
make -C firmware BOARD=pico2                  # -> build/pico2-none/jsbedded-pico2-none.uf2
make -C firmware BOARD=pico ENGINE=mquickjs SCRIPT=../examples/blink/main.js
                                              # -> build/pico-mquickjs-blink/jsbedded-pico-mquickjs-blink.uf2
. third_party/esp-idf/export.sh               # ESP32 boards need the ESP-IDF environment
make -C firmware BOARD=esp32s3_devkitc        # -> build/esp32s3_devkitc-none/jsbedded-esp32s3_devkitc-none.bin
make -C firmware boards engines               # list boards and engines
```

At boot the image prints one line (`JSBedded <version> | board=… | engine=… | script=…`),
starts the engine, installs the host APIs (`console`, timers, `gpio`, `board`),
runs the boot script, then runs the event loop: due timers → pending jobs → sleep
until the next timer. When no timer is left it prints `JSBedded: script finished`
and idles. Uncaught errors, in the script or in a timer callback, are printed and
the loop goes on. `ENGINE=none` has no JavaScript and says so. Engines:
`engines/<engine>/`; `ENGINE` defaults to `none`. The host APIs need an engine
that implements the host-function contract — today only `mquickjs`
([`engines/README.md`](engines/README.md)).

The boot script is `core/boot.js` (one `console.log`) unless `SCRIPT=<file.js>`
is given: `core/core.cmake` turns the file into a C array at configure time. With
`SCRIPT`, the build folder and image name get the name of the script's folder
as a suffix (`examples/blink/main.js` → `-blink`). The script must already be
JavaScript the engine runs (ES5 for MQuickJS); TypeScript is compiled in `js/`.

| Folder | Role |
|---|---|
| `core/` | Boot, script loader, event loop, safe mode / recovery, error reporting |
| `api/` | JSBedded hardware APIs, written once against the engine adapter and HAL |
| `engines/` | One adapter per JS engine, all implementing the same contract |
| `hal/` | Hardware abstraction layer, one folder per platform SDK |
| `boards/` | Board manifests: MCU, memory, peripherals, pin map |
| `storage/` | Script storage and flash partition layout |

Layering rule: `api/` and `core/` include only `jsb/engine.h` and `jsb/hal.h`
(plus their own `jsb/core.h`, `jsb/api.h`), never engine- or board-specific
headers; board facts arrive as build definitions (`JSB_BOARD`, `JSB_BOARD_LED`). Only `engines/*` know about an engine; only `hal/*` and `boards/*`
know about hardware.

Build flow: `boards/<board>/board.mk` sets `PLATFORM`, its settings and `BOARD_LED`,
`hal/<platform>/platform.mk` runs that SDK's build, and `core/core.cmake` lists
the shared sources every platform compiles.
