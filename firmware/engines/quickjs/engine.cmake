# QuickJS engine: adapter + the QuickJS core (no quickjs-libc, which needs an OS).
#
# third_party/quickjs stays pristine: its sources are copied to
# <build>/jsb-quickjs/quickjs/ and third_party/patches/quickjs/*.patch are
# applied there, in name order. Only <build>/jsb-quickjs goes on the include
# path (the adapter includes "quickjs/quickjs.h"), so QuickJS's list.h and
# cutils.h cannot shadow SDK headers of the same name.

get_filename_component(JSB_QUICKJS_UPSTREAM ${JSB_FIRMWARE_DIR}/../third_party/quickjs ABSOLUTE)
get_filename_component(JSB_QUICKJS_PATCH_DIR ${JSB_FIRMWARE_DIR}/../third_party/patches/quickjs ABSOLUTE)
set(JSB_QUICKJS_ROOT ${CMAKE_BINARY_DIR}/jsb-quickjs)
set(JSB_QUICKJS_SRC ${JSB_QUICKJS_ROOT}/quickjs)
set(JSB_QUICKJS_STAGE ${CMAKE_BINARY_DIR}/jsb-quickjs-stage)

set(JSB_QUICKJS_FILES
    VERSION
    cutils.c cutils.h
    dtoa.c dtoa.h
    libregexp.c libregexp.h libregexp-opcode.h
    libunicode.c libunicode.h libunicode-table.h
    list.h
    quickjs.c quickjs.h quickjs-atom.h quickjs-opcode.h
)

