# Plan Evidence 011 — Randomize Shop Armour and Shop Items

**Plan:** `docs/features/011-randomize-shop-items/plan.md`

**Spec:** `docs/features/011-randomize-shop-items/spec.md`

---

> **This document is the investigation behind the plan, not instructions.** The
> implementer consults it when a question needs context the contract left out.

---

## E1. Reference trace

`reference/Randomizer/MainWindowComponents/RandomizeFunctions.cs`,
`RandomizeShopItems`, read in full from `:609` to `:1315`.

### E1.1 The gate

`StartFunctions.cs:1541` calls the function under `shopBool ||
startingWeaponsOnlyBool`, so one function carries three features. Inside it the
branches are gated separately: weapons on `startingWeaponsOnlyBool` (`:892`),
armour on `shopBool` (`:984`), consumables on `shopBool` (`:1004`). Confirms
spec §3 exactly, and confirms `docs/plans/param-features.md` §2's correction of
`docs/windows-randomizer-technical-review.md` §5.2.

`keepGuns` appends `14000000` and `6000000` to the skip list (`:639-643`). Both
are weapon ids, so it cannot affect either of this feature's buckets. The port
has no `keepGuns` — row 5 split it into positive toggles instead.

### E1.2 The skip list

`:635-638` adds the three strings `"1000"`, `"900"`, `"240"` to
`vialAndBulletIDsList`. Both loops compare
`Rows[i].Cells[0].Value.ToString()` against every entry (`:849-856`,
`:879-886`), so the match is on the **item id**, in both passes. Nothing about
the row id or the type enters the comparison. This is the whole of the
protection; there is no second list anywhere in the file.

Note the comparison is `==` on `.Value.ToString()` — not the broken
`Cells[0].ToString()` display-string comparison that defect S-2 concerns, which
is in the weapon branch's reroll guard at `:896`. The skip list itself is
correct in both loops.

### E1.3 The pool build, `:848-875`

One loop over every row. `addToList` starts true and is cleared by a skip-list
hit. Surviving rows are appended, **as ids and with duplicates**, to
`shopWeaponList` (`Cells[7] == "0"`), `shopArmorList` (`"1"`) or
`shopConsumableList` (`"3"`). The build is **not** gated on any checkbox: the
pools are always built and the gate only decides whether they are drawn from.
It consumes no randomness.

### E1.4 The write, `:877-1022`

A second loop over every row in the same order, with the same skip-list filter
(`changeData`). Per row, an `if / else if / else if` on `Cells[7]`, each arm
also requiring `List.Count > 0`:

* armour, `:982-1001`: `randomNumber = universalRand.Next(0, shopArmorList.Count)`,
  then `Cells[0].Value = Int32.Parse(shopArmorList[randomNumber])`, then
  `shopArmorList.RemoveAt(randomNumber)`;
* consumables, `:1002-1021`: the same three statements on
  `shopConsumableList`.

Only `Cells[0]` is assigned. No other cell of the row is written anywhere in the
function outside the weapon branch's `EquipParamWeapon` work.

**Three things this settles.**

1. **Selection sampling, not Fisher-Yates.** Draw an index, take it, remove it.
   Because the pool was built from precisely the rows this loop writes, it
   empties on the last one, so the result is a uniform random permutation of the
   bucket's multiset. The `Count > 0` guard is what stops an
   `Next(0, 0)`-shaped call on an empty list, and the port needs the same guard
   for a stronger reason (§E5.2).
2. **No reroll and no self-match guard** in either of these two arms — unlike
   the weapon arm at `:896`. Confirms spec §3's "the armour and consumable
   branches are clean" and §7.7. There is nothing here to decide about.
3. **The two buckets interleave** in row order in the reference, because both
   arms live in one loop. The port uses two sequential loops (§E3), which is
   distributionally identical.

### E1.5 Where it sits relative to other passes

In the reference this is one function that opens, edits and writes the whole
param archive itself, and the enemy/drop work is elsewhere. Pass ordering
against the port's other passes therefore has **no** reference answer to match:
the reference has no character-creation grant, no hunter-tools grant, and its own
RNG (`universalRand`) is consumed in an order this port already does not
reproduce (`docs/windows-randomizer-technical-review.md` §7, no seed parity).
The ordering in plan §4.3 is chosen on the port's own terms, which is why it is
Q1 rather than a traced fact.

