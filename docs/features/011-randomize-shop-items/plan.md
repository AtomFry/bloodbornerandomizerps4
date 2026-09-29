# Plan 011 — Randomize Shop Armour and Shop Items

**Status: APPROVED** — the developer approved this plan, and the Execution
Strategy below with it, on 2026-09-28. Stage D (plan review) was skipped at the
developer's request. §8 is empty; its questions were answered the same day and
are recorded as D1-D3 in §9, D3 having been folded in after the strategy was
first presented.

**Spec:** `docs/features/011-randomize-shop-items/spec.md` — **APPROVED**
(2026-09-28). Four binding decisions, §10 D1–D4.

**Evidence:** `docs/features/011-randomize-shop-items/plan-evidence.md` — the
measurements, traces and rejected alternatives behind this plan.

**Plan review:** `docs/features/011-randomize-shop-items/plan-review.md` — added
during review.

---

## Execution Strategy

*Approving this plan approves this strategy. `CLAUDE.md` §4: milestones describe
implementation structure; gates describe when a human stops to test.*

| | |
| --- | --- |
| **Structure** | 3 functional milestones |
| **Execution** | continuous |
| **Human test gates** | final only |
| **Intermediate verification** | after every milestone: `cd app && make` (`make clean && make` in M2), plus the §6 automated checks that apply to that milestone |

No intermediate gate is proposed. The param, the field, the write and the
in-place archive edit are all proven by shipped row 5, which writes the same
four bytes of the same table; nothing here is a probe. No milestone can corrupt
or destroy data — the only write is into a staging tree, and save data is
untouched (spec §7.9). Nothing in M2 or M3 makes a failure in M1 harder to
diagnose: M1's output is checked byte-wise against vanilla by the M3 verifier
from a single saved run.

---

> **This document is the implementation contract.** §1–§7 are what the
> implementer reads, in order. §8–§10 are the record, not instructions.
> `plan-evidence.md` holds the investigation; `log.md` holds the history.

---

## 1. Objective

Add two independent settings that shuffle the game's shop stock within its own
kind: `RANDOMIZE SHOP ARMOUR` deals the 44 pieces of armour the shops sell back
out at random across their 220 listings, and `RANDOMIZE SHOP ITEMS` does the
same for 57 consumables and materials across 397 listings. Only the item id of a
listing is written, so every listing keeps its price, currency, purchase limit
and shop. Blood Vials, Quicksilver Bullets and one dangling id are left exactly
as they are, and weapon listings — row 5's — are never touched.

---

## 2. Approved behaviour

| #   | Behaviour | Source |
| --- | --------- | ------ |
| B1  | **Two** independent toggles, `RANDOMIZE SHOP ARMOUR` and `RANDOMIZE SHOP ITEMS`, both in `WEAPONS & STARTING GEAR` beside `RANDOMIZE SHOP WEAPONS`. Either, both or neither | spec §10 D1 |
| B2  | `RANDOMIZE SHOP ARMOUR` replaces the `equipId` of the 220 `equipType == 1` listings with a uniform random permutation of the ids those same listings held | spec §2, §4 F2 |
| B3  | `RANDOMIZE SHOP ITEMS` does the same for the 397 shufflable `equipType == 3` listings | spec §2, §4 F3, F5 |
| B4  | The skip list is item ids `1000`, `900`, `240`, matched on **equipId** in both the pool build and the write, so those 27 listings are neither sources nor destinations | spec §3, §4 F4, §7.2 |
| B5  | **Only `equipId` is written.** Price, `value_SAN`, `mtrlId`, `eventFlag`, `qwcId`, `sellQuantity`, `shopType` and `equipType` are never touched, and no other archive member changes | spec §7.4 |
| B6  | The two buckets never cross, and `equipType == 0` is never written by this feature | spec §7.3, §4 F11 |
| B7  | The pool is keyed by `equipType` alone — shop, currency and price tier are not consulted, so an Insight listing and an echo listing draw from one pool | spec §10 D2 |
| B8  | Once-only listings stay in the shuffle; the purchase limit belongs to the listing, not to the item | spec §10 D3 |
| B9  | The two unidentified shop families are in the pool, unrestricted | spec §10 D4 |
| B10 | A listing may be dealt the item it already had. There is no reroll against that | spec §4 F7 |
| B11 | Bounded work only: no reroll loop of any kind | spec §7.7 |
| B12 | Both shuffles are decided once per run from the run's seed. The same seed and the same set of settings produce the same shops | spec §2, §7.10 |
| B13 | With both settings off, the shop table is byte-identical to vanilla, and row 5's three settings produce exactly the bytes and the rolls they produce today | spec §8.7, §7.3 |
| B14 | The item-data archive is edited in place and written once, as another pass inside the existing phase | spec §7.5 |
| B15 | The 24 chalice materials and chalices in the consumable bucket **are** shuffled — they are ordinary Hunter's Dream shop stock | spec §6 |
| B16 | State persists in the existing config; an absent key reads as off | spec §6, house rule |

