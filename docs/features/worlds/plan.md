# Plan — Worlds

**Status: DONE** — all six milestones implemented and hardware-tested.
Approved by the developer 2026-09-22; hardware test passed in full 2026-09-25.

**Spec:** `docs/features/worlds/spec.md` — **APPROVED**, re-approved 2026-09-22.
Twenty-seven binding decisions, §7.2 and §10, of which D13 is superseded by D24;
eight binding safety rules, §7.1.

**Technical evidence:** `docs/features/worlds/technical-findings.md`.

**Evidence:** `docs/features/worlds/plan-evidence.md` — the measurements, traces
and rejected alternatives behind this plan.

**Plan review:** `docs/features/worlds/plan-review.md` — added during review.

---

> **This document is the implementation contract.** §1–§7 are what the
> implementer reads, in order. §8–§10 are the record, not instructions.
> `plan-evidence.md` holds the investigation; `log.md` holds the history.

---

## 1. Objective

Replace "enable/disable the randomizer" with **worlds**: named playthroughs, each
pairing a randomizer recipe with the save data produced by playing it. One world is
active at a time, Vanilla is a world, and activation swaps randomizer files and save
data together as one transaction that can be rolled back or refused. No randomizer
output changes.

---

## 2. Approved behaviour

| #   | Behaviour | Source |
| --- | --------- | ------ |
| B1  | The main screen has two text tabs, `WORLDS` and `DEFAULTS`, the active one boxed, with the active tab's name as a heading beneath | spec §2, D7 |
| B2  | The `WORLDS` rail lists `+ NEW WORLD`, `VANILLA`, then the player's worlds most recently played first. The first two rows are always present | spec §2 |
| B3  | The middle pane shows the highlighted world's name, seed, settings summary, save size, when it was last played, and whether it is active; the right pane explains the row | spec §2 |
| B4  | Exactly one row is marked `ACTIVE`; none is, until first-run capture has run | spec §2 |
| B5  | The `DEFAULTS` tab is today's Setup Defaults screen — what a new world starts from, plus the Bloodborne title ID | spec §2, D8 |
| B6  | `X` on `+ NEW WORLD` opens the world editor pre-filled from Defaults | spec §2 |
| B7  | `X` on an existing world opens the same editor with that world's current settings | spec §2 |
| B8  | `X` on `VANILLA` activates Vanilla: the randomizer's files are removed and the game runs unmodified | spec §2 |
| B9  | Activation does three things as one transaction: back up the live save to the outgoing world; generate and make current the incoming world's files; make the incoming world's save live, or start fresh | spec §2, §7.1 |
| B10 | A confirmation screen states, before anything happens: which world is deactivated and where its save goes, which is activated, whether its save is restored or started fresh, and roughly how long it takes | spec §2 |
| B11 | Activation then shows the existing progress log. No new result screen | spec §2 |
| B12 | A `SAVE` category in the editor holds one setting, `SAVE DATA`, valued `KEEP EXISTING` or `START FRESH`. The outgoing save is backed up first either way. It always defaults to `KEEP EXISTING`; `START FRESH` applies to one activation and then reverts, recorded as a revision, so the player chooses it deliberately every time | spec §2, D6, D17 |
| B13 | Every destructive save operation first writes a timestamped safety backup that is never overwritten and never auto-deleted, and **verifies** it — file count and per-file sizes against a manifest — before the live save is touched | spec §2, §7.1 |
| B14 | Editing a world appends a revision and makes it current. Earlier revisions remain and can be made current again, which itself appends a revision | spec §2, D2 |
| B15 | A world's save pairs with the world, not a revision. The revision current when the save was last written is recorded and shown | spec §2, D3 |
| B16 | Revisions are reached only through a `HISTORY` rail row in the editor, most recent first | D14 |
| B17 | Deleting a world asks for explicit confirmation, names what is destroyed, and keeps the world's save as a safety backup. The active world cannot be deleted | spec §2, D9 |
| B18 | A world stores a settings *recipe*, not generated files. Activation regenerates | D1 |
| B19 | Saves are copied between worlds, never shared by reference | D4 |
| B20 | Vanilla is a world: always present, no editable settings, no revisions, not deletable. Selecting it shows its save details and an explanation | D5, D16 |
| B21 | A world's identity is an opaque id assigned at creation; the name is editable metadata and duplicate names are permitted | D15 |
| B22 | Worlds belong to the account that created them and are listed only for that account | D12 |
| B23 | The active world is recorded inside `.bbrandomizer_manifest`, inside `dvdroot_ps4` | D11 |
| B24 | Activation runs the outgoing save's backup first and aborts the whole transaction if it does not complete, for any reason including a full disk. A backup that did not complete is never treated as one | D24 |
| B25 | A restore checks the owning account, the target title, the target directory and that the container is large enough **before writing a byte**, and refuses on any mismatch | spec §7.1, D23 |
| B26 | On first run the existing live save is captured into Vanilla before anything else happens | spec §7.1 |
| B27 | The Enable and Disable wizards are retired into this model | D10 |
| B28 | Output parity: a given seed and recipe produce the tree they produced before this feature | spec §7.3, §8 |
| B29 | The AFR title is the `BLOODBORNE TITLE ID` setting on the Defaults tab; the save-data title is discovered by searching. Neither is derived from the other, and the save-data title is never assumed to be the application title | spec §4 (3), §6, D25 |
| B30 | `KEEP EXISTING` means: restore **this world's own** save when it has one, and adopt the live save when it has none. A world's stored save is never overwritten with another world's. The confirmation screen says which of the two applies | D4, spec §2 |
| B31 | Left/Right switch tabs while focus is on the rail, on both tabs. Inside the settings pane Left/Right still changes a value. No new button is introduced | D19 |
| B32 | Randomizer files present with no manifest are shown as `UNMANAGED` rather than as an unmodified game, and first-run capture still puts the live save into Vanilla | D18 |
| B33 | The app never creates a save container. A world with a stored save cannot be activated until Bloodborne has created one; activation detects that up front and says so rather than failing midway | D21 |
| B34 | `START FRESH` empties the container by unlinking every `userdata*` and `backup*` and leaving `sce_sys`, after which the game starts a new playthrough | D20 |
| B35 | `sce_sys` is never written. A restore writes everything else | D22 |
| B36 | When the container exists but holds no save files, no safety backup is taken and the outgoing world's stored save is left exactly as it was. The confirmation screen says the outgoing world was left alone | D26 |
| B37 | A world targets the AFR title named by the `BLOODBORNE TITLE ID` setting and nothing else. A configured title that is not among the detected installs refuses, listing what was detected | D27 |

---

## 3. Constraints and invariants

### 3.1 Must be preserved

* **`technical-findings.md` §8.6 binds every save-data operation**, in full: never
  `CREATE2`, never `sceSaveDataDelete`, never a write under `sce_sys`; bracket
  every container write; a failed operation has changed something; check the
  container is large enough first.
* **A save mount opened read-only is never written to**, and no container is
  emptied or written outside the phase that intends it.
* **A safety backup is verified before the live save is touched**, never after.
* **Search results are selected by name, never by index** (findings §9).
* **Every save search and world listing is scoped to one user id.**
* **The AFR title and the save-data title are independent values.** Neither is
  derived from the other and neither is ever used in the other's place.
* **Output parity** — `EnemyRandomizerOptions` is still populated field by field
  from the same values; the run decision keeps its `||` chain and `SKIPPING …` lines.
* **The four progress-log constants** (`kEnemyFailPrefix`, `kPoolFellBackLine1`,
  `kPoolFellBackLine2`, `kNothingRandomizedLine`) keep their names and their
  `const char* const` form — `pool_verify.py` parses them out of the file by name.
* **`defaults.cfg` stays tolerant** — unknown keys ignored, absent key reads as the
  struct's own default.
* **`EnemyPoolTable.h`, `EnemySkipTable.h`, `BossPoolTable.h` are not
  regenerated** — their order is positional in every stored recipe.
