# Implementation Report — Randomizer Settings UI — milestone 3

**Status: AWAITING HARDWARE TEST**

**Plan:** `docs/features/randomizer-settings-ui/plan.md` — milestone 3, "the
settings model and the categorised Setup Defaults screen"

**Spec:** `docs/features/randomizer-settings-ui/spec.md`

**Implemented:** 2026-09-20

---

## 1. What was built

`app/src/UI/SettingsModel.{h,cpp}` is new: one `const SettingDef kSettings[]`
holding all 18 settings, ordered by category and then by the within-category
order of spec §7.1, plus the §4.1 accessors. `SetupDefaultsScreen` is rewritten
on top of it as the categorised screen — decorative header, a rail of
`BLOODBORNE TITLE ID` and the six categories, the selected category's settings
in the middle, contextual help on the right, and the three pickers drawn over a
dimmed copy of that screen. **`ToggleRow` and every `k*Row` constant are gone**,
and `AdjustSetting` is the only thing that writes a setting. `Controls` gained
the three pane-aware draw helpers and `kPaneLayout`; `settings_ui_verify.py` is
new and `ui_scroll_verify.py`'s `Setup Defaults` entry now models the pane.

| # | Planned change | Status | Where |
| - | -------------- | ------ | ----- |
| 1 | Write `SettingsModel.h` — three enums, `SettingDef`, the §4.1 accessors | done | `app/src/UI/SettingsModel.h` (129 lines) |
| 2 | Write `SettingsModel.cpp` — `kSettings`, today's labels, Appendix A help, plus `SEED` / `BLOODBORNE TITLE ID` / `FINISH` help | done | `app/src/UI/SettingsModel.cpp` (276 lines) |
| 3 | `DrawLabelLeft`, `DrawLabelRight`, `DrawPaneScrollHints`, `kPaneLayout` in `Controls` | done, plus `WrapText` and `Palette::SelectedBar` — see §3 | `Controls.h:35,122-145`, `Controls.cpp:82-140` |
| 4 | Rewrite `SetupDefaultsScreen` on the §4.2 focus model and the §4.4 layout | done | `app/src/UI/SetupDefaultsScreen.{h,cpp}` (113 / 401 lines) |
| 5 | Route the three pickers through `Selection*`, over the dimmed screen | done — one `Mode::Picker` rather than three modes, see §3 | `SetupDefaultsScreen.cpp:88-104, 236-250` |
| 6 | Write `settings_ui_verify.py` with the eight §6 cases | done — 28 assertions covering all eight | `app/tools/settings_ui_verify.py` |
| 7 | Point `ui_scroll_verify.py`'s `Setup Defaults` entry at `kPaneLayout` | done, plus a `foot_lines` field — see §3 | `app/tools/ui_scroll_verify.py:129-171, 199-240` |

### What the model looks like

```
Enemies        4  RANDOMIZE ENEMIES, ENEMIES INCLUDED, ENEMIES SKIPPED,
                  DO NOT RANDOMIZE CAGED DOGS
Bosses         2  RANDOMIZE BOSSES, BOSSES INCLUDED
ItemsTreasure  3  RANDOMIZE TREASURE, RANDOMIZE WORKSHOP TOOLS,
                  RANDOMIZE ENEMY DROPS
WeaponsGear    4  RANDOMIZE STARTING WEAPONS, RANDOMIZE STARTING GUNS,
                  RANDOMIZE SHOP WEAPONS, START WITH HUNTER TOOLS
Difficulty     4  EASY SHADOWS, EASY ROM, EASY FAILURES, EASY EMISSARY
WorldFlavour   1  ENABLE MERGO DARKNESS
```

18 settings, 15 toggles, 3 drill-ins, checked against spec §7.1 as parsed out of
the spec itself (case 3), not against a copy of it typed into the verifier.

### Invariants held

* **The one-`Screen` pattern.** The title-ID editor and the pickers are still
  internal modes of `SetupDefaultsScreen`; no wizard step or drill-in became a
  `ScreenId`.
