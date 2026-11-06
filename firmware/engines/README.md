# firmware/engines

One adapter per JavaScript engine; exactly one is linked into an image
(`make firmware ENGINE=<engine>`). Each folder contains:

- `engine.json` — declared facts: upstream, `submodules` CI must check out,
  `unsupported_boards` (left out of CI and releases), language level, minimum flash/RAM
- `engine.cmake` — sources and flags for the firmware build (see the variables
  listed in `../core/core.cmake`). An engine joins the CI matrix as soon as this file exists.
- adapter sources implementing [`include/jsb/engine.h`](include/jsb/engine.h)
- `sdkconfig.defaults` (optional) — ESP-IDF settings the engine needs, applied after
  `hal/esp-idf/sdkconfig.defaults`

## The contract

[`include/jsb/engine.h`](include/jsb/engine.h) is all `core/` and `api/` know
about an engine:

| Part | Functions |
|---|---|
| Runtime | `jsb_engine_create`, `jsb_engine_eval`, `jsb_engine_run_jobs`, `jsb_engine_destroy`, `jsb_engine_name` |
| Define host functions | `jsb_engine_define(engine, object, defs, count)` — functions and constants on the global object (`object == NULL`) or on a new global object (`"gpio"`) |
| Inside a host function | `jsb_argc`, `jsb_arg_type`, `jsb_arg_number`, `jsb_arg_string`, `jsb_arg_function`, `jsb_arg_log`, `jsb_return`, `jsb_throw` |
| Call back into JS | `jsb_engine_call(engine, func, argc, argv)`, `jsb_engine_release` |

A host function is `int fn(jsb_call *call)`, listed by name in a
`jsb_host_def` table (`JSB_DEF_FN`, `JSB_DEF_NUMBER`, `JSB_DEF_STRING`):

```c
static int gpio_read(jsb_call *call)
{
    double pin;
    if (jsb_arg_number(call, 0, &pin) != 0) {
        return -1;                      /* exception already pending */
    }
    jsb_value v = { JSB_NUMBER, { .number = jsb_hal_gpio_read((int)pin) } };
    jsb_return(call, v);
    return 0;
}

static const jsb_host_def defs[] = { JSB_DEF_FN("read", gpio_read) };
jsb_engine_define(engine, "gpio", defs, 1);
```

- **Values** crossing the boundary: undefined, boolean, number (double), string
  (copied both ways), function handle (`jsb_func`). Anything else is `JSB_OTHER`.
- **Errors:** a failing `jsb_arg_*` or `jsb_throw` leaves a JS exception pending
  and returns -1; the host function returns -1 and the exception reaches the JS
  caller. `jsb_engine_eval` and `jsb_engine_call` print uncaught exceptions
  (`Uncaught <Name>: <message>` + stack) and return -1.
- **Callbacks:** `jsb_arg_function` retains a function (TypeError if it isn't one)
  until `jsb_engine_release`; `jsb_engine_call` calls it with `this` undefined.
  Engines keep a fixed pool of handles (32 in MQuickJS and QuickJS).
- **Jobs:** `jsb_engine_run_jobs` runs Promise reactions; engines without
  Promises return 0.
- **Console:** `console.log` is a host function in `api/console/`;
  `jsb_arg_log` gives each engine its own formatting of non-string values.

| Engine | Host functions |
|---|---|
| `mquickjs`, `quickjs` | yes |
| `jerryscript`, `duktape` | not yet: they link [`common/host_unsupported.c`](common/host_unsupported.c), which prints `host functions … not supported by this engine yet` once; scripts run with the engine's own `console.log` but no timers, `gpio` or `board` |
| `none` | no JavaScript (`common/host_unsupported.c` keeps it linking) |

Still to add: interrupt/limit hooks, GC hooks, capability reporting, objects and
arrays as values.

`none/` is the placeholder with no JavaScript; it is the default `ENGINE` and
keeps a baseline image for every board.

Engines live in `third_party/` as submodules; local fixes, if any, are patch
files in `third_party/patches/<engine>/`, applied to a copy under `build/`.

Declared minimums stay `null` until measured — see `bench/`.
