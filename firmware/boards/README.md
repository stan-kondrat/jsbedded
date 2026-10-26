# firmware/boards

One folder per board. `board.json` holds the declared facts used by the build,
the support matrix and the website; `board.mk` (later) maps them into the build.

Adding a board = adding a folder. Boards sharing an MCU `family` share engine
results; only their extra peripherals get board-specific tests.
