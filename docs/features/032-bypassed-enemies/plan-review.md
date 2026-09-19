# Plan Review 032 — Bypassed Enemies (`ENEMIES SKIPPED`)

**Verdict: CHANGES REQUESTED**

**Plan:** `docs/features/032-bypassed-enemies/plan.md`

**Spec:** `docs/features/032-bypassed-enemies/spec.md` (APPROVED, developer, 2026-09-18)

**Reviewed:** 2026-09-19

---

## 1. Summary

This is a strong plan. Almost every measured number in it reproduced exactly
against `data/vanilla/dvdroot_ps4` and the working tree — 2,877 / 608 / 2,269 /
85, the 82-row superset relation, the three extras, the weight spread, 1,369
scaled placements, 1,171 tracked values each appearing exactly once in a 26,591
entry table, 54 / 12 / 42 / 26 maidens, the 478 → 555 byte config growth, and
the whole option-B picker geometry. The two-pool design in §3.3 is sound
including the part that is easy to get wrong: the per-map `contributedNpcIds`
dedupe sits *before* the picker test in the contribution loop, which is exactly
why "a non-empty `ENEMIES INCLUDED` always yields a non-empty fallback pool"
holds, and why contribution-time filtering is not interchangeable with
after-the-fact filtering. §5.9's disjointness argument for the boss pass
re-verified cleanly.

Two things block it, both cheap to fix and both about *verification* rather than
design:

* the new failure messages are budgeted by length only, and the font has no
  lowercase and no `:` glyph, so a message written to the engine's existing
  `Fail` convention renders as blank columns on the one screen the player is
  looking at — which is precisely what spec §7 and §8 assertion 8 exist to
  prevent (Finding 1);
* nothing in the plan captures the output tree that spec §8 assertion 4 needs,
  and after milestone 1 lands there is no build left that can produce one. The
  plan makes this exact argument for assertion 5 (P14) and misses the symmetric
  case (Finding 2).

Four "should fix" findings follow, of which Finding 3 — six of the 85 rows have
reference-excluded placements too, and two of them are also in the boss pool —
is the one with real teeth: it makes a planned automated assertion fail on a
correct implementation, and it exposes a second exception to the feature's
headline promise that neither the plan nor the spec records.

Three plan numbers did not reproduce (§5). None of them changes a decision, but
the house style is measured numbers, and one of them (168) comes from a mirror
that omits a branch of the code it is mirroring.

---

## 2. Findings

### Finding 1 — The new failure messages are budgeted for width but not for the font

**Severity:** Blocking

**Where:** Plan §3.4, §6.2 case 6, §6.4 step 3; `app/src/Platform/Font8x8.cpp`;
`app/src/Randomizer/EnemyRandomizer.cpp` `Fail()`

**Problem:** §3.4 correctly derives the 43-character budget
(`"ENEMY RANDOMIZATION FAILED: "` is 28 characters, the line holds 71), and
§6.2 case 6 pins that budget by parsing the strings out of the source. Nothing
in the plan constrains the *characters*.

