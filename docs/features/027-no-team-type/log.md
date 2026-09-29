# Feature 027 — log

Append-only. Every stage adds; none rewrites. `spec.md` and `plan.md` state what
is true now; this states how they got there.

---

## 2026-09-28 — stage B, the spec

Written by the `spec-author` subagent. Row 27 was chosen as the next item after
row 11 on the grounds that §11 pairs them as one block, `NpcParam` is already
parsed by the drop randomizer, and the work is offline-verifiable — which
mattered because the developer could not hardware-test that day.

**The investigation found the backlog row's description is probably wrong**, and
that is the main thing this spec contributes. The row says "everything is
hostile to everything — enemies fight each other". The reference is fifteen
lines that write the constant **25** into `NpcParam.teamType` on every row, with
no exclusions and no randomness. But **378 rows already carry 25** — Gehrman,
the Witch of Hemwick, the Blood-starved Beast, the Brain of Mensis and every
Maneater Boar — and none of them fight each other. The claim traces to the
checkbox's internal name `AllDmgAll`, the author's *intent*, not observed
behaviour. `docs/windows-randomizer-technical-review.md` §5.11 was more careful
and labelled its own reading an inference; §13 items 4 and 5 already flagged this
pass as unexplained.

**Three questions, all answered the same day. Two took the recommendation:**

* **D1** — port it verbatim, off by default, and learn the effect from hardware.
* **D2** — no protection list; all 31,398 rows written, matching the reference.
  Carried by two things: 19 of the 21 Hunter's Dream Messenger placements
  already sit on the *ordinary enemy* allegiance and are perfectly peaceful, so
  the field is demonstrably not sufficient to make something hostile; and
  `docs/enemy-exclusion-history.md` records what the last speculative protection
  list cost.

**D3 was taken AGAINST the spec's recommendation.** The spec proposed a
mechanism-flavoured label (`ONE FACTION FOR EVERYTHING`) because
`docs/known-traps.md` forbids naming a setting for a runtime effect nobody has
observed. The developer chose **`ENEMIES HOSTILE TO EACH OTHER`**, with the risk
stated in the question and accepted: if the write has no such effect, the label
asserts something false, which is the `ENABLE MERGO DARKNESS` failure repeated.
Two consequences were made binding rather than optional — the help text must not
compound the claim, and §8's first acceptance criterion is whether the label
tells the truth. The config key stays `no_team_type` so a label revision touches
no saved file.

Status ended at `QUESTIONS ANSWERED`, then **`APPROVED` by the developer the
same day**.

---

## 2026-09-28 — stage C, the plan

Written by the `planner` subagent against the approved spec.

**No questions were asked, and §8 was never populated.** The spec's D1–D3 settled
the behaviour, value, row set, label, category and key; the repository settled
file placement, pass position, byte arithmetic and verification shape. §8 carries
one line saying so. **Step 7 reconciled nothing**, because no decision arrived to
make any part of the body untrue.

**Two milestones, continuous, no gates.** The planner's argument for no
intermediate gate is stronger than usual and was checked rather than accepted: a
gate is not merely unnecessary but *unavailable*, because the setting is
unreachable until M2 adds it, so there is nothing a human could turn on after M1.
The whole risk of this feature is what the written value means in play, and only
the final hardware test can reduce that.

**One factual slip found in the approved spec.** §4 says three base-map
placements reference `NpcParam` rows that do not exist. Joining all 2,877
placements to the row table found **zero** unresolved references. Nothing depends
on it and it makes the spec's own conclusion stronger, so it was recorded in
`plan-evidence.md` §E6 rather than edited into an approved document. Every other
§4 figure reproduced exactly.

**Two risks the plan surfaced that the spec had not**, both verified
independently by the orchestrating session:

1. **`drops_verify.py`'s D-I2 has a hole, and it is not a wording problem.** The
   invariant claims "within `NpcParam`, only `itemLotId_1` differs — everything
   else intact". It compares bytes 0–43, then a slice
   `[vat+48 : vat+48]` which is **empty**. Bytes 52–387 of a 388-byte row are
   compared by nothing, and `teamType` at offset 303 sits inside that range —
   so the tool currently passes a tree in which every team type was rewritten.
   This is the row-11 F13 situation repeated in a different tool. The plan makes
   the correction a required M1 change, pinned in the failing direction, plus a
   `--team-type` declaration flag.
2. **The easy-mode larva loses `teamType` 26.** Rows 252100 and 6071 and all 31
   scaled variants `900014601`–`900014631` carry 26 today; this pass moves them
   to 25. Feature 018 rests on the assumption that 26 means non-hostile, so with
   this setting and any easy mode on together the larvae might stop being
   harmless — in a boss arena. The planner did **not** exempt them: D2 and
   `CLAUDE.md` §7 forbid it, and it is exactly the unevidenced protection
   `docs/enemy-exclusion-history.md` records the cost of. It is a §3.3 hazard,
   §6 states plainly that the hardware test does not cover the combination, and
   if it matters the observation becomes the justification for a one-row
   exemption later.

