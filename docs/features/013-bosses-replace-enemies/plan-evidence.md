# Plan Evidence 013 — Bosses Can Replace Enemies

**Plan:** `docs/features/013-bosses-replace-enemies/plan.md`

**Spec:** `docs/features/013-bosses-replace-enemies/spec.md`

---

> **This document is the investigation behind the plan, not instructions.** It is
> revisable; `log.md` holds the chronology.

---

## E1. Reference trace

### E1.1 The pass itself

`reference/Randomizer/MainWindowComponents/RandomizeFunctions.cs:1460-1552`,
`InsertBossesVoid(string currentMap, List<string> nonoList)`.

Read in full for this plan. Confirmed, line by line:

* **`:1462-1469`** — the Moon Presence strip. A `for` loop that, on finding the
  first entry containing `c5400`, removes it and sets `i = Count + 3` to leave the
  loop. It therefore removes **one** entry per call. Harmless in practice because
  the list is `Distinct()`-ed before the pass (E1.2), so exactly one `c5400` entry
  exists; measured (E4 M2), and that is why the port's equivalent is a plain
  filter rather than a reproduction of the loop.
* **`:1480-1489`** — the target test is `nonoList` only: `Name.Contains(pattern)`
  over the fixed unused+boss list. **No skip list, no caged-dog protection, no
  per-zone chance, no size gate, and no `m28` forced-maiden override.** The port's
  three protection tests (spec D-C) are additions; the `m28` override is
  deliberately *not* carried over, and costs nothing, because the six forced
  maidens are themselves on the fixed exclusion list and so are not eligible
  targets either way.
* **`:1491-1497`** — two draws are taken *before* the percentage roll, one into
  `insertBossesString` and one into `chaliceBossString`, and the chalice one is
  discarded for a non-`m29` map. So the reference consumes **three** `Next` calls
  per eligible placement regardless of outcome. Not reproduced: the port's RNG
  stream has no parity requirement with the reference's, and neither this plan nor
  any shipped pass claims one.
* **`:1514`** — `Console.WriteLine(enemyData[0])`, unguarded on a list the
  exclusion filter at `StartFunctions.cs:786-802` can empty. A real defect in the
  reference and irrelevant to the port, which logs differently.
* **`:1521-1524`** — the roll: `float assass = universalRand.Next(0, 101);` then
  `if (assass <= bossPercentage)`. Inclusive upper bound plus `<=` means a
  `bossPercentage` of 0.0 still converts a placement when the draw is 0, i.e. one
  in 101. `bossPercentage` has no field initializer (`FieldContainer.cs:169`) and
  is written only by the slider's handlers (`UIComponents.cs:194-309`). Confirms
  spec §3 point 2 and §7 item 1 exactly.
* **`:1536-1547`** — the write:
  `NPCParamID = tempNpcParamInt;` then
  `if (tempNpcParamInt == 551000) { tempThinkIdInt = 551111; } else { ThinkParamID = tempThinkIdInt; }`
  then `ModelName = modelName;`. The `551000` branch assigns a **local** and never
  the field, so that one identity keeps the replaced creature's `ThinkParamID`.
  Confirms spec §3 point 5. **This is the trace that settles plan P4:** the
  reference's only expression of "the replaced creature's AI" is *not writing the
  field*, so that is what the port's second state does.

### E1.2 The driver and the fourteen area gates

`StartFunctions.cs:1013-1133`.

* `:1015-1021` — `insertBossesString = insertBossesString.Distinct().ToList();`
  runs once, immediately before the pass. **This is why the list is a set of
  distinct `npc*think*model` strings** even though the harvest appends
  duplicates across map variants.
* `:1038-1101` — the fourteen `if (boss<Area>)` blocks, in this order, covering
  **22 map files**:

  | # | Flag | Map files |
  | - | ---- | --------- |
  | 1 | `bossHemwick` | `m22_00_00_00` |
  | 2 | `bossOldYharnam` | `m23_00_00_00`, `m23_00_00_01` |
  | 3 | `bossCathedralWard` | `m24_00_00_00`, `m24_00_00_01` |
  | 4 | `bossCentralYharnam` | `m24_01_00_00`, `m24_01_00_01`, `m24_01_00_11` |
  | 5 | `bossUpperCathedralWard` | `m24_02_00_00`, `m24_02_00_01` |
  | 6 | `bossCainhurst` | `m25_00_00_00` |
  | 7 | `bossMensis` | `m26_00_00_00` |
  | 8 | `bossForbiddenWoods` | `m27_00_00_00`, `m27_00_00_01` |
  | 9 | `bossYahargul` | `m28_00_00_00`, `m28_00_00_01` |
  | 10 | `bossByrgenwerthLecture` | `m32_00_00_00`, `m32_00_00_01` |
  | 11 | `bossFrontier` | `m33_00_00_00` |
  | 12 | `bossNightmare` | `m34_00_00_00` |
  | 13 | `bossResearchHall` | `m35_00_00_00` |
  | 14 | `bossHamlet` | `m36_00_00_00` |

  `m21_00_00_00` and `m21_01_00_00` appear in no block, confirming spec §2's claim
  that the Hunter's Dream and the Abandoned Old Workshop are untouched.
  `:1103-1131` is the commented-out chalice branch behind `bossChalices`.

  The grouping is what the plan's area table keys on. **The prefixes that scope
  these fourteen groups are `m22`, `m23`, `m24_00`, `m24_01`, `m24_02`, `m25`,
  `m26`, `m27`, `m28`, `m32`, `m33`, `m34`, `m35`, `m36`** — checked for mutual
  exclusivity and against `m21` in E4 M7.

* **Position in the run.** The pass sits after boss randomization
  (`:952-1010`) and, transitively, after enemy randomization. This corroborates
  spec D-N: the port's second-pass structure *matches* the reference's ordering
  rather than departing from it.

### E1.3 The harvest, and the one place the spec is wrong

`RandomizeFunctions.cs:1556-1846`, `GenerateBossList`.

The line that matters is **`:1657`**, `insertBossesEnemy.Add(...)`, and where it
sits. Reading `:1639-1686`:

