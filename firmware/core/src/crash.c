/* Crash report text and the restart decision, see jsb/crash.h. */
#include <stdarg.h>
#include <stdio.h>

#include "jsb/crash.h"
#include "jsb/version.h"

#define STR(x) #x
#define XSTR(x) STR(x)

const char *jsb_crash_kind_name(uint32_t kind)
{
    switch (kind) {
    case JSB_CRASH_FAULT:
        return "hard fault (bad memory access or stack overflow)";
    case JSB_CRASH_ASSERT:
        return "assert failed";
    case JSB_CRASH_ABORT:
        return "abort()";
    case JSB_CRASH_PANIC:
        return "panic or exit() (out of memory, for example)";
    case JSB_CRASH_FATAL:
        return "fatal error";
    default:
        return "unknown";
    }
}

const char *jsb_crash_stop_reason(const jsb_crash_info *c, bool report_setting)
{
    if (c->new_image) {
        return "first boot after flashing this firmware";
    }
    if (c->count >= JSB_CRASH_RESTART_MAX) {
        return "crashed " XSTR(JSB_CRASH_RESTART_MAX) " times in a row";
    }
    if (report_setting) {
        return "on_crash = report in config.ini";
    }
    return NULL;
}

typedef struct {
    char *out;
    size_t cap, len;
} text;

static void add(text *t, const char *fmt, ...)
{
    if (t->len + 1 >= t->cap) {
        return;
    }
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(t->out + t->len, t->cap - t->len, fmt, ap);
    va_end(ap);
    if (n > 0) {
        t->len += (size_t)n < t->cap - t->len ? (size_t)n : t->cap - t->len - 1;
    }
}

size_t jsb_crash_format(const jsb_crash_info *c, uint32_t image_id, const char *stop_reason, char *out,
                        size_t cap)
{
    if (cap == 0) {
        return 0;
    }
    out[0] = '\0';
    text t = { out, cap, 0 };
    add(&t, "JSBedded crash report\r\n"
            "=====================\r\n"
            "The previous boot crashed and the board restarted. This file is replaced\r\n"
            "at the next crash.\r\n"
            "\r\n");
    add(&t, "crash:    %s\r\n", jsb_crash_kind_name(c->kind));
    if (c->msg[0] != '\0') {
        add(&t, "message:  %s\r\n", c->msg);
    }
    if (c->file[0] != '\0') {
        add(&t, "source:   %s:%ld\r\n", c->file, (long)c->line);
    }
    add(&t, "step:     %s\r\n", c->step[0] != '\0' ? c->step : "-");
    add(&t, "pc:       %08lx\r\n", (unsigned long)c->pc);
    add(&t, "lr:       %08lx\r\n", (unsigned long)c->lr);
    add(&t, "sp:       %08lx\r\n", (unsigned long)c->sp);
    add(&t, "uptime:   %lu ms\r\n", (unsigned long)c->uptime_ms);
    add(&t, "in a row: %lu\r\n", (unsigned long)c->count);
    add(&t, "image:    %08lx%s\r\n", (unsigned long)c->image_id,
        c->image_id != image_id ? " (not the image on the board now)"
                                : c->new_image ? " (its first boot after flashing)" : "");
    add(&t, "firmware: %s, board %s, engine %s %s, image %08lx\r\n", JSB_VERSION, JSB_BOARD, JSB_ENGINE,
        JSB_ENGINE_VERSION, (unsigned long)image_id);
    add(&t, "\r\n");
    if (stop_reason != NULL) {
        add(&t, "The script was not started (%s).\r\n"
                "Reset the board to run it again.\r\n", stop_reason);
    } else {
        add(&t, "The script was started again (on_crash = restart in config.ini).\r\n");
    }
    return t.len;
}
