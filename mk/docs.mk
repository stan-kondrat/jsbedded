# Docs site targets. docs/ has its own package.json (VitePress).
# GitHub Pages layout: landing (site/) at /jsbedded/, VitePress docs at /jsbedded/docs/.

.PHONY: docs docs-gen docs-build

docs:
	cd docs && npm install --no-audit --no-fund && npx vitepress dev

# TypeDoc (.d.ts) + Doxygen/clang (C headers) + matrix data
docs-gen:
	$(call not_implemented,$@)

docs-build:
	cd docs && npm install --no-audit --no-fund && npx vitepress build
	cp site/index.html $(BUILD_DIR)/site/index.html
