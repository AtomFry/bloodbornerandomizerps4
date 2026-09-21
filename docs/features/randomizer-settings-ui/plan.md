# Plan — Randomizer Settings UI

**Status: APPROVED** — approved by the developer 2026-09-20.

**Spec:** `docs/features/randomizer-settings-ui/spec.md` — **APPROVED**
(2026-09-20). Fourteen binding decisions, §10.

**Evidence:** `docs/features/randomizer-settings-ui/plan-evidence.md` — the
measurements, traces and rejected alternatives behind this plan.

**Plan review:** `docs/features/randomizer-settings-ui/plan-review.md` — added
during review.

---

> **This document is the implementation contract.** §1–§7 are what the
> implementer reads, in order. §8–§10 are the record, not instructions.
> `plan-evidence.md` holds the investigation; `log.md` holds the history.

---

## 1. Objective

Replace the Enable wizard's flat 21-row settings list and `SetupDefaultsScreen`'s
identical one with a categorised Settings screen — persistent header, six-category
left rail, settings pane, help pane — driven by one settings model in which every
setting has a stable identity instead of a hardcoded row index; and remove the
unimplemented save-data handling in full. No randomizer behaviour changes and no
output changes: the same seed and the same toggles must produce a byte-identical
tree.

---

## 2. Approved behaviour

| #   | Behaviour | Source |
| --- | --------- | ------ |
| B1  | Settings are grouped into the six categories of spec §7.1, in that order, with the within-category order that table gives | spec §7.1, §10 |
| B2  | A persistent header displays the seed and the target title ID. The header is decorative: it is never a focus target | spec §2, §10; §9 D1 |
| B3  | The left rail does not scroll and holds, top to bottom: a `SEED` row, a separator, the six categories, a separator, and `FINISH`. Neither the seed row nor `FINISH` is a category | spec §2, §10 (9.8); §9 D1 |
| B4  | The middle pane shows the selected category's settings, each with name and current value, and scrolls if the category outgrows it | spec §2 |
| B5  | The right pane shows contextual help for the selected item, from spec Appendix A | spec §2, §10 (9.7) |
| B6  | Up/Down moves within the focused region; Left/Right changes the selected setting's value; `X` advances; `O` returns; `OPTIONS` commits on Confirm only. **`X` never toggles a setting** | spec §2, §10 (9.2) |
| B7  | `Enemies Included`, `Enemies Skipped` and `Bosses Included` open as an overlay over a dimmed Settings screen; the parent stays visible behind | spec §2 |
| B8  | Returning from an overlay, the seed editor or Confirm restores the position the player left | spec §2, §10 |
| B9  | `Finish` shows a readiness summary — seed, target title ID, how many settings are enabled — and `X` opens Confirm | spec §2 |
| B10 | Confirm summarises the selected settings as today; `OPTIONS` commits, `O` returns to the category the player left | spec §2, §10 (9.4) |
| B11 | Committing proceeds to the existing progress log, which remains the result screen. No new Result screen | spec §2, §10 (9.1) |
| B12 | `SetupDefaultsScreen` uses the same model and the same six categories, with a `BLOODBORNE TITLE ID` rail row where the wizard has `SEED`, and no `FINISH` | spec §7.1, §10 (9.6); §9 D1 |
| B13 | Save-data handling is removed in full per the spec §4.8 inventory — rows, `Step::SelectReplace`, the simulated commit lines, both `RandomizerDefaults` fields, both `defaults.cfg` keys, the `Application.cpp` startup line. Both screens go 21 rows → 19 | spec §4.8, §10 (9.3, 9.1b) |
| B14 | A `defaults.cfg` still carrying `backup_existing_save` or `replace_save_default_is_new` loads without error, those keys are ignored, and every other setting in it is honoured | spec §4.8, §8 |
| B15 | Dependent settings get no dimming, disabling or other distinct treatment; spec §7.2 is documentation only | spec §10 (9.5) |
| B16 | No artwork; the layout stands on geometry alone | spec §10 |
| B17 | Output parity: a run configured through the new screen produces output identical to the same run configured through the old one | spec §1, §8, §10 |
| B18 | Per-run settings are still not persisted; the wizard writes back `lastSeed` and nothing else | spec §4.7 |

---

## 3. Constraints and invariants

### 3.1 Must be preserved

* **The one-`Screen` pattern** — `Application.cpp` rebuilds screens on every
  switch, so a step promoted to a `ScreenId` destroys in-progress state.
* **`StartCommit`'s run decision** — the same big `||`, with
  `randomizeWorkshopTools` and `doNotRandomizeCagedDogs` still absent from it, and
  the same `SKIPPING …` lines. It decides which runs write a tree at all.
* **`EnemyRandomizerOptions` is populated field by field, from the same values** —
  the boundary output parity rests on.
* **Only `lastSeed` is written back** — never assign the wizard's per-run settings
  object over `defaults_`.
* **The four progress-log constants in `EnableWizardScreen.cpp`**
  (`kEnemyFailPrefix`, `kPoolFellBackLine1`, `kPoolFellBackLine2`,
  `kNothingRandomizedLine`) keep their names, their
  `const char* const kName = "...";` form and their file — `pool_verify.py` parses
  them out of the source, as it does the three `PickerStrings` blocks in
  `ModelPicker.h`, which keep their wording and one-string-per-line form.
* **`EnemyPoolTable.h`, `EnemySkipTable.h`, `BossPoolTable.h` are not
  regenerated** — their order is positional in saved configuration.
* **`defaults.cfg` stays tolerant** — unknown keys ignored, absent key reads as
  the struct's own default.
* **Layering** — new drawing goes through `Renderer`; no UI file calls SDL2, and
  nothing under `app/src/Randomizer/` learns about categories, help or panes.
* **`LIBS` in `app/Makefile` is untouched** (`docs/ps4-homebrew-findings.md` §7).

### 3.2 Out of scope

* Any change to randomizer behaviour, the engine, or the param and MSB passes.
* Adding or removing randomizer settings.
* Dimming or disabling dependent settings (spec §11).
* Redesigning save-data handling; `app/UI_BLUEPRINT.md` describes that deferred
  design and is deliberately left alone.
* A `ScreenManager` back-stack, or turning wizard steps into `ScreenId`s.
* Artwork, the Disable wizard, profiles, the on-screen keyboard, chalices.
* Deleting `Font8x8.cpp`, or changing `pool_verify.py`'s `renderable()` /
  `LINE_CHARS` checks — those pin the *fallback* path and stay correct.

