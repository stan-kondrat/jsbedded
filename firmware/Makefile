# Firmware build. Skeleton only.
#   make -C firmware BOARD=pico2 ENGINE=mquickjs
#
# Planned flow:
#   include boards/$(BOARD)/board.mk     -> PLATFORM, MCU, memory limits
#   include engines/$(ENGINE)/engine.mk  -> engine sources and flags
#   include hal/$(PLATFORM)/platform.mk  -> wraps the platform SDK build (CMake for pico-sdk / ESP-IDF)
#   output: ../build/$(BOARD)-$(ENGINE)/jsbedded.uf2

BOARD  ?= pico2
ENGINE ?= mquickjs

.PHONY: all clean

all:
	@echo "firmware: not implemented yet (BOARD=$(BOARD) ENGINE=$(ENGINE))" >&2; exit 1

clean:
	@true
