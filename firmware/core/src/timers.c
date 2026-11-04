/*
 * Timer queue for setTimeout/setInterval, driven by jsb_main()'s event loop
 * on jsb_hal_millis(). A fixed table of MAX_TIMERS slots; each holds a
 * retained JS function. Extra arguments after the delay are not passed to the
 * callback (yet).
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "jsb/core.h"
#include "jsb/engine.h"
#include "jsb/hal.h"

#define MAX_TIMERS 16
#define MAX_DELAY_MS 0x7fffffff

typedef struct {
    bool used;
    bool repeat;
    uint32_t id;
    uint32_t interval;
    uint64_t due;
    jsb_func func;
} jsb_timer;

static jsb_timer timers[MAX_TIMERS];
static uint32_t next_id = 1;
static jsb_engine *timer_engine;

static void timer_free(jsb_timer *t)
{
    t->used = false;
    jsb_engine_release(timer_engine, t->func);
    t->func = JSB_FUNC_NONE;
}

/* setTimeout(fn, delay) / setInterval(fn, delay) -> id */
static int add_timer(jsb_call *call, bool repeat)
{
    double delay;
    jsb_timer *slot = NULL;

    if (jsb_arg_type(call, 0) != JSB_FUNCTION) {
        return jsb_throw(call, JSB_TYPE_ERROR, "callback must be a function");
    }
    if (jsb_argc(call) < 2) {
        delay = 0;
    } else if (jsb_arg_number(call, 1, &delay) != 0) {
        return -1;
    }
    /* NaN and negative delays mean 0, as in browsers and Node.js. */
    if (!(delay >= 0)) {
        delay = 0;
    } else if (delay > MAX_DELAY_MS) {
        delay = MAX_DELAY_MS;
    }

    for (int i = 0; i < MAX_TIMERS; i++) {
        if (!timers[i].used) {
            slot = &timers[i];
            break;
        }
    }
    if (slot == NULL) {
        char msg[48];
        snprintf(msg, sizeof msg, "too many timers (max %d)", MAX_TIMERS);
        return jsb_throw(call, JSB_RANGE_ERROR, msg);
    }
    slot->func = jsb_arg_function(call, 0);
    if (slot->func == JSB_FUNC_NONE) {
        return -1;
    }
    slot->used = true;
    slot->repeat = repeat;
    slot->id = next_id++;
    slot->interval = (uint32_t)delay;
    /* An interval of 0 would starve the loop; 1 ms is the shortest period. */
    if (repeat && slot->interval == 0) {
        slot->interval = 1;
    }
    slot->due = jsb_hal_millis() + (uint32_t)delay;
    if (next_id > MAX_DELAY_MS) {
        next_id = 1;
    }

    jsb_value id = { JSB_NUMBER, { .number = (double)slot->id } };
    jsb_return(call, id);
    return 0;
}

static int js_set_timeout(jsb_call *call)
{
    return add_timer(call, false);
}

static int js_set_interval(jsb_call *call)
{
    return add_timer(call, true);
}

/* clearTimeout(id) and clearInterval(id): unknown ids are ignored, as in browsers. */
static int js_clear_timer(jsb_call *call)
{
    double id;
    if (jsb_arg_type(call, 0) != JSB_NUMBER) {
        return 0;
    }
    if (jsb_arg_number(call, 0, &id) != 0) {
        return -1;
    }
    for (int i = 0; i < MAX_TIMERS; i++) {
        if (timers[i].used && (double)timers[i].id == id) {
            timer_free(&timers[i]);
            break;
        }
    }
    return 0;
}

int jsb_timers_install(jsb_engine *engine)
{
    static const jsb_host_def defs[] = {
        JSB_DEF_FN("setTimeout", js_set_timeout),
        JSB_DEF_FN("setInterval", js_set_interval),
        JSB_DEF_FN("clearTimeout", js_clear_timer),
        JSB_DEF_FN("clearInterval", js_clear_timer),
    };
    timer_engine = engine;
    for (int i = 0; i < MAX_TIMERS; i++) {
        timers[i].used = false;
        timers[i].func = JSB_FUNC_NONE;
    }
    return jsb_engine_define(engine, NULL, defs, sizeof defs / sizeof defs[0]);
}

/*
 * Fires due timers earliest first (ties: oldest first). Timers created by the
 * callbacks of this pass (id >= id_limit) wait for the next pass, and a
 * rescheduled interval is due later than now, so the pass always ends.
 */
void jsb_timers_run_due(jsb_engine *engine)
{
    uint64_t now = jsb_hal_millis();
    uint32_t id_limit = next_id;

    for (;;) {
        jsb_timer *t = NULL;
        for (int i = 0; i < MAX_TIMERS; i++) {
            jsb_timer *c = &timers[i];
            if (!c->used || c->due > now || c->id >= id_limit) {
                continue;
            }
            if (t == NULL || c->due < t->due || (c->due == t->due && c->id < t->id)) {
                t = c;
            }
        }
        if (t == NULL) {
            return;
        }

        if (t->repeat) {
            /* Keep the period; skip missed ticks instead of firing a burst. */
            t->due += t->interval;
            if (t->due <= now) {
                t->due = now + t->interval;
            }
            /* The callback may clear its own interval; the engine holds the function during the call. */
            jsb_engine_call(engine, t->func, 0, NULL);
        } else {
            jsb_func func = t->func;
            t->used = false;
            t->func = JSB_FUNC_NONE;
            jsb_engine_call(engine, func, 0, NULL);
            jsb_engine_release(engine, func);
        }
    }
}

int64_t jsb_timers_next_wait(void)
{
    bool any = false;
    uint64_t next = 0;
    for (int i = 0; i < MAX_TIMERS; i++) {
        if (timers[i].used && (!any || timers[i].due < next)) {
            next = timers[i].due;
            any = true;
        }
    }
    if (!any) {
        return -1;
    }
    uint64_t now = jsb_hal_millis();
    return next > now ? (int64_t)(next - now) : 0;
}
