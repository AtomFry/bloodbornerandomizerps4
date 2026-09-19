# Feature 024 — Melee Movesets (and Gun Movesets)

**Status: QUESTIONS OPEN.**

**Backlog row:** `docs/randomization-feature-spec.md` §8, rows **24** and **25**
**Reference flag:** `meleeMoveset` and `gunMoveset` (`reference/Randomizer/MainWindowComponents/BooleanHandler.cs:23-24`)
**Plan:** `docs/features/024-melee-movesets/plan.md` *(once stage 1 has run)*

---

## 1. What this does, in plain terms

Bloodborne describes every weapon with a row of numbers in a table called
`EquipParamWeapon` — how heavy it is, how much damage it does, which stats you
need to hold it, and which set of animations it plays when you swing it.

These two settings take a chosen handful of those columns and **shuffle each
column independently across all weapons**. The Saw Cleaver keeps its name, its
icon and its place in the shop, but the numbers in the shuffled columns are now
some other weapon's numbers. Row 24 does this for melee weapons; row 25 does the
same for firearms, as a separate toggle over a separate set of weapons.

The intent, going by the checkbox name, is that a weapon **swings like a
different weapon**. §3 is where that intent stops matching what the code does.

**Which columns get shuffled is itself the open question**, and it is settled on
the console rather than on paper — see §10, decision 1. The feature therefore
ships with a **mode switch** selecting between two candidate column sets, so the
developer can run one, then the other, and see which actually changes a
weapon's swing. The mode is scaffolding for that test, not a permanent setting.

## 2. What the player experiences

| | |
|---|---|
| **Both off** (default) | Exactly today's behaviour — weapons are vanilla |
| **Melee movesets on** | Every non-firearm weapon row has the shuffled columns replaced with values drawn from other non-firearm weapons |
| **Gun movesets on** | The same, over the firearm rows only |
| **Both on** | Both passes run against the same `EquipParamWeapon`, over disjoint row sets, so they do not interfere |

The two settings are independent — neither implies the other, and the pools do
not overlap.

Cutting across both, one shared mode control chooses **which columns** the
shuffle touches:

| Mode | Columns shuffled | What it is |
|---|---|---|
| **A — curated** (default) | The 48 hand-picked cells the reference's own list names | What the reference author appears to have meant. Includes the three animation fields |
| **B — as shipped** | Cells 0–47, what defect R-1 actually visits | Byte-for-byte what the reference does today. Includes the upgrade-chain pointers and the weapon model id; includes no animation field |

The mode applies to both passes at once, because both reference functions share
one cell list. Exactly one mode is in effect per run.

**What the player does *not* get:** weapon names, icons, shop placement and
upgrade costs are untouched by the intended column set, so the game still calls
it a Saw Cleaver. The surprise is entirely in how it behaves once equipped.

## 3. What the reference tool does

Gate at `StartFunctions.cs:1526-1534`; implementations at
`RandomizeFunctions.cs:2694` (`RandomizeMeleeMoveset`) and `:2800`
(`RandomizeGunMoveset`). The two functions are near-identical copies that differ
in exactly one place — the sense of the row filter.

Both are called with the same two lists, built at `StartFunctions.cs:1354-1524`:

- `numbersInParamToRandomize` — **48 cell indices**, the intended columns
- `gunsToReplace` — **121 row IDs**, the firearm rows

### 3.1 The shape of the pass

For each selected column, over the rows the filter admits: collect every value in
that column, shuffle, write back. A per-column permutation, one column at a time,
rows never move as a unit. Two weapons that swap `attackBasePhysics` will not
also swap `attackBaseMagic`.

The row filter is the only difference between the two functions:

| | Filter | Pool size (measured) |
|---|---|---|
| Melee | rows **not** matching a `gunsToReplace` entry (`:2745-2754`, `doStuff` starts `true`) | **958** of 1090 |
| Gun | rows **matching** one (`:2851-2860`, `doStuff` starts `false`) | **132** of 1090 |

### 3.2 Three defects in the reference, all confirmed in source

These are not stylistic complaints. Each one changes what the feature does, and
the first changes whether it does anything at all.

**R-1 — The selected columns are not the ones in the list.** `:2726-2735`:

```csharp
for (int k = 0; k < numToReplace.Count; k++)
{
    if (k == i) { doFirst = true; }
}
```

