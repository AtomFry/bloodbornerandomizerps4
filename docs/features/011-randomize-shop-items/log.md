# Feature 011 — log

Append-only. Every stage adds; none rewrites. `spec.md` and `plan.md` state what
is true now; this states how they got there.

---

## 2026-09-28 — stage B, the spec

Written by the `spec-author` subagent. Row 11 was chosen as the next item from
the remaining backlog on the grounds that the documented next block —
`EquipParamWeapon` movesets, rows 24/25 — already has a spec sitting at
`QUESTIONS OPEN`, and rows 19–21 are already covered by spec 018. Row 11 was the
next genuinely unspecced work, and it is reference-backed, writes a param the
port already writes, and has strong offline verification — which mattered
because the developer could not hardware-test that day.

**Four questions, all answered the same day, all taking the spec's own
recommendation:**

* **D1** — two settings rather than the reference's one, both in WEAPONS AND
  STARTING GEAR. The port had already split row 5's single reference checkbox
  into three positive toggles for the same reason.
* **D2** — the shuffle crosses shops, matching the reference.
* **D3** — once-only listings stay in the shuffle.
* **D4** — the two unidentified shop families are included, and hardware settles
  whether they are reachable.

**A developer question worth recording, because the answer is a fact rather than
a decision.** The developer asked whether there was any ambiguity about which
bucket an item belongs to, and offered to fall back to the reference's single
checkbox if the split complicated things. There is none:
`ShopLineupParam.equipType` states the bucket on every row, the 1288 rows
partition cleanly as 644 weapons / 220 armour / 424 consumables, and **no item
appears under more than one type**. The split costs nothing; each setting is a
self-contained permutation of its own bucket. D1 stands.

**What the investigation found that the backlog row did not say.** The row's
**Low** cost is right about effort and misleading about consequence: purchase
limits and currencies detach from items, and 28% of the consumable bucket sits
in shops nobody has confirmed a player can reach. Recorded as F14.

Status ended at `QUESTIONS ANSWERED — awaiting developer approval`, then
**`APPROVED` by the developer the same day**.

---

## 2026-09-28 — stage C, the plan

Written by the `planner` subagent against the approved spec.

**Two questions, both non-blocking. One took the recommendation and one did
not:**

* **D1 — taken as recommended.** The two new passes run after row 5's pass and
  before the two character-creation grants. Row 5 is shipped and
  hardware-proven, so its output stays fixed; the grants stay the run's last
  rolls, which is what spec 037 §7 recorded. The cost — turning either new
  setting on changes which weapon a grant draws for a fixed seed — is
  unavoidable in shape, because one shared RNG stream means whichever pass is
  later moves when an earlier one is toggled.
* **D2 — against the recommendation.** The plan proposed two hardware runs; the
  developer chose one, with everything on. The armour-only run was the only
  observation that would catch the two labels being wired to each other's flag,
  so that property now rests on `worlds_verify.py`'s field-for-field mapping
  assertion instead of on the console.

**Reconciliation.** D1 required only repointing §4.3's forward reference.
**D2 required a real edit to §6**: its hardware section had been written around
two runs and now describes one, and says explicitly that the mapping verifier
case must not be treated as optional, because it is carrying a property the
console will no longer check. Every surviving `§8` reference in the body was
checked and points at the **spec's** §8, which exists.

**Contract budget.** §1–§7 came to 421 lines against a nominal 400 — accepted.
For calibration against this repo's own plans: caged dogs 419, startup screen
404, worlds 553, randomizer settings UI 605, trick weapon 487.

**The planner found nothing wrong in the spec** and surfaced no spec gap. Every
count in spec §4 was re-measured against the vanilla tree and agreed.

**Three risks the plan surfaced that the spec had not:**

1. **F13 is worse than "the verifier is incomplete".** Besides SW-I4 firing on
   every changed armour or consumable listing, `starting_weapons_verify.py`
   prints `counts["shop"]` as "other shop weapon rows changed" — which would
   silently absorb ~600 new changes and hide a row-5 regression behind a
   plausible number.
2. **Ids collide across param tables.** Armour id `10000` is also an
   `EquipParamWeapon` row; consumable ids `1000`, `3000`, `7000` are weapon
   rows; `1000`, `1100`–`1400` and `3000` are also `EquipParamProtector` rows.
   Verified independently by the orchestrating session. So a check of the form
   "the new id exists in some equipment table" would pass a cross-bucket leak,
   and **per-bucket multiset equality is the load-bearing invariant** rather
   than any existence test.
