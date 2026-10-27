/*
 * Duktape engine adapter. Duktape has no console of its own: console.log is
 * installed here and writes through the HAL. JSB_DUKTAPE_HEAP_LIMIT and
 * JSB_DUKTAPE_STACK_LIMIT come from engine.cmake; the duk_config.h overrides
 * (low-memory options, stack check and fatal handler hooks) are in
 * duk_config_jsb.h.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "duktape.h"

#include "jsb/engine.h"
#include "jsb/hal.h"

struct jsb_engine {
    duk_context *ctx;
    size_t heap_used; /* bytes held through the allocator, headers included */
};

/* Stack position at jsb_engine_eval(); 0 outside it (no check). */
static uintptr_t stack_base;

static void console_print(const char *s)
{
    jsb_hal_console_write(s, strlen(s));
}

/* Write text, turning "\n" into "\r\n" and ending with a line break. */
static void console_print_lines(const char *s, size_t len)
{
    size_t start = 0;

    for (size_t i = 0; i < len; i++) {
        if (s[i] == '\n') {
            jsb_hal_console_write(s + start, i - start);
            jsb_hal_console_write("\r\n", 2);
            start = i + 1;
        }
    }
    jsb_hal_console_write(s + start, len - start);
    jsb_hal_console_write("\r\n", 2);
}

/* --- hooks named in duk_config_jsb.h --- */

/* Non-zero once calls have used more than JSB_DUKTAPE_STACK_LIMIT bytes of C
   stack below jsb_engine_eval(); Duktape then throws RangeError. */
int jsb_duk_native_stack_check(void)
{
    if (stack_base == 0) {
        return 0;
    }
    return stack_base - (uintptr_t)__builtin_frame_address(0) > JSB_DUKTAPE_STACK_LIMIT;
}

/* Fatal errors (uncaught outside a protected call, out of memory in the
   middle of an operation, assertions). Duktape requires that this does not return. */
void jsb_duk_fatal(void *udata, const char *msg)
{
    (void)udata;
    console_print("JSBedded: Duktape fatal error: ");
    console_print(msg != NULL ? msg : "(no message)");
    console_print("\r\n");
    abort();
}

/*
 * Allocator: libc malloc with a size header, so the heap limit is enforced
 * without malloc_usable_size(), which not every embedded libc has. When an
 * allocation fails Duktape runs an emergency GC and retries before it throws.
 */
typedef union {
    size_t size;
    double align; /* keep the user block as aligned as malloc's */
} alloc_header;

static size_t available(const jsb_engine *engine)
{
    return engine->heap_used < JSB_DUKTAPE_HEAP_LIMIT ? JSB_DUKTAPE_HEAP_LIMIT - engine->heap_used : 0;
}

static void *duk_alloc_cb(void *udata, duk_size_t size)
{
    jsb_engine *engine = udata;
    alloc_header *h;

    if (available(engine) < sizeof *h || size > available(engine) - sizeof *h) {
        return NULL;
    }
    h = malloc(sizeof *h + size);
    if (h == NULL) {
        return NULL;
    }
    h->size = size;
    engine->heap_used += sizeof *h + size;
    return h + 1;
}

static void duk_free_cb(void *udata, void *ptr)
{
    jsb_engine *engine = udata;
    alloc_header *h;

    if (ptr == NULL) {
        return;
    }
    h = (alloc_header *)ptr - 1;
    engine->heap_used -= sizeof *h + h->size;
    free(h);
}

static void *duk_realloc_cb(void *udata, void *ptr, duk_size_t size)
{
    jsb_engine *engine = udata;
    alloc_header *h;
    size_t old_size;

    if (ptr == NULL) {
        return duk_alloc_cb(udata, size);
    }
    if (size == 0) {
        duk_free_cb(udata, ptr);
        return NULL;
    }
    h = (alloc_header *)ptr - 1;
    old_size = h->size;
    if (size > old_size && size - old_size > available(engine)) {
        return NULL;
    }
    h = realloc(h, sizeof *h + size);
    if (h == NULL) {
        return NULL;
    }
    h->size = size;
    engine->heap_used = engine->heap_used - old_size + size;
    return h + 1;
}

