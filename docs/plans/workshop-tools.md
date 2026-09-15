# Feature 7 — Randomize Workshop Tools

**Spec reference:** `docs/randomization-feature-spec.md` §2, row 7 (`workshopBool`).
**Status: DONE — implemented and hardware-confirmed.** All eight files
in §4 are done as specified, plus the §5.1 verifier changes and both new
selftest cases (8/8 passing against `data/vanilla/dvdroot_ps4`,
measured pool/location counts match §2.3 exactly). One deviation from the
D5 example: the progress line omits the parentheses shown below (`WORKSHOP
TOOLS INCLUDED` with no parens) because the app's 8x8 font has no punctuation
glyphs (`Font8x8.cpp`) and would render them as blank gaps.

The spec calls this one of "the two freebies" — the engine work is already done
and only a UI row and a saved flag are missing. That is still true. But the
investigation below turned up three things the spec line does not say, and one
of them is a real (small) piece of work, so this is not quite a twenty-line
change.

---

## 1. What this setting does, in plain terms

Bloodborne has two permanent upgrade unlocks, both one-time key items:

| Item | What it unlocks |
|---|---|
| **Blood Gem Workshop Tool** | Fitting and removing blood gems on your weapons at the Hunter's Dream workshop |
| **Rune Workshop Tool** | Equipping Caryll runes |

Neither is needed to finish the game, but between them they gate essentially the
whole character-customisation half of Bloodborne. Without the Blood Gem tool you
cannot put a single gem in a weapon; without the Rune tool you cannot wear a
single rune.

**Today, with `RANDOMIZE TREASURE` on, these two items do not move.** They are on
the randomizer's protected list, sitting in their vanilla chests while everything
else around them gets shuffled. The reference tool behaves the same way, and has
the same opt-out checkbox we are adding here.

**Turning this setting on puts them into the shuffle with everything else.**

### What the player actually experiences

`RANDOMIZE TREASURE` is a *permutation*, not a re-roll: it takes the full set of
world pickups and deals them back out into the same set of chests and corpses.
So switching this on does not delete the workshop tools or invent new ones. It
means:

- The chest in Central Yharnam that normally holds the Blood Gem Workshop Tool
  now holds something else — a Blood Vial, a weapon, anything in the pool.
- The two tools turn up somewhere else entirely, in chests that could be
  anywhere from the Old Workshop to the Nightmare.
- You might get runes before you get your first gem. You might get the Blood Gem
  tool in your first twenty minutes, or not until Yahar'gul.

**It cannot make a run unwinnable.** Both tools are optional systems, not
progression keys — the real key items (Cainhurst Summons, the Tomb of Oedon
key, and so on) stay protected either way, and this setting does not touch them.
The worst case is a run where you never find one of the two tools and simply
play without gems or without runes.

### Who should turn it on

- **Leave it off (the default)** if you want the run to feel like Bloodborne —
  upgrades unlock at roughly the usual point, and the tools are where you look
  for them.
- **Turn it on** for a stricter shuffle, where the upgrade systems unlock on the
  randomizer's schedule rather than the game's. It makes the early game
  noticeably swingier in both directions.

---

## 2. What the real data says

Everything in this section was measured against the vanilla tree in
`data/vanilla/dvdroot_ps4`, not taken from the reference's
comments.

### 2.1 It is two item lots but **four placements**

The spec says "two workshop item lots". That is right about the lots and
misleading about the work: the Blood Gem tool's chest exists in three map
variants.

| Lot | Map | Map name | Event | Entity |
|---|---|---|---|---|
| 2200360 | `m22_00_00_00` | Hemwick Charnel Lane | 199 | 314 |
| 2411000 | `m24_01_00_00` | Central Yharnam (pre-DLC) | 376 | 445 |
| 2411000 | `m24_01_00_01` | Central Yharnam | 374 | 445 |
| 2411000 | `m24_01_00_11` | Central Yharnam (unused) | 374 | 445 |

The three `m24_01` rows are the same chest (same entity ID 445) in the pre-DLC,
live, and unused variants of Central Yharnam. Only the live one is what a player
with the DLC installed actually opens.

Two consequences:

- **The Blood Gem tool enters the pool three times**, so with the setting on it
  is likely to appear in up to three different places in the world. Harmless —
  it is a one-time unlock, so duplicates are inert.
- **Its vanilla chest gets three independent rolls**, one per variant. Again
  mostly inert, because the player only ever opens the live variant.

This variant duplication is not special to this feature: 341 of the 647 distinct
eligible lots appear in more than one map. It is a pre-existing property of the
treasure randomizer and this plan does not change it.

### 2.2 The reference author's comments are correct — worth recording

