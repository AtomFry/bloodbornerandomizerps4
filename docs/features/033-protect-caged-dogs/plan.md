# Plan 033 — Protect Caged Dogs (`DO NOT RANDOMIZE CAGED DOGS`)

**Status: Approved**

**Spec:** `docs/features/033-protect-caged-dogs/spec.md` — **APPROVED**
(2026-09-19). Ten binding decisions, §10 D1–D10.

**Evidence:** `docs/features/033-protect-caged-dogs/plan-evidence.md` — the
measurements, traces and rejected alternatives behind this plan.

**Plan review:** `docs/features/033-protect-caged-dogs/plan-review.md` — added
during review.

---

> **This document is the implementation contract.** §1–§7 are what the
> implementer reads, in order. §8–§10 are the record, not instructions.
> `plan-evidence.md` holds the investigation; `log.md` holds the history.

---

## 1. Objective

Add one on/off setting, off by default, that stops enemy randomization
re-targeting the ten caged Shaggy Hunting Dog placements — six in the Central
Yharnam kennel yard, four in the Forbidden Woods cage cluster — which live in
twenty-six placement records across five map files. Every other placement, the
replacement pool, and the behaviour of the setting when off are unchanged.

---

## 2. Approved behaviour

| #   | Behaviour | Source |
| --- | --------- | ------ |
| B1  | One new on/off setting, label `DO NOT RANDOMIZE CAGED DOGS`, off by default, on the same settings list as `RANDOMIZE ENEMIES` on both screens | spec §2, §10 D2, D7 |
| B2  | With it on, a protected placement is not re-targeted: it keeps its vanilla model and behaviour row | spec §2 |
| B3  | The protected set is the six Central Yharnam placements in each of `m24_01_00_00`, `m24_01_00_01`, `m24_01_00_11` and the four Forbidden Woods placements in each of `m27_00_00_00`, `m27_00_00_01` — twenty-six records, ten dogs a player meets | spec §6, §10 D9 |
| B4  | The set is identified by verified placement identifiers — not by the creature, not by a stat row, and not by a placement-name substring | spec §7, §10 D3 |
| B5  | With it off the run is identical to today's, roll for roll | spec §7 |
| B6  | `ENEMIES SKIPPED` is untouched and wins wherever it applies; this setting never weakens, overrides or reorders it | spec §10 D4, §2 truth table |
| B7  | Protected placements still contribute to the replacement pool, which stays at 333 entries across 82 models | spec §7, §10 D6 |
| B8  | Zone stat scaling still rewrites a protected placement's stat row. "Unchanged" means "not re-targeted", never "byte-identical to vanilla" | spec §10 D8, §4 F11 |
| B9  | With it on, twenty-six placements leave the roll stream, so the same seed is a different world everywhere. This is intended | spec §2 |
| B10 | The setting only means anything when `RANDOMIZE ENEMIES` is on | spec §2 |
| B11 | The state persists in the existing `defaults.cfg`; an older file without the key loads with the setting off | spec §7, §10 D2 |
| B12 | A check recomputes the protected set from `data/vanilla/dvdroot_ps4` and fails if the shipped identifiers and the data disagree | spec §6, §8, §10 D3 |
| B13 | No user-facing text for this setting uses the words "HUNTING DOG" | spec §10 D7 |

---

## 3. Constraints and invariants

### 3.1 Must be preserved

* **`StepReadMap`'s contribution loop is not touched.** Any test added there
  changes the pool and therefore every seed's world; spec D6 requires the pool
  to stay at 333 entries across 82 models.
* **The RNG stream is unchanged when the setting is off.** The option must be
  tested before anything else the new gate does, so an off run executes the
  same sequence of `RandInt` calls it does today.
* **The existing order of tests in `StepWriteMap` does not move**: `forced`,
  then the fixed exclusion list, then `ENEMIES SKIPPED`, then the roll. The new
  test is inserted between the third and fourth; nothing is reordered.
* **The m28 forced-maiden override still outranks everything.** The new test
  sits inside the same `!forced &&` guard as the two tests above it.
* **`ENEMIES SKIPPED` semantics are unchanged** — row 32 shipped and was
  hardware-tested on 2026-09-19.
