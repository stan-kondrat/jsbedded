/* Internal to hal/pico: functions shared between its source files. */
#ifndef JSB_PICO_HAL_H
#define JSB_PICO_HAL_H

#include <stdbool.h>
#include <stdint.h>

/* devmode.c: read the dev-mode request left by the last restart, start watching BOOTSEL. */
void jsb_pico_devmode_init(void);

/*
 * flash_disk.c: true if the image has a script drive that doesn't overlap the
 * firmware; reports both sizes in KB (either pointer may be NULL).
 */
bool jsb_pico_drive_check(uint32_t *firmware_kb, uint32_t *drive_kb);

#endif
