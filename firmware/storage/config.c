/* config.ini parser and default file, see jsb/config.h. */
#include <ctype.h>
#include <string.h>

#include "jsb/config.h"

const char jsb_config_default_text[] =
    "; JSBedded firmware settings. Read at boot.\r\n"
    "[devmode]\r\n"
    "; The developer workflow: editing, running and watching the script.\r\n"
    "; Only the firmware reads it; scripts don't see these settings.\r\n"
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
    "log_level = off\r\n"
    "; After a crash (the board restarts by itself; CRASH.TXT on this drive says why):\r\n"
    ";   restart - run the script again; after 3 crashes in a row, as report\r\n"
    ";   report  - don't run it until the next reset\r\n"
    "; A crash in the first boot after flashing new firmware is always handled as report.\r\n"
    "on_crash = restart\r\n";

void jsb_config_defaults(jsb_config *cfg)
{
    cfg->usb_drive = JSB_USB_DRIVE_ALWAYS;
    cfg->soft_restart = JSB_RESTART_ON_SAVE;
    cfg->log_output = JSB_LOG_USB;
    cfg->log_level = JSB_LOG_LEVEL_OFF;
    cfg->on_crash = JSB_ON_CRASH_RESTART;
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
static const choice crash_values[] = { { "restart", JSB_ON_CRASH_RESTART }, { "report", JSB_ON_CRASH_REPORT },
                                       { NULL, 0 } };

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
        } else if (strcmp(key, "on_crash") == 0) {
            list = crash_values;
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
        } else if (list == crash_values) {
            cfg->on_crash = (jsb_on_crash)v;
        } else {
            cfg->log_level = (jsb_log_level)v;
        }
    }
#undef WARN
}

/* Start of the line after the one at pos (or len). */
static size_t next_line(const char *text, size_t len, size_t pos)
{
    while (pos < len && text[pos] != '\n') {
        pos++;
    }
    return pos < len ? pos + 1 : len;
}

/* If the line at pos is a section header, its name (trimmed, up to 15 chars) goes to name. */
static bool section_at(const char *text, size_t len, size_t pos, char name[16])
{
    while (pos < len && (text[pos] == ' ' || text[pos] == '\t')) {
        pos++;
    }
    if (pos >= len || text[pos] != '[') {
        return false;
    }
    size_t n = 0;
    for (pos++; pos < len && text[pos] != ']' && text[pos] != '\n'; pos++) {
        if (text[pos] != ' ' && text[pos] != '\t' && n < 15) {
            name[n++] = text[pos];
        }
    }
    name[n] = '\0';
    return true;
}

size_t jsb_config_set_firmware_section(const char *text, size_t len, const char *section, char *out, size_t cap)
{
    /* [from, to) is replaced: the old [firmware] section, or empty before the first section. */
    size_t from = len, to = len;
    bool found = false;
    char name[16];
    for (size_t pos = 0; pos < len; pos = next_line(text, len, pos)) {
        if (!section_at(text, len, pos, name)) {
            continue;
        }
        if (found) {
            to = pos;
            break;
        }
        if (strcmp(name, "firmware") == 0) {
            from = pos;
            to = len;
            found = true;
        } else if (from == len) {
            from = to = pos;
        }
    }
    if (!found && from == len && len > 0 && text[len - 1] != '\n') {
        return 0; /* no section and no final newline: leave such a file alone */
    }
    size_t add = strlen(section);
    size_t total = from + add + (len - to);
    if (total > cap) {
        return 0;
    }
    memcpy(out, text, from);
    memcpy(out + from, section, add);
    memcpy(out + from + add, text + to, len - to);
    return total;
}