### E1.6 Where the trace contradicted nothing

Every count and claim in spec §2, §3 and §4 F1–F9 was re-derived here or in §E4
and agreed. No contradiction with the spec's §2 acceptance target was found.

---

## E2. What exists in the port

### E2.1 `app/src/Randomizer/StartingWeapons.{h,cpp}` — read line by line

The file is row 5: three toggles, two of them over the five Hunter's Dream rows
and one over the remaining weapon listings. What matters here:

* **It already owns the constants this feature needs.** `kShopEquipId = 0`,
  `kShopEquipType = 23`, `kEquipTypeWeapon = 0`, and
  `kNeverTouchEquipIds = { 1000, 900, 240 }` with `IsNeverTouched`, all in its
  anonymous namespace (`:17-22`, `:55-60`). Spec §7.2 says the two features'
  skip lists must not drift apart, so they cannot simply be retyped. Carving
  them into `ShopLineup.h` is the same move feature 037 made with
  `CharaInitRows.h` and `WeaponRequirements.h`, and it is the only one that
  leaves a single copy.
* **Its shop pass is Fisher-Yates over the stock**, then a deal-out in row
  order, not the reference's selection sampling. Both are uniform permutations;
  the difference is only how many draws are consumed and in what order. Row 5 is
  shipped and hardware-proven, so it is **not** being changed to match the
  reference shape (§E3).
* **Its shop pass excludes the five starting rows** via `IsStartingSlotRow`, so
  two of its own toggles cannot fight. This feature needs no analogue: nothing
  else writes armour or consumable listings.
* **It takes a `WeaponRequirementWriter&`** and rewrites `EquipParamWeapon` stat
  bytes. This feature touches no second member at all, which is why it needs no
  writer, no owner ranking, and nothing like feature 037 D4's arbitration
  (spec §4 F11).

Extending this file was rejected — see §E3.

### E2.2 `EnemyRandomizer.cpp`'s item-data phase

`StepItemData` (`:980-1173`) is a three-step state machine: step 0 reads and
decompresses `/param/gameparam/gameparam.parambnd.dcx`; step 1 parses the BND4
members and runs every param feature in sequence; step 2 compresses and writes.
Adding a pass is one block in step 1, and the archive is still read and written
once — spec §7.5 satisfied by construction.

The order in step 1 today: drops → row 5 (`RandomizeStartingWeapons`) → hunter
tools (+ the temporary rune probe) → trick-weapon grant → left-hand grant. The
two grants carry explicit comments saying they are last **so that turning them
on cannot move any earlier roll** (spec 037 §7). Each block locates the param
members it needs itself and `Fail`s with its own message, which is the pattern
plan §4.3 follows.

### E2.3 The settings chain, as rows 37 and 38 built it

Seven sites, all confirmed by reading them:

| Site | What a toggle needs |
| --- | --- |
| `Randomizer/RandomizerDefaults.h` | one `bool … = false;` |
| `Randomizer/RandomizerDefaultsStore.cpp` | one key in `FormatSettings`'s format string and argument list, one `strcmp` arm in `ApplySettingKey` |
| `UI/SettingsModel.h` | one `SettingId` |
| `UI/SettingsModel.cpp` | one `kSettings` entry: id, category, `SettingKind::Toggle`, label, `&RandomizerDefaults::field`, help |
| `Game/WorldActivation.cpp` | one `options.x = run.x;` line and one `anythingOn` term |
| `UI/WorldEditorScreen.cpp` | the `SKIPPING` line, the report line, the item-data-line condition |
| `Randomizer/EnemyRandomizer.h` | the option, the `AnyParamFeature()` term, the result counter |

Neither settings screen needs a change: `SetupDefaultsScreen` and
`WorldEditorScreen` render from the model and reach a value only through
`SettingValueText` / `AdjustSetting`. `WorldStore`'s recipe is
`FormatSettings`'s text, so persistence per world comes free with the store key.