**Also recorded:** `docs/features/randomizer-settings-ui/spec.md` §7.1 lists this
row under Enemies while D3 puts it in World, and `settings_ui_verify.py` case 3
pins that table against the model — so the spec edit is a required M2 step, not
housekeeping. And the config byte arithmetic collides with feature 011's:
`no_team_type` is +15 (728 → 743), 011's two keys are +47, and whichever lands
second rebases on the other.

**Contract budget.** §1–§7 came to 408 lines against a nominal 400.

Status ended at `QUESTIONS ANSWERED — awaiting developer approval`.

---

## 2026-09-28 — the two risks, after the strategy was presented

The developer asked for fixes to the two risks reported with the Execution
Strategy. **The first was already fixed and the report had mislabelled it** —
the `drops_verify.py` D-I2 correction is M1 change 4, and it appears in §5's file
table, §6's verification, selftest cases D-S1–D-S3 and a stop condition. Nothing
to fold in.

**The second was genuinely open**, and splits into two fixes at two different
levels:

* **Plan level, folded in.** §6 gained item 7: turn `EASY SHADOWS` on alongside
  this setting and reach the Shadows of Yharnam, recording whether the
  substituted larvae still stand inert. §6 previously said the combination was
  "not covered, and known not to be"; it is covered now. §3.3's mitigation
  column was rewritten to point at it.
* **Spec level, put to the developer and declined.** Actually *exempting* rows
  `252100`, `6071` and the 31 scaled variants would contradict spec D2 and needed
  the spec amended, so it was asked rather than assumed. **D1: no exemption.**
  The reasoning recorded with it — an unevidenced protection list is the mistake
  `docs/enemy-exclusion-history.md` documents; if the larvae do stop being
  harmless, that observation is both the justification for a one-row exemption
  and the first evidence that this field decides hostility at all, which is the
  thing nothing offline can establish.

§8 now carries one line; §9 carries D1 above the planner's P1–P6.

---

## 2026-09-28 — stage E, both milestones. Plan 027 fully built.

Built by the `implementer` subagent. **Stage D (plan review) was skipped**, as it
was for row 37. Both milestones completed in one run and **no stop condition
fired**, which is what the Execution Strategy's continuous mode with a final-only
gate calls for.

**What was built.** M1 the pass and its proof — `TeamType.{h,cpp}`, the
`noTeamType` option, the call in `StepItemData`, the `drops_verify.py` D-I2
correction and the new `team_type_verify.py`. M2 the setting a player can reach —
the defaults field, `no_team_type` in both directions, the `World` row, the
activation mapping and the progress line.

**Sixteen files changed, and all sixteen are named in plan §5 — zero unnamed
files**, confirmed by the orchestrating session against the §5 table. `README.md`
and `docs/randomization-feature-spec.md` also show modified, but those are this
session's own orchestration edits for rows 11, 13 and 27; the implementer did not
touch either, and `README.md`'s row 27 still carries no implementation-report
link. `DropRandomizer.{h,cpp}` was not edited, as §5 requires.

**One deviation, recorded in plan §10.** `worlds_verify.py selftest` does not run
the checks §6 credits it with: that mode runs only manifest `rule_cases()`, while
the two pinned lists live in `source_cases()`, which only `all` runs. The
implementer ran both. **This is a defect in the plan's §6, not in the tool**, and
it is left for stage F to rule on. Independently re-run here: `selftest` **29/29**
and `all` **all checks passing**.

**Five decisions the contract left open, all in the implementation report.** The
two worth reading:

* **A bounds check on the field.** `ParseParamRows` only proves a row *starts*
  inside the member, so the pass checks `dataOffset + 303 < plain.size()` and
  **ends the run** rather than storing unchecked — matching `GrantHunterTools`.
  Chosen over skipping the row, which would have broken the "no row is excluded"
  invariant that D2 makes the whole point of this feature. It never fires on the
  real archive.
* **`--drops` tolerance extends to T-I5**, because a combined run can reassign
  `itemLotId_1` in a row that already held 25.

### Verification, re-run independently by the orchestrating session

Not taken from the report. `team_type_verify.py selftest` **20/20**;
`drops_verify.py selftest` **12/12** with the six pre-existing cases unchanged;
`pool_verify.py selftest` **96/96** (block 743, worst case 794, and
`027: no_team_type is in both load and save`); `settings_ui_verify.py` **113/113**
with `World 2 … ENEMIES HOSTILE TO EACH OTHER`, widest pane row **651/700 px** and
worst help body **7/11 lines** — exactly §4.3's predictions;
`itemdata_verify.py roundtrip` both PASS; `worlds_verify.py` per the deviation
above; `hunter_tools_verify.py selftest` **25/25** and
`easy_modes_verify.py selftest` **25/25**, both unchanged, which is what says the
neighbouring param features still produce what they did.