---

## 3. Constraints and invariants

### 3.1 Must be preserved

* **Row 5's output is unchanged.** `RANDOMIZE STARTING WEAPONS` / `GUNS` /
  `SHOP WEAPONS` keep their present bytes and their present RNG draws for a
  given seed, with the new settings on or off — the new passes run **after**
  row 5's pass has returned, and its own code is edited only to include a
  carved-out header.
* **The skip list exists once.** `kNeverTouchEquipIds` moves into a shared
  header; a second copy anywhere is a defect, because the two features'
  protection must not drift apart (spec §7.2).
* **`equipType == 0` is never written here.** The three-way partition of the
  1,288 listings is what makes row 5 and row 11 non-interfering (spec §4 F11).
* **Only `equipId` moves, in either bucket.** Every other byte of a 32-byte
  listing, and every other archive member, comes out identical.
* **No reroll loop.** An unbounded loop hangs the console (deviation SW-5,
  `docs/plans/starting-weapons.md` §6).
* **The archive is edited in place** — one 4-byte write over an existing field.
  Nothing is resized, nothing is rebuilt, and the archive is decompressed and
  re-emitted exactly once per run.
* **Both settings off draw zero randomness**, so an off run's roll sequence is
  the one the app produces today.
* **Config compatibility both ways**: unknown keys are ignored on load, and an
  absent new key reads as off.
* **Layering**: the pass, the constants and the options live under
  `app/src/Randomizer/`; no SDL2 there, and no UI file learns a param offset.

### 3.2 Out of scope

* Weapon listings and the five Hunter's Dream coffin rows — row 5's.
* Prices, currencies, stock limits, unlock flags, `shopType` and which shop a
  listing belongs to. The reference writes the id and nothing else, and D3 makes
  the consequences the feature rather than a defect.
* Restricting the pool to reachable shop families. D4 includes all of them; the
  restriction is future work if the hardware observation calls for it.
* Any protected-item list beyond the reference's three ids.
* *(Adding tolerance flags to `drops_verify.py` and `hunter_tools_verify.py` was
  out of scope here, and is now IN scope as M3 change 4 — see D3.)*
* `EnemyRandomizerJob::StatusText`'s `ItemData` label, which reads
  `RANDOMIZING ENEMY DROPS` for every param feature already.
* `docs/screen-text-inventory.md` and `docs/user-guide.md` — the documentation
  stage owns them.
* **Chalice dungeons.** 24 shuffled consumables are chalice materials and root
  chalices, and B15 shuffles them: they are stock of ordinary Hunter's Dream
  shops, and the scope decision in `docs/design-decisions.md` is about chalice
  **content** — no chalice shop is added, no chalice behaviour is examined, and
  "a ritual material changed price" is not a defect of this feature.

### 3.3 Hazards

