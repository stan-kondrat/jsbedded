# hal/esp-idf

HAL implementation on ESP-IDF (`third_party/esp-idf`, v6.0.3) for ESP32-S3 and ESP32-C3.

- Console: UART0, the dev board's USB-UART port.
- GPIO: `driver/gpio.h`; outputs are set up as input+output so `read` sees the
  driven level. DevKit LEDs are addressable RGB LEDs and need an RMT driver (later),
  so `board.LED` is undefined.
- Time: `esp_timer_get_time()`.
- Build: run `. third_party/esp-idf/export.sh`, then `make -C firmware BOARD=esp32c3_devkitm`.
  The generated `sdkconfig` goes to the build directory; defaults are in `sdkconfig.defaults`.
- Image: one merged `.bin` written at `0x0`:
  `esptool.py write_flash 0x0 build/<board>-<engine>/jsbedded-<board>-<engine>.bin`.
