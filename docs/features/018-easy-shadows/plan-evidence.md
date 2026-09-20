# Plan Evidence 018 — Easy Shadows, Easy Rom, Easy Failures, Easy Emissary

**Plan:** `docs/features/018-easy-shadows/plan.md`

**Spec:** `docs/features/018-easy-shadows/spec.md`

---

> **This document is the investigation behind the plan, not instructions.**
>
> The implementer does not read it to start work. They consult it when an
> implementation question needs context the contract deliberately left out.
> It is revisable: when a measurement is corrected, it is corrected here.

---

## E1. Reference trace

### E1.1 The function

`reference/Randomizer/MainWindowComponents/MainWindow.xaml.cs:934-1055` —
`EasyModes(string currentMap)`. It reads one MSB, walks `Parts.Enemies`, and for
a name match overwrites three properties:

```csharp
tempGUY.Parts.Enemies[i].NPCParamID  = stoneGuyParam;
tempGUY.Parts.Enemies[i].ThinkParamID = stoneGuyThink;
tempGUY.Parts.Enemies[i].ModelName    = stoneGuyModelName;
```

Nothing else is written — not `EntityID`, not position, not the part name, not
any index-based cross-reference. The map is then written back.

The branch is on the **map**, not on the flag: `currentMap.Contains("m27")` →
the two Shadow names; `else if ... Contains("m32_00")` → `c1400`; `else if ...
Contains("m24_02")` → the ten `c2500_000x` names; the Living Failures branch
matches `m35`. Which branch can fire is decided entirely by which file the
caller passes.

### E1.2 The call sites

`reference/Randomizer/MainWindowComponents/StartFunctions.cs:1239-1256` — four
independent top-level `if` blocks, one per flag, each passing full file paths:

| Flag | Maps passed |
| ---- | ----------- |
| `easyMultiBossesBool` | `m27_00_00_01` |
| `easyRomBool` | `m32_00_00_00`, `m32_00_00_01` |
| `easyFailuresBool` | `m35_00_00_00` |
| `easyWitchesBool` | `m24_02_00_00`, `m24_02_00_01` |

Six map files, not seven. The asymmetry — row 18 patching only the live
Forbidden Woods while rows 19 and 21 patch both variants of their area — is in
the reference source as written, and spec §10 D3 fixes it as agreed behaviour.

### E1.3 Where the replacement identity comes from

`MainWindow.xaml.cs:744-762`, inside `GenerateEnemyList`: while building the
enemy pool, a placement whose `ModelName` contains `c2521` has its
`NPCParamID`, `ThinkParamID` and (a five-character substring of) its model name
copied into `stoneGuyParam` / `stoneGuyThink` / `stoneGuyModelName`.

The guard is `if (!addedStoneGuyBool)`. `addedStoneGuyBool` is declared at
`FieldContainer.cs:60`, read at `MainWindow.xaml.cs:750`, and **assigned
nowhere**, so the capture re-runs on every match and the last one wins. That is
harmless: all three `c2521` placements in the tree carry identical values
(E4 M7).

`stoneGuyModelName` is `Substring(LastIndexOf("*") + 1, 5)`, so the string
written is exactly `"c2521"`.

On the enemy-picker ("oops all") path `GenerateEnemyList` is skipped, so the
three values stay `0` / `0` / `null` and `EasyModes` writes zeroed params and a
null model name. That is a reference defect, and spec §6 rules reproducing it
out of scope.

### E1.4 Ordering against the other passes

In `StartFunctions.cs` the order is enemies → bosses → `AddTheRest` → NPCs →
**`EasyModes` (`:1239`)** → … → `ParamScalingForBosses` (`:2457`).

Two consequences, both behaviour rather than preference:

* The easy pass runs **after** every randomization pass, so an easy setting wins
  over a randomized duplicate.
* It runs **before** zone scaling, so a larva placed in a scaled zone is itself
  scaled. E4 M8/M9 measure that this cannot change what the player fights: all
  31 scaled variants are identical to the base row in HP, echoes, team type,
  behaviour id and item lot.

