/*
 * Crash report: what to do after the previous boot crashed (jsb_hal_crash_take),
 * and the text of CRASH.TXT. Pure C over jsb_crash_info: unit-testable on the host.
 */
#ifndef JSB_CRASH_H
#define JSB_CRASH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "jsb/hal.h"

/* With on_crash = restart, this many crashes in a row stop the script as with report. */
#define JSB_CRASH_RESTART_MAX 3

/* Running this long without a crash ends a crash streak (jsb_hal_crash_streak_reset). */
#define JSB_CRASH_STREAK_RESET_MS 30000

/* Big enough for any report jsb_crash_format writes. */
#define JSB_CRASH_TEXT_MAX 1024

/* "hard fault", "assert failed", ... */
const char *jsb_crash_kind_name(uint32_t kind);

/*
 * Why the script must not start after crash c, or NULL if it starts again.
 * report_setting: config.ini says on_crash = report. The first boot after
 * flashing a new image and JSB_CRASH_RESTART_MAX crashes in a row stop it too.
 */
const char *jsb_crash_stop_reason(const jsb_crash_info *c, bool report_setting);

/*
 * The report for crash c (CRLF lines), into out. image_id: the image running now;
 * stop_reason: from jsb_crash_stop_reason. Returns the length (truncated to cap - 1).
 */
size_t jsb_crash_format(const jsb_crash_info *c, uint32_t image_id, const char *stop_reason, char *out,
                        size_t cap);

#endif
