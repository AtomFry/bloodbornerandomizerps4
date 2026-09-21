# Feature — Randomizer Settings UI

**Status: APPROVED** — approved by the developer 2026-09-20.

**Reference:** New feature. No reference-tool equivalent — the Windows tool is a
single WPF window showing all settings at once, which is not a shape a
controller-driven 1080p screen can borrow.

**Plan:** `docs/features/randomizer-settings-ui/plan.md` — added after this spec
is approved.

**Folder naming:** unnumbered, like `docs/features/font-atlas/`. Every numbered
folder is named after a row in `docs/randomization-feature-spec.md`; this is
platform/UI work and has no backlog row. See `docs/features/README.md`.

---

## 1. What does this feature do?

It replaces the Enable wizard's single flat list of settings with a browsable,
categorised Settings screen.

Today every randomizer setting lives in one 21-row scrolling list that shows six
rows at a time. You can see 28% of your settings at once, and there is nowhere to
explain what any of them do. The backlog holds roughly 17 more settings, so the
list is heading past 35 rows.

The new screen groups settings into six categories on a persistent left rail,
shows the selected category's settings in the middle, and explains the selected
setting on the right. A `Finish` action at the foot of the rail leads to the
Confirm screen, which is unchanged in content: it summarises the selected
settings, exactly as it does today.

The same settings model also drives `SetupDefaultsScreen`, which has the same
21-row list and the same underlying problem.

This is a presentation and navigation change. **No randomizer behaviour changes,
no setting gains or loses meaning, and no output file changes.** A run configured
through the new screen must produce byte-identical output to the same run
configured through the old one.

---

## 2. What does the player experience?

### The Settings screen

A header band shows the target title ID. Below it, three regions:

* **Left rail** — a `SEED` row, then the six categories, left-aligned, plus a
  visually separated `Finish` action at the foot. The rail does not scroll. Rail
  rows are bare labels: a row's value is shown in the middle pane, not beside it.
* **Middle pane** — the selected category's settings, each showing its name and
  current value. Scrolls if a category outgrows the pane. When the rail cursor is
  on `SEED`, the pane shows the seed value instead.
* **Right pane** — contextual help for the selected setting: what it does, and
  what its default is.

The rail and the middle pane are the two focus states. `X` from the rail enters
the settings list; `O` returns to the rail with the category still selected.

Large selection lists (`Enemies Included`, `Enemies Skipped`, `Bosses Included`)
open as an overlay over the dimmed Settings screen — the parent stays visible
behind, as in Bloodborne's own Origin picker.

Returning from anywhere — an overlay, the seed editor, Confirm — restores the
position the player left, rather than resetting to the top.

### Controls

| Input | Meaning |
|---|---|
| Up / Down | Move within the focused region |
| Left / Right | Change the selected setting's value. Nothing on the rail |
| X | Advance: rail → settings, open a picker, open the seed editor, `Finish` → Confirm |
| SQUARE | Roll a random seed — **inside the seed editor only** |
| O | Return to the previous region or screen |
| OPTIONS | Commit (on Confirm only) |

**`X` no longer toggles a setting.** Today it both toggles and drills in
depending on the row; under this split, changing a value is always Left/Right and
`X` always means "go deeper." This is a deliberate change to existing muscle
memory, made so that one button has one meaning.

### Finish and Confirm

Selecting `Finish` shows a readiness summary in the detail pane — the seed, the
target title ID, and how many settings are enabled — so the player knows what
confirming would do before committing to look. `X` opens Confirm.

Confirm is the last screen before anything irreversible happens. It summarises
the selected settings, **exactly as it does today**, and `OPTIONS` commits. `O`
returns to the Settings screen at the category the player left.

Committing proceeds to the existing progress log, which also serves as the result
screen once the run finishes — unchanged from today.

---

## 3. What does the existing randomizer do?

Investigated in `app/src/UI/EnableWizardScreen.{h,cpp}`, `ModelPicker.{h,cpp}`,
`Controls.{h,cpp}`, `Screen.h`, `ScreenManager.h`, `Application.cpp`,
`SetupDefaultsScreen.{h,cpp}`.

### The actual current flow

