# Firmware image on the Raspberry Pi Pico SDK. Configured by platform.mk:
#   PICO_SDK_PATH, PICO_BOARD, JSB_BOARD, JSB_VERSION
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
target_include_directories(jsbedded PRIVATE ${JSB_CORE_INCLUDE_DIRS})
target_compile_definitions(jsbedded PRIVATE ${JSB_CORE_DEFINITIONS})
target_link_libraries(jsbedded pico_stdlib)

pico_set_program_name(jsbedded "JSBedded")
pico_set_program_version(jsbedded "${JSB_VERSION}")
pico_enable_stdio_usb(jsbedded 1)
pico_enable_stdio_uart(jsbedded 1)
pico_add_extra_outputs(jsbedded)
