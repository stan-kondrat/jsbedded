# third_party

Pinned git submodules, never modified in place. Fixes go upstream; until they land,
local fixes live as patch files in [`patches/<lib>/`](patches/README.md), applied at build time.
Every submodule has `ignore = dirty`, so build output inside it does not show in `git status`.

| Submodule | Upstream | Pinned at | Used by |
|---|---|---|---|
| `ant/` | https://github.com/theMackabu/ant | `v16.5.c41757b4.0` | `firmware/engines/ant` |
| `mquickjs/` | https://github.com/bellard/mquickjs | `6d4d7eb` (main, 2026-09-26; no release tags) | `firmware/engines/mquickjs` |
| `quickjs/` | https://github.com/bellard/quickjs | `3d5e064` (release 2026-06-04; no tags, releases bump `VERSION`) | `firmware/engines/quickjs` |
| `pico-sdk/` | https://github.com/raspberrypi/pico-sdk | `2.3.1` | `firmware/hal/pico` |
| `esp-idf/` | https://github.com/espressif/esp-idf | `v6.0.3` (shallow) | `firmware/hal/esp-idf` |

Pins are exact releases where upstream publishes them. QuickJS fixes made after
its last release (for example the Intl use-after-free fix in `4490e58`) are
candidates for `patches/quickjs/` once the engine is built.

```bash
# Pico boards: the SDK and only the nested library the firmware uses
git submodule update --init --depth 1 third_party/pico-sdk
git -C third_party/pico-sdk submodule update --init --depth 1 lib/tinyusb

# ESP32 boards: ESP-IDF with its submodules, then its toolchains (into ~/.espressif)
git submodule update --init --depth 1 third_party/esp-idf
git -C third_party/esp-idf submodule update --init --recursive --depth 1
third_party/esp-idf/install.sh esp32s3,esp32c3

# Engines
git submodule update --init third_party/ant third_party/mquickjs third_party/quickjs
```
