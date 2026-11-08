# Feature 040 — log

Append-only. Every stage adds; none rewrites. `spec.md` states what is true now;
this states how it got there.

---

## 2026-09-29 — stage B, the spec

Written by the `spec-author` subagent. Row 40 was chosen as the next item after
row 39 was **ruled out on the day**: 39 had looked unblocked, but the developer
retracted the rune-probe result as a misremembering, so the probe is still in the
build and still unanswered. The spec author was told explicitly to treat accessory
slots as unknown and not to build on them.

The row's own framing — *"the fields are known and the picker machinery exists;
the work is scope, not mechanism"* — is the one thing the investigation did **not**
have to correct. What it understated is that the *shape* of the armour picker is
itself a mechanism question, because armour has four slots and a picker draws one
thing.

### What the investigation established that the row did not say

* **The armour route is close to proven, and the row's own evidence was the weak
  version of it.** The row argued "set on 1557 rows so plainly read". The real
  evidence is that the **22 player-origin rows themselves** name Black Hood /
  Foreign Garb / Sullied Bandage / Foreign Trousers in those four fields — exactly
  what a new hunter is visibly wearing. So the route is exercised *on the very rows
  this feature writes*. Verified independently: two distinct tuples across the 22
  rows, `[230000, 231000, 232000, 233000]` on the 2000 block and
  `[-1, 231000, 232000, 233000]` on the 3000 block and 3500/3501. **There is no
  probe milestone here**, unlike rows 37 and 38.
* **`EquipParamProtector` has no stat requirements at all.** Verified
  independently against the paramdef: **zero** `proper*` fields on the protector
  row against **four** on the weapon row, and Bloodborne has no equip load. So the
  requirement-rewrite machinery that was most of rows 37/38's risk **is not
  involved** — the single biggest cost difference, and the row did not know it.
* **The wearable set is 124 pieces in 36 outfits, 25 complete**, derived by two
  independent tests that agree on every row: 273 rows → 21 `NOT NEEDED`
  placeholders dropped → 252 in the four wearable slots → 126 named → 124 after
  dropping the two rows the game marks wearable by neither sex (Crown of
  Illusions, Enlarged Head, both cut). Separately, **120 of the 124 are obtainable
  in a shop or a pickup and the only four that are not are exactly the starting
  clothes**; not one of the 126 unnamed rows is obtainable anywhere.
* **The game names every piece and names no outfit.** No set-name text, no set
  field. This became load-bearing in D1.
* **Nine free inventory slots per origin row, seven with `START WITH HUNTER TOOLS`
  on** — a hard measured bound the row did not have.
* **Quicksilver Bullets is not a "consumable" by any derivable rule**: ammunition
  carries no effect id, so it falls outside the 40 named+obtainable+stackable+
  effect-carrying rows and any rule must name it explicitly.
* **The coffins are shop rows, not template rows**, and row 5 writes exactly five
  of them. Its pools measure 76 melee / 14 firearms against the shipped picker's
  78 / 14 — a real two-row disagreement, being the base Beast Claw and the Uncanny
  Saw Spear.
* **A free answer to row 34's open item O2.** The 2000 and 3000 blocks differ in
  `equip_Helm` (230000 vs −1) **and** in `secondaryItem_01..06` (−1 vs 0), which
  `CharaInitRows.h` does not mention. A vanilla character's bare head therefore
  identifies the **3000 block as live**, observable with no build at all.

### A defect found in a shipped generator, verified independently

`app/tools/gen_weapon_table.py:74` sets `LOT_CATEGORY_WEAPON = 1`. On real data
**category 1 is armour and category 0 is weapons** — measured here at 99 protector
ids vs 2 weapon ids for category 1, and 63 weapon vs 2 protector for category 0.
(The spec author's counts differ by one or two because ids collide across param
tables, a fact spec 011 established; the split is stark enough either way.)

**Row 037's 78-row table is unaffected** — all 78 are in a shop listing, so the lot
half contributed nothing — but the generator's header claims two independent
obtainability tests where it has one and a half, and **an armour generator copying
the pattern would silently read the weapon category.** Which is exactly what row 40
would do. Not fixed here; it belongs to whoever builds the armour generator.

Also noted: `CharaInitRows.h`'s block comment says the two ten-row blocks differ
"only in `equip_Helm`". True as scoped to the fields either shipped feature
touches, but misleading to a reader looking for the whole difference.

### The four questions, and what happened

**D2, D3 and D4 took the recommendation. D1 did not.**

