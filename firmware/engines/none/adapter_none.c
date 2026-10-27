/* Placeholder engine: no JavaScript. Keeps every board buildable without an engine. */
#include "jsb/engine.h"

const char *jsb_engine_name(void)
{
    return "none";
}

jsb_engine *jsb_engine_create(void)
{
    return NULL;
}

int jsb_engine_eval(jsb_engine *engine, const char *source, size_t len, const char *filename)
{
    (void)engine;
    (void)source;
    (void)len;
    (void)filename;
    return -1;
}

void jsb_engine_destroy(jsb_engine *engine)
{
    (void)engine;
}
