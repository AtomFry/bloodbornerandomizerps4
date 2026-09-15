# Starting Weapons — Windows Behavior and PS4 Design

**Status: DONE — implemented and hardware-confirmed** (spec row 5).
Sections 1–7 are the original plan, kept as written for the record. Sections 8–10
record the decisions taken, what was actually built, and what has been checked so far.

---

## 1. Your understanding, confirmed

> Windows has two checkboxes: *Randomize starting weapons* and *Keep regular guns at start of the game*. The first randomizes both the weapons and the guns; the second stops the guns being randomized.

**Both halves are correct.** Verified in source and against the real game data:

- `startingWeaponsOnlyBool` is what gates weapon randomization at all (`RandomizeFunctions.cs:892`). Without it, weapon rows are left alone — note that "Randomize Shop Items" (`shopBool`) does **not** touch weapons, only armour and consumables.
- `keepGuns` adds equip IDs `14000000` and `6000000` to a skip list (`:639-643`), so rows selling those are passed over. It also suppresses their stat rewrite (`:955`, `:1068`, `:1085`, `:1145`, `:1252`).

And the five slots you described are real, sitting together at the top of `ShopLineupParam`:

| Row | Vanilla equipId | Weapon |
|---|---|---|
| 2000 | 7000000 | Saw Cleaver |
| 2001 | 5000000 | Hunter Axe |
| 2002 | 22000000 | Threaded Cane |
| 2010 | 14000000 | Hunter Pistol |
| 2011 | 6000000 | Hunter Blunderbuss |

## 2. Two things your model doesn't cover

**a) The setting also shuffles the entire Messengers' shop.** `ShopLineupParam` holds **644 weapon rows**, and `startingWeaponsOnlyBool` randomizes *all* of them — not just the five above. The five starting slots are only special in that they additionally get a stat-requirement rewrite. So the Windows checkbox is badly named: it reads as "randomize the starting choice" but actually means "randomize every weapon sold anywhere, and make the starting ones usable at level 1."

**b) "Keep regular guns" protects more than the two starting slots.** The skip is matched on *equip ID*, not row number, and those two guns are also sold in the regular shop. So it protects **12 rows**, not 2:

```
2010, 2011                     <- the starting choice
100112, 100113, 110112, 110113,
120112, 120113, 130112, 130113,
140112, 140113                 <- the same guns in the Messengers' shop
```

## 3. Your proposed PS4 design

1. **Randomize starting weapons** — randomizes only the three melee choices
2. **Randomize starting guns** — randomizes only the two firearm choices

Independent; either, both, or neither.

**This makes sense and I'd recommend it.** Two positive toggles are clearer than one positive plus one negative, and scoping each to what its name says removes the mismatch in (2a). It's a deliberate deviation from Windows, and a good one.

## 4. Questions before I build it

**Q1 — the 639 other shop weapon rows: keep, drop, or separate toggle?**

Your design implies dropping them. Three options:

- **(a) Drop entirely.** Simplest, matches the names exactly. Loses a feature the Windows tool has: a randomized Messengers' inventory.
- **(b) Third toggle, "Randomize shop weapons".** Keeps parity available, clearly separated. My recommendation — it's nearly free once the plumbing exists.
- **(c) Fold into "Randomize starting weapons".** Reproduces the Windows confusion. Not recommended.

**Q2 — keep the stat-requirement rewrite?** When a starting slot gets a new weapon, Windows rewrites that weapon's Str/Skl/Bloodtinge/Arcane requirements to the vanilla starter profile (8/7/0/0, 9/8/0/0, 7/9/0/0 for the melee three; 7/9/5/0 for guns), on the base weapon and its ten upgrade tiers.

I'd **keep it** — without it you can roll a starting weapon you cannot actually wield at level 4. But there's a side effect worth stating: there is one param row per weapon, so lowering requirements applies **everywhere in the game**, not just to your starting copy. If Ludwig's Holy Blade becomes a starting option, it stays low-requirement when you find it later too.

**Q3 — should the three melee slots be allowed to duplicate each other?** Windows draws without replacement, so you get three different weapons. I'd keep that.

**Q4 — fix the `/29000000` typo?** The Windows melee list has 79 entries, one of which is `"/29000000"` with a stray slash, so that weapon can never be selected. The other 78 are all valid and all sold in the shop. I'd fix it and document the deviation, consistent with how we handled the drops off-by-one.

## 5. Implementation shape

Small, and it reuses everything the enemy-drops work already built.

**No new file-format capability needed.** The param locator and row-table reader from `Param/ParamBnd` already handle this archive, and the field offsets are known:

| What | Where |
|---|---|
| `ShopLineupParam.equipId` | s32 at byte 0 of a 32-byte row |
| `ShopLineupParam.equipType` | u8 at byte 23 (0 = weapon) |
| `EquipParamWeapon.properStrength` … `properFaith` | **u8** at bytes 237-240 of a 316-byte row |

Note those stat fields are single bytes, not 4-byte ints — different from the drop field.

**New:** `Randomizer/StartingWeapons.h/.cpp`, plus the melee/firearm ID lists transcribed from the Windows source (78 valid + 16).

**Modified:** the existing `ItemData` phase currently assumes it is doing enemy drops. It becomes a general "param edits" phase that loads the archive once, applies whichever param features are enabled, and writes once. That matters: with drops *and* starting weapons both on, we must not decompress and rewrite 28 MB twice.