| Hazard | What to do | Evidence |
| ------ | ---------- | -------- |
| `starting_weapons_verify.py`'s shop loop iterates **every** listing and asserts any changed id exists in `EquipParamWeapon` (SW-I4); an armour or consumable id fails it, so the tool is wrong in this feature's presence | Fix it in M3 exactly as §4.5 says: dispatch the existence check on the vanilla `equipType`, and re-scope the churn counter to weapon rows. Do not add a second tool that parses this table | `plan-evidence.md` §E5.1 |
| A `std::uniform_int_distribution<int>(0, pool.size() - 1)` on an empty pool is `(0, -1)` — undefined behaviour | Test the pool for empty before drawing and stop that bucket, the way the reference's `Count > 0` guard does. It cannot happen on real data; the guard is what keeps that true | §E5.2 |
| The skip list is matched on the **item id**, not the row id or the type. Filtering the pool build and the write differently by so much as one condition breaks the exactness of the permutation | Build the target list and the pool in **one** pass per bucket, from one filter, then write from that list. Never write a listing the pool build did not count | §E1 |
| Consumable ids and armour ids are small integers in overlapping numeric ranges with weapon ids, and five consumable ids also exist as `EquipParamProtector` row ids | The bucket a listing belongs to comes from its `equipType` byte, never from the shape of its id. The verifier's existence check is per-bucket for the same reason | §E4 M6 |
| `pool_verify.py` pins the settings block at an **exact** byte count and fails the moment a key is added | Update to **775 / 826** in M2, in the same pass as the keys, extending the comment that narrates the arithmetic | §E4 M7 |
| `settings_ui_verify.py` case 1 fails as soon as `RandomizerDefaults` gains a bool with no model entry, and case 3 fails until the settings-UI spec's §7.1 table lists the setting | Do M2's steps in the order given: field, model entry, `docs/features/randomizer-settings-ui/spec.md` §7.1 and `PROSE_TO_LABEL` together | §E5.3 |
| `worlds_verify.py` pins the options mapping and the `anythingOn` chain as ordered lists | Insert both fields immediately after `randomizeShopWeapons` in the source **and** in both pinned lists | §E4 M8 |
| `RandomizerDefaults` grows, and two screens and `WorldStore` hold copies of it; an incremental `make` does rebuild on a changed header today, but this trap cost eight hours once | `make clean && make` in M2 | `docs/known-traps.md` |
| Printing an item or armour name on this console raises `UnicodeEncodeError` that reads like a parse failure | The verifier prints ids and counts, never names. It needs no FMG access at all | `docs/known-traps.md` |

---

## 4. Implementation approach

> Chosen: one new engine pass in its own file, two independent per-bucket
> selection-sampling permutations, with the shop field offsets and the skip list
> carved out into a shared header so they exist once; verification by extending
> the tool that already owns this table. Alternatives considered and why they
> were rejected: `plan-evidence.md` §E3.

### 4.1 The shared constants

`app/src/Randomizer/ShopLineup.h` — **new**, header-only, the shape
`WeaponRequirements.h` already has. Three items are moved **verbatim** out of
`StartingWeapons.cpp`'s anonymous namespace, with no value changed, and two are
new:

* moved: `kShopEquipId = 0`, `kShopEquipType = 23`, `kEquipTypeWeapon = 0`, and
  `kNeverTouchEquipIds = { 1000, 900, 240 }` with its predicate, renamed
  `IsNeverTouchedEquipId` and made `inline`;
* new: `kEquipTypeArmour = 1` and `kEquipTypeGoods = 3`.

`StartingWeapons.cpp` includes it and **deletes** its local copies, keeping its
`IsNeverTouched` call sites working by calling the shared function. No value
changes, so row 5's output stays byte-identical.

### 4.2 The pass

`app/src/Randomizer/ShopStock.{h,cpp}` — **new**, beside `StartingWeapons`, not
an extension of it: the two features share a table, not a feature.

```
struct ShopStockOptions { bool randomizeShopArmour; bool randomizeShopItems; bool Any() const; }
struct ShopStockResult  { int armourRowsChanged; int itemRowsChanged; }
bool RandomizeShopStock(std::vector<uint8_t>& plain, const ParamMember& shopParam,
                        const ShopStockOptions&, std::mt19937&, ShopStockResult&,
                        std::string* error);
```

Returns immediately when `!options.Any()`, having parsed nothing and drawn
nothing. Otherwise it parses the shop rows once with `ParseParamRows` and runs
one bucket at a time — **armour first, then items** — with this body per bucket:

1. Walk the rows in parse order. Keep a listing when its `equipType` byte equals
   the bucket's type and `!IsNeverTouchedEquipId(equipId)`. Append the row to
   `targets` and its `equipId` to `pool`. **One filter, both lists.**
2. For each entry of `targets`, in order: if `pool` is empty, stop this bucket;
   otherwise draw `index` uniformly from `[0, pool.size() - 1]`, write
   `pool[index]` over that listing's `equipId`, and erase the entry from `pool`.
   This is the reference's `Next` + `RemoveAt` selection sampling, which is where
   the exactness of the permutation comes from: the pool is built from precisely
   the listings step 2 writes, so it empties on the last one.
