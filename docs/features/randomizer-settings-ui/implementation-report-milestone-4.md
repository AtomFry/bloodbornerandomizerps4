# Implementation Report — Randomizer Settings UI — milestone 4

**Status: AWAITING HARDWARE TEST**

**Plan:** `docs/features/randomizer-settings-ui/plan.md` — milestone 4, "the
categorised Enable wizard"

**Spec:** `docs/features/randomizer-settings-ui/spec.md`

**Implemented:** 2026-09-20

---

## 1. What was built

`EnableWizardScreen` is now the same categorised screen `SetupDefaultsScreen`
became in milestone 3, with two more rail rows: `SEED` above the six categories
and `FINISH` below them, each separated by its own rule. The 15 loose bools and
3 selection members are **one `RandomizerDefaults run_`**, copy-constructed from
`defaults_`, so one descriptor table now drives both screens. `Step::SaveData`
is `Step::Settings`, the three picker steps have collapsed into one, `GoToStep`
no longer resets anything, `ReturnFromPicker` is gone, and `DrawConfirm`'s
hand-written list of 19 settings is generated from the model. **No second
hardcoded list of settings exists in the file.**

| # | Planned change | Status | Where |
| - | -------------- | ------ | ----- |
| 1 | 15 bools + 3 selections → one `RandomizerDefaults run_`, copy-constructed from `defaults_` | done — verified field by field, §4 | `EnableWizardScreen.h:139-147`, `.cpp:150-158` |
| 2 | `Step::SaveData` → `Step::Settings`; delete `GoToStep`'s cursor/scroll reset and `ReturnFromPicker` | done | `EnableWizardScreen.h:68`, `.cpp:163-165`; `ReturnFromPicker` deleted |
| 3 | `Focus`, `railCursor_`, `lastCategory_`, `listCursor_[6]`, `listScroll_[6]`, the §4.2 input handling | done — `UpdateSaveData`'s left/right chain and `X` chain are both gone, and so is its `OPTIONS` branch (§2) | `EnableWizardScreen.h:70-135`, `.cpp:199-281` |
| 4 | Draw the settings step per §4.4 — header readout, `SEED` above the first rule, `FINISH` below the second | done, plus a right-aligned `TARGET` readout (§4.4's "header target") | `EnableWizardScreen.cpp:688-843` |
| 5 | Route the three pickers through the model, over the dimmed screen | done — one `Step::Picker` + `pickerSetting_`, as milestone 3's §10 row predicted (§2) | `EnableWizardScreen.cpp:317-328, 664-686` |
| 6 | Generate `DrawConfirm`'s list from the model — seed row, then 18 settings in category order | done — `ConfirmItems()`, 19 rows | `EnableWizardScreen.cpp:877-904` |
| 7 | Update `ui_scroll_verify.py`; extend `settings_ui_verify.py` to the wizard's eight-row rail and its Confirm list | done — 28 → 38 assertions | `app/tools/ui_scroll_verify.py:170-197`, `app/tools/settings_ui_verify.py` |

### Files changed by this milestone

| File | What |
| ---- | ---- |
| `app/src/UI/EnableWizardScreen.h` | rewritten — `Step::Settings`, `Focus`, the rail/pane cursors, `run_`, `pickerSetting_`, `confirmScroll_`; `titleId_` and the 18 per-setting members gone (146 → 181 lines) |
| `app/src/UI/EnableWizardScreen.cpp` | rewritten — the categorised Settings step, one picker step, model-generated Confirm; `StartCommit`/`FinishCommit`/`UpdateProgress`/`DrawProgress` carried across unchanged but for the `run_.` rename (985 → 959 lines) |
| `app/tools/ui_scroll_verify.py` | `Wizard SaveData` entry → `Wizard Settings` on `kPaneLayout`; comment block brought up to date. `Wizard Confirm` count already 19 and unchanged |
| `app/tools/settings_ui_verify.py` | extended: both screens' geometry compared, the verifier's own copy checked against them, the wizard's 8-row rail, the header band, the readiness summary, the Confirm list |
| `docs/features/randomizer-settings-ui/plan.md` | §10 only — four rows appended after milestone 3's |

Nothing else in the working tree is mine. `Renderer`, `Controls`, `ModelPicker`,
`SettingsModel`, `SetupDefaultsScreen`, the store, the engine and
`UI_BLUEPRINT.md` were not touched by this milestone.

### Invariants held

* **`StartCommit`'s run decision.** The same 13-term `||` in the same order,
  with `randomizeWorkshopTools` and `doNotRandomizeCagedDogs` still absent, and
  the same seven `SKIPPING …` lines. Diffed mechanically — §4.
* **`EnemyRandomizerOptions` field by field.** All 18 assignments, same fields,
  same source values, same order. Diffed mechanically — §4.
* **Only `lastSeed` is written back.** `defaults_.lastSeed = seed;` is the only
  assignment to `defaults_` anywhere in the file, and there is no assignment
  *over* `defaults_`. `run_` is never copied back.
* **The one-`Screen` pattern.** The seed editor, the picker, Confirm and
  Progress are still internal `Step`s. No step became a `ScreenId`.
* **The four progress-log constants** keep their names, their
  `const char* const kName = "...";` form and their file.
  `pool_verify.py` case 6 parses them and still passes.
* **Layering.** The only new drawing is `Renderer::FillRect` / `FillRectBlend` /
  `DrawText` through `Controls`. No UI file calls SDL2; nothing under
  `app/src/Randomizer/` mentions categories, help or panes (grepped).
* **`X` never toggles.** `X` in the pane opens a picker when `IsDrillIn` and
  does nothing otherwise; `AdjustSetting` is reached only from Left/Right.
* **Engine untouched.** No file under `Randomizer/`, `Msb/`, `Param/`, `Game/`
  was modified; all four engine selftests re-run and unchanged (§4).

---

## 2. Deviations from the plan

**Three. All are recorded in the plan's §10.**

### 2.1 The §4.4 geometry constants are duplicated, not promoted

Milestone 3 left them file-local in `SetupDefaultsScreen.cpp` and its report
§3.3 said milestone 4 must "either duplicate them or promote them". Promoting
them needs a shared layout file, and §5 assigns milestone 4 only
`EnableWizardScreen.{h,cpp}` plus the two tools — creating one would fire the
"the change needs a file not listed in §5" stop condition, for a change that
alters no behaviour. So `EnableWizardScreen.cpp` carries its own copy of the 32
shared constants and `kRuleColor`.

The protection is in the verifier, not in discipline.
`settings_ui_verify.py` now:

* parses **both** screens and fails if any of the 32 shared constants, or
  `kRuleColor`, disagrees with its twin;
* names the 32 explicitly, so renaming one on one side shrinks the comparison
  loudly rather than silently;
* checks its **own** 33 constants against the screens', because this file was a
  third copy and cases 5 and 6 measured against it. A geometry mirror that has
  gone stale passes a screen that is wrong, which is the one failure it must
  not have.

That last case is new work the plan did not ask for. It exists because
duplicating the constants is what made it necessary.

### 2.2 One picker step, not three

§7 step 5 says "route **the three pickers** through the model". There is one
`Step::Picker` plus a `const SettingDef* pickerSetting_`, exactly as milestone
3's §10 row predicted. `Step::EnemyPicker / SkipPicker / BossPicker` and
`Update/Draw{Enemy,Skip,Boss}Picker` are gone; the four `Selection*` functions
turn the one pointer into the table, the count, the flags and the vocabulary.
Keeping three steps would require a step→setting mapping, which is a settings
identity back in the screen.

**Consequence for a reviewer counting call sites:** milestone 2's "six host
draw sites" each doing their own `renderer.Clear` became four in milestone 3 and
are **two** now — one per categorised screen, each drawing the whole settings
screen (including its `Clear`) and then dimming it.

### 2.3 `OPTIONS` no longer opens Confirm from the settings step

The flat list treated `OPTIONS` as NEXT. B6 and spec §10 (9.2) say `OPTIONS`
commits **on Confirm only**, and §4.2's rail handling gives it no meaning, so
the only route to Confirm is now the `FINISH` rail row and `X`.

§7 step 3's done-condition names only the left/right chain and the `X` chain, so
this is called out rather than left for the reviewer to find. It is a control
change the player will feel, alongside `X` no longer toggling.

The `titleId_` member went with it: `run_.bloodborneTitleId` is the same value
from the same source (both were copies of `defaults.bloodborneTitleId` taken at
construction), and keeping both would re-create the parallel-copy problem this
feature exists to remove. §5 does not list it among the members to drop, hence
the note.

---

## 3. Decisions the plan left open

### 3.1 The footer line and its wording

§4.4 specifies "Footer y 1000, scale 3, one centred line" and no words. The line
is `UP DOWN MOVE   LEFT RIGHT CHANGE   X SELECT   O BACK` — 926 px of 1920,
asserted. It is Setup Defaults' line minus `OPTIONS SAVE`, which this screen
does not do: `OPTIONS` commits, on Confirm only. `X TOGGLE` is absent for the
same reason it is absent over there.

Confirm's own two footer lines are unchanged, including milestone 2's 30 px
move.

### 3.2 How the readiness summary is drawn

§4.2 says the help pane shows "the readiness summary (seed, target title ID,
`N OF M SETTINGS ENABLED`)" on `FINISH`; B9 says the same; Appendix A also gives
`FINISH` help text, and §6 case 4 requires it be non-empty. Drawing only one of
the two would leave the other unused.