```
else                                  // :1639  — nothing in the reject chain matched
{
    addEnemy = true;
    if (m29) { ... }                  // :1643
    if (!m29) insertBossesEnemy.Add(enemy);   // :1657  <-- THE HARVEST
    if (!m26 && name.Contains("c0000_0005")) addEnemy = false;   // :1660-1666
}
if (map.Contains("34")  && npc == 210030)     addEnemy = false;  // :1669-1675
if (map.Contains("m28") && name.Contains("c2100")) addEnemy = false;  // :1677-1683
```

So three de-eligibility tests run **after** the harvest and therefore do not
filter the insertion list:

* `!m26 && c0000_0005`,
* `m34 && NPCParamID == 210030`,
* `m28 && name contains c2100`.

Measured consequence (E4 M2): the insertion list contains **three Witch of
Hemwick identities the spec's count omits** — `m28`'s `210005*210000` and
`210005*210002`, and `m34`'s `210030*210030`. The total is **33 identities across
23 models**, or **32 across 22** once `c5400` is stripped, not 30/29.

The reject chain itself (`:1587-1638`) is: the six rejected NPC ids, `EntityID ==
-1`, `tempList.Contains(enemy)`, `NPCParamID` 0 or -1, `ThinkParamID == -1`,
`c5070` (with `!lesserBossesBool`), and `c3060 && NPCParamID != 210306016`. The
`tempList.Contains` test is **vacuous**: `MSBB.Part.Enemy` is a class with no
`Equals` override (checked — `SoulsFormats/Formats/MSB/MSBB/PartsParam.cs` has
neither `override bool Equals` nor `IEquatable`), and `tempList` only ever holds
objects from the same `tempGUY`, each added at most once, so no *other* placement
can ever compare equal. It is therefore omitted from the port's
`InsertionEligible()` rather than reproduced.

`:1838-1846` serializes `insertBossesEnemy` to `npc*think*model` strings, dropping
`ThinkParamID == 1`. `StartFunctions.cs:733-756` calls `GenerateBossList` over
**16** files (the `_00` variants plus both `m21`); the port's
`CollectBossCandidates` runs over `BossMapOrder()`'s **17** (mostly `_01`
variants). Measured: the harvest is set-identical over both, and over the enemy
pass's 24-file `kBaseMaps` order (E4 M2). The port may therefore harvest during
its existing read pass without a new map-order constant.

`StartFunctions.cs:786-802` is the only interaction between the boss-exclusion
text box and the insertion list; the port ships that instrument inverted, as a
picker, so the interaction is D-B's own table instead.

`oopsAll` suppression: `StartFunctions.cs:705` skips `GenerateBossList`, leaving
`insertBossesString` empty, and `:1489` (`insertBossesString.Count > 0`) makes the
pass a silent no-op. That is the reference's own precedent for D-G's quiet skip.

### E1.4 Where the trace confirmed the spec, and where it did not

Confirmed: §3 points 1–5 in full; the fourteen flags gate this pass and nothing
else; the pass ordering; the `bossPercentage` defect; the abandoned think-id edit;
the chalice branch being commented out; the `Distinct()` dedupe; the
quiet-no-op-on-empty-list precedent; and every per-area map grouping.

Not confirmed: **the insertion list's identity count** (§4, §5, §8 criteria 9 and
10 say 29 across 22; measured 32 across 22 — E4 M2), and the derived per-model
variant breakdown (Witch of Hemwick supplies 5, not 2). The *model* count, the
*model set*, and therefore the 22-row picker and every design decision resting on
it are exactly as the spec describes.

---

## E2. What exists in the port

### E2.1 The pass framework

`app/src/Randomizer/EnemyRandomizer.cpp`. `EnemyRandomizerJob::State::Phase` is
`Mirror → ReadMaps → BuildPool → MergeModels → [BossCollect → BossAssign] →
[TreasureCollect → TreasureAssign] → WriteMaps → Emevd → [ItemData] → Finished`.
Every map is parsed once into `maps`, mutated in memory across phases, and written
once in `StepWriteMap`.

`StepWriteMap`'s internal order is: the enemy loop (gated on
`options.randomizeEnemies`), then `ApplyEasyModes`, then the `BossScalingMaps()`
loop, then `Serialize` + `DcxCompress` + write. That fixes exactly where the
insertion pass must go — after the easy-mode pass, before the scaling loop —
because criterion 16 requires an inserted boss in a scaled area to carry that
area's variant, and the scaling pass rewrites whatever `NPCParamID` it finds.

`StepMergeModels` is **unconditional** and appends every enemy model name seen in
any of the 24 base maps to every map's `ModelParam`. That is what makes a boss
model referenceable from a world map with no new file-format work; verified that
all 22 insertion models are in that union (E4 M3).

Boss randomization runs in its own phases **before** the write loop, so by the
time the insertion pass sees a map, both the enemy shuffle for that map and the
whole boss assignment are already applied. The reference's ordering is preserved
without any reordering of phases.

### E2.2 The three protection tests

* `EnemyExclusionList.h` — `IsExcludedEnemyName`, 103 patterns. `M28ForcedMaidenList()`
  is the six-placement override, which belongs to the enemy loop only.
* `EnemySkipList.h` — `BuildSkipPatterns(selection)` / `IsSkippedName(name, patterns)`.
  `State`'s constructor already flattens the selection once per run.
* `CagedDogList.h` — `IsProtectedCagedDog(mapName, entityID)`, ten
  `(prefix, entityID)` entries covering 26 placements. Its use of a **map-name
  prefix plus an id** is the precedent the plan's area table follows.

`enemy_lookup.overwritable_placements` mirrors the first of these exactly and is
the eligible-target oracle the verifier reuses rather than reimplements.

### E2.3 The boss pool, and why it is not reused

`BossRandomizer.cpp`'s `CollectBossCandidates` is the closest neighbour and is
**not** the insertion harvest:

| | Boss pool | Insertion list |
| - | --------- | -------------- |
| `PoolEligible` | all nine tests | first six only (E1.3) |
| `kInsertReject` (15 patterns) | applied | not applied |
| per-map `npc` dedupe | applied | not applied |
| dedupe | by full identity, then a model-distinct `refill` | by full identity only |
| filtered by | `bossesIncluded` (17 rows) | `insertedBosses` (22 rows) |
| measured size | 18 identities / 17 models | 32 identities / 22 models |

