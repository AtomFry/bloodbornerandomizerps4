# Implementation Report 018 — Easy Shadows, Easy Rom, Easy Failures, Easy Emissary — milestone 1

**Status: AWAITING HARDWARE TEST**

**Plan:** `docs/features/018-easy-shadows/plan.md` — milestone 1

**Spec:** `docs/features/018-easy-shadows/spec.md`

**Implemented:** 2026-09-19

---

## 1. What was built

The whole of milestone 1: a new table-driven MSB pass (`EasyModes.h` / `.cpp`)
that replaces the duplicate bodies in four boss arenas with the Iosefka's
Clinic larva, four independent settings carried through `RandomizerDefaults`,
`defaults.cfg`, both UI screens and `EnemyRandomizerOptions`, four per-setting
result lines, and a new Python mirror that parses the table and the identity
out of the header. The pass draws no randomness and is called once per map from
inside `StepWriteMap`, between the enemy loop's closing brace and the
`BossScalingMaps()` loop. `.pkg` built from a clean tree; every automated check
in plan §6 passes.

| # | Planned change | Status | Where |
| - | -------------- | ------ | ----- |
| 1 | `EasyModes.h` — constants, structs, six-row table, both header notes | done | `app/src/Randomizer/EasyModes.h` (188 lines) — table at `:124-151`, constants at `:79-81`; `const char* const` on the model name, see §2 |
| 2 | `EasyModes.cpp` — the pass of §4.2 | done | `app/src/Randomizer/EasyModes.cpp` (132 lines). No `RandInt`, no `<random>`, three fields written per hit |
| 3 | `EnemyRandomizer.h` / `.cpp` — option, result counts, include, the call | done | option `EnemyRandomizer.h:196`, counts `:80`; include `EnemyRandomizer.cpp:72`, call `:870`, sitting between `} // if (options.randomizeEnemies)` (`:866`) and the `BossScalingMapEntry` loop (`:872`); header bullet `:57-71` |
| 4 | `RandomizerDefaults.h` + `RandomizerDefaultsStore.cpp` — four fields, four load branches, four save arguments | done | `RandomizerDefaults.h:155-158`; load `RandomizerDefaultsStore.cpp:89-100`, save format `:131-132`, arguments `:150-153`. `char buf[1024]` and the 4096-byte load buffer unchanged |
| 5 | `SetupDefaultsScreen.h` / `.cpp` — four row constants appended last, `kItemCount` 21, four `ToggleRow` branches, four `items` entries | done | constants `SetupDefaultsScreen.h:89-92`, `kItemCount` `:36`; toggles `SetupDefaultsScreen.cpp:129-144`; items `:278-285`. No existing row constant changed |
| 6 | `EnableWizardScreen.h` / `.cpp` — members, constants, `kSaveDataRowCount` 21, ctor init, toggles, both list vectors, option assignments, the `\|\|`, the four result lines | done | members `EnableWizardScreen.h:118-125`; constants `.cpp:65-69`; ctor `:140-143`; left/right `:269-287`; X `:360-378`; `\|\|` `:626-630`; options `:657-664`; result lines `:748-771`; `DrawSaveData` items `:874-881`, `DrawConfirm` items `:986-993` (identical in shape) |
| 7 | `app/tools/easy_modes_verify.py` — `show` / `verify` / `selftest` | done | new, 661 lines. `selftest` runs 25 assertions covering all twelve §6 cases; 25/25 pass |
| 8 | `boss_verify.py --easy`, `ui_scroll_verify.py` counts, `pool_verify.py` worst case | done | `boss_verify.py:421-440` (docstring + parameter), `:492-496` (suppression), `:559-576` (`easy_mode_targets`, the printout), `:713-714` (argv); `ui_scroll_verify.py:49-61`; `pool_verify.py:688-707` |
| 9 | Clean rebuild and `.pkg` | done | `rm -rf src/x64 && make` → `IV0000-BBRD00001_00-BBRANDOMIZERMAIN.pkg`, no warnings |

The §3.1 invariants were checked individually before calling this done: no
`RandInt` or `<random>` in the new code; the call site is in the one slot §4.3
names; exactly three fields written and only on matched placements; the table
holds six rows and `m27_00_00_00` is not one of them (asserted, selftest case
2); every UI row appended last with no existing index moved; `StepMergeModels`
untouched and still unconditional; `defaults.cfg` still ignores unknown keys
and reads an absent key as `false`; nothing changed in `BossRandomizer.cpp`,
`EnemyExclusionList.h`, `EnemyPoolTable.h`, `EnemySkipTable.h`, `BossList.h`
or any param table; the pass lives under `app/src/Randomizer/`, depends only on
`Msb` and `Platform/Log.h` (as four sibling passes already do) and no UI file
learned a map or placement name.

