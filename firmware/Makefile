# Firmware build.
#   make -C firmware BOARD=pico2 ENGINE=none
#   make -C firmware BOARD=pico ENGINE=mquickjs SCRIPT=../examples/blink/main.js
#
# Flow:
#   include boards/$(BOARD)/board.mk     -> PLATFORM and its settings (PICO_BOARD, IDF_TARGET)
#   include hal/$(PLATFORM)/platform.mk  -> wraps the platform SDK build (CMake for pico-sdk / ESP-IDF)
#   engines/$(ENGINE)/engine.cmake       -> engine sources, read by core/core.cmake
#   SCRIPT=<file.js>                     -> built in as the boot script (default: core/boot.js)
#   output: $(BUILD_DIR)/<variant>/jsbedded-<variant>.<uf2|bin>, where <variant> is
#   $(BOARD)-$(ENGINE), plus -<name of the script folder> with SCRIPT (./blink/main.js -> -blink)

BOARD  ?= pico2
ENGINE ?= none
SCRIPT ?=

TOP         := $(abspath ..)
BUILD_DIR   ?= $(TOP)/build
SCRIPT_ABS  := $(if $(SCRIPT),$(abspath $(SCRIPT)))
SCRIPT_TAG  ?= $(if $(SCRIPT),$(notdir $(patsubst %/,%,$(dir $(SCRIPT_ABS)))))
VARIANT     := $(BOARD)-$(ENGINE)$(if $(SCRIPT_TAG),-$(SCRIPT_TAG))
BOARD_BUILD := $(abspath $(BUILD_DIR))/$(VARIANT)
JSB_VERSION ?= $(shell git -C $(TOP) describe --tags --match 'v*' --always --dirty 2>/dev/null || echo dev)
BOARDS      := $(sort $(patsubst boards/%/board.mk,%,$(wildcard boards/*/board.mk)))
ENGINES     := $(sort $(patsubst engines/%/engine.cmake,%,$(wildcard engines/*/engine.cmake)))

.DEFAULT_GOAL := all
.PHONY: all all-boards boards engines clean

ifneq ($(filter-out boards engines clean all-boards,$(or $(MAKECMDGOALS),all)),)
ifeq ($(wildcard boards/$(BOARD)/board.mk),)
$(error unknown BOARD=$(BOARD); known boards: $(BOARDS))
endif
ifeq ($(wildcard engines/$(ENGINE)/engine.cmake),)
$(error unknown ENGINE=$(ENGINE); known engines: $(ENGINES))
endif
ifneq ($(SCRIPT),)
ifeq ($(wildcard $(SCRIPT_ABS)),)
$(error SCRIPT=$(SCRIPT) not found)
endif
endif
BOARD_LED :=
DRIVE_KB  := 0
include boards/$(BOARD)/board.mk
include hal/$(PLATFORM)/platform.mk
endif

IMAGE = $(BOARD_BUILD)/jsbedded-$(VARIANT).$(IMAGE_EXT)

all: platform-build
	@echo "firmware: $(IMAGE)"

all-boards:
	@set -e; for b in $(BOARDS); do $(MAKE) --no-print-directory BOARD=$$b ENGINE=$(ENGINE); done

boards:
	@echo $(BOARDS)

engines:
	@echo $(ENGINES)

clean:
	rm -rf $(foreach b,$(BOARDS),$(foreach e,$(ENGINES),$(abspath $(BUILD_DIR))/$(b)-$(e) $(abspath $(BUILD_DIR))/$(b)-$(e)-*))