### E1.5 Where the trace confirmed the spec, and where it did not

Confirmed: spec §3's table of flags, maps and names; the three written fields;
the `addedStoneGuyBool` defect; the late position of the pass; the asymmetric
map lists.

Not confirmed as written: spec §4's EMEVD reference counts (37-39 Shadow
references, 34-37 Living Failure). Re-measured by a raw four-byte little-endian
scan of the inflated EMEVD, the figures are 33/33/32 and 26/28/27/27/26
(E4 M10). The two methods differ in what they count; the conclusion spec §4
draws from the number — the fight logic tracks each body individually — is the
same either way, and nothing in spec §2 depends on the figure.

---

## E2. What exists in the port

### E2.1 The settings chain

Three shipped settings are the template, in increasing closeness:

* `enableMergoDarkness` — the first non-randomizing toggle carried through
  `EnemyRandomizerJob`; the origin of the "report only when it did something, no
  `SKIPPING` counterpart" rule (`docs/plans/mergo-darkness.md` D5) and of the
  "pin the output to a known value rather than to whatever the user's dump
  holds" argument (`EnemyRandomizer.cpp` `StepEmevd`'s header comment).
* `doNotRandomizeCagedDogs` (feature 033) — a baked placement table in a header
  (`CagedDogList.h`) with a Python mirror (`caged_dogs_verify.py`) that **parses
  the header** rather than restating it. This is the closest template for this
  feature's shape and is what §4 and §6 of the contract follow.
* `startWithHunterTools` (feature 034) — the most recent settings-chain
  addition, and the current example of the append-last row rule.

The chain's files, per setting: `RandomizerDefaults.h`,
`RandomizerDefaultsStore.cpp` (one load branch, one `%d` + argument in the save
`snprintf`), `EnemyRandomizer.h` (`EnemyRandomizerOptions`, and
`EnemyRandomizerResult` when there is something to report),
`SetupDefaultsScreen.h/.cpp`, `EnableWizardScreen.h/.cpp`.

### E2.2 The UI row rule, and its history

The row constants in `SetupDefaultsScreen.h:36-80` and
`EnableWizardScreen.cpp:22-59` double as **positions** in the `items` vectors
that `DrawList` / `DrawSaveData` / `DrawConfirm` build. Both files say, in
comments, that an entry added out of order "compiles, passes
`ui_scroll_verify.py` and mislabels every row below it".

The rule the code now follows is stated three times in `SetupDefaultsScreen.h`
(at `kDisableMergoDarknessRow`, `kDoNotRandomizeCagedDogsRow` and
`kStartWithHunterToolsRow`): a new row is appended **last**, because going last
is what guarantees no existing index moves.

Current values, read from the working tree:

| Constant | Value | Where |
| -------- | ----: | ----- |
| `kItemCount` | 17 | `SetupDefaultsScreen.h:36` |
| `kDisableMergoDarknessRow` | 11 | `SetupDefaultsScreen.h:59`, `EnableWizardScreen.cpp:39` |
| `kEnemiesIncludedRow` | 12 | both screens |
| `kEnemiesSkippedRow` | 13 | both screens (feature 032) |
| `kBossesIncludedRow` | 14 | both screens |
| `kDoNotRandomizeCagedDogsRow` | 15 | both screens (feature 033) |
| `kStartWithHunterToolsRow` | 16 | both screens (feature 034) |
| `kSaveDataRowCount` | 17 | `EnableWizardScreen.cpp:59` |

`UNCHANGED BELL MAIDENS` (feature 016) is **not** in the tree and never was:
feature 032 D1 retired it, and `SetupDefaultsScreen.h:45-49` records that
everything below its old row 4 moved up one. The values above therefore replace
a post-016 projection of `kItemCount` 15 / `kDisableMergoDarknessRow` 12, and
"insert before the drill-in rows" replaces nothing that the code ever did: the
drill-ins are no longer last, and inserting there would renumber five live rows
against the append-last invariant. The contract's §4.5 is derived from the
table above.

