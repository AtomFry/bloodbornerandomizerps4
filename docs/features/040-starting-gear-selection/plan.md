# Plan 040 — Starting Gear Selection

**Status: APPROVED** — 2026-09-29. §8 is empty; its two non-blocking questions are recorded as D1 and D2 in §9.

**Spec:** `docs/features/040-starting-gear-selection/spec.md` — **APPROVED**
(2026-09-29). Four binding decisions, §10 D1–D4.

**Evidence:** `docs/features/040-starting-gear-selection/plan-evidence.md` — the
measurements, traces and rejected alternatives behind this plan.

**Plan review:** `docs/features/040-starting-gear-selection/plan-review.md` —
added during review.

---

## Execution Strategy

*Approving this plan approves this strategy. `CLAUDE.md` §4: milestones describe
implementation structure; gates describe when a human stops to test.*

| | |
| --- | --- |
| **Structure** | 4 functional milestones |
| **Execution** | continuous |
| **Human test gates** | **final only** |
| **Intermediate verification** | after every milestone: `cd app && make` (**`make clean && make` in M2, M3 and M4**), plus the §6 automated checks that apply to that milestone |

**No gates.** There is no route to probe — the 22 rows this feature writes
already name the four garments a new character is visibly wearing — so no
milestone's content depends on a console observation. Nothing here can destroy
data, and each milestone's own verification localises a failure to it
(`plan-evidence.md` §E5.5).

---

> **This document is the implementation contract.** §1–§7 are what the
> implementer reads, in order. §8–§10 are the record, not instructions.
> `plan-evidence.md` holds the investigation; `log.md` holds the history.

---

## 1. Objective

Add five settings to `WEAPONS AND GEAR`: four pool pickers — one per armour slot,
offering between them all 124 wearable pieces the game names — and one toggle that
grants 20 Blood Vials and 20 Quicksilver Bullets. Each picker draws independently
for the run and writes its slot's `CharaInitParam` clothing field; the toggle
writes two free inventory slots of the same rows. Tick nothing and leave the
toggle off and a run is byte-for-byte the run it is today. Nothing about the
Hunter's Dream, the shops, or an existing save changes.

---

## 2. Approved behaviour

| #   | Behaviour | Source |
| --- | --------- | ------ |
| B1  | **Four** settings, one per slot, each a pool picker with none/one/many semantics and **nothing ticked** by default: `START WITH HEADWEAR` (33 rows), `START WITH BODY ARMOUR` (34), `START WITH GLOVES` (26), `START WITH LEGWEAR` (31) | spec §10 D1, §2, §7 |
| B2  | The four row counts **sum to exactly 124**, the wearable set: every piece the game names and marks wearable by both sexes, and nothing else | spec §2, §4.2, §10 D4 |
| B3  | Every row's displayed name is the name the game's own text gives that exact armour id. No label is invented, composed, or hand-authored | spec §4.3, §7 |
| B4  | The 126 unnamed rows in those slots, the two rows the game marks wearable by neither sex, and the 21 `NOT NEEDED` placeholders are not in any picker and can never be granted | spec §2, §4.2 |
| B5  | A picker with nothing ticked leaves that slot's vanilla garment alone. A picker with anything ticked replaces it | spec §2 |
| B6  | Each slot is drawn **independently**: changing what is ticked in one slot must not change which piece a fixed seed draws in another | spec §7, spec §8 |
| B7  | One new toggle, `START WITH SUPPLIES`, off by default, granting **20 Blood Vials and 20 Quicksilver Bullets** — no count setting, no consumables picker | spec §10 D2 |
| B8  | The supplies go into **free** inventory slots. The vanilla Hunter's Mark survives, and so does every write `START WITH HUNTER TOOLS` makes, in either pass order | spec §7, §4.4 |
| B9  | No armour requirement is rewritten and no equip-weight allowance is changed: the armour table has no requirement fields. `WeaponRequirements.h` and `ReqOwner` are not involved at all | spec §4.2 |
| B10 | Takes effect at character creation only. An existing save gains nothing and is undamaged | spec §2 |
| B11 | Independent of `START WITH HUNTER TOOLS`, `START WITH A TRICK WEAPON`, `START WITH A LEFT WEAPON`, the rune probe, and each other. **All six writers of these 22 rows may be on at once with no write lost**, whichever order the passes run in | spec §7 |
| B12 | The Hunter's Dream coffins are untouched. `ShopLineupParam` is not written by this feature at all | spec §10 D3, §6 |
| B13 | State persists in the existing config. An absent or wrong-length selection line reads as **nothing ticked**; an absent supplies key reads as **off** | spec §7 |
| B14 | Ticking one piece in one slot and nothing else is a complete request: the run does something and is not reported as "nothing is on" | spec §7 |
| B15 | The four pickers do not appear in the readiness summary's toggle count. `START WITH SUPPLIES`, being a toggle, does | spec §4.7; plan 037 D2 |

---

## 3. Constraints and invariants

### 3.1 Must be preserved

* **Row 37's table is byte-identical.** `TrickWeaponTable.h` must not change,
  including after the lot-category fix of §4.6. A changed row count changes the
  length of a saved config line.
* **Rows 34, 37 and 38 keep their writes, and so does the rune probe.** All four
  write these same 22 rows. Every inventory write goes through
  `FirstEmptySlot` / `RowHasItem`, never a fixed slot index; every clothing write
  touches only its own 4-byte field.
* **Each of the four table orders is load-bearing, independently.** A selection
  is stored positionally, one character per row. A regenerated table in a
  different order silently remaps a saved selection onto a different piece.
  Four frozen orders, four counts; a changed count in any one must be rejected by
  the length guard, not misapplied.
* **The four draws are independent of one another.** See §4.4. Breaking this
  breaks B6 and makes two runs on one seed incomparable.
* **Nothing ticked anywhere and the toggle off consumes no randomness and writes
  no byte.** Turning the pickers on with nothing ticked must leave a run
  identical roll for roll.
* **Only the 22 origin rows are written**, and within them only
  `equip_Helm/Armer/Gaunt/Leg` and free `item_NN` / `itemNum_NN` pairs. No other
  `CharaInitParam` row and no other archive member.
* **No id crosses a namespace.** An armour id goes only in a clothing field; a
  goods id only in an inventory slot. `plan-evidence.md` §E5.2.
