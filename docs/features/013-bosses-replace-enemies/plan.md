# Plan 013 — Bosses Can Replace Enemies

**Status: QUESTIONS ANSWERED — awaiting developer approval**

**Spec:** `docs/features/013-bosses-replace-enemies/spec.md` — APPROVED 2026-09-28

**Evidence:** `docs/features/013-bosses-replace-enemies/plan-evidence.md` — the
measurements, traces and rejected alternatives behind this plan

**Plan review:** `docs/features/013-bosses-replace-enemies/plan-review.md` — added
during review

---

## Execution Strategy

*Approving this plan approves this strategy. `CLAUDE.md` §4: milestones describe
implementation structure; gates describe when a human stops to test.*

| | |
| --- | --- |
| **Structure** | 5 functional milestones (M5 is post-hardware removal work) |
| **Execution** | continuous through M1–M4; **gated before M5** |
| **Human test gates** | **required after M4** only — the Optional gate after M2 was declined by the developer (§9 D4) |
| **Intermediate verification** | `cd app && make` plus `pool_verify.py selftest`, `settings_ui_verify.py`, `ui_scroll_verify.py`, `boss_verify.py selftest`, `caged_dogs_verify.py selftest`, `easy_modes_verify.py selftest` and `gen_pool_table.py … --check` after every milestone |

* **After M2 — Optional.** The Bosses pane goes from 2 rows to 10 and becomes the
  first settings pane in the app that must scroll; the eight new rows and the two
  new pickers are also the first use of the number and named-choice kinds.
  `ui_scroll_verify.py` and `settings_ui_verify.py` check that arithmetically.
  Nothing in M3 depends on it, so this is offered, not required.
* **After M4 — Required.** M5 deletes **REPLACE ALL ENEMIES**, and the only thing
  that retires it is one hardware run with it on (spec §6). No build or static
  check can supply that result, so execution stops here, the developer runs the
  hardware test plan in §6, and the outcome is recorded in `log.md` before M5
  starts.

---

> **This document is the implementation contract.** §1–§7 are what the
> implementer reads, in order. `plan-evidence.md` holds the investigation;
> `log.md` holds the history.

---

## 1. Objective

Add an eighth-setting group under **Bosses** that lets boss identities be written
over ordinary enemy placements in the fourteen explorable areas, as a second pass
over each map after the enemy and boss passes. The player controls how many per
map, whether that number is a ceiling or a quota, which areas take part, which
bosses may arrive, whether the chosen placements are spread apart, and whether an
arriving boss keeps its own AI or the replaced creature's. A seventh control
replaces every eligible placement at once and is built to be deleted.

---

## 2. Approved behaviour

