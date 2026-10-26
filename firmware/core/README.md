# firmware/core

Engine-neutral runtime services:

- **boot** — hardware init, choose normal or safe mode
- **loader** — find, validate and start the user script from `storage/`
- **event loop** — timers, I/O callbacks, promise jobs
- **console** — log output over USB serial / UART
- **recovery** — reset-button safe mode, boot-loop detection, script removal
- **errors** — uncaught exception reporting with source positions

Public headers go in `include/jsb/`, sources in `src/`.
