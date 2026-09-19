# Plan 018 — Easy Shadows, Easy Rom, Easy Failures, Easy Emissary

**Status: QUESTIONS ANSWERED — awaiting developer approval.** *(all four
decisions recorded in §9, 2026-09-16.)* The developer answered from a summary of
the questions rather than from this document, so approval is still theirs to
give. Nothing here needed a behavioural decision the spec had not already
taken.

**Spec:** `docs/features/018-easy-shadows/spec.md` — **APPROVED** (developer, 2026-09-16)
**Backlog rows:** `docs/randomization-feature-spec.md` §6, rows **18, 19, 20, 21**
**Plan review:** `docs/features/018-easy-shadows/plan-review.md` *(once stage 2 has run)*

---

## 1. What this builds

Four new boolean settings — `EASY SHADOWS`, `EASY ROM`, `EASY FAILURES`,
`EASY EMISSARY`, all default off — carried through the existing settings chain
(defaults struct → `defaults.cfg` → Setup Defaults row → Enable wizard row →
`EnemyRandomizerOptions` → engine), plus **one new table-driven MSB pass** that
the port does not have in any form today.

The pass is small. For a named set of enemy placements in six named maps it
overwrites exactly three fields — `NPCParamID`, `ThinkParamID` and the model
index — with the identity of the Iosefka's Clinic larva (`c2521`, NpcParam
252100, ThinkParam 252100), and touches nothing else. It draws no randomness.

Everything about *what* the feature does is in the spec. This document is how.
The four decisions in spec §10 that shape the build:

* **D1** — all four rows are one feature over one shared pass. So: one table,
  one function, four flags, one milestone.
* **D2** — the replacement creature is planted as-is, drop intact. So: nothing
  here touches `NpcParam` or `ItemLotParam`, and §6.2 asserts that as a
  property rather than assuming it.
* **D3** — the reference's asymmetric map lists are matched exactly. So:
  `m27_00_00_00` is **deliberately absent** from the table below even though it
  holds the same three Shadow placements, and the table carries a comment
  saying so, so nobody "fixes" it later.
* **D4** — four independent toggles, labelled from the reference's own checkbox
  text uppercased.

This plan's own four decisions — build order against plan 016, where the rows
sit, what the console prints, and baking the replacement identity — are in §9.

---

## 2. What already exists

### 2.1 The settings chain to copy

`enableMergoDarkness` is the closest neighbour and the exact template: a
boolean, default off, that is **not a randomizer** — it draws no randomness,
the same seed gives the same world either way, and it is carried through
`EnemyRandomizerJob` because that job is what writes the output tree. Its full
chain, which these four mirror file for file:

| Layer | File | What it holds |
|---|---|---|
| Randomizer | `app/src/Randomizer/RandomizerDefaults.h` | the persisted default |
| Randomizer | `app/src/Randomizer/RandomizerDefaultsStore.cpp` | `enable_mergo_darkness` key, load branch and save `snprintf` |
| Randomizer | `app/src/Randomizer/EnemyRandomizer.h` | the per-run option in `EnemyRandomizerOptions` |
| Randomizer | `app/src/Randomizer/PermaDarkness.h/.cpp` | the pass itself, its own file |
| UI | `app/src/UI/SetupDefaultsScreen.h/.cpp` | row constant, `ToggleRow` branch, `DrawList` entry |
| UI | `app/src/UI/EnableWizardScreen.h/.cpp` | row constant, member, ctor init, two toggle branches, option assignment, `||` membership, progress line, two list entries |

Three precedents inside that chain matter here and are followed below:

* **It IS in `StartCommit`'s "does anything need doing" `||`**
  (`EnableWizardScreen.cpp:501-505`), unlike `randomizeWorkshopTools_`, because
  it is a complete change on its own rather than a modifier on another feature
  (`mergo-darkness.md` D4). The four easy settings are the same shape: each one
  alone writes real map files, so ticking one alone must start a run.
* **It is the last toggle row**, appended after the RANDOMIZE rows and before
  the two drill-in rows, "because it is not a randomizer and going last means
  no existing row index has to move" (`mergo-darkness.md` D2, and the identical
  comment in both screen files).