### E2.4 The verification tools that touch this table

| Tool | Relationship | Treatment |
| --- | --- | --- |
| `starting_weapons_verify.py` | owns `ShopLineupParam`; 615 lines; parses the reference C# lists so it stays independent of the C++ it validates | **extend** — spec §6, and F13's correction is only possible here |
| `pool_verify.py` | pins the settings block at an exact byte count (`:700-733`) | update the arithmetic and add the two key cases |
| `settings_ui_verify.py` | case 1 pairs every `RandomizerDefaults` bool with exactly one model entry; case 3 checks the model against `docs/features/randomizer-settings-ui/spec.md` §7.1; case 5 measures every label and help string against the atlas | add `PROSE_TO_LABEL` entries, update the settings-UI spec's §7.1 |
| `worlds_verify.py` | `WIZARD_OPTIONS_MAPPING` and `WIZARD_RUN_DECISION` are ordered lists compared to the source by regex (`:1384-1440`); the recipe round trip derives keys from the source and needs no edit | add both fields to both lists, and to the `SKIPPING`-line field list |
| `drops_verify.py` | `D-I1`: only `NpcParam.param` may differ; no tolerance flags | leave alone; run against a tree with the shop settings off |
| `hunter_tools_verify.py` | `H-I1`: only `CharaInitParam.param` may differ, with `--granted` / `--granted-left` tolerances for the two grants but none for this table | leave alone, same reason. It already reports FAILED when `RANDOMIZE SHOP WEAPONS` is on, so this is a pre-existing convention |
| `itemdata_verify.py` | proves the write path emits a valid archive; `verify` there assumes nothing changes and belongs to increment 1 | use `roundtrip` only |

### E2.5 F13, read independently

`starting_weapons_verify.py`'s `compare()` loops
`for rid, vat in van_shop.items()` over **all 1,288** listings (`:253`). For any
listing whose `equipId` changed it asserts `o_id in van_weapons` → `SW-I4`
(`:275-277`), and otherwise counts it into `shop_changed`, printed as "other
shop weapon rows changed". An armour or consumable id fails SW-I4 outright. The
spec's F13 is correct, and the repair is narrow because two nearby things are
already right:

* the byte comparison `van_plain[vat+4:vat+32] != out_plain[oat+4:oat+32]`
  (`:262`) already covers every field except `equipId`, for **every** listing —
  including `equipType` at offset 23. So spec §8 criterion 2 is already
  asserted for the two new buckets and needs nothing new;
* `stock()` (`:286-297`) is already type-filtered to `EQUIP_TYPE_WEAPON`, so
  SW-I8 does not see the new buckets and does not move.

What has to be re-scoped to weapon rows is exactly SW-I3, SW-I4, SW-I5 and the
`counts["shop"]` figure. Everything else either already discriminates or applies
to all listings correctly.

---

## E3. Alternatives considered

