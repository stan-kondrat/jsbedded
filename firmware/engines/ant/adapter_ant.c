/*
 * Ant adapter (theMackabu/ant, Silver interpreter, no JIT) for the embed
 * profile (-DANT_EMBED=1, include/embed.h, added by third_party/patches/ant/).
 *
 * One runtime at a time, set up like Ant's own WebAssembly reactor
 * (packages/wasm/src/bridge.c). engine.cmake compiles this file with Clang,
 * together with the core, because Ant's headers need it.
 *
 * Host functions (engine.h): each one is a native closure around
 * host_trampoline() whose data slot holds an index into host_fns[]. Retained JS
 * functions live in a hidden global array, so the GC keeps them alive.
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ant.h"
#include "internal.h"
#include "cfunc.h"
#include "errors.h"
#include "embed.h"
#include "gc/roots.h"
#include "silver/call.h"
#include "runtime.h"
#include "modules/io.h"
#include "modules/bigint.h"
#include "modules/collections.h"
#include "modules/generator.h"
#include "modules/json.h"
#include "modules/math.h"
#include "modules/regex.h"
#include "modules/timer.h"
#include "modules/symbol.h"
#include "modules/iterator.h"

#include "jsb/engine.h"
#include "jsb/hal.h"

#define MAX_HOST_FNS 32
#define MAX_FUNCS 32

/* Hidden global holding retained functions (index = jsb_func). */
#define FUNCS_GLOBAL "\x01jsb_funcs"

struct jsb_engine {
    ant_t *js;
};

struct jsb_call {
    ant_t *js;
    ant_value_t *args;
    int nargs;
    ant_value_t ret;
    ant_value_t err;
};

static jsb_engine the_engine;
static jsb_host_fn host_fns[MAX_HOST_FNS];
static int host_fn_count;
static bool func_used[MAX_FUNCS];

/* --- embed host hooks (embed.h) --- */

double ant_embed_now_ms(void)
{
    return (double)jsb_hal_millis();
}

bool ant_embed_should_interrupt(ant_t *js)
{
    (void)js;
    return false;
}

/*
 * Backs Math.random(). xorshift64*, seeded from the boot time: fine for
 * Math.random (which is not cryptographic), never for keys.
 */
int ant_embed_random_fill(void *buf, size_t len)
{
    static uint64_t state;
    if (state == 0) {
        state = 0x9E3779B97F4A7C15ull ^ (jsb_hal_millis() + 1) ^ (uintptr_t)&state;
    }
    uint8_t *out = buf;
    while (len > 0) {
        state ^= state >> 12;
        state ^= state << 25;
        state ^= state >> 27;
        uint64_t r = state * 0x2545F4914F6CDD1Dull;
        size_t n = len < sizeof r ? len : sizeof r;
        memcpy(out, &r, n);
        out += n;
        len -= n;
    }
    return 0;
}

/* A failed assertion inside Ant: save a crash record (CRASH.TXT) and restart. */
_Noreturn void ant_embed_fatal(const char *msg, const char *file, int line)
{
    char buf[96];
    snprintf(buf, sizeof buf, "ant: internal error: %s", msg != NULL ? msg : "");
    jsb_hal_fatal(buf, file, line);
}

/* --- helpers --- */

static void console_print(const char *s)
{
    jsb_hal_console_write(s, strlen(s));
}

static void set_stack(ant_t *js)
{
    volatile char base;
    js_setstackbase(js, (void *)&base);
    js_setstacklimit(js, JSB_ANT_STACK_SIZE);
}

/* Prints a pending uncaught exception; returns -1 if there was one. */
static int report(ant_t *js, ant_value_t result)
{
    if (is_err(result) || print_uncaught_throw(js)) {
        if (is_err(result)) {
            print_error_value(js, result, js_mkundef(), "Uncaught");
        }
        js_take_thrown(js, js_mkundef());
        return -1;
    }
    return 0;
}

static ant_value_t to_js(ant_t *js, const jsb_value *v)
{
    switch (v->type) {
    case JSB_BOOL:
        return js_bool(v->u.boolean);
    case JSB_NUMBER:
        return js_mknum(v->u.number);
    case JSB_STRING:
        return js_mkstr(js, v->u.string, strlen(v->u.string));
    case JSB_FUNCTION: {
        ant_value_t funcs = js_get(js, js_glob(js), FUNCS_GLOBAL);
        char key[12];
        snprintf(key, sizeof key, "%d", v->u.func);
        return js_get(js, funcs, key);
    }
    default:
        return js_mkundef();
    }
}

