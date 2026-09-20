# Plan Review 018 — Easy Shadows, Easy Rom, Easy Failures, Easy Emissary

**STALE (2026-09-19)** — this review covers the pre-refinement plan. The stage C
refinement of 2026-09-19 deleted plan §9 decision 1 ("plan 016 lands first"),
whose referent no longer exists, and changed decision 2's row position from
"ahead of the drill-ins" to appended last at rows 17-20. The plan was renumbered
onto the current template, so this review's section references no longer resolve.
Its findings were acted on; see `log.md`.

**Verdict: CHANGES REQUESTED**

**Implementation handoff: NEEDS RESTRUCTURING**

**Plan:** `docs/features/018-easy-shadows/plan.md`

**Evidence:** *(none — this item predates the two-document convention; `plan.md` is the only planning artifact and carries its own evidence)*

**Spec:** `docs/features/018-easy-shadows/spec.md` — APPROVED 2026-09-16

**Reviewed:** 2026-09-19

---

## 1. Summary

The design is right. Every claim this plan makes about **game data** reproduced
exactly — all 79 placements, the seven-name emissary set, the larva's identity
and stat block, the four zone-scaled variants, the umbilical-cord lot, and even
the EMEVD reference counts down to the individual entity ID. The ordering
argument (pass inside `StepWriteMap`, after the enemy loop's closing brace and
before the `BossScalingMaps()` loop) is correct against the current engine, the
structural anchors it names still exist, and the layering is clean. The
`m27_00_00_00` omission, the baked identity, and the four-flags-one-table shape
all follow spec §10 faithfully.

What fails is everything the plan says about **the current state of this
repository**, and one class of assertion in §6.

The plan was written for a tree in which feature 016 was about to land. It did
not land: feature 032 D1 **retired** `UNCHANGED BELL MAIDENS` outright, and
features 032, 033 and 034 have since shipped three more rows. `kItemCount` and
`kSaveDataRowCount` are **17**, not 15; `kDisableMergoDarknessRow` is **11**,
not 12; and the screens now append new toggles *after* the three drill-in rows,
which is the opposite of what §4 instructs. §5.5's precondition check ("if they
are 11 and 14, 016 has not landed — subtract one from every index") reads 11 and
17 and would send the implementer down a path that renumbers five live rows.
That is finding 1, and it is blocking.

Finding 2 is independent and would cost a build cycle on its own: §6.3 asks the
verifier to assert that the survivors and the settings-off maps are
**byte-identical to vanilla**. The zone-scaling pass runs unconditionally on
every run and rewrites `NPCParamID` on four of the seven named survivors
(`c2120_0000` → 992117010, `c4030_0000` → 994037483, `c2500_0011/_0012` →
900014521, and `c2570_0001` → 900015234). A correct implementation fails that
assertion. This is the exact defect `pool_verify.py`'s `_frozen` docstring
records as having shipped once already in the retired `maidens` command — and
the plan's own §6.2 case 8 knows the scaling table well enough to have caught
it.

Three smaller but real findings follow: the scaled larva rows are **not**
protected by `IsExcludedNpcRow`, so the "drop intact" property does not hold
with drop randomization on; §6.2 case 4's over-reach assertion is false as
written; and §5.4's "67 spurious failures" does not reproduce (37 does).

None of these change the approach. The pass, the table, the identity, the call
site and the milestone stand as designed. What needs redoing is §4's arithmetic
against the tree as it is today, and §6's output-tree assertions against the
scaling pass the plan already documented.

The handoff assessment is separate and also negative, for the ordinary reason
this project's pre-convention plans fail it: the contract carries roughly 400
lines of investigation, rejected alternatives and decision history mixed into
§1–§7. §6 names the moves.

---

## 2. Findings

### Finding 1 — §4 and §5.5 are written for a tree that never existed, and the precondition check returns the wrong answer

**Severity:** Blocking

**Where:** `plan.md` §4 (the two screen rows), §5.5, §7's sequencing sentence,
§9 decision 1, §2.6. Against `app/src/UI/SetupDefaultsScreen.h:36-80` and
`app/src/UI/EnableWizardScreen.cpp:25-59`.

**Problem:** Three separate things have moved since the plan was written.

