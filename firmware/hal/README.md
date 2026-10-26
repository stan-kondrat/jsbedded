# firmware/hal

Hardware abstraction layer. [`include/jsb/hal.h`](include/jsb/hal.h) is the only
interface `core/` and `api/` may use. One folder per platform SDK; each implements
`hal.h`, provides the program entry point (which calls `jsb_main()`), and has a
`platform.mk` that wraps that SDK's own build (CMake for pico-sdk and ESP-IDF).

| Platform | SDK | MCUs | Image |
|---|---|---|---|
| `pico/` | `third_party/pico-sdk` | RP2040, RP2350 | `.uf2` |
| `esp-idf/` | `third_party/esp-idf` | ESP32-S3, ESP32-C3 | `.bin`, merged, flashed at `0x0` |