static ant_value_t host_trampoline(ant_params_t)
{
    (void)call_new_target;
    ant_value_t data = js_get_slot(js_as_obj(js_getcurrentfunc(js)), SLOT_DATA);
    int index = (int)tod(data);
    if (index < 0 || index >= host_fn_count) {
        return js_mkerr_typed(js, JS_ERR_INTERNAL, "invalid host function");
    }
    struct jsb_call call = { js, args, nargs, js_mkundef(), js_mkundef() };
    if (host_fns[index](&call) < 0) {
        return is_err(call.err) ? call.err : js_mkerr_typed(js, JS_ERR_GENERIC, "host function failed");
    }
    return call.ret;
}

/* --- runtime --- */

const char *jsb_engine_name(void)
{
    return "ant";
}

void jsb_trace(const char *what); /* TEMP-TRACE */
#define T(x) jsb_trace(x)

jsb_engine *jsb_engine_create(void)
{
    if (the_engine.js) {
        return NULL;
    }
    ant_t *js = ant_create(nullptr);
    T("ant: ant_create");
    if (!js) {
        console_print("JSBedded: ant: out of memory creating the runtime\r\n");
        return NULL;
    }
    set_stack(js);
    js->uncaught_nonfatal = true;
    ant_embed_microtasks_reset(js);
    ant_runtime_init(js, 0, NULL, NULL);
    T("ant: runtime_init");
    /* The built-ins the embed source list has, in the reactor's order. */
    init_symbol_module(js);
    init_intrinsic_symbols(js);
    T("ant: symbols");
    init_iterator_module(js);
    init_generator_module(js);
    init_math_module(js);
    init_bigint_module(js);
    init_regex_module(js);
    init_collections_module(js);
    init_json_module(js);
    T("ant: modules");

    js_set(js, js_glob(js), FUNCS_GLOBAL, Ant_Object_Create(js));
    memset(func_used, 0, sizeof func_used);
    host_fn_count = 0;
    the_engine.js = js;
    return &the_engine;
}

int jsb_engine_eval(jsb_engine *engine, const char *source, size_t len, const char *filename)
{
    (void)filename;
    ant_t *js = engine->js;
    set_stack(js);
    T("ant: eval start");
    ant_value_t result = js_eval_bytecode_eval(js, source, len);
    T("ant: eval returned");
    if (report(js, result) < 0) {
        return -1;
    }
    process_microtasks(js);
    return report(js, js_mkundef());
}

void jsb_engine_destroy(jsb_engine *engine)
{
    if (!engine || !engine->js) {
        return;
    }
    ant_embed_microtasks_reset(engine->js);
    js_destroy(engine->js);
    engine->js = NULL;
}

int jsb_engine_run_jobs(jsb_engine *engine)
{
    ant_t *js = engine->js;
    set_stack(js);
    process_microtasks(js);
    return report(js, js_mkundef()) < 0 ? -1 : 0;
}

/* --- host functions --- */

int jsb_engine_define(jsb_engine *engine, const char *object, const jsb_host_def *defs, size_t count)
{
    ant_t *js = engine->js;
    GC_ROOT_SAVE(mark, js);
    ant_value_t target = js_glob(js);
    if (object) {
        target = Ant_Object_Create(js);
        GC_ROOT_PIN(js, target);
    }
    for (size_t i = 0; i < count; i++) {
        ant_value_t v;
        if (defs[i].fn) {
            if (host_fn_count >= MAX_HOST_FNS) {
                console_print("JSBedded: ant: too many host functions\r\n");
                GC_ROOT_RESTORE(js, mark);
                return -1;
            }
            host_fns[host_fn_count] = defs[i].fn;
            v = js_heavy_mkfun(js, host_trampoline, js_mknum((double)host_fn_count));
            host_fn_count++;
        } else {
            v = to_js(js, &defs[i].value);
        }
        if (is_err(v)) {
            console_print("JSBedded: ant: out of memory defining host functions\r\n");
            GC_ROOT_RESTORE(js, mark);
            return -1;
        }
        js_set(js, target, defs[i].name, v);
    }
    if (object) {
        js_set(js, js_glob(js), object, target);
    }
    GC_ROOT_RESTORE(js, mark);
    return 0;
}