### 3.3 Hazards

| Hazard | What to do | Evidence |
| ------ | ---------- | -------- |
| Deleting two rows renumbers every row below on both screens; a constant and its `items` entry that disagree compile, pass `ui_scroll_verify.py` and mislabel every row below | In milestone 1 rewrite each constant block top-to-bottom beside its `items` vector in one pass. Three vectors move: `DrawSaveData`, `DrawConfirm`, `DrawList` | §E5.1 |
| Struct-layout changes without a clean rebuild have produced heap-corruption `SIGSEGV` on hardware | `make clean && make` in **every** milestone | `docs/build.md` |
| `pool_verify.py`'s worst-case `defaults.cfg` case is an exact equality (`worst == 669`) and fails the moment a key is removed | Update it to 616 in milestone 1. Its `unchanged_bell_maidens` case strips one exact comment line from the store source; if that line is edited, update the case with it | §E4 M6 |
| Alpha blending has never run on this hardware and the dimmed overlay depends on it | Milestone 2 exercises it on one existing prompt before any of the new screen is built, and names a no-blend fallback (§4.3) | §E5.2 |
| Every measurement here assumes the font atlas is the live text path. `Renderer` falls back to `Font8x8` silently if `FontAtlasInit` fails, and that font has no lowercase and no `&`, so the help pane would be blank columns | On each hardware handoff, confirm `live.log` says `FontAtlasInit ok`. If it says otherwise, stop — the layout is not the defect | §E5.3 |
| `ui_scroll_verify.py` models glyph height as `8 * scale`; the real line box is 44/59/73/87 px, and under the honest model two shipped lists no longer clear their footer | Milestone 2 replaces the model and adjusts only the constants §4.5 names. Do not weaken the model to make today's numbers pass | §E4 M2 |
| `ModelPicker::Draw` clears the screen; removing that without adding a clear at every host draws over the previous frame | Six host draw sites — change them in the same pass | §E2.3 |
| `GoToStep()` zeroes cursor and scroll unconditionally, which is exactly what B8 forbids | Delete the reset; each step owns its own cursor state (§4.2) | §E2.2 |

---

## 4. Implementation approach

> Chosen: one static descriptor table over `RandomizerDefaults`, addressed by a
> stable `SettingId` and reached through pointer-to-member, serving both screens
> because both hold a `RandomizerDefaults`-shaped object. Alternatives considered
> and why they were rejected: `plan-evidence.md` §E3.

### 4.1 The settings model — `app/src/UI/SettingsModel.{h,cpp}`

```
enum class SettingCategory { Enemies, Bosses, ItemsTreasure, WeaponsGear,
                             Difficulty, WorldFlavour, Count };
enum class SettingKind     { Toggle, EnemyPool, EnemySkip, BossPool };
enum class SettingId       { /* one per setting, stable, never reordered */ };

struct SettingDef {
    SettingId       id;
    SettingCategory category;
    SettingKind     kind;
    const char*     label;                      // the on-screen row label
    bool RandomizerDefaults::* flag;             // Toggle only; nullptr otherwise
    const char*     help;                       // spec Appendix A, non-empty
};
```

One `const SettingDef kSettings[]` holding all 18 settings, ordered by category
and then by the within-category order of spec §7.1. Declaration order **is**
display order; no parallel list and no row index exists anywhere.

Free functions in `namespace bbr` are the only way a screen reaches a setting:

```
int SettingCount();  const SettingDef& SettingAt(int);
const char* CategoryLabel(SettingCategory);   int CategorySize(SettingCategory);
const SettingDef& SettingInCategory(SettingCategory, int);
std::string SettingValueText(const SettingDef&, const RandomizerDefaults&);
void AdjustSetting(const SettingDef&, RandomizerDefaults&, int direction);
bool IsDrillIn(const SettingDef&);
bool* SelectionFlags(const SettingDef&, RandomizerDefaults&);
const ModelPoolEntry* SelectionTable(const SettingDef&);
int SelectionCount(const SettingDef&);
const PickerStrings& SelectionStrings(const SettingDef&);
int ToggleCount();   int EnabledToggleCount(const RandomizerDefaults&);
```

`SettingValueText` gives `YES`/`NO` for `Toggle` and `N OF M` for the pool kinds.
`AdjustSetting` flips a `Toggle` and is a no-op otherwise — **it is the only
writer of a setting.** The four `Selection*` functions are one `switch` on `kind`,
so no host repeats it; they need a `switch` rather than a pointer-to-member
because `EnemyPoolSelection`, `EnemySkipSelection` and `BossPoolSelection` are
three distinct types. `ToggleCount`/`EnabledToggleCount` feed the `Finish`
readiness summary.

Category labels: `ENEMIES`, `BOSSES`, `ITEMS & TREASURE`,
`WEAPONS & STARTING GEAR`, `DIFFICULTY`, `WORLD & FLAVOUR`. Setting labels are
today's shipped strings, unchanged (§9 D2). Help text is spec Appendix A,
transcribed with `-` for the dash and no italic markers.

The three rail rows that are not settings need help too, carried beside
`kSettings` rather than in either screen. `SEED` and `FINISH` take their Appendix A
entries; `BLOODBORNE TITLE ID` has none, so use:

> The PS4 title ID of the Bloodborne installation the randomizer writes for. Four
> letters and five digits, the shape the console itself uses. X edits it one
> character at a time. Default: CUSA03173, the Europe and Game of the Year
> release.

### 4.2 Focus and position

Both screens use one cursor over the left column and one cursor per category:

```
enum class Focus { Rail, List };
Focus focus_;
int   railCursor_;                  // 0 = SEED / BLOODBORNE TITLE ID row,
                                    // 1..6 = categories, 7 = FINISH (wizard only)
int   lastCategory_;                // 0..5, the category the pane keeps showing
int   listCursor_[6], listScroll_[6];
```

The rail column is the only focus target above the pane; the header band draws and
never takes focus.

* `Focus::Rail` — `NavigateVertical(railCursor_, railItemCount, input)` wraps for
  free, over 8 items on the wizard and 7 on Setup Defaults. `X` on 0 opens the
  seed editor (wizard) or the title-ID editor (Setup Defaults); on 1–6 sets
  `lastCategory_ = railCursor_ - 1` and enters `Focus::List`; on 7 goes to
  Confirm. Left/Right on 0 rolls a new seed on the wizard and does nothing on
  Setup Defaults or on any other row. `O` requests `ScreenId::Menu`.