* **Its state is pinned to a known value rather than to whatever the user's
  dump contains** (`EnemyRandomizer.cpp` `StepEmevd`'s header comment). That
  reasoning is the direct precedent for §3.3's baked replacement identity.

### 2.2 The engine, and where the pass has to go

`EnemyRandomizerJob` is a phase machine
(`EnemyRandomizer.cpp:205-207`): `Mirror → ReadMaps → BuildPool → MergeModels →
BossCollect → BossAssign → TreasureCollect → TreasureAssign → WriteMaps →
Emevd → ItemData → Finished`.

Two properties of that machine do the work for us:

1. **Boss randomization finishes before the first map is written.**
   `StepBossAssign` mutates `lm.msbb` in memory across the `BossAssign` phase,
   including the final `AddTheRestInMap` step
   (`EnemyRandomizer.cpp:575-583`), and only then does `WriteMaps` begin. So a
   pass added inside `StepWriteMap` **automatically wins over the boss passes**,
   which is exactly the reference's outcome (spec §3 item 3, spec §7 "Order
   against the boss passes"). No new phase, no reordering.
2. **Enemy randomization happens inside `StepWriteMap`**, in the loop at
   `:606-676`, and the per-zone scaling pass runs at `:682-693`, after it and
   before serialisation. The reference's order is enemies → bosses →
   AddTheRest → NPCs → **EasyModes** → … → `ParamScalingForBosses`
   (`StartFunctions.cs:639`, `:952`, `:980`, `:1225`, `:1239`, `:2457`). So the
   slot between `} // if (options.randomizeEnemies)` and the scaling loop
   reproduces the reference's ordering exactly.

Also relevant, and verified rather than assumed:

* **`StepMergeModels` guarantees the replacement model is declared in every
  map.** It appends every enemy model seen in any of the 24 base maps into every
  map's `Models` section, and it runs unconditionally on every run
  (`EnemyRandomizer.cpp:481-499`). `c2521` is declared as an enemy model in
  **five** of those maps — `m24_00_00_00`, `m24_00_00_01`, `m24_01_00_00`,
  `m24_01_00_01`, `m24_01_00_11` — measured against
  `data/vanilla/dvdroot_ps4`. This is the port's equivalent of the reference's
  model-merge loop and it has no picker-path hole (spec §3 item 2, spec §6).
* **Every map is written on every run**, including a run with nothing enabled,
  so an easy-mode-only run has a natural place to live.
* **`RandInt` is only called from the enemy loop, the pool shuffle and the boss
  passes.** A pass that draws nothing cannot shift the stream, so spec §7's
  "not a randomizer" constraint is satisfied by construction as long as the new
  code contains no `RandInt` call. It does not.

### 2.3 The boss randomizer targets the same placements

`AddTheRestInMap` (`BossRandomizer.cpp:382-425`) replaces
`c2120_0001`/`c2120_0002` in m27 and `c4030_0001`/`_0002`/`_0003` in m35 with
random bosses — the same five placements Easy Shadows and Easy Failures target.
`AssignEligible` (`:65-79`) excludes exactly those five from the main boss pass,
and `kFixups` (`:141-146`) syncs `c2500_0000` → `c2570_0001`, so the Celestial
Emissary pair's leader is the one placement Easy Emissary deliberately leaves
alone. None of that changes; §2.2 item 1 is what makes the easy setting win.

### 2.4 Verifiers that already parse what this needs

* **`app/tools/boss_verify.py`** — its `Msbb` class reads DCX + MSBB and yields
  `(index, name, npc, think, entityID, modelIndex)` per enemy part, which is
  every field this feature touches. `compare_trees` already diffs two trees
  part-for-part. **It also has a problem this feature creates — see §5.4.**
* **`app/tools/param_offsets.py`** — `load_defs` / `load_param` / `param_rows` /
  `field_offsets` read `NpcParam` and `ItemLotParam` by field name. Every number
  in §3.3 and §6.2 came from these.
* **`app/tools/mergo_darkness_verify.py`** — the shape to copy: a single-purpose
  verifier for a non-randomizing toggle, with `show` / `verify` / `selftest`
  subcommands and a docstring that states plainly what the tool cannot prove.
* **`app/tools/starting_weapons_verify.py`** — the precedent for a verifier that
  parses the reference C# directly, used in §6.2 case 12.
* **`app/tools/ui_scroll_verify.py:48-52`** — pins `kItemCount` and
  `kSaveDataRowCount` at 14 for three screens today. Plan 016 takes them to 15
  (§9 decision 1), and these four settings rows take them to 19.

### 2.5 Layering

Everything lives in `Randomizer` (a new pass + four option bools) and `UI` (four
rows on two screens). The new pass depends on `Msb` only. No SDL2 reaches
`Randomizer`; no AFR path reaches `UI`. No new layer crossing.

### 2.6 Prior plans worth reading first

`docs/plans/mergo-darkness.md` (D2 row placement, D4 the `||`, D5 progress
reporting, and the "pin the output to a known value" argument),
`docs/features/016-unchanged-bell-maidens/plan.md` (the most recent settings-chain
plan, and the one this work collides with — §5.5),
`docs/plans/boss-randomization.md` §4.4-4.5 (`AddTheRest` and the fixup groups),
`docs/plans/pickers.md` D12 (why the picker guards do not interact with this).

---

## 3. Approach

### 3.1 One new file, one table, one function

Add `app/src/Randomizer/EasyModes.h` / `EasyModes.cpp`, following
`PermaDarkness` and `BossParamScaling` — each non-randomizing pass in this port
owns its own file, and a table in a header is what the Python mirror can parse
(the precedent is `enemy_lookup.py` parsing `EnemyExclusionList.h`).

```
struct EasyModeOptions { bool shadows, rom, failures, emissary; };   // all false
struct EasyModeCounts  { int shadows, rom, failures, emissary; };    // all 0

int ApplyEasyModes(const std::string& mapName, MsbbFile& msbb,
                   const EasyModeOptions& options, EasyModeCounts& counts);
```

The table is seven rows, keyed by **exact map name**, with a nullptr-terminated
pattern array — the same shape as `BossRandomizer.cpp`'s `FixupGroup`:

| Flag | Map | Name patterns | Placements matched (measured) |
|---|---|---|---|
| shadows | `m27_00_00_01` | `c2120_0001`, `c2120_0002` | 2 |
| rom | `m32_00_00_00` | `c1400` | 30 |
| rom | `m32_00_00_01` | `c1400` | 30 |
| failures | `m35_00_00_00` | `c4030_0001`, `c4030_0002`, `c4030_0003` | 3 |
| emissary | `m24_02_00_00` | `c2500_0001` … `c2500_0010` (10 patterns) | 7 |
| emissary | `m24_02_00_01` | `c2500_0001` … `c2500_0010` (10 patterns) | 7 |

**79 placements across the written tree with all four on**; 42 of them in the
six maps the retail game actually loads (spec §4). Both numbers are asserted in
§6.2, because a wrong count is the cheapest possible signal that a pattern list
was mistyped.

Two deliberate notes belong in the header beside the table:

* **`m27_00_00_00` is absent on purpose** (spec §10 D3). It holds the same three
  `c2120` placements with the same values — measured — and the reference still
  does not patch it. Adding it is a behaviour change, not a tidy-up.
* **Matching is substring, not equality** — `name.find(pattern) != npos`,
  mirroring the reference's `Name.Contains`. Rom's rule *depends* on this: the
  single pattern `c1400` is what takes all thirty children rather than a chosen
  few. Verified across all 43 map files that no pattern over-reaches (§6.2
  case 4).

The table being keyed by exact map name is a deliberate tightening of the
reference, which branched on `currentMap.Contains("m27")` against a full file
*path*. Behaviour-identical here — the port's 24 map names are unambiguous — and
it removes a class of accident the reference is exposed to.

### 3.2 Where it is called from

One line in `StepWriteMap`, between the enemy loop's closing brace and the
scaling loop:

```
ApplyEasyModes(lm.name, lm.msbb, options.easyModes, result.easyCounts);
```

`ApplyEasyModes` returns immediately if no flag is set, so a run with all four
off touches nothing and the output tree is byte-identical to today's for the
same seed and settings (spec §8 criterion 4).

Model-index resolution happens inside `ApplyEasyModes` with a local scan for the
enemy model named `c2521`, the same way `BossRandomizer.cpp`'s
`FindEnemyModelIndex` does it. If the model is not found — which should be
impossible after `StepMergeModels`, but see §5.2 — the function logs and leaves
every placement in that map untouched. It does **not** write a −1 index. This is
a deliberate divergence from the reference, which throws
`KeyNotFoundException` from `MSBB.Write` in the same situation
(`MSBBB.cs:180-190`); a crash mid-run on a console is strictly worse than a
setting that quietly did nothing, and the condition is already impossible.

**Ordering against the scaling pass: before, matching the reference.** The
reference calls `EasyModes` at `StartFunctions.cs:1239` and
`ParamScalingForBosses` at `:2457`. So a larva placed in a scaled zone is itself
scaled: `NPCParamID` 252100 becomes **900014609** in `m27_00_00_01`,
**900014611** in `m32_00_00_01`, **900014614** in `m24_02_00_01` and
**900014627** in `m35_00_00_00`, and stays 252100 in `m32_00_00_00` and
`m24_02_00_00`, which `BossScalingMaps()` does not cover. All 31 scaled variants
are measured to have **2 HP, 18 echoes, teamType 26, `behaviorVariationId`
25210 and the same item lot 28040** as the base row, so this cannot change what
the player fights — but it *is* what the output bytes will say, and §6 asserts
against that set rather than against 252100 alone.

### 3.3 The replacement identity is baked, not captured

The reference captures three values at runtime from whichever map placement
happens to carry model `c2521` (`MainWindow.xaml.cs:748-762`), guarded by
`addedStoneGuyBool` — a field declared in `FieldContainer.cs:60` and **never
assigned anywhere in the reference**, so the capture re-runs on every match and
the last one wins. Verified: all three `c2521` placements in the whole 43-file
tree carry identical values, so the defect is harmless there.

This plan bakes the identity instead:

```
const int32_t kEasyModeNpcParamId   = 252100;
const int32_t kEasyModeThinkParamId = 252100;
const char*   kEasyModeModelName    = "c2521";
```

Measured provenance for the header comment: three placements, all named
`c2521_0000`, in `m24_01_00_00`, `m24_01_00_01` and `m24_01_00_11`, all
NpcParam 252100 / Think 252100 / model `c2521`, entity ID 2410771.

Why baked rather than captured — the same argument `StepEmevd`'s comment already
makes for Mergo darkness: a captured value means "whatever bytes the user's
VanillaSource happens to hold", which is unknown by construction and differs
between users whose dumps are different revisions. Baking pins the output to a
known value. It is also how this port already handles every other piece of
reference data (`ModelSizeTable.h`, `NpcScalingTable.h`, `EnemyPoolTable.h`,
`EnemyExclusionList.h`). The one thing baking loses — noticing that a particular
dump disagrees — is bought back by §6.2 case 5, which asserts the baked triple
against the real vanilla tree. This was put to the developer as the "match the
reference or improve on it" call that `CLAUDE.md` §7 reserves for them, and
baking was chosen — §9 decision 4. The constants above are settled.

Note the reference's own capture already truncates the model name to five
characters (`Substring(LastIndexOf("*") + 1, 5)`), so the string it writes is
`"c2521"` exactly — the same literal baked above.

### 3.4 Four flags through the chain

Four plain bools, named `easyShadows`, `easyRom`, `easyFailures`,
`easyEmissary`, in `RandomizerDefaults` and in `EnemyRandomizerOptions` (the
latter grouped into the `EasyModeOptions` struct §3.1 defines, so the pass takes
one argument rather than four). Keys in `defaults.cfg`: `easy_shadows`,
`easy_rom`, `easy_failures`, `easy_emissary`. Absent key reads as `false`, which
means off, which means the maps are written exactly as they are today — so an
older `defaults.cfg` keeps meaning what it meant, with no special handling
(spec §7).

Four plain bools rather than one bitmask or one nested struct in
`RandomizerDefaults`, because that is what `randomizeStartingWeapons` /
`randomizeStartingGuns` / `randomizeShopWeapons` already are: three independent
bools for one conceptual group.

`SaveRandomizerDefaults`'s buffer is 1024 bytes with a current worst case near
450; four keys add about 80. `LoadRandomizerDefaults` reads into a 4096-byte
buffer. Neither needs to grow, and the existing `len > sizeof(buf)` clamp still
covers the failure it was written for.

### 3.5 Reporting the run

`EnemyRandomizerResult` gains four `int`s
(`easyShadowsReplaced`, `easyRomReplaced`, `easyFailuresReplaced`,
`easyEmissaryReplaced`) and `FinishCommit` emits one line per **enabled**
setting:

```
EASY SHADOWS REPLACED 2 PLACEMENTS
EASY ROM REPLACED 60 PLACEMENTS
EASY FAILURES REPLACED 3 PLACEMENTS
EASY EMISSARY REPLACED 14 PLACEMENTS
```

No `SKIPPING` counterparts — off means "leave the maps alone", which is not a
skipped step worth a line (`mergo-darkness.md` D5). The counts are fixed
constants, so printing them turns the console into an assertion: a `0` or a
wrong number is an immediate, unmissable sign that a pattern list or a map name
is wrong, which is the one failure mode a player could otherwise mistake for
"the setting does nothing in this fight". Note `EASY ROM` reads **60** and
`EASY EMISSARY` **14**, because the port writes both variants of those two maps;
see §5.3 so this is not mistaken for a bug.

### 3.6 Considered and rejected

**A1 — a new job phase (`EasyModes`) between `TreasureAssign` and
`WriteMaps`.** Rejected: it would need its own map loop, its own progress
denominator contribution and its own step accounting, to achieve exactly what
one call inside `StepWriteMap` already achieves. The phases exist because a step
must fit in one frame; this pass touches at most 30 placements in one map.

**A2 — run the pass after `ApplyBossParamScaling` so the written value is a
clean 252100 everywhere.** Tempting, because it makes §6 simpler and the output
more readable. Rejected: it is not what the reference produces
(`StartFunctions.cs:1239` precedes `:2457`), `CLAUDE.md` §7 says match before
improving, and the measurement in §3.2 shows the scaled variants are
indistinguishable in play — so the only thing this would buy is a tidier
verifier, at the cost of a deliberate deviation.

**A3 — normalise the map lists, adding `m27_00_00_00` for symmetry.**
Rejected outright: spec §10 D3 forbids it, and §6.2 case 2 asserts the omission
so nobody re-adds it by accident.

**A4 — reproduce the reference's runtime capture, including the
`addedStoneGuyBool` guard.** Rejected: the guard is a never-assigned field, i.e.
a defect, and the port has no `GenerateEnemyList`-equivalent that a picker run
skips, so the capture would have no hole to reproduce and nothing to gain.
Spec §6 rules the picker-path defect out of scope explicitly.

**A5 — blank the replaced placements' item lots, or strip the larva's drop, to
avoid relocating One Third of Umbilical Cord.** Out of scope unconditionally by
spec §10 D2 and spec §6. `NpcParam` is not touched by this feature at all, and
§6.2 case 9 asserts that.

**A6 — put the pass in `BossRandomizer.cpp`, beside `AddTheRestInMap`, since it
targets the same placements.** Rejected: two of the four settings (Rom,
Emissary) target ordinary randomizable enemies rather than bosses, the pass runs
whether or not boss randomization is on, and `BossRandomizer`'s phases run
before the write loop — which is the wrong side of the ordering guarantee in
§2.2.

**A7 — gate the four settings out of `StartCommit`'s `||`, like
`randomizeWorkshopTools_`.** Rejected: workshop tools is a *modifier* on
treasure randomization and does nothing alone, whereas each easy setting alone
produces a real, different map tree. `mergo-darkness.md` D4 is the matching
precedent.

---

## 4. Changes, file by file

Two new files. No file-format layout change, but `RandomizerDefaults`,
`EnemyRandomizerOptions` and `EnemyRandomizerResult` all gain members, so
**CLAUDE.md §2's full clean rebuild applies** — see §6.1.

**Every row count and row index below assumes plan 016 has already landed**, per
§9 decision 1. 016 inserts `UNCHANGED BELL MAIDENS` as row 4 (its §9.1), which
pushes `kDisableMergoDarknessRow` from 11 to 12 and both drill-in rows up by one
before this feature adds anything. §5.5 holds the arithmetic. If that ordering
ever changes, subtract one from every count and index here.

| File | Change |
|---|---|
| `app/src/Randomizer/EasyModes.h` | **New.** `EasyModeOptions`, `EasyModeCounts`, the baked replacement constants of §3.3 with their measured provenance, the seven-row target table of §3.1 with the `m27_00_00_00`-is-absent-on-purpose comment, and the `ApplyEasyModes` declaration. Written so `easy_modes_verify.py` can parse the table out of it. |
| `app/src/Randomizer/EasyModes.cpp` | **New.** The pass: early-return when no flag is set; per-map table lookup by exact name; local `c2521` model-index scan with the log-and-skip guard of §3.2; substring match; three field writes per hit; per-flag counters; one `Log` line per replaced placement in the existing `map/name: old -> new` shape. No `RandInt`. |
| `app/src/Randomizer/EnemyRandomizer.h` | Add `EasyModeOptions easyModes;` to `EnemyRandomizerOptions`, with a comment stating it draws no randomness and applies regardless of the other settings. Add the four `int`s of §3.5 to `EnemyRandomizerResult`. |
| `app/src/Randomizer/EnemyRandomizer.cpp` | `#include "EasyModes.h"`. One call in `StepWriteMap` between the enemy loop's closing brace and the `BossScalingMaps()` loop (§3.2). Add a bullet to the file header comment recording that the easy pass runs after enemy/boss randomization and before scaling, and why. |
| `app/src/Randomizer/RandomizerDefaults.h` | Four `bool … = false;` fields with the standard absent-key-means-false comment. |
| `app/src/Randomizer/RandomizerDefaultsStore.cpp` | Four `else if` load branches (`easy_shadows`, `easy_rom`, `easy_failures`, `easy_emissary`) and four `%d` plus arguments in the save `snprintf`. Buffers unchanged — §3.4. |
| `app/src/UI/SetupDefaultsScreen.h` | `kItemCount` 15 → 19; four new row constants directly after `kDisableMergoDarknessRow` (row 12), occupying rows 13–16 in the order `EASY SHADOWS`, `EASY ROM`, `EASY FAILURES`, `EASY EMISSARY`; `kEnemiesIncludedRow` 13 → 17 and `kBossesIncludedRow` 14 → 18. Position and order fixed by §9 decision 2. |
| `app/src/UI/SetupDefaultsScreen.cpp` | Four `ToggleRow` branches with their `defaults: easy … = YES/NO` logs; four entries in `DrawList`'s `items`, in the same order as the row constants. |
| `app/src/UI/EnableWizardScreen.h` | Four `bool easy…_;` members beside the other per-run toggles. |
| `app/src/UI/EnableWizardScreen.cpp` | Four row constants in the same order and position as the Setup Defaults screen; `kSaveDataRowCount` 15 → 19; drill-in row constants renumbered; ctor init from defaults; four left/right branches; four X branches; four entries in **both** `DrawSaveData` and `DrawConfirm` item lists (they must stay identical — `ui_scroll_verify` treats them as one shape); four `options.easyModes.… = easy…_;` assignments in `StartCommit`; the four flags **added** to the big `||` (§3.6 A7); four result lines in `FinishCommit` (§3.5). The two `NoneEnabled()` picker guards are untouched — they are gated on `randomizeEnemies_`/`randomizeBosses_` and an easy-only run passes straight through them. |
| `app/tools/easy_modes_verify.py` | **New.** `show` / `verify` / `selftest`, modelled on `mergo_darkness_verify.py`. §6.2 and §6.3. |
| `app/tools/boss_verify.py` | Add an `--easy` flag to `verify` that suppresses the V2 non-boss false positives this feature creates, and says in its output that it is doing so. §5.4. |
| `app/tools/ui_scroll_verify.py` | Row counts 15 → 19 for `Setup Defaults`, `Wizard SaveData`, `Wizard Confirm`; raise the `Progress log` worst-case count to cover the four new result lines; update the comment that states why. |

**Explicitly not changed:** `NpcParam` or any param member (spec §8 criterion 6),
`EnemyExclusionList.h`, `EnemyPoolTable.h`, `BossList.h`, `BossRandomizer.cpp`,
`gen_pool_table.py`, `docs/randomization-feature-spec.md`, `docs/user-guide.md`,
anything under `reference/`.

---

## 5. Risks and unknowns

### 5.1 The fights may not end — the one risk no verifier can reach

This is the headline risk and it is stated first because it is the only one that
could make the feature worthless.

**What is established.** `ApplyEasyModes` writes three fields and nothing else:
EntityID, position, part name and every index-based cross-reference are left
exactly as they were. Measured: all three Shadow entity IDs (2700800-2700802)
appear ~32-33 times each in `event/m27_00_00_00.emevd.dcx`, and all five Living
Failure IDs (3500850-3500854) ~26-28 times each in
`event/m35_00_00_00.emevd.dcx`, so the fight logic clearly tracks the individual
bodies. Leaving those IDs alone leaves that logic intact.

*(Spec §4 quotes 37-39 and 34-37 for these counts. The difference is method: the
numbers here are a raw four-byte little-endian scan of the compressed-then-
inflated EMEVD, not an instruction-argument parse, so they undercount arguments
that are not 4-byte-aligned literals. The conclusion — heavy per-body scripting
— is identical, and nothing in spec §2 depends on the exact figure.)*

**What is not established.** Whether the game's completion condition is "these
three entities are dead" (fine) or something tied to the creature's own death
behaviour (not fine). Only hardware answers it — §6.4 steps 2-4.

**A related unknown specific to Rom.** Rom's children are measured as 30 static
MSB placements referenced only ~3 times each in
`event/m32_00_00_00.emevd.dcx` — far lighter scripting than the Shadows or the
Failures, consistent with a simple enable/disable per wave. Whether her fight
*also* spawns children dynamically at runtime is not knowable from map data. If
it does, Easy Rom thins the fight rather than emptying it. Same in the
reference; recorded so a hardware observation of "some real spiders appeared"
is read as this rather than as a broken rule.

**Nothing is deleted.** Worth stating explicitly because spec §2's "None of her
thirty children remain" reads at a glance like removal: all 30 bodies are still
in the map, still have to be killed, and still carry their entity IDs. Spec §1
says so; the documentation stage should make sure `docs/user-guide.md` does too.

### 5.2 The replacement model must be declared in the map being written

A placement cannot reference a model name the map's `Models` section does not
contain. `StepMergeModels` guarantees it for all 24 maps and runs
unconditionally, and `c2521` is declared in five of them (§2.2). The risk is not
that this fails today — it is that a future change conditionalises or removes
the merge and nobody notices until a map fails to load on hardware.

Handled three ways: the log-and-skip guard in §3.2 means a missing model can
never write a −1 index; §6.2 case 6 asserts `c2521` is declared in at least one
base map; and spec §7 already records the constraint. Failure on hardware would
look like a map that does not load at all.

### 5.3 79 replacements, not 42 — do not read the extra as a bug

The port writes **both** variants of Byrgenwerth and Upper Cathedral Ward, so
with all four settings on the tree contains 79 replaced placements: 2 + 30 + 30
+ 3 + 7 + 7. The retail game loads one variant of each, so the player meets 42
(spec §4). Both numbers are asserted separately in §6.2 so that a future count
mismatch points at which one moved.

### 5.4 `boss_verify.py verify` will report false failures on an easy tree

Concrete and already reasoned through, not speculative.
`compare_trees` classifies every changed placement, and:

* `c1400_*` and `c2500_0001…_0010` are **not** matched by `BOSS_NAMES`, so
  without `--enemies-also` each one raises `V2 non-boss placement changed`. That
  is 67 spurious failures on an all-four-on tree.
* `c2120_0001/0002` and `c4030_0001…_0003` **are** boss names, and the existing
  `addtherest` exemption already covers them, so they are quiet.
* `identity_ok` accepts the larva either way: `(252100, 252100, "c2521")` is a
  real vanilla triple, and each scaled variant is in the scaling table with
  `(252100, "c2521")` as a real vanilla think/model pair. So the identity checks
  pass; only the "may this placement change at all" check misfires.

The fix is the minimal `--easy` flag in §4: suppress the non-boss objection for
exactly the names and maps the table lists, print that it is doing so, and leave
the positive assertions to `easy_modes_verify.py`. Keeping the positive work out
of `boss_verify.py` is deliberate — that tool's V2 semantics are load-bearing
for boss randomization and should not grow a second meaning.

### 5.5 This collides with plan 016, which is not yet implemented

`docs/features/016-unchanged-bell-maidens/plan.md` is at "QUESTIONS ANSWERED —
awaiting developer approval" and has not been implemented (no `app/src` change
is present in the working tree). It touches **six of the same files**:
`RandomizerDefaults.h`, `RandomizerDefaultsStore.cpp`, both screen headers, both
screen sources, and `ui_scroll_verify.py`; it also edits the same
`StepWriteMap` loop this plan appends after, and removes `StepBuildPool`'s
up-front empty-pool `Fail`.

