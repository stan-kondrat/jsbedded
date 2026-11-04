# examples

Small projects, each with `main.ts`. Docs embed these files directly, and CI
type-checks them, so examples cannot go stale.

- `main.ts` + `tsconfig.json` — source, compiled to ES5 `main.js` (committed) by
  `cd js && npm run examples`.
- `example.json` (optional) — `{"script", "boards", "engines"}`: CI builds firmware
  images with the script built in, released as `jsbedded-<board>-<engine>-<name>`.

| Example | Status |
|---|---|
| [`blink`](blink/) | runs on pico, pico2 (mquickjs) |
| `button`, `uart-echo`, `i2c-sensor` | planned |
