# Feature 032 — Bypassed Enemies (`ENEMIES SKIPPED`) — log

Append-only. Every stage adds an entry; none rewrites an existing one.

---

## 2026-09-17 — Stage B, spec written (`/spec 32`)

`spec.md` created from backlog row 32. Three developer decisions recorded as
§10 D1–D3: retire `UNCHANGED BELL MAIDENS` without migrating its saved value,
name the setting `ENEMIES SKIPPED`, and leave `c2561` as the doubled-ID row.

Ended at `QUESTIONS ANSWERED — awaiting developer approval`.

---

## 2026-09-18 — Stage B, spec refined (`/refine-spec 32`)

**Feedback that prompted it** (developer, verbatim):

> two things I want to make sure, (1) i'd like a clear instruction on the enemy
> selection screen that states what this setting does (something to the effect
> of, select enemies that will not be randomized) and (2) i want to make sure we
> don't have that error that we had earlier with the chime maidens. So in the
> case of enemies selected were the two chime maidens and then "unchanged chime
> maidens" or in the case of feature 32, we select the chime maidens to be
> ignored by the randomizer, I don't want this to error. Instead, I want the
> existing chime maidens in the vanilla maps to be left alone and i want the
> rest of the enemies in the game to be randomized by the selection.
>
> Does this make sense? If not ask any questions you have. If so please update
> the spec accordingly

**Classification: no previously-taken decision was changed.** D1, D2 and D3 are
untouched. The commit-time refusal for a starved selection, which this feedback
reverses, was the spec's own drafted position and had never been put to the
developer, so no approval contract was broken. The spec was
`QUESTIONS ANSWERED — awaiting developer approval` and not `APPROVED` when the
refinement began, so there was nothing to re-approve and nothing downstream:
**no artifact was marked `STALE`** — there is no `plan.md` for this feature.

**What changed.** An on-screen instruction line reading
`SELECT ENEMIES THAT WILL NOT BE RANDOMIZED` is now required on the picker, with
the layout cost measured (§4, F13). The starved-selection refusal is replaced
throughout — §2, §4 F10, §6, §7, §8 — by defined non-failing behaviour.

**Two new questions were raised and answered**, recorded as §10 D4 and D5:

* **D4** — a starved selection does not error; `ENEMIES INCLUDED` wins the draw
  for that run while the skipped placements stay frozen. Chosen over the spec's
  recommendation of matching the reference's "randomize nothing". **This puts one
  deliberate exception on the feature's headline promise**, now stated in §1, §2
  and §8 assertion 7: in that one configuration a skipped creature does appear
  somewhere new.
* **D5** — the existing empty-list refusals for `ENEMIES INCLUDED` and
  `BOSSES INCLUDED` stay exactly as they are; the resulting asymmetry is
  accepted.

**Carried in from row 16 and now binding here:** an unusable vanilla source must
still fail and must be distinguishable from a starved selection (§7, §8
assertion 8). This is `016-unchanged-bell-maidens/plan-review.md` blocking
finding 2.1, and removing the error without it would report a bad installation
as a successful run.

Ended at `QUESTIONS ANSWERED — awaiting developer approval`.

---

## 2026-09-18 — Stage B, spec approved

The developer read the refined spec and approved it. Status
`QUESTIONS ANSWERED — awaiting developer approval` → `APPROVED`.

This is human gate 0h. §10 holds five decisions (D1–D5) and §9 is gone.
Ready for stage C (`/plan 32`).

Any change from here that alters an approved decision drops the status back to
`QUESTIONS ANSWERED — awaiting developer approval` and requires re-approval; a
correction that alters none keeps `APPROVED` and is recorded as an amendment.

---

## 2026-09-18 — Stage C, plan written (`/plan 32`)

`plan.md` created against the approved spec. **Two milestones, in this order:**

1. **The starved run finishes, and a broken source still fails.** Two pools built
   during the read phase, a four-case decision in `StepBuildPool`, two distinct
   failure messages, two progress lines. No UI row moves.
2. **`ENEMIES SKIPPED`.** The generated 85-row table, the polarity parameter, the
   skip predicate at both engine call sites, the picker, both screens, the config
   key, D1's removal of `UNCHANGED BELL MAIDENS`, and four tool updates.

The order is a dependency, not a preference: the list makes starvation a
one-button action, so the fix lands before the hazard is widened — and milestone
1 is only independently testable while `UNCHANGED BELL MAIDENS` still exists.

