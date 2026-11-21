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

/* --- crash record --- */

/*
 * A crash (CPU fault, failed assert(), abort(), SDK panic or exit(), engine fatal
 * error via jsb_hal_fatal) is saved in RAM that survives the restart that follows,
 * and the next boot reads it with jsb_hal_crash_take(). It is saved whatever the
 * log settings are. core/src/crash.c turns it into text (CRASH.TXT) and decides
 * whether the script starts again. Platforms that can't keep a record return
 * false from jsb_hal_crash_take() and implement the rest as no-ops.
 */
typedef enum {
    JSB_CRASH_FAULT,  /* CPU fault (hard fault): bad pointer, stack overflow, ... */
    JSB_CRASH_ASSERT, /* failed C assert() */
    JSB_CRASH_ABORT,  /* abort() */
    JSB_CRASH_PANIC,  /* SDK panic (e.g. out of memory) or exit() */
    JSB_CRASH_FATAL,  /* jsb_hal_fatal(): an engine's fatal error */
} jsb_crash_kind;

typedef struct {
    uint32_t kind;      /* jsb_crash_kind */
    uint32_t pc, lr, sp; /* registers at the fault; for the others lr is the caller, pc and sp 0 */
    uint32_t uptime_ms; /* time since boot */
    uint32_t count;     /* crashes in a row, 1 for the first (see jsb_hal_crash_streak_reset) */
    uint32_t image_id;  /* jsb_hal_image_id() of the image that crashed */
    uint32_t new_image; /* 1: it crashed in the first boot after that image was flashed */
    int32_t line;       /* source line, 0 if none */
    char step[32];      /* last jsb_hal_crash_step(), "" if none */
    char msg[96];       /* message or failed expression, "" if none */
    char file[64];      /* source file, "" if none */
} jsb_crash_info;

/*
 * The previous boot's crash: fills *info and returns true once, then forgets it.
 * False if the previous boot didn't crash or the record was lost (power cycle).
 */
bool jsb_hal_crash_take(jsb_crash_info *info);

/* Name what the firmware is doing now (a string constant); a crash record keeps it. */
void jsb_hal_crash_step(const char *step);

/* Mark this boot as the first one after a new image was flashed (kept in a crash record). */
void jsb_hal_crash_set_new_image(bool new_image);

/* The firmware has run long enough without crashing: the next crash counts from 1 again. */
void jsb_hal_crash_streak_reset(void);

/* Save a crash record for an unrecoverable error and restart the board. */
_Noreturn void jsb_hal_fatal(const char *msg, const char *file, int line);

/*
 * An id of the firmware image: changes when different firmware is flashed (a hash of
 * the image, not of its version string), 0 if the platform can't tell.
 */
uint32_t jsb_hal_image_id(void);

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