`team_type_verify.py census` reproduces the plan's measurements exactly: **31,398
rows, stride 388, offset 303, 13 distinct values, 378 already at 25, 31,020 to
change**, and the tool prints `matches what plan 027 was measured against: yes`.

**Also confirmed independently:**

* **The `.pkg` is real and is this work.** 7,143,424 bytes at 20:07:10, which
  postdates every changed source file (latest 20:03:36). A stale artifact would
  have failed this check.
* **`plan.md` §1–§7 is untouched.** Every section heading sits on the same line
  number recorded before dispatch — Execution Strategy 19, §1 50, §2 61, §3 79,
  §4 142, §5 241, §6 267, §7 393, §8 467, §9 480, §10 496 — so only §10's body
  grew. The folder is untracked, so git could not have shown this; the offsets
  are what made it checkable.
* **The off path is genuinely inert.** The whole block, the `FindParamMember`
  lookup included, sits inside `if (options.noTeamType)` at
  `EnemyRandomizer.cpp:1075`, and `ApplyOneTeamType`'s signature takes no
  `std::mt19937` — so P2's invariant is enforced by the type, not by a comment.

### The drops_verify.py correction, checked rather than accepted

This was the change most likely to be declared and not delivered, so it was read
line by line. **It is genuine.** D-I2 now compares `vrow[:44]` and
`vrow[48:]` — the second slice runs to the end of the 388-byte row, where before
it was `[vat+48 : vat+48]`, **empty**. Bytes 52–387 went from compared-by-nothing
to fully compared.

The `--team-type` flag is better than the tolerance the plan asked for: it does
not ignore byte 303, it **asserts the row holds 25**, so a declared run still
fails if the value is wrong. Its selftest cases are pinned in both directions and
include the two that matter — a **partial** rewrite is rejected even with the flag,
and a stray write elsewhere in the row is still rejected with the flag on. So the
flag grants precise tolerance rather than blanket tolerance.

D-S1's own comment records that both of its cases — a stray byte at offset 100 and
one at offset 303 — **passed before the correction**.

### Awaiting hardware, and what the result means

Ready for hardware testing per `CLAUDE.md` §3, not done. Three `verify` modes
cannot run here — `team_type_verify.py verify`, `verify --off`, and
`drops_verify.py verify --team-type` — because all three need an output tree only
the console produces, and `data/runs/` holds no post-feature run.

**The test's shape is unusual and worth restating: "nothing visibly changed" is a
result, not a failure.** 378 rows ship with 25 already and none of them is known
to be hostile to anything. If nothing changes in play, the thing that needs
revising is the label the developer chose against the spec's recommendation
(spec D3), not the code — and the config key `no_team_type` was named so that
such a revision touches no saved file.

The §6 procedure's own first-priority item is the **Hunter's Dream intact** check,
because D2 took no protection list and a broken Dream is the run-ending failure
case. §6 item 7 is the `EASY SHADOWS` combination, where a failure is the finding
plan §9 D1 declined to pre-empt: it would be the first evidence this field decides
hostility at all.

---

## 2026-09-28 — hardware: no effect either way. The audit that followed, and the rename.

**What was tested.** The setting on, nothing else on: the log read
`SET 31398 CREATURE RECORDS TO ONE ALLEGIANCE`, the **Hunter's Dream was
completely intact** — so D2's decision to ship no protection list is vindicated —
and the developer saw "not a whole lot of enemies fighting," with the few
candidate cases indistinguishable from large creatures bumping into each other
and into fires. Then a run with it **off**: no significant difference either.

**The orchestrating session's first reading was wrong in emphasis.** It was that
25 happens not to be a hostility value. True but incidental. The developer
proposed the better one — *"maybe we're setting everyone on the same team… if we
want them to fight they need different values"* — and the audit that followed
confirmed it and went further.

### The audit

* **`teamType` has 13 distinct values across 31,398 rows, and 95.8% are 23.**
  Restricted to the **553** rows that are actually placed in a map anyone loads:
  **451 — 81.6% — are already 23.** So this pass replaces one shared value with
  another shared value. **Uniform to uniform cannot change anything**, whatever
  25 means. That is the real reason both runs were null, and it is a stronger
  account than the first one.
* **Only 553 of the 31,398 rows written correspond to a creature a player meets.**
  The other 98% are chalice variants, scaled variants and cut content.
