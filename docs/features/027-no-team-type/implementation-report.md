# Implementation Report 027 — No Team Type (`ENEMIES HOSTILE TO EACH OTHER`) — milestones 1 and 2

**Status: AWAITING HARDWARE TEST**

**Plan:** `docs/features/027-no-team-type/plan.md` — milestones 1 and 2, built
continuously as the Execution Strategy specifies (2 milestones, continuous, final
gate only).

**Spec:** `docs/features/027-no-team-type/spec.md`

**Implemented:** 2026-09-28

---

## 1. What was built

One on/off setting, off by default, that writes the fixed value 25 into
`NpcParam.teamType` — byte 303 of each 388-byte row — in all 31,398 rows with no
exclusion of any kind, plus the settings chain that lets a player turn it on and
the verification that can tell a correct output tree from a nearly-correct one.
The pass draws no randomness (its signature takes no `std::mt19937`), touches no
byte outside `teamType`, and with the setting off neither runs nor loads the
archive for its own sake. `DropRandomizer.cpp` was not edited; its verifier was
corrected, because `drops_verify.py`'s D-I2 checked only bytes 0–43 and an empty
slice and therefore passed a tree in which every `teamType` had been rewritten.

### Milestone 1

| # | Planned change | Status | Where |
| - | -------------- | ------ | ----- |
| 1 | `TeamType.{h,cpp}` per §4.1 | done, with one addition — see §3.1 | `app/src/Randomizer/TeamType.h` (63 lines), `app/src/Randomizer/TeamType.cpp` (62 lines) |
| 2 | `noTeamType` option after `enableMergoDarkness`, in `AnyParamFeature()`; `teamTypeRowsWritten` result counter | done | `app/src/Randomizer/EnemyRandomizer.h:240` (option), `:160` (`AnyParamFeature`), `:82-87` (counter) |
| 3 | The pass call in `StepItemData` at §4.2's position | done | `app/src/Randomizer/EnemyRandomizer.cpp:1075-1089`, after the starting-weapon / shop block and before the `startWithHunterTools` block; `#include "TeamType.h"` at `:94` |
| 4 | `drops_verify.py`'s D-I2 correction, `--team-type`, D-S1–D-S3 | done | `app/tools/drops_verify.py` — new `ROW_BYTES`/`TEAM_TYPE_*` constants, `compare(..., team_type=False)`, six new selftest cases |
| 5 | `team_type_verify.py` with `census`, `verify`, `selftest` | done, S0–S10 plus two source-pinning cases (§3.3) | `app/tools/team_type_verify.py` (443 lines) |

### Milestone 2

| # | Planned change | Status | Where |
| - | -------------- | ------ | ----- |
| 1 | `RandomizerDefaults` field | done | `app/src/Randomizer/RandomizerDefaults.h:131`, after `enableMergoDarkness` |
| 2 | `no_team_type` in both halves of the store | done | `app/src/Randomizer/RandomizerDefaultsStore.cpp:40` (`FormatSettings`), `:131-138` (`ApplySettingKey`), both after `enable_mergo_darkness` |
| 3 | `SettingId::NoTeamType` and the `kSettings` entry | done, label and help verbatim from §4.3 | `app/src/UI/SettingsModel.h:105`, `app/src/UI/SettingsModel.cpp:124-128` |
| 4 | Settings-UI spec §7.1 and Appendix A, and `PROSE_TO_LABEL` | done — `No Team Type` moved out of the **Enemies** backlog cell into the **World** current cell, after `Enable Mergo Darkness`; one Appendix A row | `docs/features/randomizer-settings-ui/spec.md` §7.1 and Appendix A (World), `app/tools/settings_ui_verify.py` `PROSE_TO_LABEL` |
| 5 | `WorldActivation.cpp`'s mapping line and `anythingOn` term, and `worlds_verify.py`'s two pinned lists | done, in the pinned position after `enableMergoDarkness` | `app/src/Game/WorldActivation.cpp:772`, `:796`; `app/tools/worlds_verify.py` `WIZARD_OPTIONS_MAPPING`, `WIZARD_RUN_DECISION` |
| 6 | `WorldEditorScreen.cpp`'s report line and the `ITEM DATA` condition | done, no `SKIPPING` line added | `app/src/UI/WorldEditorScreen.cpp:1300-1303`, `:1381` |
| 7 | `pool_verify.py`'s 743 / 794 arithmetic and the key case | done | `app/tools/pool_verify.py` — settings block 728 → 743, worst case 779 → 794, new `027: no_team_type is in both load and save` case |

