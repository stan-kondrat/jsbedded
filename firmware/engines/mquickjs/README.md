# engines/mquickjs

Adapter for MQuickJS. Upstream: `third_party/mquickjs` (https://github.com/bellard/mquickjs),
used unmodified; there are no patches in `third_party/patches/mquickjs/`.

```sh
make firmware BOARD=pico ENGINE=mquickjs
```

## Files

- `engine.cmake` — compiles `mquickjs.c`, `dtoa.c`, `libm.c`, `cutils.c` and the adapter,
  and generates the stdlib at configure time (below).
- `adapter_mquickjs.c` — implements [`engine.h`](../include/jsb/engine.h), including the
  host-function contract, and the C functions the stdlib refers to.
- `jsb_stdlib.c` — the JSBedded standard library: input of the stdlib generator (below).

## Generated stdlib

MQuickJS keeps its standard library in ROM tables produced by a host tool. At
configure time `engine.cmake` compiles `jsb_stdlib.c` + upstream `mquickjs_build.c`
with the host compiler (`cc`, `gcc` or `clang` on `PATH`; override with `HOST_CC`
or `-DJSB_HOST_CC=`) and runs it with `-m32` (target word size) to write
`mquickjs_atom.h` and `jsb_stdlib.h` into `<build>/mquickjs/`. Outputs are only
replaced when they change, and editing the generator sources reruns the configure.
This works for both pico-sdk and ESP-IDF, which cross-compile; the host needs a C
compiler with `-lm` (Linux or macOS).

`jsb_stdlib.c` `#include`s upstream `mqjs_stdlib.c` (its `main` renamed) to reuse
every class and object definition unchanged, and lists its own global object: the
upstream one minus the REPL host functions (`console`, `print`, `load`,
`setTimeout`, `clearTimeout`). When upstream adds a global, add it there too.

## Host functions

MQuickJS can't register C functions at runtime: every C function is an index into
the generated `js_c_function_table`. `jsb_stdlib.c` therefore declares one extra C
function, `jsb_host` (a `generic_params` closure, `JS_CFUNCTION_USER + 0`).
`jsb_engine_define()` creates one closure of it per host function with
`JS_NewCFunctionParams()`; its params value is the index of the C callback in a
static table (32 host functions per runtime). Objects such as `gpio` are plain RAM
objects set on the global object. Retained callbacks (`jsb_arg_function`) are a pool
of 32 `JSGCRef`s, and host return values are held in a pushed `JSGCRef`, since the
compacting GC moves objects. Host functions show up as `jsb_host` in stack traces.

## Globals

The upstream language set (close to ES5: `Object` … `Float64Array`, `Math`, `JSON`,
`RegExp`, `Date`), plus:

| Global | From | Behaviour |
|---|---|---|
| `console.log(...)` | `api/console` | arguments separated by spaces, then `\r\n`. Strings print as-is; other values as MQuickJS prints them (`{ a: 1 }`, `[ 1, 2 ]`). |
| `setTimeout`, `setInterval`, `clearTimeout`, `clearInterval` | `core/src/timers.c` | event-loop timers |
| `gpio` | `api/gpio` | `mode`, `write`, `read` |
| `board` | `core/src/board.c` | `name`, `LED` |
| `gc()` | adapter | runs the garbage collector |
| `Date`, `Date.now()`, `performance.now()` | adapter | milliseconds from `gettimeofday()`: time since boot until something sets the clock |

`print` and `load` of the `mqjs` REPL don't exist. Specs: `js/packages/<module>/index.d.ts`.

`Math.random()` is seeded from the clock at `jsb_engine_create()`.

Uncaught exceptions print `Uncaught <Name>: <message>` followed by the stack
(`at main.js:1:7`), with `\r\n` line ends, and `jsb_engine_eval()` returns -1.

`JS_Eval` assumes `source[len] == '\0'`; callers must pass a NUL-terminated buffer.

## Memory

The engine allocates nothing from the C heap. Every JS object and the JS value
stack live in one static buffer, `JSB_MQUICKJS_HEAP_SIZE` bytes (in `.bss`, so
the linker rejects an image that doesn't fit). Per board, from `engine.cmake`:

| Board | SRAM | JS heap |
|---|---|---|
| pico, pico_w (RP2040) | 264 KB | 64 KB |
| pico2, pico2_w (RP2350) | 520 KB | 128 KB |
| esp32s3_devkitc | 512 KB | 128 KB |
| esp32c3_devkitm | 400 KB | 96 KB |

Each leaves well over half of SRAM for the SDK, Wi-Fi/BLE stacks and later host
APIs. Override with `-DJSB_MQUICKJS_HEAP_SIZE=<bytes>` on the CMake command line.

The interpreter and parser don't recurse on the C stack, so JS depth is limited by
the JS heap, not the C stack. The adapter needs at most a few hundred bytes of C stack
on top of what MQuickJS uses (128-byte buffers for error messages and backtraces).
Not measured on hardware yet.

## Boards

All six boards are expected to work; `unsupported_boards` is empty. Code size is
about 100 KB of Thumb-2 per upstream, well inside 2–8 MB of flash. `min_flash_kb`
and `min_ram_kb` stay `null` until measured (`bench/`).

## Toolchain notes

Ubuntu's arm-none-eabi newlib leaves `PRIu64` and the other 64-bit `PRI*` macros
undefined, so the MQuickJS sources are compiled with
`../include/compat/` on the include path. Its `inttypes.h` includes the real one
(`#include_next`) and defines only the missing macros.
