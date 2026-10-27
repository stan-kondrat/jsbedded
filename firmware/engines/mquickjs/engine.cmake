# MQuickJS: adapter + engine sources from third_party/mquickjs (kept pristine).
# The standard library lives in ROM tables (mqjs_stdlib.h, mquickjs_atom.h) that
# a host-built tool generates at configure time into ${CMAKE_BINARY_DIR}/mquickjs.
#
# Cache variables:
#   JSB_HOST_CC                 host C compiler for the generator (default: cc, gcc or clang on PATH,
#                               or $HOST_CC)
#   JSB_MQUICKJS_HEAP_SIZE      JS heap in bytes (default: per board, see below)

get_filename_component(JSB_MQUICKJS_SRC ${JSB_FIRMWARE_DIR}/../third_party/mquickjs ABSOLUTE)
set(JSB_MQUICKJS_GEN ${CMAKE_BINARY_DIR}/mquickjs)

if(NOT EXISTS ${JSB_MQUICKJS_SRC}/mquickjs.c)
    message(FATAL_ERROR "MQuickJS sources missing: git submodule update --init third_party/mquickjs")
endif()

# JS heap: one static buffer holding every JS object and the JS value stack.
# Sized to leave room for the SDK, networking and later host APIs.
if(NOT JSB_MQUICKJS_HEAP_SIZE)
    if(JSB_BOARD MATCHES "^pico2")
        set(JSB_MQUICKJS_HEAP_SIZE 131072)   # RP2350, 520 KB SRAM
    elseif(JSB_BOARD MATCHES "^pico")
        set(JSB_MQUICKJS_HEAP_SIZE 65536)    # RP2040, 264 KB SRAM
    elseif(JSB_BOARD MATCHES "^esp32s3")
        set(JSB_MQUICKJS_HEAP_SIZE 131072)   # 512 KB SRAM
    elseif(JSB_BOARD MATCHES "^esp32c3")
        set(JSB_MQUICKJS_HEAP_SIZE 98304)    # 400 KB SRAM
    else()
        set(JSB_MQUICKJS_HEAP_SIZE 65536)
    endif()
endif()

set(JSB_MQUICKJS_LIB_SOURCES
    ${JSB_MQUICKJS_SRC}/mquickjs.c
    ${JSB_MQUICKJS_SRC}/dtoa.c
    ${JSB_MQUICKJS_SRC}/libm.c
    ${JSB_MQUICKJS_SRC}/cutils.c
)

set(JSB_ENGINE_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/adapter_mquickjs.c
    ${JSB_MQUICKJS_LIB_SOURCES}
)
set(JSB_ENGINE_INCLUDE_DIRS ${JSB_MQUICKJS_GEN} ${JSB_MQUICKJS_SRC})
set(JSB_ENGINE_DEFINITIONS JSB_MQUICKJS_HEAP_SIZE=${JSB_MQUICKJS_HEAP_SIZE})

# ESP-IDF first reads component CMakeLists in script mode to collect requirements;
# generating files and setting source properties only make sense in the real configure.
if(NOT CMAKE_BUILD_EARLY_EXPANSION)
    if(NOT JSB_HOST_CC)
        if(DEFINED ENV{HOST_CC})
            set(JSB_HOST_CC $ENV{HOST_CC} CACHE FILEPATH "Host C compiler for the MQuickJS generator")
        else()
            find_program(JSB_HOST_CC NAMES cc gcc clang NO_CMAKE_FIND_ROOT_PATH)
        endif()
    endif()
    if(NOT JSB_HOST_CC)
        message(FATAL_ERROR "MQuickJS needs a host C compiler (cc, gcc or clang) to generate its stdlib; set HOST_CC")
    endif()

    # The generated tables encode JSValue words, so they must match the target word size.
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(_mqjs_word_flag -m64)
    else()
        set(_mqjs_word_flag -m32)
    endif()

    set(_mqjs_tool ${JSB_MQUICKJS_GEN}/mqjs_stdlib)
    set(_mqjs_tool_inputs
        ${JSB_MQUICKJS_SRC}/mqjs_stdlib.c
        ${JSB_MQUICKJS_SRC}/mquickjs_build.c
        ${JSB_MQUICKJS_SRC}/mquickjs_build.h
    )
    file(MAKE_DIRECTORY ${JSB_MQUICKJS_GEN})
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${_mqjs_tool_inputs})

    set(_mqjs_tool_stale FALSE)
    foreach(_mqjs_input ${_mqjs_tool_inputs})
        if(${_mqjs_input} IS_NEWER_THAN ${_mqjs_tool})
            set(_mqjs_tool_stale TRUE)
        endif()
    endforeach()
    if(_mqjs_tool_stale)
        execute_process(
            COMMAND ${JSB_HOST_CC} -O2 -D_GNU_SOURCE -fno-math-errno -fno-trapping-math
                    -o ${_mqjs_tool} ${JSB_MQUICKJS_SRC}/mqjs_stdlib.c ${JSB_MQUICKJS_SRC}/mquickjs_build.c -lm
            RESULT_VARIABLE _mqjs_rc
            ERROR_VARIABLE _mqjs_err)
        if(NOT _mqjs_rc EQUAL 0)
            message(FATAL_ERROR "building the MQuickJS stdlib generator with ${JSB_HOST_CC} failed:\n${_mqjs_err}")
        endif()
    endif()

    # Write to .tmp and copy only on change, so a reconfigure doesn't rebuild mquickjs.c.
    foreach(_mqjs_out mquickjs_atom.h mqjs_stdlib.h)
        if(_mqjs_out STREQUAL mquickjs_atom.h)
            set(_mqjs_args ${_mqjs_word_flag} -a)
        else()
            set(_mqjs_args ${_mqjs_word_flag})
        endif()
        execute_process(
            COMMAND ${_mqjs_tool} ${_mqjs_args}
            OUTPUT_FILE ${JSB_MQUICKJS_GEN}/${_mqjs_out}.tmp
            RESULT_VARIABLE _mqjs_rc
            ERROR_VARIABLE _mqjs_err)
        if(NOT _mqjs_rc EQUAL 0)
            message(FATAL_ERROR "generating ${_mqjs_out} failed:\n${_mqjs_err}")
        endif()
        execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${JSB_MQUICKJS_GEN}/${_mqjs_out}.tmp ${JSB_MQUICKJS_GEN}/${_mqjs_out})
    endforeach()

    # Upstream flags (Makefile, CONFIG_SMALL). -w keeps third-party warnings from
    # failing ESP-IDF's -Werror=all; the adapter keeps the platform's warnings.
    set_source_files_properties(${JSB_MQUICKJS_LIB_SOURCES} PROPERTIES
        COMPILE_OPTIONS "-Os;-fno-math-errno;-fno-trapping-math;-w")
endif()
