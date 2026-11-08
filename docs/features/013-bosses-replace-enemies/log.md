# Feature 013 — log

Append-only. Every stage adds; none rewrites. `spec.md` states what is true now;
this states how it got there.

---

## 2026-09-28 — stage B, the spec

Written by the `spec-author` subagent, then **revised twice** — the only spec in
this repo so far to need that, because the developer added three functional
requirements after the first draft and then reshaped the central mechanism after
the second.

Row 13 was chosen as the next unspecced item after rows 11 and 27. It covers
**backlog row 17** as well, under D-A: the investigation established the fourteen
per-area flags gate `InsertBossesVoid` and nothing else in the reference tool, so
they are this feature's own gate rather than a separable row. Row 17's backlog
description — "chooses which zones' bosses take part" — is wrong; they have no
effect on boss randomization.

**The feature grew from one checkbox to eight settings.** The reference has a
checkbox and a percentage slider. The port ships a master toggle, a two-state
count mode, a count, a temporary replace-all override, a 14-row areas picker, a
22-row inserted-bosses picker, a spacing toggle and an AI choice. Three of those
came from the developer during the revisions: the count with its up-to/exactly
distinction, the inclusion list, and the spacing setting.

**Fifteen decisions, D-A to D-O.** Two were superseded in place rather than
deleted (D-E by D-K, the maximum count 10 to 15). The ones worth reading:

* **D-D — a count, not a percentage.** The reference's slider has no field
  initializer, so its shipped default fires on ~1% of placements by arithmetic
  accident: the roll is `<=` against an inclusive `Next(0, 101)`, so 0.00 still
  converts one placement in 101. The deviation replaces the *control*, not the
  shipped experience — the count's default of one per map gives 14 per run, within
  two of the reference's accident.
* **D-K — the maximum is 15**, because that is the last count at which spreading
  still keeps every map's closest pair above 20 paces (20 falls to 12, 25 to 10).
* **D-I — growing `char buf[1024]` to 2,048 is a prerequisite, not a tidy-up.**
  Measured during the second revision: eight settings cost 241 bytes, taking the
  block to 969, and with features 011 and 027 also landing to **1,031 — seven
  bytes over**, which `snprintf` would truncate silently. All three features share
  the buffer.

**Four factual corrections the investigation made to project documents.** Row
13's own description says the feature "lets boss identities into the ordinary
enemy pool"; the reference trace shows `insertBossesString` is a separate list and
the enemy pool is never given a boss entry. `docs/plans/pickers.md` §7.5 and
`docs/features/032-bypassed-enemies/spec.md` §8 both repeat that error.
`docs/windows-randomizer-technical-review.md` §5.4–§5.5 is the only project
document that had it right.

**A claim the spec-author made that was checked rather than accepted:** that
`SettingKind::SaveChoice` could not be reused for the two named two-state
settings. Both blockers verified — `SettingsModel.cpp:220` hardcodes
`"START FRESH"`/`"KEEP EXISTING"` as the only text a `SaveChoice` can render, and
`settings_ui_verify.py:893` pins *exactly one* `SaveChoice` entry, `SAVE DATA`.
So the four new setting kinds are real; the count went 7 to 11, not 7 to 8, and
the second revision had not reported that.

### The two questions put to the developer, and what happened

**Q6 — the crash warning, became D-O, taken as recommended.** The reference pops a
modal on this checkbox and no other. The port has no modal for any setting and
will not grow one: the warning becomes help text on **three** rows rather than
one, because this feature now carries three distinct risks where the reference had
one — instability, the replace-all test control, and `BOSS BEHAVIOUR`'s second
state possibly not working at all. Flagged for revisit after hardware, and flagged
under `docs/known-traps.md` so a softened warning does not harden into a label.

**Q10 — what UP TO means. Answered against both options offered, and it reshaped
the mechanism.** The question offered "random 0..N" (recommended) or "N capped by
supply, and then delete the setting", the latter because at max 15 against the
smallest map's 54 targets no map can ever run out, so the two states would have
been byte-identical in every reachable configuration.

The developer chose neither: **bosses join the draw, with the count as the
control** — *"randomly choose from the pool of enemies and bosses to replace each
enemy and keep a count ... exclude bosses from the randomization pool for normal
enemies once we reach the cap."*

