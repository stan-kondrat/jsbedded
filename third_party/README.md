# third_party

Pinned git submodules, never modified in place. Fixes go upstream; until they land,
local fixes live as patch files in [`patches/<lib>/`](patches/README.md), applied at build time.

| Submodule | Upstream | Used by |
|---|---|---|
| `ant/` | https://github.com/theMackabu/ant | `firmware/engines/ant` |
| `mquickjs/` | https://github.com/bellard/mquickjs | `firmware/engines/mquickjs` |
| `quickjs/` | https://github.com/bellard/quickjs | `firmware/engines/quickjs` |
| `pico-sdk/` | https://github.com/raspberrypi/pico-sdk | `firmware/hal/pico` |

```bash
git submodule update --init                     # engines + SDK
git -C third_party/pico-sdk submodule update --init   # pico-sdk's own deps (tinyusb, …)
```
