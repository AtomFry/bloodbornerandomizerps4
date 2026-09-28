# Plan Evidence 037 — Start With A Trick Weapon

**Plan:** `docs/features/037-start-with-trick-weapon/plan.md`

**Spec:** `docs/features/037-start-with-trick-weapon/spec.md`

---

> **This document is the investigation behind the plan, not instructions.**
>
> The implementer does not read it to start work. They consult it when an
> implementation question needs context the contract deliberately left out —
> "why this number", "was the other approach considered", "how was that
> measured".
>
> It is revisable. When a measurement is corrected, correct it here and say what
> it replaces; the chronology lives in `log.md`.

---

## E1. Reference trace

**There is nothing to trace.** The spec's §3 claim was re-checked rather than
taken: `CharaInitParam` appears nowhere in `reference/Randomizer/`, and the only
occurrences in the `reference/` tree are in `reference/SoulsFormats/`, in MSB
part-record definitions belonging to other FromSoftware titles. The Windows tool
has no code path that edits character creation, so there is no function, no call
site, no field list and no pass ordering to reproduce.

That matters for two reasons the plan depends on.

**There is no ordering to inherit.** Every other param feature in this port sits
somewhere the reference put it. This one has no such anchor, so the position of
the pass inside `StepItemData` is the port's own decision and is argued on RNG
determinism instead (§E3, row "draw early").

**There is no quirk to preserve** (`CLAUDE.md` §7). The nearest reference
behaviour is `RandomizeShopItems` (`RandomizeFunctions.cs:609-1315`), which this
port already ships as row 5's three toggles. That function rewrites
`ShopLineupParam.equipId` and then lowers the four requirement bytes on whatever
landed in a Hunter's Dream slot (`:1106-1176`) — it changes **what the Dream
offers**, never what a character begins with. The only thing row 037 takes from
it is that requirement rewrite, which the port has as
`StartingWeapons.cpp`'s `ApplyStatProfile`.

---

## E2. What exists in the port

### E2.1 `HunterTools.{h,cpp}` — the closest sibling, and why row 037 does not extend it

Read in full. It is 144 lines of `.cpp` and it is this feature's nearest
neighbour in every respect that matters: the same param member
(`CharaInitParam.param`), the same 22 origin rows, the same "the archive is
never rebuilt" discipline, the same honest UNVERIFIED banner in its header, and
the same deletion rule if the console says no.

What it owns that row 037 needs:

| Symbol | Value | Row 037's need |
| --- | --- | --- |
| `kOriginRows[22]` | 2000–2009, 3000–3009, 3500, 3501 | identical |
| `kRowBytes` | 300 | identical |
| `kItemIdBase` / `kItemNumBase` / `kItemSlots` | 124 / 204 / 10 | identical, for the inventory route |
| `FindRow` / `RowHasItem` / `FirstEmptySlot` | private helpers | identical |
| `kBloodGemWorkshopTool` / `kRuneWorkshopTool` | 4103 / 4104 | irrelevant |

What row 037 needs that it does not have: a second param member
(`EquipParamWeapon.param`), a selection, an RNG draw, a requirement rewrite, and
a second field family (`equip_Wep_Right` at offset 16).

Three options were weighed.

**Extend `HunterTools` into a single "starting gear" pass.** Rejected. The two
features are independently toggled, report separate counters and separate
progress lines, and have separate deletion rules. A merged pass would have to
carry two option flags and two result blocks, and row 34 has not been
hardware-tested — a regression introduced by refactoring it would be
indistinguishable from the feature never having worked. `EasyModes.cpp` sets the
precedent for the opposite arrangement: four independent settings in one file
*because they are the same edit*; these are not.

**A new file that copies the 22 rows and the helpers.** Rejected as the primary
choice, though it is the fallback if Q3 is answered "no". `HunterTools.cpp`'s own
comment says the list "can be narrowed to whichever block is live" once hardware
answers, and the spec appendix records that row 34's open item O2 is blocked on
the *same* hardware test as this feature. A list that is about to be narrowed,
held in two places, is a drift waiting to happen: narrowing one copy and not the
other would leave one feature writing rows the other does not, which is precisely
the failure nobody would notice.

**A new file sharing a carved-out header.** Chosen (plan P6). The move is pure —
no value changes, no logic changes — so row 34's output stays byte-identical, and
`hunter_tools_verify.py` is the check that says so: it already asserts
"`HunterTools.cpp` offsets match this tool" and "`HunterTools.cpp` targets all 22
origin rows" by parsing the `.cpp`, and repointing that parse at
`CharaInitRows.h` keeps the assertion and moves the target.

### E2.2 `StartingWeapons.cpp` — `ApplyStatProfile` and why D4 needs more than reuse

`ApplyStatProfile(plain, weaponRows, weaponId, profile)` sits in an **anonymous
namespace** — it is not reachable from another translation unit as it stands, so
"reuse `ApplyStatProfile` unchanged" (spec D3) cannot be taken literally. It
walks `tier = 0..10`, looks up `weaponId + tier * 100`, and writes four single
bytes at 237/238/239/240, skipping absent tiers.

`kStatProfiles` is keyed by **shop row**, not by weapon:

```
{ 2000, 8, 7, 0, 0 }   { 2001, 9, 8, 0, 0 }   { 2002, 7, 9, 0, 0 }
{ 2010, 7, 9, 5, 0 }   { 2011, 7, 9, 5, 0 }
```

