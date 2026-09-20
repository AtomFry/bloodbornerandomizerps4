# Plan 018 — Easy Shadows, Easy Rom, Easy Failures, Easy Emissary

**Status: Approved** §8 is empty: the
spec and the repository settled every implementation question.

**Spec:** `docs/features/018-easy-shadows/spec.md` — **APPROVED** (developer,
2026-09-16). Four binding decisions, §10 D1–D4.

**Evidence:** `docs/features/018-easy-shadows/plan-evidence.md` — the
measurements, traces and rejected alternatives behind this plan.

**Plan review:** `docs/features/018-easy-shadows/plan-review.md`

**Backlog rows:** `docs/randomization-feature-spec.md` §6, rows **18, 19, 20, 21**

---

> **This document is the implementation contract.** §1–§7 are what the
> implementer reads, in order. §8–§10 are the record, not instructions.
> `plan-evidence.md` holds the investigation; `log.md` holds the history.

---

## 1. Objective

Add four on/off settings, all off by default — `EASY SHADOWS`, `EASY ROM`,
`EASY FAILURES`, `EASY EMISSARY` — each turning one multi-body boss arena into
a duel by replacing the duplicate bodies with the Iosefka's Clinic larva. One
new table-driven MSB pass overwrites three fields (`NPCParamID`,
`ThinkParamID`, model index) on named placements in six named maps: 79
placements tree-wide, 42 in the maps the retail game loads. It draws no
randomness and applies whether or not any randomizer is on.

---

## 2. Approved behaviour

| #   | Behaviour | Source |
| --- | --------- | ------ |
| B1  | Four independent toggles, one per fight, all default off, labelled `EASY SHADOWS`, `EASY ROM`, `EASY FAILURES`, `EASY EMISSARY`. Any combination is valid, including all four with every randomizer off | spec §10 D4, §7 |
| B2  | Each setting replaces exactly the placements the reference selects: Shadows `c2120_0001/_0002` in `m27_00_00_01`; Rom every `c1400` in both `m32_00_00_*`; Failures `c4030_0001/_0002/_0003` in `m35_00_00_00`; Emissary the seven existing names of `c2500_0001`…`_0010` in both `m24_02_00_*` | spec §3, §4 |
| B3  | The replacement is the reference's own creature — model `c2521`, NpcParam 252100, ThinkParam 252100 — planted as-is, drop intact. Relocating One Third of Umbilical Cord is accepted | spec §10 D2 |
| B4  | Exactly three fields are written per replaced placement. EntityID, part name, position and every index-based cross-reference are untouched | spec §3, §8 criterion 3 |
| B5  | The reference's asymmetric map lists are matched exactly. `m27_00_00_00` is **not** patched, even though it holds the same three Shadow placements | spec §10 D3 |
| B6  | Not a randomizer: the pass draws no randomness, so the same seed produces the same world with a setting on or off, apart from the affected placements | spec §7 |
| B7  | With a setting off, the output tree is what the same seed and settings produce today | spec §8 criterion 4 |
| B8  | The setting wins over the boss passes: a duplicate slot that boss randomization filled becomes a larva anyway | spec §2, §7 |
| B9  | The survivors are untouched by this feature: one Shadow (`c2120_0000`), Rom herself, one Living Failure (`c4030_0000`), `c4030_0004`, one small emissary (`c2500_0000`), the Celestial Emissary (`c2570_0001`) and the three `c2500_0011/_0012/_0013` elsewhere in Upper Cathedral Ward | spec §2, §8 criterion 2 |
| B10 | The four settings are independent: turning one on changes only its own maps | spec §8 criterion 5 |
| B11 | State persists in the existing `defaults.cfg`; an older file without the keys loads with all four off | spec §7 |
| B12 | This feature writes no param bytes. NpcParam row 252100 and ItemLotParam row 28040 are unchanged in the output tree whether or not drop randomization is on | spec §8 criterion 6 |
| B13 | The pass is the last writer of its placements, so an easy setting also wins over `ENEMIES SKIPPED` and any other enemy-side setting: its targets are larvae regardless of what the seed or the skip list did | spec §2, §7 |

