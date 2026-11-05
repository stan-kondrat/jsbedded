# hal/pico

HAL implementation on the Raspberry Pi Pico SDK (RP2040 and RP2350).

- Console: USB serial and UART0 (GP0/GP1).
- Console: waits up to 2 s at boot for a USB serial host
  (`PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS`), so the first lines aren't lost.
- GPIO: GP0–GP29 (GP0–GP47 on RP2350B). The LED is GP25 on Pico and Pico 2
  (`board.LED`). On the W boards the LED is on the CYW43 chip and needs the cyw43
  driver (later); GP23–25 and GP29 belong to that chip there.
- Time: `time_us_64()`.
- USB: one composite device, the console (CDC) and the script drive (MSC).
  `usb/tusb_config.h` and `usb_drive.c` replace pico_stdio_usb's CDC-only
  descriptors; pico_stdio_usb still starts TinyUSB, runs `tud_task()` in the
  background and resets to BOOTSEL at 1200 baud. USB IDs: Raspberry Pi's VID with
  the SDK's PID and bcdDevice 0x0200 (a PID of our own is a follow-up).
- Script drive (`flash_disk.c`): the last `DRIVE_KB` of flash in 512-byte
  sectors over a 4 KB write-back block cache. Shows as "no medium" until core
  has read the script ([`../../storage/README.md`](../../storage/README.md)).
- Dev mode (`devmode.c`): BOOTSEL is sampled every 50 ms from a timer; held 2 s
  and released, the board restarts in dev mode (flag in watchdog scratch 0).
  See [recovery](../../../docs/guide/recovery.md).
- Build: `make -C firmware BOARD=pico2` (needs `cmake`, `ninja`, `arm-none-eabi-gcc`).
  The SDK builds `picotool` on first use; set `PICOTOOL_FETCH_FROM_GIT_PATH` to
  keep it outside `build/`.
