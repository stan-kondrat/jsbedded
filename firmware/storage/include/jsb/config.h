/*
 * config.ini on the script drive: firmware behaviour settings, read at boot.
 * A small INI subset: [sections], key = value, ; and # comment lines. Unknown
 * keys and bad values are reported through warn() and ignored; parsing never
 * fails, so a broken file can't stop the firmware from booting.
 * Pure C, no FatFs: unit-testable on the host.
 */
#ifndef JSB_CONFIG_H
#define JSB_CONFIG_H

#include <stdbool.h>
#include <stddef.h>

typedef enum { JSB_USB_DRIVE_ALWAYS, JSB_USB_DRIVE_NONE } jsb_usb_drive;
typedef enum { JSB_RESTART_ON_SAVE, JSB_RESTART_ON_EJECT, JSB_RESTART_NONE } jsb_soft_restart;
typedef enum { JSB_LOG_USB, JSB_LOG_UART0, JSB_LOG_BOTH, JSB_LOG_OFF } jsb_log_output;
typedef enum { JSB_LOG_LEVEL_OFF, JSB_LOG_LEVEL_INFO, JSB_LOG_LEVEL_DEBUG } jsb_log_level;
typedef enum { JSB_ON_CRASH_RESTART, JSB_ON_CRASH_REPORT } jsb_on_crash;

/* The [devmode] section. */
typedef struct {
    jsb_usb_drive usb_drive;
    jsb_soft_restart soft_restart;
    jsb_log_output log_output;
    jsb_log_level log_level;
    jsb_on_crash on_crash; /* applied (core/src/main.c); the others are parsed only, for now */
} jsb_config;

/* Commented default file, written to the drive when it is missing. */
extern const char jsb_config_default_text[];

void jsb_config_defaults(jsb_config *cfg);

/* Apply text over *cfg (call jsb_config_defaults first). warn is optional; line is 1-based. */
void jsb_config_parse(const char *text, size_t len, jsb_config *cfg,
                      void (*warn)(int line, const char *msg, void *ctx), void *ctx);

/*
 * Put section (a complete "[firmware]\r\n..." block ending in a newline) into the
 * config text: in place of an existing [firmware] section, else before the first
 * section, else at the end. Writes to out and returns the new length, or 0 if it
 * doesn't fit in cap. The [firmware] section describes the running firmware and
 * is not a setting; the parser ignores it.
 */
size_t jsb_config_set_firmware_section(const char *text, size_t len, const char *section, char *out, size_t cap);

#endif