`TreasureRandomizer.cpp` carries the reference's annotations with an explicit
warning that they are "unverified labels, not established fact", and
`boss-and-treasure-findings.md` says the same. For these two, they check
out:

- `2411000` — "chest after gascoigne". The chest is in Central Yharnam, and the
  MSB event is named `Item_宝箱_1000_魔石着脱解禁` — "treasure chest, blood gem
  attach/detach unlock". Confirmed.
- `2200360` — "after witches". The chest is in Hemwick Charnel Lane, home of the
  Witch of Hemwick. Confirmed.

Both lots resolve through `ItemLotParam` to category-4 (goods) items 4103 and
4104 respectively, one each; `Blood Gem Workshop Tool` and `Rune Workshop Tool`
are both present in the English item-name table. The identification is solid.

I am **not** proposing to drop the unverified-label warning in general — it still
applies to the other nine protected lots. Just these two can be stated as fact.

### 2.3 Measured pool and location counts

| | Setting OFF (today) | Setting ON |
|---|---|---|
| Eligible locations | 1041 | **1045** |
| Pool entries | 1044 | **1048** |
| Unplaced (Old Workshop double-pass) | 3 | 3 |

Pool accounting still balances exactly (`1048 = 1045 + 3`), which is the property
the verifier checks.

### 2.4 One quantified risk

`TreasureMapOrder()` lists `m21_01_00_00` (Abandoned Old Workshop — no relation
to the workshop tools, just an unlucky name collision) twice, so its 3 slots are
assigned twice and the first three lots drawn are overwritten and lost. That is
existing, deliberate reference-matching behaviour.

Applied to this feature: the **Rune Workshop Tool has a single pool copy**, so it
has a ~3/1048 ≈ **0.3% chance of being discarded** and never appearing in the
world at all. The Blood Gem tool has three copies, so its chance of vanishing
entirely is negligible.

A 0.3% chance of losing runes for a run is, in my view, acceptable — it is the
same exposure every ordinary item already carries, and the outcome is a missing
optional system, not a dead save. **I am not proposing to fix it here.** Worth
stating in this document so it is a known property rather than a future
mystery bug report.

---

## 3. Design decisions

**D1 — Name and placement.** `RANDOMIZE WORKSHOP TOOLS`, inserted directly below
`RANDOMIZE TREASURE` in both the Enable wizard and Setup Defaults, so the
dependency reads off the list order. This matches the spec's own name and the
existing `RANDOMIZE ...` row convention.

**D2 — Default is NO.** Matches the reference, matches every other randomizer
field's "a defaults.cfg written before this key existed must not silently turn
something on" rule, and keeps the conservative behaviour as the default.

**D3 — It does not gate a run on its own.** The setting is meaningless unless
`RANDOMIZE TREASURE` is on. The wizard's commit condition (the big `||` in
`StartCommit`) must **not** gain `randomizeWorkshopTools_` — otherwise ticking it
alone would start a run that does nothing.

**D4 — No disabled/greyed row.** The UI has no concept of a disabled row today
and building one for this is out of proportion. If treasure randomization is off,
this row is simply inert. Its position under `RANDOMIZE TREASURE` carries the
hint.

**D5 — Report it in the progress log, not as a SKIPPING line.** A
`SKIPPING WORKSHOP TOOLS` line would appear even when treasure randomization was
never going to run, which is noise. Instead, when treasure randomization does
run, append the state to the existing treasure result line:

```
RANDOMIZED 1045 TREASURE PICKUPS (WORKSHOP TOOLS INCLUDED)
```

---

## 4. Implementation

Eight files. No new engine logic — `IsProtectedItemLot`, `CollectTreasureLots`
and `AssignTreasureLots` already take and honour the flag.

### 4.1 Settings chain (follows `randomizeTreasure` exactly)

| File | Change |
|---|---|
| `src/Randomizer/RandomizerDefaults.h` | Add `bool randomizeWorkshopTools = false;` under `randomizeTreasure`, with the usual comment about why `false` is the struct's own default |
| `src/Randomizer/RandomizerDefaultsStore.cpp` | Read `randomize_workshop_tools` in the key loop; add it to the `snprintf` format and argument list |

### 4.2 Setup Defaults screen

| File | Change |
|---|---|
| `src/UI/SetupDefaultsScreen.h` | `kItemCount` 10 → 11; add `kRandomizeWorkshopToolsRow = 6`; **renumber** `kRandomizeDropsRow` 6→7, `kStartingWeaponsRow` 7→8, `kStartingGunsRow` 8→9, `kShopWeaponsRow` 9→10 |
| `src/UI/SetupDefaultsScreen.cpp` | One `else if` branch in `ToggleRow`; one entry in the `DrawList` items vector, positioned to match |

### 4.3 Enable wizard

