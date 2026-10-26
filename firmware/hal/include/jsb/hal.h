/*
 * Hardware abstraction layer: the only hardware interface core/ and api/ may use.
 * Each platform in hal/<platform>/ implements these functions and provides the
 * program entry point, which calls jsb_main().
 */
#ifndef JSB_HAL_H
#define JSB_HAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Initialize clocks, console and on-board peripherals. */
void jsb_hal_init(void);

/* Write len bytes to the console (USB serial or UART, per platform). */
void jsb_hal_console_write(const char *s, size_t len);

/* Block the caller for at least ms milliseconds. */
void jsb_hal_sleep_ms(uint32_t ms);

/* Switch the on-board status LED; a no-op on boards without a simple LED. */
void jsb_hal_led_set(bool on);

#endif