so there is no profile a *granted* weapon could borrow, which is what D3's single
profile supplies.

The D4 problem is visible in the shape of the call: `ApplyStatProfile` is called
once per assignment in a loop over `assigned`, and it writes unconditionally. If
the grant pass calls it too, whichever call runs second wins — exactly what D4
forbids as a mechanism. Three ways to make the rule explicit were weighed in §E3;
the owner-ranked writer was chosen.

Worth recording: the **practical** stakes are nil. The coffin profiles are all
*lower* than the grant's `(9, 9, 5, 6)` in every component except
`{2001, 9, 8, 0, 0}`'s strength (equal) — so if a coffin profile won, the weapon
would still be wieldable by every origin. D4 is a correctness-of-rule
requirement, not a bug fix, and the plan spends about thirty lines on it for that
reason and no more.

### E2.3 `StepItemData`'s sequence

`EnemyRandomizer.cpp:972-1093`. Step 0 reads and decompresses
`/param/gameparam/gameparam.parambnd.dcx`. Step 1 parses the members and then, in
this order:

1. `RandomizeEnemyDrops` (`NpcParam.param`) — draws randomness.
2. `RandomizeStartingWeapons` (`ShopLineupParam.param`, `EquipParamWeapon.param`)
   — draws randomness: three melee draws, two gun draws, then a Fisher-Yates
   over the shop stock.
3. `GrantHunterTools` (`CharaInitParam.param`) — draws none. Its comment says
   "Order doesn't matter here: it touches CharaInitParam, which nothing else in
   this phase reads or writes." Row 037 makes that sentence false, which is why
   the plan states the new pass's position rather than leaving it to the
   implementer.

Step 2 recompresses and writes. `Phase::ItemData` is the **last** phase —
`Step()` goes to `Phase::Finished` after it — so a draw placed at the end of step
1 is the last draw of the entire run. That is the whole determinism argument for
plan P8.

Each feature locates only the members it needs, and a missing member is fatal
only to the feature that wants it. Row 037 follows that: it locates
`CharaInitParam.param` and `EquipParamWeapon.param` itself.

### E2.4 The settings chain, as it now is

The chain the caged-dogs plan described no longer exists: `EnableWizardScreen` is
gone, and row indices with it. The current chain, traced end to end for
`startWithHunterTools`:

| Site | What it holds |
| --- | --- |
| `Randomizer/RandomizerDefaults.h` | the field, with its own default |
| `Randomizer/RandomizerDefaultsStore.cpp` | the key, in `FormatSettings` and `ApplySettingKey` |
| `UI/SettingsModel.h` | `SettingId`, `SettingKind` |
| `UI/SettingsModel.cpp` | one `kSettings` entry: id, category, kind, label, pointer-to-member, help |
| `Randomizer/EnemyRandomizer.h` | the option field, and `AnyParamFeature()` |
| `Game/WorldActivation.cpp:766-795` | the options mapping and the `anythingOn` chain |
| `UI/WorldEditorScreen.cpp:1293` | the progress line |

**The screens need no change for a fourth picker.** Both reach a picker only
through `IsDrillIn` and the four `Selection*` functions
(`SetupDefaultsScreen.cpp:134,228,303`; `WorldEditorScreen.cpp:694,810,853-864,1431`),
including the revision-diff code at 853-864, which walks `SelectionFlags` and
`SelectionCount` generically. This was checked specifically because the caged-dogs
plan had to edit three parallel `items` vectors and two row-constant blocks; that
whole class of work is gone.

`kSettings` declaration order **is** display order, so "after
`START WITH HUNTER TOOLS` in `WeaponsGear`" is the entire placement decision.
There is no renumbering hazard of the kind plan 033 §9 P5 accepted a worse
placement to avoid.

### E2.5 `ModelPoolSelection` and the picker

`ModelPoolSelection<N, DefaultSelected>` already carries everything this feature
needs, and `DefaultSelected = false` is exactly the polarity row 037 wants: a
fresh object ticks nothing, an unknown row reads as not ticked, and `Decode`
returns false and **leaves the selection untouched** unless the string is exactly
N characters. `EnemySkipSelection` is the proof that the second template
parameter works in shipped code.

`Encode`/`Decode` are one character per row, positional — which is the whole
reason the table's order is load-bearing and why the plan says a changed count
must be rejected rather than misapplied.

`ModelPicker` is generic over `(strings, table, count, flags)` and stores none of
them. Two of its properties bear on this feature:

* `RowLabel` (`ModelPicker.cpp:83`) composes `model + " " + displayName` and
  uppercases the first five characters. For a weapon row that gives
  `7010000 UNCANNY SAW CLEAVER`. The spec flagged this in §4.6; §E3 records the
  three ways out and why a `PickerStrings` field won.
* The band is sized from "the widest row any of the three tables can produce —
  the boss list's `C4520 LADY MARIA OF THE ASTRAL CLOCKTOWER` at 791 px"
  (`ModelPicker.cpp:44-47`), asserted by `settings_ui_verify.py` case 9. The
  widest weapon row is 653 px (§E4 M7), so the band is untouched.

### E2.6 The generators and verifiers