3. Count a listing in the bucket's result counter only when the value actually
   changed, and log the bucket's target count and changed count.

A bucket that is not enabled is not walked and draws nothing.

### 4.3 Where the pass runs, and what that fixes

In `StepItemData`, step 1, the call goes **immediately after** row 5's
`RandomizeStartingWeapons` block and **before** the `startWithHunterTools`
block, so:

* every draw row 5 makes precedes every draw this feature makes — B13;
* the two character-creation grants stay the last rolls of the run, which is the
  property spec 037 §7 recorded and the comment in `StepItemData` states.

Turning either new setting on therefore moves the grants' draws for a fixed
seed, exactly as turning `RANDOMIZE SHOP WEAPONS` on does today, and armour's
220 draws precede items' 397. That ordering is fixed and is what B12 means: a
seed reproduces a run's shops given the same **set** of settings, not across
different sets. Settled by D1: after row 5, before the grants.

The block locates `ShopLineupParam.param` itself, per feature, as every other
block in that function does, and `Fail`s with its own message if it is absent.
`EnemyRandomizerOptions` gains `randomizeShopArmour` and `randomizeShopItems`
after `randomizeShopWeapons`, both in `AnyParamFeature()`;
`EnemyRandomizerResult` gains `shopArmourChanged` and `shopItemsChanged`.

### 4.4 The settings chain

| Site | What to add |
| --- | --- |
| `RandomizerDefaults.h` | `bool randomizeShopArmour = false;` and `bool randomizeShopItems = false;` after `randomizeShopWeapons`, with the standing "an older `defaults.cfg` must not silently turn this on" note |
| `RandomizerDefaultsStore.cpp` | keys **`randomize_shop_armour`** and **`randomize_shop_items`** in `FormatSettings` **and** `ApplySettingKey`. 24 + 23 = 47 bytes: settings block 728 → **775**, worst-case `defaults.cfg` 779 → **826**, inside `char buf[1024]` with 249 bytes spare |
| `SettingsModel.h` | `SettingId::RandomizeShopArmour`, `SettingId::RandomizeShopItems`, declared after `RandomizeShopWeapons` |
| `SettingsModel.cpp` | two `kSettings` entries, kind `Toggle`, category `WeaponsGear`, declared **after** `RandomizeShopWeapons` and before `StartWithHunterTools` |
| labels | `RANDOMIZE SHOP ARMOUR` (444 px) and `RANDOMIZE SHOP ITEMS` (395 px), both inside the 700 px pane row |
| help — armour | `"Randomizes the armour sold by the Bath Messengers. Prices and purchase limits stay with the listing, not the item."` |
| help — items | `"Randomizes the items and materials sold by the Bath Messengers. Blood Vials and Quicksilver Bullets are left alone."` |
| `Game/WorldActivation.cpp` | `options.randomizeShopArmour = run.randomizeShopArmour;` and the same for items, immediately after `randomizeShopWeapons` in the frozen field order, and both added to the `anythingOn` chain in the same position |
| `UI/WorldEditorScreen.cpp` | two `SKIPPING` lines — `SKIPPING SHOP ARMOUR RANDOMIZATION`, `SKIPPING SHOP ITEM RANDOMIZATION`; two report lines — `RANDOMIZED <n> SHOP ARMOUR LISTINGS`, `RANDOMIZED <n> SHOP ITEM LISTINGS`; and both settings added to the item-data-archive line's condition |
| `docs/features/randomizer-settings-ui/spec.md` | §7.1's `Weapons & Starting Gear` row gains both settings after `Randomize Shop Weapons`; `Shop Items` leaves the `Items & Treasure` backlog cell |

Both settings screens need **no change**: they render any `Toggle` generically
from the model, and the editor's Confirm list is generated from the same table.
No Makefile change — it compiles `.cpp` recursively. No saved world is
invalidated and no `defaults.cfg` needs migration.

### 4.5 The verification mirror

`app/tools/starting_weapons_verify.py` is **extended**, not duplicated (spec
§6): it already reads this table out of both trees, already asserts that only
`equipId` may differ in any listing, and is the tool F13 says is wrong here. Its
name stays — `CLAUDE.md` §1 pins the path.

Three changes to `compare()`:

1. **The F13 correction.** Read each listing's `equipType` from the **vanilla**
   row and dispatch the existence check on it: type 0 against
   `EquipParamWeapon.param` (SW-I4, unchanged), type 1 against
   `EquipParamProtector.param`, type 3 against `EquipParamGoods.param`
   (SW-I10). Both new members are read-only and stay **untolerated** by SW-I1,
   so they must come out byte-identical.
2. **Re-scoped to weapon rows only:** SW-I3, SW-I4, SW-I5 and the
   `counts["shop"]` churn figure. `stock()` and SW-I8 are already type-filtered
   and do not move.
3. **New invariants** SW-I10 to SW-I14, and two repeatable-style declaration
   flags `--shop-armour` / `--shop-items` on `verify`, following `--granted`'s
   precedent: a bucket that is **not** declared must come out byte-identical to
   vanilla, and a bucket that **is** must satisfy SW-I11/I12/I14.

---

## 5. Files and changes

| File | Change | M |
| ---- | ------ | - |
| `app/src/Randomizer/ShopLineup.h` | **new** — shop field offsets, the three `equipType` constants, the skip list and its predicate, moved verbatim from `StartingWeapons.cpp` | 1 |
| `app/src/Randomizer/StartingWeapons.cpp` | include the new header; delete the moved constants and the local `IsNeverTouched` | 1 |
| `app/src/Randomizer/ShopStock.h` | **new** — options, result, `RandomizeShopStock` | 1 |
| `app/src/Randomizer/ShopStock.cpp` | **new** — the two per-bucket permutations | 1 |
| `app/src/Randomizer/EnemyRandomizer.h` | two options after `randomizeShopWeapons`, both in `AnyParamFeature()`; two result counters | 1 |
| `app/src/Randomizer/EnemyRandomizer.cpp` | the pass call in `StepItemData` step 1, after row 5's block and before hunter tools | 1 |
| `app/src/Randomizer/RandomizerDefaults.h` | two bool fields after `randomizeShopWeapons` | 2 |
| `app/src/Randomizer/RandomizerDefaultsStore.cpp` | `randomize_shop_armour` and `randomize_shop_items` in `FormatSettings` and `ApplySettingKey` | 2 |
| `app/src/UI/SettingsModel.h` | two `SettingId` values | 2 |
| `app/src/UI/SettingsModel.cpp` | two `kSettings` entries with §4.4's labels and help | 2 |
| `app/src/Game/WorldActivation.cpp` | the two mapping lines and the two `anythingOn` terms | 2 |
| `app/src/UI/WorldEditorScreen.cpp` | two `SKIPPING` lines, two report lines, the item-data line's condition | 2 |
| `app/tools/drops_verify.py` | a `--shop-stock` tolerance flag: with it, `ShopLineupParam` differing is not a failure; without it, unchanged | 3 |
| `app/tools/hunter_tools_verify.py` | the same `--shop-stock` flag, same semantics | 3 |
| `app/tools/pool_verify.py` | settings block 728 → 775, worst case 779 → 826, and a written-and-read case per new key | 2 |
| `app/tools/settings_ui_verify.py` | `PROSE_TO_LABEL` gains both prose names | 2 |
| `app/tools/worlds_verify.py` | `WIZARD_OPTIONS_MAPPING`, `WIZARD_RUN_DECISION` and the `SKIPPING`-line field list gain both fields | 2 |
| `docs/features/randomizer-settings-ui/spec.md` | §7.1 table, per §4.4 | 2 |
| `app/tools/starting_weapons_verify.py` | the F13 correction, the re-scoping, SW-I10–SW-I14, the two flags, the new selftest cases, and the docstring's invariant list | 3 |

A clean rebuild is required in M2 (`RandomizerDefaults` changes size).

---

## 6. Verification

### Build

`cd app && make` after every milestone; **`make clean && make` in M2**. The
`.pkg` must be produced.

### Automated