| # | Behaviour | Source |
| - | --------- | ------ |
| B1 | Eight settings, all in the **Bosses** category; setting 1 is the master switch and with it off none of the other seven does anything | spec §2 |
| B2 | With setting 1 off the pass writes nothing **and draws no randomness** — the tree is byte-identical to today's for the same seed and settings | spec §7, §8.1 |
| B3 | The pass is a **second pass** over each map, after the enemy pass and after boss assignment, and **before** the stat-scaling pass | spec §10 D-N, §8.16 |
| B4 | It writes only `NPCParamID`, `ThinkParamID` and the model reference on `Part.Enemy` blobs. Position, rotation and scale are read-only | spec §7, §8.17 |
| B5 | An **eligible target** is an enemy placement that the 103-pattern fixed exclusion list does not name, that **ENEMIES SKIPPED** does not tick, that **DO NOT RANDOMIZE CAGED DOGS** does not protect when it is on, and whose model name resolves. All three protections hold under **REPLACE ALL ENEMIES** too | spec §5, §10 D-C, §8.15 |
| B7 | No boss placement and no Hunter's Dream or Abandoned Old Workshop placement is ever a target; neither map is offered as an area | spec §8.13, §8.14 |
| B8 | **BOSS REPLACEMENTS PER MAP** is an integer 0–15, default 1. 0 means no insertion anywhere and the run succeeds | spec §2, §10 D-K, §8.6 |
| B9 | **EXACTLY N**: a participating map file gets exactly `min(N, its eligible-target count)` insertions — an equality, never more | spec §10 D-L, §8.3 |
| B10 | **UP TO N** (default): a per-placement draw whose probability is derived from `min(N, eligible targets)` and the eligible-target count **only**, capped at `min(N, eligible targets)`. Never from pool composition | spec §10 D-L, **D-M**, §8.4 |
| B11 | Narrowing **INSERTED BOSSES** to one boss changes **which** boss arrives and leaves the expected number unchanged | spec §10 D-M, §8.4 |
| B12 | **REPLACE ALL ENEMIES** replaces every eligible target in every ticked area; settings 2 and 3 and setting 7 have no effect on the result | spec §2, §8.5 |
| B13 | **AREAS INCLUDED**: 14 rows, all ticked by default. An unticked area is not visited. Nothing ticked means nothing happens, and the run succeeds | spec §10 D-A, §8.8 |
| B14 | **INSERTED BOSSES**: its own generated 22-row table and its own config key. `BossPoolTable.h` and **BOSSES INCLUDED** are untouched | spec §10 D-B |
| B15 | An empty **INSERTED BOSSES** selection **skips the pass silently** — no error, no warning, nothing refused at commit; the log records the skip | spec §10 D-G, §7, §8.7 |
| B16 | The insertion list is harvested from vanilla map data by the reference's own `GenerateBossList` rules, **excluding** the three de-eligibility tests the reference applies after the harvest, with Moon Presence (`c5400`) removed | spec §3, §4 |
| B17 | The run pool is drained **by identity** and refilled from the full enabled list when it empties, **across the whole run**, not per map | spec §10 D-G, §7 item 5 |
| B18 | **SPREAD BOSSES OUT** (default off) selects the chosen placements by greedy farthest-point over placement positions; at a count of 1 both states produce the same placement and consume the same number of draws | spec §10 D-F, D-N, §8.12 |
| B19 | **BOSS BEHAVIOUR** = OWN AI (default) writes the drawn identity's own `ThinkParamID`; = REPLACED ENEMY'S AI leaves the placement's existing `ThinkParamID` untouched | spec §10 D-J, §3 item 5, §8.11 |
| B20 | A map with fewer eligible targets than the quota inserts as many as it can, reports the shortfall, and **never fails the run** | spec §7, §8.18 |
| B21 | All eight settings round-trip through `defaults.cfg` and a world revision, and seed a new world from the DEFAULTS tab by the existing whole-struct copy | spec §10 D-H, §8.19, §8.21 |
| B22 | Three rows carry an explicit risk warning in their help text: setting 1 (may make the game unstable or crash), setting 4 (a test control expected to break the game), setting 8 (its second state may not work at all) | spec §10 D-O |
| B23 | **REPLACE ALL ENEMIES** ships commented in the `RuneProbe.h` shape — temporary, its question, its design, what retires it — and is deleted once one hardware run has answered | spec §6 |
| B24 | `char buf[1024]` in `FormatSettings` grows to **2,048**, and `pool_verify.py`'s exact-equality assertion is rebased, not removed | spec §10 D-I |

**One measured correction to the spec.** The insertion list holds **32 identities
across 22 models**, not 29 across 22. The model set and the 22-row picker are
exactly as the spec describes. Every acceptance criterion that names 29 reads 32;
nothing else changes. Derivation: `plan-evidence.md` §E4 M2.

---

## 3. Constraints and invariants

### 3.1 Must be preserved

* **A run with setting 1 off is byte-identical to today's, roll for roll** — every
  gate is tested before any `RandInt`, exactly as `doNotRandomizeCagedDogs` is.
  A single stray draw reshuffles every map after it on every seed.
* **The enemy pass, the boss pass and the easy-mode pass are unchanged** —
  `EnemyPoolTable.h` gains no boss row and `BossRandomizer.cpp` is not edited.
* **`BossParamScaling` still runs last in a map's step** — moving insertion after
  it would give inserted bosses their arena stats instead of the area's (B3).
* **Generated table row order is frozen** — `EnemyPoolTable.h`,
  `EnemySkipTable.h`, `BossPoolTable.h`, `TrickWeaponTable.h` and
  `LeftHandWeaponTable.h` must not be regenerated, reordered or renumbered.
  The two new tables' order is frozen from the moment they land.
* **`SettingKind::SaveChoice` and its verifier case are untouched** — the named
  choice is a new kind beside it, not a replacement for it
  (`SettingsModel.cpp:220`, `settings_ui_verify.py:893`).
* **The 103-pattern fixed exclusion list is not extended** — a placement this
  feature must avoid is a rule in this feature.
* **`pool_verify.py`'s settings-block assertion stays an exact equality.**
* **Layering** — the pass lives under `Randomizer/` and touches no SDL2; the UI
  reaches every setting only through `SettingsModel`'s accessors.

### 3.2 Out of scope

* Backlog **row 14** (`lesserBossesBool`), and any change to `BossPoolTable.h`.
* Chalice dungeons and the reference's fifteenth area flag `bossChalices`.
* A size gate on this pass; the reference has none.
* Any change to the stat-scaling pass, including the seven unscaled identities.
* Writing positions, rotations or scales. Nothing moves.
* Navmesh, region or traversal-aware distance.
* Re-expressing **SAVE DATA** as the new named-choice kind.
* Raising the maximum count above 15.
* Correcting `docs/plans/pickers.md` §7.5, `032/spec.md` §8 or row 17's backlog
  description — report only; the documentation stage owns them.