**016 lands first** (§9 decision 1), so §4 is written for the post-016 tree:
`kItemCount`/`kSaveDataRowCount` are 15 → 19, and `kDisableMergoDarknessRow` is
12 rather than 11, because 016 inserts its row at index 4 and shifts everything
below it down one. Nothing else changes — the two features do not interact
functionally (016 is an exclusion-list change, this is a placement overwrite;
`docs/features/016-unchanged-bell-maidens/spec.md` §6 and `docs/features/018-easy-shadows/spec.md` §6 each rule the other out).

The collision is therefore sequencing, not design, and the sequencing is
settled. What remains is a **precondition on implementation**: before this
feature is built, confirm that 016 is actually in `app/src` — read
`kDisableMergoDarknessRow` and `kItemCount` out of `SetupDefaultsScreen.h` and
check they are 12 and 15. If they are 11 and 14, 016 has not landed after all
and every index in §4 is one too high.

### 5.6 Smaller risks

* **"Team type 26 means non-hostile" is a byte, not observed behaviour.**
  Measured: across all 43 map files the creatures sharing teamType 26 are the
  Doll, Gehrman, Willem, the Messengers and the strapped-down patients. That is
  strong, and it is still exactly the kind of inference `CLAUDE.md` §5 and the
  standing note on setting polarity say not to trust without a console. §6.4
  step 2 is the check.
