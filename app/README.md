# Bloodborne Randomizer (PS4 homebrew)

The application. A native C++ SDL2 homebrew app (title `BBRD00001`) that
randomizes Bloodborne directly on a GoldHEN PS4, replacing the Windows-side WPF
tool for everything it has reached parity on.

## What it does

Reads a vanilla `dvdroot_ps4` from `/data/bbrandomizer/VanillaSource/` and
writes a randomized tree into `/data/GoldHEN/AFR/<titleId>/dvdroot_ps4/`, which
AFR overlays onto the installed game. The vanilla source has to be put on the
console by hand once, over FTP — installed game content is not reachable from
homebrew (see [../docs/ps4-homebrew-findings.md](../docs/ps4-homebrew-findings.md) §1).

Implemented: enemy randomization, boss randomization and per-zone boss scaling,
treasure randomization, enemy drops, starting weapons, workshop tools, Mergo
darkness, a seed you can roll or type, and drill-in pickers for which enemies
and bosses stay in the pool. Current status of every setting, implemented or
not, is in [../docs/randomization-feature-spec.md](../docs/randomization-feature-spec.md).

## Layout

| Path | Role |
|---|---|
| `src/Platform/` | SDL2, input, renderer, logging, the 8x8 font |
| `src/UI/` | Screens, screen manager, shared controls |
| `src/Game/` | Game detection, AFR paths |
| `src/Randomizer/` | Settings, the randomizer engine, progress, baked tables |
| `src/Msb/` | Map format — DCX codec, MSBB parser/serializer |
| `src/Param/` | PARAM/BND4 access |
| `tools/` | Python verifiers and table generators |
| `tools/data/` | Committed reference data and fixtures — creature names, and `mods/` (see below) |

UI never touches raw AFR paths; the randomizer core never touches SDL2.

Bulk game data lives outside this directory, in the gitignored `../data/`:
`../data/vanilla/dvdroot_ps4` is the trusted baseline the verifiers run against,
and `../data/runs/` holds saved output from past hardware runs for diffing.

### `tools/data/mods/` — fixtures for the future mod-merging feature

`no-logo/` is a real 4 KB mod (three `.gfx` files that blank the boot logos),
committed as the reference example of the layout a mergeable mod has: a
`dvdroot_ps4/` subtree, exactly the same shape as the vanilla source and the AFR
output. That shape is why merging is cheap — it is an overlay onto the AFR tree
after randomization, using the mirror path that already exists.

It is a **test fixture, not a shipped asset.** Mods are not bundled into the
`.pkg`: at runtime they are expected to live on the console at
`/data/bbrandomizer/Mods/<name>/dvdroot_ps4/`, parallel to `VanillaSource/`, so
adding one is an FTP copy rather than a rebuild. The feature itself is not
designed or built yet.

## Build

```bash
cd app
make
```

Output: `IV0000-BBRD00001_00-BBRANDOMIZERMAIN.pkg`. Requires `OO_PS4_TOOLCHAIN`
set, and lld 18.1.8 available (`LLD18_BIN`) — the Makefile comments and
[../docs/ps4-homebrew-findings.md](../docs/ps4-homebrew-findings.md) §2
explain why the linker is pinned.

**Do a full clean rebuild (`rm -rf src/x64`) after any class or struct layout
change.** A stale partial rebuild once caused a real heap-corruption SIGSEGV on
hardware.

## Verifying

There is no host C++ compiler and no emulator, so `tools/*_verify.py` scripts
mirror the algorithms in Python and check them against real game data. Each has
a `selftest`:

```bash
python tools/pool_verify.py selftest ../data/vanilla/dvdroot_ps4
```

A passing selftest means the rules are right, not that the C++ implements them
right. Only a hardware test proves a feature works.

## Install and run

Copy the `.pkg` to `/data/pkg/` over FTP (GoldHEN's FTP server is on port 2121),
then install via Settings → Debug Settings → Game → Package Installer, or
GoldHEN's own installer. Appears on the home screen as **Bloodborne Randomizer**.

Live log, crash-resilient, one line per event: `/data/bbrandomizer/live.log`.
