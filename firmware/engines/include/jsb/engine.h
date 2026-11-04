/*
 * Engine adapter contract. Each firmware/engines/<engine>/ implements these
 * functions; exactly one engine is linked into an image (ENGINE= at build time).
 *
 * Runtime: create, evaluate scripts, run pending jobs, destroy. Uncaught
 * exceptions are written to the console through jsb_hal_console_write().
 *
 * Host functions: core/ and api/ expose native functions and constants to JS
 * with jsb_engine_define(), read arguments with jsb_arg_*(), and call back
 * into JS functions they retained with jsb_engine_call(). Values crossing the
 * boundary are simple: undefined, boolean, number, string, function handle.
 *
 * Error rule: a jsb_arg_*() or jsb_throw() call that fails leaves a JS
 * exception pending and returns -1; the host function then returns -1, and
 * the exception is thrown to the JS caller.
 */
#ifndef JSB_ENGINE_H
#define JSB_ENGINE_H

#include <stdbool.h>
#include <stddef.h>

typedef struct jsb_engine jsb_engine;

/* Engine name as used in ENGINE= and image names, e.g. "mquickjs". */
const char *jsb_engine_name(void);

/* Create a runtime with its built-in globals; NULL if it cannot start. */
jsb_engine *jsb_engine_create(void);

/*
 * Evaluate a script. Returns 0 on success. On an uncaught exception, writes
 * the error (with filename and line where known) to the console and returns -1.
 */
int jsb_engine_eval(jsb_engine *engine, const char *source, size_t len, const char *filename);

/* Free the runtime and everything it allocated, including retained functions. */
void jsb_engine_destroy(jsb_engine *engine);

/* --- values --- */

typedef enum {
    JSB_UNDEFINED,
    JSB_BOOL,
    JSB_NUMBER,
    JSB_STRING,
    JSB_FUNCTION,
    JSB_OTHER /* null, objects, arrays: only reported by jsb_arg_type() */
} jsb_type;

/* Handle to a JS function retained by the engine; valid until jsb_engine_release(). */
typedef int jsb_func;
#define JSB_FUNC_NONE (-1)

typedef struct {
    jsb_type type;
    union {
        bool boolean;
        double number;
        const char *string; /* NUL-terminated, copied by the engine */
        jsb_func func;
    } u;
} jsb_value;

/* --- host functions --- */

/* Arguments and result of one host function call; valid only during that call. */
typedef struct jsb_call jsb_call;

/* Native function callable from JS. Returns 0, or -1 with an exception pending. */
typedef int (*jsb_host_fn)(jsb_call *call);

/* One property: a function (fn != NULL) or a constant value (fn == NULL). */
typedef struct {
    const char *name;
    jsb_host_fn fn;
    jsb_value value;
} jsb_host_def;

#define JSB_DEF_FN(name, fn) { (name), (fn), { JSB_UNDEFINED, { false } } }
#define JSB_DEF_NUMBER(name, n) { (name), NULL, { JSB_NUMBER, { .number = (n) } } }
#define JSB_DEF_STRING(name, s) { (name), NULL, { JSB_STRING, { .string = (s) } } }

/*
 * Define count properties on the global object (object == NULL) or on a new
 * object stored in the global variable `object`. Returns 0, or -1 (and writes
 * the reason to the console) if the engine can't: out of memory, too many host
 * functions, or no host function support in this engine.
 */
int jsb_engine_define(jsb_engine *engine, const char *object, const jsb_host_def *defs, size_t count);

/*
 * Call a retained function with this = undefined and argc arguments. Returns 0,
 * or -1 after writing the uncaught exception to the console.
 */
int jsb_engine_call(jsb_engine *engine, jsb_func func, int argc, const jsb_value *argv);

/* Release a function handle from jsb_arg_function(); JSB_FUNC_NONE is ignored. */
void jsb_engine_release(jsb_engine *engine, jsb_func func);

/*
 * Run pending jobs (Promise reactions). Returns the number run, or -1 after
 * writing an uncaught exception. Engines without jobs return 0.
 */
int jsb_engine_run_jobs(jsb_engine *engine);

/* --- inside a host function --- */

/* Number of arguments passed. */
int jsb_argc(jsb_call *call);

/* Type of argument i; JSB_UNDEFINED when i >= jsb_argc(). */
jsb_type jsb_arg_type(jsb_call *call, int i);

/* Argument i converted as Number(arg). Returns 0 or -1. */
int jsb_arg_number(jsb_call *call, int i, double *out);

/* Argument i converted as String(arg) into buf (NUL-terminated, truncated to size - 1).
 * Returns the stored length or -1. */
int jsb_arg_string(jsb_call *call, int i, char *buf, size_t size);

/* Retain argument i, which must be a function (TypeError otherwise). Returns a
 * handle or JSB_FUNC_NONE (-1) with an exception pending. */
jsb_func jsb_arg_function(jsb_call *call, int i);

/* Write argument i to the console as console.log shows it: strings as-is,
 * other values in the engine's inspect format. Returns 0 or -1. */
int jsb_arg_log(jsb_call *call, int i);

/* Set the call's return value (default: undefined). */
void jsb_return(jsb_call *call, jsb_value value);

typedef enum {
    JSB_ERROR,
    JSB_TYPE_ERROR,
    JSB_RANGE_ERROR
} jsb_error;

/* Throw a new error with this message. Always returns -1. */
int jsb_throw(jsb_call *call, jsb_error kind, const char *message);

#endif