# ESP-IDF first reads every component's CMakeLists.txt in script mode only to
# collect requirements; skip the file work there.
if(NOT CMAKE_BUILD_EARLY_EXPANSION)
    if(NOT EXISTS ${JSB_QUICKJS_UPSTREAM}/quickjs.c)
        message(FATAL_ERROR "QuickJS missing: git submodule update --init third_party/quickjs")
    endif()

    # Stage a fresh copy and patch it, then update the build copy only where
    # the content changed, so unchanged files keep their timestamps.
    file(REMOVE_RECURSE ${JSB_QUICKJS_STAGE})
    file(MAKE_DIRECTORY ${JSB_QUICKJS_STAGE})
    foreach(jsb_qjs_file ${JSB_QUICKJS_FILES})
        file(COPY ${JSB_QUICKJS_UPSTREAM}/${jsb_qjs_file} DESTINATION ${JSB_QUICKJS_STAGE})
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${JSB_QUICKJS_UPSTREAM}/${jsb_qjs_file})
    endforeach()

    file(GLOB jsb_qjs_patches ${JSB_QUICKJS_PATCH_DIR}/*.patch)
    list(SORT jsb_qjs_patches)
    if(jsb_qjs_patches)
        find_package(Git REQUIRED)
    endif()
    foreach(jsb_qjs_patch ${jsb_qjs_patches})
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${jsb_qjs_patch})
        # The ceiling keeps git from finding the enclosing repository, so
        # "git apply" patches the staged files like a plain patch tool.
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E env GIT_CEILING_DIRECTORIES=${CMAKE_BINARY_DIR}
                    ${GIT_EXECUTABLE} apply --whitespace=nowarn ${jsb_qjs_patch}
            WORKING_DIRECTORY ${JSB_QUICKJS_STAGE}
            RESULT_VARIABLE jsb_qjs_rc
            ERROR_VARIABLE jsb_qjs_err)
        if(NOT jsb_qjs_rc EQUAL 0)
            message(FATAL_ERROR "QuickJS patch does not apply: ${jsb_qjs_patch}\n${jsb_qjs_err}")
        endif()
    endforeach()

    foreach(jsb_qjs_file ${JSB_QUICKJS_FILES})
        configure_file(${JSB_QUICKJS_STAGE}/${jsb_qjs_file} ${JSB_QUICKJS_SRC}/${jsb_qjs_file} COPYONLY)
    endforeach()
    file(STRINGS ${JSB_QUICKJS_SRC}/VERSION JSB_QUICKJS_VERSION LIMIT_COUNT 1)
endif()

set(JSB_ENGINE_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/adapter_quickjs.c
    ${JSB_QUICKJS_SRC}/quickjs.c
    ${JSB_QUICKJS_SRC}/dtoa.c
    ${JSB_QUICKJS_SRC}/libregexp.c
    ${JSB_QUICKJS_SRC}/libunicode.c
    ${JSB_QUICKJS_SRC}/cutils.c
)
set(JSB_ENGINE_INCLUDE_DIRS ${JSB_QUICKJS_ROOT})

# Per board: the JS heap limit (bytes QuickJS takes from the adapter's
# allocator) and the C stack QuickJS may use before it throws a RangeError.
# An empty QuickJS context is estimated at 150-200 KB on 32-bit targets; the
# limits leave room for the SDK, the console and later networking.
#
# pico-sdk: the core 0 stack starts at the top of SCRATCH_Y and grows through
# SCRATCH_X (unused without core 1), 8 KB in all, then into the top of main RAM,
# where the heap ends. With these limits the heap stays far below the top of
# RAM, so 16 KB of stack is free in practice, but the linker does not reserve
# it. pico-sdk panics on a failed malloc (PICO_MALLOC_PANIC), another reason to
# keep QuickJS well inside the heap.
#
# ESP-IDF: QuickJS runs on the main task, whose stack is
# CONFIG_ESP_MAIN_TASK_STACK_SIZE (3584 bytes by default; parsing a script needs
# ~16 KB). 3 KB of it is kept for the frames below jsb_engine_eval() and the
# console path.
if(JSB_BOARD STREQUAL "pico2")
    set(jsb_qjs_memory_limit 393216)    # 384 KB of 520 KB SRAM
    set(jsb_qjs_stack_size 16384)
elseif(JSB_BOARD STREQUAL "pico2_w")
    set(jsb_qjs_memory_limit 327680)    # 320 KB: room for the CYW43 driver and lwIP
    set(jsb_qjs_stack_size 16384)
elseif(JSB_BOARD MATCHES "^pico")
    set(jsb_qjs_memory_limit 163840)    # RP2040 (unsupported): 160 KB of 264 KB SRAM
    set(jsb_qjs_stack_size 8192)
elseif(JSB_BOARD STREQUAL "esp32s3_devkitc")
    set(jsb_qjs_memory_limit 262144)    # 256 KB of ~380 KB free internal heap
elseif(JSB_BOARD STREQUAL "esp32c3_devkitm")
    set(jsb_qjs_memory_limit 196608)    # unsupported: 192 KB of ~300 KB free heap
else()
    set(jsb_qjs_memory_limit 262144)
    set(jsb_qjs_stack_size 16384)
endif()
if(NOT jsb_qjs_stack_size)
    if(CONFIG_ESP_MAIN_TASK_STACK_SIZE)
        math(EXPR jsb_qjs_stack_size "${CONFIG_ESP_MAIN_TASK_STACK_SIZE} - 3072")
    endif()
    if(NOT jsb_qjs_stack_size OR jsb_qjs_stack_size LESS 512)
        set(jsb_qjs_stack_size 512)
    endif()
endif()

set(JSB_ENGINE_DEFINITIONS
    JSB_QUICKJS_MEMORY_LIMIT=${jsb_qjs_memory_limit}
    JSB_QUICKJS_STACK_SIZE=${jsb_qjs_stack_size}
    CONFIG_VERSION="${JSB_QUICKJS_VERSION}"
    CONFIG_NO_ATOMICS               # 0001-embedded-config-switches.patch: no pthreads
    CONFIG_NO_MALLOC_USABLE_SIZE    # same patch; the adapter brings its own allocator
    CONFIG_NO_TM_GMTOFF             # 0002-no-tm-gmtoff.patch: newlib struct tm has no tm_gmtoff
)

# For all engine sources: size over speed (comes after, so overrides, the SDKs'
# -O3 / -Og), and -fwrapv, which QuickJS relies on for signed overflow
# (upstream Makefile).
set(JSB_ENGINE_COMPILE_OPTIONS -Os -fwrapv)

# QuickJS's own sources only: -w, so upstream warnings cannot fail ESP-IDF's
# -Werror=all (GCC drops inhibited warnings before promoting them to errors).
# COMPILE_FLAGS, because jsb_apply_engine_options() replaces COMPILE_OPTIONS on
# every engine source; the adapter keeps the platform's warnings.
if(NOT CMAKE_BUILD_EARLY_EXPANSION)
    set_source_files_properties(
        ${JSB_QUICKJS_SRC}/quickjs.c
        ${JSB_QUICKJS_SRC}/dtoa.c
        ${JSB_QUICKJS_SRC}/libregexp.c
        ${JSB_QUICKJS_SRC}/libunicode.c
        ${JSB_QUICKJS_SRC}/cutils.c
        PROPERTIES COMPILE_FLAGS -w)
endif()

set(JSB_ENGINE_LINK_LIBRARIES m)
