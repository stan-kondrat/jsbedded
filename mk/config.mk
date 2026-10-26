# Shared defaults. Override on the command line: make firmware BOARD=pico ENGINE=ant

BOARD     ?= pico2
ENGINE    ?= mquickjs
BUILD_DIR ?= build

FIRMWARE_DIR := firmware
JS_DIR       := js
DOCS_DIR     := docs

# Placeholder recipe for targets that are declared but not built yet.
define not_implemented
	@echo "make $(1): not implemented yet" >&2; exit 1
endef