---

## 3. Constraints and invariants

### 3.1 Must be preserved

* **No `RandInt` call in any new code.** A draw from the stream changes every
  seed's world (B6).
* **The pass runs inside `StepWriteMap`, after the enemy loop's closing brace
  and before the `BossScalingMaps()` loop.** Earlier and the boss passes would
  overwrite it (B8); later and the written stat row would stop matching the
  reference.
* **Exactly three fields per hit, and only on matched placements.** Anything
  else written breaks B4 and the fight scripts that track each body by entity
  ID.
* **The table's map list does not grow or shrink.** `m27_00_00_00` stays out
  (B5).
* **UI rows are appended last**, at the end of every row-constant block and at
  the end of all three `items` vectors, with no existing row index changed. An
  out-of-order entry compiles, passes `ui_scroll_verify.py` and mislabels every
  row below it.
* **`StepMergeModels` stays unconditional.** It is what guarantees `c2521` is
  declared in every map being written.
* **`defaults.cfg` stays forward and backward compatible**: unknown keys are
  ignored on load, an absent key reads as `false`.
* **"Unchanged" never means byte-identical to vanilla.** The scaling pass runs
  on every map on every run, and every map is re-serialised and re-compressed.
* **Nothing in `BossRandomizer.cpp`, `EnemyExclusionList.h`, `EnemyPoolTable.h`,
  `EnemySkipTable.h`, `BossList.h` or any param table changes.**
* **Layering**: the pass lives under `app/src/Randomizer/` and depends on `Msb`
  only; no SDL2 there, and no UI file learns a map name or a placement name.

### 3.2 Out of scope

* Adding `m27_00_00_00`, or otherwise normalising the map lists.
* Choosing a different replacement creature, blanking the replaced placements'
  item lots, or stripping the larva's drop.
* Extending `DropRandomizer.cpp`'s `IsExcludedNpcRow` to the 31 zone-scaled
  larva rows `900014601`–`900014631`. They are randomizable today, in the port
  and in the reference alike; that stays true (`plan-evidence.md` §E5.5).
* Reproducing the reference's runtime capture of the replacement identity, or
  its picker-path defect.
* Any change to how the four fights are randomized — pools, eligibility,
  fixup groups — and any other multi-body fight.
* Rows 22 "No Scaling" and 23 "Custom Scaling". Chalice dungeons.

### 3.3 Hazards

| Hazard | What to do | Evidence |
| ------ | ---------- | -------- |
| A settings row whose constant and `items` entry disagree compiles, passes `ui_scroll_verify.py` and mislabels every row below it — including two picker rows | Append all four rows last, in the same order, to every constant block and all three `items` vectors in one pass. Change no existing row index | `plan-evidence.md` §E5.2 |
| Struct layout changes without a clean rebuild have produced a heap-corruption `SIGSEGV` on hardware | `rm -rf src/x64 && make`; three structs gain members | `docs/build.md` §"Clean builds after layout changes" |
| Asserting that survivors or settings-off maps are byte-identical to vanilla fails on a **correct** build — the unconditional scaling pass rewrites the stat rows of Rom, `c2120_0000`, `c4030_0000`, `c2500_0011/_0012/_0013` and `c2570_0001` | Use `pool_verify._frozen`'s definition everywhere: same model, same think, npc either vanilla or that map's zone-scaled variant | `plan-evidence.md` §E5.3, §E4 M11 |
| `pool_verify.py`'s worst-case `defaults.cfg` case is an exact equality (611) and fails the moment a key is added | Update it to 669 in the same milestone that adds the four keys | `plan-evidence.md` §E4 M15 |
| `boss_verify.py verify` reports 37 false `V2 non-boss placement changed` failures on an easy tree | Add the `--easy` flag of §5, suppressing by (map, name) from the table | `plan-evidence.md` §E5.4 |
| A name pattern matches placements in maps other than its own — `c1400` also matches in two chalice maps, `c2120_0002` in `m26_00_00_00` | Key the table by **exact** map name and assert pattern reach per map, never tree-wide | `plan-evidence.md` §E4 M5 |
| With `RANDOMIZE ENEMY DROPS` on, larvae in the four scaled maps can drop something other than the cord | Run the cord hardware step with drops **off**; do not treat a missing cord in a drops-on run as a defect | `plan-evidence.md` §E5.5 |
| `result.npcParamsScaled` gains 42 and `result.enemiesRandomized` can double-count | Cosmetic and what the reference does. Do not "fix" either; the four new result lines are the numbers to read | `plan-evidence.md` §E5.8 |
| Easy Rom empties a group of thirty rather than thinning it, and Rom may also spawn children dynamically | Expected. A hardware sighting of a real spider is the dynamic-spawn unknown, not a broken rule | `plan-evidence.md` §E5.1 |

