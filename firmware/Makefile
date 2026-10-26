# Firmware build.
#   make -C firmware BOARD=pico2
#
# Flow:
#   include boards/$(BOARD)/board.mk     -> PLATFORM and its settings (PICO_BOARD, IDF_TARGET)
#   include hal/$(PLATFORM)/platform.mk  -> wraps the platform SDK build (CMake for pico-sdk / ESP-IDF)
#   output: $(BUILD_DIR)/$(BOARD)/jsbedded-$(BOARD).<uf2|bin>
#
# ENGINE is accepted but unused until engine adapters exist; the image name
# becomes jsbedded-<board>-<engine> then.

BOARD  ?= pico2
ENGINE ?= mquickjs

TOP         := $(abspath ..)
BUILD_DIR   ?= $(TOP)/build
BOARD_BUILD := $(abspath $(BUILD_DIR))/$(BOARD)
JSB_VERSION ?= $(shell git -C $(TOP) describe --tags --match 'v*' --always --dirty 2>/dev/null || echo dev)
BOARDS      := $(sort $(patsubst boards/%/board.mk,%,$(wildcard boards/*/board.mk)))

.DEFAULT_GOAL := all
.PHONY: all all-boards boards clean

ifneq ($(filter-out boards clean all-boards,$(or $(MAKECMDGOALS),all)),)
ifeq ($(wildcard boards/$(BOARD)/board.mk),)
$(error unknown BOARD=$(BOARD); known boards: $(BOARDS))
endif
include boards/$(BOARD)/board.mk
include hal/$(PLATFORM)/platform.mk
endif

IMAGE = $(BOARD_BUILD)/jsbedded-$(BOARD).$(IMAGE_EXT)

all: platform-build
	@echo "firmware: $(IMAGE)"

all-boards:
	@set -e; for b in $(BOARDS); do $(MAKE) --no-print-directory BOARD=$$b; done

boards:
	@echo $(BOARDS)

clean:
	rm -rf $(addprefix $(abspath $(BUILD_DIR))/,$(BOARDS))
