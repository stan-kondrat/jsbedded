/*
 * TinyUSB device configuration for the Pico: USB serial (CDC) for the console
 * plus mass storage (MSC) for the script drive. pico_stdio_usb still runs the
 * console and calls tud_task() in the background (see CMakeLists.txt);
 * usb_drive.c provides the descriptors and the MSC callbacks.
 * CFG_TUSB_MCU and CFG_TUSB_OS come from the SDK's tinyusb CMake target.
 */
#ifndef JSB_TUSB_CONFIG_H
#define JSB_TUSB_CONFIG_H

#define CFG_TUSB_RHPORT0_MODE   (OPT_MODE_DEVICE)
#define CFG_TUD_ENABLED         1

#ifndef CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_SECTION
#endif
#ifndef CFG_TUSB_MEM_ALIGN
#define CFG_TUSB_MEM_ALIGN      __attribute__((aligned(4)))
#endif

#define CFG_TUD_ENDPOINT0_SIZE  64

#define CFG_TUD_CDC             1
#define CFG_TUD_MSC             1
#define CFG_TUD_HID             0
#define CFG_TUD_MIDI            0
#define CFG_TUD_VENDOR          0

#define CFG_TUD_CDC_RX_BUFSIZE  256
#define CFG_TUD_CDC_TX_BUFSIZE  256
#define CFG_TUD_CDC_EP_BUFSIZE  64

/* One 512-byte sector per read/write callback (usb_drive.c relies on it). */
#define CFG_TUD_MSC_EP_BUFSIZE  512

#endif