### 3.3 Hazards

| Hazard | What to do | Evidence |
| ------ | ---------- | -------- |
| The pass draws randomness when the feature is off, silently changing every existing seed | Test `bossesCanReplaceEnemies`, the picker emptiness and the area gate **before** the first draw, and assert criterion 1 as a tree diff in M4 | §E5.1 |
| Deriving the per-placement probability from pool composition instead of from N — the failure D-M forecloses | Compute `p` from the count and the map's eligible-target count only. `insert_bosses_verify.py` measures the single-boss mean against the all-22 mean | §E5.2 |
| `SettingDef` gains five members and 21 existing rows are aggregate-initialised | Declare every new member **after** `help`; trailing initialisers are value-initialised, so no shipped row changes | §E2.4 |
| `CountChangedSettings` decides bool-versus-selection on `def.flag`, so a number setting would be counted as neither and the history line would under-report | Add an explicit number case in `WorldEditorScreen.cpp` in M2 | §E2.5 |
| Three of the four new kinds break `settings_ui_verify.py`'s parser (`&RandomizerDefaults::` matches an int member; `len(selections) == 5`) | Extend the parser and the three affected cases in the same milestone as the kinds | §E2.6 |
| The settings buffer is shared with features 011 and 027, both approved and unlanded | §5 carries the arithmetic for every landing order; whoever lands last rebases | §E4 M8 |
| Greedy farthest-point can see two placements at the same coordinates | Compare squared distances in `double`, never divide, break ties on the lower index | §E4 M5 |
| The identity dedupe order decides which identity a given draw returns | Freeze it as first-seen over `kBaseMaps` order, and have the mirror use the same order | §E4 M2 |
| The model the pass writes must exist in that map's Models section | `StepMergeModels` already merges the union of all 24 base maps' enemy models into every map, unconditionally, and all 22 insertion models are in it; if the lookup still fails, leave the placement alone and log | §E4 M3 |

---

## 4. Implementation approach

> Chosen: a new `Randomizer/BossInsertion.{h,cpp}` pass called from inside
> `StepWriteMap`, with its candidate list harvested during the existing
> `StepReadMap` walk. Alternatives considered and why they were rejected:
> `plan-evidence.md` §E3.

**The pass, per map file, in order.** `ApplyBossInsertion(mapName, msbb, options,
pool, rng, counts)`:

1. Return immediately, before any draw, if the master toggle is off, if the
   insertion pool is empty, if the map's area is not ticked, or if the count is 0
   and the override is off.
2. Collect the map's **eligible targets** (B5) into a vector of part indices, in
   parts order, reading each one's position from the blob.
3. Decide **k**:
   * override on → `k = |E|`;
   * **EXACTLY** → `k = min(N, |E|)`, no draws;
   * **UP TO** → `cap = min(N, |E|)`, then one `RandInt(0, |E| - 1)` per element
     of `E` in order, converting while `k < cap` when the draw is `< cap`. The
     probability is therefore `cap / |E|` exactly, from the count and the supply
     and nothing else (B10).
4. Choose **which k**:
   * override on → all of `E`, no draws;
   * spacing off → `k` draws without replacement over `E`
     (`RandInt(0, remaining - 1)`, swap-and-shrink);
   * spacing on → one `RandInt(0, |E| - 1)` for the first pick, then `k - 1`
     deterministic greedy farthest-point picks, maximising the minimum squared
     distance to the already-chosen set, lowest index winning a tie.
     At `k == 1` this is one draw and the same index as spacing off (B18).
5. For each chosen placement, in the order step 4 produced: draw an identity from
   the run pool, resolve its model to that map's model index, write
   `NPCParamID`, write `ThinkParamID` **only** when BOSS BEHAVIOUR is OWN AI
   (B19), write the model index, count it, and log the placement the way the
   enemy loop logs one.

**The run pool.** `InsertionPool { std::vector<BossIdentity> entries, refill; }`.
`refill` is the **full** enabled identity list, not a model-deduped one — that is
what makes "any two identities differ by at most one use" true (B17, §8 criterion
10). Draw = `RandInt(0, entries.size() - 1)`, erase that entry, and when
`entries` empties assign `refill` to it.

**The harvest.** In `StepReadMap`'s existing parts loop, behind the master
toggle, apply `InsertionEligible()` — `IsBossName`, then the reject-NPC set,
`EntityID == -1`, `NPCParamID` 0 or -1, `ThinkParamID == -1`, `c5070`, and
`c3060` with `NPCParamID != 210306016`. It deliberately omits the three further
tests `BossRandomizer.cpp`'s `PoolEligible` applies — `!m26 && c0000_0005`,
`m34 && npc == 210030`, `m28 && c2100` — because in the reference those run
*after* the harvest line (B16). Then drop `ThinkParamID == 1`, drop model
`c5400`, apply the **INSERTED BOSSES** selection, and append if the
`(npc, think, model)` triple is not already present. `StepBuildPool` copies the
result into `refill` and logs the size.

