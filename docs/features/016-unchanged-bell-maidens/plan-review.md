# Plan Review 016 — Unchanged Bell Maidens

**Verdict: CHANGES REQUESTED.**

**Plan:** `docs/features/016-unchanged-bell-maidens/plan.md` (status at review time: QUESTIONS ANSWERED — awaiting developer approval)
**Spec:** `docs/features/016-unchanged-bell-maidens/spec.md` (status: APPROVED, developer, 2026-09-15)
**Reviewed:** 2026-09-15

---

## 1. Verdict and why

The feature itself — the settings chain, the conditional exclusion at both call
sites, and the m28 override — is planned correctly and in unusual depth. Every
measured number in the plan reproduced exactly against
`data/vanilla/dvdroot_ps4` (§5), every cited line number in `app/src` and
`reference/` resolves, and the three approaches most likely to do real damage
(A2's `changeData` restructure, A5's table regeneration, A3's skipped roll) are
rejected for the right reasons with the right arithmetic.

What drives the verdict is §9.3, the empty-pool change that arrived with
decision 3 and is the one part of this plan not about bell maidens.
**Removing the up-front `Fail` removes the only hard failure the enemy path has
for an unusable `VanillaSource`** — `StepMirror` only logs a warning and
`StepReadMap` silently skips a missing map — so after this change a wrong or
half-transferred vanilla tree produces a run that reports *success* having
randomized nothing. The plan treats the picker as the only route to an empty
pool; it is not. Alongside that, the plan specifies §9.3's replacement guard far
more loosely than it specifies everything else, and the failure mode there is
`RandInt(0, -1)`.

Three smaller findings would each cost the implementer or the developer real
time. Nothing here suggests the approach is wrong; the bell-maiden half is ready.

## 2. Findings

### 2.1 Blocking — removing the up-front `Fail` deletes the only guard against an unusable `VanillaSource`

**What is wrong.** §5.2 and §9.3 reason about the empty pool as if the enemy
picker were the only way to reach it, and conclude that "nothing fails, so
nothing can fail halfway". The other route — a `VanillaSource` that does not
contain readable maps — produces exactly the same empty pool, and the `Fail`
being removed is the only thing that reports it.

**Where.** Plan §5.2, §9.3, §4 row (e).
`app/src/Randomizer/EnemyRandomizer.cpp:304-312` (`StepMirror` logs
`"warning - incomplete mirror"` and continues when `CopyDirRecursive` fails);
`:335-337` (`StepReadMap` logs `"skipping missing map"` and returns);
`:407-409` (the `Fail` being removed). There is no other hard error between the
mirror and the write for a missing or unreadable `map/mapstudio`.

**Why it matters.** Today, pointing the app at a wrong or incompletely
transferred `/data/bbrandomizer/VanillaSource` fails the run with a message.
After this change the same user gets a completed run, a success result screen
and `RANDOMIZED 0 ENEMIES ACROSS 0 MAPS`, plus a partially mirrored AFR tree the
console will happily load. CLAUDE.md §5 records that a contaminated vanilla
source already cost this project a day of chasing a code bug that did not exist;
this change makes that class of problem quieter, not louder. §6.5 step 8
exercises the picker route only, so nothing in the verification plan would
notice.

There is a second, procedural edge. The log entry for decision 3 records the
question the developer answered as the maidens-only picker case. "Remove the
up-front `Fail`" is a strictly larger change than "make the maidens-only picker
case not fail halfway", and the plan does not flag the difference.

**Class.** Assumption — the plan assumes the picker is the only route to an
empty pool, and proceeds on it without stating the assumption.

**Confidence.** High on the mechanism (read directly from the three code sites
above). What would settle the user-facing severity is a hardware run with a
deliberately empty `VanillaSource/dvdroot_ps4/map`, which nothing in this loop
can do.

### 2.2 Should fix — §9.3's replacement guard is the least specified change in the plan, and its failure mode is UB

**What is wrong.** §3.2 gives "the exact new shape" of the placement decision as
four lines, and those four lines contain no empty-pool guard. §4 row (e) says
only "guard every site that indexes `pool` instead". Everywhere else the plan
names the file, the line and the code; here it names neither the site nor the
form.

**Where.** Plan §3.2, §4 row (e), §9.3.
`app/src/Randomizer/EnemyRandomizer.cpp:235-238` (`DrawCandidate`, the only
function that indexes `pool` by a random index), called from `:620`, `:624`,
`:631`, `:647`, `:654`.

**Why it matters.** "Every site that indexes `pool`" has at least three
readings, and they are not equivalent. Guarding inside `DrawCandidate` means
inventing a sentinel `PoolEntry` that then flows through `LookupModelSize("")`,
both reroll loops and `ContainsAny` before `enemyModelIndex.find("")` happens to
drop it — that works, but by accident, and nothing would document it. Guarding
the five call sites is five chances to miss one. The plan itself calls a missed
guard "the worst possible trade", and §6.5's failure list says a crash there
"would mean the draw-site guard was missed" — so the plan knows the stakes and
still leaves the decision to the implementer.

Worth noting for whoever addresses this: the reference's own gate is a *single
per-placement test at the write* — `if (changeData && enemyDataRandomized.Count
> 0)`, `reference/Randomizer/MainWindowComponents/RandomizeFunctions.cs:322` —
not a guard at the draw. Since §9.3's whole justification is "match the
reference", the shape of the match is worth naming rather than leaving open.

**Class.** Correction — under-specified against the plan's own standard, not
wrong.

**Confidence.** High.

### 2.3 Should fix — §5.1 claims the mirror pins a rule that none of §6.2's ten cases models

**What is wrong.** §5.1 discharges the flag-off behaviour change with "this
change is verified by (a) the Python mirror pinning the rule, (b) code review of
four lines". None of the ten selftest cases in §6.2 models the placement
decision. Cases 1-4 are pool arithmetic, case 5 is the baked table, cases 6-10
are data and fidelity pins — which placements match which pattern, where the six
live, that the six appear in the reference source. Not one of them expresses "a
forced placement bypasses both the exclusion test and the zone roll".

**Where.** Plan §5.1 ("Why it is not hardware-observable, and what is done
instead"), §6.2 cases 1-10.

**Why it matters.** The m28 override is the part of this feature that changes
already-shipped output, and it is the part with no automated coverage at all
until after a hardware run feeds §6.4. That is a defensible position; §5.1
states a stronger one. The precedent for doing better exists in this repository:
`app/tools/ui_scroll_verify.py` mirrors `Controls.cpp`'s arithmetic line for
line precisely because no C++ can be run here, and §3.2's decision is four lines
of equally mirrorable logic. A case asserting that the 12 forced placements
randomize at any `zoneChance`, and that a flag-on excluded placement does not,
would pin the rule the plan says is pinned.

**Class.** Correction.

**Confidence.** High — derived by reading §6.2's ten cases against §3.2's four
lines, not by running anything.

### 2.4 Should fix — `enemy_lookup.py frozen --bell` will not print 662

**What is wrong.** §6.3 gives
`python tools/enemy_lookup.py frozen ../data/vanilla/dvdroot_ps4 --bell` with
the expected result "662 frozen", and says "`608 + 54 = 662` is the arithmetic
the last line checks by inspection". With only the changes §4 lists for
`enemy_lookup.py`, that line will still read 608.

**Where.** Plan §6.3 and §4's `app/tools/enemy_lookup.py` row.
`app/tools/enemy_lookup.py:202-203` — `cmd_frozen`'s summary is
`print("total frozen placements: %d by the reference's list, %d by ours" %
(totals["reference"], totals["ours"]))`. A new `("bell", pattern)` kind returned
by `exclusion_reason` is counted into `totals["bell"]` and then dropped; only
the per-map `len(rows)` lines would include it.

**Why it matters.** Small in itself, but it is one of only four pre-hardware
verification commands in §6.3. The implementer will run it, see 608, and have to
work out whether the feature is broken or the tool is. §4's `enemy_lookup.py`
row lists four changes and this is not among them.

**Class.** Correction.

**Confidence.** High — read directly from `cmd_frozen`.

### 2.5 Should fix — row index 4 is positional in three parallel arrays, and §4 says so for only one of them

**What is wrong.** §9.1 and §4 are precise about `SetupDefaultsScreen.h`
("insert the new row constant at index 4 ... and renumber the ten constants
below it") but describe the item lists as "add the ... entry to `DrawList`'s
`items`" and "entry in **both** `DrawSaveData` and `DrawConfirm` item lists".
Those lists are parallel arrays whose *position* is the row index; an entry
appended rather than inserted at position 4 compiles, passes
`ui_scroll_verify.py`, and mislabels every row from 4 down.

**Where.** Plan §4 rows for `SetupDefaultsScreen.cpp` and
`EnableWizardScreen.cpp`, and §9.1.
`app/src/UI/SetupDefaultsScreen.cpp:212-226` (`DrawList`'s `items`),
`app/src/UI/EnableWizardScreen.cpp:633-651` (`DrawSaveData`) and the matching
`DrawConfirm` list — all three consumed by `DrawMenuList`
(`app/src/UI/Controls.cpp:10-17`), which pairs `items[i]` with `selected == i`.

**Why it matters.** The symptom is "the cursor is on RANDOMIZE BOSSES but X
toggles bell maidens" — obvious on hardware, invisible to every check in §6.2 to
§6.4, so it costs a whole hardware cycle. One sentence prevents it. This is a
cost the row-4 decision creates that appending would not have; the existing
`kDisableMergoDarknessRow` comment in both screen files
(`SetupDefaultsScreen.h:48-50`, `EnableWizardScreen.cpp:33-35`) records exactly
that reasoning. §9.1 overrides it on the developer's instruction, which is fine
— the consequence just needs carrying into §4.

**Class.** Correction.

**Confidence.** High.

### 2.6 Worth considering — the backlog row will still say "Trivial. Three strings appended"

**What is wrong.** §4 puts `docs/randomization-feature-spec.md` on the
"explicitly not changed" list. Row 16 currently reads *"**Trivial.** Three
strings appended to the exclusion list"* with status **TODO** — a description
this spec and plan have shown to be wrong about the decision content, and the
row every future reader uses to judge what row 16 costs.

**Where.** Plan §4 "Explicitly not changed";
`docs/randomization-feature-spec.md:134`.

**Why it matters.** CLAUDE.md §8 makes status lines load-bearing and says a
plan's status line and the spec table row are updated in the same pass; four
stale plans have already misled readers on this project. Stage 6 of
`ai-dev-process.md` does list "specification status update" among its outputs,
so this may be correctly deferred rather than missed — but the plan does not say
which, and "explicitly not changed" reads as a decision that it never happens
here.

**Class.** Assumption.

**Confidence.** Medium — it turns on whether stage 6 owns the backlog row, which
no artifact states.

## 3. What the plan got right

Calibration matters more than the list above, so: this is a strong plan, and
several things I went looking for were already answered.

- **A2 is the right rejection for the right reason, and its numbers are real.**
  Re-derived independently: 2877 enemy parts across the 24 base maps, 608
  matched by `EnemyExclusionList()`, 2269 reaching the roll. The `changeData`
  restructure would indeed inject 608 extra `RandInt` calls and reshuffle every
  seed. While checking it I also traced the reference's `cc` flag and it cannot
  change any outcome in a base map, which makes the port's
  `continue`-before-roll a pure RNG-stream difference — exactly what §3.3
  assumes.
- **§3.2 property 3 is the subtle one and it is correct.** I expected the plan
  to apply the m28 override to the pool as well as to the placements. It does
  not, and `GenerateEnemyList`
  (`reference/Randomizer/MainWindowComponents/MainWindow.xaml.cs:734-790`)
  confirms the reference applies `noNoList` plainly there with no override. That
  is what makes §6.4's headline assertion deterministic, and the plan says so
  explicitly.
- **The "no default argument" choice on `IsExcludedEnemyName` is right, and
  cheap.** There are exactly two call sites in `app/src`
  (`EnemyRandomizer.cpp:373`, `:611`) — verified by grep, including that the
  boss path never consults it — so the cost is two edits and the benefit is that
  a third site cannot silently inherit flag-off behaviour.
- **The picker-table hazard (§5.3) is correctly identified as the one way this
  does real damage.** `pool_verify.py`'s `cmd_table` compares row for row
  against `engine_pool_models`, so an 80-row regeneration would pass its own
  check while remapping all 82 saved characters. Keeping the table at 82 and
  adding §6.2 case 5 to assert it is *not* compared against the flag-on pool is
  the right shape.
- **§5.1 is honest in the way this project asks for.** "No single output tree
  can distinguish the two builds" is true, and labelling §6.5 step 7 a
  regression check rather than a confirmation is exactly the distinction
  CLAUDE.md §3 demands. My only complaint is the mirror claim in 2.3.
- **The six forced names, their order and their provenance all check out
  verbatim**, as do the reference line numbers cited for them, and I found no
  second m28 override that the plan missed (the other two `Contains("m28")`
  blocks in `RandomizeFunctions.cs`, at `:1677` and `:2141`, are about `c2100`
  in the boss path).
- **`_load_list` really will parse the two new arrays unchanged**, given they
  are placed beside the existing one and use the local name `kList` — checked
  against `enemy_lookup.py:57-75`'s `find`/`rfind`/`split` logic rather than
  assumed.

## 4. What this review could not check

- **Everything that requires the console.** Whether a frozen maiden's bell still
  summons and resurrects (spec §4's assumption, §6.5 step 5), whether the six
  changed Yahar'gul placements look right in play, and whether §9.3's empty-pool
  case completes cleanly rather than crashing. Nothing in this environment runs
  the game or the `.pkg`.
- **The build.** I did not run `make`; `OO_PS4_TOOLCHAIN` and the lld-18.1.8
  link step are outside what this pass touched, so "the plan compiles" is
  unverified. §6.1's clean-rebuild requirement is correctly stated —
  `RandomizerDefaults` and `EnemyRandomizerOptions` both gain a member.
- **The flag-off RNG-stream claim, end to end.** §3.2 property 1 and §5.1's ~11%
  are arithmetic I re-derived, not something I simulated. I did not replay
  `std::mt19937` against the engine to confirm that a fired override shifts
  every subsequent map; that follows from one seeded generator and I took it on
  reasoning.
- **`event/m28_00_00_00.emevd.dcx`.** The spec's claim that all 15 Yahar'gul
  maiden entity IDs appear in the event script is stage-0 evidence I did not
  re-derive; nothing in the plan depends on it.
- **Not investigated:** `BossRandomizer.cpp`, `TreasureRandomizer.cpp`,
  `DropRandomizer.cpp` and the param path, beyond confirming by grep that
  `IsExcludedEnemyName` has no call sites there; `Msb/` beyond the three fields
  the plan writes; and the reference's `oopsAll` path.

## 5. Verification of the plan's own numbers

Re-derived from `data/vanilla/dvdroot_ps4` (checked first for `.bak` siblings
per CLAUDE.md §5 — there are none) by driving `app/tools/enemy_lookup.py`'s own
`engine_pool` / `enemies` / `exclusion_reason` from a scratch script, with
`exclusion_reason` monkey-patched to model the flag-on list.

| Claim | Plan says | Reviewer measured | Agrees? |
|---|---|---|---|
| Enemy parts, 24 base maps (§3.3 A2) | 2877 | 2877 | yes |
| Matched by `EnemyExclusionList()` today (§3.3 A2) | 608 | 608 | yes |
| Reaching the zone roll today (§3.3 A2) | 2269 | 2269 | yes |
| Flag-off pool (§6.2 case 1) | 333 entries, 82 models | 333, 82 | yes |
| Flag-on pool (§6.2 case 2) | 317 entries, 80 models | 317, 80 | yes |
| Models lost (§6.2 case 3) | exactly `c1050`, `c1051` | exactly those two | yes |
| Baked weights of those rows (§6.2 case 4) | 4 and 12, sum 16 | `EnemyPoolTable.h:49-50` gives 4 + 12 = 16; 333 − 317 = 16 | yes |
| Maidens newly frozen (§6.3) | 54, so 608 + 54 = 662 | 54 (31 `c1050*`, 23 `c1051*`), none already matched by the base list | yes (but see 2.4 — the tool will not print it) |
| `c1055` placements (§5.4, §6.2 case 7) | zero | 0 across all 43 `.msb.dcx` files | yes |
| Forced placements (§6.2 case 8) | 12, all in m28, all `NPCParamID 105810` | 12, six per m28 map, all 105810, entities 2800520-2800529 | yes |
| Forced positions (§5.1) | 17-22 of 145 and 19-24 of 147 | 17-22 of 145 (`m28_00_00_00`), 19-24 of 147 (`m28_00_00_01`) | yes |
| m28 walk order (§5.1) | 17th and 18th of 24 | 17 and 18 | yes |
| Non-forced maidens in retail-loaded maps (§6.4) | 28 of 42 | 34 retail-loaded maidens − 6 forced = 28 | yes |
| Yahar'gul zone chance (§2.2) | 100, `EnemyRandomizer.cpp:114-115` | 100 at lines 114-115; `FieldContainer.cs:24` `YahargulChance = 100` | yes |
| Odds (§5.1) | 0.119 expected, ~11% at least one | 12/101 = 0.1188; 1 − (100/101)^12 = 0.1126 | yes |
| Progress line fit (§3.4) | 61 chars, 27 px per char of 1920 | "RANDOMIZED 2269 ENEMIES ACROSS 24 MAPS BELL MAIDENS UNCHANGED" = 61 chars; `kAdvance = 9` × scale 3 = 27 px; 1647 px, centred | yes |
| Config buffer headroom (§4) | buffer 1024, worst case about 450 | 446 bytes with a full 82/17 picker string and a 10-digit seed | yes |
| `IsExcludedEnemyName` call sites (§2.2) | exactly two, `:373` and `:611` | exactly two in `app/src` | yes |
| Reference line numbers (§2.2, §3.1) | exclusion `:26`, override `288-320`, write gate `:322` | `:28`, `289-320`, `:322` | close enough; two block starts cited one or two lines early, no consequence |

Every measured number in the plan reproduced. That is unusual, and worth saying
plainly.

---

<!--
Not in this document, on purpose:

  - edits to the plan (§3.3 — reviewers do not modify what they review)
  - production code
  - a rewritten version of the approach
-->
