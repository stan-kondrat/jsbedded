/*
 * USB device on the Pico: the console (CDC, run by pico_stdio_usb) and the
 * script drive (MSC on flash_disk.c), as one composite device.
 *
 * The drive reports "no medium" until core calls jsb_hal_drive_attach(), after
 * it has read the script, so the PC and the firmware never use the FAT volume at
 * the same time. Writes are written back to flash at the end of each WRITE(10)
 * command, on SYNCHRONIZE CACHE and on eject.
 *
 * USB IDs: Raspberry Pi's VID with the Pico SDK's CDC PID, as the SDK's own
 * stdio descriptors use; bcdDevice differs from the SDK's (0x0100) because the
 * interfaces differ and Windows caches them per VID/PID/bcdDevice. A PID of our
 * own (raspberrypi/usb-pid) is a follow-up.
 */
#include <string.h>

#include "pico/unique_id.h"
#include "tusb.h"

#include "jsb/hal.h"

#ifndef JSB_USB_VID
#define JSB_USB_VID 0x2e8a
#endif
#ifndef JSB_USB_PID
#if PICO_RP2040
#define JSB_USB_PID 0x000a
#else
#define JSB_USB_PID 0x0009
#endif
#endif
#define JSB_USB_BCD_DEVICE 0x0200

/* --- descriptors --- */

enum {
    ITF_CDC = 0,
    ITF_CDC_DATA,
    ITF_MSC,
    ITF_COUNT
};

enum {
    STR_LANGID = 0,
    STR_MANUFACTURER,
    STR_PRODUCT,
    STR_SERIAL,
    STR_CDC,
    STR_MSC,
    STR_COUNT
};

#define EP_CDC_NOTIF 0x81
#define EP_CDC_OUT   0x02
#define EP_CDC_IN    0x82
#define EP_MSC_OUT   0x03
#define EP_MSC_IN    0x83

#define CONFIG_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_MSC_DESC_LEN)

static const tusb_desc_device_t device_desc = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    /* Interface Association Descriptors, required for CDC in a composite device */
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = JSB_USB_VID,
    .idProduct = JSB_USB_PID,
    .bcdDevice = JSB_USB_BCD_DEVICE,
    .iManufacturer = STR_MANUFACTURER,
    .iProduct = STR_PRODUCT,
    .iSerialNumber = STR_SERIAL,
    .bNumConfigurations = 1,
};

static const uint8_t config_desc[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_COUNT, 0, CONFIG_LEN, 0, 250),
    TUD_CDC_DESCRIPTOR(ITF_CDC, STR_CDC, EP_CDC_NOTIF, 8, EP_CDC_OUT, EP_CDC_IN, 64),
    TUD_MSC_DESCRIPTOR(ITF_MSC, STR_MSC, EP_MSC_OUT, EP_MSC_IN, 64),
};

const uint8_t *tud_descriptor_device_cb(void)
{
    return (const uint8_t *)&device_desc;
}

const uint8_t *tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index;
    return config_desc;
}

const uint16_t *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void)langid;
    static uint16_t desc[32 + 1];
    static char serial[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1];
    const char *str;
    size_t len;

    switch (index) {
    case STR_LANGID:
        desc[1] = 0x0409; /* English (US) */
        len = 1;
        desc[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * len + 2));
        return desc;
    case STR_MANUFACTURER:
        str = "JSBedded";
        break;
    case STR_PRODUCT:
        str = "JSBedded " JSB_BOARD;
        break;
    case STR_SERIAL:
        if (serial[0] == '\0') {
            pico_get_unique_board_id_string(serial, sizeof serial);
        }
        str = serial;
        break;
    case STR_CDC:
        str = "JSBedded console";
        break;
    case STR_MSC:
        str = "JSBedded drive";
        break;
    default:
        return NULL;
    }
    len = strlen(str);
    if (len > 32) {
        len = 32;
    }
    for (size_t i = 0; i < len; i++) {
        desc[1 + i] = (uint8_t)str[i];
    }
    desc[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * len + 2));
    return desc;
}

/* --- mass storage --- */

static volatile bool attached;
static volatile bool ejected;

void jsb_hal_drive_attach(void)
{
    attached = jsb_hal_disk_sectors() > 0;
}

void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4])
{
    (void)lun;
    memcpy(vendor_id, "JSBedded", 8);
    memcpy(product_id, "Script drive    ", 16);
    memcpy(product_rev, "1.0 ", 4);
}

bool tud_msc_test_unit_ready_cb(uint8_t lun)
{
    if (!attached || ejected) {
        tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, 0x3a, 0x00); /* medium not present */
        return false;
    }
    return true;
}

void tud_msc_capacity_cb(uint8_t lun, uint32_t *block_count, uint16_t *block_size)
{
    (void)lun;
    *block_count = jsb_hal_disk_sectors();
    *block_size = JSB_HAL_DISK_SECTOR_SIZE;
}

bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject)
{
    (void)lun;
    (void)power_condition;
    if (load_eject) {
        if (start) {
            ejected = false;
        } else {
            jsb_hal_disk_sync();
            ejected = true;
        }
    }
    return true;
}

bool tud_msc_is_writable_cb(uint8_t lun)
{
    (void)lun;
    return true;
}

static int32_t sectors_of(uint32_t offset, uint32_t bufsize)
{
    if (!attached || offset != 0 || bufsize % JSB_HAL_DISK_SECTOR_SIZE != 0) {
        return -1;
    }
    return (int32_t)(bufsize / JSB_HAL_DISK_SECTOR_SIZE);
}

int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize)
{
    int32_t count = sectors_of(offset, bufsize);
    if (count < 0 || jsb_hal_disk_read(lba, buffer, (uint32_t)count) != 0) {
        tud_msc_set_sense(lun, SCSI_SENSE_MEDIUM_ERROR, 0x11, 0x00); /* unrecovered read error */
        return -1;
    }
    return (int32_t)bufsize;
}

int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize)
{
    int32_t count = sectors_of(offset, bufsize);
    if (count < 0 || jsb_hal_disk_write(lba, buffer, (uint32_t)count) != 0) {
        tud_msc_set_sense(lun, SCSI_SENSE_MEDIUM_ERROR, 0x03, 0x00); /* peripheral device write fault */
        return -1;
    }
    return (int32_t)bufsize;
}

void tud_msc_write10_complete_cb(uint8_t lun)
{
    (void)lun;
    jsb_hal_disk_sync();
}

int32_t tud_msc_scsi_cb(uint8_t lun, const uint8_t scsi_cmd[16], void *buffer, uint16_t bufsize)
{
    (void)buffer;
    (void)bufsize;
    switch (scsi_cmd[0]) {
    case 0x35: /* SYNCHRONIZE CACHE (10) */
        return jsb_hal_disk_sync() == 0 ? 0 : -1;
    default:
        tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00); /* invalid command */
        return -1;
    }
}
