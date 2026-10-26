# CI

## `workflows/firmware.yml`

Builds every board's firmware from source on Ubuntu, with pinned SDK submodules:

- **Pull request:** build all boards; images are workflow artifacts.
- **Push to `main`:** build, then replace the rolling **`latest`** pre-release
  (tag `latest` moves to the new commit) with all images and `SHA256SUMS`.
- **Tag `v*`:** build, then publish a release for that tag with the same files.

Cached between runs: the pico-sdk and ESP-IDF checkouts (keyed on their pinned
commits), ESP-IDF toolchains (`~/.espressif`), picotool, the apt toolchain
packages, and ccache per board.

## Planned

- **test** — every PR: firmware unit tests, host language tests, simulator conformance
- **docs** — build the site; fail if generated reference docs or data are out of date
- size-check every engine × board combination not marked `unsupported`
