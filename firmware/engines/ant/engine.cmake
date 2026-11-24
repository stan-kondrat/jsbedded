# Ant (theMackabu/ant): the Silver interpreter on its embed profile
# (-DANT_EMBED=1, include/embed.h after third_party/patches/ant/), no JIT.
#
# third_party/ant stays pristine: include/, src/, sources.json and
# packages/wasm/ are copied to <build>/jsb-ant/ant/ and
# third_party/patches/ant/*.patch are applied there, in name order. The copy
# is redone only when the submodule commit or a patch changes.
#
# Ant's headers need Clang (constexpr from string literals, C23), so the core,
# its dependencies and the adapter are compiled by Clang into objects that the
# platform toolchain links. The C library and libstdc++ headers come from the
# platform compiler, so the objects match what it links.
#
# Configure time: downloads the pinned dependencies (SHA-256 checked) into
# ${CMAKE_BINARY_DIR}/jsb-ant-deps and applies Ant's pcre2 patch and
# third_party/patches/<dependency>/ (double-conversion, crprintf).
#
# Cache variables:
#   JSB_ANT_CC, JSB_ANT_CXX   Clang C / C++ compilers (default: clang, clang++)
#   JSB_ANT_TARGET_FLAGS      target flags for them (default: Cortex-M0+, RP2040)
#   JSB_ANT_DEPS_DIR          dependency cache (default: <build>/jsb-ant-deps)

if(NOT JSB_BOARD STREQUAL "pico")
    message(FATAL_ERROR "ENGINE=ant supports BOARD=pico (RP2040) only so far; see engines/ant/README.md")
endif()

get_filename_component(JSB_ANT_UPSTREAM ${JSB_FIRMWARE_DIR}/../third_party/ant ABSOLUTE)
get_filename_component(JSB_ANT_PATCHES ${JSB_FIRMWARE_DIR}/../third_party/patches ABSOLUTE)
set(JSB_ANT_DIR ${CMAKE_CURRENT_LIST_DIR})
set(JSB_ANT_SRC ${CMAKE_BINARY_DIR}/jsb-ant/ant)
set(JSB_ANT_GEN ${CMAKE_BINARY_DIR}/jsb-ant-gen)
set(JSB_ANT_OBJ ${CMAKE_BINARY_DIR}/jsb-ant-obj)
set(JSB_ANT_DEPS_DIR ${CMAKE_BINARY_DIR}/jsb-ant-deps CACHE PATH "Ant dependency cache")

# Clang 19 or newer (C23 constexpr); versioned names first, since a distribution's
# plain "clang" may be older.
find_program(JSB_ANT_CC NAMES clang-21 clang-20 clang-19 clang DOC "Clang C compiler for Ant")
find_program(JSB_ANT_CXX NAMES clang++-21 clang++-20 clang++-19 clang++ DOC "Clang C++ compiler for Ant")
set(JSB_ANT_TARGET_FLAGS "--target=thumbv6m-none-eabi;-mcpu=cortex-m0plus;-mfloat-abi=soft;-fshort-enums"
    CACHE STRING "Target flags for JSB_ANT_CC / JSB_ANT_CXX")

