# Log — 018 Easy Shadows / Easy Rom / Easy Failures / Easy Emissary

Append-only. Newest entry at the bottom.

## 2026-09-19 — Stage D: plan review

`plan-reviewer` reviewed `plan.md` (status QUESTIONS ANSWERED) against `spec.md`
and the repository. No `plan-evidence.md` exists for this item — it predates the
two-document convention — so the plan was reviewed as the sole planning artifact.
Review written to `plan-review.md`.

**Verdict: CHANGES REQUESTED. Implementation handoff: NEEDS RESTRUCTURING.**

Blocking findings:

* **F1 — §4/§5.5 row arithmetic is written for a tree that never existed.** The
  plan assumes feature 016 lands first (`kItemCount`/`kSaveDataRowCount` 15 → 19,
  `kDisableMergoDarknessRow` 12). 016 was retired by feature 032 D1, and 032/033/034
  have since appended rows. Actual values are 17, 17 and 11. §5.5's precondition
  check tests for (12, 15) or (11, 14) and matches neither. §4 also prescribes
  inserting the four new rows *before* the drill-ins and renumbering them, which
  inverts the append-last invariant stated twice in `SetupDefaultsScreen.h`.
* **F2 — §6.3's "byte-identical to vanilla" assertions fail on a correct
  implementation.** The `BossScalingMaps()` loop in `EnemyRandomizer.cpp` runs
  outside the `randomizeEnemies` gate, so NpcParamIDs in stat-scaled maps are
  rewritten in any output tree. This is the documented retired-`maidens` bug
  recorded in `pool_verify.py` `_frozen`.

Non-blocking: F3 scaled larva rows 900014601–900014631 absent from
`IsExcludedNpcRow`; F4 §6.2 case 4's tree-wide claim is per-map; F5 §5.4's "67
spurious failures" does not reproduce (37 does); F6 six rows described as seven;
F7–F9 `enemyModelIndex` already in scope, stale line citations, weaker identity
assertion in §6.3 than §6.2 case 8 supports.

§6 restructuring — moves and cuts only, no additions: §2.1–§2.4, §3.6 A1–A7,
§5.1's EMEVD derivation, §5.4's `identity_ok`/`addtherest` analysis and §6.2's
per-case provenance move to a new `plan-evidence.md`; §9's "Notes carried from
the decisions", §5.5 and §7's 016 sequencing, and the status paragraph are cut.
The reviewer agrees §7's single milestone should not split.

Verified independently before relaying: F1 in full — `kItemCount` = 17
(`SetupDefaultsScreen.h:36`), `kSaveDataRowCount` = 17 and
`kDisableMergoDarknessRow` = 11 (`EnableWizardScreen.cpp:39,59`), the 016
retirement recorded in `SetupDefaultsScreen.h`'s own comment, and the append-last
invariant at `SetupDefaultsScreen.h:59,66-80`. F2's mechanism — the scaling loop
at `EnemyRandomizer.cpp:855` sits outside `if (options.randomizeEnemies)` — and
the `_frozen` precedent at `pool_verify.py:188-200`; the per-map scaled-ID
mapping was not re-derived. F3–F9 were not spot-checked (non-blocking).

Findings aimed at the **spec**, not the plan:

* Spec §4's EMEVD figures (37–39 Shadows, 34–37 Failures) do not reproduce by the
  plan's method; 33/33/32 and 26–28 do. The plan reconciles this, but the spec
  still carries the looser numbers.
* Spec §8 criterion 6 — "`NpcParam` is unchanged … whether or not enemy-drop
  randomization is on" — is not achievable as stated. The 31 scaled variants that
  carry the drop in the four retail-loaded maps are outside `IsExcludedNpcRow`, in
  the port and in the reference alike. The criterion holds only for row 252100.

Could not be checked: anything requiring hardware (fights ending, hostility, cord
location), the teamType-26 cross-map census, C++/Python mirror agreement,
byte-level DCX comparison against a real generated tree, and the three
`docs/plans/*.md` precedent documents.

Plan status unchanged by this review.