* `Focus::List` — Up/Down move `listCursor_[lastCategory_]` with `ScrollToShow`
  updating `listScroll_[lastCategory_]`; Left/Right call `AdjustSetting`; `X`
  opens the picker when `IsDrillIn` and otherwise does nothing; `O` returns to
  `Focus::Rail` with `railCursor_` unchanged.

Moving the rail cursor onto 1–6 updates `lastCategory_`; moving it to 0 or 7 does
not. The pane therefore always draws `lastCategory_`'s settings, highlighting the
selected row only while `focus_ == List`.

Delete `GoToStep()`'s cursor and scroll reset and delete `ReturnFromPicker`:
every return path then restores position because nothing cleared it. Confirm keeps
its own `confirmScroll_`, zeroed only when Confirm is entered from `Finish`.

The help pane shows the readiness summary (seed, target title ID,
`N OF M SETTINGS ENABLED`) when `focus_ == Rail && railCursor_ == 7`; the rail's
first-row help when `railCursor_ == 0`; otherwise the selected setting's help.

### 4.3 Renderer primitives

`Renderer` gains exactly three methods, all built from SDL2 calls already present
in the linked `libSDL2.a`:

| Method | SDL calls | Status |
| ------ | --------- | ------ |
| `FillRect(x, y, w, h, r, g, b)` | `SDL_SetRenderDrawColor`, `SDL_RenderFillRect` | proven on hardware — `Font8x8` drew every glyph pixel this way |
| `FillRectBlend(x, y, w, h, r, g, b, a)` | `SDL_SetRenderDrawBlendMode(BLEND)`, `SDL_SetRenderDrawColor`, `SDL_RenderFillRect`, restore `BLENDMODE_NONE` | **unproven on hardware** |
| `int LineHeight(int scale) const` | `FontAtlasLineHeight`, or `8 * scale` on the fallback | arithmetic only |

Separators and rules are 2 px `FillRect`s; no `SDL_RenderDrawLine` is added, so
this change introduces exactly one unproven SDL entry point. `kOverlayAlpha = 190`
of 255, black. If milestone 2 shows blending does not work, the fallback is an
opaque 1 px-on/1 px-off scanline fill — do not implement it speculatively.

### 4.4 Layout

One geometry, shared by the wizard's Settings step and `SetupDefaultsScreen`, on
the 1920×1080 surface. Every value is asserted by `settings_ui_verify.py` (§6).

| Element | x / y | size / scale | Notes |
| ------- | ----- | ------------ | ----- |
| Rail column | x 60 | w 580, scale 3 | holds the widest row, `BLOODBORNE TITLE ID   CUSA03173` |
| Settings pane | x 680 | w 700, scale 3 | label at x 680, value right-aligned to x 1380 |
| Help pane | x 1420 | w 440, scale 3 | |
| Rules (2 px `FillRect`) | header `(60, 196, 1800, 2)`; columns `(660, 210, 2, 750)` and `(1400, 210, 2, 750)`; help `(1420, 330, 440, 2)`; rail `(60, 296, 580, 2)` and, wizard only, `(60, 778, 580, 2)` | | |
| Screen title | y 36 | scale 5 | `ENABLE RANDOMIZER` / `SETUP DEFAULTS` |
| Header readout | y 118 | scale 4 | `SEED  <10 digits>` / `BLOODBORNE TITLE ID   <id>`; **not focusable** |
| Header target | y 118 | scale 3 | `TARGET  <titleId>`, right-aligned to x 1860; wizard only |
| Rail row 0 | y 230 | scale 3 | `SEED   <10 digits>` / `BLOODBORNE TITLE ID   <id>` |
| Rail categories | y 330, pitch 76 | scale 3 | six rows, last at 710; same y and pitch as the pane rows |
| `FINISH` | y 806 | scale 3 | wizard only |
| Pane heading | y 220 | scale 4 | `lastCategory_`'s label |
| Pane rows | `kPaneLayout{330, 76, 880, 52}` | scale 3 | 8 visible rows |
| Help title | y 220, pitch 52 | scale 3 | selected item's label, wrapped, ≤ 2 lines |
| Help body | y 356, pitch 52 | scale 3 | wrapped, ≤ 11 lines |
| Footer | y 1000 | scale 3 | one centred line |

Rail row 0 and `FINISH` sit outside the category block, each separated from it by
a rule. The column's ink ends at y 850, inside its own column rule and clear of
the footer.

The focus highlight is a `FillRect` bar behind the focused row in a dark tint of
`Palette::Selected`, with the row's text in `Palette::Selected`. The bar must
contain the row's ink box and must not reach the adjacent row's bar; at pitch 76
with scale-3 text, `(rowY - 10, height 64)` satisfies both. Word wrapping is
greedy on spaces, measured with `Renderer::TextWidth` — never from character
counts.

### 4.5 Scroll hints and the corrected geometry model

`ListLayout` gains a fourth field `hintGap`; `DrawScrollHints` uses it and the
file-scope `kHintGap` is deleted. These are the only values that clear both the
rows and the surrounding furniture under the real atlas ink box (§E4 M2):

| Layout | firstY | spacing | bottomLimit | hintGap |
| ------ | -----: | ------: | ----------: | ------: |
| `kSettingsLayout` (wizard settings + Confirm) | 420 | 90 | 870 | 60 |
| `kListLayout` (Setup Defaults, until milestone 3) | 300 | 90 | 870 | 52 |
| `kProgressLayout` | 300 | 70 | 920 | 50 |
| `kPickerLayout` | 280 | 52 | 900 | 52 |
| `kPickerLayoutWithInstruction` | 332 | 52 | 900 | 52 |
| `kPaneLayout` (new, both categorised screens) | 330 | 76 | 880 | 52 |

The wizard's two footer lines move from `kScreenHeight - 130 / - 80` to
`- 100 / - 50` in `DrawSaveData` and `DrawConfirm`; without that move no `hintGap`
clears both the last row and the footer.

Correct, in the same pass, the obsolete comments claiming character counts predict
width — "71 characters fit a line at scale 3" in `EnableWizardScreen.cpp`, and the
same claim plus the "A-Z, 0-9, space and `' ( ) - ,`" note in `ModelPicker.h`. Say
instead that the atlas is proportional and covers printable ASCII, that width is
measured with `TextWidth()`, and that the `Font8x8` limits now bind only the
fallback path — which is what `pool_verify.py`'s `renderable()` still checks.

### 4.6 Overlays and the save-data removal

