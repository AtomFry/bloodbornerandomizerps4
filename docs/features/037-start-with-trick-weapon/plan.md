# Plan 037 — Start With A Trick Weapon

**Status: APPROVED** — the developer approved this plan, and the Execution
Strategy below with it, on 2026-09-27. Stage D (plan review) was deliberately
skipped at the developer's request. §8 is empty; its three questions were
answered the same day and are recorded as D1-D3 in §9.

**Spec:** `docs/features/037-start-with-trick-weapon/spec.md` — **APPROVED**
(2026-09-27). Six binding decisions, §10 D1–D6, plus the version-8/9 exclusion
finding.

**Evidence:** `docs/features/037-start-with-trick-weapon/plan-evidence.md` — the
measurements, traces and rejected alternatives behind this plan.

**Plan review:** `docs/features/037-start-with-trick-weapon/plan-review.md` —
added during review.

---

## Execution Strategy

*Approving this plan approves this strategy. `CLAUDE.md` §4: milestones describe
implementation structure; gates describe when a human stops to test.*

| | |
| --- | --- |
| **Structure** | 4 functional milestones |
| **Execution** | continuous through M1–M3, then **gated**; M4 is selected by the gate's result |
| **Human test gates** | after **M3** (Required), and the final hardware test after M4 |
| **Intermediate verification** | after every milestone: `cd app && make` (clean rebuild in M2 and M3), plus the §6 automated checks that apply to that milestone |

* **Gate after M3 — Required.** M3's build is the probe. Nothing offline can
  establish whether Bloodborne grants a weapon written into
  `CharaInitParam.equip_Wep_Right` (spec §4.5), and **M4's content is chosen by
  the answer** — one of the two write routes is deleted, or the feature is
  deleted. Writing M4 before the console answers would be writing three
  milestones and throwing two away.
* No gate after M1 or M2. Neither changes what a run produces: M1 adds a
  generated table and a verifier, M2 adds a setting that nothing reads yet.

---

> **This document is the implementation contract.** §1–§7 are what the
> implementer reads, in order. §8–§10 are the record, not instructions.
> `plan-evidence.md` holds the investigation; `log.md` holds the history.

---

## 1. Objective

Add one pool-picker setting, nothing ticked by default, that grants a new
character one of Bloodborne's 78 named right-hand trick-weapon versions at
character creation, with its strength/skill/bloodtinge/arcane requirements
lowered to the origin minimum so it can be used immediately. Tick nothing and
nothing is granted; tick one and every new character gets it; tick several and
one is drawn for the run. Nothing else about the starting character, and nothing
about the Hunter's Dream, changes.

---

## 2. Approved behaviour

| #   | Behaviour | Source |
| --- | --------- | ------ |
| B1  | One new setting, label `START WITH A TRICK WEAPON`, a pool picker with none/one/many semantics and **nothing ticked** by default | spec §2, §7 |
| B2  | The picker offers **78 rows** — 26 right-hand trick weapons × plain / Uncanny / Lost — each its own independent choice | spec §2, §10 D2 |
| B3  | Every row's displayed name is the name the game's own text gives that exact weapon id, never a name assembled from a prefix | spec §4.1, §7 |
| B4  | Firearms, shields, torches, bare fists, and the unnamed unobtainable right-hand rows are not in the picker and can never be granted | spec §2, §10 D1, §10 (exclusion finding) |
| B5  | The granted weapon's requirements become strength 9, skill 9, bloodtinge 5, arcane 6 — on its base row and all ten upgrade tiers — through the existing requirement rewrite | spec §10 D3 |
| B6  | Where the granted weapon is also a `RANDOMIZE STARTING WEAPONS` coffin pick, the grant's profile is what stands, as an **explicit rule in the code** rather than a consequence of call order | spec §10 D4, §7 |
| B7  | A granted weapon arrives **in hand** if the game allows it, **in the inventory** if that is all it allows; either ships | spec §10 D5 |
| B8  | The grant is at tier +0. The picker offers versions, never upgrades | spec §5, §6 |
| B9  | Nothing else in the origin rows changes: same origin, stats, level, the starting Hunter's Mark, and all four clothing entries | spec §2, §8 |
| B10 | Takes effect at character creation only. An existing save gains nothing and is undamaged | spec §2 |
| B11 | Independent of `RANDOMIZE STARTING WEAPONS`, `RANDOMIZE STARTING GUNS`, `RANDOMIZE SHOP WEAPONS` and `START WITH HUNTER TOOLS`; any combination is valid and none loses its write | spec §2, §7 |
| B12 | State persists in the existing config; an absent or wrong-length selection line reads as **nothing ticked** | spec §7 |
| B13 | Turning the setting on must not change the rest of a run for a given seed. The same seed grants the same weapon | spec §7 |
| B14 | Ticking one weapon and nothing else is a complete request: the run does something and is not reported as "nothing is on" | spec §7 |
| B15 | If the console grants nothing by either route, the feature is **deleted**, not grown | spec §7, §10 D5 |

---

## 3. Constraints and invariants

### 3.1 Must be preserved

* **Row 5 produces the same bytes when this setting is off.** `RANDOMIZE
  STARTING WEAPONS` / `GUNS` / `SHOP WEAPONS` keep their present output exactly;
  the requirement-rewrite logic is reused, not altered.
* **Row 34 keeps its write.** `START WITH HUNTER TOOLS` writes the same origin
  rows of the same param member. Both features on must leave both writes
  present — which is why every item-slot write goes through a first-free-slot
  search and never a fixed slot index.
* **The RNG stream is unchanged for every other pass.** The weapon is drawn by
  the **last** consumer of the run's RNG, after the drop and starting-weapon
  passes, so turning this on cannot move any other roll.
