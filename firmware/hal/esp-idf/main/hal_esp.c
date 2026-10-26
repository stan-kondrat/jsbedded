/* HAL on ESP-IDF (ESP32-S3, ESP32-C3). Console: UART0 (the dev board's USB-UART port). */
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "jsb/core.h"
#include "jsb/hal.h"

void jsb_hal_init(void)
{
}

void jsb_hal_console_write(const char *s, size_t len)
{
    fwrite(s, 1, len, stdout);
    fflush(stdout);
}

void jsb_hal_sleep_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

/* DevKit status LEDs are addressable RGB LEDs; they need an RMT driver (later). */
void jsb_hal_led_set(bool on)
{
    (void)on;
}

void app_main(void)
{
    jsb_main();
}
