# Install the firmware

## Download

Download the image for your board from the
[`latest` pre-release](https://github.com/stan-kondrat/jsbedded/releases/tag/latest)
(rebuilt on every push to `main`) or a tagged release. Check it against `SHA256SUMS`.

Images are built per board and engine; `none` has no JavaScript. Without a
script of your own they print one hello line from the built-in boot script.

## Pico family

Pico, Pico W, Pico 2 and Pico 2 W.

1. Hold BOOTSEL and plug in USB.
2. Copy `jsbedded-<board>-<engine>.uf2` to the drive that appears (`RPI-RP2` or
   `RP2350`, the boot ROM's drive, which only takes UF2 files).

The board restarts into JSBedded and shows up as two devices:

- a USB serial port: the console (`console.log`, errors);
- a USB drive named `JSBEDDED`: put `main.js` there and reset to run it
  ([your first script](./first-script.md)). It is formatted on first boot and
  kept when you flash a newer UF2.

## ESP32-S3 / ESP32-C3

1. Connect the dev board's USB-UART port.
2. Flash the image:

   ```bash
   esptool.py write_flash 0x0 jsbedded-<board>-<engine>.bin
   ```

Output appears on the same serial port at 115200 baud.