* **`/data/GoldHEN/AFR/...` appears in exactly one file, `Game/AfrManager.cpp`.**
* **Layering** — orbis calls only in `app/src/Platform/`; no UI file calls SDL2 or
  an orbis API; nothing under `Randomizer/` learns about screens or panes.
* **`LIBS` gains nothing beyond the `-lSceSaveData` already there.**
* **Files are sized by reading to a short read**, never `st_size` (findings §9).

### 3.2 Out of scope

* Any change to randomization behaviour, the engine, the param or MSB passes, or
  the settings list beyond the one `SAVE` setting.
* Sharing a save by reference; import/export; re-binding a save to another account;
  a cross-account view; tab icons; artwork; chalice settings.
* `patch_sfo`-style rewriting of `param.sfo`; any privileged path; libjbc;
  reading the game's own `param.sfo`.
* `app/UI_BLUEPRINT.md`, `docs/randomization-feature-spec.md`,
  `docs/ps4-homebrew-findings.md` — the documentation stage owns those.
* Deleting or pruning safety backups. They accumulate; usage is shown.

### 3.3 Hazards

| Hazard | What to do | Evidence |
| ------ | ---------- | -------- |
| A world's save carries however many `backup*` files its playthrough accumulated, so restoring it over another world's leaves the surplus behind | Empty before every restore, per findings §8.4 | §E5.9 |
| A directory opened read-only reads as a file, silently losing its contents | Walk and reconcile per findings §8.2 and §9 | §E5.1 |
| The save-data title ID is not the application title ID | Discover it per findings §8.1; refuse on zero or multiple matches | §E4 M3 |
| `PARAMS` offsets are inferred from one save | Scan the blob for `CUSA#####`; if the two copies disagree, stop and report | findings §3 |
| Renaming `EnableWizardScreen.{h,cpp}` breaks three verifier parses that name the file | Update `pool_verify.py:533`, `settings_ui_verify.py:403` and `:677` in the same pass | §E2.5 |
| A seventh `SettingCategory` would appear on the Defaults screen, where a per-world save policy does not belong | `SettingCategory::Save` is editor-only; Defaults iterates its own category list | §E2.4 |
| Struct-layout changes without a clean rebuild have produced heap-corruption `SIGSEGV` | `make clean && make` in **every** milestone | `docs/build.md` |
| Any console that used the current build already has an AFR tree with no manifest | Treat seeded-without-manifest as `UNMANAGED`: no row `ACTIVE`, and say so | §E5.3 |
| Free space cannot be measured, so a backup can run out of disk with no warning | Copy into `<name>.partial/`, verify, write the manifest, rename to the final name; a failure aborts the transaction and anything still `.partial` is removed at startup | §E5.2 |
| A stored `START FRESH` that outlived its activation would decline to load that world's save on every future activation | Phase 7 reverts it to `KEEP EXISTING` as part of the same transaction, so it cannot survive a successful run (B12) | §E5.4 |

---

## 4. Implementation approach

> Chosen: a disk-derived world store under `/data/bbrandomizer/Worlds/`, a
> journalled activation that stages before it swaps, and the categorised settings
> screen repurposed as the world editor. Rejected alternatives: evidence §E3.

### 4.1 Storage layout

```
/data/bbrandomizer/
  defaults.cfg                     unchanged; the DEFAULTS tab edits it
  VanillaSource/dvdroot_ps4/       unchanged, read-only source
  activation.journal               present only while an activation is in flight
  SaveBackups/<saveTitle>_<dir>_<stamp>_<reason>/   manifest.txt + data/
  Worlds/acct-<16 hex account id>/
    worlds.cfg                     next_world_id only
    vanilla/  world.cfg  save/{manifest.txt,data/}
    w-0001/   world.cfg  rev-0001.cfg  rev-0002.cfg  save/{manifest.txt,data/}
```

* `world.cfg`: `name`, `created`, `last_played`, `last_played_revision`,
  `account_id`. `rev-NNNN.cfg`: `seed=` plus every setting, in `defaults.cfg`'s
  format and serializer. `bloodborne_title_id` comes from Defaults, not a revision.
* World id `w-NNNN`, allocated as `max(next_world_id, highest existing + 1)`, never
  reused. Vanilla's id is the literal `vanilla`. A rename never changes it.
* A revision is appended **only** when the recipe differs from the current one; a
  rename rewrites `world.cfg` and appends nothing.
* `manifest.txt`: `title_id`, `dir_name`, `account_id`, `blocks`, `files`,
  `bytes`, `captured`, `world`, `revision`, then one `f <relpath> <bytes>` line
  per file. Verification walks the copy and compares count, paths, sizes, total.
  Backups are written with the `.partial`-then-rename discipline of findings §8.2.
* Rail order: `last_played` descending, then never-played by `created` descending.

### 4.2 Deriving the active world

`.bbrandomizer_manifest` is `key=value`: `world_id`, `world_name`, `revision`,
`seed`, `title_id`, `account_id`, `written`. There is no stored active flag.

| On disk | State |
| ------- | ----- |
| No account world directory | **First run** — no row `ACTIVE`; run capture (B26) |
| `dvdroot_ps4` absent, or present but not seeded and without a manifest | **Vanilla active** |
| Manifest present, `world_id` known | **That world is active** |
| Manifest present with an unknown `world_id`, or seeded with no manifest | **Unmanaged** — no row `ACTIVE`; activation is offered as the fix |

### 4.3 The activation transaction

`Game/WorldActivation` is a resumable job shaped like `EnemyRandomizerJob`, so the
frame loop keeps drawing. The journal is `sceKernelFsync`'d before each phase.

1. **Check** — every refusal in §4.4. Nothing is written.
2. **Safety backup** — back up the live save into `SaveBackups/…_preactivate`,
   per findings §8.2 and §4.1. **Any failure aborts the whole transaction.**
   Skipped when no container exists, or none holds `userdata*`.
3. **Capture** — copy that verified backup, disk to disk, into the outgoing
   world's `save/`; set `last_played` and `last_played_revision`. Skipped whenever
   phase 2 was, leaving that world's stored save exactly as it was; the
   confirmation screen says so before the player commits (B36).
4. **Generate** — `EnemyRandomizerJob` into `…/dvdroot_ps4.staging`, then write
   `.bbrandomizer_manifest` inside it. Skipped when the incoming world is Vanilla.

5. **Swap files** — rename `dvdroot_ps4` → `.old`, `dvdroot_ps4.staging` →
   `dvdroot_ps4`, delete `.old`. Vanilla omits the second rename.
6. **Swap save** — per the table below.
7. **Commit** — if the policy was `START FRESH`, append a revision setting it back
   to `KEEP EXISTING` (B12); then delete the journal.

| Incoming world | `SAVE DATA` | Phase 6 does |
| -------------- | ----------- | ------------ |
| Is the outgoing world | `KEEP EXISTING` | Nothing: phases 2–3 already captured it |
| Has a stored save | `KEEP EXISTING` | Restore **that world's own** save, per findings §8.4, then verify the written set against the manifest |
| Has no stored save | `KEEP EXISTING` | Adopt the live save: leave the container as it is and copy the verified backup into the incoming world's `save/` — a copy, not a link (B19) |
| Any | `START FRESH` | Empty the container, per findings §8.3. Nothing is restored, and the incoming world's stored save, if any, is left untouched |
| Any, no container | either | Nothing. A world with a stored save was refused in phase 1; otherwise the game creates the container at next launch |

Every step here is bracketed per findings §8.6, and the confirmation screen names
which row applies.

**Reconciliation.** On startup the app removes every `SaveBackups/*.partial`, then
reconciles any `activation.journal` before listing anything. Phase ≤ 4: delete any
staging tree and the journal; nothing user-visible changed. Phase 5: if
`dvdroot_ps4.old` exists and `dvdroot_ps4` does not, rename it back, else the swap
completed; then continue at phase 6. Phase 6: re-run it from the journal's data,
and if that cannot complete, restore the named safety backup and report. The
journal carries `from_world`, `to_world`, `revision`, `save_policy`,
`save_title_id`, `save_dir_name`, `safety_backup`, `phase`.