`Font8x8.cpp`'s glyph table is space, `A`–`Z`, `0`–`9`, `'`, `(`, `)`, `-`, `,`
and nothing else. `DrawChar8x8` returns immediately on an unknown glyph while
`DrawText8x8` still advances the cursor, so unknown characters render as blank
columns of the right width. Every existing `Fail()` string in
`EnemyRandomizer.cpp` is lowercase, and the same string is also fed to `Log()`
with a lowercase `"enemy randomizer: "` prefix, so lowercase is the local
convention an implementer will follow.

The consequence is visible in today's build and the plan half-diagnoses it. The
current message is 70 characters, 98 on screen; §3.4 attributes its invisibility
to clipping, but the dominant cause is that all 70 characters are lowercase and
render blank. What is actually on screen today is `ENEMY RANDOMIZATION FAILED`
followed by ~44 blank columns, drawn off-centre.

**Why it matters:** §6.4 step 3's pass criterion is "the new one must be fully
readable", and spec §7 makes "an unusable vanilla source must still fail,
loudly and distinguishably" a requirement of this feature rather than a nicety.
A 43-character lowercase message satisfies every automated check in §6.2 and
still shows the player a blank line. The hardware step would fail, and the
rework lands after a build-and-test cycle.

There is a real decision hidden here, which is why it is blocking rather than a
note: the same string goes to the text log and to the screen. Uppercasing the
`Fail()` strings changes the log's house style; uppercasing at the UI changes
`FinishCommit` for every error; carrying a separate display string changes
`EnemyRandomizerResult`. The plan should pick one.

**Recommendation:** State the character set as a constraint on the two new
messages alongside the length budget, decide where the uppercasing happens, and
extend §6.2 case 6 to assert renderability against `Font8x8.cpp`'s glyph table
(the generator's `RENDERABLE` set in `gen_pool_table.py` is already the right
predicate) rather than length alone. Note that `:` is also not renderable, so
the fixed prefix loses its colon whatever is decided.

**Confidence:** High. Verified by reading the glyph table and both draw
functions, and by re-measuring the current message.

---

### Finding 2 — Spec §8 assertion 4 has no check, and its baseline expires when milestone 1 starts

**Severity:** Blocking

**Where:** Plan §3.3 (last bullet), §6.4 step 1, §6.5 step 9, §6.2

**Problem:** Spec §8 assertion 4 is "Nothing skipped is byte-identical to a run
of the same seed made before the feature existed."

* §3.3 argues it holds by construction (no extra `RandInt` is drawn). The
  argument is correct as far as it goes.
* §6.2 has no case for it. Case 1 pins that the flag-off *pool* is still 333
  entries / 82 models, which is necessary but not the assertion.
* §6.5 step 9 says "Same seed, nothing ticked, compare against `A`", where `A`
  is defined three lines earlier as "seed `S` with nothing skipped". That
  compares a tree against itself. It cannot fail.
* §6.4 step 1's pass criterion for milestone 1 is "the two-pool build must
  change nothing", with nothing to compare against.

So neither milestone has a check that could detect a perturbed roll stream, and
the one that would — a tree from a build without this work — can only be
produced by the build that exists **now**. `EnemyRandomizer.cpp`,
`EnableWizardScreen.cpp`, `RandomizerDefaults*` and the three tools are all
uncommitted working-tree modifications (see `git status`), so once
implementation starts the pre-change state is not recoverable from `git` either.

**Why it matters:** This is the same hazard the plan identified for assertion 5
and made a required, dated hardware step for (§6.4 step 4, P14). Assertion 4 is
the one that protects every shipped seed: if the two-pool refactor accidentally
shifts the stream — a stray `RandInt`, a reordered contribution, a dedupe set
built in the wrong scope — the default configuration silently produces a
different world and nothing in the plan notices.

**Recommendation:** Add a step *before* milestone 1's first code change: run the
build currently in hand with `RANDOMIZE ENEMIES` on, everything else default and
a recorded seed (seed `1234567` would let it share §6.4 step 4's bookkeeping),
and keep the tree under `data/runs/`. Then make §6.4 step 1 and §6.5 step 9 diff
the 24 `.msb.dcx` files against *that* tree rather than against a tree from the
same build.

**Confidence:** High for the gap. Medium on whether the developer already has a
usable tree: `data/runs/Randomize Enemies Only/PS4 1..3` and
`data/runs/New Test Runs/...` are map-only trees, but no document records their
seeds or settings, so I could not confirm any of them is usable as a baseline.
If one is, saying so in the plan closes the finding.

---

### Finding 3 — "No placement anywhere uses a skipped model" is false for six of the 85 rows, and the boss pass can spawn two of them

**Severity:** Should fix

**Where:** Plan §6.3 assertion 2, §5.9, §6.5 step 2

**Problem:** §6.3 states assertion 2 as "no placement anywhere in the output uses
a skipped model", suppressed only by `--starved`. Two separate things break it.

*Six of the 85 also have reference-excluded placements.* Measured across the 24
base maps, these models have both overwritable and already-excluded placements:

| Model | Name |
|---|---|
| `c1060` | Brainsucker |
| `c2090` | Blood Starved Beast |
| `c2100` | Witch of Hemwick |
| `c2120` | Shadow of Yharnam |
| `c2500` | Small Celestial Emissary |
| `c2710` | Father Gascoinge |

Tick any of those six and the output tree still contains placements of that
model — the frozen, reference-protected ones — so a check written to the literal
wording fails on a correct implementation with nothing else enabled.

*Two of them are in the boss pool.* `BossPoolTable.h`'s 17 models intersect the
85 at exactly `c2090` and `c2710`. Boss randomization draws from its own pool
and writes into boss arenas, all of which are matched by the fixed exclusion
list — so §5.9's conclusion (a skipped *placement* can never be touched by the
boss pass) is correct and I re-verified it: every `kFixups` companion and every
`AddTheRestInMap` target is matched by `EnemyExclusionList()`. But the *pool*
half does not survive: with `RANDOMIZE BOSSES` on, ticking
`C2090 BLOOD STARVED BEAST` in `ENEMIES SKIPPED` does not stop a boss arena from
becoming a Blood Starved Beast. §5.9 establishes only the placement half and
does not mention this.

**Why it matters:** Two consequences. First, a planned automated assertion fails
on a correct implementation, which costs a debugging cycle and, worse, trains
the reader to ignore it. Second, the feature ships with a second exception to
its headline promise ("no other place in the game becomes that creature") that
nothing records — D4 is documented as "the only case in which it does not"
(spec §10 D4), and that statement is not true when bosses are on.

**Recommendation:** Restate assertion 2 as "no *eligible* placement was
*changed* to a skipped model" — which is the shape `pool_verify.py filter`
already has — and say explicitly that boss-written placements are out of its
scope. Add the boss-pool overlap to §5 as a named risk. The underlying
behavioural question (should skipping `c2090` also remove it from the boss pool?)
is a spec decision, not the planner's; see §4 of this review.

**Confidence:** High. Both intersections computed from `EnemyPoolTable.h`,
`BossPoolTable.h`, `EnemyExclusionList.h` and the 24 base maps.

---

### Finding 4 — §6.5 hardware step 3 names a row that will not exist

**Severity:** Should fix

**Where:** Plan §6.5 step 3, against §5.5, §9 P17 and spec §10 D6

**Problem:** §5.5, P17 and spec D6 all settle that `c1130`'s row reads
`C1130 OEDON CHAPEL DWELLER`. §6.5 step 3 nevertheless instructs the tester to
"Tick the row reading `C1130 LABYRINTH RITEKEEPER`".

**Why it matters:** This is the identical defect D6 was raised to remove from
spec §8 hardware step 2 — a hardware instruction naming a string that will not
appear on screen — reintroduced in the plan's own hardware table. A tester
paging through 85 rows looking for LABYRINTH RITEKEEPER will not find it.

**Recommendation:** One-word fix. Worth a pass over the other hardware steps for
the same class of drift.

**Confidence:** High.

---

### Finding 5 — The generator's D6 name override is required by §5.5 but missing from §4.2 and from §6.2

**Severity:** Should fix

**Where:** Plan §4.2 (`gen_pool_table.py` row), §3.1 ("names" bullet), §6.2

**Problem:** §5.5 says "the generator therefore needs a small project-owned
name-override map rather than taking `Characters.json` as the only source, and
that override must be visible as ours in the generated header." Neither §3.1's
names bullet nor §4.2's `gen_pool_table.py` row mentions it — that row says only
"Third `Kind`, `skip`; `KINDS`/`main` accept it; `both` stays enemy+boss". §6.2
pins `c2561`'s fallback (case 11) but has no case pinning `c1130`'s override.

Two secondary points in the same area:

* the override is safe to apply globally — `c1130` appears in neither
  `EnemyPoolTable.h` nor `BossPoolTable.h` (verified) — but the plan should say
  so, because P1's promise that `EnemyPoolTable.h` is untouched is otherwise
  resting on a fact nobody has written down;
* §4.2's `enemy_lookup.py` row lists `exclusion_reason` / `engine_pool` /
  `randomizes` as taking `skipped=()` in place of `bell`, but
  `engine_pool_models(root, bell=False)` and `cmd_frozen(..., bell=False)` also
  carry the parameter, and `pool_verify.py:348-349` calls the former.

**Why it matters:** The files-and-changes table is what an implementer works
from. A required change described only in the risks section is a change likely
to be missed, and D6 is the only reason hardware step 3 is meaningful.

**Recommendation:** Move the override into §3.1 and §4.2, and add a selftest case
that the `c1130` row reads `OEDON CHAPEL DWELLER` and is marked in the generated
header as project-owned.

**Confidence:** High.

---

### Finding 6 — The fallback and its progress line are not gated on `randomizeEnemies`

**Severity:** Worth considering

**Where:** Plan §3.3 case 3, §3.4 first bullet

**Problem:** Cases 1 and 2 are both gated on `options.randomizeEnemies`,
mirroring today's `Fail`. Case 3 is written as a bare `if (pool.empty())`, and
§3.4's fallback progress line is gated on `result.poolFellBack` alone.

`StartCommit`'s empty-selection refusal only fires when `randomizeEnemies_` is
true, so a bosses-only run with an empty saved `ENEMIES INCLUDED` is reachable
from the shipped screens. In that run both vectors are empty, case 3 fires,
`poolFellBack` is set and `FinishCommit` prints "every selected enemy was also
skipped and the skipped creatures were used as replacements for this run" on a
run that never intended to randomize an enemy.

The plan also does not say what happens to today's
`Log("enemy randomizer: enemy pool empty and enemies disabled - skipping enemy shuffle")`
line, which sits in the block being rewritten.

**Why it matters:** A confusing progress line on a legitimate configuration,
and one existing log line silently dropped. Neither is harmful; both are the
kind of thing that gets found on hardware instead of in review.

**Recommendation:** Gate case 3 and the progress line on
`options.randomizeEnemies`, and say in §4.1 whether the enemies-disabled log
line survives.

**Confidence:** High for reachability; the consequence is cosmetic.

---

### Finding 7 — The no-chaining evidence is measured against the wrong value set (and the stronger claim is true)

**Severity:** Worth considering

**Where:** Plan §3.6, §6.2 case 22

**Problem:** §3.6 justifies treating `ApplyBossParamScaling` as a plain
dictionary with two measurements, the second being "no sequential-rewrite
chaining occurs on any real map (simulating the engine's in-place loop gives
byte-identical results to a single-shot mapping on all fifteen scaled maps)".
Case 22 pins the same thing.

That is measured over *vanilla* map contents. The mirror's job is to validate
*randomized* output trees, where a scaled map can hold any NPCParamID in the
global pool, not just the ones that map held in vanilla. The vanilla-only check
does not cover the value set the mirror will actually meet.

It also understates how load-bearing the property is: the table structurally
permits chaining. For each zone scale, between 151 and 351 of the 1,171 tracked
values scale to a value that is itself a tracked value processed later in the
loop.

**Why it matters:** Low, because I re-ran it over the full reachable set — all
553 distinct NPCParamIDs that can appear in any map after randomization (every
vanilla placement value ∪ every pool value) — at all fifteen zone scales, and
found zero chaining. The claim holds, more strongly than stated. But the check
as written would not have caught it if it did not.

**Recommendation:** Restate case 22 over the reachable value set rather than
over the fifteen vanilla maps, and record the 151–351 figure so the next reader
knows the property is a data accident and not a structural guarantee.

**Confidence:** High.

---

### Finding 8 — `log.md` has no entry for the pass that produced P16, P17 and spec D6

**Severity:** Worth considering

**Where:** `docs/features/032-bypassed-enemies/log.md`, last entry

**Problem:** The log is append-only and "every stage adds an entry". Its final
entry ends "One question remains open — §8 Q4 ... Plan status: `QUESTIONS
OPEN`". The plan now says all five questions are answered, §8 is deliberately
empty, and P16/P17 record the answers; the spec carries a D6 amendment dated the
same day. None of that pass is logged.

**Why it matters:** The log is the evidence trail. A reader who trusts it will
believe the plan still has an open question and that the spec has five decisions
rather than six.

**Recommendation:** Append an entry for the refine-plan pass.

**Confidence:** High.

---

## 3. What looks good

* **The two-pool design (§3.3) is right for a non-obvious reason, and the plan
  knows what the reason is.** The "Rejected — build one pool and filter it
  afterwards" paragraph is the sharpest thing in the document: contribution-time
  filtering and after-the-fact filtering genuinely are different rules because
  `contributedNpcIds` is inserted only when a placement contributes, and the
  reference filters at contribution. I checked the loop at
  `EnemyRandomizer.cpp:369-388` and the dedupe test sits *before* the picker
  test, which is exactly what makes "a non-empty selection always yields a
  non-empty fallback pool" true rather than merely plausible. §6.2 case 5's
  single-row sampling is sufficient for that claim, and I could not construct a
  counterexample.
* **The UB hazard is closed by construction and the plan says why row 16's
  finding 2.2 does not recur.** I confirmed `DrawCandidate` is the only site
  indexing `pool`, and that `StepWriteMap`'s entire body is inside
  `if (options.randomizeEnemies)` at `:618`. No draw-site guard is needed and the
  plan resists adding one.
* **§5.9's boss disjointness was re-verified rather than inherited.** Every
  `kFixups` companion (`c2570_0001`, `c5510_0001`, `c5510_0002`, `c4520_0000`,
  `c4030_0000`) and every `AddTheRestInMap` target (`c2100_0001`, `c2120_0001`,
  `c2120_0002`, `c4030_0001..3`) is matched by `EnemyExclusionList()`, and
  `BossNameList()`'s 38 entries are exactly the tail of it. The placement half of
  the claim is solid. (The pool half is Finding 3.)
* **The polarity-as-template-parameter decision (§3.2, P2) is the right call and
  the rejected alternatives are argued honestly.** `ModelPoolSelection` has
  exactly three instantiation sites and no forward declarations, so the defaulted
  parameter is genuinely invisible to rows 9 and 10. The "reusing
  `ModelPoolSelection<85>` with enabled-means-skipped" rejection is correct:
  spec §7 forbids it and it would make the fail-safe depend on call sites.
* **The `defaults.cfg` correction (§4.2) is a genuine improvement over the
  approved spec and was handled properly** — raised as a spec gap in `log.md`
  rather than silently changed. `strtok` + unknown-keys-ignored is what the code
  does (`RandomizerDefaultsStore.cpp:38-83`), and 478 → 555 of 1,024 reproduced
  to the byte.
* **§3.6 finds and routes around the `pool_verify.py maidens` error correctly.**
  I confirmed `cmd_maidens` asserts plain three-field equality and expects 42,
  and that 26 of the 42 sit in scaled maps with tracked NPCParamIDs — so it would
  report 16 frozen and 26 "retargeted" and fail twice. Retiring it with the
  setting (P8) while pinning the 26 as a selftest case is the right disposal.
* **`boss_verify.load_scaling_ids` really does return an empty set** (I ran it;
  it returns 0 ids from a path that no longer exists), and §2.4 flags it as a
  trap without building on it. That is the sort of thing that would otherwise be
  rediscovered as this feature's bug.
* **The picker geometry (§3.5, option B) is fully correct.** I re-derived it
  through `ui_scroll_verify.py`'s own model: `(900-332)/52+1 = 11` rows, last row
  at 852, `MORE BELOW` bottom 922 against a 950 footer, `MORE ABOVE` at 286
  against an instruction bottom of 259 — 27px, as claimed. 85 rows is 8 pages at
  both 11 and 12. The 42-character instruction and the 45-character footer are
  both well inside the 71-character line, and `39 + 3 + 7 = 49` characters is
  1,323px of 1,920.
* **The milestone split and its justification (§7) are convincing.** "The
  starved case is only independently testable while `UNCHANGED BELL MAIDENS`
  still exists" is a real dependency, not a preference, and the "why not three"
  argument correctly rules out a milestone with nothing to hardware-test.
* **§5.7 flagging assertion 3 as a rate check rather than an equality check** is
  the right call and was raised rather than quietly downgraded.

---

## 4. What could not be verified

* **Anything that requires the console.** Whether a frozen chime maiden still
  rings her bell, whether the Oedon Chapel dweller still works, whether the
  instruction line is legible at viewing distance, whether a starved run's write
  phase reads as slow rather than hung. `CLAUDE.md` §5 applies throughout: none
  of my measurements say anything about what the game *does*.
* **I did not build.** The OpenOrbis toolchain is present at
  `C:/Users/Fry/OpenOrbis/PS4Toolchain`, but nothing in this review required a
  compile and the tree carries uncommitted modifications I did not want to
  disturb.
* **Whether a usable pre-feature baseline tree already exists.**
  `data/runs/Randomize Enemies Only/PS4 1..3` and
  `data/runs/New Test Runs/*` are map-only trees, but no document records their
  seeds or settings, so I could not tell whether one of them could serve
  Finding 2's purpose.
* **The §5.3 timing estimate** ("ten to thirty seconds"). Only the console can
  say. I did re-derive the placement count it rests on, and got a different
  number (see §5 and Finding 7's neighbour below).
* **Reference-source fidelity for the new behaviour.** D4 is explicitly *not*
  the reference's answer (spec §3, §10 D4), so there is nothing in
  `reference/` to check it against. I did not re-read
  `RandomizeFunctions.cs`; the plan's reference claims were already checked at
  spec stage and the existing selftest asserts the six forced names appear
  verbatim in the reference source.
* **Areas I did not examine:** the treasure, drops, starting-weapons and
  Mergo-darkness paths; `Msb`/`Param`; the AFR/Platform layer. Nothing in the
  plan touches them and §3.7's layering claim looks right on inspection — the UI
  gains no AFR knowledge and nothing in `Randomizer` gains SDL2.

---

## 5. Numbers and evidence

All measured against `data/vanilla/dvdroot_ps4` and the current working tree.

| Claim | Plan says | Verified | Result |
| --- | ---: | ---: | --- |
| Enemy placements in the 24 base maps | 2,877 | 2,877 | Confirmed |
| Matched by the fixed exclusion list | 608 | 608 | Confirmed |
| Overwritable placements | 2,269 | 2,269 | Confirmed |
| Distinct models on them | 85 | 85 | Confirmed |
| Skip-table weights sum | 2,269 | 2,269 | Confirmed |
| Per-row spread (min / median / max) | 1 / 14 / 283 | 1 / 14 / 283 | Confirmed |
| Rows with ≤3 placements / exactly 1 | 17 / 5 | 17 / 5 | Confirmed |
| Overwritable in retail-loaded maps | 1,602 | 1,602 | Confirmed |
| 82 pool models ⊂ 85 | strict superset | true | Confirmed |
| The three extras | c1130, c2121, c2561 | c1130, c2121, c2561 | Confirmed |
| Longest row label | 39 (`C2620 WHEELCHAIR HUNTSMAN (GATLING GUN)`) | 39, same string | Confirmed |
| `name[:5] == model`, all placements | no exception | 0 of 2,877 fail | Confirmed |
| Cross hits (a model id inside another's name) | 0 | 0 | Confirmed |
| `c2561` display name | falls back to `C2561 C2561` | `model_name('c2561') == 'c2561'` | Confirmed |
| `c1130` in `Characters.json` | "Labyrinth Ritekeeper" | same | Confirmed |
| `IsExcludedEnemyName` call sites | `:375`, `:629` | `:375`, `:629` | Confirmed |
| The `Fail` being replaced | `:418` / `:419` | `if` at 418, `Fail` at 419 | Confirmed |
| Write-loop `randomizeEnemies` gate | `:616` | `:618` | Off by 2 |
| Size-gate reroll loop | `:652-657` | `:645-657` | Off by ~6 |
| ` BELL MAIDENS UNCHANGED` suffix | `:557` | `:559` | Off by 2 |
| `BossScalingMaps()` entries / scaled | 16 / 15 | 16 / 15 | Confirmed |
| Scaling pass runs outside the enemy branch | yes | yes, after `} // if (randomizeEnemies)` | Confirmed |
| Tracked values | 1,171 | 1,171 | Confirmed |
| Scaling table entries | 26,591 | 26,591 | Confirmed |
| Each tracked value appears exactly once | yes | 0 exceptions | Confirmed |
| Overwritable placements reached by the stat pass | 1,369 | 1,369 | Confirmed |
| Maiden placements / forced / non-forced | 54 / 12 / 42 | 54 / 12 / 42 | Confirmed |
| Non-forced maidens in scaled maps | 26 | 26 (all with tracked npc) | Confirmed |
| Sequential vs single-shot scaling, vanilla maps | identical | identical | Confirmed |
| …over the full reachable value set (553 values) | not claimed | identical | Confirmed (stronger) |
| `c1055` placements | 0 | 0 (selftest) | Confirmed |
| `BossNameList()` is the tail of `EnemyExclusionList()` | 38 entries | 38, tail matches | Confirmed |
| Boss fixups/targets all reference-excluded | yes | all 15 matched | Confirmed |
| `ModelSizeTable` prefixes | 245 | 245 | Confirmed |
| `kSizeToleranceMultiplier` | 2.0 | 2.0 | Confirmed |
| `Font8x8` `kAdvance` / char width at scale 3 | 9 / 27px | 9 / 27px | Confirmed |
| Characters per line at scale 3 | 71 | 71 | Confirmed |
| Instruction line length | 42 | 42 | Confirmed |
| New footer length | 45 | 45 | Confirmed |
| Skipped-picker visible rows at `{332,52,900}` | 11 | 11 | Confirmed |
| Pages for 85 rows at 11 and at 12 | 8 and 8 | 8 and 8 | Confirmed |
| `MORE ABOVE` clearance, option B | 27px | 286 − 259 = 27 | Confirmed |
| `defaults.cfg` worst case, now → after | 478 → 555 of 1,024 | 478 → 555 | Confirmed |
| Retired / added config line lengths | −25 / +102 | 25 / 102 | Confirmed |
| Failure-message budget | ≤43 chars | 71 − 28 = 43 | Confirmed |
| `pool_verify.py selftest` | 43/43 | 43/43 | Confirmed |
| `pool_verify.py table both` | PASS 82/17 | PASS 82/17 | Confirmed |
| `ui_scroll_verify.py` | PASS, pickers at 12 rows | PASS, 12 rows | Confirmed |
| `boss_verify.load_scaling_ids` | returns empty | returns 0 ids | Confirmed |
| Six mirror lines then failure in the chime-maiden log | `:26394-26400` | lines ~26391-26399 | Confirmed |
| **Old failure message length / on-screen** | **69 / 97** | **70 / 98** | **Not reproduced** |
| **Placements burning the full reroll, starved case** | **168** | **162 at the 30,000 cap, plus 73 at the 30-try cap = 235** | **Not reproduced** |
| **Option A list band `{260,52,900}`** | **12 rows** | **13 rows** | **Not reproduced** |

Three notes on the three that did not reproduce.

* **70, not 69.** `"enemy pool is empty - every enemy is excluded, or nothing was
  eligible"` is 70 characters, 98 on screen. Immaterial to the 43-character
  budget, which I re-derived independently and which is right.
* **168 comes from a mirror that drops a branch.** Applying the `× 2.0` gate
  uniformly to the 2,215 non-maiden overwritable placements gives exactly 168 —
  but `StepWriteMap` uses a different gate in `m24_02` and `m35`
  (`> originalSize`, cap 30 tries). With the engine's actual branching, 235
  placements exhaust their loop: 162 at the 30,000 cap and 73 at the 30-try cap.
  The plan's conclusion (a starved run is noticeably slower, tell the tester) is
  unaffected and if anything understated, but the number in §5.3 should be 162
  with the 73 mentioned separately, and the omitted branch is worth noting
  because §6.2 proposes extending `enemy_lookup.randomizes()` as the single
  mirror of that decision.
* **Option A would give 13 rows, not 12.** `(900-260)/52 + 1 = 13`. The plan's
  own option-A row also quotes a `MORE ABOVE` y of 234, which is the *current*
  band's hint position (280 − 46), not 260 − 46 = 214. This is in a rejected
  alternative, so it changes nothing — option B is independently correct and I
  verified every number in its column — but the comparison table is not
  reproducible as written.

---

## For the developer, not the planner

Two items belong in `log.md` as notes against the **spec**, since the spec is
not the planner's to change:

1. **Spec §10 D4 says its exception is "the only case in which" a skipped
   creature appears somewhere new. With `RANDOMIZE BOSSES` on, that is not
   true.** `c2090` (Blood Starved Beast) and `c2710` (Father Gascoinge) are in
   both `BossPoolTable.h` and the 85-row list, so a boss arena can become a
   creature the player ticked here. Spec §2 and §4 F8 address only the placement
   half ("no boss placement is one of the 2,269"). The decision needed is
   whether this is accepted and documented, or whether `ENEMIES SKIPPED` should
   also filter the boss pool for those two models. Finding 3 covers the plan-side
   consequence either way.
2. **Spec §8 assertion 4 needs a baseline tree captured before implementation
   starts**, for the same reason P14 captures one for assertion 5. See
   Finding 2. Unlike assertion 5's, this one cannot be recovered from `git`,
   because the relevant sources are uncommitted working-tree modifications.

---

<!--

This review evaluates the plan. It does not rewrite or modify it.

The planner addresses findings and updates the plan separately.

-->