3. **`drops_verify.py` and `hunter_tools_verify.py` will report FAILED on a
   combined run**, because each tolerates only its own param member. This is
   pre-existing with `RANDOMIZE SHOP WEAPONS` and is left alone; it is recorded
   so a red verifier during hardware testing is not misread as a regression.

Status ended at `QUESTIONS ANSWERED — awaiting developer approval`.

---

## 2026-09-28 — D3 folded in after the strategy was presented

The developer challenged the three risks reported alongside the Execution
Strategy, asking why they were not being fixed as part of this work. **Two of
the three already were**, and the report had mislabelled them: the misleading
`counts["shop"]` churn line is part of M3's first change, and the cross-table id
collision is not an open risk at all — it is the finding that *shaped* the
verifier, and `SW-I11` and `SW-I12` with selftest cases S4 and S5 exist because
of it.

**The third was genuinely out of scope, and the challenge showed D2 had
undermined its reasoning.** Leaving `drops_verify.py` and
`hunter_tools_verify.py` alone rested on §6 running each against a tree where
only its own member changed. D2 takes a single hardware run with everything on,
and against that tree there is no such thing — so both tools would have produced
**no signal at all**, not merely a confusing FAILED.

**D3:** both gain a `--shop-stock` tolerance flag, folded into M3 as change 4.
Same pattern M3 already adds to `starting_weapons_verify.py`, and the same one
row 38 added to three tools. The done-condition requires that every existing
case of both tools is unchanged either way — the flag adds tolerance, it does
not relax anything that already holds.

Edited in place: §3.2 (no longer out of scope), §5 (two files added at M3), §6
(the all-off-tree note replaced, and why D2 makes the flag necessary), M3 (goal
widened, change 4 inserted, verification extended). §1–§7 went from 421 to 432
lines.

---

## 2026-09-28 — stage E, all three milestones. Plan 011 fully built.

Built by the `implementer` subagent. **Stage D was skipped**, as for rows 27 and
37. All three milestones completed in one continuous run and **no stop condition
fired**, which is what the Execution Strategy's continuous mode with a final-only
gate calls for.

**What was built.** M1 the pass — `ShopLineup.h` carving the shop offsets, the
three `equipType` constants and the skip list out of `StartingWeapons.cpp`, plus
`ShopStock.{h,cpp}` with the two per-bucket permutations. M2 the two settings, both
config keys, the activation mapping and the report lines. M3 the verification —
`starting_weapons_verify.py` corrected and extended, and `--shop-stock` tolerance
added to `drops_verify.py` and `hunter_tools_verify.py` per spec D3.

**The measured table matched spec §2 exactly before any edit** — 1288 listings
splitting 644 / 220 / 424, 27 skipped, 220 and 397 shufflable across 44 and 57
distinct ids.

### The stale byte arithmetic, which was the one real hazard going in

Row 27 landed and was committed **after** this plan was approved, so §3, §5, §6, M3
change 7 and §9 P4 all carried **728 → 775** and **779 → 826**, computed from a
pre-row-27 baseline. The dispatch told the implementer the measured post-27 figures
— a **743**-byte block and a **794**-byte worst case — and instructed it explicitly
**not** to adjust the assertion to whatever the code emitted, since that assertion
is pinned in the failing direction on purpose and fitting it to the code is the one
way the check could be defeated.

It derived **790 / 841** from measurement plus the plan's own +47 (24 + 23), which
is what the orchestrating session had predicted independently before dispatch. Both
are **exact equalities** in `pool_verify.py` — `settings_block == 790` and
`worst == 841` — so agreement here is two independent derivations meeting, not a
number fitted to an outcome. 234 bytes spare in `char buf[1024]`.

This is the shared-buffer collision plan 013 anticipated, arriving exactly as
predicted and costing one dispatch note.

### Five deviations, all in plan §10

The three worth reading:

1. **The config rebase above.**
2. **The call site now sits between row 5 and row 27's team-type block**, rather
   than directly before hunter tools as §4.3 describes, because row 27 inserted
   itself in between. Verified independently: `StepItemData` runs starting weapons
   (`:1056`) → **shop stock (`:1099`)** → team type (`:1125`) → hunter tools
   (`:1144`) → trick weapon (`:1184`) → left-hand weapon (`:1208`). **Plan D1's
   actual requirement holds literally** — after row 5's pass, before the two
   character-creation grants — so row 5's rolls are unmoved and the grants remain
   the run's last draws. The interposed team-type pass is harmless because it draws
   no randomness, which its RNG-free signature enforces by type.