### 4.4 Refusals — all checked in phase 1, before any write

| Check | Refuse when |
| ----- | ----------- |
| User | No foreground user, or no account id for it |
| Ownership | The incoming world's `account_id` is not the signed-in account (B22) |
| Save title | The findings §8.1 sweep yields zero, or more than one, save title (B29) |
| Save directory | The search returns more than one directory for the chosen title, or none where one is required |
| Container | The incoming world has a stored save and no container exists for the resolved save title. The message says Bloodborne must be run once first (B33) |
| Container size | The incoming world's manifest `blocks` exceeds the blocks the container reports (B25) |
| Manifest | The incoming world's stored save fails manifest verification |
| Source | The incoming world is not Vanilla and `VanillaSource/dvdroot_ps4` is missing |
| AFR title | The `BLOODBORNE TITLE ID` setting names a title `GameInfo::DetectAll` did not find. The message lists what was detected (B37) |
| AFR | The AFR root for the `BLOODBORNE TITLE ID` setting is not writable (B29) |

There is no free-space check — the platform offers no way to measure it. Phase 2
carries that weight instead (B24).

### 4.5 Screens and controls

Four screens, one `Screen` each. Sub-steps are internal modes, never `ScreenId`s.

* `WorldsScreen` — the `WORLDS` tab. Modes `Reconciling`, `FirstRunCapture`,
  `Browse`, `ConfirmDelete`. Its rail scrolls via `Controls.h`'s `ListLayout`.
* `SetupDefaultsScreen` — the `DEFAULTS` tab, plus a tab strip.
* `WorldEditorScreen` — `EnableWizardScreen` renamed and extended. Modes
  `Settings`, `EditName`, `EditSeed`, `Picker`, `History`, `Confirm`, `Progress`.
  Its rail is `NAME`, `SEED`, rule, the six existing categories plus `SAVE`, rule,
  `HISTORY`; `OPTIONS` activates, opening `Confirm`.
* World names: at most **16 characters** from `A–Z`, `0–9` and space, edited
  character by character like the title-ID editor (measured, §E4 M5).
* The active marker is a `FillRect` swatch in `Palette::Good` at the rail row's
  right edge, `ACTIVE` in the details pane. No new glyph.
* `TRIANGLE` deletes a world, after a confirmation naming it and stating its save
  is kept (B17). The details pane states when a world cannot be activated and why
  — no container yet (B33), or its stored save needs a larger one (B25).
* **Tabs switch on Left/Right while focus is on the rail**, on both tabs, in each
  screen's `UpdateRail`, which consumes neither input today. Left/Right inside a
  settings pane is unchanged and `L1`/`R1` are not re-bound.
* `Screen` gains `virtual std::string RequestedWorldId() const { return {}; }` so
  `Application::MakeScreen` can build the editor for a chosen world.

### What this reuses

| Existing code or tool | How it is used | Change |
| --------------------- | -------------- | ------ |
| `Platform/SaveDataProbe.{h,cpp}`, `UI/SaveProbeScreen.{h,cpp}` | The harness that hardware-tests milestones 1–3 | extend; delete in M6 |
| `Randomizer/FileIo.cpp` | `CopyDirRecursive`, `MakeDirsRecursive`, chunked read | as-is |
| `Randomizer/RandomizerDefaultsStore.cpp` | Its `key=value` writer and parser become the shared recipe serializer | extend |
| `UI/SettingsModel.{h,cpp}` | The editor's categories and rows | extend |
| `UI/EnableWizardScreen.{h,cpp}` | Rail, panes, pickers, seed editor, confirm, progress log all carry over | rename, extend |
| `UI/Controls.{h,cpp}` | Scroll maths, pane drawing, palette, `FillRect` | add `DrawTabs` |
| `Game/AfrManager.{h,cpp}` | The one owner of AFR paths | extend |
| `Randomizer/EnemyRandomizer.h` | Generation, with a staging output path | as-is |

---

## 5. Files and changes

| File | Change | Milestone |
| ---- | ------ | --------- |
| `app/src/Platform/SaveDataProbe.{h,cpp}`, `app/src/UI/SaveProbeScreen.{h,cpp}` | Harness operations for each milestone | 1–3 |
| `app/src/Platform/SaveData.{h,cpp}` | **New.** Identity, save-title discovery, search, mount, container read, empty-container, stepped backup/restore jobs, manifests | 1 |
| `app/src/Randomizer/WorldStore.{h,cpp}` | **New.** Worlds, revisions, world saves, safety backups, account scoping, first-run capture | 2 |
| `app/src/Randomizer/RandomizerDefaults.h` | Add `bool startFreshSave = false;` | 2 |
| `app/src/Randomizer/RandomizerDefaultsStore.{h,cpp}` | Extract the shared serializer/parser; add `start_fresh_save` | 2 |
| `app/src/Game/AfrManager.{h,cpp}` | Manifest read/write, staging path, swap, remove tree | 2 |
| `app/src/Game/WorldActivation.{h,cpp}` | **New.** Transaction, journal, refusals, reconciliation | 3 |
| `app/src/UI/Controls.{h,cpp}` | `DrawTabs` and the tab-strip geometry | 4 |
| `app/src/UI/WorldsScreen.{h,cpp}` | **New.** Tabs, rail, details, help, first-run and reconciliation views | 4 |
| `app/src/UI/SetupDefaultsScreen.{h,cpp}` | Tab strip, `DEFAULTS` heading, Defaults-only category list | 4 |
| `app/src/UI/Screen.h` | `ScreenId::Worlds`, `Defaults`, `WorldEditor`; `RequestedWorldId()` | 4 |
| `app/src/Application.cpp` | Screen factory, world id plumbing, startup reconciliation | 4, 6 |
| `app/src/UI/SettingsModel.{h,cpp}` | `SettingCategory::Save`, `SettingKind::SaveChoice`, the `SAVE DATA` entry and help, editor-only category list | 5 |
| `app/src/UI/WorldEditorScreen.{h,cpp}` | **Renamed** from `EnableWizardScreen.{h,cpp}`; name editor, `SAVE`, `HISTORY`, revision append | 5 |
| `app/src/UI/MenuScreen.{h,cpp}`, `app/src/UI/PlaceholderScreen.{h,cpp}` | **Deleted** | 6 |
| `app/src/Platform/SaveDataProbe.{h,cpp}`, `app/src/UI/SaveProbeScreen.{h,cpp}` | **Deleted**, with `ScreenId::SaveDataProbe` and the menu row | 6 |
| `app/tools/worlds_verify.py` | **New.** Manifest, refusal table, recipe round-trip, journal phase machine | 1–3 |
| `app/tools/settings_ui_verify.py` | New category and kind; the new screens' geometry and strings; renamed source file | 4, 5 |
| `app/tools/ui_scroll_verify.py` | Rows for the worlds rail, the details pane, the revision list | 4, 5 |
| `app/tools/pool_verify.py` | The progress-log constants' file name | 5 |

`app/Makefile` does not change. No saved data is invalidated: `defaults.cfg` gains
one key and keeps its tolerance for unknown ones.

---

## 6. Verification

### Build

`make clean && make` from `app/` in every milestone, producing
`IV0000-BBRD00001_00-BBRANDOMIZERMAIN.pkg`. Clean is mandatory: `RandomizerDefaults`
changes shape in milestone 2.

### Automated

| Check | Command | Asserts |
| ----- | ------- | ------- |
| Worlds mirror | `python app/tools/worlds_verify.py` | Manifests round-trip; a manifest disagreeing in count, path set, size or total fails; each §4.4 refusal fires on a constructed mismatch and on nothing else; a recipe round-trips to identical settings; every journal phase has exactly one reconciliation action and none is unhandled |
| Settings UI | `python app/tools/settings_ui_verify.py` | Every setting has exactly one category; `SAVE` is in the editor's list and not Defaults'; every new label, value, heading, tab and help string fits its column measured from the atlas; no rail row, rule, bar or footer ink box overlaps another on either new screen |
| Scroll geometry | `python app/tools/ui_scroll_verify.py` | The worlds rail, details pane and revision list each fit their band and clear their hints and footer |
| Output parity | `pool_verify.py`, `boss_verify.py`, `treasure_verify.py`, `drops_verify.py`, `starting_weapons_verify.py`, `caged_dogs_verify.py`, `easy_modes_verify.py`, `hunter_tools_verify.py`, `mergo_darkness_verify.py`, `itemdata_verify.py` | B28. All pass unchanged |