*Feature 016 did not land — it was deleted.* `docs/features/016-unchanged-bell-maidens/`
is now under `docs/features/Completed/`, and the in-code comment at
`SetupDefaultsScreen.h:45-49` records why: *"UNCHANGED BELL MAIDENS used to sit
at 4… Feature 032 D1 retired it and everything below moved up one."* So §9
decision 1's dependency, §5.5's whole collision analysis, §7's "starts after
plan 016 is implemented", and §2.6's pointer to
`docs/features/016-unchanged-bell-maidens/plan.md` (a path that no longer
exists) are all describing a sequencing problem that has evaporated.

*The counts are wrong in both directions.* §4 says `kItemCount` /
`kSaveDataRowCount` go 15 → 19 and §6.4's comment says 18. Both are 17 today
(features 033 and 034 appended `DO NOT RANDOMIZE CAGED DOGS` and `START WITH
HUNTER TOOLS`), and four new rows take them to 21. `kEnemiesIncludedRow` and
`kBossesIncludedRow` are 12 and 14, and there is now a third drill-in,
`kEnemiesSkippedRow` at 13, that the plan does not know exists.

*The placement rule has inverted.* §4 instructs the implementer to insert the
four rows "directly after `kDisableMergoDarknessRow` (row 12), occupying rows
13–16" and to renumber the drill-ins upward. The current screens do the
opposite, and say so in a comment written twice:
`SetupDefaultsScreen.h:67-71` — *"Appended last… going last is what guarantees
no existing row index moves and so no existing row can be mislabelled (feature
033 P5, P13)"*. Following §4 as written renumbers `kEnemiesIncludedRow`,
`kEnemiesSkippedRow`, `kBossesIncludedRow`, `kDoNotRandomizeCagedDogsRow` and
`kStartWithHunterToolsRow`, against an invariant those files state explicitly —
and both headers warn that a `DrawList` / `DrawSaveData` entry added out of
order "compiles, passes `ui_scroll_verify.py` and mislabels every row below it."

*And the safety net fires backwards.* §5.5 tells the implementer: "read
`kDisableMergoDarknessRow` and `kItemCount` out of `SetupDefaultsScreen.h` and
check they are 12 and 15. If they are 11 and 14, 016 has not landed after all
and every index in §4 is one too high." The actual values are **11 and 17** —
neither branch. The half that matches ("11") points at the wrong conclusion and
prescribes subtracting one from indices that are already four too low.

**Why it matters:** This is the one part of the plan an implementer executes
mechanically rather than thinking about, and it is wrong in a way that both
compiles and passes `ui_scroll_verify.py`. The visible symptom on hardware is a
Setup Defaults screen where five rows carry the wrong labels — including two
that open pickers — and a saved `defaults.cfg` written from the wrong rows.
§9 decision 2's *reasoning* ("the placement rule the last non-randomizer already
follows") still holds; it is only the arithmetic and the "ahead of the two
drill-in rows" clause that have gone stale.