* **The archive is edited in place.** Every write is one value over another
  inside an existing row. Nothing is resized, the archive is never rebuilt.
* **Config compatibility both ways**: unknown keys ignored on load, absent keys
  read as the fail-safe.
* **Layering**: the tables, the selections and the two passes live under
  `app/src/Randomizer/`; no SDL2 there, and no UI file learns a param offset or
  an armour id.

### 3.2 Out of scope

* **The Hunter's Dream coffin picks** (spec D3) — its own backlog row.
* **Matched-outfit grouping in the UI.** There is no outfit concept anywhere in
  this feature: four piece pickers, nothing else.
* **Runes, accessories and blood gems.** The rune probe stays exactly as it is,
  enabled, riding on `START WITH HUNTER TOOLS`. Nothing here reads, writes or
  assumes an accessory slot, and nothing depends on row 39.
* **Any consumable other than Blood Vials and Quicksilver Bullets**, any count
  other than 20, and any use of the six `secondaryItem_NN` slots.
* **Starting stats, level, origin, Insight or echoes.**
* **`equip_*_GenId`.** All eight are `-1` on all 1700 rows; they stay `-1`, the
  same ground on which `TrickWeaponGrant.cpp` leaves them alone.
* **Narrowing the 22 origin rows to the live block** — row 34's open item O2.
* Chalice dungeons.

### 3.3 Hazards

| Hazard | What to do | Evidence |
| ------ | ---------- | -------- |
| `gen_weapon_table.py`'s `LOT_CATEGORY_WEAPON = 1` is wrong: category 1 is **armour**, 0 is **weapons** | Do not copy it. §4.6 fixes it at source | §E4 M5 |
| The two membership tests **disagree on armour in both directions**, so `gen_weapon_table.py`'s "agree or die" guard would refuse to write | Use §4.2's two-sided assertion, the shape `gen_left_hand_table.py` uses | §E4 M4 |
| Three names contain `/`, which the 8x8 fallback font draws as a **blank column**, not an error | Add the `/` glyph in M1. Do not sanitise a game string | §E4 M6 |
| `HUNTER GARB` and `STUDENT UNIFORM` each name **two** body rows, firing the generator's duplicate-name guard | §4.2's order makes each pair adjacent; assert the duplicate set exactly. §9 D1 Q1 | §E4 M3 |
| A sequential shared RNG stream makes the gloves draw depend on whether headwear is ticked — that is B6 broken | §4.4's four raw words | §E5.1 |
| The count at `204 + slot` is a **single byte**; a 4-byte write corrupts three neighbours | `plain[row + kItemNumBase + slot] = 20;`, as `HunterTools.cpp` does | `CharaInitRows.h` |
| `FirstEmptySlot` tests for `-1`, and the first rune probe found "empty" meaning `0` — in `secondaryItem_NN` | All 22 rows hold `item = [100, -1×9]`; the search is correct here. Do not extend it to `secondaryItem_NN` | §E4 M8 |
| `hunter_tools_verify.py`, `trick_weapons_verify.py` and `left_hand_weapons_verify.py` each assert "only these fields differ", so a combined run reads as a regression | Add §6's tolerance flags in the milestone that creates the write — M3 supplies, M4 armour | §E5.3 |
| `pool_verify.py`'s config-size cases are exact equalities | Re-measure per §4.3. **Row 13's M5 may have removed a key by then** | §E4 M9 |
| `settings_ui_verify.py` case 3 compares the model against the settings-UI spec §7.1 | Update that table and `PROSE_TO_LABEL` in the same pass as the model entries | §E5.4 |
| `RandomizerDefaults` grows 124 bytes; both screens and `WorldStore` hold copies | `make clean && make` in M2–M4 | `docs/build.md` |
| This console is cp1252; printing a name or a Japanese member name raises `UnicodeEncodeError` that reads like a parse failure | The generator writes files and never prints a name; the verifier prints ASCII only | `docs/known-traps.md` |

---

## 4. Implementation approach

> Chosen: one generated header carrying four frozen tables whose counts are
> asserted to sum to 124; four `ModelPoolSelection` typedefs and four picker
> kinds on the shipped pattern; and **two** engine passes — a deterministic
> supplies grant beside `HunterTools`, and an armour grant that runs last and
> draws four independent words. Alternatives considered and why they were
> rejected: `plan-evidence.md` §E3.

### 4.1 The slot order, used everywhere

**head → body → gloves → legwear** — the game's own `protectorCategory` order
(0/1/2/3) and its own field order (`equip_Helm` 32, `equip_Armer` 36,
`equip_Gaunt` 40, `equip_Leg` 44). Use it for the table declarations, the config
keys, the UI rows and the four draws alike.

### 4.2 The four tables, and how they are derived

The set is **derived, never listed**. A row belongs to slot *s*'s picker when all
three hold against `data/vanilla/dvdroot_ps4`:

1. `EquipParamProtector.param` row with `protectorCategory` (offset 210, u8)
   equal to *s*, for *s* in 0..3. Category 4 is the 21 `NOT NEEDED` placeholders,
   excluded by this test and **not** by a name filter.
2. The armour-name FMG holds a string for that exact id which is non-empty, not
   `*`, does not match `^en\d+$`, and is not `NOT NEEDED`.
3. `equipModelGender` (offset 209, u8) is **3** — wearable by both sexes.

That yields 33 / 34 / 26 / 31 = **124**. Assert the four counts and the total.

**Obtainability is a two-sided cross-check, not a membership gate.** Take the
armour ids sold in a `ShopLineupParam` row with `equipType == 1` or present in an
`ItemLotParam` slot whose matching `lotItemCategory` is **0**, and assert exactly:

* `included − obtainable == {230000, 231000, 232000, 233000}` — the four starting
  garments, which need no shop and no pickup;
* `obtainable − included == {250000, 430000}` — Crown of Illusions and Enlarged
  Head, wearable by neither sex.

**Names** come from the `防具名.fmg` member of
`data/vanilla/dvdroot_ps4/msg/engus/item.msgbnd.dcx`, uppercased, read at each
exact id. **Order**, per slot: uppercased name, then ascending id. Frozen; the id
tie-break makes each duplicate-name pair adjacent.

