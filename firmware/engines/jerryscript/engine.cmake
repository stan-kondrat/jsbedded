# JerryScript: adapter (with the jerry_port_* layer) + jerry-core from
# third_party/jerryscript (kept pristine). jerry-port, jerry-ext, jerry-math and
# the upstream CMake build are not used: the adapter implements the port
# functions on the HAL and the engine is configured with the defines below.
#
# Cache variables:
#   JSB_JERRYSCRIPT_HEAP_KB     JS heap in KB (default: per board, see below; at most 512)

get_filename_component(JSB_JERRYSCRIPT_SRC ${JSB_FIRMWARE_DIR}/../third_party/jerryscript ABSOLUTE)
set(JSB_JERRYSCRIPT_CORE ${JSB_JERRYSCRIPT_SRC}/jerry-core)

if(NOT EXISTS ${JSB_JERRYSCRIPT_CORE}/api/jerryscript.c)
    message(FATAL_ERROR "JerryScript sources missing: git submodule update --init third_party/jerryscript")
endif()

# JS heap: one static buffer (jerry_global_heap in .bss) holding every JS
# object, string and byte code. The linker checks that it fits. 16-bit
# compressed pointers cap it at 512 KB. Sized to leave room for the SDK,
# networking and later host APIs.
if(NOT JSB_JERRYSCRIPT_HEAP_KB)
    if(JSB_BOARD STREQUAL "pico2")
        set(JSB_JERRYSCRIPT_HEAP_KB 256)    # RP2350, 520 KB SRAM
    elseif(JSB_BOARD STREQUAL "pico2_w")
        set(JSB_JERRYSCRIPT_HEAP_KB 224)    # RP2350: room for the CYW43 driver and lwIP
    elseif(JSB_BOARD STREQUAL "pico")
        set(JSB_JERRYSCRIPT_HEAP_KB 128)    # RP2040, 264 KB SRAM
    elseif(JSB_BOARD STREQUAL "pico_w")
        set(JSB_JERRYSCRIPT_HEAP_KB 96)     # RP2040: room for the CYW43 driver and lwIP
    elseif(JSB_BOARD MATCHES "^esp32s3")
        set(JSB_JERRYSCRIPT_HEAP_KB 192)    # 512 KB SRAM; Wi-Fi needs internal heap
    elseif(JSB_BOARD MATCHES "^esp32c3")
        set(JSB_JERRYSCRIPT_HEAP_KB 128)    # 400 KB SRAM
    else()
        set(JSB_JERRYSCRIPT_HEAP_KB 96)
    endif()
endif()

# C stack JerryScript may use before it throws RangeError (JERRY_STACK_LIMIT,
# in KB, measured from jerry_init()).
#
# pico-sdk: the core 0 stack is SCRATCH_Y + SCRATCH_X (8 KB, core 1 unused),
# then grows into the top of main RAM, where malloc's heap ends. The JS heap is
# static, so malloc stays small and that top stays free in practice, but the
# linker does not reserve it.
#
# ESP-IDF: JerryScript runs on the main task. sdkconfig.defaults in this folder
# sets CONFIG_ESP_MAIN_TASK_STACK_SIZE=16384; 3 KB of it is kept for the frames
# below jsb_engine_eval() and the console path.
if(JSB_BOARD MATCHES "^pico2")
    set(jsb_jerry_stack_kb 16)
elseif(JSB_BOARD MATCHES "^pico")
    set(jsb_jerry_stack_kb 8)
elseif(CONFIG_ESP_MAIN_TASK_STACK_SIZE)
    math(EXPR jsb_jerry_stack_kb "(${CONFIG_ESP_MAIN_TASK_STACK_SIZE} - 3072) / 1024")
endif()
if(NOT jsb_jerry_stack_kb OR jsb_jerry_stack_kb LESS 1)
    set(jsb_jerry_stack_kb 1)
endif()

# Every jerry-core source; upstream's jerry-core/CMakeLists.txt lists the same
# 200 files at v3.0.0.
file(GLOB_RECURSE JSB_JERRYSCRIPT_LIB_SOURCES ${JSB_JERRYSCRIPT_CORE}/*.c)
list(SORT JSB_JERRYSCRIPT_LIB_SOURCES)

set(JSB_ENGINE_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/adapter_jerryscript.c
    ${JSB_JERRYSCRIPT_LIB_SOURCES}
)
# Public headers only (jerryscript*.h). jerry-core's private headers have
# generic names (config.h, common.h, debugger.h), so they go on the jerry-core
# sources alone, below.
set(JSB_ENGINE_INCLUDE_DIRS ${JSB_JERRYSCRIPT_CORE}/include)

# Configuration: the es.next profile (every built-in, the default) with these
# changes from jerry-core/config.h. They are target-wide so the adapter sees
# the same configuration as the engine.
set(JSB_ENGINE_DEFINITIONS
    JSB_JERRYSCRIPT_HEAP_KB=${JSB_JERRYSCRIPT_HEAP_KB}
    JERRY_NDEBUG                    # no engine assertions (upstream sets it for non-Debug builds)
    JERRY_GLOBAL_HEAP_SIZE=${JSB_JERRYSCRIPT_HEAP_KB}
    JERRY_CPOINTER_32_BIT=0         # 16-bit compressed pointers: heap <= 512 KB
    JERRY_SYSTEM_ALLOCATOR=0        # static heap, not malloc
    JERRY_EXTERNAL_CONTEXT=0        # static context; one runtime at a time
    JERRY_STACK_LIMIT=${jsb_jerry_stack_kb}
    JERRY_ERROR_MESSAGES=1          # "x is not defined" instead of empty messages
    JERRY_LINE_INFO=1               # error.stack with file:line:column
    JERRY_MODULE_SYSTEM=0           # no import/export until there is a filesystem
    JERRY_PARSER=1
    JERRY_SNAPSHOT_EXEC=0
    JERRY_SNAPSHOT_SAVE=0
    JERRY_DEBUGGER=0
    JERRY_LOGGING=0
    JERRY_MEM_STATS=0
    JERRY_VM_HALT=0
    JERRY_VM_THROW=0
)

# For all engine sources: size over speed (comes after, so overrides, the SDKs'
# -O3 / -Og).
set(JSB_ENGINE_COMPILE_OPTIONS -Os)

# jerry-core sources only: its private include directories as -iquote, so its
# headers are found first for "..." includes and never shadow SDK headers for
# other sources; and -w, so upstream warnings cannot fail ESP-IDF's -Werror=all.
# COMPILE_FLAGS, because jsb_apply_engine_options() replaces COMPILE_OPTIONS on
# every engine source; the adapter keeps the platform's warnings.
if(NOT CMAKE_BUILD_EARLY_EXPANSION)
    set(jsb_jerry_flags "-w")
    foreach(jsb_jerry_dir
            . api debugger ecma/base ecma/builtin-objects ecma/builtin-objects/typedarray
            ecma/operations jcontext jmem jrt lit parser/js parser/regexp vm)
        string(APPEND jsb_jerry_flags " -iquote ${JSB_JERRYSCRIPT_CORE}/${jsb_jerry_dir}")
    endforeach()
    set_source_files_properties(${JSB_JERRYSCRIPT_LIB_SOURCES} PROPERTIES COMPILE_FLAGS "${jsb_jerry_flags}")
endif()

set(JSB_ENGINE_LINK_LIBRARIES m)