| Check | Command | Asserts |
| ----- | ------- | ------- |
| Row 5 unaffected | `python app/tools/starting_weapons_verify.py selftest data/vanilla/dvdroot_ps4` | after M1's carve-out and M3's rewrite, every existing case still passes with the same expectations |
| The mirror | the same command, after M3 | SW-I10–SW-I14 and the new cases below |
| Output tree | `python app/tools/starting_weapons_verify.py verify <vanilla> <output> [--shop-armour] [--shop-items] [--granted <id>]…` | spec §8 criteria 1–9 against a real run |
| Table listing | `python app/tools/starting_weapons_verify.py pool data/vanilla/dvdroot_ps4` | prints the two new bucket sizes and distinct counts beside the weapon pool; not a pass/fail gate |
| Config | `python app/tools/pool_verify.py selftest data/vanilla/dvdroot_ps4` | the 775 / 826 arithmetic, and that each new key is in both load and save |
| Settings model | `python app/tools/settings_ui_verify.py` | each new bool has exactly one entry, the rows fit the pane, help is non-empty and wraps, and the model matches the settings-UI spec §7.1 |
| Worlds parity | `python app/tools/worlds_verify.py selftest` | the options mapping and the run decision carry both fields in the pinned order, and a recipe round-trips both keys |
| Archive round trip | `python app/tools/itemdata_verify.py roundtrip data/vanilla/dvdroot_ps4` | the write path still emits a valid archive at 28 MB scale |

`drops_verify.py verify` and `hunter_tools_verify.py verify` both take a
`--shop-stock` flag as of M3 (D3), so they can be run against the **all-on**
tree rather than only against an all-off one. Without the flag both still assert
"only my member differs" and both still fail on a shop-randomised tree, which is
the behaviour every existing case depends on.

This matters because D2 takes a single hardware run with everything on: without
the flag there would be no tree either tool could say anything about, so the
verification those two provide would be absent rather than merely noisy.

The extended selftest must assert, against the vanilla tree, by simulating
output in Python:

| # | Assertion |
| - | --------- |
| S1 | An unmodified archive passes with no flags, and passes with both flags — an identity permutation is a permutation |
| S2 | A simulated armour-only permutation passes with `--shop-armour`, and is **rejected** without it (SW-I14) |
| S3 | A simulated items-only permutation passes with `--shop-items`, and is **rejected** without it |
| S4 | Both simulated together pass with both flags; the armour bucket's 220 and the items bucket's 397 output multisets equal vanilla's (SW-I11) |
| S5 | An armour listing given a consumable id, and a consumable listing given an armour id, are both rejected (SW-I12) |
| S6 | An armour listing given an id with no `EquipParamProtector` row, and a consumable listing given an id with no `EquipParamGoods` row, are both rejected (SW-I10) |
| S7 | Swapping two Blood Vial listings' ids with two other listings' is rejected, and so is writing `1000`, `900` or `240` into any listing (SW-I13) |
| S8 | Changing any byte of a listing outside `equipId` is rejected, `equipType` included (SW-I2, unchanged) |
| S9 | Changing one byte of `EquipParamProtector.param` or `EquipParamGoods.param` is rejected (SW-I1) |
| S10 | A permuted archive with **one** listing per bucket reverted still passes, and one with 200 of 220 armour listings reverted is rejected (SW-I14 — about 5 of 220 and 9 of 397 are expected to hold their vanilla item) |
| S11 | A combined row 5 + row 11 + grant archive passes with all three declarations, and each existing row-5 case still fails for its own reason (spec §8.8) |

`verify` must additionally assert, for a real tree: the member list and every
member size unchanged; only `ShopLineupParam.param` differs unless another
declared feature owns the member; the row id set unchanged; and the per-bucket
changed counts reported alongside row 5's.

**A mirror pins the rules, not the C++ implementation of them.** It shows that
an output tree satisfies the permutation, the skip list, the field and the
bucket boundaries; it cannot show that the engine drew uniformly, and it
deliberately does not predict which item a seed puts in a given listing.
Determinism is a hardware check.

### Hardware

The implementer cannot run any of this. It is spec §8's criteria 11–16 verbatim
and in that order, with these first because they are the run-deciding ones:

1. **The vials are there.** First visit to the Hunter's Dream: Blood Vials and
   Quicksilver Bullets on sale at vanilla prices, unlimited. Fail here is fatal
   to the feature.
2. **Both lists are scrambled and coherent** — real names, real icons, buying
   works and delivers what was shown.
3. **The currencies crossed** — armour or items that are normally echo purchases
   appear in the Insight stock, or the reverse.
4. **Which shops exist** — record what is reachable, to settle spec F10's
   assumption behind D4.
5. **Off is off** — with both settings off the shops are vanilla.

**One run with everything on, per D2** — the armour-only run is deliberately not
taken. That run was the only observation that would catch the two labels being
wired to each other's flag, so that property rests on `worlds_verify.py`'s
field-for-field mapping assertion instead. An implementer must not treat the
mapping case as optional for that reason.