**Settings:** `randomizeStartingWeapons` and `randomizeStartingGuns` following the existing pattern through defaults → store → Setup Defaults → Enable wizard.

**Algorithm**, per enabled toggle:

1. Build the candidate pool from `ShopLineupParam` rows with `equipType == 0`, filtered to the melee list (for slots 2000/2001/2002) or the firearm list (for 2010/2011).
2. Draw without replacement per group and write `equipId`.
3. For each slot written, rewrite that weapon's four stat requirements — and the same for its ten `+100n` upgrade rows, whose existence I verified earlier for all five vanilla starters.

**Validation:** a `starting_weapons_verify.py` in the same shape as the others — only `ShopLineupParam` and `EquipParamWeapon` may change; only rows 2000-2002 and/or 2010-2011 change `equipId`; every assigned ID exists in `EquipParamWeapon`; melee slots get melee weapons and gun slots get guns; nothing changes when the toggles are off; plus a selftest that deliberately breaks each rule.

## 6. Deviations from Windows this design makes

| # | Windows | Proposed | Why |
|---|---|---|---|
| SW-1 | One toggle randomizes all 644 weapon rows | Starting slots scoped separately from shop inventory | The name should mean what it says |
| SW-2 | Guns controlled by a negative "keep" toggle | Positive "Randomize starting guns" toggle | Clearer, and independent of the melee choice |
| SW-3 | "Keep guns" also protects 10 shop rows selling those guns | Gun toggle affects only rows 2010/2011 | Consequence of SW-1 |
| SW-4 | `/29000000` typo makes one weapon unselectable | Fixed | Obvious defect, same call as the drops off-by-one |
| SW-5 | Three uncapped reroll loops for the hand constraints | Bounded, leaving the slot vanilla on exhaustion | Uncapped loops hang a console |

## 7. Out of scope

Shop armour and consumables (`shopBool`), and everything else param-based. Unchanged.

---

## 8. Decisions taken

| Q | Decision |
|---|---|
| Q1 | **(b)** — a third toggle, *Randomize shop weapons*, covering the Messengers' stock with the five starting rows excluded |
| Q2 | **Keep** the stat rewrite, accepting that it lowers that weapon's requirements game-wide |
| Q3 | **Three distinct** melee weapons; drawn without replacement, same for the two guns |
| Q4 | **Fix** the `/29000000` typo (deviation SW-4) |

On Q4 the evidence is unambiguous: `29000000` is a real weapon, it is sold in five
shop rows, its sibling `29020000` appears in the same list unslashed, and the three
genuine duplicates in the list are not slashed. The slash is a typo, not a marker.

## 9. What was built

**New files**

- `Randomizer/StartingWeaponLists.h` — the two candidate lists transcribed
  mechanically from the reference C# (`rightHandList` 79 raw → 76 unique,
  `leftHandList` 16 raw → 14 unique), slash stripped.
- `Randomizer/StartingWeapons.h/.cpp` — the feature itself.
- `tools/starting_weapons_verify.py` — the property validator.

**Changed**

- `Randomizer/EnemyRandomizer.*` — the `ItemData` phase is now a general param-edits
  phase. It decompresses the archive **once**, applies enemy drops and/or starting
  weapons, and writes **once**, so enabling both features does not pay the 28 MB
  decompress-and-rewrite cost twice.
- Settings chain (`RandomizerDefaults.h`, `RandomizerDefaultsStore.cpp`,
  `UI/SetupDefaultsScreen.*`, `UI/EnableWizardScreen.*`) — three new toggles.

**Three deviations worth restating**, beyond SW-1…SW-5 in §6:

- **SW-6** — the shop pass deliberately *excludes* rows 2000-2002 and 2010-2011.
  Both passes writing the same row would make the toggles interfere: turning shop
  randomization on would silently override the starting choice.
- **SW-7** — the shop pass is a **permutation** of the stock the game already sells
  (Fisher-Yates, then dealt back out in row order), not an independent draw per row.
  The reference draws with replacement from its list. A permutation keeps every
  weapon obtainable somewhere, which a draw with replacement does not guarantee.
- **SW-8** — the stat profile is keyed by **slot**, not by the weapon that used to
  occupy it, so whatever lands in slot 2000 becomes an 8/7/0/0 weapon.

## 10. Verified so far (PC, no hardware)

- **Clean build**, no warnings, from a wiped `src/x64`.
- **`starting_weapons_verify.py selftest` — 12/12.** Every invariant has a mutation
  that breaks it and is caught: melee slot given a firearm, gun slot given a melee
  weapon, fabricated weapon id, duplicate melee slots, stat profile not applied,
  an unrelated weapon restatted, a non-stat byte changed inside an assigned weapon
  row, shop stock that is not a permutation, `equipType` changed, another param
  modified — plus an unmodified archive and a well-formed randomization, which must
  both come back clean.
- **List transcription cross-checked.** The validator parses the candidate lists out
  of the reference C# rather than out of our header, and the two agree exactly:
  76 melee, 14 firearms, **no overlap** between the lists (so the melee/firearm split
  is unambiguous).
- **Pools resolve against the real data.** All 76 and all 14 are sold somewhere in
  `ShopLineupParam` and all have an `EquipParamWeapon` row, so no candidate is
  unreachable. The clean vanilla tree reports the expected five starters
  (7000000 / 5000000 / 22000000 / 14000000 / 6000000).

Not yet verified: an actual randomized archive produced by the PS4 build. That needs
a hardware run, after which `starting_weapons_verify.py verify <vanilla> <afr>` gives
the property check.
