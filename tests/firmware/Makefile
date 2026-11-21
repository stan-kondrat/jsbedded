# Host unit tests for the pure-C parts of the firmware.
#   make -C tests/firmware            build and run all
#   make -C tests/firmware CC=clang   with another compiler

CC     ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -O1 -g
FW     := ../../firmware
OUT    ?= ../../build/tests/firmware
INC    := -I$(FW)/core/include -I$(FW)/hal/include -I$(FW)/storage/include
HDRS   := test.h $(FW)/core/include/jsb/crash.h $(FW)/hal/include/jsb/hal.h $(FW)/storage/include/jsb/config.h

.PHONY: all clean
all: $(OUT)/test_config $(OUT)/test_crash
	@set -e; for t in $^; do $$t; done

$(OUT)/test_config: test_config.c $(FW)/storage/config.c $(HDRS)
	@mkdir -p $(OUT)
	$(CC) $(CFLAGS) $(INC) -o $@ test_config.c $(FW)/storage/config.c

$(OUT)/test_crash: test_crash.c $(FW)/core/src/crash.c $(HDRS)
	@mkdir -p $(OUT)
	$(CC) $(CFLAGS) $(INC) -o $@ test_crash.c $(FW)/core/src/crash.c

clean:
	rm -rf $(OUT)
