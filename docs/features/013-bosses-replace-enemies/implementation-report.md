# Implementation Report 013 — Bosses Can Replace Enemies — milestones M1–M4

**Status: AWAITING HARDWARE TEST**

**Plan:** `docs/features/013-bosses-replace-enemies/plan.md` — milestones 1–4 of 5

**Spec:** `docs/features/013-bosses-replace-enemies/spec.md`

**Implemented:** 2026-09-29

---

## 1. What was built

All four milestones of the continuous block: the four new setting kinds and both
generated tables (M1), the eight settings with their persistence and the buffer
growth to 2,048 (M2), the insertion pass itself (M3), and
`insert_bosses_verify.py` (M4). Execution stopped at the Required gate before M5
without touching **REPLACE ALL ENEMIES**, which ships live and documented as
temporary. The `.pkg` builds warning-free and every automated check in plan §6
that does not need a console passes.

### M1 — the settings model can express the four new kinds

| # | Planned change | Status | Where |
| - | -------------- | ------ | ----- |
| 1 | `insertion_list()` / `insertion_models()` / `insertion_areas()` in `boss_verify.py` | done — 32 identities / 22 models, `c5400` absent, set-identical over all three map orders | `app/tools/boss_verify.py:306-437` |
| 2 | `insertboss` and `area` `Kind`s; both headers generated | done — `--check` passes for both and for all three shipped tables | `app/tools/gen_pool_table.py`, `app/src/Randomizer/InsertedBossTable.h`, `InsertAreaTable.h` |
| 3 | The two selection typedefs | done — `InsertedBossSelection` (22), `InsertAreaSelection` (14), each bound to one table | `app/src/Randomizer/EnemyPoolSelection.h:65-66` |
| 4 | `SettingKind` 7 → 11; five `SettingDef` members **after** `help` | done — no shipped `kSettings` row edited | `app/src/UI/SettingsModel.h:79-105,145-165` |
| 5 | `SettingValueText`, `AdjustSetting`, `IsDrillIn`, the four `Selection*` switches, two `PickerStrings` blocks | done — every switch exhaustive, no `default:` label anywhere | `app/src/UI/SettingsModel.cpp`, `app/src/UI/ModelPicker.h` |
| 6 | `CountChangedSettings` gains a `Number` case | done — before the `SelectionFlags` fall-through, which is where the bug was | `app/src/UI/WorldEditorScreen.cpp:864-872` |

### M2 — the eight settings exist, persist and are editable

| # | Planned change | Status | Where |
| - | -------------- | ------ | ----- |
| 1 | Six scalars and two selections in `RandomizerDefaults.h`, each with its field comment | done | `app/src/Randomizer/RandomizerDefaults.h:241-318` |
| 2 | `char buf[1024]` → `2048`, comment rewritten to the real arithmetic | done — **rebased to the measured tree**, see §2 | `app/src/Randomizer/RandomizerDefaultsStore.cpp:28-46` |
| 3 | Eight keys in `FormatSettings` **and** `ApplySettingKey` | done — all eight written and read; the count key is clamped 0–15 on load too | same file |
| 4 | Eight `SettingId` values and eight `kSettings` rows after `BOSSES INCLUDED`, three carrying B22's warning | done — widest row is `BOSS BEHAVIOUR` at 663 px of 700, exactly D1's figure | `app/src/UI/SettingsModel.h`, `SettingsModel.cpp:50-120` |
| 5 | The eight into `randomizer-settings-ui/spec.md` §7.1 and `PROSE_TO_LABEL` | done — `Bosses Can Replace Enemies` and `Per-Zone Toggles` left the backlog column | `docs/features/randomizer-settings-ui/spec.md`, `settings_ui_verify.py` |
| 6 | `settings_ui_verify.py` parser/`KIND_TO_COUNT`/`widest_value`/case 1/case 9; `pool_verify.py` block, buffer, per-key cases | done — 116/116 and 108/108 | `app/tools/settings_ui_verify.py`, `app/tools/pool_verify.py` |

### M3 — the insertion pass

