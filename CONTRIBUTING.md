# Contributing

JSBedded is in the planning stage. Issues and design discussion are the most
useful contributions right now.

## Ground rules

- Every top-level folder builds on its own; `make` at the root just orchestrates.
- Public JS API changes start in the API typings in `js/` (the `.d.ts` files are the spec).
- Firmware internals are documented in C headers; reference docs are generated.
- Support claims come from test results, never from hand-edited tables.
- Engine fixes go upstream (for example to Ant); submodules stay pristine; unavoidable local fixes go in `third_party/patches/`.

## Tooling

Node.js with npm for `js/` and `docs/`; no pnpm or yarn.

## Commit style

Short imperative subject, optional scope: `firmware/gpio: add pull-up option`.