int jsb_engine_call(jsb_engine *engine, jsb_func func, int argc, const jsb_value *argv)
{
    ant_t *js = engine->js;
    if (func < 0 || func >= MAX_FUNCS || !func_used[func] || argc > 8) {
        return -1;
    }
    set_stack(js);
    GC_ROOT_SAVE(mark, js);
    jsb_value fv = { JSB_FUNCTION, { .func = func } };
    ant_value_t fn = to_js(js, &fv);
    GC_ROOT_PIN(js, fn);
    ant_value_t args[8];
    for (int i = 0; i < argc; i++) {
        args[i] = to_js(js, &argv[i]);
        GC_ROOT_PIN(js, args[i]);
    }
    ant_value_t result = sv_call_value(js, fn, js_mkundef(), args, argc);
    GC_ROOT_RESTORE(js, mark);
    return report(js, result);
}

void jsb_engine_release(jsb_engine *engine, jsb_func func)
{
    if (func < 0 || func >= MAX_FUNCS || !func_used[func]) {
        return;
    }
    ant_t *js = engine->js;
    char key[12];
    snprintf(key, sizeof key, "%d", func);
    js_set(js, js_get(js, js_glob(js), FUNCS_GLOBAL), key, js_mkundef());
    func_used[func] = false;
}

/* --- inside a host function --- */

int jsb_argc(jsb_call *call)
{
    return call->nargs;
}

jsb_type jsb_arg_type(jsb_call *call, int i)
{
    if (i >= call->nargs) {
        return JSB_UNDEFINED;
    }
    ant_value_t v = call->args[i];
    switch (vtype(v)) {
    case kTypeUndefined:
        return JSB_UNDEFINED;
    case kTypeBool:
        return JSB_BOOL;
    case kTypeNumber:
        return JSB_NUMBER;
    case kTypeString:
        return JSB_STRING;
    default:
        return is_callable(v) ? JSB_FUNCTION : JSB_OTHER;
    }
}

static ant_value_t arg(jsb_call *call, int i)
{
    return i < call->nargs ? call->args[i] : js_mkundef();
}

int jsb_arg_number(jsb_call *call, int i, double *out)
{
    *out = js_to_number(call->js, arg(call, i));
    if (vtype(call->js->exception) != kTypeUndefined) {
        call->err = js_take_thrown(call->js, js_mkundef());
        return -1;
    }
    return 0;
}

int jsb_arg_string(jsb_call *call, int i, char *buf, size_t size)
{
    ant_value_t s = js_tostring_val(call->js, arg(call, i));
    if (is_err(s)) {
        call->err = s;
        return -1;
    }
    size_t len = 0;
    const char *p = js_getstr(call->js, s, &len);
    if (!p) {
        return jsb_throw(call, JSB_TYPE_ERROR, "cannot convert to string");
    }
    if (size == 0) {
        return 0;
    }
    if (len > size - 1) {
        len = size - 1;
    }
    memcpy(buf, p, len);
    buf[len] = '\0';
    return (int)len;
}

jsb_func jsb_arg_function(jsb_call *call, int i)
{
    ant_value_t fn = arg(call, i);
    if (!is_callable(fn)) {
        jsb_throw(call, JSB_TYPE_ERROR, "argument is not a function");
        return JSB_FUNC_NONE;
    }
    for (int f = 0; f < MAX_FUNCS; f++) {
        if (!func_used[f]) {
            char key[12];
            snprintf(key, sizeof key, "%d", f);
            js_set(call->js, js_get(call->js, js_glob(call->js), FUNCS_GLOBAL), key, fn);
            func_used[f] = true;
            return f;
        }
    }
    jsb_throw(call, JSB_RANGE_ERROR, "too many retained functions");
    return JSB_FUNC_NONE;
}

int jsb_arg_log(jsb_call *call, int i)
{
    char buf[256];
    int n = jsb_arg_string(call, i, buf, sizeof buf);
    if (n < 0) {
        return -1;
    }
    jsb_hal_console_write(buf, (size_t)n);
    return 0;
}

void jsb_return(jsb_call *call, jsb_value value)
{
    call->ret = to_js(call->js, &value);
}

int jsb_throw(jsb_call *call, jsb_error kind, const char *message)
{
    int type = kind == JSB_TYPE_ERROR ? JS_ERR_TYPE : kind == JSB_RANGE_ERROR ? JS_ERR_RANGE : JS_ERR_GENERIC;
    call->err = js_mkerr_typed(call->js, type, "%s", message);
    return -1;
}