`ModelPicker::Draw` and `DrawConfirm` stop calling `renderer.Clear`; every host
draws its own background first, then
`FillRectBlend(0, 0, 1920, 1080, 0, 0, 0, kOverlayAlpha)`, then the picker.

The save-data removal deletes exactly the spec §4.8 inventory. `Step::SaveData`
becomes `Step::Settings`, so the flow is `Settings → Confirm → Progress`. The
`Application.cpp` startup line reports the loaded title ID instead of the backup
default. Nothing in `StartCommit` other than the backup line and the three-way
replace block is touched.

### What this reuses

| Existing code or tool | How it is used | Change needed |
| --------------------- | -------------- | ------------- |
| `Controls.h` `ListLayout`, `VisibleRowCount`, `ClampScroll`, `ScrollToShow`, `NavigateVertical` | the pane's scroll window and both cursors | `hintGap` field; add left/right-aligned label draws and a pane-local hint draw |
| `Controls.h` `Palette` | every colour | reuse as-is |
| `ModelPicker.{h,cpp}` | all three drill-in lists, now over a dimmed parent | drop the `Clear`; hosts supply the background |
| `SetupDefaultsScreen.cpp` `working_` | already a whole `RandomizerDefaults` — the shape the wizard adopts | reuse as-is |
| `Platform/FontAtlas.h` `FontAtlasLineHeight` | backs `Renderer::LineHeight` | reuse as-is |
| `Randomizer/RandomizerDefaults.h` | the struct the descriptor table points into | two fields removed |
| `tools/ui_scroll_verify.py` | scroll and geometry mirror | replace the height model; per-screen `hintGap`; new entries |
| `tools/pool_verify.py` | config-size and picker-string cases | arithmetic update and one new case |
| `tools/font_atlas_verify.py` | precedent for parsing `FontAtlasData.h` in Python | reuse its parsing approach |

---

## 5. Files and changes

| File | Change | Milestone |
| ---- | ------ | --------- |
| `app/src/UI/EnableWizardScreen.h` | drop `Step::SelectReplace`, `ReplaceChoice`, `backupExistingSave_`, `replaceChoice_`, `selectedBackupLabel_`, `ReplaceDisplayText`, the two `SelectReplace` methods | 1 |
| `app/src/UI/EnableWizardScreen.cpp` | drop the two row constants and renumber the rest 0–18, `kSaveDataRowCount` 19; drop `ReplaceOption`, `kReplaceOptions`, `kReplaceOptionCount`, `kNewSaveIndex`, `kLeaveExistingIndex`, `Update/DrawSelectReplace`, two ctor initialisers, two entries in both `items` vectors, `StartCommit`'s backup line and replace block | 1 |
| `app/src/UI/SetupDefaultsScreen.h` | drop `kBackupRow`, `kReplaceDefaultRow`, renumber, `kItemCount` 19 | 1 |
| `app/src/UI/SetupDefaultsScreen.cpp` | drop the two `ToggleRow` branches and the two `items` entries | 1 |
| `app/src/Randomizer/RandomizerDefaults.h` | drop `backupExistingSaveData`, `replaceSaveDefaultIsNew` | 1 |
| `app/src/Randomizer/RandomizerDefaultsStore.cpp` | drop both keys from load and from the save format string and its argument list; extend the unknown-key tolerance comment | 1 |
| `app/src/Application.cpp` | replace the backup-default startup line with a title-ID one | 1 |
| `app/tools/ui_scroll_verify.py` | three 21-row entries → 19; progress-log budget 20 → 18 | 1 |
| `app/tools/pool_verify.py` | worst-case `defaults.cfg` 669 → 616; new case asserting neither key survives in load or save | 1 |
| `docs/user-guide.md` | remove the two settings rows and the save-data prose; state that the app does not touch save data | 1 |
| `app/src/Platform/Renderer.{h,cpp}` | **new methods** `FillRect`, `FillRectBlend`, `LineHeight` | 2 |
| `app/src/UI/Controls.{h,cpp}` | `ListLayout::hintGap`; delete `kHintGap`; add `DrawLabelLeft`, `DrawLabelRight`, `DrawPaneScrollHints` | 2 |
| `app/src/UI/ModelPicker.{h,cpp}` | remove both `renderer.Clear` calls; correct the stale character-budget comments; **do not touch the three `PickerStrings` blocks** | 2 |
| `app/src/UI/EnableWizardScreen.cpp` | hosts clear + dim before `picker_.Draw`; `kSettingsLayout` `hintGap 60`; both footer lines move 30 px down; stale comment corrected | 2 |
| `app/src/UI/SetupDefaultsScreen.cpp` | same for its three picker branches; `kListLayout` `hintGap 52` | 2 |
| `app/tools/ui_scroll_verify.py` | replace `8 * scale` with the ink box parsed from `FontAtlasData.h`; per-screen `hintGap`; assert hints clear rows as well as furniture | 2 |
| `app/src/UI/SettingsModel.{h,cpp}` | **new** — the descriptor table and its accessors | 3 |
| `app/src/UI/SetupDefaultsScreen.{h,cpp}` | rewritten on the model: decorative header, rail with `BLOODBORNE TITLE ID` as row 0, pane, help, overlays | 3 |
| `app/tools/settings_ui_verify.py` | **new** — model coverage, help coverage, text fitting, pane geometry | 3 |
| `app/tools/ui_scroll_verify.py` | `Setup Defaults` entry becomes `kPaneLayout` | 3 |
| `app/src/UI/EnableWizardScreen.{h,cpp}` | rewritten on the model: `run_` as a `RandomizerDefaults`, decorative header, rail with `SEED` and `FINISH`, pane, help, overlays, Confirm generated from the model | 4 |
| `app/tools/ui_scroll_verify.py` | `Wizard SaveData` entry becomes `kPaneLayout`; `Wizard Confirm` count 19 | 4 |
| `app/tools/settings_ui_verify.py` | extend to the wizard's eight-row rail and its Confirm list | 4 |

No `Makefile` change: `SRC_CPP` uses `find`, so `SettingsModel.cpp` is picked up
automatically. No saved data is invalidated — the two dropped keys are ignored on
load and every other key keeps its meaning and its encoding.

---

## 6. Verification

### Build

`cd app && make clean && make` in **every** milestone. Milestones 1 and 4 change
struct layouts the UI and the engine each hold copies of, and the Makefile has no
header dependency tracking. The `.pkg` must be produced.

### Automated

