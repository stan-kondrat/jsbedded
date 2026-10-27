# Architecture

```
developer tools        js/        CLI, compiler, typings, transport
        │  device protocol (docs/protocol)
hardware APIs          firmware/api       console, timers, gpio, uart, i2c, spi, net*, fs*
engine adapter         firmware/engines   ant · mquickjs · quickjs · …
HAL                    firmware/hal       pico-sdk · esp-idf · …
boards                 firmware/boards    pin maps, memory, peripherals
```
\* capability-gated, later.

## Rules

- One engine per firmware image, chosen at build time (`ENGINE=`).
- `api/` and `core/` depend only on `jsb/engine.h` and `jsb/hal.h`.
- The public JS API is defined once, in the API typings in `js/`; C bindings and
  reference docs are generated from it.
- No bytecode portability is assumed across engines or engine versions.

## Support matrix

Facts are split so most are proven once, not per combination:

| Question | Depends on | Proven by |
|---|---|---|
| Language features | engine | host-run language tests |
| Peripherals | board / MCU family | board manifest + peripheral tests |
| Fit, boot, performance | engine × board | build size check, boot smoke test, bench |

Each engine × board cell climbs: `unsupported → declared → builds → fits →
boots → passes n/m → verified`, and turns `stale` after ~90 days without a result.

## Open questions

Script storage and loader design, module format, TS compilation and deployment
details, HAL boundaries, simulator fidelity, recovery UX, signing and secure
device management.
