# Docs site targets. docs/ has its own package.json (VitePress).

.PHONY: docs docs-gen docs-build

docs:
	$(call not_implemented,$@)

# TypeDoc (.d.ts) + Doxygen/clang (C headers) + matrix data
docs-gen:
	$(call not_implemented,$@)

docs-build:
	$(call not_implemented,$@)
