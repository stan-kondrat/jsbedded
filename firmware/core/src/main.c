/*
 * Firmware main loop. Picks the script (see pick_script), starts the engine,
 * installs the host APIs, runs the script, then runs the event loop: due timers
 * -> pending jobs -> sleep until the next timer. When no timer is left the
 * script has finished and the board idles.
 *
 * Script, where the platform has a script drive (JSB_STORAGE): the first of
 * main.js, index.js, main.mjs, index.mjs on the drive, read once at boot; else
 * the boot script built into the image (SCRIPT=, default core/boot.js). After
 * reading, the drive is handed to the PC; a changed script runs after a reset.
 * In dev mode (jsb_hal_devmode) the drive is handed over and no script runs.
 */
#include <stdio.h>
#include <string.h>

#include "jsb/api.h"
#include "jsb/core.h"
#include "jsb/engine.h"
#include "jsb/hal.h"
#include "jsb/version.h"
#ifdef JSB_STORAGE
#include "jsb/storage.h"
#endif

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

/* Dev mode: no script. The on-board LED, where there is one, blinks briefly once a second. */
static void devmode_idle(void) __attribute__((noreturn));
static void devmode_idle(void)
{
#ifdef JSB_BOARD_LED
    jsb_hal_gpio_mode(JSB_BOARD_LED, JSB_GPIO_OUTPUT);
    for (;;) {
        jsb_hal_gpio_write(JSB_BOARD_LED, true);
        jsb_hal_sleep_ms(100);
        jsb_hal_gpio_write(JSB_BOARD_LED, false);
        jsb_hal_sleep_ms(900);
    }
#else
    idle_forever();
#endif
}

typedef struct {
    const char *source;
    size_t len;
    const char *name;
    const char *from;
} script_choice;

/*
 * Read the script from the drive if there is one, then hand the drive to the PC.
 * A script file that can't be read (too large, I/O error) gives source NULL:
 * nothing runs, rather than the built-in script in its place.
 */
static script_choice pick_script(bool devmode)
{
    script_choice s = { (const char *)jsb_script, jsb_script_len, jsb_script_name, "built in" };
#ifdef JSB_STORAGE
    static jsb_storage_script file;
    if (jsb_storage_mount() == 0) {
        int found = devmode ? 0 : jsb_storage_load_script(&file);
        if (found == 1) {
            s.source = file.source;
            s.len = file.len;
            s.name = file.name;
            s.from = "drive " JSB_STORAGE_LABEL;
        } else if (found < 0) {
            s.source = NULL;
            s.name = "-";
            s.from = "not loaded";
        }
        jsb_storage_unmount();
    }
    jsb_hal_drive_attach();
#else
    (void)devmode;
#endif
    return s;
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
    bool devmode = jsb_hal_devmode();
    script_choice script = pick_script(devmode);

    int n = snprintf(line, sizeof line, "JSBedded %s | board=%s | engine=%s | script=%s (%s)\r\n",
                     JSB_VERSION, JSB_BOARD, jsb_engine_name(), devmode ? "-" : script.name,
                     devmode ? "dev mode" : script.from);
    if (n > 0) {
        jsb_hal_console_write(line, (size_t)n < sizeof line ? (size_t)n : sizeof line - 1);
    }
    if (devmode) {
        console_print("JSBedded: dev mode: the script is not started. Edit it on the drive, then reset "
                      "(or hold the boot button 2 s again) to run it.\r\n");
        devmode_idle();
    }
    if (script.source == NULL) {
        console_print("JSBedded: no script started; fix it on the drive, then reset\r\n");
        idle_forever();
    }

    jsb_engine *engine = jsb_engine_create();
    if (engine == NULL) {
        console_print("JSBedded: no JavaScript engine in this image\r\n");
        idle_forever();
    }
    install_apis(engine);

    /* An uncaught error is reported by the engine; timers the script already set keep running. */
    jsb_engine_eval(engine, script.source, script.len, script.name);
    jsb_engine_run_jobs(engine);
    run_event_loop(engine);

    console_print("JSBedded: script finished\r\n");
    idle_forever();
}
