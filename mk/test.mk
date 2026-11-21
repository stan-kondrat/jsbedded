# Test, benchmark and matrix targets.

.PHONY: test test-firmware bench matrix

# Conformance tests (tests/conformance, on hardware) are not wired up yet.
test: test-firmware

test-firmware:
	$(MAKE) -C tests/firmware OUT=$(abspath $(BUILD_DIR))/tests/firmware

bench:
	$(call not_implemented,$@)

matrix:
	$(call not_implemented,$@)
