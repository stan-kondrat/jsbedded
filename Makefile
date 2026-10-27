# JSBedded top-level Makefile — the single entry point.
# Targets that are declared but not built yet fail with "not implemented yet".

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
	@echo "  firmware        build one image   -> $(BUILD_DIR)/<board>-<engine>/jsbedded-<board>-<engine>.<uf2|bin>"
	@echo "  firmware-all    build every board for ENGINE (default: none)"
	@echo "  js              install and build the js/ workspace"
	@echo "  codegen         generate C bindings from the API typings in js/"
	@echo "  docs            docs dev server"
	@echo "  docs-gen        generate reference docs and matrix data"
	@echo "  docs-build      static site -> $(BUILD_DIR)/site/"
	@echo "  test            firmware unit + conformance (simulator)"
	@echo "  bench           size / startup / performance measurements"
	@echo "  matrix          merge manifests + results -> docs/data/matrix.json"
	@echo "  dist            collect built images + SHA256SUMS -> $(BUILD_DIR)/dist/"
	@echo "  clean           remove $(BUILD_DIR)/"

dist:
	sh scripts/release/dist.sh $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
