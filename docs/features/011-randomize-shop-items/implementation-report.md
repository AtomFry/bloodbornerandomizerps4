# Implementation Report 011 — Randomize Shop Armour and Shop Items — milestones 1, 2 and 3

**Status: AWAITING HARDWARE TEST**

**Plan:** `docs/features/011-randomize-shop-items/plan.md` — all three
milestones, executed continuously per the plan's Execution Strategy (3
milestones, continuous, final gate only).

**Spec:** `docs/features/011-randomize-shop-items/spec.md` — APPROVED

**Implemented:** 2026-09-28

---

## 1. What was built

Two independent settings, `RANDOMIZE SHOP ARMOUR` and `RANDOMIZE SHOP ITEMS`,
which deal the game's shop stock back out within its own kind: 44 pieces of
armour over 220 `equipType == 1` listings, and 57 consumables and materials over
397 shufflable `equipType == 3` listings. Only `equipId` is written, so every
listing keeps its price, currency, purchase limit and shop. The three
never-touched ids (1000 Blood Vials, 900 Quicksilver Bullets, 240) are neither
sources nor destinations, and the 644 `equipType == 0` weapon listings — backlog
row 5's — are never written by this feature. The shop field offsets and the skip
list now exist once, in a shared header, and the tool that owns the shop table
has been corrected so it can judge an armour or consumable listing at all.

The measured table agrees with §2 exactly, checked before any edit was made:
1288 listings split 644 / 220 / 424, 27 skipped by the never-touch list, 220 and
397 shufflable, 44 and 57 distinct ids. No stop condition fired.

### Milestone 1 — the shuffle

| # | Planned change | Status | Where |
| - | -------------- | ------ | ----- |
| 1 | Carve `ShopLineup.h` out of `StartingWeapons.cpp` per §4.1 | done | `app/src/Randomizer/ShopLineup.h` (new, 53 lines); `StartingWeapons.cpp:7,18-21,104,171` |
| 2 | Write `ShopStock.{h,cpp}` per §4.2 | done | `app/src/Randomizer/ShopStock.h` (new), `ShopStock.cpp` (new) |
| 3 | Two options, `AnyParamFeature()` terms, two result counters | done | `EnemyRandomizer.h` — options after `randomizeShopWeapons`, counters after `shopWeaponsChanged` |
| 4 | Call the pass in `StepItemData` at §4.3's position | done, one position detail — see §2 | `EnemyRandomizer.cpp`, in `StepItemData` step 1 |

`StartingWeapons.cpp` declares none of the moved constants and no local
`IsNeverTouched`; both of its call sites now call the shared
`IsNeverTouchedEquipId`. No value changed in the move, so row 5 writes exactly
the bytes it wrote before. `ShopStock.cpp` contains no reroll loop of any kind:
the only loop bound by anything other than a container size is `pool.empty()`,
which is the guard against a `(0, -1)` distribution.

### Milestone 2 — the two settings a player can set

| # | Planned change | Status | Where |
| - | -------------- | ------ | ----- |
| 1 | The two `RandomizerDefaults` fields | done | `RandomizerDefaults.h`, after `randomizeShopWeapons`, with the standing absent-key note |
| 2 | The two keys in both halves of `RandomizerDefaultsStore.cpp` | done | `FormatSettings` and `ApplySettingKey` |
| 3 | Two `SettingId` values and two `kSettings` entries | done | `SettingsModel.h:97-98`, `SettingsModel.cpp` after `RandomizeShopWeapons` |
| 4 | Settings-UI spec §7.1 and `PROSE_TO_LABEL` | done | `docs/features/randomizer-settings-ui/spec.md` §7.1; `settings_ui_verify.py` |
| 5 | `WorldActivation.cpp` mapping and `anythingOn`, plus `worlds_verify.py`'s pinned lists | done | `WorldActivation.cpp`; `worlds_verify.py` `WIZARD_OPTIONS_MAPPING`, `WIZARD_RUN_DECISION`, the `SKIPPING` field list |
| 6 | `WorldEditorScreen.cpp` — two `SKIPPING` lines, two report lines, the item-data condition | done | `WorldEditorScreen.cpp` |
| 7 | `pool_verify.py`'s byte arithmetic and the two key cases | done, rebased — see §2 | `pool_verify.py` — 790 / 841, plus one written-and-read case per key |

