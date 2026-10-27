# Duktape engine: adapter + the amalgamated Duktape 2.7.0 release sources.
#
# The sources come from the official release tarball vendored in
# third_party/duktape-dist/ (the git tree has no duktape.c, and its generator,
# tools/configure.py, is Python 2 only; see third_party/README.md). At configure
# time the tarball's SHA-256 is checked, src/ is extracted to
# <build>/jsb-duktape-stage/, third_party/patches/duktape/*.patch are applied
# there in name order, and '#include "duk_config_jsb.h"' is appended to
# duk_config.h. The result is copied to <build>/jsb-duktape/, replacing only
# files whose content changed.

set(JSB_DUKTAPE_VERSION 2.7.0)
set(JSB_DUKTAPE_SHA256 90f8d2fa8b5567c6899830ddef2c03f3c27960b11aca222fa17aa7ac613c2890)
get_filename_component(JSB_DUKTAPE_TARBALL
    ${JSB_FIRMWARE_DIR}/../third_party/duktape-dist/duktape-${JSB_DUKTAPE_VERSION}.tar.xz ABSOLUTE)
get_filename_component(JSB_DUKTAPE_PATCH_DIR ${JSB_FIRMWARE_DIR}/../third_party/patches/duktape ABSOLUTE)
set(JSB_DUKTAPE_SRC ${CMAKE_BINARY_DIR}/jsb-duktape)
set(JSB_DUKTAPE_STAGE ${CMAKE_BINARY_DIR}/jsb-duktape-stage)

set(JSB_DUKTAPE_FILES duktape.c duktape.h duk_config.h)

