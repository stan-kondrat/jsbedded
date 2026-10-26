# third_party/patches

Local patches for third-party libraries, one folder per submodule. Empty for now.

- Submodule checkouts stay pristine; patches are applied by the build to a copy
  under `build/`, never committed into a submodule.
- File naming: `NNNN-short-description.patch` (`git format-patch` output), applied in order.
- Every patch starts with a header line linking its upstream PR or issue, or saying
  why it is local-only. Prefer upstreaming; delete a patch once upstream has it.
- A patch that no longer applies after a submodule bump fails the build.
