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

/* --- dev mode --- */

/*
 * True if this boot is dev mode: the user asked for it with the board's button
 * (on the Pico: hold BOOTSEL for 2 s while the firmware runs). In dev mode the
 * script drive is shown to the PC and the script is not started. Platforms
 * without a button return false.
 */
bool jsb_hal_devmode(void);

/* --- script drive (only on platforms that build storage/, JSB_STORAGE=1) --- */

/*
 * Flash set aside for the FAT volume in storage/, as 512-byte sectors. Until
 * jsb_hal_drive_attach() only the firmware uses it; after that only the PC does.
 */
#define JSB_HAL_DISK_SECTOR_SIZE 512

/* Number of sectors, or 0 if this image has no drive. */
uint32_t jsb_hal_disk_sectors(void);

/* Read or write count sectors starting at sector. Return 0, or -1 if out of range. */
int jsb_hal_disk_read(uint32_t sector, void *buf, uint32_t count);
int jsb_hal_disk_write(uint32_t sector, const void *buf, uint32_t count);

/* Write cached sectors to flash. Returns 0 or -1. */
int jsb_hal_disk_sync(void);

/*
 * Show the drive to the PC (USB mass storage). The firmware must not use the
 * jsb_hal_disk_* functions afterwards: the PC owns the volume until the next reset.
 */
void jsb_hal_drive_attach(void);

#endif
