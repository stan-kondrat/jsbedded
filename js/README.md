# js

TypeScript tooling: an npm workspace whose packages publish under the
[`@jsbedded`](https://www.npmjs.com/org/jsbedded) npm organization.
Independent of the root; work inside this folder:

```bash
cd js && npm install && npm run build --workspaces --if-present
npm run examples    # type-check examples/*/main.ts and compile them to ES5 main.js
```

## Packages

API typings, one package per module: global declarations (`.d.ts` + TSDoc), the
spec of the firmware's JS API ([docs/api](../docs/api/index.md)).

| Package | Folder | Declares |
|---|---|---|
| `@jsbedded/console` | `packages/console` | `console.log` |
| `@jsbedded/timers` | `packages/timers` | `setTimeout`, `setInterval`, `clearTimeout`, `clearInterval` |
| `@jsbedded/gpio` | `packages/gpio` | `gpio.mode`, `gpio.write`, `gpio.read` |
| `@jsbedded/board` | `packages/board` | `board.name`, `board.LED` |

Use them with `"types": ["@jsbedded/console", "@jsbedded/timers", …]` and
`"lib": ["ES5"]` in `tsconfig.json`, so browser and Node globals don't leak in.

## Compiling for MQuickJS

MQuickJS runs ES5, so examples compile with **`tsc`** (`typescript ~5.9`,
`target: ES5`): one tool that type-checks and down-levels `let`/`const`, arrow
functions and the like. esbuild can't lower `let`/`const` to ES5 and doesn't
type-check; TypeScript 7 removed `target: ES5` (6.0 deprecated it), hence the pin
to 5.9. The output `examples/*/main.js` is committed, so firmware builds need no
Node; CI recompiles it and fails if it differs.

Planned responsibilities, not yet assigned to packages:

- `jsbedded` command: flash, run, logs, recover, test, info
- Device protocol over serial / Web Serial / WebUSB
- TS → engine-compatible JS, checked against the target's capabilities
- Project template