`app/tools/gen_armour_table.py` owns the derivation and writes **one** header,
`app/src/Randomizer/ArmourTables.h`, with the same `--check` contract and
frozen-order warning `gen_weapon_table.py` carries: four
`std::array<ModelPoolEntry, N>` accessors, four counts (`kHeadwearCount` 33,
`kBodyArmourCount` 34, `kGlovesCount` 26, `kLegwearCount` 31), and a
`static_assert` that the four sum to 124. Each row is
`{ "<decimal armour id>", "<NAME>", 1 }`; the id goes in `model` because the
picker keys and logs on that field, and the engine converts it with `strtol`.
Emit no second array of ids.

### 4.3 The settings chain

Four selections and one boolean. The pickers have **no** accompanying boolean —
their whole state is the pool, exactly as rows 37 and 38.

| Site | What to add |
| --- | --- |
| `EnemyPoolSelection.h` | four typedefs beside the others: `ModelPoolSelection<kHeadwearCount, false> HeadwearSelection`, and the same for `BodyArmourSelection`, `GlovesSelection`, `LegwearSelection`. `false` is the fail-safe: fresh struct, absent key and wrong-length value all read as nothing ticked. Include `ArmourTables.h` |
| `RandomizerDefaults.h` | the four selections, then `bool startWithSupplies = false;` |
| `RandomizerDefaultsStore.cpp` | five keys in `FormatSettings` **and** `ApplySettingKey`, in §4.1's order: `armour_head_included`, `armour_body_included`, `armour_gloves_included`, `armour_legwear_included`, `start_with_supplies`. These exact names — the config arithmetic depends on their lengths |
| `SettingsModel.h` | five `SettingId`s; four kinds `HeadwearPool`, `BodyArmourPool`, `GlovesPool`, `LegwearPool` |
| `SettingsModel.cpp` | five `kSettings` entries, category `WeaponsGear`, declared **after** `StartWithLeftHandWeapon` in §4.1's order with the toggle last; `SettingValueText` → `N OF 33` / `N OF 34` / `N OF 26` / `N OF 31`; the four `Selection*` functions gain four cases each |
| labels | `START WITH HEADWEAR`, `START WITH BODY ARMOUR`, `START WITH GLOVES`, `START WITH LEGWEAR`, `START WITH SUPPLIES`. All five measured against the 700-px pane budget; the widest row is `START WITH BODY ARMOUR` at **609 px** (§E4 M7) |

**Config arithmetic — re-derive it from the file; do not copy these numbers.**
The five keys cost **239 bytes**: 52 + 56 + 43 + 49 for the four picker lines
(key, `=`, the selection line, newline) and 22 for `start_with_supplies=1\n`.
Against today's file that is a settings block of **1277** and a worst-case
`defaults.cfg` of **1328**, from 1038 / 1089. **Row 13's milestone 5 is still
pending** and removes `REPLACE ALL ENEMIES`, 22 bytes, giving **1255 / 1306**
instead. Pin whichever baseline is in the file plus 239, update the narrating
comment to match, and halt if the sum does not come out (§7).

Both screens need **no change**: they reach a picker entirely through `IsDrillIn`
and the four `Selection*` functions. `ToggleCount()` counts `SettingKind::Toggle`
entries, so `START WITH SUPPLIES` joins the readiness summary automatically and
the four pickers do not — that is B15, and no number is pinned to it.

Four `PickerStrings` beside the shipped five: heading = the setting's label,
instruction `TICK ANY NUMBER - ONE IS DRAWN FOR THE RUN`, `YES`/`NO`,
`ENABLE ALL`/`DISABLE ALL`, `ALL`/`NONE`, **`showRowId = false`**. The instruction
line puts each list on the 11-row layout: 33, 26 and 31 rows are 3 pages, 34 is 4.

### 4.4 The armour grant, and why the draws are independent

`app/src/Randomizer/ArmourGrant.{h,cpp}` — its own file beside
`TrickWeaponGrant`. `CharaInitRows.h` gains the four clothing offsets as an array
in §4.1's order (`kEquipHelm` 32, `kEquipArmer` 36, `kEquipGaunt` 40,
`kEquipLeg` 44): they are the shape of the row, which is what that header holds,
and not a feature's own constant, which it does not.

Given the archive, `CharaInitParam.param`, the four selections and the RNG:

1. Build four candidate lists from the four tables' ticked rows.
2. **If all four are empty, return at once** — nothing written, RNG untouched.
3. Otherwise draw **exactly four raw words**, `rng()` once per slot in §4.1's
   order, whether or not that slot has anything ticked. Slot *i*'s piece is
   `candidates[i][word_i % candidates[i].size()]`; a slot with an empty list
   discards its word and is left alone.
4. For each of the 22 origin rows: bounds-check the whole 300-byte row, then
   write one s32 per granted slot at its own offset. Leave every other byte
   alone, including the three untouched clothing fields, the ten item slots and
   the four `*_GenId`s.
5. Log the four chosen ids and names, and the rows written.

**Step 3 is what makes B6 true, and neither half of it is optional.** The draw
count must be a constant four whenever the feature does anything, and each index
must be `word % k` and not a `uniform_int_distribution` — a distribution consumes
a variable number of raw words as *k* changes, which reintroduces the coupling the
constant count removes (§E5.1).

This pass runs **last** in `StepItemData`, after the left-hand weapon grant, so
its four draws cannot move any earlier roll. It locates `CharaInitParam.param`
itself, per feature, as the existing passes do.

### 4.5 The supplies grant

`app/src/Randomizer/StartingSupplies.{h,cpp}` — the same shape as
`HunterTools.cpp`. Two goods ids, **1000** (Blood Vial) and **900** (Quicksilver
Bullets), at count **20** each, which is each row's own `maxNum` in
`EquipParamGoods` and is asserted as such (A9) so the constant is pinned to the
game's data and not to a choice.

Per origin row, per item: skip if `RowHasItem`; otherwise take `FirstEmptySlot`,
write the s32 id at `kItemIdBase + 4·slot` and the **single byte** 20 at
`kItemNumBase + slot`. A full row increments a counter and is not fatal.

It draws nothing, so it commutes with every pass. Call it immediately after the
`START WITH HUNTER TOOLS` block. Both features on fill four of the ten slots.

### 4.6 The lot-category fix

`gen_weapon_table.py`'s `LOT_CATEGORY_WEAPON = 1` becomes `0`, with a comment
giving the measured split (category 0: 63 weapon ids against 3 protector;
category 1: 101 protector against 3 weapon). It moves no row of row 37's table,
and `gen_weapon_table.py --check` passing is the assertion that says so — verified
inert before the decision was taken, at both constant values. See §9 D2.