* **The umbilical cord relocates.** Accepted by spec §10 D2. Verified here:
  ItemLot 28040 is the **only** row in `ItemLotParam` granting item 4321, it is
  referenced by exactly 32 `NpcParam` rows (252100 plus its 31 scaled variants),
  it carries a single `getItemFlagId` (50001205), and `NpcParam` 252100 is one
  of the two rows `DropRandomizer.cpp:24`'s `IsExcludedNpcRow` protects. §6.4
  step 5 is the hardware check that decides which reading of `getItemFlagId` is
  true; a cord per larva is the trigger for revisiting D2 with evidence.
* **`result.npcParamsScaled` inflates.** The scaling pass runs after the easy
  pass and will rewrite every larva it placed in a scaled zone, adding 42
  to that counter. Cosmetic, it is what the reference does, and the new
  per-setting lines in §3.5 are the numbers to read instead.
* **`result.enemiesRandomized` may double-count.** With RANDOMIZE ENEMIES on,
  the enemy loop can randomize a Rom child or a small emissary and the easy pass
  then overwrites it; the enemy counter still counts it. Also what the reference
  does. Cosmetic.
* **Renumbering the drill-in row constants is safe.** They are compile-time
  constants carrying no saved meaning. The only row order that is load-bearing
  for saved configs is `EnemyPoolTable.h`'s, which this feature does not touch
  (`pickers.md`; plan 016 §5.3).
