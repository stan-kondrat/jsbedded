# Recovery

## Dev mode (Pico family)

Dev mode starts the board without running the script, so a script that crashes,
loops or takes over the USB port can be fixed on the `JSBEDDED` drive.

1. With the board running, **hold BOOTSEL for 2 seconds**. The LED lights up.
2. **Release BOOTSEL.** The board restarts in dev mode: the drive is there, the
   script is not started, and the LED blinks briefly once a second. The serial
   port says `dev mode`.
3. Fix or delete `main.js` on the drive, eject it, then reset the board (unplug
   and plug in, or hold BOOTSEL 2 s again) to run the script.

Holding BOOTSEL *while plugging in* is different: that starts the boot ROM's
`RPI-RP2` / `RP2350` drive for flashing a UF2 (see
[install](./install-firmware.md)). It also gets out of any situation, since the
ROM never runs JSBedded; flashing a UF2 doesn't erase the `JSBEDDED` drive.

Dev mode is remembered across the restart only; a power cycle starts normally.

Planned: settings for the drive and restarts in `config.ini`; boot loops
detected and falling back to dev mode automatically; the BOOT button on ESP32
boards.
