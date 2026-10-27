# pico-sdk platform: configures and builds hal/pico with the SDK's CMake.
# Inputs from firmware/Makefile: BOARD, ENGINE, PICO_BOARD, BOARD_BUILD, IMAGE, JSB_VERSION.
# picotool is built by the SDK on first use; set PICOTOOL_FETCH_FROM_GIT_PATH
# in the environment to keep it outside the build directory.

PICO_SDK_PATH ?= $(TOP)/third_party/pico-sdk
IMAGE_EXT     := uf2
PICO_CMAKE    := $(BOARD_BUILD)/cmake
PICO_CCACHE   := $(if $(shell command -v ccache 2>/dev/null),-DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache)

.PHONY: platform-build
platform-build:
	@test -f $(PICO_SDK_PATH)/pico_sdk_init.cmake || { echo "pico-sdk missing: git submodule update --init third_party/pico-sdk" >&2; exit 1; }
	cmake -S hal/pico -B $(PICO_CMAKE) -G Ninja -DCMAKE_BUILD_TYPE=Release \
		-DPICO_SDK_PATH=$(PICO_SDK_PATH) -DPICO_BOARD=$(PICO_BOARD) \
		-DJSB_BOARD=$(BOARD) -DJSB_ENGINE=$(ENGINE) -DJSB_VERSION=$(JSB_VERSION) $(PICO_CCACHE)
	cmake --build $(PICO_CMAKE)
	cp $(PICO_CMAKE)/jsbedded.uf2 $(IMAGE)