That answer had two consequences the spec did not account for, and **both were
measured before anything was recorded**:

1. **The naive form of pool-mixing makes the boss rate depend on how many bosses
   are ticked, not on the count.** The ordinary enemy pool is **333 entries across
   82 models** (`enemy_lookup.py`'s `engine_pool`, which replays the contribution
   loop line for line). One entry per ticked boss gives an **8.01%** boss share
   with all 29 ticked but **0.30%** with one — 4.3 expected bosses in the smallest
   map against **0.16**. A ceiling cannot repair that, because a ceiling only ever
   removes bosses. Narrowing `ENEMIES INCLUDED` distorts it the other way, a
   10-model pool making 74% of draws bosses, though the ceiling does contain that
   direction.
2. **Spacing became impossible.** D-F had already specified greedy farthest-point
   selection over placement positions, which needs to know which placements get
   bosses up front — and a sequential per-placement draw cannot provide that.

Both went back to the developer, and both answers again went past the options:

* **On the rate, became D-M.** *"No rate setting. Our up to or exactly setting
  indicates how many bosses can (up to) or will (exactly) exist PER MAP. If the
  number is 5 and only Amygdala is selected then we get either up to 5 Amygdalas
  or exactly 5 Amygdalas."* That sentence is only true of both states under one
  reading — the per-placement probability is **derived from the count and the
  map's eligible-target count**, never from pool composition. So ticking fewer
  bosses changes **which** bosses arrive and never **how many**, and §8 criterion
  4 now measures exactly that: the single-boss mean must equal the all-29 mean.
* **On spacing, became D-N.** *"What if we take two passes, first replace all
  enemies in the map, then a second pass that randomizes the boss
  replacements."* This dissolves the problem rather than working around it: pass 2
  holds the whole map's eligible placements before choosing any, so **D-F stands
  unchanged**. It also restores the reference's own pass ordering —
  `InsertBossesVoid` runs after enemy randomization — so the port's structure now
  *matches* the reference here instead of departing from it, and §6 already said
  "after the enemy and boss passes".

**The net effect of the developer's three answers is a smaller deviation from the
reference than the spec had drafted, not a larger one.** §7 item 2 was rewritten
to say so: under D-L, `UP TO` *is* a per-placement draw, the same shape as the
reference's coin flip, with the probability calibrated from a count and bounded by
a ceiling instead of taken from an uninitialized slider with no bound at all.
`EXACTLY` is the only genuine departure of the two.

### What step 7 reconciled

* §8 criterion 4 rewritten — it had said "not every map is at N", which a pass
  that does nothing would also satisfy. It now carries the ceiling, the
  not-every-map property, **and** the single-boss mean, which is the case that
  fails if the probability comes from pool composition.
* §8 criterion 12 — notes that D-N's second pass is what makes up-front selection
  available.
* §4 — gained the 333-entry measurement and the ticked-bosses rate table, since
  D-M rests on them.
* §7 item 2 — the deviation narrowed, per above. §7 item 4 — records that the
  spacing guarantee is *reachable* only because of the second pass.
* §3 item 1 — notes that D-N means the port does not mix bosses into the enemy
  pool either, so the port follows the reference here and row 13's own description
  ships as neither.
* §2 — row 1 gained D-O's warning; row 2 restated as a ceiling the map
  approaches, with the note that the selection governs variety and not number.
* §9 deleted entire, matching specs 011, 027 and 037.

Status ended at `QUESTIONS ANSWERED — awaiting developer approval`.

Status set to **APPROVED** by the developer the same day.

---

## 2026-09-28 — stage C, the plan

Written by the `planner` subagent against the approved spec.

**Five milestones, continuous through M1–M4, one required gate before M5.** M1 the
four new setting kinds and both generated tables; M2 all eight settings, their
persistence and the buffer growth; M3 the insertion pass; M4
`insert_bosses_verify.py`; M5 the removal of **REPLACE ALL ENEMIES**.

**Both gates were questioned rather than accepted.** The M4 gate is genuinely
*required* and unusually well-founded: M5's entire content is deleting the setting
the hardware run exists to exercise, so building M5 before the run would destroy
the test rather than merely front-run it. The M2 gate is correctly classed
Optional — the Bosses pane goes from 2 rows to 10 and becomes the first settings
pane in the app that must scroll, but `ui_scroll_verify.py` and
`settings_ui_verify.py` check that arithmetically and nothing in M3 depends on it.