Implemented as: title `FINISH`, then body lines

```
SEED  0001234567
TARGET  CUSA03173
7 OF 15 SETTINGS ENABLED
<blank>
Review the run and start randomizing. Shows the seed, the target
title ID, and how many settings are enabled.
```

— 8 of the 11 body lines the pane draws. Each summary line goes through
`WrapText` like everything else, so a long one folds rather than running off the
pane; the verifier asserts none of them needs to (worst 437 px of 440, the
enabled-count line).

`M` is `ToggleCount()` — 15, the toggles — not 18. The three drill-in lists have
no on/off state to count, and "18 OF 18 SETTINGS ENABLED" would be a lie the
moment a pool was narrowed.

### 3.3 The header's `TARGET` readout is `Palette::Dim`

§4.4 gives the header target its position, alignment and scale but no colour.
It is `Palette::Dim` against the seed readout's `Palette::Text`: both halves are
decorative, but the seed is the one the player is choosing and the target is
context. At scale 3 against scale 4 the sizes already say so; the colour agrees
with them rather than fighting them.

### 3.4 `confirmScroll_` as Confirm's own offset

§4.2 says "Confirm keeps its own `confirmScroll_`, zeroed only when Confirm is
entered from `Finish`" and does not say where it lives. It is a plain `int`
member beside the rail and pane cursors, written in exactly two places: the
`FINISH` branch of `UpdateRail` (the zeroing) and `UpdateConfirm`'s own
up/down. `selected_` and `scrollOffset_` — the two members that used to be
shared between the settings list and Confirm, and were the reason `GoToStep`
zeroed anything — are gone.