**Eleven planning decisions (P1–P11)** were taken by the planner and **four
questions were answered by the developer (P12–P15):** two milestones in that
order; the flag column reads `SKIPPED` / `-`; the assertion-5 baseline tree is
captured during milestone 1's hardware test; and the broken-source failure keeps
today's `options.randomizeEnemies` gate.

**One question remains open** — §8 Q4, where the `ENEMIES SKIPPED` row sits on
the two screens. It blocks nothing and changes two compile-time constants.

**Three spec gaps were raised by stage C and are NOT resolved here**, because
they are questions about what the feature should do:

1. `c1130`'s row will read `C1130 LABYRINTH RITEKEEPER`, but spec §8 hardware
   step 2 tells the tester to tick "the Cathedral Ward chapel dweller".
   `Characters.json` and `docs/enemy-exclusion-history.md:118-122` disagree about
   what this creature is. D3 settled only `c2561`.
2. Spec §7's "a positional format means removing a line is not a free operation"
   is wrong about `defaults.cfg` — it is `key=value` parsed by `strtok` with
   unknown keys ignored, so only the selection *values* are positional. D1 is
   cheaper and safer than the spec assumed. No behaviour changes; the
   justification does.
3. Spec §8 assertion 3 cannot be an equality assertion — skipping shifts the roll
   stream, so only the *proportion* is comparable. The plan implements it as a
   rate check with a tolerance.

Plan status: `QUESTIONS OPEN`.

---

## 2026-09-19 — Stage D: plan reviewed

`plan-reviewer` reviewed `plan.md` (status `QUESTIONS ANSWERED`) against
`spec.md` and the repository. Review written to `plan-review.md`.

**Verdict: CHANGES REQUESTED.**

**Blocking findings:**

1. **Failure messages are budgeted by width only, not by the font.**
   `Font8x8.cpp` is uppercase-only with no `:` glyph, and `FindGlyph` returns
   `nullptr` for an unknown character, which `DrawChar8x8` renders as a blank
   advance. A 43-character lowercase message passes §6.2 case 6 and still shows
   the player a blank line, defeating spec §7 / §8-8 and §6.4 step 3's own
   "fully readable" criterion.
2. **Spec §8 assertion 4 has no check, and its baseline expires when milestone 1
   starts.** §6.5 step 9 compares tree `A` against tree `A` — both produced by
   the post-change build. §6.4 step 1's "must change nothing" names nothing to
   diff against. §6.4 step 4 captures a baseline for assertion 5 only.

**Verified independently by stage 2 (this pass):**

