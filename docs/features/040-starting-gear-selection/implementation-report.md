# Implementation Report 040 — Starting Gear Selection — milestones 1–4 (complete)

**Status: AWAITING HARDWARE TEST**

**Plan:** `docs/features/040-starting-gear-selection/plan.md` — **APPROVED**. Work
order: resume at M1 step 3 and run to the end of M4, continuous, no intermediate
gates.

**Spec:** `docs/features/040-starting-gear-selection/spec.md`

**Implemented:** 2026-09-29 (this pass); M1 steps 1–2 by an earlier pass on the
same day, M1 step 3 by the orchestrating session.

---

## 1. What was built

All four milestones. The feature is complete and the tree is green: the `.pkg`
builds from clean, all four generated tables `--check` unchanged, and every §6
automated check passes at or above its pre-existing count with no existing case
changing state. A player can now tick pieces in four armour pickers and switch
`START WITH SUPPLIES` on, and a run dresses every new character accordingly and
gives it 20 Blood Vials and 20 Quicksilver Bullets — alongside, not instead of,
whatever the three neighbouring grants wrote.

The one thing in this contract most at risk — §4.4's four-draw mechanism — was
implemented exactly as written: four raw `rng()` words whenever anything is
ticked, one per slot in slot order, a slot with nothing ticked discarding its
word, and `word % k` indexing rather than a `std::uniform_int_distribution`.
`starting_gear_verify.py`'s A15 asserts the resulting independence across 4 slots
× 5 tick counts × 4 neighbour patterns, asserts the draw count is exactly 4 in
every non-empty case and 0 otherwise, and asserts against the compiled source
that no distribution appears in `ArmourGrant.cpp`'s code.

### Milestone 1 — the 124 pieces, derived and pinned

| # | Planned change | Status | Where |
| - | -------------- | ------ | ----- |
| 1 | `fmg.py` gains `armour_names()` / `goods_names()` | **done by the earlier pass** | `app/tools/fmg.py` |
| 2 | `/` in `Font8x8.cpp`'s `kGlyphs` and `gen_pool_table.py`'s `RENDERABLE` | **done by the earlier pass** | `app/src/Platform/Font8x8.cpp:71`, `app/tools/gen_pool_table.py` |
| 3 | `gen_weapon_table.py`'s `LOT_CATEGORY_WEAPON` → 0 | **already done on arrival** — skipped per the work order | `app/tools/gen_weapon_table.py:84` |
| 4 | `gen_armour_table.py` with §4.2's three tests, the two-sided cross-check, the frozen orders | **done** | `app/tools/gen_armour_table.py` (new, 420 lines) |
| 5 | Generate `ArmourTables.h` | **done** | `app/src/Randomizer/ArmourTables.h` (new, generated, 124 rows) |
| 6 | `starting_gear_verify.py` with `list` and `selftest` | **done** | `app/tools/starting_gear_verify.py` (new) |

Derived on the vanilla tree: **33 / 34 / 26 / 31 = 124**, cross-check exact both
ways, one `static_assert` spelling 124.

### Milestone 2 — the five settings a player can set

| # | Planned change | Status | Where |
| - | -------------- | ------ | ----- |
| 1 | Four typedefs, five `RandomizerDefaults` fields | **done** | `EnemyPoolSelection.h`, `RandomizerDefaults.h` |
| 2 | Five keys in both halves of the store | **done** | `RandomizerDefaultsStore.cpp` |
| 3 | Five ids, four kinds, five `kSettings` entries, five `switch` sites | **done** | `SettingsModel.h`, `SettingsModel.cpp` |
| 4 | Four `PickerStrings`, `showRowId = false` | **done** | `ModelPicker.h:186–221` |
| 5 | Settings-UI spec §7.1 and `PROSE_TO_LABEL` | **done** | `docs/features/randomizer-settings-ui/spec.md:321`, `settings_ui_verify.py` |
| 6 | Re-measured `pool_verify.py` arithmetic; `settings_ui_verify.py`'s counts and eleven-selection case; four `ui_scroll_verify.py` entries | **done** | `pool_verify.py`, `settings_ui_verify.py`, `ui_scroll_verify.py` |

