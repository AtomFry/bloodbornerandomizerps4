# Log — Randomizer Settings UI

Append-only. Newest entry at the bottom.

Unnumbered folder: this is platform/UI work with no row in
`docs/randomization-feature-spec.md`, following the precedent of
`docs/features/font-atlas/`. See `docs/features/README.md`.

## 2026-09-20 — Stage C: plan and evidence written

`planner` investigated against `spec.md` (status APPROVED, 2026-09-20) and wrote
`plan.md` — the implementation contract — and `plan-evidence.md` — the
investigation behind it. Four milestones: save-data removal; renderer primitives
and a corrected geometry model; the settings model plus a categorised
`SetupDefaultsScreen`; the categorised Enable wizard.

The contract came in at 591 lines against a ~400-line budget. Checked for leaked
evidence and history and found none; the overage is scope — four milestones,
~30 ordered steps, 25 files, a pixel-geometry contract and eight verifier
assertions. Recorded rather than trimmed by deleting steps.

Three things the planner found wrong or understated in the approved spec, all
surfaced rather than planned around:

* **Spec §4.5 understates the verifier coupling.** `pool_verify.py` binds more
  than string literals in `EnableWizardScreen.cpp`: it also parses `ModelPicker.h`
  (lines 661–663) and `RandomizerDefaultsStore.cpp`, and line 706 asserts
  `worst == 669` as an exact equality over a sum that begins, at line 686, with
  `backup_existing_save=1`. The save-data removal therefore breaks that case by
  construction; new value 616.
* **Spec §4.6 is right that the height model is stale and wrong that correcting it
  is free.** Under the real atlas ink box, `Wizard SaveData` and `Wizard Confirm`
  have no feasible scroll-hint gap at all, and `Setup Defaults` overlaps by 3 px.
  The fix is a per-`ListLayout` `hintGap` plus a 30 px footer move on the wizard.
  A single-constant fix passes a line-box model and is wrong.
* **Spec Appendix B's "the model must express editable-here-but-not-there" is not
  borne out.** All 18 settings are editable on both screens; the asymmetry is the
  two host items and what happens at exit.

Also recorded: the reference tool does carry per-setting help, in `ToolTip`
attributes on all 51 checkboxes, which corroborates spec Appendix A.

Four questions went to §8: the seed's focus position (flagged as a **spec gap**),
the setting-label wording (flagged as a **spec contradiction** between §6 and
§7.1/Appendix A), Confirm's row order, and whether milestone 3 should wait on the
font atlas being hardware tested (flagged **blocking**).

## 2026-09-20 — Stage C: questions answered, plan reconciled

The developer answered all four.

* **Q1 — seed access. Answered against the planner's recommendation.** The seed
  becomes its own row in the rail column, and the header band is purely
  decorative and never a focus target. By symmetry `BLOODBORNE TITLE ID` becomes
  the same row on `SetupDefaultsScreen`. Recorded as §9 D1.
* **Q2 — setting labels.** Keep today's shipped strings; spec §7.1 and Appendix A
  are prose, not label specifications. Recorded as §9 D2.
* **Q3 — Confirm's row order.** Generate from the model in category order, seed
  first. Recorded as §9 D3.
* **Q4 — the font-atlas gate.** Overtaken by events: the developer has verified
  the atlas on hardware and it is good. No milestone is gated on it. Recorded as
  §9 D4.

Step 7 reconciliation, the bulk of it driven by D1:

* **Rail geometry re-derived from the ink-box model, not patched.** Adding a rail
  row that must hold `BLOODBORNE TITLE ID   CUSA03173` (552 px at scale 3) made
  the 520 px rail 32 px short, so the columns were resolved again:
  `60 + 580 + 20 + 2 + 18 + 700 + 20 + 2 + 18 + 440 + 60 = 1920`. Rail row 0 at
  y 230, a rule at y 296, the six categories at y 330 pitch 76 — deliberately the
  same band and pitch as the pane rows — a second rule at y 778 and `FINISH` at
  y 806. The column's ink ends at y 850, inside its own column rule and clear of
  the footer. The 60 px the rail gains comes out of the help pane, costing it one
  wrapped line at worst (9 → 10 of an allowed 11). `kPaneLayout` and every
  `hintGap` are unaffected: rail and pane are separate columns.
* **A new rule the answer forced:** the pane always draws `lastCategory_`,
  highlighted only while focus is in the pane, so selecting the seed row or
  `FINISH` never blanks a third of the screen. Recorded as §9 P13.
