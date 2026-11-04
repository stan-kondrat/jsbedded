/* Global `gpio`. Spec: js/packages/gpio/index.d.ts. */
#include <stdio.h>
#include <string.h>

#include "jsb/api.h"
#include "jsb/engine.h"
#include "jsb/hal.h"

/* Upper bound for pin numbers before the HAL checks the board's real range. */
#define MAX_PIN 255

static int arg_pin(jsb_call *call, int *pin)
{
    double d;
    if (jsb_arg_type(call, 0) != JSB_NUMBER) {
        return jsb_throw(call, JSB_TYPE_ERROR, "pin must be a number");
    }
    if (jsb_arg_number(call, 0, &d) != 0) {
        return -1;
    }
    if (!(d >= 0 && d <= MAX_PIN) || d != (double)(int)d) {
        return jsb_throw(call, JSB_RANGE_ERROR, "pin must be a GPIO number");
    }
    *pin = (int)d;
    return 0;
}

static int throw_bad_pin(jsb_call *call, int pin)
{
    char msg[48];
    snprintf(msg, sizeof msg, "GPIO %d does not exist on this board", pin);
    return jsb_throw(call, JSB_RANGE_ERROR, msg);
}

/* gpio.mode(pin, "input" | "output") */
static int gpio_mode(jsb_call *call)
{
    char mode[8];
    int pin;
    jsb_gpio_mode m;

    if (arg_pin(call, &pin) != 0) {
        return -1;
    }
    if (jsb_arg_type(call, 1) != JSB_STRING) {
        return jsb_throw(call, JSB_TYPE_ERROR, "mode must be \"input\" or \"output\"");
    }
    if (jsb_arg_string(call, 1, mode, sizeof mode) < 0) {
        return -1;
    }
    if (strcmp(mode, "output") == 0) {
        m = JSB_GPIO_OUTPUT;
    } else if (strcmp(mode, "input") == 0) {
        m = JSB_GPIO_INPUT;
    } else {
        return jsb_throw(call, JSB_RANGE_ERROR, "mode must be \"input\" or \"output\"");
    }
    if (jsb_hal_gpio_mode(pin, m) != 0) {
        return throw_bad_pin(call, pin);
    }
    return 0;
}

/* gpio.write(pin, 0 | 1); true and false are accepted as 1 and 0. */
static int gpio_write(jsb_call *call)
{
    double value;
    int pin;

    if (arg_pin(call, &pin) != 0) {
        return -1;
    }
    jsb_type type = jsb_arg_type(call, 1);
    if (type != JSB_NUMBER && type != JSB_BOOL) {
        return jsb_throw(call, JSB_TYPE_ERROR, "value must be 0 or 1");
    }
    if (jsb_arg_number(call, 1, &value) != 0) {
        return -1;
    }
    if (value != 0 && value != 1) {
        return jsb_throw(call, JSB_RANGE_ERROR, "value must be 0 or 1");
    }
    if (jsb_hal_gpio_write(pin, value != 0) != 0) {
        return throw_bad_pin(call, pin);
    }
    return 0;
}

/* gpio.read(pin) -> 0 | 1 */
static int gpio_read(jsb_call *call)
{
    int pin;
    if (arg_pin(call, &pin) != 0) {
        return -1;
    }
    int level = jsb_hal_gpio_read(pin);
    if (level < 0) {
        return throw_bad_pin(call, pin);
    }
    jsb_value v = { JSB_NUMBER, { .number = level } };
    jsb_return(call, v);
    return 0;
}

int jsb_api_gpio_install(jsb_engine *engine)
{
    static const jsb_host_def defs[] = {
        JSB_DEF_FN("mode", gpio_mode),
        JSB_DEF_FN("write", gpio_write),
        JSB_DEF_FN("read", gpio_read),
    };
    return jsb_engine_define(engine, "gpio", defs, sizeof defs / sizeof defs[0]);
}