Config arithmetic came out at **1279 / 1330** exactly, as the work order
predicted. Both screens needed no change, as §4.3 says.

### Milestone 3 — what the character carries

| # | Planned change | Status | Where |
| - | -------------- | ------ | ----- |
| 1 | `CharaInitRows.h` gains the four clothing offsets; block comment corrected | **done** | `CharaInitRows.h:58–72` |
| 2 | `StartingSupplies.{h,cpp}` per §4.5 | **done** | new, both files |
| 3 | The option, counters, and the call after the hunter-tools block | **done** | `EnemyRandomizer.h`, `EnemyRandomizer.cpp` |
| 4 | `WorldActivation.cpp` mapping + `anythingOn`; `WorldEditorScreen.cpp` progress line and item-data condition | **done** | both |
| 5 | `worlds_verify.py`'s two lists; `hunter_tools_verify.py --supplies`; `pool_verify.py`'s 71-character case | **done, with an added invariant** — §2 D-5 | all three |
| 6 | `starting_gear_verify.py` A9–A13 and the supplies half of `verify` | **done, A12 restated** — §2 D-4 | `starting_gear_verify.py` |

### Milestone 4 — what the character wears

| # | Planned change | Status | Where |
| - | -------------- | ------ | ----- |
| 1 | `ArmourGrant.{h,cpp}` per §4.4 | **done** | new, both files |
| 2 | `ArmourPieceName(slot, id)` exported | **done** | `ArmourGrant.h`/`.cpp` |
| 3 | Four options, `AnyParamFeature()`, result fields, the call **last** in `StepItemData` | **done** | `EnemyRandomizer.h`, `EnemyRandomizer.cpp` |
| 4 | `WorldActivation.cpp` mappings + `anythingOn`; one `WEARING <NAME>` line per granted slot; item-data condition | **done** | both |
| 5 | `--armour h,b,g,l` on the three neighbouring verifiers | **done** | `hunter_tools_verify.py`, `trick_weapons_verify.py`, `left_hand_weapons_verify.py` |
| 6 | `starting_gear_verify.py` A14, A15 and the armour half of `verify` | **done, plus eleven `compare()` cases** — §3.6 | `starting_gear_verify.py` |

### Files changed, one line each

**New**

| File | What |
| ---- | ---- |
| `app/tools/gen_armour_table.py` | derives the four tables, `--check`, both-sided cross-check, frozen orders |
| `app/src/Randomizer/ArmourTables.h` | generated: four tables, four counts, the `static_assert` |
| `app/tools/starting_gear_verify.py` | `list` / `selftest` (A1–A16) / `verify` (G1–G7) |
| `app/src/Randomizer/StartingSupplies.h` | `START WITH SUPPLIES`, the interface and the reasoning |
| `app/src/Randomizer/StartingSupplies.cpp` | the pass: two goods ids at 20, free slots only, draws nothing |
| `app/src/Randomizer/ArmourGrant.h` | the armour pass's interface, and why the four draws are independent |
| `app/src/Randomizer/ArmourGrant.cpp` | four candidate lists, the all-empty early return, four raw draws, one s32 per granted slot |

**Modified**