**Where it is called.** In `StepWriteMap`, after the enemy loop and after
`ApplyEasyModes`, before the `BossScalingMaps()` loop (B3). The easy-mode pass
stays the last writer of its own placements because every placement it touches is
on the fixed exclusion list and so is never an insertion target.

**Positions.** `part_fields::GetPosition(blob)` returns the three little-endian
`float32` at `entry + 0x28`. Read-only, fixed offset, nothing else decoded.

**Areas.** `InsertAreaTable.h`, 14 generated rows, each carrying the area's
**map-name prefix** in `ModelPoolEntry::model` — `m22`, `m23`, `m24_00`,
`m24_01`, `m24_02`, `m25`, `m26`, `m27`, `m28`, `m32`, `m33`, `m34`, `m35`,
`m36`. They are mutually exclusive, match exactly the 22 area map files, and
match neither `m21` file. The picker draws names only (`showRowId = false`).

**Setting kinds.** `SettingKind` goes 7 → 11: `NamedChoice` (settings 2 and 8),
`Number` (setting 3), `InsertAreaPool`, `InsertedBossPool`. `SettingDef` gains,
**after** `help`: `int RandomizerDefaults::* number`, `int numberMin`,
`int numberMax`, `const char* choiceOff`, `const char* choiceOn`.
`SettingValueText` gains a decimal case and a label-pair case; `AdjustSetting`
gains a clamped `±1` case keyed on `direction`; `IsDrillIn` is false for
`NamedChoice` and `Number`; each of the four `Selection*` switches gains two
cases. `ToggleCount()` is untouched and goes 15 → 18 from the three new plain
toggles alone.

### What this reuses

| Existing code or tool | How it is used | Change needed |
| --------------------- | -------------- | ------------- |
| `EnemyRandomizer.cpp` `StepReadMap` / `StepWriteMap` | harvest site and call site | extend |
| `EnemyExclusionList.h`, `EnemySkipList.h`, `CagedDogList.h` | the three B5 protection tests | reuse as-is |
| `BossRandomizer.h` `BossIdentity` | the pass's identity type | reuse as-is |
| `Msbb.h` `part_fields` | three-field write; new position accessor | extend |
| `ModelPoolSelection.h` | both new selections, `DefaultSelected = true` | reuse as-is |
| `gen_pool_table.py` | generates both new tables | extend |
| `boss_verify.py` `Msbb`, `is_boss_name`, `REJECT_NPC` | the mirror's map reader and harvest rules | extend |
| `enemy_lookup.py` `overwritable_placements`, `exclusion_reason`, `zone_scaled_npc`, `enemies()["pos"]` | the mirror's eligible-target, skip and scaling oracles | reuse as-is |
| `WorldEditorScreen.cpp` progress lines | shortfall and skip reporting | extend |

---

## 5. Files and changes