`ui_scroll_verify.py:48-79` carries seven screen entries; the three
settings-screen counts (`Setup Defaults`, `Wizard SaveData`, `Wizard Confirm`)
are all 17, and `Progress log`'s 16 is a line budget rather than a row count.

### E2.3 The engine

`EnemyRandomizer.cpp:241` — the phase machine:
`Mirror → ReadMaps → BuildPool → MergeModels → BossCollect → BossAssign →
TreasureCollect → TreasureAssign → WriteMaps → Emevd → ItemData → Finished`.

* Boss assignment, including `AddTheRestInMap` (`:716`), finishes before the
  first map is written, so a pass inside `StepWriteMap` automatically wins over
  the boss passes. No new phase is needed to reproduce E1.4.
* `StepWriteMap` (`:724`) opens by building `enemyModelIndex`, a name → index
  map over the map's enemy models (`:729-736`). Then the enemy loop, gated on
  `if (options.randomizeEnemies)` (`:745`) and closed by
  `} // if (options.randomizeEnemies)` (`:853`). Then the `BossScalingMaps()`
  loop (`:855-866`), then `Serialize()` + `DcxCompress` + write.
* The scaling loop is **outside** the `randomizeEnemies` gate, so
  `ApplyBossParamScaling` runs on every map of its list on every run, whatever
  the settings are. This is the fact behind E5.3.
* `StepMergeModels` (`:619-635`) appends every enemy model seen in any of the 24
  base maps into every map's `Models` section, unconditionally. `c2521` is
  declared as an enemy model in five of those maps (E4 M7), so the model this
  feature names is always resolvable — the port's equivalent of the reference's
  model-merge loop, without the picker-path hole.
* `RandInt` is called only from the enemy loop, the pool shuffle and the boss
  passes, so a pass that draws nothing cannot shift the stream.

### E2.4 The boss randomizer targets the same placements

`BossRandomizer.cpp`: `AssignEligible` (`:65`) excludes `c2120_0001/0002` and
`c4030_0001/_0002/_0003` from the main boss pass; `kFixups` (`:141`) syncs
`c2500_0000` → `c2570_0001`; `AddTheRestInMap` (`:382`) is what fills those five
excluded slots with random bosses. They are exactly the placements Easy Shadows
and Easy Failures target, which is why ordering decides the outcome. Nothing in
`BossRandomizer.cpp` changes.

### E2.5 Verification tools

* `boss_verify.py` — `Msbb` reads DCX + MSBB and yields
  `(index, name, npc, think, entityID, modelIndex)`; `compare_trees` (`:453`)
  diffs two trees over `BOSS_MAP_ORDER` only (17 maps, `:162-168`). See E5.4.
* `enemy_lookup.py` — `load_map`, `enemies`, `exclusion_reason`, and
  `zone_scaled_npc(mapname, npc)` (`:210`), the existing helper for "what the
  scaling pass turns this value into in this map".
* `pool_verify.py` — `_frozen(mapname, a, b)` (`:186-203`), the repository's
  definition of "untouched by the enemy pass": same model, same think, and an
  npc that is either unchanged or the zone-tuned variant. Its docstring records
  that plain equality with vanilla "is exactly the bug the retired `maidens`
  command shipped with". Also carries the worst-case `defaults.cfg` arithmetic
  as an exact equality (`:686-703`).
* `caged_dogs_verify.py` — `list` / `protected` / `selftest`, header parsed
  rather than restated, three independent derivations of the protected set, and
  a docstring that says plainly what the tool cannot prove. The shape for
  `easy_modes_verify.py`.
* `param_offsets.py` — `load_defs` / `load_param` / `param_rows` /
  `field_offsets`; every param number in E4 came from these.
* `starting_weapons_verify.py` — the precedent for parsing reference C#
  directly, which selftest case 11 follows.

### E2.6 Layering

The pass lives in `Randomizer` and depends on `Msb` only; the four rows live in
`UI` and know nothing but a label and a bool. No SDL2 reaches `Randomizer`, no
AFR path reaches `UI`.

---

## E3. Alternatives considered