---

## 4. Implementation approach

> Chosen: one new pass file holding a six-row table keyed by exact map name,
> called once per map from inside `StepWriteMap`. Alternatives considered and
> why they were rejected: `plan-evidence.md` §E3.

### 4.1 The table

`app/src/Randomizer/EasyModes.h` carries the feature's whole data: six rows,
keyed by exact map name, each with a `nullptr`-terminated pattern array — the
shape `BossRandomizer.cpp`'s `FixupGroup` already uses.

| Flag | Map | Name patterns | Placements matched |
| ---- | --- | ------------- | -----------------: |
| shadows | `m27_00_00_01` | `c2120_0001`, `c2120_0002` | 2 |
| rom | `m32_00_00_00` | `c1400` | 30 |
| rom | `m32_00_00_01` | `c1400` | 30 |
| failures | `m35_00_00_00` | `c4030_0001`, `c4030_0002`, `c4030_0003` | 3 |
| emissary | `m24_02_00_00` | `c2500_0001` … `c2500_0010` (10 patterns) | 7 |
| emissary | `m24_02_00_01` | `c2500_0001` … `c2500_0010` (10 patterns) | 7 |

**Six rows. 79 placements tree-wide, 42 in the retail-loaded maps.** Both
totals are asserted in §6.

Two notes belong in the header beside the table:

* **`m27_00_00_00` is absent on purpose** (B5) — it holds the same three
  `c2120` placements with the same values and the reference still does not
  patch it, so adding it is a behaviour change.
* **Matching is substring, not equality** — `name.find(pattern) != npos`,
  mirroring the reference's `Name.Contains`. Rom's rule depends on it; exact
  map keying is what keeps it from reaching other maps.

### 4.2 The pass

`app/src/Randomizer/EasyModes.h` / `.cpp`, self-contained in the shape of
`ApplyBossParamScaling`:

```
struct EasyModeOptions { bool shadows, rom, failures, emissary; };   // all false
struct EasyModeCounts  { int shadows, rom, failures, emissary; };    // all 0

// returns how many placements it replaced in this map; counts accumulates per flag
int ApplyEasyModes(const std::string& mapName, MsbbFile& msbb,
                   const EasyModeOptions& options, EasyModeCounts& counts);
```

Behaviour, in order:

1. Return immediately if no flag is set, or if `mapName` matches no table row
   whose flag is set.
2. Scan the map's `Models` section for the enemy model named `c2521` and take
   its index. If it is not there — impossible after `StepMergeModels`, but see
   §3.1 — log once and leave every placement in that map untouched. Never write
   a −1 index (a deliberate divergence from the reference, which throws;
   `plan-evidence.md` §E5.6).
3. For each enemy part whose name contains one of the row's patterns, write
   `SetEnemyNPCParamID(252100)`, `SetEnemyThinkParamID(252100)` and
   `SetModelIndex(<the c2521 index>)`, increment that flag's counter, and log
   one line in the existing `map name: old -> new` shape.

No `RandInt`, no other field written, no other map touched.

### 4.3 Where it is called from

One line in `StepWriteMap`, between `} // if (options.randomizeEnemies)`
(`EnemyRandomizer.cpp:853`) and the `for (const BossScalingMapEntry& ...)` loop
(`:855`):

```
ApplyEasyModes(lm.name, lm.msbb, options.easyModes, result.easyCounts);
```

That slot reproduces the reference's ordering exactly
(`plan-evidence.md` §E1.4) and gives B8 for free, because boss assignment
including `AddTheRestInMap` completes before the first map is written.