## 2026-09-19 — Stage C: plan refined against the stage D review

Feedback, verbatim: *"see plan-review.md in 18's folder"* — the review is the
feedback, so the planner worked through all of it: F1, F2, F3–F9 and the ten §6
moves and cuts. Spec was confirmed `APPROVED` and was **not** edited.

**Classification: an already-approved decision changed.** Status accordingly set
to `QUESTIONS ANSWERED — awaiting developer approval`; re-approval is required
before `/implement`.

What changed at decision level (plan §9 only; no spec §10 decision touched):

* Old §9 decision 1 — *"plan 016 lands first; build on the tree 016 leaves
  behind"* — **deleted**. 016 shipped and feature 032 D1 then retired the setting
  outright, so the decision has no referent.
* Old §9 decision 2's **position clause changed**: the four rows were to sit after
  `ENABLE MERGO DARKNESS` and ahead of the drill-ins; they now append last at rows
  17–20 after `kStartWithHunterToolsRow`, `kItemCount`/`kSaveDataRowCount` 17 → 21,
  no existing constant renumbered. The reasoning survives as P1.
* Decisions 3 and 4 survive unchanged as P2 and P3. New planner decision **P4**:
  `ApplyEasyModes` scans for the `c2521` model index itself rather than taking the
  caller's `enemyModelIndex` (answers F7).

Corrections of fact, nothing decided: row arithmetic re-derived against live
headers; §5.5's unmatchable precondition check cut with the whole 016 dependency;
output-tree assertions restated in `pool_verify._frozen`'s form with the written
`NPCParamID` asserted exactly per map (F2); selftest case 4 scoped per-map (F4);
`boss_verify` false positives 67 → 37 (F5); "seven rows" → six (F6); citations
refreshed (F8); the weak "252100 or scaled variant" replaced by exact values (F9).
The planner also found a survivor the review's table missed — **Rom herself scales,
510000 → 995107013** — and two tool updates no finding named: `pool_verify.py`'s
`defaults.cfg` worst case is an exact equality (611) that fails the moment a key is
added, so it goes to 669; and `ui_scroll_verify.py`'s `Progress log` budget 16 → 20.
One behaviour clarification B13: the easy pass is the last writer of its
placements, so an easy setting also wins over `ENEMIES SKIPPED` (feature 032,
which shipped after the spec was approved).

Restructuring: `plan-evidence.md` created (445 lines, E1–E7); all ten §6 moves and
cuts landed there or were cut. The contract (§1–§7) went **735 → 413 lines**, and
the plan was renumbered onto `docs/features/_templates/plan.md`'s sections. §7
remains a single milestone, per the review.

Marked `STALE`: `plan-review.md` (it reviews the pre-refinement plan and its
section references no longer resolve). `implementation-report.md`,
`code-review.md` and `test-report.md` do not exist for this item.

Spec findings — recorded here, **not** written into the spec:

* §8 criterion 6 is assertable as literally written (row 252100 and lot 28040 are
  untouched, drops on or off) and the plan asserts it. What does not hold is the
  property a reader infers: in the four retail-loaded scaled maps the larvae carry
  rows 900014601–900014631, outside `IsExcludedNpcRow`, so with drops on their
  drop is randomized and lot 28040 enters the shared pool 31 times. The plan
  states this as fact, asserts it in selftest case 9, and pins the cord hardware
  test to drops off. Whether the spec should say so is the developer's call.
* §4's EMEVD counts do not reproduce: measured 33/33/32 (Shadows) and 26/28/27/27/26
  (Failures) against the spec's 37–39 and 34–37. Rom's children are exactly 3 each.
  Different counting method; no §2 conclusion depends on the figure.
* §4's "42 in retail-loaded maps" assumes one Byrgenwerth variant loads — `names.py`
  tags the pre-DLC Forbidden Woods and Upper Cathedral Ward `unused` but not
  `m32_00_00_00`. Nothing in the plan depends on it; both variants are patched.

## 2026-09-19 — Stage E: implemented milestone 1

