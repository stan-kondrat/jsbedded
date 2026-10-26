# ESP-IDF platform: builds hal/esp-idf with idf.py and merges it into one image
# flashed at 0x0. Needs the ESP-IDF environment in the calling shell:
#   third_party/esp-idf/install.sh esp32s3,esp32c3   (once)
#   . third_party/esp-idf/export.sh
# Inputs from firmware/Makefile: BOARD, IDF_TARGET, BOARD_BUILD, IMAGE, JSB_VERSION.

IMAGE_EXT  := bin
IDF_BUILD  := $(BOARD_BUILD)/idf
IDF_CCACHE := $(if $(shell command -v ccache 2>/dev/null),--ccache)

.PHONY: platform-build
platform-build:
	@command -v idf.py >/dev/null || { echo "idf.py not found: run '. third_party/esp-idf/export.sh' first" >&2; exit 1; }
	IDF_COMPONENT_MANAGER=0 idf.py -C hal/esp-idf -B $(IDF_BUILD) $(IDF_CCACHE) \
		-DIDF_TARGET=$(IDF_TARGET) -DSDKCONFIG=$(IDF_BUILD)/sdkconfig \
		-DJSB_BOARD=$(BOARD) -DJSB_VERSION=$(JSB_VERSION) \
		build merge-bin -o $(IMAGE)