| File | Change | M |
| ---- | ------ | - |
| `app/src/UI/SettingsModel.h` | `SettingKind` 7 → 11; five new `SettingDef` members **after** `help`; header comment for each new kind | 1 |
| `app/src/UI/SettingsModel.cpp` | `SettingValueText`, `AdjustSetting`, `IsDrillIn` and the four `Selection*` switches gain the new kinds | 1 |
| `app/src/UI/ModelPicker.h` | `kInsertedBossesStrings` (`showRowId = true`) and `kBossAreasStrings` (`showRowId = false`) | 1 |
| `app/src/UI/WorldEditorScreen.cpp` | `CountChangedSettings` gains a `Number` case | 1 |
| `app/src/Randomizer/InsertedBossTable.h` | **new, generated** — 22 rows, `poolEntries` = identity count per model, summing to 32 | 1 |
| `app/src/Randomizer/InsertAreaTable.h` | **new, generated** — 14 rows, `model` = area prefix, `poolEntries` = that area's eligible-target count, summing to 1,381 | 1 |
| `app/src/Randomizer/EnemyPoolSelection.h` | `InsertedBossSelection` = `ModelPoolSelection<22>`, `InsertAreaSelection` = `ModelPoolSelection<14>` | 1 |
| `app/tools/gen_pool_table.py` | two new `Kind`s, `insertboss` and `area`; `both` keeps its present meaning | 1 |
| `app/tools/boss_verify.py` | `insertion_list()` and `insertion_models()`; `insertion_areas()` for the area table | 1 |
| `app/src/Randomizer/RandomizerDefaults.h` | `bossesCanReplaceEnemies`, `bossReplacementsExact`, `replaceAllEnemies`, `spaceOutInsertedBosses`, `insertedBossKeepsEnemyAi` (bool, all false), `bossReplacementsPerMap` (int, 1), `insertedBosses`, `bossAreas` | 2 |
| `app/src/Randomizer/RandomizerDefaultsStore.cpp` | `char buf[1024]` → `char buf[2048]`, comment rewritten to the new arithmetic; eight keys in `FormatSettings` **and** `ApplySettingKey`. The two picker keys are **`inserted_boss_areas`** and **`inserted_bosses_included`** (§9 D2), which is what the 248-byte figure below is measured from | 2 |
| `app/src/UI/SettingsModel.cpp` | eight `kSettings` rows in the **Bosses** category after `BOSSES INCLUDED`, with help text; three of them carry B22's warning | 2 |
| `app/src/UI/SettingsModel.h` | eight new `SettingId` values | 2 |
| `docs/features/randomizer-settings-ui/spec.md` | §7.1: the eight settings move into the **Bosses** row's "current" column; `Bosses Can Replace Enemies` and `Per-Zone Toggles` leave the backlog column | 2 |
| `app/tools/settings_ui_verify.py` | `parse_defaults` learns int fields and the two new selection types; `KIND_TO_COUNT`, `widest_value`, the case-1 kind invariant, `PROSE_TO_LABEL`, and case 9's table list | 2 |
| `app/tools/pool_verify.py` | settings block **728 → 976**, worst case **779 → 1,027**, buffer assertion against 2,048, and a written-and-read case per new key | 2 |
| `app/src/Msb/Msbb.h` / `.cpp` | `part_fields::GetPosition` — three `float32` at `+0x28`, read-only | 3 |
| `app/src/Randomizer/BossInsertion.h` / `.cpp` | **new** — `InsertionPool`, `CollectInsertionCandidates`, `FinalizeInsertionPool`, `ApplyBossInsertion`, `InsertionOptions`; `REPLACE ALL ENEMIES` documented in the `RuneProbe.h` shape | 3 |
| `app/src/Randomizer/EnemyRandomizer.h` | eight option fields; `insertedBosses`, `insertShortfallMaps`, `insertPoolSize` on the result | 3 |
| `app/src/Randomizer/EnemyRandomizer.cpp` | harvest in `StepReadMap`, finalize in `StepBuildPool`, `ApplyBossInsertion` in `StepWriteMap` between the easy-mode pass and the scaling loop, summary lines in `StepEmevd`, one new header bullet | 3 |
| `app/src/Game/WorldActivation.cpp` | eight `options.… = run.…` lines | 3 |
| `app/src/UI/WorldEditorScreen.cpp` | progress lines: the insertion count, the silent-skip line, the shortfall line | 3 |
| `app/tools/insert_bosses_verify.py` | **new** — `verify` against a real generated tree plus `selftest` | 4 |
| `app/tools/pool_verify.py` | rebase the block **down** by 22 bytes when `replace_all_enemies` goes | 5 |

**Clean rebuild:** not required. **Saved data:** no existing `defaults.cfg` key
changes meaning and no existing selection is reinterpreted; both new selections
default to all-ticked, so an older file reads as every area and every boss
enabled with the master switch off, which is a run identical to today's.

**Byte arithmetic, and how it rebases.** The eight keys cost **248 bytes**
(per-key figures: `plan-evidence.md` §E4 M8). Against today's tree the settings
block is **728 → 976** and the worst-case `defaults.cfg` **779 → 1,027**. Rebase
on whichever of the other two shared-buffer features has landed: **011** (+47)
gives 1,023 / 1,074, **027** (+15) gives 991 / 1,042, both give **1,038 / 1,089**
— 14 bytes over `char buf[1024]`, which is why M2 grows it to 2,048 whatever the
order. Rename none of the eight keys without redoing this.

---

## 6. Verification

### Build

`cd app && make` after every milestone. No clean rebuild required.

### Automated