| Check | Command | Asserts |
| ----- | ------- | ------- |
| Scroll and geometry | `python app/tools/ui_scroll_verify.py` | every list's rows, hints and furniture clear each other under the real atlas ink box |
| Settings model | `python app/tools/settings_ui_verify.py` | the eight cases below |
| Pool, strings, config | `python app/tools/pool_verify.py selftest data/vanilla/dvdroot_ps4` | every existing case, plus the 616-byte config case and the save-data-removal case |
| Pool tables unchanged | `python app/tools/pool_verify.py table both data/vanilla/dvdroot_ps4` | the baked tables still match the engine pool |
| Engine mirrors unchanged | `boss_verify.py`, `caged_dogs_verify.py`, `easy_modes_verify.py`, `hunter_tools_verify.py`, each `selftest data/vanilla/dvdroot_ps4` | no engine behaviour moved |
| Font atlas | `python app/tools/font_atlas_verify.py` | the metrics the geometry model is parsed from are still the baked ones |

`settings_ui_verify.py` must assert:

1. Every `bool` settings field of `RandomizerDefaults` appears in exactly one
   `kSettings` entry, and every entry's `flag` names a field that exists.
2. Every `SettingId` appears exactly once; no two entries share an `id`.
3. The six categories, their order, their membership and the within-category order
   match spec §7.1 exactly — including the two placements §7.1 says must not be
   "corrected".
4. Every entry, plus `SEED`, `BLOODBORNE TITLE ID` and `FINISH`, has non-empty
   help text.
5. Measured from the advances in `FontAtlasData.h`, never from character counts:
   every rail row — the six category labels, `FINISH`, and rail row 0 at its
   widest value — fits 580 px at scale 3; every `label + 40 + widest value` fits
   700 px at scale 3; every category heading fits 700 px at scale 4; every help
   title wraps into at most 2 lines at 440 px; every help body wraps into at most
   11 lines at 440 px and no single word exceeds 440 px.
6. The constants of §4.4 and §4.5 do not overlap, checked with the ink box of
   case 5's metrics: header readout, header rule, rail row 0, the first rail rule,
   the six category rows, the second rail rule and `FINISH`, in that order down
   the column, and the whole column inside its own column rule and clear of the
   footer; pane heading, hints, rows and footer; help title, help rule and body.
7. No identifier from the spec §4.8 inventory appears anywhere under `app/src`.
8. A synthetic `defaults.cfg` carrying both removed keys alongside every live key,
   parsed by a mirror of the store's key chain, ignores both and round-trips
   everything else.

These mirrors pin the rules — the category mapping, the text budgets, the
geometry — and **not** the C++ implementation of them. Only hardware proves the
screen draws what the arithmetic says.

### Hardware

The implementer cannot run any of this.

**After milestone 1.** An existing `defaults.cfg` from the shipped build still
loads and every other setting reads back correctly; both settings screens show 19
rows with every label against the right value; a commit runs and the progress log
contains none of the five save-data lines and is otherwise as before.

**After milestone 2.** Open `ENEMIES INCLUDED` and press `SQUARE`: the confirm
prompt must appear over a visibly dimmed but still readable list. **If the list
behind it is not dimmed, or is invisible, or the screen corrupts, alpha blending
does not work on this hardware — stop and report.** Also confirm all three pickers
still draw over a clean background and the two moved footer lines are not clipped.

**After milestone 3.** On `SETUP DEFAULTS`: every category reachable from the
rail; the `BLOODBORNE TITLE ID` rail row opens the existing editor on `X`; every
setting reachable, readable and changeable with Left/Right; `X` on a drill-in
opens its picker over the dimmed screen and `O` returns to the same row; `X` on a
toggle does nothing; the pane keeps showing the last category while the rail
cursor sits on the title-ID row; help text legible at TV distance and not clipped;
`OPTIONS` still saves and `O` still discards. Watch frame pacing — three panes are
considerably more drawing than one list on a CPU rasteriser.

**After milestone 4.** All of the above on the Enable wizard, plus: the `SEED`
rail row rolls a new seed on Left/Right — with the header readout changing to
match — and opens the digit editor on `X`; `FINISH` shows the readiness summary
and `X` opens Confirm; `O` from Confirm lands on the category the player left; and
**output parity** — one run with a fixed seed and a fixed set of toggles whose
`dvdroot_ps4` tree is byte-identical to the same run configured on the shipped
build. That diff is the only thing that can prove B17.

---

## 7. Milestones and stop conditions

### Milestone 1 — remove the save-data handling

**Goal.** Both settings screens go from 21 rows to 19, the simulated save-data
steps and their persisted keys are gone, and nothing else changes.

**Changes**, in order:

1. Remove the two fields from `RandomizerDefaults` — done when nothing in
   `app/src` still names them.
2. Remove both keys from the store's load chain and from the save format string
   *and* its argument list, and extend the tolerance comment to name them — done
   when conversions and arguments still correspond one for one.
3. Rewrite `SetupDefaultsScreen.h`'s row constants as a sequential block in the
   same order as `DrawList`'s `items`, drop the two rows from both, `kItemCount`
   19 — done when constant *n* names `items[n]` for all 19.
4. The same for `EnableWizardScreen.cpp`'s constants and **both** `DrawSaveData`
   and `DrawConfirm` vectors, `kSaveDataRowCount` 19 — done when the two vectors
   are identical in shape and each entry's index matches its constant.
5. Delete `Step::SelectReplace`, `Update/DrawSelectReplace`, `ReplaceChoice`,
   `ReplaceDisplayText`, `kReplaceOptions` and friends, and the three members —
   done when the `Step` switch has no missing case.
6. Delete `StartCommit`'s backup line and three-way replace block — done when a
   run's first progress line is `USING SEED …`.
7. Replace the `Application.cpp` startup log line — done when it names the title
   ID.
8. Update `ui_scroll_verify.py` and `pool_verify.py` (§5) — done when both pass
   with no other case changing state.
9. Update `docs/user-guide.md` — done when it no longer documents either setting
   and says plainly that the app does not touch save data.

**Invariants:** `StartCommit`'s run decision, the `EnemyRandomizerOptions`
assignments, `defaults.cfg` tolerance, the four progress-log constants (§3.1).

**Verification:** clean build; `ui_scroll_verify.py`; `pool_verify.py selftest`;
the four engine selftests unchanged.

**Completion gate.** All pass, the `.pkg` builds, and the milestone is handed to
the developer for the milestone-1 hardware test. Do not begin milestone 2.

