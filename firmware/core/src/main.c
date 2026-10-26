/*
 * Firmware main loop. For now it only proves the build and HAL wiring:
 * print a hello line and toggle the status LED once a second.
 */
#include <stdio.h>

#include "jsb/core.h"
#include "jsb/hal.h"
#include "jsb/version.h"

void jsb_main(void)
{
    char line[128];
    bool led = false;

    jsb_hal_init();

    for (;;) {
        int n = snprintf(line, sizeof line, "JSBedded %s | board=%s | hello\r\n",
                         JSB_VERSION, JSB_BOARD);
        if (n > 0) {
            size_t len = (size_t)n < sizeof line ? (size_t)n : sizeof line - 1;
            jsb_hal_console_write(line, len);
        }

        led = !led;
        jsb_hal_led_set(led);
        jsb_hal_sleep_ms(1000);
    }
}