`worlds_verify.py` pins **the rules** — manifest format, refusal conditions, recipe
round-trip, phase coverage. It cannot prove the C++ implements them; only the
hardware tests do that.

### Hardware

The developer runs these; they are the handoff, not a step, and are ordered so each
is safe before the next (spec §8).

| # | Test | Pass condition | After |
| - | ---- | -------------- | ----- |
| H4 | Backup and restore through the production service | Backup, empty the container, restore, back up again; the two backups match file for file and in total bytes outside `sce_sys`. Elapsed times reported | M1 |
| H5 | First-run capture | On a console with a save and no worlds, the save becomes Vanilla's, its manifest verifies, the live save is unchanged | M2 |
| H6 | Activation round-trip | Play world A, activate B, play, return to A; A's progress is intact and its randomization is A's | M3 |
| H7 | `START FRESH` | The live save is backed up, the container is left holding `sce_sys` only, the game starts a new playthrough at next launch, and the world is back on `KEEP EXISTING` | M3 |
| H8 | Interrupted activation | Power off mid-activation; on next launch the app reconciles, loses no save, removes any `.partial` backup, and says what it did | M3 |
| H9 | Refusals | Activating a world with a stored save on a console where Bloodborne has never run refuses and says to run the game once; a second account sees none of the first's worlds | M3 |
| H10 | Tabs, listing, editor, delete | Left/Right on either rail switches tabs and inside a pane still changes a value; rail ordering; details figures; a rename appends no revision and a settings edit appends one; `HISTORY` lists newest first; delete keeps the save | M4, M5 |
| H11 | Full flow with the wizards gone | Create, activate, play, edit, re-activate keeping progress, activate Vanilla, return | M6 |

---

## 7. Milestones and stop conditions

Every milestone ends the same way: its steps are done, its verification passes,
`make clean && make` produces the `.pkg`, and it is handed to the developer for its
hardware tests. **Do not begin the next milestone before that.**

The platform mechanics these milestones rest on were established by a hardware
probe run before milestone 1; the four operations it proved are written up as
sequences in `technical-findings.md` §8. Its scaffolding —
`Platform/SaveDataProbe.{h,cpp}`, `UI/SaveProbeScreen.{h,cpp}` — is the harness
that hardware-tests milestones 1–3, and is deleted in milestone 6.

### Milestone 1 — the save-data service

**Goal.** `Platform/SaveData` as production code: discovery, identity, container
reads, backup, empty, restore, manifests.

**Changes**, in order:

1. `Platform/SaveData.h` — `ResolveUser`, `DiscoverSaveTitle`, `FindSaveDirs`,
   `ReadContainer`, `EmptyContainer`, `BackupJob`, `RestoreJob` — done when it
   compiles with no orbis type in the header.
2. Identity and discovery, findings §8.1 and §8.5, refusing on zero or multiple
   matches — done when the harness prints the save title, directory, account and
   AFR title.
3. `ReadContainer` — the findings §8.2 walk, returning file list, per-file sizes,
   total and block count; it is also the bracketing read — done when it prints for
   a container and for a missing one.
4. The manifest of §4.1 and its verifier — done when a backup produces
   `manifest.txt`, a re-verify passes, and an interrupted one leaves only a
   `.partial` directory.
5. `EmptyContainer`, findings §8.3 — done when the harness shows both bracketing
   listings and `sce_sys` survives.
6. `BackupJob` and `RestoreJob` as resumable jobs, one file per step, findings §8.2
   and §8.4 — done when the harness shows a per-file line and a refusal outside
   `sce_sys` fails the job.
7. Point the harness at the service — done when `SaveDataProbe.cpp` has no mount
   call of its own.
8. `app/tools/worlds_verify.py` covering the manifest cases — done when it passes.

**Invariants:** findings §8.6 in full; no orbis API outside `Platform/`.

**Verification:** build; `worlds_verify.py`; H4.

### Milestone 2 — the world store

**Goal.** Worlds, revisions, world saves and safety backups exist on disk, scoped
to the account, with Vanilla created and the live save captured into it.

**Changes**, in order:

1. Add `startFreshSave` to `RandomizerDefaults` and `start_fresh_save` to the
   store; extract the serializer and parser into functions shared by `defaults.cfg`
   and a revision file — done when `defaults.cfg` round-trips every existing key.
2. `Randomizer/WorldStore.{h,cpp}` — create, list, load, rename, delete a world;
   append and list revisions; `world.cfg` and `worlds.cfg`; account scoping keyed
   by `sceUserServiceGetNpAccountId` — done when the harness creates and lists
   worlds.
3. World save storage: store, load and verify a world's save through the milestone
   1 manifest — done when a stored save verifies.
4. Safety backups into `SaveBackups/…_<reason>/`, verified before the caller may
   proceed, plus a startup sweep of every `*.partial` — done when an interrupted
   backup leaves nothing usable.
5. First-run capture: when the account directory is absent, create it and Vanilla,
   then take, verify and copy in a safety backup only if a container with
   `userdata*` exists — done when both cases behave and a second run does nothing.
6. `AfrManager`: read and write `.bbrandomizer_manifest`, and expose the §4.2
   derivation — done when the harness prints the derived state for each row of
   that table.
7. Extend `worlds_verify.py` with recipe round-trip and world-store cases — done
   when it passes.

**Invariants:** account scoping; verify before any destructive operation; AFR paths
confined to `AfrManager.cpp`; `defaults.cfg` tolerance.

**Verification:** build; `worlds_verify.py`; H5.

### Milestone 3 — the activation transaction

**Goal.** A world can be activated end to end, with refusals up front, a journal,
and reconciliation after an interruption.

**Changes**, in order:

1. `Game/WorldActivation.h` — the job interface and a `Refusal` result carrying a
   reason and a human sentence — done when it compiles.
2. Phase 1's checks, every §4.4 entry, returning a refusal and writing nothing —
   done when the harness can trigger each one.
3. The journal: write, fsync, advance, delete — done when it appears and
   disappears around a run.
4. Phases 2–7 in order, every row of the phase-6 table and phase 7's revert — done
   when the harness activates a randomized world and Vanilla, a `START FRESH` run
   leaves the world on `KEEP EXISTING`, and a restore over another world's save
   leaves none of its files behind.
5. Reconciliation per §4.3, callable from the harness — done when a hand-written
   journal at each phase produces the documented action and `.partial` is swept.
6. Extend `worlds_verify.py` with the refusal table and the phase machine — done
   when it passes.

**Invariants:** nothing written before phase 1 passes; the safety backup verified
before phase 6, and its failure aborts the transaction; the container is emptied
before it is written; output parity — the options mapping and run decision
unchanged, only the output path differs.

**Verification:** build; `worlds_verify.py`; the full parity suite; H6–H9.

### Milestone 4 — the worlds screen

**Goal.** The tabbed main screen lists worlds, shows their details, and runs
first-run capture and reconciliation visibly.

**Changes**, in order:

1. `Controls`: `DrawTabs` and the tab-strip constants — done when
   `settings_ui_verify.py` measures both tab labels as fitting.
2. `UI/WorldsScreen.{h,cpp}` `Browse` mode: rail with `+ NEW WORLD`, `VANILLA` and
   the sorted worlds, scrolling; details pane per B3; help pane — done when the
   regions draw and navigate.
3. The active marker, the `UNMANAGED` case and the cannot-activate reasons of §4.5
   — done when each §4.2 state renders and a world needing an absent container
   says so.
4. `Reconciling` and `FirstRunCapture` modes, driven by the milestone 2 and 3 jobs
   and drawn with the progress-log layout — done when both complete on screen.
