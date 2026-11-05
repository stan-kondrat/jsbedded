---
board: pico
---

# Raspberry Pi Pico

Facts: `firmware/boards/pico/board.json`. Status: see the [matrix](../matrix.md).

## Script drive and dev mode

- USB drive `JSBEDDED`: the last 1 MB of the 2 MB flash (`DRIVE_KB` in
  `firmware/boards/pico/board.mk`). Copy `main.js` there and reset the board
  (unplug and plug in; the Pico has no reset button). See
  [your first script](../guide/first-script.md).
- Dev mode: hold BOOTSEL for 2 s while the firmware runs, then release it; see
  [recovery](../guide/recovery.md). Holding BOOTSEL while plugging in starts the
  boot ROM's UF2 drive instead.
- LED: GP25 (`board.LED`); lights while BOOTSEL has been held 2 s, blinks once
  a second in dev mode.
