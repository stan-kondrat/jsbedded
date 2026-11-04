/* Hardware APIs (firmware/api/<module>/), installed by core before the script runs. */
#ifndef JSB_API_H
#define JSB_API_H

#include "jsb/engine.h"

/* Global `console` with log(). Returns 0 or -1. */
int jsb_api_console_install(jsb_engine *engine);

/* Global `gpio` with mode(), write(), read(). Returns 0 or -1. */
int jsb_api_gpio_install(jsb_engine *engine);

#endif