| # | Planned change | Status | Where |
| - | -------------- | ------ | ----- |
| 1 | `part_fields::GetPosition`, offset derivation in the header comment | done — read-only, three `float32` at `+0x28`, no `Set` counterpart | `app/src/Msb/Msbb.h:105-122`, `Msbb.cpp:159-173` |
| 2 | `BossInsertion.h` — the four entry points, the pass's position as behaviour, `REPLACE ALL ENEMIES` in the `RuneProbe.h` shape | done — the block states the question, the design and what retires it, naming §6's hardware run and `log.md` | `app/src/Randomizer/BossInsertion.h` |
| 3 | `CollectInsertionCandidates` / `FinalizeInsertionPool`, wired into `StepReadMap` and `StepBuildPool` behind the master toggle | done — logs the identity count, or that nothing is ticked | `BossInsertion.cpp:125-165`, `EnemyRandomizer.cpp` |
| 4 | `ApplyBossInsertion` steps 1–5 | done — both count modes, the override, both spacing states, both AI states; the off path reaches no `RandInt` | `BossInsertion.cpp:168-320` |
| 5 | Called in `StepWriteMap` after `ApplyEasyModes`, before the `BossScalingMaps()` loop | done | `EnemyRandomizer.cpp` (`StepWriteMap`) |
| 6 | Options and result plumbing, `WorldActivation.cpp`, three progress lines | done — count, silent skip (two flavours: no bosses ticked, no areas ticked), shortfall | `EnemyRandomizer.h`, `Game/WorldActivation.cpp`, `UI/WorldEditorScreen.cpp` |

### M4 — the pass is verified against real data

| # | Planned change | Status | Where |
| - | -------------- | ------ | ----- |
| 1 | The oracles | done — 56 selftest cases | `app/tools/insert_bosses_verify.py` |
| 2 | The Python model and its distribution checks, including the single-boss mean | done — the single-boss and all-32 means are asserted **equal**, not close | same |
| 3 | `verify <vanilla> <output>` reporting criteria 2–18 individually | done — each criterion on its own line, each failure naming map and placement | same |
| 4 | Run it against hardware-generated trees for the default, the maximum count and the override | **NOT RUN — no console.** See §2 and §5 | — |

---

## 2. Deviations from the plan

All twelve are in plan §10. In brief, and in order of how much they matter:

1. **The byte arithmetic is rebased.** §5 says 976 / 1,027; the measured tree
   gives **1,038 / 1,089**, which is §5's own "013 + 011 + 027" row. Derived from
   the baseline `pool_verify.py selftest` reported before any edit — 790 / 841 —
   plus this plan's 248, and the two figures came out at 1,038 and 1,089 with no
   assertion adjusted to fit the code. D-I's conclusion stands: at
   `char buf[1024]` the block overflows by 14 bytes and `snprintf` clamps in
   silence.
2. **`pool_verify.py`'s buffer fit now reads `FormatSettings`' own
   `char buf[N];`.** It was comparing against `max(bufs)`, which is
   `LoadRandomizerDefaults`' 4,096-byte *read* buffer — so the fit check would
   have passed at 1,038 bytes against a 1,024-byte buffer. This feature is the
   first change where the difference decides the answer. The regex also gained a
   `;`, so the narrating comment's prose mentions of `char buf[1024]` are not
   read as code.
3. **M1 and M2 were built and verified as one unit.** M1 step 5 extends
   `SettingValueText` and the four `Selection*` switches, which cannot compile
   without the `RandomizerDefaults` fields M2 step 1 adds; and the two new
   selections break `settings_ui_verify.py`'s `len(selections) == 5` until M2
   step 6 extends its parser. A compile dependency, not a judgement. Step order
   within the two is unchanged, and no gate sits between them.
4. **`ToggleCount()` goes 18 → 21**, not §7 M2's 15 → 18 — the same rebase for
   rows 11 and 27. The invariant holds exactly: the number, the two named choices
   and the two pickers are excluded.
5. **The eight options are nested** as `EnemyRandomizerOptions::bossInsertion`,
   following `easyModes`. `WorldActivation.cpp` still gains exactly eight lines.
6. **`InsertionOptions` carries `skipPatterns` and `doNotRandomizeCagedDogs`**,
   filled by `EnemyRandomizerJob::State`'s constructor. §4's signature passes one
   options struct, and the three B5 protections must be stated in one place.
7. **`gen_pool_table.py`'s `Kind` gained `name_fn` and `weight_blurb`**, and the
   row format pads the model column. All three shipped tables are byte-identical
   — `--check` proves it.
8. **`pool_verify.py`'s PickerStrings count reads seven**, plus one new case for
   the two new pickers' `showRowId`.
9. **The `replace_all_enemies` comment says "the retired bell-maidens key"**
   rather than spelling it, because `pool_verify.py` asserts the file's only
   mention of that key name is the one tolerance comment.
