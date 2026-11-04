# hal/pico

HAL implementation on the Raspberry Pi Pico SDK (RP2040 and RP2350).

- Console: USB serial and UART0 (GP0/GP1).
- Console: waits up to 2 s at boot for a USB serial host
  (`PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS`), so the first lines aren't lost.
- GPIO: GP0–GP29 (GP0–GP47 on RP2350B). The LED is GP25 on Pico and Pico 2
  (`board.LED`). On the W boards the LED is on the CYW43 chip and needs the cyw43
  driver (later); GP23–25 and GP29 belong to that chip there.
- Time: `time_us_64()`.
- Build: `make -C firmware BOARD=pico2` (needs `cmake`, `ninja`, `arm-none-eabi-gcc`).
  The SDK builds `picotool` on first use; set `PICOTOOL_FETCH_FROM_GIT_PATH` to
  keep it outside `build/`.