---

## 2. Deviations from the plan

Four, all recorded in plan §10. None changes what the feature does.

**D1 — `kEasyModeModelName` is `const char* const`, not §4.4's `const char*`.**
A non-const pointer at namespace scope has external linkage in C++, so the
plain form is a duplicate symbol the moment a second translation unit includes
the header — and `EnemyRandomizer.cpp` includes it. `const char* const` gives
the constant internal linkage and is the spelling the port already uses for a
named string constant (`EnableWizardScreen.cpp:85`'s `kEnemyFailPrefix`). Name
and value unchanged; the header records the reason inline.

**D2 — `easy_modes_verify.py verify` accepts an extra `--no-randomizers`
argument.** §6's settings-off bullet is conditional: those placements are
frozen "when the run had every randomizer off", and otherwise differ "as the
enemy and boss passes allow". The flag is how the caller states which of the
two trees it has. Without it, the survivor and settings-off checks print as
notes rather than failures, so the tool never passes judgement on the enemy
pass — which is `boss_verify.py`'s and `pool_verify.py`'s job, not this one's.
The target checks and the B12 param checks are asserted either way.

**D3 — one stale figure updated in a comment.**
`RandomizerDefaultsStore.cpp`'s buffer comment said "Worst case today is 585
bytes"; it was already wrong (611) before this feature and these four keys
would have made it wronger. Changed to 669, directly above the `snprintf` this
milestone extends. The buffer itself is unchanged, as §5 requires.

**D4 — `boss_verify.py --easy` imports the table rather than re-parsing it.**
§5 asks the flag to suppress "exactly the (map, name) pairs the table selects".
Two parsers of `EasyModes.h` on the Python side is precisely the drift §3.3's
first hazard warns about, so `boss_verify.easy_mode_targets` does a deferred
`from easy_modes_verify import parse_table, matched_in`. The import is inside
the function because `easy_modes_verify` imports `boss_verify` at load time.

---

## 3. Decisions the plan left open

### 3.1 The row's flag is an enum, and the pass resolves options and counters through two small switches

§4.2 names the structs and the signature but not how a table row says which of
the four settings owns it. Options were a pointer-to-member pair
(`bool EasyModeOptions::*`), an index, or an enum. Taken: an
`enum class EasyModeFlag` in the row plus `FlagEnabled` / `CounterFor` in the
`.cpp`'s anonymous namespace. It is the most readable in a table meant to be
eyeballed, it is trivially parseable by the mirror (`EasyModeFlag::kShadows` →
`shadows`), and member pointers appear nowhere else in this port.

### 3.2 The pass walks the parts once per enabled row naming the map, not once per map

Each map appears in exactly one row today, so both shapes do one pass. The
per-row shape was chosen because it mirrors the reference's own structure (one
branch, one name list, one map) and because a future second row on one map
would then work without a rewrite. The map-applies test and the `c2521` lookup
still happen once, before the row loop, in the order §4.2 lists.

### 3.3 `EasyModeOptions` carries an `Any()` helper

Not in §4.2. Added so §4.2 step 1's "return immediately if no flag is set" is
one call at the top of the pass rather than a four-term disjunction, matching
how `EnemyRandomizerOptions::AnyParamFeature()` already reads.

### 3.4 The per-replacement log line reuses the enemy loop's shape

§4.2 step 3 says "log one line in the existing `map name: old -> new` shape".
Taken literally: the same `map name placement: oldNpc*oldThink*oldModel ->
newNpc*newThink*newModel` format `StepWriteMap`'s enemy loop emits, so an easy
replacement reads out of the log next to the roll it overwrote.

### 3.5 `easy_modes_verify.py`'s "retail-loaded" total is derived, not listed

Case 1 needs the 42. The four live variants are exactly the ones
`BossParamScaling` names for their zone, so the tool computes the total over
rows where `zone_scaled_npc(map, 252100) != 252100` rather than hard-coding a
map list. `names.py`'s `unused` tag could not be used: it marks
`m24_02_00_00` but not `m32_00_00_00` (plan-evidence E4 M17).

### 3.6 Selftest case 12 builds its synthetic tree with a fake Models entry

`c2521` is declared in five maps and none of them is one this feature writes —
`StepMergeModels` is what puts it there on a real run. Reproducing the merge is
not the mirror's job, and the checker only needs a name at an index, so
`_fake_model_entry` appends a minimal 16-byte model blob. The synthetic tree
exists solely to prove the checker catches things; it is never written to disk.