5. `SetupDefaultsScreen`: tab strip, `DEFAULTS` heading, Defaults-only category
   list — done when it draws inside the tab frame.
6. Tab switching on Left/Right while focus is on the rail, in both screens'
   `UpdateRail` — done when neither pane's Left/Right behaviour changes and both
   rails switch tabs.
7. `Screen.h` and `Application.cpp`: new `ScreenId`s, `RequestedWorldId`, startup
   reconciliation — done when tabs switch and the app opens on `WORLDS`.
   `MenuScreen` stays reachable this milestone.
8. Extend `settings_ui_verify.py` and `ui_scroll_verify.py` — done when both pass.

**Invariants:** the one-`Screen` pattern; no SDL2 or orbis call from `UI/`; no raw
AFR path in `UI/`; Left/Right inside a settings pane still changes a value.

**Verification:** build; `settings_ui_verify.py`; `ui_scroll_verify.py`; H10
(navigation and listing only).

### Milestone 5 — the world editor

**Goal.** Creating and editing a world, with a name, a `SAVE` category, and an
append-only revision history reachable through `HISTORY`.

**Changes**, in order:

1. `SettingsModel`: `SettingCategory::Save`, `SettingKind::SaveChoice`, the
   `SAVE DATA` entry with `KEEP EXISTING`/`START FRESH` value text, help saying
   the save is backed up first and that a fresh start empties the container, and
   the editor-only category list. `KEEP EXISTING` is the struct's own default and
   what an absent `start_fresh_save` reads as (B12) — done when
   `settings_ui_verify.py` passes and a new world opens on `KEEP EXISTING`.
2. Rename `EnableWizardScreen.{h,cpp}` to `WorldEditorScreen.{h,cpp}` and update
   `pool_verify.py:533`, `settings_ui_verify.py:403` and `:677` in the same pass —
   done when both verifiers pass against the new name.
3. The editor rail of §4.5, adjusting the rail constants until
   `settings_ui_verify.py` reports no overlap — done when it passes.
4. `EditName` mode: a 16-character editor over `A–Z`, `0–9` and space — done when
   a name round-trips through `world.cfg`.
5. Create and edit: `+ NEW WORLD` pre-fills from Defaults; `OPTIONS` writes the
   world and appends a revision only when the recipe differs — done when a rename
   appends none and a settings change appends one.
6. `History` mode: revisions newest first with seed and changed-setting count;
   selecting one makes it current by appending a new revision — done when the
   history grows rather than rewinds.
7. Delete: `TRIANGLE` plus a confirmation naming the world and stating its save is
   kept; refused on the active world — done when both cases behave.
8. Extend `ui_scroll_verify.py` with the revision list — done when it passes.

**Invariants:** the four progress-log constants' names and form; `X` never toggles
a setting; returning from a mode restores position; the pool tables are not
regenerated.

**Verification:** build; `settings_ui_verify.py`; `ui_scroll_verify.py`;
`pool_verify.py`; H10.

### Milestone 6 — activation from the UI, and retirement

**Goal.** The editor activates, and everything the worlds model replaces is removed.

**Changes**, in order:

1. Editor `Confirm` mode: the B10 statement — outgoing world and where its save
   goes, incoming world, and **which row of §4.3's phase-6 table applies** in the
   player's words (this world's save restored, the live save adopted, or a fresh
   start), or the §4.4 refusal that stops it in the details pane's words, plus a
   coarse duration — done when every clause draws from real data.
2. Editor `Progress` mode: run `WorldActivation` rather than `EnemyRandomizerJob`
   directly, reporting each phase into the existing progress log — done when an
   activation completes on screen.
3. `X` on `VANILLA` routes to the same confirmation — done when Vanilla activation
   runs from the rail.
4. Surface phase 7's revert in the editor (B12) — done when re-opening a world
   just activated with `START FRESH` shows `KEEP EXISTING` and `HISTORY` shows the
   appended revision.
5. Delete `MenuScreen.{h,cpp}`, `PlaceholderScreen.{h,cpp}`,
   `ScreenId::EnableWizard` and `ScreenId::DisableWizard` — done when nothing
   references them.
6. Delete `Platform/SaveDataProbe.{h,cpp}`, `UI/SaveProbeScreen.{h,cpp}`,
   `ScreenId::SaveDataProbe` and the `SAVE DATA PROBE (TEST)` row — done when
   `grep -r SaveDataProbe app/src` is empty. `-lSceSaveData` **stays**.
7. Run the full parity suite — done when every verifier passes.

**Invariants:** output parity; the AFR path stays in `AfrManager.cpp`;
`-lSceSaveData` remains the only library added to `LIBS`.

**Verification:** build; every check in §6; H11.

### Stop conditions

Halt and report rather than deciding, if any of these occur:

* A save-data call returns an error this plan does not name, at any point where
  the live save could have been modified.
* A bracketing read shows the container changed in a way the phase did not intend.
* Setting `CREATE2`, calling `sceSaveDataDelete`, or writing under `sce_sys` looks
  like the answer to anything.
* The two `CUSA#####` copies in a save's `PARAMS` blob disagree.
* More than one save directory is found for the resolved title.
* A verification check fails and the cause is not an obvious implementation slip.
* An implementation decision would contradict the spec or §2.
* The change needs a file not listed in §5.
* A §3.1 invariant cannot be preserved.
* A milestone would end without something the developer can test on hardware.

---

## 8. Open questions

*None.* The one milestone 3 raised — B37 making B8 a one-way door — was put to
the developer on 2026-09-24 and answered as **P26** below.

The two this plan raised were both spec gaps rather than plan choices,
and the developer answered both on 2026-09-22 as recommended. They are binding
in the spec as **D26** (a container holding no save files means no backup and no
capture) and **D27** (the `BLOODBORNE TITLE ID` setting alone decides the AFR
title, and a title that is not installed refuses), and recorded here as P24 and
P25. The heading is kept so the gap between §7 and §9 cannot be mistaken for an
editing accident.

---

## 9. Decisions

