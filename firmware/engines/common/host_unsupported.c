/*
 * Host-function part of engine.h for engines that don't implement it yet
 * (quickjs, jerryscript, duktape, none). jsb_engine_define() and
 * jsb_engine_call() report "not supported" once and fail, so the firmware
 * still runs the script with the engine's own globals (console.log) but
 * without timers, gpio or board. The jsb_arg_*() functions are never reached:
 * no host function can be defined.
 */
#include <string.h>

#include "jsb/engine.h"
#include "jsb/hal.h"

static void report_unsupported(void)
{
    static bool reported;
    if (!reported) {
        const char *name = jsb_engine_name();
        const char *msg = ": host functions (timers, gpio, board) not supported by this engine yet\r\n";
        jsb_hal_console_write("JSBedded: ", 10);
        jsb_hal_console_write(name, strlen(name));
        jsb_hal_console_write(msg, strlen(msg));
        reported = true;
    }
}

int jsb_engine_define(jsb_engine *engine, const char *object, const jsb_host_def *defs, size_t count)
{
    (void)engine;
    (void)object;
    (void)defs;
    (void)count;
    report_unsupported();
    return -1;
}

int jsb_engine_call(jsb_engine *engine, jsb_func func, int argc, const jsb_value *argv)
{
    (void)engine;
    (void)func;
    (void)argc;
    (void)argv;
    report_unsupported();
    return -1;
}

void jsb_engine_release(jsb_engine *engine, jsb_func func)
{
    (void)engine;
    (void)func;
}

int jsb_engine_run_jobs(jsb_engine *engine)
{
    (void)engine;
    return 0;
}

int jsb_argc(jsb_call *call)
{
    (void)call;
    return 0;
}

jsb_type jsb_arg_type(jsb_call *call, int i)
{
    (void)call;
    (void)i;
    return JSB_UNDEFINED;
}

int jsb_arg_number(jsb_call *call, int i, double *out)
{
    (void)call;
    (void)i;
    (void)out;
    return -1;
}

int jsb_arg_string(jsb_call *call, int i, char *buf, size_t size)
{
    (void)call;
    (void)i;
    (void)buf;
    (void)size;
    return -1;
}

jsb_func jsb_arg_function(jsb_call *call, int i)
{
    (void)call;
    (void)i;
    return JSB_FUNC_NONE;
}

int jsb_arg_log(jsb_call *call, int i)
{
    (void)call;
    (void)i;
    return -1;
}

void jsb_return(jsb_call *call, jsb_value value)
{
    (void)call;
    (void)value;
}

int jsb_throw(jsb_call *call, jsb_error kind, const char *message)
{
    (void)call;
    (void)kind;
    (void)message;
    return -1;
}
