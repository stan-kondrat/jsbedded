# firmware/storage

The script drive: a FAT volume in the board's flash that the PC sees as a USB
drive named `JSBEDDED`. Copy `main.js` to it, reset the board, and it runs.

Built only where the platform implements the drive (`JSB_STORAGE` in
`core/core.cmake`; today `hal/pico`):

| File | Role |
|---|---|
| [`include/jsb/storage.h`](include/jsb/storage.h) | mount / format, load the script, unmount |
| `drive.c` | FatFs calls: format on first boot, script lookup, read into RAM |
| `diskio.c` | FatFs disk layer on `jsb_hal_disk_*` (512-byte sectors) |
| `ffconf.h` | FatFs configuration (upstream R0.16 file, changes listed at the top) |

FatFs itself is [`third_party/fatfs`](../../third_party/fatfs), vendored unmodified.

## Boot sequence

1. `jsb_storage_mount()` mounts the volume. If it holds no FAT volume (first boot,
   or the PC formatted it as exFAT), it is formatted: FAT12/16 without a
   partition table, 4 KB clusters aligned to the 4 KB flash erase blocks, label
   `JSBEDDED`.
2. `jsb_storage_load_script()` reads the first of `main.js`, `index.js`,
   `main.mjs`, `index.mjs` into RAM (at most 64 KB, `JSB_STORAGE_SCRIPT_MAX`). With none of
   them the built-in boot script runs; with one that can't be read, nothing runs.
3. `jsb_storage_unmount()` writes everything back, then `jsb_hal_drive_attach()`
   shows the drive to the PC. Until then the drive reports "no medium".

From then on only the PC uses the volume and the firmware never reads or writes
it again until the next reset. That avoids the problem of two caches of one FAT
volume (the PC's and the firmware's) getting out of sync, at the cost that a
changed script runs only after a reset.

Dev mode (`jsb_hal_devmode()`, see [recovery](../../docs/guide/recovery.md))
skips step 2.

## Flash layout (Pico)

```
0x000000  firmware image (UF2)
          … free …
size-N    script drive: the last DRIVE_KB of flash (boards/<board>/board.mk)
```

`DRIVE_KB` is 1024 on Pico and Pico W (2 MB flash) and 2048 on Pico 2 and Pico
2 W (4 MB). If a firmware image grows into that area, the drive is turned off
and the boot log says so. Flashing a new UF2 doesn't touch the drive, so files
survive firmware updates as long as `DRIVE_KB` stays the same.

The HAL (`hal/pico/flash_disk.c`) keeps one 4 KB block in RAM and writes it back
when a write moves to another block, at the end of each USB write command, on
SYNCHRONIZE CACHE and on eject. Sectors rewritten with unchanged data cost no
erase.

## Limits

- No wear levelling: the FAT sectors are erased on every save. At ~100,000
  erase cycles per block that is many thousands of saves; fine for development.
- A power loss while a block is written back can corrupt that block. If the
  volume becomes unreadable, the firmware reformats it at the next boot, which
  deletes the files on it.
- 8.3 file names only for the files the firmware looks for (`main.js`,
  `index.mjs`, …); the PC may still store long names for other files.