- Finding 1 — **confirmed** at the mechanism level. `Font8x8.cpp:7-8` declares
  uppercase-plus-space; the glyph table's only punctuation is `'`, `(`, `)`,
  `-`, `,`; `FindGlyph` (`:70-74`) returns `nullptr` → blank; `Fail()` messages
  reach the screen verbatim via `EnableWizardScreen.cpp:619`
  (`AddProgressLine` does not transform case), and the only `Upper()` call site
  is the mirror progress line at `EnemyRandomizer.cpp:945`. Several existing
  `Fail()` strings already carry `:` and lowercase.
- Finding 2 — **confirmed.** §6.5 defines `A` as a milestone-2 tree, so step 9
  is a post-change/post-change comparison. The pre-change baseline is not
  recoverable from git: all nine relevant sources under `app/src/Randomizer/`
  and `app/src/UI/` are uncommitted working-tree modifications.
- Also confirmed (non-blocking, reviewer's finding 4): §6.5 step 3 (`plan.md`
  line 762) tells the tester to tick `C1130 LABYRINTH RITEKEEPER`, contradicting
  §5.5 (line 564) and log entry P17 (line 856) and spec D6, which set the row to
  `C1130 OEDON CHAPEL DWELLER` — the exact defect D6 was raised to fix.

**Findings aimed at the SPEC, not the plan:**

- Spec §10 D4's claim that its exception is "the only case in which" a skipped
  creature appears somewhere new does not hold with `RANDOMIZE BOSSES` on:
  `c2090` and `c2710` appear in both `BossPoolTable.h` and the 85-row list, so a
  boss arena can become a skipped creature. Needs a developer decision — accept
  and document, or have `ENEMIES SKIPPED` also filter the boss pool for those two.
- Spec §8 assertion 4 needs a baseline tree captured *before* implementation
  starts, for the same reason P14 captures one for assertion 5. Unlike that one,
  it cannot be recovered from git.

**The reviewer could not check:** anything requiring the console (frozen-creature
behaviour, on-screen legibility, starved-run timing); whether any existing tree
under `data/runs/` could serve as finding 2's baseline (they are map-only, with
no recorded seed or settings); the §5.3 "ten to thirty seconds" estimate.

No artifact status was changed by this pass.

---

## 2026-09-19 — Stage C, plan refined (`/refine-plan 32`)

**Feedback that prompted it** (developer, verbatim):

> please address issues in 32's plan-review.md in 32's folder

The `planner` subagent was re-dispatched against `plan.md`, the approved
`spec.md` and `plan-review.md` (verdict CHANGES REQUESTED). It ran in two
passes: the review response, then the developer's answers to what that response
raised.

**Classification: no approved decision was altered.** No spec §10 decision
(D1–D6) was touched — D6 moved from being *mentioned* to being *implemented* in
§3.1/§4.2, and §6.5 step 3 now complies with it instead of contradicting it. No
developer-answered plan decision (P12–P17) was reversed. **The plan was not
`APPROVED` when the refinement began** — it stood at `QUESTIONS ANSWERED —
awaiting developer approval` — so there was no approval to survive and nothing
downstream to invalidate: **no artifact was marked `STALE`.** `plan-review.md`
is left exactly as written; it is the record this pass answers, not a document
this pass outdates.

**One detail change inside a developer-answered decision, flagged not buried.**
P14 named seed `1234567` for assertion 5's baseline. Step 0's tree now exists at
some other, not-yet-transcribed seed, so the plan substitutes `S` = the seed of
the `20260919-Enemies Only` tree throughout. P14's substance is untouched —
assertion 5's baseline is still captured during milestone 1's hardware test —
but the literal moved, to keep all five trees on one seed. Recorded in §6.4,
§9 P21 and §10.

**What changed, by review finding.** Findings 1–7 are answered; finding 8 was
about this log and is answered by this entry.

* **Finding 1 (blocking) — the font, not just the width.** §2.3 now measures the
  glyph table (42 characters, no lowercase, no `:`); §3.4 is rewritten around a
  character-set constraint; §6.2 case 6 asserts renderability against
  `gen_pool_table.RENDERABLE` rather than length alone; new case 25 covers the
  picker strings. The uppercasing question is settled as **P18**.
* **Finding 2 (blocking) — assertion 4 had no check and no baseline.** New
  §6.4 **step 0**; §6.4 step 1 and §6.5 step 9 now diff against it instead of
  against a post-change tree.
* **Finding 3** — §6.3 assertion 2 restated as "no *eligible* placement was
  *changed* to a skipped model", boss-written placements scoped out; new **§5.10**
  records the `c2090` / `c2710` boss-pool overlap and the six dual-sided models.
* **Finding 4** — §6.5 step 3 corrected to `C1130 OEDON CHAPEL DWELLER`; the
  other hardware steps swept for the same drift.
* **Finding 5** — the D6 name override moved into §3.1 and §4.2 as required
  work, with `c1130`'s absence from both pool tables measured; new case 26.
* **Finding 6** — all five `StepBuildPool` cases and the fallback progress line
  gated on `options.randomizeEnemies`; the surviving `:423` log line named.
* **Finding 7** — §3.6 and case 22 restated over the 553-value reachable set;
  the 151–351 structural figure recorded.
* **The three non-reproducing numbers** were re-measured rather than taken on
  trust. 70/98 and 162 + 73 = 235 confirmed against the review. The reviewer's
  option-A fix was **judged wrong**: 13 rows at band `{260,52,900}` would put
  `MORE ABOVE` at 214, above the instruction line's 220 bottom, failing
  `ui_scroll_verify.py`'s G3 — and spec §4 F13's option A moves the *heading*,
  not the list. The wrong cell was the band. Option B, the chosen layout, is
  unaffected. The reviewer's account of today's failure screen was also
  overstated: it reads `ATION FAILED` plus two stray glyphs, clipped at both
  ends, not a clean heading followed by blanks.

**Three new planning decisions, P18–P20**, and **P21** recording the developer's
answers: the uppercasing site and the `: ` → ` - ` prefix change (P18, put to
the developer and accepted); the baseline capture as required work (P19); what
the tree checker asserts (P20).

**Q6 was raised and answered the same day.** The developer captured a fresh
pre-implementation baseline at `data/runs/20260919-Enemies Only` — a full
`dvdroot_ps4` tree, 102 files, all `.dcx`. The planner re-derived its settings
from its contents against `data/vanilla/dvdroot_ps4` and every determinable one
reads as `RANDOMIZE ENEMIES` on with everything else default: exactly the 24
base maps differ and all 19 chalice maps are byte-identical; 1,974 of 2,269
eligible placements re-modelled at rates tracking each map's zone chance; zero
reference-excluded placements and zero non-enemy MSB parts changed; `param/`
byte-identical; the `common.emevd.dcx` darkness poke carrying the OFF pattern;
50 of 54 maidens re-modelled, so `UNCHANGED BELL MAIDENS` was off. **The seed is
recorded nowhere in the tree and must be transcribed into §6.4 step 0 from the
run's own `USING SEED <n>` line before milestone 1's first code change.** It is
a transcription, not a decision, so §8 is empty rather than carrying it.

**Spec findings raised by this pass and NOT resolved here** — the spec is not
the planner's to change:

1. **§10 D4's "only case" claim does not hold with `RANDOMIZE BOSSES` on.**
   `c2090` (Blood Starved Beast) and `c2710` (Father Gascoinge) are in both
   `BossPoolTable.h`'s 17 models and the 85-row list, so a boss arena can become
   a creature ticked in `ENEMIES SKIPPED`. §4 F8 covers only the placement half,
   correctly. **Needs a developer decision** — accept and document, or have
   `ENEMIES SKIPPED` filter the boss pool for those two. The plan is written for
   accept-and-document; §5.10 says what a `/refine-spec 32` would change.
2. **§2's headline promise and §8's "failure would look like" list inherit the
   same gap.**
3. **§4 F11's `478 → 556 (+103)` is `478 → 555 (+102)`**, measured from
   `RandomizerDefaultsStore.cpp:94-120`. No consequence.
4. Previously raised by stage C and still true: §7's "positional format"
   description of `defaults.cfg`, and §8 assertion 3's equality framing.

Plan status: `QUESTIONS OPEN` → `QUESTIONS ANSWERED — awaiting developer
approval`. §8 is an empty heading; §9 runs P1–P21.

---

## 2026-09-19 — Milestone 1 implemented (developer: "implement the plan for feature 32")

**Plan status when work began:** `QUESTIONS ANSWERED — awaiting developer
approval`. The developer's instruction to implement is the approval; recorded
here rather than by an agent setting `APPROVED` on its own behalf. **Milestone 1
only** was implemented, per plan §7 and `CLAUDE.md` §4 — a full stop before
milestone 2, awaiting the §6.4 hardware test and explicit approval.

**One precondition is still outstanding and was not satisfied by this pass.**
§6.4 step 0's `SEED = ____________` is still blank. The tree at
`data/runs/20260919-Enemies Only` carries no log, no `defaults.cfg` and no
`README.txt` — re-confirmed by listing it — so the seed exists nowhere on disk
and could not be transcribed here. **The first code change has now happened
anyway**, because the seed is knowable only from the run in front of the
developer and blocking on it would have delivered nothing. The consequence is
stated plainly: **until that number is written down, spec §8 assertion 4 cannot
be checked at all**, because step 1's diff has no reproducible seed to run at.
Nothing else in milestone 1 depends on it.

### What was built (plan §4.1, all rows)

| File | What landed |
|---|---|
| `EnemyRandomizer.h` | `EnemyRandomizerResult::poolFellBack` and `::poolSize`, with D4 named as the reason |
| `EnemyRandomizer.cpp` | `State::poolIgnoringSkips`; the contribution loop feeding both vectors with their own dedupe sets; `StepBuildPool`'s five-case decision; the `DrawCandidate` comment on why `RandInt(0, -1)` is unreachable; the two new `Fail` messages; the file header's new D4 bullet |
| `EnableWizardScreen.cpp` | The two `poolFellBack` lines and the `enemiesRandomized == 0` line, all gated on `randomizeEnemies_`; the failure prefix `: ` → ` - ` (P18). `StartCommit`'s D5 refusals untouched |
| `enemy_lookup.py` | `engine_pool(root, bell, included, maps)` |
| `pool_verify.py` | Ten new selftest cases covering §6.2's milestone-1 group |

### Three implementation choices the plan left open

1. **The messages are named constants, not inline literals** —
   `kFailNoMapsRead` / `kFailNoCandidates` in `EnemyRandomizer.cpp`, and
   `kEnemyFailPrefix` / `kPoolFellBackLine1` / `kPoolFellBackLine2` /
   `kNothingRandomizedLine` in `EnableWizardScreen.cpp`. §6.2 case 6 requires
   the strings to be *parsed out of the source* rather than retyped into the
   test, and a named definition is what makes that parse deterministic. It also
   puts the font constraint in a comment at the definition site, where the next
   person to edit the wording will see it.
2. **The per-map dedupe test moved below the c1110 / c2561 / model / picker
   rules** so one pass can feed both vectors. Every rule between is a pure
   predicate on the placement and a rejected placement never reached the insert
   either way, so the move is output-preserving — **verified empirically, not
   argued**: replaying both orderings over the 24 base maps gives identical
   pools in content *and* order, at `bell=False` (333) and `bell=True` (317),
   and the flag-on fallback vector is exactly the flag-off pool (333).
3. **`engine_pool` gained a `maps=` parameter** (with `load_base_maps`) beyond
   the plan's signature. §6.2 case 5 samples all 82 single-row selections, and
   re-reading 24 maps per selection made the selftest unusably slow. No
   behaviour change; existing callers are unaffected.

**One thing deliberately not added.** The ordering equivalence in (2) was left
as a one-off measurement rather than a permanent selftest case. Making it
permanent would mean carrying a *second* Python copy of the contribution loop,
and plan §2.4 and §5.3 are explicit that a duplicated mirror is the failure mode
to avoid. The standing guarantee for assertion 4 is §6.4 step 1's byte-diff
against the step-0 tree, which is exactly why that tree was required.

### Verification

* Clean rebuild (`rm -rf src/x64 && make`) — **succeeded, no warnings**,
  `IV0000-BBRD00001_00-BBRANDOMIZERMAIN.pkg` produced. Clean and not
  incremental, as §6.1 requires: `EnemyRandomizerResult` gained members.
* `pool_verify.py selftest` — **53/53**, i.e. the pre-existing 43 all still
  green plus 10 new.
* `pool_verify.py table both` — **PASS** at 82/17. `EnemyPoolTable.h` untouched.
* `ui_scroll_verify.py` — **PASS**. No geometry changed in this milestone.
* `enemy_lookup.py frozen` — **608**, unchanged.

Measurements the new cases pin, all reproducing the plan: the maidens-in-both
selection starves the pool to **0**; its fallback is **16** entries, equal to
the two rows' baked weights; every baked pool model owns overwritable
placements; all **82** single-row selections leave the fallback non-empty; all
six new display strings are inside `Font8x8.cpp`'s 42-character set, each
failure message is within the **42** characters the **29**-character prefix
leaves, and the two failure messages differ.

**Not verified here, and not verifiable in this loop:** everything runtime. A
clean build and green mirrors mean **ready for hardware testing**, not done —
`CLAUDE.md` §3. §6.4 steps 1–4 are the gate, and step 4 captures the
assertion-5 baseline that milestone 2 needs and that this build is the last one
able to produce.

**Status: awaiting the §6.4 hardware test and explicit approval before
milestone 2.**

---

## 2026-09-19 — Milestone 1 hardware test, step 0 resolved and step 1 PASSED

**The missing seed is recovered.** The developer located `live.log` for the
baseline run and added it to `data/runs/20260919-Enemies Only`. The run's own
line reads `enable wizard: USING SEED 1234567890`, so **`S` = `1234567890`**,
now written into `plan.md` §6.4 step 0 (and §8's closing note, which no longer
carries an outstanding item). A `README.txt` beside the tree records the seed,
every setting, the engine counters and the fact that the build which produced
it no longer exists.

The log also **confirms by direct evidence what §6.4 step 0 had only been able
to re-derive from the bytes**: bosses, treasure, workshop tools, drops,
starting weapons, starting guns, shop weapons and Mergo darkness all `NO`. The
planner's byte-level reconstruction of the settings was correct in every
particular.

### Step 1 — PASSED, and more strongly than the plan asked

Run: this build, seed `1234567890`, `RANDOMIZE ENEMIES` on, everything else
default. Output kept at `data/runs/20260919_STEP1-Enemies Only`.

| Check | Baseline (pre-change) | Step 1 (post-change) |
|---|---|---|
| Seed | `1234567890` | `1234567890` |
| Pool built | 333 candidates from 24 maps | **333 candidates from 24 maps** |
| Enemies randomized | 2120 across 24 maps | **2120 across 24 maps** |
| `map/mapstudio` (24 `.msb.dcx`) | — | **byte-identical** |
| Entire `dvdroot_ps4` tree | — | **byte-identical** |

The plan only required the 24 map files to match; the **whole tree** matches,
`event/common.emevd.dcx` included. The §6.4 preamble expected that file to
differ run to run, which is true against *vanilla* but not between two runs of
the same seed — the rewrite is deterministic. Worth recording so the next
reader does not treat a whole-tree match as suspicious.

**This is spec §8 assertion 4, confirmed on hardware.** It is the one thing no
Python mirror in this repository could establish, and the reason stage D
finding 2 was blocking. The two-pool refactor — a second vector, a second
per-map dedupe set, and the contribution loop's dedupe test moved below four
other rules — perturbs the roll stream by exactly nothing. The pre-flight
measurement that predicted this (identical pools in content and order at
`bell=False` and `bell=True`) is now corroborated by the console.

**No fallback line appeared**, which is correct: nothing was skipped, so
`poolFellBack` stayed false and §3.3 case 5 fired.

**Still outstanding for milestone 1:** §6.4 steps 2 (starved run completes),
3 (broken source fails legibly) and 4 (capture the assertion-5 baseline, which
only this build can produce). Step 5 is persistence.

---

## 2026-09-19 — Milestone 1 hardware test complete; developer closed it

**Step 2 — the starved run — PASSED.** Settings verified from the run's own log:
`unchanged bell maidens = YES`, `model picker: closed with 2 of 82 enabled`,
every other randomizer `NO`. The engine logged

    enemy randomizer: every selected enemy was also skipped - falling back to the selection for this run
    enemy randomizer: pool built - 16 distinct candidates from 24 maps

and the write phase then ran through the maps instead of ending in `Fail`.
Spec §10 D4 is satisfied on hardware: the configuration that previously died
after `StepMirror` — leaving the half-built tree of
`data/runs/Error Log - Chime Maidens` — now completes.

**16 is the correct fallback size**, reproducing plan §5.3 (12 `c1051` + 4
`c1050`) and selftest case 3. Recorded because the instruction given to the
tester said to expect 333, which was wrong: `poolIgnoringSkips` drops the
user's *skip*, not `ENEMIES INCLUDED`, which still narrowed it to the two
maiden rows. The error was in the test instruction only; no artifact and no
code carried it.

**Step 3 — the broken source — confirmed by the developer.** The failure path
fires and the message is legible, so stage D finding 1 is answered on hardware:
`VANILLA SOURCE UNREADABLE - NO MAPS FOUND` is distinguishable from the starved
run and renders inside `Font8x8.cpp`'s character set.

**Step 4 — the assertion-5 baseline — NOT captured. Developer's decision, taken
with the consequence stated.** The tree at
`data/runs/20260919_STEP4-Bell Maidens Unchanged` is **not** usable for it: its
final run had bosses, treasure, workshop tools, drops, starting weapons,
starting guns and shop weapons all on — only `unchanged bell maidens` was
toggled that session, the rest having been loaded from saved defaults. The
developer was told, chose not to re-run, and asked to proceed. That reverses
plan §9 P14 ("required, not optional").

**What that costs:** spec §8 assertion 5 — "ticking the two chime maiden rows
reproduces what `UNCHANGED BELL MAIDENS` did, for the same seed" — has no
baseline, so **§6.5 step 6 cannot be performed**. The equivalence will rest on
the argument in §6.3 (same placements skipped at the same two points, `c1055`
inert) plus selftest cases 17–19, and not on a byte comparison. Assertion 4 is
unaffected: its baseline exists and already passed.

**The decision was made reversible rather than final.** Milestone 1's `.pkg` —
the last build that can produce the assertion-5 baseline — was copied to
`data/runs/milestone1-build-KEEP.pkg` before milestone 2's first edit. If the
developer later wants assertion 5 checked, the run is still available; only the
scheduling was given up, not the capability.

**Milestone 1 is closed.** Steps 1, 2 and 3 passed; step 5 (persistence) was not
separately reported and is subsumed by the later runs, which loaded and saved
settings correctly. Proceeding to milestone 2.

---

## 2026-09-19 — Milestone 2 implemented: `ENEMIES SKIPPED`

Every row of plan §4.2 landed. Clean rebuild succeeded with no warnings;
`pool_verify.py selftest` is **85/85** (43 pre-existing, 10 from milestone 1,
32 new here).

### What was built

| Area | What landed |
|---|---|
| `EnemySkipTable.h` | **New, generated.** 85 rows, `kEnemySkipModelCount = 85`, weights summing to 2,269 |
| `ModelPoolSelection.h` | Second template parameter `bool DefaultSelected = true`; the ctor and the unknown-model fallback both key off it; `IsModelSkipped` alias |
| `EnemyPoolSelection.h` | `typedef ModelPoolSelection<kEnemySkipModelCount, false> EnemySkipSelection` |
| `EnemySkipList.h` | **New.** `BuildSkipPatterns` / `IsSkippedName`, kept out of `EnemyExclusionList.h` |
| `EnemyExclusionList.h` | **D1:** `BellMaidenExclusionList()` deleted; `IsExcludedEnemyName` back to one argument. `M28ForcedMaidenList()` untouched |
| `EnemyRandomizer.h/.cpp` | `unchangedBellMaidens` out, `enemiesSkipped` in; patterns built once in `State`'s ctor; the skip test at both call sites, at `:629` inside the same `!forced &&` |
| `RandomizerDefaults.h` / `Store.cpp` | `unchanged_bell_maidens` out of load and save, `enemies_skipped` into both |
| `ModelPicker.h/.cpp` | `PickerStrings` passed to both `Update` and `Draw`; second `ListLayout` chosen by `instruction != nullptr`; flag column, confirm verbs and footer all read from the struct |
| Both screens | `SkipPicker` mode, the drill-in row after `ENEMIES INCLUDED`, the summary text, the `N OF 85 ENEMIES SKIPPED` progress line, and D1's removals |
| Four tools | `gen_pool_table.py` third kind + `NAME_OVERRIDES`; `enemy_lookup.py` six `bell` → `skipped` plus `overwritable_*` and `zone_scaled_npc`; `pool_verify.py` third table, `maidens` → `skipped`, 32 new cases; `ui_scroll_verify.py` the 11-row entry |

### Measurements, all reproducing the plan

2,877 placements, 608 excluded, **2,269 overwritable, 85 distinct models**;
82 ⊂ 85 with the extras exactly `c1130`, `c2121`, `c2561`; the maidens-skipped
pool **317 entries / 80 models**, 16 weights lost; **1,171** tracked scaling
values each appearing **exactly once** in **26,591**; the stat pass reaches
**1,369** of the 2,269 — spec §4 F6 exactly; **26** of the 42 non-forced
maidens in scaled maps; no chaining over the **553** reachable values at all
fifteen zone scales; worst-case `defaults.cfg` **555** bytes; the widest skip
row **49** characters.

Geometry, from `ui_scroll_verify.py`: the skipped picker gets **11 rows**,
band `{332, 52, 900}`, `MORE ABOVE` at **286** against the instruction line's
259 bottom. **The two shipped pickers are unchanged at 12 rows and 234**, and
all three 15-row screens are untouched — F13 option B did what it promised.

### Four decisions taken while implementing

1. **The three lists' vocabulary lives in `ModelPicker.h`** as
   `kEnemiesIncludedStrings` / `kBossesIncludedStrings` /
   `kEnemiesSkippedStrings`, not inline at the four call sites across two
   screens. One copy, and the selftest parses that copy.
2. **`gen_pool_table.py`'s `both` still means enemy+boss.** Widening it to
   include `skip` would silently regenerate a third file for every existing
   caller.
3. **`enemy_lookup.load_base_maps` and the `maps=` parameter.** Case 5 samples
   all 82 single-row selections; re-reading 24 maps per selection made the
   selftest unusable.
4. **`--bell` became `--skip=c1050,c1051`**, and `BELL` became `SKIP_MAIDENS`.
   The 18 retargeted selftest cases keep their measured numbers intact, which
   is what makes them evidence that the two mechanisms are equivalent.

### One real bug found and fixed during verification

`cmd_skipped` first counted the twelve forced m28 placements as *not
eligible*, giving a denominator of 2,257 instead of 2,269. They are eligible —
they are exempt from assertion 1 only — so the exemption now skips the frozen
check rather than the placement.

### Partial evidence for assertion 5, obtained by accident

`pool_verify.py skipped` was run against
`data/runs/20260919_STEP4-Bell Maidens Unchanged` — a real tree produced by the
milestone-1 build with `UNCHANGED BELL MAIDENS` **on** — with `c1050,c1051`
given as the skip list. It **PASSES** both halves: every maiden frozen by the
corrected definition, and none arriving anywhere new.

That is not spec §8 assertion 5, which needs a byte comparison against a
milestone-2 run at the same seed and which has no baseline (see the previous
entry). But it is worth recording for two reasons. It shows the two mechanisms
agree on *which placements are protected*, which is most of what assertion 5
asks. And it is the first real-tree confirmation that §3.6's corrected
`frozen` definition works: the retired `maidens` command, which demanded plain
equality, would have **failed** on this very tree, because 26 of the 42
maidens sit in stat-scaled maps whose `NPCParamID` the stat pass rewrites.

The tree's boss and treasure passes did not disturb the check, which also
confirms §5.10's scoping: boss-written placements are all reference-excluded
and so never enter assertion 2's eligible set.

### Verification

* Clean rebuild (`rm -rf src/x64 && make`) — **succeeded, no warnings**, `.pkg` produced
* `pool_verify.py selftest` — **85/85**
* `pool_verify.py table both` — **PASS** at 82/17, `EnemyPoolTable.h` byte-identical
* `pool_verify.py table skip` — **PASS** at 85
* `gen_pool_table.py {skip,both} --check` — **PASS** on all three headers
* `ui_scroll_verify.py` — **PASS**
* `enemy_lookup.py frozen` — **608**, unchanged
* `boss_verify.py selftest` **5/5**, `starting_weapons_verify.py selftest` **12/12**

**Not verified here:** everything runtime. Ready for the §6.5 hardware test,
whose step 6 cannot be performed — see the previous entry.

---

## 2026-09-19 — Milestone 2 hardware-tested; feature CLOSED

The developer ran the §6.5 hardware test and closed the feature as working as
expected. That is the last human gate: spec APPROVED → plan → review →
implement → verify → **hardware test passed**.

`docs/randomization-feature-spec.md` is the authority on feature status and
now records row 32 as **DONE**; this entry records how it got there.

### Documentation stage — what was written, and where

Plan §4.2 deliberately left these to this stage, naming two things that had to
be recorded. Both are.

| Document | Change |
|---|---|
| `docs/randomization-feature-spec.md` | Row 32 → **DONE** with both exceptions named. Row 16 → **OUT**, retired by row 32, with D1's un-migrated value spelled out. §3 heading 1 → 0 remaining; tally 20 → 19. The §3 "open decision" on retiring row 16 marked answered. The §3 starvation paragraph corrected: it proposed a commit-time refusal, and what shipped was D4's non-failing run |
| `docs/user-guide.md` | New **Enemies skipped** section: what ticking does in both directions, why 85 and not 82, the four caveats, and both starved outcomes with their on-screen wording. Glance table gains the row. A prominent note that `UNCHANGED BELL MAIDENS` is gone and **will not carry over** |
| `docs/design-decisions.md` | New **Divergences from the reference, taken deliberately** section, holding all three: D4's non-failing starved run, the separated vanilla-source failure with its two messages and the font constraint, and the unfiltered boss pool |
| `docs/features/README.md` | Row 32's plan marked implemented, tested and closed |

### The two exceptions are now recorded in three places each

Neither is a defect and both are easy to rediscover as one, so each appears in
the backlog row, the user guide and the design decisions:

1. **Six Yahar'gul maidens still change** with both chime maiden rows ticked.
   The reference's override is unconditional and outranks the skip list.
2. **The boss pool is not filtered.** `c2090` and `c2710` are in both tables,
   so with `RANDOMIZE BOSSES` on a boss arena can still become one. This was
   the open spec question §5.10 raised; the developer saw it on hardware at
   §6.5 step 12 and closed the feature anyway, which settles it as
   accept-and-document. `docs/design-decisions.md` records what changing it
   would cost, so the option stays open without staying open as a question.

### What was NOT verified, stated plainly

**Spec §8 assertion 5 was never checked.** It needed a byte comparison against
a milestone-1 build with `UNCHANGED BELL MAIDENS` on, at seed `1234567890`,
with everything else off. That tree was not captured — the developer's
decision, recorded two entries above — and the build that could produce it is
preserved at `data/runs/milestone1-build-KEEP.pkg` should it ever be wanted.

The nearest evidence obtained: `pool_verify.py skipped` passed against
`data/runs/20260919_STEP4-Bell Maidens Unchanged`, showing the retired flag and
the new list protect exactly the same placements. That is a placement-level
match, not a byte-level one. The feature is closed with that gap known rather
than papered over.

Assertions 1, 2, 3, 4, 6, 7 and 8 were all checked. Assertion 4 in particular
passed twice — once per milestone — against the pre-implementation baseline.

### Final state

* `pool_verify.py selftest` **85/85**
* `table both` PASS 82/17, `table skip` PASS 85, all three generators `--check` PASS
* `ui_scroll_verify.py` PASS, `enemy_lookup.py frozen` 608
* `boss_verify.py selftest` 5/5, `starting_weapons_verify.py selftest` 12/12
* Clean rebuild, no warnings

**Feature 032 is closed.** The folder is the record: `spec.md` (APPROVED),
`plan.md`, `plan-review.md` (CHANGES REQUESTED, answered) and this log.
