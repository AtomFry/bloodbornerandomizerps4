# Implementation Report — Randomizer Settings UI — milestone 2

**Status: AWAITING HARDWARE TEST**

**Plan:** `docs/features/randomizer-settings-ui/plan.md` — milestone 2,
"renderer primitives and an honest geometry model"

**Spec:** `docs/features/randomizer-settings-ui/spec.md`

**Implemented:** 2026-09-20

> Milestone 1's report is `implementation-report.md` in this folder and is not
> touched by this pass. See §3.7 for why this milestone's report is a separate
> file.

---

## 1. What was built

`Renderer` can now fill a rectangle, opaque or blended, and can report the
atlas's real line height. `ListLayout` carries its own `hintGap`, because the
corrected geometry model shows no single global value clears every screen.
`ModelPicker` no longer clears the screen — its six hosts do — and its
select-all/select-none prompt is now drawn *over* the still-visible list
through one `FillRectBlend`, which is the single unproven SDL entry point this
whole feature adds and the thing the milestone-2 hardware test exists to
settle. `ui_scroll_verify.py` stopped modelling a line of text as `8 * scale`
and now derives each scale's ink box from `FontAtlasData.h`, asserting rows,
both hints, the heading above and the footer below all clear one another.

No randomizer behaviour was touched. No file outside plan §5's milestone-2 list
was touched.

| # | Planned change | Status | Where |
| - | -------------- | ------ | ----- |
| 1 | `FillRect`, `FillRectBlend`, `LineHeight` on `Renderer`; `FillRectBlend` restores `SDL_BLENDMODE_NONE` | done | `app/src/Platform/Renderer.h:33,41,46`; `app/src/Platform/Renderer.cpp:48-68` |
| 2 | `ListLayout::hintGap`; delete `kHintGap`; the six §4.5 values at their declaration sites | done, five sites — `kPaneLayout` is milestone 3's | `app/src/UI/Controls.h:63-73`; `app/src/UI/Controls.cpp:25-28,54,59`; `EnableWizardScreen.cpp:110,115`; `SetupDefaultsScreen.cpp:24`; `ModelPicker.cpp:21,29` |
| 3 | Wizard's two footer lines to `kScreenHeight - 100 / - 50` | done, in `DrawSaveData` and `DrawConfirm` only | `app/src/UI/EnableWizardScreen.cpp:815-822,926-929` |
| 4 | Remove the `renderer.Clear` calls from `ModelPicker.cpp`; add `renderer.Clear(20, 24, 28)` at all six host draw sites | done — see §2 on "both" | `ModelPicker.cpp:127-133`; `EnableWizardScreen.cpp:859,865,871`; `SetupDefaultsScreen.cpp:222-236` |
| 5 | The select-all/none prompt becomes an overlay: host list, `FillRectBlend`, then the prompt | done | `ModelPicker.cpp:175-177` (prompt drawn last), `:180-191` (the blend) |
| 6 | Correct the stale character-budget comments; leave the three `PickerStrings` blocks untouched | done | `EnableWizardScreen.cpp:79-97`; `ModelPicker.h:60-68` |
| 7 | Rewrite `ui_scroll_verify.py`'s height model on the parsed ink box; assert rows as well as furniture | done | `app/tools/ui_scroll_verify.py`, rewritten |

### The geometry, as the corrected model now reports it

```
Setup Defaults    7 rows visible of 19   rows y=300..840   hints 248 / 892   gap 52   clearances above 22 / below 3
Wizard SaveData   6 rows visible of 19   rows y=420..870   hints 360 / 930   gap 60   clearances above 30 / below 11
Wizard Confirm    6 rows visible of 19   rows y=420..870   hints 360 / 930   gap 60   clearances above 30 / below 11
Progress log      9 rows visible of 18   rows y=300..860   hints 250 / 910   gap 50   clearances above 15 / below 15
Enemy picker     12 rows visible of 82   rows y=280..852   hints 228 / 904   gap 52   clearances above 17 / below 17
Boss picker      12 rows visible of 17   rows y=280..852   hints 228 / 904   gap 52   clearances above 17 / below 17
Skipped picker   11 rows visible of 85   rows y=332..852   hints 280 / 904   gap 52   clearances above 17 / below 17

ink box, parsed from FontAtlasData.h:
  scale 3: ink 9..44 below the draw y, line box 44
  scale 4: ink 14..58 below the draw y, line box 59
  scale 5: ink 16..73 below the draw y, line box 73
  scale 6: ink 20..86 below the draw y, line box 87
```

