# Add an engine

1. Add the engine as a submodule under `third_party/`.
2. Create `firmware/engines/<engine>/` with `engine.json`, `engine.cmake` and an
   adapter implementing `firmware/engines/include/jsb/engine.h`.
3. List boards it cannot fit in `unsupported_boards`, then build:
   `make firmware BOARD=<board> ENGINE=<engine>`. CI picks the engine up automatically.