Conclusion: **extend nothing in `BossRandomizer.cpp`.** The rules are different
enough that a shared function would need three flags, and the file is shipped and
hardware-tested. `BossIdentity` is reused as a plain value type. `DrainPool`'s
*shape* — draw, erase, refill-when-empty — is the precedent the plan follows, but
its body is not: `DrainPool` also erases same-model entries and refills from a
model-deduped list, both of which would break §8 criterion 10.

### E2.4 The settings model

`SettingsModel.h` / `.cpp`. `SettingKind` is `Toggle, SaveChoice, EnemyPool,
EnemySkip, BossPool, TrickWeaponPool, LeftHandWeaponPool`. `SettingDef` is
`{ id, category, kind, label, bool RandomizerDefaults::* flag, help }`, and
`kSettings` is a 21-entry aggregate-initialised array.

Checked: because C++ value-initialises **trailing** missing initialisers in
aggregate initialisation, adding members after `help` leaves all 21 shipped rows
compiling untouched. Adding them anywhere earlier would require editing every
row. This is why the plan specifies the position.

Both hosting screens reach every setting only through `SettingValueText`,
`AdjustSetting`, `IsDrillIn` and the four `Selection*` functions
(`SetupDefaultsScreen.cpp:134,218-228,303,422`;
`WorldEditorScreen.cpp:686-696,812,1458,1610`). No screen switches on kind, so
none needs a new branch. Confirms spec §4.

`SaveChoice`'s reuse question, re-verified rather than taken from the spec:
`SettingsModel.cpp:220-221` is
`return (values.*(def.flag)) ? "START FRESH" : "KEEP EXISTING";` — the two labels
are inside the switch, so a second `SaveChoice` row would render save-data
wording; and `settings_ui_verify.py:893-895` asserts
`[labels where kind == "SaveChoice"] == ["SAVE DATA"]`. Both blockers are real.
`AdjustSetting`'s `twoState` test and `IsDrillIn` are already generic, so the
*machinery* transfers to a new kind unchanged.

### E2.5 `CountChangedSettings`

`WorldEditorScreen.cpp:854-872` decides bool-versus-selection on
`if (def.flag) … else SelectionFlags(...)`. A `Number` row carries no `flag` and
no selection, so it would take the `else` branch, get `nullptr`, `continue`, and a
changed count would never appear in the history line's "N CHANGED". Not a
compile error and not caught by any current verifier case — hence the explicit
hazard and the M1 step.

`WorldStore.cpp:394`'s `SameRecipe` is defined as "they serialize identically"
through `FormatSettings`, so the eight new keys are covered with no edit.

### E2.6 What the verifiers will reject

* `settings_ui_verify.py:130` extracts a flag with `&RandomizerDefaults::(\w+)` —
  which matches an `int RandomizerDefaults::*` member too, so the case-1
  invariant "every entry is a toggle or a save choice with a flag, or a pool
  without one" fails unless the parser learns the kinds.
* `parse_defaults` (`:167-175`) finds `bool (\w+) = ` and a fixed list of five
  selection type names; `len(selections) == 5` is asserted at `:879`.
* `KIND_TO_COUNT` (`:443-454`) and `widest_value` (`:456-465`) raise `KeyError`
  on an unknown kind.
* Case 3 (`:900-925`) compares `kSettings` against **§7.1 of
  `docs/features/randomizer-settings-ui/spec.md`** through `PROSE_TO_LABEL`. That
  spec's table currently lists "Bosses Can Replace Enemies" and "Per-Zone
  Toggles" in the *backlog* column; both must move. Precedent: the trick-weapon
  and left-hand-weapon rows are already in the current column.
* Case 9 (`:1944-1951`) measures the picker band against
  `EnemyPoolTable.h`, `EnemySkipTable.h` and `BossPoolTable.h` only — the two
  weapon tables are already absent, which is a pre-existing gap. Both new tables
  should be added to that list.
* `pool_verify.py:688-734` pins the settings block at exactly 728 bytes and
  narrates every previous rebase in a comment. Its `char buf\[(\d+)\]` parse picks
  up the buffer size from the source, so growing the buffer needs no change there
  beyond the block figure.

Geometry: `PANE_FIRST_Y=330`, `PANE_PITCH=76`, `PANE_BOTTOM=880` give
`visible = 8` rows with MORE ABOVE / MORE BELOW hints. The Bosses pane goes 2 → 10
rows and becomes **the first settings pane that scrolls**. The mechanism is
already implemented and already checked by `ui_scroll_verify.py`; nothing new is
needed, which is why the M2 gate is optional rather than required.

### E2.7 Tools

`gen_pool_table.py` builds a table from `(models_fn, weights_fn)` and supports
`--check`. `boss_verify.py` supplies `Msbb`, `read_dcx`, `is_boss_name`,
`REJECT_NPC`, `pool_eligible`, `build_pool` and `boss_pool_models`.
`enemy_lookup.py` supplies `overwritable_placements`, `exclusion_reason`,
`zone_scaled_npc`, `engine_pool`, and — already — **`enemies()["pos"]`, three
floats unpacked from `0x28`**, so the position reader the mirror needs exists
today. `names.py` supplies `model_name` and `map_name`.

---

## E3. Alternatives considered

