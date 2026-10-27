# Core and engine sources, shared by every platform build. Include it from the
# directory that creates the firmware target, then call jsb_apply_engine_options().
# Inputs (cache variables): JSB_BOARD, JSB_VERSION, JSB_ENGINE.
#
# engines/<engine>/engine.cmake sets:
#   JSB_ENGINE_SOURCES          adapter + engine sources (required)
#   JSB_ENGINE_INCLUDE_DIRS     extra include directories
#   JSB_ENGINE_DEFINITIONS      extra compile definitions
#   JSB_ENGINE_COMPILE_OPTIONS  options for the engine sources only (e.g. -Wno-...)
#   JSB_ENGINE_LINK_LIBRARIES   extra libraries (e.g. m)

set(JSB_FIRMWARE_DIR ${CMAKE_CURRENT_LIST_DIR}/..)

if(NOT JSB_ENGINE)
    set(JSB_ENGINE none)
endif()
if(NOT EXISTS ${JSB_FIRMWARE_DIR}/engines/${JSB_ENGINE}/engine.cmake)
    message(FATAL_ERROR "unknown JSB_ENGINE=${JSB_ENGINE}: no engines/${JSB_ENGINE}/engine.cmake")
endif()

set(JSB_ENGINE_SOURCES)
set(JSB_ENGINE_INCLUDE_DIRS)
set(JSB_ENGINE_DEFINITIONS)
set(JSB_ENGINE_COMPILE_OPTIONS)
set(JSB_ENGINE_LINK_LIBRARIES)
include(${JSB_FIRMWARE_DIR}/engines/${JSB_ENGINE}/engine.cmake)

set(JSB_CORE_SOURCES
    ${JSB_FIRMWARE_DIR}/core/src/main.c
    ${JSB_ENGINE_SOURCES}
)

set(JSB_CORE_INCLUDE_DIRS
    ${JSB_FIRMWARE_DIR}/core/include
    ${JSB_FIRMWARE_DIR}/hal/include
    ${JSB_FIRMWARE_DIR}/engines/include
    ${JSB_ENGINE_INCLUDE_DIRS}
)

set(JSB_CORE_DEFINITIONS
    JSB_BOARD="${JSB_BOARD}"
    JSB_VERSION="${JSB_VERSION}"
    JSB_ENGINE="${JSB_ENGINE}"
    ${JSB_ENGINE_DEFINITIONS}
)

set(JSB_CORE_LINK_LIBRARIES ${JSB_ENGINE_LINK_LIBRARIES})

# Source properties are per directory, so this runs where the target is created.
function(jsb_apply_engine_options)
    if(JSB_ENGINE_COMPILE_OPTIONS)
        set_source_files_properties(${JSB_ENGINE_SOURCES} PROPERTIES
            COMPILE_OPTIONS "${JSB_ENGINE_COMPILE_OPTIONS}")
    endif()
endfunction()