| Approach | Why rejected |
| -------- | ------------ |
| Extend `StartingWeapons.cpp` with the two buckets | It is row 5's file by name, by header comment and by its `StartingWeaponOptions` / `StartingWeaponsResult` types, and row 5 is shipped and hardware-proven. Three more toggles in one function makes every edit to either feature an edit that can break the other, and the shared constants can be shared without it |
| Copy the skip list into the new file | Spec §7.2 forbids exactly this. Two copies of `{1000, 900, 240}` is the drift the spec names |
| Put the shared constants in `StartingWeapons.h` and include that | `ShopStock.cpp` would then depend on row 5's options, result and `WeaponRequirements.h` for three integers. A leaf header is what `WeaponRequirements.h` and `CharaInitRows.h` established as the house answer |
| Fisher-Yates, matching row 5's shop pass | Distributionally identical, and it would read consistently with the neighbour. Rejected because selection sampling is the reference's own form (`:986-999`), traces to it line for line, and `CLAUDE.md` §7 makes reference fidelity the default where nothing argues otherwise. Row 5 is not being changed either way |
| One interleaved loop over all rows handling both types, exactly as the reference does | Each bucket is still a uniform permutation of its own multiset, and the two are independent, so the joint distribution is identical. Two sequential loops make each bucket self-contained, make "this bucket was not enabled so it drew nothing" trivially true, and avoid a loop whose body has to reason about two pools at once. Nothing observable distinguishes them: there is no seed parity with the Windows tool (spec §7.10) |
| Separate the pool build from the write into two passes over the rows, as the reference does | Two filters that must agree is the one way to break the exactness of the permutation. One pass that fills `targets` and `pool` together cannot disagree with itself |
| A per-shop or per-currency pool | Contradicts D2. Not reopened |
| Protect the once-only listings, or the upgrade materials, or the chalice items | Contradicts D3 and D4 and §7.1, and adds a protected list the reference does not have |
| A new verifier, `shop_stock_verify.py`, parsing the table a second time | Spec §6 names the extension explicitly. A second parser would also leave F13 unfixed in the first tool, so a combined run would still report false failures |
| Rename `starting_weapons_verify.py` to something that covers the whole table | `CLAUDE.md` §1 pins the path, `docs/testing.md` names it, and several implementation reports reference it. The rename buys a better name and costs edits in all of them |
| Infer which settings were on from the output instead of declaring them with flags | "Off" and "on but the permutation was the identity" are indistinguishable in principle. They are separated in practice by astronomical odds, but a verifier that guesses cannot state spec §8 criterion 7 at all. `--granted` already set the declaration precedent |
| An owner-ranked writer for `equipId`, like feature 037 D4's for requirements | Spec §4 F11: the three `equipType` values partition the table exactly (measured, §E4 M1), so no listing is a candidate for two features. There is nothing to arbitrate |

---

## E4. Measurements

All against `data/vanilla/dvdroot_ps4`. The shop-table numbers come from one
reusable probe; `P` below stands for this preamble, run from `app/tools`:

```python
import sys, struct, collections
sys.path.insert(0, ".")
from boss_verify import read_dcx
from param_offsets import bnd4_members, param_rows
p = bytearray(read_dcx("../../data/vanilla/dvdroot_ps4/param/gameparam/gameparam.parambnd.dcx"))
m = {n: (o, s) for n, o, s in bnd4_members(p)}
rows = lambda k: {r: m[k][0] + x for r, x in param_rows(p[m[k][0]:m[k][0] + m[k][1]])}
i32 = lambda a: struct.unpack_from("<i", p, a)[0]
sh = rows("ShopLineupParam.param")
NEVER = {1000, 900, 240}
```