/* console.log(...args): arguments as strings, separated by spaces. */
static duk_ret_t js_console_log(duk_context *ctx)
{
    duk_idx_t argc = duk_get_top(ctx);

    for (duk_idx_t i = 0; i < argc; i++) {
        duk_size_t len;
        const char *s = duk_to_lstring(ctx, i, &len);
        if (i > 0) {
            jsb_hal_console_write(" ", 1);
        }
        jsb_hal_console_write(s, len);
    }
    jsb_hal_console_write("\r\n", 2);
    return 0;
}

static duk_ret_t install_console(duk_context *ctx, void *udata)
{
    (void)udata;
    duk_push_global_object(ctx);
    duk_push_object(ctx);
    duk_push_c_function(ctx, js_console_log, DUK_VARARGS);
    duk_put_prop_string(ctx, -2, "log");
    duk_put_prop_string(ctx, -2, "console");
    return 0;
}

/* [ error ] -> prints "Uncaught <Name>: <message>" and, when Duktape recorded
   them, "    at <file>:<line>". Runs in duk_safe_call(): getters and
   toString() may throw. */
static duk_ret_t print_error(duk_context *ctx, void *udata)
{
    duk_size_t len;
    const char *s;

    (void)udata;
    if (duk_is_error(ctx, 0)) {
        duk_get_prop_string(ctx, 0, "fileName");
        duk_get_prop_string(ctx, 0, "lineNumber");
    } else {
        duk_push_undefined(ctx);
        duk_push_undefined(ctx);
    }
    duk_dup(ctx, 0);
    s = duk_to_lstring(ctx, -1, &len);

    console_print("Uncaught ");
    console_print_lines(s, len);
    if (duk_is_string(ctx, 1) && duk_is_number(ctx, 2)) {
        char line[24];
        int n = snprintf(line, sizeof line, ":%lu\r\n", (unsigned long)duk_get_uint(ctx, 2));
        console_print("    at ");
        console_print(duk_get_string(ctx, 1));
        if (n > 0) {
            jsb_hal_console_write(line, (size_t)n < sizeof line ? (size_t)n : sizeof line - 1);
        }
    }
    return 0;
}

struct eval_args {
    const char *source;
    size_t len;
    const char *filename;
};

/* Compile the source as a global program and run it. */
static duk_ret_t eval_script(duk_context *ctx, void *udata)
{
    const struct eval_args *args = udata;

    duk_push_string(ctx, args->filename);
    duk_compile_lstring_filename(ctx, 0, args->source, args->len);
    duk_call(ctx, 0);
    return 0;
}

const char *jsb_engine_name(void)
{
    return "duktape";
}

jsb_engine *jsb_engine_create(void)
{
    jsb_engine *engine = calloc(1, sizeof *engine);

    if (engine == NULL) {
        return NULL;
    }
    engine->ctx = duk_create_heap(duk_alloc_cb, duk_realloc_cb, duk_free_cb, engine, jsb_duk_fatal);
    if (engine->ctx == NULL || duk_safe_call(engine->ctx, install_console, NULL, 0, 1) != DUK_EXEC_SUCCESS) {
        console_print("JSBedded: Duktape could not start (out of memory?)\r\n");
        jsb_engine_destroy(engine);
        return NULL;
    }
    duk_pop(engine->ctx);
    return engine;
}

/* Duktape compiles from (source, len) directly: no NUL terminator needed. */
int jsb_engine_eval(jsb_engine *engine, const char *source, size_t len, const char *filename)
{
    struct eval_args args = { source, len, filename != NULL ? filename : "<eval>" };
    duk_context *ctx;
    int ret = 0;

    if (engine == NULL) {
        return -1;
    }
    ctx = engine->ctx;
    /* Measure the stack limit from here, not from where the heap was created. */
    stack_base = (uintptr_t)__builtin_frame_address(0);
    if (duk_safe_call(ctx, eval_script, &args, 0, 1) != DUK_EXEC_SUCCESS) {
        if (duk_safe_call(ctx, print_error, NULL, 1, 1) != DUK_EXEC_SUCCESS) {
            console_print("Uncaught exception\r\n");
        }
        ret = -1;
    }
    duk_pop(ctx);
    stack_base = 0;
    return ret;
}

void jsb_engine_destroy(jsb_engine *engine)
{
    if (engine == NULL) {
        return;
    }
    if (engine->ctx != NULL) {
        duk_destroy_heap(engine->ctx);
    }
    free(engine);
}