`gen_pool_table.py` is the pattern: a `Kind` per table, `--check` that exits
non-zero if the committed file is stale, the frozen-order warning in the
generated header, and a `RENDERABLE` set that makes an unrenderable name a
generator failure rather than a blank gap on a TV. All three of its kinds take a
list of **model ids** and look names up in `tools/data/Characters.json` through
`names.model_name`. Neither of those applies here, which is why row 037 gets its
own generator (§E3).

What each verifier will need is in the plan's §5. The two that need the most care
are the ones whose invariants this feature *breaks by design*:

* `hunter_tools_verify.py` H-I1 ("only `CharaInitParam.param` differs inside the
  archive") and H-I3 ("within an origin row, only `item_*` and `itemNum_*`
  differ").
* `starting_weapons_verify.py` SW-I1 ("only `ShopLineupParam.param` and
  `EquipParamWeapon.param` may differ"), SW-I6 and SW-I7 (requirement bytes may
  differ "only on an assigned weapon or one of its upgrade tiers").

A run with row 037 on and either of those on violates all five. See §E5.3.

`worlds_verify.py` holds two **deliberately frozen** lists —
`WIZARD_OPTIONS_MAPPING` (18 pairs) and `WIZARD_RUN_DECISION` (13 names) — whose
comment says a field added to `EnemyRandomizerOptions` "has to be added here
deliberately, with the output question asked out loud". Its recipe round-trip
handles the new key without help: it keys off the `_included` suffix
(`worlds_verify.py:553`), which `trick_weapons_included` has.

`settings_ui_verify.py` case 3 compares the model against
`docs/features/randomizer-settings-ui/spec.md` §7.1's table, cell for cell,
through `PROSE_TO_LABEL`. This is the non-obvious coupling: a new setting fails a
verifier until **another feature's spec** is edited. See §E5.4.

---

## E3. Alternatives considered

| Approach | Why rejected |
| --- | --- |
| Hardcode the 78 weapon ids and names in a header, as `CagedDogList.h` does for ten dogs | Spec §8 requires the exclusion to be "re-derived rather than hardcoded as six ids", and the names to be the game's own. A hand list would also have shipped the spec's count error (§E6) into the code |
| Add a `weapon` Kind to `gen_pool_table.py` | Its `build()` hardcodes the Characters.json provenance lines and a `{ "cNNNN", ... }` row shape, and every Kind is `models_fn(root) → model ids`. Parameterising all of that to add one unrelated source is more edit than a separate generator, and it puts FMG, shop and item-lot parsing inside the creature-table tool |
| Compose version names as `"UNCANNY " + base` | Wrong for four of 26: `Ludwig's Uncanny Holy Blade`, `Logarius' Uncanny Wheel`, `Uncanny Bowblade`, `Uncanny Parasite`. Measured, not assumed (§E4 M5) |
| Derive membership from `weaponCategory` | Value 0 covers both the Hunter Blunderbuss and the Threaded Cane. Confirmed in the data before the hand bits were used |
| Derive membership from `StartingWeaponLists.h` | It is the reference's two **hand** lists, 76 + 14 entries, with the Loch Shield and Hunter's Torch among the "firearms" and the base Beast Claw and Uncanny Saw Spear missing. 76 ≠ 78 |
| Sort the table purely by display name | Puts every `LOST …` row in one alphabetical block, every `UNCANNY …` in another, and the base weapons in a third, so a player looking for the three Saw Cleavers scrolls three separate pages of a 78-row list. Base-name-then-version keeps them adjacent at no cost, since the order only has to be *frozen*, not alphabetical |
| Keep the picker's id column and let rows read `7010000 UNCANNY SAW CLEAVER` | All 78 names are distinct, so the id disambiguates nothing; and the id is meaningless to a player choosing a weapon. The creature pickers need it because names repeat (`ABHORRENT BEAST` twice) |
| Blank `ModelPoolEntry::model` so `RowLabel` draws a leading space instead | Silent, layout-shifting, and it takes the weapon id away from the picker's own log line and from the engine |
| Give the weapon table its own row struct and template `ModelPicker` on it | Templating a 271-line component that three shipped screens call, to avoid one `bool`, and `settings_ui_verify.py`/`pool_verify.py` both parse its strings as they are |
| Emit a parallel `std::array<int32_t, 78>` of ids beside the table | A second thing to keep in step for a `strtol` the engine calls 78 times, once, per run |
| Express D4 by ordering the two calls | Exactly what D4 forbids: "the code must say so rather than inheriting it from the order of two calls" |
| Express D4 by passing the granted weapon id into `RandomizeStartingWeapons` so it skips that weapon | Requires drawing the weapon **before** the coffin draws, which moves every subsequent roll and breaks the determinism constraint (next row) |
| Draw the weapon at the start of `StepItemData`, or in an earlier phase | Consumes RNG ahead of the drop and coffin draws, so turning the setting on changes the coffins and the drops for a given seed. Spec §7: "Turning the setting on must not silently reshuffle the rest of a run's world" |
| Give the grant its own RNG stream, seeded from the run seed | Would also make the granted weapon stable when *other* settings change, which nothing asks for, at the cost of being the only pass in the app with a private stream. The pass already runs last, so the requirement is met without it |
| Probe route A only, and add route B as a second milestone if it fails | Costs a second hardware cycle for an answer one build can give. Kept as the fallback if character creation breaks under the double write (plan §6) |
| Write `equip_Wep_Right_GenId` alongside `equip_Wep_Right` | All four GenId fields are `-1` on all 1700 rows, so there is no example of a legal value and any choice is a guess. Spec §6 puts a third route out of scope |
| Make the setting a toggle plus a pool, like `RANDOMIZE ENEMIES` + `ENEMIES INCLUDED` | Two controls for one decision, and "toggle on, nothing ticked" would then need a meaning. The spec's none/one/many semantics already cover the off state |

---

## E4. Measurements

Everything below is against `data/vanilla/dvdroot_ps4` using
`app/tools/param_offsets.py`'s `load_defs`, `field_offsets`, `load_param`,
`param_rows` and `bnd4_members`, plus a ~25-line wide-FMG reader written for this
investigation (the one plan §4.1 specifies as `app/tools/fmg.py`). Scripts were
run with `PYTHONIOENCODING=utf-8`; without it, printing a weapon name or the
FMG's Japanese member name raises `UnicodeEncodeError` on this cp1252 console,
which looks like a parse failure and is not.

### M1 — field offsets and the hand bits

`EQUIP_PARAM_WEAPON_ST`, computed row size **316**:

| Field | Type | Offset |
| --- | --- | --- |
| `sortId` | s32 | 4 |
| `basicPrice` | s32 | 24 |
| `sellValue` | s32 | 28 |
| `properStrength` | u8 | **237** |
| `properAgility` | u8 | **238** |
| `properMagic` | u8 | **239** |
| `properFaith` | u8 | **240** |
| `rightHandEquipable` | u8:1 | **byte 256, bit 0** |
| `leftHandEquipable` | u8:1 | byte 256, bit 1 |
| `bothHandEquipable` | u8:1 | byte 256, bit 2 |

The four requirement offsets confirm `StartingWeapons.cpp`'s constants exactly.
`field_offsets()` reports width 0 for a packed bit field and does **not** give
the shift, so the bit positions were obtained by replaying its own
`bit_offset` accumulator; that replay is what plan §4.1 asks the generator to
reproduce (mask bit 0 of byte 256).

`CHARACTER_INIT_PARAM`, computed row size **300** — which confirms
`HunterTools.cpp`'s `kRowBytes`:

| Field | Type | Offset |
| --- | --- | --- |
| `equip_Wep_Right` | s32 | **16** |
| `equip_Subwep_Right` | s32 | 20 |
| `equip_Wep_Left` | s32 | 24 |
| `equip_Subwep_Left` | s32 | 28 |
| `equip_Helm` | s32 | 32 |
| `item_01` | s32 | 124 (stride 4) |
| `soulLv` | s16 | 192 |
| `baseStr` / `baseDex` / `baseMag` / `baseFai` | u8 | 197 / 198 / 199 / 200 |
| `itemNum_01` | u8 | 204 (stride 1) |
| `equip_Wep_Right_GenId` | s32 | 232 |
| `secondaryItem_01` | s32 | 268 |

`baseStr`/`baseDex`/`baseMag`/`baseFai` sitting in that order is the independent
confirmation that `properMagic` is **bloodtinge** and `properFaith` is
**arcane** — the origin minimums below land on D3's numbers exactly, which they
could not do under any other mapping.

```
python <<< "load_defs / field_offsets for EQUIP_PARAM_WEAPON_ST and CHARACTER_INIT_PARAM"
```

### M2 — the weapon id decomposition

`EquipParamWeapon.param` holds **1090** rows, ids 1000 … 41000000. Every id is a
multiple of 100. The decomposition that survives every row:

```
family  = id / 1000000
weapon  = (id / 100000) % 10
version = (id / 10000)  % 10
tier    = (id / 100)     % 100      # 0..10, so tier 10 is +1000
```

`tier` must be `% 100`, not `% 10`: tier 10 is `id + 1000` (e.g. `2001000`), and
`% 10` reads that as version 0 tier 0 and collides with the base row. Versions
observed across the param: 0, 1, 2, 8, 9. **145** rows are at tier 0.

Version 1 is **Uncanny** and version 2 is **Lost**, for all 26 weapons, read
from the game's own text — the backlog row states the opposite, and the spec
§4.3 correction is confirmed.

### M3 — the 78 rows, and the seven exclusions

Tier-0 rows with `rightHandEquipable` set: **85**.

Of those 85, exactly **78** carry a name in `武器名.fmg` that is neither absent
nor `*`; and exactly **78** are obtainable (sold in a `ShopLineupParam` row with
`equipType == 0`, or present in an `ItemLotParam` slot whose matching category
is 1). **The two sets are identical**, measured both ways:
`named-not-obtainable = []`, `obtainable-not-named = []`.

The seven excluded rows, with every signal:

| id | fam/w/ver | name | desc | shop | lot | price | sell | sortId |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 900000 | 0/9/0 | — | — | no | no | 0 | -1 | -1 |
| 12080000 | 12/0/8 | `*` | — | no | no | 160 | -1 | -1 |
| 37000000 | 37/0/0 | — | — | no | no | 140 | -1 | 70100 |
| 38090000 | 38/0/9 | — | — | no | no | 0 | -1 | -1 |
| 39000000 | 39/0/0 | — | — | no | no | 140 | -1 | -1 |
| 40000000 | 40/0/0 | — | — | no | no | 140 | -1 | -1 |
| 41000000 | 41/0/0 | — | — | no | no | 140 | -1 | -1 |

**This is seven, not the spec's six, and the candidate count is 85, not 84.** See
§E6 for what the spec said and why the difference does not move the picker.

The 26 weapons, by family/weapon: 2/0 Chikage, 4/0 Blade of Mercy, 5/0 Hunter
Axe, 5/1 Burial Blade, 7/0 Saw Cleaver, 7/1 Saw Spear, 8/0 Kirkhammer, 8/1
Ludwig's Holy Blade, 9/0 Beast Claw, 10/0 Rifle Spear, 10/1 Reiterpallasch, 11/0
Stake Driver, 12/0 Logarius' Wheel, 13/0 Tonitrus, 22/0 Threaded Cane, 23/0
Beasthunter Saif, 24/0 Beast Cutter, 25/0 Amygdalan Arm, 26/0 Holy Moonlight
Sword, 27/0 Rakuyo, 28/0 Boom Hammer, 29/0 Bloodletter, 30/0 Church Pick, 31/0
Whirligig Saw, 32/0 Simon's Bowblade, 38/0 Kos Parasite. Families 23–32 and 38
are The Old Hunters: **11 weapons, 33 of the 78 rows**, confirming the spec.

Left-hand (bit 1, not bit 0): **16** — Hunter Blunderbuss, Ludwig's Rifle, Hunter
Pistol, Evelyn, Repeating Pistol, Cannon, Rosmarinus, Flamesprayer, Wooden
Shield, Loch Shield, Hunter's Torch, Torch, Gatling Gun, Fist of Gratia, Church
Cannon, Piercing Rifle. Neither bit: **1** — Bare Fists (1000000). No row sets
both bits.

### M4 — the FMG

`msg/engus/item.msgbnd.dcx` holds 17 BND4 members. Weapon names are
`武器名.fmg` (member size 74540, offset 21168); `武器説明.fmg` is the
descriptions, a second independent signal that agrees with the name test on all
seven exclusions.

Header of the name FMG, byte for byte:

```
00 00 02 00 | 2c 23 01 00 | 01 00 00 00 | 80 05 00 00 | 80 05 00 00 | ff 00 00 00 | 28 58 00 00 ...
```

* `0x00` big-endian flag = 0
* `0x02` version = **2** (wide)
* `0x04` file size = 74540 = the member size exactly
* `0x08` = 1
* `0x0C` group count = 1408
* `0x10` string count = 1408
* `0x14` = 255
* `0x18` string-offset-table address = 22568 = `0x28 + 16 × 1408`

So the two framing checks plan §4.1 requires are both exact equalities on this
file. Entries: 1408, ids 1000 … 38090000, `7000000 → "Saw Cleaver"`,
`2000000 → "Chikage"`, offset 0 meaning "no string" (`1010000`, `16000000`).

### M5 — the four irregular names

Read, not composed:

| Weapon | version 1 | version 2 |
| --- | --- | --- |
| Ludwig's Holy Blade | `Ludwig's Uncanny Holy Blade` | `Ludwig's Lost Holy Blade` |
| Logarius' Wheel | `Logarius' Uncanny Wheel` | `Logarius' Lost Wheel` |
| Simon's Bowblade | `Uncanny Bowblade` | `Lost Bowblade` |
| Kos Parasite | `Uncanny Parasite` | `Lost Parasite` |

The last two drop the possessive entirely, so no prefix rule of any kind
reproduces them. All 78 uppercased names are **distinct**.

### M6 — requirements, origins and wieldability

The 22 origin rows, measured: all carry exactly one starting item (`item_01` =
goods 100, count 1, Hunter's Mark) with slots 2–10 at `-1`; `equip_Helm` is
230000 on 2000–2009 and `-1` on 3000–3009/3500/3501; all four weapon fields are
`-1`. `soulLv` is 10 except 2008/3008 (Waste of Skin) at 4.

The ten origins' `(str, skl, blt, arc)`:

```
2000 12 10  9  8      2005 14 13  7  6
2001 11 10  7  7      2006  9 13 14  9
2002  9 13  6  9      2007 10  9  5 14
2003 15  9  6  7      2008 10  9  7  9   (level 4)
2004  9 15  7  8      2009 10 10 10 10
```

Component-wise minimum: **(9, 9, 5, 6)** — D3's profile, exactly, with no
rounding or judgement in between. 3000–3009 repeat 2000–2009 field for field;
3500 and 3501 repeat 2000.

Requirement identity across versions: for all 26 weapons, the plain, Uncanny and
Lost rows carry identical `(237, 238, 239, 240)` — **0 of 26 differ**.

Wieldability as shipped, comparing each weapon's four requirements against each
of the ten origins' four stats: **4** weapons wieldable by every origin (12
rows), **9** more by at least one (27 rows) — so 13 weapons / 39 rows by at least
one — and **13** by none (**39 rows**). The extremes: Logarius' Wheel
`(20, 12, 0, 10)`, Rakuyo `(10, 20, 0, 0)`, Bloodletter `(14, 6, 16, 0)`, Kos
Parasite `(0, 0, 0, 20)`. Every one of the 78 is wieldable by every origin under
`(9, 9, 5, 6)`.

Upgrade tiers: all 78 rows have all ten `+100n` tiers present, so
`ApplyStatProfile` writes 11 rows for any choice — no "missing tier" case exists
on this data.

### M7 — text metrics, from the baked atlas

Measured with `settings_ui_verify.py`'s own `width()` against
`FontAtlasData.h`, never from character counts:

| String | scale 3 | scale 4 | scale 5 |
| --- | --- | --- | --- |
| `START WITH A TRICK WEAPON` | 491 | 657 | 818 |
| `START WITH HUNTER TOOLS` (shipped) | 464 | 624 | 772 |
| `RANDOMIZE STARTING WEAPONS` (shipped) | 537 | 723 | 898 |
| `DO NOT RANDOMIZE CAGED DOGS` (shipped) | 550 | 744 | 922 |

The new label is narrower than two shipped labels at every scale, so no rail or
pane constant can be the thing that breaks.

Picker rows: `UNCANNY HOLY MOONLIGHT SWORD` is 600 px at scale 3, 653 px with
`YES`, against the 791 px `C4520 LADY MARIA OF THE ASTRAL CLOCKTOWER` the band
was sized for. In the 8x8 fallback path, 28 + 3 + 3 = **34 characters** against
`pool_verify.py`'s 71-character budget; with an id column it would be 43, which
is what spec §4.1 quoted. Every one of the 78 names uses only letters, spaces and
`'`, all in `Font8x8.cpp`'s table.

Longest names: `Uncanny Holy Moonlight Sword` (28), `Ludwig's Uncanny Holy
Blade` (27), `Lost Holy Moonlight Sword` (25).

Paging: 78 rows at 11 visible (the instruction-line layout) is **8 pages**; at 12
it would be 7.

### M8 — the config arithmetic

`pool_verify.py` computes the settings block at **584** bytes and the worst-case
whole `defaults.cfg` at **635**, both as exact equalities. Adding
`trick_weapons_included=` (23 characters) + 78 + newline = **102**:

```
584 + 102 = 686      635 + 102 = 737
```

686 is inside `char buf[1024]`, with 338 bytes spare — matching spec §4.6's
figures, which is why the key name has to stay exactly `trick_weapons_included`.

### M9 — the two candidate routes

`equip_Wep_Right`, `equip_Subwep_Right`, `equip_Wep_Left`, `equip_Subwep_Left`:
**`-1` on all 1700 rows**, player origins and NPC templates alike — zero
non-empty values anywhere in the param. `equip_Helm`: set on **1557** rows. Both
confirm the spec.

`item_01..item_10` carrying weapon ids (`>= 900000`): **9 entries across 3 rows**,
and this is the shape route B copies:

```
183  slots 4,5,6 = 5000000 (Hunter Axe), 8000000 (Kirkhammer), 14000000 (Hunter Pistol), each count 1
184  slots 4,5,6 = 7100000 (Saw Spear),  8000000,              14000000,                each count 1
185  slots 4,5,6 = 5000000,              8000000,              14000000,                each count 1
```

Slots 0–3 of those rows hold goods at counts of 20, and slot 9 holds id 1000
count 2 — so a weapon in an ordinary item slot at count 1 is exactly what the
shipped data does.

`secondaryItem_01..10` with ids `>= 900000`: **1818 entries across 1217 rows**
(the spec says 120; see §E6). Not used by either route.

`equip_Wep_Right_GenId` and its three siblings: `-1` on all 1700 rows.

---

## E5. Risk analysis

### E5.1 The central risk: the field nobody has exercised

`equip_Wep_Right` is empty on every one of 1700 rows. `equip_Helm` is set on
1557 and produces the Black Hood a new character wears, so the engine
demonstrably reads *equipment* out of these rows — but "reads the row family" and
"reads this field" are different claims, and only the first is established.
Meanwhile the ordinary item slots demonstrably carry weapon ids in three shipped
rows. So the route the backlog row assumed is the unexercised one and the
alternative is the exercised one.

`docs/known-traps.md`'s "Byte decoding does not prove game behavior" and
`docs/plans/mergo-darkness.md` §2.7 are the standing reason not to argue past
this point: on row 8 the byte analysis was right in every particular and the
conclusion about what the game did with those bytes was backwards.

What the plan does: makes M3's build write **both** routes, with a different
recognisable weapon on each, so one hardware cycle discriminates six distinct
outcomes (plan §6). The cost is one narrow ambiguity — if character creation
breaks, either write could be the cause — and the response is to re-probe with
one write at a time, which is the two-cycle plan that was rejected as the
*default* but is exactly right as the *fallback*.

Residual risk after a negative result: route A might fail for a reason a third
field would fix (a GenId, an equip index). That is deliberately out of scope
(spec §6) and would be a new backlog row, because guessing a legal value for a
field that is `-1` on 1700 rows is not investigation, it is permutation.

### E5.2 Order, and the 78 characters

A regenerated table in a different order silently remaps a saved selection onto
the wrong row, and with 78 rows the most likely wrong row is *another version of
the same weapon* — a Lost Saw Cleaver where a Saw Cleaver was ticked, which is
the one error a player would not notice and would not report usefully.
`Decode`'s length check catches a changed **count** and cannot catch a changed
**order**.

Bounds: the order is a pure function of the vanilla tree (base name, then
version), the generator's `--check` fails if the committed file drifts, and a
count change fails `Decode` into "nothing ticked", which is the safe state. The
plan's stop conditions halt on a row count other than 78 for this reason.

### E5.3 Two shipped verifiers that this feature makes fail

Not a hypothetical: `hunter_tools_verify.py verify` and
`starting_weapons_verify.py verify` both assert "only member X differs", and both
assert per-row or per-weapon containment that a second feature's write violates.

* Row 34 on + row 37 on → `EquipParamWeapon.param` also differs (H-I1 fails) and
  `equip_Wep_Right` differs inside an origin row (H-I3 fails).
* Row 5 on + row 37 on → `CharaInitParam.param` also differs (SW-I1 fails) and
  the granted weapon's requirement bytes changed although it was not assigned to
  a slot (SW-I6, SW-I7 fail).

This matters more than it sounds: those two tools are what the developer runs
against a hardware output tree, and a *correct* combined run reporting FAILED
during the most delicate hardware test in the feature is the kind of false signal
that gets a good build discarded. Hence the `--granted <id>` tolerance in M3,
default off so every existing command line keeps meaning what it did.

### E5.4 A verifier coupled to another feature's spec

`settings_ui_verify.py` case 3 rebuilds the expected category membership from
`docs/features/randomizer-settings-ui/spec.md` §7.1 and maps prose names to
labels through `PROSE_TO_LABEL`. Adding a setting to the model and not to that
table fails case 3 with a message about the spec, which reads like the spec is
wrong. Both edits are one line each and belong in the same pass; the plan lists
the spec file in §5 for that reason.

This is working as designed — the coupling is what stops a setting existing in
code and nowhere in the design record — but it is invisible from the C++ side.

### E5.5 `PickerStrings` aggregate initialisation

`PickerStrings` is an `inline constexpr` aggregate initialised positionally at
three sites, each listing eight values. Appending `bool showRowId` means a site
that is not updated compiles and **zero-initialises the new field to `false`** —
which would silently drop the id column from the enemy, boss and skip pickers,
changing three shipped, hardware-proven screens. There is no compiler warning
worth relying on here.

Bounds: `pool_verify.py` already parses this file's string literals, so a case
asserting that all four declarations mention `showRowId` explicitly is cheap and
catches exactly this. The plan requires it.

The inverse risk — putting the flag on the *component* instead, as a fifth
`Draw` parameter — was considered and is worse: five call sites, two of which are
in the world editor, and a missed one is a compile error only if the parameter is
required, which means touching all five anyway.

### E5.6 Single-byte fields, twice over

Three of this feature's writes are single bytes: the four requirement values
(237–240) and the item count (204 + slot). `HunterTools.cpp` and
`StartingWeapons.cpp` both carry warnings about this, and the hand bits at byte
256 are one step worse — a packed bit field, where a byte-wide write would clear
`leftHandEquipable` and three other flags on a weapon row.

The plan's answer is structural rather than a warning: the hand byte is **read
only, in Python only**, by the generator. The C++ never touches offset 256.

### E5.7 Determinism, in both directions

The pass draws once, and it is the last RNG consumer of the last phase of the
run, so turning the setting on cannot move any other roll — which is the
direction spec §7 requires.

The other direction is not guaranteed and is not required: turning
`RANDOMIZE ENEMY DROPS` or `RANDOMIZE STARTING WEAPONS` on changes the RNG state
the grant draws from, so it can change which weapon is granted for a fixed seed.
That is true of every pass in the app that shares the stream, and no requirement
asks otherwise. Recorded here so a reviewer does not read it as an oversight.

### E5.8 Low enough not to change the plan

* **The `.pkg` growing.** The table is 78 pointers and 78 short strings.
* **The archive rewrite.** The item-data archive is already rewritten whenever
  any param feature is on, and this feature adds no member and resizes nothing.
* **Row 5's coffin draws colliding with each other.** `DrawDistinct` removes as
  it draws, so the five coffin claims are always distinct weapons and the
  owner-ranked writer never sees a `CoffinSlot`-versus-`CoffinSlot` conflict.
* **Waste of Skin at level 4.** Its stats are `(10, 9, 7, 9)`, all at or above
  the profile, so the level-4 origin is not a special case.

---

## E6. What the spec's appendix claimed, and what the trace found

The appendix was treated as leads and checked item by item.

| Claim | Found |
| --- | --- |
| `HunterTools.cpp` — `kOriginRows`, `FirstEmptySlot`, the bounds check; "what is genuinely shared is the row list and the guard pattern" | Confirmed, and the item-slot offsets are shared too. §E2.1 |
| `EnemyRandomizer.cpp` `StepItemData` step 1 is where the hunter-tools pass sits relative to starting weapons | Confirmed, and step 1 is also the **last** step that draws randomness in the whole job — the fact plan P8 rests on. §E2.3 |
| `StartingWeapons.cpp` `ApplyStatProfile` / `kStatProfiles` is the rewrite D3 reuses | Confirmed, with one thing the spec could not know: it is in an **anonymous namespace**, so "reuse unchanged" requires moving it. §E2.2 |
| `ModelPoolSelection.h`'s `DefaultSelected` is the polarity this needs | Confirmed exactly |
| `SettingsModel.cpp` `kSettings` — "one entry adds a row, and `SettingKind` is where a fourth pool kind would go" | Confirmed, and stronger than claimed: **no screen file changes at all**. §E2.4 |
| `gen_pool_table.py` is the model for a weapon table | Confirmed as a *pattern*; not as a host. §E3 |
| `item.msgbnd.dcx` FMG: group table at 0x28, 16-byte entries, offset table address at 0x18, 8-byte offsets, UTF-16LE, "about twenty lines" | Confirmed in every particular. The version byte is at **0x02**, not 0x01, and the two framing equalities of §E4 M4 are worth asserting |
| `ShopLineupParam` + `ItemLotParam` make the obtainability test cheap; the lot param has eight item-id fields | Confirmed. Categories are a parallel array at +32; weapons are category **1** |
| The id decomposes as family / weapon / version / tier; "derive it rather than assuming any single divisor" | Confirmed, with the tier caveat of §E4 M2 |
| Uncanny/Lost order is the opposite of the backlog row's | Confirmed |
| Hand bits are packed bits in one byte | Confirmed: byte 256, bits 0 and 1 |
| cp1252 console; `PYTHONIOENCODING=utf-8` needed | Confirmed the hard way, on the first run |
| `pool_verify.py`'s config cases are exact equalities and will fail | Confirmed: 584 → 686, 635 → 737 |
| Row 34's open item O2 should be resolved once for both features | Agreed, and it is the main argument for the shared header. §E2.1 |
| Dead ends: no reference equivalent; `weaponCategory` fails; `StartingWeaponLists.h` is not a classification; composing names fails; no example of a weapon in the char-init weapon fields; the GenId fields are empty | All confirmed. None was re-walked further than needed to confirm it |

### Three corrections to the spec's §4

None of them moves the picker, which is 78 rows on every count.

**C1 — the candidate count is 85 and the exclusions are seven, not 84 and six.**
Spec §4.2 says "counting every right-hand row at tier 0 regardless of version
gives 84, six more than the picker" and enumerates six. The measurement gives
**85**, and the seventh excluded row is **id 900000** — family 0, weapon 9,
version 0, no name, no description, in no shop and no item lot, price 0,
`sellValue` -1, `sortId` -1. It is the same species of row as the version-9
placeholders and fails both membership tests. The spec's other five and its
`38090000` are all confirmed. Because the plan derives membership rather than
subtracting a list, this correction changes no instruction — it only means the
verifier must report the excluded rows rather than assert a count of six.

**C2 — "neither hand: 5" is wrong.** Spec §4.2's third group is "Bare Fists, and
four rows the game's text does not name". By the hand bits, only **Bare Fists**
has neither bit set; families 37, 39, 40 and 41 **do** have
`rightHandEquipable` set, which is why they are candidates that the name and
obtainability tests exclude rather than rows the hand test never sees. Same 78.

**C3 — "120 further rows list weapons in the secondary-item slots"** is
measured here as **1217 rows / 1818 entries** by an `id >= 900000` test. The test
is crude (it does not prove every such id is a weapon) and the field is used by
neither route, so this is recorded and not pursued.

### One ambiguity the plan resolves rather than asks about

Spec §8's probe table reads "**Nothing at all** → Either the game does not read
the weapon fields, or it does not read these origin rows. The feature cannot be
built this way and §7's deletion rule applies", while D5 and §6 say the inventory
route is "an accepted outcome, not a failure" and not a fallback. Read together,
"nothing at all" can only mean "nothing by **either** route" — which is precisely
what a build writing both routes tests in one cycle. The plan's §6 table says so
explicitly, so no reading of a negative result depends on which sentence was read
first.

---

## E7. Anything that could not be established

* **Whether the game reads `equip_Wep_Right` at all.** The whole feature. Nothing
  offline touches it (§E5.1). The console decides.
* **Whether a weapon in an ordinary item slot reaches a new character's
  inventory.** Shipped rows 183/184/185 do it, which is evidence that the field
  is *used*, not proof that the engine instantiates those rows or that it would
  do it for an origin row.
* **Which of the two ten-row origin blocks the game actually instantiates.** The
  rows are identical in every field either feature touches except `equip_Helm`.
  Writing all 22 is row 34's hedge and row 037 inherits it; the narrowing is
  blocked on the same hardware test for both.
* **Whether a granted weapon behaves like any other once held** — equippable,
  transformable, upgradeable, gem-fittable. Assumed; spec §8's hardware list is
  the verification.
* **Whether the requirement reduction has side effects beyond wieldability.**
  It persists on that weapon's ten tiers for the whole run, which spec D3
  accepts deliberately. Nothing was found that reads `properStrength` for
  anything other than the wield check, and nothing offline could prove that.
* **Which weapon a given seed draws.** Deliberately not mirrored: matching
  `std::mt19937` and `uniform_int_distribution` across languages tests the PRNG,
  not the rules (`boss_verify.py`'s standing position). The mirror asserts the
  granted weapon is *one of the ticked ones*; "the same seed grants the same
  weapon" is a hardware check.
* **Whether `equip_Wep_Right_GenId` is a required companion to
  `equip_Wep_Right`.** Both are `-1` on all 1700 rows, so the data says nothing
  in either direction. If route A fails and route B works, this is the first
  place a follow-up backlog row would look — and it would be a new row, not an
  extension of this one.

---

<!--

Nothing in this file is an instruction. If something here tells the implementer
what to do, it belongs in plan.md.

Nothing in this file is process history. If something here records who said what
and when, it belongs in log.md.

-->