### What this reuses

§5 lists every file. What it does not say is how five of them are treated:

| Existing code | Treatment |
| --- | --- |
| `WeaponRequirements.h`, `ReqOwner`, `StartingWeapons.*`, `ShopLineup.h`, `ShopStock.*` | **not used**, not extended, not derived from. Armour has no requirements and this feature writes no shop row |
| `RuneProbe.{h,cpp}` | left exactly as it is, **enabled**. Nothing here reads or writes an accessory slot |
| `HunterTools.cpp` | the pattern for §4.5, copied in shape only. Not extended and not called |
| `UI/SetupDefaultsScreen.*`, `UI/WorldEditorScreen.h`, `ModelPicker.cpp` | **unchanged** — they host any picker generically, and `showRowId` already exists |
| `tools/param_offsets.py`, `gen_pool_table.py` | reused as-is: every offset from the first, the `--check` contract from the second. Only `RENDERABLE` changes |

---

## 5. Files and changes

| File | Change | M |
| ---- | ------ | - |
| `app/tools/fmg.py` | `armour_names()` and `goods_names()`, member names spelled as escapes | 1 |
| `app/tools/gen_pool_table.py` | `RENDERABLE` gains `/` | 1 |
| `app/src/Platform/Font8x8.cpp` | one `kGlyphs` entry for `/` | 1 |
| `app/tools/gen_weapon_table.py` | `LOT_CATEGORY_WEAPON` 1 → 0, with the measured comment | 1 |
| `app/tools/gen_armour_table.py` | **new** — §4.2's derivation, frozen orders, `--check` | 1 |
| `app/src/Randomizer/ArmourTables.h` | **new, generated** — four tables, four counts, the `static_assert` | 1 |
| `app/tools/starting_gear_verify.py` | **new** — `list`, `selftest`, `verify` | 1, 3, 4 |
| `app/tools/font_atlas_verify.py` | whatever case enumerates the fallback glyph set, if one does | 1 |
| `app/src/Randomizer/EnemyPoolSelection.h` | four typedefs; include `ArmourTables.h` | 2 |
| `app/src/Randomizer/RandomizerDefaults.h` | four selections and `startWithSupplies` | 2 |
| `app/src/Randomizer/RandomizerDefaultsStore.cpp` | five keys in `FormatSettings` and `ApplySettingKey` | 2 |
| `app/src/UI/SettingsModel.h` | five `SettingId`s, four `SettingKind`s | 2 |
| `app/src/UI/SettingsModel.cpp` | five `kSettings` entries and the five `switch` sites | 2 |
| `app/src/UI/ModelPicker.h` | four `PickerStrings` | 2 |
| `docs/features/randomizer-settings-ui/spec.md` | §7.1 table gains the five rows in order | 2 |
| `app/tools/pool_verify.py` | the re-measured config arithmetic and its comment; the new-key case; the new progress lines in the 71-character case | 2, 3, 4 |
| `app/tools/settings_ui_verify.py` | four entries in `KIND_TO_COUNT` and `POOL_COUNTS`; "seven selection fields" → eleven; five `PROSE_TO_LABEL` rows; the config round-trip's value shape | 2 |
| `app/tools/ui_scroll_verify.py` | four `SCREENS` entries, 33 / 34 / 26 / 31 rows on the instruction layout | 2 |
| `app/src/Randomizer/StartingSupplies.{h,cpp}` | **new** — §4.5 | 3 |
| `app/src/Randomizer/CharaInitRows.h` | four clothing offsets; block-comment correction | 3 |
| `app/src/Randomizer/ArmourGrant.{h,cpp}` | **new** — §4.4 | 4 |
| `app/src/Randomizer/EnemyRandomizer.h` | five options, `AnyParamFeature()`, the result fields | 3, 4 |
| `app/src/Randomizer/EnemyRandomizer.cpp` | the supplies call after the hunter-tools block (M3); the armour call **last** (M4) | 3, 4 |
| `app/src/Game/WorldActivation.cpp` | options mapping in the frozen field order; the `anythingOn` terms | 3, 4 |
| `app/src/UI/WorldEditorScreen.cpp` | progress lines; the item-data line's condition | 3, 4 |
| `app/tools/worlds_verify.py` | `WIZARD_OPTIONS_MAPPING` and `WIZARD_RUN_DECISION` gain the five fields | 3, 4 |
| `app/tools/hunter_tools_verify.py` | `--supplies` (M3) and `--armour h,b,g,l` (M4) tolerances | 3, 4 |
| `app/tools/trick_weapons_verify.py`, `app/tools/left_hand_weapons_verify.py` | `--armour h,b,g,l` tolerance | 4 |

No Makefile change: it compiles `.cpp` recursively and tracks header
dependencies. An existing `defaults.cfg` needs no migration — the five keys are
absent and read as the fail-safe. No saved world is invalidated.

---

## 6. Verification

### Build

`cd app && make`. **`make clean && make` in M2, M3 and M4** — M2 grows
`RandomizerDefaults` by 124 bytes and both screens plus `WorldStore` hold copies.
The `.pkg` must be produced.

### Automated

| Check | Command | Asserts |
| ----- | ------- | ------- |
| Armour tables current | `python app/tools/gen_armour_table.py data/vanilla/dvdroot_ps4 --check` | the committed header is what the vanilla tree produces now |
| Row 37 table unmoved | `python app/tools/gen_weapon_table.py data/vanilla/dvdroot_ps4 --check` | the lot-category fix did not move one row |
| Table listing | `python app/tools/starting_gear_verify.py list data/vanilla/dvdroot_ps4` | prints the four tables plus every excluded row and why; not a pass/fail gate |
| The mirror | `python app/tools/starting_gear_verify.py selftest data/vanilla/dvdroot_ps4` | A1–A16 below |
| Output tree | `python app/tools/starting_gear_verify.py verify <vanilla> <output>` | G1–G7 below |
| Row 34 unaffected | `python app/tools/hunter_tools_verify.py selftest data/vanilla/dvdroot_ps4` | every existing case still passes, plus the new tolerances |
| Rows 37/38 unaffected | `python app/tools/trick_weapons_verify.py selftest …`, `left_hand_weapons_verify.py selftest …` | every existing case still passes |
| Row 5 unaffected | `python app/tools/starting_weapons_verify.py selftest data/vanilla/dvdroot_ps4` | unchanged |
| Config and picker strings | `python app/tools/pool_verify.py selftest data/vanilla/dvdroot_ps4` | the re-measured arithmetic, the five keys in load **and** save, the new progress lines inside 71 characters |
| Settings model | `python app/tools/settings_ui_verify.py` | eleven selection fields with one entry each, five rows fitting the pane, non-empty help, and the model matching the settings-UI spec §7.1 |
| Picker geometry | `python app/tools/ui_scroll_verify.py` | four lists over 3 / 4 / 3 / 3 pages on the instruction layout, clear of the count line and the footer |
| Font | `python app/tools/font_atlas_verify.py` | the atlas and the fallback both carry `/` |
| Worlds parity | `python app/tools/worlds_verify.py selftest` | the options mapping and run decision carry the five fields; a recipe round-trips all four selection lines at their exact lengths |