The labels are `RANDOMIZE SHOP ARMOUR` and `RANDOMIZE SHOP ITEMS`, the help text
is §4.4's verbatim, and `Shop Items` was removed from the spec's `Items &
Treasure` backlog cell. `settings_ui_verify.py` measures the widest pane row as
649 px of 700 (`START WITH A TRICK WEAPON`), so both new rows fit with room to
spare, and `WEAPONS & STARTING GEAR` now reports eight settings in the spec's
order. Neither settings screen needed a change.

### Milestone 3 — the verification this feature can be judged by

| # | Planned change | Status | Where |
| - | -------------- | ------ | ----- |
| 1 | The F13 correction and the re-scoping of SW-I3/I4/I5 and the churn counter | done | `starting_weapons_verify.py` `compare()` |
| 2 | SW-I10–SW-I14 and the `--shop-armour` / `--shop-items` flags | done | `starting_weapons_verify.py` docstring, `compare()`, `cmd_verify()`, `main()` |
| 3 | Selftest cases S1–S11 | done, S10 built differently — see §2 | `starting_weapons_verify.py` `cmd_selftest()` |
| 4 | A `--shop-stock` tolerance flag on `drops_verify.py` and `hunter_tools_verify.py` (D3) | done | both tools' docstring, `compare()`, `cmd_verify()`, `main()`, `cmd_selftest()` |
| 5 | Extend `pool` to print both new bucket sizes | done | `starting_weapons_verify.py` `cmd_pool()` |

Every per-listing check now dispatches on the listing's **vanilla** `equipType`
byte: type 0 against `EquipParamWeapon` (SW-I4, unchanged), type 1 against
`EquipParamProtector`, type 3 against `EquipParamGoods` (SW-I10). Both new
members are read-only and remain **untolerated** by SW-I1 — a single changed byte
in either is a failure, and two selftest cases say so. SW-I3, SW-I5 and the
`counts["shop"]` churn figure are inside the `equipType == 0` branch; `stock()`
and SW-I8 were already type-filtered and did not move. The two new bucket counts
are printed beside row 5's.

`pool` output against the vanilla tree:

```
armour bucket: 220 listings, 220 shufflable, 44 distinct   (0 skipped by the never-touch list)
items bucket: 424 listings, 397 shufflable, 57 distinct   (27 skipped by the never-touch list)
```

---

## 2. Deviations from the plan

All five are also recorded in the plan's §10.

**2.1 The config byte arithmetic is rebased to 790 / 841, not 775 / 826.**
The plan's figures were computed before backlog row 27 was implemented and
committed, which added the key `no_team_type` — 15 bytes with its `=1` and
newline. `pool_verify.py selftest` reported a **743**-byte settings block and a
**794**-byte worst-case `defaults.cfg` before this feature's first edit; this
plan's two keys add 47 (24 for `randomize_shop_armour`, 23 for
`randomize_shop_items`), giving **790** and **841**. Those are the figures the
exact-equality assertions now carry, and they are derived from the measured
baseline plus the plan's own arithmetic rather than from whatever the code
produces. 234 bytes of `char buf[1024]` remain spare. The narrating comment was
extended in the same pass, as the hazard row asks.

**2.2 The pass is called immediately after row 5's block, which is now before
the `noTeamType` block.** §4.3 says "immediately after row 5's
`RandomizeStartingWeapons` block and before the `startWithHunterTools` block";
row 27 inserted its own pass between those two points. The call sits directly
after row 5's block, so both halves of §4.3's reasoning hold literally — every
draw row 5 makes precedes every draw this feature makes, and the two
character-creation grants stay the run's last rolls. `ApplyOneTeamType` draws no
randomness and writes byte 303 of `NpcParam`, which nothing here reads, so the
two commute and the choice is not observable.