| Approach | Why rejected |
| -------- | ------------ |
| Give each ticked boss an entry in the ordinary enemy pool and draw uniformly | The measured failure D-M forecloses: the boss share becomes a function of how many bosses are ticked (6.2% at 22 ticked, 0.30% at one), so "up to 5 Amygdalas" yields 0.16 expected in the smallest map. A ceiling only removes bosses and cannot repair it. E4 M6 |
| Decide insertion per placement during the enemy loop (one pass) | Spacing needs the whole map's eligible set before choosing any placement; a sequential draw cannot supply it. This is exactly what D-N's second pass dissolves |
| `UP TO N` as a single uniform draw of `0..N` | Offered to the developer and declined (log.md, Q10). The per-placement form with a derived probability is what §7 item 2 describes, and it is the same shape as the reference's coin flip |
| Extend `CollectBossCandidates` with a "for insertion" flag | Three of its rules differ and two must be skipped; the flag count makes the function harder to read than a second function, and the file is shipped and hardware-tested. E2.3 |
| Reuse `BossPool` / `DrainPool` for the insertion pool | `DrainPool` erases same-model entries and refills from a model-deduped list. Either behaviour breaks §8 criterion 10's "use counts differ by at most one" |
| Reuse `SettingKind::SaveChoice` for the two named choices | Two verified blockers: hardcoded labels at `SettingsModel.cpp:220` and the one-`SaveChoice` assertion at `settings_ui_verify.py:893`. E2.4 |
| One new pool kind shared by both new pickers | `ModelPoolSelection<22>` and `ModelPoolSelection<14>` are distinct types, so they cannot share a pointer-to-member — the same reason `SettingsModel.h` already gives for the existing five |
| A full-screen numeric editor for the count, modelled on `Step::EditSeed` | A 0–15 range is 16 positions; `AdjustSetting`'s clamped ±1 on Left/Right reaches any of them in at most 15 presses and adds no new screen mode. The editor stays available if the range ever grows |
| Store the area selection as a bitmask or per-area booleans in `RandomizerDefaults` | `ModelPoolSelection` already gives encode/decode, the length fail-safe, the picker, and the "N OF 14" row for free. A bespoke type would reimplement all four |
| Key the area table on the **live** map file name rather than a prefix | The pass must also visit the unused variants (the reference does), so a single file name would silently drop 8 of the 22 files |
| Extend the 103-pattern fixed exclusion list to keep bosses out of particular places | Forbidden by `docs/design-decisions.md` and `docs/enemy-exclusion-history.md`: a well-meant extension already froze 119 placements |
| Apply the enemy pass's ×2 size gate to insertion | The reference has none, and measuring what it would admit shows it excludes nothing outright — it would only concentrate large bosses onto large mobs. Spec §6 puts it out of scope |
| Harvest the insertion list over `BossMapOrder()` to match the boss pool | Measured set-identical to the `kBaseMaps` order (E4 M2), and harvesting in `StepReadMap` costs no extra pass and no new constant |

---

## E4. Measurements

All against `data/vanilla/dvdroot_ps4`. Scripts were throwaway and are reproduced
below in enough detail to re-derive each figure; every one of them imports the
port's own tools rather than restating a rule.

Common preamble for every script below:

```python
import sys; sys.path.insert(0, "app/tools")
import enemy_lookup as EL, boss_verify as BV
ROOT = "data/vanilla/dvdroot_ps4"
maps = EL.load_base_maps(ROOT)
```

### M1 — Eligible targets per area and per map file

| Quantity | Value | How |
| -------- | ----: | --- |
| Eligible targets, all 24 base map files | **2,269** | `len(EL.overwritable_placements(ROOT, maps))` |
| …summed over the 22 area files of E1.2 | **2,269** | same, grouped by map name |
| `m21_00_00_00` / `m21_01_00_00` contribution | **0 / 0** | same |
| Eligible targets over the fourteen **live** area files | **1,381** | the `BossScalingMaps()` file per area |

Per live area — count, distinct creature kinds, bounding diagonal in paces:

| Area | Live file | Targets | Kinds | Diagonal |
| ---- | --------- | ------: | ----: | -------: |
| Hemwick Charnel Lane | `m22_00_00_00` | 73 | 9 | 440 |
| Old Yharnam | `m23_00_00_01` | 81 | **5** | 268 |
| Cathedral Ward | `m24_00_00_01` | 87 | 16 | 363 |
| Central Yharnam | `m24_01_00_01` | 149 | 17 | 474 |
| Upper Cathedral Ward | `m24_02_00_01` | **54** | 7 | 274 |
| Forsaken Castle Cainhurst | `m25_00_00_00` | 96 | 7 | 302 |
| Nightmare of Mensis | `m26_00_00_00` | 108 | 17 | 391 |
| Forbidden Woods | `m27_00_00_01` | **178** | 18 | 523 |
| Yahar'gul, Unseen Village | `m28_00_00_01` | 118 | 14 | 625 |
| Byrgenwerth | `m32_00_00_01` | 72 | 8 | 325 |
| Nightmare Frontier | `m33_00_00_00` | 60 | 8 | 264 |
| Hunter's Nightmare | `m34_00_00_00` | 106 | 12 | 465 |
| Research Hall | `m35_00_00_00` | 84 | 9 | **184** |
| Fishing Hamlet | `m36_00_00_00` | 115 | 9 | 274 |

Every figure reproduces spec §4's table exactly. Old Yharnam's five kinds are
`c1000`, `c1051`, `c1090`, `c1170`, `c1290`, confirming the five-skip-rows
starvation case; the next-narrowest are Upper Cathedral Ward and Cainhurst at
seven, then Byrgenwerth and Nightmare Frontier at eight — also as the spec says.

The `BossScalingMaps()` live-file choice was read from
`app/src/Randomizer/BossParamScaling.h`: `m22_00_00_00`, `m23_00_00_01`,
`m24_00_00_01`, `m24_01_00_01`, `m24_02_00_01`, `m25`, `m26`, `m27_00_00_01`,
`m28_00_00_01`, `m32_00_00_01`, `m33`, `m34`, `m35`, `m36`, plus the two `m21`
files. Note the consequence: **only the live file of a doubled area is scaled**,
so an inserted boss in an unused variant keeps its arena stats. Harmless — the
retail game never loads those files — but the verifier has to expect it.

### M2 — The insertion list *(corrects spec §4, §5 and §8 criteria 9–10)*

Replays E1.3's rules — `is_boss_name`, then the six-test reject chain, then
`think == 1`, then `Distinct()` by `(npc, think, model)` — over three map orders:

```python
def harvest(order):
    out = []
    for mn in order:
        m = maps[mn]
        for e in EL.enemies(m):
            if not BV.is_boss_name(e["name"]): continue
            if e["npc"] in BV.REJECT_NPC: continue
            if e["entity"] == -1 or e["npc"] in (0, -1) or e["think"] == -1: continue
            if "c5070" in e["name"]: continue
            if "c3060" in e["name"] and e["npc"] != 210306016: continue
            if e["think"] == 1: continue
            t = (e["npc"], e["think"], e["model"])
            if t not in out: out.append(t)
    return out
```

