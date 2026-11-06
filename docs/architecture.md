# Architecture

<ArchitectureDiagram />

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
| Language features | engine | language tests run on a board |
| Peripherals | board / MCU family | board manifest + peripheral tests |
| Fit, boot, performance | engine × board | build size check, boot smoke test, bench |

Each engine × board cell climbs: `unsupported → declared → builds → fits →
boots → passes n/m → verified`, and turns `stale` after ~90 days without a result.
All results come from real boards.

## Scripts

Scripts live on the board's `JSBEDDED` USB drive. At boot the firmware runs the
first of `main.js`, `index.js`, `main.mjs`, `index.mjs`. A script is one file
today; `import` / `export` is planned for engines that support ES modules.

TypeScript is compiled with your own `tsc` against the `@jsbedded` typings, for
the engine's language level. Engines that accept TypeScript (Ant) will run it
directly. If a script stops the board from working, use
[dev mode](./guide/recovery.md).

## Security

JSBedded is made for fast prototyping. Anyone with USB access to a board can
change its script; there is no signing or access control.