### 3.5 `ConfirmItems()` is a named method

The old `DrawConfirm` built its `items` vector inline. Generating it inline
would have worked, but the whole point of step 6 is that there is now **one**
list of settings; a named `std::vector<std::string> ConfirmItems() const` on the
header makes that visible where a reviewer looks for a second list, and gives
`UpdateConfirm` a single place to get the row count from
(`SettingCount() + 1`).

### 3.6 The `ui_scroll_verify.py` entry is renamed

`Wizard SaveData` → `Wizard Settings`. Milestone 2 deliberately kept the old
name to match §5's milestone-4 wording; §5's milestone-4 row is now done and the
step is `Step::Settings`, so the entry follows it. Its numbers are `kPaneLayout`
— byte for byte the same row as `Setup Defaults`, which is the point.

`Wizard Confirm`'s count is **19** and was already 19: milestone 1 set it when
the two save-data rows went, and the model-generated list is the same 19 rows
(seed + 18 settings). No change was needed, and the new
`6: Confirm lists 19 rows` case asserts the two mirrors agree rather than
trusting that.

### 3.7 Tombstone comments naming removed identifiers

`EnableWizardScreen.h:12` still names `Step::SaveData` and `.cpp:22-26` still names
`DrawSaveData`/`DrawConfirm`'s parallel vectors — both explaining what was
removed and why, matching this repo's habit and milestone 3's §3.8. No code,
constant or identifier of that kind survives; `settings_ui_verify.py` case 7
strips comments before searching, so these cannot make it pass or fail.

