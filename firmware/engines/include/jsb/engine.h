/*
 * Engine adapter contract. Each firmware/engines/<engine>/ implements these
 * functions; exactly one engine is linked into an image (ENGINE= at build time).
 *
 * Required JavaScript globals: console.log(...args) writes its arguments,
 * converted to strings and separated by spaces, followed by "\r\n", through
 * jsb_hal_console_write().
 */
#ifndef JSB_ENGINE_H
#define JSB_ENGINE_H

#include <stddef.h>

typedef struct jsb_engine jsb_engine;

/* Engine name as used in ENGINE= and image names, e.g. "mquickjs". */
const char *jsb_engine_name(void);

/* Create a runtime with its globals installed; NULL if it cannot start. */
jsb_engine *jsb_engine_create(void);

/*
 * Evaluate a script. Returns 0 on success. On an uncaught exception, writes
 * the error (with filename and line where known) to the console and returns -1.
 */
int jsb_engine_eval(jsb_engine *engine, const char *source, size_t len, const char *filename);

/* Free the runtime and everything it allocated. */
void jsb_engine_destroy(jsb_engine *engine);

#endif
