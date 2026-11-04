# firmware/core

Engine-neutral runtime services:

- **boot** — hardware init, choose normal or safe mode
- **loader** — find, validate and start the user script from `storage/`
- **event loop** — timers, I/O callbacks, promise jobs
- **console** — log output over USB serial / UART
- **recovery** — reset-button safe mode, boot-loop detection, script removal
- **errors** — uncaught exception reporting with source positions

Public headers go in `include/jsb/`, sources in `src/`; `core.cmake` lists them for
every platform build. Today:

- `src/main.c` — `jsb_main()`: boot line, engine, host APIs, boot script, event loop
  (due timers → pending jobs → sleep until the next timer)
- `src/timers.c` — timer queue and `setTimeout`/`setInterval`/`clearTimeout`/`clearInterval`
- `src/board.c` — the `board` global (`JSB_BOARD`, `JSB_BOARD_LED`)
- `boot.js` — default boot script; `make firmware SCRIPT=<file.js>` replaces it
  (`core.cmake` writes it as a C array to `<build>/jsb_script.c`)