# ESP-IDF first reads every component's CMakeLists.txt in script mode only to
# collect requirements; skip the file work there.
if(NOT CMAKE_BUILD_EARLY_EXPANSION)
    if(NOT EXISTS ${JSB_DUKTAPE_TARBALL})
        message(FATAL_ERROR "Duktape release tarball missing: ${JSB_DUKTAPE_TARBALL}")
    endif()
    file(SHA256 ${JSB_DUKTAPE_TARBALL} jsb_duk_sha256)
    if(NOT jsb_duk_sha256 STREQUAL JSB_DUKTAPE_SHA256)
        message(FATAL_ERROR "Duktape tarball checksum mismatch: ${JSB_DUKTAPE_TARBALL}\n"
                            "  expected ${JSB_DUKTAPE_SHA256}\n  actual   ${jsb_duk_sha256}")
    endif()
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${JSB_DUKTAPE_TARBALL})

    # Extract only src/ (cmake -E tar reads .tar.xz itself; no xz needed).
    file(REMOVE_RECURSE ${JSB_DUKTAPE_STAGE})
    file(MAKE_DIRECTORY ${JSB_DUKTAPE_STAGE})
    set(jsb_duk_members)
    foreach(jsb_duk_file ${JSB_DUKTAPE_FILES})
        list(APPEND jsb_duk_members duktape-${JSB_DUKTAPE_VERSION}/src/${jsb_duk_file})
    endforeach()
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xf ${JSB_DUKTAPE_TARBALL} ${jsb_duk_members}
        WORKING_DIRECTORY ${JSB_DUKTAPE_STAGE}
        RESULT_VARIABLE jsb_duk_rc
        ERROR_VARIABLE jsb_duk_err)
    set(jsb_duk_stage_src ${JSB_DUKTAPE_STAGE}/duktape-${JSB_DUKTAPE_VERSION}/src)
    if(NOT jsb_duk_rc EQUAL 0 OR NOT EXISTS ${jsb_duk_stage_src}/duktape.c)
        message(FATAL_ERROR "extracting Duktape from ${JSB_DUKTAPE_TARBALL} failed:\n${jsb_duk_err}")
    endif()

    file(GLOB jsb_duk_patches ${JSB_DUKTAPE_PATCH_DIR}/*.patch)
    list(SORT jsb_duk_patches)
    if(jsb_duk_patches)
        find_package(Git REQUIRED)
    endif()
    foreach(jsb_duk_patch ${jsb_duk_patches})
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${jsb_duk_patch})
        # The ceiling keeps git from finding the enclosing repository, so
        # "git apply" patches the staged files like a plain patch tool.
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E env GIT_CEILING_DIRECTORIES=${CMAKE_BINARY_DIR}
                    ${GIT_EXECUTABLE} apply --whitespace=nowarn ${jsb_duk_patch}
            WORKING_DIRECTORY ${jsb_duk_stage_src}
            RESULT_VARIABLE jsb_duk_rc
            ERROR_VARIABLE jsb_duk_err)
        if(NOT jsb_duk_rc EQUAL 0)
            message(FATAL_ERROR "Duktape patch does not apply: ${jsb_duk_patch}\n${jsb_duk_err}")
        endif()
    endforeach()

    # The JSBedded options go after everything in the release duk_config.h.
    file(APPEND ${jsb_duk_stage_src}/duk_config.h
        "\n/* Added by firmware/engines/duktape/engine.cmake */\n#include \"duk_config_jsb.h\"\n")

    foreach(jsb_duk_file ${JSB_DUKTAPE_FILES})
        configure_file(${jsb_duk_stage_src}/${jsb_duk_file} ${JSB_DUKTAPE_SRC}/${jsb_duk_file} COPYONLY)
    endforeach()
    configure_file(${CMAKE_CURRENT_LIST_DIR}/duk_config_jsb.h ${JSB_DUKTAPE_SRC}/duk_config_jsb.h COPYONLY)
endif()

set(JSB_ENGINE_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/adapter_duktape.c
    ${JSB_DUKTAPE_SRC}/duktape.c
)
set(JSB_ENGINE_INCLUDE_DIRS ${JSB_DUKTAPE_SRC})

# Per board: the JS heap limit (bytes Duktape may hold through the adapter's
# allocator; at the limit Duktape runs an emergency GC, then throws RangeError)
# and the C stack Duktape calls may use, measured from jsb_engine_eval(), before
# they throw RangeError. An empty heap with lightfunc built-ins is estimated at
# 25-40 KB on 32-bit targets; the limits leave room for the SDK, the console and
# later networking.
#
# pico-sdk: the core 0 stack starts at the top of SCRATCH_Y and grows through
# SCRATCH_X (unused without core 1), 8 KB in all, then into the top of main RAM,
# where the heap ends. With these limits the heap stays far below the top of
# RAM, so the stack has room in practice, but the linker does not reserve it.
# pico-sdk panics on a failed malloc (PICO_MALLOC_PANIC), another reason to keep
# Duktape well inside the heap.
#
# ESP-IDF: Duktape runs on the main task, whose stack is
# CONFIG_ESP_MAIN_TASK_STACK_SIZE (16 KB from this folder's sdkconfig.defaults).
# 4 KB of it is kept for the frames below jsb_engine_eval(), the console path
# and the compiler, whose recursion is bounded by DUK_USE_COMPILER_RECLIMIT
# rather than the stack check.
if(JSB_BOARD STREQUAL "pico")
    set(jsb_duk_heap_limit 98304)       # 96 KB of 264 KB SRAM
    set(jsb_duk_stack_limit 12288)
elseif(JSB_BOARD STREQUAL "pico_w")
    set(jsb_duk_heap_limit 81920)       # 80 KB: room for the CYW43 driver and lwIP
    set(jsb_duk_stack_limit 12288)
elseif(JSB_BOARD STREQUAL "pico2")
    set(jsb_duk_heap_limit 262144)      # 256 KB of 520 KB SRAM
    set(jsb_duk_stack_limit 12288)
elseif(JSB_BOARD STREQUAL "pico2_w")
    set(jsb_duk_heap_limit 229376)      # 224 KB: room for the CYW43 driver and lwIP
    set(jsb_duk_stack_limit 12288)
elseif(JSB_BOARD STREQUAL "esp32s3_devkitc")
    set(jsb_duk_heap_limit 196608)      # 192 KB of ~380 KB free internal heap
elseif(JSB_BOARD STREQUAL "esp32c3_devkitm")
    set(jsb_duk_heap_limit 131072)      # 128 KB of ~300 KB free heap
else()
    set(jsb_duk_heap_limit 98304)
    set(jsb_duk_stack_limit 12288)
endif()
if(NOT jsb_duk_stack_limit)
    if(CONFIG_ESP_MAIN_TASK_STACK_SIZE)
        math(EXPR jsb_duk_stack_limit "${CONFIG_ESP_MAIN_TASK_STACK_SIZE} - 4096")
    endif()
    if(NOT jsb_duk_stack_limit OR jsb_duk_stack_limit LESS 1024)
        set(jsb_duk_stack_limit 1024)
    endif()
endif()

set(JSB_ENGINE_DEFINITIONS
    JSB_DUKTAPE_HEAP_LIMIT=${jsb_duk_heap_limit}
    JSB_DUKTAPE_STACK_LIMIT=${jsb_duk_stack_limit}
)

# For all engine sources: size over speed (comes after, so overrides, the SDKs'
# -O3 / -Og).
set(JSB_ENGINE_COMPILE_OPTIONS -Os)

# duktape.c only: -w, so upstream warnings cannot fail ESP-IDF's -Werror=all
# (GCC drops inhibited warnings before promoting them to errors).
# COMPILE_FLAGS, because jsb_apply_engine_options() replaces COMPILE_OPTIONS on
# every engine source; the adapter keeps the platform's warnings.
if(NOT CMAKE_BUILD_EARLY_EXPANSION)
    set_source_files_properties(${JSB_DUKTAPE_SRC}/duktape.c PROPERTIES COMPILE_FLAGS -w)
endif()

set(JSB_ENGINE_LINK_LIBRARIES m)
