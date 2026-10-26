# scripts

| Folder | Role |
|---|---|
| `codegen/` | API typings `.d.ts` (`js/`) → C binding headers and per-target typings |
| `matrix/` | board.json + engine.json + API requirements + results → `docs/data/matrix.json` |
| `docs/` | TypeDoc and Doxygen/clang runs → reference Markdown in `docs/` |
| `release/` | UF2 packaging, checksums, npm publish |
