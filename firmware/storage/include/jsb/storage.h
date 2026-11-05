/*
 * Script drive: a FAT volume in the board's flash (jsb_hal_disk_*) that the PC
 * sees as a USB drive named JSBEDDED. The firmware reads it only at boot, before
 * jsb_hal_drive_attach(); after that only the PC writes to it, so the two never
 * cache the volume at the same time.
 */
#ifndef JSB_STORAGE_H
#define JSB_STORAGE_H

#include <stddef.h>

/* Volume label, also the drive name the PC shows. */
#define JSB_STORAGE_LABEL "JSBEDDED"

/* Largest script the loader reads, in bytes. */
#ifndef JSB_STORAGE_SCRIPT_MAX
#define JSB_STORAGE_SCRIPT_MAX (64 * 1024)
#endif

typedef struct {
    char *source;  /* NUL-terminated, kept for the life of the program */
    size_t len;    /* without the NUL */
    char name[13]; /* 8.3 file name as asked for, e.g. "main.js" */
} jsb_storage_script;

/*
 * Mount the drive, formatting it first (FAT, label JSBEDDED) if it holds no FAT
 * volume. Returns 0, or -1 if the image has no drive or it can't be mounted
 * (reported on the console).
 */
int jsb_storage_mount(void);

/*
 * Read the first of main.js, index.js, main.mjs, index.mjs from the mounted
 * drive. Returns 1 and fills *script if one was read, 0 if there is none, or -1
 * if one exists but can't be read (too large, out of memory, I/O error; reported).
 */
int jsb_storage_load_script(jsb_storage_script *script);

/* Write everything back and unmount. Call before jsb_hal_drive_attach(). */
void jsb_storage_unmount(void);

#endif
