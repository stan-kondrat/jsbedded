# tests

| Folder | What | Runs on |
|---|---|---|
| `conformance/` | JS tests, tagged by feature, every engine × board must pass | simulator, hardware |
| `firmware/` | C unit tests for core and adapters, against a mock HAL | host |

Each run writes one result file (see `bench/README.md` for the shared format);
`make matrix` merges results into `docs/data/matrix.json`.
