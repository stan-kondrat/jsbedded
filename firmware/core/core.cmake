# Engine-neutral core sources, shared by every platform build.
# Inputs (cache variables): JSB_BOARD, JSB_VERSION.

set(JSB_FIRMWARE_DIR ${CMAKE_CURRENT_LIST_DIR}/..)

set(JSB_CORE_SOURCES
    ${JSB_FIRMWARE_DIR}/core/src/main.c
)

set(JSB_CORE_INCLUDE_DIRS
    ${JSB_FIRMWARE_DIR}/core/include
    ${JSB_FIRMWARE_DIR}/hal/include
)

set(JSB_CORE_DEFINITIONS
    JSB_BOARD="${JSB_BOARD}"
    JSB_VERSION="${JSB_VERSION}"
)
