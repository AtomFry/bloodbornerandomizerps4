# Implementation Report 038 — Start With A Left-Hand Weapon

**Status: BUILT — READY FOR HARDWARE TESTING.** The `.pkg` builds clean from
`make clean` with no warnings and every applicable automated check is green.
Nothing here shows that Bloodborne honours `equip_Wep_Left`; that is the one
question the console still has to answer (§6).

**Spec and plan were deliberately skipped at the developer's request, and this
report is the only artifact for this row.** That is the repository's convention
for a pipeline-skipped item — see `docs/features/README.md`, where **n/a —
skipped** marks an item the developer took outside the pipeline and the
implementation report stands alone. There is no `spec.md`, no `plan.md`, no
`plan-evidence.md` and no `log.md` in this folder, and none is missing.

**The contract this was built to** was feature 037's shipped implementation plus
the developer's written brief. Row 38 is row 37 for the other hand: the same 22
`CharaInitParam` origin rows, the same requirement writer, the same picker, the
same config encoding, a different field (`equip_Wep_Left`, s32 at offset 24) and
a different table of weapons. Row 37's 2026-09-27 hardware test proved the game
reads that row family at character creation and equips what it finds in
`equip_Wep_Right`, so there was no probe here and no gated milestone; this was
built through in one pass.

**Implemented:** 2026-09-27

---

## 1. What was built

`START WITH A LEFT WEAPON` is a pool-picker setting on both settings screens,
nothing ticked by default, offering the **14** left-hand weapons a player can
actually obtain in vanilla. Tick nothing and nothing is granted; tick one and
every new character starts with it in the left hand; tick several and one is
drawn for the run. The granted weapon's requirements come down to 9/9/5/6 on its
base row and its upgrade tiers, through the same owner-ranked
`WeaponRequirementWriter` row 5 and row 37 already share, so the grant still wins
against a Hunter's Dream coffin pick in either call order.

The table is **generated and `--check`-pinned** from the vanilla tree, derived
from the `leftHandEquipable` bit plus the game's own text plus obtainability. No
weapon id is hardcoded anywhere, and no row was hand-excluded.

### The 14 rows, in the frozen table order

Order is by name, the same frozen rule `TrickWeaponTable.h` uses (base-weapon
name, then version — and with every row at version 0 that reduces to
alphabetical).

| # | id | name | requirements as shipped (STR/SKL/BLT/ARC) |
| - | -- | ---- | ----------------------------------------- |
| 0 | 15000000 | `CANNON` | 30 / 13 / 0 / 0 — **no origin can wield it** |
| 1 | 35000000 | `CHURCH CANNON` | 27 / 0 / 16 / 0 — **no origin** |
| 2 | 14100000 | `EVELYN` | 9 / 11 / 18 / 0 — **no origin** |
| 3 | 34000000 | `FIST OF GRATIA` | 7 / 9 / 5 / 0 |
| 4 | 18100000 | `FLAMESPRAYER` | 0 / 8 / 0 / 8 |
| 5 | 33000000 | `GATLING GUN` | 28 / 12 / 0 / 0 — **no origin** |
| 6 | 6000000 | `HUNTER BLUNDERBUSS` | 7 / 9 / 5 / 0 |
| 7 | 14000000 | `HUNTER PISTOL` | 7 / 9 / 5 / 0 |
| 8 | 20000000 | `HUNTER'S TORCH` | 6 / 0 / 0 / 0 |
| 9 | 19100000 | `LOCH SHIELD` | 11 / 8 / 0 / 0 — **and it has no upgrade tiers** |
| 10 | 6100000 | `LUDWIG'S RIFLE` | 9 / 10 / 9 / 0 |
| 11 | 36000000 | `PIERCING RIFLE` | 9 / 10 / 9 / 0 |
| 12 | 14200000 | `REPEATING PISTOL` | 10 / 11 / 8 / 0 — **no origin** |
| 13 | 18000000 | `ROSMARINUS` | 0 / 8 / 0 / 8 |