* **No placement's name, entity ID, position or collision index is ever
  written.** The enemy pass writes model index, behaviour row and stat row and
  nothing else; this feature writes nothing at all.
* **`EnemyPoolTable.h` and `EnemySkipTable.h` are not regenerated.** Their order
  is positional in saved configuration; a regenerated order silently remaps
  every saved selection.
* **`defaults.cfg` stays forward and backward compatible**: unknown keys are
  ignored on load, and an absent key reads as `false`.
* **Layering**: the new list header and the gate live under
  `app/src/Randomizer/`; no SDL2 there, and no UI file learns a map name or an
  entity ID.

### 3.2 Out of scope

* Any change to `ENEMIES SKIPPED`, `ENEMIES INCLUDED`, or their tables.
* Adding the creature to any global exclusion list.
* Special-casing the replacement creatures that misbehaved.
* `c1240_0007`, the seventh Central Yharnam yard dog (spec §10 D5), and any
  other constrained placement that is not one of the ten.
* The boss, treasure, drop, starting-weapon and Mergo passes.
* Diagnosing the lag and the unkillable replacements (spec §4 H1).
* Chalice dungeons.

### 3.3 Hazards

| Hazard | What to do | Evidence |
| ------ | ---------- | -------- |
| A settings row whose constant and `items` entry disagree compiles, passes `ui_scroll_verify.py` and mislabels every row below it | The row is appended last (§9 P5), so no existing index moves. Append to all three `items` vectors and set both counts in the same pass, and change no existing row constant | `plan-evidence.md` §E5.1 |
| Struct layout changes without a clean rebuild have produced a heap-corruption `SIGSEGV` on hardware | `make clean && make` in both milestones — both add a field to a struct the UI also holds | `docs/build.md` §"Clean builds after layout changes" |
| Matching these placements by name substring would freeze 58 placements across nine map files | Match on entity ID with a map-prefix guard only. Never add these names to any name-matching list | `plan-evidence.md` §E4 M6 |
| Two Central Yharnam dogs carry entity ID `-1`, and `-1` is common across the maps | The lookup returns false for any entity ID `<= 0`, before it compares anything | `plan-evidence.md` §E4 M7 |
| `pool_verify.py`'s worst-case `defaults.cfg` case is an exact equality and will fail the moment the new key is added | Update that arithmetic in the same milestone that adds the key | `plan-evidence.md` §E4 M9 |
| A hand-written EMEVD reader can silently mis-frame a file and still produce plausible output | Run both framing checks per file (§6) and halt if either fails | `plan-evidence.md` §E5.4 |
| Both cage areas hold more cage props than caged dogs, so an empty cage on hardware is not a miss | Central Yharnam has 13 cage props and 6 caged dogs; the Forbidden Woods has 6 and 4. Say so in the hardware handoff | `plan-evidence.md` §E4 M10 |

---

## 4. Implementation approach

> Chosen: a hard-coded ten-entry table of (map prefix, entity ID), consulted by
> a new gate in the write loop. Alternatives considered and why they were
> rejected: `plan-evidence.md` §E3.

### 4.1 The protected set

These ten identifiers are the feature's data. Each matches one placement in
each map file whose name contains the prefix, giving twenty-six records.

| Map prefix | Entity ID | Vanilla placement | Where |
| ---------- | --------: | ----------------- | ----- |
| `m24_01` | 2410271 | `c1240_0004` | Central Yharnam, breakout pair |
| `m24_01` | 2410272 | `c1240_0005` | Central Yharnam, breakout pair |
| `m24_01` | 2410275 | `c1240_0008` | Central Yharnam, penned |
| `m24_01` | 2410277 | `c1240_0010` | Central Yharnam, penned |
| `m24_01` | 2410278 | `c1240_0011` | Central Yharnam, penned |
| `m24_01` | 2410279 | `c1240_0012` | Central Yharnam, penned |
| `m27`    | 2700301 | `c1240_0001` | Forbidden Woods cluster |
| `m27`    | 2700302 | `c1240_0002` | Forbidden Woods cluster |
| `m27`    | 2700308 | `c1240_0004` | Forbidden Woods cluster |
| `m27`    | 2700309 | `c1240_0005` | Forbidden Woods cluster |

The placement-name column is documentation only. The eight names are reused by
unrelated dogs in other maps and must never be matched (§3.3).

