# Firmware targets. Delegates to firmware/Makefile, which reads
# firmware/boards/$(BOARD)/board.mk and firmware/hal/<platform>/platform.mk.

.PHONY: firmware firmware-all

FIRMWARE_ARGS = BOARD=$(BOARD) ENGINE=$(ENGINE) BUILD_DIR=$(abspath $(BUILD_DIR))

firmware:
	$(MAKE) -C $(FIRMWARE_DIR) $(FIRMWARE_ARGS)

firmware-all:
	$(MAKE) -C $(FIRMWARE_DIR) all-boards $(FIRMWARE_ARGS)
