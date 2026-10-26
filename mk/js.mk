# JS/TS tooling targets. Runs npm inside js/ only.

.PHONY: js codegen

js:
	$(call not_implemented,$@)

# API typings in js/ (*.d.ts) -> C binding headers in firmware/api/
codegen:
	$(call not_implemented,$@)