They live in a new header `app/src/Randomizer/CagedDogList.h`, alongside
`EnemySkipList.h` and `EnemyExclusionList.h` and deliberately separate from
both: those two match names game-wide, this one matches an entity ID inside a
named area. The header exposes the table and one predicate taking the map name
and an entity ID, which returns false for any entity ID `<= 0` and otherwise
requires both an entity-ID match and the map name to contain the entry's
prefix. The predicate is the only way the rest of the engine asks the question.

### 4.2 Where the gate goes

In `EnemyRandomizer.cpp`'s `StepWriteMap` placement loop, immediately after the
`IsSkippedName` test and before `int roll = RandInt(0, 100);`, inside the same
`!forced &&` guard as the two tests above it. It reads the option first, then
`part_fields::GetEntityID(partBlob)`, and `continue`s on a match.

That position is what delivers B9 (no randomness drawn), B6 (`ENEMIES SKIPPED`
has already claimed anything it covers) and B5 (off costs one boolean compare).

Count the protected placements in `EnemyRandomizerResult` and emit one per-run
text-log line naming the count when the option is on. No progress-screen line
and no commit-screen counter (§9 P8).

### 4.3 What does not change in the engine

`StepReadMap` is not edited. A protected placement contributes to the pool
exactly as it does today, which is what keeps the pool at 333 entries and keeps
stat variants `124401` and `124501` in circulation (B7). `BossParamScaling`
runs after the enemy pass and is not edited either (B8).

### 4.4 The settings chain

One boolean, following `enableMergoDarkness` site for site:

* `RandomizerDefaults::doNotRandomizeCagedDogs`, default `false`.
* `defaults.cfg` key `do_not_randomize_caged_dogs`, in both load and save.
* `EnemyRandomizerOptions::doNotRandomizeCagedDogs`, assigned in
  `EnableWizardScreen::StartCommit`.
* A per-run member on `EnableWizardScreen`, seeded from the defaults in the
  constructor and not written back, like every other toggle there.
* A row on both settings screens.

The setting is a modifier on `RANDOMIZE ENEMIES`, so — exactly as
`randomizeWorkshopTools` is for treasure — it must **not** join the big `||` in
`StartCommit` that decides whether a run has anything to do, and gets no
`SKIPPING …` progress line.

**Row position.** The new row is **appended last** on both screens, at index
15, and **no existing row constant changes** (§9 P5). `kItemCount` and
`kSaveDataRowCount` go 15 → 16. Rows 0–2 differ between the two screens
(Setup Defaults: title ID, backup, default replace; wizard: backup, replace,
seed); from row 3 both screens read:
3 `RANDOMIZE ENEMIES`, 4 bosses, 5 treasure, 6 workshop tools, 7 drops,
8 starting weapons, 9 starting guns, 10 shop weapons, 11 mergo darkness,
12 enemies included, 13 enemies skipped, 14 bosses included,
**15 `DO NOT RANDOMIZE CAGED DOGS`**.

The setting therefore sits away from `RANDOMIZE ENEMIES`, the toggle it
modifies. That cost was accepted deliberately to buy the guarantee that no
existing row can be mislabelled by this change.

**Row text.** `DO NOT RANDOMIZE CAGED DOGS   YES` is 33 characters; the list
draws at scale 4, which fits 53 characters across. No other user-facing string
is added: there is no in-app help text for this setting (§9 P11).

### 4.5 The verification mirror

A new tool `app/tools/caged_dogs_verify.py`, because the recomputation needs
object parts and an EMEVD reader that `pool_verify.py` has no business owning.
It reuses `enemy_lookup.load_base_maps` / `enemies`, `boss_verify.read_dcx`,
and `pool_verify`'s frozen definition, and parses the ten identifiers out of
`CagedDogList.h` rather than restating them.

It recomputes the set three independent ways and asserts all three agree with
the shipped header (§6).

### What this reuses