Save the output tree and `live.log` of two runs into `data/runs/`: one with both
settings on, and one with both off, so `verify` can be run against each.

---

## 7. Milestones and stop conditions

### Milestone 1 — the shuffle

**Goal.** A run can deal the armour and consumable shop listings back out at
random, reachable through `EnemyRandomizerOptions`, with row 5's output
untouched.

**Changes**, in order:

| # | Change | Done when |
| - | ------ | --------- |
| 1 | Carve `ShopLineup.h` out of `StartingWeapons.cpp` per §4.1 | `StartingWeapons.cpp` declares none of the moved constants, calls the shared predicate, and the build is clean |
| 2 | Write `ShopStock.{h,cpp}` per §4.2 | both buckets permute, the empty-pool guard is present, and no reroll loop exists anywhere in the file |
| 3 | Add the two options, `AnyParamFeature()` terms and two result counters to `EnemyRandomizer.h` | a run with both off reaches `StepItemData` only when another param feature is on |
| 4 | Call the pass in `StepItemData` at §4.3's position | the call sits after row 5's block and before the hunter-tools block, and locates `ShopLineupParam.param` with its own `Fail` message |

**Invariants**: row 5's output and roll order; one copy of the skip list; only
`equipId` written; bounded work; off draws nothing (§3.1).

**Verification:** `make`; `starting_weapons_verify.py selftest`;
`itemdata_verify.py roundtrip`.

**On completion.** The `.pkg` builds and the checks pass. **Continue to M2.**

### Milestone 2 — the two settings a player can set

**Goal.** A player can turn each setting on independently on both screens, the
choice persists, an activation honours it, and the progress log reports it.

**Changes**, in order:

| # | Change | Done when |
| - | ------ | --------- |
| 1 | The two `RandomizerDefaults` fields | a fresh struct reads both off |
| 2 | The two keys in both halves of `RandomizerDefaultsStore.cpp` | a config without them loads with both off, and a saved file round-trips both |
| 3 | The two `SettingId` values and the two `kSettings` entries with §4.4's labels and help | both rows appear in `WEAPONS & STARTING GEAR` after `RANDOMIZE SHOP WEAPONS`, reading `NO` |
| 4 | `docs/features/randomizer-settings-ui/spec.md` §7.1 and `PROSE_TO_LABEL` | `settings_ui_verify.py` cases 1 and 3 pass |
| 5 | `WorldActivation.cpp`'s mapping and `anythingOn` terms, and `worlds_verify.py`'s two pinned lists | `worlds_verify.py selftest` passes with the fields in the pinned order |
| 6 | `WorldEditorScreen.cpp`'s two `SKIPPING` lines, two report lines and the item-data condition | a run with one setting on prints one report line and one `SKIPPING` line |
| 7 | `pool_verify.py`'s 775 / 826 arithmetic and the two key cases | `pool_verify.py selftest` passes with no unrelated case changing state |

**Invariants**: absent key reads as off; no screen file learns a param offset;
no UI file touches a raw AFR path (§3.1).

**Verification:** `make clean && make`; `pool_verify.py selftest`;
`settings_ui_verify.py`; `worlds_verify.py selftest`;
`starting_weapons_verify.py selftest` still passing.

**On completion.** **Continue to M3.**

### Milestone 3 — the verification this feature can be judged by

**Goal.** The tool that owns this table judges an armour or consumable listing
correctly, a combined run with row 5 can be verified at all, and the two
neighbouring tools can still say something about a tree this feature has
touched.

**Changes**, in order:

| # | Change | Done when |
| - | ------ | --------- |
| 1 | The F13 correction and the re-scoping of SW-I3/I4/I5 and the churn counter (§4.5 items 1–2) | every existing selftest case passes unchanged, and a changed armour listing no longer reports SW-I4 |
| 2 | SW-I10–SW-I14 and the `--shop-armour` / `--shop-items` flags | the docstring's invariant list names all five, and `verify` accepts the flags in any combination with `--granted` |
| 3 | Selftest cases S1–S11 | all pass, and the tool still reports the weapon-side counts it always did |
| 4 | A `--shop-stock` tolerance flag on `drops_verify.py` and `hunter_tools_verify.py` (D3) | each tool passes on an all-on tree **with** the flag, still fails on one without it, and every existing case of both is unchanged either way |
| 5 | Extend `pool` to print both new bucket sizes | it prints 220 armour / 44 distinct and 397 items / 57 distinct beside the weapon pool |

