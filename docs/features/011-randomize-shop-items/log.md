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
