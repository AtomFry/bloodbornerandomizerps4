# Plan 027 — No Team Type (`ENEMIES HOSTILE TO EACH OTHER`)

**Status: APPROVED** — the developer approved this plan, and the Execution
Strategy below with it, on 2026-09-28. Stage D (plan review) was skipped at the
developer's request. §8 is empty; its one question was answered the same day and
is recorded as D1 in §9.

**Spec:** `docs/features/027-no-team-type/spec.md` — **APPROVED** (2026-09-28).
Three binding decisions, §10 D1–D3.

**Evidence:** `docs/features/027-no-team-type/plan-evidence.md` — the
measurements, traces and rejected alternatives behind this plan.

**Plan review:** `docs/features/027-no-team-type/plan-review.md` — added during
review.

---

## Execution Strategy

*Approving this plan approves this strategy. `CLAUDE.md` §4: milestones describe
implementation structure; gates describe when a human stops to test.*

| | |
| --- | --- |
| **Structure** | 2 functional milestones |
| **Execution** | continuous |
| **Human test gates** | final only |
| **Intermediate verification** | after M1: `cd app && make`, `team_type_verify.py selftest`, `drops_verify.py selftest`, `itemdata_verify.py roundtrip`. After M2: `make clean && make`, `pool_verify.py selftest`, `settings_ui_verify.py`, `worlds_verify.py selftest`, and M1's checks still passing |

No intermediate gate is proposed, and one is not available to propose: the
setting is unreachable until M2 adds it, so there is nothing a human could turn
on after M1. Nothing here can corrupt or destroy data — the only write is one
byte per row into a staging tree, save data is untouched, and no map file is
read or written. M2 cannot make an M1 failure harder to diagnose: M1's output is
judged byte-wise against vanilla by M1's own verifier, from a saved run.

The whole risk of this feature is what the written value *means in play*, and
that is only knowable from the final hardware test (§6). A mid-way gate cannot
reduce it.

---

> **This document is the implementation contract.** §1–§7 are what the
> implementer reads, in order. §8–§10 are the record, not instructions.
> `plan-evidence.md` holds the investigation; `log.md` holds the history.

---

## 1. Objective

Add one on/off setting, off by default, that writes the fixed value **25** into
`NpcParam.teamType` — byte 303 of each 388-byte row — in **every one of the
31,398 rows**, with no exclusion of any kind, exactly as the Windows reference
tool's `TeamTypeRando()` does. Every creature in the game therefore ends on one
shared allegiance. The pass draws no randomness, touches no other byte, and
changes nothing when the setting is off.

---

## 2. Approved behaviour

| #   | Behaviour | Source |
| --- | --------- | ------ |
| B1  | One new on/off setting, off by default, label `ENEMIES HOSTILE TO EACH OTHER`, category **WORLD**, config key `no_team_type` | spec §10 D3 |
| B2  | With it on, every `NpcParam` row's `teamType` byte is set to the reference's fixed **25** | spec §10 D1, §4 |
| B3  | **No exclusions of any kind** — no protection list, no NPC or boss exemption, no map scoping, no row-id skip. All 31,398 rows are written | spec §10 D2, §3 |
| B4  | No byte outside `teamType` changes, in `NpcParam` or in any other archive member. Row count, row ids and row stride are unchanged | spec §8 |
| B5  | With it off, `teamType` is byte-identical to vanilla in every row, and the archive is untouched by this feature | spec §7 |
| B6  | The pass draws **no randomness at all**, so the same seed produces the same world with the setting on or off — the same property `ENABLE MERGO DARKNESS` and `START WITH HUNTER TOOLS` have | spec §2, §7 |
| B7  | It is independent of every other setting. Because every row ends with the same value, it does not matter which creature the enemy, boss or easy-mode pass put in a spot, nor whether the drop pass ran | spec §2, §4 |
| B8  | The help text must **not** assert an in-game effect the project has not observed | spec §10 D3(a), §7 |
| B9  | The state persists in the existing `defaults.cfg` and in a world revision; a file without the key loads with the setting off | spec §7 |
| B10 | The field is global, so chalice creatures are reached. No chalice behaviour is added, measured or supported | spec §6 |
| B11 | Turning it on changes no map file | spec §8 |

---

## 3. Constraints and invariants

### 3.1 Must be preserved

* **`RANDOMIZE ENEMY DROPS` output is unchanged**, bytes and rolls both. It is
  the only other `NpcParam` writer and it is shipped and hardware-proven; its
  file is not edited by this feature at all.