### Milestone 2 — renderer primitives and an honest geometry model

**Goal.** `Renderer` can fill a rectangle and a blended rectangle, one existing
prompt uses both, and `ui_scroll_verify.py` models the font the app draws with.

**Changes**, in order:

1. Add `FillRect`, `FillRectBlend`, `LineHeight` to `Renderer` — done when
   `FillRectBlend` restores `SDL_BLENDMODE_NONE` before returning.
2. Add `hintGap` to `ListLayout`, delete `kHintGap`, set the six §4.5 values at
   their declaration sites — done when every `ListLayout` initialiser has four
   fields.
3. Move the wizard's two footer lines to `kScreenHeight - 100 / - 50` — done when
   both draw below the `MORE BELOW` band.
4. Remove both `renderer.Clear` calls from `ModelPicker.cpp` and add
   `renderer.Clear(20, 24, 28)` at all six host draw sites — done when every
   picker path still starts from a known background.
5. Make the picker's select-all/none prompt an overlay: host list, then
   `FillRectBlend(0, 0, 1920, 1080, 0, 0, 0, kOverlayAlpha)`, then the prompt —
   done when the list is still drawn underneath.
6. Correct the stale character-budget comments, leaving the three `PickerStrings`
   blocks untouched — done when `pool_verify.py selftest` still passes every
   picker-string case.
7. Rewrite `ui_scroll_verify.py`'s height model: parse `ascent`, `descent` and
   per-glyph `bearingY`/`height` from `FontAtlasData.h`, derive each scale's ink
   box, and assert rows, both hints, the heading above and the footer below all
   clear one another — done when every screen passes and **no constant was changed
   to make it pass except those in §4.5**.

**Invariants:** layering, the three `PickerStrings` blocks, the four progress-log
constants.

**Verification:** clean build; `ui_scroll_verify.py`; `pool_verify.py selftest`;
`font_atlas_verify.py`.

**Completion gate.** All pass, the `.pkg` builds, and the milestone is handed to
the developer for the milestone-2 hardware test — which is the alpha-blending
decision. Do not begin milestone 3.

### Milestone 3 — the settings model and the categorised Setup Defaults screen

**Goal.** One descriptor table replaces positional row indices, and
`SetupDefaultsScreen` is the categorised screen end to end.

**Changes**, in order:

1. Write `SettingsModel.h` — the three enums, `SettingDef`, the §4.1 accessor
   declarations — done when it compiles with no UI file including it.
2. Write `SettingsModel.cpp` — `kSettings` with all 18 entries in spec §7.1 order,
   today's labels, spec Appendix A help, plus the help for `SEED`,
   `BLOODBORNE TITLE ID` and `FINISH` (§4.1) — done when `settings_ui_verify.py`
   cases 1–5 pass.
3. Add `DrawLabelLeft`, `DrawLabelRight`, `DrawPaneScrollHints` and `kPaneLayout`
   to `Controls` — done when each takes an explicit x and width and none centres
   on the screen.
4. Rewrite `SetupDefaultsScreen` with the §4.2 focus model — rail row 0 is
   `BLOODBORNE TITLE ID`, opening the existing editor on `X` — and a `Draw` that
   paints the decorative header, the rules, the rail, the pane and the help per
   §4.4 — done when `ToggleRow` and every `k*Row` constant are gone, the pane keeps
   drawing `lastCategory_` while the rail cursor is on row 0, and `AdjustSetting`
   is the only writer.
5. Route its three pickers through `SelectionFlags`/`SelectionTable`/
   `SelectionCount`/`SelectionStrings`, drawn over the dimmed screen — done when
   no `SettingId` is named at the call site.
6. Write `settings_ui_verify.py` with the eight §6 cases — done when all pass.
7. Point `ui_scroll_verify.py`'s `Setup Defaults` entry at `kPaneLayout` — done
   when it passes.

**Invariants:** the one-`Screen` pattern, layering, `defaults.cfg` tolerance, and
the rule that `X` never toggles.

**Verification:** clean build; `settings_ui_verify.py`; `ui_scroll_verify.py`;
`pool_verify.py selftest`.

**Completion gate.** All pass, the `.pkg` builds, and the milestone is handed to
the developer for the milestone-3 hardware test. Do not begin milestone 4.

### Milestone 4 — the categorised Enable wizard

**Goal.** The wizard's settings step becomes the same categorised screen, with
`SEED` and `FINISH` as rail rows either side of the categories, and Confirm is
generated from the model.

**Changes**, in order:

1. Replace the wizard's 15 loose bools and 3 selections with one
   `RandomizerDefaults run_`, copy-constructed from `defaults_` — done when
   `StartCommit` reads every option from `run_` and assigns
   `EnemyRandomizerOptions` field for field exactly as before.
2. Rename `Step::SaveData` to `Step::Settings`; delete `GoToStep`'s cursor and
   scroll reset and delete `ReturnFromPicker` — done when each step owns its own
   cursor state.
3. Add `Focus`, `railCursor_` (0 `SEED`, 1–6 categories, 7 `FINISH`),
   `lastCategory_`, `listCursor_[6]`, `listScroll_[6]` and the §4.2 input handling
   — done when all of `UpdateSaveData`'s left/right chain and `X` chain are gone.
4. Draw the settings step per §4.4 — the decorative header readout, `SEED` above
   the first rail rule, `FINISH` below the second — done when Left/Right on the
   `SEED` row rolls a seed that both the row and the header readout show, and the
   readiness summary shows seed, target title ID and `N OF M SETTINGS ENABLED`.
5. Route the three pickers through the model, over the dimmed settings screen —
   done when `O` from a picker returns to the same category and the same row.
6. Generate `DrawConfirm`'s list from the model — the seed row, then the 18
   settings in category order — done when no second hardcoded list of settings
   exists in the file.
7. Update `ui_scroll_verify.py` and extend `settings_ui_verify.py` to the wizard's
   eight-row rail and its Confirm list — done when both pass.

**Invariants:** `StartCommit`'s run decision and option assignments, "only
`lastSeed` is written back", the one-`Screen` pattern, the four progress-log
constants (§3.1).

**Verification:** clean build; `settings_ui_verify.py`; `ui_scroll_verify.py`;
`pool_verify.py selftest`; every engine selftest unchanged.

**Completion gate.** All pass, the `.pkg` builds, and the milestone is handed to
the developer for the milestone-4 hardware test, including the byte-identical
output-parity run.

### Stop conditions

Halt and report rather than deciding, if any of these occur:

* the build fails in a way the plan did not anticipate, or fails only after a
  clean rebuild;
* **the milestone-2 hardware test shows no dimming, an invisible parent, or a
  corrupted frame** — alpha blending is unavailable and §4.3's fallback is a
  design change the developer must approve;
* a hardware handoff comes back with `FontAtlasInit: texture build failed` in
  `live.log` — every measurement in §4.4 assumes the atlas is the live path;
* `settings_ui_verify.py` reports a setting in no category, in two categories, or
  with empty help — the model and the struct have diverged;
* a label, value or help string does not fit its pane and the only way to make it
  fit is to shrink a pane or the type;
* `ui_scroll_verify.py` fails and the only way to pass is to weaken the ink-box
  model;
* any `pool_verify.py` case other than the config-size one and the new
  save-data-removal one changes state;
* the output-parity run is not byte-identical;
* an implementation decision would contradict the spec or §2 — in particular any
  temptation to dim a dependent setting, add a Result screen, make `X` toggle, or
  turn a wizard step into a `ScreenId`;
* the change needs a file not listed in §5;
* a §3.1 invariant cannot be preserved.

---

## 8. Open questions

None. The four questions this section held were answered on 2026-09-20 and are
recorded in §9 as D1–D4.

---

## 9. Decisions

| #   | Date | Decision | Taken by |
| --- | ---- | -------- | -------- |
| P1  | 2026-09-20 | One static `SettingDef` table over `RandomizerDefaults`, reached by pointer-to-member for toggles and a `kind` switch for the three pool types; declaration order is display order | planner |
| P2  | 2026-09-20 | The wizard holds one `RandomizerDefaults run_` instead of 15 loose bools and 3 selections, so one table serves both screens | planner |
| P3  | 2026-09-20 | One cursor spans the whole rail column — row 0, six categories, `FINISH` — with `Focus` distinguishing only rail from pane | planner |
| P4  | 2026-09-20 | `Renderer` gains `FillRect`, `FillRectBlend` and `LineHeight` only; separators are 2 px `FillRect`s, so exactly one unproven SDL entry point is added | planner |
| P5  | 2026-09-20 | The dimmed overlay is proven on the picker's existing select-all prompt in milestone 2, before any of the new screen is built | planner |
| P6  | 2026-09-20 | `ui_scroll_verify.py` models the atlas ink box parsed from `FontAtlasData.h`, not `8 * scale`, and the model is not weakened to fit existing constants | planner |
| P7  | 2026-09-20 | `kHintGap` becomes a per-`ListLayout` field; the wizard's two footer lines move 30 px down so the 6-row Confirm window survives the corrected model | planner |
| P8  | 2026-09-20 | Category labels use `&` | planner |
| P9  | 2026-09-20 | Four milestones: save-data removal, renderer primitives, model + Setup Defaults, wizard. Setup Defaults goes first of the two screens because it has no commit path | planner |
| P10 | 2026-09-20 | `docs/user-guide.md` is corrected in milestone 1 rather than deferred, because it otherwise documents two settings that no longer exist | planner |
| P11 | 2026-09-20 | `app/UI_BLUEPRINT.md` is left alone — it describes the deferred save-data redesign, not shipped behaviour | planner |
| D1  | 2026-09-20 | The seed is its own rail row above the categories, separated by a rule as `FINISH` is below them; the header band displays the seed and the target title ID and is never a focus target. `SetupDefaultsScreen` takes `BLOODBORNE TITLE ID` as the same row | developer |
| D2  | 2026-09-20 | Setting labels stay as shipped — `DO NOT RANDOMIZE CAGED DOGS` and the rest. Spec §7.1 and Appendix A are prose, not label specifications | developer |
| D3  | 2026-09-20 | Confirm's list is generated from the model in category order, seed first | developer |
| D4  | 2026-09-20 | The font atlas is hardware-verified and good. No milestone is gated on it; the §4.4 measurements simply assume it is the live text path | developer |
| P12 | 2026-09-20 | D1's rail geometry, re-derived: the rail column widens to 580 px so it holds `BLOODBORNE TITLE ID   CUSA03173`; the pane moves to x 680 and the help pane to x 1420 at 440 px wide. Row 0 sits at y 230, the categories at y 330 pitch 76 — the same band as the pane rows — and `FINISH` at y 806. `kPaneLayout` and every `hintGap` are unaffected | planner |
| P13 | 2026-09-20 | The pane always draws `lastCategory_`, highlighted only while focus is in the pane, so selecting the seed row or `FINISH` never blanks it | planner |
| P14 | 2026-09-20 | `SettingsModel` carries help for the three non-setting rail rows. `SEED` and `FINISH` take their Appendix A entries; `BLOODBORNE TITLE ID` has none, so §4.1 supplies one drawn from the field comment in `RandomizerDefaults.h` | planner |

---

## 10. Changes during implementation

