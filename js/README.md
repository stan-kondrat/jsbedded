# js

TypeScript tooling: an npm workspace whose packages publish under the
[`@jsbedded`](https://www.npmjs.com/org/jsbedded) npm organization.
Independent of the root; work inside this folder:

```bash
cd js && npm install && npm run build --workspaces --if-present
```

Packages go in `packages/<name>/`, one `package.json` each. None are defined yet;
the package split is decided when the code exists.

Planned responsibilities, not yet assigned to packages:

- Hardware API `.d.ts` + TSDoc — the public API spec; per-target typings
- `jsbedded` command: flash, run, logs, recover, test, info
- Device protocol over serial / Web Serial / WebUSB
- TS → engine-compatible JS, checked against the target's capabilities
- Project template
