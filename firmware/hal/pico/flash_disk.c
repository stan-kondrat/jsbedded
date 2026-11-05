/*
 * Script drive sectors (jsb_hal_disk_*) in the last JSB_DRIVE_KB of flash.
 *
 * FAT works in 512-byte sectors, the flash erases 4 KB blocks. Writes go into a
 * RAM copy of one 4 KB block, written back (erase + program) when a write moves
 * to another block or on jsb_hal_disk_sync(). Sectors written with the data
 * they already hold don't dirty the block, so rewriting an unchanged FAT costs
 * no erase. A power loss while a block is written back loses that block.
 *
 * Callers: storage/ at boot, then only the USB drive (usb_drive.c) from the
 * TinyUSB task, never both at once (see jsb_hal_drive_attach). Erase/program
 * run with interrupts off, as the flash isn't readable (XIP) meanwhile.
 */
#include <string.h>

#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"

#include "jsb/hal.h"
#include "pico_hal.h"

#ifndef JSB_DRIVE_KB
#define JSB_DRIVE_KB 0
#endif

#define DRIVE_BYTES   ((uint32_t)JSB_DRIVE_KB * 1024u)
#define DRIVE_OFFSET  ((uint32_t)PICO_FLASH_SIZE_BYTES - DRIVE_BYTES)
#define BLOCK_SIZE    FLASH_SECTOR_SIZE                       /* 4096: erase size */
#define SECTOR_SIZE   JSB_HAL_DISK_SECTOR_SIZE
#define PER_BLOCK     (BLOCK_SIZE / SECTOR_SIZE)

_Static_assert(DRIVE_BYTES % BLOCK_SIZE == 0, "JSB_DRIVE_KB must be a multiple of 4");
_Static_assert(DRIVE_BYTES <= PICO_FLASH_SIZE_BYTES, "JSB_DRIVE_KB is larger than the flash");

extern char __flash_binary_end; /* linker: end of the firmware image in flash */

static uint8_t cache[BLOCK_SIZE] __attribute__((aligned(4)));
static int32_t cache_block = -1;
static bool cache_dirty;
static int drive_state = -1; /* -1 not checked yet, 0 no drive, 1 drive */

static const uint8_t *flash_at(uint32_t offset)
{
    return (const uint8_t *)(XIP_BASE + offset);
}

bool jsb_pico_drive_check(uint32_t *firmware_kb, uint32_t *drive_kb)
{
    uint32_t firmware_end = (uint32_t)((uintptr_t)&__flash_binary_end - XIP_BASE);
    if (firmware_kb != NULL) {
        *firmware_kb = (firmware_end + 1023) / 1024;
    }
    if (drive_kb != NULL) {
        *drive_kb = JSB_DRIVE_KB;
    }
    if (drive_state < 0) {
        drive_state = DRIVE_BYTES > 0 && firmware_end <= DRIVE_OFFSET;
    }
    return drive_state == 1;
}

uint32_t jsb_hal_disk_sectors(void)
{
    return jsb_pico_drive_check(NULL, NULL) ? DRIVE_BYTES / SECTOR_SIZE : 0;
}

static bool in_range(uint32_t sector, uint32_t count)
{
    uint32_t total = jsb_hal_disk_sectors();
    return sector < total && count <= total - sector;
}

static int write_back(void)
{
    if (!cache_dirty) {
        return 0;
    }
    uint32_t offset = DRIVE_OFFSET + (uint32_t)cache_block * BLOCK_SIZE;
    uint32_t irq = save_and_disable_interrupts();
    flash_range_erase(offset, BLOCK_SIZE);
    flash_range_program(offset, cache, BLOCK_SIZE);
    restore_interrupts(irq);
    cache_dirty = false;
    return memcmp(flash_at(offset), cache, BLOCK_SIZE) == 0 ? 0 : -1;
}

int jsb_hal_disk_read(uint32_t sector, void *buf, uint32_t count)
{
    if (!in_range(sector, count)) {
        return -1;
    }
    uint8_t *out = buf;
    for (uint32_t i = 0; i < count; i++, sector++, out += SECTOR_SIZE) {
        uint32_t block = sector / PER_BLOCK;
        uint32_t in_block = (sector % PER_BLOCK) * SECTOR_SIZE;
        if ((int32_t)block == cache_block) {
            memcpy(out, cache + in_block, SECTOR_SIZE);
        } else {
            memcpy(out, flash_at(DRIVE_OFFSET + block * BLOCK_SIZE + in_block), SECTOR_SIZE);
        }
    }
    return 0;
}

int jsb_hal_disk_write(uint32_t sector, const void *buf, uint32_t count)
{
    if (!in_range(sector, count)) {
        return -1;
    }
    const uint8_t *in = buf;
    for (uint32_t i = 0; i < count; i++, sector++, in += SECTOR_SIZE) {
        uint32_t block = sector / PER_BLOCK;
        uint32_t in_block = (sector % PER_BLOCK) * SECTOR_SIZE;
        if ((int32_t)block != cache_block) {
            if (write_back() != 0) {
                return -1;
            }
            memcpy(cache, flash_at(DRIVE_OFFSET + block * BLOCK_SIZE), BLOCK_SIZE);
            cache_block = (int32_t)block;
        }
        if (memcmp(cache + in_block, in, SECTOR_SIZE) != 0) {
            memcpy(cache + in_block, in, SECTOR_SIZE);
            cache_dirty = true;
        }
    }
    return 0;
}

int jsb_hal_disk_sync(void)
{
    return write_back();
}