# --- RP2040 memory layout (ANT_EMBED_* overrides, see the patches) ---
# 264 KB SRAM. The GC heap grows in 2 KB chunks up to 128 KB of objects; the VM
# reserves a 512-slot value stack (4 KB) and 32 frames at startup.
set(JSB_ANT_EMBED_DEFINITIONS
    ANT_EMBED=1
    ANT_EMBED_ARENA_CHUNK_SHIFT=11
    ANT_EMBED_ARENA_MAX=131072u
    ANT_EMBED_CLOSURE_ARENA_MAX=32768u
    ANT_EMBED_UPVALUE_ARENA_MAX=16384u
    ANT_EMBED_PROMISE_ARENA_MAX=16384u
    ANT_EMBED_ARENA_MIN=16384u
    ANT_EMBED_PAGE_SIZE=256u
    ANT_EMBED_POOL_PAGE_SIZE=256u
    ANT_EMBED_POOL_ROPE_BLOCK_SIZE=1024u
    ANT_EMBED_POOL_SYMBOL_BLOCK_SIZE=1024u
    ANT_EMBED_POOL_BIGINT_BLOCK_SIZE=1024u
    ANT_EMBED_POOL_STRING_BLOCK_SIZE=1024u
    ANT_EMBED_CODE_ARENA_BLOCK_SIZE=2048
    ANT_EMBED_CODE_UNIT_BLOCK_SIZE=1024u
    ANT_EMBED_INTERN_CHUNK_SIZE=2048u
    ANT_EMBED_OUTPUT_INITIAL_CAP=512
    ANT_EMBED_POOL_CLASS_MIN_ITEMS=8u
    ANT_EMBED_SHAPE_TRANSITION_MAX_PROPS=4u
    ANT_EMBED_COMPACT_TABLES=1
    ANT_EMBED_ARENA_LAZY=1
    ANT_EMBED_STRINGIFY_DEPTH=16
    ANT_EMBED_GC_ROSTER_CAP=64u
    ANT_EMBED_STACK_DEPTH_SCRATCH=64
    ANT_EMBED_ERROR_LINE_BYTES=256
    ANT_EMBED_INTERN_INDEX_CACHE_SIZE=128u
    ANT_EMBED_HEAP_BASE=0x20000000u
    ANT_EMBED_HEAP_SPAN=0x42000u
    ANT_EMBED_VM_STACK_SLOTS=512
    ANT_EMBED_VM_FRAMES=32
    ANT_EMBED_VM_GUARD_SIZE=0u
    ANT_EMBED_C_STACK_SIZE=16384u
    ANT_EMBED_CFUNC_SLAB_SIZE=2048
    ANT_EMBED_EXTERNAL_CHUNK_BITS=8
    ANT_EMBED_EXTERNAL_CHUNK_COUNT=64
    ANT_EMBED_REGISTRY_BUCKET_BITS=6
    # C stack the interpreter may use before it throws a RangeError (core 0
    # stack: SCRATCH_Y + SCRATCH_X, 8 KB, see engines/quickjs/engine.cmake).
    JSB_ANT_STACK_SIZE=10240
    CRPRINTF_MAX_VARS=4
    PCRE2_CODE_UNIT_WIDTH=8
    PCRE2_STATIC=1
    BASE64_STATIC_DEFINE=1
    UTF8PROC_STATIC=1
)

# --- dependencies (versions and hashes as in Ant's vendor/*.wrap at the pinned tag) ---
set(_ant_deps
    "utf8proc|https://github.com/JuliaStrings/utf8proc/releases/download/v2.10.0/utf8proc-2.10.0.tar.gz|276a37dc4d1dd24d7896826a579f4439d1e5fe33603add786bb083cab802e23e|utf8proc-2.10.0"
    "pcre2|https://github.com/PCRE2Project/pcre2/releases/download/pcre2-10.47/pcre2-10.47.tar.bz2|47fe8c99461250d42f89e6e8fdaeba9da057855d06eb7fc08d9ca03fd08d7bc7|pcre2-10.47"
    "yyjson|https://github.com/ibireme/yyjson/archive/refs/tags/0.12.0.tar.gz|b16246f617b2a136c78d73e5e2647c6f1de1313e46678062985bdcf1f40bb75d|yyjson-0.12.0"
    "crprintf|https://github.com/themackabu/crprintf/archive/d396ba00998fc4c99283e9fd30b27ed2ccf58291.tar.gz|7bab8081be7284865e7a49719a2d33dcfb0984bd5a3224cade20c4fa8b0bdfa2|crprintf-d396ba00998fc4c99283e9fd30b27ed2ccf58291"
    "uthash|https://github.com/troydhanson/uthash/archive/v2.3.0.tar.gz|e10382ab75518bad8319eb922ad04f907cb20cccb451a3aa980c9d005e661acc|uthash-2.3.0"
    "base64|https://github.com/aklomp/base64/archive/refs/tags/v0.5.2.tar.gz|723a0f9f4cf44cf79e97bcc315ec8f85e52eb104c8882942c3f2fba95acc080d|base64-0.5.2"
    "double-conversion|https://github.com/google/double-conversion/archive/refs/tags/v3.4.0.tar.gz|42fd4d980ea86426e457b24bdfa835a6f5ad9517ddb01cdb42b99ab9c8dd5dc9|double-conversion-3.4.0"
)