* **A string the spec does not draft.** `BLOODBORNE TITLE ID` has no Appendix A
  entry but §6 case 4 requires non-empty help for every rail row, so §4.1 now
  supplies one, drawn from the field comment in `RandomizerDefaults.h`. Recorded
  as §9 P14.
* Rewritten for D1: §2 B2/B3/B12, §4.2's `railCursor_` semantics and help-pane
  rule, the whole §4.4 geometry table, §5's two screen rows, §6 verifier cases 4–6
  and the milestone-3 and milestone-4 hardware handoffs, and §7's milestone-3
  step 4 and milestone-4 steps 3, 4 and 7.
* Rewritten for D4: the §3.3 atlas hazard is no longer a gate but a `live.log`
  check on each handoff; the §7 stop condition became "`FontAtlasInit: texture
  build failed` appears in `live.log`"; `plan-evidence.md` §E5.3 was rewritten
  from "the atlas is unconfirmed" to "the fallback, if it ever fires", and §E7
  dropped it as an unknown.
* `plan-evidence.md` also updated: §E2.4 and §E6 for D1 and D2; §E3 gained the
  rejected header-as-focus-target and value-in-header-only alternatives; §E4 M3
  and M4 re-measured, with the superseded column split recorded and why it was
  wrong; §E7 replaced the settled questions with what remains open — whether two
  copies of the seed read as redundant, and legibility of 10 prose lines in a
  440 px column.
* §E2.6 and §E6 now cite `pool_verify.py` lines 686 and 706 as established fact
  rather than as something to check.

§8 emptied, heading kept with one line pointing at §9 D1–D4. §9 P3 and P8 were
corrected where D1 and D2 settled what they had left provisional. Status set to
`QUESTIONS ANSWERED — awaiting developer approval` — the developer has not yet
read the reconciled plan.

Contract after reconciliation: 626 lines for §1–§7. D1 added roughly 35 lines of
genuine new content (a rail row, two rules, a help string, the `lastCategory_`
rule); about 5 were clawed back by tightening, and the budget overage stands as
recorded above.

---

## 2026-09-20 — Stage E, milestone 1

The developer approved the plan and asked for implementation directly; stage D
(plan review) was offered and deliberately not run.

Milestone 1, "remove the save-data handling", **completed at its completion
gate**. No stop condition fired. Milestone 2 not started.

**Deviations from the contract: none.** Every §7 step was implemented as written,
in order.

**Three decisions the contract left to the implementer**, recorded in `plan.md`
§10 and `implementation-report.md` §3:

* The wizard's drawn sub-heading `SAVE DATA` → `SETTINGS`. The C++
  `Step::SaveData` identifier is untouched; that rename stays in milestone 4.
* Stale header and tombstone comments corrected, only in files §5 already
  assigns to milestone 1. The tombstones name the retired config *keys* but
  avoid the retired *field* identifiers, so milestone 3's
  `settings_ui_verify.py` case 7 grep is not tripped by prose.
* The new `pool_verify.py` case checks the quoted key literal, the `key=%d`
  format fragment and the field name separately, rather than searching the whole
  file — a whole-file search would have failed against the tolerance comment §7
  step 2 required be written.

**Verification, re-run independently by the dispatching session rather than
taken on report:** `ui_scroll_verify.py` PASSED at 19 rows on all three settings
lists and 18 on the progress log; `pool_verify.py selftest` 88/88 with the config
case at 616 bytes; `boss_verify` 5/5, `caged_dogs_verify` 23/23,
`easy_modes_verify` 25/25, `hunter_tools_verify` 13/13, `font_atlas_verify` PASS.
`make clean && make` produced the `.pkg` with no warnings.

Also confirmed independently: `plan.md` §1–§7 is unchanged at 626 lines and only
§10 was written; every one of the ten changed files appears in §5's milestone-1
rows, with no file touched outside them; the four progress-log constants keep
their names and `const char* const` form; `StartCommit`'s run decision `||` is
intact with `randomizeWorkshopTools` and `doNotRandomizeCagedDogs` still absent
from it; `defaults_.lastSeed` is the only write-back; and no identifier from the
spec §4.8 inventory survives anywhere under `app/src`.

**Not run:** `settings_ui_verify.py`, which does not exist until milestone 3.

**Noticed and deliberately not fixed**, because the files are not in §5 and doing
so would have hit a stop condition: `RandomizerDefaultsStore.h` line 14 still
describes a fresh install as "backup existing save = YES"; and
`docs/user-guide.md`'s "Settings at a glance" table was already missing
`DO NOT RANDOMIZE CAGED DOGS`, `START WITH HUNTER TOOLS` and the four easy-mode
settings before this milestone touched it.

**Now awaiting hardware test.** A clean build and green mirrors mean ready for
hardware testing, not done (`CLAUDE.md` §3). Milestone 2 must not start until
that test passes — starting it first would chain two milestones, which
`CLAUDE.md` §4 exists to prevent.

### Process deviation — milestone 1 not hardware tested before milestone 2

The developer could not run hardware tests at this point and chose not to delay,
stating they would test the accumulated milestones together later. Milestone 2
was therefore started with milestone 1 built but untested.

This is a deliberate, developer-authorised departure from `CLAUDE.md` §4, which
exists to stop milestones chaining. Recorded because it changes what a later
failure can tell us: a fault found in a combined test cannot be attributed to a
milestone without bisecting, and the milestone-1 renumbering hazard (§3.3) is
exactly the kind of fault a combined test reports late and vaguely.

Milestone 2's completion gate is itself the alpha-blending decision — the point
of testing it early is to avoid building milestones 3 and 4 on an unproven
primitive. Deferring past milestone 2 therefore costs more than deferring past
milestone 1 did.

---

## 2026-09-20 — Stage E, milestone 2

Milestone 2, "renderer primitives and an honest geometry model", **completed at
its completion gate**. No stop condition fired. Milestone 3 not started.
Milestone 1 remains built but not hardware tested (see the deviation above).

**One deviation, and the contract was at fault.** §5 and §7 step 4 instruct the
implementer to remove "both `renderer.Clear` calls" from `ModelPicker.cpp`. The
file has **one**, at the top of `Draw`. It served the prompt path as well,
because `Draw` cleared and then early-returned into `DrawConfirm`, so removing
the single call removes the clear from both paths and the intent is satisfied.
Confirmed independently against `git show HEAD:app/src/UI/ModelPicker.cpp`:
exactly one `renderer.Clear`, at line 129. The plan's count was wrong, not the
implementation.

**Seven decisions the contract left to the implementer**, recorded in `plan.md`
§10. The notable ones: `kOverlayAlpha` placed in `Controls.h`; the blend issued
as `DrawConfirm`'s first line so the prompt cannot be drawn without its backdrop;
no `kPaneLayout` entry added to the verifier, because milestone 3 step 7 owns it;
and a fifth assertion G5 added that the second footer line is on screen, since
the step-3 footer move is what spends that margin.

**Verification, re-run independently by the dispatching session:**
`ui_scroll_verify.py` PASSED on all seven screens plus the running log;
`pool_verify.py selftest` 88/88, unchanged from milestone 1; `font_atlas_verify.py`
PASS; `make clean && make` produced the `.pkg` with no warnings. The implementer
additionally ran a three-mutation test of the new height model, reporting that all
three were caught and reproduced §E4 M2's predicted failures to the pixel.

Also confirmed independently: `plan.md` §1–§7 unchanged at 626 lines with only
§10 appended; every `ListLayout` initialiser matches the §4.5 table exactly
(`kSettingsLayout` 420/90/870/**60**, `kProgressLayout` 300/70/920/**50**, both
picker layouts .../**52**); the two moved footer lines sit at `kScreenHeight -
100` and `- 50` on the two `kSettingsLayout` screens only, with every other
footer in the app left at its original position; `FillRectBlend` restores
`SDL_BLENDMODE_NONE` unconditionally before returning; and the three
`PickerStrings` blocks in `ModelPicker.h` are untouched — the diff there is
comment-only.

**§7 step 7's condition held.** No constant was changed to make the rewritten
verifier pass except the six §4.5 `hintGap` values and the two footer positions.
The derived ink box (9..44 / 14..58 / 16..73 / 20..86) matches `plan-evidence.md`
§E4 M1 exactly, and every screen's visible-row count is unchanged from the
shipped build.

**Not run:** `settings_ui_verify.py` (does not exist until milestone 3);
`pool_verify.py table both` (no pool table touched, and §3.1 forbids
regenerating them).

**Noticed and deliberately not fixed:** `Controls.h`'s `DrawScrollHints` comment
and `pool_verify.py`'s `LINE_CHARS = 71` comment both still read as though
`Font8x8` were the live text path. Neither is false — both describe the fallback,
and printable ASCII has no arrow glyphs either — but both are misleading. §4.5
names only two comments to correct and §3.2 forbids touching `pool_verify.py`'s
`renderable()`/`LINE_CHARS` checks, so both were left.

**Now awaiting hardware test — and this is the alpha-blending decision.** Two
milestones are now accumulated and untested.

### Process deviation extended — milestone 3 started with two milestones untested

The developer was advised that milestone 2's completion gate is the
alpha-blending decision, and that starting milestone 3 means building the
categorised screen — every overlay in it — on a primitive no hardware has
confirmed. They elected to proceed. Recorded as their decision.

Consequence: if `FillRectBlend` does not work on this hardware, §4.3's scanline
fallback will be approved with milestone 3's screen already written against
blending, and the combined test will be reporting faults from three milestones
at once.

---

## 2026-09-20 — Stage E, milestone 3

Milestone 3, "the settings model and the categorised Setup Defaults screen",
**completed at its completion gate**. No stop condition fired. Milestone 4 not
started. Milestones 1 and 2 remain built but not hardware tested.

**The central objective is met.** Spec §4.1 identified positional row indices as
the feature's main obstacle. Confirmed independently: no `k*Row` constant and no
`ToggleRow` survives in `SetupDefaultsScreen.{h,cpp}` outside of two tombstone
comments, and `AdjustSetting` is the only writer of a setting value — the one
other assignment, `working_.bloodborneTitleId = editBuf_`, is the title-ID
editor committing a host item, not a setting.

**One deviation, and as in milestone 2 the contract's wording was at fault.**
§7 step 5 says "route its **three** pickers". The implementation has one
`Mode::Picker` plus a `const SettingDef* pickerSetting_`, because the step's own
done-condition — "no `SettingId` is named at the call site" — cannot be satisfied
while three modes each map back to a named setting. The collapse is what the
condition asks for. Knock-on: milestone 2's six host `renderer.Clear` sites
become four, since this screen's three became one. The wizard's three are
untouched.

**Ten decisions the contract left to the implementer**, recorded in `plan.md`
§10. The one with downstream consequence: **the §4.4 geometry constants are
file-local**, because §5 assigns milestone 3 no shared layout file. Milestone 4
therefore has a duplicate-or-promote decision waiting. `settings_ui_verify.py`
pins the numbers, so a divergence fails the verifier rather than shipping.

**Verification, re-run independently by the dispatching session:**
`settings_ui_verify.py` **28/28** across all eight §6 cases;
`ui_scroll_verify.py` PASSED on all seven screens; `pool_verify.py selftest`
88/88 with no case changing state; `font_atlas_verify.py` PASS;
`make clean && make` produced the `.pkg` with no warnings, picking up
`SettingsModel.cpp` with no Makefile change.

Also confirmed independently: `plan.md` §1–§7 unchanged at 626 lines with only
§10 appended; setting labels are the shipped strings per §9 D2, with the spec's
shorter prose names appearing only inside a comment that explains the decision;
and the Appendix A help text is carried faithfully, with markdown emphasis
stripped and the em-dash rendered as a hyphen — a necessary change, since the
font atlas covers printable ASCII and has no em-dash glyph.

The verifier reports the geometry budgets as: widest rail row 552 of 580 px,
widest pane row 643 of 700 px, worst help body 10 of 11 lines — all inside the
margins §4.4 sized, with the help body the tightest.

**Not run:** the four engine selftests and `pool_verify.py table both`.
Milestone 3's §7 verification list does not call for them and nothing under
`app/src/Randomizer/` was touched. Recorded as not run, not as passed.

**Noticed and deliberately not fixed:** two tombstone comments still name
`ToggleRow` and `kEnemiesIncludedRow` as history, though no such identifier
survives; `EnableWizardScreen.cpp` still carries its 19 row constants and its
duplicated settings list, which is milestone 4's job; and
`Renderer::LineHeight`, added in milestone 2, still has no caller because the
pane spaces rows by the plan's fixed pitch.

**Now awaiting hardware test. Three milestones are accumulated and untested**,
and the alpha-blending decision is still outstanding — this screen's overlays
stand or fall with it.

---

## 2026-09-20 — Stage E, milestone 4 (final)

Milestone 4, "the categorised Enable wizard", **completed at its completion
gate**. No stop condition fired. **All four milestones are now built. None has
been hardware tested.**

**B17 output parity — the strongest static evidence available, obtained
independently.** The dispatching session did not rely on the implementer's
side-by-side reading. It extracted every `options.<field> = <source>` assignment
from `git show HEAD:app/src/UI/EnableWizardScreen.cpp` and from the current file,
normalised only the `run_.` prefix and the trailing-underscore member naming, and
diffed:

```
HEAD assignments: 18   NOW: 18
IDENTICAL - same fields, same sources, same ORDER
```

This is a stronger check than the one the contract asked for, because HEAD
predates **all four** milestones: it shows the option boundary did not drift at
any point in the feature, not merely across milestone 4.

The run-decision `||` was compared the same way: **13 terms in HEAD, 13 now,
identical set**, with `randomizeWorkshopTools` and `doNotRandomizeCagedDogs`
confirmed still absent. `defaults_.lastSeed` is the only assignment to
`defaults_`.

This closes everything static analysis can close. The residual risk is unchanged
and irreducible: only the byte-identical run can prove B17.

**The feature's central objective is met.** No per-setting row constant and no
second hardcoded settings list survives in `EnableWizardScreen.{h,cpp}`. Zero
literal setting labels remain in the file — every label comes from
`SettingsModel`, and `ConfirmItems()` generates Confirm's list by iterating
`SettingAt(i)`. The one surviving `*Row` identifier, `kFinishRow`, is a rail
position derived from `kCategoryCount`, not a positional setting index.

**Three deviations**, all recorded in `plan.md` §10:

1. **Geometry constants duplicated into `EnableWizardScreen.cpp` rather than
   promoted** to a shared file, because §5 assigns milestone 4 no such file and
   creating one is a stop condition. Mitigated beyond the contract: the verifier
   now parses *both* screens, fails on any disagreement, and checks its own third
   copy against them.
2. **One `Step::Picker`, not three** — as milestone 3's §10 row predicted. Draw
   sites performing their own `Clear`: six → four → **two**.
3. **`OPTIONS` no longer opens Confirm from the settings step.** `FINISH` + `X`
   is the only route, per B6 and spec 9.2. §7's done-conditions name only the
   left/right and `X` chains, so this was flagged rather than assumed.

**Verification, re-run independently by the dispatching session — the full §7
list, nothing skipped:** `settings_ui_verify.py` **38/38**;
`ui_scroll_verify.py` PASSED; `pool_verify.py selftest` 88/88 with no case
changing state; `boss_verify` 5/5; `caged_dogs_verify` 23/23;
`easy_modes_verify` 25/25; `hunter_tools_verify` 13/13; `font_atlas_verify` PASS;
`make clean && make` with no warnings. `plan.md` §1–§7 unchanged at 626 lines,
§10 only.

The implementer additionally negative-tested the new cross-screen geometry case,
reporting that perturbing one constant fails it by name and that 32 of 32 names
resolve in both files — i.e. the case is not vacuous.

**Noticed and deliberately not fixed:** `settings_ui_verify.py` still retypes the
footer *strings* rather than parsing them from the screens — the same hole that
was closed for the geometry constants; `Palette::SelectedBar {56,44,18}` and
`kRuleColor {64,72,82}` remain monitor-chosen and unseen on a TV.

**Stage E is complete. The feature now rests entirely on one hardware session**
carrying four milestones' worth of untested change, including the alpha-blending
decision on which every overlay in both new screens depends.

---

## 2026-09-21 — Post-implementation amendments (A1–A4)

Developer feedback after reviewing the built screens. Four changes, made
directly rather than through a new plan cycle: none alters the settings model,
the wizard flow, or anything `EnemyRandomizerOptions` touches. Recorded as
amendments in `spec.md` §10 (A1–A4) with the spec body reconciled; `plan.md`
§1–§7 was **not** edited, so stage F can still compare the implementation
against the contract that was approved.

**A1 — the seed moved to the middle pane.** It had been on the rail row and in
the header band; the pane made three copies visible at once. The developer chose
to keep it only in the pane. The rail row is now a bare `SEED` label and the
header carries only `TARGET`. The seed is still on `FINISH`'s readiness summary,
which is where it matters — immediately before committing.

**This forced a change the developer did not ask for, and it is worth flagging.**
`SetupDefaultsScreen` mirrored the old wizard exactly: `BLOODBORNE TITLE ID`
appeared both in its header and on its rail row. Leaving it alone would have made
the two screens behave differently for their row-0 item, which is precisely the
divergence `settings_ui_verify.py` case 6 exists to catch — and it did catch it.
The same treatment was applied to Setup Defaults rather than weakening the
invariant. A consequence the developer may want to revisit: Setup Defaults has no
`FINISH` summary, so its title ID is now only visible while row 0 is selected.

**A2 — the seed is only ever changed in its own editor.** Left/Right on the rail
did an irreversible thing from a direction press, with no confirmation and no
undo. They now do nothing anywhere on the rail. `SQUARE` rolls a random seed
**into the editor's buffer**, so `X` still confirms and `O` still discards it.
This required splitting `RollNewSeed()` — which assigns `seed_` — from a new
`NextSeedValue()` that only returns a value. Without that split, SQUARE would
have committed the seed the moment it was pressed and `O` would not have
discarded it. That was caught while editing, not by a verifier.

**A3 — picker rows are aligned, not padded.** `ModelPicker` built each row by
space-padding the label to a fixed character count and drawing the result
centred. Character padding only aligns columns in a monospace font, so this had
been silently broken since the font atlas landed — the developer reported it as
"a bit disorganized", which is exactly what it was. `PadTo` and `NameFieldWidth`
are gone; rows are drawn label-left at x 420 and flag-right at x 1500, a 1080 px
band still centred on the surface. Sized against the widest row any of the three
tables can produce — the boss list's `C4520 LADY MARIA OF THE ASTRAL CLOCKTOWER`
at 791 px — plus the widest flag, `SKIPPED` at 127 px, leaving 162 px between.
This was the last surviving monospace-padding site in the codebase.

New `settings_ui_verify.py` **case 9** asserts the band against the real tables
and that it stays centred, so a regenerated table with a longer display name
fails there rather than overlapping the flag column on a TV.

**A4 — `World & Flavour` renamed `World`**, in the model, the enum, the verifier
and the spec.

**Verifier changes.** `kHeaderY` and `kHeaderScale` left `SHARED_GEOMETRY`:
`kHeaderScale` is gone from both screens, and `kHeaderY` is now wizard-only since
Setup Defaults' header band is empty. `kHeaderY` is still pinned against the
wizard by the mirror check, so it is unshared, not unpinned.

**Verification after the amendments:** `settings_ui_verify.py` **40/40** (38 plus
the two new picker cases); `ui_scroll_verify.py` PASSED; `pool_verify.py
selftest` 88/88; `boss_verify` 5/5; `caged_dogs_verify` 23/23;
`easy_modes_verify` 25/25; `hunter_tools_verify` 13/13; `font_atlas_verify` PASS;
`make clean && make` with no warnings.

Nothing under `app/src/Randomizer/` was touched, and no `options.` assignment
changed, so the B17 output-parity evidence recorded for milestone 4 still stands.

**Still awaiting the hardware session**, now carrying four milestones plus these
amendments, with the alpha-blending decision still outstanding.

### 2026-09-21 — A5, picker scrim darkened

Developer feedback from **hardware**: text from the settings screen showed
through the picker's overlay clearly enough to compete with the picker's own
rows.

**This also settles the feature's biggest open risk.** For the parent's text to
be visible behind the overlay at all, `FillRectBlend` must be compositing
correctly — so alpha blending works on the PS4's software renderer. That was the
one unproven SDL entry point in this feature, the thing milestone 2's completion
gate existed to decide, and §4.3's no-blend scanline fallback is now unnecessary.
The developer should confirm this reading explicitly before it is recorded as a
hardware-test result.

Blur was considered and rejected: a box blur over 1920x1080 on a CPU rasteriser
costs several passes over two million pixels per frame, which this renderer
cannot afford. Opacity is the available lever.

`kOverlayAlpha` is replaced by two constants, because the two overlays want
opposite things and one number could not serve both:

* `kScrimAlpha` = **235** - a picker covering the settings screen. What is
  behind is context, not information. A pixel survives at (255-alpha)/255, so
  235 leaves 7.8%: parent body text lands near (17,17,18) on a (2,2,2) ground,
  present as shape but not legible as words. At the old 190 it survived at
  25.5%, text at (55,56,58), which is what the developer was reading through.
* `kPromptAlpha` = **190**, unchanged - the select-all/none prompt over its own
  list. Here what is behind IS the information the prompt is asking about;
  "SKIP ALL" means nothing without the rows it would skip. Darkening this one
  to match would have broken milestone 2's stated intent that the counts be
  readable against the rows they came from.

Applies to all three picker paths: the wizard's, Setup Defaults', and the
prompt within them.

**Verification:** `settings_ui_verify.py` 40/40; `ui_scroll_verify.py` PASSED;
`pool_verify.py selftest` 88/88; `make clean && make` with no warnings. No
geometry changed, so no verifier case needed updating.
