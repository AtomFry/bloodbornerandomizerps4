# Architecture

## Repository roles

`app/` is the active PS4 application. `reference/` is the read-only Windows behavioral reference.

## Application structure

```text id="o5s7pg"
Application
├── Platform       SDL2, input, renderer, PS4/platform services, SaveData
├── Game           AfrManager, WorldActivation
├── UI             Screen, ScreenManager, Controls, the four screens
├── Randomizer     Settings, Engine, Progress, WorldStore
├── Msb            MSB map format
└── Param          game parameter formats
```

`Platform/SaveData` is the only place the save-data API is called, and no orbis
type reaches its header. `Game/WorldActivation` is the activation transaction —
refusals, the journal, the seven phases, reconciliation. `Randomizer/WorldStore`
owns worlds, revisions and the on-disk layout under `/data/bbrandomizer/Worlds/`.

`UI/` holds exactly four screens: `WorldsScreen`, `SetupDefaultsScreen`,
`WorldEditorScreen` and `ModelPicker`. The Enable/Disable wizards, the main menu
and the save-data probe were retired by the worlds feature.

Maintain these boundaries:

* UI must not access raw AFR paths, call SDL2, or make an orbis call.
* The AFR path literal appears in exactly one file, `Game/AfrManager.cpp`.
  `worlds_verify.py` asserts this.
* Randomizer core must not depend on SDL2.
* Platform code owns platform-specific concerns, the save-data API included.
* `Msb` owns MSB format handling.
* `Param` owns parameter format handling.

## AFR

AFR is a file-redirect overlay.

Files under:

`/data/GoldHEN/AFR/<titleId>/dvdroot_ps4/...`

shadow matching files from the installed game. Files without an AFR override continue to load from the installed game.

### AFR directory requirements

A sparse AFR tree is insufficient for this project in practice.

The reference tool requires these six directories to be present:

`chr/`
`event/`
`map/`
`param/`
`script/`
`sfx/`

Hardware testing has confirmed this requirement.

The randomizer therefore copies all six directories from the vanilla source into AFR before replacing the files it actually modifies.

When observed behavior conflicts with assumptions about how AFR should work, defer to the reference tool and hardware-tested behavior.

## Vanilla source

The app cannot access vanilla game files inside the encrypted game installation.

The user provides a vanilla `dvdroot_ps4` tree on the PS4 at:

`/data/bbrandomizer/VanillaSource/dvdroot_ps4`

The randomizer reads source files from this tree and writes randomized output to AFR.

**Never bundle vanilla game data into the `.pkg`.**

Keeping the vanilla data external avoids unnecessary package size and makes build/deployment iteration practical.

## MSB

MSB map entries are intentionally treated as opaque byte blobs.

Offsets within an entry are relative to that entry's own start, allowing entries to relocate without rewriting their contents.

Field-level access is intentionally limited to:

* `Part.Enemy`: `ThinkParamID`, `NPCParamID`, and `modelIndex` at known offsets
* `Model`

The MSB implementation was validated with a Python reimplementation that produced byte-exact round trips before the C++ implementation was trusted.

Do not introduce general field parsing into opaque MSB entries without establishing and testing the required format assumptions.

## DCX

DCX writes use stored (uncompressed) DEFLATE blocks.

The available toolchain's zlib support is decompression-only, so the application writes spec-compliant stored DEFLATE blocks instead of compressed blocks.

Decompression uses the vendored `puff.c` implementation.

The output must remain readable by compliant DEFLATE/ZIP-style inflate implementations.