| Quantity | Value |
| -------- | ----: |
| Harvest over the reference's 16 files | **33 identities / 23 models** |
| Harvest over `BossMapOrder()`'s 17 | 33 / 23, **set-identical** |
| Harvest over `kBaseMaps`' 24 | 33 / 23, **set-identical** |
| After removing model `c5400` (Moon Presence) | **32 identities / 22 models** |
| `c5400` entries before removal | **1** — which is why the reference's one-shot strip loop is correct despite being written wrong |

Per-model identity counts, which are the `poolEntries` column of the new table:

Witch of Hemwick **5**, Living Failure **4**, Shadow of Yharnam **3**, Ludwig
**2**, and one each for Amygdala, Blood Starved Beast, Celestial Emissary, Cleric
Beast, Darkbeast Paarl, Ebrietas, Father Gascoigne, Father Gascoigne (Beast),
Gehrman (Boss), Lady Maria, Laurence, Martyr Logarius, Mergo's Wet Nurse, Rom,
Small Celestial Emissary, Standing Orphan of Kos, Standing Orphan of Kos (Flaps),
Vicar Amelia. Total 32 across 22 models.

**What this replaces.** Spec §4 says 30 identities across 23 models, 29 across 22
after Moon Presence, and gives the breakdown as "Living Failure 4, Shadow of
Yharnam 3, Witch of Hemwick and Ludwig 2 each, the remaining eighteen 1 each".
The first measurement was wrong by three, all of them Witch of Hemwick:

| Identity | Source | Why the spec omitted it |
| -------- | ------ | ----------------------- |
| `210005*210000*c2100` | `m28_00_00_00` `c2100_0000` | the reference's `m28 && c2100` test runs **after** the harvest line |
| `210005*210002*c2100` | `m28_00_00_00` `c2100_0001` | as above |
| `210030*210030*c2100` | `m34_00_00_00` `c2100_0000` | the reference's `m34 && npc == 210030` test runs **after** the harvest line |

The spec's figure is what you get by treating the harvest as governed by the final
value of `addEnemy`. E1.3 shows the add is at `:1657` and those tests are at
`:1669` and `:1677`. The **model** set is unaffected, because `m22` already
contributes `c2100`, so the 22-row picker, D-B, and the 13.8%-versus-3.4%
weighting *argument* all stand; only the counts move — Living Failure is 12.5% of
arrivals rather than 13.8%, and Witch of Hemwick is 15.6%.

### M3 — Model availability

| Quantity | Value | How |
| -------- | ----: | --- |
| Insertion models present in the union of all 24 base maps' `ModelParam` enemy entries | **22 of 22** | collect `m.model_name(i)` for every model entry whose type word at `0x08` is 2, over `EL.BASE_MAPS`, and compare against the 22 |

Since `StepMergeModels` copies that union into every map unconditionally, every
insertion model resolves to a model index in every map. Confirms spec §4 and means
no new file-format capability.

### M4 — Nothing protected is reachable

| Quantity | Value | How |
| -------- | ----: | --- |
| Boss-named placements across the 24 base maps | **62** | `BV.is_boss_name` over `EL.enemies` |
| …of which are eligible targets | **0** | set-intersect with `overwritable_placements` |
| Caged-dog placements (from `CagedDogList.h`, parsed) that are eligible targets | **26** — 6+6+6 in the three `m24_01` files, 4+4 in the two `m27` files | `(prefix, entityID)` match |

The 0 is the load-bearing one: every boss placement is already spared by the fixed
exclusion list, so §8 criteria 13 and 15 hold by construction rather than by a
rule this feature adds. Note the spec's "42 boss-named placements across the boss
maps" counts a different denominator than 62 across all 24 base files; the claim
that matters — none is a target — is confirmed either way.

### M5 — Positions and spacing

Offset, derived three ways and agreeing:

1. `reference/SoulsFormats/SoulsFormats/Formats/MSB/MSBB/PartsParam.cs`,
   `Part(BinaryReaderEx)`: `descOffset` i64 @0x00, `nameOffset` i64 @0x08,
   `ModelLocalID` @0x10, `Type` @0x14, ID @0x18, `modelIndex` @0x1C,
   `placeholderOffset` i64 @0x20, **`Position = ReadVector3()` @0x28**, `Rotation`
   @0x34, `Scale` @0x40, three 8-word group arrays @0x4C/0x6C/0x8C, `UnkFA4`
   @0xAC, `baseDataOffset` @0xB0, `typeDataOffset` @0xB8.
2. `app/src/Msb/Msbb.h`'s own documented offsets bracket it exactly: `Type` 0x14
   and `modelIndex` 0x1C below, `baseDataOffset` 0xB0 and `typeDataOffset` 0xB8
   above, with every byte between accounted for by the arithmetic in (1).
3. `app/tools/enemy_lookup.py`'s `enemies()` already unpacks
   `struct.unpack_from("<fff", blob, 0x28)` and produces plausible world
   coordinates.

| Quantity | Value |
| -------- | ----: |
| Distinct coordinates among the 1,381 live-area targets | **1,367** |
| Worst duplication | Nightmare of Mensis 101 distinct for 108; Forbidden Woods 170 for 178 |

So a zero distance is reachable and a spacing implementation must tolerate it —
hence squared distances in `double`, no division, lowest index on a tie.

Mean minimum pairwise distance over the fourteen live areas, uniform draw versus
greedy farthest-point, 12 trials per area per count, `random.Random(1234)`:

| Count | Uniform | Spread | Worst area, spread |
| ----: | ------- | ------ | ------------------ |
| 5 | 17 – 53 | 60 – 156 | 60 (Research Hall) |
| 10 | 6 – 16 | 36 – 89 | 36 (Research Hall) |
| **15** | 2 – 10 | **20 – 67** | **20 (Upper Cathedral Ward)** |
| 20 | 2 – 8 | 12 – 54 | 12 (Upper Cathedral Ward) |
| 25 | 1 – 5 | 10 – 47 | 10 (Upper Cathedral Ward) |