**2.3 Selftest S10 is built out of genuine permutations.** The plan's wording —
"a permuted archive with one listing per bucket reverted", "200 of 220 armour
listings reverted" — breaks SW-I11 if taken literally: overwriting one listing
with its vanilla id duplicates that id and loses another, so the bucket stops
being a permutation and `compare()` correctly rejects it for the wrong reason.
The first half now *swaps* the two listings involved, so the target holds its
vanilla item and the bucket is still a permutation. The second half permutes only
the last 20 of the 220 armour listings and leaves the other 200 at vanilla — a
genuine permutation with 20 listings changed, so SW-I11 holds and SW-I14 is the
only invariant that can catch it. That is the property S10 exists to pin.

**2.4 SW-I14's declared half reads "untouched, or wholesale-changed".**
§6 S1 requires an unmodified archive to pass with both flags, while §6 S10
requires a mostly-vanilla partial permutation to be rejected with both flags. A
bound of "a declared bucket must have changed" fails S1; no bound at all fails
S10. The implemented rule is: an **undeclared** bucket must show zero changed
listings, and a **declared** bucket must show either zero or more than half
(`DECLARED_CHURN_FRACTION = 0.5`). About 5 of 220 and 9 of 397 listings are
expected to draw the item they already had, so the bound is nowhere near the real
distribution.

**2.5 A never-touched id written into any listing is reported as SW-I13, not
SW-I2.** §6 S7 assigns the skip list to SW-I13 for any listing, and the old
SW-I2-labelled check covered the same rule for weapon rows. Keeping both would
have been two checks for one rule — the drift §3.1 forbids for the list itself.
One check now covers every bucket in both directions: a protected listing that
changed, and a protected id written anywhere. No existing selftest case asserted
the old label.

No other departure. Every file the change needed is in §5; nothing outside it was
touched.

---

## 3. Decisions the plan left open

### 3.1 A changed listing of an unexpected `equipType`

`compare()` dispatches on `equipType` 0, 1 and 3. The real archive contains no
other value, but a fourth value is representable, so a changed listing whose
vanilla `equipType` is none of the three is reported as an SW-I12 failure naming
the type. The alternative was to ignore it, which would have left a silent hole
in "the buckets never cross". It cannot fire on real data.

### 3.2 `--shop-stock` tolerates its member wholesale

`drops_verify.py`'s `--team-type` *asserts* the tolerated value (every row must
hold 25). `--shop-stock` does not assert anything about `ShopLineupParam`: it
removes that one member from the "must be byte-identical" set and nothing more.
`starting_weapons_verify.py` owns that table's positive invariants and would
duplicate them badly here, and the plan's §5 wording is "with it,
`ShopLineupParam` differing is not a failure". Both tools' docstrings say so
explicitly, and both have a case showing the flag is not an amnesty for a third
member or for their own feature's rules.

### 3.3 Four selftest cases beyond the plan's list

`--shop-items` alone does not cover an armour-only permutation (the
counterpart of S2 that catches a crossed pair of flags in the tool itself); the
vanilla tree really does contain at least two Blood Vial listings to protect (a
precondition for S7, which would otherwise pass vacuously); and row 5's melee
rule still fails on the combined all-features archive (the second half of S11).
All are additions, none replaces a listed case.

### 3.4 The bucket order inside `RandomizeShopStock`

§4.2 fixes armour first, then items, and the implementation does that with two
sequential loops (§9 P3). The order is stated in a comment as the reason a seed
plus a *set* of settings reproduces a run, rather than being left as incidental
control flow.

---

## 4. Verification run

Every command below was run from the repository root with the vanilla tree at
`data/vanilla/dvdroot_ps4`. The "before" figures are the same commands run on the
clean tree at `52534a4`, so a case that changed state would be visible.