* **No randomness is drawn on any path of the new pass.** Not one `RandInt`, not
  one distribution, not even when the setting is on. This is what makes B6 a
  stronger claim than rows 11 or 37 can make: turning this setting on moves
  *no* roll, rather than moving only later ones.
* **Only byte 303 of a row is written.** 302 is `npcType` and 304 is `moveType`;
  an off-by-one there is a silent behavioural change to every creature.
* **No row is excluded.** In particular `252100` and `6071`, which the drop pass
  skips, are **written** here (§3.3).
* **The archive is edited in place** — one 1-byte store over an existing field.
  Nothing is resized, nothing is rebuilt, and the archive is decompressed and
  re-emitted exactly once per run.
* **Off draws nothing and writes nothing**, so an off run's tree and roll
  sequence are the ones the app produces today.
* **Config compatibility both ways**: unknown keys are ignored on load, and an
  absent `no_team_type` reads as off.
* **Layering**: the pass, its offset constant and its option live under
  `app/src/Randomizer/`; no SDL2 there, and no UI file learns a param offset or
  the value 25.

### 3.2 Out of scope

* Any protection list, NPC exemption, boss exemption or row-id skip. D2 takes
  none. If the hardware test shows the Hunter's Dream breaks, the smallest
  possible exemption is added **afterwards**, with that observation as its
  justification.
* Writing any value other than 25, or writing it conditionally. D1 ports the
  reference verbatim.
* Any other `NpcParam` field, `npcType` included, and any change to enemy drops.
* Re-serialising the param the way the reference does (`PARAM.Write()` over
  every member). The port pokes bytes; see `plan-evidence.md` §E1.
* Changing `EnemyRandomizerJob::StatusText`'s `ItemData` label, which reads
  `RANDOMIZING ENEMY DROPS` for every param feature already.
* Renaming or re-describing backlog row 27 in
  `docs/randomization-feature-spec.md`, and `docs/user-guide.md`. Both belong to
  the documentation stage — including the correction that value 25 is not a
  "hostile to everything" marker.
* **Chalice dungeons.** The field is global so chalice creatures are reached;
  nothing chalice-specific is added, measured or tested.
* Revising the label. D3 accepted the risk; the revision, if the hardware test
  calls for it, is later work and does not touch persistence.

### 3.3 Hazards

| Hazard | What to do | Evidence |
| ------ | ---------- | -------- |
| `drops_verify.py`'s D-I2 claims "only `itemLotId_1` differs" but only checks bytes 0–43 and an empty slice; bytes 52–387 are **unchecked**, so it passes a tree in which every `teamType` was rewritten, and passes a tree with a stray byte at offset 100 | Correct D-I2 in M1 to compare the whole row outside `itemLotId_1`, then add the `--team-type` declaration flag. Without the correction the two features' verification has a hole rather than a collision | `plan-evidence.md` §E4 M7, §E5.1 |
| The easy-mode larva `NpcParam` 252100 and its 31 scaled variants all carry `teamType` **26**, and plan 018 rests on "26 means non-hostile". This pass moves them to 25 | Do not exempt them — D2 and `CLAUDE.md` §7 forbid it, and an unevidenced exemption is the mistake `docs/enemy-exclusion-history.md` records. Instead **test the combination**: §6 item 7 turns `EASY SHADOWS` on alongside this setting and reaches the Shadows of Yharnam, so the question is answered rather than left open. A failure there is the evidence that would justify the exemption | `plan-evidence.md` §E5.2 |
| An off-by-one on the offset writes `npcType` (302) or `moveType` (304) and still produces a plausible tree | `team_type_verify.py` selftest case S5 pins exactly this: writing 25 at 302 or 304 must be **rejected** | `plan-evidence.md` §E4 M2 |
| `pool_verify.py` pins the settings block at an **exact** byte count and fails the moment a key is added | Update to **743 / 794** in M2, extending the comment that narrates the arithmetic. If feature 011 has landed first, rebase on its 775 / 826 to give **790 / 841** | `plan-evidence.md` §E4 M8 |
| `settings_ui_verify.py` case 1 fails as soon as `RandomizerDefaults` gains a bool with no model entry, and case 3 fails until the settings-UI spec §7.1 lists the setting — where `No Team Type` currently sits in the **Enemies** backlog cell, not World | Do M2's steps in the given order: field, model entry, then spec §7.1 and `PROSE_TO_LABEL` together. Move the prose name into the **World** current cell | `plan-evidence.md` §E5.3 |
| `worlds_verify.py` pins the options mapping and the `anythingOn` chain as ordered lists | Insert the field immediately after `enableMergoDarkness` in the source **and** in both pinned lists | `plan-evidence.md` §E4 M9 |
| `RandomizerDefaults` grows and two screens plus `WorldStore` hold copies; a stale object file cost eight hours once | `make clean && make` in M2 | `docs/known-traps.md` |
| The label asserts an effect nobody has observed (D3, accepted) | The help text must not compound it (B8), and §6's hardware verdict is the label question. Do not "improve" the help into a claim | spec §10 D3 |