```
SaveData ──OPTIONS──► Confirm ──OPTIONS──► Progress
   │  ▲                  │
   │  └────── O ─────────┘
   ├──X──► SelectReplace
   ├──X──► EditSeed
   └──X──► EnemyPicker / SkipPicker / BossPicker
```

**`Step::SaveData` is the settings editor.** Despite the name it holds all 21
rows: the backup toggle and replace choice, the seed, and every randomizer
setting. `Step::Confirm` is already a review screen — no cursor, Up/Down scroll
the window, `OPTIONS` commits, `O` returns.

> **Correction to an earlier claim made in conversation.** It was previously
> stated that Confirm was the settings editor and that `X` committed directly
> from it. That is wrong on both counts. The behaviour above is what this spec is
> written against.

**There is no Result step.** `Step::Progress` doubles as the result screen —
`UpdateProgress` says so directly: *"this is a result screen, not a step with its
own choices."*

**Navigation loses position.** `GoToStep()` unconditionally resets `selected_`
and `scrollOffset_` to 0. `ReturnFromPicker(row)` exists solely to undo that for
the three pickers. Returning from Confirm does not, so `O` there drops the player
at the top of a 21-row list.

**Everything is one Screen.** The wizard is a single `Screen` with an internal
`Step` enum. `ScreenManager` holds one screen and has no stack; `Screen.h` states
the flat `ScreenId` enum is deliberate because *"not enough screens exist yet to
justify a back-stack."* `ModelPicker` is explicitly **not** a `Screen`, because
`Application.cpp` rebuilds screens on every switch and a real drill-in would
destroy the wizard's in-progress toggles.

**This pattern is sound and must be preserved.** The categorised Settings screen
needs an internal focus stack, not a `ScreenManager` rework.

---

## 4. What do we know?

### 4.1 Settings are addressed by hardcoded row indices

Rows are integer constants — `kEnemiesIncludedRow = 12`,
`kStartWithHunterToolsRow = 16`, `kSaveDataRowCount = 21` — and the source
comments are emphatic that new settings must be **appended last**, because
inserting shifts every row below it and silently mislabels them. Two parallel
structures (the label vector and the `selected_ ==` chains) must stay in lockstep.

**This is the central obstacle.** Categories are a regrouping of rows, which is
precisely the operation the current design forbids. Positional indices must give
way to a settings model where each setting has a stable identity, a category, a
value accessor and help text. Until that exists, no amount of layout work
produces the proposed screen.

### 4.2 Every toggle is written twice

`UpdateSaveData` handles each setting once in the Left/Right chain and again in
the `X` chain — roughly 180 lines of near-identical branches for 16 toggles. A
settings model collapses this to one table, and the §2 decision that `X` no
longer toggles removes the second chain outright.

### 4.3 The renderer cannot draw a pane

`Renderer` exposes exactly `Clear`, `Present`, `DrawText`, `TextWidth`. There is
no filled rectangle, no line, no texture, no blend mode. Three panes, separators
and a dimmed overlay are all currently inexpressible.

`SDL_RenderFillRect` is nonetheless proven on hardware — `Font8x8.cpp` drew every
glyph pixel with it — and the font atlas work confirmed `SDL_RenderDrawLine`,
`SDL_SetTextureColorMod` and `SDL_SetTextureBlendMode` are present in the linked
`libSDL2.a`. The capability exists; the API surface does not. **Alpha blending has
never been exercised on hardware**, and the dimmed overlay depends on it.

### 4.4 Reusable abstractions already exist

* `Controls.h` — `ListLayout`, `VisibleRowCount`, `ClampScroll`, `ScrollToShow`,
  `DrawScrollHints`, `DrawScrollableList`, `NavigateVertical`. The scroll
  windowing maths is layout-agnostic and applies unchanged to the middle pane.
* `Palette` — `Heading`, `Text`, `Selected`, `Good`, `Bad`, `Dim`.
* `ModelPicker` + `PickerStrings` — already a host-owned component parameterised
  by vocabulary, already driving all three lists, already used by two hosting
  screens. What it lacks is drawing over a dimmed parent instead of clearing.
* `struct ReplaceOption { label; detail; }` — a per-row label-plus-detail pair
  already exists and is already drawn at `kDetailScale`. This is a precedent for
  the help pane, not a new idea.

### 4.5 Verifiers are coupled to the current UI