* **The values look like archetypes describing the relationship to the *player*,
  not faction ids.** The find that shows it: **`Gehrman in Chair` is teamType 26
  and `Gehrman (Boss)` is 25** — the same character, passive then hostile. 26 also
  holds the Plain Doll, the Messengers, Master Willem, Lady Maria and the
  Celestial Larvae, all of which sit inert until engaged; 23 holds 105 ordinary
  enemy kinds; 25 holds Gehrman, the Blood-Starved Beast, the Maneater Boar, the
  Mensis Brain and the Witch of Hemwick.
* **The developer's field reports were checked one by one.** *Shadows hostile to
  pigs* — **supported**: Shadow of Yharnam is 23 and Maneater Boar is 25, genuinely
  different teams. *Humanoids fighting beasts in randomized runs* — **explained**
  below. *Everyone hostile to Beast Possessed Soul* — **not supported**: it is 23,
  the same as almost everything, so whatever causes that is not this field, and
  the claim was set aside rather than folded in.
* **The mechanism, and it is the opposite of what the row claimed.** The enemy
  pool spans **six** teams — 20, 23, 24, 25, 26, 27 — because each pool entry
  carries its `(npc, think, model)` triple and the team travels with the npc id.
  Shuffling therefore stands rival teams side by side and **infighting emerges**;
  forcing one team **removes** it. Measured cross-team adjacency for two
  neighbouring pool draws: **12.1% with the setting off, 0% on.** 12% is exactly
  the rate that reads as "I saw a few."

### The find that invalidated D3's reasoning

```xml
<CheckBox x:Name="AllDmgAll" Content="No Team Type" ... />   MainWindow.xaml:207
```

`AllDmgAll` is a stale **variable** name; `Content="No Team Type"` is what the
reference author actually put on screen. Spec D3 chose
`ENEMIES HOSTILE TO EACH OTHER` partly on the grounds that mutual hostility was
the author's evident intent — and that inference rested on the identifier, not on
the UI. **The backlog row's own name was the reference's real label the whole
time.**

### The test design was the orchestrating session's error, and it is worth recording

The hardware instructions said to run the setting **alone, with nothing else on**.
With `RANDOMIZE ENEMIES` off, vanilla is already 81.6% one team, so there was no
infighting to suppress and nothing the setting could possibly do. **Both runs were
null because the experiment could not see the effect, not because the effect is
absent.** The code was correct throughout.

The test that would show it, and which is still outstanding:
**`RANDOMIZE ENEMIES` on for both runs, the same seed, toggling only this
setting** — identical creatures in identical places, so the team byte is the only
variable.

### What was decided and changed

**Spec §10 D4 supersedes D3: the label is now `ENEMIES ON SAME TEAM`.** The old
label did not merely assert something unproven — it asserted the **opposite** of
what the setting does. The rename cost nothing because D3 had already kept the key
`no_team_type` named for the mechanism against exactly this contingency: no saved
file, no identifier and no byte changed.

**A three-state enum was considered and declined by the developer.** It would have
been *retain team* / *one team* / *randomize team*, and the randomize state
measures at ~80% cross-team adjacency over a curated set. Declined because it
needs a new multi-state `SettingKind` (today's are all two-state or pickers), it
would consume randomness where the other two states consume none — shifting every
downstream roll for a fixed seed — and the value space is unsafe to draw from
freely: 26 is the passive team and 20/21 are Player teams, so a naive randomize
would leave enemies standing inert or on the player's side. Recorded in D4 rather
than dropped silently, because it is the obvious next idea and the reasons not to
take it are not obvious.

**Files touched — strings only, no behaviour.** `SettingsModel.cpp` (label and a
rewritten help text that now states what off and on actually do),
`settings_ui_verify.py`'s `PROSE_TO_LABEL`, the three C++ header comments,
`drops_verify.py`'s header, the settings-UI spec's Appendix A row, the backlog
row 27 description — **which was wrong in the same way and is corrected** — and
`README.md`. Plan §1–§7 was deliberately **left saying the old label**: it is the
contract as approved, and the change is recorded in plan §10 where a
post-approval change belongs.

**Re-verified after the change:** `make` clean with **zero errors or warnings**,
`.pkg` rebuilt; `settings_ui_verify.py` **113/113** now reading
`World 2 ENABLE MERGO DARKNESS, ENEMIES ON SAME TEAM`;
`team_type_verify.py selftest` 20/20; `drops_verify.py selftest` 12/12;
`pool_verify.py selftest` 96/96; `worlds_verify.py all` 97/97;
`ui_scroll_verify.py` PASSED; `hunter_tools_verify.py selftest` 25/25.

**The label got cheaper as well as truer:** 392 px against 558, and the pane row
485 px against 651. The app's widest row goes back to
`START WITH A TRICK WEAPON` at 649 px, which `settings_ui_verify.py` now reports.