Consequence to expect, not to correct: a larva placed in a scaled map is
itself scaled, so the written `NPCParamID` is the per-map value §6 case 8
lists, not a flat 252100.

### 4.4 The replacement identity is baked

```
const int32_t kEasyModeNpcParamId   = 252100;
const int32_t kEasyModeThinkParamId = 252100;
const char*   kEasyModeModelName    = "c2521";
```

Settled by P3. The header comment records the provenance (`plan-evidence.md`
§E4 M7), and selftest case 5 asserts the triple against the real vanilla tree
on every run.

### 4.5 Four flags through the settings chain

Four plain bools — `easyShadows`, `easyRom`, `easyFailures`, `easyEmissary` —
in `RandomizerDefaults` and, grouped into `EasyModeOptions`, in
`EnemyRandomizerOptions`. `defaults.cfg` keys: `easy_shadows`, `easy_rom`,
`easy_failures`, `easy_emissary`.

Row placement on both screens, per P1 and the append-last rule of §3.1: the
four rows go **after** `kStartWithHunterToolsRow` (16), in backlog order —
`kEasyShadowsRow` 17, `kEasyRomRow` 18, `kEasyFailuresRow` 19,
`kEasyEmissaryRow` 20. `kItemCount` (`SetupDefaultsScreen.h`) and
`kSaveDataRowCount` (`EnableWizardScreen.cpp`) both go **17 → 21**. No
existing row constant changes. The `items` entries go last in `DrawList`,
`DrawSaveData` and `DrawConfirm`, in the same order, each with a position
comment like the two rows above them.

All four flags join `StartCommit`'s big `||` (`EnableWizardScreen.cpp:572-576`)
beside `enableMergoDarkness_` and `startWithHunterTools_`: each one alone
writes real map files, so ticking one alone must start a run. The two
`NoneEnabled()` picker guards are untouched — gated on `randomizeEnemies_` /
`randomizeBosses_`, an easy-only run passes straight through. The store's
`char buf[1024]` and the 4096-byte load buffer do not grow (611 → 669 bytes).

### 4.6 Reporting the run

`EnemyRandomizerResult` gains an `EasyModeCounts easyCounts;`, and
`FinishCommit` emits one line per **enabled** setting:

```
EASY SHADOWS REPLACED 2 PLACEMENTS
EASY ROM REPLACED 60 PLACEMENTS
EASY FAILURES REPLACED 3 PLACEMENTS
EASY EMISSARY REPLACED 14 PLACEMENTS
```

No `SKIPPING` counterparts. The counts are fixed, so a `0` or a wrong number
on the console means a pattern list or a map name is wrong. `EASY ROM` reads
60 and `EASY EMISSARY` 14 because the port writes both variants of those two
areas.

### What this reuses

| Existing code or tool | How it is used | Change needed |
| --------------------- | -------------- | ------------- |
| `EnemyRandomizer.cpp` `StepWriteMap` / `StepMergeModels` | The call site; and the guarantee that `c2521` is declared in every written map | one call + include |
| `Msb/Msbb.h` `part_fields` / `model_fields` | `GetName`, `GetType`, `SetEnemyNPCParamID`, `SetEnemyThinkParamID`, `SetModelIndex` | none |
| `BossParamScaling.h`, `CagedDogList.h` + `caged_dogs_verify.py` | File shape for a non-randomizing pass; baked table in a header with a mirror that parses it | none |
| `enemy_lookup.py` (`zone_scaled_npc`, `load_map`, `enemies`), `pool_verify.py` `_frozen` | The frozen and zone-scaled comparisons in the new verifier | reuse as-is |
| `boss_verify.py` `compare_trees`, `ui_scroll_verify.py`, `pool_verify.py` selftest | Post-run diff, row geometry, config budget | see §5 |

---

## 5. Files and changes

