# engines/ant

Adapter for Ant (theMackabu/ant). Upstream: `third_party/ant` (https://github.com/theMackabu/ant),
pinned at `v16.5.c41757b4.0`.

**Status: not feasible yet, so there is no `engine.cmake` (Ant is left out of CI).**
Ant's smallest configuration, the interpreter-only Wasm embed, allocates
several megabytes before running any JavaScript, and its embed profile only compiles
for `__wasm__`. Every board has at most 520 KB of SRAM. The blockers below are
compile-time constants and build-system issues in Ant. Each one can be fixed
upstream, and none can be fixed from an adapter without patching
`third_party/ant`.

This is a source-reading assessment (2026-10-27). Nothing was compiled or measured.

## Minimal source set (interpreter + GC, no JIT/FFI/Wasm/NAPI/HTTP/Temporal)

Upstream already defines one: the `@antjs.org/wasm` reactor,
`packages/wasm/meson.build:46-101`. It builds with `-DANT_WASM_EMBED=1`:

- runtime: `src/{ant,base64,builder,cage,debug,descriptors,errors,error_render,exceptions,escape,handles,output,pool,interrupt,protectors,ptr,realm,runtime,shapes,sugar,utf8,utils_portable}.c`, plus `src/numbers.cc` (C++)
- GC: `src/gc/{bigints,gc,objects,roots,ropes,strings,weak}.c` (and `src/gc/arena.c`, which defines `fixed_arena_init`; the wasm list omits it, so check how it gets linked)
- Silver: `src/silver/{ast,call,code_unit,compile_ctx,compiler,directives,engine,eval_env,glue,lexer,limits,stack_trace}.c` (plus `src/silver/ops/*.h`, about 8.2k lines)
- built-ins: `src/modules/{bigint,collections,generator,io,json,iterator,math,regex,symbol}.c`
- embed glue, Wasm-only: `packages/wasm/src/{bridge,stubs,jit_stub,utils}.c`. `stubs.c` replaces events, timers, microtasks, ESM, Date and Buffer. `jit_stub.c` turns the JIT into no-ops.
- vendored deps (Meson wraps, **downloaded at configure time, not in the submodule**): pcre2 10.47 (`jit=disabled`), utf8proc 2.10.0, double-conversion 3.4.0 (C++), yyjson 0.12.0, crprintf, uthash, aklomp-base64 (`packages/wasm/meson.build:28-44`, `vendor/*.wrap`)

That is about 72k lines of Ant C in the core, plus the dependencies. Even this set has no
Date, timers, typed arrays or modules.

## C23 vs the toolchains

Ant requires `-std=gnu23` and GCC ≥ 14 or Clang ≥ 18 (`BUILDING.md:58,82`). The core uses:

- `constexpr` objects (more than 100 files), `enum : uint8_t` (fixed underlying types,
  e.g. `include/ant.h:55`), `nullptr`, `typeof`, `static_assert`, and `[[...]]` attributes. GCC 13
  supports all of them under `-std=gnu2x`.
- It does not use `#embed`, `_BitInt`, `<stdckdint.h>` or `<stdbit.h>`. Those would be blockers for GCC 13.
- GNU statement expressions (`ANT_ASSERT`, `include/ant.h:22`), `__builtin_*`, and `_Thread_local`
  (`src/handles.c:156`, `src/gc/objects.c:938`).

arm-none-eabi-gcc 13 (Ubuntu 24.04 apt, used for the Pico boards) does not accept
the `gnu23` spelling, but it probably compiles the code with `-std=gnu2x`. The ESP-IDF v6
toolchains should be GCC 14 or newer. Both are **unverified**, because nothing could be
compiled here. `src/numbers.cc` and double-conversion also need a C++ compiler and
`libstdc++` headers.

## OS dependencies and replacements

| Dependency | Native build | `ANT_WASM_EMBED` build | MCU replacement |
|---|---|---|---|
| mmap/mprotect pointer cage | `src/cage.c:100-149` reserves 8–32 GB | `aligned_alloc`/`free` (`src/cage.c:20-51`) | same as Wasm: plain heap |
| VM stack | mmap + guard page (`src/silver/engine.c:258-275`) | `calloc(1, SV_VM_RESERVE)` | heap, but the size is the problem (below) |
| pthread mutexes | `src/handles.c:15,66`, `include/sync.h:21` (**not** guarded for Wasm; wasi-libc provides stubs) | same | FreeRTOS/IDF pthread on ESP32; nothing in pico-sdk newlib |
| `_Thread_local` | as above | same | needs TLS support from newlib + the RTOS/linker script on RP2xxx (unverified) |
| clock | `clock_gettime` (`include/gc/objects.h:39-45`) | `ant_wasm_now_ms()` import | HAL tick |
| randomness, interrupts | OS | `ant_wasm_random_fill`, `ant_wasm_should_interrupt` imports | HAL RNG, budget hook |
| stdout | `stdout`/`stderr` FILE streams (`src/modules/io.c:647`) | WASI `fd_write` | newlib retarget to `jsb_hal_console_write()` |
| filesystem, libuv, TLS, HTTP | everywhere | excluded or stubbed | not needed |

The `ANT_WASM_EMBED` branches are close to what a microcontroller port needs.
32-bit pointers already work, because wasm32 is a 32-bit target (`UINTPTR_MAX` checks in
`src/silver/ops/property.h:211,672`, `include/silver/engine.h:238`). But the profile
cannot be enabled on a non-Wasm target.

## Pointer cage, heap minimums vs 512 KB

In `ANT_WASM_EMBED` the cage is a no-op: `include/cage.h:8-10` sets
`ANT_CAGE_MIN_SIZE` to 16 MB, but nothing reserves it, and `ant_cage_set_size` returns false.
So the cage itself is not the blocker. What blocks is the fixed allocation done
when an isolate is created:

| Allocation | Source | Size under `ANT_WASM_EMBED` |
|---|---|---|
| GC arena chunk size | `include/arena.h:48` (`ANT_ARENA_CHUNK_SHIFT = 20`) | 1 MB, 1 MB-aligned |
| arenas per isolate | `src/ant.c:20587-20598` (object, closure, upvalue, promise) | 4 × one full chunk = **4 MB**. `ant_arena_commit` is a no-op in Wasm (`include/arena.h:98-104`), and `ant_cage_reserve` is `aligned_alloc(1 MB, 1 MB)` |
| object arena floor | `include/arena.h:53` `ANT_ARENA_MIN = 8 MB`, clamped in `src/ant.c:20581-20583` | `object_arena_bytes` cannot go lower |
| VM value stack + frames | `include/silver/engine.h:1039-1041`, `src/silver/engine.c:49-52,259-260` | 131072 × 8 B = 1 MB + 64 KB guard + 16384 × `sizeof(sv_frame_t)`, all `calloc`'d up front |
| arena owner tables | `src/gc/arena.c:19-21` | 4096 slots (the whole 4 GiB address space) ≈ 24 KB static |
| assumed C stack | `src/silver/limits.c:29-30` | 1 MB |

The upstream Wasm package documents a 32 MiB minimum memory (`packages/wasm/README.md`,
`packages/wasm/abi.json` `INITIAL_MEMORY`). On-chip SRAM is 264–520 KB, so the
floor is about 10× too large on the best board. An ESP32-S3 module with 8 MB PSRAM
could in principle hold it, but `esp32s3_devkitc` here is configured as SRAM-only.

## Estimated flash

Not measured. A rough estimate by analogy (QuickJS is about 55k lines and builds to roughly
500–700 KB of Thumb-2 at `-Os`): the Ant core (about 80k lines with the ops headers) plus pcre2,
the utf8proc tables, yyjson and double-conversion would be about **1.2–2 MB**. That fits in the flash of
pico2/pico2_w (4 MB), esp32s3_devkitc (8 MB) and esp32c3_devkitm (4 MB), and is
marginal or too large for pico/pico_w (2 MB). Flash is not the binding constraint. RAM is.

## Blockers

1. **The embed profile is tied to Wasm.** `include/wasm_embed.h:10-11` stops the build with
   `#error "ANT_WASM_EMBED requires a WebAssembly target"`, and the host hooks are
   `import_module` attributes (`include/wasm_embed.h:13-30`). Without `ANT_WASM_EMBED`,
   the native paths need mmap, libuv, tlsuv and argtable3 (`src/ant.c:66-81`,
   `src/runtime.c:10-12`).
2. **Megabyte-sized GC arenas.** Chunk size, `ANT_ARENA_MIN` and the per-kind maximums
   are compile-time constants (`include/arena.h:47-53`), and isolate creation takes
   4 chunks (`src/ant.c:20587-20598`). The total is at least 4 MB.
3. **VM stack is reserved up front.** `SV_STACK_HARD_MAX`/`SV_FRAMES_HARD_MAX`
   (`include/silver/engine.h:1039-1041`) are allocated in full by
   `sv_vm_reserve_storage` (`src/silver/engine.c:258-260`), which is at least 1 MB.
4. **The embed glue lives outside `src/`.** The stubs that make the interpreter-only build link
   are in `packages/wasm/src/{stubs,jit_stub,bridge}.c` and include `wasm_embed.h`.
   A non-Wasm embedder would have to copy them.
5. **Dependencies are not in the submodule.** pcre2, utf8proc, double-conversion, yyjson,
   crprintf, uthash and aklomp-base64 are Meson wraps fetched at configure time.
   JSBedded's CMake/CI only checks out submodules.
6. **Unguarded pthreads.** `src/handles.c:15,66` and `include/sync.h:21` use pthread
   mutexes with no single-thread option. pico-sdk has no pthread implementation.

Blockers 1–3 make an adapter impossible without patching `third_party/ant`.
The local patches would be invasive: they would touch headers used across the whole engine. That
goes against the project's contribute-upstream policy.

## Upstream asks (would make this feasible)

1. **A generic embed profile**, e.g. `ANT_EMBED`, that is separate from `__wasm__`.
   `ANT_WASM_EMBED` would then build on top of it. The host hooks (`now_ms`, `random_fill`,
   `should_interrupt`, and console write) would become plain functions or a callback struct that the embedder
   supplies, instead of Wasm imports.
2. **Configurable heap geometry**: `ANT_ARENA_CHUNK_SHIFT` (e.g. 12–14 for 4–16 KB
   chunks), `ANT_ARENA_MIN` and the per-kind maximums as overridable `#ifndef` macros. The
   arena owner tables would be sized from the heap instead of the full 32-bit address space.
3. **Configurable or growable VM stack**: overridable `SV_STACK_HARD_MAX`/`SV_FRAMES_HARD_MAX`,
   or lazy growth instead of a full up-front `calloc`.
4. **Move the stubs into `src/`** (e.g. `src/embed/`), plus a plain source list or
   amalgamation, so that builds outside Meson (CMake/ESP-IDF/pico-sdk) can use them.
5. **A single-threaded option** for `handles.c`/`sync.h` mutexes and `_Thread_local`.
6. **Optional or replaceable heavy deps**: e.g. a lighter regex backend, UTF-8 normalization
   without full utf8proc tables, and a C replacement for `numbers.cc`/double-conversion.
7. **A CI job for the embed profile on a 32-bit non-Wasm host** (e.g. Linux armhf or i386),
   so that the profile keeps compiling outside Wasm.

With 1–4 merged, a JSBedded adapter becomes a small `engine.cmake` plus the adapter `.c`
(the deps from 5 would also need submodules). With 2–3 tuned to roughly a 64–128 KB heap, the
pico2/pico2_w and esp32s3_devkitc boards are the realistic first targets. RP2040 boards would likely stay
unsupported.