| # | Quantity | Value | How measured |
| - | -------- | ----: | ------------ |
| M1 | `ShopLineupParam` listings, by `equipType` | **1288 = 644 type 0 + 220 type 1 + 424 type 3**, no other type | `P` then `collections.Counter(p[a+23] for a in sh.values())` |
| M2 | Row layout | 32-byte rows; `equipId` s32 @ 0, `value` @ 4, `mtrlId` @ 8, `eventFlag` @ 12, `qwcId` @ 16, `sellQuantity` s16 @ 20, `shopType` u8 @ 22, `equipType` u8 @ 23, `value_SAN` s16 @ 24, 6 pad | `python app/tools/param_offsets.py fields data/vanilla/dvdroot_ps4 SHOP_LINEUP_PARAM` |
| M3 | Armour bucket | **220 listings, 44 distinct ids, exactly 5 listings each**; expected fixed points Σnᵢ²/N = **5.0**; all 220 `sellQuantity == 1` | `P`, group type-1 rows by `i32(a)`, then `Counter` |
| M4 | Consumable bucket | 424 listings; **397 shufflable, 57 distinct**, per-item counts ∈ {4, 5, 9, 10, 13, 14}; expected fixed points **8.693**; `sellQuantity`: **357 unlimited (−1), 40 once-only (1)** | same, type 3, excluding `NEVER` |
| M5 | The skip list on real data | `1000` → 13 listings, `900` → 13, `240` → 1; **all 27 are type 3**, so no armour listing is ever skipped | `P`, filter `i32(a) in NEVER` |
| M6 | Id-namespace collisions | The three buckets' id sets are **pairwise disjoint** (0 overlaps). But **across param tables they are not**: armour id `10000` is also an `EquipParamWeapon` row id; consumable ids `3000` and `7000` are also weapon rows; consumable ids `1100, 1200, 1300, 1400, 3000` are also `EquipParamProtector` rows | `P` plus `rows("EquipParam{Weapon,Protector,Goods}.param")` set intersections |
| M7 | Every shuffled id resolves | All **44** armour ids have an `EquipParamProtector` row (273 rows); all **57** consumable ids have an `EquipParamGoods` row (295 rows); `240` has **neither** | same |
| M8 | Config byte cost | `randomize_shop_armour=1\n` = **24** bytes, `randomize_shop_items=1\n` = **23**; settings block 728 → **775**; worst-case `defaults.cfg` 779 → **826**; `char buf[1024]` leaves **249** spare | `len()` on the key strings, against `pool_verify.py:689-733`'s existing arithmetic |
| M9 | Label and help metrics | `RANDOMIZE SHOP ARMOUR` **444 px**, `RANDOMIZE SHOP ITEMS` **395 px**, against the 700 px pane row (the shipped `RANDOMIZE SHOP WEAPONS` is 452 px); both help strings wrap to **4** lines against `HELP_BODY_MAX_LINES = 11` | `python -c "import settings_ui_verify as s; s.width(lab, s.ROW_SCALE); len(s.wrap(h, s.ROW_SCALE, s.HELP_W))"` from `app/tools` |
| M10 | Listing-id blocks | 20 blocks by `id // 10000`: `0`×6, `10–14`×144 each, `20–24`×51 each, `51–54`×38 each, `61–64`×38 each, `90`×3 | `P`, `Counter(r // 10000 for r in sh)` |
| M11 | Expected changed listings | armour **215 of 220**, items **≈388 of 397** (N − Σnᵢ²/N) | M3, M4 |
| M12 | Archive shape | 65 members; `ShopLineupParam.param` 96,278 bytes; `EquipParamProtector.param` 85,690; `EquipParamGoods.param` 45,246 | `P`, `len(m)` and `m[k][1]` |

M1–M5, M7 and M10 re-derive spec §4 F1, F2, F3, F4, F5, F8, F9 and F10's block
table and agree with all of them. M6 and M11 are new.

**Why M6 matters.** The verifier's existence check must be dispatched on
`equipType` and run against the bucket's **own** table, because an id can exist
in the wrong table by coincidence: a consumable id landing in an armour listing
could satisfy a naive "exists in `EquipParamProtector` or `EquipParamGoods`"
test for five of the 57 ids. The multiset check (SW-I11) is the load-bearing one;
the existence check (SW-I10) is a second, weaker net.

---

## E5. Risk analysis

### E5.1 The verifier is wrong in this feature's presence (spec F13)

Confirmed independently, §E2.5. Severity: it is not a false alarm that can be
read past — the moment a run with either setting on is verified, SW-I4 fires
several hundred times and the real signal is buried. Probability: certain.
What bounds it: the fix is confined to `compare()` and is a dispatch on a byte
the tool already reads. The plan puts the correction and its selftest cases in
M3, before any hardware run exists to verify.

The subtler part is the **counter**: `counts["shop"]` is printed as "other shop
weapon rows changed" and would silently absorb 600 armour and consumable
changes, making a row-5 regression invisible behind a plausible-looking number.
That is why the re-scoping is listed as a change and not left implicit.

### E5.2 The empty-pool draw

`std::uniform_int_distribution<int> pick(0, (int)pool.size() - 1)` with an empty
pool is `(0, -1)`, which is undefined behaviour — and on this toolchain the
plausible outcomes are a garbage index and an out-of-range write into the
archive. The reference's own arms are guarded by `List.Count > 0`.

On real data it cannot happen: one filter builds `targets` and `pool` together,
so `pool.size() == targets.size()` at the start and the pool empties exactly on
the last target. The guard is there so that this stays true if the filter is
ever edited. Cost: one comparison per listing.