---

## 4. Implementation approach

> Chosen: a new engine file beside `DropRandomizer`, one unconditional byte
> store per row, verified by a new mirror plus a correction to the tool that
> already owns `NpcParam`. Alternatives considered and why they were rejected:
> `plan-evidence.md` §E3.

### 4.1 The pass

`app/src/Randomizer/TeamType.{h,cpp}` — **new**, beside `DropRandomizer`, not an
extension of it and sharing no helper with it: the two are different features on
the same table, and `DropRandomizer.cpp` is shipped, hardware-proven and has no
reason to be reopened.

```
struct TeamTypeResult { int rowsWritten = 0; int rowsChanged = 0; };
bool ApplyOneTeamType(std::vector<uint8_t>& plain, const ParamMember& npcParam,
                      TeamTypeResult& result, std::string* error);
```

No `std::mt19937` parameter — the signature is where B6 is enforced, so the
function is given no source of randomness to draw from.

Its anonymous namespace declares `kTeamTypeOffset = 303` and
`kOneTeamTypeValue = (uint8_t)25`, each with the paramdef citation (`cell 100`,
`teamType`, `u8`, `NPC_TEAM_TYPE`) and the reference line
(`RandomizeFunctions.cs:3293-3298`).

The body: `ParseParamRows` once, then for every row unconditionally read
`plain[row.dataOffset + 303]`, increment `rowsChanged` if it differs from 25,
store 25, and increment `rowsWritten`. **There is no filter, no row-id test and
no early `continue`** — a `continue` anywhere in that loop is this plan being
implemented wrongly. Then one `Log` line reporting both counts.

### 4.2 Where the pass runs

In `StepItemData`, step 1, **after** the starting-weapon / shop block and
**before** the `startWithHunterTools` block. That is the reference's own
position among the passes this port shares with it — `StartFunctions.cs` runs
`TeamTypeRando()` at 1557, after enemy drops (1536) and shop items (1541). If
feature 011's `ShopStock` block has landed, this goes after it.

The position is reference fidelity and readability only, **not** behaviour:
because the pass draws nothing and writes a byte no other pass reads or writes,
it commutes with every pass in the run. The two `NpcParam` passes are provably
disjoint — bytes 44/48 versus byte 303 of the same 388-byte row — so no ordering
constraint exists between them in either direction, and neither reads the
other's field. The enemy, boss, boss-scaling and easy-mode passes write MSB
placement fields (`NPCParamID`, `ThinkParamID`, model index), never this table,
and because *every* row ends with the same value the answer to "whose team type
applies" is the same whichever row a placement now points at.

The block locates `NpcParam.param` itself, per feature, as every other block in
that function does, and `Fail`s with its own message if it is absent.
`EnemyRandomizerOptions` gains `noTeamType` immediately after
`enableMergoDarkness`, and it joins `AnyParamFeature()` so the archive is loaded
when this setting is the only one on. `EnemyRandomizerResult` gains
`teamTypeRowsWritten`.

### 4.3 The settings chain