The comparison is against `k`, the **loop index**, not `numToReplace[k]`, the
**value**. So the condition is true exactly when `i < numToReplace.Count`. The
list has 48 entries, so the pass randomizes **cells 0–47** and ignores the
curated list entirely. Both functions carry the identical line.

**R-2 — The shuffle aliases live cells, so it is not a permutation.**
`PARAM.Cell` is a **class**, and `Row.Cells` is `IReadOnlyList<Cell>` holding
those objects (`SoulsFormats/Formats/PARAM/PARAM.cs:590`, `:288`). `:2739`
collects *references* into `cellList`, and the write loop at `:2778-2781`
assigns into the very objects the shuffled list still points at. A cell drawn
after it has already been overwritten yields its **new** value, not its
original.

**R-3 — The row filter is a substring test.** `:2749` and `:2855` both use
`Rows[j].ID.ToString().Contains(rowsToReplace[k].ToString())`, not equality.

**R-4 — the last cell is unreachable.** `numOfCells = Cells.Count - 1` (`:2724`)
makes cell 160 unvisitable. Harmless under either column set — the intended list
stops at 158 — but it confirms `:2724` was written expecting a value loop.

### 3.3 The consequence, stated plainly

**As shipped, the reference's "Melee Movesets" setting does not randomize
movesets.** The fields that select a weapon's animations — `wepmotionCategory`
(cell 70), `wepmotionOneHandId` (78), `wepmotionBothHandId` (79) — are all in the
intended list and all outside the 0–47 window R-1 actually visits.

This is a reference defect, not a port decision, and §7 of `CLAUDE.md` does not
settle it. It is carried to the developer as **question 1**, which is blocking.

## 4. Evidence

All measurements are against `data/vanilla/dvdroot_ps4`, via
`app/tools/param_offsets.py`, which resolves cell indices from the real paramdef.

**fact — `EQUIP_PARAM_WEAPON_ST` has 161 fields, 1090 rows, 316-byte stride.**
`python tools/param_offsets.py fields ../data/vanilla/dvdroot_ps4 EQUIP_PARAM_WEAPON_ST`.
The stride matches what `src/Param/ParamBnd.h` already records as
hardware-validated, so the port's existing locator reaches this param today.

**fact — the intended list is 48 cells:** 3, 8–15, 19–21, 51–70, 76–82, 90–96,
157, 158. Extracted from `StartFunctions.cs`; `grep -c` gives exactly 48.

**fact — those 48 cells resolve to plausible combat fields.** Among them:
`weight` (3), the four `correct*` scaling coefficients (8–11), the `attackBase*`
damage columns (53–57), `saWeaponDamage` (58), `equipModelCategory`/
`weaponCategory`/`wepmotionCategory` (67, 69, 70), `wepmotionOneHandId` (78),
`wepmotionBothHandId` (79), the stat requirements `properStrength`/
`properAgility`/`properMagic` (80–82), and `wepRegainHp` (158).

**fact — the effective list, cells 0–47, resolves to a different set entirely.**
It contains `behaviorVariationId` (0), `sortId` (1), `fixPrice`/`basicPrice`/
`sellValue` (5–7), `materialSetId` (22), **`originEquipWep` through
`originEquipWep15` (23–38)**, `levelSyncCorrectId` (43), a `dummy8` pad (44),
the two `vagrant*EneDropItemLotId` columns (45, 46), and `equipModelId` (47).
It contains **none** of the three `wepmotion*` fields.

**fact — the two column sets overlap in exactly 12 cells, and the overlap is
what a player would notice first.** Computed by set intersection over the two
lists: 48 cells each, **12 shared**, 36 unique to each. The shared 12 are cells
3, 8–15 and 19–21 — `weight`, the four `correct*` scaling coefficients, the four
`*GuardCutRate` columns, and the three `residentSpEffectId` columns.

**This is the single most important fact for the A/B test.** Both modes shuffle
weapon weight and stat scaling, so **both modes will visibly change how a weapon
performs**. "Did anything change?" cannot discriminate between them, and a
developer who stops at that question will conclude both modes work. The
discriminating observations are narrower, and §8 lists them.