| # | Date | Decision | Taken by |
| - | ---- | -------- | -------- |
| P1 | 2026-09-21 | The active world is derived from `.bbrandomizer_manifest` alone; "Vanilla active" and "never used" are told apart by whether the account's world directory exists, not by a flag | planner |
| P2 | 2026-09-21 | A seeded AFR tree with no manifest is `UNMANAGED`, not Vanilla — no row shows `ACTIVE` | planner |
| P3 | 2026-09-21 | World ids are `w-NNNN`, one above the highest ever used, never reused; Vanilla's id is `vanilla` | planner |
| P4 | 2026-09-21 | A revision is appended only when the recipe differs from the current revision; a rename appends none | planner |
| P5 | 2026-09-21 | `SAVE DATA` is stored in the recipe; regeneration ignores it, so it cannot affect output parity | planner |
| P6 | 2026-09-21 | Save manifests carry count, relative path, per-file size and total only — exactly the verification spec §7.1 defines. No checksums | planner |
| P7 | 2026-09-21 | Activation stages into `dvdroot_ps4.staging` and swaps by rename; the save swap is the last phase, because a failed save swap is recoverable from a just-verified backup while a tree is regenerable from the recipe | planner |
| P8 | 2026-09-21 | `EnableWizardScreen` is renamed and extended into `WorldEditorScreen` rather than replaced | planner |
| P9 | 2026-09-21 | Deleting a world is `TRIANGLE`, the one button the control scheme leaves unassigned | planner |
| P10 | 2026-09-21 | World names are capped at 16 characters of `A–Z`, `0–9` and space — measured against the 580px rail at scale 3, and renderable by the `Font8x8` fallback | planner |
| P11 | 2026-09-21 | Milestones 1–3 are hardware-tested through the existing probe screen, promoted to an operations harness, so no milestone ends untestable and no UI is built on unproven storage | planner |
| P12 | 2026-09-21 | `SAVE DATA` always defaults to `KEEP EXISTING`; `START FRESH` applies to one activation and reverts afterwards as a recorded revision, so the player must choose it deliberately every time. Recorded in the spec as D17 | developer |
| P13 | 2026-09-21 | Tabs switch on Left/Right while focus is on the rail, on both tabs. `UpdateRail` consumes neither input today, so nothing is re-bound and `L1`/`R1` stay reserved for paging a picker. Recorded in the spec as D19 | developer |
| P14 | 2026-09-22 | Worlds are scoped by `sceUserServiceGetNpAccountId`, which hardware showed returns exactly the `ACCOUNT_ID` in the save's own `param.sfo` | developer |
| P15 | 2026-09-22 | A restore writes everything except `sce_sys`, whose four entries refuse with EACCES and hold only values already correct for the container. A refusal anywhere else fails the job | developer |
| P16 | 2026-09-21 | On a console whose randomizer output predates this feature, the live save is still captured into Vanilla and the app shows an `UNMANAGED` state. Raised by this plan as a spec gap; recorded in the spec as D18 | developer |
| P17 | 2026-09-21 | §1–§7 runs well past the ~400-line guidance. Accepted as one contract rather than split or trimmed: the overage is seven milestones' step lists, not leaked evidence or history | developer |
| P18 | 2026-09-22 | The app never mounts with `CREATE2`. A world's stored save is written only into a container Bloodborne has already created, and a world with a stored save is refused up front on a console where it has not | developer (D21) |
| P19 | 2026-09-22 | `START FRESH` unlinks every `userdata*` and `backup*` and leaves `sce_sys`. `sceSaveDataDelete` is not used anywhere | developer (D20) |
| P20 | 2026-09-22 | A restore empties the container before writing, using the same operation as `START FRESH`, so the previous occupant's surplus `backup*` files cannot survive into another world's playthrough | planner |
| P21 | 2026-09-22 | No pre-flight disk check. A safety backup is written into `<name>.partial/` with its manifest last and renamed on success; any failure aborts the whole transaction, and `.partial` directories are swept at startup | developer (D24), planner (mechanism) |
| P22 | 2026-09-22 | Every container write is bracketed by a read of the container's file list and total, before and after. A failed save-data operation is not a no-op, and the directory it names does not bound the damage | planner |
| P23 | 2026-09-22 | The AFR title and the save-data title are independent values, neither derived from the other | developer (D25) |
| P24 | 2026-09-22 | A container that exists but holds no save files skips phases 2 and 3 and leaves the outgoing world's stored save untouched, rather than capturing an empty save over a good one | developer (D26) |
| P25 | 2026-09-22 | The `BLOODBORNE TITLE ID` setting alone decides the AFR title. A configured title that `GameInfo::DetectAll` did not find refuses at phase 1, listing what was detected | developer (D27) |
| P26 | 2026-09-24 | **Supersedes the second sentence of P25/D27.** The `BLOODBORNE TITLE ID` setting alone decides the AFR title and is used as entered — AFR handling never infers or validates it. The §4.4 `AFR title` refusal and `RefusalReason::AfrTitleNotDetected` are removed in milestone 6, resolving the B37/B8 one-way door: `GameInfo::DetectAll` inspects the AFR overlay, which a Vanilla activation deletes, so it could never be evidence that a title is installed. Save-title discovery is unaffected — it keeps inspecting `param.sfo`, which is a different question | developer |

---

## 10. Changes during implementation

### Milestone 1 — 2026-09-22

**One deviation.** §7 milestone 1 step 7 asks that `SaveDataProbe.cpp` end up
with "no mount call of its own". Milestone 0's write probe was **deleted**
rather than rewritten to go through the service: its `RDWR|CREATE2` mount, its
`sceSaveDataDelete`, its `statvfs` dump and its scratch-directory guard cannot
be expressed through `Platform/SaveData`, because §3.1 forbids the service from
having any of them. The read-only probe and the two delete-by-prefix probes went
with it, being `ReadContainer` and `EmptyContainer` now. `SaveDataProbe.{h,cpp}`
are a 394-line harness containing no orbis call at all, which is P11's
"promoted to an operations harness". Everything the deleted code established is
in `technical-findings.md` §§1–7; the cost is that those diagnostics cannot be
re-run from this build.

Nothing else in §1–§7 was departed from. The thirteen choices the contract left
open — the `bbr::savedata` namespace, no fallback to the initial user, the
six-SKU sweep, a name-only walk inside `BackupJob`, verification as a separate
call, confirming only the written set, the restore's own refusals, a refused
unlink failing the operation, the harness as a stepped job, the AFR side read in
`UI/` rather than `Platform/`, backup naming left to the harness, no
`Randomizer/FileIo.h` include from `Platform/`, and `worlds_verify.py` covering
the source invariants as well as the manifest rules — are set out with their
reasoning in `implementation-report.md` §3.

### Milestone 2 — 2026-09-22

**Three deviations, and one knock-on the plan should be aware of.**

**1. `AfrManager` gained the manifest and the derivation, not the staging
path, the swap or the remove-tree.** §5's `AfrManager` row lists four changes
for milestone 2; §7 milestone 2 step 6 asks for two of them — "read and write
`.bbrandomizer_manifest`, and expose the §4.2 derivation" — and its
done-condition is about the derivation only. §7 was followed. The other three
are pure path plumbing with no caller until §4.3's phases 4 and 5, and
building them now would have added code no milestone-2 hardware test can
reach. They land in milestone 3, in the same file §5 names.

**2. `app/tools/pool_verify.py` was edited in milestone 2, not milestone 5.**
§5 assigns it to milestone 5 for the progress-log constants' file name. Step 1
of this milestone adds `start_fresh_save` to `defaults.cfg`, and
`pool_verify.py` carries an **exact equality** on that file's worst-case size —
"616 bytes", with a comment saying it "fails the moment a key is added or
removed without the buffer being thought about". It is doing exactly what it
was built to do. The figure is now 635, and the case was split in two because
the serializer was split in two: the 584-byte settings block is what
`char buf[1024]` holds, and the 635-byte total is what the file costs on disk.

**3. The milestone-2 harness operations live in `UI/SaveProbeScreen.{h,cpp}`,
not `Platform/SaveDataProbe.{h,cpp}`.** §5 lists both files as "harness
operations for each milestone". These operations drive
`Randomizer/WorldStore` and `Game/AfrManager`, and `Platform/` may include
neither — `WorldStore.h` already includes `Platform/SaveData.h`, so a
`Platform/` → `Randomizer/` include would be a cycle between layers. `UI/` is
the one layer both halves are reachable from, and milestone 1 already put the
AFR half of the harness there for the same reason. `SaveDataProbe` keeps the
save-data operations unchanged, so milestone 1's H4 instructions still hold
button for button.

**Knock-on: `settings_ui_verify.py` case 1 now fails, and was left failing.**
Adding `startFreshSave` to `RandomizerDefaults` in milestone 2 gives that
struct a seventeenth `bool` with no `SettingsModel` entry, which is precisely
what case 1 exists to catch. Milestone 5 step 1 adds the entry and its
done-condition is "done when `settings_ui_verify.py` passes", so the plan
already expects this failure to be closed by the entry rather than by an
exemption in the verifier. It will be red at milestone 4's gate too, where §6
lists that verifier — the developer may want to decide whether milestone 4
should absorb step 1 of milestone 5, or simply expect the one failure.

Nothing else in §1–§7 was departed from. The eighteen choices the contract
left open — `WorldStore` as an account-scoped class, the recipe as seed plus a
whole `RandomizerDefaults`, recipe identity as serializer equality, name
normalisation in the store, the shape of the extracted serializer, the
`.partial`/`.old` swap for a world's save, verification folded into
`SafetyBackupJob`, first-run capture refusing rather than spending its one run,
the world-save half of the `.partial` sweep, `Delete` not knowing which world
is active, `DeriveActive` as a pure function, a manifest with no `world_id`
reading as absent, `WorldStore.cpp` carrying its own file helpers, the harness
button map, the store test refusing to run before capture, the verifier's third
section and its pinned AFR-path allowlist, Vanilla's empty
`last_played_revision`, and the `acct-%016llx` directory name — are set out
with their reasoning in `implementation-report.md` §3.