| File | Change | Milestone |
| ---- | ------ | --------- |
| `app/src/Randomizer/EasyModes.h` | **New.** `EasyModeOptions`, `EasyModeCounts`, the baked constants of §4.4 with their provenance comment, the six-row table of §4.1 with both header notes, and the `ApplyEasyModes` declaration. Written so `easy_modes_verify.py` can parse the table out of it | 1 |
| `app/src/Randomizer/EasyModes.cpp` | **New.** The pass of §4.2 | 1 |
| `app/src/Randomizer/EnemyRandomizer.h` | `EasyModeOptions easyModes;` on `EnemyRandomizerOptions`, with a comment that it draws no randomness and applies regardless of the other settings; `EasyModeCounts easyCounts;` on `EnemyRandomizerResult` | 1 |
| `app/src/Randomizer/EnemyRandomizer.cpp` | `#include "EasyModes.h"`; the one call of §4.3; a bullet in the file header comment recording that the easy pass runs after enemy/boss randomization and before scaling, and why | 1 |
| `app/src/Randomizer/RandomizerDefaults.h` | Four `bool … = false;` fields with the standard absent-key comment | 1 |
| `app/src/Randomizer/RandomizerDefaultsStore.cpp` | Four load branches and four `%d` + arguments in the save `snprintf`. Buffers unchanged | 1 |
| `app/src/UI/SetupDefaultsScreen.h` | Four row constants appended after `kStartWithHunterToolsRow` (17–20) with the append-last comment; `kItemCount` 17 → 21 | 1 |
| `app/src/UI/SetupDefaultsScreen.cpp` | Four `ToggleRow` branches with their `defaults: easy … = YES/NO` logs; four `items` entries appended last, in row order | 1 |
| `app/src/UI/EnableWizardScreen.h` | Four `bool easy…_;` members beside the other per-run toggles | 1 |
| `app/src/UI/EnableWizardScreen.cpp` | Four row constants in the same positions; `kSaveDataRowCount` 17 → 21; ctor init from defaults; four left/right branches and four X branches; four entries appended last to **both** `DrawSaveData` and `DrawConfirm` (they must stay identical in shape); four `options.easyModes.… = easy…_;` assignments; the four flags added to the big `||`; the four result lines of §4.6 in `FinishCommit` | 1 |
| `app/tools/easy_modes_verify.py` | **New.** `show` / `verify` / `selftest`, modelled on `caged_dogs_verify.py`, parsing the table and constants out of `EasyModes.h` | 1 |
| `app/tools/boss_verify.py` | `--easy` flag on `verify`: suppress the `V2 non-boss placement changed` objection for exactly the (map, name) pairs the table selects, and print that it is doing so | 1 |
| `app/tools/ui_scroll_verify.py` | `Setup Defaults`, `Wizard SaveData`, `Wizard Confirm` counts 17 → 21; `Progress log` budget 16 → 20 for the four new result lines; update the comment that explains both | 1 |
| `app/tools/pool_verify.py` | Worst-case `defaults.cfg` equality 611 → 669, with the four keys added to the arithmetic and the comment | 1 |

**Not changed:** any param file or param table, `EnemyExclusionList.h`,
`EnemyPoolTable.h`, `EnemySkipTable.h`, `BossList.h`, `BossRandomizer.cpp`,
`gen_pool_table.py`, `DropRandomizer.cpp`, `docs/randomization-feature-spec.md`,
`docs/user-guide.md`, anything under `reference/`.

**Clean rebuild required**: `RandomizerDefaults`, `EnemyRandomizerOptions` and
`EnemyRandomizerResult` all gain members.

---

## 6. Verification

### Build

```
cd app
rm -rf src/x64
make
```

The clean rebuild is required, not optional — see §3.3.

### Automated

| Check | Command | Asserts |
| ----- | ------- | ------- |
| New mirror, rules | `python tools/easy_modes_verify.py selftest ../data/vanilla/dvdroot_ps4` | the twelve cases below |
| New mirror, output | `python tools/easy_modes_verify.py verify <V> <B> [shadows] [rom] [failures] [emissary]` | the output-tree contract below |
| UI geometry | `python tools/ui_scroll_verify.py` | 21 rows on three screens, all scroll properties |
| Picker + config | `python tools/pool_verify.py selftest ../data/vanilla/dvdroot_ps4` | worst-case `defaults.cfg` is 669 bytes and fits `buf[1024]` |
| Boss rules | `python tools/boss_verify.py selftest ../data/vanilla/dvdroot_ps4` | unchanged, must still pass |
| Boss output | `python tools/boss_verify.py verify <V> <B> --easy [--enemies-also]` | no boss-rule violation once the easy placements are suppressed |
| Drops | `python tools/drops_verify.py selftest ../data/vanilla/dvdroot_ps4` | D-I4 still holds |

