# Firmware image on the Raspberry Pi Pico SDK. Configured by platform.mk:
#   PICO_SDK_PATH, PICO_BOARD, JSB_BOARD, JSB_VERSION, JSB_ENGINE, JSB_BOARD_LED, JSB_SCRIPT
cmake_minimum_required(VERSION 3.13)

include(${PICO_SDK_PATH}/pico_sdk_init.cmake)

project(jsbedded C CXX ASM)
set(CMAKE_C_STANDARD 11)

pico_sdk_init()

include(${CMAKE_CURRENT_LIST_DIR}/../../core/core.cmake)

add_executable(jsbedded
    ${JSB_CORE_SOURCES}
    hal_pico.c
)
jsb_apply_engine_options()
target_include_directories(jsbedded PRIVATE ${JSB_CORE_INCLUDE_DIRS})
target_compile_definitions(jsbedded PRIVATE ${JSB_CORE_DEFINITIONS}
    # Wait up to 2 s for a USB serial host at boot, so the first lines aren't lost.
    PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=2000)
target_link_libraries(jsbedded pico_stdlib ${JSB_CORE_LINK_LIBRARIES})

pico_set_program_name(jsbedded "JSBedded")
pico_set_program_version(jsbedded "${JSB_VERSION}")
pico_enable_stdio_usb(jsbedded 1)
pico_enable_stdio_uart(jsbedded 1)
pico_add_extra_outputs(jsbedded)
