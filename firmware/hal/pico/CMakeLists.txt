# Firmware image on the Raspberry Pi Pico SDK. Configured by platform.mk:
#   PICO_SDK_PATH, PICO_BOARD, JSB_BOARD, JSB_VERSION, JSB_ENGINE, JSB_BOARD_LED, JSB_SCRIPT,
#   JSB_DRIVE_KB (flash at the end set aside for the script drive; 0 = none)
cmake_minimum_required(VERSION 3.13)

include(${PICO_SDK_PATH}/pico_sdk_init.cmake)

project(jsbedded C CXX ASM)
set(CMAKE_C_STANDARD 11)

pico_sdk_init()

if(NOT JSB_DRIVE_KB)
    set(JSB_DRIVE_KB 0)
endif()
set(JSB_STORAGE ON) # script drive: storage/ + FatFs, on flash_disk.c
include(${CMAKE_CURRENT_LIST_DIR}/../../core/core.cmake)

add_executable(jsbedded
    ${JSB_CORE_SOURCES}
    hal_pico.c
    flash_disk.c
    devmode.c
    usb_drive.c
)
jsb_apply_engine_options()
# usb/ holds tusb_config.h: CDC + MSC instead of pico_stdio_usb's CDC-only config.
target_include_directories(jsbedded PRIVATE ${JSB_CORE_INCLUDE_DIRS}
    ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/usb)
target_compile_definitions(jsbedded PRIVATE ${JSB_CORE_DEFINITIONS}
    JSB_DRIVE_KB=${JSB_DRIVE_KB}
    # Wait up to 2 s for a USB serial host at boot, so the first lines aren't lost.
    PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=2000
    # The app links tinyusb_device for the drive, which turns these off by default;
    # pico_stdio_usb should still start TinyUSB, run tud_task() in the background
    # and reset to BOOTSEL when the host sets 1200 baud.
    PICO_STDIO_USB_ENABLE_TINYUSB_INIT=1
    PICO_STDIO_USB_ENABLE_IRQ_BACKGROUND_TASK=1
    PICO_ENABLE_USB_RESET_VIA_BAUD_RATE=1)
target_link_libraries(jsbedded pico_stdlib pico_unique_id hardware_flash tinyusb_device
    ${JSB_CORE_LINK_LIBRARIES})

pico_set_program_name(jsbedded "JSBedded")
pico_set_program_version(jsbedded "${JSB_VERSION}")
pico_enable_stdio_usb(jsbedded 1)
pico_enable_stdio_uart(jsbedded 1)
pico_add_extra_outputs(jsbedded)