Feature 011's `ShopStock` block has **not** landed (no `app/` changes for it in
the tree), so §4.2's "after the shop block if 011 landed" and §3.3's 775 / 826
rebase did not apply: the byte arithmetic is the plain 743 / 794.

### Files changed

| File | Change |
| ---- | ------ |
| `app/src/Randomizer/TeamType.h` | new — `TeamTypeResult`, `ApplyOneTeamType`, paramdef and reference citations, and the unverified-behaviour warning |
| `app/src/Randomizer/TeamType.cpp` | new — `kTeamTypeOffset = 303`, `kOneTeamTypeValue = (uint8_t)25`, the unconditional per-row store, one `Log` line with both counts |
| `app/src/Randomizer/EnemyRandomizer.h` | `noTeamType` option, `AnyParamFeature()` term, `teamTypeRowsWritten` counter |
| `app/src/Randomizer/EnemyRandomizer.cpp` | `#include "TeamType.h"`; the pass block in `StepItemData` step 1 |
| `app/src/Randomizer/RandomizerDefaults.h` | `bool noTeamType = false;` |
| `app/src/Randomizer/RandomizerDefaultsStore.cpp` | `no_team_type` written and read |
| `app/src/UI/SettingsModel.h` | `SettingId::NoTeamType` |
| `app/src/UI/SettingsModel.cpp` | one `kSettings` entry — `ENEMIES HOSTILE TO EACH OTHER`, category `World`, kind `Toggle` |
| `app/src/Game/WorldActivation.cpp` | `options.noTeamType = run.noTeamType;` and the `anythingOn` term |
| `app/src/UI/WorldEditorScreen.cpp` | the `SET n CREATURE RECORDS TO ONE ALLEGIANCE` line and the `ITEM DATA` condition |
| `app/tools/team_type_verify.py` | new — `census`, `verify` (`--off`, `--drops`), `selftest` |
| `app/tools/drops_verify.py` | D-I2 corrected to the whole 388-byte row outside `itemLotId_1`; `--team-type` declaration flag; D-S1–D-S3 |
| `app/tools/pool_verify.py` | 743 / 794 arithmetic and the new key case |
| `app/tools/settings_ui_verify.py` | `PROSE_TO_LABEL` gains `"No Team Type": "ENEMIES HOSTILE TO EACH OTHER"` |
| `app/tools/worlds_verify.py` | the field in both pinned lists |
| `docs/features/randomizer-settings-ui/spec.md` | §7.1 category move, one Appendix A row |
| `docs/features/027-no-team-type/plan.md` | **§10 only** |

Not touched: `DropRandomizer.{h,cpp}` (not one line), the Makefile, both
settings screens (they render any `Toggle` generically), `EnemyRandomizerJob::StatusText`,
`docs/randomization-feature-spec.md`, `docs/user-guide.md`, `reference/`.

---

## 2. Deviations from the plan

**One, and it is a verification addition rather than a behavioural change.**

1. **`worlds_verify.py selftest` does not run the checks §6 credits it with.**
   §6 and M2's verification list both name `worlds_verify.py selftest` as the
   check that the two pinned lists carry the field. That mode runs only
   `rule_cases()` (manifest rules, 29 cases); `WIZARD_OPTIONS_MAPPING` and
   `WIZARD_RUN_DECISION` are asserted in `source_cases()`, which only the `all`
   mode runs. Both modes were run and both pass — `selftest` 29/29 and `all`
   97/97 — so the plan's intent is satisfied, but a reviewer reading §6 alone
   would not have seen the pinned lists checked. Recorded in plan §10.