`starting_gear_verify.py selftest` must assert, against the vanilla tree:

| # | Assertion | M |
| - | --------- | - |
| A1 | The armour-name FMG frames validly by both of `fmg.py`'s checks | 1 |
| A2 | `EquipParamProtector` holds 273 rows; `protectorCategory` splits them 76 / 76 / 48 / 52 / 21, and all 21 category-4 rows are named `NOT NEEDED` | 1 |
| A3 | §4.2's three tests give **33 / 34 / 26 / 31**, summing to 124, and the header's `static_assert` spells the same total | 1 |
| A4 | The obtainability cross-check holds in **both** directions exactly as §4.2 states them. Report the excluded rows; do not assert a list of them | 1 |
| A5 | Every row's name is the uppercased FMG string **for that exact id**; every name is renderable by `Font8x8.cpp`; the widest name plus the flag word is inside 71 characters | 1 |
| A6 | The committed tables are in §4.2's frozen order, and the only duplicated display names are `HUNTER GARB` and `STUDENT UNIFORM`, twice each, both in the body slot and adjacent | 1 |
| A7 | The armour row layout has **zero** `proper*` requirement fields, against four on the weapon row — B9, asserted rather than assumed | 1 |
| A8 | `id/1000 % 10` equals `protectorCategory` on all 252 wearable-slot rows, so no piece can be written into another slot's field by an id mistake | 1 |
| A9 | Goods 1000 and 900 exist, are named `Blood Vial` and `Quicksilver Bullets`, and each carries `maxNum == 20` — the count the C++ writes | 3 |
| A10 | All 22 origin rows hold `item = [100, -1×9]` and `itemNum = [1, 0×9]`, so nine slots are free and `FirstEmptySlot` finds them | 3 |
| A11 | Supplies on: every origin row holds 1000 and 900 exactly once each at count 20, in slots that were empty; slot 0 is still the Hunter's Mark; no other byte of the row changed; the pass is idempotent over an already-edited buffer | 3 |
| A12 | Supplies **and** hunter tools, in **both** pass orders: all four entries present exactly once in every origin row, no row out of slots, identical bytes either way | 3 |
| A13 | Nothing ticked and the toggle off: the simulated passes leave the archive byte-identical and consume no RNG | 3, 4 |
| A14 | One piece ticked in one slot: all 22 origin rows name that piece in that slot, **the other three clothing fields keep their vanilla values**, and no other byte of the 300-byte row changed; no non-origin row and no other member differs | 4 |
| A15 | **B6**: with a fixed seed, the piece drawn for each slot is the same whatever the other three slots have ticked — asserted across several tick patterns per slot, not one | 4 |
| A16 | Each of the four selection lines round-trips at its exact length; a line one character short and one long each leave nothing ticked | 2 |

`starting_gear_verify.py verify <vanilla> <output>` must assert, for a real tree:
**G1** only `CharaInitParam.param` differs (plus members named by a tolerance
flag); **G2** within it only the 22 origin rows differ; **G3** within an origin
row only the granted clothing fields and the supplies' two slot pairs differ;
**G4** all 22 rows name the same piece per granted slot, and each is one of that
slot's ticked rows; **G5** a slot whose picker was empty holds its vanilla value;
**G6** no clothing field holds an id belonging to another slot; **G7** the
archive's member list is unchanged.

**A mirror pins the rules, not the C++ implementation of them.** It cannot show
that the engine consults the tables correctly, and it deliberately does not
predict *which* piece a seed draws (§E7). Determinism is a hardware check.

### Hardware

The implementer cannot run any of this. The handoff is **spec §8's hardware
list, verbatim, in its order**, including its free pre-build observation: start a
vanilla new character and look at its head, which answers row 34's open item O2
at no cost.

Do three of spec §8's checks first, because they are the ones that fail silently:
a piece arriving in the **wrong slot**; the supplies **displacing** the Hunter's
Mark or a hunter tool rather than taking a free slot; and the three `/` names
drawing with a hole in them.

Capture `live.log` and the output tree of one fully-granted run and one
nothing-ticked run into `data/runs/`, so `starting_gear_verify.py verify` can run
against both.

---

## 7. Milestones and stop conditions

### Milestone 1 — the 124 pieces, derived and pinned

**Goal.** The four sets of grantable pieces and their in-game names exist as one
generated, verified header. No behaviour changes.

| # | Change | Done when |
| - | ------ | --------- |
| 1 | `fmg.py` gains `armour_names()` and `goods_names()` | both read their members and return the strings for ids 230000 and 1000 |
| 2 | `/` added to `Font8x8.cpp`'s `kGlyphs` and to `gen_pool_table.py`'s `RENDERABLE` | `font_atlas_verify.py` passes and no shipped string's rendering changed |
| 3 | `gen_weapon_table.py`'s `LOT_CATEGORY_WEAPON` → 0 | `gen_weapon_table.py --check` passes, i.e. `TrickWeaponTable.h` is untouched |
| 4 | `gen_armour_table.py` with §4.2's three tests, the two-sided cross-check and the frozen orders | it derives 33 / 34 / 26 / 31, asserts the cross-check both ways, and refuses to write if any name is unrenderable |
| 5 | Generate `ArmourTables.h` | `--check` passes, the `static_assert` spells 124, and the header carries the order warning |
| 6 | `starting_gear_verify.py` with `list` and `selftest` | A1–A8 pass, and `list` prints the four tables plus each excluded row with its reason |