* **D1 — four per-slot armour pickers, against the spec's recommendation of one
  36-row outfit picker.** The developer chose mix-and-match: a player can ask for
  the Crowfeather hood over Gehrman's coat, which no outfit picker can express.
  Counts 33 / 34 / 26 / 31, **summing to exactly 124**, so nothing wearable is
  hidden. Accepted cost: four tables, four keys, four UI rows, four draws, four
  frozen orders; 216 bytes against 62; `WEAPONS AND GEAR` from eight settings to
  thirteen. **Cost avoided, which the spec had understated in arguing for the other
  option:** no outfit label has to be invented, since every row of all four pickers
  is a piece the game names. Measured by the orchestrating session before
  recording: all five labels fit, widest `START WITH BODY ARMOUR` at **609 px** of
  700, under the app's current worst row at 663 px.
* **D2 — one supplies toggle, 20 vials and 20 bullets.** No count, no picker. 20 is
  correct whether the engine clamps an over-cap count or spills it to storage,
  which cannot be settled offline.
* **D3 — the coffins are out**, filed as their own row. The only part of row 40
  that could have broken something shipped, and the only part needing a precedence
  rule. Keeping it out leaves the feature additive throughout.
* **D4 — offer everything, no curation.** The question asked whether a 36-row
  outfit picker should include the 11 outfits the game ships incomplete. **D1
  dissolved it**: per-slot pickers have no notion of an outfit, so all 124 pieces
  are independently offerable. Recorded as the standing principle rather than as an
  instruction.

### What step 7 reconciled — more than usual, because D1 went the other way

The spec's §1, §2, §4.3, §4.4, §4.7, §6, §7 and §8 had all been written to the
single-outfit-picker recommendation. Edited in place:

* **§2** — the settings table replaced with the four per-slot rows; the "36
  outfits" framing replaced with the four counts summing to 124; the
  "what it takes away" paragraph rewritten per-slot, noting that the head slot is
  the one place the feature *adds* rather than replaces, because the 3000 block's
  head is empty; and the consequence stated plainly that **matched outfits are no
  longer something the UI knows about**.
* **§4.7** — picker sizes (four small lists, not one 124-row list); the byte
  arithmetic **re-measured** against post-row-13 figures to **1285 / 1336 with 712
  bytes spare**, with a note that stage C should re-measure rather than trust it,
  since rows 11, 13 and 27 each rebased the previous feature's numbers; and the
  category count with the measured label widths.
* **§6** — in-scope rewritten to the four pickers; coffins moved under D3;
  matched-outfit grouping added as explicitly out of scope.
* **§7** — frozen order is now four independent orders; determinism is now four
  draws that must be taken in a fixed slot order, so ticking headwear cannot move
  which gloves a seed produces; the invented-label exception deleted, since D1
  removes it; the "four features write these rows" constraint updated to six, and
  the rune probe noted as still in the build and still unanswered.
* **§8** — the table criterion now asserts **four tables whose counts sum to 124**,
  which is what says no piece was dropped between them or counted twice; the grant
  criteria rewritten per-slot, with a new one requiring that changing what is
  ticked in one slot must not change which piece a fixed seed draws in another.
* **§9** deleted entire, matching specs 011, 013, 027 and 037; its two surviving
  references repointed at §10.

Status ended at `QUESTIONS ANSWERED — awaiting developer approval`.

### Could not be established offline

Whether the engine clamps or spills an inventory count above the carry cap (the
data proves the field accepts it, 4,505 shipped instances; nothing says where the
excess goes — D2 sidesteps it). Whether a granted piece is *owned* as well as worn,
and whether the four displaced starting garments stay in the inventory. Which of
the two origin blocks the game instantiates — §4.1 infers the 3000 block from the
bare head, which rests on observing a vanilla character rather than on data.

Status set to **APPROVED** by the developer the same day.

---

## 2026-09-29 — stage C, the plan

Written by the `planner` subagent against the approved spec. **Four milestones,
continuous, no gates** — and no gate is *available* to propose, because there is no
route to probe: the 22 rows this feature writes already name the four garments a
new character wears. An optional gate after M3 was considered and rejected on the
grounds that nothing in M4 depends on its answer.

**Two questions, both non-blocking, both taking the recommendation** — D1 and D2 in
§9. Reconciliation changed nothing in §1–§7, because both answers matched what the
body already described; only §8, §9, the status line and four internal `§8`
references moved.

* **D1 — ship the two duplicate-name pairs.** `HUNTER GARB` is ids 71000 and
  281000; `STUDENT UNIFORM` is 191000 and 301000. Spec §7 forbids inventing a
  label and D4 requires every wearable piece offered, and only listing both
  satisfies both. The generator asserts the duplicate set is exactly those two
  pairs, so a third is a stop condition.