**inference — the buggy set scrambles upgrade chains and weapon appearance.**
`originEquipWep0..15` are the per-tier weapon-id pointers and `equipModelId`
selects the visual model. Shuffling those columns across 958 rows is a different
feature from the one on the checkbox, with a plausible route to a broken save.
Reasoning is from field names and the paramdef, not from observed game
behaviour — see the constraint in §7.

**inference — the buggy set is not entirely inert.** `behaviorVariationId`
(cell 0) is in range, and in FromSoftware params it is the key that a weapon's
attacks resolve through. So the shipped reference may well produce *some* visible
combat change, which is a likely reason the defect went unnoticed. **Unproven —
this is exactly the row-8 trap and only the console can settle it.**

**fact — R-2 destroys half of every shuffled column.** Simulated the alias-and-
overwrite order over 200 seeds at both real pool sizes:

| Pool | Distinct values surviving | Rows keeping their own value |
|---|---|---|
| 958 (melee) | 479.5 of 958 — **50.0% of the column's values are lost** | 1.6 (0.17%) |
| 132 (gun) | 66.5 of 132 — **49.6% lost** | 1.8 (1.40%) |

Half the values vanish and the survivors are duplicated. The ~50% figure is
stable across both pool sizes.

**fact — R-3 misfiles the Holy Moonlight Sword.** Exact matching against the 121
gun IDs hits **121** rows. `Contains` hits **132**. The 11 extra are
`26000000`–`26001000`, which the reference's own comment at
`RandomizeFunctions.cs:692` identifies as *"Holy sword of moonlight (moonlight
sword)"*. `"6000000"` is a substring of `"26000000"`. So the Holy Moonlight
Sword's whole upgrade family is **excluded from the melee pool and included in
the gun pool**.

**fact — the intended list collides with the starting-weapons feature.** Cells
80–82 (`properStrength`, `properAgility`, `properMagic`) are in the intended 48,
and `src/Randomizer/StartingWeapons.cpp:22-25` already writes byte offsets 237,
238, 239 — the same three fields. In the reference the moveset pass runs *first*
(`:1526`) and `RandomizeShopItems` *after* (`:1541`), so the starting-weapon stat
profile wins on the five slot rows. Under the effective 0–47 set there is no
collision at all today.

**fact — `properFaith` (83) is omitted from the intended list** while 80–82 are
present, and `guardmotionCategory` (71) is omitted while `wepmotionCategory` (70)
is present. The list was hand-curated, not a range.

**fact — cell 157 is `dmypolyId_Slot3LeftFormB`**, one of 18 `dmypolyId_*`
attachment-point fields spanning cells 140–157. The list includes only that one.
No evident reason; recorded as a quirk rather than explained.

**assumption — the 48-cell list is the author's real intent.** Supported by its
contents being coherently combat-related and by the count matching the feature
name, but the author never states it. Cannot be verified further from the repo.

## 5. Terminology

**Cell index** — a field's ordinal position in the paramdef, counting padding and
bitfields. One-to-one with `PARAM.Row.Cells[i]`. `tools/param_offsets.py` turns
one into a byte offset; the port bakes the offset in as a constant and never
ships a PARAMDEF parser.

**Moveset fields** — in this document, specifically `wepmotionCategory` (70),
`wepmotionOneHandId` (78) and `wepmotionBothHandId` (79): the columns that select
which animation set a weapon plays. Used to distinguish them from the other 45
combat columns in the same list.

**Intended list / effective list** — the 48 curated cell indices, versus the
cells 0–47 that defect R-1 actually visits. Kept distinct throughout because the
whole A/B test turns on the difference.

**Mode A / mode B** — the two shipped column sets: A is the intended list, B is
the effective one. Used in §8 and §10 so the hardware protocol can name them
without restating which cells each covers.

## 6. Scope

**In scope:** backlog rows **24** (`meleeMoveset`) and **25** (`gunMoveset`).
They share the column list, the row list, the param, the shuffle and the
validator; the backlog's §11 already groups them, and specifying one without the
other would leave the second spec with nothing of its own to say. Two independent
user-facing toggles, per §11's suggested order block 2.

**Also in scope: both candidate column sets, and a shared control selecting
between them** (§10, decision 1). Everything else about the two modes is
identical by construction — same pools, same shuffle, same row filter — because
the moment they differ in a second way, the test stops answering question 1.

