# 034 — Start With Hunter Tools — implementation report

**Status: built, verified offline, READY FOR HARDWARE TESTING — not complete.**
**Backlog row:** `docs/randomization-feature-spec.md` §4, row 34.
**Date:** 2026-09-19.

> **Process note.** Stages B (spec) and C (plan) were deliberately skipped at
> the user's explicit request — "skip the spec and the plan and go straight to
> implementing". This document therefore carries the investigation, the
> decisions and the verification that would normally be spread across
> `spec.md`, `plan.md` and `plan-evidence.md`. It is written after the fact,
> which is exactly what the pipeline normally forbids; treat its reasoning with
> the extra scepticism that earns.

---

## 1. What it does

A new setting, `START WITH HUNTER TOOLS` (default **NO**). When on, a new
character begins holding both of Bloodborne's workshop key items:

| Item | Goods id | What it unlocks |
|---|---|---|
| Blood Gem Workshop Tool | `4103` | Fitting and removing blood gems on weapons |
| Rune Workshop Tool | `4104` | Equipping Caryll runes |

**The problem it solves.** The randomizer hands out blood gems and Caryll runes
from the first area, but vanilla gates the ability to *fit* either one behind
two key items that sit most of the way through the early game — a chest in
Central Yharnam and a chest in Hemwick Charnel Lane. Until both are found, every
gem and rune the randomizer gives you is dead weight.

**It is not the same feature as row 7.** `RANDOMIZE WORKSHOP TOOLS`
(`docs/plans/workshop-tools.md`) puts those same two items into the treasure
*shuffle* — it moves them. This one grants them outright and leaves the world
alone. They are independent; either, both, or neither may be on. Both on means
the tools are in your inventory from the start *and* their vanilla chests hold
something else.

---

## 2. The unverified assumption this rests on

**We do not know whether Bloodborne unlocks gem fitting because the player
HOLDS the item, or because an event flag was set when the vanilla chest was
opened.** Nothing in the param data settles it, and no amount of further
offline work can. This is the single thing hardware testing must decide.

The evidence leans towards possession, but it is inference, not proof:

- Goods `4103` and `4104` carry `refId = -1` — no `SpEffect` attached, so the
  unlock is not driven by an effect the item grants.
- Their item lots carry only `getItemFlagId = 52411000` / `52200360`, which is
  the mechanical `5` + lot id "this pickup has been collected" marker that
  *every* ordinary lot has (`2100000` → `52100000`, `2410100` → `52410100`).
  There is no bespoke unlock flag on either of them.
- The reference Windows randomizer ships a shuffle of these two items as a
  played feature, which would be broken if the unlock were keyed to the
  specific vanilla chest entity.

**The counter-evidence is that nobody has actually checked.**
`docs/plans/workshop-tools.md` §5.3 defines that feature's hardware check as
"confirm the Central Yharnam chest gives something other than the Blood Gem
Workshop Tool" — i.e. it confirmed the chest changed, **not** that a relocated
tool still unlocks anything. So row 7 rests on the same unverified assumption
this row does, and a negative result here is also a finding about row 7.

**If the unlock turns out to be flag-based, the honest fix is to delete this
feature, not to grow it.** Granting two inert key items is worse than not
offering the setting.

---

## 3. What the data says

All measured against `data/vanilla/dvdroot_ps4`, not taken from any document.

### 3.1 The item identification is solid

Resolved through `ItemLotParam` rather than asserted — the two lots
`TreasureRandomizer.cpp` already protects as the workshop tools:

```
lot 2411000  ids=[4103, 0, ...]  cats=[4, 0, ...]      (category 4 = goods)
lot 2200360  ids=[4104, 0, ...]  cats=[4, 0, ...]
```

Both goods rows exist in `EquipParamGoods`. `hunter_tools_verify.py` re-derives
this every run rather than hardcoding the conclusion.

### 3.2 `CharaInitParam` has room

`CharaInitParam.param` is already inside `gameparam.parambnd.dcx`, the archive
the app decompresses, edits in place and rewrites for enemy drops and starting
weapons. The row layout (from `tools/param_offsets.py`, 300-byte row):

| Field | Offset | Type |
|---|---|---|
| `item_01` … `item_10` | 124, stride 4 | s32 (`-1` = empty) |
| `itemNum_01` … `itemNum_10` | 204, stride 1 | **u8** |

The two arrays are **not adjacent** — 80 bytes of other fields sit between
them — and the count is a *single byte*. Writing four bytes there would corrupt
neighbours, the same trap `StartingWeapons.cpp` documents for its u8 stat
requirements.

Every targeted row in vanilla holds exactly `1×` goods `100` in slot 0 and
leaves slots 1–9 at `-1`, so there are **nine free slots per origin** and the
feature needs two.

### 3.3 Which rows — and why 22 of them

Exactly 22 rows in the whole 1699-row param carry the player-origin signature
(one starting item, `1×` goods `100`): **2000–2009, 3000–3009, 3500, 3501**.
The other 1658 rows with items set are NPC templates holding `5×1000` and `900`.

`2000–2009` and `3000–3009` are byte-identical in every field this feature
touches, differing only in `equip_Helm` (`230000` vs `-1`). **The data cannot
say which block the game actually reads.**

**Decision: write all 22.** Writing a starting item into a row the game never
instantiates does nothing at all, whereas guessing wrong and writing only one
block would look exactly like "the feature does not work" — and would send the
hardware test chasing the wrong question. This is a deliberate hedge and it is
cheap.

**If the hardware test shows the tools arrive, `kOriginRows` can be narrowed to
whichever block is live.** It cannot be narrowed before then.

---

## 4. Decisions

