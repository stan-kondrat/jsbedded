/* Global `console`. Spec: js/packages/console/index.d.ts. */
#include "jsb/api.h"
#include "jsb/engine.h"
#include "jsb/hal.h"

/* console.log(...args): arguments separated by spaces, then "\r\n". */
static int console_log(jsb_call *call)
{
    int argc = jsb_argc(call);
    for (int i = 0; i < argc; i++) {
        if (i != 0) {
            jsb_hal_console_write(" ", 1);
        }
        if (jsb_arg_log(call, i) != 0) {
            return -1;
        }
    }
    jsb_hal_console_write("\r\n", 2);
    return 0;
}

int jsb_api_console_install(jsb_engine *engine)
{
    static const jsb_host_def defs[] = {
        JSB_DEF_FN("log", console_log),
    };
    return jsb_engine_define(engine, "console", defs, sizeof defs / sizeof defs[0]);
}