* **The four labels render.** Checked against `app/src/Platform/Font8x8.cpp`'s
  actual glyph table rather than any list written elsewhere: every character in
  `EASY SHADOWS`, `EASY ROM`, `EASY FAILURES`, `EASY EMISSARY` and in §3.5's
  result lines is present, space included.
* **A Python mirror pins the rules, not the C++ implementation of them.** There
  is no host C++ compiler, so `easy_modes_verify.py` proves that the *table and
  the identity* are right and that a generated tree has the right *properties*.
  It cannot prove `ApplyEasyModes` matches it. The two can drift; this is the
  standing, accepted weakness of this setup.

### 5.7 Nothing here can make a run unwinnable

The affected placements hold no items, gate no progression and are not key-item
sources. The worst case short of §5.1 is a fight that is easier than intended,
which is the point of the feature. The one item involved — One Third of
Umbilical Cord — is optional and, per §5.6, relocated rather than lost.

---

## 6. Verification

### 6.1 Build

```
cd app
rm -rf src/x64
make
```

**The clean rebuild is required, not optional.** `RandomizerDefaults`,
`EnemyRandomizerOptions` and `EnemyRandomizerResult` all gain members, which is
a struct layout change, and `CLAUDE.md` §2 records that a stale partial rebuild
once produced a real heap-corruption SIGSEGV on hardware.