### Milestone 3 — 2026-09-22

**Three deviations, and one contradiction between approved documents that the
developer has to settle before H11.**

**1. `CheckActivation` carries a thirteenth refusal, `EmptySelection`, that
§4.4 does not list.** It is the Enable wizard's two existing up-front guards —
`NO ENEMIES SELECTED` and `NO BOSSES SELECTED` — moved into phase 1 rather
than added as new behaviour. §3.1 requires the run decision to be unchanged,
and those guards are part of it: the enemy case fails after most of the tree
is written, and the boss case draws `RandIndex(rng, 0)`, which
`BossRandomizer.cpp`'s own header calls out as undefined behaviour rather than
a clean failure. Leaving them out would have made an activation able to invoke
UB after the safety backup and the capture had already run. They are checked
where every other refusal is, write nothing, and never fire for Vanilla.

**2. Phase 4 runs `EnemyRandomizerJob` for every non-Vanilla world, including
one with every setting off.** §4.3 says phase 4 is "skipped when the incoming
world is Vanilla" and nothing else, while §3.1 asks that the run decision keep
its `||` chain. The two cannot both be read as "skip the run when the chain is
false": a world with no tree has nowhere to put `.bbrandomizer_manifest`, so
§4.2 would derive it as Vanilla and B4's "exactly one row is marked ACTIVE"
would be false for it. §4.3 was followed and the chain is evaluated and
reported instead — with every option off the job mirrors the vanilla tree
unchanged, so the world plays exactly as the game shipped and is still a world
the app can see. The `||` chain and the options mapping are now pinned against
the wizard's, field for field, by `worlds_verify.py`.

**3. `PlanActivation` is a public function, not phase 1's private body.**
§7's step 2 asks for "phase 1's checks ... returning a refusal and writing
nothing" and its done-condition is that the harness can trigger each one. A
harness that triggered them by running the job would have had to stop it
before phase 2, which is the one thing a transaction must not offer. Phase 1
is therefore a function of its own that the job calls, the harness calls, and
milestone 6's confirmation screen (B10) will call.

**The contradiction: B37 makes B8 a one-way door.** `GameInfo::DetectAll`
detects a title by finding `<title>/dvdroot_ps4/event/common.emevd.dcx` inside
its AFR folder, and that tree exists only because the randomizer wrote it —
AFR is an overlay, not a copy of the game. Activating Vanilla removes it (B8,
§4.3 phase 5), after which `DetectAll` no longer reports that title, and
B37/§4.4's AFR-title row refuses every subsequent activation of a randomized
world for it. H11 — "activate Vanilla, return" — cannot pass as specified, and
nor can a first activation on a console whose AFR folder has never been
seeded. B37 was implemented exactly as written rather than reinterpreted, and
the harness prints the check's live verdict so the state is visible. **This
needs a spec decision before milestone 6**; `implementation-report.md` §2.4
sets out what was considered.

Nothing else in §1–§7 was departed from. The twelve choices the contract left
open — the refusal-reason enum and the order it is tested in, "zero save
titles" refusing with a sentence that still tells the player to run the game
once, the AFR title being taken from the setting on reconciliation rather than
carried in the journal, the journal being created at the start of phase 2, a
failure leaving the journal for the next launch, the staged tree rather than
`dvdroot_ps4.old` being what says whether phase 5 completed, `SetLastPlayed`
on the incoming world when phase 6 adopts the live save, `AdoptLive`
collapsing to nothing when the container holds no save files, `TakeLines`
beside the `EnemyRandomizerJob` job shape, reconciliation resuming through the
activation job itself, the two-button harness on `L1` and `R1`, and the
three-world `HARNESS A → HARNESS B → VANILLA` cycle — are set out with their
reasoning in `implementation-report.md` §3.

### Milestone 4 — 2026-09-23

**Two deviations, and one carried-over verifier failure left standing.**

**1. `ScreenId::SetupDefaults` was renamed to `ScreenId::Defaults` rather than
kept beside it.** §5 says `Screen.h` gains `Worlds`, `Defaults` and
`WorldEditor`, and says nothing about the id that already named the same
screen. Two ids for one screen is one more thing to keep in step, so the one
the plan names is the one that survives. Its single caller is
`UI/MenuScreen.cpp`, which §5 assigns to milestone 6 (deleted) — one token on
one line, no behaviour change, and flagged rather than buried because it is a
file this milestone was not given.

**2. `WorldsScreen` has three modes, not §4.5's four.** §7's milestone 4 step
list asks for `Reconciling`, `FirstRunCapture` and `Browse`. `ConfirmDelete`
belongs to **milestone 5 step 7**, which is the step that defines what
deleting does and whose done-condition is that both its cases behave; and
`TRIANGLE` has nothing to delete until that milestone can create a world. The
consequence is that milestone 5 step 7 will have to edit
`UI/WorldsScreen.{h,cpp}`, which §5's table lists only against milestone 4 —
the same table-versus-§7 gap milestone 2 recorded for `AfrManager`, not a new
file.

**`settings_ui_verify.py` is 60 of 61, and the one failure is milestone 2's.**
Check 1: `startFreshSave` has no `SettingsModel` entry. §6 lists that verifier
against this milestone, so it is red at this gate. It was **not** closed by
exempting the field, and milestone 5 step 1 was **not** pulled forward — the
first is a decision for the developer and the second is the next milestone.
All 21 cases this milestone adds pass, as do `ui_scroll_verify.py`,
`worlds_verify.py` and the whole output-parity suite.

**The B37/B8 contradiction milestone 3 raised was not touched.** It bears on
milestone 6; nothing built here activates anything.

Nothing else in §1–§7 was departed from. The eleven choices the contract left
open — the details pane as label/value rows plus at most one note, the header
band as where `UNMANAGED` and `FIRST RUN` are stated, `WorldsSession` as
`Application`-owned state so the save container is read once per launch rather
than once per tab switch, the startup log waiting for `X` only when something
happened, `O` exiting from `WORLDS` with `MenuScreen` reached through the
`DEFAULTS` tab, `X` on a world opening the Enable wizard with the world id
plumbed but not yet read, `X` on `VANILLA` doing nothing until milestone 6,
the `DEFAULTS` footer changing with focus, the two pre-existing
`/data/GoldHEN/AFR` occurrences left alone, whole-unit sizes with no decimal
point for the `Font8x8` fallback, and the rail's swatch clearance as a
verifier requirement rather than a draw-time clip — are set out with their
reasoning in `implementation-report.md` §3.

### Milestone 5 — 2026-09-24

**Four deviations, and one verifier failure closed rather than carried.**

**1. The rename touched `worlds_verify.py` too.** §3.3's hazard row names
three parses that break when `EnableWizardScreen.{h,cpp}` is renamed —
`pool_verify.py:533`, `settings_ui_verify.py:403` and `:677`. There are six.
`worlds_verify.py` names the file in three places milestone 3 added: the
AFR-path allowlist, the output-parity comparison pinning the activation job's
options mapping and `||` chain against the wizard's, and the four
progress-log constants' location check. All were updated in the same pass and
all still pass. Two smaller corrections to the row itself: the
`settings_ui_verify.py` lines had moved to `:520` and `:803`, and three source
files carried the old name in prose comments (`EasyModes.h`,
`EnemyRandomizer.cpp`, `SetupDefaultsScreen.h`).

**2. The editor's rail no longer shares Setup Defaults' pitch or category
band.** §4.4's "one geometry, shared by the wizard's Settings step and
`SetupDefaultsScreen`" cannot hold for §4.5's rail: Setup Defaults' is seven
rows and the editor's is ten — `NAME`, `SEED`, rule, seven categories, rule,
`HISTORY` — and ten rows on the shared 330/76 band put the last row's ink at
958, two pixels inside a column rule that ends at 960, with its focus bar
running past it. The editor's rail is its own uniform 70px grid from the
still-shared `kRailRow0Y = 230`, ending its ink at 900. Three constants —
`kRailRuleY`, `kRailFirstY`, `kRailPitch` — left `SHARED_GEOMETRY` and are
pinned by the verifier's own mirror instead, as `kHeaderY` already was; the
three-column split, the pane, the help column and the scales are all still
shared and still compared. The verifier now checks two rail stacks and also
pins the DEFAULTS rail's three constants, which the move would otherwise have
left unpinned. Visible consequence: the editor's category rows no longer line
up with the pane rows beside them. Setup Defaults' still do.

