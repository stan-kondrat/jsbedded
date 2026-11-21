# Recovery

## Dev mode (Pico family)

Dev mode starts the board without running the script, so a script that crashes,
loops or takes over the USB port can be fixed on the `JSBEDDED` drive.

1. With the board running, **hold BOOTSEL for 2 seconds**. The LED lights up.
2. **Release BOOTSEL.** The board restarts in dev mode: the drive is there, the
   script is not started, and the LED blinks briefly once a second. The serial
   port says `dev mode`.
3. Fix or delete `main.js` on the drive, eject it, then reset the board (unplug
   and plug in, or hold BOOTSEL 2 s again) to run the script.

Holding BOOTSEL *while plugging in* is different: that starts the boot ROM's
`RPI-RP2` / `RP2350` drive for flashing a UF2 (see
[install](./install-firmware.md)). It also gets out of any situation, since the
ROM never runs JSBedded; flashing a UF2 doesn't erase the `JSBEDDED` drive.

Dev mode is remembered across the restart only; a power cycle starts normally.

The drive holds `config.ini` (commented defaults, created when missing in dev mode
or on first format) and a short `README.txt` with a link to these docs. For now the
firmware applies only `on_crash` (below); the other settings are parsed, with
warnings about bad lines, and take effect in later releases.

The `[firmware]` section at the top of `config.ini` shows what the board runs:
the firmware version, board, engine, engine version and `image_id`, a checksum
of the firmware image that changes with every different UF2, even between two
builds with the same version. The firmware rewrites the section at every boot
when it changes (after flashing another image, for example), so edits there
don't stick.

## Crash report (Pico family)

When the firmware or the JavaScript engine crashes, the board restarts by itself
and writes `CRASH.TXT` to the `JSBEDDED` drive. A crash is a CPU fault (bad
memory access, stack overflow), a failed C `assert`, `abort()`, an SDK panic such
as running out of memory, or an engine's fatal error. A JavaScript exception
in the script is not a crash: the engine prints it and the board keeps running.
Crashes are recorded whatever the `log_*` settings say.

`CRASH.TXT` holds:

| Line | Meaning |
|---|---|
| `crash` | what happened: hard fault, assert failed, abort(), panic or exit(), fatal error |
| `message` | the failed expression or the error message, when there is one |
| `source` | C file and line of the assert or fatal error, when known |
| `step` | what the firmware was doing: `reading the drive`, `starting the engine`, `installing the APIs`, `running the script`, `event loop` |
| `pc`, `lr`, `sp` | CPU registers at a hard fault (look `pc` and `lr` up with `arm-none-eabi-addr2line -e jsbedded.elf`); for the others `lr` is the caller |
| `uptime` | time from boot to the crash |
| `in a row` | crashes without 30 s of normal running in between |
| `image`, `firmware` | the image that crashed and the one on the board now |

The last line says whether the script was started again or not, and why. The
report is also printed on the USB serial port at boot. The file stays on the
drive until the next crash replaces it; you can delete it.

What happens next is set by `on_crash` in the `[devmode]` section of `config.ini`:

| `on_crash` | After a crash |
|---|---|
| `restart` (default) | The script runs again as after a reset. After 3 crashes in a row, it stops as with `report`, so a script that crashes at once doesn't restart the board for ever. |
| `report` | The script is not started. The drive is there with `CRASH.TXT`; nothing runs until you reset the board. |

`restart` is the default because a board that runs a script on its own should
keep working after a rare crash; the report is on the drive either way.

**A crash in the first boot after flashing a new UF2 is always handled as
`report`**, whatever `on_crash` says: you see at once that the new firmware
crashes, and why, instead of the board quietly restarting. "New" means the
`[firmware]` section of `config.ini` changed in that boot (a different
`image_id`, or the section was missing); without `config.ini` on the drive the
firmware can't tell, and uses `on_crash`.

To get back to normal after `report`: fix the script on the drive if it is the
cause, then reset the board (unplug and plug it in, or the RUN pin). The next
boot starts the script again, with the crash count back at zero. `CRASH.TXT`
itself stays on the drive.

The crash record lives in RAM until the restart that follows the crash; if the
board loses power before that, nothing is reported.

Planned: crash records on ESP32 boards; falling back to dev mode after
repeated crashes; the BOOT button on ESP32 boards.
