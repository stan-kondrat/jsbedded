/*
 * Script drive (jsb/storage.h): mounts the FAT volume on the HAL's flash
 * sectors, formats it on first boot, and reads the user's script.
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ff.h"
#include "diskio.h"

#include "jsb/hal.h"
#include "jsb/storage.h"

/* Looked up in this order; the first one that exists is the script. */
static const char *const script_names[] = { "main.js", "index.js", "main.mjs", "index.mjs" };

static FATFS fs;
static bool mounted;
static bool formatted;

static void say(const char *fmt, ...)
{
    char line[120];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(line, sizeof line, fmt, ap);
    va_end(ap);
    if (n > 0) {
        jsb_hal_console_write(line, (size_t)n < sizeof line ? (size_t)n : sizeof line - 1);
    }
}

/*
 * FAT12/16 without a partition table, like a floppy: every OS mounts it. Data
 * clusters are 4 KB and aligned to 4 KB, the flash erase size, so writing one
 * cluster erases one flash block.
 */
static FRESULT format(void)
{
    static const MKFS_PARM opt = {
        .fmt = FM_FAT | FM_SFD,
        .n_fat = 0,   /* default: 2 */
        .align = 8,   /* sectors: 4 KB */
        .n_root = 0,  /* default: 512 entries */
        .au_size = 4096,
    };
    BYTE work[FF_MAX_SS];
    FRESULT fr = f_mkfs("", &opt, work, sizeof work);
    if (fr != FR_OK) {
        return fr;
    }
    fr = f_mount(&fs, "", 1);
    if (fr != FR_OK) {
        return fr;
    }
    fr = f_setlabel(JSB_STORAGE_LABEL);
    if (fr == FR_OK) {
        fr = disk_ioctl(0, CTRL_SYNC, NULL) == RES_OK ? FR_OK : FR_DISK_ERR;
    }
    return fr;
}

int jsb_storage_mount(void)
{
    if (jsb_hal_disk_sectors() == 0) {
        return -1;
    }
    FRESULT fr = f_mount(&fs, "", 1);
    if (fr == FR_NO_FILESYSTEM) {
        /* Never formatted, or the PC formatted it as something FatFs can't read (exFAT). */
        say("JSBedded: formatting the %s drive\r\n", JSB_STORAGE_LABEL);
        fr = format();
        formatted = fr == FR_OK;
    }
    if (fr != FR_OK) {
        say("JSBedded: can't mount the drive (FatFs error %d)\r\n", (int)fr);
        f_unmount("");
        return -1;
    }
    mounted = true;
    return 0;
}

static int read_file(const char *name, jsb_storage_script *script)
{
    FIL file;
    FRESULT fr = f_open(&file, name, FA_READ);
    if (fr != FR_OK) {
        say("JSBedded: can't open %s (FatFs error %d)\r\n", name, (int)fr);
        return -1;
    }
    FSIZE_t size = f_size(&file);
    if (size > JSB_STORAGE_SCRIPT_MAX) {
        f_close(&file);
        say("JSBedded: %s is %lu bytes; the limit is %lu\r\n", name, (unsigned long)size,
            (unsigned long)JSB_STORAGE_SCRIPT_MAX);
        return -1;
    }
    char *buf = malloc((size_t)size + 1);
    if (buf == NULL) {
        f_close(&file);
        say("JSBedded: not enough memory to load %s\r\n", name);
        return -1;
    }
    UINT got = 0;
    fr = f_read(&file, buf, (UINT)size, &got);
    f_close(&file);
    if (fr != FR_OK || got != (UINT)size) {
        free(buf);
        say("JSBedded: can't read %s (FatFs error %d)\r\n", name, (int)(fr != FR_OK ? fr : FR_DISK_ERR));
        return -1;
    }
    buf[size] = '\0';
    script->source = buf;
    script->len = (size_t)size;
    snprintf(script->name, sizeof script->name, "%s", name);
    return 1;
}

int jsb_storage_load_script(jsb_storage_script *script)
{
    if (!mounted) {
        return 0;
    }
    for (size_t i = 0; i < sizeof script_names / sizeof script_names[0]; i++) {
        FILINFO info;
        FRESULT fr = f_stat(script_names[i], &info);
        if (fr == FR_OK && !(info.fattrib & AM_DIR)) {
            return read_file(script_names[i], script);
        }
        if (fr != FR_OK && fr != FR_NO_FILE) {
            say("JSBedded: can't look for %s (FatFs error %d)\r\n", script_names[i], (int)fr);
            return -1;
        }
    }
    return 0;
}

void jsb_storage_unmount(void)
{
    if (!mounted) {
        return;
    }
    f_unmount("");
    jsb_hal_disk_sync();
    mounted = false;
}

bool jsb_storage_was_formatted(void)
{
    return formatted;
}

static const char readme_text[] =
    "JSBedded script drive\r\n"
    "=====================\r\n"
    "\r\n"
    "main.js    your script; runs after the board restarts\r\n"
    "config.ini firmware settings (usb_drive, soft_restart, log_output, log_level)\r\n"
    "\r\n"
    "Dev mode (you are probably here): the script is not started. Fix or edit it\r\n"
    "here, eject the drive, then reset the board to run it. Hold BOOTSEL for 2 s\r\n"
    "while it runs to enter or leave dev mode.\r\n"
    "\r\n"
    "Docs: https://github.com/stan-kondrat/jsbedded/tree/main/docs\r\n";

static void write_if_missing(const char *name, const char *text, size_t len)
{
    FILINFO info;
    if (f_stat(name, &info) != FR_NO_FILE) {
        return;
    }
    FIL file;
    UINT wrote = 0;
    FRESULT fr = f_open(&file, name, FA_WRITE | FA_CREATE_NEW);
    if (fr == FR_OK) {
        fr = f_write(&file, text, (UINT)len, &wrote);
        FRESULT closed = f_close(&file);
        if (fr == FR_OK) {
            fr = closed;
        }
    }
    if (fr != FR_OK || wrote != (UINT)len) {
        say("JSBedded: can't create %s (FatFs error %d)\r\n", name, (int)fr);
    }
}

void jsb_storage_provision(void)
{
    if (!mounted) {
        return;
    }
    write_if_missing("config.ini", jsb_config_default_text, strlen(jsb_config_default_text));
    write_if_missing("README.txt", readme_text, sizeof readme_text - 1);
}

static void config_warning(int line, const char *msg, void *ctx)
{
    (void)ctx;
    say("JSBedded: config.ini line %d: %s\r\n", line, msg);
}

void jsb_storage_load_config(jsb_config *cfg)
{
    jsb_config_defaults(cfg);
    if (!mounted) {
        return;
    }
    FIL file;
    if (f_open(&file, "config.ini", FA_READ) != FR_OK) {
        return;
    }
    static char buf[2048];
    UINT got = 0;
    FRESULT fr = f_read(&file, buf, sizeof buf, &got);
    f_close(&file);
    if (fr != FR_OK) {
        say("JSBedded: can't read config.ini (FatFs error %d), defaults used\r\n", (int)fr);
        return;
    }
    if (got == sizeof buf) {
        say("JSBedded: config.ini is longer than %u bytes, the rest is ignored\r\n", (unsigned)sizeof buf);
    }
    jsb_config_parse(buf, got, cfg, config_warning, NULL);
}