| Approach | Why rejected |
| -------- | ------------ |
| A new job phase `EasyModes` between `TreasureAssign` and `WriteMaps` | Needs its own map loop, progress denominator and step accounting to achieve what one call inside `StepWriteMap` achieves. Phases exist so a step fits in one frame; this pass touches at most 30 placements in one map. |
| Run the pass **after** `ApplyBossParamScaling`, so the written value is a clean 252100 everywhere | Not what the reference produces (E1.4), and `CLAUDE.md` §7 says match before improving. E4 M9 shows the scaled variants are indistinguishable in play, so the only gain would be a tidier verifier. |
| Normalise the map lists by adding `m27_00_00_00` | Spec §10 D3 forbids it. The map holds the same three placements with the same values (E4 M3) and the reference still does not patch it. |
| Reproduce the reference's runtime capture, `addedStoneGuyBool` guard included | The guard is a never-assigned field, i.e. a defect; the port has no `GenerateEnemyList` equivalent for a picker run to skip, so there would be no hole to reproduce and nothing to gain. |
| Blank the replaced placements' item lots, or strip the larva's drop, to avoid relocating One Third of Umbilical Cord | Out of scope unconditionally by spec §10 D2 and spec §6. |
| Extend `IsExcludedNpcRow` to the 31 scaled variants so the larva's drop survives drop randomization | A deliberate deviation from the reference, which excludes the same two rows (`RandomizeFunctions.cs:3125`). It would also change existing shipped behaviour for the clinic larva, which already scales to 900014601 today. Spec §10 D2 chose reference fidelity; this stays out and the consequence is recorded in E5.5. |
| Put the pass in `BossRandomizer.cpp` beside `AddTheRestInMap` | Two of the four settings target ordinary randomizable enemies, the pass runs whether or not bosses are on, and the boss phases run before the write loop — the wrong side of the ordering guarantee. |
| Leave the four flags out of `StartCommit`'s big `||`, like `randomizeWorkshopTools_` | Workshop tools is a modifier that does nothing alone; each easy setting alone produces a real, different map tree. `mergo-darkness.md` D4 is the matching precedent, and `startWithHunterTools_` is the most recent one. |
| Pass the caller's `enemyModelIndex` into the pass instead of scanning for `c2521` locally | The lookup **is** in scope at the call site (`EnemyRandomizer.cpp:729-736`), so this would work and save a scan of a few hundred entries per map. Rejected on the smaller of two costs: a self-contained `ApplyEasyModes(mapName, msbb, options, counts)` matches `ApplyBossParamScaling(msbb, zoneScale)`, keeps the pass's failure handling (log and skip) inside the pass, and gives the Python mirror one thing to reason about. Decision P4. |
| Assert that survivors and settings-off maps are **byte-identical to vanilla** | Fails on a correct implementation: the scaling pass is unconditional (E2.3) and rewrites the stat rows of most of the survivors (E4 M11), and every map is re-serialised and re-compressed on every run. `pool_verify._frozen` exists because this mistake shipped once already. The contract's §6 uses the frozen form. |
| Split the milestone: the pass first, the UI second | A milestone must end in something the developer can hardware-test; the pass with no UI cannot be switched on. |
| Split the milestone: Shadows first, the other three after | The mechanism is identical across the four and the marginal cost of the other three rows is small, while splitting renumbers the same UI rows twice for no extra hardware answer. Recorded because it is the honest counter-argument: the biggest unknown (E5.1) is per-fight, and Shadows passing does not prove Rom passes. |

---

## E4. Measurements

All against `data/vanilla/dvdroot_ps4`. Commands are written to be run from
`app/tools/` with `R` standing for `../../data/vanilla/dvdroot_ps4`.

