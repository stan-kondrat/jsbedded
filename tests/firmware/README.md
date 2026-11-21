# tests/firmware

Host-side C unit tests for the pure-C parts of the firmware: no SDK, no HAL,
plain `cc`.

| Test | Covers |
|---|---|
| `test_config.c` | `firmware/storage/config.c`: `config.ini` parser (`[devmode]`, `on_crash`), default file, `[firmware]` section writer |
| `test_crash.c` | `firmware/core/src/crash.c`: restart-or-stop decision after a crash, `CRASH.TXT` text |

```sh
make test                                  # from the repo root
make -C tests/firmware CC=clang            # or directly, with any C11 compiler
```

Binaries go to `build/tests/firmware/`. A test prints `<name>: ok` or the failed
checks, and exits 1 on failure.
