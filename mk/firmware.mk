# Firmware targets. Delegates to firmware/Makefile, which reads
# firmware/boards/$(BOARD)/board.mk and firmware/engines/$(ENGINE)/engine.mk.

.PHONY: firmware firmware-all

firmware:
	$(call not_implemented,$@)

firmware-all:
	$(call not_implemented,$@)