### 6.2 `easy_modes_verify.py selftest` — the rules, pinned

`python tools/easy_modes_verify.py selftest ../data/vanilla/dvdroot_ps4`.
The table and the constants are parsed out of `app/src/Randomizer/EasyModes.h`,
so the mirror cannot drift from the port. Each case asserts a measured fact, and
every number below was measured while writing this plan.

1. **The table resolves to exactly the expected placement sets.** Per map:
   m27_00_00_01 → `{c2120_0001, c2120_0002}`; m32_00_00_00 → 30 `c1400_*`;
   m32_00_00_01 → 30; m35_00_00_00 → `{c4030_0001, _0002, _0003}`;
   m24_02_00_00 → `{c2500_0001, _0002, _0003, _0006, _0007, _0009, _0010}`;
   m24_02_00_01 → the same seven. **Totals: 79 tree-wide, 42 in the
   retail-loaded maps.**
2. **`m27_00_00_00` is not in the table**, and it holds three `c2120`
   placements with the same NpcParams as m27_00_00_01 (212700/212710/212720).
   Spec §10 D3 asserted rather than assumed.
3. **The survivors are exactly these, and untouched by any pattern:**
   `c2120_0000` (NpcParam 212700, 1425 HP — the strongest of the three against
   900 and 800), `c4030_0000` (403000, 320 HP), `c4030_0004` (403050,
   ThinkParamID **1**, the value the port's own pool rules treat as "not a live
   enemy"), `c2500_0000` (250080, the Celestial Emissary pair's leader in
   `BossRandomizer.cpp`'s `kFixups`), and `c2500_0011/_0012/_0013` (250082 —
   the three elsewhere in Upper Cathedral Ward). Rom herself is a different
   model and matches no pattern.
