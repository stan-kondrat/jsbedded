# CI (planned)

Workflows in `workflows/` (not written yet):

- **build** — every PR: build and size-check every combination not marked `unsupported`
- **test** — every PR: firmware unit tests, host language tests, simulator conformance
- **docs** — build the site; fail if generated reference docs or data are out of date
- **release** — tagged: UF2s, npm packages, checksums