**In scope but explicitly temporary: the mode control itself.** It exists to
settle decision 1 on hardware. Once the console has answered, the losing column
set and the control both come out, and this spec's §2 collapses back to two
plain toggles. Question 7 covers the one case where that would not happen.

**Out of scope, and why:**

- **Row 5, starting weapons** — shipped. This spec only records where the two
  features touch the same three cells (§4), and question 5 settles the ordering.
- **Row 11, shop items** and **row 27, no team type** — the next block in §11.
  Different params (`ShopLineupParam`, `NpcParam`), no shared machinery.
- **Row 26, gems and runes** — `GemGenParam`, a param the port has never opened.
- **Rerouting the `originEquipWep` upgrade chain as a feature in its own right.**
  If question 1 resolves toward the intended list, that behaviour disappears
  from this feature. It is not proposed here and should not be smuggled in.
- **Chalice dungeons** — out by standing decision.

## 7. Constraints

**Font** — 8×8, uppercase A–Z, digits, space and `'` `(` `)` `-` `,`. Proposed
labels `MELEE MOVESETS` and `GUN MOVESETS` are clean. A comma in a progress line
is fine — the glyph exists; an earlier draft of this spec said otherwise, from a
stale reading of `CLAUDE.md` §5. Check `app/src/Platform/Font8x8.cpp`, which is
the authority.

**Byte decoding never proves game behaviour** — the governing constraint here,
and the one this feature is most exposed to. Every claim in §4 about what a field
*does* is read off a field name. Row 8 shipped a permanently dark game on exactly
that basis. The three `wepmotion*` fields being "the moveset" is the single
assumption the whole feature rests on, and **only the console can confirm it**.

**Run safety** — under the intended list, cells 80–82 shuffle stat requirements
across all 958 melee rows, including the three vanilla starter weapons. A level-4
character can be handed a starting weapon requiring far more Strength than they
have. Not unwinnable — stats can be levelled — but a bad opening hour. This is
question 6.

**Mode B carries a save risk mode A does not.** Cells 23–38 are
`originEquipWep0..15`, the per-tier upgrade pointers, and cell 47 is
`equipModelId`. Shuffling those across 958 rows can plausibly send an upgrade
down a chain that resolves to a different weapon, and the outcome is written into
the save the moment the player upgrades. Mode B also writes cell 44, a `dummy8`
pad — spec-legal to write but meaningless, and a reminder that the 0–47 window
was never curated for safety. **Mode B should be tested on a character the
developer is willing to lose**, not the one carrying a real playthrough.

**The A/B comparison needs vanilla as a third input.** `CLAUDE.md` §3 records
what happens without it: comparing two randomized runs against each other, with
no vanilla baseline, produced 221 phantom anomalies. The same trap applies here
in its in-game form — a weapon that feels unfamiliar in both modes tells you
nothing unless you know what it felt like unmodded.

**Fidelity** — `CLAUDE.md` §7 makes the reference the default authority. That
rule exists to stop the port "improving" behaviour that works. R-1 is the case it
does not anticipate: the reference does not implement the feature it names.
Shipping both candidates defers that judgement to the console instead of
resolving it by argument, which is the §3 methodology this project already
prefers — but it does not remove the need to state a reason once the answer
lands.

**Param writes are pokes, not rebuilds** — `src/Param/ParamBnd.h` is a locator
with no writer by design. Field widths in the intended list range from `s8` to
`f32`; `StartingWeapons.cpp:21-23` already records that writing four bytes into a
`u8` field silently corrupts its neighbours.

**AFR** — `gameparam.parambnd.dcx` is read from `VanillaSource` and written to
the overlay; all six folders mirrored. Unchanged from the shipped param features.

**No emulator** — nothing in this loop can run the game.

## 8. How we will know it works

**Automated.** A new mirror, `app/tools/moveset_verify.py`. No existing tool
covers `EquipParamWeapon` column shuffles — `starting_weapons_verify.py` touches
the param but only the four stat-requirement cells on five rows.

It should follow `starting_weapons_verify.py`'s pattern of **parsing the cell and
gun lists straight out of `StartFunctions.cs`**, so the fixture cannot drift from
the reference, and reuse `param_offsets.py` for cell-to-offset resolution rather
than duplicating it. What a `selftest` should establish:

- pool partition: 958 melee / 132 gun under exact matching, 1090 total, with the
  `26000000` family landing on whichever side question 3 decides
- every non-selected cell is byte-identical to vanilla in every one of the 1090 rows
- each selected column is a true multiset permutation of its vanilla values —
  which is the direct test for R-2, and will fail against the reference's output
- no row outside the relevant pool is modified by that pass
- both passes enabled together produce the same bytes as running them in sequence

**State the limitation: this pins the rules, not the C++ that implements them.**
The mirror and the port can drift, and nothing here executes the PS4 code.

**Hardware — the A/B protocol.** This is the part only the developer can run,
and it is what decision 1 rests on, so it is written as a procedure rather than a
checklist.

**Hold everything constant except the mode.** Same seed both times, melee
movesets on and gun movesets off for runs 2 and 3, and whatever questions 2 and 3
resolve to answered identically in both modes. A difference in any second
variable and the comparison stops being about columns.

**Run 0 — vanilla, first.** Before either mode, play the opening with the
randomizer off and pay deliberate attention to one weapon's swing: the rhythm of
the R1 chain, and what the transformed form does. This is the baseline the §7
constraint says the comparison cannot do without. It is the step most likely to
feel skippable and most likely to invalidate the result if skipped.

**Runs 1 and 2 — mode A, then mode B**, same seed, same starting weapon choice.
In each, on a character the developer is willing to lose:

| # | Observation | Why it discriminates |
|---|---|---|
| 1 | **Does the R1 chain play a different animation than run 0?** | The whole question. `wepmotion*` is in A only, so a genuine animation change in A and not in B confirms both the assumption and mode A |
| 2 | Does the weapon look right in hand and in the inventory icon? | `equipModelId` is in B only. A changed model is a positive identification of mode B |
| 3 | At the workshop, does the weapon still upgrade into *itself*? | `originEquipWep*` is in B only. Upgrading into a different weapon identifies B, and is the save risk in §7 |
| 4 | Is the weapon still equippable at the level the game gives you? | Cells 80–82, mode A only — the question 6 concern |
| 5 | Does it hit harder or weigh differently than run 0? | **Does not discriminate.** The shared 12 cells guarantee this in both modes (§4). Recorded so it is not mistaken for a result |

**Then the gun pass**, once a mode has won: melee off, gun movesets on. Confirm
firearms changed and melee weapons did not, and check the Holy Moonlight Sword
against whatever question 3 resolved to.

**How the result reads:**

- Animation changes in **A** and not in B → decision 1 resolves to the curated
  list, the `wepmotion*` assumption holds, mode B is removed.
- Animation changes in **neither** → the `wepmotion*` assumption is wrong. This
  is the outcome worth planning for, because it is not a build failure and not a
  no-op: both modes still shuffle the shared 12 cells, so the feature "works"
  while doing something other than what its name says. It would send the feature
  back to §9 rather than forward.
- Animation changes in **B** → the field-name reading in §4 is wrong somewhere,
  and nothing downstream of it should be trusted until that is understood.

**Failure would look like:** no observable change in either mode (the poke did
not land at all); a weapon that cannot be equipped on a fresh character; a crash
or a refusal to load the param.

## 9. Questions for the developer

*Question 1 has been decided — see §10.* The numbering below is left unchanged so
that references elsewhere in this document stay valid.

**2. Reproduce the R-2 aliasing, or do a clean permutation?**

*Established:* the alias-and-overwrite order destroys ~50% of each column's
distinct values and duplicates the survivors, measured over 200 seeds at both
pool sizes (§4).

*Recommended: clean permutation.* R-2 has no possible design intent — the code
shuffles into `randomizedCellList` precisely because a permutation was wanted,
and the aliasing silently defeats it. Reproducing it would mean deliberately
copying an artefact of C# reference semantics that the author could not have
intended and cannot have observed. Non-blocking: the feature works either way,
but the output differs substantially and the verifier's permutation check depends
on the answer.

**3. Exact row matching, or reproduce the `Contains` substring test?**

*Established:* `Contains` moves the Holy Moonlight Sword family
(`26000000`–`26001000`, 11 rows) out of the melee pool and into the gun pool.
121 exact matches versus 132 substring matches (§4).