**3. `FINISH` and its readiness summary are gone, and `Application.cpp` was
edited.** §4.5's rail has no `FINISH` row — `OPTIONS` activates — and `FINISH`
was where the help pane drew the readiness summary. That summary is now the
first three rows of the Confirm list (`NAME`, `SEED`, `TARGET`), and
`SettingsModel`'s `FinishHelp()` is replaced by `NameHelp()` and
`HistoryHelp()`. `Application.cpp`, which §5 assigns to milestones 4 and 6,
gained two lines: milestone 4 plumbed `worldId` as far as `MakeScreen` and left
the constructor call to take it, which is this milestone's step 5. The menu's
`ENABLE RANDOMIZER` row now opens the editor on a new world, because that is
what the Enable wizard became (P8); the row and `ScreenId::EnableWizard` go in
milestone 6.

**4. The delete confirmation reports its own outcome before returning.** §4.5
says `TRIANGLE` deletes "after a confirmation naming it and stating its save is
kept" and says nothing about afterwards. Returning straight to the rail and
stating the outcome in the header band does not fit: `kStateY` is 140 and the
header rule is at 196, so a second scale-3 row's ink would run from 197 through
it. `X` deletes and stays to say what it did; `O` returns and re-reads the
rail. Step 7 also edited `UI/WorldsScreen.{h,cpp}`, which §5's table assigns
only to milestone 4 — the gap milestone 4's record already flagged, not a new
file.

**`settings_ui_verify.py` is back to green, 71 of 71.** Milestone 2's
carried-over check-1 failure — `startFreshSave` with no `SettingsModel` entry —
is closed by step 1's entry. The field was not exempted from the check.

Nothing else in §1–§7 was departed from. The twelve choices the contract left
open — `SaveChoice` as a second bool-backed kind rather than a third state on
`Toggle`, the editor iterating its own `kCategories` list, `SAVE` last on the
rail, `WORLD` as a new world's name, a new world's `SAVE DATA` forced to
`KEEP EXISTING` rather than inherited from `defaults.cfg`, a `HISTORY`
selection appending immediately rather than on the next `OPTIONS`, the
revision line's shape and its `CREATED` baseline, `O` returning to `WORLDS`
from both the rail and Progress, the last `OPTIONS` outcome stated on the
`NAME` pane and on Confirm, `History` as a full-screen mode rather than a
fourth column, the name editor's cursor bar, and `kCategoryRows` written twice
under a `static_assert` — are set out with their reasoning in
`implementation-report.md` §3.

### Milestone 6 — 2026-09-24

**Six deviations.** Five are files or steps §7's list does not name; the sixth
is what replaced a check that could no longer exist.

**1. P26 was implemented here, and §7's step list predates it.** The developer
took P26 on 2026-09-24, after §7 was written, so milestone 6's six steps do not
mention it. Removed: §4.4's `AFR title` row, `RefusalReason::AfrTitleNotDetected`,
`ActivationFacts::afrTitleDetected` and `::detectedAfrTitles`, the
`GameInfo::DetectAll` sweep in `GatherFacts`, the `GameInfo.h` include in
`WorldActivation.cpp`, and `worlds_verify.py`'s mirror of all of it — the
reason, the fact, the refusal case and the "vanilla still refuses on an
undetected AFR title" case. §2's B37 and §4.4's row are left as written, per
P26's own instruction not to edit §1–§9. **This meant editing
`Game/WorldActivation.{h,cpp}`, which §5 assigns to milestone 3.** The B37/B8
one-way door milestone 3's report §2.4 raised is closed by the removal: a
Vanilla activation deletes the AFR tree `DetectAll` reads, so nothing now
refuses on it and H11's "activate Vanilla, return" can run.

**2. `ScreenId::Menu` was removed with `MenuScreen`.** §7 step 5 names
`EnableWizard` and `DisableWizard` and not the id of the screen it deletes.
Leaving `Menu` would have left an id no factory could build and no screen could
reach — the dangling-id case the step's own done-condition is about.
`ScreenId::SaveDataProbe` went with step 6 as written.

**3. `UI/SetupDefaultsScreen.cpp` was edited, and §5 assigns it to milestone
4.** It held the last two `ScreenId::Menu` transitions — `OPTIONS` after a save
and `O` from the rail — and both now go to `ScreenId::Worlds`. Two tokens, no
behaviour beyond where the screen returns to; flagged rather than buried
because it is a file this milestone was not given. Exiting the app is now `O`
on the `WORLDS` rail, one press further on.

**4. Confirm's head rows went from three to eight and its list band moved, so
`settings_ui_verify.py` and `ui_scroll_verify.py` were edited here.** §5 lists
both against milestones 4 and 5. B10's statement is five more label/value rows
at the head of the same review list, and two wrapped rows of sentence above it;
at `kSettingsLayout`'s old `{420, 90, ...}` the list's `MORE ABOVE` hint drew
inside the second sentence row, which `ui_scroll_verify.py` caught. The layout
is now `{470, 80, 870, 60}` — the same six visible rows, the band cleared. Both
verifiers were extended rather than relaxed: 81 of 81 and all geometry passing.

**5. `worlds_verify.py`'s output-parity check lost the thing it compared
against.** It pinned the activation job's `EnemyRandomizerOptions` mapping and
`||` chain field for field against the Enable wizard's copy in
`WorldEditorScreen.cpp`, with a comment saying "milestone 6 deletes the
wizard's copy; until then the two must say the same thing". Step 2 deleted it.
The wizard's two lists are now **frozen in the verifier** — eighteen mapped
fields in order and the thirteen-field chain — so the one surviving copy is
still pinned, against exactly what it was pinned against before. Two new cases
assert the editor no longer carries a second copy at all. The verifier's
"harness refuses more than one save directory" case, which read the deleted
`SaveDataProbe.cpp`, now reads the same rule out of phase 1.

**6. The `SKIPPING …` lines and the generation's result lines stayed in the
editor.** §7 step 2 moves the run to `WorldActivation`; §3.1 requires the four
progress-log constants to stay `const char* const` in the file `pool_verify.py`
parses, and requires the run decision to keep its `SKIPPING …` lines. Moving
the reporting to `WorldActivation.cpp` would have taken the constants with it.
So `StartCommit` still emits the seven `SKIPPING` lines from `run_`, and
`FinishCommit` still reports the generation from
`WorldActivationJob::Result().generation` — the `||` chain and the options
mapping went, the reporting did not. `OPTIONS` on Confirm is refused whenever
`storeError_` is set, so the recipe the editor reports and the revision the
activation regenerates from cannot come apart.

Nothing else in §1–§7 was departed from. The fifteen choices the contract left
open — Vanilla routed through the editor opening on Confirm rather than a fifth
`WorldsScreen` mode, eight fixed head rows rather than a variable statement
block, the phase-6 row and the refusal sentence sharing one wrapped two-line
band at the row scale, the coarse duration as three phrases and the arithmetic
behind them, `OPTIONS` gated on `storeError_` as well as on the refusal, Vanilla
contributing no settings rows to Confirm, phase 1 run on Confirm's first frame
rather than on the `OPTIONS` press, the state line's three-way precedence, the
job's drained lines pushed into the log without a second `Log()`, `LoadWorld()`
re-read after a successful activation, `kSettingsLayout` at `{470, 80}`, the two
Confirm footers, a refusal and a failure ending the log differently,
`Game/GameInfo.{h,cpp}` left in place with no caller, and
`settings_ui_verify.py`'s 24-character stand-in for a refusal sentence's runtime
half — are set out with their reasoning in `implementation-report.md` §3.