**Three questions, all non-blocking, all taking the planner's recommendation** —
D1–D3 in plan §9:

* **D1** — keep `BOSS BEHAVIOUR` rather than `BOSS AI`. Measured at 663 px of the
  pane's 700 px budget with its widest value `REPLACED ENEMY'S AI`, making it the
  app's widest row with 37 px spare. `BOSS AI` would bank 157 px but is confusable
  with the four `EASY …` rows, which are also about boss behaviour and are not
  this setting.
* **D2** — the config keys are `inserted_boss_areas` and
  `inserted_bosses_included`. 248 bytes against the spec §4 estimate's 241; §5's
  976-byte block is measured from 248, and D-I's conclusion holds either way.
* **D3** — the area picker shows no map prefix (`showRowId = false`).

**Reconciliation found the body already written to all three answers** — B19 for
the label, lines 211 and 245 for `showRowId`, and §5's arithmetic derived from the
248-byte keys. **One genuine edit was needed:** the two key names appeared *only*
inside §8's question, so emptying §8 would have removed them from the contract
entirely. They are now in §5's `RandomizerDefaultsStore.cpp` row, where the
implementer will look for them. Every surviving `§8` reference in the body was
checked and points at the **spec's** §8, which exists.

### The planner found a factual error in the approved spec, and it was verified independently

**The insertion list is 32 identities across 22 models, not 29.** The three extra
are all Witch of Hemwick: `210005*210000` and `210005*210002` in `m28`, and
`210030*210030` in `m34`.

Confirmed from both directions by the orchestrating session rather than taken from
the report. Structurally: `insertBossesEnemy.Add(...)` is at
`RandomizeFunctions.cs:1657` and the two de-eligibility tests
(`m34 && npc == 210030`, `m28 && c2100`) are at `:1669` and `:1677` — *after* it,
and they set `addEnemy`, which gates a different list further down. From the data:
an independent harvest reproduces **33 identities before Moon Presence removal and
32 after**, matching exactly. The spec's figure is what you get by treating the
harvest as governed by the final value of `addEnemy`.

**Two errors the orchestrating session made while checking, recorded because
either would have made the planner look wrong:**

1. Moon Presence was filtered as model `c5120`. **`c5120` is Amygdala**; Moon
   Presence is `c5400`. `BossList.h`'s `c5120_0001` is Amygdala's *placement*
   name, not Moon Presence's model.
2. The `think == 1` drop was missed, giving 34. It **is** the reference's own rule,
   at `:1841`, written `ThinkParamID.ToString() != "1"` — a string comparison,
   which is why a numeric grep for `ThinkParamID == 1` finds nothing.

**Why it is safe to plan against:** the **model** set is unchanged at 22, so the
22-row picker, D-B, D-G's drain-by-identity and D-M's whole argument stand. What
moves is verification constants and two derived percentages — Living Failure is
12.5% of arrivals rather than 13.8%, Witch of Hemwick 15.6%. Spec §4's "24 of 29
tracked by the scaling table" reads **25 of 32 tracked, seven untracked**. The plan
reads 32 throughout and §7's stop conditions halt the implementer if a build
measures anything else. **Spec §4, §5 and §8 criteria 9–10 need correcting at the
documentation stage.**

**The planner also independently concluded `tempList.Contains` is dead code** —
`MSBB.Part.Enemy` has no value equality and `tempList` only ever holds objects
from the same map, each added once — which the orchestrating session had reached
separately. It is omitted from the port's `InsertionEligible()` rather than
reproduced.

**One wording ambiguity the planner decided rather than asked**, because it
resolves from the spec's own text: §8 criterion 11 says `REPLACED ENEMY'S AI`
makes the written `ThinkParamID` equal the placement's **vanilla** value, which is
only exact with `RANDOMIZE ENEMIES` off. §2 row 8 and §3 point 5 both say "the
creature it replaced", and the reference expresses it by *not writing the field at
all* — so the plan implements "leave `ThinkParamID` untouched" (P4) and the
verifier asserts the vanilla equality only in the configuration where it is exact.