| # | Quantity | Value | How measured |
| - | -------- | ----- | ------------ |
| M1 | Placements each table row selects | m27_00_00_01 → 2; m32_00_00_00 → 30; m32_00_00_01 → 30; m35_00_00_00 → 3; m24_02_00_00 → 7; m24_02_00_01 → 7 | `python -c "import sys;sys.path.insert(0,'.');from enemy_lookup import load_map,enemies;m=load_map(R,'m24_02_00_01');print(sorted({e['name'] for e in enemies(m) if any(p in e['name'] for p in ['c2500_%04d'%i for i in range(1,11)])}))"` |
| M2 | Totals | **79** tree-wide across the 24 loaded maps; **42** in the retail-loaded variants (M1 minus `m32_00_00_00` and `m24_02_00_00`) | sum of M1 |
| M3 | `m27_00_00_00` holds the same three Shadows | `c2120_0000/0001/0002`, NpcParam 212700 / 212710 / 212720, same as `m27_00_00_01` | same command with `'m27_00_00_00'` and pattern `c2120` |
| M4 | The emissary set is seven of the ten names | `_0001,_0002,_0003,_0006,_0007,_0009,_0010`; `_0004`, `_0005`, `_0008` do not exist in `m24_02` | M1's command |
| M5 | Pattern reach outside its own map (all 43 `.msb.dcx`, chalice included) | `c1400` → 138 placements in m32_00_00_00/01 **and m29_30_90_00/01**; `c2500_0001` → 9 maps including m24_01_*, m27_*, m29_53_90_*; `c2120_0002` → also m26_00_00_00. Within the 24 loaded maps: `c1400` → 60, in the two m32 maps only | glob over `map/mapstudio/**/*.msb.dcx` with `boss_verify.Msbb`; this is why §4.1's table is keyed by exact map name and why selftest case 4 is scoped per map |
| M6 | Survivors and their stats | `c2120_0000` 1425 HP (against 900 and 800); `c4030_0000` 320 HP; `c4030_0004` NpcParam 403050, ThinkParamID **1**; `c2500_0000` 250080; `c2500_0011/_0012` 250082, `c2500_0013` 250082/think 250070; `c2570_0001` 257010; Rom is a different model | `enemies()` per map + `param_offsets` `hp` field at NpcParam offset 32 |
| M7 | The replacement creature | model `c2521`, three placements tree-wide, all named `c2521_0000`, all NpcParam 252100 / Think 252100, all entity ID 2410771, in `m24_01_00_00`, `m24_01_00_01`, `m24_01_00_11`; declared as an enemy model in five maps (those three plus `m24_00_00_00`, `m24_00_00_01`) | `enemy_lookup` scan for `c2521`; models section scan for the declaration |
| M8 | NpcParam 252100 | hp 2, getSoul 18, teamType 26, hitHeight 1.0, hitRadius 0.2, `behaviorVariationId` 25210, `itemLotId_1` 28040, `itemLotId_2` −1 | `param_offsets.load_defs/load_param/param_rows/field_offsets` over `NpcParam.param` |
| M9 | The 31 scaled variants 900014601–900014631 | **all 31 identical to 252100** in every field of M8 | same command over the 32 row ids; they form one signature group |
| M10 | EMEVD references | Shadows `2700800/0801/0802` → 33 / 33 / 32 in `m27_00_00_00.emevd.dcx`; Living Failures `3500850`-`3500854` → 26 / 28 / 27 / 27 / 26 in `m35_00_00_00.emevd.dcx`; Rom's children `3200200`-`3200229` → exactly **3 each** | four-byte little-endian scan of the inflated EMEVD via `boss_verify.read_dcx`. Replaces spec §4's 37-39 / 34-37, which came from a different counting method (E1.5). The Rom figure is the useful one: 3 against 30+ is consistent with a simple enable/disable per wave |
| M11 | What the scaling pass writes in the six maps | larva 252100 → **900014609** (m27_00_00_01, +9), **900014611** (m32_00_00_01, +11), **900014614** (m24_02_00_01, +14), **900014627** (m35_00_00_00, +27); unchanged in m32_00_00_00 and m24_02_00_00, which are absent from `BossScalingMaps()`. Survivors: Rom `c5100_0000` 510000 → 995107013; `c2120_0000` 212700 → 992117010; `c4030_0000` 403000 → 994037483; `c2500_0011/_0012/_0013` 250082 → 900014521; `c2570_0001` 257010 → 900015234; `c4030_0004` 403050 and `c2500_0000` 250080 are not tracked and do not move | `python -c "import sys;sys.path.insert(0,'.');from enemy_lookup import zone_scaled_npc;print(zone_scaled_npc('m27_00_00_01',252100))"` |
| M12 | The umbilical cord | ItemLot **28040** is the only row in `ItemLotParam` granting item 4321, `lotItemBasePoint01` 100, single `getItemFlagId` 50001205; exactly **32** NpcParam rows reference it — 252100 plus 900014601–900014631 | `param_offsets` scan of `ItemLotParam.param` for `lotItemId0x == 4321`, then of `NpcParam.param` for `itemLotId_1/2 == 28040` |
| M13 | Drop-randomizer exclusions | `DropRandomizer.cpp:22-24` excludes exactly `252100` and `6071`. The 31 scaled variants are **not** excluded | read from the source; `drops_verify.py` `EXCLUDED_ROWS` carries the same pair |
| M14 | `boss_verify verify` false positives on an all-four-on tree | **37** — 30 `c1400_*` in `m32_00_00_01` plus 7 `c2500_000x` in `m24_02_00_01`. `m32_00_00_00` and `m24_02_00_00` are not in `BOSS_MAP_ORDER`, and the five `c2120`/`c4030` names are boss names already covered by the `addtherest` exemption at `boss_verify.py:512-518`. Replaces an earlier figure of 67, which counted both Rom variants and one emissary variant — i.e. maps `compare_trees` never walks | `BOSS_MAP_ORDER` (`boss_verify.py:162-168`) ∩ M1 |
| M15 | `defaults.cfg` worst case | 611 bytes today, pinned as an exact equality at `pool_verify.py:702-703`. Four new keys add 58 bytes (`easy_shadows=1\n` 15, `easy_rom=1\n` 11, `easy_failures=1\n` 16, `easy_emissary=1\n` 16) → **669**, still inside `char buf[1024]` | arithmetic over the `snprintf` format in `RandomizerDefaultsStore.cpp:112-140` |
| M16 | Font | `EASY SHADOWS`, `EASY ROM`, `EASY FAILURES`, `EASY EMISSARY`, their `   YES/NO` rows and the four `… REPLACED n PLACEMENTS` lines are all renderable and inside the 71-character line | `python -c "import sys;sys.path.insert(0,'.');from pool_verify import renderable,LINE_CHARS;print(renderable('EASY EMISSARY REPLACED 14 PLACEMENTS'))"` against `Font8x8.cpp`'s 42-glyph table |
| M17 | Map variants the retail game loads | `names.py` tags `m27_00_00_00` and `m24_02_00_00` `unused`; `m32_00_00_00` is named "Byrgenwerth (pre-DLC)" and is not tagged | `python -c "import sys;sys.path.insert(0,'.');from names import map_is_unused,map_name;print(map_is_unused('m24_02_00_00'),map_name('m32_00_00_00'))"`. Community data, per `app/tools/data/README.md`'s own accuracy caveat |

