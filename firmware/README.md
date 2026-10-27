# firmware

C code that runs on the microcontroller. Builds on its own:

```bash
make -C firmware BOARD=pico2                  # -> build/pico2-none/jsbedded-pico2-none.uf2
. third_party/esp-idf/export.sh               # ESP32 boards need the ESP-IDF environment
make -C firmware BOARD=esp32s3_devkitc        # -> build/esp32s3_devkitc-none/jsbedded-esp32s3_devkitc-none.bin
make -C firmware boards engines               # list boards and engines
```

Today the image runs a one-line boot script on the engine (`ENGINE=none` has no
JavaScript and says so), then prints a heartbeat line once a second and blinks
the LED where there is a simple one. Engines: `engines/<engine>/`; `ENGINE`
defaults to `none` until a real engine builds.

| Folder | Role |
|---|---|
| `core/` | Boot, script loader, event loop, safe mode / recovery, error reporting |
| `api/` | JSBedded hardware APIs, written once against the engine adapter and HAL |
| `engines/` | One adapter per JS engine, all implementing the same contract |
| `hal/` | Hardware abstraction layer, one folder per platform SDK |
| `boards/` | Board manifests: MCU, memory, peripherals, pin map |
| `storage/` | Script storage and flash partition layout |

Layering rule: `api/` and `core/` never include engine- or board-specific
headers. Only `engines/*` know about an engine; only `hal/*` and `boards/*`
know about hardware.

Build flow: `boards/<board>/board.mk` sets `PLATFORM` and its settings,
`hal/<platform>/platform.mk` runs that SDK's build, and `core/core.cmake` lists
the shared sources every platform compiles.
