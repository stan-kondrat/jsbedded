# engines/jerryscript

Adapter for JerryScript (Apache-2.0). Upstream: `third_party/jerryscript`
(https://github.com/jerryscript-project/jerryscript), pinned at release `v3.0.0`
(`5020015`, 2024-12-18). JerryScript is the engine Kaluma and mcujs use on the
Pico, so it also serves as a direct comparison with them.

```sh
make firmware BOARD=pico ENGINE=jerryscript
make firmware BOARD=esp32c3_devkitm ENGINE=jerryscript
```

## Files

- `engine.cmake`: compiles every `jerry-core/*.c` (the same 200 files upstream's
  `jerry-core/CMakeLists.txt` lists) straight from the submodule, plus the
  adapter. Upstream's CMake build, `jerry-port`, `jerry-ext`, `jerry-math` and the
  amalgamation script (needs Python) are not used. The configuration is a list of
  `JERRY_*` defines (see [Configuration](#configuration)).
- `adapter_jerryscript.c`: implements [`engine.h`](../include/jsb/engine.h) and
  the port layer (`jerry_port_*` on top of `jsb_hal_*`).
- `sdkconfig.defaults`: the ESP-IDF main task stack (see [C stack](#c-stack)).

## Patches

None. `third_party/jerryscript` is compiled as it is. jerry-core is plain C99
on the libc: no `<time.h>`/`struct tm` (so no `tm_gmtoff`), no pthreads or
`__atomic` (Atomics works on the JS heap only), and no `malloc()` with
`JERRY_SYSTEM_ALLOCATOR=0`. It uses `<setjmp.h>`, `<math.h>` (C99 libm, e.g.
`nextafter`, `cbrt`, `expm1`) and `string.h`/`ctype.h`, which newlib and
ESP-IDF provide. Time comes only through the port layer (`gettimeofday()`). If a fix becomes
necessary, it goes in `third_party/patches/jerryscript/` and `engine.cmake` copies
the sources to the build directory and applies it there, as
[`engines/quickjs`](../quickjs/README.md#patches) does.

## Configuration

The `es.next` profile (upstream's default: every built-in) with these changes
from `jerry-core/config.h`, set in `JSB_ENGINE_DEFINITIONS`:

| Define | Value | Why |
|---|---|---|
| `JERRY_NDEBUG` | set | no engine assertions (upstream sets it for every non-Debug build) |
| `JERRY_GLOBAL_HEAP_SIZE` | per board, KB | see [Memory](#memory) |
| `JERRY_CPOINTER_32_BIT` | 0 | 16-bit compressed pointers: smaller objects, heap ≤ 512 KB |
| `JERRY_SYSTEM_ALLOCATOR`, `JERRY_EXTERNAL_CONTEXT` | 0 | static heap and context: no `malloc()`, the linker checks the fit |
| `JERRY_STACK_LIMIT` | per board, KB | `RangeError` instead of a C stack overflow |
| `JERRY_ERROR_MESSAGES` | 1 | error messages ("x is not defined"); off upstream |
| `JERRY_LINE_INFO` | 1 | `error.stack` with `file:line:column`; off upstream |
| `JERRY_MODULE_SYSTEM` | 0 | no `import`/`export` until there is a filesystem; drops the file-system port functions |
| snapshots, debugger, logging, mem stats, VM halt/throw callbacks | 0 | not used yet |

Kept: everything else the profile has, including Proxy, Reflect, typed arrays,
DataView, Map/Set/WeakMap/WeakSet, WeakRef, Promise, async functions,
generators, BigInt, Symbol, RegExp, JSON, Date and Unicode case conversion.
`Function.prototype.toString()` does not return the source
(`JERRY_FUNCTION_TO_STRING` is off upstream).

Compiler flags:

- `-Os` for all engine sources (`JSB_ENGINE_COMPILE_OPTIONS`), overriding the SDK's
  `-O3` (pico-sdk Release) or `-Og` (ESP-IDF default).
- On the jerry-core sources only (`COMPILE_FLAGS`):
  - `-w`, so upstream warnings can't fail ESP-IDF's `-Werror=all`. The adapter
    keeps the platform's warnings.
  - jerry-core's private include directories as `-iquote`. Its headers have
    generic names (`config.h`, `common.h`, `debugger.h`). `-iquote` puts them
    ahead of the SDKs' `-I` directories for jerry-core's own `"..."` includes and
    keeps them off every other source's include path. Only `jerry-core/include`
    (`jerryscript*.h`) is a normal include directory.
- `-lm`: without `jerry-math`, jerry-core uses the C library's `<math.h>`.

## Port layer

`jerry-core` calls these port functions in this configuration. The adapter
implements them:

| Port function | Implementation |
|---|---|
| `jerry_port_init` | nothing to do |
| `jerry_port_fatal` | prints `JSBedded: JerryScript fatal error: <cause>; halted` and sleeps forever (`jsb_hal_sleep_ms`) |
| `jerry_port_current_time` | `gettimeofday()` in ms: time since boot until a clock source sets the time |
| `jerry_port_local_tza` | 0 (local time is UTC) |
| `jerry_port_log`, `jerry_port_print_buffer` | console, `\n` → `\r\n` (unused with logging off; kept for later) |
| `jerry_port_sleep` | `jsb_hal_sleep_ms` (only the debugger calls it) |

Not needed: the context functions (no external context), and the path and
source functions (no module system), line reading (only the REPL uses it).

## Globals

The `es.next` set plus:

| Global | Behaviour |
|---|---|
| `console.log(...)` | arguments converted to strings, separated by spaces, then `\r\n`, via `jsb_hal_console_write()` |

Strings are written in 64-byte chunks straight from the engine's string storage,
with no copy on the JS heap.

Uncaught exceptions print `Uncaught <Name>: <message>` and then one
`    at boot.js:1:7` line per frame of `error.stack`, with `\r\n` line ends.
`jsb_engine_eval()` then returns -1. Syntax errors carry the position in the message
(`Uncaught SyntaxError: Unexpected end of input [boot.js:1:12]`). After the
script, pending promise jobs run, and an exception in one is printed the same
way. Unhandled promise rejections are not reported yet (`JERRY_PROMISE_CALLBACK`
is off).

`jerry_parse()` takes a length, so the source needs no NUL terminator and is
not copied.

## Memory

The JS heap is one static buffer (`jerry_global_heap`, in `.bss`) of
`JERRY_GLOBAL_HEAP_SIZE` KB. Every object, string, property table and byte code
lives there. Built-in objects are created lazily on first use and their
property tables are ROM data. The empty context therefore takes only a few KB
of heap. GC runs after `heap / 32` bytes of new allocations, and again when an
allocation fails.

**Running out of JS heap is fatal in JerryScript** (`JERRY_FATAL_OUT_OF_MEMORY`):
the port prints the error and halts the firmware. Other engines throw an
exception instead.

Per board, from `engine.cmake` (override with `-DJSB_JERRYSCRIPT_HEAP_KB=`):

| Board | SRAM | JS heap | C stack for JS (`JERRY_STACK_LIMIT`) |
|---|---|---|---|
| pico (RP2040) | 264 KB | 128 KB | 8 KB |
| pico_w (RP2040) | 264 KB | 96 KB (room for CYW43 + lwIP) | 8 KB |
| pico2 (RP2350) | 520 KB | 256 KB | 16 KB |
| pico2_w (RP2350) | 520 KB | 224 KB (room for CYW43 + lwIP) | 16 KB |
| esp32s3_devkitc | 512 KB | 192 KB | main task stack − 3 KB = 13 KB |
| esp32c3_devkitm | 400 KB | 128 KB | main task stack − 3 KB = 13 KB |

The heap is static, so it is not returned to `malloc()` when the engine is
destroyed. It also never competes with the SDK's heap at run time: what is left
after `.bss` is for the SDK, USB, Wi-Fi and lwIP. These sizes are estimates, not
measurements.

### C stack

JerryScript's parser, RegExp engine and VM recurse on the C stack.
`JERRY_STACK_LIMIT` makes them throw `RangeError: Maximum call stack size
exceeded` instead of overflowing. It is measured from `jerry_init()`, which
`jsb_engine_create()` calls from about the same depth as `jsb_engine_eval()`.

- **pico-sdk:** the core 0 stack is SCRATCH_Y + SCRATCH_X (8 KB, core 1 unused).
  Below that it grows into the top of main RAM, where `malloc()`'s heap ends.
  The JS heap is in `.bss`, so `malloc()` use stays small and that top stays free
  in practice. The 16 KB limit on RP2350 relies on this, but the linker does not
  reserve it.
- **ESP-IDF:** JerryScript runs on the main task. `sdkconfig.defaults` in this
  folder sets `CONFIG_ESP_MAIN_TASK_STACK_SIZE=16384`. `engine.cmake` reads it
  and leaves 3 KB for the frames below `jsb_engine_eval()`. The ESP-IDF default
  (3584 bytes) would leave no stack for scripts at all.

### ESP-IDF

`sdkconfig.defaults` only raises the main task stack. JerryScript is an
estimated 200–300 KB of code, so the default 1 MB factory partition is enough.

## Boards

Expected to work on all six: **pico, pico_w, pico2, pico2_w, esp32s3_devkitc,
esp32c3_devkitm**. `unsupported_boards` is empty. Kaluma runs JerryScript on the
RP2040, and the JS heap here is sized per board to leave the rest of SRAM to the
SDK. `min_flash_kb` and `min_ram_kb` stay `null` until measured (`bench/`).