| File | What |
| ---- | ---- |
| `app/src/Randomizer/EnemyPoolSelection.h` | four typedefs, include `ArmourTables.h` |
| `app/src/Randomizer/RandomizerDefaults.h` | four selections and `startWithSupplies` |
| `app/src/Randomizer/RandomizerDefaultsStore.cpp` | five keys in `FormatSettings` and `ApplySettingKey`; the byte arithmetic comment rebased to 1279 / 1330 |
| `app/src/Randomizer/CharaInitRows.h` | `kEquipClothing[4]`, `kClothingSlots`; block comment now names all six writers |
| `app/src/Randomizer/EnemyRandomizer.h` | five options, `AnyArmourTicked()`, `AnyParamFeature()`, five result fields |
| `app/src/Randomizer/EnemyRandomizer.cpp` | supplies call after the hunter-tools block; armour call **last** in `StepItemData` |
| `app/src/Game/WorldActivation.cpp` | five options mapped in the frozen order; five `anythingOn` terms |
| `app/src/UI/SettingsModel.h` | five `SettingId`s, four `SettingKind`s |
| `app/src/UI/SettingsModel.cpp` | five `kSettings` entries, five `switch` sites |
| `app/src/UI/ModelPicker.h` | four `PickerStrings`; header comment lists the eleven lists |
| `app/src/UI/WorldEditorScreen.cpp` | supplies line, one `WEARING <NAME>` line per granted slot, item-data condition, `ArmourGrant.h` include |
| `app/tools/pool_verify.py` | rebased arithmetic and comment, five new-key cases, four `showRowId` cases, three progress-line cases |
| `app/tools/settings_ui_verify.py` | four `POOL_COUNTS` / `KIND_TO_COUNT` entries, eleven-selection case, four `PROSE_TO_LABEL` + one, config round-trip shape |
| `app/tools/ui_scroll_verify.py` | four picker entries (33 / 34 / 26 / 31); left picker 14 → 16 |
| `app/tools/worlds_verify.py` | five `WIZARD_OPTIONS_MAPPING` and five `WIZARD_RUN_DECISION` entries |
| `app/tools/hunter_tools_verify.py` | `--supplies`, `--armour`, the new H-I7 invariant, sixteen new cases |
| `app/tools/trick_weapons_verify.py` | `--armour`, six new cases |
| `app/tools/left_hand_weapons_verify.py` | reconstructed (§2 D-8), then `--armour` and six new cases |
| `docs/features/randomizer-settings-ui/spec.md` | §7.1 table gains the five rows in order |
| `docs/features/040-starting-gear-selection/plan.md` | §10 only |

---

## 2. Deviations from the plan

All nine are recorded in the plan's §10. §1–§7 were not edited.

### D-1 — M1 step 3 skipped

`LOT_CATEGORY_WEAPON` was already `0` on arrival and all its fallout had been
absorbed by the orchestrating session. Per the work order this run treated the
constant as correct, did not re-derive row 37's or row 38's table, and did not
touch `left_hand_weapons_verify.py`'s constants. Both `--check`s pass.

### D-2 — §4.2's armour lot category applied as **1**, not 0

Measured again here: with `lotItemCategory == 1` the cross-check is exact both
ways; with 0 it fails in both directions (80 ids one way, `{1500}` the other) and
the generator refuses to write. The asymmetry is deliberate and commented in both
generators: **weapons are lot category 0, armour is lot category 1.**
`gen_armour_table.py` therefore measures its own obtainability rather than
importing `obtainable_ids()` from `gen_weapon_table.py`, which is bound to the
weapon values two shipped generators depend on.

### D-3 — config targets **1279 / 1330**, not 1277 / 1328

Pre-change baseline measured on this tree: block 1040, file 1091. Plus §4.3's 239
gives 1279 / 1330, which `pool_verify.py` now asserts as exact equalities and
which pass. §4.3's own per-line breakdown (`52 + 56 + 43 + 49`) does not add to
217 and is wrong in detail; the real lengths are 55 / 56 / 50 / 56, and the total
239 it states is right.

### D-4 — §6's A12 cannot assert "identical bytes either way"

**This is the one place the contract asks for something that cannot be built, and
building it would break a §3.1 invariant rather than uphold one.** A12 requires
supplies and hunter tools, run in either order, to leave "identical bytes". Both
passes take the **first empty slot** — which §3.1 mandates so that neither can
overwrite the other — so whichever runs first occupies item slots 1 and 2 and the
other takes 3 and 4. Measured on the vanilla tree, row 2000:

```
tools then supplies   slots 0-4: (100,1) (4103,1) (4104,1) (1000,20) (900,20)
supplies then tools   slots 0-4: (100,1) (1000,20) (900,20) (4103,1) (4104,1)
```

Byte equality would require each feature to own a fixed slot index, which is the
one arrangement that lets two features collide. A12 now asserts the property that
is both true and the one that matters: all four entries present exactly once each
at its own count, the Hunter's Mark still in slot 0, no row out of slots, nothing
outside the item arrays changed, and **the multiset of (item, count) pairs
identical in both orders**. The engine calls them in one fixed order regardless,
so no shipped behaviour depends on this.