Nothing else in §1–§7 was implemented differently, and nothing was left out. No
row is excluded, no value other than 25 is written, the pass takes no RNG, and
`DropRandomizer.cpp` is untouched.

---

## 3. Decisions the plan left open

### 3.1 A bounds check on the field, not just on the row start

`ParseParamRows` only proves a row *starts* inside the member
(`app/src/Param/ParamBnd.cpp:126`), so a store at `dataOffset + 303` is
unchecked. `GrantHunterTools` handles the same problem by validating the row's
full extent before touching it. Options were: write unchecked (smallest code,
undefined behaviour on a malformed archive), skip the row (forbidden — that is
exactly the `continue` §4.1 rules out and the exclusion §3.2 forbids), or fail
the run. **Taken: fail the run**, matching `HunterTools.cpp`'s wording and the
documented contract "returns false only on a structural problem". The loop still
visits every row, so §3.1's "no row is excluded" holds: the check cannot skip a
row, only end the run. On the real vanilla archive it never fires.

### 3.2 Where the progress line sits, and what it says

§4.3 fixes the text (`"SET " + rowsWritten + " CREATURE RECORDS TO ONE
ALLEGIANCE"`) but not its position among the other lines. **Taken:** immediately
after the `MERGO DARKNESS ENABLED` line, which is where `noTeamType` sits in the
options struct, the config file, the settings model and the `anythingOn` chain —
so all five orderings read the same. No `SKIPPING` counterpart, per §4.3.

### 3.3 Two selftest cases beyond S0–S10

`team_type_verify.py`'s selftest also asserts that `TeamType.cpp` declares
`kTeamTypeOffset = 303` and `kOneTeamTypeValue = (uint8_t)25` (the shape
`hunter_tools_verify.py` already uses to keep the C++ and the paramdef from
drifting apart), and that its row loop contains no `continue` and no `row.id ==`
test. The second mechanises §4.1's "a `continue` anywhere in that loop is this
plan being implemented wrongly", which was otherwise only reviewable by eye. Both
are in the file §5 lists, and both pass. Recorded in plan §10.

### 3.4 `--drops` tolerance extends to T-I5

T-I5 says the 378 rows already holding 25 must be byte-identical to vanilla. On a
combined run the drop pass can reassign `itemLotId_1` in one of those rows, so
under `--drops` those four bytes are excluded from T-I5 as well as from T-I2.
Without that, S10 (a combined tree must pass with `--drops`) could not pass on
any real run. `drops_verify.py` still owns whether the value written there is
legal.

### 3.5 The spec's Appendix A row says more than the model's help text

§4.3 requires "one Appendix A row carrying the help text above". The row carries
that text verbatim and then two sentences of context the shipped help cannot fit:
that the label is `ENEMIES HOSTILE TO EACH OTHER` while the prose name follows
the `Protect Caged Dogs` precedent, and that the setting is not a randomizer.
Neither asserts an in-game effect, so B8 holds. `settings_ui_verify.py` does not
compare Appendix A prose to the model's help string.

---

## 4. Verification run

Every command below was run from the repository root on 2026-09-28, in this
order, after the milestone it belongs to.

### After milestone 1

| Check | Command | Result |
| ----- | ------- | ------ |
| Build | `cd app && make` | `IV0000-BBRD00001_00-BBRANDOMIZERMAIN.pkg` built, 7,143,424 bytes, compile and link clean |
| The new mirror | `python app/tools/team_type_verify.py selftest data/vanilla/dvdroot_ps4` | **selftest 20/20** — S0–S10 (18 cases) plus the two source-pinning cases |
| Drops mirror | `python app/tools/drops_verify.py selftest data/vanilla/dvdroot_ps4` | **selftest 12/12** — the six existing cases unchanged, plus D-S1 (2), D-S2 (2), D-S3 (2) |
| Archive round trip | `python app/tools/itemdata_verify.py roundtrip data/vanilla/dvdroot_ps4` | `PASS round trip byte-identical (28309992 bytes back)`, `PASS archive still lists the same 65 entries` |