| Check | Command | Asserts |
| ----- | ------- | ------- |
| Tables are fresh | `python app/tools/gen_pool_table.py insertboss data/vanilla/dvdroot_ps4 --check` and the same for `area` | both new tables match the vanilla tree, row for row, in the frozen order |
| No table regressed | `python app/tools/gen_pool_table.py both data/vanilla/dvdroot_ps4 --check` and `… skip …` | the three shipped tables are byte-identical |
| Settings chain | `python app/tools/pool_verify.py selftest` | the block is exactly 976 bytes (see §5 for other landing orders), fits 2,048, and each of the eight keys is both written and read |
| Settings UI | `python app/tools/settings_ui_verify.py` | all eight rows exist in the Bosses category, every bool and int field is exactly one row, exactly one `SaveChoice` still, both new selections have one row each, every label + gap + widest value fits the 700 px pane, every help string is non-empty and wraps, both new picker tables fit the picker band |
| Pane scrolling | `python app/tools/ui_scroll_verify.py` | the 10-row Bosses pane's scroll bands, hints and row boxes |
| Nothing else moved | `python app/tools/boss_verify.py selftest`, `caged_dogs_verify.py selftest`, `easy_modes_verify.py selftest`, `enemy_lookup.py pool` | the shipped passes' rules are unchanged |
| **The feature** | `python app/tools/insert_bosses_verify.py verify data/vanilla/dvdroot_ps4 <output tree>` | spec §8 criteria 2–18 as properties of a real generated tree against the real vanilla input |
| The mirror's own rules | `python app/tools/insert_bosses_verify.py selftest data/vanilla/dvdroot_ps4` | the oracles below |

**`insert_bosses_verify.py selftest` asserts**, against
`data/vanilla/dvdroot_ps4` and the port's own headers:

* eligible targets are 1,381 across the fourteen live area files and 2,269 across
  all 24, with zero boss-named placements among them; all 26 caged-dog placements
  and every `ENEMIES SKIPPED`-reachable placement fall outside the set when their
  setting is on;
* the insertion list is **32 identities across 22 models** with Moon Presence
  absent, identical over `kBaseMaps`, the reference's 16-map order and
  `BossMapOrder`; both new tables match what the generator would write; the
  fourteen area prefixes partition the 22 area files and match neither `m21` file;
* a Python model of the pass reproduces `EXACTLY` as an equality on
  `min(N, |E|)`; `UP TO` as never exceeding it with a mean over 200 seeds that
  approaches it and **is unchanged when the list is narrowed to one identity**;
  the override as `|E|`; and count 0, an empty list and no ticked area each as 0
  with no draw consumed;
* the pool property — any two enabled identities' use counts differ by at most
  one; spacing's minimum pairwise distance beating the measured uniform-draw
  expectation, and agreeing with spacing-off at `k == 1`;
* the seven identities the scaling table does not track, so `verify` does not
  misread an unscaled arrival as a defect.

A mirror pins **the rules** and not the C++ implementation of them.

### Hardware

Run in this order, so a failure can be attributed (spec §8):

1. Default count 1, OWN AI, spacing off — does a loose boss aggro, attack and
   die with no fog gate; is there a health bar, music or a defeat message; does a
   save made in an affected area still load?
2. Count 15, spacing off, then on — are two bosses four paces apart survivable,
   and does spacing visibly move them?
3. **REPLACED ENEMY'S AI** at the default count — the test most likely to fail.
4. **REPLACE ALL ENEMIES**, last, once. Expected to crash or to make the game
   unplayable. **Record the outcome in `log.md`; that is what unblocks M5.**

Also look at: the multi-phase bosses' transitions; the seven unscaled identities
arriving at arena strength in the first area; and the three worst cases — a crash
on entering an affected area, an unkillable inert boss in a doorway the player
must pass, and a boss whose death fails to fire an event the area needs.

---

## 7. Milestones and stop conditions

### Milestone 1 — The settings model can express the four new kinds

**Goal.** `SettingsModel` can describe a number, a named two-state choice and two
more pickers, and both new picker tables exist.

**Changes**, in order:

1. Add `insertion_list()`, `insertion_models()` and `insertion_areas()` to
   `boss_verify.py` — done when `insertion_list` returns 32 identities over 22
   models with `c5400` absent, over all three map orders.
2. Add the `insertboss` and `area` `Kind`s to `gen_pool_table.py` and generate
   both headers — done when `--check` passes for both and for the three shipped
   tables unchanged.
3. Add the two selection typedefs — done when the header compiles and each is a
   distinct type from every existing selection.
4. Extend `SettingKind` and `SettingDef` (new members after `help`) — done when
   the build is clean with no edit to any of the 21 shipped `kSettings` rows.
5. Extend `SettingValueText`, `AdjustSetting`, `IsDrillIn` and the four
   `Selection*` switches, and add the two `PickerStrings` blocks — done when
   every switch is exhaustive with no `default:` label.
6. Add the `Number` case to `CountChangedSettings` — done when a number
   difference is counted exactly once.

**Invariants:** shipped table order frozen; `SaveChoice` untouched; layering.

**Verification:** build; `gen_pool_table.py … --check` ×4; `settings_ui_verify.py`
and `pool_verify.py selftest` still pass unchanged.

**On completion.** Continue to M2.

### Milestone 2 — The eight settings exist, persist and are editable

**Goal.** All eight appear under **Bosses** on both screens, round-trip through
`defaults.cfg` and a world revision, and seed a new world from the DEFAULTS tab.