* **Layering.** No UI file calls SDL2 — the only new drawing is through
  `Renderer::FillRect` / `FillRectBlend` / `DrawText`. Nothing under
  `app/src/Randomizer/` mentions categories, help or panes (grepped).
* **`defaults.cfg` tolerance.** The store is untouched by this milestone;
  verifier case 8 re-establishes that a config carrying both retired keys and an
  unknown future key loads with them ignored and all 20 live keys round-tripped.
* **`X` never toggles.** `X` in the pane opens a picker when `IsDrillIn` and
  does nothing otherwise; `AdjustSetting` is reached only from Left/Right.
* **Engine untouched.** No file under `app/src/Randomizer/` or `app/src/Msb`,
  `Param`, `Game` was modified.

---

## 2. Deviations from the plan

**One, and it is a structural one in §7 step 5.**

* **The plan says "its three pickers"; there is now one picker mode.** §7 step 5
  is satisfied by its own done-condition ("no `SettingId` is named at the call
  site"), and the way to satisfy it was to stop distinguishing the three lists
  at all: `Mode::EnemyPicker / SkipPicker / BossPicker` became one
  `Mode::Picker` plus a `const SettingDef* pickerSetting_`, and the four
  `Selection*` functions turn that pointer into the table, the count, the flags
  and the vocabulary. Keeping three modes would have required a mode→setting
  mapping, which is a settings identity in the screen — the thing this milestone
  removes. **Consequence for a reviewer counting call sites:** milestone 2's
  "six host draw sites" that each do their own `renderer.Clear` are now four —
  three of Setup Defaults' collapsed into one, which draws the whole settings
  screen (including its `Clear`) and then dims it. The wizard's three are
  untouched and milestone 4 will collapse them the same way.

Everything else in §7 steps 1–7 was implemented as written, in order, with the
values, names and files the plan gave.

---

## 3. Decisions the plan left open

### 3.1 `WrapText` lives in `Controls`, not in the screen

§4.4 requires greedy wrapping measured with `Renderer::TextWidth`, but no
section says where it lives; §7 step 3 enumerates three draw helpers and
`kPaneLayout` for `Controls` and does not mention wrapping. The options were a
file-local helper in `SetupDefaultsScreen.cpp` (which milestone 4 would then
have to duplicate or move) or a fourth `Controls` entry. It went in `Controls`,
returning `std::vector<std::string>` rather than drawing, so the pitch and
line-limit decisions stay in the screen where the geometry constants are.

### 3.2 `Palette::SelectedBar` and a file-local rule colour

§4.4 asks for "a dark tint of `Palette::Selected`" and leaves the value to the
implementer (evidence §E7 says so explicitly). `Palette::SelectedBar =
{56, 44, 18}` is that tint, and it is in `Palette` rather than in the screen
because milestone 4 draws the same bar — the same reasoning milestone 2 used for
`kOverlayAlpha`. **It has never been seen on a TV and may want adjusting.**

The 2 px rules are drawn in a file-local `kRuleColor = {64, 72, 82}`, dimmer
than `Palette::Dim` (which is text). That one stayed local because the plan
gives the wizard its own copy of the layout constants anyway (§5 assigns
milestone 4 only `EnableWizardScreen.{h,cpp}`).

### 3.3 The screen's own geometry constants are file-local

§4.4 describes "one geometry, shared by the wizard's Settings step and
`SetupDefaultsScreen`", but §5 gives milestone 3 no shared layout file and gives
milestone 4 only the wizard's two files. The constants therefore sit in
`SetupDefaultsScreen.cpp`'s anonymous namespace, matching how every other screen
in this app carries its own. **Milestone 4 will have to either duplicate them or
promote them**; `settings_ui_verify.py` pins the numbers either way, so a
divergence between the two screens would fail the verifier rather than ship.

### 3.4 Which rail row reads as "current"

§4.4 says the pane highlights its selected row only while focus is in the pane,
and P13 says the pane always draws `lastCategory_`. It does not say what the
rail shows meanwhile. Implemented as: the focused row gets the bar plus
`Palette::Selected`; the category the pane is showing gets `Palette::Selected`
with no bar whenever the cursor is elsewhere. Without this, moving into the pane
or onto row 0 leaves nothing on screen saying which category those settings
belong to.

### 3.5 One footer line, and its wording

§4.4 specifies "Footer y 1000, scale 3, one centred line" where the old screen
drew two at 950 / 1000. The line is
`UP DOWN MOVE   LEFT RIGHT CHANGE   X SELECT   O BACK   OPTIONS SAVE`
(1174 px of 1920, asserted). It keeps `OPTIONS SAVE`, which this screen's old
footer advertised and which is the only way to save; `X TOGGLE` is gone from it
because `X` no longer toggles. The title-ID editor's own two footer lines are
unchanged.

`ui_scroll_verify.py` needed a `foot_lines` field for this: its G5 checks the
*second* footer line is on screen, and checking a line that is not drawn would
have failed a screen whose footer is fine. Every existing entry passes `2`; the
new one passes `1`. This is not a weakening — G5 now checks the last line each
screen actually draws.

### 3.6 The verifier's title-ID width case

Plan §4.4 sizes the 580 px rail on `BLOODBORNE TITLE ID   CUSA03173` (552 px,
evidence M3). The editor can, however, produce any 4 letters and 5 digits, and
the widest typeable ID makes that row 586 px. Rather than weaken or widen
anything, case 5 asserts both facts separately: every rail row fits the 580 px
budget at its widest *real* value, and the widest *typeable* row still clears
the column rule at x 660 (646 < 660 — the 20 px gutter absorbs it). No constant
was changed to make this pass.

### 3.7 Per-setting log lines are now generic

`ToggleRow` logged `defaults: randomize enemies = YES`, one hand-written line
per setting. The pane logs `defaults: RANDOMIZE ENEMIES = YES`, built from the
label and `SettingValueText`. Nothing parses these lines (`pool_verify.py`
parses only `EnableWizardScreen.cpp`'s four progress constants and
`ModelPicker.h`'s three `PickerStrings` blocks, all untouched).

### 3.8 Two comments still say `ToggleRow` and `kEnemiesIncludedRow`

Both are tombstones explaining why the model exists — `SetupDefaultsScreen.h:12`
and `SettingsModel.h:4` — matching this repo's habit of recording what was
removed and why. No code, constant or identifier of that kind survives; a grep
for them in compiled code finds nothing.

### 3.9 Report file name

Written as `implementation-report-milestone-3.md` so milestones 1 and 2's
reports are not overwritten, following milestone 2's precedent.

---

## 4. Verification run

| Check | Command | Result |
| ----- | ------- | ------ |
| Clean build | `cd app && make clean && make` | `.pkg` built (7,143,424 bytes), **no warnings, no errors**. `SettingsModel.cpp` was picked up by the `find`-based `SRC_CPP` with no `Makefile` change, as §5 predicted |
| Settings model | `python app/tools/settings_ui_verify.py` | **28/28 passing** — all eight §6 cases |
| Scroll and geometry | `python app/tools/ui_scroll_verify.py` | **all geometry and scroll properties PASSED** (7 screens; Setup Defaults now 8 rows visible of 4, rows y=330..862, clearances 17 px above and below) |
| Pool, strings, config | `python app/tools/pool_verify.py selftest data/vanilla/dvdroot_ps4` | **88/88 passing** — no case changed state |
| Font atlas | `python app/tools/font_atlas_verify.py` | **PASS** — the metrics every width in §4.4 is measured from are still the baked ones |

`settings_ui_verify.py`, case by case:

```
1: all 15 bool fields appear in exactly one entry                     ok
1: the three selection fields have exactly one entry each             ok
1: every entry is a toggle with a flag or a pool without one          ok
2: every SettingId appears exactly once, none is unused               ok
3: six categories, their order and their membership match spec 7.1    ok
3: the four easy modes are in Difficulty, not Bosses (spec 7.1)       ok
3: START WITH HUNTER TOOLS is not beside RANDOMIZE WORKSHOP TOOLS     ok
3: the six rail labels are declared, one per category                 ok
4: all 18 settings have non-empty help                                ok
4: SEED, BLOODBORNE TITLE ID and FINISH have non-empty help           ok
5: every rail row fits 580 px at scale 3 (widest 552 px)              ok
5: the widest TYPEABLE title-ID row (586 px) still clears the rule    ok
5: every label + 40 + widest value fits 700 px (widest 643 px)        ok
5: every category heading fits 700 px at scale 4 (widest 614 px)      ok
5: every help title wraps into at most 2 lines at 440 px (worst 2)    ok
5: every help body wraps into at most 11 lines at 440 px (worst 10)   ok
5: no single help word exceeds 440 px (widest 196 px)                 ok
5: the footer line fits the screen (1174 px of 1920)                  ok
6: the rail column's 13 elements are in order and clear of each other ok
6: the rail's ink ends at 850, inside its column rule (210..960)      ok
6: pane heading, hints, 8 rows and footer clear each other            ok
6: the focus bar contains its row's ink and misses the next bar       ok
6: help title, rule and 11 body lines clear each other and the footer ok
7: no save-data identifier from spec 4.8 survives under app/src       ok
8: the store neither reads nor writes either retired key              ok
8: a defaults.cfg carrying both retired keys ignores them             ok
8: every one of the 20 live keys round-trips unchanged                ok
8: every key the store reads it also writes, and vice versa           ok
```

**Not run:** the four engine selftests (`boss_verify.py`,
`caged_dogs_verify.py`, `easy_modes_verify.py`, `hunter_tools_verify.py`).
Milestone 3's §7 verification list does not call for them and this milestone
modified nothing under `app/src/Randomizer/`; they were last run in milestone 1.
`pool_verify.py table both` was likewise not re-run — no pool table was
regenerated.

---

## 5. What this does not prove

Everything above is a cross-compile and a set of Python mirrors. Per
`CLAUDE.md` §3 the PS4 is the only authority on runtime behaviour, and this is
**ready for hardware testing**, not working.

Specifically unproven:

* **Alpha blending, still.** `Renderer::FillRectBlend` has never run on this
  hardware; milestone 2's test that would settle it has not happened. The dimmed
  overlay behind every picker on this screen depends on it, and §4.3's no-blend
  fallback was deliberately not implemented.
* **That the screen draws what the arithmetic says.** The mirrors pin the rules
  — category mapping, text budgets, pixel geometry — not the C++ that applies
  them. A transposed x, a bar drawn after its text, or a pane drawn off-column
  would pass everything above.
* **Legibility.** The help pane is the first body copy this app has drawn. Scale
  3 is proven for list rows, not for ten lines of prose in a 440 px column
  (evidence §E7).
* **The highlight and rule colours.** `{56, 44, 18}` and `{64, 72, 82}` were
  chosen on a monitor.
* **Frame pacing.** Three panes plus a blended overlay is roughly 43 strings and
  5 filled rects a frame against today's ~8 strings, on a CPU rasteriser
  (evidence §E5.5).
* **That the font atlas is the live path on this build.** Every width in §4.4
  assumes it. `live.log` must say `FontAtlasInit ok`.

---

## 6. Hardware test handoff

Install the `.pkg` from `app/IV0000-BBRD00001_00-BBRANDOMIZERMAIN.pkg`.

**0. Before anything else**, check `live.log` says `FontAtlasInit ok`. If it
says `texture build failed`, stop — the 8×8 fallback has no lowercase, so the
help pane would be blank columns and the layout is not the defect (plan §3.3).

**1. The milestone-3 screen — `SETUP DEFAULTS` from the main menu.**

1. The screen should show: a centred `SETUP DEFAULTS` title; a
   `BLOODBORNE TITLE ID   CUSA03173` readout under it; a horizontal rule; a left
   rail with that same row, a rule, then `ENEMIES`, `BOSSES`,
   `ITEMS & TREASURE`, `WEAPONS & STARTING GEAR`, `DIFFICULTY`,
   `WORLD & FLAVOUR`; the `ENEMIES` settings in the middle with values on the
   right of that column; help text on the right; one footer line.
2. Up/Down on the rail: all seven rows reachable, wrapping at both ends. The
   middle pane should change with each category and **never go blank** — in
   particular, moving the cursor up onto the title-ID row must leave the
   previous category's settings showing.
3. `X` on the title-ID row opens the existing character-by-character editor;
   `X` saves it, `O` cancels; both return to the settings screen with the rail
   cursor still on row 0.
4. `X` on a category moves focus into the pane. Up/Down moves the highlight bar;
   Left/Right flips `YES`/`NO` and the value changes on screen; `O` returns to
   the rail with the same category still selected.
5. **`X` on a toggle must do nothing at all.** This is the deliberate control
   change and the one most likely to feel wrong in the hand.
6. `X` on `ENEMIES INCLUDED`, `ENEMIES SKIPPED` (both under `ENEMIES`) and
   `BOSSES INCLUDED` (under `BOSSES`) opens that picker **over a dimmed but
   still readable settings screen**. `O` returns to the same category and the
   same row, and the row's `N OF M` value reflects what was changed.
7. Read the help pane at normal TV distance for several settings, including
   `DO NOT RANDOMIZE CAGED DOGS` (the longest label, two title lines) and
   `RANDOMIZE WORKSHOP TOOLS` (a long body). Nothing should be clipped at the
   right edge or run under the footer.
8. `OPTIONS` saves and returns to the menu; re-enter and confirm the values
   stuck. `O` from the rail discards and returns to the menu; re-enter and
   confirm the change did **not** stick.
9. Watch frame pacing while holding Up/Down — three panes is considerably more
   drawing than one list.

**What a failure looks like.**

* Picker opens over black, or the screen behind it vanishes or corrupts → alpha
  blending is unavailable on this hardware. **Stop and report**; §4.3's
  scanline fallback is a design change only the developer can approve (plan §7
  stop conditions).
* A category's pane is empty, or a setting cannot be reached → the model and the
  struct have diverged; report which.
* Text clipped at a pane edge → report the exact string; the width budgets are
  asserted, so a clip means the draw origin is wrong, not the wording.
* Rows drawn on top of each other or over the footer → report the screen; the
  geometry is asserted under the atlas ink box.

**2. Accumulated and still untested across all three milestones.** Neither
milestone 1 nor milestone 2 has been on hardware, so this build carries three
milestones of untested change. Worth covering in the same session:

* *(milestone 1)* An existing `defaults.cfg` from the shipped build still loads
  with every other setting correct; a full commit from the Enable wizard runs
  and the progress log contains none of the five save-data lines.
* *(milestone 1)* The Enable wizard's flat list shows **19** rows, each label
  against the right value — it is still the old screen until milestone 4.
* *(milestone 2)* Open any picker and press `SQUARE`: the confirm prompt must
  appear over a **visibly dimmed but still readable** list. This is the
  alpha-blending decision, and this screen's overlays stand or fall with it.
* *(milestone 2)* All three pickers still draw over a clean background, and the
  wizard's two moved footer lines are not clipped.

---

## 7. Stop point

**Completion gate reached, no stop condition fired.** All four milestone-3
checks pass, the `.pkg` builds clean, and the milestone is handed to the
developer for the milestone-3 hardware test.

**Milestone 4 was not started** — the Enable wizard is untouched and still
carries its 19 row constants, `Step::SaveData` and its own `DrawConfirm` list.