---

## E5. Risk analysis

### E5.1 The fights may not end

The one risk no verifier can reach, and the only one that could make the feature
worthless.

Established: the pass writes three fields and nothing else, so EntityID,
position, part name and every index-based cross-reference survive. M10 shows the
fight scripts reference each Shadow and each Living Failure ~30 times, i.e. the
logic tracks individual bodies, and leaving the IDs alone leaves that logic
intact.

Not established: whether the completion condition is "these entities are dead"
(fine) or something tied to the creature's own death behaviour (not fine). Only
hardware answers it.

Rom-specific: her children are referenced only 3 times each, consistent with a
simple enable/disable per wave. Whether her fight *also* spawns children
dynamically is not knowable from map data. If it does, Easy Rom thins the fight
rather than emptying it — the same as in the reference. Recorded so that a
hardware observation of "some real spiders appeared" is read as this rather than
as a broken rule.

Nothing is deleted: all 30 bodies remain in the map, still have to be killed,
and still carry their entity IDs. Worth saying because spec §2's "None of her
thirty children remain" reads at a glance like removal.

### E5.2 A mislabelled UI row

The row constants are positions in three parallel `items` vectors (E2.2). An
entry added out of order compiles, passes `ui_scroll_verify.py` and mislabels
every row below it — including two rows that open pickers — and a `defaults.cfg`
then gets written from the wrong rows. The whole mitigation is the append-last
rule plus changing no existing index, which is why the contract states the rule
rather than a set of absolute indices.