| Existing code or tool | How it is used | Change needed |
| --------------------- | -------------- | ------------- |
| `app/src/Msb/Msbb.h` `part_fields::GetEntityID` | reads the placement's entity ID in the gate | reuse as-is |
| `app/src/Randomizer/EnemyRandomizer.cpp` `StepWriteMap` | host for the new gate | extend |
| `app/src/Randomizer/EnemySkipList.h` | the shape the new header is modelled on and deliberately separate from | none |
| `app/src/UI/*Screen.*`, `RandomizerDefaults*` | the boolean settings chain, copied from `enableMergoDarkness` | extend |
| `app/tools/enemy_lookup.py` | `load_base_maps`, `enemies`, `zone_scaled_npc` | reuse as-is |
| `app/tools/pool_verify.py` | `_frozen`; and its worst-case config arithmetic | extend |
| `app/tools/boss_verify.py` | `read_dcx`, `_i32`, `_i64`, `_utf16z` | reuse as-is |
| `app/tools/ui_scroll_verify.py` | per-screen row counts | extend |
| `data/runs/20260919-Enemies Only/` | the seed-1234567890 pre-feature baseline tree for the off-is-a-no-op diff | reuse as-is |

---

## 5. Files and changes

| File | Change | Milestone |
| ---- | ------ | --------- |
| `app/src/Randomizer/CagedDogList.h` | **new** — the ten identifiers and the predicate | 1 |
| `app/src/Randomizer/EnemyRandomizer.h` | `doNotRandomizeCagedDogs` option field; `cagedDogsProtected` result counter | 1 |
| `app/src/Randomizer/EnemyRandomizer.cpp` | include the new header; the gate in `StepWriteMap`; the per-run log line | 1 |
| `app/tools/caged_dogs_verify.py` | **new** — `list`, `protected`, `selftest` | 1 |
| `app/src/Randomizer/RandomizerDefaults.h` | `doNotRandomizeCagedDogs`, default `false` | 2 |
| `app/src/Randomizer/RandomizerDefaultsStore.cpp` | `do_not_randomize_caged_dogs` in load and in the save format string and its argument list | 2 |
| `app/src/UI/SetupDefaultsScreen.h` | new row constant at 15; **no existing constant changes**; `kItemCount` 15 → 16 | 2 |
| `app/src/UI/SetupDefaultsScreen.cpp` | `ToggleRow` branch; `DrawList` entry appended last | 2 |
| `app/src/UI/EnableWizardScreen.h` | per-run member | 2 |
| `app/src/UI/EnableWizardScreen.cpp` | new row constant at 15 in the anonymous namespace, **no existing constant changes**, `kSaveDataRowCount` 16, ctor init, left/right branch, X branch, `DrawSaveData` and `DrawConfirm` entries appended last, `StartCommit` option assignment | 2 |
| `app/tools/ui_scroll_verify.py` | the three 15-row screen entries become 16 | 2 |
| `app/tools/pool_verify.py` | worst-case `defaults.cfg` 555 → 585 bytes; a case asserting the new key is in load and save | 2 |

No build-system change: the Makefile globs `.cpp` recursively and the new file
is a header. Nothing invalidates an existing `defaults.cfg` — the new key is
simply absent from it and reads as off.

---

## 6. Verification

### Build

`cd app && make clean && make`, in **both** milestones. Both add a field to a
struct that the UI and the engine each hold a copy of, and the Makefile has no
header dependency tracking, so an incremental build can link mismatched object
files. The `.pkg` must be produced.

### Automated

| Check | Command | Asserts |
| ----- | ------- | ------- |
| Protected set recomputation | `python app/tools/caged_dogs_verify.py selftest data/vanilla/dvdroot_ps4` | the cases below |
| Protected set listing | `python app/tools/caged_dogs_verify.py list data/vanilla/dvdroot_ps4` | prints 26 rows for eyeballing; not a pass/fail gate |
| Failing direction | `python app/tools/caged_dogs_verify.py protected data/vanilla/dvdroot_ps4 "data/runs/20260919-Enemies Only/dvdroot_ps4"` | **must FAIL**, reporting 26 of 26 not frozen — that tree predates the feature and every protected placement was re-targeted in it |
| Pool and config mirrors | `python app/tools/pool_verify.py selftest data/vanilla/dvdroot_ps4` | 85 existing cases still pass, plus the updated config-size case |
| Pool unchanged | `python app/tools/pool_verify.py table enemy data/vanilla/dvdroot_ps4` | the baked 82-row table still matches the engine pool |
| Settings-list geometry | `python app/tools/ui_scroll_verify.py` | the 16-row lists still scroll and fit |

