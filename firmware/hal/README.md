# firmware/hal

Hardware abstraction layer. `hal.h` (not written yet) is the only interface
`core/` and `api/` may use. One folder per platform SDK; each provides
`platform.mk`, which wraps that SDK's own build (CMake for pico-sdk and ESP-IDF).

| Platform | SDK | MCUs |
|---|---|---|
| `pico/` | `third_party/pico-sdk` | RP2040, RP2350 |
| `esp-idf/` | ESP-IDF (later) | ESP32-S3, ESP32-C3, … |
