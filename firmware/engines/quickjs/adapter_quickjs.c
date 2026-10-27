/*
 * QuickJS engine adapter. Uses the QuickJS core only (no quickjs-libc, which
 * needs an OS): console.log is installed here and writes through the HAL.
 * JSB_QUICKJS_MEMORY_LIMIT and JSB_QUICKJS_STACK_SIZE come from engine.cmake.
 */
#include <stdlib.h>
#include <string.h>

#include "jsb/engine.h"
#include "jsb/hal.h"
#include "quickjs/quickjs.h"

struct jsb_engine {
    JSRuntime *rt;
    JSContext *ctx;
    size_t heap_used; /* mirror of the runtime's malloc_size, for the GC threshold */
};

static void console_print(const char *s)
{
    jsb_hal_console_write(s, strlen(s));
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

/* console.log(...args): arguments as strings, separated by spaces. */
static JSValue js_console_log(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    (void)this_val;
    for (int i = 0; i < argc; i++) {
        size_t len;
        const char *s = JS_ToCStringLen(ctx, &len, argv[i]);
        if (s == NULL) {
            return JS_EXCEPTION;
        }
        if (i > 0) {
            jsb_hal_console_write(" ", 1);
        }
        jsb_hal_console_write(s, len);
        JS_FreeCString(ctx, s);
    }
    jsb_hal_console_write("\r\n", 2);
    return JS_UNDEFINED;
}

static int install_console(JSContext *ctx)
{
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue console = JS_NewObject(ctx);
    int ret = -1;

    if (!JS_IsException(console) &&
        JS_SetPropertyStr(ctx, console, "log",
                          JS_NewCFunction(ctx, js_console_log, "log", 1)) >= 0) {
        ret = JS_SetPropertyStr(ctx, global, "console", console) < 0 ? -1 : 0;
    } else {
        JS_FreeValue(ctx, console);
    }
    JS_FreeValue(ctx, global);
    return ret;
}

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
    if (engine->ctx == NULL || install_console(engine->ctx) < 0) {
        goto fail;
    }

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

int jsb_engine_eval(jsb_engine *engine, const char *source, size_t len, const char *filename)
{
    JSContext *ctx;
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

    /* Run the promise jobs the script queued. */
    for (;;) {
        int r = JS_ExecutePendingJob(engine->rt, &ctx);
        if (r == 0) {
            break;
        }
        if (r < 0) {
            print_exception(ctx != NULL ? ctx : engine->ctx);
            ret = -1;
        }
    }
    return ret;
}

void jsb_engine_destroy(jsb_engine *engine)
{
    if (engine == NULL) {
        return;
    }
    if (engine->ctx != NULL) {
        JS_FreeContext(engine->ctx);
    }
    if (engine->rt != NULL) {
        JS_FreeRuntime(engine->rt);
    }
    free(engine);
}