**Changes**, in order:

1. Add the six scalar fields and the two selections to `RandomizerDefaults.h`,
   each with the field comment this file's convention requires, including what an
   older `defaults.cfg` reads as — done when a fresh struct is the run the app
   makes today.
2. Grow `char buf[1024]` to 2,048 and rewrite the comment to the §5 arithmetic —
   done when the comment states the figure for the landing order actually in the
   tree.
3. Add the eight keys to `FormatSettings` and `ApplySettingKey` — done when every
   key is both written and read.
4. Add the eight `SettingId` values and the eight `kSettings` rows, after
   `BOSSES INCLUDED`, with help text; three rows carry B22's warning — done when
   every label and help string is renderable and within budget.
5. Move the eight into the **Bosses** row of `randomizer-settings-ui/spec.md`
   §7.1 and add them to `PROSE_TO_LABEL` — done when `settings_ui_verify.py`
   case 3 passes.
6. Extend `settings_ui_verify.py` (parser, `KIND_TO_COUNT`, `widest_value`, the
   case-1 kind invariant, case 9's table list) and `pool_verify.py` (block 976,
   worst case 1,027, buffer 2,048, eight key cases) — done when both pass.

**Invariants:** no existing key changes meaning; no existing selection is
reinterpreted; `ToggleCount()` reports 18 and excludes the number, the two named
choices and the two pickers.

**Verification:** build; `pool_verify.py selftest`; `settings_ui_verify.py`;
`ui_scroll_verify.py`.

**On completion.** The `.pkg` builds and the checks pass. **An optional gate sits
here** — see the Execution Strategy. Otherwise continue to M3.

### Milestone 3 — The insertion pass

**Goal.** A run with the feature on writes boss identities over ordinary
placements, in the numbers, areas and positions the eight settings describe.

**Changes**, in order:

1. `part_fields::GetPosition` in `Msbb.{h,cpp}`, with the offset derivation in the
   header comment — done when it returns plausible coordinates for a known
   Central Yharnam placement and writes nothing.
2. `BossInsertion.h` — `InsertionOptions`, `InsertionPool`, the four entry points,
   and the file header stating the pass's position as behaviour. The
   `REPLACE ALL ENEMIES` block follows `RuneProbe.h`'s shape: temporary, the
   question, the design, what retires it — done when the header names §6's
   retirement condition.
3. `CollectInsertionCandidates` / `FinalizeInsertionPool`, wired into
   `StepReadMap` and `StepBuildPool` behind the master toggle — done when the log
   reports 32 identities with everything ticked and 0 with nothing ticked.
4. `ApplyBossInsertion` steps 1–5 of §4 — done when each of the two count modes,
   the override, the two spacing states and the two AI states is implemented
   exactly as §4 states, and when the off path reaches no `RandInt`.
5. Call it in `StepWriteMap` after `ApplyEasyModes` and before the
   `BossScalingMaps()` loop — done when a scaled area's inserted boss carries
   that area's variant.
6. Options and result plumbing: the eight fields on `EnemyRandomizerOptions`, the
   three result counters, the eight lines in `WorldActivation.cpp`, and the three
   progress lines in `WorldEditorScreen.cpp` — done when a shortfall run reports
   the shortfall, an empty list reports the silent skip, and neither fails.

**Invariants:** off is byte-identical and draw-identical; the enemy, boss and
easy-mode passes are unchanged; only the three fields are written; positions are
read-only; `BossRandomizer.cpp` is not edited.

**Verification:** build; `boss_verify.py selftest`; `caged_dogs_verify.py
selftest`; `easy_modes_verify.py selftest`; `enemy_lookup.py pool`; and a
generated-tree diff of a feature-off run against a run from the same seed on the
pre-M3 build, which must be byte-identical.

**On completion.** Continue to M4.

### Milestone 4 — The pass is verified against real data

**Goal.** Spec §8 criteria 2–18 are machine-checked on a real generated tree.

**Changes**, in order:

1. `insert_bosses_verify.py` oracles: eligible targets per map file, the
   insertion list, the area partition, the scaling map — done when `selftest`
   asserts every bullet in §6.
2. The Python model of the pass and its distribution checks, including the
   single-boss-mean case — done when the model fails if `p` is taken from pool
   composition.
3. `verify <vanilla> <output>` — done when it reports criteria 2–18 individually,
   naming the map and placement for each failure.
4. Run it against trees generated on hardware for at least the default
   configuration, the maximum count, and the override — done when all three pass
   or a failure is reported under §7's stop conditions.

**Invariants:** the mirror reads the port's own headers rather than restating
their contents.

**Verification:** every command in §6's automated table.

**On completion.** **Execution stops.** Hand off §6's hardware plan. M5 does not
begin until the developer has recorded the **REPLACE ALL ENEMIES** result in
`log.md`.