| Check | Command | Result |
| ----- | ------- | ------ |
| Build, after M1 | `cd app && make` | `.pkg` built (7,143,424 bytes), no errors |
| Build, after M2 | `cd app && make clean && make` | `.pkg` built, no errors — the clean rebuild `RandomizerDefaults` growing requires |
| Build, after M3 | `cd app && make` | `make: Nothing to be done for 'all'` — M3 touched no C++, as §7 expects |
| Row 5 unaffected + the mirror | `python app/tools/starting_weapons_verify.py selftest data/vanilla/dvdroot_ps4` | **45/45** (was 21/21; all 21 existing cases pass unchanged, 24 added) |
| Table listing | `python app/tools/starting_weapons_verify.py pool data/vanilla/dvdroot_ps4` | 220 armour / 44 distinct, 397 shufflable items / 57 distinct, 27 skipped, beside 644 weapon rows / 95 distinct |
| Config | `python app/tools/pool_verify.py selftest data/vanilla/dvdroot_ps4` | **98/98** (was 96/96); `worst-case defaults.cfg is 841 bytes` and `the 790-byte settings block fits char buf[1024]` both pass |
| Settings model | `python app/tools/settings_ui_verify.py` | all passing, exit 0; widest pane row 649/700 px, widest rail row 552/580, worst help body 9/11 lines; `WeaponsGear` now 8 settings in spec order |
| Worlds parity | `python app/tools/worlds_verify.py selftest` | **29/29**, all checks passing — the options mapping and the run decision carry both fields in the pinned position, and the recipe round-trip covers both keys |
| Drops tolerance | `python app/tools/drops_verify.py selftest data/vanilla/dvdroot_ps4` | **16/16** (was 12/12; no existing case changed state) |
| Hunter tools tolerance | `python app/tools/hunter_tools_verify.py selftest data/vanilla/dvdroot_ps4` | **29/29** (was 25/25; no existing case changed state) |
| Archive round trip | `python app/tools/itemdata_verify.py roundtrip data/vanilla/dvdroot_ps4` | PASS — byte-identical at 28,309,992 bytes, still 65 entries |
| Neighbouring tools, unaffected | `team_type_verify`, `trick_weapons_verify`, `left_hand_weapons_verify`, `treasure_verify`, `easy_modes_verify`, `caged_dogs_verify`, `mergo_darkness_verify`, `font_atlas_verify`, `ui_scroll_verify` | all pass |

Nothing failed at any point, so nothing is pasted here.

