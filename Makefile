# JSBedded top-level Makefile — the single entry point.
# Skeleton only: targets are declared, recipes are not implemented yet.

include mk/config.mk
include mk/firmware.mk
include mk/js.mk
include mk/docs.mk
include mk/test.mk

.DEFAULT_GOAL := help

.PHONY: help clean dist

help:
	@echo "JSBedded make targets (BOARD=$(BOARD) ENGINE=$(ENGINE))"
	@echo ""
	@echo "  firmware        build one image   -> $(BUILD_DIR)/<board>-<engine>/jsbedded.uf2"
	@echo "  firmware-all    build every combination not marked unsupported"
	@echo "  js              install and build the js/ workspace"
	@echo "  codegen         generate C bindings from the API typings in js/"
	@echo "  docs            docs dev server"
	@echo "  docs-gen        generate reference docs and matrix data"
	@echo "  docs-build      static site -> $(BUILD_DIR)/site/"
	@echo "  test            firmware unit + conformance (simulator)"
	@echo "  bench           size / startup / performance measurements"
	@echo "  matrix          merge manifests + results -> docs/data/matrix.json"
	@echo "  dist            release bundle: UF2s, npm tarballs, checksums"
	@echo "  clean           remove $(BUILD_DIR)/"

dist:
	$(call not_implemented,$@)

clean:
	rm -rf $(BUILD_DIR)