*Recommended: exact matching.* The Holy Moonlight Sword is a greatsword; it is in
the gun pool only because `"6000000"` is a substring of `"26000000"`. Also worth
noting it is on the reference's own `rightHandList` at `:772`, so the author
plainly considered it a melee right-hand weapon. Low stakes, easy to revisit.

**4. Two toggles, or one combined?**

*Established:* the reference has two independent flags over disjoint row sets.

*Recommended: two toggles*, `MELEE MOVESETS` and `GUN MOVESETS`. Matches the
reference, matches the port's positive-toggle house style, and both labels are
clean in the 8×8 font. Splitting costs one extra row.

**5. Where does the moveset pass sit relative to starting weapons?**

*Established:* cells 80–82 are written by both features. The reference runs
movesets first (`:1526`) and the shop/starting-weapon pass second (`:1541`), so
the fixed stat profile wins on the five slot rows. Only matters if question 1
lands on the intended list.

*Recommended: keep the reference's order.* It is the safer of the two — the stat
profile exists specifically so a randomized starter is wieldable at level 4, and
letting the moveset shuffle land after it would undo that protection for exactly
the weapons the player is stuck with.

**6. Should cells 80–82 be shuffled at all?**

*Established:* they are `properStrength`, `properAgility`, `properMagic` — the
stat requirements. Shuffling them across 958 melee rows means a fresh character
may be unable to wield the weapon they chose. Question 5 protects only the five
starting-slot rows, and only when starting weapons is enabled.

*Recommended: keep them in, and make this hardware check 5.* Requirement
shuffling is legitimately part of "this weapon now behaves like another one", the
author included them on purpose while deliberately omitting `properFaith` (83),
and the situation is recoverable by levelling rather than unwinnable. But this is
a player-experience judgement rather than a technical one, so it is the
developer's to make — and if the hardware test shows a genuinely unplayable
opening, dropping the three cells is a one-line change.

**7. What happens to the losing column set after the test?**

*Established:* decision 1 ships both modes to settle which is right. Once the
console answers, one of them is known to be wrong about movesets.

*Recommended: remove the loser and the mode control, and say so in the plan's
deviations section.* Carrying two column sets forever doubles the verifier
surface and leaves a setting whose only purpose was an experiment that has since
concluded — exactly the kind of stale scaffolding `CLAUDE.md` §8 warns about.

*The one case for keeping it:* if mode B turns out to be independently
entertaining — scrambled upgrade chains and randomized weapon models is a real
thing someone might want — then it survives as its **own** feature with its own
backlog row and an honest name, along the lines of `SHUFFLE WEAPON DATA`. It
would not survive as a second moveset mode. Not blocking; it cannot be answered
until the test has run.

## 10. Decisions taken

**1. Ship both candidate column sets behind a mode control, and let hardware
decide.** *(2026-09-15, developer.)*

Question 1 asked whether to reproduce defect R-1 or implement the reference's
curated 48-cell list. The answer is to **build both** — mode A (curated) and mode
B (as shipped) per §2 — and resolve it by testing one, then the other, on the
console.

*Why this over picking one:* every argument for either set is read off paramdef
field names, and `CLAUDE.md` is unambiguous that byte decoding never proves game
behaviour. Row 8 is the standing example — a correct byte-level analysis produced
a shipped build with a permanently dark game. This decision declines to repeat
that pattern on a question the console can answer directly and cheaply.

*What it commits to:*

- Both modes are identical in every respect except the column set (§6). Questions
  2 and 3 must be answered the same way in both, or the comparison is confounded.
- The mode control is temporary scaffolding, removed once the answer lands
  (§6, question 7).
- The A/B protocol in §8 governs the test, including the vanilla baseline run and
  the five-observation table. The protocol is part of this decision, not advice —
  the 12 shared cells (§4) mean a casual "yes it changed" reading would confirm
  both modes and settle nothing.

*What it does not do:* it does not decide which mode is correct, and it does not
pre-commit to mode A. It replaces a recommendation with a measurement.

*Still open:* questions 2–7. The spec stays **QUESTIONS OPEN**.

---

<!--
Not in this document, on purpose:

  - implementation approach, file-by-file changes, code
  - the verification *mechanism* (§8 says what correct means, not how to check)

Those belong to stage 1.
-->