- **D1 — Name.** `START WITH HUNTER TOOLS`, the user's wording. Named for its
  action, so YES is the only state that writes anything and NO asks for nothing
  to be done — the polarity rule row 8 exists to enforce.
- **D2 — Default NO.** A `defaults.cfg` written before this key existed reads
  as off and the run is unchanged, roll for roll.
- **D3 — It gates a run on its own.** Unlike `randomizeWorkshopTools` and
  `doNotRandomizeCagedDogs`, which are modifiers on another feature, this one
  changes the game by itself. So it **is** in `StartCommit`'s big `||` and
  **is** in `AnyParamFeature()`: ticking it alone must build a tree.
- **D4 — Appended last on all three lists** (row 16, counts 16 → 17), not
  grouped next to `RANDOMIZE WORKSHOP TOOLS` which it is easily confused with.
  Going last is what guarantees no existing row index moves and so no existing
  row can be mislabelled.
- **D5 — Reported as a state, not a count.** The progress log says
  `STARTING WITH BOTH HUNTER WORKSHOP TOOLS`, not "22 rows changed": the honest
  count covers both origin blocks and a player who picks one origin would read
  22 as a defect. No `SKIPPING` counterpart, for the same reason
  `ENABLE MERGO DARKNESS` has none.
- **D6 — Free-slot search, not hardcoded slots 1 and 2.** So that a future
  feature which also grants a starting item cannot silently overwrite this one.
  Re-running the pass over an already-edited buffer is a no-op.

---

## 5. Files changed

| File | Change |
|---|---|
| `app/src/Randomizer/HunterTools.h` | **New.** Interface, and the assumption in §2 stated where it will be read |
| `app/src/Randomizer/HunterTools.cpp` | **New.** Offsets, the 22 target rows, the grant pass |
| `app/src/Randomizer/EnemyRandomizer.h` | `startWithHunterTools` option; added to `AnyParamFeature()`; two result counters |
| `app/src/Randomizer/EnemyRandomizer.cpp` | Locates `CharaInitParam.param` and calls `GrantHunterTools` in `StepItemData` |
| `app/src/Randomizer/RandomizerDefaults.h` | `startWithHunterTools = false` |
| `app/src/Randomizer/RandomizerDefaultsStore.cpp` | `start_with_hunter_tools` in both load and save |
| `app/src/UI/SetupDefaultsScreen.h/.cpp` | Row 16, `kItemCount` 16 → 17 |
| `app/src/UI/EnableWizardScreen.h/.cpp` | Row 16, `kSaveDataRowCount` 16 → 17, commit gate, options, progress line |
| `app/tools/hunter_tools_verify.py` | **New.** Property validator, 13 selftest cases |
| `app/tools/pool_verify.py` | Config-buffer worst case 585 → 611; both-load-and-save case for the new key |
| `app/tools/ui_scroll_verify.py` | Three settings screens 16 → 17 rows |

No `Makefile` change — sources are discovered with `find`.

---

## 6. Verification

**Cross-compile:** clean, no warnings, `.pkg` produced.

**Python verification:**

```
hunter_tools_verify.py selftest   13/13
hunter_tools_verify.py rows       all 22 rows present, 9 free slots each
pool_verify.py selftest           87/87   (was 86/86; +1 case)
ui_scroll_verify.py               all geometry and scroll properties PASSED
treasure_verify.py selftest        8/8    (regression - row 7 untouched)
drops_verify.py selftest           6/6
starting_weapons_verify.py         12/12
boss_verify.py selftest            5/5
caged_dogs_verify.py               23/23
mergo_darkness_verify.py            9/9
itemdata_verify.py roundtrip       PASS
```

`hunter_tools_verify.py` checks six invariants against real vanilla bytes, and
its selftest confirms the comparison **rejects** each way of getting it wrong:
doing nothing, granting only one tool, writing only one origin block, a count of
0, displacing the vanilla starting item, a stray write elsewhere in the row,
touching a non-origin row, and touching another param member. It also asserts
the C++ constants match the paramdef — without that, both implementations could
read the wrong bytes in agreement.

**What none of this proves:** that the game does anything with the items. See §2.

---

## 7. Hardware test

1. Enable with `START WITH HUNTER TOOLS = YES`. Nothing else is required — it
   builds a tree on its own (D3). The progress log should end with
   `STARTING WITH BOTH HUNTER WORKSHOP TOOLS` and an `ITEM DATA … MB` line.
2. Start a **new character** (the setting only affects character creation — an
   existing save will not gain anything).
3. Check the inventory for both tools. **If they are absent**, the live origin
   block is not among the 22 rows, or `CharaInitParam` is not what feeds a new
   character — report which, and this is a data question, not a code one.
4. **The real test:** reach the Hunter's Dream workshop and try to fit a blood
   gem, then try to equip a Caryll rune.
   - **Both work** → the unlock is possession-based, the feature is correct,
     and row 7's relocated tools are confirmed working too. `kOriginRows` can
     then be narrowed to the live block.
   - **Items present but fitting is still locked** → the unlock is event-flag
     based. The feature cannot work this way and should be deleted (§2), and
     row 7 needs re-examining.

---

## 8. Open items

- **O1** — §2 is unresolved and gates the whole feature.
- **O2** — If §2 resolves positively, narrow `kOriginRows` to the live block and
  tighten `hunter_tools_verify.py`'s `ORIGIN_ROWS` with it.
- **O3** — `docs/plans/workshop-tools.md` §5.3 should be amended to record that
  its hardware check did not cover the relocated-tool unlock, whichever way this
  test goes.
- **O4** — `docs/user-guide.md` has no entry for this setting yet.