| File | Change |
|---|---|
| `src/UI/EnableWizardScreen.h` | Add `bool randomizeWorkshopTools_;` next to `randomizeTreasure_` |
| `src/UI/EnableWizardScreen.cpp` | Add `kRandomizeWorkshopToolsRow = 6` and renumber rows 6-9 → 7-10; `kSaveDataRowCount` 10 → 11; initialise from `defaults.randomizeWorkshopTools` in the ctor init list; add the left/right branch and the `input.cross` branch in `UpdateSaveData`; add `options.randomizeWorkshopTools = randomizeWorkshopTools_;` in `StartCommit`; extend the treasure line in `FinishCommit` per D5; add the row to both the `DrawSaveData` and `DrawConfirm` item vectors |

`EnemyRandomizer.h` already declares `randomizeWorkshopTools` and both call sites
already pass it. **No change to `EnemyRandomizer.h/.cpp` or
`TreasureRandomizer.h/.cpp` at all.**

Progress step counting is unaffected — `treasureSteps` is `2 × map count` and
does not depend on pool size.

### 4.4 One piece of hardening worth doing while we are here

`SaveRandomizerDefaults` writes into `char buf[512]` and then does:

```c
int len = snprintf(buf, sizeof(buf), ...);
if (len <= 0) return;
sceKernelWrite(fd, buf, (size_t)len);
```

`snprintf` returns the length it *would* have written. If the format ever
exceeds 512 bytes, `len` exceeds the buffer and `sceKernelWrite` reads past the
end of it.

Current worst case is **268 bytes**; this feature's key adds **27**, for 295 —
217 bytes of headroom, so nothing is at risk today. But we are adding the eighth
setting to a list that the spec says has ~24 more coming, and the failure mode is
silent. Recommend clamping at the same time:

```c
if (len < 0) return;
if ((size_t)len > sizeof(buf)) len = (int)sizeof(buf);
```

Two lines, unrelated to this feature's behaviour, and it stops a future setting
from turning into a memory bug.

---

## 5. Verification

### 5.1 The verifier currently cannot check a workshop-tools-on run

`tools/treasure_verify.py` already threads `randomize_workshop_tools` through
`protected()`, `build_pool()` and `eligible_locations()` — but **`cmd_verify` and
`cmd_pool` never pass it**, so both run with it hardcoded off.

Verify a run made with the setting on and the tool reports false failures: every
moved workshop lot trips `I5 protected lot ... placed into a randomized slot`,
and the `I4` pool accounting comes out short. This is the one genuine piece of
work in the feature, and it has to land with it — otherwise the first hardware
test looks like a regression.

**Changes to `tools/treasure_verify.py`:**

1. Accept a trailing `--workshop` flag on `pool` and `verify`, threaded into
   `build_pool` / `eligible_locations` / `compare`.
2. Replace `EXPECTED_ELIGIBLE = 1041` with a pair keyed on the flag:
   `1041` off, `1045` on (both measured in §2.3).
3. Add two selftest cases:
   - with the flag **on**, moving a workshop lot must **not** be reported;
   - with the flag **off**, moving a workshop lot **must** be reported as `I5`.

   Between them these pin the setting's actual meaning, in both directions —
   which nothing in the suite does today.

### 5.2 Test matrix

| Run | Expect |
|---|---|
| Treasure ON, workshop OFF | Byte-identical to today's output for the same seed. This is the regression guard: a UI renumbering mistake would show up here |
| Treasure ON, workshop ON | `verify --workshop` passes; the 4 placements in §2.1 have changed; `1045` slots placed |
| Treasure OFF, workshop ON | No treasure phase runs at all; treasure output identical to vanilla. Confirms D3 |

### 5.3 Hardware check

Start a run with treasure and workshop tools both on, and confirm the Central
Yharnam chest (entity 445) gives something other than the Blood Gem Workshop
Tool. That is the whole user-visible surface; the rest is the existing,
already-verified treasure path.

---

## 6. Open questions

**Q1 — Should the progress log say where the tools went?** We know the two lot
IDs and could log which slots received them, turning the log into a light spoiler
/ debugging aid. Cheap to add, but it is the first time the randomizer would
report *contents* rather than counts, and it spoils the run for anyone reading
the screen. My recommendation: **no**, keep it to the count in D5.

**Q2 — Is 0.3% acceptable for the Rune Workshop Tool (§2.4)?** My recommendation
is yes, and that we document rather than fix it, since fixing it means deviating
from the reference's double-pass on the Abandoned Old Workshop — which is a
separate decision affecting all 1048 pool entries, not just this one.

**Q3 — Worth updating `TreasureRandomizer.cpp`'s comments?** The two workshop
lots are now verified (§2.2) while the other nine protected lots are not.
Recommendation: annotate just these two as confirmed, leave the blanket warning
in place for the rest.