Row 5 has the same shape of guard by construction (`stock.size() ==
targets.size()`, and its Fisher-Yates loop is `for (i = size; i > 1; i--)`).

### E5.3 The settings-chain verifiers fail mid-milestone

`settings_ui_verify.py` case 1 fails as soon as `RandomizerDefaults` gains a bool
with no model entry; case 3 fails until `docs/features/randomizer-settings-ui/spec.md`
§7.1 lists the setting; `pool_verify.py`'s byte figures are exact equalities.
None of these is a defect in the feature — they are the interlocks working — but
an implementer who runs the checks between steps of M2 will see failures that
mean nothing. The plan orders M2's steps so the interlocking edits land together
and states the target numbers so there is nothing to derive under pressure.

Precedent: feature 037's plan hazard table carried the same three lines, and its
implementation report records them as the expected mid-milestone states.

### E5.4 Determinism and what a seed means

All passes share one `std::mt19937`. Consequences, in order of how much they
matter:

* **An off run is unchanged.** Both new passes return before parsing when both
  options are false, so no draw is consumed and today's runs reproduce exactly.
  This is the invariant that actually protects shipped behaviour.
* **Row 5 is unchanged when row 11 is on**, because the new passes run after
  `RandomizeStartingWeapons` returns.
* **The grants move when row 11 is on.** Each new pass consumes one draw per
  listing (220, then 397), so the trick-weapon and left-hand draws land
  elsewhere in the stream. This is already true of `RANDOMIZE SHOP WEAPONS`,
  which consumes 638 draws ahead of them. It is visible to a player only as "a
  different weapon was granted at the same seed when I also turned shop armour
  on", and the settings are part of the recipe that a world stores, so a world
  re-generates identically.
* **Neither new setting is independent of the other in the stream.** Armour's
  220 draws precede items', so enabling armour changes the item shuffle at a
  fixed seed. No ordering avoids this with one stream; the alternative would be
  a second engine seeded per feature, which nothing else in this port does.

§8 Q1 puts the ordering choice to the developer because it trades a property one
spec recorded (037 §7's "the grants are last") against a property nobody has
asked for (shop settings not moving the grants).

### E5.5 Unreachable shop families (spec F10 / D4)

If families C and D are not stock a player can reach, 112 of the 397 consumable
listings are a sink and the global permutation moves items into them and off
sale. Bounded by spec F15: nothing whose loss blocks a run is in the bucket, the
two items that would matter are protected outright, and the Hunter Chief Emblem's
door has a walkable alternative. The plan does nothing about it by D4's
instruction; the hardware step records what is reachable. The restriction, if it
is ever wanted, is a filter on `rowId // 10000` in the one loop that builds both
lists (§E4 M10) — genuinely a one-line change, as D4 assumed.

### E5.6 Purchase limits detaching from items (spec F8 / D3)

Measured: 40 of the 397 consumable listings are once-only and 357 unlimited, so
after the shuffle each of the nine normally-once-only items lands on an unlimited
listing with probability 357/397 ≈ 90 %. Root chalices and the Resonant Bells
become repeatable purchases, and about 36 ordinary consumables become buy-once.
This is the reference's behaviour, D3 accepts it, and spec §2 states it to the
player. No implementation consequence; recorded here so a reviewer does not read
it as a defect discovered late.

### E5.7 Chalice scope

24 of the 57 shuffled consumables are chalice materials and root chalices.
`docs/design-decisions.md` puts chalice **dungeons** out of scope and names the
three settings affected; none of them is this one. What this feature does is
change which Hunter's Dream listing sells a ritual material — a shop edit, in a
param the port already rewrites, with no chalice map, chalice shop or chalice
behaviour involved. Excluding them would be an unrequested deviation from the
reference, which §7.1 forbids. Spec §6 states the same conclusion.

### E5.8 Incremental build and struct growth

`RandomizerDefaults` gains two bytes, and `WorldStore`, both settings screens and
`WorldActivation` all hold whole copies. The Makefile's missing header
dependency was fixed on 2026-09-26, but the failure mode it produced —
`Application.o` allocating an old `sizeof` while the constructor wrote the new
layout, surfacing as a crash at the first screen switch — cost eight hours to
find (`docs/known-traps.md`). `make clean && make` in M2 costs minutes.

### E5.9 Judged low enough to change nothing

* **A listing dealt its own vanilla item** (spec F7): expected 5 of 220 and 8.7
  of 397. No reroll, per §7.7 and the reference. The verifier must not require
  every listing to change, which is what SW-I14's 60 % threshold is for (chosen
  so far from the ~2 % expected unchanged rate that no correct run can fail it,
  and low enough that a half-completed pass cannot pass it).
