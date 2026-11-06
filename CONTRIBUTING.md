# Contributing

JSBedded is in early development. Issues, board and engine reports, and design
discussion are the most useful contributions right now.

## Ground rules

- Every top-level folder builds on its own; `make` at the root just orchestrates.
- Public JS API changes start in the API typings in `js/` (the `.d.ts` files are the spec).
- Firmware internals are documented in C headers; reference docs are generated.
- Support claims come from test results, never from hand-edited tables.
- Engine fixes go upstream (for example to Ant); submodules stay pristine; unavoidable local fixes go in `third_party/patches/`.

## Tooling

Node.js with npm for `js/` and `docs/`; no pnpm or yarn.

## Required software

| For | You need |
|---|---|
| Everything | `git` (clone with `--recurse-submodules`, or run `git submodule update --init` for the ones you use) and GNU `make` |
| `js/`, `docs/` | Node.js with npm |
| Pico firmware (`BOARD=pico*`) | `cmake`, `ninja`, `arm-none-eabi-gcc` with newlib (`gcc-arm-none-eabi`, `libnewlib-arm-none-eabi`, `libstdc++-arm-none-eabi-newlib` on Debian/Ubuntu), Python 3 (pico-sdk's build runs it), and a host C/C++ compiler: the SDK builds `picotool` from source on first use. Alternatively install a prebuilt [picotool](https://github.com/raspberrypi/pico-sdk-tools/releases) matching the SDK version (see `third_party/pico-sdk`) and point CMake at it with `-Dpicotool_DIR=<dir>` |
| ESP32 firmware (`BOARD=esp32*`) | the ESP-IDF prerequisites for the pinned `third_party/esp-idf` version (Python 3, cmake, ninja, git); `make` installs the rest |
| `ENGINE=mquickjs` | a host C compiler (`cc`, `gcc` or `clang`, or `HOST_CC`): it generates the standard library tables at configure time |

CI installs the Pico tools with `apt` (see `.github/workflows/firmware.yml`), so
Ubuntu/Debian package names above match what CI uses. Without root access the
ARM toolchain, cmake and ninja can also be installed with
[xpm](https://xpack.github.io/xpm/) (`@xpack-dev-tools/arm-none-eabi-gcc`,
`@xpack-dev-tools/cmake`, `@xpack-dev-tools/ninja-build`).

Engines that are not GPL-compatible with MIT, such as `mjs` (GPLv2), produce
images released under their own license; see the engine's README.

## Commit style

Short imperative subject, optional scope: `firmware/gpio: add pull-up option`.