10. **UP TO's mean is asserted against the analytic mean of §4 step 3's own
    rule**, not against N. See §3 — this one is a finding, not a preference.
11. **`insert_bosses_verify.py selftest` gained eight injection cases** and
    `verify`'s comparison was split into `compare_trees` so it could.
12. **M4 step 4 was not run.** It needs trees generated on the PS4.

---

## 3. Decisions the plan left open

### The UP TO ceiling costs more at a low count than §6's wording suggests

§6 asks for "a mean over 200 seeds that approaches" N. The rule in §4 step 3 —
one draw per eligible placement at probability `cap / supply`, with a hard
ceiling at `cap` — has an *uncapped* mean of exactly `cap`, and truncating the
upper tail lowers it. Measured in the smallest area's 54 targets:

| N | Measured mean over 200 seeds | Analytic `E[min(X, cap)]` |
| -: | ---: | ---: |
| 1 | 0.65 | 0.64 |
| 5 | 4.29 | 4.16 |
| 15 | 13.93 | 13.69 |

The implementation is §4 step 3 exactly; the gap belongs to the rule. Rather
than loosen either, the verifier asserts against the rule's own analytic number,
which is roughly twenty times tighter than a tolerance around N and is still the
check the D-M failure fails. **The consequence worth telling the developer and
the documentation stage:** at the default of count 1 in **UP TO**, a full run
places about **nine** bosses, not the fourteen spec §2 row 3 quotes. **EXACTLY
1** is the setting that gives fourteen. Spec §2 row 2 already describes UP TO as
a ceiling some maps do not reach, so this contradicts nothing — but the figure in
row 3 reads as a promise and is worth a sentence.

### No boss-name test in the eligible-target collector

B5 defines an eligible target by three protections plus "the model resolves", and
`plan-evidence.md` §E4 M4 measures zero of the 62 boss-named placements as
eligible — all of them are on the fixed exclusion list. So the collector applies
the three rules and no fourth one. Adding a defensive `IsBossName` test would put
a second copy of a rule that already holds, in a place where it could only ever
disagree with the first. B7 therefore holds by construction, and
`insert_bosses_verify.py` asserts it as a measurement rather than trusting it.

### All `supply` draws are taken in UP TO, even after the ceiling fills

§4 step 3 says "one `RandInt(0, |E| - 1)` per element of `E` in order". Read
literally, that keeps drawing after `k == cap`. The alternative — stop at the
ceiling — would make the number of draws depend on how early the ceiling filled,
and so on the seed, which makes the rest of the run's stream harder to reason
about. The literal reading is implemented and the model asserts the draw count.

### `REPLACED ENEMY'S AI` logs the think id the placement keeps

The per-placement log line prints the identity that was written. With
`keepEnemyAi` on, no `ThinkParamID` is written, so the line prints the
placement's existing think id rather than the drawn identity's — otherwise the
log would claim a write that did not happen.

### Two silent-skip progress lines, not one

§7 M3 step 6 asks for "the silent-skip line". An empty **INSERTED BOSSES** and
an empty **AREAS INCLUDED** both produce zero insertions for different reasons,
and a player who ticked the master switch needs to know which. Both lines are
statements of what the run did, in the same voice as every other progress line —
not warnings, which D-G forbids.

### The area table's display names come from each area's live map file

`MapNames.json` names `m23_00_00_00` "Old Yharnam (pre-DLC) [unused]" and
`m23_00_00_01` "Old Yharnam". The row is keyed on the prefix `m23`, so the name
is taken from the live file, which is also where the row's target count comes
from. `BossParamScaling.h` supplies which file that is, so the generator holds no
second list of live files.

---

## 4. Verification run

Every command below was run from the repository root, in this order, after the
last edit.