`implementer` built **milestone 1**, the plan's only milestone, against
`plan.md` (status Approved, §8 empty). `plan-review.md` was marked STALE by the
stage C refinement and was excluded from the handoff; the plan went in as the
sole contract. **The milestone completed at its completion gate. No stop
condition fired.**

The tree already carried uncommitted work from features 033 and 034 touching
eight of §5's files. That state was recorded before the first edit — a diff and
per-file hashes — so this milestone's changes could be separated from it
afterwards. Nothing was stashed, reset or cleaned.

**Built:** `EasyModes.h` (six-row table, three baked constants, both header
notes), `EasyModes.cpp` (the pass), the option and result-count members, the
call site, four defaults fields with load and save, rows 17–20 on both screens,
`easy_modes_verify.py`, and the `boss_verify.py --easy` / `ui_scroll_verify.py`
/ `pool_verify.py` updates §5 lists. No file outside §5's table changed.

**Deviations, all recorded in plan §10:** `kEasyModeModelName` declared
`const char* const` (the plan's spelling is a duplicate symbol across two TUs);
`easy_modes_verify.py verify` takes an extra `--no-randomizers` argument, which
is how the caller states which of §6's two tree kinds it holds; one already-stale
buffer comment figure in `RandomizerDefaultsStore.cpp` corrected 585 → 669; and
`boss_verify --easy` imports `easy_modes_verify.parse_table` rather than parsing
`EasyModes.h` a second time. Decisions the plan left to the implementer are in
implementation-report.md §3 — an enum flag in the table rows, one parts pass per
enabled row, an `Any()` helper, the per-replacement log format, deriving the
retail-loaded 42 from the scaling map list, and a synthetic model entry in
selftest case 12.

**Verification.** Clean rebuild (`rm -rf src/x64 && make`) produced the `.pkg`
with zero warnings and zero errors. `easy_modes_verify selftest` 25/25 (all
twelve §6 cases), `pool_verify selftest` 87/87, `boss_verify selftest` 5/5,
`drops_verify selftest` 6/6, `ui_scroll_verify` PASS. Measured placement counts
match §4.1 exactly: 2 / 30 / 30 / 3 / 7 / 7 = 79 tree-wide, 42 retail-loaded.

**Not run:** `easy_modes_verify verify <V> <B> …` and `boss_verify verify <V> <B>
--easy` against a real easy tree — both need PS4 output that does not exist yet.
The implementer smoke-tested the `verify` path against an existing enemies-only
run tree and tested `--easy` in-process against a synthetic easy tree (42
failures without it, 37 of them `V2 non-boss placement changed` exactly as
evidence M14 predicts; 0 with it).

**Independently confirmed by the dispatching session**, rather than taken from
the report: the `.pkg` and `eboot.bin` exist and postdate every source file, and
`EasyModes.o` is in the object tree; all five verifier commands re-run with the
reported results; the set of changed files is exactly §5's table plus the report
(`git` against the recorded baseline); plan §1–§9 are byte-identical to the
pre-implementation file and only §10 was written; and the §3.1 invariants the
milestone could have broken all hold — no `RandInt` or SDL2 in the new pass, it
includes only `Msb` and the log, the call sits between
`} // if (options.randomizeEnemies)` and the `BossScalingMaps()` loop, exactly
three setters fire per hit, the table holds six rows keyed by exact map name
with `m27_00_00_00` absent and commented, and rows 0–16 kept their indices on
both screens with the four new rows last in all three `items` vectors.

**Noticed, pre-existing, deliberately not fixed:** `boss_verify.py`'s
`load_scaling_ids` reads `<repo_root>/PS4/bbrandomizer/src/Randomizer/NpcScalingTable.h`,
a path that has not existed since the tree moved to `app/`. Confirmed present at
HEAD and unrelated to this feature. It fails silently and returns an empty
scaling table, which on an easy tree costs five spurious failures. Worth fixing
before hardware step 1, since that step runs `boss_verify verify`.

**Now awaiting hardware test** — §6's Hardware steps 1–8, three trees (vanilla
V, seed S all-off A, the same seed all-on B). A clean build and green selftests
mean ready for hardware testing, never done.
