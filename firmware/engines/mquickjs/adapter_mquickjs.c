/*
 * MQuickJS adapter. The engine allocates only from one static heap of
 * JSB_MQUICKJS_HEAP_SIZE bytes (set per board in engine.cmake), which also holds
 * the JS value stack; the standard library is ROM tables generated at build time
 * from jsb_stdlib.c (jsb_stdlib.h). One runtime exists at a time.
 *
 * Host functions (engine.h): MQuickJS can't add C functions at runtime, so the
 * stdlib declares one C function, jsb_host, and jsb_engine_define() creates a
 * closure of it per host function, whose params value indexes host_fns[].
 * Retained JS functions live in a fixed pool of GC references.
 *
 * Built-in host functions here: gc(), and Date/performance counting from
 * gettimeofday() (time since boot until a clock source sets the time).
 */
#include <stdbool.h>
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

/* Host functions defined with jsb_engine_define(), per runtime. */
#define MAX_HOST_FNS 32
/* JS functions retained with jsb_arg_function() (timers, callbacks). */
#define MAX_FUNCS 32

/* Index of jsb_host in jsb_stdlib.c's jsb_c_function_decl[]. */
#define JS_CFUNCTION_jsb_host (JS_CFUNCTION_USER + 0)

struct jsb_engine {
    JSContext *ctx;
};

struct jsb_call {
    JSContext *ctx;
    int argc;
    JSValue *argv;
    JSValue *ret; /* GC root for the return value */
};

static uint32_t js_heap[JSB_MQUICKJS_HEAP_SIZE / sizeof(uint32_t)] __attribute__((aligned(8)));
static jsb_engine the_engine;

static jsb_host_fn host_fns[MAX_HOST_FNS];
static int host_fn_count;

static JSGCRef func_refs[MAX_FUNCS];
static bool func_used[MAX_FUNCS];

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

static void console_print(const char *s)
{
    console_write(s, strlen(s));
}

/* Engine output (JS_PrintValueF, error dumps). */
static void js_log_write(void *opaque, const void *buf, size_t buf_len)
{
    (void)opaque;
    console_write(buf, buf_len);
}

/* Errors print as "Name: message" followed by their stack ("at file:line:col"). */
static void print_exception(JSContext *ctx, const char *prefix)
{
    console_print(prefix);
    JS_PrintValueF(ctx, JS_GetException(ctx), JS_DUMP_LONG);
    console_write("\n", 1);
}

static int64_t get_time_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000 + (tv.tv_usec / 1000);
}

static bool func_valid(jsb_func func)
{
    return func >= 0 && func < MAX_FUNCS && func_used[func];
}

/* Allocates for strings and non-integer numbers; JS_EXCEPTION when out of memory. */
static JSValue to_js(JSContext *ctx, const jsb_value *v)
{
    switch (v->type) {
    case JSB_BOOL:
        return JS_NewBool(v->u.boolean);
    case JSB_NUMBER:
        return JS_NewFloat64(ctx, v->u.number);
    case JSB_STRING:
        return v->u.string != NULL ? JS_NewString(ctx, v->u.string) : JS_UNDEFINED;
    case JSB_FUNCTION:
        return func_valid(v->u.func) ? func_refs[v->u.func].val : JS_UNDEFINED;
    default:
        return JS_UNDEFINED;
    }
}

/* --- globals referenced by the generated stdlib (jsb_stdlib.h) --- */

/* Every host function: a closure whose params is its index in host_fns[]. */
static JSValue jsb_mquickjs_host(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv,
                                 JSValue params)
{
    JSGCRef ret_ref;
    jsb_call call;
    int idx = JS_VALUE_GET_INT(params);
    int rc;

    (void)this_val;
    if (!JS_IsInt(params) || idx < 0 || idx >= host_fn_count) {
        return JS_ThrowInternalError(ctx, "unknown host function");
    }
    call.ctx = ctx;
    call.argc = argc & ~FRAME_CF_CTOR;
    call.argv = argv;
    call.ret = JS_PushGCRef(ctx, &ret_ref);
    rc = host_fns[idx](&call);
    JSValue ret = JS_PopGCRef(ctx, &ret_ref);
    return rc != 0 ? JS_EXCEPTION : ret;
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

#include "jsb_stdlib.h"

/* --- engine.h: runtime --- */

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
    host_fn_count = 0;
    memset(func_used, 0, sizeof func_used);
    the_engine.ctx = ctx;
    return &the_engine;
}

