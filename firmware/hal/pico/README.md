# hal/pico

HAL implementation on the Raspberry Pi Pico SDK (RP2040 and RP2350).

- Console: USB serial and UART0 (GP0/GP1).
- LED: `PICO_DEFAULT_LED_PIN` on Pico and Pico 2. On the W boards the LED is on
  the CYW43 chip and needs the cyw43 driver, so it is a no-op for now.
- Build: `make -C firmware BOARD=pico2` (needs `cmake`, `ninja`, `arm-none-eabi-gcc`).
  The SDK builds `picotool` on first use; set `PICOTOOL_FETCH_FROM_GIT_PATH` to
  keep it outside `build/`.