This reproduces spec §4's table within trial noise at every count, and reproduces
the two figures D-K rests on exactly: 12 at a count of 20 and 10 at 25, against
20 at 15. **D-K's maximum of 15 is confirmed measured, not assumed.** Research
Hall is the worst case up to a count of 10 and Upper Cathedral Ward from 15 up,
which also matches.

### M6 — The 333-entry enemy pool, and what pool-mixing would have cost

| Quantity | Value | How |
| -------- | ----: | --- |
| `engine_pool` size | **333 entries across 82 models** | `EL.engine_pool(ROOT, maps=maps)` |

Which reproduces D-M's arithmetic: one pool entry per ticked boss gives a boss
share of `n / (333 + n)` — 6.2% at 22 ticked, 0.30% at one — so the expected
count in the 54-target map runs from 3.3 down to 0.16 as the player narrows the
list. The spec's 8.01% figure used 29 ticked entries; with the corrected 22-model
list the number is 6.2%, and the *conclusion* is unchanged and in fact slightly
stronger, because the spread between "all ticked" and "one ticked" is what breaks
D-L's governing sentence.

### M7 — The area table

The fourteen prefixes of E1.2, checked against all 24 base map names:

| Prefix | Matches | Live file |
| ------ | ------- | --------- |
| `m22` | `m22_00_00_00` | same |
| `m23` | `m23_00_00_00`, `m23_00_00_01` | `_01` |
| `m24_00` | `m24_00_00_00`, `m24_00_00_01` | `_01` |
| `m24_01` | `m24_01_00_00`, `m24_01_00_01`, `m24_01_00_11` | `_01` |
| `m24_02` | `m24_02_00_00`, `m24_02_00_01` | `_01` |
| `m25` / `m26` | one each | same |
| `m27` | `m27_00_00_00`, `m27_00_00_01` | `_01` |
| `m28` | `m28_00_00_00`, `m28_00_00_01` | `_01` |
| `m32` | `m32_00_00_00`, `m32_00_00_01` | `_01` |
| `m33` / `m34` / `m35` / `m36` | one each | same |

No prefix matches a file belonging to another area, every one of the 22 area files
is matched exactly once, and neither `m21` file is matched. 14 prefixes, 22 files.

Frozen row order — display name, then prefix, matching every other generated
table: Byrgenwerth, Cathedral Ward, Central Yharnam, Fishing Hamlet, Forbidden
Woods, Forsaken Castle Cainhurst, Hemwick Charnel Lane, Hunter's Nightmare,
Nightmare Frontier, Nightmare of Mensis, Old Yharnam, Research Hall, Upper
Cathedral Ward, Yahar'gul Unseen Village.

Widths, measured with `settings_ui_verify.py`'s own atlas metrics at
`PICKER_ROW_SCALE = 3` against the picker band of 1,080 px and the widest flag
word of 171 px:

| Table | Widest row | Gap | Minimum |
| ----- | ---------: | --: | ------: |
| AREAS INCLUDED (names only) | 500 px — `FORSAKEN CASTLE CAINHURST` | 453 | 60 |
| INSERTED BOSSES (id + name) | 791 px — `C4520 LADY MARIA OF THE ASTRAL CLOCKTOWER` | 162 | 60 |

The 791 px row already exists in `BossPoolTable.h`, so the picker's widest row
does not change.

Settings-pane widths at `ROW_SCALE = 3`, budget `label + 40 + widest value ≤ 700`:

| Row | Widest value | Width | Spare |
| --- | ------------ | ----: | ----: |
| BOSSES CAN REPLACE ENEMIES | YES | 578 | 122 |
| BOSS REPLACEMENTS | EXACTLY | 528 | 172 |
| BOSS REPLACEMENTS PER MAP | 15 | 555 | 145 |
| REPLACE ALL ENEMIES | YES | 452 | 248 |
| AREAS INCLUDED | 14 OF 14 | 439 | 261 |
| INSERTED BOSSES | 22 OF 22 | 432 | 268 |
| SPREAD BOSSES OUT | YES | 407 | 293 |
| **BOSS BEHAVIOUR** | REPLACED ENEMY'S AI | **663** | **37** |
| *(alternative)* BOSS AI | REPLACED ENEMY'S AI | 506 | 194 |

`BOSS BEHAVIOUR` becomes the widest row in the app, displacing
`START WITH A TRICK WEAPON` at 649 px. It fits. That 37 px is what §8 Q1 is about.

### M8 — Settings-block bytes

`pool_verify.py:688-733` pins the block at **728** today. Per-key cost, key
length + `=` + value + `\n`:

| Key | Value | Bytes |
| --- | ----- | ----: |
| `bosses_can_replace_enemies` | `1` | 29 |
| `boss_replacements_exact` | `1` | 26 |
| `boss_replacements_per_map` | `15` | 29 |
| `replace_all_enemies` | `1` | 22 |
| `inserted_boss_areas` | 14 chars | 35 |
| `inserted_bosses_included` | 22 chars | 48 |
| `space_out_inserted_bosses` | `1` | 28 |
| `inserted_boss_keeps_enemy_ai` | `1` | 31 |
| **Total** | | **248** |

| Landing order | Block | Worst `defaults.cfg` |
| ------------- | ----: | -------------------: |
| 013 alone | 976 | 1,027 |
| 013 + 011 (+47) | 1,023 | 1,074 |
| 013 + 027 (+15) | 991 | 1,042 |
| **013 + 011 + 027** | **1,038** | **1,089** |

1,038 is **14 bytes over `char buf[1024]`**, and `snprintf`'s clamp would drop the
tail of the last key in silence. At 2,048 the three together leave 1,010 spare.
The read buffer in `LoadRandomizerDefaults` is already `char buf[4096]`, so the
1,089-byte worst case loads.

**What this replaces.** Spec §4 measured 241 bytes and 969/1,031, assuming a
15-character area key and a 23-character insertion key. The plan's chosen names
are 19 and 24 characters, costing 7 more bytes. D-I's conclusion is unaffected:
the block overflows `char buf[1024]` under either estimate once 011 and 027 land.