### E5.3 "Unchanged" is not "byte-identical to vanilla"

`BossScalingMaps()` runs outside the `randomizeEnemies` gate (E2.3), so four of
the six maps this feature touches have their tracked stat rows rewritten on
**every** run — including a run with every setting off. M11 lists what moves.
On top of that, every map is re-serialised and re-compressed, so even an
untouched map is not byte-identical at file level (`itemdata_verify.py:164`
records the same distinction for the item archive).

Any assertion of the form "this placement did not change" therefore has to be
`pool_verify._frozen`'s: same model, same think, npc either vanilla or that
map's zone-scaled variant. Demanding equality is the bug the retired `maidens`
command shipped with.

### E5.4 `boss_verify verify` on an easy tree

`compare_trees` classifies every changed placement. `c1400_*` and
`c2500_000x` are not boss names, so without `--enemies-also` each raises
`V2 non-boss placement changed` — 37 of them (M14). The five `c2120`/`c4030`
names are boss names and the existing `addtherest` exemption already covers
them. `identity_ok` accepts the larva both plain and scaled, because
`all_vanilla_triples` walks the whole tree so `(252100, 252100, "c2521")` is a
real vanilla triple and the scaled ids are in the scaling table. `FIXUP_GROUPS`
contains no name this feature touches, so the fixup-sync check cannot misfire.

So the only misfire is the "may this placement change at all" question, and the
minimal fix is a `--easy` flag that suppresses it for exactly the (map, name)
pairs in the table. Keeping positive assertions out of `boss_verify.py` is
deliberate: its V2 semantics are load-bearing for boss randomization and should
not grow a second meaning.

### E5.5 The umbilical cord, and the unprotected scaled rows

ItemLot 28040 is the game's only source of One Third of Umbilical Cord (M12) and
carries a single `getItemFlagId`, the field FromSoftware uses to mark a lot as
already taken. The expected outcome is therefore that the first easy-mode larva
killed yields the cord and every other larva — the clinic one included — yields
nothing. The alternative reading is a cord per larva, which would put up to 30
in the world from Easy Rom alone. Only hardware separates them; spec §10 D2
accepted the relocation either way.

Independently: with `RANDOMIZE ENEMY DROPS` on, the rows the easy-mode larvae
actually carry in the four scaled maps — 900014609, 900014611, 900014614,
900014627 — are outside `IsExcludedNpcRow` (M13), so their `itemLotId_1` is
rewritten and lot 28040 is fed into the shared pool 31 times. This is not new
and not a fidelity problem: the clinic larva already scales to 900014601 today,
and the reference excludes the same two rows. But it means a cord test run with
drops on can report "no cord from any easy-mode larva" for a reason that has
nothing to do with this feature — which is exactly what the test was written to
detect. Hence the contract pins that hardware step to drops **off**.

### E5.6 The replacement model must be declared in the map being written

A placement cannot reference a model name the map's `Models` section does not
contain; `MSBB.Write` throws `KeyNotFoundException` in the reference for
precisely this. `StepMergeModels` guarantees it for all 24 maps and runs
unconditionally, and `c2521` is declared in five of them (M7). The residual risk
is that a future change conditionalises the merge and nobody notices until a map
fails to load on hardware, so the pass logs and skips rather than writing a −1
index, and selftest case 6 asserts the declaration.

### E5.7 Team type 26 is a byte, not observed behaviour

Across the base maps the creatures sharing teamType 26 are the Doll, Gehrman,
Willem, the Messengers and the strapped-down patients — no ordinary hostile
enemy. That is strong, and it is still exactly the kind of inference
`CLAUDE.md` §5 and the standing note on setting polarity say not to trust
without a console. The hardware step asks whether the larvae attack.

