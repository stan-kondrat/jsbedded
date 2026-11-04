# Firmware targets. Delegates to firmware/Makefile, which reads
# firmware/boards/$(BOARD)/board.mk and firmware/hal/<platform>/platform.mk.

.PHONY: firmware firmware-all

# SCRIPT is relative to the caller, so it is passed on as an absolute path.
FIRMWARE_ARGS = BOARD=$(BOARD) ENGINE=$(ENGINE) BUILD_DIR=$(abspath $(BUILD_DIR)) \
	$(if $(SCRIPT),SCRIPT=$(abspath $(SCRIPT)))

firmware:
	$(MAKE) -C $(FIRMWARE_DIR) $(FIRMWARE_ARGS)

firmware-all:
	$(MAKE) -C $(FIRMWARE_DIR) all-boards $(FIRMWARE_ARGS)