| Date | Change | Reason |
| ---- | ------ | ------ |
| 2026-09-20 | **Milestone 1: no deviations.** Every §7 step was implemented as written, in order. Three decisions the plan left open were taken and are recorded in `implementation-report.md` §3: the wizard's drawn sub-heading `SAVE DATA` → `SETTINGS` (the `Step::SaveData` identifier is untouched and its rename stays in milestone 4); stale header and tombstone comments corrected in the three files §5 already assigns to milestone 1; and the new `pool_verify.py` case written against the quoted key literal, the `key=%d` format fragment and the field name rather than a whole-file string search, so the tolerance comment §7 step 2 requires cannot make it pass or fail | Recorded per §10's purpose; none of the three changes a §1–§7 instruction, a §3.1 invariant or a spec §10 decision |
| 2026-09-20 | **Milestone 2: one wording discrepancy, no change of substance.** §5 and §7 step 4 say "remove **both** `renderer.Clear` calls from `ModelPicker.cpp`"; the file holds **one**, at the top of `Draw`, which served the prompt path too because `Draw` cleared and then early-returned into `DrawConfirm`. Removing it removes the clear from both paths. Every other §7 step was implemented as written, in order, and no constant was changed to make `ui_scroll_verify.py` pass except the six §4.5 `hintGap` values and the two footer positions of step 3 | Recorded per §10's purpose; the contract's call-site count differs from the source, so a reviewer counting them would otherwise find one missing |
| 2026-09-20 | **Milestone 2: seven decisions the plan left open**, all recorded in `implementation-report-milestone-2.md` §3 — `kOverlayAlpha` placed in `Controls.h` rather than `ModelPicker.cpp`; the `FillRectBlend` issued as `DrawConfirm`'s first line rather than beside the call; no `kPaneLayout` entry added to `ui_scroll_verify.py` (milestone 3 step 7 owns that, though the layout's numbers were checked offline and hold); the verifier entry kept as `Wizard SaveData` to match §5's milestone-4 wording; a fifth assertion G5 added, that the second footer line is on screen, since the step-3 move is what spends that margin; two comments falsified by the `hintGap` change itself corrected (`ModelPicker.cpp:27`, `ModelPicker.h:27`); and this milestone's report written as a separate file so milestone 1's is not overwritten | None affects a §1–§7 instruction, a §3.1 invariant or a spec §10 decision; recorded here because §10 is where stage F looks for them |
| 2026-09-20 | **Milestone 3: one structural deviation.** §7 step 5 says "route **its three pickers**"; there is now one `Mode::Picker` plus a `const SettingDef* pickerSetting_`, because the step's own done-condition — "no `SettingId` is named at the call site" — cannot be met while three modes have to be mapped back to three settings. The four `Selection*` functions turn that one pointer into the table, count, flags and vocabulary. Knock-on for a reviewer counting them: milestone 2's **six** host draw sites each doing their own `renderer.Clear` are now **four**, this screen's three having collapsed into one that draws the whole settings screen and then dims it. The wizard's three are untouched | Recorded per §10's purpose; the contract's call-site count differs from what the model makes possible, and milestone 4 will collapse the wizard's three the same way |
| 2026-09-20 | **Milestone 3: nine decisions the plan left open**, all recorded in `implementation-report-milestone-3.md` §3 — `WrapText` placed in `Controls` (returning lines, not drawing) rather than in the screen, since milestone 4 needs it too; `Palette::SelectedBar = {56, 44, 18}` as §4.4's "dark tint of `Palette::Selected`", in `Palette` for the same reason `kOverlayAlpha` is; a file-local `kRuleColor = {64, 72, 82}` for the 2 px rules; the §4.4 geometry constants kept file-local, as §5 gives milestone 3 no shared layout file; the rail marking `lastCategory_` in `Palette::Selected` without the bar whenever the cursor is elsewhere, so the pane's category is never unlabelled; the single footer line's wording, `UP DOWN MOVE   LEFT RIGHT CHANGE   X SELECT   O BACK   OPTIONS SAVE` (1174 px), with `X TOGGLE` dropped because `X` no longer toggles; a `foot_lines` field added to `ui_scroll_verify.py` so its G5 checks the last footer line each screen actually draws rather than a second line the categorised screens do not have; `settings_ui_verify.py` asserting the 580 px rail budget against the widest REAL title ID (552 px, as §4.4 sized it) and separately that the widest TYPEABLE one (586 px) still clears the column rule at x 660; and per-setting log lines now built from the label and `SettingValueText` instead of one hand-written line each | None affects a §1–§7 instruction, a §3.1 invariant or a spec §10 decision; recorded here because §10 is where stage F looks for them |
| 2026-09-20 | **Milestone 4: the §4.4 geometry constants are DUPLICATED into `EnableWizardScreen.cpp`, not promoted.** Milestone 3 left them file-local in `SetupDefaultsScreen.cpp` (its report §3.3) and said milestone 4 must "either duplicate them or promote them". Promoting needs a shared layout file, and §5 assigns milestone 4 only `EnableWizardScreen.{h,cpp}` plus the two tools — creating one would fire the "the change needs a file not listed in §5" stop condition for a change that alters no behaviour. So the 32 shared constants and `kRuleColor` are a second copy, and `settings_ui_verify.py` now parses **both** screens and fails if any of them disagrees with its twin, and fails again if the verifier's own third copy has gone stale against them. Three copies, one assertion each way | Recorded per §10's purpose; the contract says "one geometry, shared by" both screens and the source now holds it twice, which a reviewer must be able to see is deliberate and guarded |
| 2026-09-20 | **Milestone 4: one structural deviation, the same one milestone 3 made.** §7 step 5 says "route **the three pickers** through the model"; there is now one `Step::Picker` plus a `const SettingDef* pickerSetting_`, exactly as milestone 3's §10 row predicted ("milestone 4 will collapse the wizard's three the same way"). `Step::EnemyPicker / SkipPicker / BossPicker` and `Update/Draw{Enemy,Skip,Boss}Picker` are gone. Knock-on for a reviewer counting draw sites: milestone 2's six hosts doing their own `renderer.Clear` became four in milestone 3 and are **two** now — one per categorised screen, each drawing the whole settings screen and then dimming it | Recorded per §10's purpose; the contract's call-site count differs from what the model makes possible |
| 2026-09-20 | **Milestone 4: `OPTIONS` no longer opens Confirm from the settings step, and `titleId_` is gone.** The flat list treated `OPTIONS` as NEXT; B6 and spec §10 (9.2) say `OPTIONS` commits **on Confirm only**, and §4.2's rail handling gives it no meaning, so the only route to Confirm is the `FINISH` rail row. §7's steps do not mention the `OPTIONS` chain — only the left/right and `X` chains — so this is called out rather than buried. Separately, the `titleId_` member was dropped: `run_.bloodborneTitleId` is the same value from the same source (both were copies of `defaults.bloodborneTitleId` taken at construction), and keeping both would re-create the parallel-copy problem this feature removes | Recorded per §10's purpose; the first is a control change a player will feel and §7 does not name it, the second removes a member §5 does not list |
| 2026-09-20 | **Milestone 4: seven decisions the plan left open**, all recorded in `implementation-report-milestone-4.md` §3 — the footer wording `UP DOWN MOVE   LEFT RIGHT CHANGE   X SELECT   O BACK` (926 px, `OPTIONS SAVE` dropped because this screen does not save); the readiness summary drawn as three wrapped lines, a blank, then `FinishHelp()`, 8 of the 11 body lines the pane draws; the header's `TARGET` readout in `Palette::Dim` so the decorative half does not compete with the seed; `confirmScroll_` as Confirm's own offset, zeroed only on entry from `FINISH`; `ConfirmItems()` as a named const method rather than an inline vector, so "there is one list" is visible in the header; `ui_scroll_verify.py`'s entry renamed `Wizard SaveData` → `Wizard Settings` now that the step is renamed; and the verifier's new cross-screen and mirror-staleness cases described in the row above | None affects a §1–§7 instruction, a §3.1 invariant or a spec §10 decision; recorded here because §10 is where stage F looks for them |
