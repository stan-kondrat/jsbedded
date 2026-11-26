/*
 * jsb_trace(step): boot and engine bring-up trace, for developing and troubleshooting engines.
 *
 * Off (the default): just records the step for CRASH.TXT, like jsb_hal_crash_step.
 * On (make firmware ... TRACE=1 -> JSB_TRACE): the HAL also prints heap and stack use on the serial
 * port, prints every allocation of 1 KB or more, and waits up to 20 s for a USB serial host at boot.
 * Pico only (hal/pico/hal_pico.c, where platform.mk sets JSB_TRACE); other HALs never define it.
 */
#ifndef JSB_TRACE_H
#define JSB_TRACE_H

#include "jsb/hal.h"

#ifdef JSB_TRACE
void jsb_trace(const char *what);
#else
#define jsb_trace(what) jsb_hal_crash_step(what)
#endif

#endif
