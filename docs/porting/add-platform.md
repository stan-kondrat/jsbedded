# Add a platform

1. Create `firmware/hal/<platform>/` implementing `firmware/hal/include/jsb/hal.h`
   and a program entry point that calls `jsb_main()`.
2. Provide `platform.mk` with a `platform-build` target that wraps the platform
   SDK's own build, compiles the sources listed in `firmware/core/core.cmake`,
   and copies the image to `$(IMAGE)`.
3. Pin the SDK as a submodule in `third_party/`, and add at least one board that uses it.