### 3.7 `check_params` treats a missing output archive as B12 satisfied

An output tree only holds `gameparam.parambnd.dcx` if some param feature ran.
When it is absent the tool prints a note saying B12 holds trivially rather than
failing — an easy-only run legitimately produces no archive.

---

## 4. Verification run

| Check | Command | Result |
| ----- | ------- | ------ |
| Clean build | `cd app && rm -rf src/x64 && make` | `IV0000-BBRD00001_00-BBRANDOMIZERMAIN.pkg` built, 7,143,424 bytes. No compiler warnings or errors from any file |
| New mirror, rules | `python tools/easy_modes_verify.py selftest ../data/vanilla/dvdroot_ps4` | **25/25 passing** — all twelve §6 cases. Measured: 79 placements tree-wide, 42 retail-loaded; `m27_00_00_00` out of the table and holding 212700/212710/212720; larva 2 HP / 18 echoes / team 26 / lot 28040 with all 31 variants identical; written values 900014609 / 900014611 / 900014614 / 900014627; lot 28040 the only source of item 4321 with 32 rows referencing it; `IsExcludedNpcRow` exactly {252100, 6071}; the four reference name lists verbatim |
| New mirror, table | `python tools/easy_modes_verify.py show ../data/vanilla/dvdroot_ps4` | 2 / 30 / 30 / 3 / 7 / 7 = 79, 42 retail-loaded — matching §4.1 exactly |
| UI geometry | `python tools/ui_scroll_verify.py` | PASSED — 21 rows on all three settings screens, 20-line progress budget, every scroll property |
| Picker + config | `python tools/pool_verify.py selftest ../data/vanilla/dvdroot_ps4` | **87/87 passing**, including "worst-case defaults.cfg is 669 bytes and fits char buf[1024]" |
| Boss rules | `python tools/boss_verify.py selftest ../data/vanilla/dvdroot_ps4` | **5/5 passing** — unchanged, still catches all four corruptions |
| Drops | `python tools/drops_verify.py selftest ../data/vanilla/dvdroot_ps4` | **6/6 passing** — D-I4 still holds |
| `--easy` behaviour | in-process: apply the mirror of the pass to the boss maps, then run `compare_trees` both ways | without `--easy`: 42 failures, 37 of them `V2 non-boss placement changed` — exactly plan-evidence M14. With `--easy`: 0 failures, once a real scaling table is supplied. See the caveat below |
| `verify` smoke test | `python tools/easy_modes_verify.py verify ../data/vanilla/dvdroot_ps4 "../data/runs/20260919-Enemies Only/dvdroot_ps4"` | PASS — 0 placements selected with all four settings off, and both B12 param rows byte-identical. Exercises the command path against a real output tree, but it is not the §6 step-1 check |

**Not run, and why.**

* `easy_modes_verify.py verify <V> <B> shadows rom failures emissary` and
  `boss_verify.py verify <V> <B> --easy` against a real easy tree. Both need an
  output tree produced by the PS4, which does not exist yet. They are hardware
  step 1.
* Every §6 Hardware row. No console access (`CLAUDE.md` §3).

**One thing the developer needs to know before hardware step 1.**
`boss_verify.py`'s `load_scaling_ids` reads
`<repo_root>/PS4/bbrandomizer/src/Randomizer/NpcScalingTable.h`, a path that has
not existed since the tree was reorganised into `app/`. It fails silently and
returns an **empty** scaling table — visible in `verify`'s own banner
("scaling table: 0 ids") and in the selftest line "scaling-only npc rewrite
SKIPPED (no scaling table)". On an easy tree that costs five spurious failures:
the boss-named easy targets (`c2120_0001`, `c2120_0002`, `c4030_0001/2/3`) get
their *scaled* larva identity — 900014609 and 900014627 — reported as "in
neither vanilla data nor the scaling table". With the real table loaded from
`app/src/Randomizer/NpcScalingTable.h` (26,506 ids), the same comparison
returns **0 failures**, which is what plan-evidence §E5.4 predicts. This is a
pre-existing defect that predates and is unrelated to this feature, it affects
every `boss_verify verify` run rather than only easy trees, and fixing it is
outside this milestone's approved scope — so it was deliberately left alone and
is reported here instead.

---

## 5. What this does not prove

Nothing here establishes runtime behaviour. There is no host C++ compiler and
no emulator, so a clean cross-compile proves the code compiles and links, and
the Python mirrors prove the **rules and the data** are right — not that
`ApplyEasyModes` implements them. The two can drift and only the console would
notice (`plan-evidence` §E5.10).