| Site | What to add |
| --- | --- |
| `RandomizerDefaults.h` | `bool noTeamType = false;` immediately after `enableMergoDarkness`, with a comment saying it is not a randomizer, that off leaves the archive untouched, and that the label asserts an unobserved effect the hardware test has yet to settle |
| `RandomizerDefaultsStore.cpp` | key **`no_team_type`** in `FormatSettings` and `ApplySettingKey`, both after `enable_mergo_darkness`. `no_team_type=1` plus its newline is **15 bytes**: settings block 728 → **743**, worst-case `defaults.cfg` 779 → **794**, inside `char buf[1024]` with 281 bytes spare |
| `SettingsModel.h` | `SettingId::NoTeamType`, declared after `EnableMergoDarkness` |
| `SettingsModel.cpp` | one `kSettings` entry, kind `Toggle`, category `World`, after `EnableMergoDarkness` |
| label | `ENEMIES HOSTILE TO EACH OTHER` — 558 px, and 651 px with the 40 px gap and `YES`, inside the 700 px pane row. It becomes the widest row on the screen |
| help | `"Puts every creature in the game onto one shared allegiance - enemies, bosses and the Hunter's Dream residents alike. What this changes in play is not yet established, so try it on a spare save."` — 7 lines of the 11 allowed at 440 px, and it states the mechanism without asserting the label's effect (B8) |
| `Game/WorldActivation.cpp` | `options.noTeamType = run.noTeamType;` immediately after the `enableMergoDarkness` line, and `run.noTeamType` added to the `anythingOn` chain in the same position |
| `UI/WorldEditorScreen.cpp` | one report line when the setting is on — `"SET " + rowsWritten + " CREATURE RECORDS TO ONE ALLEGIANCE"`, 44 characters against the 71-character line — and `run_.noTeamType` added to the `ITEM DATA …` line's condition. **No `SKIPPING` line**, the same rule `ENABLE MERGO DARKNESS`, `START WITH HUNTER TOOLS` and the four easy modes follow: NO means the app left the file alone |
| `docs/features/randomizer-settings-ui/spec.md` | §7.1: `No Team Type` leaves the **Enemies** backlog cell and joins the **World** current cell, after `Enable Mergo Darkness`; one Appendix A row carrying the help text above |

The count rather than a state line, because the number is **fixed at 31,398**:
a 0 or a wrong figure means the row walk is wrong, which is the same reasoning
the four easy-mode lines use. Both settings screens need no change — they render
any `Toggle` generically from the model. No Makefile change. No saved
`defaults.cfg` or world revision is invalidated.