* **The weapon table's order is load-bearing.** The selection is stored
  positionally, one character per row. A regenerated table in a different order
  silently remaps a saved selection onto the wrong *version* of a weapon. Order
  is frozen; a changed count must be rejected, not misapplied.
* **The archive is edited in place.** Every write is one value over another
  inside an existing row. Nothing is resized and the archive is never rebuilt.
* **Only the 22 origin rows are written.** No other `CharaInitParam` row, and no
  other member of the archive, is touched by this feature.
* **No other weapon's requirements change.** Only the granted weapon and its ten
  upgrade tiers.
* **Config compatibility both ways**: unknown keys are ignored on load, and an
  absent `trick_weapons_included` reads as nothing ticked.
* **Layering**: the table, the selection type and the grant live under
  `app/src/Randomizer/`; no SDL2 there, and no UI file learns a param offset.

### 3.2 Out of scope

* Firearms, shields, torches and bare fists (spec D1).
* The seven unnamed, unobtainable right-hand rows (`plan-evidence.md` §E4 M3).
* Upgrade tiers as picker rows; starting armour, runes, gems, vials or bullets.
* Any change to what the Hunter's Dream offers.
* **Any third route into the game.** If neither route of M3 works, finding
  another way in — `equip_Wep_Right_GenId`, the secondary item slots, an event
  script — is new work and a new backlog row (spec §6).