| Check | Command | Result |
| ----- | ------- | ------ |
| Build | `cd app && make` | `IV0000-BBRD00001_00-BBRANDOMIZERMAIN.pkg` built; **zero warnings, zero errors** across a forced rebuild of every touched translation unit |
| New tables fresh | `python app/tools/gen_pool_table.py insertboss data/vanilla/dvdroot_ps4 --check` | `PASS: InsertedBossTable.h matches the vanilla tree` |
| | `python app/tools/gen_pool_table.py area data/vanilla/dvdroot_ps4 --check` | `PASS: InsertAreaTable.h matches the vanilla tree` |
| No shipped table regressed | `python app/tools/gen_pool_table.py both data/vanilla/dvdroot_ps4 --check` | `PASS` for `EnemyPoolTable.h` and `BossPoolTable.h` |
| | `python app/tools/gen_pool_table.py skip data/vanilla/dvdroot_ps4 --check` | `PASS: EnemySkipTable.h matches the vanilla tree` |
| Settings chain | `python app/tools/pool_verify.py selftest data/vanilla/dvdroot_ps4` | **108/108**, including `worst-case defaults.cfg is 1089 bytes` and `the 1038-byte settings block fits FormatSettings' char buf[2048]`, plus one written-and-read case per new key |
| Settings UI | `python app/tools/settings_ui_verify.py` | **116/116**. `Bosses 10` rows; widest pane row **663/700 px (BOSS BEHAVIOUR)**; 34 live config keys round-trip |
| Pane scrolling | `python app/tools/ui_scroll_verify.py` | `all geometry and scroll properties PASSED` |
| Nothing else moved | `python app/tools/boss_verify.py selftest data/vanilla/dvdroot_ps4` | `selftest 5/5` |
| | `python app/tools/caged_dogs_verify.py selftest data/vanilla/dvdroot_ps4` | **23/23** |
| | `python app/tools/easy_modes_verify.py selftest data/vanilla/dvdroot_ps4` | **25/25** |
| | `python app/tools/enemy_lookup.py pool data/vanilla/dvdroot_ps4` | 333 entries / 82 models, unchanged |
| **The feature's rules** | `python app/tools/insert_bosses_verify.py selftest data/vanilla/dvdroot_ps4` | **56/56** |
| `verify`, exercised | `python app/tools/insert_bosses_verify.py verify data/vanilla/dvdroot_ps4 data/vanilla/dvdroot_ps4` | all seventeen criteria reported, all PASS, with the "nothing was inserted" note — the null case |
| **The feature on real output** | `insert_bosses_verify.py verify <vanilla> <hardware tree>` | **NOT RUN.** Needs a tree generated on the PS4 |
| Feature-off tree diff (§7 M3) | a run from one seed against the pre-M3 build | **NOT RUN.** Needs two PS4 runs |

The measurements worth reading out of `insert_bosses_verify.py selftest`, because
they are the plan's own numbers reproduced independently:

* eligible targets **1,381** over the fourteen live area files and **2,269** over
  all 24, with **zero** boss-named placements among them and zero from either
  `m21` file;
* all **26** caged-dog placements eligible, and all 26 outside the set once
  **DO NOT RANDOMIZE CAGED DOGS** is on; every `ENEMIES SKIPPED`-reachable
  placement outside it once its rows are ticked;
* the insertion list **32 identities across 22 models**, Moon Presence absent,
  set-identical over `kBaseMaps`' 24 files, the reference's 16 and
  `BossMapOrder()`'s 17;
* the fourteen prefixes partition the **22** area map files, no file matches two,
  neither `m21` file matches any;
* **EXACTLY** is an equality on `min(N, |E|)` in all fourteen areas at N = 1, 5
  and 15, and takes no draw to decide the count;
* **UP TO** never exceeds the ceiling over 200 seeds, and its mean is
  **identical** — 4.285 against 4.285 — with 32 identities ticked and with one,
  which is the D-M failure measured rather than argued;
* count 0, count 0 in EXACTLY, and no eligible target each insert nothing and
  consume **no draw**; and every gate in `ApplyBossInsertion` is read out of
  `BossInsertion.cpp` as preceding the first `RandInt`;
* use counts across a whole run at the maximum count differ by at most one (max
  7, min 6) and every enabled identity is used;
* spacing beats the uniform draw's own measured mean minimum distance in all
  fourteen areas at a count of 10, and at a count of 1 produces the same
  placement and the same draw count as spacing off;
* **25 of 32** identities tracked by the stat-scaling table and **seven** not —
  three Witch of Hemwick, three Living Failure and the Small Celestial Emissary.

`verify`'s own failure paths are exercised by eight injection cases inside the
same selftest: a simulated insertion is seen and not objected to; a replaced boss
placement raises 13 and 15; an insertion in the Hunter's Dream raises 14; a
fabricated `NPCParamID` raises 9; a moved placement raises 17; an unmet
`EXACTLY 1` raises 3; an unmet override raises 5.

---

## 5. What this does not prove

Nothing here establishes runtime behaviour. `CLAUDE.md` §3: the PS4 is the final
authority and this stage has no access to it. A clean build and green mirrors mean
**ready for hardware testing**.