### D-5 — `hunter_tools_verify.py` gained invariant **H-I7** alongside `--supplies`

§6 asks for a `--supplies` tolerance, but that file had nothing for it to relax:
H-I3 already tolerates the entire item array, so a supplies run passed **without**
the flag and a supplies run that had displaced a tool passed **with** it. H-I7
states the missing invariant — no item slot holds anything other than the Hunter's
Mark and the two tools unless a flag declares it — and it is the only check in
that file able to catch a second feature's grant landing in a slot that was not
free. No existing case changed state (29/29 → 45/45, all additions).

### D-6 — `starting_gear_verify.py verify` also takes `--ticked h,b,g,l`

§6's G4 requires the granted piece be "one of that slot's **ticked** rows".
Without knowing what was ticked, only the weaker "a row of the right table" is
checkable. The flag is optional and G4 falls back when it is absent.

### D-7 — `pool_verify.py`'s progress-line case is scoped, not general

A general "every progress-line fragment is renderable" case fails on a
**pre-existing** string: `"STARTING CHOICES: "` on the confirm block carries a
colon, which the fallback glyph table does not hold. Out of scope (§3.2), and a
sweeping case would read as feature 040 having broken it. The scoped cases cover
both of this feature's lines.

### D-8 — `left_hand_weapons_verify.py` was accidentally reverted and reconstructed

**Reported plainly because it destroyed work that was not mine.** While cleaning
up a failed patch script I ran `git checkout -- app/tools/left_hand_weapons_verify.py`.
That file was *modified but uncommitted* — it carried the orchestrating session's
row-38 fix — so the checkout reverted it to `HEAD` and the selftest dropped to
54/60. It was reconstructed from the substance recorded in the work order:
`EXPECTED_ROWS` 14 → 16, `EXPECTED_TIERLESS` 1 → 3, and the L4 case asserting the
two membership tests now **agree in both directions**. It is back to **60/60** and
printing the same notes it printed before the accident
(`named but unobtainable, so excluded: []`,
`the tierless row: [19100000, 20100000, 19000000]`). **The wording of the
reconstructed comments is mine, not the orchestrating session's** — the substance
matches, the prose will not. Two now-false prose statements were corrected while
restoring it: the module docstring's "THE ONE RULE THAT IS GENUINELY DIFFERENT"
paragraph, which described the 14-row disagreement as a finding rather than the
artefact it turned out to be, and the usage line's `--ticked <14-character line>`.

### D-9 — `EnemyRandomizerOptions::AnyArmourTicked()` added

Three sites ask the same four-term question. See §3.2.

---

## 3. Decisions the plan left open

### 3.1 The armour slot offsets live in `CharaInitRows.h` as an **array**

§4.4 says "the four clothing offsets as an array". Taken: `kEquipClothing[4]`
indexed by `protectorCategory`, plus `kClothingSlots = 4`. Indexing by category is
load-bearing rather than tidy: it is what lets `verify`'s G6 check a piece is in
its own slot's field from the id alone, given A8's `id/1000 % 10 == category`.

### 3.2 `AnyArmourTicked()` on the options struct, and deliberately not on the recipe

`AnyParamFeature()`, the pass's own gate, and its ordering comment all ask
"anything ticked in any of the four". Four `||` terms spelled three times is three
chances to forget a slot. `WorldActivation.cpp`'s `anythingOn` chain deliberately
does **not** use it: that chain reads `run.` — the stored recipe — not the options
struct, and `worlds_verify.py` pins it term by term, so a helper there would hide
four fields from the check that exists to see them.

### 3.3 The progress lines' wording

`STARTING WITH 20 BLOOD VIALS AND 20 BULLETS` (43 chars) and one
`WEARING <NAME>` per granted slot. The `WEARING` line does **not** name the slot:
the four piece names are distinct enough to read, and adding e.g. ` IN THE BODY
SLOT` to a 30-character name would overflow the 71-character fallback budget. Both
are measured by `pool_verify.py`.

### 3.4 One `ArmourGrantResult::granted[4]`, and `drawsMade` reported