4. **No pattern over-reaches.** Across all 43 `.msb.dcx` files, the set each
   pattern matches is exactly the set in case 1 — in particular
   `Contains("c2500_0001")` does not also catch `c2500_0011`. This is the one
   rule that could silently take more than intended.
5. **The baked replacement triple is real.** `(252100, 252100, "c2521")` is the
   identity of all three `c2521_0000` placements in the tree —
   `m24_01_00_00`, `m24_01_00_01`, `m24_01_00_11`, entity ID 2410771 — and they
   are identical to each other, which is what makes the reference's
   never-assigned `addedStoneGuyBool` harmless.
6. **`c2521` is declared as an enemy model** in at least one of the 24 base maps
   the port loads — measured, in five: `m24_00_00_00`, `m24_00_00_01`,
   `m24_01_00_00`, `m24_01_00_01`, `m24_01_00_11`. This is what makes
   `StepMergeModels` able to satisfy §5.2.
7. **The replacement cannot be dangerous.** `NpcParam` 252100 has **2 HP, 18
   blood echoes, teamType 26, hitHeight 1.0, hitRadius 0.2**, and all 31 scaled
   variants (900014601-900014631) have **identical** HP, echoes, teamType,
   `behaviorVariationId` (25210) and item lot. Therefore the §3.2 ordering
   choice cannot change what the player fights.
8. **The per-zone scaled values are what §3.2 claims:** 252100 sits in
   `NpcScalingTable.h` immediately before 900014601-900014631, so
   `m27_00_00_01` (+9) → 900014609, `m32_00_00_01` (+11) → 900014611,
   `m24_02_00_01` (+14) → 900014614, `m35_00_00_00` (+27) → 900014627, and
   `m32_00_00_00`/`m24_02_00_00` are absent from `BossScalingMaps()` so they
   keep 252100.
9. **The drop is protected on both sides.** ItemLot 28040 is the only row in
   `ItemLotParam` granting item 4321, with a single `getItemFlagId` 50001205;
   exactly 32 `NpcParam` rows reference it; and `252100` appears in
   `DropRandomizer.cpp`'s `IsExcludedNpcRow`, parsed straight out of the C++.
10. **No pattern matches a boss arena slot the feature must leave alone** —
    `c2570_0001` (the Celestial Emissary itself) and `c2120_0000` are matched by
    nothing in the table.
11. **The reference's own name lists appear verbatim** in
    `reference/Randomizer/MainWindowComponents/MainWindow.xaml.cs` — a fidelity
    pin, following `starting_weapons_verify.py`'s precedent of parsing the
    reference C# directly. Note this couples the selftest to that path, which
    `CLAUDE.md` §1 already fixes in place.
12. **Corrupt-and-catch cases**, in `boss_verify.py selftest`'s style, so a
    validator that has only ever returned PASS is not trusted: a survivor
    changed → caught; a target left at its vanilla identity → caught; an
    EntityID changed on a replaced placement → caught; an unmodified tree →
    clean, no false positives.

The tool's docstring must state plainly that it pins the rules and the data, not
the C++ implementation of them (§5.6), and that nothing here can establish that
the replacement is harmless or that the fights end.

### 6.3 `easy_modes_verify.py verify` — for after a run

```
python tools/easy_modes_verify.py verify <vanilla_dvdroot> <output_dvdroot> \
       [shadows] [rom] [failures] [emissary]
```

Asserts spec §8's contract from the two trees:

* every placement the named settings' table rows select carries the replacement
  identity — NpcParam 252100 **or** the map's scaled variant from case 8,
  ThinkParamID 252100, model `c2521`;
* every one of them keeps its **EntityID and its position** (the three floats at
  part entry + 0x28);
* the survivors of case 3 are **byte-identical** to vanilla in all three written
  fields;
* **no other placement in the affected maps differs from vanilla** beyond what
  enemy/boss randomization legitimately changed, and with every randomizer off,
  no other placement differs at all;
* maps whose settings are off are byte-identical to vanilla;
* `NpcParam` and `ItemLotParam` in the output tree are unchanged in rows 252100
  and 28040 — spec §8 criterion 6, checked even when drop randomization is on.

The "unchanged with the setting off" comparison runs **against vanilla**, not
against a second generated tree. A second tree would only prove the two runs
agree; vanilla is the only input that can tell "frozen" from "redrew the same
thing" (`CLAUDE.md` §3).

### 6.4 Other existing verifiers

```
python tools/ui_scroll_verify.py                                  # 18 rows, three screens
python tools/boss_verify.py selftest ../data/vanilla/dvdroot_ps4  # must still pass untouched
python tools/boss_verify.py verify  <V> <B> --enemies-also --easy # §5.4
python tools/drops_verify.py selftest ../data/vanilla/dvdroot_ps4 # D-I4 still holds
```

### 6.5 Hardware — what the developer runs

Three trees: vanilla `V`, seed `S` with all four settings **off** giving `A`,
the same seed `S` with all four **on** giving `B`. Vanilla as the third input is
mandatory (`CLAUDE.md` §3).

| # | Step | Pass looks like |
|---|---|---|
| 1 | `easy_modes_verify.py verify V B shadows rom failures emissary`, and `boss_verify.py verify V B --easy` | both PASS; the four progress lines on console read 2 / 60 / 3 / 14 |
| 2 | Play `B`, Shadows of Yharnam | one Shadow fights; two small creatures stand in for the others. **Do they attack?** Do they die in one hit? **Does the fight end after the third body dies, and does the fog lift?** This is the §5.1 inference that matters most |
| 3 | Play `B`, Rom | every child is a larva; Rom's fight still progresses through her teleports and she dies normally. Note whether any *real* spider appears — see §5.1 |
| 4 | Play `B`, Living Failures and Celestial Emissary | one opponent each, both fights end normally, and the Emissary still grows out of the surviving small body |
| 5 | Play `B`, kill the first easy-mode larva you reach, then visit Iosefka's Clinic | **one** One Third of Umbilical Cord in total is the expected result. A cord per larva contradicts §5.6 and is the trigger for revisiting spec §10 D2 with evidence |
| 6 | Play `A` (all four off) | the four fights are exactly as they were. This is what makes steps 2-5 mean anything |
| 7 | Run with **only** `EASY SHADOWS` on and every randomizer off | the run starts (§3.6 A7), completes, and only m27_00_00_01 differs from vanilla — the independence claim in spec §8 criterion 5 |
| 8 | Reopen Setup Defaults after saving | the four new rows persist, and `ENEMIES INCLUDED` still reads `82 OF 82` — a changed count would mean the config string was misread when the four new keys were added |