### Milestone 5 — Remove REPLACE ALL ENEMIES

**Goal.** The test control is gone, and nothing else about the feature changes.

**Changes**, in order:

1. Delete the `kSettings` row, its `SettingId`, its help text and its
   `RandomizerDefaults` field — done when `settings_ui_verify.py` passes with
   seven settings in the group.
2. Delete `replace_all_enemies` from `FormatSettings` and `ApplySettingKey`, and
   the override branch from `ApplyBossInsertion` and `InsertionOptions` — done
   when no source file mentions it.
3. Rebase `pool_verify.py`'s block **down by 22 bytes** and extend the narrating
   comment; leave `char buf[2048]` alone — done when the exact-equality case
   passes at the new figure.
4. Drop the override's criterion from `insert_bosses_verify.py` — done when
   `verify` and `selftest` pass.

**Invariants:** an existing `defaults.cfg` still carrying the retired key loads
with that line ignored, exactly as `unchanged_bell_maidens` does.

**Verification:** build; `pool_verify.py selftest`; `settings_ui_verify.py`;
`insert_bosses_verify.py selftest`.

### Stop conditions

Halt and report rather than deciding, if any of these occur:

* the build fails in a way this plan did not anticipate;
* a verification check fails and the cause is not an obvious implementation slip;
* the feature-off tree diff in M3 is not byte-identical;
* the measured insertion list is not 32 identities over 22 models;
* an implementation decision would contradict the spec or §2;
* a required behaviour cannot be implemented as described — in particular, if the
  count modes, the derived probability or the spacing rule cannot be written as
  §4 states them;
* the change needs a file not listed in §5;
* a §3.1 invariant cannot be preserved;
* M5 is reached without a hardware result recorded in `log.md`.

---

## 8. Open questions

Empty — Q1–Q3 were answered on 2026-09-28 and are recorded as D1–D3 in §9.

---

## 9. Decisions

| # | Date | Decision | Taken by |
| - | ---- | -------- | -------- |
| D1 | 2026-09-28 | **Q1 — keep `BOSS BEHAVIOUR`** as the row label rather than `BOSS AI`. Measured at 663 px of the pane's 700 px budget with its widest value, making it the app's widest row with 37 px spare. Taken as recommended: it is the spec's wording and `BOSS AI` would be confusable with the four `EASY …` rows | developer |
| D2 | 2026-09-28 | **Q2 — the config keys are `inserted_boss_areas` and `inserted_bosses_included`**, per §5. Taken as recommended, following the `…_included` convention the other four picker keys use. These give **248 bytes** against the spec §4 estimate's 241; §5's 976-byte block is measured from 248, and D-I's conclusion is unaffected either way | developer |
| D3 | 2026-09-28 | **Q3 — the area picker shows no map prefix** (`showRowId = false`). Taken as recommended: all fourteen area names are already distinct, and the text log names the file anyway | developer |
| D4 | 2026-09-28 | **The Optional gate after M2 is declined — M1–M4 run continuously.** `ui_scroll_verify.py` and `settings_ui_verify.py` cover the ten-row scrolling Bosses pane arithmetically, and nothing in M3 depends on the pane being looked at. **The Required gate before M5 stands unchanged**, because M5's entire content is deleting the setting the hardware run exists to exercise | developer |
| P1 | 2026-09-28 | The insertion list is **32 identities across 22 models**, not 29 — measured; the reference harvests before its `m28`/`m34` de-eligibility tests | planner |
| P2 | 2026-09-28 | `UP TO`'s probability is `min(N, \|E\|) / \|E\|`, applied as one draw per eligible placement with a hard ceiling — the shape spec §7 item 2 describes | planner |
| P3 | 2026-09-28 | `k` and "which k" are two steps, so spacing changes neither the count nor the number of draws at `k == 1` | planner |
| P4 | 2026-09-28 | `REPLACED ENEMY'S AI` is implemented as *not writing* `ThinkParamID`, reproducing the reference's abandoned edit exactly (§3 item 5) | planner |
| P5 | 2026-09-28 | The harvest lives in `StepReadMap` and the pass in `BossInsertion.cpp`; `BossRandomizer.cpp` is not edited | planner |
| P6 | 2026-09-28 | The area table keys on the map-name **prefix**, following `CagedDogList.h`'s precedent | planner |
| P7 | 2026-09-28 | Pool order is first-seen over `kBaseMaps` order; `refill` is the full identity list, not model-deduped | planner |
| P8 | 2026-09-28 | Config keys and byte arithmetic per §5: block 728 → 976, buffer 2,048 | planner |

---

## 10. Changes during implementation

| Date | Change | Reason |
| ---- | ------ | ------ |
| | | |
