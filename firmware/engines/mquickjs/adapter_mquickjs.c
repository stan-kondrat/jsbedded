/*
 * MQuickJS adapter. The engine allocates only from one static heap of
 * JSB_MQUICKJS_HEAP_SIZE bytes (set per board in engine.cmake), which also holds
 * the JS value stack; the standard library is ROM tables generated at build time
 * (mqjs_stdlib.h). One runtime exists at a time.
 *
 * The globals are those of upstream mqjs_stdlib.c. Its host-specific functions
 * are implemented here: console.log/print write to the HAL console, gc() runs
 * the GC, load/setTimeout/clearTimeout throw until the firmware supports them,
 * and Date/performance count from gettimeofday() (time since boot until a clock
 * source sets the time).
 */
#include <stdint.h>
#include <string.h>
#include <sys/time.h>

#include "cutils.h"
#include "mquickjs.h"

#include "jsb/engine.h"
#include "jsb/hal.h"

#ifndef JSB_MQUICKJS_HEAP_SIZE
#define JSB_MQUICKJS_HEAP_SIZE (64 * 1024)
#endif

struct jsb_engine {
    JSContext *ctx;
};

static uint32_t js_heap[JSB_MQUICKJS_HEAP_SIZE / sizeof(uint32_t)] __attribute__((aligned(8)));
static jsb_engine the_engine;

/* Console output with "\n" turned into "\r\n", as the firmware console expects. */
static void console_write(const char *s, size_t len)
{
    size_t start = 0;
    for (size_t i = 0; i < len; i++) {
        if (s[i] == '\n' && (i == 0 || s[i - 1] != '\r')) {
            if (i > start) {
                jsb_hal_console_write(s + start, i - start);
            }
            jsb_hal_console_write("\r\n", 2);
            start = i + 1;
        }
    }
    if (len > start) {
        jsb_hal_console_write(s + start, len - start);
    }
}

/* Engine output (JS_PrintValueF, error dumps). */
static void js_log_write(void *opaque, const void *buf, size_t buf_len)
{
    (void)opaque;
    console_write(buf, buf_len);
}

static int64_t get_time_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000 + (tv.tv_usec / 1000);
}

/* --- globals referenced by the generated stdlib (mqjs_stdlib.h) --- */

/* console.log(...args) and print(...args): strings as-is, other values as MQuickJS prints them. */
static JSValue js_print(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    (void)this_val;
    for (int i = 0; i < argc; i++) {
        if (i != 0) {
            console_write(" ", 1);
        }
        if (JS_IsString(ctx, argv[i])) {
            JSCStringBuf buf;
            size_t len;
            const char *str = JS_ToCStringLen(ctx, &len, argv[i], &buf);
            if (str != NULL) {
                console_write(str, len);
            }
        } else {
            JS_PrintValueF(ctx, argv[i], JS_DUMP_LONG);
        }
    }
    console_write("\n", 1);
    return JS_UNDEFINED;
}

static JSValue js_gc(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    (void)this_val;
    (void)argc;
    (void)argv;
    JS_GC(ctx);
    return JS_UNDEFINED;
}

static JSValue js_date_constructor(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    double val;
    (void)this_val;
    argc &= ~FRAME_CF_CTOR;
    if (argc == 0) {
        val = (double)get_time_ms();
    } else if (argc == 1 && JS_IsNumber(ctx, argv[0])) {
        if (JS_ToNumber(ctx, &val, argv[0])) {
            return JS_EXCEPTION;
        }
    } else {
        return JS_ThrowTypeError(ctx, "unsupported Date() parameter");
    }
    return JS_NewDate(ctx, val);
}

static JSValue js_date_now(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    (void)this_val;
    (void)argc;
    (void)argv;
    return JS_NewInt64(ctx, get_time_ms());
}

static JSValue js_performance_now(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    (void)this_val;
    (void)argc;
    (void)argv;
    return JS_NewInt64(ctx, get_time_ms());
}

static JSValue js_load(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    (void)this_val;
    (void)argc;
    (void)argv;
    return JS_ThrowTypeError(ctx, "load() is not supported");
}

static JSValue js_setTimeout(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    (void)this_val;
    (void)argc;
    (void)argv;
    return JS_ThrowTypeError(ctx, "setTimeout() is not supported yet");
}

static JSValue js_clearTimeout(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    (void)this_val;
    (void)argc;
    (void)argv;
    return JS_ThrowTypeError(ctx, "clearTimeout() is not supported yet");
}

#include "mqjs_stdlib.h"

/* --- engine.h --- */

const char *jsb_engine_name(void)
{
    return "mquickjs";
}

jsb_engine *jsb_engine_create(void)
{
    if (the_engine.ctx != NULL) {
        return NULL;
    }
    JSContext *ctx = JS_NewContext(js_heap, sizeof js_heap, &js_stdlib);
    if (ctx == NULL) {
        return NULL;
    }
    JS_SetLogFunc(ctx, js_log_write);
    /* Math.random() is xorshift64*: the seed must not be 0. */
    JS_SetRandomSeed(ctx, ((uint64_t)get_time_ms() << 1) | 1);
    the_engine.ctx = ctx;
    return &the_engine;
}

/* MQuickJS expects source[len] == '\0' (see JS_Eval in mquickjs.c). */
int jsb_engine_eval(jsb_engine *engine, const char *source, size_t len, const char *filename)
{
    JSContext *ctx = engine->ctx;
    JSValue val = JS_Eval(ctx, source, len, filename != NULL ? filename : "<eval>", 0);
    if (JS_IsException(val)) {
        /* Errors print as "Name: message" followed by their stack ("at file:line:col"). */
        console_write("Uncaught ", 9);
        JS_PrintValueF(ctx, JS_GetException(ctx), JS_DUMP_LONG);
        console_write("\n", 1);
        return -1;
    }
    return 0;
}

void jsb_engine_destroy(jsb_engine *engine)
{
    if (engine == NULL || engine->ctx == NULL) {
        return;
    }
    JS_FreeContext(engine->ctx);
    engine->ctx = NULL;
}
