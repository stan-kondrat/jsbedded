# engines/quickjs

Adapter for QuickJS (Fabrice Bellard). Upstream: `third_party/quickjs`
(https://github.com/bellard/quickjs), pinned at release 2026-06-04 (`3d5e064`).
QuickJS is the full-language engine (ES2025, almost complete) for the boards with
the most RAM; MQuickJS is the small engine.

```sh
make firmware BOARD=pico2 ENGINE=quickjs
make firmware BOARD=esp32s3_devkitc ENGINE=quickjs
```

## Files

- `engine.cmake`: copies the QuickJS core to `<build>/jsb-quickjs/quickjs/`, applies
  `third_party/patches/quickjs/*.patch` there, and compiles `quickjs.c`, `dtoa.c`,
  `libregexp.c`, `libunicode.c`, `cutils.c` and the adapter. `quickjs-libc` (needs
  an OS), `qjs`, `qjsc` and the REPL are not built.
- `adapter_quickjs.c`: implements [`engine.h`](../include/jsb/engine.h).
- `sdkconfig.defaults`: ESP-IDF settings QuickJS needs (main task stack, app
  partition). `hal/esp-idf/platform.mk` applies it after the platform defaults
  (see [ESP-IDF](#esp-idf)).

## Patches

`third_party/quickjs` is never modified. At configure time `engine.cmake` copies
the sources to a staging directory and applies the patches with `git apply`
(with `GIT_CEILING_DIRECTORIES`, so it works as a plain patch tool inside the
build tree). It then copies the result into `<build>/jsb-quickjs/quickjs/`,
replacing only files whose content changed, so a reconfigure doesn't rebuild
`quickjs.c`. Editing a patch or bumping the submodule reruns the configure. A
patch that no longer applies fails the configure. Building needs `git` on `PATH`.

- `0001-embedded-config-switches.patch` (local only) adds two opt-outs:
  - `CONFIG_NO_ATOMICS`: upstream defines `CONFIG_ATOMICS` on every target except
    Emscripten. Atomics pull in `<pthread.h>`, pthread mutexes and condition
    variables, and `clock_gettime()`. arm-none-eabi newlib has no pthreads.
  - `CONFIG_NO_MALLOC_USABLE_SIZE`: the default allocator calls
    `malloc_usable_size()`, which is only declared on Linux/glibc. GCC 14
    (ESP-IDF v6) rejects the implicit declaration. The adapter installs its own
    allocator, so the default one is unused.
- `0002-no-tm-gmtoff.patch` (local only) adds `CONFIG_NO_TM_GMTOFF`:
  `getTimezoneOffset()` reads `struct tm` `tm_gmtoff`, which newlib (pico-sdk,
  ESP-IDF) does not have. The switch uses the `gmtime()`/`mktime()` path upstream
  already has for Windows. With no time zone set, the offset is 0.

The Intl use-after-free fix upstream (`4490e58`) doesn't apply: it patches
`intl/`, which upstream added after this release. The pinned tree has no Intl.

## Size and configuration

- Compiled out: Atomics (patch above). Intl and Temporal are not in this
  release, and BigInt is built into the core (there is no libbf / BigFloat
  option any more).
- Kept: everything `JS_NewContext()` installs (Date, eval, RegExp, JSON, Proxy,
  Map/Set/WeakMap, typed arrays, Promise, WeakRef/FinalizationRegistry,
  `String.prototype.normalize`). The full Unicode tables
  (`CONFIG_ALL_UNICODE`, ~40 KB of flash) are always on in `libunicode.h`, and
  turning them off would mean another patch and lose `\p{...}` in RegExp. They
  are kept because flash is not the tight resource on the supported boards.
- `-Os` and `-fwrapv` for all engine sources (`JSB_ENGINE_COMPILE_OPTIONS`).
  `-Os` overrides the SDK's `-O3` (pico-sdk Release) or `-Og` (ESP-IDF default).
  QuickJS relies on `-fwrapv`.
- `-w` on the QuickJS sources only (`COMPILE_FLAGS`), so upstream warnings can't
  fail ESP-IDF's `-Werror=all`. The adapter keeps the platform's warnings.
- `JS_STRIP_SOURCE`: function source text is not kept (saves RAM), so
  `Function.prototype.toString()` doesn't return the source. Line numbers are kept
  for errors.

## Globals

The standard `JS_NewContext()` set plus:

| Global | Behaviour |
|---|---|
| `console.log(...)` | arguments converted to strings, separated by spaces, then `\r\n`, via `jsb_hal_console_write()` |

Uncaught exceptions print `Uncaught <Name>: <message>` and the QuickJS stack
(`    at <eval> (boot.js:1:7)`), with `\r\n` line ends, and
`jsb_engine_eval()` returns -1. After the script, pending promise jobs run;
an exception in one is printed the same way. Unhandled promise rejections are not
reported yet.

`jsb_engine_eval()` copies the source into a NUL-terminated buffer (QuickJS
needs one), so callers may pass any buffer.

## Memory

The adapter's allocator puts an 8-byte size header on each block it gets from
`malloc()`. That lets it enforce the limit without `malloc_usable_size()`.
QuickJS packs small objects into 4 KB arenas from this allocator. Per board,
from `engine.cmake`:

| Board | SRAM | JS heap limit | C stack for JS |
|---|---|---|---|
| pico2 (RP2350) | 520 KB | 384 KB | 16 KB |
| pico2_w (RP2350) | 520 KB | 320 KB (room for CYW43 + lwIP) | 16 KB |
| esp32s3_devkitc | 512 KB (~380 KB free heap) | 256 KB | main task stack − 3 KB |
| pico, pico_w (RP2040, unsupported) | 264 KB | 160 KB | 8 KB |
| esp32c3_devkitm (unsupported) | 400 KB (~300 KB free heap) | 192 KB | main task stack − 3 KB |

The first GC threshold is set halfway between the empty context and the limit.
QuickJS's default first GC runs at 256 KB, at or above the limit here. After
each GC QuickJS sets the next threshold to 1.5× the live heap.

Memory use is estimated, not measured. An empty context (all intrinsics, about
550 built-in functions and 240 predefined atoms) is estimated at 150–200 KB on
32-bit targets, including arena slack.

### C stack

QuickJS's parser and interpreter recurse on the C stack.
`JS_SetMaxStackSize()` makes it throw `RangeError` instead of overflowing. The
limit is measured from `jsb_engine_eval()`.

- **pico-sdk:** the core 0 stack starts at the top of SCRATCH_Y and grows down
  through SCRATCH_X (unused without core 1), 8 KB in all. Below that it runs into
  the top of main RAM, where the heap ends. pico-sdk reserves only
  `PICO_STACK_SIZE` (2 KB) and has no stack guard by default. With the heap
  limits above, newlib's heap stays more than 60 KB below the top of RAM, so
  16 KB of stack is free in practice, but the linker does not reserve it. Note
  that pico-sdk panics on a failed `malloc()` (`PICO_MALLOC_PANIC`); the JS limit
  keeps QuickJS well inside the heap.
- **ESP-IDF:** QuickJS runs on the main task. Its stack is
  `CONFIG_ESP_MAIN_TASK_STACK_SIZE`, and `engine.cmake` gives QuickJS that size
  minus 3 KB. The ESP-IDF default (3584 bytes) is far too small: the boot script
  would fail with `RangeError`.

### ESP-IDF

`sdkconfig.defaults` in this folder sets the main task stack to 16 KB and
selects the 1.5 MB single-app partition table (the default factory partition is
1 MB, and QuickJS adds an estimated 700–900 KB of code). `hal/esp-idf/platform.mk`
passes it in `SDKCONFIG_DEFAULTS` after `hal/esp-idf/sdkconfig.defaults`, as it does
for any engine that has an `sdkconfig.defaults`.

## Boards

Expected to work: **pico2, pico2_w, esp32s3_devkitc**. Listed in
`unsupported_boards` (left out of CI and releases):

- **pico, pico_w (RP2040, 264 KB SRAM):** after the SDK, about 240 KB of heap is
  left. The empty context alone is estimated at 150–200 KB. That leaves almost
  nothing for scripts and no room for the CYW43/lwIP stack on pico_w. Flash
  (2 MB) would fit. Use MQuickJS.
- **esp32c3_devkitm (400 KB SRAM):** about 300 KB of heap is free at boot, and
  Wi-Fi/BLE will need 50–100 KB of it. That leaves roughly the size of an empty
  context. RISC-V code is also larger (estimated 1–1.3 MB of QuickJS in the
  1.5 MB partition). Use MQuickJS.

The unsupported boards still configure locally (`make firmware BOARD=pico
ENGINE=quickjs`) with the limits above, for experiments. `min_flash_kb` and
`min_ram_kb` stay `null` until measured (`bench/`).