### After milestone 2

| Check | Command | Result |
| ----- | ------- | ------ |
| Clean build | `cd app && make clean && make` | `.pkg` rebuilt from scratch, 7,143,424 bytes, no warnings or errors |
| Config | `python app/tools/pool_verify.py selftest data/vanilla/dvdroot_ps4` | **96/96 passing**, including `worst-case defaults.cfg is 794 bytes`, `the 743-byte settings block fits char buf[1024]`, `027: no_team_type is in both load and save` |
| Settings model | `python app/tools/settings_ui_verify.py` | **113/113 passing**. `World 2 ENABLE MERGO DARKNESS, ENEMIES HOSTILE TO EACH OTHER`; `widest pane row 651/700 px`; `worst help body 7/11 lines` — exactly §4.3's predicted 651 px and 7 lines |
| Worlds parity | `python app/tools/worlds_verify.py selftest` | **29/29 passing** (manifest rules only — see §2) |
| Worlds parity, pinned lists | `python app/tools/worlds_verify.py all` | **all checks passing** — 29/29 manifest, 54/54 world store, 54/54 activation, 97/97 source invariants, including `the options mapping is field for field the wizard's (21 fields)` and `the run decision is the wizard's \|\| chain, field for field (16)` |
| M1's checks, still green | `team_type_verify.py selftest`, `drops_verify.py selftest`, `itemdata_verify.py roundtrip` | 20/20, 12/12, both `PASS` |

### Also run, not required by the plan

| Check | Command | Result |
| ----- | ------- | ------ |
| Vanilla census (§6, not a gate) | `python app/tools/team_type_verify.py census data/vanilla/dvdroot_ps4` | 31,398 rows, stride `[388]`, `teamType` at 303, 13 distinct values, **378 already at 25**, 31,020 to change — `matches what plan 027 was measured against: yes`. No stop condition fired |
| Neighbouring `NpcParam` / `CharaInitParam` mirrors | `hunter_tools_verify.py selftest`, `easy_modes_verify.py selftest` | 25/25, 25/25 — unchanged |

### Not run, and why

* **`team_type_verify.py verify <vanilla> <output>`** and **`verify … --off`**,
  and **`drops_verify.py verify … --team-type`**. All three need an output tree
  from a real run, which only the PS4 produces. They are the first thing to run
  once the two saved runs land in `data/runs/` (§6).
* **Anything about in-game behaviour.** See §5.

---

## 5. What this does not prove

A clean build and green mirrors mean **ready for hardware testing** and nothing
stronger (`CLAUDE.md` §3). Specifically:

* **Nothing here establishes what team type 25 does in Bloodborne.** That is the
  whole feature. The mirrors show that an output tree carries the byte the
  reference writes, in every row and nowhere else. They say nothing about
  hostility, aggro, factions or dialogue. 378 rows already hold 25 in vanilla and
  are not known to be hostile to anything. `docs/known-traps.md`: byte decoding
  does not prove game behaviour.
* **The label `ENEMIES HOSTILE TO EACH OTHER` is unproven.** Spec D3 accepted
  that risk knowingly; §6's hardware verdict is the label question.
* **No output tree has been produced by the C++ at all.** The selftests simulate
  output in Python; they cannot show the engine's loop is what put the byte
  there. `team_type_verify.py verify` against a real run is what closes that gap.
* **The Hunter's Dream is written with no protection**, by design (spec D2). That
  it does not break is a hypothesis, not a result.
* **The easy-mode interaction is unmeasured.** The larva `NpcParam` 252100 and its
  31 scaled variants carry `teamType` 26 in vanilla and 25 after this pass, and
  feature 018 rests on "26 means non-hostile". Plan D1 chose to test the
  combination rather than exempt the rows; that test has not been run.
* **Chalice creatures are reached and untested**, as §3.2 states.