That is the 11 firearms plus the Loch Shield, the Hunter's Torch and the Fist of
Gratia, exactly as the brief said, and all 14 are in scope. **Five of the 14 no
origin can wield as the game ships them**, which is what the requirement profile
is for — without it a third of the picker would grant a weapon the character is
holding and cannot fire.

### The derivation, and where it differs from row 37's

Measured against `data/vanilla/dvdroot_ps4`:

| | |
| --- | --- |
| tier-0 rows with `leftHandEquipable` (byte 256 **bit 1**) set | **18** |
| ...named by the game's own text, not `*` and not empty | **16** |
| ...sold in a shop or present in an item lot | **14** |
| the picker | **14** — the intersection |

The four exclusions, all derived and all reported by the tool rather than listed
in it:

| id | why |
| -- | --- |
| 6180000 | version 8 under Ludwig's Rifle — no name in the game's text, in no shop and no item lot |
| 20090000 | version 9 under the Hunter's Torch — name is the placeholder `*`, unobtainable |
| 19000000 | **Wooden Shield** — the game names it, but no shop and no item lot can give it to a player |
| 20100000 | **Torch** — same |

**This is the one structural difference from row 37 and it drove the main design
decision (§3.1).** On the right hand the name test and the obtainability test
agree on every one of the 85 candidates, and `gen_weapon_table.py` treats a
disagreement as proof that the derivation is wrong and refuses to write. On the
left hand they genuinely disagree: 16 named, 14 obtainable. So obtainability is
the governing rule here, the name test is a second condition rather than a
cross-check, and the assertion that holds is the implication — **obtainable
implies named, never the reverse.** The new generator asserts that direction and
fails loudly if it ever breaks; the new verifier asserts both the implication and
that every row the two tests disagree on is excluded.