`caged_dogs_verify.py selftest` must assert:

1. The header parses to exactly ten entries, all distinct, with map prefixes
   drawn from `m24_01` and `m27` only.
2. Those ten match exactly **26** enemy placements across the 24 base maps —
   6 in each of the three `m24_01` files, 4 in each of the two `m27` files.
3. **Derivation A, cage proximity.** The enemy placements within 1.0 unit of an
   object part whose model is `o243010`, `o243011` or `o273011` are exactly
   those 26.
4. **Derivation B, nearest object per dog.** The `c1240` placements whose
   nearest object part of any model is within 1.0 unit are exactly those 26.
5. **Threshold independence.** The largest protected distance is 0.899 and the
   smallest unprotected one is 2.671, so every radius in that range gives the
   same answer. Assert the two bounds, not the radius.
6. **Derivation C, the cage scripts.** In `event/m24_01_00_00.emevd.dcx`, one
   event initialiser family names exactly the six Central Yharnam entity IDs
   over six instances; in `event/m27_00_00_00.emevd.dcx`, one names exactly the
   four Forbidden Woods entity IDs, each paired with the entity ID of the very
   cage object derivation A put it next to. Both files' framing is validated
   first, two ways each: the header's section offsets reconcile with the
   section sizes, and the per-event instruction counts sum to the file's own
   instruction total.
7. Every protected placement is a `c1240` whose vanilla stat row is one of
   `124400`, `124401`, `124501`, and none of them is matched by the fixed
   exclusion list.
8. The name-substring trap, pinned in the failing direction: matching the eight
   placement names as substrings catches 58 placements across nine map files,
   so the header must not be reachable by name.

`caged_dogs_verify.py protected <vanilla> <output>` must, for a given output
tree, report and assert: all 26 protected placements frozen — same model, same
behaviour row, and a stat row that is either the vanilla value or the
zone-scaled variant of it — and report how many of the other 70 `c1240`
placements changed, as a rate rather than an assertion.

A mirror pins the rules and not the C++ implementation of them. None of this
proves the engine consults the header correctly; only hardware does.

### Hardware

The implementer cannot run any of this.

**After milestone 1** — one run, no gameplay needed: seed **1234567890**,
`RANDOMIZE ENEMIES` on, everything else off, all 82 enemies included, nothing
skipped. Pass condition: the resulting `dvdroot_ps4` tree is byte-identical to
`data/runs/20260919-Enemies Only/dvdroot_ps4`. This is B5 for the off state,
and it is the only thing that can catch an RNG-stream shift.

**After milestone 2**, with the setting **on**:

1. Central Yharnam kennel yard: six dogs in cages behaving as they do in the
   unmodified game, including the two that come out, killable in the usual
   couple of hits. Empty cages in the yard are expected — vanilla has 13 cage
   props there and 6 caged dogs.
2. Forbidden Woods cage cluster: six cages, four holding dogs, two empty. The
   empty pair is vanilla.
3. The seventh yard dog, the rest of Central Yharnam and the other four
   Forbidden Woods dogs still changed.
4. Shaggy Hunting Dogs in Cathedral Ward and Yahar'gul still changed, and the
   creature still turns up somewhere new.
5. No frame-rate collapse in either cage area, and nothing in a cage that takes
   damage without dying. If either symptom persists, the identification is
   incomplete or spec §4 H1 is wrong — report it rather than adjusting the
   list.
6. One run with the setting **off**: both cage areas behave exactly as they do
   today, replacements and all.

Capture `live.log` and the output tree for runs 1 and 6 into `data/runs/`, so
`caged_dogs_verify.py protected` can be run against both.

---

## 7. Milestones and stop conditions

### Milestone 1 — the protected set and the engine gate

**Goal.** The engine can leave the twenty-six placements alone, and a verifier
proves the ten identifiers still describe the vanilla data. No way to turn it
on yet, so no behaviour changes.

**Changes**, in order:

1. Write `app/src/Randomizer/CagedDogList.h` with the ten entries of §4.1 and
   the predicate of §4.1 — done when the file compiles standalone and the
   predicate rejects entity IDs `<= 0` before any comparison.
2. Add `doNotRandomizeCagedDogs` to `EnemyRandomizerOptions` (default `false`)
   and `cagedDogsProtected` to `EnemyRandomizerResult` — done when every
   existing call site still compiles untouched.
