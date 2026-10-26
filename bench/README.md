# bench

Measurements per engine × board: flash and RAM use (from the linker map),
boot time, and micro-benchmarks. Results go to `results/` and feed the site.

Result file (shared with test runs):

```json
{ "engine": "mquickjs", "engine_rev": "…", "board": "pico2", "fw_rev": "…",
  "date": "YYYY-MM-DD", "env": "sim | device",
  "size": { "flash_kb": 0, "ram_kb": 0 }, "boot_ms": 0,
  "tests": [{ "id": "gpio/toggle", "feature": "gpio", "status": "pass" }] }
```
