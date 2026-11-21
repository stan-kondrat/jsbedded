# Raspberry Pi board on the pico-sdk platform.
PLATFORM   := pico
PICO_BOARD := pico
# On-board LED GPIO, as "led" in board.json -> board.LED in JS.
BOARD_LED  := 25
# Script drive (USB drive with main.js): the last DRIVE_KB of flash, 1 MB of 2 MB.
# Ant needs about 1.77 MB of firmware, so its images keep a 256 KB drive.
DRIVE_KB   := $(if $(filter ant,$(ENGINE)),256,1024)