---

## 6. Hardware test handoff

Install the rebuilt `app/IV0000-BBRD00001_00-BBRANDOMIZERMAIN.pkg`. This is
**discovery, not confirmation**: a null observation means the game does not act
on the field the way the label claims, not that the pass failed — the offline
checks have already proved the bytes are in the tree.

**Run A — the setting on, nothing else on.**

1. Turn `ENEMIES HOSTILE TO EACH OTHER` on in `WORLD` (it sits directly under
   `ENABLE MERGO DARKNESS`, reading `NO` by default). Activate the world.
2. Confirm the progress log prints **`SET 31398 CREATURE RECORDS TO ONE
   ALLEGIANCE`** and an `ITEM DATA … ENTRIES, WROTE … MB` line. A 0 or any other
   figure means the row walk is wrong — stop there and report it.
3. **The Hunter's Dream is intact.** This is first and it is the run-ending
   failure case, because spec D2 took no protection and a new character spawns
   there. The Doll can be spoken to and does not attack; Gehrman can be spoken to
   and does not attack; the Messengers behave normally; levelling up, the shop and
   travel all work.
4. **Whether enemies fight each other.** Somewhere densely populated with mixed
   creature types — Central Yharnam's mob, the Forbidden Woods, Yahar'gul. Watch
   for creatures attacking one another unprompted or taking damage from one
   another's attacks.
5. **Whether quest NPCs still work.** A window NPC conversation, plus at least one
   of Master Willem, the kneeling Lady Maria or Ludwig's head.
6. **Whether cooperating adds still behave.** The Witch of Hemwick and her Mad
   Ones is the sharpest case: those two sit on different allegiances in vanilla
   and end on the same one.
7. **Whether anything changes at all.** "No observable difference" is a real and
   likely result and is worth recording precisely.
8. **The easy-mode combination, on this same run.** Turn `EASY SHADOWS` on
   alongside and reach the Shadows of Yharnam. Record whether the two substituted
   larvae still stand inert. A failure here is a **finding, not a defect**: it
   would be the first evidence that this field decides hostility at all, and it is
   what would justify a one-row exemption that nothing justifies today.

**Run B — the setting off**, to confirm the game is the one it was.

**Reading the result** (plan §6):

| Observation | Reading | Consequence |
| ----------- | ------- | ----------- |
| Creatures attack or damage one another unprompted, where they did not before | the label is true | ship it; the documentation stage corrects the spec's §4 finding |
| The Hunter's Dream or the game's talkers turn hostile or stop talking, but enemies still ignore each other | the write does something, but not what the label says | **stop and report** — label and help both need revising, and the smallest exemption becomes justified work |
| Nothing changes anywhere | the write is inert as far as play is concerned | report it; the label is false and must be revised, and `docs/windows-randomizer-technical-review.md` §13 question 5 is answered |

**Afterwards**, save each run's output tree and `live.log` into `data/runs/` (do
not commit them — `CLAUDE.md` §9) and run:

```
python app/tools/team_type_verify.py verify data/vanilla/dvdroot_ps4 <run A tree>
python app/tools/team_type_verify.py verify data/vanilla/dvdroot_ps4 <run B tree> --off
```

Add `--drops` to either if `RANDOMIZE ENEMY DROPS` was also on, and then also run
`python app/tools/drops_verify.py verify data/vanilla/dvdroot_ps4 <tree> --team-type`.

---

## 7. Stop point

**Both milestones completed.** No stop condition fired: the build is clean, every
applicable check passes, the census matches the tree the plan was measured
against, and correcting `drops_verify.py`'s D-I2 left all six of its existing
cases passing.

M2's completion gate is the handoff in §6. Nothing further is implementable
offline; the next step is the developer's hardware test, and after it either the
documentation stage or — if the Hunter's Dream or the Shadows of Yharnam break —
the newly justified exemption work, which is a separate feature item.

No branch was created and nothing was committed; the working tree is the
developer's (`CLAUDE.md` §9).