Specifically still open:

* **Whether a loose boss works at all** — whether it aggros, attacks, dies
  without a fog gate, produces a health bar, music or a defeat message, and
  whether a save made in an affected area still loads.
* **Whether `REPLACED ENEMY'S AI` does anything sane.** A think id refers to
  behaviours that belong to a particular body. The inference is that a boss
  running a huntsman's script may stand inert, flail, or crash. Nothing in this
  milestone reduces that; it is contained only by OWN AI being the default and by
  the warning in the row's help text.
* **What `REPLACE ALL ENEMIES` does.** That is the question it exists to ask.
* **Criterion 1 — that an off run is byte-identical to today's.** What is proved
  statically is the property the diff would be testing: every gate precedes the
  first draw, asserted by reading `BossInsertion.cpp`, and the model shows every
  off path consuming zero draws. The tree diff itself needs two PS4 runs.
* **Criteria 2–18 on real output.** The rules are pinned; the C++ implementation
  of them is not. `insert_bosses_verify.py verify` is the thing that closes this
  and it needs a hardware tree.
* **The seven unscaled identities arriving at arena strength**, and whether that
  is survivable in the first area.

---

## 6. Hardware test handoff

Install the `.pkg`, then run these four in order, so a failure can be attributed.
Copy each output tree off the console if you can — `insert_bosses_verify.py
verify` turns each into seventeen machine-checked criteria.

**1. The default configuration.** Enable **BOSSES CAN REPLACE ENEMIES** and
nothing else new: **BOSS REPLACEMENTS** = `UP TO`, **BOSS REPLACEMENTS PER MAP**
= `1`, **SPREAD BOSSES OUT** off, **BOSS BEHAVIOUR** = `OWN AI`, every area and
every boss ticked. Build a world and play into Central Yharnam and one other
area.

* Expect **about nine** inserted bosses across a full run, not fourteen — see §3.
  The progress line says how many were placed; the text log names every one with
  its map and placement.
* Look for: does a loose boss aggro, attack and die with no fog gate? Is there a
  health bar, music, a defeat message? Does a save made in an affected area still
  load afterwards?
* Verify: `python app/tools/insert_bosses_verify.py verify data/vanilla/dvdroot_ps4 <tree>`

**2. The maximum count, both spacing states.** Count `15`, spacing off; then the
same seed with spacing on.

* Are two bosses four paces apart survivable? Does spacing visibly move them?
* Verify: `… verify data/vanilla/dvdroot_ps4 <tree> --count=15` and
  `… --count=15 --spread`.

**3. `BOSS BEHAVIOUR` = `REPLACED ENEMY'S AI`**, at the default count, on its
own. **This is the test most likely to fail, and a crash here is a plausible
non-defective outcome.** Verify with `… --keep-ai`.

**4. `REPLACE ALL ENEMIES`, last, once.** Expect a crash or an unplayable game.
**Record the outcome in `log.md` — that, and only that, unblocks M5.** Verify
with `… --replace-all`.

Also worth looking at across all four: the multi-phase bosses' transitions; the
seven unscaled identities (three Witch of Hemwick, three Living Failure, the
Small Celestial Emissary) arriving at arena strength in the first area; and the
three worst cases — a crash on entering an affected area, an unkillable inert
boss in a doorway the player must pass, and a boss whose death fails to fire an
event the area needs.

---

## 7. Stop point

**Completion gate, not a stop condition.** M4 finished and execution stopped at
the **Required gate before M5**, exactly as the Execution Strategy places it. No
§7 stop condition fired.

M5 — deleting **REPLACE ALL ENEMIES**, rebasing `pool_verify.py`'s block down by
22 bytes and dropping the override's criterion from the verifier — does not begin
until the developer has recorded the hardware result of test 4 above in
`log.md`.

Two things noticed and deliberately not fixed, both out of scope by §3.2:

* `docs/plans/pickers.md` §7.5 and `docs/features/032-bypassed-enemies/spec.md`
  §8 still say this feature puts bosses into the ordinary enemy pool. It does
  not, and the port follows the reference here. Reported only — the documentation
  stage owns them, along with spec §4/§5's 29-identity figures and row 13's own
  backlog description.
* `settings_ui_verify.py` case 9 measured the picker band against three of the
  five shipped tables; the two weapon tables were already absent. The two new
  tables were added to that list, and the pre-existing gap is left as it is.
