/*
 * QuickJS engine adapter. Uses the QuickJS core only (no quickjs-libc, which
 * needs an OS). JSB_QUICKJS_MEMORY_LIMIT and JSB_QUICKJS_STACK_SIZE come from
 * engine.cmake.
 *
 * Host functions (engine.h): every host function is a C function with magic,
 * host_trampoline(), whose magic indexes engine->host_fns[]. console, timers,
 * gpio and board all come from core/ and api/ this way. Retained JS functions
 * (timer callbacks) live in a fixed pool of duplicated JSValues.
 */
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "jsb/engine.h"
#include "jsb/hal.h"
#include "quickjs/quickjs.h"

/* Host functions defined with jsb_engine_define(), per runtime. */
#define MAX_HOST_FNS 32
/* JS functions retained with jsb_arg_function() (timers, callbacks). */
#define MAX_FUNCS 32
/* Arguments jsb_engine_call() can pass. */
#define MAX_CALL_ARGS 8

struct jsb_engine {
    JSRuntime *rt;
    JSContext *ctx;
    size_t heap_used; /* mirror of the runtime's malloc_size, for the GC threshold */
    jsb_host_fn host_fns[MAX_HOST_FNS];
    int host_fn_count;
    JSValue funcs[MAX_FUNCS];
    bool func_used[MAX_FUNCS];
};

struct jsb_call {
    JSContext *ctx;
    int argc;
    JSValueConst *argv;
    JSValue ret; /* owned; undefined until jsb_return() */
};

static void console_print(const char *s)
{
    jsb_hal_console_write(s, strlen(s));
}

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

/* Write text, turning "\n" into "\r\n" and ending with a line break. */
static void console_print_lines(const char *s)
{
    const char *nl;

    while ((nl = strchr(s, '\n')) != NULL) {
        jsb_hal_console_write(s, (size_t)(nl - s));
        jsb_hal_console_write("\r\n", 2);
        s = nl + 1;
    }
    if (*s != '\0') {
        console_print(s);
        jsb_hal_console_write("\r\n", 2);
    }
}

/*
 * Allocator: libc malloc with a size header, so the memory limit is enforced
 * without malloc_usable_size(), which not every embedded libc has. QuickJS
 * itself packs small objects into 4 KB arenas taken from here.
 */
typedef union {
    size_t size;
    double align; /* keep the user block as aligned as malloc's */
} alloc_header;

static size_t available(const JSMallocState *s)
{
    return s->malloc_size < s->malloc_limit ? s->malloc_limit - s->malloc_size : 0;
}

static void track(JSMallocState *s)
{
    jsb_engine *engine = s->opaque;
    engine->heap_used = s->malloc_size;
}

static void *qjs_malloc(JSMallocState *s, size_t size)
{
    alloc_header *h;

    if (available(s) < sizeof *h || size > available(s) - sizeof *h) {
        return NULL;
    }
    h = malloc(sizeof *h + size);
    if (h == NULL) {
        return NULL;
    }
    h->size = size;
    s->malloc_count++;
    s->malloc_size += sizeof *h + size;
    track(s);
    return h + 1;
}

static void qjs_free(JSMallocState *s, void *ptr)
{
    alloc_header *h;

    if (ptr == NULL) {
        return;
    }
    h = (alloc_header *)ptr - 1;
    s->malloc_count--;
    s->malloc_size -= sizeof *h + h->size;
    track(s);
    free(h);
}

static void *qjs_realloc(JSMallocState *s, void *ptr, size_t size)
{
    alloc_header *h;
    size_t old_size;

    if (ptr == NULL) {
        return size == 0 ? NULL : qjs_malloc(s, size);
    }
    if (size == 0) {
        qjs_free(s, ptr);
        return NULL;
    }
    h = (alloc_header *)ptr - 1;
    old_size = h->size;
    if (size > old_size && size - old_size > available(s)) {
        return NULL;
    }
    h = realloc(h, sizeof *h + size);
    if (h == NULL) {
        return NULL;
    }
    h->size = size;
    s->malloc_size = s->malloc_size - old_size + size;
    track(s);
    return h + 1;
}