### E5.8 Cosmetic counters

`result.npcParamsScaled` gains 42 on an all-four-on run, because the scaling
pass rewrites every larva the easy pass placed in a scaled zone.
`result.enemiesRandomized` may double-count a Rom child or small emissary that
the enemy loop randomized and the easy pass then overwrote. Both are what the
reference does, and the four new per-setting lines are the numbers to read
instead.

### E5.9 Nothing here can make a run unwinnable

The affected placements hold no items, gate no progression and are not key-item
sources. The worst case short of E5.1 is a fight that is easier than intended,
which is the point of the feature. The one item involved is optional and, per
E5.5, relocated rather than lost.

### E5.10 A Python mirror pins the rules, not the C++

There is no host C++ compiler, so `easy_modes_verify.py` can prove that the
table and the identity are right and that a generated tree has the right
properties. It cannot prove `ApplyEasyModes` matches it. The two can drift; this
is the standing, accepted weakness of this setup.

---

## E6. What the spec's appendix claimed

| Appendix claim | What the trace found |
| -------------- | -------------------- |
| `EasyModes` is the whole feature; four branches, one write per map | Confirmed (E1.1). Six map files, not seven — the count that matters for the table. |
| `StartFunctions.cs:1239-1256` holds the four call sites | Confirmed exactly (E1.2). |
| `GenerateEnemyList`'s `addedStoneGuyBool` block is the only place the three values are set | Confirmed; the field is declared at `FieldContainer.cs:60` and never assigned (E1.3). |
| The model-merge loop explains why the reference can name a model the target maps do not declare | Confirmed, and the port has the equivalent guarantee in `StepMergeModels` (E2.3). |
| `StepMergeModels` and `StepWriteMap` are the port-side anchors | Confirmed, with current line numbers in E2.3. `StepWriteMap` also already holds the `enemyModelIndex` lookup (E3). |
| `BossRandomizer.cpp`'s `AddTheRestInMap` / `kFixups` target the same placements | Confirmed (E2.4). |
| `PermaDarkness.h` + `EnemyRandomizerOptions` are the pattern for a non-randomizing toggle | Confirmed, and two closer precedents have shipped since: `CagedDogList.h` + `caged_dogs_verify.py`, and `startWithHunterTools` (E2.1). |
| `DropRandomizer.cpp`'s `IsExcludedNpcRow` protects the replacement's drop row | Confirmed for row 252100 only. The 31 scaled variants are outside it (M13, E5.5). |
| Whether the easy pass runs before or after `ApplyBossParamScaling` is a readability choice | Not accepted. It is a readability choice for *what the player fights* (M9) but not for the output bytes, and the reference's order is behaviour (E1.4). Before scaling, matching the reference. |
| `boss_verify.py`'s `Msbb` and `param_offsets.py` were enough for every §4 measurement | Confirmed; E4 used the same two plus `enemy_lookup.zone_scaled_npc`. |
| Item names resolve from `msg/engus/item.msgbnd.dcx`; no repository tool reads FMG | Confirmed, and not needed: the plan cites item 4321 by id, and no FMG reader is added. |

---

## E7. Anything that could not be established

* Whether the fights end, whether the larvae attack, whether they die in one
  hit, and whether Rom spawns children dynamically (E5.1). Nothing in this
  environment runs the game.
* Where the umbilical cord lands. `getItemFlagId` 50001205 is a byte; both
  readings are consistent with it (E5.5).
* That teamType 26 means non-hostile (E5.7).
* That the C++ pass matches the Python mirror (E5.10).
* Byte-level DCX output: no tree was generated for this plan, so E5.3's
  compression point rests on `itemdata_verify.py:164` and on the fact that the
  port re-serialises and re-compresses every map on every run, not on a measured
  diff.
* Which variant of Byrgenwerth the retail game loads. M17 shows `names.py` tags
  the pre-DLC Forbidden Woods and Upper Cathedral Ward `unused` but not
  `m32_00_00_00`; the 42-in-retail figure follows spec §4 in assuming one m32
  variant is loaded. Nothing in the plan depends on which: both are patched.