The ink box reproduces `plan-evidence.md` §E4 M1 exactly. **No constant was
changed to make the verifier pass except the six §4.5 `hintGap` values and the
two footer positions in step 3.** Every visible-row count is unchanged from the
shipped build.

### The model was mutation-tested, not just run

A passing verifier proves nothing if it cannot fail. Three mutations were run
against it in a scratch copy and each was caught:

| Mutation | Result |
| -------- | ------ |
| every `hintGap` back to the shipped global `46` | 3 failures — `Setup Defaults: MORE BELOW ink top 895 overlaps last row ending at 898`; `Wizard SaveData` and `Wizard Confirm`: `925` vs `928` |
| the single `kHintGap = 36` §E4 M2 records as the obvious wrong answer, footers unmoved | 3 failures — `MORE BELOW ink top 885 overlaps last row ending at 898`, and the same for both wizard lists |
| planned gaps, but the wizard's footers left at `- 130 / - 80` | 2 failures — `Wizard SaveData: MORE BELOW ink bottom 974 collides with footer ink at 959`, same for Confirm |

The first two are the verdicts §E4 M2 predicted from an independent
measurement, which is the cross-check that matters: the planner's arithmetic
and this implementation of it agree to the pixel.

---

## 2. Deviations from the plan

**One, and it is a discrepancy in the contract's wording rather than a change
of substance.**

Plan §5 and §7 step 4 both say "remove **both** `renderer.Clear` calls from
`ModelPicker.cpp`". The file contains **one** — at the top of
`ModelPicker::Draw`. It served both of the component's drawing paths, because
`Draw` cleared and then early-returned into `DrawConfirm`, so the prompt path
inherited the same clear. That single call was removed, which removes the clear
from both paths, and both now start from the host's background. Nothing was
left behind: `grep -n "Clear(" app/src/UI/ModelPicker.cpp` returns nothing.

This is recorded because a reviewer counting call sites against the contract
would otherwise find one missing.

Everything else in §7 steps 1–7 was implemented as written, in order.

---

## 3. Decisions the plan left open

### 3.1 `kOverlayAlpha` lives in `Controls.h`

§4.3 names the constant and its value (190) but not its home. The options were
`ModelPicker.cpp`'s anonymous namespace — where the only current use is — or
`Controls.h`, beside `Palette`. `Controls.h` was chosen: §4.6 has every host
dimming with the same constant in milestones 3 and 4, and a constant defined
inside the one component that uses it today would have to move then, or be
copied. It is UI-level styling, so it does not belong in `Platform/Renderer.h`
beside `kScreenWidth` — that would put an overlay policy in the layer §3.1 says
owns only SDL.

### 3.2 The blend is issued inside `DrawConfirm`, not beside it

Step 5 reads "host list, then `FillRectBlend`, then the prompt". That could be
three statements in `Draw`. It is instead the first line of `DrawConfirm`, with
`Draw` calling `DrawConfirm` last. The reason is that the dim and the prompt
are one thing: a future caller cannot draw the prompt without its backdrop and
get an unreadable frame. `Draw`'s structure changed from "clear, maybe prompt
and return, else list" to "list, then prompt if pending", which is what makes
the list visible underneath.

### 3.3 No `kPaneLayout` entry was added to `ui_scroll_verify.py`

§4.5's table lists `kPaneLayout {330, 76, 880, 52}` among the six layouts, but
that layout does not exist in `Controls` until milestone 3 step 3, and milestone
3 step 7 is the step that points the verifier at it. The verifier's header says
it mirrors "screens, as actually coded", so adding an entry for a layout with no
draw call behind it would make it mirror the plan instead of the code.

It was checked offline all the same, against the same model and the §4.4
constants (pane heading y 220 scale 4, footer y 1000): 8 visible rows, all four
hint constraints clear by 9–51 px. **`kPaneLayout`'s numbers hold under the
honest model** — milestone 3 should not find a surprise there.

### 3.4 The verifier entry is still called `Wizard SaveData`

The drawn sub-heading became `SETTINGS` in milestone 1 and `Step::SaveData` is
renamed in milestone 4. The entry was briefly renamed and then put back,
because plan §5's milestone-4 row refers to it by that name — `"Wizard
SaveData" entry becomes kPaneLayout` — and a verifier entry that no longer
matches the instruction naming it is a trap for the next pass. It is named after
the step, which still exists.