**Invariants**: four frozen orders; names read and never composed; no hardcoded
id list anywhere; row 37's table byte-identical (§3.1).
**Verification:** build; both `--check`s; `starting_gear_verify.py selftest`
(A1–A8); `trick_weapons_verify.py selftest`; `font_atlas_verify.py`.
**Then continue to M2.**

### Milestone 2 — the five settings a player can set

**Goal.** A player can tick pieces in four pickers and switch the supplies
toggle on both screens, the choices persist, and the run ignores all five. Zero
change to any run's output.

| # | Change | Done when |
| - | ------ | --------- |
| 1 | The four typedefs and the five `RandomizerDefaults` fields | a fresh struct reads nothing ticked and the toggle off |
| 2 | The five keys in both halves of `RandomizerDefaultsStore.cpp` | a config without them loads as the fail-safe, a saved file round-trips 33 / 34 / 26 / 31 characters, and a wrong-length line leaves that slot untouched |
| 3 | The ids, the four kinds, the five `kSettings` entries after `START WITH A LEFT WEAPON`, and the five `switch` sites | each picker row reads `0 OF N` with the right N, `IsDrillIn` is true for the four and false for the toggle |
| 4 | The four `PickerStrings`, `showRowId = false` | each picker draws names only, on the 11-row layout |
| 5 | The settings-UI spec §7.1 and `PROSE_TO_LABEL` | `settings_ui_verify.py` case 3 passes |
| 6 | The **re-measured** `pool_verify.py` arithmetic and its comment; `settings_ui_verify.py`'s four counts and eleven-selection case; four `ui_scroll_verify.py` entries | all three pass with no unrelated case changing state |

**Invariants**: the fail-safe polarity on all five; config compatibility both
ways; the shipped pickers render unchanged; no screen file learns a param offset
(§3.1). **Verification:** `make clean && make`; `pool_verify.py selftest`
(including A16); `settings_ui_verify.py`; `ui_scroll_verify.py`;
`worlds_verify.py selftest`; `starting_gear_verify.py selftest` still passing.
**Then continue to M3.**

### Milestone 3 — what the character carries

**Goal.** A run with `START WITH SUPPLIES` on gives every new character 20 Blood
Vials and 20 Quicksilver Bullets, in free slots, alongside whatever
`START WITH HUNTER TOOLS` granted, and the archive is provably otherwise
untouched.

| # | Change | Done when |
| - | ------ | --------- |
| 1 | `CharaInitRows.h` gains the four clothing offsets and its block comment is corrected | no existing constant's value changed, and `hunter_tools_verify.py selftest` passes against the edited header |
| 2 | `StartingSupplies.{h,cpp}` per §4.5 | the pass draws nothing, writes one byte for each count, and is idempotent over an already-edited buffer |
| 3 | The option, the result counters, and the call immediately after the hunter-tools block in `StepItemData` | a supplies-only run rewrites the item-data archive and nothing else |
| 4 | The `WorldActivation.cpp` mapping and `anythingOn` term, the `WorldEditorScreen.cpp` progress line, and the item-data line's condition | a supplies-only world does not report "no randomizer settings are on" |
| 5 | `worlds_verify.py`'s two frozen lists; `hunter_tools_verify.py --supplies`; `pool_verify.py`'s 71-character progress-line case | `hunter_tools_verify` passes with and without the flag, and rejects the same bytes without it |
| 6 | Extend `starting_gear_verify.py` with A9–A13 and the supplies half of `verify` | A12 passes in both pass orders |

**Invariants**: the Hunter's Mark survives; row 34 keeps both writes in either
order; the pass draws nothing; only the 22 origin rows; in-place edits only
(§3.1). **Verification:** `make clean && make`; every §6 check that exists yet.
**Then continue to M4.**

### Milestone 4 — what the character wears

**Goal.** A run with pieces ticked in any combination of the four pickers dresses
every new character accordingly, each slot drawn independently of the others.

| # | Change | Done when |
| - | ------ | --------- |
| 1 | `ArmourGrant.{h,cpp}` per §4.4 — four candidate lists, the all-empty early return, exactly four raw draws, one s32 per granted slot over the 22 rows, full-row bounds check | an empty set of selections returns having written nothing and drawn nothing, and the draw count is four in every other case |
| 2 | `ArmourPieceName(slot, id)` exported from the engine for the progress line | no UI file learns how an armour id is spelled |
| 3 | The four options, `AnyParamFeature()`, the result fields, and the call **last** in `StepItemData`, after the left-hand grant | this pass is the last RNG consumer in the run |
| 4 | The `WorldActivation.cpp` mappings and `anythingOn` terms; one `WEARING <NAME>` progress line per granted slot; the item-data line's condition | a headwear-only world builds a tree and names the piece it drew |
| 5 | `worlds_verify.py`; `--armour h,b,g,l` on `hunter_tools_verify.py`, `trick_weapons_verify.py` and `left_hand_weapons_verify.py` | all three pass with and without the flag, and each rejects the same bytes without it and a wrong id with it |
| 6 | Extend `starting_gear_verify.py` with A14, A15 and the armour half of `verify` | A15 passes across several tick patterns per slot |

**Invariants**: four independent draws; nothing ticked writes nothing and draws
nothing; an empty picker leaves its vanilla garment; rows 34/37/38 and the rune
probe keep their writes; only the four clothing fields of only the 22 origin rows
(§3.1). **Verification:** `make clean && make`; every check in §6; then hand off
spec §8's hardware list.

### Stop conditions

Halt and report rather than deciding, if any of these occur:

* the build fails in a way the plan did not anticipate, or only after a clean
  rebuild;
* `gen_armour_table.py` derives anything other than 33 / 34 / 26 / 31, or its
  cross-check fails in either direction — the derivation is then wrong, and no
  adjustment to a threshold or a list fixes it;
* a third duplicate display name appears, or a duplicate appears outside the body
  slot;
* `gen_weapon_table.py --check` fails after the lot-category fix — that means the
  fix moved row 37's table, which §4.6 says it cannot;
* any name in a table is unrenderable, or any row's name is not the FMG string
  for that exact id;
* the config arithmetic does not come out to the file's own baseline plus 239
  bytes;
* a row count, and therefore a selection line's length, would have to change;
* the four draws cannot be made independent of one another as §4.4 describes;
* any existing case in `hunter_tools_verify.py`, `trick_weapons_verify.py`,
  `left_hand_weapons_verify.py`, `starting_weapons_verify.py` or `pool_verify.py`
  changes state other than the ones §5 and §6 name;