static size_t qjs_malloc_usable_size(const void *ptr)
{
    return ptr == NULL ? 0 : ((const alloc_header *)ptr - 1)->size;
}

static const JSMallocFunctions malloc_functions = {
    qjs_malloc,
    qjs_free,
    qjs_realloc,
    qjs_malloc_usable_size,
};


/* Print the pending exception: "Uncaught <Error>: <message>", then the stack
   lines QuickJS records ("    at <function> (file:line:column)"). */
static void print_value_lines(JSContext *ctx, JSValueConst value, const char *fallback)
{
    const char *s = JS_ToCString(ctx, value);

    if (s != NULL) {
        console_print_lines(s);
        JS_FreeCString(ctx, s);
    } else {
        JS_FreeValue(ctx, JS_GetException(ctx)); /* conversion threw: drop it */
        console_print_lines(fallback);
    }
}

static void print_exception(JSContext *ctx)
{
    JSValue exception = JS_GetException(ctx);

    console_print("Uncaught ");
    print_value_lines(ctx, exception, "exception");
    if (JS_IsError(ctx, exception)) {
        JSValue stack = JS_GetPropertyStr(ctx, exception, "stack");
        if (JS_IsException(stack)) {
            JS_FreeValue(ctx, JS_GetException(ctx));
        } else if (!JS_IsUndefined(stack)) {
            print_value_lines(ctx, stack, "    at <unknown>");
        }
        JS_FreeValue(ctx, stack);
    }
    JS_FreeValue(ctx, exception);
}

static bool func_valid(const jsb_engine *engine, jsb_func func)
{
    return func >= 0 && func < MAX_FUNCS && engine->func_used[func];
}

/* New reference; JS_EXCEPTION when out of memory (strings, numbers). */
static JSValue to_js(jsb_engine *engine, JSContext *ctx, const jsb_value *v)
{
    switch (v->type) {
    case JSB_BOOL:
        return JS_NewBool(ctx, v->u.boolean);
    case JSB_NUMBER:
        return JS_NewFloat64(ctx, v->u.number);
    case JSB_STRING:
        return v->u.string != NULL ? JS_NewString(ctx, v->u.string) : JS_UNDEFINED;
    case JSB_FUNCTION:
        return func_valid(engine, v->u.func) ? JS_DupValue(ctx, engine->funcs[v->u.func])
                                             : JS_UNDEFINED;
    default:
        return JS_UNDEFINED;
    }
}

/* Every host function: magic is its index in engine->host_fns[]. */
static JSValue host_trampoline(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv,
                               int magic)
{
    jsb_engine *engine = JS_GetContextOpaque(ctx);
    jsb_call call;

    (void)this_val;
    if (engine == NULL || magic < 0 || magic >= engine->host_fn_count) {
        return JS_ThrowInternalError(ctx, "unknown host function");
    }
    call.ctx = ctx;
    call.argc = argc;
    call.argv = argv;
    call.ret = JS_UNDEFINED;
    if (engine->host_fns[magic](&call) != 0) {
        JS_FreeValue(ctx, call.ret);
        if (!JS_HasException(ctx)) {
            JS_ThrowInternalError(ctx, "host function failed");
        }
        return JS_EXCEPTION;
    }
    return call.ret;
}

/* --- engine.h: runtime --- */

const char *jsb_engine_name(void)
{
    return "quickjs";
}