**Failure would look like:** a fight that never ends because a replaced body no
longer registers as dead; a replacement that is hostile, or tough enough to
matter; an easy setting losing to boss randomization so the duplicates are still
bosses; a map that fails to load at all, which would point at the model
declaration (§5.2); or a wrong count on a progress line, which points at the
table.

Building cleanly and passing every selftest means **ready for hardware test**,
never done.

---

## 7. Milestones

One milestone, and it **starts after plan 016 is implemented** (§9 decision 1).
That is a sequencing constraint on when this work begins, not a milestone of its
own: 016 carries its own build and its own hardware cycle, and this plan's §4
numbers are written for the tree 016 leaves behind (§5.5).

| # | Milestone | Ends with |
|---|---|---|
| 1 | `EasyModes.h/.cpp` with its table and baked identity, the call site in `StepWriteMap`, the four flags through defaults/store/both screens, the four result lines, and the three verifier changes. Clean rebuild, §6.2-6.4 green. | `.pkg` built, awaiting the §6.5 hardware test |

**Why not four milestones, one per setting.** They share one table, one
function, one identity and one settings chain; splitting them would mean
renumbering the same UI rows four times and four hardware trips to answer
questions that one trip answers. Spec §10 D1 approved the four as one feature
and spec §8 already describes the hardware test as a single session covering all
four.

**Why not two — the pass first, the UI second.** A milestone must end in
something the developer can hardware-test. The pass with no UI has no way to be
switched on, so the stop would buy a `.pkg` and a console cycle to learn
nothing.

**The honest counter-argument, recorded so it can be overruled:** the biggest
unknown (§5.1) is per-fight, and Shadows passing does not prove Rom passes. A
Shadows-only first milestone would fail fast on the *mechanism* — three fields,
one larva, does the fight end — before the other three rows are wired up. It is
rejected because the mechanism is identical across the four and the marginal
cost of the other three rows is small; but if the developer would rather buy
that de-risking, splitting after §6.2 passes costs nothing but a second UI pass.

---

## 9. Decisions

*§8 Open questions is deliberately absent. All four questions it held were put to
the developer and answered on 2026-09-16 and are recorded below. The heading is
not reused and nothing is renumbered, so other documents can keep citing these
sections by number. §1–§7 have been reconciled against the answers.*

| Date | Decision |
| --- | --- |
| 2026-09-16 | **1. Plan 016 lands first; this feature is built on the tree 016 leaves behind.** The plan's recommendation. The two features do not interact functionally — the collision is purely the arithmetic of UI row indices — but 016 is older, smaller, and changes already-shipped flag-off behaviour, which deserves its own hardware cycle rather than being attributed inside a four-setting build. §4 is written for the post-016 numbers and §5.5 states the precondition to check before starting. |
| 2026-09-16 | **2. The four rows are appended after `ENABLE MERGO DARKNESS`, in backlog order** — `EASY SHADOWS`, `EASY ROM`, `EASY FAILURES`, `EASY EMISSARY` — ahead of the two drill-in rows, which stay last. This is the placement rule the last non-randomizer already follows (`mergo-darkness.md` D2) and the order the backlog, the spec and its decisions table all use. The reference window's left-to-right checkbox layout (Shadows, Failures, Rom, Emissary) is treated as layout accident, not intent. |
| 2026-09-16 | **3. One progress line per enabled setting, each carrying its count.** `EASY SHADOWS REPLACED 2 PLACEMENTS`, `EASY ROM REPLACED 60 PLACEMENTS`, `EASY FAILURES REPLACED 3 PLACEMENTS`, `EASY EMISSARY REPLACED 14 PLACEMENTS`. The counts are fixed measured constants, so printing them makes the console a free assertion — a `0` or a wrong number is the one failure a player would otherwise read as "this fight just isn't affected". No `SKIPPING` counterparts (`mergo-darkness.md` D5). |
| 2026-09-16 | **4. The replacement identity is baked, not captured at runtime.** `252100 / 252100 / "c2521"` as named constants in `EasyModes.h`, with §6.2 case 5 asserting the baked triple against the real vanilla tree on every selftest run. Determinism over a captured value that means "whatever bytes this user's dump holds", consistency with every other baked reference table in the port, and less code. The reference's runtime capture is guarded by `addedStoneGuyBool`, declared and never assigned, so its guard does nothing. |

**Notes carried from the decisions into the body:**

* Decision 1 rewrote every row count and index in §4 to the post-016 values and
  turned §5.5 from an open collision into a precondition to verify before
  implementation starts. It also puts a sequencing sentence at the head of §7.
* Decision 2 fixes the position and the within-group order in §4's two screen
  rows; nothing else in the plan depended on it.
* Decision 3 confirms §3.5 as written — four `int`s on `EnemyRandomizerResult`
  and four `AddProgressLine` calls.
* Decision 4 confirms §3.3 as written; the alternative (a new job-state field, a
  scan in `StepReadMap`, and a policy for "no `c2521` placement seen") is now
  out, and §3.3 no longer presents it as open.

---

## 10. Changes from the plan

*Left empty until implementation begins.*

---

<!--
Not in this document, on purpose:

  - what the feature is and why it exists (that is docs/features/018-easy-shadows/spec.md)
  - the plan review's findings (stage 2, its own file)
  - production code
-->
