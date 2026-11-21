# engines/ant

Adapter for Ant (theMackabu/ant). Upstream: `third_party/ant` (https://github.com/theMackabu/ant),
pinned at `v16.5.c41757b4.0`. The build applies [`third_party/patches/ant/`](../../../third_party/patches/ant/)
to a copy under `build/` (see `engine.cmake`); the submodule stays pristine.

## Status (2026-11-21)

**Builds for `pico` (RP2040). Not yet run on hardware.**

```bash
make firmware BOARD=pico ENGINE=ant SCRIPT=examples/blink/main.js
```

Needs, besides the pico-sdk toolchain: Clang ≥ 18 (`clang`/`clang++` on `PATH`, or
`JSB_ANT_CC`/`JSB_ANT_CXX`) and network access at configure time (the dependencies are
downloaded with pinned SHA-256s, see "How it builds").

What scripts get: Ant's core with Symbol, iterators, generators, Math, BigInt, RegExp,
Map/Set/WeakMap/WeakSet, JSON, Promise and async/await, plus JSBedded's own APIs
(`console`, timers, `board`). Not in this build: Date, typed arrays and `Buffer`, `import`,
Intl, the Node/Web APIs, and printf-style `%` substitution in `console.log`. `Math.random`
uses xorshift64* (not cryptographic).

Measured on the blink image (arm-none-eabi-gcc 15.2.1 + Clang 19.1.7):

| | Size |
|---|---|
| flash used | 1767 KB of 1792 KB (25 KB free) |
| script drive | 256 KB (other engines keep 1 MB on `pico`; set in `boards/pico/board.mk`) |
| static RAM (`.data` + `.bss`) | 28 KB of 264 KB |

Not measured: heap use at startup and whether the blink script fits in RAM.

### Patches

One per upstream ask (below), each written as an upstream PR:

| Patch | Upstream ask |
|---|---|
| `0001` generic `ANT_EMBED` profile, hooks in `include/embed.h` | 1 |
| `0002` embed glue in `src/embed/`, `"embed"` source list in `sources.json` | 4 |
| `0003` configurable heap geometry (`ANT_EMBED_ARENA_*`, `ANT_EMBED_HEAP_*`) | 2 |
| `0004` configurable VM stack and C stack (`ANT_EMBED_VM_*`, `ANT_EMBED_C_STACK_SIZE`) | 3 |
| `0005` single-threaded locks (`sync.h`) | 5 |
| `0006` smaller fixed-size tables (pools, handles, caches, C-stack buffers) | 2 |
| `0007` callee-saved register scan on Thumb-1 (Cortex-M0+) | – |
| `0008` embed list that links without the OS modules (promise, arena, microtask queue, module stubs) | 4 |
| `0009` two layout asserts that fail on 32-bit targets | – |

`0008` and `0009` are needed because at this tag the embed build no longer compiles for any
32-bit target, wasm32 included: `src/ant.c` calls the event loop, N-API, Wasm and ESM code
unconditionally, and two `static_assert`s only hold with 8-byte pointers.

### How it builds

- Ant's headers need Clang, so `engine.cmake` compiles Ant, its dependencies and the adapter
  with Clang (`--target=thumbv6m-none-eabi -fshort-enums -femulated-tls`, matching
  arm-none-eabi GCC's ABI), using GCC's newlib and libstdc++ headers (Clang's own headers
  first, and `-include stdint.h` for newlib's `stdatomic.h`). GCC links.
- Sources: the `"embed"` list of the patched `sources.json`, plus `adapter_ant.c`.
- Dependencies (versions from Ant's `vendor/*.wrap` at the tag): pcre2 10.47 (Ant's patch; JIT
  off), utf8proc 2.10.0, yyjson 0.12.0, crprintf, uthash 2.3.0, aklomp-base64 0.5.2,
  double-conversion 3.4.0 (`third_party/patches/double-conversion/`: no `std::locale`).
- RP2040 memory settings (`ANT_EMBED_*` in `engine.cmake`): 2 KB GC chunks, 128 KB object heap
  limit, owner tables over SRAM only, 512 VM stack slots, 32 frames, small handle tables.

## Assessment (2026-10-27, on this tag)

The rest of this file is the source-reading assessment that led to the patches. The blockers
and asks still describe upstream; the patches above work around them locally.

### Minimal source set (interpreter + GC, no JIT/FFI/Wasm/NAPI/HTTP/Temporal)

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

### C23 vs the toolchains

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

### OS dependencies and replacements

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

### Pointer cage, heap minimums vs 512 KB

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

### Estimated flash

Not measured. A rough estimate by analogy (QuickJS is about 55k lines and builds to roughly
500–700 KB of Thumb-2 at `-Os`): the Ant core (about 80k lines with the ops headers) plus pcre2,
the utf8proc tables, yyjson and double-conversion would be about **1.2–2 MB**. That fits in the flash of
pico2/pico2_w (4 MB), esp32s3_devkitc (8 MB) and esp32c3_devkitm (4 MB), and is
marginal or too large for pico/pico_w (2 MB). Flash is not the binding constraint. RAM is.

### Blockers

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

Blockers 1–3 make an adapter impossible without changing Ant's source. The changes touch
headers used across the whole engine, so they are kept small and each one is shaped as an
upstream PR (`third_party/patches/ant/`, contribute-upstream policy).

### Upstream asks (would make this feasible)

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