3. **Selftest case S10 was built from genuine permutations** — a swap rather than
   an overwrite, and "only the last 20 of 220 moved" rather than "200 reverted".
   Taken literally, both of the plan's phrasings break SW-I11 and the case would
   have been rejected for the wrong reason.

### Verification, re-run independently by the orchestrating session

Not taken from the report. `starting_weapons_verify.py selftest` **45/45** (was
21/21, every pre-existing case unchanged); `pool_verify.py selftest` **98/98** with
790 and 841 both asserting clean; `settings_ui_verify.py` **113/113**, `WeaponsGear`
now listing **8** settings in spec order with `RANDOMIZE SHOP ARMOUR` and
`RANDOMIZE SHOP ITEMS` in place, widest pane row still 649/700 px;
`worlds_verify.py selftest` **29/29** and `all` passing; `drops_verify.py selftest`
**16/16** (was 12/12); `hunter_tools_verify.py selftest` **29/29** (was 25/25);
`team_type_verify.py selftest` **20/20** unchanged, which is what says row 27 still
produces what it did; `itemdata_verify.py roundtrip` PASS byte-identical at
28,309,992 bytes; `ui_scroll_verify.py` PASSED.

`make` clean with **zero errors or warnings**; `.pkg` 7,143,424 bytes at 22:26,
postdating the newest changed source at 22:25:40.

**Also confirmed independently:**

* **Every changed file is named in plan §5 — zero unnamed files.** The only extras
  in the tree are `implementation-report.md`, the expected output, and `plan.md`
  itself for §10.
* **`plan.md` §1–§7 is byte-identical.** Every section heading sits on its
  pre-dispatch line number, and `git diff -U0` puts both hunks at lines 510 and
  516 — inside §10, which starts at 508.

### Awaiting hardware

Ready for hardware testing per `CLAUDE.md` §3, not done. The three `verify`
subcommands need an output tree only the console produces, so the real-tree
assertions — member list and sizes unchanged, only `ShopLineupParam.param`
differing, row id set unchanged — are **unverified**.

**The first hardware check is the one that can kill the feature:** Blood Vials and
Quicksilver Bullets still on sale, at vanilla prices, unlimited. The 27-entry skip
list exists to guarantee that, and if it has a hole the run is unplayable rather
than merely shuffled. After that: the two progress lines reading about 215 and
about 388, both lists scrambled but coherent, the currencies crossed, and which
shops are actually reachable — which settles spec F10 behind D4.

**Determinism is also a hardware question.** Nothing automated here establishes
that one seed and one set of settings reproduce the same shops, nor that row 5's
rolls are unmoved for a fixed seed; the pass-order check above is a static argument
for it, not a measurement.

### Noticed and deliberately not fixed

`StatusText`'s `ItemData` label still reads `RANDOMIZING ENEMY DROPS` for every
param feature (§3.2, out of scope). `docs/screen-text-inventory.md` and
`docs/user-guide.md` are now stale on the two new labels, and the settings-UI
spec's Appendix A has no rows for them — §4.4 scoped that edit to §7.1 only, and
nothing machine-checks Appendix A. All three belong to the documentation stage.

---

## 2026-09-29 — hardware test passed. Row 11 is DONE.

The developer reports the feature tested and working on the console. The backlog
row is **DONE** — `CLAUDE.md` §3's bar, implemented and confirmed on hardware.

The check that mattered was the first one: **Blood Vials and Quicksilver Bullets
still on sale at vanilla prices, unlimited.** The 27-entry skip list is what
guarantees that, and a hole in it would have made a run unplayable rather than
merely shuffled. It held.

Nothing in the code changed for this entry; it records the status transition and
the fact that the console, not a verifier, is what moved it.

**Still not established, and worth knowing it is not:** the three `verify`
subcommands were only runnable if the generated trees were saved to `data/runs/`.
If they were not, then the real-tree assertions — member list and sizes unchanged,
only `ShopLineupParam.param` differing, row id set unchanged — remain unverified,
as does determinism across a fixed seed. The pass-order argument in the stage E
entry is static reasoning, not a measurement.