jsb_engine *jsb_engine_create(void)
{
    jsb_engine *engine = calloc(1, sizeof *engine);

    if (engine == NULL) {
        return NULL;
    }
    engine->rt = JS_NewRuntime2(&malloc_functions, engine);
    if (engine->rt == NULL) {
        goto fail;
    }
    JS_SetMemoryLimit(engine->rt, JSB_QUICKJS_MEMORY_LIMIT);
    JS_SetMaxStackSize(engine->rt, JSB_QUICKJS_STACK_SIZE);
    /* Keep line numbers for errors, drop the source text kept for Function.prototype.toString. */
    JS_SetStripInfo(engine->rt, JS_STRIP_SOURCE);

    engine->ctx = JS_NewContext(engine->rt);
    if (engine->ctx == NULL) {
        goto fail;
    }
    JS_SetContextOpaque(engine->ctx, engine);

    /* The default first GC runs at 256 KB, at or above the limit on these
       boards: collect cycles halfway between the empty context and the limit.
       QuickJS raises the threshold to 1.5x the live heap after each GC. */
    if (engine->heap_used < JSB_QUICKJS_MEMORY_LIMIT) {
        JS_SetGCThreshold(engine->rt, engine->heap_used +
                                      (JSB_QUICKJS_MEMORY_LIMIT - engine->heap_used) / 2);
    }
    return engine;

fail:
    console_print("JSBedded: QuickJS could not start (out of memory?)\r\n");
    jsb_engine_destroy(engine);
    return NULL;
}

/* Promise jobs the script queues run in jsb_engine_run_jobs(), which the
   firmware calls right after this and on every event-loop pass. */
int jsb_engine_eval(jsb_engine *engine, const char *source, size_t len, const char *filename)
{
    JSValue result;
    char *buf;
    int ret = 0;

    if (engine == NULL) {
        return -1;
    }
    /* JS_Eval() needs a NUL after the source. */
    buf = malloc(len + 1);
    if (buf == NULL) {
        console_print("JSBedded: out of memory for the script\r\n");
        return -1;
    }
    memcpy(buf, source, len);
    buf[len] = '\0';

    /* Measure the stack limit from here, not from where the runtime was created. */
    JS_UpdateStackTop(engine->rt);
    result = JS_Eval(engine->ctx, buf, len, filename != NULL ? filename : "<eval>",
                     JS_EVAL_TYPE_GLOBAL);
    free(buf);
    if (JS_IsException(result)) {
        print_exception(engine->ctx);
        ret = -1;
    }
    JS_FreeValue(engine->ctx, result);
    return ret;
}

void jsb_engine_destroy(jsb_engine *engine)
{
    if (engine == NULL) {
        return;
    }
    if (engine->ctx != NULL) {
        /* Retained functions first: JS_FreeRuntime() expects no live objects. */
        for (int k = 0; k < MAX_FUNCS; k++) {
            if (engine->func_used[k]) {
                JS_FreeValue(engine->ctx, engine->funcs[k]);
                engine->func_used[k] = false;
            }
        }
        JS_FreeContext(engine->ctx);
    }
    if (engine->rt != NULL) {
        JS_FreeRuntime(engine->rt);
    }
    free(engine);
}

int jsb_engine_run_jobs(jsb_engine *engine)
{
    int count = 0;
    bool failed = false;

    if (engine == NULL) {
        return 0;
    }
    JS_UpdateStackTop(engine->rt);
    for (;;) {
        JSContext *ctx;
        int r = JS_ExecutePendingJob(engine->rt, &ctx);
        if (r == 0) {
            break;
        }
        count++;
        if (r < 0) {
            print_exception(ctx != NULL ? ctx : engine->ctx);
            failed = true;
        }
    }
    return failed ? -1 : count;
}

/* --- engine.h: host functions --- */

