# JSBedded

Open-source, privacy-first JavaScript platform for microcontrollers, built for
AI-agent development: typed APIs, headless deploy, logs and recovery that a coding
agent can drive end to end. Board-neutral and JavaScript-engine-neutral.

> **Status: planning.** Nothing here is implemented yet. Every engine and
> board listed below is a target, not a claim of working support.

## The idea

1. Flash ready-made firmware (UF2 for the Pico family).
2. Write `main.ts` with typings published under the `@jsbedded` npm scope.
3. Deploy and run it without rebuilding firmware.
4. Read logs over serial; recover from a faulty script with the reset button.

Local-first: no account, no cloud dependency, telemetry off by default.

## Repository layout

| Path | What lives there |
|---|---|
| `Makefile`, `mk/` | The single build entry point (`make help`) |
| `firmware/` | C firmware: core, hardware APIs, engine adapters, HAL, boards |
| `js/` | TypeScript tooling, an npm workspace publishing under `@jsbedded` |
| `docs/` | Specs, guides and the website (VitePress renders the `.md` files) |
| `third_party/` | Engines and SDKs as pinned git submodules |
| `tests/` | Conformance and firmware unit tests |
| `examples/` | Small runnable projects |
| `bench/` | Size, startup and performance measurements |
| `scripts/` | Codegen, matrix aggregation, release tooling |

See [docs/architecture.md](docs/architecture.md) for how the layers fit together.

## Getting the source

```bash
git clone --recurse-submodules git@github.com:stan-kondrat/jsbedded.git
```

## License

MIT, see [LICENSE](LICENSE). Third-party engines and SDKs keep their own licenses.
