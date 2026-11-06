/* config.ini parser and default file, see jsb/config.h. */
#include <ctype.h>
#include <string.h>

#include "jsb/config.h"

const char jsb_config_default_text[] =
    "; JSBedded firmware settings. Read at boot.\r\n"
    "; [devmode] is the developer workflow: editing, running and watching the script.\r\n"
    "; Only the firmware reads it; scripts don't see these settings.\r\n"
    "[devmode]\r\n"
    "; USB drive with the script and this file:\r\n"
    ";   always - on every boot\r\n"
    ";   none   - only in dev mode: restart the board and long-press BOOTSEL\r\n"
    "usb_drive = always\r\n"
    "; When to restart and run the script after the drive changes:\r\n"
    ";   on_save  - after a file on the drive is saved\r\n"
    ";   on_eject - only after the drive is ejected\r\n"
    ";   none     - only after a hard restart (reset button or power)\r\n"
    "soft_restart = on_save\r\n"
    "; Where console.log output and firmware messages go: usb, uart0, both, off.\r\n"
    "; With uart0 or both, the firmware reserves UART0 and scripts can't open it.\r\n"
    "log_output = usb\r\n"
    "; Firmware's own messages (boot, loader, drive, config): off, info, debug.\r\n"
    "; console.log from the script is always shown.\r\n"
    "log_level = off\r\n";

void jsb_config_defaults(jsb_config *cfg)
{
    cfg->usb_drive = JSB_USB_DRIVE_ALWAYS;
    cfg->soft_restart = JSB_RESTART_ON_SAVE;
    cfg->log_output = JSB_LOG_USB;
    cfg->log_level = JSB_LOG_LEVEL_OFF;
}

typedef struct {
    const char *name;
    int value;
} choice;

static const choice usb_drive_values[] = { { "always", JSB_USB_DRIVE_ALWAYS }, { "none", JSB_USB_DRIVE_NONE }, { NULL, 0 } };
static const choice restart_values[] = { { "on_save", JSB_RESTART_ON_SAVE }, { "on_eject", JSB_RESTART_ON_EJECT },
                                         { "none", JSB_RESTART_NONE }, { NULL, 0 } };
static const choice output_values[] = { { "usb", JSB_LOG_USB }, { "uart0", JSB_LOG_UART0 }, { "both", JSB_LOG_BOTH },
                                        { "off", JSB_LOG_OFF }, { NULL, 0 } };
static const choice level_values[] = { { "off", JSB_LOG_LEVEL_OFF }, { "info", JSB_LOG_LEVEL_INFO },
                                       { "debug", JSB_LOG_LEVEL_DEBUG }, { NULL, 0 } };

static bool pick(const choice *list, const char *value, int *out)
{
    for (; list->name != NULL; list++) {
        if (strcmp(list->name, value) == 0) {
            *out = list->value;
            return true;
        }
    }
    return false;
}

static char *trim(char *s)
{
    while (isspace((unsigned char)*s)) {
        s++;
    }
    char *end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) {
        *--end = '\0';
    }
    return s;
}

void jsb_config_parse(const char *text, size_t len, jsb_config *cfg,
                      void (*warn)(int line, const char *msg, void *ctx), void *ctx)
{
#define WARN(msg) do { if (warn != NULL) warn(lineno, msg, ctx); } while (0)
    bool in_devmode = false;
    int lineno = 0;
    size_t pos = 0;
    while (pos < len) {
        char buf[96];
        size_t n = 0;
        bool too_long = false;
        while (pos < len && text[pos] != '\n') {
            if (n + 1 < sizeof buf) {
                buf[n++] = text[pos];
            } else {
                too_long = true;
            }
            pos++;
        }
        pos++; /* the newline */
        buf[n] = '\0';
        lineno++;
        if (too_long) {
            WARN("line too long, ignored");
            continue;
        }
        char *line = trim(buf);
        if (*line == '\0' || *line == ';' || *line == '#') {
            continue;
        }
        if (*line == '[') {
            char *close = strchr(line, ']');
            if (close == NULL) {
                WARN("bad section line, ignored");
                in_devmode = false;
                continue;
            }
            *close = '\0';
            in_devmode = strcmp(trim(line + 1), "devmode") == 0;
            continue;
        }
        if (!in_devmode) {
            continue; /* other sections belong to the script or to later versions */
        }
        char *eq = strchr(line, '=');
        if (eq == NULL) {
            WARN("expected key = value, ignored");
            continue;
        }
        *eq = '\0';
        char *key = trim(line);
        char *value = trim(eq + 1);
        int v;
        const choice *list;
        if (strcmp(key, "usb_drive") == 0) {
            list = usb_drive_values;
        } else if (strcmp(key, "soft_restart") == 0) {
            list = restart_values;
        } else if (strcmp(key, "log_output") == 0) {
            list = output_values;
        } else if (strcmp(key, "log_level") == 0) {
            list = level_values;
        } else {
            WARN("unknown key, ignored");
            continue;
        }
        if (!pick(list, value, &v)) {
            WARN("invalid value, default used");
            continue;
        }
        if (list == usb_drive_values) {
            cfg->usb_drive = (jsb_usb_drive)v;
        } else if (list == restart_values) {
            cfg->soft_restart = (jsb_soft_restart)v;
        } else if (list == output_values) {
            cfg->log_output = (jsb_log_output)v;
        } else {
            cfg->log_level = (jsb_log_level)v;
        }
    }
#undef WARN
}