* an implementation decision would contradict the spec or §2 — including any
  temptation to add an unnamed piece, to write an accessory slot, to grant a
  second consumable, to change a count away from 20, to touch `ShopLineupParam`,
  or to give a picker an accompanying boolean;
* the change needs a file not listed in §5;
* a §3.1 invariant cannot be preserved.

---

## 8. Open questions

Empty — Q1 and Q2 were answered by the developer on 2026-09-29 and are recorded as
D1 and D2 in §9. Both took the planner's recommendation, so §1–§7 already described
the chosen shape and reconciliation changed nothing in the body.

---

## 9. Decisions

| #   | Date | Decision | Taken by |
| --- | ---- | -------- | -------- |
| D1  | 2026-09-29 | **Q1 — ship the two duplicate-name pairs** rather than turning on the id column. `HUNTER GARB` (71000, 281000) and `STUDENT UNIFORM` (191000, 301000) appear twice each, under the name the game gives them. Taken as recommended: spec §7 forbids inventing a label and D4 requires every wearable piece offered, and only listing both satisfies both. The pairs sit adjacent in the frozen order, so a player sees two identical rows rather than one silently hiding another. The generator asserts the duplicate set is **exactly** those two pairs, so a third is a stop condition. `showRowId = true` remains available later as a one-field change | developer |
| D2  | 2026-09-29 | **Q2 — fix `gen_weapon_table.py`'s `LOT_CATEGORY_WEAPON`, in M1.** It reads 1; category 1 is armour and 0 is weapons. Taken as recommended, and the orchestrating session **proved the fix inert for row 37** before the decision: with the constant at 1 and again at 0, `gen_weapon_table.py <vanilla> --check` prints `PASS: TrickWeaponTable.h matches the vanilla tree` — the derived 78-row set is byte-identical either way, because all 78 rows are in a shop listing so the lot half never contributed. Leaving a known-wrong constant beside a correct one in the same directory is how the wrong one gets copied next | developer |
| P1  | 2026-09-29 | The 124 pieces are **derived** from `protectorCategory`, the game's own text and `equipModelGender == 3`, and pinned by a generator with `--check`. No id is hardcoded | planner |
| P2  | 2026-09-29 | Obtainability is a **two-sided cross-check with two named exceptions each way**, not a membership gate and not an "must agree or die" guard | planner |
| P3  | 2026-09-29 | One generated header, `ArmourTables.h`, carrying all four tables plus a `static_assert` that the counts sum to 124 | planner |
| P4  | 2026-09-29 | Order per slot is uppercased name then ascending id. Frozen. The id tie-break makes each duplicate-name pair adjacent | planner |
| P5  | 2026-09-29 | Slot order is head / body / gloves / legwear everywhere — tables, config keys, UI rows and draws — taken from the game's own category and field order | planner |
| P6  | 2026-09-29 | Four pickers with **no** accompanying boolean; one boolean for the supplies. Config keys `armour_{head,body,gloves,legwear}_included` and `start_with_supplies`, costing 239 bytes | planner |
| P7  | 2026-09-29 | **Four raw `rng()` words, always exactly four whenever anything is ticked**, with `% k` indexing — the mechanism that makes the four draws independent (B6) | planner |
| P8  | 2026-09-29 | Two engine passes, not one: a deterministic supplies grant beside `HunterTools`, and an armour grant that runs **last** in `StepItemData` | planner |
| P9  | 2026-09-29 | The supplies count 20 is pinned to each goods row's own `maxNum` by the verifier rather than chosen | planner |
| P10 | 2026-09-29 | `/` is added to the fallback font rather than the three game names being sanitised | planner |
| P11 | 2026-09-29 | Four milestones, continuous, **no gate** — there is no route to probe and nothing later depends on a console observation | planner |

---

## 10. Changes during implementation

