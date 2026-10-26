# Add a board

1. Create `firmware/boards/<board>/board.json` (and `board.mk` once the build exists).
2. Set `platform` and `family`; reuse an existing family where the MCU matches.
3. Run `make firmware-all` — the new board joins the matrix automatically.