**No version dimension. Confirmed, and asserted.** No left-hand family has an
Uncanny or a Lost form: every one of the 14 is version 0, the only other versions
any of those families carry are the param's placeholder version 9 and the two
nameless version-8 rows, and the generator refuses to write if that ever stops
being true (a version dimension would change the row count and therefore the
config line's length). So this is a flat 14-row table, and the verifier proves
flatness against the whole weapon param rather than against the table.

### Every file changed

**New**

| File | What |
| ---- | ---- |
| `app/tools/gen_left_hand_table.py` | the left-hand derivation, the frozen order, `--check` |
| `app/src/Randomizer/LeftHandWeaponTable.h` | generated: 14 rows, `kLeftHandWeaponCount` |
| `app/src/Randomizer/LeftHandWeaponGrant.h` / `.cpp` | the pass: candidate list, one draw, the `Grant`-ranked profile, one write per origin row |
| `app/tools/left_hand_weapons_verify.py` | `list`, `selftest` (60 cases), `verify` (G1–G6) |

**Modified — the app**

| File | What |
| ---- | ---- |
| `app/src/Randomizer/EnemyPoolSelection.h` | includes the new table; `LeftHandWeaponSelection` typedef, `false` fail-safe |
| `app/src/Randomizer/RandomizerDefaults.h` | `LeftHandWeaponSelection leftHandWeapons;` |
| `app/src/Randomizer/RandomizerDefaultsStore.cpp` | `left_hand_weapons_included` in `FormatSettings` and `ApplySettingKey` |
| `app/src/Randomizer/EnemyRandomizer.h` | the option, `AnyParamFeature()`, two result counters |
| `app/src/Randomizer/EnemyRandomizer.cpp` | includes the pass; calls it after the right-hand grant with the shared `reqWriter` |
| `app/src/UI/SettingsModel.h` | `SettingId::StartWithLeftHandWeapon`, `SettingKind::LeftHandWeaponPool` |
| `app/src/UI/SettingsModel.cpp` | the `kSettings` entry and the five `switch` cases |
| `app/src/UI/ModelPicker.h` | `kLeftHandWeaponsStrings`, `showRowId` explicitly `false` |
| `app/src/UI/WorldEditorScreen.cpp` | the progress line naming the weapon and the hand; the item-data line's condition |
| `app/src/Game/WorldActivation.cpp` | the options mapping in frozen order; `anythingOn` gains the term |

**Modified — the tools and one spec**

| File | What |
| ---- | ---- |
| `app/tools/gen_weapon_table.py` | `right_hand_tier0()`'s body extracted into `tier0_with_hand_bit(root, field)`; `right_hand_tier0()` is now a one-line wrapper. **No behaviour change** — `--check` still passes and `trick_weapons_verify.py` still imports the same name |
| `app/tools/pool_verify.py` | settings block 686 → **728**, worst case 737 → **779**; the new key's load/save case; `PickerStrings` count 4 → 5 and the new `showRowId` assertion |
| `app/tools/settings_ui_verify.py` | the fifth selection type, kind and pool count; `PROSE_TO_LABEL`; the config round-trip's value shape |
| `app/tools/ui_scroll_verify.py` | a `Left picker` entry — 14 rows on the instruction layout |
| `app/tools/worlds_verify.py` | `WIZARD_OPTIONS_MAPPING` 19 → 20 fields, `WIZARD_RUN_DECISION` 14 → 15 terms |
| `app/tools/hunter_tools_verify.py` | `--granted-left <id>` tolerance, beside the existing `--granted` |
| `app/tools/starting_weapons_verify.py` | `--granted` is now **repeatable**, so a run with both grants can be named |
| `app/tools/trick_weapons_verify.py` | `--granted-left <id>` tolerance, so a both-hands tree can be verified from row 37's side too |
| `docs/features/randomizer-settings-ui/spec.md` | §7.1's Weapons & Starting Gear row gains `Start With A Left Weapon` |

Nothing under `docs/features/037-start-with-trick-weapon/` was touched.
`docs/features/README.md` and `docs/randomization-feature-spec.md` were not
touched — the orchestrating session owns those. No branch, no commit.

---

## 2. The reuse-versus-new-file decision

**Taken: new files beside row 37's, sharing every hand-agnostic part by import,
with one small additive refactor inside row 37's generator.**

Concretely:

* `gen_left_hand_table.py` imports `decompose`, `base_id`, `named_ids`,
  `obtainable_ids` and the new `tier0_with_hand_bit` from
  `gen_weapon_table.py`, and `RENDERABLE` from `gen_pool_table.py`. It owns the
  left-hand membership rule, the flatness assertion, the order and the emitter.
* `left_hand_weapons_verify.py` imports the archive reader, the paramdef offset
  lookup, the 22 origin rows, the requirement offsets, the origin-stat reader,
  the owner-rank mirror and the report printer from `trick_weapons_verify.py`.
  It owns everything about which field is written and which weapons are offered.
* `LeftHandWeaponGrant.{h,cpp}` is its own pass, using `CharaInitRows.h` and
  `WeaponRequirements.h` exactly as `TrickWeaponGrant.cpp` does — nothing in
  either shared header changed.
* The only edit to a row 37 file that was not a tolerance flag is the extraction
  of `tier0_with_hand_bit()`; `right_hand_tier0()` remains and now calls it.

**Why not a `hand` parameter on row 37's generator and verifier.** Four reasons,
in order of weight:

1. **The disagreement assertion cannot be parameterised without weakening it.**
   `derive()`'s refusal to write when the name test and the obtainability test
   disagree is row 37's strongest guard and one of its stop conditions. Making
   it conditional on which hand is being derived would turn a hard invariant into
   a flag, and the left hand would be the case that switches it off. Keeping
   row 37's guard absolute and stating the left-hand rule separately loses
   nothing and hides nothing.
2. **The two derivations are not the same shape.** One is 78 rows over three
   versions with a prefix-composition trap to prove absent; the other is 14 flat
   rows with a flatness claim to prove present. The order rule, the assertions
   and the generated header's prose differ throughout. A single `build()` would
   have been two long `if hand ==` branches wearing one name.
3. **Row 37 is shipped and hardware-proven, and its `--check` pins a committed
   file.** Every future left-hand edit inside that generator would carry a risk
   of moving the right-hand table's bytes. As built, `gen_weapon_table.py
   --check` still passes and `trick_weapons_verify.py`'s 53 pre-existing cases
   still pass unchanged.
4. **Duplication was avoided anyway.** There is exactly one FMG reader, one shop
   and item-lot reader, one id decomposition, one packed-bit locator, one
   hand-bit filter, one archive reader, one origin-row list, one requirement
   writer and one picker component. What is duplicated is the emitter skeleton
   and the G1–G6 comparison loop, and those are duplicated because they are
   describing different fields of different tables — a shared version would need
   the field and the table passed in at every step, which is the same code with a
   parameter list.

**Row 37's output did not change.** `gen_weapon_table.py --check` passes,
`TrickWeaponTable.h` is byte-identical, and `hunter_tools_verify.py`,
`starting_weapons_verify.py` and `trick_weapons_verify.py` all pass with **no
pre-existing case changing state** (§4).

---

## 3. Decisions I had to take

### 3.1 The left-hand membership rule is a conjunction, not an agreement

Covered in §1. Row 37's tooling asserts that the name test and the obtainability
test *agree*; on the left hand that is false about correct data. Taken:
obtainability governs, the name test is the second condition, the picker is the
intersection, and what is asserted is `obtainable ⊆ named`. The generator raises
and refuses to write if an obtainable-but-unnamed row ever appears, because that
would mean the two sources are describing different data. The verifier proves the
generator really does refuse (it stubs `named_ids` to return nothing and requires
a `SystemExit`), so the implication is a test rather than a comment.

### 3.2 The label is `START WITH A LEFT WEAPON`, not `START WITH A LEFT-HAND WEAPON`

Forced, and measured. `settings_ui_verify.py` case 5 requires
`label + 40 px + widest value` to fit the 700 px settings pane. With a 118 px
`10 OF 14`, the longer wording measures 573 px and totals **731 px** — 31 px
over. `START WITH A LEFT WEAPON` totals 623 px. The picker's heading uses the
same string, and the help text carries what "left" means:

> "Start holding one of the firearms, shields or torches you tick here in your
> left hand. Tick several and one is drawn for the run. Its requirements are
> lowered so you can use it at once."

The settings-UI spec's §7.1 row and `PROSE_TO_LABEL` use the prose name
`Start With A Left Weapon` to match. The feature, the folder and the backlog row
keep the name "left-hand weapon"; only the on-screen row is shortened.

### 3.3 The Loch Shield has no upgrade tiers, and nothing was changed to accommodate it

`19100000` is the only row in either hand's table that ships as a base row and
nothing else — no `+1`…`+10`. Row 37's T7 asserts that all 78 of its rows have
all ten tiers; that assertion is **false for this table** and was not copied.
`WeaponRequirementWriter::Apply` already skips a missing tier rather than
erroring, so the profile covers one row for the Loch Shield and eleven for the
other thirteen, and no code changed. The verifier asserts the shape as a property
(*exactly one row has no tiers and none has only some*) and reports which row it
is rather than hardcoding the id, and it runs the whole G1–G6 comparison on a
simulated Loch Shield grant specifically so that "and its ten tiers carry the
profile" cannot become a false failure.

### 3.4 The config key is `left_hand_weapons_included`, 42 bytes

26-character key plus `=` plus a 14-character line plus a newline. The settings
block goes 686 → **728** of `char buf[1024]`, and the worst-case `defaults.cfg`
737 → **779**, leaving 296 bytes spare. `pool_verify.py`'s two exact equalities
are updated and its narration comment records the arithmetic's history, as it
does for every earlier key. The two selection lines being **different lengths**
(78 and 14) is load-bearing and now pinned: one pasted into the other's key is
rejected by the length guard rather than silently remapped onto another feature's
weapons.

### 3.5 The pass runs after the right-hand grant, and the two draw independently

`StepItemData` now ends with the right-hand grant and then the left-hand grant,
so the two are the last two RNG consumers of the run and turning either on cannot
move any earlier roll. The left-hand draw is from its own table and never
consults the right hand's choice. They share the one `WeaponRequirementWriter`
created for the run, so spec 037 D4's precedence rule covers all three passes
that lower a requirement.

I did *not* try to make the two grants one pass with a hand parameter. They write
different fields of the same row and offer different tables; two passes of forty
lines each read better than one of sixty with a hand argument threaded through
it, and the ordering claim ("this is the last roll") is easier to state and to
test per pass.

### 3.6 `SettingKind::LeftHandWeaponPool` is a fifth kind, not a parameter

The four `Selection*` switches are the one place the picker types are told apart,
and the two weapon selections are different C++ types bound to different tables
of different sizes. A fifth enumerator keeps that pairing stated where it is
already stated; a parameter would have moved it somewhere the compiler stops
checking it.

### 3.7 The progress line says which hand

Row 37's line is `STARTING WITH <NAME>`. With both features on the player would
see two of those and could not tell which hand was which, so row 38's reads
`STARTING WITH <NAME> IN THE LEFT HAND`. Row 37's line was left exactly as
shipped. The name comes from `LeftHandWeaponName()` in the engine, so no screen
file learns how a weapon id is spelled (`CLAUDE.md` §6).

### 3.8 Tolerance flags on the three older tools

Three tools assert "only member X differs" or "only field Y differs" and would
report a correct combined hardware run as FAILED. That is the hazard row 37's
plan named and fixed for its own feature; the same fix was extended rather than
left for the developer to trip over:

* `hunter_tools_verify.py` gains `--granted-left <id>`, beside the existing
  `--granted`. **Field-specific on purpose**: `--granted-left` does not tolerate
  a write to `equip_Wep_Right` and vice versa, because a crossed write between
  two features whose fields are eight bytes apart is the mistake that can
  actually happen. Six new cases, including that negative one and the
  all-three-features-on direction.
* `starting_weapons_verify.py`'s `--granted` becomes **repeatable**. That tool
  reads `EquipParamWeapon`, where a weapon is a weapon and the hand is invisible;
  what it needs to know is which ids were allowed to be restatted, so one flag
  taking several ids is the honest shape rather than a second per-hand flag. A
  single `int` still behaves exactly as before, and there is a case asserting
  that an int and a one-element list give the identical result.
* `trick_weapons_verify.py verify` gains `--granted-left <id>`, so a both-hands
  output tree can be checked from row 37's side as well as row 38's. Also
  field-specific, with a negative case.

Every one of these is additive: **no pre-existing case in any of the three
changed state.**

### 3.9 Shape assertions run against the source with `//` comments stripped

`left_hand_weapons_verify.py` asserts that the pass touches no item slot and no
other equip field. The pass's header comment necessarily *names*
`equip_Wep_Right`, `equip_Subwep_Left` and the item slots in order to say that it
deliberately leaves them alone, so a raw-text search would have been checking the
prose. The tool strips line comments first and documents why. Row 37's equivalent
cases search the raw text; they pass either way today and were left alone.

---

## 4. Verification run

Commands as run, from the repository root, with `PYTHONIOENCODING=utf-8`.

### Build

| Check | Command | Result |
| ----- | ------- | ------ |
| Clean build | `cd app && make clean && make` | exit 0, **no `error` or `warning` anywhere in the log**, `IV0000-BBRD00001_00-BBRANDOMIZERMAIN.pkg` (7 143 424 bytes) produced, `src/x64/Debug/Randomizer/LeftHandWeaponGrant.o` present |

A clean rebuild was required and done: `RandomizerDefaults` grows by 14 bytes and
both settings screens and `WorldStore` hold copies of it.

### Automated — the new feature

| Check | Command | Result |
| ----- | ------- | ------ |
| Table is current | `python app/tools/gen_left_hand_table.py data/vanilla/dvdroot_ps4 --check` | `PASS: LeftHandWeaponTable.h matches the vanilla tree` |
| The mirror | `python app/tools/left_hand_weapons_verify.py selftest data/vanilla/dvdroot_ps4` | **`selftest 60/60`** |
| Table listing | `python app/tools/left_hand_weapons_verify.py list data/vanilla/dvdroot_ps4` | 14 rows in the frozen order plus all four excluded rows with a reason each. Not a pass/fail gate |
| `verify` smoke test | `python app/tools/left_hand_weapons_verify.py verify data/vanilla/dvdroot_ps4 data/vanilla/dvdroot_ps4` | exit 1 with `G4 0 of 22 origin rows were written` — correct: a tree that granted nothing is not a granted run |

What the 60 cases cover: the candidate set and that the hand bit really is bit 1
of the same byte as bit 0 (and that no tier-0 row is equipable in both hands);
the three derivations and the implication between them; that the generator
refuses the impossible direction; the committed table's count, frozen order,
distinct names, renderability and line width; that every name is the FMG string
for that exact id; flatness measured against the whole weapon param; the tier
shape; the origin minimum re-derived from `CharaInitParam` and the
five-of-fourteen wieldability figure; the C++'s offset, its eight-byte
relationship to row 37's, and its profile; the pass's shape (returns before the
draw and before any write, exactly one draw, exactly one write, no item slot, no
other equip field, no fixed weapon id, goes through the shared writer); G1–G6 on
a simulated grant including the tierless row; eleven negative cases, of which the
important one is **writing the right hand's field is rejected**; both hands
granted in either order; all three character-creation features on at once; D4's
precedence both ways round; and the config line at 14, 13, 15 and 78 characters.

### Automated — nothing else moved

| Check | Command | Result |
| ----- | ------- | ------ |
| Row 37's table is current | `python app/tools/gen_weapon_table.py … --check` | `PASS` |
| Row 37 unaffected | `python app/tools/trick_weapons_verify.py selftest …` | `selftest 56/56` (was 53/53; the three new are the `--granted-left` tolerance. **No existing case changed state**) |
| Row 34 unaffected | `python app/tools/hunter_tools_verify.py selftest …` | `selftest 25/25` (was 19/19; the six new are the `--granted-left` tolerance. **No existing case changed state**) |
| Row 5 unaffected | `python app/tools/starting_weapons_verify.py selftest …` | `selftest 21/21` (was 17/17; the four new are the repeatable `--granted`. **No existing case changed state**) |
| Config and picker strings | `python app/tools/pool_verify.py selftest …` | `95/95 passing` (was 94/94; the new one is the new key's load/save case. The 728/779 pair and the `PickerStrings` count are the same cases with updated numbers) |
| Settings model | `python app/tools/settings_ui_verify.py` | `113/113 passing` — same case count; the model now reports 21 settings, 15 toggles, 6 drill-ins, `WeaponsGear` 6 |
| Picker geometry | `python app/tools/ui_scroll_verify.py` | `all geometry and scroll properties PASSED`; `Left picker 11 rows visible of 14` → 2 pages |
| Worlds parity | `python app/tools/worlds_verify.py` | `97/97 passing`, with `the options mapping is field for field the wizard's (20 fields)` and `the run decision is the wizard's || chain, field for field (15)` |
| Item-data round trip | `python app/tools/itemdata_verify.py roundtrip …` | `PASS round trip byte-identical (28309992 bytes back)`, `PASS archive still lists the same 65 entries` |
| The rest of the suite | `caged_dogs_verify` 23/23, `drops_verify` 6/6, `easy_modes_verify` 25/25, `mergo_darkness_verify` 9/9, `treasure_verify` 8/8, `boss_verify` 5/5, `font_atlas_verify` PASS | all green, all unchanged |

**Not run, and why:** `left_hand_weapons_verify.py verify <vanilla> <output>`
against a real granted output tree, and the two older tools' tolerance flags
against one. There is no such tree — producing one needs the app to run on the
console, and `data/runs/` holds only pre-feature runs. The `verify` code path was
smoke-tested as above and its G1–G6 logic is exercised by nineteen positive and
negative cases against real vanilla bytes, but it has not seen output the PS4
actually wrote.

---

## 5. What this does not prove

**Nothing here shows the feature works** (`CLAUDE.md` §3). Specifically unproven:

* **Whether Bloodborne honours `equip_Wep_Left`.** This is the whole risk. It is
  a much smaller risk than row 37's was — the console has already shown that the
  game reads this row family at character creation and equips what it finds in
  `equip_Wep_Right`, and `equip_Wep_Left` is the next-but-one s32 in the same
  struct — but it is still an inference. Both fields are `-1` on all 1700 rows of
  the shipped param, so the data says nothing about either, and
  `docs/known-traps.md`'s "byte decoding does not prove game behaviour" applies
  unchanged.
* **Whether a left-hand item behaves differently in kind.** A firearm needs
  Quicksilver Bullets, a torch is a light source, a shield is a shield, and the
  Fist of Gratia is a fist. Whether the game is willing to start a character
  holding each of those, and whether the bullets a new character has are enough
  to fire anything, is a console question. The starting bullet count was not
  changed and is out of scope.
* **Whether the requirement rewrite makes the weapon usable in game.** The bytes
  are 9/9/5/6 on eleven rows (one for the Loch Shield); that every origin's four
  stats meet those numbers is measured from `CharaInitParam`. Whether the game
  re-reads the requirement at equip time is a console question — though row 37's
  hardware test is partial evidence that it does.
* **Determinism.** The pass draws once, after the right-hand grant, and the
  mirror deliberately does not predict which weapon a seed picks. That two runs
  on one seed grant the same pair is a hardware check.
* **The picker on screen.** Geometry is asserted arithmetically against the baked
  atlas; nobody has looked at 14 weapon rows over 2 pages on a 1080p panel, and
  nobody has looked at the shortened `START WITH A LEFT WEAPON` row next to
  `START WITH A TRICK WEAPON`.

---

## 6. Hardware test handoff

### Do this first

1. Install the `.pkg`. In a world's settings, under **WEAPONS & STARTING GEAR**,
   open **START WITH A LEFT WEAPON** and tick exactly one row:
   **`HUNTER PISTOL`**. Leave every other setting off, including
   `START WITH A TRICK WEAPON`.
   `HUNTER PISTOL` is the right first choice for the same reason plain
   `SAW CLEAVER` was row 37's: it is one of the rows every origin can already
   wield as shipped (7/9/5/0), so a failure cannot be a requirements failure in
   disguise, and it is the firearm a player recognises instantly.
2. Activate the world. **Start a new character** — an existing save gains
   nothing by design.
3. Look at the character in Iosefka's Clinic, and look in the inventory.

| Observed | Meaning |
| --- | --- |
| Hunter Pistol **in the left hand** | `equip_Wep_Left` works exactly as its sibling does. Proceed |
| Hunter Pistol **in the inventory**, not in hand | the grant works and the equip does not — the outcome spec 037 D5 accepts. The feature ships; the help text's "start holding" and the progress line would need rewording |
| **Nothing at all** | the game reads `equip_Wep_Right` on these rows but not `equip_Wep_Left`. **Report before changing anything** — the deletion decision is the developer's, and the honest alternative is deleting the feature rather than growing it |
| Character creation fails, or the character is broken | halt and report |

### Then, in this order

Two first, because they are the ones that fail silently:

* Tick **`CANNON`**, then **`CHURCH CANNON`**, and confirm each is usable without
  levelling. Those are the extremes — 30 STR and 27 STR / 16 BLT as shipped — so
  they are what proves the requirement rewrite covers the heavy end of this list
  and not just the four easy rows. A "requirements not met" message here means
  the rewrite did not land.
* Tick **`LOCH SHIELD`** and confirm it arrives and equips. It is the one row
  with no upgrade tiers, so its profile covers one weapon row rather than eleven,
  and it is the row most likely to expose a wrong assumption about the tier loop.

Then:

* The granted weapon can actually be **used** — a firearm can be fired (check
  the starting Quicksilver Bullet count is enough to fire it at all), the torch
  lights, the shield blocks, the Fist of Gratia swings.
* Every origin gets it, including Waste of Skin at level 4.
* **Both features on at once**: tick one trick weapon and one left-hand weapon,
  start a new character, and confirm the character holds **both**, one in each
  hand. This is the combination the two features' shared row and eight-byte field
  gap make worth checking explicitly.
* **All three on**: add `START WITH HUNTER TOOLS` and confirm the two workshop
  tools are still in the inventory alongside both weapons, and the Hunter's Mark
  is still there.
* With several ticked, two runs on the same seed grant the same left-hand weapon
  and the rest of the world is identical; different seeds vary.
* With nothing ticked, the clinic spawn is empty-handed exactly as vanilla.
* With `RANDOMIZE STARTING GUNS` also on, the granted weapon is present **and**
  the Hunter's Dream still offers its randomized firearm choices.
* An existing save loaded with the setting on gains nothing and is undamaged.

### Capture, so the offline mirrors can be run

Save `live.log` and the **output tree** of at least one granted run and one
nothing-ticked run into `data/runs/`. Then:

```
python app/tools/left_hand_weapons_verify.py verify data/vanilla/dvdroot_ps4 <output> \
       --ticked <the 14-character line from that world's config>
python app/tools/hunter_tools_verify.py     verify data/vanilla/dvdroot_ps4 <output> \
       --granted-left 14000000
python app/tools/starting_weapons_verify.py verify data/vanilla/dvdroot_ps4 <output> \
       --granted 14000000
```

For a **both-hands** tree, add the right-hand weapon's id everywhere it is
wanted:

```
python app/tools/left_hand_weapons_verify.py verify … --ticked <14 chars> --granted-right 7000000
python app/tools/trick_weapons_verify.py    verify … --ticked <78 chars> --granted-left 14000000
python app/tools/hunter_tools_verify.py     verify … --granted 7000000 --granted-left 14000000
python app/tools/starting_weapons_verify.py verify … --granted 7000000 --granted 14000000
```

Without those flags the tools correctly report a combined run as FAILED.

---

## 7. Noticed and deliberately not fixed

* **`docs/testing.md` does not list the two new tools.** It is the index of
  available verifiers and now has two gaps (`gen_left_hand_table.py` and
  `left_hand_weapons_verify.py`). Documentation is its own stage and the brief
  scoped this pass to the report, so it was left and reported.
* **`RandomizerDefaultsStore.cpp`'s `FormatSettings` comment still says "Worst
  case today is 636 bytes".** Row 37 reported the same thing; it was already
  stale before feature 037 (the pinned figure was 635), was 737, and is now 779.
  Left alone again for the same reason — the file's scope in this pass was the
  new key — and reported again so it does not become invisible.
* **`settings_ui_verify.py` case 9's picker alignment band still measures the
  three creature tables only.** Its row regex keys on `"c\d+"` and neither
  weapon picker draws an id column, so it cannot see either new table without
  being rewritten. Row 37 reported this; it is now two tables unmeasured there
  rather than one. `ui_scroll_verify.py` covers both pickers' bands, and the
  widest left-hand row (`HUNTER BLUNDERBUSS`, 18 characters) is narrower than the
  widest trick-weapon row, which was itself measured at 653 px against a band
  sized for 791 px. Nothing is at risk; it is genuinely unmeasured.
* **`hunter_tools_verify.py`'s header note** was corrected where it said feature
  037 writes into a free slot found by search (it writes no slot); the new
  sentence says neither grant takes a slot. That was inside a comment block this
  pass was already editing.
* **The settings-UI spec's Appendix A help table gains no row.** Row 37's edit was
  scoped to §7.1 and this one matched it. `settings_ui_verify.py` checks help for
  non-emptiness and wrapping, not against Appendix A, so nothing is unpinned.
* **Row 34's open item O2 — narrowing the 22 origin rows to the live block.**
  Still open, still blocked on the same observation, and now three features
  depend on the list. Untouched.
* **`bothHandEquipable`, bit 2 of the same byte,** exists and is read by neither
  feature. Whether anything belongs in a "both hands" picker was not
  investigated; it is not in scope and the generators do not consult it.
* **The starting Quicksilver Bullet count was not changed.** Eleven of the 14
  rows are firearms, and a granted firearm is only as useful as the ammunition a
  new character has. Whether that is a problem is a hardware observation, and
  changing it would be a new backlog row.