* **`shopType`** (offset 22) is never read by this feature. The buckets are
  defined by `equipType` alone, per D2. The field is covered by the
  "unchanged outside `equipId`" assertion for free.
* **Archive size**: no member is resized, so the ~15× stored-block inflation
  this port's compressor already produces is unchanged.

---

## E6. What the spec's appendix claimed

| Claim | Found |
| ----- | ----- |
| `:848-875` is the pool loop; read it before the write loop | Confirmed, §E1.3. The exactness of the permutation is indeed a property of the two loops sharing one filter |
| `:982-1021` is the whole of this feature; everything else in the function is row 5 | Confirmed. `:609-980` is setup, the hand lists and the weapon arm; `:1022-1315` is the stat rewrite |
| `StartFunctions.cs:1541` is the single gate, and `keepGuns` only extends the skip list with two weapon ids | Confirmed, §E1.1 |
| `StartingWeapons.cpp` already has `kNeverTouchEquipIds`, `kShopEquipId`, `kShopEquipType` | Confirmed, §E2.1. They are file-local, which is why the carve-out is needed |
| `EnemyRandomizer.cpp:1030-1062` is where a pass is added and the order decided | Confirmed; the block is inside `StepItemData` step 1 |
| `starting_weapons_verify.py`'s `stock()` is already type-filtered and the surrounding loop is not | Confirmed exactly, §E2.5 — and this is what makes the fix small |
| `param_offsets.py load_param` wants `"ShopLineupParam.param"`, not the param type | Not exercised: the probe here uses `bnd4_members` + `param_rows` directly, as the shipped verifiers do. The `fields` subcommand does take the type string, `SHOP_LINEUP_PARAM` (§E4 M2) |
| Grouping by `id // 10000` produces F10's family table | Confirmed, §E4 M10 |
| The stock-limit and price fields are worth reading even though nothing writes them | Confirmed and measured (§E4 M3, M4). The existing byte-range assertion covers them, so no new invariant is needed |
| No shop-specific protected-item list exists in the reference | Confirmed. `:635-643` is the whole of it |
| No bug to decide about in these two branches | Confirmed, §E1.4 |
| An owner-ranked writer is unnecessary | Confirmed by measurement: the three buckets partition the table (§E4 M1) |
| Families C and D could not be identified; event data or shop-opening scripts are the next thing to try | Not attempted. D4 sends it to hardware, so opening `event/` would be work the spec deliberately deferred |

---

## E7. Anything that could not be established

* **Whether shop families C and D are reachable.** Spec F10's assumption
  stands unchanged; nothing offline settles it and D4 says the console does.
  Recorded as a hardware observation, not a design input.
* **Whether the game accepts a shuffled listing at runtime.** The write is one
  s32 over another in a param row row 5 already writes on hardware, so the
  mechanism is proven; that a *consumable* or *armour* row survives the game's
  own shop menu code is not, and cannot be, shown offline.
* **What a seed actually produces.** The mirror deliberately does not model
  `std::mt19937` or the draw order, so it validates properties of an output tree
  and never predicts one. Same-seed reproducibility is a hardware check.
* **Whether either label reads the way it is meant to.** Per the standing note
  on setting naming, nothing about a label or a polarity is confirmed until it
  has been seen on hardware — which is the risk §8 Q2's armour-only run exists
  to cover.
* **The exact number of `mt19937` draws a bucket consumes.** Whether
  `std::uniform_int_distribution<int>(0, 0)` consumes a value from the engine is
  an implementation detail of this toolchain's libc++. Nothing in the plan
  depends on the count — only on the order — so it was not measured.