---

## 4. Verification run

### The field-by-field check of step 1

The task's highest-stakes step, and the one no verifier can close. It was done
**mechanically and by eye**, not by trusting the build.

The pre-edit `EnableWizardScreen.cpp` was reconstructed, the milestone's rename
(`randomizeEnemies_` → `run_.randomizeEnemies`, and so on for all 18 members
plus `titleId_` → `run_.bloodborneTitleId`) was applied to the **old** source,
comments were stripped, and the two were diffed:

```
=== StartCommit:  old 62 code lines, new 62
    IDENTICAL line for line after the run_ rename
=== FinishCommit: old 101 code lines, new 101
    IDENTICAL line for line after the run_ rename
```

Both functions are literally unchanged apart from where each value is read
from. Broken out, as the invariants require:

| Check | Result |
| ----- | ------ |
| `EnemyRandomizerOptions` assignments | **18 in the old, 18 in the new, same fields in the same order**, each reading the `run_` field with the same name as the member it replaced — including the three selections last (`enemiesIncluded`, `bossesIncluded`, `enemiesSkipped`) and the four `easyModes.*` sub-fields in their original order |
| The run-decision `||` | **13 terms, same order**: `randomizeEnemies, randomizeBosses, randomizeTreasure, randomizeEnemyDrops, randomizeStartingWeapons, randomizeStartingGuns, randomizeShopWeapons, enableMergoDarkness, startWithHunterTools, easyShadows, easyRom, easyFailures, easyEmissary`. `randomizeWorkshopTools` and `doNotRandomizeCagedDogs` are **still absent** |
| Is `run_` seeded the same way the 18 members were? | Every old ctor initialiser was `member_(defaults.member)` — checked mechanically, no exceptions. `run_(defaults)` copies every field from the same object at the same moment, so every value is identical at construction |
| Write-back | `defaults_.lastSeed =` is the only assignment to `defaults_`; no assignment over `defaults_` exists |
| The `SKIPPING …` lines | seven, same wording, same order, same conditions |
| The four progress-log constants | present, same names, same `const char* const kName = "...";` form, same file |

I read the before and after side by side, field by field, as well as running the
diff. They agree.

### Everything else

| Check | Command | Result |
| ----- | ------- | ------ |
| Clean build | `cd app && make clean && make` | `.pkg` built (7,143,424 bytes), **no warnings, no errors** |
| Settings model | `python app/tools/settings_ui_verify.py` | **38/38 passing** — the 28 from milestone 3 unchanged, plus 10 new |
| Scroll and geometry | `python app/tools/ui_scroll_verify.py` | **all geometry and scroll properties PASSED** (7 screens; `Wizard Settings` 8 rows visible of 4, rows y=330..862, clearances 17 px above and below — identical to `Setup Defaults`) |
| Pool, strings, config | `python app/tools/pool_verify.py selftest data/vanilla/dvdroot_ps4` | **88/88 passing** — no case changed state |
| Pool tables unchanged | `python app/tools/pool_verify.py table both data/vanilla/dvdroot_ps4` | PASS — 82 enemy models, 17 boss models |
| Engine mirrors | `boss_verify.py` | selftest **5/5**, unmodified tree CLEAN |
| Engine mirrors | `caged_dogs_verify.py` | **23/23 passing** |
| Engine mirrors | `easy_modes_verify.py` | **25/25 passing** |
| Engine mirrors | `hunter_tools_verify.py` | selftest **13/13** |
| Font atlas | `python app/tools/font_atlas_verify.py` | PASS — the metrics every width is measured from are still the baked ones |

The ten new `settings_ui_verify.py` cases:

```
6: both screens' 32 shared geometry constants are identical             ok
6: both screens draw their rules in the same colour                     ok
6: this file's 33 geometry constants match the screens'                 ok
6: the wizard's rail is 8 rows - SEED, six categories, FINISH           ok
5: the header's two readouts clear each other (1120 px apart)           ok
5: the wizard footer line fits the screen (926 px of 1920)              ok
5: each readiness line fits 440 px on one line (widest 437 px)          ok
5: the readiness block is 8 of the 11 body lines drawn                  ok
6: Confirm lists 19 rows - the seed row and all 18 settings             ok
5: every Confirm row fits the screen at scale 4 (widest 841 px,
   DO NOT RANDOMIZE CAGED DOGS   YES)                                   ok
```

The cross-screen case was negative-tested: perturbing one parsed constant makes
it report `['kRailFirstY']` and fail. It is not passing vacuously — 32 of 32
names were found in both files.

**Not run:** nothing in milestone 4's §7 verification list was skipped. The
`table both` and `font_atlas_verify.py` runs above are extra, not required by
this milestone.

---

## 5. What this does not prove

Everything above is a cross-compile and a set of Python mirrors. Per
`CLAUDE.md` §3 the PS4 is the only authority on runtime behaviour, and this is
**ready for hardware testing**, not working.

Specifically unproven:

* **Output parity — B17.** Nothing here can establish it. The mirrors show the
  option boundary is byte-for-byte the same code reading the same values in the
  same order; only the diff of two `dvdroot_ps4` trees proves it. That test is
  §6 below and it is the only thing that can close this feature.
* **Alpha blending, still.** `Renderer::FillRectBlend` has never run on this
  hardware; milestone 2's test that would settle it has not happened. Every
  picker on both categorised screens dims its parent with it, and §4.3's
  no-blend fallback was deliberately not implemented — that is the developer's
  visual decision.
* **That the screen draws what the arithmetic says.** The mirrors pin the rules,
  not the C++ applying them. A transposed x, a bar drawn after its text, or a
  pane drawn off-column would pass everything above.
* **That the rail's two extra rows land where they are measured.** `SEED` at
  y 230 and `FINISH` at y 806 are asserted against the ink box; whether the
  column reads as three groups rather than one long list is a TV judgement.
* **The readiness summary's legibility and usefulness.** Three short lines and a
  paragraph in a 440 px column, never seen.
* **Frame pacing on the wizard.** Same cost as Setup Defaults — roughly 43
  strings and 5 filled rects a frame against today's ~8 — on a CPU rasteriser.
* **That the font atlas is the live path on this build.** Every width assumes
  it. `live.log` must say `FontAtlasInit ok`.

---

## 6. Hardware test handoff

Install the `.pkg` from `app/IV0000-BBRD00001_00-BBRANDOMIZERMAIN.pkg`.

**0. Before anything else**, check `live.log` says `FontAtlasInit ok`. If it
says `texture build failed`, stop — the 8×8 fallback has no lowercase and no
`&`, so the help pane would be blank columns and the layout is not the defect
(plan §3.3).

### 1. Milestone 4's own test — `ENABLE RANDOMIZER` from the main menu

1. The screen should show: a centred `ENABLE RANDOMIZER` title; `SEED
   <10 digits>` under it on the left and `TARGET  CUSA03173` on the right; a
   horizontal rule; a left rail of `SEED <digits>`, a rule, the six categories,
   a rule, `FINISH`; the `ENEMIES` settings in the middle with values right
   aligned; help on the right; one footer line.
2. Up/Down on the rail: all **eight** rows reachable, wrapping at both ends.
   The middle pane changes with each category and **never goes blank** — moving
   onto `SEED` or `FINISH` must leave the previous category's settings showing.
3. **Left/Right on the `SEED` row rolls a new seed**, and **both** the rail row
   and the header readout change to match. This is the one place two things on
   screen show the same value.
4. `X` on the `SEED` row opens the digit editor; `X` accepts and `O` cancels;
   both return with the rail cursor still on `SEED` and the new value shown in
   both places.
5. `X` on a category moves focus into the pane. Up/Down moves the bar;
   Left/Right flips `YES`/`NO`; `O` returns to the rail with the same category
   selected. **`X` on a toggle must do nothing at all.**