**Not run:** `starting_weapons_verify.py verify <vanilla> <output>`,
`drops_verify.py verify` and `hunter_tools_verify.py verify`. All three need an
output tree from a real run, which only the console can produce. The declaration
flags they take were exercised against Python-simulated output in the selftests
instead, which is what the selftests are for — but the real-tree assertions in
§6 ("the member list and every member size unchanged; only
`ShopLineupParam.param` differs unless another declared feature owns the member;
the row id set unchanged") have **not** been run against any real output.

---

## 5. What this does not prove

A clean `.pkg` and green mirrors mean **ready for hardware testing** and nothing
stronger (`CLAUDE.md` §3). Specifically:

* **Nothing here has run on a PS4.** There is no host C++ compiler and no
  emulator; the cross-compile proves the code compiles and links, not that the
  pass executes, that the archive the console writes is valid, or that the game
  loads it.
* **The mirror pins the rules, not the C++.** `starting_weapons_verify.py` shows
  that an output tree satisfies the permutation, the skip list, the field and the
  bucket boundaries. It cannot show that the engine drew uniformly, and it
  deliberately does not predict which item a seed puts in a given listing.
  **Determinism is a hardware check** — no automated check here establishes that
  the same seed and settings produce the same shops.
* **Row 5's byte-for-byte output is argued, not measured.** The constants moved
  verbatim, the new pass runs after row 5's has returned, and every existing
  selftest case still passes — but "the same seed gives row 5 the same rolls it
  gave before" has not been observed against a real pair of runs.
* **Whether the game tolerates the result is unknown.** That a Bath Messenger
  listing can now sell a rune at a Bold Hunter's Mark price and limit is the
  intended behaviour (spec D3), but only the console shows whether the shop UI,
  the currency and the purchase actually work that way.
* **Which shops are reachable is still assumed.** Spec F10's assumption behind
  D4 — that the unidentified shop families are reachable ordinary stock — is a
  hardware observation, item 4 of the handoff below.

---

## 6. Hardware test handoff

**One run with everything on, per D2.** The armour-only run is deliberately not
taken; the property it would have checked — that the two labels are not wired to
each other's flag — rests on `worlds_verify.py`'s field-for-field mapping
assertion, which passes.

Save the output tree and `live.log` of **two** runs into `data/runs/`: one with
both settings on, and one with both off, so `verify` can be run against each.

1. **Enable.** Build a world with `RANDOMIZE SHOP ARMOUR` and `RANDOMIZE SHOP
   ITEMS` both on, in `WEAPONS & STARTING GEAR`. Turning row 5's three settings
   and the other param features on as well is fine and is the more useful run —
   the three verifiers can all be pointed at it now.
2. **Check the progress log.** Two lines should read `RANDOMIZED <n> SHOP ARMOUR
   LISTINGS` and `RANDOMIZED <n> SHOP ITEM LISTINGS`, with `<n>` a little under
   220 and a little under 397 respectively — about 215 and about 388. A 0, or a
   number far from those, means the `equipType` filter or the row walk is wrong.
   With a setting off, the matching `SKIPPING SHOP ARMOUR RANDOMIZATION` /
   `SKIPPING SHOP ITEM RANDOMIZATION` line should appear instead.
3. **The vials are there.** First visit to the Hunter's Dream: **Blood Vials and
   Quicksilver Bullets on sale, at vanilla prices, unlimited.** A failure here is
   fatal to the feature — stop and report it rather than continuing.
4. **Both lists are scrambled and coherent.** Real names, real icons; buying
   works and delivers what was shown.
5. **The currencies crossed.** Armour or items that are normally echo purchases
   appear in the Insight stock, or the reverse. This is the observation that
   shows the pool is keyed by `equipType` alone (D2 in the spec).
6. **Record which shops exist** — what stock is actually reachable, to settle
   spec F10's assumption behind D4.
7. **Off is off.** With both settings off, the shops are vanilla.

Then, against the saved trees:

```
python app/tools/starting_weapons_verify.py verify data/vanilla/dvdroot_ps4 <on-tree> \
    --shop-armour --shop-items [--granted <id>]...
python app/tools/starting_weapons_verify.py verify data/vanilla/dvdroot_ps4 <off-tree>
python app/tools/drops_verify.py        verify data/vanilla/dvdroot_ps4 <on-tree> --shop-stock [--team-type]
python app/tools/hunter_tools_verify.py verify data/vanilla/dvdroot_ps4 <on-tree> --shop-stock [--granted <id>]
```

The off-tree run takes **no** flags on purpose: that is the check that an off run
leaves the shop table byte-identical to vanilla (SW-I14).

**What a failure looks like.** A shop with no Blood Vials, or a listing whose
price or purchase limit changed, or an item with no name or no icon, or
`SW-I11`/`SW-I12` from `verify` — each points at a different half of the pass and
each is a stop-and-report, not a tune-and-retry.

---

## 7. Stop point

All three milestones are complete. No stop condition fired: the measured bucket
sizes agreed with §2, removing the moved constants changed no value
`StartingWeapons.cpp` uses, no check failed, and no file outside §5 was needed.

The work order ended at M3's completion gate — "hand off the §6 hardware test" —
so the next step is the developer's, not another milestone. The working tree is
left uncommitted and unbranched for the developer (`CLAUDE.md` §9).