**Spec figures the planner reproduced and confirmed:** 1,381 / 2,269 eligible
targets and every per-area and creature-kind count; the bounding diagonals; Old
Yharnam's five kinds; 26 caged-dog placements all eligible; zero boss placements
eligible; 333 pool entries across 82 models; the whole spacing table including the
20/12/10 figures D-K rests on; all 22 insertion models present in the merged model
union; position offset `+0x28`; 1,367 distinct coordinates for 1,381 targets; both
`SaveChoice` reuse blockers.

**Four risks recorded with the strategy**, none of which the plan closes:

1. **`REPLACED ENEMY'S AI` may simply not work**, and nothing in the plan reduces
   that — it is an inference from what a think id is for, not a measurement.
   Contained only by OWN AI being the default, D-O's warning, and testing it third
   and in isolation. A crash there is a plausible, non-defective outcome.
2. **A stray draw on the off path silently changes every existing seed.** Up to 178
   draws per map in `UP TO` mode share the run's single `mt19937`. Guarded by the
   master-toggle test as the pass's first statement plus M3's byte-identical tree
   diff against the pre-M3 build. This is the one invariant whose breakage stays
   invisible until a player re-uses a seed.
3. **Three approved features now share a buffer one of them must grow.** 013 + 011
   + 027 is 1,038 bytes against `char buf[1024]` — 14 over, truncated in silence.
   The plan carries arithmetic for all four landing orders.
4. **Spacing is spatial, not traversal-aware.** Two placements 30 paces apart can
   be on different storeys of the Research Hall, whose 84 targets fit a 184-pace
   diagonal — the tightest of the fourteen and the worst case at every count.

**Contract budget.** §1–§7 came to **477 lines** against a nominal 400. The
planner compressed three times and stopped; the residue is 24 traced behaviour
rows, a 30-row file table, five milestones of ordered steps and eight verification
commands, and the measurements and per-key byte figures are already in
`plan-evidence.md` (808 lines). Accepted rather than trimmed, and it sits mid-range
against this repo's own plans — caged dogs 419, startup screen 404, trick weapon
487, worlds 553, randomizer settings UI 605. The planner's own note is that the
honest lever would be splitting M3 from M2, not cutting steps.

Status ended at `QUESTIONS ANSWERED — awaiting developer approval`.

---

## 2026-09-28 — the spec corrected to 32, and the M2 gate declined

Two developer decisions after the Execution Strategy was presented.

**1. The approved spec was corrected from 29 identities to 32**, rather than
leaving it to the documentation stage. Twelve sites, all in `spec.md`, all
measured independently before editing:

| What it said | What it says now |
| --- | --- |
| 30 identities / 23 models before Moon Presence | **33 / 23** |
| 29 identities / 22 models after | **32 / 22** |
| Living Failure 4, Shadow 3, Witch 2, Ludwig 2, eighteen 1 each | **Witch 5**, Living Failure 4, Shadow 3, Ludwig 2, eighteen 1 each |
| Living Failure 13.8%, single-variant 3.4% | **Witch 15.6%, Living Failure 12.5%, single-variant 3.1%** |
| 24 of 29 tracked by the scaling table, five untracked | **25 of 32 tracked, seven untracked** |
| the untracked: Witch, small Celestial Emissary, 3 of 4 Living Failure | **three of the five Witch identities**, small Celestial Emissary, 3 of 4 Living Failure |
| rate table's top row, 29 bosses → 8.01% / 4.3 / 111 | **32 bosses → 8.77% / 4.7 / 121** |
| `ENEMIES INCLUDED` direction: 100 models 22.5%, 10 models 74.4% | **100 entries 24.2%, 10 entries 76.2%** |
| D-M's own figures, 8.01% and 4.3 | **8.77% and 4.7** |

The §4 headline fact carries an inline note saying it was corrected during stage
C and why, so the change is visible in the document rather than only here. **D-M's
dated decision row was corrected in place** — it states a measurement rather than
a preference, and leaving a figure stage C would read as authoritative was worse
than editing a decision record. Its *argument* is untouched: what carries D-M is
the ratio between all-ticked and one-ticked, and 8.77 : 0.30 makes the same case
8.01 : 0.30 did.

**The scaling-coverage figure was the one worth re-deriving rather than accepting.**
An initial check against `BossParamScaling.h` returned 0 of 32 tracked, which was
the orchestrating session's own error — that header holds the sixteen per-map scale
constants, and the ~1,170 tracked base identities are in `NpcScalingTable.h`.
Against the right table it is **25 of 32, seven untracked**, exactly the planner's
figure, and the seven are three Witch of Hemwick, three Living Failure and the
Small Celestial Emissary.

