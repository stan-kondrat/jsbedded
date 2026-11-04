/*
 * Firmware main loop. Starts the engine, installs the host APIs, runs the boot
 * script (built in with SCRIPT=, see core.cmake), then runs the event loop:
 * due timers -> pending jobs -> sleep until the next timer. When no timer is
 * left the script has finished and the board idles.
 */
#include <stdio.h>
#include <string.h>

#include "jsb/api.h"
#include "jsb/core.h"
#include "jsb/engine.h"
#include "jsb/hal.h"
#include "jsb/version.h"

/* Longest sleep, so later event sources (I/O, serial upload) are polled at least this often. */
#define MAX_SLEEP_MS 1000

static void console_print(const char *s)
{
    jsb_hal_console_write(s, strlen(s));
}

static void idle_forever(void) __attribute__((noreturn));
static void idle_forever(void)
{
    for (;;) {
        jsb_hal_sleep_ms(MAX_SLEEP_MS);
    }
}

/* Each API reports its own failure; the script still runs without it. */
static void install_apis(jsb_engine *engine)
{
    jsb_api_console_install(engine);
    jsb_timers_install(engine);
    jsb_api_gpio_install(engine);
    jsb_board_install(engine);
}

static void run_event_loop(jsb_engine *engine)
{
    for (;;) {
        jsb_timers_run_due(engine);
        jsb_engine_run_jobs(engine);
        int64_t wait = jsb_timers_next_wait();
        if (wait < 0) {
            return;
        }
        jsb_hal_sleep_ms(wait < MAX_SLEEP_MS ? (uint32_t)wait : MAX_SLEEP_MS);
    }
}

void jsb_main(void)
{
    char line[160];

    jsb_hal_init();

    int n = snprintf(line, sizeof line, "JSBedded %s | board=%s | engine=%s | script=%s\r\n",
                     JSB_VERSION, JSB_BOARD, jsb_engine_name(), jsb_script_name);
    if (n > 0) {
        jsb_hal_console_write(line, (size_t)n < sizeof line ? (size_t)n : sizeof line - 1);
    }

    jsb_engine *engine = jsb_engine_create();
    if (engine == NULL) {
        console_print("JSBedded: no JavaScript engine in this image\r\n");
        idle_forever();
    }
    install_apis(engine);

    /* An uncaught error is reported by the engine; timers the script already set keep running. */
    jsb_engine_eval(engine, (const char *)jsb_script, jsb_script_len, jsb_script_name);
    jsb_engine_run_jobs(engine);
    run_event_loop(engine);

    console_print("JSBedded: script finished\r\n");
    idle_forever();
}