Specifically still open, and only hardware can close them:

* **Do the four fights end?** The pass leaves EntityID, part name and position
  intact and the scripts track each body by entity ID, but whether the
  completion condition is "these entities are dead" or something tied to the
  creature's own death behaviour is not knowable from map data (§E5.1).
* **Is the larva harmless?** 2 HP and team type 26 are bytes in a param row,
  not observed behaviour. Whether the larvae attack, and whether they die in
  one hit, is a hardware question (§E5.7, and the standing note on setting
  polarity).
* **Does Rom spawn children dynamically?** If she does, Easy Rom thins her
  fight rather than emptying it — the same as in the reference (§E5.1).
* **Where does One Third of Umbilical Cord end up?** The expected outcome is one
  cord in total, because lot 28040 carries a single `getItemFlagId`. A cord per
  larva is the other reading and would send spec §10 D2 back for review (§E5.5).
* **Do the maps load at all?** The model declaration is guaranteed by
  `StepMergeModels`, which was not changed, but a map that fails to load is the
  failure mode that would show it otherwise.

---

## 6. Hardware test handoff

Three trees are needed: vanilla **V**; seed **S** with all four settings
**off** → **A**; the same seed **S** with all four **on** → **B**. Vanilla as
the third input is mandatory (`CLAUDE.md` §3).

1. **Install** the freshly built `.pkg` from `app/`. This is a clean rebuild —
   three structs gained members, which is the case `docs/build.md` warns can
   produce a heap-corruption `SIGSEGV` from a stale object file.
2. **Enable the four new rows.** They are the last four on both Setup Defaults
   and the Enable wizard's Save Data screen: `EASY SHADOWS`, `EASY ROM`,
   `EASY FAILURES`, `EASY EMISSARY`. Left/right or X toggles each.
3. **Run the checks on the trees**, once **B** is off the console:
   ```
   cd app/tools
   python easy_modes_verify.py verify <V> <B> shadows rom failures emissary
   python boss_verify.py verify <V> <B> --easy
   ```
   Both should PASS, and the console's own progress lines should have read
   `EASY SHADOWS REPLACED 2 PLACEMENTS`, `EASY ROM REPLACED 60 PLACEMENTS`,
   `EASY FAILURES REPLACED 3 PLACEMENTS`, `EASY EMISSARY REPLACED 14
   PLACEMENTS`. A wrong number means a pattern list or a map name is wrong.
   Expect the five spurious `boss_verify` failures described in §4 until
   `load_scaling_ids`' path is fixed.
4. **Play B, Shadows of Yharnam.** One Shadow fights; two small creatures stand
   in for the others. *Do they attack? Do they die in one hit? Does the fight
   end after the third body dies, and does the fog lift?*
5. **Play B, Rom.** Every child should be a larva, and her fight should still
   progress through her teleports and end normally. Note whether any *real*
   spider appears — that answers the dynamic-spawn question.
6. **Play B, Living Failures and Celestial Emissary.** One opponent each, both
   fights end normally, and the Emissary still grows out of the surviving small
   body.
7. **Play B with `RANDOMIZE ENEMY DROPS` off**, kill the first easy-mode larva
   you reach, then visit Iosefka's Clinic. Expect **one** One Third of
   Umbilical Cord in total. Drops must be off or the result means nothing: with
   drops on, the four scaled larva rows are re-assigned and a missing cord
   proves nothing (§E5.5).
8. **Play A (all four off).** The four fights should be exactly as they were.
   This is what makes steps 4–7 mean anything.
9. **Run with only `EASY SHADOWS` on and every randomizer off.** The run should
   start and complete, and only `m27_00_00_01` should differ. Verify with
   `python easy_modes_verify.py verify <V> <tree> shadows --no-randomizers`.
10. **Reopen Setup Defaults after saving.** The four new rows should persist,
    and `ENEMIES INCLUDED` should still read `82 OF 82` — a changed count would
    mean the config string was misread when the four keys were added.

**Failure looks like:** a fight that never ends; a replacement that is hostile
or tough enough to matter; an easy setting losing to boss randomization (a
duplicate body that is not a larva); a map that fails to load at all; or a
wrong count on a progress line.

---

## 7. Stop point

Milestone 1's **completion gate**, reached rather than a stop condition: every
change in §7 is built, every automated check in §6 passes, and the `.pkg`
builds from a clean tree. No stop condition fired.

The milestone is handed to the developer for the §6 Hardware steps. Plan §7
has no milestone 2 — feature 018 is one milestone — so the next step after a
green hardware run is stage F (code review) and then the documentation stage,
not further implementation. Nothing further was started.
