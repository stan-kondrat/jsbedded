/*
 * FatFs disk I/O layer (third_party/fatfs/diskio.h) on the HAL's script-drive
 * sectors. One volume (pdrv 0), 512-byte sectors.
 */
#include "ff.h"
#include "diskio.h"

#include "jsb/hal.h"

DSTATUS disk_status(BYTE pdrv)
{
    return pdrv == 0 && jsb_hal_disk_sectors() > 0 ? 0 : STA_NOINIT;
}

DSTATUS disk_initialize(BYTE pdrv)
{
    return disk_status(pdrv);
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    if (pdrv != 0) {
        return RES_PARERR;
    }
    return jsb_hal_disk_read((uint32_t)sector, buff, count) == 0 ? RES_OK : RES_ERROR;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
    if (pdrv != 0) {
        return RES_PARERR;
    }
    return jsb_hal_disk_write((uint32_t)sector, buff, count) == 0 ? RES_OK : RES_ERROR;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    if (pdrv != 0) {
        return RES_PARERR;
    }
    switch (cmd) {
    case CTRL_SYNC:
        return jsb_hal_disk_sync() == 0 ? RES_OK : RES_ERROR;
    case GET_SECTOR_COUNT:
        *(LBA_t *)buff = jsb_hal_disk_sectors();
        return RES_OK;
    case GET_SECTOR_SIZE:
        *(WORD *)buff = JSB_HAL_DISK_SECTOR_SIZE;
        return RES_OK;
    case GET_BLOCK_SIZE:
        *(DWORD *)buff = 1; /* erase block unknown to FatFs; the HAL handles it */
        return RES_OK;
    default:
        return RES_PARERR;
    }
}