**`easy_modes_verify.py selftest`** parses the table and the constants out of
`EasyModes.h`, so the mirror cannot drift from the port:

| # | Case | Asserts |
| - | ---- | ------- |
| 1 | The table resolves to the expected placement sets | `m27_00_00_01` → `{c2120_0001, c2120_0002}`; `m32_00_00_00` and `m32_00_00_01` → 30 `c1400_*` each; `m35_00_00_00` → `{c4030_0001, _0002, _0003}`; `m24_02_00_00` and `m24_02_00_01` → `{c2500_0001, _0002, _0003, _0006, _0007, _0009, _0010}`. Totals **79** tree-wide, **42** retail-loaded |
| 2 | `m27_00_00_00` is not in the table | and it holds three `c2120` placements with NpcParams 212700 / 212710 / 212720 — B5 asserted, not assumed |
| 3 | The survivors of B9 are matched by no pattern in their own map | and `c2120_0000` is the strongest Shadow, 1425 HP against 900 and 800 |
| 4 | No pattern over-reaches **inside its own map** | for each row, the set its patterns match in that row's map is exactly case 1's set — in particular `c2500_0001` does not also take `c2500_0011`. Asserted per (map, row), never tree-wide: the same patterns do match in other maps |
| 5 | The baked triple is real | `(252100, 252100, "c2521")` is the identity of all three `c2521_0000` placements, entity ID 2410771, all identical |
| 6 | `c2521` is declared as an enemy model | in at least one of the 24 base maps — measured, in five |
| 7 | The replacement cannot be dangerous | NpcParam 252100: 2 HP, 18 blood echoes, teamType 26, hitHeight 1.0, hitRadius 0.2, `behaviorVariationId` 25210, item lot 28040 — and all 31 scaled variants `900014601`–`900014631` identical in every one of those fields |
| 8 | The per-map written value is exactly | 900014609 in `m27_00_00_01`, 900014611 in `m32_00_00_01`, 900014614 in `m24_02_00_01`, 900014627 in `m35_00_00_00`, 252100 in `m32_00_00_00` and `m24_02_00_00` |
| 9 | The drop situation, asserted as it is | ItemLot 28040 is the only `ItemLotParam` row granting item 4321, `getItemFlagId` 50001205; exactly 32 `NpcParam` rows reference it (252100 plus the 31 variants); and `IsExcludedNpcRow`, parsed out of `DropRandomizer.cpp`, holds 252100 and 6071 and **nothing else**, so the 31 variants are deliberately unprotected (§3.2). A change on either side is caught |
| 10 | No pattern matches an arena leader | `c2120_0000`, `c2500_0000`, `c2570_0001`, `c4030_0000` |
| 11 | The reference's own name lists appear verbatim | in `reference/Randomizer/MainWindowComponents/MainWindow.xaml.cs`, following `starting_weapons_verify.py`. This couples the selftest to that path, which `CLAUDE.md` §1 already fixes in place |
| 12 | Corrupt-and-catch, in `boss_verify.py selftest`'s style | a survivor changed → caught; a target left at its vanilla identity → caught; an EntityID changed on a replaced placement → caught; an unmodified tree → clean |

**`easy_modes_verify.py verify`** compares a vanilla tree against an output
tree and asserts:

* every placement the named settings select carries model `c2521`,
  `ThinkParamID` 252100, and the **exact** `NPCParamID` case 8 gives for that
  map, and keeps its EntityID and its position (the three floats at part
  entry + 0x28);
* every survivor of B9 is **frozen** in `pool_verify._frozen`'s sense — same
  model, same think, and an `NPCParamID` that is either the vanilla value or
  that map's zone-scaled variant. Never byte equality (§3.3);