### 3.5 A fifth geometry assertion, G5: the second footer line is on screen

Step 7 asks that "the footer below" be cleared. Moving the wizard's footers
down 30 px spends the bottom margin, and §E4 M2 justifies the move partly by
"leaves the lower footer line's ink bottom at 1074 of 1080" — a number nothing
in the verifier checked. G5 asserts it directly, for every screen, from a
`FOOTER_LINE_GAP = 50` that is true of all seven. Without it, the one thing the
footer move actually risks would have gone unasserted.

### 3.6 Two comments falsified by my own constant changes were corrected

Beyond step 6's two named comments, changing `kHintGap 46` to a per-layout 52
made two adjacent comments arithmetically false:

* `ModelPicker.cpp:27` — "MORE ABOVE then sits at 332 - 46 = 286, comfortably
  below the instruction line's 259 bottom" → now 280, and the clearance is
  restated as ink against ink (10 px).
* `ModelPicker.h:27` — "MORE ABOVE keeps a 27px gap rather than 14px", both
  figures being old `8 * scale` measurements. The sentence now states the
  property without the dead numbers, and says where they came from.

This follows milestone 1's precedent for correcting comments in files the
milestone already owns. Neither is a `PickerStrings` block and neither is
parsed by any verifier.

### 3.7 This report is a separate file

`implementation-report.md` holds milestone 1's report. Overwriting it would
destroy that record and appending would bury it, so milestone 2's report is
`implementation-report-milestone-2.md`. If the pipeline wants one accumulating
file, merging these two is trivial and is the developer's call, not mine.

---

## 4. Verification run

| Check | Command | Result |
| ----- | ------- | ------ |
| Clean build | `cd app && make clean && make` | `IV0000-BBRD00001_00-BBRANDOMIZERMAIN.pkg` built, 7 143 424 bytes; no warnings or errors |
| Scroll and geometry | `python app/tools/ui_scroll_verify.py` | `all geometry and scroll properties PASSED` — 7 screens + the running progress log |
| Geometry model is not vacuous | three mutations, scratch copies | all three caught; see §1 |
| Pool, strings, config | `python app/tools/pool_verify.py selftest data/vanilla/dvdroot_ps4` | `88/88 passing` — identical to milestone 1's result; every picker-string and progress-line case still green |
| Font atlas | `python app/tools/font_atlas_verify.py` | `PASS` — 95 glyphs × 4 sizes, 5 samples × 4 sizes |
| Boss engine mirror | `python app/tools/boss_verify.py selftest data/vanilla/dvdroot_ps4` | `selftest 5/5` |
| Caged dogs mirror | `python app/tools/caged_dogs_verify.py selftest data/vanilla/dvdroot_ps4` | `23/23 passing` |
| Easy modes mirror | `python app/tools/easy_modes_verify.py selftest data/vanilla/dvdroot_ps4` | `25/25 passing` |
| Hunter tools mirror | `python app/tools/hunter_tools_verify.py selftest data/vanilla/dvdroot_ps4` | `selftest 13/13` |

The four engine mirrors are not on milestone 2's verification list — no engine
file was touched — and were run as insurance. All were already green.

**Not run:** `settings_ui_verify.py`, which does not exist until milestone 3;
`pool_verify.py table both`, because no pool table was touched and §3.1 forbids
regenerating them.

### Invariants checked

* **Layering** — the three new methods are on `Renderer`, and every SDL call
  the feature adds is inside `Renderer.cpp`. `grep -rn "SDL_" app/src/UI`
  returns one hit and it is a comment (`ModelPicker.cpp:102`, naming
  `SDL_JOYHATMOTION`); no UI file calls SDL2. Nothing under
  `app/src/Randomizer/` was touched.
* **The three `PickerStrings` blocks** — unchanged in wording, order and
  one-string-per-line form. Only the comment above them changed, and
  `pool_verify.py`'s picker-string cases still parse and pass them.
* **The four progress-log constants** — `kEnemyFailPrefix`,
  `kPoolFellBackLine1`, `kPoolFellBackLine2`, `kNothingRandomizedLine` keep
  their names, their `const char* const kName = "...";` form and their file
  (`EnableWizardScreen.cpp:98-101`). Only the comment above them changed.
* **`pool_verify.py`'s `renderable()` / `LINE_CHARS`** — untouched, per §3.2;
  they pin the `Font8x8` fallback and are still correct.
