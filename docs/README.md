# docs

Specs, guides and the website, in one place. Every page is a
Markdown file; VitePress renders them. Has its own `package.json` (later),
separate from the `js/` workspace.

| Folder | Content | Source |
|---|---|---|
| `guide/` | Getting started, firmware install, first script, recovery | hand-written |
| `api/` | Hardware API reference | `index.md` hand-written, `reference/` generated from the API typings in `js/` |
| `protocol/` | Device ↔ host wire protocol | hand-written spec |
| `boards/`, `engines/` | One page per board / engine | hand-written intro + facts from `board.json` / `engine.json` |
| `porting/` | Adding a board, engine or platform | hand-written, `reference/` generated from C headers |
| `data/` | `matrix.json` and other generated data | `make matrix` |
| `public/` | Static assets | — |
| `.vitepress/` | Site config, theme, components (`<SupportMatrix />`, …) | later |

```bash
make docs        # dev server
make docs-build  # static site -> build/site/
```