**2. The Optional gate after M2 was declined; M1–M4 run continuously.** Recorded
as plan §9 D4 and written into the Execution Strategy table. The two UI verifiers
cover the ten-row scrolling Bosses pane arithmetically and nothing in M3 depends
on a human looking at it. **The Required gate before M5 stands**, for the reason
that made it required rather than merely prudent: M5's entire content is deleting
the setting the hardware run exists to exercise.

The plan needed no other edit — it already read 32 throughout, which is what made
correcting the spec a documentation change rather than a re-plan.

---

## 2026-09-29 \u2014 plan approved

The developer approved the plan and the Execution Strategy. Stage D (plan review)
was skipped, as for rows 11, 27 and 37.

**The byte arithmetic needs rebasing, and this is the third feature to hit it.**
�5 was measured against a 728-byte settings block; rows **27** (+15) and **11**
(+47) have both landed and been hardware-confirmed since, so the real baseline is
**790 / 841**. This plan adds +248, giving **1038 / 1089** \u2014 both over
`char buf[1024]` by 14 and 65 bytes, which **confirms D-I**: the growth to 2,048 is
a prerequisite, not a tidy-up, and it is this feature that forces it. The
implementer was given the measured figures and told to derive from them rather than
from what the code emits.

---

## 2026-09-29 — stage E, milestones 1–4. Stopped at the Required gate.

Built by the `implementer` subagent. **Stage D was skipped**, as for rows 11, 27
and 37. **M1–M4 completed, no stop condition fired**, and the run ended on the
**M4/M5 Required gate** — which is where the Execution Strategy puts it.
**`REPLACE ALL ENEMIES` is untouched and ships live**, documented in the shape
`RuneProbe.h` used, because only a hardware run can retire it.

**What was built.** M1 the four new setting kinds and both generated tables
(22 insertion rows summing to 32 identities, 14 area rows summing to 1,381
targets). M2 all eight settings with persistence and the 2,048-byte buffer. M3 the
insertion pass — `BossInsertion.{h,cpp}`, the run pool, the harvest,
`part_fields::GetPosition` read-only at `+0x28`. M4 `insert_bosses_verify.py`.

### The byte arithmetic, and a real hole it exposed

Rebased as instructed: baseline measured at **790 / 841** before any edit, plus
this plan's **+248**, giving **1,038 / 1,089**, both now exact equalities in
`pool_verify.py`, with `char buf[1024]` grown to **2,048**. Reproduced
independently by the orchestrating session. Third consecutive feature to need this
rebase; the first where the overflow is real rather than forecast.

**And the check that was supposed to catch the overflow did not work.**
`pool_verify.py`'s fit assertion was `settings_block < max(bufs)`, where `bufs` is
every `char buf[N]` in `RandomizerDefaultsStore.cpp`. There are two:
`FormatSettings`' own **2,048** (line 45, grown by this feature) and
`LoadRandomizerDefaults`' **4,096** *read* buffer (line 286). `max()` therefore
picked the read buffer, so the assertion compared the write block against 4,096 —
**a 1,038-byte block would have "fitted" `char buf[1024]` silently**, which is
precisely the `snprintf` truncation D-I exists to prevent. It now parses
`FormatSettings`' own `char buf[N];` — with the semicolon, so the comment's prose
figures are not read as code.

**This is the second shipped verifier in this feature family found to have a hole
in the exact dimension it guarded**, after `drops_verify.py`'s D-I2 empty slice
(row 27). Both were found by an agent implementing against the tool rather than by
reading it. Verified independently here: the two buffer sizes are as stated and
`1038 < 4096` is True.

### The substantive finding: UP TO's ceiling truncates the mean

`UP TO`'s rule is a per-placement draw at probability `min(N, supply)/supply`,
bounded by `min(N, supply)`, so its expectation is
`E[min(Binomial(supply, cap/supply), cap)]` — strictly **below** N, because the
ceiling clips the upper tail while nothing lifts the lower one. **The implementation
is §4 step 3 exactly; the gap belongs to the rule, not the code.** The verifier
therefore asserts against the rule's own analytic value rather than a tolerance
around N — about 20× tighter, and still failing the D-M violation it has to catch.

