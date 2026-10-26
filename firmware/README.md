# firmware

C code that runs on the microcontroller. Builds on its own:

```bash
make -C firmware BOARD=pico2 ENGINE=mquickjs
```

| Folder | Role |
|---|---|
| `core/` | Boot, script loader, event loop, safe mode / recovery, error reporting |
| `api/` | JSBedded hardware APIs, written once against the engine adapter and HAL |
| `engines/` | One adapter per JS engine, all implementing the same contract |
| `hal/` | Hardware abstraction layer, one folder per platform SDK |
| `boards/` | Board manifests: MCU, memory, peripherals, pin map |
| `storage/` | Script storage and flash partition layout |

Layering rule: `api/` and `core/` never include engine- or board-specific
headers. Only `engines/*` know about an engine; only `hal/*` and `boards/*`
know about hardware.
