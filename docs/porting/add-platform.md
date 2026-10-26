# Add a platform

1. Create `firmware/hal/<platform>/` implementing `hal.h`.
2. Provide `platform.mk` wrapping the platform SDK's own build.
3. Add at least one board that uses it.