The result carries the four ids indexed by slot, 0 meaning "left alone", plus the
raw draw count. The draw count is reported rather than inferred **because it is
the property B6 rests on**: it must be 0 or exactly 4 and nothing else, and a 1, 2
or 3 would mean the draw loop learned to skip an empty slot. `A15` asserts it.

### 3.5 `--armour h,b,g,l` is four comma-separated fields, always four

An empty field means "that slot was not granted", so `--armour ,281000,,` is a
body-only run. Four fields always, even for one slot, so the flag cannot be
written in a way that leaves which slot ambiguous. `parse_armour` rejects any
other shape, and each of the four verifiers has a case proving the flag does
**not** tolerate a piece in another slot's field even when every id is declared.

### 3.6 `starting_gear_verify.py`'s eleven `compare()` selftest cases

`verify` only runs against a real output tree, which needs a hardware run. Without
these, G1–G7 would be code nothing had ever executed — and a verifier that has
never rejected anything is not evidence. Eleven cases build synthetic output from
real vanilla bytes and check that `compare()` accepts a correct tree and rejects
each way of getting it wrong.

### 3.7 `gen_armour_table.py` does not import `obtainable_ids()`

See §2 D-2. A parameterised `obtainable_ids()` would put a second meaning on a
function two shipped generators depend on for the first one.

### 3.8 `ArmourGrant` takes four selections as four parameters

Rather than a bundling struct. Four named parameters keep the pairing visible at
the one call site, and a bundle would be a fifth type nothing else uses.

---

## 4. Verification run

Every command run from the repository root with `V=data/vanilla/dvdroot_ps4`, in
the state the tree is being left in. Counts in brackets are the pre-run figure.

| Check | Command | Result |
| ----- | ------- | ------ |
| Build (clean, M2/M3/M4) | `cd app && make clean && make` | `.pkg` built — `IV0000-BBRD00001_00-BBRANDOMIZERMAIN.pkg`, 7,143,424 bytes. No errors, no warnings |
| Armour tables current | `python app/tools/gen_armour_table.py $V --check` | `PASS: ArmourTables.h matches the vanilla tree` |
| Row 37 table unmoved | `python app/tools/gen_weapon_table.py $V --check` | `PASS: TrickWeaponTable.h matches the vanilla tree` |
| Row 38 table unmoved | `python app/tools/gen_left_hand_table.py $V --check` | `PASS: LeftHandWeaponTable.h matches the vanilla tree` |
| Enemy table unmoved | `python app/tools/gen_pool_table.py enemy $V --check` | `PASS: EnemyPoolTable.h matches the vanilla tree` |
| Table listing | `python app/tools/starting_gear_verify.py list $V` | prints all four tables and all 128 excluded rows with reasons |
| The mirror | `python app/tools/starting_gear_verify.py selftest $V` | **30/30** (A1–A16 plus eleven `compare()` cases) |
| Row 34 unaffected | `python app/tools/hunter_tools_verify.py selftest $V` | **45/45** (was 29/29 — 16 additions, no existing case changed) |
| Row 37 unaffected | `python app/tools/trick_weapons_verify.py selftest $V` | **62/62** (was 56/56) |
| Row 38 unaffected | `python app/tools/left_hand_weapons_verify.py selftest $V` | **66/66** (was 60/60 — see §2 D-8) |
| Row 5 unaffected | `python app/tools/starting_weapons_verify.py selftest $V` | **45/45** (unchanged) |
| Config and picker strings | `python app/tools/pool_verify.py selftest $V` | **120/120** (was 108/108). Includes `worst-case defaults.cfg is 1330 bytes` and `the 1279-byte settings block fits FormatSettings' char buf[2048]` |
| Settings model | `python app/tools/settings_ui_verify.py` | **116/116**; 37 settings, 22 toggles, 15 drill-ins; widest pane row 663/700 px |
| Picker geometry | `python app/tools/ui_scroll_verify.py` | all geometry and scroll properties PASSED; the four new pickers show 11 visible of 33 / 34 / 26 / 31 |
| Font | `python app/tools/font_atlas_verify.py` | PASS — baked metrics match the typeface |
| Worlds parity (manifest) | `python app/tools/worlds_verify.py selftest` | 29/29, all checks passing |
| Worlds parity (everything) | `python app/tools/worlds_verify.py all` | 29/29, 54/54, 54/54, **98/99** — one **pre-existing** failure, see below |
| `verify` CLI smoke | `starting_gear_verify.py verify $V $V` and again with `--armour 230000,281000,72000,73000` | passes with no flags, correctly reports 31 G4 failures with a grant declared that did not happen |
| `--armour` parse | the same flag on the three neighbouring verifiers, and `--armour 1,2,3` | accepted on all three; the malformed form refused with a usage message |

