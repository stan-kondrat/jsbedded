# Add a board

1. Create `firmware/boards/<board>/board.json` with the board's facts.
2. Add `firmware/boards/<board>/board.mk`: `PLATFORM` (`pico` or `esp-idf`) and
   that platform's setting (`PICO_BOARD` or `IDF_TARGET`). Reuse an existing
   `family` where the MCU matches.
3. Build it: `make firmware BOARD=<board>`; `make firmware-all` picks it up automatically.
4. Add it to the matrix in `.github/workflows/firmware.yml` so CI builds and releases it.