Feature 011's and 027's own figures were read from their plans rather than assumed
— `011/plan.md:230` (+47, `randomize_shop_armour` and `randomize_shop_items`) and
`027/plan.md:206` (+15, `no_team_type`). 027's plan already carries the
rebase-on-011 instruction, which is the pattern this plan follows.

### M9 — Stat scaling of inserted bosses

| Quantity | Value | How |
| -------- | ----: | --- |
| Insertion identities the scaling table rewrites in at least one live area | **25 of 32** | `EL.zone_scaled_npc(mapfile, npc) != npc` over the fourteen live files |
| Identities rewritten in **all fourteen** live areas | 25 | same |
| Identities never rewritten | **7** | same |

The seven: both `m22` Witch of Hemwick identities (`210020*210020`,
`210020*210021`), the `m34` Witch (`210030*210030`), the Small Celestial Emissary
(`250080*250060`), and three of the four Living Failure variants (`403010`,
`403020`, `403030`).

Spec §4 says 24 of 29 tracked and five untracked, naming "the Witch of Hemwick,
the small Celestial Emissary, and three of the four Living Failure variants". The
**model** list is right; the identity counts follow M2's correction — the Witch
contributes three untracked identities, not one. `m28`'s Witch pair
(`npc 210005`) **is** tracked.

`zone_scaled_npc` returns its input unchanged for an untracked value, which is
easy to misread as "everything is scaled" — the spec's appendix warns of this and
the measurement above tests the inequality rather than the return value alone.

---

## E5. Risk analysis

### E5.1 A stray draw on the off path

`std::mt19937` is a single shared stream for the whole run. Every existing gate in
`StepWriteMap` is tested before `RandInt` for exactly this reason, and the
`016-unchanged-bell-maidens` plan's A3 records a case where moving 12 draws
reshuffled every map after them on every seed.

The insertion pass adds up to `|E|` draws per map in `UP TO` mode — 178 in the
Forbidden Woods — so a mistake here is not subtle in consequence, but it is
completely silent in appearance: a player would see a different world for a seed
they had already used, with no error.

Bounded by: the master-toggle test as the first statement of `ApplyBossInsertion`,
the harvest also being behind it, and M3's verification step being a tree diff of
a feature-off run against a run from the same seed on the pre-M3 build. That diff
is the check; the code discipline is what makes it pass.

### E5.2 Probability taken from pool composition

The failure mode D-M forecloses is not a crash and not visibly wrong — it is a
world where ticking one boss gives almost no bosses. M6 quantifies it: 3.3
expected in the smallest map at 22 ticked against 0.16 at one.

It is easy to write by accident in one specific way: building a combined
"enemies + ticked bosses" candidate list and drawing from it, which is what row
13's own backlog description and two project documents describe. The plan's
structure makes that shape unavailable — `k` is computed from `N` and `|E|` before
the pool is consulted at all, and the pool is only ever asked *which* boss.

The check is §8 criterion 4's third clause and the corresponding `selftest`
assertion: the single-identity mean must equal the all-22 mean. A pool-composition
implementation fails it by a factor of about 22.

### E5.3 `REPLACED ENEMY'S AI` may simply not work

The largest unknown in the feature, and it is an inference from what a
`ThinkParamID` is for rather than a measurement. Plausible outcomes: it works; the
boss stands inert because every animation the script names is missing from its
skeleton; it flails or T-poses; or the game crashes on the first behaviour lookup.
Nothing in the map data distinguishes them.

Circumstantial evidence, and it is weak: the reference's only accidental instance
of this (E1.1, `:1538-1545`) sits inside the one feature whose checkbox carries the
author's own instability warning.

The plan does not reduce this risk and cannot. It contains it: OWN AI is the
default, the row carries D-O's warning, and §6's hardware plan tests it third, in
isolation, so a failure is attributable to this setting and not to insertion in
general. A crash here retires nothing and invalidates nothing else.

### E5.4 1,381 resident boss models

**REPLACE ALL ENEMIES** is the most extreme form of a question backlog row 36
(boss rush) is also blocked on. The developer's own expectation is that it
crashes. Model residency, health-bar registration and arena scripting fired
outside an arena are all involved and none is observable from map data.

Contained by: off by default; unreachable from any other setting; a help string
that says it is a test control expected to break the game; the `RuneProbe.h`
comment shape; and M5, which deletes it. The one thing that must not happen is
that it quietly becomes a feature, which is why its removal is a milestone rather
than a note.

### E5.5 Shortfall becoming a failure

Feature 032's D4 exists because an empty-pool `Fail` fired *after* the mirror
phase had copied all six folders, leaving the player a half-built tree and an
error they could not act on. The same shape is reachable here — `EXACTLY 15`
against a map the player has skipped down to three eligible targets — and the
starvation case is real: five ticked skip rows empty Old Yharnam entirely (M1).

The plan's rule is that a shortfall is counted and reported and never fatal, which
is why `insertShortfallMaps` is a result field and not a `Fail` path.

### E5.6 Risks judged low enough not to change the implementation

* **Performance.** Greedy farthest-point is `O(k · |E|)` with `k ≤ 15` and
  `|E| ≤ 178`: at most ~2,700 squared-distance computations per map, inside a step
  that already decompresses, mutates and recompresses a map file. Not measured
  because it cannot matter at this scale.
* **Float precision.** Positions are `float32` and compared only against each
  other, accumulated in `double`. No threshold and no equality test, so precision
  affects only which of two near-equidistant placements is chosen.
* **The unused map variants getting their own quota.** Harmless — the retail game
  loads one file per area — and it is what the reference does. It only means the
  verifier must expect 2,269 rather than 1,381 under the override.
* **The `m28` forced-maiden override not being reproduced.** Those six placements
  are on the fixed exclusion list, so they are not eligible targets and the
  override has nothing to override.
* **Picker confusion between INSERTED BOSSES and BOSSES INCLUDED.**
  `docs/plans/pickers.md` §7.4's Father Gascoigne case is the precedent; both
  rows appear in the same category, four rows apart, with distinct help text, and
  the ids are shown. Spec §2 already explains the distinction to the player.

---

## E6. What the spec's appendix claimed