* `tools/ui_scroll_verify.py` hardcodes geometry for `Wizard SaveData`,
  `Wizard Confirm` and `Setup Defaults` as 21-row lists.
* `tools/pool_verify.py` selftest case 6 **parses string literals out of
  `EnableWizardScreen.cpp`** and asserts properties of them.

Both need rework, and `pool_verify.py`'s file-parsing coupling means renaming or
moving those constants breaks a verifier rather than a build.

### 4.6 Two font-era assumptions are now stale

* `ui_scroll_verify.py` still models glyph height as `8 * scale`. Actual line
  heights are 44/59/73/87 px for scales 3/4/5/6. It passes because it is
  internally self-consistent, **but it no longer models the font the app draws
  with.**
* `ModelPicker.h` and `EnableWizardScreen.cpp` carry character-budget comments
  ("71 characters fit a line at scale 3"; "A-Z, 0-9, space and `' ( ) - ,` and
  nothing else"). Both are obsolete: the atlas covers printable ASCII and is
  proportional, so character counts no longer predict width — `TextWidth()` does.

Neither blocks this feature, but a plan touching these files should correct them
rather than propagate them.

### 4.7 Settings are per-run and are not persisted

The wizard copies defaults into per-run members and writes back **only**
`lastSeed` at commit. Nothing else the player changes survives the run. This is
deliberate and documented, and this feature does not change it.

### 4.8 What "remove the save-data handling" covers

Per the 2026-09-20 decision (§10), the save-data handling is removed outright
rather than left dormant. It is presently unimplemented — every operation is
simulated — and is to be redesigned as separate work.

The full inventory, established by searching `app/src`:

| Location | What goes |
|---|---|
| `EnableWizardScreen` rows | `BACKUP EXISTING SAVE`, `REPLACE SAVE` (21 rows → 19) |
| `EnableWizardScreen` step | `Step::SelectReplace`, `UpdateSelectReplace`, `DrawSelectReplace` |
| `EnableWizardScreen` support | `ReplaceChoice`, `kReplaceOptions`, `kReplaceOptionCount`, `kNewSaveIndex`, `kLeaveExistingIndex`, `ReplaceDisplayText()`, `selectedBackupLabel_`, `backupExistingSave_`, `replaceChoice_` |
| `StartCommit()` | The simulated backup line and the three-way replace-choice block — "BACKING UP EXISTING SAVE (SIMULATED)", "SKIPPING BACKUP PROCESS", "REMOVING EXISTING SAVE DATA (SIMULATED)", "LEAVING EXISTING SAVE DATA", "RESTORING SAVE DATA … (SIMULATED)" |
| `SetupDefaultsScreen` rows | `BACKUP EXISTING SAVE`, `DEFAULT REPLACE SAVE` (21 rows → 19) |
| `RandomizerDefaults` | `backupExistingSaveData`, `replaceSaveDefaultIsNew` |
| `RandomizerDefaultsStore` | The `backup_existing_save` and `replace_save_default_is_new` keys, both read and write |
| `Application.cpp` | The startup log line reporting the backup default |

**Old `defaults.cfg` files stay loadable.** The store parses a known-key chain
and silently ignores anything it does not recognise, so a file still carrying
`backup_existing_save=1` loads fine with that line ignored — the same tolerance
feature 032 D1 relied on when it retired `unchanged_bell_maidens`. The keys are
simply no longer written on the next save.

**The wizard's entry step becomes the Settings screen.** With the two save rows
gone, `Step::SaveData` is nothing but the settings list, and the flow is
`Settings → Confirm → Progress/Result`.

**What is deliberately *not* removed:** the progress log's structure, and the
`Result`-as-`Progress` behaviour. Only the save-data lines within `StartCommit()`
go; the seed line, the randomizer steps and the completion lines are untouched.

---

## 5. Terminology

| Term | Meaning |
|---|---|
| **Rail** | The persistent left column of categories plus `Finish`. |
| **Category** | One of the six groups. A pure UI grouping — no effect on randomization. |
| **Overlay** | A list drawn over the dimmed Settings screen rather than replacing it. |
| **Readiness summary** | What the detail pane shows when `Finish` is selected. |
| **Dependent setting** | A setting with no effect unless another is enabled. Recognised here, but not given distinct treatment — see §11. |

---

## 6. Scope

### In scope

* The categorised Settings screen: rail, middle pane, help pane, header.
* A settings model giving each setting a stable identity, category, value and
  help text, replacing positional row indices.
* The six-category mapping (§7.1).
* Applying the same model to `SetupDefaultsScreen`.
* `Finish` as a rail action with a readiness summary.
* Removing the save-data handling in full — rows, the `SelectReplace` step, the
  simulated commit lines, both defaults fields and both config keys. Inventory in
  §4.8.
* The control scheme in §2.
* Selection lists as overlays over a dimmed parent.
* Navigation that restores prior position on return, everywhere.
* Whatever renderer primitives the layout requires.
* Updating `ui_scroll_verify.py` and `pool_verify.py` to match.
* Help text for every setting (drafted in Appendix A).

### Out of scope

* **Any change to randomizer behaviour or output.** Byte-identical runs are an
  acceptance criterion (§8).
* Adding, removing or renaming randomizer settings.
* **Dimming or disabling dependent settings** — explicitly deferred (§11).
* Redesigning save-data handling. The rows are being removed pending a rethink,
  not reimplemented.
* Artwork — textures, filigree, parchment. The layout must stand on geometry
  alone.
* Confirm's long-term scalability. It keeps today's full-list summary by
  decision; see §11.
* The Disable wizard (still `PlaceholderScreen`).
* Profile / save-data management, and persisting per-run settings.
* The PS4 on-screen keyboard.
* Chalice dungeon settings — out of scope project-wide.

---

## 7. Constraints and decisions

### 7.1 Category mapping

| Category | Settings (current) | Settings (backlog, when built) |
|---|---|---|
| **Enemies** | Randomize Enemies · Enemies Included · Enemies Skipped · Protect Caged Dogs | Randomize NPCs · No Team Type · Melee/Gun Movesets |
| **Bosses** | Randomize Bosses · Bosses Included | Per-Zone Toggles · Include Lesser Bosses · Bosses Can Replace Enemies |
| **Items & Treasure** | Randomize Treasure · Randomize Workshop Tools · Randomize Enemy Drops | Non-Key Overworld · Key Overworld (Logic) · Shop Items · Gems + Runes |
| **Weapons & Starting Gear** | Randomize Starting Weapons · Randomize Starting Guns · Randomize Shop Weapons · Start With Hunter Tools | — |
| **Difficulty** | Easy Shadows · Easy Rom · Easy Failures · Easy Emissary | No Scaling · Custom Scaling |
| **World** | Enable Mergo Darkness | Blood Decals · Face Data · Talk Data · VFX / AI Sound |

Two placements are deliberate and must not be "corrected" during implementation:

* **The four Easy modes go in Difficulty, not Bosses.** They are boss-arena
  edits, but they are not randomizers — the same seed yields the same world with
  them on or off — and they belong beside No Scaling / Custom Scaling.
* **Start With Hunter Tools is separated from Randomize Workshop Tools.** They
  act on the same two items and are constantly confused;
  `RandomizerDefaults.h` carries a comment specifically warning they are
  independent. Different categories is the fix, not an accident.

`SetupDefaultsScreen` uses the same six categories, plus whatever non-randomizer
rows it retains (at minimum `Bloodborne Title ID`).

### 7.2 Dependencies, recorded but not yet surfaced

| Dependent setting | Prerequisite |
|---|---|
| Enemies Included, Enemies Skipped, Protect Caged Dogs | Randomize Enemies |
| Bosses Included | Randomize Bosses |
| Randomize Workshop Tools | Randomize Treasure |

The third is from `RandomizerDefaults.h`: *"Only meaningful when
randomizeTreasure is also on."*

These relationships are **documented here but deliberately not given any distinct
UI treatment in this feature** (§11). Grouping alone already improves matters:
each prerequisite now sits at the top of its own category, directly above what it
governs. The help text in Appendix A states each dependency in words.

### 7.3 Constraints

* **Output parity.** No change to what a given seed and settings produce.
* **Preserve the one-Screen pattern.** The wizard's internal step machine and
  `ModelPicker`-as-component exist because `Application.cpp` rebuilds screens on
  switch, which would destroy in-progress state. Do not convert steps into
  `ScreenId`s.
* **Renderer layering.** UI must not touch SDL2 directly; new primitives go
  through `Renderer`.
* **No new libraries.** `LIBS` stays as it is (`ps4-homebrew-findings.md` §7).
* **Every setting stays reachable**, behind no more than one drill-in.
* **Font atlas is the text path**, with the 8×8 fallback still in place. Width
  must be measured with `TextWidth()`, never assumed from character counts.

---

## 8. How will we know it works?

### Automated testing

* **Output parity** — the strongest available check. A run with a fixed seed and
  a fixed set of toggles must produce output identical to the same run before the
  change. Existing verifiers (`boss_verify.py`, `pool_verify.py`,
  `caged_dogs_verify.py`, `easy_modes_verify.py`, `hunter_tools_verify.py`) must
  continue to pass.
* **Category completeness** — every setting in `RandomizerDefaults` appears in
  exactly one category, and every category entry maps to a real setting. Worth
  checking mechanically: the failure mode is a setting that silently becomes
  unreachable.
* **Help text coverage** — every setting has non-empty help text.
* **Geometry** — `ui_scroll_verify.py` extended to the new panes, and corrected
  to model the atlas's real line heights rather than `8 * scale` (§4.6).
* **Save-data removal is complete** — no identifier from the §4.8 inventory
  survives anywhere in `app/src`. This is greppable and should be asserted
  mechanically rather than eyeballed.
* **Old config files still load** — a `defaults.cfg` containing
  `backup_existing_save` and `replace_save_default_is_new` loads without error,
  those keys are ignored, and every other setting in the file is honoured. This
  is the regression most likely to slip through, because it only shows up with a
  config written by an older build.

### Hardware testing

* Every category reachable; every setting reachable, readable and changeable.
* The control scheme behaves as §2 describes — in particular that `X` no longer
  toggles, which is the change most likely to feel wrong in the hand.
* The overlay genuinely dims the parent and the parent remains visible.
  **Highest-risk item: alpha blending has never run on hardware.**
* Returning from an overlay, from Confirm, and from a category all restore
  position.
* Help text is legible at TV distance and fits its pane without clipping.
* A full commit still runs and the progress log is unchanged.
* Frame pacing: three panes plus overlays draw considerably more than the
  current single list, on a CPU rasteriser.

---

## 9. Open questions

*Deliberately empty.* Every question raised during the spec was answered on
2026-09-20; the answers are recorded in §10. The heading is kept so the gap
between §8 and §10 cannot be mistaken for an editing accident.

---

## 10. Decisions

| Date | Decision |
| ---------- | -------- |
| 2026-09-20 | Six-category model adopted, as listed in §7.1. |
| 2026-09-20 | `Finish` is a rail action, visually separated, not a seventh category. |
| 2026-09-20 | Seed lives in a persistent header, not inside a category. |
| 2026-09-20 | Navigation restores prior position on return, everywhere. |
| 2026-09-20 | Artwork is out of scope; the layout must stand on geometry alone. |
| 2026-09-20 | Output parity is an acceptance criterion — presentation changes only. |
| 2026-09-20 | **(9.1)** No new Result screen. `Progress` continues to serve as the result screen, exactly as today. |
| 2026-09-20 | **(9.2)** Controls: `X` advances, `O` returns, Left/Right changes values, `OPTIONS` commits. `X` no longer toggles a setting. |
| 2026-09-20 | **(9.3)** The `Backup Existing Save` and `Replace Save` rows are removed. Save-data handling is to be rethought as separate work; it is not being reimplemented here. |
| 2026-09-20 | **(9.1b)** The save-data handling is removed **in full**, not left dormant — UI rows, the `SelectReplace` step, the simulated commit lines, both `RandomizerDefaults` fields, and both `defaults.cfg` keys. The inventory is §4.8. Old config files remain loadable because unknown keys are ignored. Nothing is left unreachable-but-present. |
| 2026-09-20 | **(9.4)** Confirm summarises the selected settings exactly as it does today. No defaults diff, so no defaults-baseline question arises. |
| 2026-09-20 | **(9.5)** Dependent settings get no dimming or disabling in this feature. Deferred to future work (§11). |
| 2026-09-20 | **(9.6)** `SetupDefaultsScreen` adopts the same settings model and categories. |
| 2026-09-20 | **(9.7)** Help text is drafted in this spec (Appendix A) for developer review and adjustment. |
| 2026-09-20 | **(9.8)** The rail does not scroll. Revisit only if categories outgrow the pane. |
| 2026-09-21 | **(A1)** The seed value is shown in the **middle pane** when the `SEED` rail row is selected, and nowhere else — removed from both the header band and the rail row. It remains on `FINISH`'s readiness summary. The same treatment is applied to `BLOODBORNE TITLE ID` on `SetupDefaultsScreen`, so the two screens stay identical in behaviour. |
| 2026-09-21 | **(A2)** Left/Right no longer roll a seed from the rail. The seed changes only inside its own editor, reached with `X`. **SQUARE** rolls a random seed into that editor, where `X` still confirms and `O` still discards — rolling is never itself the irreversible step. |
| 2026-09-21 | **(A3)** Picker rows (`Enemies Included`, `Enemies Skipped`, `Bosses Included`) are drawn label-left / flag-right inside a fixed 1080 px band, replacing the centred space-padded string. The old padding only aligned in a monospace font and had been silently broken since the font atlas landed. |
| 2026-09-21 | **(A4)** The `World & Flavour` category is renamed **`World`**. |
| 2026-09-21 | **(A5)** The picker scrim is darkened from alpha 190 to **235**, so the settings screen behind a picker reads as a dim shape rather than competing text. The overlay constant is split in two: `kScrimAlpha` (235) for a screen covering another screen, and `kPromptAlpha` (190, unchanged) for the select-all prompt, whose job is the opposite — the list under it must stay readable, because the prompt is asking about those rows. |

---

## 11. Deferred to future work

Recorded here so they are visible decisions rather than oversights.

* **Dependent-setting treatment.** Dimming, disabling, or otherwise marking a
  setting whose prerequisite is off. The relationships are documented in §7.2 and
  stated in the Appendix A help text, but nothing in the UI distinguishes them.
  A player can still enable `Enemies Skipped` with `Randomize Enemies` off and get
  silence. Deferred by decision, not forgotten.
* **Save-data handling — removed entirely, to be redesigned.** Backup, replace
  and restore were never implemented; every operation was simulated. All of it is
  deleted here (§4.8) rather than carried forward, so the redesign starts from a
  clean slate instead of inheriting a stub. Until that work happens **the app
  does not touch save data at all**, which is a behaviour change worth stating
  plainly in `docs/user-guide.md`.
* **Confirm's scalability.** Keeping the full-list summary means Confirm faces the
  same growth pressure the Settings screen is being built to relieve — at 35+
  settings it becomes a scrolling wall again. Acceptable now; revisit when the
  backlog lands.
* **Rail scrolling**, if categories ever outgrow the pane.
* **Artwork** — panel textures and filigree over the geometry.

---

## Appendix A — Draft help text

**For developer review.** Drafted from the per-field comments in
`RandomizerDefaults.h`, which are unusually thorough. Wording is provisional;
adjust freely. Each entry is what the right-hand pane shows for that setting.

Every default below is `Off` unless stated, matching the struct's own defaults.

### Header

| Item | Help text |
|---|---|
| **Seed** | The number that determines this run's randomization. The same seed with the same settings always produces the same world. Left/Right rolls a new one; X edits it digit by digit. |

### Enemies

| Setting | Help text |
|---|---|
| **Randomize Enemies** | Replaces each enemy placement in the world with another creature drawn from the enemy pool. Does not affect bosses. *Default: Off.* |
| **Enemies Included** | Chooses which creatures may be used as replacements. Everything is included by default. Has no effect unless Randomize Enemies is on. |
| **Enemies Skipped** | Chooses which enemies are left exactly as the game placed them. Nothing is skipped by default. A skipped enemy can still appear elsewhere as a replacement. Has no effect unless Randomize Enemies is on. |
| **Protect Caged Dogs** | Leaves the caged dogs of Central Yharnam and the Forbidden Woods alone. This protects the ten cage *placements*, not the creature — Shaggy Hunting Dogs still appear elsewhere and still feed the pool. Replacements dropped into the Central Yharnam cages misbehave badly. *Default: Off.* |

### Bosses

| Setting | Help text |
|---|---|
| **Randomize Bosses** | Replaces each boss with another boss. Independent of Randomize Enemies — either, both or neither may be on. *Default: Off.* |
| **Bosses Included** | Chooses which bosses may be used as replacements. Everything is included by default. Has no effect unless Randomize Bosses is on. |

### Items & Treasure

| Setting | Help text |
|---|---|
| **Randomize Treasure** | Shuffles the items found lying in the world. *Default: Off.* |
| **Randomize Workshop Tools** | Adds the two workshop tools — the Blood Gem and Rune workshop tools — to the treasure shuffle instead of leaving them where the game put them. Only meaningful when Randomize Treasure is also on. Different from Start With Hunter Tools, which grants them outright. *Default: Off.* |
| **Randomize Enemy Drops** | Shuffles what enemies drop when killed. *Default: Off.* |

### Weapons & Starting Gear

| Setting | Help text |
|---|---|
| **Randomize Starting Weapons** | Randomizes the trick weapon choices offered in the Hunter's Dream. *Default: Off.* |
| **Randomize Starting Guns** | Randomizes the firearm choices offered in the Hunter's Dream. Independent of the trick weapon setting. *Default: Off.* |
| **Randomize Shop Weapons** | Randomizes the weapons sold by the Bath Messengers. *Default: Off.* |
| **Start With Hunter Tools** | Grants the Blood Gem and Rune workshop tools at character creation, so gems and runes work from the first area instead of sitting unusable until their chests turn up. Different from Randomize Workshop Tools, which shuffles them into the treasure pool. *Default: Off.* |

### Difficulty

| Setting | Help text |
|---|---|
| **Easy Shadows** | Replaces the duplicate bodies in the Shadows of Yharnam fight with harmless larvae, so it plays as a duel. Not a randomizer — the same seed gives the same world either way. *Default: Off.* |
| **Easy Rom** | The same, for Rom's attendant spiders. *Default: Off.* |
| **Easy Failures** | The same, for the Living Failures. *Default: Off.* |
| **Easy Emissary** | The same, for the Celestial Emissary's lesser emissaries. *Default: Off.* |

### World

| Setting | Help text |
|---|---|
| **Enable Mergo Darkness** | Cuts the scripted darkness in Mergo's Loft. Not a randomizer — it is a single fixed edit, and it applies whether or not anything else is on. Off leaves the area exactly as the game shipped it. *Default: Off.* |

### Rail action

| Item | Help text |
|---|---|
| **Finish** | Review the run and start randomizing. Shows the seed, the target title ID, and how many settings are enabled. |

---

## Appendix B — Research notes for stage C

* The settings model is the load-bearing change; layout is comparatively
  mechanical once settings have stable identities. A plan that starts with panes
  before the model hits §4.1 immediately.
* The §10 control decision (`X` no longer toggles) deletes the entire second
  branch chain in `UpdateSaveData` — the model change and the control change
  reinforce each other.
* `ModelPicker` is the closest existing thing to the overlay requirement. The gap
  is that it clears the screen rather than compositing over a dimmed parent.
* `Controls.h`'s scroll windowing needs no changes — already layout-agnostic and
  correct for the middle pane.
* Renderer primitives needed: filled rectangle, line/separator, and an
  alpha-blended full-screen fill. All three are in the linked SDL2; none is
  exposed through `Renderer` yet.
* `pool_verify.py` parses string literals out of `EnableWizardScreen.cpp` — check
  it before moving or renaming any constant in that file.
* Serving both `EnableWizardScreen` and `SetupDefaultsScreen` from one model
  means the model must express "this setting is editable here but not there" —
  the wizard edits a per-run copy, Setup Defaults edits the persisted defaults,
  and Setup Defaults carries rows (title ID) the wizard does not.
* Both screens drop from 21 rows to 19 once the save-data rows go. That happens
  to be the last moment the old flat lists exist, so it is worth sequencing the
  removal as its own step before the categorised screen lands — a smaller,
  independently testable change, and one that shrinks the surface the rest of the
  work has to move.
* `Application.cpp` logs the backup default at startup. It is the one save-data
  reference outside the two screens and the defaults store, and it is easy to
  miss.
