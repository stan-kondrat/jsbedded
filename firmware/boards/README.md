# firmware/boards

One folder per board:

- `board.json` — declared facts used by the support matrix and the website.
- `board.mk` — maps the board into the build: `PLATFORM` (`pico` or `esp-idf`)
  plus that platform's setting (`PICO_BOARD` or `IDF_TARGET`).

Adding a board = adding a folder. Boards sharing an MCU `family` share engine
results; only their extra peripherals get board-specific tests.