int jsb_engine_define(jsb_engine *engine, const char *object, const jsb_host_def *defs, size_t count)
{
    JSContext *ctx = engine->ctx;
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue obj;
    const char *failed = object;
    int rc = -1;

    if (object != NULL) {
        obj = JS_NewObject(ctx);
        if (JS_IsException(obj)) {
            goto done_noobj;
        }
        /* JS_SetPropertyStr() takes one reference; obj keeps ours. */
        if (JS_SetPropertyStr(ctx, global, object, JS_DupValue(ctx, obj)) < 0) {
            goto done;
        }
    } else {
        obj = JS_DupValue(ctx, global);
    }

    for (size_t i = 0; i < count; i++) {
        const jsb_host_def *def = &defs[i];
        JSValue val;
        failed = def->name;
        if (def->fn != NULL) {
            if (engine->host_fn_count >= MAX_HOST_FNS) {
                console_print("JSBedded: quickjs: too many host functions, can't define ");
                console_print(def->name);
                console_print("\r\n");
                JS_FreeValue(ctx, obj);
                JS_FreeValue(ctx, global);
                return -1;
            }
            engine->host_fns[engine->host_fn_count] = def->fn;
            val = JS_NewCFunctionMagic(ctx, host_trampoline, def->name, 0, JS_CFUNC_generic_magic,
                                       engine->host_fn_count);
            if (!JS_IsException(val)) {
                engine->host_fn_count++;
            }
        } else {
            val = to_js(engine, ctx, &def->value);
        }
        if (JS_IsException(val) || JS_SetPropertyStr(ctx, obj, def->name, val) < 0) {
            goto done;
        }
    }
    rc = 0;

done:
    JS_FreeValue(ctx, obj);
done_noobj:
    JS_FreeValue(ctx, global);
    if (rc != 0) {
        console_print("JSBedded: can't define ");
        console_print(failed != NULL ? failed : "?");
        console_print(": ");
        print_exception(ctx);
    }
    return rc;
}

int jsb_engine_call(jsb_engine *engine, jsb_func func, int argc, const jsb_value *argv)
{
    JSContext *ctx;
    JSValue args[MAX_CALL_ARGS];
    JSValue fn, result;
    int ret = 0;

    if (engine == NULL || !func_valid(engine, func) || argc < 0 || argc > MAX_CALL_ARGS) {
        return -1;
    }
    ctx = engine->ctx;
    for (int i = 0; i < argc; i++) {
        args[i] = to_js(engine, ctx, &argv[i]);
        if (JS_IsException(args[i])) {
            JS_FreeValue(ctx, JS_GetException(ctx));
            args[i] = JS_UNDEFINED;
        }
    }
    /* Hold the function during the call: the callback may release its own
       handle (clearInterval() inside the interval). */
    fn = JS_DupValue(ctx, engine->funcs[func]);
    JS_UpdateStackTop(engine->rt);
    result = JS_Call(ctx, fn, JS_UNDEFINED, argc, args);
    if (JS_IsException(result)) {
        print_exception(ctx);
        ret = -1;
    }
    JS_FreeValue(ctx, result);
    JS_FreeValue(ctx, fn);
    for (int i = 0; i < argc; i++) {
        JS_FreeValue(ctx, args[i]);
    }
    return ret;
}

void jsb_engine_release(jsb_engine *engine, jsb_func func)
{
    if (engine == NULL || engine->ctx == NULL || !func_valid(engine, func)) {
        return;
    }
    JS_FreeValue(engine->ctx, engine->funcs[func]);
    engine->funcs[func] = JS_UNDEFINED;
    engine->func_used[func] = false;
}

/* --- engine.h: inside a host function --- */

static JSValueConst arg(jsb_call *call, int i)
{
    return i >= 0 && i < call->argc ? call->argv[i] : JS_UNDEFINED;
}

int jsb_argc(jsb_call *call)
{
    return call->argc;
}

jsb_type jsb_arg_type(jsb_call *call, int i)
{
    JSValueConst v = arg(call, i);
    if (JS_IsUndefined(v)) {
        return JSB_UNDEFINED;
    }
    if (JS_IsBool(v)) {
        return JSB_BOOL;
    }
    if (JS_IsNumber(v)) {
        return JSB_NUMBER;
    }
    if (JS_IsString(v)) {
        return JSB_STRING;
    }
    if (JS_IsFunction(call->ctx, v)) {
        return JSB_FUNCTION;
    }
    return JSB_OTHER;
}

int jsb_arg_number(jsb_call *call, int i, double *out)
{
    return JS_ToFloat64(call->ctx, out, arg(call, i)) != 0 ? -1 : 0;
}