/* MQuickJS expects source[len] == '\0' (see JS_Eval in mquickjs.c). */
int jsb_engine_eval(jsb_engine *engine, const char *source, size_t len, const char *filename)
{
    JSContext *ctx = engine->ctx;
    JSValue val = JS_Eval(ctx, source, len, filename != NULL ? filename : "<eval>", 0);
    if (JS_IsException(val)) {
        print_exception(ctx, "Uncaught ");
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
    host_fn_count = 0;
    memset(func_used, 0, sizeof func_used);
}

/* --- engine.h: host functions --- */

int jsb_engine_define(jsb_engine *engine, const char *object, const jsb_host_def *defs, size_t count)
{
    JSContext *ctx = engine->ctx;
    JSGCRef obj_ref;
    JSValue *obj = JS_PushGCRef(ctx, &obj_ref);
    const char *failed = object;
    int rc = -1;

    if (object != NULL) {
        *obj = JS_NewObject(ctx);
        if (JS_IsException(*obj) ||
            JS_IsException(JS_SetPropertyStr(ctx, JS_GetGlobalObject(ctx), object, *obj))) {
            goto done;
        }
    } else {
        *obj = JS_GetGlobalObject(ctx);
    }

    for (size_t i = 0; i < count; i++) {
        const jsb_host_def *def = &defs[i];
        JSValue val;
        failed = def->name;
        if (def->fn != NULL) {
            if (host_fn_count >= MAX_HOST_FNS) {
                console_print("JSBedded: mquickjs: too many host functions, can't define ");
                console_print(def->name);
                console_write("\n", 1);
                JS_PopGCRef(ctx, &obj_ref);
                return -1;
            }
            host_fns[host_fn_count] = def->fn;
            val = JS_NewCFunctionParams(ctx, JS_CFUNCTION_jsb_host, JS_NewInt32(ctx, host_fn_count));
            if (!JS_IsException(val)) {
                host_fn_count++;
            }
        } else {
            val = to_js(ctx, &def->value);
        }
        if (JS_IsException(val) || JS_IsException(JS_SetPropertyStr(ctx, *obj, def->name, val))) {
            goto done;
        }
    }
    rc = 0;

done:
    JS_PopGCRef(ctx, &obj_ref);
    if (rc != 0) {
        console_print("JSBedded: can't define ");
        console_print(failed != NULL ? failed : "?");
        print_exception(ctx, ": ");
    }
    return rc;
}

int jsb_engine_call(jsb_engine *engine, jsb_func func, int argc, const jsb_value *argv)
{
    JSContext *ctx = engine->ctx;

    if (!func_valid(func)) {
        return -1;
    }
    if (JS_StackCheck(ctx, (uint32_t)argc + 2)) {
        print_exception(ctx, "Uncaught ");
        return -1;
    }
    /* Arguments go on the stack last first, then the function and this. */
    for (int i = argc - 1; i >= 0; i--) {
        JSValue v = to_js(ctx, &argv[i]);
        if (JS_IsException(v)) {
            (void)JS_GetException(ctx);
            v = JS_UNDEFINED;
        }
        JS_PushArg(ctx, v);
    }
    JS_PushArg(ctx, func_refs[func].val);
    JS_PushArg(ctx, JS_UNDEFINED);
    if (JS_IsException(JS_Call(ctx, argc))) {
        print_exception(ctx, "Uncaught ");
        return -1;
    }
    return 0;
}

void jsb_engine_release(jsb_engine *engine, jsb_func func)
{
    if (engine == NULL || engine->ctx == NULL || !func_valid(func)) {
        return;
    }
    JS_DeleteGCRef(engine->ctx, &func_refs[func]);
    func_used[func] = false;
}

/* MQuickJS has no Promises, so no jobs. */
int jsb_engine_run_jobs(jsb_engine *engine)
{
    (void)engine;
    return 0;
}

/* --- engine.h: inside a host function --- */

static JSValue arg(jsb_call *call, int i)
{
    return i >= 0 && i < call->argc ? call->argv[i] : JS_UNDEFINED;
}

int jsb_argc(jsb_call *call)
{
    return call->argc;
}

jsb_type jsb_arg_type(jsb_call *call, int i)
{
    JSContext *ctx = call->ctx;
    JSValue v = arg(call, i);
    if (JS_IsUndefined(v)) {
        return JSB_UNDEFINED;
    }
    if (JS_IsBool(v)) {
        return JSB_BOOL;
    }
    if (JS_IsNumber(ctx, v)) {
        return JSB_NUMBER;
    }
    if (JS_IsString(ctx, v)) {
        return JSB_STRING;
    }
    if (JS_IsFunction(ctx, v)) {
        return JSB_FUNCTION;
    }
    return JSB_OTHER;
}

int jsb_arg_number(jsb_call *call, int i, double *out)
{
    return JS_ToNumber(call->ctx, out, arg(call, i)) != 0 ? -1 : 0;
}

int jsb_arg_string(jsb_call *call, int i, char *buf, size_t size)
{
    JSCStringBuf sbuf;
    size_t len;
    const char *s = JS_ToCStringLen(call->ctx, &len, arg(call, i), &sbuf);
    if (s == NULL) {
        return -1;
    }
    if (size == 0) {
        return 0;
    }
    if (len > size - 1) {
        len = size - 1;
    }
    memcpy(buf, s, len);
    buf[len] = '\0';
    return (int)len;
}

jsb_func jsb_arg_function(jsb_call *call, int i)
{
    JSContext *ctx = call->ctx;
    if (!JS_IsFunction(ctx, arg(call, i))) {
        JS_ThrowTypeError(ctx, "argument %d is not a function", i + 1);
        return JSB_FUNC_NONE;
    }
    for (int k = 0; k < MAX_FUNCS; k++) {
        if (!func_used[k]) {
            JSValue *ref = JS_AddGCRef(ctx, &func_refs[k]);
            *ref = arg(call, i);
            func_used[k] = true;
            return k;
        }
    }
    JS_ThrowInternalError(ctx, "too many pending callbacks (max %d)", MAX_FUNCS);
    return JSB_FUNC_NONE;
}

/* Strings as-is, other values as MQuickJS prints them ({ a: 1 }, [ 1, 2 ]). */
int jsb_arg_log(jsb_call *call, int i)
{
    JSContext *ctx = call->ctx;
    JSValue v = arg(call, i);
    if (JS_IsString(ctx, v)) {
        JSCStringBuf sbuf;
        size_t len;
        const char *s = JS_ToCStringLen(ctx, &len, v, &sbuf);
        if (s == NULL) {
            return -1;
        }
        console_write(s, len);
    } else {
        JS_PrintValueF(ctx, v, JS_DUMP_LONG);
    }
    return 0;
}

void jsb_return(jsb_call *call, jsb_value value)
{
    *call->ret = to_js(call->ctx, &value);
    if (JS_IsException(*call->ret)) {
        (void)JS_GetException(call->ctx);
        *call->ret = JS_UNDEFINED;
    }
}

int jsb_throw(jsb_call *call, jsb_error kind, const char *message)
{
    JSObjectClassEnum cls = kind == JSB_TYPE_ERROR    ? JS_CLASS_TYPE_ERROR
                            : kind == JSB_RANGE_ERROR ? JS_CLASS_RANGE_ERROR
                                                      : JS_CLASS_ERROR;
    JS_ThrowError(call->ctx, cls, "%s", message);
    return -1;
}