**Invariants**: the tool keeps its filename and its independence from the C++
(it parses the reference lists, not `StartingWeaponLists.h`); the two new param
members stay untolerated by SW-I1 (§3.1, §4.5).

**Verification:** `make` (unchanged sources, so a no-op build);
`starting_weapons_verify.py selftest`; `drops_verify.py selftest` and
`hunter_tools_verify.py selftest`, both of which must pass with **no existing
case changing state** — the flag adds tolerance, it does not relax anything that
already holds; every other §6 automated check still passing.

**On completion.** Hand off the §6 hardware test.

### Stop conditions

Halt and report rather than deciding, if any of these occur:

* the build fails in a way the plan did not anticipate;
* a verification check fails and the cause is not an obvious implementation
  slip;
* removing the moved constants from `StartingWeapons.cpp` would change any
  value it uses;
* the measured bucket sizes disagree with §2 — 644 / 220 / 424 listings, 27
  skipped, 220 and 397 shuffled;
* an implementation decision would contradict the spec, §2 or §3.1;
* the change needs a file not listed in §5;
* a §3.1 invariant cannot be preserved.

---

## 8. Open questions

Empty. Its two questions were answered by the developer on 2026-09-28 and are
recorded as D1 and D2 in §9; `log.md` holds how they were put and answered.

---

## 9. Decisions

The developer's answers to §8, both taken 2026-09-28:

| Ref | Date | Decision | Source |
| --- | ---- | -------- | ------ |
| D1 | 2026-09-28 | **The two new passes run after row 5's pass and before the two character-creation grants**, as §4.3 states. Row 5 is shipped and hardware-proven and its output stays fixed; the grants stay the run's last rolls, which is the property spec 037 §7 recorded. The accepted cost is that turning either new setting on changes which weapon a grant draws for a fixed seed — unavoidable in shape, since one shared stream means whichever pass is later moves when an earlier one is toggled | developer |
| D2 | 2026-09-28 | **One hardware run, everything on** — *not* the plan's recommendation of two. The armour-only run is not taken, so the check that a crossed pair of labels would fail is not performed on the console. The label-to-bucket wiring rests instead on `worlds_verify.py`'s field-for-field mapping assertion, which is real evidence of a different kind. §6's hardware list is written to one run accordingly | developer |
| D3 | 2026-09-28 | **`drops_verify.py` and `hunter_tools_verify.py` gain a `--shop-stock` tolerance flag**, folded into M3 as change 4. It had been out of scope as a pre-existing convention, on the reasoning that §6 would run each tool against a tree where only its own member changed. **D2 removes that mitigation**: with a single all-on run there is no such tree, so without the flag those two tools would produce no signal at all rather than a noisy one. The flag is the same pattern M3 already adds to `starting_weapons_verify.py` and that row 38 added to three tools | developer |

| # | Date | Decision | Taken by |
| - | ---- | -------- | -------- |
| P1 | 2026-09-28 | A new file `ShopStock.{h,cpp}` beside `StartingWeapons`, not an extension of it, with the shop constants and the skip list carved into a shared `ShopLineup.h` | planner |
| P2 | 2026-09-28 | Selection sampling (draw an index, write, erase) per bucket, reproducing the reference's `Next` + `RemoveAt` rather than the Fisher-Yates row 5 uses. Distributionally identical; this one traces line for line to the reference | planner |
| P3 | 2026-09-28 | Two sequential per-bucket loops, armour then items, rather than the reference's single interleaved loop over rows. The two buckets' joint distribution is the same either way | planner |
| P4 | 2026-09-28 | Config keys `randomize_shop_armour` and `randomize_shop_items`; settings block 728 → 775, worst case 779 → 826 | planner |
| P5 | 2026-09-28 | Verification extends `app/tools/starting_weapons_verify.py` under its existing filename, with declaration flags rather than inference | planner |
| P6 | 2026-09-28 | The progress log reports each bucket's changed-listing count, matching `RANDOMIZE SHOP WEAPONS`, rather than a state line | planner |

---

## 10. Changes during implementation

Keep empty until implementation begins.

| Date | Change | Reason |
| ---- | ------ | ------ |
| | | |