| Date | Change | Reason |
| ---- | ------ | ------ |
| 2026-09-29 | **STOPPED at M1 step 3.** M1 steps 1 and 2 are built; steps 3-6 and M2-M4 are not started. | A stop condition fired: §4.6 / §9 D2's `LOT_CATEGORY_WEAPON` 1 -> 0 is **not inert**. It is inert for row 37's table, as D2 proved, but `gen_left_hand_table.py` imports `obtainable_ids()` from the same module, and on the left hand obtainability is the **governing rule** rather than a cross-check. With the constant at 0, Wooden Shield (19000000) and Torch (20100000) become obtainable and **row 38's table goes from 14 rows to 16**. Four existing `left_hand_weapons_verify.py` cases change state (L3, L4 x2, L5) and `gen_left_hand_table.py --check` fails. That trips three of §7's stop conditions at once: an existing case in `left_hand_weapons_verify.py` changing state, a row count and therefore a selection line's length having to change, and the change needing files §5 does not list (`gen_left_hand_table.py`, `LeftHandWeaponTable.h`). It also rebases this feature's own config arithmetic: the settings block would become 1040, not 1038, so the §4.3 targets become 1279 / 1330 rather than 1277 / 1328. `gen_weapon_table.py` was reverted to its committed state; the tree is fully green. |
| 2026-09-29 | **§4.2's armour lot category is wrong and must read 1, not 0** (measured, not yet applied - the generator is not written). | §4.2 says an armour id counts as obtainable when it is "present in an `ItemLotParam` slot whose matching `lotItemCategory` is **0**". Category 0 is weapons; armour is category 1, exactly as §3.3's hazard row and `log.md`'s stage-C entry state. Measured on the vanilla tree: with category **1** the two-sided cross-check holds **exactly** as §4.2 requires - `included - obtainable == {230000, 231000, 232000, 233000}` and `obtainable - included == {250000, 430000}`. With category 0 it fails in both directions (80 ids in the first set, `{1500}` in the second) and the generator would refuse to write - the same failure mode `log.md` records for the spec's own unsatisfiable §8 criterion. The 124 do not change either way. |
| 2026-09-29 | **M1 step 3 was skipped: `LOT_CATEGORY_WEAPON` was already `0` on arrival.** | The orchestrating session cleared the blocker above before this run started, and did the whole of its fallout: `LeftHandWeaponTable.h` regenerated at **16 rows**, `left_hand_weapons_verify.py`'s `EXPECTED_ROWS` 14 -> 16, `EXPECTED_TIERLESS` 1 -> 3 and its L4 case rewritten to assert the two tests now **agree**, both generators' docstrings corrected, and `pool_verify.py`'s five picker line lengths now READ from the generated headers rather than hardcoded. Row 38 is back to 60/60. This run treated the constant as already correct and did not re-derive row 37's or row 38's table; both `--check`s pass untouched. |
| 2026-09-29 | **§4.2's lot category applied as 1, not 0** - the correction measured by the previous run. `gen_armour_table.py` uses `LOT_CATEGORY_ARMOUR = 1` and `SHOP_ROW_TYPE_ARMOUR = 1`. | Confirmed again on this tree: with category 1 the two-sided cross-check holds exactly as §4.2 states it, `included - obtainable == {230000, 231000, 232000, 233000}` and `obtainable - included == {250000, 430000}`; with 0 it fails both ways and the generator refuses to write. **The asymmetry is real and is commented as such in both generators: weapons are lot category 0, armour is lot category 1.** `gen_armour_table.py` therefore does NOT import `obtainable_ids()` from `gen_weapon_table.py` - that function is bound to the weapon values and two shipped generators depend on it for them - it measures its own. |
| 2026-09-29 | **The §4.3 config targets are 1279 / 1330, not 1277 / 1328.** | The baseline moved by 2 when `LeftHandWeaponTable.h` went 14 -> 16 rows. Measured on this tree before the change: settings block **1040**, worst-case `defaults.cfg` **1091**. §4.3's rule is baseline + 239, and the five keys cost exactly 239 (55 + 56 + 50 + 56 for the four picker lines, 22 for `start_with_supplies=1`). `pool_verify.py` now asserts **1279** and **1330** as exact equalities and both pass. Note that §4.3's own per-line breakdown "52 + 56 + 43 + 49" does not add to 217 and is wrong in detail; the total 239 it states is right. Row 13's M5 has not landed, so the alternative baseline §4.3 offers does not apply. |
| 2026-09-29 | **§6's A12 cannot assert "identical bytes either way", and now asserts identical CONTENTS instead.** | A12 requires that supplies and hunter tools, run in either order, leave "identical bytes". They cannot, and requiring it would break a §3.1 invariant rather than uphold one. Both passes take the **first empty slot**, which §3.1 mandates precisely so that neither can overwrite the other, so whichever runs first occupies item slots 1 and 2 and the other takes 3 and 4. Byte equality would need each feature to own a fixed slot index - the one arrangement that lets two features collide. What commutes is the row's contents, so A12 now asserts: all four entries present exactly once each at its own count, the Hunter's Mark still in slot 0, no row out of slots, nothing outside the item arrays changed, and **the multiset of (item, count) pairs identical in both orders**. The engine calls them in one fixed order regardless, so nothing about the shipped behaviour depends on this. |
| 2026-09-29 | `hunter_tools_verify.py` gained a new invariant, **H-I7**, alongside `--supplies`. | §6 asks for a `--supplies` tolerance, but the tool had nothing for it to relax: H-I3 already tolerates the whole item array wholesale, so a supplies run passed **without** the flag and a supplies run that had displaced a tool passed **with** it. H-I7 states what was missing - no item slot holds anything other than the Hunter's Mark and the two tools unless a flag declares it - which is what makes `--supplies` mean something and is the only check in that file able to catch a second feature's grant landing in a slot that was not free. No existing case changed state (29/29 -> 45/45, all additions). |
| 2026-09-29 | `starting_gear_verify.py verify` also takes **`--ticked h,b,g,l`**, four selection lines at their own lengths. | §6's G4 requires that each granted piece "is one of that slot's **ticked** rows". Without knowing what was ticked the tool can only check that it is a row of the right table, which is the weaker half. The flag is optional; G4 falls back to the table check when it is absent. |
| 2026-09-29 | `pool_verify.py`'s new progress-line case measures **whole-literal** `AddProgressLine` calls plus the widest `WEARING <NAME>` line, not every string fragment. | A general "every progress-line fragment is renderable" case fails on a **pre-existing** string: `"STARTING CHOICES: "` on the confirm block carries a colon, which the FALLBACK glyph table does not hold. That predates this feature and is out of its scope (§3.2), and a sweeping case would read as feature 040 having broken it. The new cases cover both of this feature's lines: the supplies line is a whole literal, and the `WEARING` line's worst case is computable because the 124 names are in a generated header. |
| 2026-09-29 | `EnemyRandomizerOptions` gained a small helper, **`AnyArmourTicked()`**. | Three sites ask "does any of the four pickers have anything ticked" - `AnyParamFeature()`, the pass's own gate, and the pass ordering comment. Four `||` terms spelled out three times is three chances to forget a slot. `WorldActivation.cpp`'s `anythingOn` chain deliberately does **not** use it: that chain reads the stored recipe (`run.`), not the options struct the helper belongs to, and `worlds_verify.py` pins it term by term. |
| 2026-09-29 | `left_hand_weapons_verify.py` was **accidentally reverted and then reconstructed**, and its module docstring corrected. | An errant `git checkout --` in this run destroyed the orchestrating session's uncommitted edit to that file. It was reconstructed from the substance recorded in the work order - `EXPECTED_ROWS` 14 -> 16, `EXPECTED_TIERLESS` 1 -> 3, the L4 case asserting agreement in both directions - and is back to **60/60** with the same notes it printed before. The wording of the reconstructed comments is this run's, not the orchestrating session's. Two now-false prose statements were also corrected while restoring it: the module docstring's "THE ONE RULE THAT IS GENUINELY DIFFERENT" paragraph, which described the 14-row disagreement as a finding rather than the artefact it turned out to be, and the usage line's "`--ticked <14-character line>`". |
| 2026-09-29 | **Not fixed, and noted:** `worlds_verify.py all`'s options-mapping case fails on **row 013's** eight uncommitted `options.bossInsertion.*` mappings, which were never added to `WIZARD_OPTIONS_MAPPING`. | Pre-existing and not this feature's. Demonstrated: at `HEAD` the case passes; on the working tree the extracted list minus the eight `bossInsertion.*` entries equals the frozen list **exactly**, so feature 040's five additions are in the right positions with the right spellings and the only discrepancy is row 013's. Also worth knowing: `worlds_verify.py **selftest**`, which §6 names, runs only the 29 manifest cases and never reaches this one - the mapping and run-decision cases need `worlds_verify.py all` (or `activation`). |