* **`LIBS` in `app/Makefile`** — untouched. `FillRectBlend` uses
  `SDL_SetRenderDrawBlendMode`, which §E4 M7 confirmed is already in the linked
  `libSDL2.a`; the clean link proves the symbol resolved.

---

## 5. What this does not prove

A clean cross-compile proves these calls link. It proves nothing about what
appears on a television (`CLAUDE.md` §3).

Specifically still unproven:

* **`SDL_SetRenderDrawBlendMode` on this hardware.** This is the point of the
  milestone. The software renderer may ignore the blend mode, may render the
  overlay opaque, or may corrupt the surface. Nothing short of the console can
  say which.
* **That the geometry the verifier asserts is the geometry drawn.** The
  verifier mirrors the constants; it does not execute `DrawScrollHints`. A
  wired-up-wrong `hintGap` would still pass it.
* **The moved footers.** 1074 of 1080 px is six pixels of margin, and TV
  overscan is a real thing on a PS4. The arithmetic cannot tell you whether the
  bottom line is visible on the developer's set.
* **That the six host `Clear` calls cover every path into the picker.** They
  cover every path that exists in the source; a path reached only at runtime
  would show as the previous frame bleeding through.

Milestone 1 is also still untested on hardware (see `log.md`'s process
deviation), so anything found now must be bisected between the two.

---

## 6. Hardware test handoff

Install the `.pkg`. **Before anything else, check `live.log` says
`FontAtlasInit ok`.** If it does not, stop — every geometry number above
assumes the atlas is the live text path, and the `Font8x8` fallback has no
lowercase, so a layout fault would be the wrong diagnosis (plan §3.3).

### 6a. The milestone-2 test — the alpha-blending decision

1. **Open a picker.** `SETUP DEFAULTS` → `ENEMIES INCLUDED`, or the same row in
   the Enable wizard.
2. **Confirm the list draws normally** over a clean dark background. The picker
   no longer clears the screen, so this is the check that its host does.
3. **Press `SQUARE`.** The `ENABLE ALL 82` prompt must appear **over a visibly
   dimmed but still readable list** — you should be able to read the rows
   behind the prompt.
   * **Failure looks like:** the list behind the prompt is not dimmed at all;
     or it has vanished and the prompt sits on black; or the frame tears,
     flickers or shows garbage. **Any of those three means alpha blending does
     not work on this hardware — stop and report.** Plan §4.3's no-blend
     scanline fallback is a visual change the developer approves, not one the
     implementer picks, and it was deliberately not written speculatively.
4. **Press `O` to cancel, then `TRIANGLE`** and check the `DISABLE ALL` prompt
   behaves the same way. Cancel again.
5. **Repeat step 3 on all three pickers** — `ENEMIES INCLUDED`,
   `ENEMIES SKIPPED` (it has an instruction line and a different list band),
   `BOSSES INCLUDED` — from **both** screens, which is six host draw sites.
   * **Failure looks like:** on one of the six, the picker draws over whatever
     was on screen before it, leaving old text showing through.
6. **Check the moved footers.** In the wizard's settings list and in Confirm,
   scroll to the bottom so `MORE BELOW` appears.
   * Both footer lines must be fully visible and not clipped by the bottom of
     the screen.
   * `MORE BELOW` must not touch the last settings row above it, nor the footer
     below it.
   * **Failure looks like:** the lower footer line cut off or missing, or
     `MORE BELOW` sitting on top of a row's descenders.
7. **Scroll to the top** and check `MORE ABOVE` clears the `SETTINGS` /
   `CONFIRM` sub-heading and the first row.

### 6b. Accumulated and still untested — milestone 1

Milestone 1 has never run on hardware. Its test is §6 of
`implementation-report.md` and should be run in the same session: the 19-row
lists with every label against the right value, an existing `defaults.cfg`
still loading, a commit whose first progress line is `USING SEED …` and whose
log contains none of the five save-data lines.

If something is wrong, the milestone-1 renumbering hazard (plan §3.3) is the
first thing to suspect, not the drawing changes above.

---

## 7. Stop point

Milestone 2's **completion gate** ended this pass: every check in the
milestone's verification list passes and the `.pkg` builds. **No stop condition
fired.**

Milestone 3 — the settings model and the categorised `SetupDefaultsScreen` —
has **not** been started. It waits on §6a, because §4.6 has every overlay in
milestones 3 and 4 built on the `FillRectBlend` that test settles.
