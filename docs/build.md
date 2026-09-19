# Build and Run

This document contains the build, packaging, installation, and runtime details for the Bloodborne PS4 randomizer.

## Prerequisites

The project uses the OpenOrbis PS4 toolchain.

Set `OO_PS4_TOOLCHAIN` to the OpenOrbis installation before building. The Makefile checks this variable and fails if it is unset.

## Build

From the repository root:

```bash
cd app
make
```

The build produces:

`IV0000-BBRD00001_00-BBRANDOMIZERMAIN.pkg`

## Toolchain constraints

### Linker version

The project must be linked with **lld 18.1.8**.

The OpenOrbis SDK's clang is used for compilation. The linker is pinned to lld 18.1.8 because builds using later lld versions have produced `.oelf` files that the PS4 loader rejects at launch.

This behavior has been confirmed on hardware. Do not upgrade the linker version without testing the resulting package on the PS4.

The expected lld installation is currently:

`C:/Users/Fry/LLVM18-portable/bin`

### C++ standard

The project uses:

`gnu++17`

Do not change this to `c++17`. libc++'s `<atomic>` requires POSIX declarations that are not exposed when using strict `c++17`.

### Clean builds after layout changes

After changing the layout of a class or struct, perform a full clean rebuild before testing:

```bash
rm -rf src/x64
```

A stale partial rebuild has previously produced a heap-corruption `SIGSEGV` on hardware. Treat a clean rebuild as required after layout changes rather than as general troubleshooting advice.

## Install

After building, copy the `.pkg` to the PS4:

`/data/pkg/`

The package can then be installed through:

**Settings → Debug Settings → Package Installer**

## Runtime logging

The application writes a live, crash-resilient event log to:

`/data/bbrandomizer/live.log`

Use this log when investigating startup failures, crashes, or unexpected runtime behavior.

## Troubleshooting

### Build fails because `OO_PS4_TOOLCHAIN` is unset

Set `OO_PS4_TOOLCHAIN` to the OpenOrbis installation and rerun the build.

### Package builds but fails to launch

Check the linker version first. Confirm that lld 18.1.8 was used to produce the `.oelf`.

### Unexpected crash after changing a class or struct

Perform a full clean rebuild:

```bash
rm -rf src/x64
```

Then rebuild and retest on hardware.

### Runtime behavior is unclear

Check:

`/data/bbrandomizer/live.log`

For persistent or unexplained failures, record the relevant log output and the exact build/install steps used before changing implementation code.
