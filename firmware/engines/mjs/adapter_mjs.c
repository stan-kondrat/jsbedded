/*
 * mJS engine adapter (minimal): evaluates a script and prints its output.
 * Host functions are not bound yet (common/host_unsupported.c). mJS keeps its
 * state on the C heap, so one runtime exists at a time.
 *
 * console.log is a native function installed with mjs_set() +
 * mjs_mk_foreign_func(), the way mJS installs its own print(). Strings are
 * written as they are, other values in mJS's own format (mjs_fprintf, JSON
 * style). mJS strings are byte strings, written to the console unchanged.
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mjs.h"

#include "jsb/engine.h"

struct jsb_engine {
    struct mjs *mjs;
};

static jsb_engine the_engine;

static void console_print(const char *s)
{
    fwrite(s, 1, strlen(s), stdout);
}

/* console.log(...args): arguments separated by spaces, then "\r\n". */
static void js_console_log(struct mjs *mjs)
{
    int n = mjs_nargs(mjs);

    for (int i = 0; i < n; i++) {
        mjs_val_t arg = mjs_arg(mjs, i);

        if (i > 0) {
            fputc(' ', stdout);
        }
        if (mjs_is_string(arg)) {
            size_t len;
            const char *s = mjs_get_string(mjs, &arg, &len);

            fwrite(s, 1, len, stdout);
        } else {
            mjs_fprintf(arg, mjs, stdout);
        }
    }
    console_print("\r\n");
    fflush(stdout);
    mjs_return(mjs, mjs_mk_undefined());
}

static bool install_console(struct mjs *mjs)
{
    mjs_val_t console = mjs_mk_object(mjs);

    if (mjs_set(mjs, console, "log", ~0, mjs_mk_foreign_func(mjs, (mjs_func_ptr_t)js_console_log)) != MJS_OK) {
        return false;
    }
    return mjs_set(mjs, mjs_get_global(mjs), "console", ~0, console) == MJS_OK;
}

const char *jsb_engine_name(void)
{
    return "mjs";
}

jsb_engine *jsb_engine_create(void)
{
    if (the_engine.mjs != NULL) {
        return NULL;
    }
    the_engine.mjs = mjs_create();
    if (the_engine.mjs == NULL || !install_console(the_engine.mjs)) {
        console_print("JSBedded: mJS could not start\r\n");
        jsb_engine_destroy(&the_engine);
        return NULL;
    }
    return &the_engine;
}

int jsb_engine_eval(jsb_engine *engine, const char *source, size_t len, const char *filename)
{
    mjs_val_t res = mjs_mk_undefined();
    mjs_err_t err;

    (void)filename; /* mJS error messages carry no file name */
    if (engine == NULL || engine->mjs == NULL) {
        return -1;
    }
    /* mjs_exec_buf() is declared in mjs.h but not defined upstream; mjs_exec()
       needs a NUL-terminated string, and the source is not guaranteed to be. */
    char *copy = malloc(len + 1);

    if (copy == NULL) {
        console_print("Uncaught out of memory\r\n");
        return -1;
    }
    memcpy(copy, source, len);
    copy[len] = '\0';
    err = mjs_exec(engine->mjs, copy, &res);
    free(copy);
    if (err != MJS_OK) {
        console_print("Uncaught ");
        mjs_print_error(engine->mjs, stdout, NULL, 1);
        fflush(stdout);
        return -1;
    }
    return 0;
}

void jsb_engine_destroy(jsb_engine *engine)
{
    if (engine == NULL || engine->mjs == NULL) {
        return;
    }
    mjs_destroy(engine->mjs);
    engine->mjs = NULL;
}