3. Insert the gate in `StepWriteMap` at the position in §4.2, incrementing the
   counter — done when the option is the first thing tested and no `RandInt`
   call sits above it that did not before.
4. Emit the per-run count to the text log when the option is on — done when an
   off run logs nothing new.
5. Write `app/tools/caged_dogs_verify.py` with `list`, `protected` and
   `selftest` — done when `selftest` passes all eight cases of §6 and
   `protected` against `data/runs/20260919-Enemies Only/dvdroot_ps4` fails with
   26 of 26 not frozen.

**Invariants** this milestone must not break: the `StepReadMap` loop, the RNG
stream when off, the order of the existing `StepWriteMap` tests, the m28
override, and the no-writes rule for name/entity ID/position (§3.1).

**Verification:** clean build; `caged_dogs_verify.py selftest`;
`caged_dogs_verify.py protected` against the pre-feature tree (expected FAIL);
`pool_verify.py selftest` and `pool_verify.py table enemy` unchanged.

**Completion gate.** All of the above pass, the `.pkg` builds, and the
milestone is handed to the developer for the byte-identical seed-1234567890 run
in §6. Do not begin milestone 2.

### Milestone 2 — the setting

**Goal.** A player can turn the protection on, it persists, and both settings
screens show it.

**Changes**, in order:

1. Add the field to `RandomizerDefaults` with default `false` — done when a
   fresh struct reads off.
2. Add the `do_not_randomize_caged_dogs` key to both halves of
   `RandomizerDefaultsStore.cpp` — done when a `defaults.cfg` without the key
   loads with the setting off and a saved file round-trips it.
3. Add the new row constant at 15 in `SetupDefaultsScreen.h` and in
   `EnableWizardScreen.cpp`'s anonymous namespace, and set both counts to 16.
   **Change no existing row constant** — done when `git diff` on both screens
   shows no edit to any pre-existing `k*Row` value.
4. Append the row string last in all three `items` vectors
   (`SetupDefaultsScreen::DrawList`, `EnableWizardScreen::DrawSaveData`,
   `EnableWizardScreen::DrawConfirm`) — done when the three lists read
   identically top to bottom and each entry's index matches its constant.
5. Add the toggle branches: `SetupDefaultsScreen::ToggleRow`, and the
   left/right and X branches in `EnableWizardScreen::UpdateSaveData` — done
   when the row toggles from both input paths on both screens.
6. Add the wizard member, its constructor initialiser, and the assignment in
   `StartCommit` — done when the option reaches `EnemyRandomizerOptions`, and
   the setting is **absent** from `StartCommit`'s big `||`.
7. Update `ui_scroll_verify.py`'s three 15-row entries to 16 — done when it
   passes.
8. Update `pool_verify.py`'s worst-case config arithmetic to 585 bytes and add
   a case asserting the new key appears in load and save — done when its
   selftest passes with no other case changed.

**Invariants** this milestone must not break: row constants and list positions
agreeing, `ENEMIES SKIPPED` semantics, `defaults.cfg` compatibility, and the
layering rule that no UI file names a map or an entity ID (§3.1).

**Verification:** clean build; `ui_scroll_verify.py`; `pool_verify.py selftest`;
`caged_dogs_verify.py selftest` still passing.

**Completion gate.** All of the above pass, the `.pkg` builds, and the
milestone is handed to the developer for the six hardware tests in §6.

### Stop conditions

Halt and report rather than deciding, if any of these occur:

* the build fails in a way the plan did not anticipate, or fails only after a
  clean rebuild;
* `caged_dogs_verify.py selftest` fails any of its eight cases against the
  vanilla tree — in particular, if the three derivations disagree, the ten
  identifiers are wrong and no amount of adjusting the radius fixes that;
* the EMEVD framing checks fail on either file;
* `protected` against the pre-feature tree **passes** — the check cannot fail
  and is therefore worthless;
* any `pool_verify.py` case other than the config-size one changes state;
* the pool stops reporting 333 entries across 82 models;
* an implementation decision would contradict the spec or §2 — including any
  temptation to widen the set to the seventh Central Yharnam dog, to a stat row
  or to a collision surface;
