# firmware/boards

One folder per board:

- `board.json` — declared facts used by the support matrix and the website.
- `board.mk` — maps the board into the build: `PLATFORM` (`pico` or `esp-idf`)
  plus that platform's setting (`PICO_BOARD` or `IDF_TARGET`), `BOARD_LED`
  when the on-board LED is a plain GPIO, and `DRIVE_KB`: the flash at the end
  set aside for the script drive (`"drive_kb"` in `board.json`; 0 or unset = no
  drive). See [`../storage/README.md`](../storage/README.md).

`"led"` in `board.json` (repeated as `BOARD_LED` in `board.mk`, which the build
reads) becomes `board.LED` in JavaScript: 25 on `pico` and `pico2`. The W boards
(LED on the CYW43 chip) and the ESP32 dev boards (addressable RGB LED) have none
until their drivers exist, so `board.LED` is `undefined` there.

Adding a board = adding a folder. Boards sharing an MCU `family` share engine
results; only their extra peripherals get board-specific tests.
