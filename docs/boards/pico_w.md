---
board: pico_w
---

# Raspberry Pi Pico W

Facts: `firmware/boards/pico_w/board.json`. Status: see the [matrix](../matrix.md).

## Script drive and dev mode

As on the [Pico](./pico.md): USB drive `JSBEDDED` in the last 1 MB of the 2 MB
flash (`DRIVE_KB`), and dev mode by holding BOOTSEL 2 s while the firmware runs.
The LED is on the CYW43 chip, so there is no LED feedback yet. Not tested on hardware yet.