Computed independently over the fourteen real supplies:

| setting | UP TO, full run | EXACTLY, full run |
| --- | ---: | ---: |
| count 1 (**default**) | **8.9** | 14 |
| count 5 | 58.1 | 70 |
| count 15 | 190.4 | 210 |

**Consequence, and it was a user-facing documentation defect.** Spec §2 row 3 said
"At the default that is 14 bosses in a full run" as a statement of fact. 14 is
`EXACTLY 1`; the default *mode* is `UP TO`, so the shipped default places about
**nine**. Row 2 already describes `UP TO` as a ceiling some maps do not reach, so
nothing contradicted — but row 3 read as a promise. **§2 row 3 corrected in place**
on 2026-09-29, giving both modes' figures and citing the measurement.

### Twelve deviations, all in plan §10

The three worth reading: the rebase; the `max(bufs)` fix; and **M1 and M2 were
built as one unit**, because M1 step 5's switch statements cannot compile without
M2 step 1's fields and the two new selections break `settings_ui_verify.py` until
M2 step 6. No gate sat between them, so nothing was skipped — but the boundary was
not real, and D4 had already declined the Optional gate there.

Also decided, and recorded: no boss-name test in the collector, the fixed exclusion
list already sparing all 62 (measured); all `supply` draws taken in `UP TO` even
after the ceiling fills, which is what keeps the stream position independent of how
early the ceiling is reached; and two silent-skip log lines rather than one.

### Verification, re-run independently by the orchestrating session

`make clean && make` from scratch, **zero errors or warnings**, 7,143,424-byte
`.pkg`; `insert_bosses_verify.py selftest` **56/56**; `pool_verify.py selftest`
**108/108** including 1,089 / 1,038 / 2,048; `settings_ui_verify.py` **116/116**
with the **Bosses pane at 10 rows** carrying all eight new settings and the widest
pane row at **663/700 px** — exactly D1's predicted figure for `BOSS BEHAVIOUR`;
`boss_verify.py selftest` 5/5; `caged_dogs_verify.py` 23/23;
`easy_modes_verify.py` 25/25; `team_type_verify.py` 20/20;
`starting_weapons_verify.py` 45/45; `trick_weapons_verify.py` 56/56;
`ui_scroll_verify.py` PASSED.

**Also confirmed independently:** `plan.md` §1–§7 is byte-identical — the only
diff hunks are line 3 (the orchestrating session's own APPROVED edit) and line 552
onward, inside §10 which starts at 548. Every changed file is named in plan §5;
`Msbb.cpp` and `BossInsertion.cpp` appeared to be unnamed only because §5 writes
them in the `` `X.h` / `.cpp` `` form. **`REPLACE ALL ENEMIES` is still present** in
`SettingsModel.cpp` and its key in both halves of the store, which is what says M5
was correctly not begun.

**Not verifiable here:** M4 step 4's `verify` against real output trees, and §7
M3's feature-off tree diff — both need PS4 runs. In their place the implementer
exercised `verify` end-to-end on the vanilla tree as its own output plus **eight
injection cases** that corrupt a known-good tree and require the right criterion to
fire, and asserted the off-path invariant by reading `BossInsertion.cpp` to confirm
every gate precedes the first `RandInt`. That last one is the risk-2 invariant from
plan 013's own risk list, and it is currently carried by a static argument rather
than by the byte-identical tree diff §7 M3 specifies.

### The gate

Four runs, in order, each tree worth copying into `data/runs/` for
`insert_bosses_verify.py verify`: **(1)** defaults — does a loose boss aggro, fight
and die with no fog gate, and does a save in an affected area reload; **(2)** count
15, spacing off then on; **(3)** `REPLACED ENEMY'S AI` alone, the most likely to
fail, where a crash is a plausible non-defective outcome; **(4)**
`REPLACE ALL ENEMIES`, last and once — **its outcome recorded here is what unblocks
M5.**

### Noticed, not fixed

`docs/plans/pickers.md` §7.5 and `032/spec.md` §8 still claim this feature puts
bosses into the ordinary enemy pool. It does not, and spec §3 item 1 and §10 D-N
both say so — these two are the last places the original error survives, and §3.2
scopes them out.