`noTeamType` / `no_team_type` are named for the mechanism and the backlog row,
not for the label, so that revising the label after the hardware test touches no
identifier, no key and no saved file (D3's own reasoning for fixing the key).

### 4.4 The verification mirrors

**`app/tools/team_type_verify.py` — new**, modelled on `hunter_tools_verify.py`:
a property validator over the real archive, offsets from `param_offsets.py`
against the real paramdef, never re-deriving the C++'s answer. It owns the
positive assertions, which `drops_verify.py` has no business making.

**`app/tools/drops_verify.py` — corrected and extended.** It owns `NpcParam`, so
it must be able to judge a tree this feature has touched, and its D-I2 is
currently weaker than its docstring (§3.3). Two changes: D-I2 compares the whole
row outside `itemLotId_1`, and a `--team-type` flag declares the run had this
setting on — tolerating byte 303 while **asserting** every row holds 25, the way
`--granted` asserts the tolerated weapon id rather than merely allowing it.

---

## 5. Files and changes

| File | Change | M |
| ---- | ------ | - |
| `app/src/Randomizer/TeamType.h` | **new** — the result struct and `ApplyOneTeamType`, with the paramdef and reference citations | 1 |
| `app/src/Randomizer/TeamType.cpp` | **new** — the unconditional per-row byte store | 1 |
| `app/src/Randomizer/EnemyRandomizer.h` | `noTeamType` option after `enableMergoDarkness`, added to `AnyParamFeature()`; `teamTypeRowsWritten` result counter | 1 |
| `app/src/Randomizer/EnemyRandomizer.cpp` | include `TeamType.h`; the pass call in `StepItemData` at §4.2's position | 1 |
| `app/tools/team_type_verify.py` | **new** — `census`, `verify`, `selftest` | 1 |
| `app/tools/drops_verify.py` | the D-I2 correction, the `--team-type` flag, and the new selftest cases | 1 |
| `app/src/Randomizer/RandomizerDefaults.h` | `noTeamType` bool after `enableMergoDarkness` | 2 |
| `app/src/Randomizer/RandomizerDefaultsStore.cpp` | `no_team_type` in `FormatSettings` and `ApplySettingKey` | 2 |
| `app/src/UI/SettingsModel.h` | `SettingId::NoTeamType` | 2 |
| `app/src/UI/SettingsModel.cpp` | one `kSettings` entry with §4.3's label and help | 2 |
| `app/src/Game/WorldActivation.cpp` | the mapping line and the `anythingOn` term | 2 |
| `app/src/UI/WorldEditorScreen.cpp` | the report line and the `ITEM DATA` condition | 2 |
| `app/tools/pool_verify.py` | settings block 728 → 743, worst case 779 → 794, and a written-and-read case for the new key | 2 |
| `app/tools/settings_ui_verify.py` | `PROSE_TO_LABEL` gains `"No Team Type": "ENEMIES HOSTILE TO EACH OTHER"` | 2 |
| `app/tools/worlds_verify.py` | `WIZARD_OPTIONS_MAPPING` and `WIZARD_RUN_DECISION` gain the field in the pinned position | 2 |
| `docs/features/randomizer-settings-ui/spec.md` | §7.1 and Appendix A, per §4.3 | 2 |

A clean rebuild is required in M2 (`RandomizerDefaults` changes size). Nothing
invalidates an existing `defaults.cfg` or world revision.

---

## 6. Verification

### Build

`cd app && make` after M1; **`make clean && make` in M2**. The `.pkg` must be
produced.

### Automated

| Check | Command | Asserts |
| ----- | ------- | ------- |
| The new mirror | `python app/tools/team_type_verify.py selftest data/vanilla/dvdroot_ps4` | the cases below |
| Vanilla census | `python app/tools/team_type_verify.py census data/vanilla/dvdroot_ps4` | prints the 13-value distribution and the 378 rows already at 25; not a pass/fail gate |
| Output tree, on | `python app/tools/team_type_verify.py verify <vanilla> <output> [--drops]` | T-I1–T-I6 against a real run |
| Output tree, off | `python app/tools/team_type_verify.py verify <vanilla> <output> --off [--drops]` | T-I7 — `teamType` byte-identical to vanilla in every row |
| Drops mirror | `python app/tools/drops_verify.py selftest data/vanilla/dvdroot_ps4` | the six existing cases still pass unchanged, plus the corrected D-I2 and the `--team-type` cases |
| Drops on a combined tree | `python app/tools/drops_verify.py verify <vanilla> <output> --team-type` | the drop invariants still hold when this feature also ran |
| Archive round trip | `python app/tools/itemdata_verify.py roundtrip data/vanilla/dvdroot_ps4` | the write path still emits a valid archive at 28 MB scale |
| Config | `python app/tools/pool_verify.py selftest data/vanilla/dvdroot_ps4` | the 743 / 794 arithmetic, and that `no_team_type` is in both load and save |
| Settings model | `python app/tools/settings_ui_verify.py` | the new bool has exactly one model entry, the row fits the pane, the help is non-empty and wraps, and the model matches settings-UI spec §7.1 |
| Worlds parity | `python app/tools/worlds_verify.py selftest` | the options mapping and the run decision carry the field in the pinned order, and a recipe round-trips the key |

`team_type_verify.py`'s invariants:

| # | Invariant |
| - | --------- |
| T-I1 | Only `NpcParam.param` differs inside the archive |
| T-I2 | Within `NpcParam`, only byte 303 of a row differs — bytes 0–302 and 304–387 are intact (`itemLotId_1` tolerated only under `--drops`) |
| T-I3 | **Every** row carries 25. No row carries any other value, and the count written is 31,398 |
| T-I4 | The row id set, the row count and the 388-byte stride are unchanged |
| T-I5 | The 378 rows that already held 25 are byte-identical to vanilla |
| T-I6 | The archive holds the same members, each the same size |
| T-I7 | Under `--off`, every row's `teamType` equals vanilla's |

Its selftest must assert, against the vanilla tree, by simulating output in
Python:

| # | Case |
| - | ---- |
| S0 | The vanilla census is **31,398 rows, 13 distinct `teamType` values, 378 already at 25, 31,020 to change** — so a different data tree is reported rather than silently changing what the other cases mean |
| S1 | An unmodified archive passes `--off`, and is **rejected** in on-mode on T-I3 — proving T-I3 can fail |
| S2 | A fully simulated pass (25 in every row) passes on-mode |
| S3 | …and is **rejected** under `--off` (T-I7) |
| S4 | The same tree with **one** row left at its vanilla value is rejected (T-I3). 31,398 of 31,398 is the whole claim, so one miss must fail |
| S5 | Writing 25 at offset **302** or **304** instead of 303 is rejected (T-I2 and T-I3) — the off-by-one that would otherwise ship |
| S6 | Writing a uniform value that is not 25 in every row is rejected (T-I3) |
| S7 | `252100` and `6071` — the drop pass's two exclusions — must be **written**: a tree that skips them is rejected |
| S8 | A changed `itemLotId_1` is rejected without `--drops` and tolerated with it, and T-I3 is still asserted either way |
| S9 | Changing one byte of another member is rejected (T-I1); a member list or member size change is rejected (T-I6) |
| S10 | A combined tree — 25 everywhere plus a simulated drop shuffle — passes with `--drops` and fails without |

`drops_verify.py`'s new cases:

| # | Case |
| - | ---- |
| D-S1 | Every existing case passes unchanged, and the corrected D-I2 now catches a stray byte at offset 100 and at offset 303 — both of which pass today |
| D-S2 | A full `teamType` rewrite passes **with** `--team-type` and is rejected without it |
| D-S3 | `--team-type` still rejects a tree where only *some* rows hold 25, and still rejects a stray write elsewhere in the row |

**A mirror pins the rules, not the C++ implementation of them.** These tools
show that an output tree carries the byte the reference writes, everywhere and
only there. They cannot show that the engine's loop is the one that put it
there, and — this being the whole point of the feature — they say **nothing
whatever** about what Bloodborne does with the value. `docs/known-traps.md`:
byte decoding does not prove game behaviour.

### Hardware

The implementer cannot run any of this. It is the handoff, and it is
**discovery, not confirmation**: nothing offline establishes what the value
means.

**The verdict.** D3's binding first question: **is `ENEMIES HOSTILE TO EACH
OTHER` a true label?** The offline checks have already proved the bytes are in
the tree, so a null observation on console cannot mean "the pass failed" — it
means the game does not act on the field the way the label claims. Classify the
run by what is seen:

| Observation | Reading | Consequence |
| ----------- | ------- | ----------- |
| Creatures attack or damage one another unprompted, where they did not before | The label is true | Ship it; the documentation stage corrects the spec's §4 finding |
| The Hunter's Dream or the game's talkers turn hostile, or stop talking, but enemies still ignore each other | The write does something, but **not** what the label says | Stop. Report. The label and the help both need revising, and the smallest exemption becomes justified work |
| Nothing changes anywhere, in any of the five checks below | The write is inert as far as play is concerned | Report it. The label is false and must be revised; `docs/windows-randomizer-technical-review.md` §13 question 5 is answered |

**The procedure**, with the setting **on** and nothing else on. The Hunter's
Dream is first because D2 took no protection and that is where an unprotected
write shows, and because it is where a new character spawns:

1. **The Hunter's Dream is intact.** The Doll can be spoken to and does not
   attack. Gehrman in his wheelchair can be spoken to and does not attack. The
   Messengers behave normally. Levelling up, the shop and travel all work. This
   is the run-ending failure case.
2. **Whether enemies fight each other.** Somewhere densely populated with mixed
   creature types — Central Yharnam's mob, the Forbidden Woods, Yahar'gul —
   watch for creatures attacking one another unprompted or taking damage from
   one another's attacks.
3. **Whether quest NPCs still work.** A window NPC conversation, and at least
   one of Master Willem, the kneeling Lady Maria or Ludwig's head, all of which
   move off the NPC allegiance.
4. **Whether cooperating adds still behave.** The Witch of Hemwick and her Mad
   Ones is the sharpest case: those two sit on *different* allegiances today and
   end on the same one.
5. **Whether anything changes at all.** "No observable difference" is a real and
   likely result, and recording it is as valuable as recording a dramatic one.
6. **One run with the setting off**, to confirm the game is the one it was.
7. **The easy-mode combination, on the same run as 1–5.** Turn `EASY SHADOWS` on
   alongside this setting and reach the Shadows of Yharnam. The two substituted
   larvae carry `teamType` **26** today and **25** after this pass (§3.3), and
   feature 018 rests on 26 meaning non-hostile — so this is the one interaction
   where a plausible reading of the field predicts a broken boss fight rather
   than a cosmetic change. Record whether the larvae still stand inert.
   **A failure here is a finding, not a defect in this feature**: it would be
   the first evidence that the field decides hostility at all, and it is what
   would justify a one-row exemption that nothing justifies today (§3.3).

**Not covered, and known not to be.** The other three easy modes are not
exercised — `EASY SHADOWS` is the cheapest to reach and the substitution is
identical in all four, so testing one stands for the set. Chalice dungeons are
reached and are not tested.

Save the output tree and `live.log` of two runs into `data/runs/` — one on, one
off — so `team_type_verify.py verify` and `verify --off` can be run against
each.

---

## 7. Milestones and stop conditions

### Milestone 1 — the pass and its proof

**Goal.** A run can put every creature record on one allegiance, reachable
through `EnemyRandomizerOptions`, with the drop pass's output untouched and a
verifier that can tell a correct tree from a nearly-correct one.

**Changes**, in order:

| # | Change | Done when |
| - | ------ | --------- |
| 1 | Write `TeamType.{h,cpp}` per §4.1 | the loop has no filter, no row-id test and no `continue`; the signature takes no RNG; the two constants carry their paramdef and reference citations |
| 2 | Add `noTeamType` to `EnemyRandomizerOptions` after `enableMergoDarkness`, put it in `AnyParamFeature()`, and add `teamTypeRowsWritten` to `EnemyRandomizerResult` | every existing call site still compiles untouched, and a run with only this option on reaches `StepItemData` |
| 3 | Call the pass in `StepItemData` at §4.2's position | the call sits after the starting-weapon block and before the hunter-tools block, locates `NpcParam.param` with its own `Fail` message, and no line of `DropRandomizer.cpp` is edited |
| 4 | Correct `drops_verify.py`'s D-I2 and add `--team-type` per §4.4 | the six existing cases pass unchanged, and D-S1–D-S3 pass |
| 5 | Write `app/tools/team_type_verify.py` with `census`, `verify` and `selftest` | S0–S10 all pass, and `census` prints the §6 distribution |

**Invariants** this milestone must not break: drop-pass output and rolls, no
randomness drawn, byte 303 only, no row excluded, in-place edit, off writes
nothing, layering (§3.1).

**Verification:** `make`; `team_type_verify.py selftest`;
`drops_verify.py selftest`; `itemdata_verify.py roundtrip`.

**On completion.** The `.pkg` builds and the checks pass. **Continue to M2.**

### Milestone 2 — the setting a player can set

**Goal.** A player can turn the setting on, the choice persists, an activation
honours it, and the run reports it.

**Changes**, in order:

| # | Change | Done when |
| - | ------ | --------- |
| 1 | The `RandomizerDefaults` field | a fresh struct reads off |
| 2 | `no_team_type` in both halves of `RandomizerDefaultsStore.cpp` | a config without the key loads with the setting off, and a saved file round-trips it |
| 3 | `SettingId::NoTeamType` and the `kSettings` entry with §4.3's label and help | the row appears in `WORLD` after `ENABLE MERGO DARKNESS`, reading `NO` |
| 4 | settings-UI spec §7.1 and Appendix A, and `PROSE_TO_LABEL` | `settings_ui_verify.py` cases 1, 3 and 5 pass, with the new row the widest at 651 px |
| 5 | `WorldActivation.cpp`'s mapping line and `anythingOn` term, and `worlds_verify.py`'s two pinned lists | `worlds_verify.py selftest` passes with the field in the pinned position |
| 6 | `WorldEditorScreen.cpp`'s report line and the `ITEM DATA` condition | a run with only this setting on prints the count line and the item-data line, and no `SKIPPING` line is added |
| 7 | `pool_verify.py`'s 743 / 794 arithmetic and the key case | `pool_verify.py selftest` passes with no unrelated case changing state |

**Invariants** this milestone must not break: absent key reads as off; no screen
file learns a param offset or the value 25; no UI file touches a raw AFR path
(§3.1). The help text must not assert an unobserved effect (B8).

**Verification:** `make clean && make`; `pool_verify.py selftest`;
`settings_ui_verify.py`; `worlds_verify.py selftest`; M1's three checks still
passing.

**On completion.** Hand off §6's hardware test.

### Stop conditions

Halt and report rather than deciding, if any of these occur:

* the build fails in a way the plan did not anticipate;
* a verification check fails and the cause is not an obvious implementation
  slip;
* `team_type_verify.py census` reports anything other than 31,398 rows, 388-byte
  stride, `teamType` at offset 303, or 378 rows already at 25 — that means the
  data tree is not the one this plan was measured against;
* correcting `drops_verify.py`'s D-I2 makes any of its six existing cases fail
  against the vanilla tree;
* an implementation decision would contradict the spec, §2 or §3.1 — including
  any temptation to exclude a row, protect the Hunter's Dream, protect the
  easy-mode larva, or write a value other than 25;
* the change needs a file not listed in §5;
* a §3.1 invariant cannot be preserved.

---

## 8. Open questions

Empty. One question was put to the developer on 2026-09-28 and is recorded as D1
in §9; `log.md` holds how it was put and answered. Otherwise:

**None arose.** The spec's D1–D3 settled the behaviour, the value, the row set, the
label, the category and the config key; the repository settled the file
placement, the pass position, the byte arithmetic and the verification shape.
Every number this plan depends on was measured against
`data/vanilla/dvdroot_ps4` (`plan-evidence.md` §E4).

---

## 9. Decisions

| # | Date | Decision | Taken by |
| - | ---- | -------- | -------- |
| D1 | 2026-09-28 | **The easy-mode larva rows are NOT exempted, and the combination is tested instead.** The developer was asked whether to protect `252100`, `6071` and the 31 scaled variants, which would have contradicted spec D2 and required amending an approved spec. The answer was no: keep D2, and answer the question on hardware. §6 gained item 7 — `EASY SHADOWS` on alongside this setting, reaching the Shadows of Yharnam — so the interaction is established rather than left open, and §3.3's mitigation points at it. A failure there is the evidence that would justify a one-row exemption, and it would also be the first proof that this field decides hostility at all | developer |
| P1 | 2026-09-28 | A new file `TeamType.{h,cpp}` beside `DropRandomizer`, sharing no helper with it and not editing it. Nothing is genuinely shared — two different fields, two different features — and the drop pass is shipped and hardware-proven | planner |
| P2 | 2026-09-28 | `ApplyOneTeamType` takes no `std::mt19937`. B6 is enforced by the signature, not by a comment | planner |
| P3 | 2026-09-28 | The pass runs after the starting-weapon / shop block and before hunter tools, matching the reference's relative position. Stated as reference fidelity, not as an ordering constraint: the pass commutes with everything | planner |
| P4 | 2026-09-28 | Field `noTeamType` and key `no_team_type` are named for the mechanism and the backlog row, not the label, extending D3's reasoning so a label revision touches no identifier and no saved file | planner |
| P5 | 2026-09-28 | Verification is split: a new `team_type_verify.py` owns the positive assertions, and `drops_verify.py` is corrected and given a `--team-type` declaration flag. The correction is not optional — D-I2 currently passes a tree in which every `teamType` was rewritten | planner |
| P6 | 2026-09-28 | The progress line reports the fixed count 31,398 rather than a state, following the easy-mode lines: a fixed number makes a wrong figure legible. No `SKIPPING` counterpart | planner |
| P7 | 2026-09-28 | Two milestones — the pass with its proof, then the setting. No intermediate gate: the setting is unreachable until M2, so there is nothing a human could test after M1 | planner |
| P8 | 2026-09-28 | The settings-UI spec's §7.1 prose name stays **No Team Type** while the shipped label is `ENEMIES HOSTILE TO EACH OTHER`, following the `Protect Caged Dogs` precedent. It ties the row to the backlog and survives a label revision | planner |

---

## 10. Changes during implementation

Implemented 2026-09-28, both milestones in one continuous pass. Nothing in
§1–§7 was changed. Three items, none behavioural:

| Date | Change | Reason |
| ---- | ------ | ------ |
| 2026-09-28 | **The shipped label is now `ENEMIES ON SAME TEAM`, not `ENEMIES HOSTILE TO EACH OTHER`, and the help text is rewritten.** §1–§7 are deliberately left saying the old label: they are the contract as approved, and §10 is where a post-approval change belongs. The authority for the new wording is **spec §10 D4**, which supersedes D3. | Two hardware runs showed no difference on or off, and the audit found why: the reference's own checkbox text is `Content="No Team Type"` (`MainWindow.xaml:207`, `AllDmgAll` being only a variable name), 81.6% of placed creatures are already on team 23 so uniform-to-uniform is a no-op in kind, and the pass's real effect is to **suppress** the infighting that shuffling creates across the pool's six teams. The old label asserted the opposite of what the setting does. No behaviour, byte, key or count changed — label and help strings only, so B1's geometry claim is re-measured rather than invalidated: **392 px label, 485 px row against the 700 px budget** (was 558/651), and the row is no longer the app's widest |
| 2026-09-28 | `ApplyOneTeamType` fails the run if a row's byte 303 would fall outside the decompressed buffer, instead of writing it unchecked. Not a filter and not a `continue`: the loop still visits every row and the function returns `false`, ending the run, exactly as `GrantHunterTools` does for its own row bounds | `ParseParamRows` only proves a row *starts* inside the member (`ParamBnd.cpp:126`), so an unchecked store at +303 would be out-of-bounds on a malformed archive. The plan did not cover it; §3.1's "no row is excluded" is preserved because the check cannot skip a row, only end the run |
| 2026-09-28 | Verification also ran `worlds_verify.py all`, not only `selftest` | §6 names `worlds_verify.py selftest`, but that mode runs only `rule_cases()` (manifest rules). The two pinned lists §5 requires updating — `WIZARD_OPTIONS_MAPPING` and `WIZARD_RUN_DECISION` — are asserted in `source_cases()`, which only `all` runs. Both were run; both pass |
| 2026-09-28 | `team_type_verify.py`'s selftest carries two cases beyond S0–S10: `TeamType.cpp`'s two constants must match the tool's, and its row loop must contain no `continue` and no `row.id ==` test | The same shape `hunter_tools_verify.py` already uses to pin the C++ constants against the paramdef. The second case mechanises §4.1's "a `continue` anywhere in that loop is this plan being implemented wrongly", which was otherwise only reviewable by eye |
