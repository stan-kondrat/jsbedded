# Add an engine

1. Add the engine as a submodule under `third_party/`.
2. Create `firmware/engines/<engine>/engine.json` and implement `adapter.h`.
3. Run language conformance on the host build, then `make firmware-all`.