* in a map whose setting is off, every placement is frozen in that same sense
  when the run had every randomizer off, and otherwise differs only as the
  enemy and boss passes allow. Never a file-level byte comparison: every
  `.msb.dcx` is re-serialised and re-compressed on every run;
* `NpcParam` row 252100 and `ItemLotParam` row 28040 are byte-identical
  between the two trees, whether or not drop randomization was on (B12).

The tool's docstring must say plainly that it pins the rules and the data, not
the C++ implementation of them, and that nothing here can establish that the
replacement is harmless or that the fights end.

### Hardware

Three trees: vanilla `V`; seed `S` with all four settings **off** giving `A`;
the same seed `S` with all four **on** giving `B`. Vanilla as the third input
is mandatory (`CLAUDE.md` §3).

| # | Step | Pass looks like |
| - | ---- | --------------- |
| 1 | `easy_modes_verify.py verify V B shadows rom failures emissary` and `boss_verify.py verify V B --easy` | both PASS; the four console lines read 2 / 60 / 3 / 14 |
| 2 | Play `B`, Shadows of Yharnam | one Shadow fights; two small creatures stand in for the others. **Do they attack? Do they die in one hit? Does the fight end after the third body dies, and does the fog lift?** |
| 3 | Play `B`, Rom | every child is a larva; her fight still progresses through her teleports and she dies normally. Note whether any *real* spider appears |
| 4 | Play `B`, Living Failures and Celestial Emissary | one opponent each, both fights end normally, and the Emissary still grows out of the surviving small body |
| 5 | Play `B` **with `RANDOMIZE ENEMY DROPS` off**, kill the first easy-mode larva you reach, then visit Iosefka's Clinic | **one** One Third of Umbilical Cord in total. A cord per larva is the trigger for revisiting spec §10 D2 with evidence. Drops must be off or the result means nothing (§3.3) |
| 6 | Play `A` (all four off) | the four fights are exactly as they were. This is what makes steps 2–5 mean anything |
| 7 | Run with **only** `EASY SHADOWS` on and every randomizer off | the run starts, completes, and only `m27_00_00_01` differs — B10 |
| 8 | Reopen Setup Defaults after saving | the four new rows persist, and `ENEMIES INCLUDED` still reads `82 OF 82` — a changed count would mean the config string was misread when the four keys were added |

**Failure would look like:** a fight that never ends; a replacement that is
hostile or tough enough to matter; an easy setting losing to boss
randomization; a map that fails to load at all (the model declaration); or a
wrong count on a progress line (the table).

A clean build and green selftests mean **ready for hardware test**, never done.

---

## 7. Milestones and stop conditions

### Milestone 1 — the pass, the four settings, and the verifiers

**Goal.** Four working toggles that replace the duplicate bodies in the four
arenas, with a Python mirror that pins the table and the identity.

**Changes**, in order:

1. `EasyModes.h` — constants, structs, the six-row table, both header notes —
   done when it compiles standalone and the table reads exactly as §4.1.
2. `EasyModes.cpp` — the pass of §4.2 — done when it builds with no `RandInt`.
3. `EnemyRandomizer.h` / `.cpp` — option, result counts, include, the call of
   §4.3 — done when the call sits between the enemy loop's closing brace and
   the `BossScalingMaps()` loop and the job builds.
4. `RandomizerDefaults.h` and `RandomizerDefaultsStore.cpp` — four fields,
   four load branches, four save arguments — done when a save/load round trip
   preserves all four and a file without the keys still loads.
5. `SetupDefaultsScreen.h` / `.cpp` — four row constants appended last,
   `kItemCount` 21, four `ToggleRow` branches, four `items` entries — done
   when no existing row constant changed and the four entries are last, in row
   order.
6. `EnableWizardScreen.h` / `.cpp` — members, constants, `kSaveDataRowCount`
   21, ctor init, toggle branches, both list vectors, option assignments, the
   `||`, the four result lines — done when `DrawSaveData` and `DrawConfirm`
   hold identical shapes and no existing row constant moved.