* the change needs a file not listed in §5;
* a §3.1 invariant cannot be preserved.

---

## 8. Open questions

None. Both questions this section held were answered on 2026-09-19 and are
recorded in §9 as P11 and P12.

---

## 9. Decisions

| #   | Date | Decision | Taken by |
| --- | ---- | -------- | -------- |
| P1  | 2026-09-19 | The protected set is a ten-entry table of (map prefix, entity ID) in a new `CagedDogList.h`; matching is entity ID plus map prefix, never a placement name | planner |
| P2  | 2026-09-19 | The gate goes in `StepWriteMap` after the `ENEMIES SKIPPED` test and before the zone roll, inside the same `!forced &&` | planner |
| P3  | 2026-09-19 | `StepReadMap` is not touched, so protected placements keep contributing to the pool (spec D6) | planner |
| P4  | 2026-09-19 | Field and key are named for the label: `doNotRandomizeCagedDogs` / `do_not_randomize_caged_dogs` | planner |
| P5  | 2026-09-19 | The new row is appended last, at index 15 on both screens; no existing row constant changes and nothing is renumbered | developer |
| P6  | 2026-09-19 | A new tool `caged_dogs_verify.py` owns the recomputation; `pool_verify.py` and `ui_scroll_verify.py` get arithmetic updates only | planner |
| P7  | 2026-09-19 | The recomputation derives the set three independent ways and asserts threshold independence rather than a magic radius | planner |
| P8  | 2026-09-19 | The protection count goes to the text log only | planner, confirmed by developer |
| P9  | 2026-09-19 | Two milestones: engine and verifier, then the setting. Milestone 1's hardware test is a byte-identical diff against the seed-1234567890 baseline | planner |
| P10 | 2026-09-19 | Both milestones require `make clean && make`, because both change a struct layout the UI also holds | planner |
| P11 | 2026-09-19 | **No in-app help text.** The port has no per-row help facility on either settings screen, and adding one is a geometry change to every row. Spec D10's requirement is satisfied in `docs/user-guide.md` at the documentation stage, naming Central Yharnam and the Forbidden Woods and never the words "Hunting Dog". **This supersedes spec §10 D10 as written; the spec needs amending to match** | developer |
| P12 | 2026-09-19 | No progress-screen or commit-screen count line. The confirm screen already shows the setting as `YES`, and the honest figure of 26 would invite the wrong question from a player expecting 10 | developer |
| P13 | 2026-09-19 | The row-order cost of P5 — the setting no longer sits beside `RANDOMIZE ENEMIES` — is accepted in exchange for removing the renumbering hazard entirely | developer |

---

## 10. Changes during implementation

| Date | Change | Reason |
| ---- | ------ | ------ |
| 2026-09-19 | **Milestone 1: no deviations.** All five §7 changes built as written, in the files §5 names, with the names §4 gives. Four choices the plan did not settle are recorded in `implementation-report.md` §3: the gate is written `!forced && options.doNotRandomizeCagedDogs && IsProtectedCagedDog(...)` so it reads like its two siblings (behaviourally identical either way round); the per-run log line is emitted after the existing `enemy randomizer: done - …` summary in `StepEmevd`; `caged_dogs_verify.py` hard-codes the three cage object models as derivation A's input and also asserts the two cage-prop counts from §3.3; the new header's symbols are `CagedDogEntry` / `CagedDogList()` / `IsProtectedCagedDog()` | — |
| 2026-09-19 | **Milestone 2: no deviations.** All eight §7 changes built as written, in the files §5 names, with the field name and key §4.4 gives and the row at index 15 on both screens with no existing row constant touched. Three choices the plan did not settle are recorded in `implementation-report.md` §3 of the milestone-2 report: the new `RandomizerDefaults` field sits after `enableMergoDarkness` and before `lastSeed`; `RandomizerDefaultsStore.cpp`'s comment quoting the worst-case config size was updated 555 → 585 alongside the `pool_verify.py` case it refers to, since adding the key made the sentence false; the two new toggle log lines read `defaults: do not randomize caged dogs = YES` / `enable wizard: … = NO`, following the form every other toggle on those screens uses. A fourth, documentary: milestone 2's report is appended to the same `implementation-report.md` under its own title, and milestone 1's status line there was updated to record the hardware pass | — |
