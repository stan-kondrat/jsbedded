# Getting started

## 1. Install the firmware

Download the image for your board from the GitHub release and flash it. Pico
boards take a UF2 file over USB, with no tools needed.

[Read more →](./install-firmware.md)

## 2. Save your script

The board shows up as a USB drive named `JSBEDDED`. Save your `main.js` there.
`console.log` output appears on the board's USB serial port.

[Read more →](./first-script.md)

## 3. Run it

`main.js` runs after a reset, or as soon as you save it, depending on
`soft_restart` in `config.ini` on the same drive (default: `on_save`).

If the firmware crashes, the board restarts and `CRASH.TXT` on the drive says
what happened; `on_crash` in `config.ini` chooses whether the script then runs
again (default) or waits for a reset.

[Read more →](./recovery.md)