7. `app/tools/easy_modes_verify.py` — done when `selftest` passes all twelve
   §6 cases against `data/vanilla/dvdroot_ps4`.
8. `boss_verify.py --easy`, `ui_scroll_verify.py` counts, `pool_verify.py`
   worst case — done when all three tools pass.
9. Clean rebuild and `.pkg`.

**Invariants** this milestone must not break: every item in §3.1 — in
particular the RNG stream, the call-site position, the append-last row rule and
the unconditional model merge.

**Verification:** all of §6's Build and Automated rows.

**Completion gate.** Everything above passes, the `.pkg` builds, and the
milestone is handed to the developer for the §6 Hardware steps. Do not begin
any further work.

### Stop conditions

Halt and report rather than deciding, if any of these occur:

* the build fails in a way this plan did not anticipate;
* a selftest case cannot be made to pass without weakening what it asserts;
* the measured placement counts differ from §4.1 — that means the data, not the
  code, disagrees with the plan;
* `c2521` cannot be resolved in a map being written;
* making a check pass would require writing a fourth field, a different map, or
  touching a param file;
* an existing UI row index would have to move;
* a §3.1 invariant cannot be preserved, or a change is needed in a file not
  listed in §5.

---

## 8. Open questions

None — spec §10 D1–D4 and the repository settle every implementation question
this feature raises.

---

## 9. Decisions

| # | Date | Decision | Taken by |
| - | ---- | -------- | -------- |
| P1 | 2026-09-16 | The four rows are appended **last** on both screens, in backlog order — `EASY SHADOWS`, `EASY ROM`, `EASY FAILURES`, `EASY EMISSARY` — following the placement rule the last three settings already use, so no existing row index moves. The reference window's left-to-right checkbox layout is treated as layout accident, not intent | developer |
| P2 | 2026-09-16 | One progress line per enabled setting, each carrying its count; no `SKIPPING` counterparts | developer |
| P3 | 2026-09-16 | The replacement identity is baked as named constants, not captured at runtime from map data | developer |
| P4 | 2026-09-19 | `ApplyEasyModes` is self-contained and resolves the `c2521` model index itself, rather than taking the caller's `enemyModelIndex` | planner |

---

## 10. Changes during implementation

| Date | Change | Reason |
| ---- | ------ | ------ |
| 2026-09-19 | `kEasyModeModelName` is declared `const char* const`, not §4.4's `const char*` | A non-const pointer at namespace scope has external linkage, so the plain form is a duplicate symbol as soon as a second translation unit includes the header — and `EnemyRandomizer.cpp` does. Same spelling the port already uses for a named string constant (`EnableWizardScreen.cpp`'s `kEnemyFailPrefix`). Name and value unchanged |
| 2026-09-19 | `easy_modes_verify.py verify` takes an extra `--no-randomizers` argument beyond §6's `[shadows] [rom] [failures] [emissary]` | §6's third bullet is conditional — settings-off maps are frozen "when the run had every randomizer off", and otherwise differ "as the enemy and boss passes allow". The flag is how the caller says which tree it has. Without it the survivor and settings-off checks are reported and not asserted, so the tool never passes judgement on the enemy pass, which is `boss_verify.py`'s and `pool_verify.py`'s job |
| 2026-09-19 | The stale worst-case figure in `RandomizerDefaultsStore.cpp`'s buffer comment updated 585 → 669 | One word in the comment directly above the `snprintf` this milestone extends. It was already wrong (611) before this feature and this change would have made it wronger. The buffer itself is unchanged, as §5 requires |
| 2026-09-19 | `boss_verify.py`'s `--easy` suppression is computed by importing `easy_modes_verify.parse_table`, not by parsing `EasyModes.h` a second time | §5 asks the flag to suppress "exactly the (map, name) pairs the table selects". Two parsers of one header is the drift `§3.3`'s first hazard is about. The import is deferred into the function because `easy_modes_verify` imports `boss_verify` at load time |

---

<!--

The spec describes WHAT the feature should do.
This plan describes WHAT THE IMPLEMENTER DOES, and how it is verified.
plan-evidence.md describes WHY the plan says what it says.
log.md describes HOW the plan got here.

-->
