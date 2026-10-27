/*
 * Firmware main loop. Starts the engine, runs the built-in boot script once,
 * then prints a heartbeat line and toggles the status LED once a second.
 * The boot script is replaced by the user script loader later.
 */
#include <stdio.h>
#include <string.h>

#include "jsb/core.h"
#include "jsb/engine.h"
#include "jsb/hal.h"
#include "jsb/version.h"

static const char boot_script[] =
    "console.log('JSBedded: hello from JavaScript (' + 6 * 7 + ')');\n";

static void console_print(const char *s)
{
    jsb_hal_console_write(s, strlen(s));
}

static void run_boot_script(void)
{
    jsb_engine *engine = jsb_engine_create();
    if (engine == NULL) {
        console_print("JSBedded: no JavaScript engine in this image\r\n");
        return;
    }
    if (jsb_engine_eval(engine, boot_script, sizeof boot_script - 1, "boot.js") != 0) {
        console_print("JSBedded: boot script failed\r\n");
    }
    jsb_engine_destroy(engine);
}

void jsb_main(void)
{
    char line[160];
    bool led = false;

    jsb_hal_init();
    run_boot_script();

    for (;;) {
        int n = snprintf(line, sizeof line, "JSBedded %s | board=%s | engine=%s | hello\r\n",
                         JSB_VERSION, JSB_BOARD, jsb_engine_name());
        if (n > 0) {
            size_t len = (size_t)n < sizeof line ? (size_t)n : sizeof line - 1;
            jsb_hal_console_write(line, len);
        }

        led = !led;
        jsb_hal_led_set(led);
        jsb_hal_sleep_ms(1000);
    }
}