# "git apply" in dir, with a ceiling so git does not find an enclosing repository.
function(_jsb_ant_apply dir patch what)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E env GIT_CEILING_DIRECTORIES=${CMAKE_BINARY_DIR}
                ${GIT_EXECUTABLE} apply --whitespace=nowarn --exclude=meson.build ${patch}
        WORKING_DIRECTORY ${dir} RESULT_VARIABLE rc ERROR_VARIABLE err)
    if(NOT rc EQUAL 0)
        message(FATAL_ERROR "${what}: patch does not apply: ${patch}\n${err}")
    endif()
endfunction()

# Copies Ant's sources to JSB_ANT_SRC and applies third_party/patches/ant/,
# unless the copy is already there for this submodule commit and these patches.
function(_jsb_ant_stage)
    file(GLOB patches ${JSB_ANT_PATCHES}/ant/*.patch)
    list(SORT patches)
    execute_process(COMMAND ${GIT_EXECUTABLE} -C ${JSB_ANT_UPSTREAM} rev-parse HEAD
        OUTPUT_VARIABLE stamp OUTPUT_STRIP_TRAILING_WHITESPACE RESULT_VARIABLE rc)
    if(NOT rc EQUAL 0)
        set(stamp "no-git")
    endif()
    foreach(p ${patches})
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${p})
        file(SHA256 ${p} h)
        string(APPEND stamp "\n${h}")
    endforeach()
    set(stamp_file ${JSB_ANT_SRC}/.jsb-stamp)
    if(EXISTS ${stamp_file})
        file(READ ${stamp_file} old)
        if(old STREQUAL stamp)
            return()
        endif()
    endif()

    set(stage ${CMAKE_BINARY_DIR}/jsb-ant/stage)
    file(REMOVE_RECURSE ${stage})
    file(MAKE_DIRECTORY ${stage}/packages/wasm)
    file(COPY ${JSB_ANT_UPSTREAM}/include ${JSB_ANT_UPSTREAM}/src ${JSB_ANT_UPSTREAM}/sources.json
         DESTINATION ${stage})
    file(COPY ${JSB_ANT_UPSTREAM}/packages/wasm/meson.build ${JSB_ANT_UPSTREAM}/packages/wasm/src
         DESTINATION ${stage}/packages/wasm)
    foreach(p ${patches})
        _jsb_ant_apply(${stage} ${p} "Ant")
    endforeach()
    file(WRITE ${stage}/.jsb-stamp "${stamp}")
    file(REMOVE_RECURSE ${JSB_ANT_SRC})
    file(RENAME ${stage} ${JSB_ANT_SRC})
endfunction()

# Downloads, extracts and patches one dependency; sets JSB_ANT_DEP_<name>.
function(_jsb_ant_dep spec)
    string(REPLACE "|" ";" spec "${spec}")
    list(GET spec 0 name)
    list(GET spec 1 url)
    list(GET spec 2 sha)
    list(GET spec 3 top)
    set(dir ${JSB_ANT_DEPS_DIR}/${name})
    set(stamp ${dir}/.jsb-${sha})
    file(GLOB local_patches ${JSB_ANT_PATCHES}/${name}/*.patch)
    list(SORT local_patches)
    foreach(p ${local_patches})
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${p})
        file(SHA256 ${p} h)
        string(SUBSTRING ${h} 0 12 h)
        string(APPEND stamp "-${h}")
    endforeach()
    if(NOT EXISTS ${stamp})
        get_filename_component(file ${url} NAME)
        set(archive ${JSB_ANT_DEPS_DIR}/dl/${name}-${file})
        file(DOWNLOAD ${url} ${archive} EXPECTED_HASH SHA256=${sha} TLS_VERIFY ON STATUS st)
        list(GET st 0 rc)
        if(NOT rc EQUAL 0)
            message(FATAL_ERROR "Ant dependency ${name}: download failed: ${st}")
        endif()
        file(REMOVE_RECURSE ${dir} ${dir}.tmp)
        file(ARCHIVE_EXTRACT INPUT ${archive} DESTINATION ${dir}.tmp)
        file(RENAME ${dir}.tmp/${top} ${dir})
        file(REMOVE_RECURSE ${dir}.tmp)
        set(patches)
        if(name STREQUAL "pcre2")
            # Ant's own pcre2 patch (vendor/pcre2.wrap diff_files).
            list(APPEND patches ${JSB_ANT_UPSTREAM}/vendor/packagefiles/patches/pcre2-size-optimize-jit-compiler.patch)
        endif()
        list(APPEND patches ${local_patches})
        foreach(p ${patches})
            # meson.build comes from Meson's wrapdb overlay, which is not used here.
            _jsb_ant_apply(${dir} ${p} "Ant dependency ${name}")
        endforeach()
        file(TOUCH ${stamp})
    endif()
    set(JSB_ANT_DEP_${name} ${dir} PARENT_SCOPE)
endfunction()

# ESP-IDF reads engine files in script mode first; only the real configure works.
if(NOT CMAKE_BUILD_EARLY_EXPANSION)
    if(NOT EXISTS ${JSB_ANT_UPSTREAM}/src/ant.c)
        message(FATAL_ERROR "Ant missing: git submodule update --init third_party/ant")
    endif()
    if(NOT JSB_ANT_CC OR NOT JSB_ANT_CXX)
        message(FATAL_ERROR "ENGINE=ant needs Clang (Ant's headers do not build with GCC): set JSB_ANT_CC and JSB_ANT_CXX")
    endif()
    execute_process(COMMAND ${JSB_ANT_CC} -dumpversion OUTPUT_VARIABLE _ant_clang_ver OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(_ant_clang_ver VERSION_LESS 19)
        message(FATAL_ERROR "ENGINE=ant needs Clang 19 or newer for C23 constexpr; ${JSB_ANT_CC} is ${_ant_clang_ver}. Set JSB_ANT_CC and JSB_ANT_CXX.")
    endif()
    find_package(Git REQUIRED)

    _jsb_ant_stage()
    foreach(d ${_ant_deps})
        _jsb_ant_dep("${d}")
    endforeach()
    set(_utf8proc ${JSB_ANT_DEP_utf8proc})
    set(_pcre2 ${JSB_ANT_DEP_pcre2})
    set(_b64 ${JSB_ANT_DEP_base64})

    # --- generated headers (pcre2 and base64 configuration) ---
    file(MAKE_DIRECTORY ${JSB_ANT_GEN}/pcre2 ${JSB_ANT_GEN}/base64)
    configure_file(${_pcre2}/src/pcre2.h.generic ${JSB_ANT_GEN}/pcre2/pcre2.h COPYONLY)
    configure_file(${_pcre2}/src/pcre2_chartables.c.dist ${JSB_ANT_GEN}/pcre2/pcre2_chartables.c COPYONLY)
    file(READ ${_pcre2}/src/config.h.generic _pcre2_config)
    string(FIND "${_pcre2_config}" "/* #undef SUPPORT_UNICODE */" _pcre2_unicode)
    if(_pcre2_unicode EQUAL -1)
        message(FATAL_ERROR "Ant dependency pcre2: config.h.generic has no SUPPORT_UNICODE line to enable")
    endif()
    string(REPLACE "/* #undef SUPPORT_UNICODE */" "#define SUPPORT_UNICODE 1" _pcre2_config "${_pcre2_config}")
    file(WRITE ${JSB_ANT_GEN}/pcre2/config.h.tmp "${_pcre2_config}")
    configure_file(${JSB_ANT_GEN}/pcre2/config.h.tmp ${JSB_ANT_GEN}/pcre2/config.h COPYONLY)
    # base64 0.5.2: no SIMD codecs.
    set(_b64_config "")
    foreach(f SSSE3 SSE41 SSE42 AVX AVX2 AVX512 NEON32 NEON64)
        string(APPEND _b64_config "#define HAVE_${f} 0\n")
    endforeach()
    file(WRITE ${JSB_ANT_GEN}/base64/config.h.tmp "${_b64_config}")
    configure_file(${JSB_ANT_GEN}/base64/config.h.tmp ${JSB_ANT_GEN}/base64/config.h COPYONLY)

    # --- C library / libstdc++ headers of the platform compiler (without GCC's own) ---
    # Clang's own headers (stdatomic.h, stdint.h, ...) go right before the C
    # library's. Clang's stdatomic.h still hands over to newlib's, which needs
    # <stdint.h> first, so the Ant sources get -include stdint.h (below).
    execute_process(COMMAND ${JSB_ANT_CC} -print-resource-dir
        OUTPUT_VARIABLE _ant_clang_res OUTPUT_STRIP_TRAILING_WHITESPACE RESULT_VARIABLE _ant_res_rc)
    if(NOT _ant_res_rc EQUAL 0 OR NOT IS_DIRECTORY "${_ant_clang_res}/include")
        message(FATAL_ERROR "Ant: ${JSB_ANT_CC} -print-resource-dir gave no include directory")
    endif()
    function(_jsb_ant_sysincludes lang compiler out)
        execute_process(COMMAND ${compiler} -mcpu=cortex-m0plus -mthumb -x ${lang} -E -v -
            INPUT_FILE /dev/null OUTPUT_QUIET ERROR_VARIABLE verbose)
        string(REGEX MATCH "#include <...> search starts here:\n(.*)End of search list" _ "${verbose}")
        string(REGEX REPLACE "[ \t]*\n[ \t]*" ";" dirs "${CMAKE_MATCH_1}")
        set(flags)
        foreach(d ${dirs})
            string(STRIP "${d}" d)
            if(d)
                # Resolve "lib/gcc/<target>/<ver>/../../../../<target>/include" first.
                get_filename_component(d ${d} ABSOLUTE)
                if(NOT d MATCHES "/lib/gcc/")
                    if(NOT d MATCHES "c\\+\\+" AND NOT _clang_added)
                        list(APPEND flags -isystem ${_ant_clang_res}/include)
                        set(_clang_added TRUE)
                    endif()
                    list(APPEND flags -isystem ${d})
                endif()
            endif()
        endforeach()
        if(NOT flags)
            message(FATAL_ERROR "Ant: no C library headers found for ${compiler} (${lang})")
        endif()
        set(${out} ${flags} PARENT_SCOPE)
    endfunction()
    _jsb_ant_sysincludes(c ${CMAKE_C_COMPILER} _ant_sys_c)
    _jsb_ant_sysincludes(c++ ${CMAKE_CXX_COMPILER} _ant_sys_cxx)

    # --- sources: the "embed" list of sources.json (added by the patches) ---
    file(READ ${JSB_ANT_SRC}/sources.json _ant_sources_json)
    string(JSON _ant_n ERROR_VARIABLE _ant_json_err LENGTH "${_ant_sources_json}" embed files)
    if(_ant_json_err)
        message(FATAL_ERROR "Ant: sources.json has no embed file list (${_ant_json_err})")
    endif()
    set(_ant_core)
    math(EXPR _ant_last "${_ant_n} - 1")
    foreach(i RANGE ${_ant_last})
        string(JSON f GET "${_ant_sources_json}" embed files ${i})
        list(APPEND _ant_core ${JSB_ANT_SRC}/${f})
    endforeach()

    file(GLOB _b64_arch ${_b64}/lib/arch/*/codec.c)
    file(GLOB _dc ${JSB_ANT_DEP_double-conversion}/double-conversion/*.cc)
    set(_pcre2_files)
    foreach(p auto_possess chkdint compile compile_cgroup compile_class config context convert
              dfa_match error extuni find_bracket jit_compile maketables match match_data
              match_next newline ord2utf pattern_info script_run serialize string_utils study
              substitute substring tables ucd valid_utf xclass)
        if(EXISTS ${_pcre2}/src/pcre2_${p}.c)
            list(APPEND _pcre2_files ${_pcre2}/src/pcre2_${p}.c)
        endif()
    endforeach()
    list(APPEND _pcre2_files ${JSB_ANT_GEN}/pcre2/pcre2_chartables.c)

    set(_ant_all
        ${JSB_ANT_DIR}/adapter_ant.c
        ${_ant_core}
        ${JSB_ANT_DEP_yyjson}/src/yyjson.c
        ${JSB_ANT_DEP_crprintf}/src/crprintf.c
        ${_utf8proc}/utf8proc.c
        ${_b64_arch} ${_b64}/lib/tables/tables.c ${_b64}/lib/codec_choose.c ${_b64}/lib/lib.c
        ${_dc}
        ${_pcre2_files}
    )

    # -femulated-tls: arm-none-eabi GCC and pico-sdk implement _Thread_local
    # with __emutls_get_address, not Clang's default __aeabi_read_tp.
    set(_ant_flags ${JSB_ANT_TARGET_FLAGS}
        -Os -ffunction-sections -fdata-sections -fno-math-errno -fno-trapping-math -fwrapv -w
        -femulated-tls -include stdint.h)
    foreach(d ${JSB_ANT_EMBED_DEFINITIONS})
        list(APPEND _ant_flags -D${d})
    endforeach()
    foreach(i ${JSB_FIRMWARE_DIR}/engines/include/compat
              ${JSB_ANT_SRC}/include ${JSB_ANT_GEN}
              ${JSB_ANT_DEP_yyjson}/src ${JSB_ANT_DEP_crprintf}/src ${JSB_ANT_DEP_uthash}/src ${_utf8proc}
              ${_b64}/include ${JSB_ANT_DEP_double-conversion}
              ${JSB_ANT_GEN}/pcre2 ${_pcre2}/src
              ${JSB_FIRMWARE_DIR}/engines/include ${JSB_FIRMWARE_DIR}/hal/include)
        list(APPEND _ant_flags -I${i})
    endforeach()

    set(JSB_ANT_OBJECTS)
    foreach(src ${_ant_all})
        file(RELATIVE_PATH rel ${CMAKE_BINARY_DIR} ${src})
        if(rel MATCHES "^\\.\\.")
            file(RELATIVE_PATH rel ${JSB_FIRMWARE_DIR}/.. ${src})
        endif()
        string(REGEX REPLACE "[/.]" "_" objname "${rel}")
        set(obj ${JSB_ANT_OBJ}/${objname}.o)
        if(src MATCHES "\\.cc$")
            set(cmd ${JSB_ANT_CXX} -std=gnu++23 -fno-exceptions -fno-rtti ${_ant_sys_cxx})
        else()
            set(cmd ${JSB_ANT_CC} -std=gnu23 ${_ant_sys_c})
        endif()
        set(extra)
        if(src MATCHES "/pcre2")
            set(extra -DHAVE_CONFIG_H)
        elseif(src MATCHES "/base64-|/base64/")
            set(extra -I${JSB_ANT_GEN}/base64 -I${_b64}/lib)
        endif()
        add_custom_command(OUTPUT ${obj}
            COMMAND ${cmd} ${_ant_flags} ${extra} -MD -MF ${obj}.d -c ${src} -o ${obj}
            DEPENDS ${src}
            DEPFILE ${obj}.d
            COMMENT "Ant (clang): ${rel}"
            COMMAND_EXPAND_LISTS VERBATIM)
        list(APPEND JSB_ANT_OBJECTS ${obj})
    endforeach()
    file(MAKE_DIRECTORY ${JSB_ANT_OBJ})
    set_source_files_properties(${JSB_ANT_OBJECTS} PROPERTIES EXTERNAL_OBJECT TRUE GENERATED TRUE)
endif()

set(JSB_ENGINE_SOURCES ${JSB_ANT_OBJECTS})
# pico_malloc panics on a failed allocation by default; Ant reports out of memory itself.
# Ant's parser and compiler recurse deeply (blink: 8.4 KB measured): 4 KB of main RAM joins the 8 KB
# scratch-RAM stack (hal/pico/hal_pico.c), and Ant's own limit (JSB_ANT_STACK_SIZE)
# stays ~2 KB short of the total for the frames between its checks.
set(JSB_ENGINE_DEFINITIONS JSB_PICO_STACK_RESERVE=4096) # TEMP-TRACE: was also PICO_MALLOC_PANIC=0
set(JSB_ENGINE_LINK_LIBRARIES m)