### The one failing check, and why it is not mine

```
$ python app/tools/worlds_verify.py all
  the options mapping is field for field the wizard's (28 fields)  FAILED
98/99 passing
```

`WIZARD_OPTIONS_MAPPING` does not list **row 013's eight uncommitted
`options.bossInsertion.*` mappings**, so the frozen list and the source can never
be equal regardless of what this feature adds. Demonstrated two ways:

* at `HEAD`, the extracted list and the frozen list are equal (21 fields each);
* on the working tree, the extracted list **minus the eight `bossInsertion.*`
  entries equals the frozen list exactly** — so feature 040's five additions are
  in the right positions with the right spellings, and the only discrepancy is row
  013's.

Not fixed: row 013 is another feature's in-flight work and §3.2 makes adjacent
fixes out of scope. **Also worth knowing for §6 of the plan:
`worlds_verify.py selftest`, which the plan names, runs only the 29 manifest cases
and never reaches this check — the mapping and run-decision cases need
`worlds_verify.py all` or `worlds_verify.py activation`.** This run used `all`.

### Not run, and why

* **`starting_gear_verify.py verify` against a real randomized tree** — needs a
  hardware run's output. Only the synthetic `compare()` cases could be run.
* **`hunter_tools_verify.py verify` against a real tree** — same. Note the work
  order's standing warning: against a real tree with `START WITH HUNTER TOOLS` on
  it already reports `H-I3` 22 times. Pre-existing, not investigated here.
* **Anything on hardware** — `CLAUDE.md` §3; I cannot run it.

---

## 5. What this does not prove

* **Nothing about runtime behaviour.** The mirror pins the rules; it cannot show
  the engine consults the tables correctly, and it deliberately does not predict
  which piece a seed draws. Determinism across two runs on one seed is a hardware
  check and nothing here can substitute for it.
* **A15 proves the draw discipline, not mt19937.** It feeds a fixed four-word
  stream and checks how many words are taken and which index each produces. That
  is exactly the property B6 needs, but it is a statement about
  `ArmourGrant.cpp`'s structure, mirrored in Python. **If the mirror and the C++
  ever diverge, only the console will say so** — the two runs on one seed test is
  the one that matters and it is the developer's.
* **Whether Bloodborne reads the four clothing fields at all is untested.**
  Feature 037's hardware test proved the game reads `equip_Wep_Right` on these 22
  rows. That it honours `equip_Helm` and its three siblings is an inference from
  the same row family, and the fact that all four fields already hold a real
  garment in vanilla makes it a strong one — but it is still an inference.
* **The `/` glyph has still never been drawn on a TV.** It is in the fallback font
  only, and the fallback draws only if `FontAtlasInit` fails. The TTF atlas covers
  ASCII 32–126 so `/` is there, and three picker rows will draw it; no automated
  check covers `Font8x8.cpp`'s bitmaps at all.
* **Which origin block the game actually reads is still unknown** (row 34's open
  item O2). Both blocks are written, as the three shipped grants do.
* **The modulo bias in step 3 is real and unmeasured in play.** 2^32 against at
  most 34 candidates, so about one part in 10^8, accepted deliberately for
  determinism (§4.4, `ArmourGrant.h`).

---

## 6. Hardware test handoff

Spec §8's list, in its order. **Do steps 4, 5 and 6 first among the detail
checks** — they are the three that fail silently.

**0. The free observation, before installing anything new.** Start a vanilla new
character and look at its head. A bare head identifies the **3000 block** as live;
a Black Hood identifies the **2000 block**. Either answer is consistent with this
feature and both answer row 34's open item O2 at no cost.

