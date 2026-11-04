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

/* Initialize clocks and the console. */
void jsb_hal_init(void);

/* Write len bytes to the console (USB serial or UART, per platform). */
void jsb_hal_console_write(const char *s, size_t len);

/* Milliseconds since boot; monotonic, never wraps in practice. */
uint64_t jsb_hal_millis(void);

/* Block the caller for at least ms milliseconds. */
void jsb_hal_sleep_ms(uint32_t ms);

/* --- GPIO: pins are the MCU's GPIO numbers (GP25 -> 25) --- */

typedef enum {
    JSB_GPIO_INPUT,
    JSB_GPIO_OUTPUT
} jsb_gpio_mode;

/* Configure a pin. Returns 0, or -1 if the pin doesn't exist or can't take that mode. */
int jsb_hal_gpio_mode(int pin, jsb_gpio_mode mode);

/* Drive an output pin high (true) or low. Returns 0, or -1 for an invalid pin. */
int jsb_hal_gpio_write(int pin, bool high);

/* Read a pin's level: 1 high, 0 low, -1 for an invalid pin. */
int jsb_hal_gpio_read(int pin);

#endif