* Narrowing the 22 origin rows to the live block (row 34's open item O2).
* Chalice dungeons.

### 3.3 Hazards

| Hazard | What to do | Evidence |
| ------ | ---------- | -------- |
| `equip_Wep_Right` is unused on all 1700 rows of the shipped param, so the assumed route is the unexercised one | M3 writes **both** routes in one build so one hardware cycle settles both. Do not delete either before the gate | `plan-evidence.md` §E5.1 |
| `rightHandEquipable` and `leftHandEquipable` are packed **bits in one byte**, not fields | Read byte 256 and mask bit 0 / bit 1. Never read the byte as a value, and never write that byte at all | §E4 M1 |
| The four requirement values and the item counts are **single bytes**; a 4-byte write corrupts the neighbour | Write one byte each at 237/238/239/240 and at 204+slot | `StartingWeapons.cpp` header comment |
| Composing "UNCANNY " + base name gives the wrong string for four of the 26 weapons | Read every name from the FMG at its own id. The verifier asserts each row against the game's text | §E4 M5 |
| A weapon id decomposed with the wrong divisors produces 37 or 47 weapons instead of the right set | Use `fam = id/1000000`, `weapon = (id/100000)%10`, `version = (id/10000)%10`, `tier = (id/100)%100`, and derive membership from the hand bit plus obtainability — never from a count | §E4 M2 |
| `StartingWeaponLists.h` looks like a ready-made classification and is not: it is the reference's hand lists, it is 76 rows, and it misfiles a shield and a torch | Do not read it, extend it or derive from it. It stays exactly as it is for row 5 | spec §4.2 |
| `hunter_tools_verify.py` and `starting_weapons_verify.py` both assert "only member X differs"; a run with this feature on makes both report FAILED | Add the tolerance flag of §6 in M3, in the same pass. Without it a correct combined run looks like a regression during hardware testing | §E5.3 |
| `pool_verify.py`'s config-size cases are exact equalities and fail the moment a key is added | Update to 686 / 737 in M2, with the comment that narrates the arithmetic's history | §E4 M8 |
| `settings_ui_verify.py` case 3 checks the model against the settings-UI spec's §7.1 table, so a new setting fails it until that table is updated too | Update `docs/features/randomizer-settings-ui/spec.md` §7.1 and `PROSE_TO_LABEL` in the same pass as the model entry | §E5.4 |
| `PickerStrings` is aggregate-initialised; a new trailing field silently defaults to `false` at a site that forgets it | Set it explicitly at all four sites, and add the verifier case of §6 that counts them | §E5.5 |
| The console is cp1252 here, so printing a weapon name or the FMG's Japanese member name raises `UnicodeEncodeError` that reads like a parse failure | The generator writes files, never prints names; the verifier prints ASCII-uppercased names only | `docs/known-traps.md` |
| `RandomizerDefaults` grows by 78 bytes and both screens and `WorldStore` hold copies | `make clean && make` in M2 and M3 | `docs/build.md` |

---

## 4. Implementation approach

> Chosen: a generated 78-row table pinned by `--check`; a new engine pass beside
> `HunterTools`, sharing its origin-row list through a carved-out header; and an
> owner-ranked requirement writer that both this pass and row 5 write through.
> Alternatives considered and why they were rejected: `plan-evidence.md` §E3.

### 4.1 The 78 rows, and how they are derived

The set is **derived, never listed**. A row belongs to the picker when all four
hold against `data/vanilla/dvdroot_ps4`:

1. It is an `EquipParamWeapon.param` row at **tier 0** (`(id/100)%100 == 0`).
2. Byte 256 **bit 0** (`rightHandEquipable`) is set.
3. The weapon-name FMG holds a non-empty name for that exact id that is not `*`.
4. The id is sold in a `ShopLineupParam.param` row with `equipType == 0`, **or**
   appears in an `ItemLotParam.param` slot whose matching category is 1.

Tests 3 and 4 are independent and must **agree**; the generator asserts that and
fails if they do not, so neither is trusted alone and no id is ever hardcoded.

**Names** come from the `武器名.fmg` member of
`data/vanilla/dvdroot_ps4/msg/engus/item.msgbnd.dcx`, uppercased. Nothing in
`app/tools/` parses FMG yet, so a new `app/tools/fmg.py` does it — wide 64-bit
FMG, header layout and both framing equalities as measured in §E4 M4.

**Order** is by the weapon's **base (version-0) name**, alphabetically, then
version 0, 1, 2, so a weapon's three rows are adjacent: `AMYGDALAN ARM`,
`UNCANNY AMYGDALAN ARM`, `LOST AMYGDALAN ARM`, `BEAST CLAW`, … Frozen.

`app/tools/gen_weapon_table.py` owns the derivation and writes
`app/src/Randomizer/TrickWeaponTable.h`: a `std::array<ModelPoolEntry, 78>` plus
`const int kTrickWeaponCount = 78;`, with the same `--check` contract and
frozen-order warning `gen_pool_table.py` carries. Each row is
`{ "<decimal weapon id>", "<NAME>", 1 }`; the id goes in `model` because the
picker keys and logs on that field, and the engine converts it with `strtol`.
Emit no second array of ids.

### 4.2 The settings chain

One selection, **no boolean**: the setting's only state is its pool, so there is
no toggle field and no toggle key anywhere.

| Site | What to add |
| --- | --- |
| `EnemyPoolSelection.h` | `typedef ModelPoolSelection<kTrickWeaponCount, false> TrickWeaponSelection;` beside the other three. `false` is the fail-safe: fresh struct, absent key and wrong-length value all read as nothing ticked |
| `RandomizerDefaults.h` | `TrickWeaponSelection trickWeapons;` |
| `RandomizerDefaultsStore.cpp` | key **`trick_weapons_included`** — this exact name, since the arithmetic depends on its length — in `FormatSettings` **and** `ApplySettingKey`. 78 characters, 102 bytes with key and newline: settings block 584 → **686**, worst-case `defaults.cfg` 635 → **737**, inside `char buf[1024]` |
| `SettingsModel.h` | `SettingId::StartWithTrickWeapon`, `SettingKind::TrickWeaponPool` |
| `SettingsModel.cpp` | the `kSettings` entry, category `WeaponsGear`, declared **after** `StartWithHunterTools`; label `START WITH A TRICK WEAPON`; `SettingValueText` → `N OF 78`; the four `Selection*` functions gain the fourth case |
| help text | `"Start holding one of the trick weapons you tick here. Tick several and one is drawn for the run. Its requirements are lowered so you can use it at once."` |

Both screens need **no change**: they reach a picker entirely through
`IsDrillIn` and the four `Selection*` functions.

### 4.3 The picker

`PickerStrings` gains one field, `bool showRowId`, and `ModelPicker.cpp`'s
`RowLabel` omits the id column when it is false. The three shipped pickers set
it **`true`** explicitly — they must render pixel for pixel as they do today —
and the weapon picker sets it `false`.

`kTrickWeaponsStrings`, declared beside the other three:

| field | value |
| --- | --- |
| heading | `START WITH A TRICK WEAPON` |
| instruction | `TICK ANY NUMBER - ONE IS DRAWN FOR THE RUN` |
| flagOn / flagOff | `YES` / `NO` |
| verbAll / verbNone | `ENABLE ALL` / `DISABLE ALL` |
| allVerb / noneVerb | `ALL` / `NONE` |
| showRowId | `false` |

The instruction line puts the list on the **11-row layout**, so 78 rows is 8
pages. No geometry constant moves (§E4 M7).

### 4.4 The engine pass

`app/src/Randomizer/TrickWeaponGrant.{h,cpp}` — its own file beside
`HunterTools`, not an extension of it (§E2.1). Two headers are carved out so
nothing is duplicated:

* **`CharaInitRows.h`** — the 22 origin row ids, the row size, the item-slot
  offsets, and `FindRow` / `RowHasItem` / `FirstEmptySlot`, moved **verbatim**
  out of `HunterTools.cpp`, which then includes it. No value changes, so row
  34's output stays byte-identical.
* **`WeaponRequirements.h`** — the four requirement offsets, the +100n tier
  stride, `enum class ReqOwner { CoffinSlot = 0, Grant = 1 }`, and a
  `WeaponRequirementWriter` that records the owner rank per weapon id and
  **applies a profile only when the incoming rank is at least the recorded
  rank**. `StartingWeapons.cpp`'s five coffin profiles go through it as
  `CoffinSlot`; the grant as `Grant`. That is B6 — the grant wins under either
  call order.

The pass, given the decompressed archive, `CharaInitParam.param`,
`EquipParamWeapon.param`, the selection, the RNG and the writer:

1. Collect the ticked table rows into a candidate list of weapon ids. Empty list
   → return at once, having written nothing and drawn nothing.
2. Draw one id uniformly from it. **One draw, and it is the run's last.**
3. Apply the profile `{ strength 9, skill 9, bloodtinge 5, arcane 6 }` to that
   id through the writer as `ReqOwner::Grant`.
4. For each of the 22 origin rows: bounds-check the whole 300-byte row, then
   write the grant (§4.5), leaving every other byte of the row alone.
5. Log the chosen id and name, the rows written and the slots used.

In `StepItemData` the pass runs **last**, after the hunter-tools grant, and
locates `CharaInitParam.param` and `EquipParamWeapon.param` itself, per feature,
as the existing code does. The `WeaponRequirementWriter` is created once there
and passed to both the starting-weapon pass and this one.
`EnemyRandomizerOptions` gains `TrickWeaponSelection trickWeapons`;
`AnyParamFeature()` gains `trickWeapons.CountEnabled() > 0`;
`EnemyRandomizerResult` gains `trickWeaponGranted` (the id, 0 for none) and
`trickWeaponRowsChanged`.

`WorldActivation.cpp` maps `options.trickWeapons = run.trickWeapons;` in the
frozen field order and adds `run.trickWeapons.CountEnabled() > 0` to the
`anythingOn` chain — that is B14. `WorldEditorScreen.cpp` adds one progress line
when something was granted, naming the weapon from the table, and adds the
setting to the item-data-archive line's condition.

### 4.5 The two routes, and what M3 writes

M3's build writes **both** candidate routes into every origin row, so one
hardware cycle tells them apart:

| Route | Write | Weapon |
| --- | --- | --- |
| **A — equip** | `equip_Wep_Right` (offset 16, s32) | the drawn weapon |
| **B — inventory** | first free `item_NN` slot (id at 124 + 4·slot, count **1 byte** at 204 + slot) | the fixed probe weapon `22000000`, Threaded Cane |

Route B's weapon is deliberately **fixed and different**, so the observation is
unambiguous: a Saw Cleaver means route A, a Threaded Cane means route B, both
means both. Leave `equip_Wep_Right_GenId` and its three siblings at `-1`
(§3.2). M4 deletes the losing write and the probe constant.

### 4.6 The verification mirror

`app/tools/trick_weapons_verify.py`, a new tool rather than an extension of
`pool_verify.py` (§E3). It imports the derivation from `gen_weapon_table.py`,
re-derives membership a second independent way (§6), takes every offset from
`param_offsets.py`, and parses the C++'s offsets out of the source rather than
restating them — the shape `hunter_tools_verify.py` already uses.

### What this reuses

§5 lists every file. What it does not say is how four of them are treated:

| Existing code | Treatment |
| --- | --- |
| `StartingWeaponLists.h` | **not used**, not extended, not derived from |
| `UI/SetupDefaultsScreen.*`, `UI/WorldEditorScreen.h` | **unchanged** — they host any picker generically |
| `tools/param_offsets.py`, `tools/gen_pool_table.py` | **reused as-is**: every offset comes from the first, and the `--check` contract and `RENDERABLE` set from the second |
| `ModelPoolSelection.h`, `ModelPicker.cpp` | reused for the selection and the list; only the one `showRowId` field is new |

---

## 5. Files and changes

| File | Change | M |
| ---- | ------ | - |
| `app/tools/fmg.py` | **new** — the wide-FMG reader and its framing validation | 1 |
| `app/tools/gen_weapon_table.py` | **new** — derivation, frozen order, `--check` | 1 |
| `app/src/Randomizer/TrickWeaponTable.h` | **new, generated** — 78 rows, `kTrickWeaponCount` | 1 |
| `app/tools/trick_weapons_verify.py` | **new** — `list`, `verify`, `selftest` | 1, 3 |
| `app/src/Randomizer/EnemyPoolSelection.h` | `TrickWeaponSelection` typedef; include the table | 2 |
| `app/src/Randomizer/RandomizerDefaults.h` | `TrickWeaponSelection trickWeapons;` | 2 |
| `app/src/Randomizer/RandomizerDefaultsStore.cpp` | `trick_weapons_included` in `FormatSettings` and `ApplySettingKey` | 2 |
| `app/src/UI/SettingsModel.h` | `SettingId::StartWithTrickWeapon`, `SettingKind::TrickWeaponPool` | 2 |
| `app/src/UI/SettingsModel.cpp` | the `kSettings` entry and the five `switch` cases | 2 |
| `app/src/UI/ModelPicker.h` | `PickerStrings::showRowId`; `kTrickWeaponsStrings`; `true` at the three existing sites | 2 |
| `app/src/UI/ModelPicker.cpp` | `RowLabel` honours `showRowId` | 2 |
| `app/tools/pool_verify.py` | settings block 584 → 686, worst case 635 → 737; new-key case; `showRowId` case | 2 |
| `app/tools/ui_scroll_verify.py` | a `Trick weapon picker` screen entry, 78 rows, instruction layout | 2 |
| `app/tools/settings_ui_verify.py` | fourth selection field and kind; `PROSE_TO_LABEL`; the config round-trip's value shape | 2 |
| `docs/features/randomizer-settings-ui/spec.md` | §7.1 table gains `Start With A Trick Weapon` in Weapons & Starting Gear | 2 |
| `app/src/Randomizer/CharaInitRows.h` | **new** — origin rows, row size, slot offsets and helpers moved from `HunterTools.cpp` | 3 |
| `app/src/Randomizer/HunterTools.cpp` | include the new header; delete the moved definitions | 3 |
| `app/tools/hunter_tools_verify.py` | parse the moved constants from the new header; `--granted <id>` tolerance | 3 |
| `app/src/Randomizer/WeaponRequirements.h` | **new** — offsets, `ReqOwner`, `WeaponRequirementWriter` | 3 |
| `app/src/Randomizer/StartingWeapons.h/.cpp` | take a `WeaponRequirementWriter&`; route the five profiles through it as `CoffinSlot` | 3 |
| `app/tools/starting_weapons_verify.py` | `--granted <id>` tolerance for SW-I1/I6/I7 | 3 |
| `app/src/Randomizer/TrickWeaponGrant.h/.cpp` | **new** — the draw, the profile, the two writes | 3 |
| `app/src/Randomizer/EnemyRandomizer.h` | the option, `AnyParamFeature()`, two result counters | 3 |
| `app/src/Randomizer/EnemyRandomizer.cpp` | the writer, the pass call last in `StepItemData` | 3 |
| `app/src/Game/WorldActivation.cpp` | options mapping; `anythingOn` | 3 |
| `app/src/UI/WorldEditorScreen.cpp` | the progress line; the item-data line's condition | 3 |
| `app/tools/worlds_verify.py` | `WIZARD_OPTIONS_MAPPING` and `WIZARD_RUN_DECISION` gain the field | 3 |
| `app/src/Randomizer/TrickWeaponGrant.cpp` | delete the losing route and the probe constant | 4 |
| `app/tools/trick_weapons_verify.py` | assert the shipped route only | 4 |

No Makefile change: it compiles `.cpp` recursively and tracks header
dependencies. An existing `defaults.cfg` needs no migration — the new key is
absent and reads as nothing ticked. No saved world is invalidated.

---

## 6. Verification

### Build

`cd app && make`, and **`make clean && make` in M2 and M3** — M2 grows
`RandomizerDefaults` by 78 bytes, M3 changes `StartingWeapons`'s signature. The
`.pkg` must be produced.

### Automated

| Check | Command | Asserts |
| ----- | ------- | ------- |
| Table is current | `python app/tools/gen_weapon_table.py data/vanilla/dvdroot_ps4 --check` | the committed header is what the vanilla tree produces now |
| Table listing | `python app/tools/trick_weapons_verify.py list data/vanilla/dvdroot_ps4` | prints 78 rows plus the excluded rows and why; not a pass/fail gate |
| The mirror | `python app/tools/trick_weapons_verify.py selftest data/vanilla/dvdroot_ps4` | T1–T14 below |
| Output tree | `python app/tools/trick_weapons_verify.py verify <vanilla> <output>` | G1–G6 below |
| Row 34 unaffected | `python app/tools/hunter_tools_verify.py selftest data/vanilla/dvdroot_ps4` | every existing case still passes after the carve-out |
| Row 5 unaffected | `python app/tools/starting_weapons_verify.py selftest data/vanilla/dvdroot_ps4` | every existing case still passes |
| Config and picker strings | `python app/tools/pool_verify.py selftest data/vanilla/dvdroot_ps4` | the 686/737 arithmetic, the new key in load and save, and that all four `PickerStrings` set `showRowId` explicitly |
| Settings model | `python app/tools/settings_ui_verify.py` | the fourth selection field has exactly one entry, the row fits its pane, help is non-empty, and the model matches the settings-UI spec's §7.1 |
| Picker geometry | `python app/tools/ui_scroll_verify.py` | 78 rows over 8 pages on the instruction layout, clear of the count line and the footer |
| Worlds parity | `python app/tools/worlds_verify.py selftest` | the options mapping and run decision carry the new field; a recipe round-trips the 78-character line |

`trick_weapons_verify.py selftest` must assert, against the vanilla tree:

| # | Assertion | M |
| - | --------- | - |
| T1 | The name FMG frames validly by both checks of §4.1, and yields 1408 entries | 1 |
| T2 | Derivation by name gives exactly **78** rows | 1 |
| T3 | Derivation by obtainability gives exactly **78** rows | 1 |
| T4 | T2 and T3 name the same rows, and every other candidate fails **both**. Report the excluded ids; do not assert a list of them | 1 |
| T5 | The committed table is 78 rows in §4.1's order, names distinct, every name renderable by `Font8x8.cpp`, widest row plus flag word inside 71 characters | 1 |
| T6 | Every row's name is the uppercased FMG string **for that exact id** — nothing was composed from a prefix | 1 |
| T7 | All 78 have all ten `+100n` tiers, so the profile covers eleven rows per weapon | 1 |
| T8 | The ten origins' component-wise minimum is 9/9/5/6; all 78 are wieldable under the profile; **39 of 78 are wieldable by no origin as shipped** | 1 |
| T9 | All three versions of each of the 26 weapons carry identical requirements | 1 |
| T10 | Nothing ticked: the simulated pass leaves the archive byte-identical | 3 |
| T11 | One ticked: all 22 origin rows carry that **exact version id** by the route under test; the Hunter's Mark and the four clothing entries survive; every other byte of each 300-byte row is intact; no non-origin row and no other member differs | 3 |
| T12 | Both features on: the hunter-tools write and the grant both survive, neither taking the other's slot | 3 |
| T13 | **B6, both ways round**: coffin-then-grant and grant-then-coffin on the same weapon both leave the grant's four bytes | 3 |
| T14 | A 78-character selection line round-trips; 77 and 79 leave nothing ticked | 3 |

`trick_weapons_verify.py verify <vanilla> <output>` must assert, for a real
tree: **G1** only `CharaInitParam.param` and `EquipParamWeapon.param` differ;
**G2** within `CharaInitParam` only the 22 origin rows differ; **G3** within an
origin row only the fields the route writes differ; **G4** all 22 rows name the
same weapon id and it is one of the ticked rows; **G5** that weapon and its ten
tiers carry the profile and no other weapon's requirement bytes changed;
**G6** the member list is unchanged.

**A mirror pins the rules, not the C++ implementation of them.** It cannot show
that the engine consults the table correctly, and it deliberately does not
predict *which* weapon a seed draws (§E7). Determinism is a hardware check.

### Hardware

The implementer cannot run any of this.

**The gate after M3 — the probe.** Tick exactly one row, plain `SAW CLEAVER`,
everything else off. Build, install, **start a new character**, and look at the
character in Iosefka's Clinic.

| Observed | Meaning | M4 becomes |
| --- | --- | --- |
| Saw Cleaver **in hand** | route A works as intended | keep A, delete B |
| Saw Cleaver **in the inventory** (no Threaded Cane) | route A grants, the equip does not — accepted under D5 | keep A, delete B, reword "in hand" to "in the inventory" |
| Saw Cleaver in hand **and** a Threaded Cane in the inventory | both routes work | keep A, delete B |
| **Threaded Cane** in the inventory, no Saw Cleaver anywhere | route A is dead, route B works — accepted under D5 | keep B, delete A, grant the drawn weapon through B, reword to "in the inventory" |
| **Nothing at all** | the game reads neither route on these rows | **delete the feature** (spec §7). Report before deleting anything |
| Character creation fails, or the character is broken | the two writes cannot be told apart | halt and report. Re-probe with one write at a time |

Once the probe is positive, the remaining checks are **spec §8's hardware list
verbatim**, in its order, with its pass conditions. Do two of them first,
because they are the ones that fail silently: a ticked **Lost** or **Uncanny**
row must grant *that version*, named as that version; and `LOGARIUS' WHEEL` and
`KOS PARASITE` must be usable without levelling.

Capture `live.log` and the output tree of one granted and one nothing-ticked run
into `data/runs/`, so `trick_weapons_verify.py verify` can be run against both.

---

## 7. Milestones and stop conditions

### Milestone 1 — the 78 rows, derived and pinned

**Goal.** The set of grantable weapons and their in-game names exist as a
generated, verified table. No behaviour changes.

**Changes**, in order:

| # | Change | Done when |
| - | ------ | --------- |
| 1 | Write `app/tools/fmg.py` | it reads the weapon-name FMG, validates both framing checks, and returns 1408 entries including `7000000 → "Saw Cleaver"` |
| 2 | Write `app/tools/gen_weapon_table.py` with §4.1's four tests and frozen order | it derives 78 rows, asserts tests 3 and 4 agree on every candidate, and refuses to write if any name is unrenderable |
| 3 | Generate `app/src/Randomizer/TrickWeaponTable.h` | `--check` passes against the committed file and the header carries the order warning |
| 4 | Write `app/tools/trick_weapons_verify.py` with `list` and `selftest` | T1–T9 pass, and `list` prints the 78 rows plus each excluded row with its reason |

**Invariants**: the frozen order; names read and never composed; no hardcoded id
list anywhere (§3.1). **Verification:** build; `gen_weapon_table.py --check`;
`trick_weapons_verify.py selftest` (T1–T9). **Then continue to M2.**

### Milestone 2 — the setting a player can set

**Goal.** A player can tick weapons on both screens, the choice persists, and
the run ignores it. Zero change to any run's output.

**Changes**, in order:

| # | Change | Done when |
| - | ------ | --------- |
| 1 | The `TrickWeaponSelection` typedef and the `RandomizerDefaults` field | a fresh struct reads nothing ticked |
| 2 | `trick_weapons_included` in both halves of `RandomizerDefaultsStore.cpp` | a config without the key loads with nothing ticked, a saved file round-trips 78 characters, and a wrong-length line leaves nothing ticked |
| 3 | The id, the kind, the `kSettings` entry after `START WITH HUNTER TOOLS`, and the five `switch` cases | `SettingValueText` reads `0 OF 78` and `IsDrillIn` is true for it |
| 4 | `PickerStrings::showRowId` with `true` at the three existing sites, the `RowLabel` change, and `kTrickWeaponsStrings` | the three shipped pickers still draw their id column and the new one does not |
| 5 | `docs/features/randomizer-settings-ui/spec.md` §7.1 and `PROSE_TO_LABEL` | `settings_ui_verify.py` case 3 passes |
| 6 | `pool_verify.py` (686 / 737, the new key, the `showRowId` case), `ui_scroll_verify.py` (the 78-row picker entry), `settings_ui_verify.py` (the fourth selection field and kind, the config round-trip's value shape) | all three pass with no unrelated case changing state |

**Invariants**: the fail-safe polarity; config compatibility both ways; the three
shipped pickers render unchanged; no screen file learns a param offset (§3.1).
**Verification:** `make clean && make`; `pool_verify.py selftest`;
`settings_ui_verify.py`; `ui_scroll_verify.py`; `worlds_verify.py selftest`;
`trick_weapons_verify.py selftest` still passing. **Then continue to M3.**

### Milestone 3 — the grant, and the probe build

**Goal.** A run with a weapon ticked writes that weapon into every origin row by
both candidate routes and lowers its requirements, and the archive is provably
otherwise untouched.

**Changes**, in order:

| # | Change | Done when |
| - | ------ | --------- |
| 1 | Carve `CharaInitRows.h` out of `HunterTools.cpp` and include it there | no constant's value changed, `HunterTools.cpp` declares none of them itself, and `hunter_tools_verify.py selftest` passes against the new header |
| 2 | `WeaponRequirements.h` — `ReqOwner` and the owner-ranked writer, with the four offsets and the tier loop moved out of `StartingWeapons.cpp` | the writer refuses a `CoffinSlot` profile for a weapon already claimed by `Grant`, in either call order |
| 3 | Route `StartingWeapons.cpp`'s five profiles through the writer | `starting_weapons_verify.py selftest` passes unchanged and no byte of row 5's output moves with the grant off |
| 4 | `TrickWeaponGrant.{h,cpp}` — candidate list, single draw, `Grant`-ranked profile, both routes of §4.5 over the 22 rows, full-row bounds check | an empty selection returns having written nothing and drawn nothing, and route B uses `FirstEmptySlot` |
| 5 | The option, `AnyParamFeature()`, the two result counters, and the call last in `StepItemData` with the writer shared with the starting-weapon pass | the pass is the last RNG consumer in the run |
| 6 | The `WorldActivation.cpp` mapping and `anythingOn` term, and the `WorldEditorScreen.cpp` progress line naming the weapon | a weapon-only world does not report "no randomizer settings are on" |
| 7 | Extend `trick_weapons_verify.py` with T10–T14 and `verify`; add `--granted <id>` to `hunter_tools_verify.py` and `starting_weapons_verify.py` | T13 passes both ways round and both older tools pass with and without the flag |
| 8 | `worlds_verify.py`'s two frozen lists | it passes |

**Invariants**: row 5 byte-identical with the grant off; row 34 keeps its write;
one draw and it is last; only the 22 origin rows; only the granted weapon's
requirements; in-place edits only (§3.1). **Verification:**
`make clean && make` and every automated check in §6.

**On completion.** **Stop.** Hand off the probe of §6 and do not begin M4.

### Milestone 4 — settle the route

**Goal.** The feature ships on the route the console supports, with no dead code
and no misleading wording — or it is deleted.

**Changes** depend on the gate's result; §6's table says which branch applies.
In every surviving branch, in order:

| # | Change | Done when |
| - | ------ | --------- |
| 1 | Delete the losing route's write and the probe constant; the surviving route grants the **drawn** weapon | `TrickWeaponGrant.cpp` has exactly one write per origin row and no fixed weapon id |
| 2 | Update the header comment, the help text and the progress line to say what actually happens — "in hand" or "in your inventory" | no user-facing string promises the route that lost |
| 3 | Narrow `trick_weapons_verify.py`'s T11 and `verify`'s G3 to the shipped route | `selftest` passes and G3 rejects a tree carrying the deleted write |

If the probe granted **nothing at all**: stop, report, and take the deletion
decision with the developer before removing anything — B15 is a spec rule, not a
licence to delete four milestones of work unasked.

**Verification:** `make clean && make`; every check in §6; then spec §8's full
hardware list.

### Stop conditions

Halt and report rather than deciding, if any of these occur:

* the build fails in a way the plan did not anticipate, or only after a clean
  rebuild;
* `gen_weapon_table.py` derives anything other than 78 rows, or its two
  membership tests disagree — that means the derivation is wrong, and no
  adjustment to a threshold or a list fixes it;
* any name in the table is unrenderable, or any row's name is not the FMG
  string for that exact id;
* `hunter_tools_verify.py` or `starting_weapons_verify.py` changes state in any
  case other than the tolerance ones §6 names;
* T13 fails, or passes only in one call order — precedence is then still
  ordering and B6 is unmet;
* the row count, and therefore the config line's length, would have to change;
* the probe result is not one of the six rows of §6's table;
* an implementation decision would contradict the spec or §2 — including any
  temptation to add a firearm, to include an unnamed row, to grant an upgrade
  tier, or to make the setting a toggle with a pool beside it;
* the change needs a file not listed in §5;
* a §3.1 invariant cannot be preserved.

---

## 8. Open questions

Empty. Its three questions were answered by the developer on 2026-09-27 and are
recorded as D1-D3 in §9; `log.md` holds how they were put and answered.

---

## 9. Decisions

| #   | Date | Decision | Taken by |
| --- | ---- | -------- | -------- |
| P1  | 2026-09-27 | The 78 rows are **derived** from the hand bit, the game's own text and obtainability, and pinned by a generator with `--check`. No id is ever hardcoded | planner |
| P2  | 2026-09-27 | Table order is base-weapon name, then version 0/1/2, so a weapon's three rows are adjacent. Frozen | planner |
| P3  | 2026-09-27 | A new `app/tools/fmg.py` owns FMG parsing; `gen_weapon_table.py` owns the derivation; `trick_weapons_verify.py` owns the assertions. `gen_pool_table.py` is not extended | planner |
| P4  | 2026-09-27 | The setting is a picker with **no toggle**; its config key is `trick_weapons_included`, 78 characters, and nothing ticked is the fail-safe | planner |
| P5  | 2026-09-27 | The picker draws the name only. `PickerStrings` gains `showRowId`, explicitly `true` at the three shipped sites | planner |
| P6  | 2026-09-27 | The grant is its own engine file beside `HunterTools`, sharing the origin rows through a carved-out `CharaInitRows.h` | planner |
| P7  | 2026-09-27 | D4 is implemented as an **owner-ranked requirement writer** shared by row 5 and the grant, so the grant wins in either call order | planner |
| P8  | 2026-09-27 | The pass runs **last** in `StepItemData` and draws once, so turning the setting on moves no other roll | planner |
| P9  | 2026-09-27 | M3 writes both candidate routes, with a fixed distinguishable weapon on the inventory route, so one hardware cycle settles D5 | planner, confirmed by D1 |
| P10 | 2026-09-27 | Four milestones, continuous through M3, with a **Required** gate after M3 because M4's content is chosen by the probe | planner |

The developer's answers to §8, all taken 2026-09-27:

| Ref | Date | Decision | Source |
| --- | ---- | -------- | ------ |
| D1  | 2026-09-27 | **M3 writes both routes**, a different weapon on each so the observation is unambiguous. One hardware cycle settles the route instead of two; an ambiguous failure is re-probed one write at a time per §6 | developer |
| D2  | 2026-09-27 | **The readiness summary is left alone.** `EnabledToggleCount / ToggleCount` keeps meaning "toggles that are on", so a world whose only setting is a ticked weapon still reads `0 OF 15 ON`. The picker's own row and the progress line carry it instead | developer |
| D3  | 2026-09-27 | **The origin-row list is carved into a shared header.** It changes no value and no byte of row 34's output, and keeps the eventual narrowing a one-place edit | developer |

---

## 10. Changes during implementation

M1–M3 implemented 2026-09-27. Everything in §5's file table was built as listed;
the entries below are the differences and the choices §1–§7 left open. Full
account in `implementation-report.md`.

| Date | Change | Reason |
| ---- | ------ | ------ |
| 2026-09-27 | `ui_scroll_verify.py`'s new entry is named **`Weapon picker`**, not §5's `Trick weapon picker` | That file prints its screen names through `%-16s`; a 19-character name breaks the column every other row is aligned to. Same geometry, same 78 rows, same instruction layout |
| 2026-09-27 | `WeaponRequirements.h` also carries an `inline FindWeaponRow()` | §4.4 lists the offsets, the stride, `ReqOwner` and the writer; the writer needs a row lookup and neither `StartingWeapons.cpp`'s anonymous `FindRow` nor `CharaInitRows.h`'s is reachable from that header. A distinct name so no translation unit sees two |
| 2026-09-27 | `TrickWeaponGrant.h` also declares `TrickWeaponName(int32_t)` | §4.4 has `WorldEditorScreen.cpp` name the weapon "from the table". Exposing the lookup from the engine keeps the table out of a UI file, which `CLAUDE.md` §6 wants |
| 2026-09-27 | `TrickWeaponGrantResult` carries four counters beyond §4.4's two (`slotsWritten`, `reqRowsWritten`, `rowsMissing`, `rowsFull`) | Mirrors `HunterToolsResult` field for field, and they are what the log line reports. Only the two §4.4 names are copied into `EnemyRandomizerResult` |
| 2026-09-27 | `trick_weapons_verify.py verify` takes an optional `--ticked <78-char line>` | §6's command is unchanged and still works. Without a selection G4 can only require the granted weapon to be one of the 78 rows; with the line it can require it to be one of the **ticked** rows, which is what G4 actually says |
| 2026-09-27 | `SettingsModel.cpp`'s "nineteen entries scanned once per draw" now reads "twenty" | One stale count in a comment about the table this milestone grew, in a file §5 already lists |
| 2026-09-27 | **Not done:** `RandomizerDefaultsStore.cpp`'s `FormatSettings` comment still says "Worst case today is 636 bytes" | Already stale before this feature (the pinned figure was 635) and now 737. §5 scopes that file to the new key, so it was left alone and reported rather than tidied |
| 2026-09-27 | **Not done:** `settings_ui_verify.py` case 9's picker alignment band still measures the three creature tables only | Its row regex keys on `"c\d+"`, and the weapon picker draws no id column, so the case cannot see the new table without being rewritten. Not in §5, and `plan-evidence.md` M7 measured the widest weapon row at 653 px against the 791 px the band was sized for. Reported, not fixed |
| 2026-09-27 | **Not done:** the settings-UI spec's Appendix A help table gains no row | §5 scopes that spec edit to §7.1. `settings_ui_verify.py` checks help for non-emptiness and wrapping, not against Appendix A, so nothing is unpinned by leaving it |

M4 implemented 2026-09-27, after the probe answered on hardware: two rows were
ticked and the new character spawned in Iosefka's Clinic **holding** the drawn
weapon, so **route A won** and §6's first branch applied — keep A, delete B.
Both of M4's changes were made as §7 lists them; the entries below are the
differences and the choices §1–§7 left open.

| Date | Change | Reason |
| ---- | ------ | ------ |
| 2026-09-27 | `TrickWeaponGrantResult` **loses** two of the four extra counters added in M3 (`slotsWritten`, `rowsFull`) | Both counted route B's inventory write. With route B deleted they could only ever report 0. The two §4.4 names and `reqRowsWritten` / `rowsMissing` stay; nothing in `EnemyRandomizerResult` changes |
| 2026-09-27 | M4's step 2 required **no** change to the help text or the progress line | The help already read "Start holding one of the trick weapons you tick here" and the progress line reads `STARTING WITH <NAME>`. Route A won *in hand*, so no user-facing string promised the losing route. Only `TrickWeaponGrant.h`'s header comment and one sentence of its "what the player sees" paragraph were reworded, plus `TrickWeaponGrant.cpp`'s offset comment |
| 2026-09-27 | `trick_weapons_verify.py`'s two route-shape cases (§10's M3 entry 3.5) become **four** M4 cases, and T11 gains a negative case for the deleted write | §7 step 3 asks for `selftest` to pass and G3 to reject a tree carrying the deleted write. The four assert: route A writes `weapon`; exactly one `WriteI32LE` in the pass; none of `kProbeInventoryWeapon` / `kItemIdBase` / `kItemNumBase` / `RowHasItem` / `FirstEmptySlot` survives in it; and no 7-or-8-digit weapon id is spelled anywhere in it. `selftest` is 53 cases, up from 50 |
| 2026-09-27 | **G3 now rejects an output tree that also had `START WITH HUNTER TOOLS` on** | Narrowing G3 to the shipped route means the item slots are no longer an allowed difference, and the deleted write was an item-slot write — so "reject the deleted write" and "tolerate row 34's write" cannot both hold in one predicate. `hunter_tools_verify.py --granted <id>` covers the combined direction; `trick_weapons_verify.py verify` now describes a grant-only tree. Recorded in the tool's own comment at the predicate |
| 2026-09-27 | T12 **tightened**: the item array of every origin row must now be exactly the Hunter's Mark plus the two workshop tools | The grant takes no slot at all, so "neither feature takes the other's slot" can be asserted as an exact array rather than as three presence checks |
| 2026-09-27 | **Not done:** `hunter_tools_verify.py`'s header note still says "Feature 037 writes into a FREE slot found by search rather than a fixed index" | Now false — feature 037 writes no slot. §5 does not list that file for M4 and its `--granted` tolerance already modelled route A only, so it was left and reported rather than tidied |
| 2026-09-27 | `TrickWeaponGrant.cpp`'s bounds-check comment reworded | It said "the fields written below sit up to 213 bytes in", which was route B's item slots. Only four bytes are written now. The **check itself is unchanged** — still the whole 300-byte row, per §4.4 step 4 — and the comment now gives the reason (the row is the unit the format guarantees) rather than a stale offset |
