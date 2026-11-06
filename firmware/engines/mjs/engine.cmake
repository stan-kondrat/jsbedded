# mJS: adapter + the amalgamated mjs.c / mjs.h from third_party/mjs (kept
# pristine). mJS is GPLv2 (or commercial): images built with this engine are
# GPLv2, see README.md.
#
# CS_PLATFORM is CS_P_CUSTOM (0): mJS then includes <platform_custom.h>, which
# is in this folder.

get_filename_component(JSB_MJS_SRC ${JSB_FIRMWARE_DIR}/../third_party/mjs ABSOLUTE)

if(NOT EXISTS ${JSB_MJS_SRC}/mjs.c)
    message(FATAL_ERROR "mJS sources missing: git submodule update --init third_party/mjs")
endif()

set(JSB_ENGINE_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/adapter_mjs.c
    ${CMAKE_CURRENT_LIST_DIR}/../common/host_unsupported.c
    ${JSB_MJS_SRC}/mjs.c
)
# compat/inttypes.h: adds PRIu64 & co., missing in arm-none-eabi newlib (pico-sdk).
set(JSB_ENGINE_INCLUDE_DIRS ${JSB_FIRMWARE_DIR}/engines/include/compat ${CMAKE_CURRENT_LIST_DIR} ${JSB_MJS_SRC})
set(JSB_ENGINE_DEFINITIONS CS_PLATFORM=0)
set(JSB_ENGINE_COMPILE_OPTIONS -Os)

# mjs.c only: -w, so upstream warnings cannot fail ESP-IDF's -Werror=all. The
# adapter keeps the platform's warnings.
if(NOT CMAKE_BUILD_EARLY_EXPANSION)
    set_source_files_properties(${JSB_MJS_SRC}/mjs.c PROPERTIES COMPILE_FLAGS "-w")
endif()

set(JSB_ENGINE_LINK_LIBRARIES m)