* **D2 — fix `gen_weapon_table.py`'s `LOT_CATEGORY_WEAPON`, in M1.** It reads 1;
  category 1 is armour and 0 is weapons. **The orchestrating session proved the fix
  inert before the decision was taken**: with the constant at 1 and again at 0,
  `gen_weapon_table.py <vanilla> --check` prints `PASS`, so the derived 78-row set
  is byte-identical either way — all 78 rows are in a shop listing, so the lot half
  never contributed. The generator was then restored to its committed state.

**Three errors the planner found in the approved spec, two of which were verified
independently here and all three of which have been corrected in `spec.md`:**

1. **§8 carried an unsatisfiable acceptance criterion.** It required "every
   excluded row is both unnamed and absent from every shop and every pickup" —
   but Crown of Illusions (250000) and Enlarged Head (430000) are **named and both
   obtainable**, each present in an `ItemLotParam` slot with `lotItemCategory 1`
   (verified). They are excluded on the sex test alone. **A generator built on that
   criterion would have refused to write.** Replaced with the two-sided equality:
   `included − obtainable == {the four starting garments}` and
   `obtainable − included == {250000, 430000}`. The 124 do not change.
2. **§4.2's effect premise was false.** Not "exactly one named row carries a
   permanent effect, and it is the Enlarged Head": `residentSpEffectId2` is
   uniformly 6300 across the included pieces, and the one row carrying a value at
   `residentSpEffectId` in the wearable both-sexes set is a different row
   (verified — 130 of the 133 rows my filter reached carry 6300 at offset 40). The
   spec's **conclusion** survives, which is the part the feature relies on.
3. **§4.7's picker paging was wrong** — the instruction-line layout fits 11 rows,
   not 12, so three of the four lists are three pages and one is four.

Also re-measured: §4.7's byte arithmetic of 1285 / 1336 is 8 bytes off; the plan
states the **rule** (`baseline + 239`) rather than a number, with the
row-13-M5-has-landed branch giving 1255 / 1306. That is the right framing, and it
is the third consecutive feature whose baseline moved during its own specification.

**Contract budget.** §1–§7 came to **492 lines** against a nominal 400 — the top of
this repo's band (037 was 487, 013 477, 011 432, worlds 553, settings UI 605) and
accepted.

**The risk most worth weighing before approval, in the planner's own assessment and
mine:** the four-draw independence requirement is the one thing a natural
implementation gets wrong. Copying `TrickWeaponGrant.cpp`'s single
`uniform_int_distribution` four times fails twice over — an empty slot draws
nothing and shifts later slots, and `uniform_int_distribution` consumes a
*variable* number of raw words as the candidate count changes. The plan mandates
exactly four raw `rng()` words whenever anything is ticked, indexed `% k`, with a
return before the first call when nothing is. **The wrong implementation compiles,
passes every other check, and shows up only as "two runs on one seed differ".**

**A pre-existing defect the survey turned up, not caused here and not fixed here:**
no verifier tolerates the rune probe's accessory writes, so
`hunter_tools_verify.py verify <vanilla> <output>` against any real tree built with
`START WITH HUNTER TOOLS` on already reports `H-I3 row N changed outside item_*`
22 times. Worth knowing before the hardware test, because it will look like this
feature broke it.

Status ended at `QUESTIONS ANSWERED — awaiting developer approval`.

Status set to **APPROVED** by the developer the same day.

---

## 2026-09-29 — stage E. Stopped once, then completed all four milestones.

Built by the `implementer` subagent across **two runs**. Stage D was skipped, as
for rows 11, 13, 27 and 37.

**Run 1 built M1 steps 1-2 and then correctly stopped**, firing three of §7's stop
conditions at M1 step 3. It did not work around them, which is the behaviour the
stop conditions exist to produce. **Run 2 completed M1 steps 4-6 and M2-M4.** No
stop condition fired in run 2.

### The stop, and what it found

M1 step 3 is §9 D2 — correct `gen_weapon_table.py`'s `LOT_CATEGORY_WEAPON` from 1
to 0. **The orchestrating session had certified that change inert and was wrong.**
It proved `gen_weapon_table.py --check` passes at both values and generalised from
one consumer; `gen_left_hand_table.py` imports `obtainable_ids()` from the same
module, and for the left hand obtainability GOVERNS rather than cross-checking.

What the implementer found, and the orchestrating session then verified
independently: **row 38 shipped two rows short.** The Torch (20100000) and the
Wooden Shield (19000000) are in no shop and in `ItemLotParam` category 0, so the
wrong constant made both fail obtainability outright. Fixed as its own piece of
work before run 2 — see `docs/features/038-start-with-left-hand-weapon/log.md`.