**Recommendation:** Re-derive §4's two screen rows against the current headers
and delete the 016 dependency wherever it appears (§2.6, §5.5, §7, §9
decision 1's rationale). State the placement rule in the form the code now uses
rather than as absolute indices, so it survives the next feature. Note also that
`ui_scroll_verify.py` now carries seven entries including a `Skipped picker`,
and that its three settings-screen counts are 17.

**Confidence:** High. Read directly out of the headers and the working tree.

---

### Finding 2 — §6.3's "byte-identical to vanilla" assertions cannot pass a correct implementation

**Severity:** Blocking

**Where:** `plan.md` §6.3, bullets 3 and 5 ("the survivors of case 3 are
**byte-identical** to vanilla in all three written fields" and "maps whose
settings are off are byte-identical to vanilla").

**Problem:** The `BossScalingMaps()` loop in `StepWriteMap`
(`EnemyRandomizer.cpp:855-866`) is **unconditional** — it runs on every map, on
every run, regardless of which settings are on. Four of the six target maps are
in its list, and most of the survivors §6.2 case 3 names are tracked values that
it rewrites. Re-derived from `NpcScalingTable.h`:

| Survivor | Map (zone scale) | Vanilla `NPCParamID` | In the output tree |
| --- | --- | ---: | ---: |
| `c2120_0000` | m27_00_00_01 (+9) | 212700 | **992117010** |
| `c4030_0000` | m35_00_00_00 (+27) | 403000 | **994037483** |
| `c2500_0011` / `_0012` | m24_02_00_01 (+14) | 250082 | **900014521** |
| `c2570_0001` | m24_02_00_01 (+14) | 257010 | **900015234** |
| `c4030_0004` | m35_00_00_00 | 403050 | 403050 (not tracked) |
| `c2500_0000` | m24_02_00_01 | 250080 | 250080 (not tracked) |

So three of the seven survivors §6.2 case 3 enumerates, plus the Celestial
Emissary itself that §6.2 case 10 names, change their `NPCParamID` in any
correct output tree. A verifier written to §6.3's bullet 3 fails on correct
output. Bullet 5 is worse: with the setting off the *whole map* still goes
through the scaling pass, and on top of that the `.msb.dcx` is re-serialised and
re-compressed by `DcxCompress`, so it is not byte-identical to vanilla at the
file level either — `itemdata_verify.py:164` records the same distinction
("only the compression wrapper differs").

This exact mistake is already documented in the repository.
`app/tools/pool_verify.py:188-200`, `_frozen`:

> *NOT plain equality with vanilla, and that distinction is the whole point. The
> stat pass (BossParamScaling) runs AFTER the enemy pass and rewrites
> NPCParamID on 1,369 of the 2,269 overwritable placements — a frozen one
> included. Demanding equality would fail on a correct implementation, which is
> exactly the bug the retired `maidens` command shipped with.*

**Why it matters:** §6.2 and §6.3 are the plan's only pre-hardware gate. An
implementer who builds them as specified gets red output from a correct build,
and the natural debugging move — loosening the assertion until it passes — is
how a verifier stops asserting anything. It also costs the hardware cycle the
milestone was aiming at.

**Recommendation:** Restate §6.3's survivor and settings-off assertions in the
`_frozen` form the repository already uses: same model, same think, and an
`NPCParamID` that is either the vanilla value or that map's zone-scaled variant
of it. §6.2 case 8 already contains everything needed to compute the expected
value per map. `pool_verify.py`'s `zone_scaled_npc` is the existing helper.

**Confidence:** High. The scaled values above were computed from
`NpcScalingTable.h` and `BossParamScaling.h` directly; the unconditional
placement of the scaling loop was read from `EnemyRandomizer.cpp`.

---

### Finding 3 — the larva's drop is not protected in the four scaled maps, which is where this feature puts most of them

**Severity:** Should fix

**Where:** `plan.md` §5.6 ("The umbilical cord relocates"), §6.2 case 9, §6.3's
last bullet, §6.5 step 5.

**Problem:** `DropRandomizer.cpp:22-24` excludes exactly two rows:

```cpp
bool IsExcludedNpcRow(int32_t id) {
    return id == 252100 || id == 6071;
}
```

The 31 scaled variants `900014601`–`900014631` are **not** excluded, and all 31
carry `itemLotId_1 = 28040` (I confirmed the 32 referencing rows are 252100 plus
exactly those 31). With `RANDOMIZE ENEMY DROPS` on, `RandomizeEnemyDrops` will
rewrite `itemLotId_1` on every one of them, and will also feed lot 28040 into
the shared pool 31 times so other enemies can receive it.

Four of the six maps this feature touches are zone-scaled, so the larvae in
m27_00_00_01, m32_00_00_01, m24_02_00_01 and m35_00_00_00 land on rows
900014609, 900014611, 900014614 and 900014627 — all randomizable. Only
m32_00_00_00 and m24_02_00_00 keep row 252100, and those are the two variants
the retail game does not load.

The effect is not new (the clinic larva in m24_01_00_01 already scales to
900014601 today), and it matches the reference, which excludes the same two
rows — so this is not a fidelity problem. But the plan states the protection as
unqualified fact in §5.6 ("`NpcParam` 252100 is one of the two rows
`DropRandomizer.cpp:24`'s `IsExcludedNpcRow` protects"), §6.3 asserts only rows
252100 and 28040, and §6.5 step 5 gives no setting state for the cord test.

**Why it matters:** §6.5 step 5 is the designated evidence gate for revisiting
spec §10 D2. Run with drops on, it reports "no cord from any easy-mode larva" —
which contradicts neither the plan nor the reference, but reads exactly like the
failure the step was written to detect. Conversely a cord appearing on some
unrelated enemy would look like a new bug rather than the existing drop pool
doing its job.

**Recommendation:** Pin the setting state in §6.5 step 5 (drops off), and either
extend §6.3's `NpcParam` assertion to the 31 variants or state explicitly that
they are deliberately unprotected and why. §6.2 case 9 is the natural place for
the scaled-variant fact, since it already parses `IsExcludedNpcRow` out of the
C++.

**Confidence:** High for the data and the code path. The player-visible
consequence is inference — no verifier and no reading of `getItemFlagId` settles
where the cord actually lands.

---

### Finding 4 — §6.2 case 4 asserts something that is false, and would be written as a failing test

**Severity:** Should fix

**Where:** `plan.md` §6.2 case 4.

**Problem:** The case reads: *"No pattern over-reaches. Across all 43 `.msb.dcx`
files, the set each pattern matches is exactly the set in case 1."* Measured
across all 43 files, it is not:

| Pattern | Maps matched tree-wide |
| --- | --- |
| `c1400` | m32_00_00_00, m32_00_00_01, **m29_30_90_00, m29_30_90_01** — 138 placements, 39 distinct names |
| `c2500_0001` | m24_02_00_00/01, **m24_01_00_00/01/11, m27_00_00_00/01, m29_53_90_00/01** |
| `c2120_0002` | m27_00_00_00/01, **m26_00_00_00** |

The *behaviour* is safe, because §3.1's table is keyed by exact map name and a
pattern is only ever applied inside its own row's map — within m32_00_00_00 the
single pattern `c1400` matches exactly 30, and within m24_02_00_00 the ten
`c2500` patterns match exactly 7. That is the property worth asserting, and it is
the property the plan means. The sentence as written asserts the tree-wide one.

**Why it matters:** §6.2 is the specification for a file that does not exist yet.
An implementer writing case 4 literally produces a test that fails on a correct
table, then has to work out which of the two readings was intended — and case 4
is described in the same paragraph as "the one rule that could silently take more
than intended", so weakening it on a guess is the worst outcome.

**Recommendation:** Scope case 4 to "within its own map" and keep the tree-wide
numbers as the reason the exact-map keying in §3.1 matters — they are the
strongest available evidence for that design choice, and they currently sit
nowhere.

**Confidence:** High. Measured with `boss_verify.py`'s `Msbb` reader over all 43
files.

---

### Finding 5 — §5.4's "67 spurious failures" does not reproduce

**Severity:** Should fix

**Where:** `plan.md` §5.4.

**Problem:** `compare_trees` iterates `BOSS_MAP_ORDER` only
(`boss_verify.py:453`), and `load_maps` loads only those 17 maps
(`boss_verify.py:238-244`). Of the six maps this feature touches, only four are
in that list — `m32_00_00_00` and `m24_02_00_00` are not. So the V2 non-boss
false positives are 30 (`c1400_*` in m32_00_00_01) + 7 (`c2500_000x` in
m24_02_00_01) = **37**, not 67. 67 looks like 60 + 7 — both Rom variants but only
one emissary variant.

Everything else in §5.4 checks out, and I verified the two claims that actually
matter: the `addtherest` exemption at `boss_verify.py:512-518` does cover
`c2120_0001/0002` in m27 and `c4030_0001…_0003` in m35; and `identity_ok`
accepts the larva both plain and scaled, because `all_vanilla_triples` walks the
whole map tree (`boss_verify.py:372-391`) so `(252100, 252100, "c2521")` is in
the set, and the scaled ids are in the scaling table. I also checked the one
thing §5.4 does not mention and found it clean: `FIXUP_GROUPS` is
`{c2500_0000: [c2570_0001], c5510_0000: [...], c4520_0002: [...], c4030_0004:
[c4030_0000]}`, and this feature touches no member of any of them, so the
fixup-sync check cannot misfire.

**Why it matters:** Small on its own — the `--easy` flag is the right fix either
way and does not depend on the count. It matters because the number is the kind
a reader trusts without rechecking, and because getting it wrong suggests the
`--easy` suppression list was scoped against all six maps rather than the four
`boss_verify` actually walks. Suppressing by (map, name) from the table is
correct and covers both.

**Confidence:** High.

---

### Finding 6 — the table is six rows, described twice as seven

**Severity:** Should fix

**Where:** `plan.md` §3.1 ("The table is seven rows"), §4 ("the seven-row target
table of §3.1"). §1 correctly says "six named maps", and the table itself has six
data rows.

**Problem:** A count mismatch on the single load-bearing artifact of the feature.

**Why it matters:** The implementer's first act is transcribing this table into
`EasyModes.h`, and §3.5 makes the console print its consequences as an assertion.
Being told the answer is seven while counting six invites exactly the wrong
correction — adding `m27_00_00_00`, which spec §10 D3 forbids and §3.6 A3
rejects outright.

**Recommendation:** Six.

**Confidence:** High.

---

### Finding 7 — `StepWriteMap` already has the model index the pass needs

**Severity:** Worth considering

**Where:** `plan.md` §3.2, "Model-index resolution happens inside
`ApplyEasyModes` with a local scan".

**Problem:** The insertion point §3.2 names sits inside a block that opens by
building exactly that lookup (`EnemyRandomizer.cpp:729-736`):

```cpp
std::unordered_map<std::string, int32_t> enemyModelIndex;
for (size_t i = 0; i < lm.msbb.models.entries.size(); i++) {
    if (model_fields::GetType(lm.msbb.models.entries[i]) == ModelType::kEnemy) {
        enemyModelIndex[model_fields::GetName(lm.msbb.models.entries[i])] = (int32_t)i;
    }
}
```

`enemyModelIndex` is in scope at the call site and already keyed by name.

**Why it matters:** Not much — the cost of a second scan is negligible, and a
self-contained `ApplyEasyModes(mapName, msbb, options, counts)` is a cleaner
signature and easier for the Python mirror to reason about. I raise it only
because §3.2 justifies the scan by precedent (`FindEnemyModelIndex`) without
noting that the caller has already done the work. If the planner prefers the
self-contained signature, that is a defensible call and not a finding — say so
and it is settled.

**Confidence:** High on the fact; this is a taste disagreement on the design.

---

### Finding 8 — stale citations, and a closer precedent than the ones cited

**Severity:** Worth considering

**Where:** `plan.md` §2.1, §2.2, §2.4, §6.1.

**Problem:** The line numbers into the two files features 032/033/034 edited have
all drifted: the phase machine is at `EnemyRandomizer.cpp:241` not `:205-207`;
`StepMergeModels` at `:619-635` not `:481-499`; the enemy loop at `:745-853` not
`:606-676`; the scaling loop at `:855-866` not `:682-693`; `StartCommit`'s `||`
at `EnableWizardScreen.cpp:572-576` not `:501-505`. The `BossRandomizer.cpp`
citations in §2.3 (`:65`, `:141`, `:382`) are all still exact, and every
structural anchor the plan relies on — the `} // if (options.randomizeEnemies)`
brace, the `BossScalingMaps()` loop immediately after it, the `||` — still exists
and is still adjacent, so §3.2's call-site instruction remains executable as
written.

Two smaller ones: §6.1 attributes the heap-corruption SIGSEGV to `CLAUDE.md` §2;
it is `docs/build.md:54`. And §2.4/§3.1 model the new verifier on
`mergo_darkness_verify.py` and the new table on `EnemyExclusionList.h`, but
feature 033 has since shipped `CagedDogList.h` + `caged_dogs_verify.py` — a
baked placement table in a header with a Python mirror that parses the entries
rather than restating them, which is precisely this feature's shape and a closer
template.

**Confidence:** High.

---

### Finding 9 — §6.3 accepts a weaker identity than §6.2 case 8 establishes

**Severity:** Worth considering

**Where:** `plan.md` §6.3, first bullet: "NpcParam 252100 **or** the map's scaled
variant from case 8".

**Problem:** Because the scaling pass is unconditional, the value is not a choice
— it is 900014609 in m27_00_00_01, 900014611 in m32_00_00_01, 900014614 in
m24_02_00_01, 900014627 in m35_00_00_00, and 252100 in the two unscaled maps,
every time. Accepting either would let an output tree in which the scaling pass
silently stopped running pass the check.

**Recommendation:** Assert the exact expected value per map. §6.2 case 8 already
lists all six.

**Confidence:** High.

---

## 3. What looks good

**Every measurement about game data reproduced.** This is unusual and worth
saying plainly. I re-derived 79 tree-wide and 42 retail placements; the 2/60/3/14
per-flag counts; the exact seven-name emissary set
(`_0001,_0002,_0003,_0006,_0007,_0009,_0010` — the `_0004/_0005/_0008` gap is
real); the three `c2521_0000` placements with identical `252100/252100` and
entity ID 2410771; the five maps declaring `c2521`; the larva's full stat block
(2 HP, 18 echoes, teamType 26, hitHeight 1.0, hitRadius 0.2,
`behaviorVariationId` 25210, lot 28040); all 31 scaled variants identical to the
base row; the four zone-scaled ids; and lot 28040 as the sole source of item 4321
with `getItemFlagId` 50001205 and exactly 32 referencing rows. Not one was off.

**The EMEVD counts, including the correction.** §5.1 reports ~32-33 Shadow and
~26-28 Failure references where spec §4 says 37-39 and 34-37, explains the
methodological difference, and states that nothing depends on the exact figure. I
measured 33/33/32 and 26/28/27/27/26. The plan's numbers are the reproducible
ones, and it caught its own spec's looser figure without making a fuss about it.
The Rom-children figure — exactly 3 references each, against 30+ for the
scripted bodies — is the single most useful measurement in the document, and
§5.1's "if she spawns children dynamically, Easy Rom thins rather than empties"
is precisely the right thing to have written down before hardware.

**The ordering argument is correct and cheaply verified.** I checked the phase
enum (`EnemyRandomizer.cpp:241`) against §2.2's list — identical, in order. Boss
assignment including `AddTheRestInMap` (`:716`) completes before `WriteMaps`, so
the easy pass wins without a new phase; and the slot between the enemy loop's
closing brace and the `BossScalingMaps()` loop reproduces
`StartFunctions.cs:1239` → `:2457` exactly. §3.6 A1, A2 and A6 reject the three
alternatives for the right reasons, and A2 in particular resists the tempting
"run after scaling so the bytes are tidier" improvement on `CLAUDE.md` §7
grounds, with the measurement to show nothing is lost.

**The reference reading is accurate where it counts.** I read `EasyModes`
(`MainWindow.xaml.cs:934-1055`) and the four call sites
(`StartFunctions.cs:1239-1256`). The name lists, the map lists and the
if/else-if map branching are as described. `addedStoneGuyBool` is declared at
`FieldContainer.cs:60`, read once at `MainWindow.xaml.cs:750`, and assigned
nowhere — §3.3's claim is exactly right, and the observation that all three
`c2521` placements are identical so the defect is harmless is the correct
conclusion rather than the alarming one. A4's rejection of reproducing the
capture, and A3's refusal to normalise `m27_00_00_00`, both follow spec §10
faithfully.

**Risks I went looking for and found already answered.** The unconditional model
merge (§5.2) with the log-and-skip guard instead of the reference's
`KeyNotFoundException` — a justified divergence, well argued. The `NoneEnabled()`
picker guards being gated on `randomizeEnemies_`/`randomizeBosses_` so an
easy-only run passes through (verified at `EnableWizardScreen.cpp:534,549`). The
`RandInt` argument for the not-a-randomizer constraint. The defaults buffer
headroom (still 1024/4096, still ample). The font check against `Font8x8.cpp`'s
actual 42-glyph table rather than a list written elsewhere — I re-ran it; all
four labels and all four result lines are clean. And §5.7 is the right shape: a
short, specific argument that nothing here can make a run unwinnable.

**§7's honest counter-argument.** Recording the Shadows-only-first option, the
reason it was rejected, and the note that splitting after §6.2 costs nothing, is
better practice than either taking the split or not mentioning it.

---

## 4. What could not be verified

* **Everything in §5.1's headline risk.** Whether the fights end, whether the
  larva is hostile, whether it dies in one hit, whether Rom spawns children
  dynamically. Nothing in this environment runs the game. The plan says so and
  puts it first; that is the right handling.
* **Where the umbilical cord lands.** `getItemFlagId` 50001205 is a byte. Which
  of the two readings is true is hardware, and finding 3 makes it hardware *with
  drops off*.
* **"teamType 26 means non-hostile."** I confirmed 252100 carries teamType 26 and
  did not re-derive the full cross-map census behind §5.6's inference. The
  standing note on setting polarity applies and the plan cites it.
* **The C++ will match the Python mirror.** No host compiler; §5.6's last bullet
  states this accepted weakness correctly.
* **`docs/plans/mergo-darkness.md` D2/D4/D5 and `docs/plans/pickers.md` D12.** I
  verified the *current code* implements what the plan says they do — the
  `||` membership and the append-last row rule are both in the source with
  comments — but did not read the plan documents themselves.
* **Byte-level DCX output.** I did not generate a tree. Finding 2's compression
  point rests on `itemdata_verify.py:164`'s "only the compression wrapper
  differs" and on the fact that the port re-serialises and re-compresses every
  map, not on a measured diff.
* **`docs/randomization-feature-spec.md` rows 18-21.** Not re-read; the plan and
  spec agree on the grouping and the developer approved it.

---

## 5. Numbers and evidence

| Claim | Plan says | Verified value | Result |
| --- | ---: | ---: | --- |
| Total replacements tree-wide | 79 | 79 | Confirmed |
| Replacements in retail-loaded maps | 42 | 42 | Confirmed |
| Console counts, four settings on | 2 / 60 / 3 / 14 | 2 / 60 / 3 / 14 | Confirmed |
| `m27_00_00_01` matched | 2 | 2 | Confirmed |
| `m32_00_00_00` / `_01` matched | 30 / 30 | 30 / 30 | Confirmed |
| `m35_00_00_00` matched | 3 | 3 | Confirmed |
| `m24_02_00_00` / `_01` matched | 7 / 7 | 7 / 7, set as listed | Confirmed |
| `m27_00_00_00` `c2120` NpcParams | 212700 / 212710 / 212720 | same | Confirmed |
| `c2521` placements tree-wide | 3, all `c2521_0000`, 252100/252100, eid 2410771 | same, in m24_01_00_00/01/11 | Confirmed |
| `c2521` declared as enemy model | 5 maps, named | same 5 | Confirmed |
| NpcParam 252100 | 2 HP, 18 echoes, team 26, h 1.0, r 0.2, behav 25210 | same | Confirmed |
| Scaled variants identical to base | all 31 | all 31 | Confirmed |
| Scaled ids (+9/+11/+14/+27) | 900014609 / 611 / 614 / 627 | same | Confirmed |
| Lot 28040 sole source of item 4321 | yes, `getItemFlagId` 50001205 | yes, 50001205 | Confirmed |
| NpcParam rows referencing 28040 | 32 | 32 | Confirmed |
| `IsExcludedNpcRow` protects 252100 | yes | yes — **and only 252100 / 6071** | Confirmed, incomplete (finding 3) |
| Shadow entity IDs in m27 EMEVD | ~32-33 each | 33 / 33 / 32 | Confirmed |
| Living Failure IDs in m35 EMEVD | ~26-28 each | 26 / 28 / 27 / 27 / 26 | Confirmed |
| Rom children in m32 EMEVD | ~3 each | exactly 3, all 30 | Confirmed |
| `.msb.dcx` files in tree | 43 | 43 | Confirmed |
| `Font8x8.cpp` covers all four labels | yes | yes (42 glyphs: A-Z 0-9 space `( ) , -`) | Confirmed |
| Save / load buffers | 1024 / 4096, no growth needed | 1024 / 4096, ~630 worst case with 4 keys | Confirmed (the "near 450" figure is stale) |
| Target table row count | 7 | 6 | **Not reproduced** (finding 6) |
| `kItemCount` / `kSaveDataRowCount` | 15 → 19 (§4); 18 (§6.4) | 17 → 21 | **Not reproduced** (finding 1) |
| `kDisableMergoDarknessRow` | 12 | 11 | **Not reproduced** (finding 1) |
| Drill-in rows are last | 2 rows, last | 3 rows, with 2 toggles after them | **Not reproduced** (finding 1) |
| Spurious `boss_verify` V2 failures | 67 | 37 | **Not reproduced** (finding 5) |
| Survivors byte-identical to vanilla | asserted | 4 of 7 are zone-scaled | **Not reproduced** (finding 2) |
| Pattern over-reach across 43 files | none | `c1400` → 138, `c2500_000x` → 4-9 maps | **Not reproduced** (finding 4) |

---

## 6. Implementation handoff

| Dimension | Question | Assessment |
| --- | --- | --- |
| **Scope** | Bounded to one coherent pass, with exclusions stated? | Yes. One pass, one table, one call site, four flags. §4's "Explicitly not changed" list is concrete and §3.6 A5/A6 close the two directions scope could creep. |
| **Sequence** | Are §7's changes ordered and independently understandable? | One milestone, and the ordering *within* it is implicit in §4's file table rather than stated. Workable — the file table reads top-down as a dependency order — but the implementer has to infer it. |
| **Decisions** | Is every behaviour-affecting decision settled? | Yes. All four spec §10 decisions and all four plan §9 decisions are closed, and §3.3 explicitly marks the baked constants "settled". Nothing behaviour-affecting hides inside an instruction. |
| **Dependencies** | Are prerequisites and their ordering identified? | No — this is finding 1. The single stated prerequisite (016 lands first) describes a world that does not exist, and the check written to catch that returns a wrong answer against the real tree. |
| **Verification** | Can the implementer tell whether the milestone succeeded from §6 and §7? | Partly. §6.1 and §6.2 are executable; §6.3 as written fails on correct output (finding 2) and §6.2 case 4 fails on a correct table (finding 4). §7's single completion gate is clear. |
| **Separation** | Is the contract free of investigation, review history and superseded reasoning? | No. §2 (~120 lines), §3.6 (~40), §5 (~145) and §9's notes are investigation, rejected alternatives and planning history inside §1--§7. |
| **Handoff** | Could a fresh agent execute §1--§7 cold? | Not today. It would transcribe wrong row indices, and it must read ~400 lines of background to locate ~200 lines of instruction. |

### Conclusion

**Implementation handoff: NEEDS RESTRUCTURING** — for two of the four named
reasons: the implementation-facing part is **mixed with material that is not an
instruction**, and it is **missing something the implementer needs** (a correct
dependency statement and correct row arithmetic).

It is *not* because the plan knows too much. The investigation here is the
process working, and finding 2 exists because the plan did not carry *enough* of
what it knew into §6.3. Had `plan-evidence.md` existed for this item, the moves
below would be the whole of this section.

### Restructuring findings — moves and cuts

* **Move** `plan.md` §2.1--§2.4 into `plan-evidence.md` §E1 — it is orientation
  and measurement. Keep in the contract only the three facts §3 and §5 act on:
  the call-site anchor (`} // if (options.randomizeEnemies)` then the
  `BossScalingMaps()` loop), the unconditional model merge, and the
  settings-chain file list, which §4 already restates in full.
* **Move** `plan.md` §3.6's A1--A7 into `plan-evidence.md` §E2 — rejected
  alternatives are evidence, and none of the seven changes what the implementer
  types.
* **Move** `plan.md` §5.1's EMEVD derivation and the spec-count reconciliation
  parenthesis into `plan-evidence.md` §E3, keeping in §5 the one sentence the
  implementer and the hardware tester act on: three fields change, entity IDs and
  positions do not, and whether the fights end is unknown until §6.5 steps 2-4.
* **Move** `plan.md` §5.4's analysis of `identity_ok` and the `addtherest`
  exemption into `plan-evidence.md` §E3 — the instruction is §4's one-line
  `--easy` row.
* **Move** `plan.md` §6.2's per-case provenance sentences ("measured while
  writing this plan", the five map names in case 6, the 31-variant census in case
  7) into `plan-evidence.md` §E4, leaving each case as the assertion the verifier
  must make.
* **Cut** `plan.md` §9's "Notes carried from the decisions into the body" — it
  narrates how the plan changed, which is `log.md`'s job. The decisions table
  itself stays; it is binding.
* **Cut** `plan.md` §5.5 entirely and the 016 sequencing sentence at the head of
  §7 — the dependency no longer exists. The only implementer-facing residue is
  §4's row arithmetic, which finding 1 requires re-deriving in place.
* **Cut** the status paragraph at the head of `plan.md` ("The developer answered
  from a summary of the questions rather than from this document…") — that is
  review and approval history, and `log.md` carries it.

The §7 milestone itself should **not** split. It touches two new source files,
six existing ones and three tools, which is large, but the four settings share
one table, one function and one settings chain, and §7 already records the
Shadows-first alternative with reasons I accept. Splitting would renumber the
same UI rows twice for no hardware answer.

---

<!--

This review evaluates the plan. It does not rewrite or modify it.

The planner addresses findings and updates the plan separately.

-->
