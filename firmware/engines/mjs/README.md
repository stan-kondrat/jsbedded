# engines/mjs

Adapter for mJS (https://github.com/cesanta/mjs), Cesanta's "embedded JavaScript
engine for C/C++", from `third_party/mjs` (kept pristine).

> **License: GPLv2.** mJS is dual-licensed GPLv2 or commercial (Cesanta).
> JSBedded itself is MIT, but **images that contain mJS (`*-mjs*`) are released
> under GPLv2**, and the release notes for those images must say so. Build and
> distribute them accordingly.

```sh
make firmware BOARD=pico ENGINE=mjs
```

## Status: minimal

The adapter evaluates a script and prints its output. Host functions are not
bound yet: the engine links [`common/host_unsupported.c`](../common/host_unsupported.c),
so there are no timers, `gpio` or `board`. `console.log` is installed by the adapter
as a native function; mJS's own `print()` also works.

Untested: no board build and no run on hardware yet.

## Language

"A strict subset of ES6" (see the mJS README): `let`, strict mode, JSON. No closures,
exceptions, `new`, `var`, prototypes, classes, arrow functions, promises, template
strings or `==`, and no String, Number, Date or RegExp objects. The blink example
closes over variables, so it needs an mJS-specific version.

## Files

- `engine.cmake`: compiles `mjs.c` (the amalgamation) plus the adapter, `-Os`, `-w` on `mjs.c`.
- `platform_custom.h`: mJS has no platform for pico-sdk, so `CS_PLATFORM=0` (custom)
  makes it include this libc-only header instead of the Unix/ESP32 sections.
- `adapter_mjs.c`: `engine.h` runtime functions and `console.log`.

## Notes

- Memory comes from `malloc()`; there is no static heap. The project states about
  50 KB flash and under 1 KB RAM on 32-bit ARM; not measured here, so
  `min_flash_kb` and `min_ram_kb` stay `null` (see `bench/`).
- Output goes through stdio (`mjs_print` and `mjs_fprintf` use `stdout`), which both
  HALs route to the console. mJS's own `print()` ends lines with `\n`; `console.log` uses `\r\n`.
- Error messages carry no file name; the `filename` argument is ignored.

## Still to do

- Bind host functions: natively (`mjs_set` + `mjs_mk_foreign_func`, as `console.log`
  does) through `jsb_engine_define`, or through mJS's FFI with a static resolver
  (`mjs_set_ffi_resolver`). Decide when doing it.
- Check byte-string behaviour of `jsb_arg_string`.
- Flash/RAM measurements.