**The lesson is about the shape of the proof, not the constant.** "This change is
inert" needed every consumer of the shared function checked, which is one `grep`.
One consumer was checked and the conclusion was stated as proven.

### §4.4 implemented exactly, and verified at the source

The four-draw mechanism was the identified pre-approval risk. Confirmed by reading
`ArmourGrant.cpp`: `const uint32_t word = (uint32_t)rng();` unconditionally per
slot in slot order, `if (candidates[slot].empty()) continue;` **after** the draw so
the word is discarded rather than never taken, and `word % (uint32_t)k` indexing.
The file's single occurrence of `uniform_int_distribution` is in a comment saying
why it is not used. A15 asserts independence across 4 slots × 5 tick counts × 4
neighbour patterns, asserts the draw count is `{4}` or 0, and greps the
comment-stripped source for the distribution.

### Deviations worth reading, of twelve in plan §10

* **§6's A12 could not be built as written.** It demanded identical *bytes* for
  supplies + hunter tools either order; both use `FirstEmptySlot`, which §3.1
  mandates, so whichever runs first takes slots 1-2. Byte equality would require
  fixed slot indices — the one arrangement that lets two features collide. A12 now
  asserts identical **contents** as a multiset, plus no write lost and the Mark
  intact. The weaker assertion is the correct one.
* **`--supplies` was meaningless without a new invariant.** `hunter_tools_verify`
  already tolerated the whole item array, so a supplies run passed *without* the
  flag and a supplies run that displaced a tool passed *with* it. H-I7 added.
* **The implementer destroyed the orchestrating session's uncommitted row-38 fix**
  with a `git checkout --` and reconstructed it from the work order. Re-verified
  here: the constant, the 16-row table and `pool_verify`'s header-derived lengths
  all survived, and the reconstruction is **stronger** than the original — L4 now
  asserts the two tests agree in *both* directions where the original asserted one.

### A defect in row 13 that this run surfaced, and that the orchestrating session had missed

`worlds_verify.py all` failed on `the options mapping is field for field the
wizard's`. **Row 13 never added its eight `options.bossInsertion.*` mappings to
`WIZARD_OPTIONS_MAPPING`** — `bossInsertion` appeared zero times in that file.

Not row 40's doing, and confirmed so: at `HEAD` the case passes, and the extracted
list minus the eight `bossInsertion.*` entries equals the frozen list exactly.

**Why it was missed is the part worth keeping.** Plan 013 §6 names
`worlds_verify.py selftest`, and that mode runs only the 29 manifest cases — it
never reaches this check, which lives in `source_cases()` and runs only under
`all`. **Row 27's own log records this exact trap as a deviation**, and the
orchestrating session ran only `selftest` after row 13 regardless. Fixed here:
the eight pairs inserted at indices 19-26, matching source order, and
`worlds_verify.py all` now reports **36 fields** and all checks passing.

### Verification, re-run independently by the orchestrating session

`make clean && make` from scratch, **zero errors or warnings**, 7,143,424-byte
`.pkg`. `starting_gear_verify` **30/30**; `pool_verify` **120/120** with the byte
targets landing at exactly **1279 / 1330**; `settings_ui_verify` **116/116** with
`WeaponsGear` at **13** settings carrying all five new rows and the widest pane row
still 663 px, so the new labels did not become the pane's constraint;
`hunter_tools` 45/45 (was 29); `trick_weapons` 62/62 (56); `left_hand` 66/66 (60);
`starting_weapons` 45/45; `insert_bosses` 56/56; `worlds_verify all` **99/99**;
`team_type` 20/20; `drops` 16/16; `ui_scroll` and `font_atlas` passing.

**Plan §1-§7 is untouched** — every section heading sits on the line number
recorded before dispatch (§8 534, §9 542, §10 562), with 12 rows in §10. The folder
is untracked so git could not show this; the offsets are what made it checkable.

### Awaiting hardware

Three checks fail **silently** and should go first: a piece landing in the wrong
slot, supplies displacing the Hunter's Mark or a workshop tool, and the three `/`
names in the legwear picker. Then the determinism pair, which is the only real test
of §4.4: **two runs on one seed, and the same seed with one slot's ticks changed.**
Free before any of it: start a vanilla character and look at its head — a bare head
answers row 34's open item O2.

### Noticed, not fixed

`"STARTING CHOICES: "` is unrenderable on the fallback path (the colon);
`hunter_tools_verify verify` still reports 22 × `H-I3` against any real tree with
`START WITH HUNTER TOOLS` on, because no verifier tolerates the rune probe's
accessory writes; `gen_left_hand_table.py`'s docstring still says "14 of them are
obtainable" and still calls the Shield and Torch unobtainable. `RuneProbe` left
alone and still unanswered.