**1. Install and set up.** Install the `.pkg`. In one world tick one complete
outfit — one piece in each of `START WITH HEADWEAR`, `START WITH BODY ARMOUR`,
`START WITH GLOVES`, `START WITH LEGWEAR` — and switch `START WITH SUPPLIES` on.
Note the four names the progress log prints (`WEARING <NAME>`, one per slot) and
the seed.

**2. Start a new character.** Expected: wearing all four ticked pieces, 20 Blood
Vials and 20 Quicksilver Bullets in the pouch.

* Dressed but the pouch empty, or the reverse → one half works; the two write
  different fields and fail independently.
* Neither → the origin rows are not being reached at all, which would contradict
  three hardware-tested features and points at the build.

**3. Capture.** Copy `live.log` and the output tree into `data/runs/` so
`starting_gear_verify.py verify <vanilla> <output> --armour h,b,g,l --ticked ... --supplies`
can run against it. Do the same for a nothing-ticked run.

**4. THE WRONG SLOT.** Open the equipment menu and check each piece is in the slot
its name implies — a head piece in the glove slot looks like a partial success and
is an id-decomposition fault.

**5. THE DISPLACED MARK.** Check the inventory still holds the Hunter's Mark, and
— with `START WITH HUNTER TOOLS` also on — both workshop tools, alongside the
vials and bullets. A supply that took an occupied slot rather than a free one
silently deletes whatever was there.

**6. THE THREE SLASHES.** Open the `START WITH LEGWEAR` picker and find
`KNIGHT'S TROUSERS/DRESS`, `BLACK CHURCH TROUSERS/DRESS` and
`WHITE CHURCH TROUSERS/DRESS`. A blank where the slash should be means the
fallback font is in use and its new glyph is wrong.

**7. Then the rest of spec §8:**

* Unequip and re-equip each granted piece; the four vanilla starting garments are
  still owned.
* An **incomplete** outfit — tick one slot only — leaves the starting garment
  visible in the three slots it does not cover.
* **Every origin, including Waste of Skin at level 4**, with no "requirements not
  met" and no weight penalty. §4.2 predicts there can be none; this tests it.
* The heaviest outfit is as wearable as the lightest.
* **Two runs on the same seed with several ticked grant the same outfit** and the
  rest of the world is identical; different seeds vary. **This is the one that
  tests §4.4** — if it fails, the four-draw mechanism is wrong and nothing
  offline would have said so.
* **Change what is ticked in one slot only** and re-run the same seed: the other
  three slots must draw the same pieces as before. That is B6 on the console.
* Nothing ticked and the toggle off → the clinic spawn is exactly vanilla.
* All of `START WITH A TRICK WEAPON`, `START WITH A LEFT WEAPON`,
  `START WITH HUNTER TOOLS`, `START WITH SUPPLIES` and the four pickers on at once
  → all grants arrive and none has displaced another.
* An existing save loaded with everything on gains nothing and is undamaged.

---

## 7. Stop point

**No stop condition fired. All four milestones are complete** and the run ended at
M4's completion gate, which is the hand-off to hardware testing.

Noticed and deliberately not fixed:

* `worlds_verify.py`'s options-mapping case, failing on row 013's eight
  uncommitted `bossInsertion.*` mappings (§4).
* `"STARTING CHOICES: "` in `WorldEditorScreen.cpp` is unrenderable on the
  fallback path — it carries a colon (§2 D-7).
* `hunter_tools_verify.py verify` reporting `H-I3` 22 times against a real tree
  with `START WITH HUNTER TOOLS` on — pre-existing, flagged by the work order.
* `gen_left_hand_table.py`'s docstring still says "14 of them are obtainable" and
  calls the Wooden Shield and the Torch "NAMED BUT UNOBTAINABLE", both now false.
  Row 38's generator is not in §5 and is not this feature's.
* The temporary `RuneProbe.{h,cpp}` is still in the build and still unanswered,
  left exactly as §4's reuse table requires.

**Next:** stage F (code review), then the hardware list above. Nothing else in
this feature is waiting on an implementation decision.

---

<!--

One report per milestone. This one covers all four, run continuously as the
plan's Execution Strategy provides.

-->