int jsb_arg_string(jsb_call *call, int i, char *buf, size_t size)
{
    size_t len;
    const char *s = JS_ToCStringLen(call->ctx, &len, arg(call, i));
    if (s == NULL) {
        return -1;
    }
    if (size == 0) {
        JS_FreeCString(call->ctx, s);
        return 0;
    }
    if (len > size - 1) {
        len = size - 1;
    }
    memcpy(buf, s, len);
    buf[len] = '\0';
    JS_FreeCString(call->ctx, s);
    return (int)len;
}

jsb_func jsb_arg_function(jsb_call *call, int i)
{
    JSContext *ctx = call->ctx;
    jsb_engine *engine = JS_GetContextOpaque(ctx);
    JSValueConst v = arg(call, i);

    if (!JS_IsFunction(ctx, v)) {
        JS_ThrowTypeError(ctx, "argument %d is not a function", i + 1);
        return JSB_FUNC_NONE;
    }
    for (int k = 0; k < MAX_FUNCS; k++) {
        if (!engine->func_used[k]) {
            engine->funcs[k] = JS_DupValue(ctx, v);
            engine->func_used[k] = true;
            return k;
        }
    }
    JS_ThrowInternalError(ctx, "too many pending callbacks (max %d)", MAX_FUNCS);
    return JSB_FUNC_NONE;
}

static void print_value_write(void *opaque, const char *buf, size_t len)
{
    (void)opaque;
    console_write(buf, len);
}

/* Strings as-is, other values as QuickJS inspects them ({ a: 1 }, [ 1, 2 ]). */
int jsb_arg_log(jsb_call *call, int i)
{
    JSContext *ctx = call->ctx;
    JSValueConst v = arg(call, i);

    if (JS_IsString(v)) {
        size_t len;
        const char *s = JS_ToCStringLen(ctx, &len, v);
        if (s == NULL) {
            return -1;
        }
        console_write(s, len);
        JS_FreeCString(ctx, s);
    } else {
        JSPrintValueOptions opts;
        JS_PrintValueSetDefaultOptions(&opts);
        opts.max_depth = 4;
        opts.max_item_count = 32;
        JS_PrintValue(ctx, print_value_write, NULL, v, &opts);
    }
    return 0;
}

void jsb_return(jsb_call *call, jsb_value value)
{
    JSContext *ctx = call->ctx;
    JSValue v = to_js(JS_GetContextOpaque(ctx), ctx, &value);

    if (JS_IsException(v)) {
        JS_FreeValue(ctx, JS_GetException(ctx));
        v = JS_UNDEFINED;
    }
    JS_FreeValue(ctx, call->ret);
    call->ret = v;
}

/* A plain Error goes through the global constructor, so it gets a stack. */
static void throw_plain_error(JSContext *ctx, const char *message)
{
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue ctor = JS_GetPropertyStr(ctx, global, "Error");
    JSValue msg = JS_NewString(ctx, message);
    JSValue err = JS_EXCEPTION;

    if (!JS_IsException(ctor) && !JS_IsException(msg)) {
        err = JS_CallConstructor(ctx, ctor, 1, (JSValueConst *)&msg);
    }
    if (!JS_IsException(err)) {
        JS_Throw(ctx, err);
    } else if (!JS_HasException(ctx)) {
        JS_ThrowInternalError(ctx, "%s", message);
    }
    JS_FreeValue(ctx, msg);
    JS_FreeValue(ctx, ctor);
    JS_FreeValue(ctx, global);
}

int jsb_throw(jsb_call *call, jsb_error kind, const char *message)
{
    switch (kind) {
    case JSB_TYPE_ERROR:
        JS_ThrowTypeError(call->ctx, "%s", message);
        break;
    case JSB_RANGE_ERROR:
        JS_ThrowRangeError(call->ctx, "%s", message);
        break;
    default:
        throw_plain_error(call->ctx, message);
        break;
    }
    return -1;
}