6. `X` on `ENEMIES INCLUDED`, `ENEMIES SKIPPED` and `BOSSES INCLUDED` opens the
   picker **over a dimmed but still readable settings screen**. `O` returns to
   the same category and the same row, with the row's `N OF M` updated.
7. `FINISH` shows, in the help pane: the seed, `TARGET  <title id>`, and
   `N OF 15 SETTINGS ENABLED` — and the count must change as you turn toggles on
   and off.
8. **`OPTIONS` on the settings screen now does nothing.** `X` on `FINISH` is the
   only way to Confirm. This is deliberate (spec §10 9.2) and is the second
   control change after `X` no longer toggling.
9. Confirm lists `SEED` then all 18 settings **in category order** — enemies
   first, `ENABLE MERGO DARKNESS` last — six at a time with `MORE BELOW`.
   Scroll to the bottom and check the last row is not clipped by the footer.
10. `O` from Confirm returns to the settings screen with the rail cursor on
    `FINISH` and the pane still on the category you left.
11. `OPTIONS` on Confirm commits and the progress log runs as before.

### 2. The output-parity run — the only thing that can prove B17

This is the acceptance criterion for the whole feature, and it needs the
**shipped** build as well as this one.

1. On the shipped build, configure a run with a **fixed, typed seed** and a
   known set of toggles. Use a set that exercises the boundary: at least one
   enemy setting, `RANDOMIZE BOSSES`, one treasure setting, one starting-gear
   setting, one easy mode, a narrowed `ENEMIES INCLUDED` and a non-empty
   `ENEMIES SKIPPED`. Commit, and keep the resulting
   `/data/GoldHEN/AFR/<titleId>/dvdroot_ps4` tree.
2. Install this build, configure **the same seed and the same settings** —
   they are now reached through the categories, so tick them off against the
   list rather than by row position — and commit into a clean output folder.
3. Diff the two trees. **Byte-identical is the pass.** Any difference at all is
   a stop condition: report which files differ before anything else is
   investigated.
4. Also compare the progress logs: the same `SKIPPING …` lines in the same
   order, and the same result lines.

### 3. Accumulated and still untested — all four milestones

**None of milestones 1–4 has been on hardware.** This build carries four
milestones of untested change. Worth covering in the same session:

* *(milestone 1)* An existing `defaults.cfg` from the shipped build still loads
  with every other setting correct, and the progress log contains none of the
  five save-data lines.
* *(milestone 2)* Open any picker and press `SQUARE`: the confirm prompt must
  appear over a **visibly dimmed but still readable** list. **This is the
  alpha-blending decision, and every overlay on both new screens stands or
  falls with it.**
* *(milestone 2)* All three pickers still draw over a clean background, and
  Confirm's two moved footer lines are not clipped.
* *(milestone 3)* The whole `SETUP DEFAULTS` procedure in
  `implementation-report-milestone-3.md` §6 — `OPTIONS` still saves there and
  `O` still discards.

### What a failure looks like

* Picker opens over black, or the screen behind it vanishes or corrupts → alpha
  blending is unavailable. **Stop and report**; §4.3's scanline fallback is a
  design change only the developer can approve.
* The output-parity diff is not empty → **stop and report**, with the list of
  differing files. Do not adjust anything first.
* The header seed and the rail seed disagree after Left/Right → one of the two
  is reading a stale value; report which.
* `N OF 15 SETTINGS ENABLED` does not move when a toggle changes → the summary
  is reading the wrong object.
* A category's pane is empty, or a setting cannot be reached → the model and the
  struct have diverged; report which.
* Text clipped at a pane edge → report the exact string; every width is
  asserted, so a clip means the draw origin is wrong, not the wording.

---

## 7. Stop point

**Completion gate reached, no stop condition fired.** All five milestone-4
checks pass — clean build, `settings_ui_verify.py`, `ui_scroll_verify.py`,
`pool_verify.py selftest`, and all four engine selftests unchanged — the `.pkg`
builds, and the milestone is handed to the developer for the milestone-4
hardware test **including the byte-identical output-parity run**.

**This is the last milestone.** There is no milestone 5 to hold back from. The
feature is code-complete and unproven: everything now rests on §6.