| Appendix claim | Trace found |
| -------------- | ----------- |
| `RandomizeFunctions.cs:1460-1552` is the whole pass, including the Moon Presence strip and two defects | **Confirmed**, E1.1. The strip is also buggy in a second way the appendix does not mention — it removes only the first match — which is harmless after `Distinct()` |
| `StartFunctions.cs:1013-1133` is the driver; the order of the fourteen flags is a candidate for the table's frozen order | **Confirmed**, E1.2. The order was *not* used: the plan sorts by display name, like every other generated table, because a reader can check that rule and cannot check the reference's arbitrary order |
| `RandomizeFunctions.cs:1650-1665` harvests, and the harvest sits inside the `addEnemy = true` branch, which is why the list is a superset of the boss pool | **Confirmed and extended.** The harvest is at `:1657`. The appendix stops one step short of the consequence: three de-eligibility tests run *after* it, which is the three-identity correction of M2 |
| `:1838-1846` serializes the list | **Confirmed**, and the `Distinct()` at `StartFunctions.cs:1015` — which the appendix does not mention — is what makes the list a set |
| `BossRandomizer.cpp`'s `CollectBossCandidates` "is where the insertion list would be harvested" | **Contradicted as a plan.** Same loop shape, three different rules; E2.3 and E3 record why a second function beats a flag. The harvest went into `StepReadMap` instead, which costs no extra pass |
| `DrainPool` / `BossPool::refill` are the shipped precedent for the pool | **Half confirmed.** The *shape* is the precedent; the *body* is not, because `DrainPool` erases same-model entries and refills from a model-deduped list, either of which breaks criterion 10 |
| `BossParamScaling.h` names the fourteen live area files plus the two Hunter's Dream files | **Confirmed**, M1 — and the consequence the appendix does not draw is that only the live file of a doubled area is scaled |
| `Msbb.h`'s header is what the position offset was cross-checked against; `part_fields` is where an accessor belongs | **Confirmed**, M5, and the bracketing arithmetic closes exactly |
| `SettingsModel.cpp:213-252` — three lines are the whole `SaveChoice` reuse question | **Confirmed**, E2.4; both blockers re-verified against the source |
| `settings_ui_verify.py:893` pins exactly one `SaveChoice` | **Confirmed**, E2.4 |
| `enemy_lookup.py`'s `overwritable_placements`, `exclusion_reason`, `zone_scaled_npc` are three of four oracles and already exist | **Confirmed**, and the fourth is there too: `enemies()["pos"]` already reads the three floats at `0x28` |
| `boss_verify.py`'s `build_pool` needs its own function for the insertion list rather than a parameter | **Confirmed**, E2.3 |
| Whether `ModelPoolEntry::model` should carry a map name for the area table "is a judgement the plan should make explicitly" | **Made:** the map-name **prefix**, following `CagedDogList.h`. M7 checks the fourteen prefixes partition the 22 files. A live file name would have dropped 8 of the 22 |
| `pool_verify.py:688-734` is where the arithmetic lands and must be rebased on 011 | **Confirmed**, M8, with figures for all four landing orders |
| `docs/plans/pickers.md` §7.4's Gascoigne trap is set again | **Confirmed** as a documentation risk; no code consequence, E5.6 |
| A few placements share exact coordinates, so spacing must tolerate a zero distance | **Confirmed**, M5: 1,367 distinct coordinates for 1,381 targets |
| Dead end — "the enemy pool"; `EnemyPoolTable.h` needs no change | **Confirmed** |
| Dead end — the model-size gate | **Confirmed**; out of scope per spec §6 |
| Dead end — a per-map pool | **Confirmed**: 32 identities cannot be exhausted by a count of 15, so the refill would be unreachable |
| Dead end — reusing `SaveChoice` | **Confirmed** |
| Dead end — the boss-pool collection order is irrelevant here, "bit-identical either way, measured" | **Confirmed independently**, M2: set-identical over all three orders |
| Dead end — per-area control via `WorldStore` | **Confirmed**: "world" is a playthrough, not an area |

---

## E7. Anything that could not be established

* **Whether a boss with no arena behaves at all.** Fog gate, health-bar
  registration, music, multi-phase transitions, arena-scripted adds, boss-defeated
  events an area depends on. None is in map data. Every field the pass writes is a
  field two shipped, hardware-tested passes already write to the same blobs, so
  nothing about the *file* is new; the game's reaction is entirely unproven.
* **Whether a boss can run an ordinary enemy's AI script.** E5.3. Inference only.
* **Whether the stat-scaling variants are *easier* or merely different.** Nobody
  has read the resulting `NpcParam` rows. `BossParamScaling.h` calls them
  "pre-tuned … appropriate for the zone" and the reference's opt-out is named "No
  Scaling", which is suggestive and not evidence. Verifiable offline by dumping
  the scaled rows' HP and damage from `gameparam.parambnd.dcx`; not done, because
  nothing in the plan depends on it. `docs/known-traps.md` is explicit that byte
  decoding does not prove game behaviour.
* **Whether spacing helps in play rather than in space.** The metric is 3D
  Euclidean distance over placement coordinates with no knowledge of walls,
  floors, doors or traversal. Two placements 30 paces apart may be on different
  storeys of the Research Hall, whose 84 targets fit a 184-pace diagonal. The
  guarantee is spatial only, and §8's hardware step 2 is the only thing that can
  say whether a player notices.
* **Whether 15 is the right maximum in play.** It is the right maximum *for the
  spacing promise*, which M5 measures. Whether 15 loose bosses is enjoyable, or
  survivable, is a hardware question.
* **Whether features 011 and 027 will land before this one.** The plan carries
  arithmetic for all four orders rather than assuming one. The residual risk is
  that a fourth shared-buffer feature is specced between approval and
  implementation, which the exact-equality assertion in `pool_verify.py` would
  catch as a failing case rather than as silent truncation.
* **The unused map variants' behaviour if a player's install does load one.**
  Assumed never loaded, on the strength of `BossParamScaling.h` naming one file
  per area and of spec §4's reasoning. Not independently verified, and the cost of
  being wrong is small: that file would have received its own quota of inserted
  bosses and no stat scaling.
