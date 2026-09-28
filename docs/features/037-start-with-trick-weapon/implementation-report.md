# Implementation Report 037 — Start With A Trick Weapon — milestones 1, 2, 3 and 4

**Status: READY FOR HARDWARE TESTING** — all four milestones built. The Required
gate after M3 was answered on the console on 2026-09-27 and M4 was implemented
from its result; spec §8's full hardware list remains.

**Plan:** `docs/features/037-start-with-trick-weapon/plan.md` — milestones 1–4

> **Sections 1–7 below record milestones 1–3 as they stood at the gate and are
> left unaltered.** Milestone 4 is section 8 onward. Where the two disagree — the
> probe, route B, the two deleted counters — section 8 is current.

**Spec:** `docs/features/037-start-with-trick-weapon/spec.md`

**Implemented:** 2026-09-27

---

## 1. What was built

All three milestones completed. The 78 grantable trick-weapon versions are now a
generated, twice-derived, `--check`-pinned table; `START WITH A TRICK WEAPON` is
a real pool-picker setting on both settings screens that persists in
`defaults.cfg` and gates a run on its own; and a run with a weapon ticked writes
that weapon into all 22 CharaInitParam origin rows by **both** candidate routes
and lowers its requirements to 9/9/5/6 on the base row and all ten upgrade tiers,
through a shared owner-ranked writer that makes the grant beat a coffin pick in
either call order. The `.pkg` builds clean from `make clean` and every automated
check in the plan's §6 passes. Nothing about whether Bloodborne *reads* either
route has been established, and cannot be offline — that is the gate.

### Milestone 1 — the 78 rows, derived and pinned

| # | Planned change | Status | Where |
| - | -------------- | ------ | ----- |
| 1 | `app/tools/fmg.py` | done | `app/tools/fmg.py` — wide 64-bit FMG, both framing equalities checked, 1408 ids, `7000000 → "Saw Cleaver"` |
| 2 | `app/tools/gen_weapon_table.py` with §4.1's four tests and frozen order | done | derives 78 from 85 candidates, asserts the name test and the obtainability test agree on every candidate, refuses to write an unrenderable name |
| 3 | `app/src/Randomizer/TrickWeaponTable.h` | done, generated | 78 rows, `kTrickWeaponCount = 78`, frozen-order warning in the header, `--check` passes |
| 4 | `app/tools/trick_weapons_verify.py` with `list` and `selftest` | done | T1–T9 pass; `list` prints the 78 rows and all seven excluded rows with their reasons |

### Milestone 2 — the setting a player can set

| # | Planned change | Status | Where |
| - | -------------- | ------ | ----- |
| 1 | `TrickWeaponSelection` typedef and the `RandomizerDefaults` field | done | `EnemyPoolSelection.h`, `RandomizerDefaults.h` — `ModelPoolSelection<kTrickWeaponCount, false>`, no boolean anywhere |
| 2 | `trick_weapons_included` in both halves of the store | done | `RandomizerDefaultsStore.cpp` — `FormatSettings` and `ApplySettingKey` |
| 3 | The id, the kind, the `kSettings` entry after `START WITH HUNTER TOOLS`, the five `switch` cases | done | `SettingsModel.h/.cpp` — `SettingId::StartWithTrickWeapon`, `SettingKind::TrickWeaponPool`, `0 OF 78`, `IsDrillIn` true |
| 4 | `PickerStrings::showRowId`, `true` at the three existing sites, the `RowLabel` change, `kTrickWeaponsStrings` | done | `ModelPicker.h/.cpp` |
| 5 | settings-UI spec §7.1 and `PROSE_TO_LABEL` | done | `docs/features/randomizer-settings-ui/spec.md` §7.1, `settings_ui_verify.py` |
| 6 | `pool_verify.py` (686/737, new key, `showRowId`), `ui_scroll_verify.py`, `settings_ui_verify.py` | done | see §4; no unrelated case changed state |

### Milestone 3 — the grant, and the probe build

| # | Planned change | Status | Where |
| - | -------------- | ------ | ----- |
| 1 | Carve `CharaInitRows.h` out of `HunterTools.cpp` | done | new `CharaInitRows.h`; `HunterTools.cpp` includes it and declares none of the moved names. No value changed |
| 2 | `WeaponRequirements.h` with `ReqOwner` and the owner-ranked writer | done — plus a `FindWeaponRow` helper (§2) | new `WeaponRequirements.h` |
| 3 | Route row 5's five profiles through the writer | done | `StartingWeapons.h/.cpp` take `WeaponRequirementWriter&`, write as `ReqOwner::CoffinSlot` |
| 4 | `TrickWeaponGrant.{h,cpp}` — candidate list, single draw, `Grant` profile, both routes, full-row bounds check | done | new files; empty selection returns before the RNG and before any write; route B uses `FirstEmptySlot` |
| 5 | The option, `AnyParamFeature()`, two result counters, the call last in `StepItemData` with a shared writer | done | `EnemyRandomizer.h/.cpp` |
| 6 | `WorldActivation.cpp` mapping and `anythingOn`; `WorldEditorScreen.cpp` progress line and item-data condition | done | both |
| 7 | T10–T14 and `verify`; `--granted <id>` in the two older tools | done | `trick_weapons_verify.py`, `hunter_tools_verify.py`, `starting_weapons_verify.py` |
| 8 | `worlds_verify.py`'s two frozen lists | done | `WIZARD_OPTIONS_MAPPING` 18 → 19 fields, `WIZARD_RUN_DECISION` 13 → 14 terms |

### Every file changed

**New**

| File | What |
| ---- | ---- |
| `app/tools/fmg.py` | wide-FMG reader, both framing equalities, prints nothing (cp1252 console) |
| `app/tools/gen_weapon_table.py` | the derivation, the frozen order, `--check` |
| `app/tools/trick_weapons_verify.py` | `list`, `selftest` (T1–T14 plus M3 offset-agreement cases), `verify` (G1–G6) |
| `app/src/Randomizer/TrickWeaponTable.h` | generated: 78 rows, `kTrickWeaponCount` |
| `app/src/Randomizer/CharaInitRows.h` | origin rows, row size, slot offsets, `FindRow`/`RowHasItem`/`FirstEmptySlot`, moved verbatim |
| `app/src/Randomizer/WeaponRequirements.h` | the four offsets, the tier stride, `ReqOwner`, `WeaponRequirementWriter` |
| `app/src/Randomizer/TrickWeaponGrant.h/.cpp` | the pass: candidate list, one draw, the `Grant` profile, both routes |

**Modified**

| File | What |
| ---- | ---- |
| `app/src/Randomizer/EnemyPoolSelection.h` | includes the table; `TrickWeaponSelection` typedef |
| `app/src/Randomizer/RandomizerDefaults.h` | `TrickWeaponSelection trickWeapons;` |
| `app/src/Randomizer/RandomizerDefaultsStore.cpp` | `trick_weapons_included` in `FormatSettings` and `ApplySettingKey` |
| `app/src/Randomizer/HunterTools.cpp` | includes `CharaInitRows.h`; the moved definitions deleted |
| `app/src/Randomizer/StartingWeapons.h` | takes `WeaponRequirementWriter&` |
| `app/src/Randomizer/StartingWeapons.cpp` | `StatProfile` holds a `WeaponRequirements`; the five profiles go through the writer as `CoffinSlot`; `ApplyStatProfile` and the four offsets deleted |
| `app/src/Randomizer/EnemyRandomizer.h` | `trickWeapons` option, `AnyParamFeature()`, `trickWeaponGranted` / `trickWeaponRowsChanged` |
| `app/src/Randomizer/EnemyRandomizer.cpp` | includes the pass; creates the shared `reqWriter`; the grant runs last in `StepItemData` |
| `app/src/UI/SettingsModel.h` | `SettingId::StartWithTrickWeapon`, `SettingKind::TrickWeaponPool` |
| `app/src/UI/SettingsModel.cpp` | the `kSettings` entry and five `switch` cases |
| `app/src/UI/ModelPicker.h` | `PickerStrings::showRowId`, `true` at the three shipped sites, `kTrickWeaponsStrings` |
| `app/src/UI/ModelPicker.cpp` | `RowLabel` honours `showRowId` |
| `app/src/UI/WorldEditorScreen.cpp` | the progress line naming the weapon; the item-data line's condition |
| `app/src/Game/WorldActivation.cpp` | options mapping in frozen order; `anythingOn` gains the term |
| `app/tools/pool_verify.py` | 584 → 686, 635 → 737; new-key case; four `showRowId` cases |
| `app/tools/ui_scroll_verify.py` | `Weapon picker` entry, 78 rows on the instruction layout |
| `app/tools/settings_ui_verify.py` | fourth selection field and kind; `PROSE_TO_LABEL`; the round-trip's value shape |
| `app/tools/hunter_tools_verify.py` | parses the moved constants from `CharaInitRows.h`; `--granted <id>` |
| `app/tools/starting_weapons_verify.py` | `--granted <id>` for SW-I1 / I6 / I7 |
| `app/tools/worlds_verify.py` | both frozen lists |
| `docs/features/randomizer-settings-ui/spec.md` | §7.1 gains `Start With A Trick Weapon` |
| `docs/features/037-start-with-trick-weapon/plan.md` | §10 only |

`docs/features/README.md` and `docs/randomization-feature-spec.md` are also
modified in the working tree. **Those are not mine** — they were already changed
before this pass started, and the work order said not to touch them.

---

## 2. Deviations from the plan

All nine are in the plan's §10. Summarised:

1. **`ui_scroll_verify.py`'s entry is `Weapon picker`, not `Trick weapon
   picker`.** That file prints screen names through `%-16s`; a 19-character name
   breaks the column every other row aligns to. Same geometry, same 78 rows.
2. **`WeaponRequirements.h` also carries `inline FindWeaponRow()`.** §4.4 lists
   the offsets, the stride, `ReqOwner` and the writer. The writer needs a row
   lookup, and neither `StartingWeapons.cpp`'s anonymous `FindRow` nor
   `CharaInitRows.h`'s is reachable from that header. Given a distinct name so no
   translation unit ever sees two.
3. **`TrickWeaponGrant.h` also declares `TrickWeaponName(int32_t)`.** §4.4 has
   the progress line name the weapon "from the table"; exposing the lookup from
   the engine keeps the table out of a UI file (`CLAUDE.md` §6).
4. **`TrickWeaponGrantResult` has four counters beyond §4.4's two.** Mirrors
   `HunterToolsResult`; only the two §4.4 names reach `EnemyRandomizerResult`.
5. **`trick_weapons_verify.py verify` takes an optional `--ticked`.** §6's
   command is unchanged; the flag lets G4 check "one of the *ticked* rows" rather
   than only "one of the 78".
6. **One stale comment count in `SettingsModel.cpp`** ("nineteen entries" →
   "twenty"), in a file §5 already lists.
7. **Not done — `RandomizerDefaultsStore.cpp`'s "636 bytes" comment.** Already
   wrong before this feature (the pinned figure was 635) and now 737. §5 scopes
   that file to the key, so it was left and reported.
8. **Not done — `settings_ui_verify.py` case 9's picker alignment band** still
   measures the three creature tables only. Its row regex keys on `"c\d+"` and
   the weapon picker draws no id column. Not in §5, and `plan-evidence.md` M7
   measured the widest weapon row at 653 px against the 791 px the band was
   sized for, so nothing is at risk — but it is genuinely unmeasured there.
   `ui_scroll_verify.py` does cover the new picker's band.
9. **Not done — the settings-UI spec's Appendix A help table.** §5 scopes that
   edit to §7.1. Nothing is unpinned: `settings_ui_verify.py` checks help for
   non-emptiness and wrapping, not against Appendix A.

Nothing else in §1–§7 was built differently, and no stop condition fired.

---

## 3. Decisions the plan left open

### 3.1 "1408 entries" is 1408 **ids**, not 1408 strings

M1's done-condition for `fmg.py` says it "returns 1408 entries". The measured
file covers 1408 ids across its group table, of which **1245 carry a string** —
the rest have string offset 0, which the format uses for "no text for this id"
and which is a different thing from an empty string. `load_fmg` therefore returns
both numbers, and T1 asserts the id count is 1408 and that `7000000` is
`"Saw Cleaver"`. Reading the id count as the count of *strings* would have made
T1 assert something false about a correctly parsed file.

### 3.2 The excluded set is seven, not the spec's six

Spec §4.2 says 84 right-hand tier-0 candidates and six exclusions;
`plan-evidence.md` M3 measured 85 and seven, and §E6 records the correction. The
generator derives the split rather than counting to either number, and it
measures 85 / 78 / 7 today. `trick_weapons_verify.py` asserts 85 candidates and
78 rows, **reports** the seven excluded ids as a note, and asserts only that
every excluded candidate fails *both* tests — exactly as T4 requires, so the
count of exclusions is never a hardcoded expectation.

### 3.3 Where the trick-weapon row lookup lives

`WeaponRequirementWriter` needs to find a weapon row by id. Options were: pass a
lookup in from each caller; include `CharaInitRows.h` from
`WeaponRequirements.h`; or give the header its own. Taken: its own, named
`FindWeaponRow`. Passing one in puts the same three lines at both call sites, and
making a weapon-requirements header depend on a character-creation header is a
dependency neither file's subject justifies.

### 3.4 `RowLabel` with no id column returns the display name alone

The plan says `RowLabel` "omits the id column when it is false". It also omits
the separating space and the five-character uppercasing pass, because with no id
there is nothing there to uppercase and a leading space would shift every weapon
row one glyph right of every creature row. `pool_verify.py` pins the exact
expression.

### 3.5 The probe's fixed weapon is asserted to be fixed

§4.5 fixes route B's weapon at `22000000` so the observation is unambiguous.
Nothing in §6 checks that it *stayed* fixed, and a later edit making route B
write the drawn weapon would silently destroy the probe's ability to answer. Two
selftest cases now parse `TrickWeaponGrant.cpp` and assert route A writes
`weapon` and route B writes `kProbeInventoryWeapon`.

### 3.6 T10 is checked as source shape, not as a mirror run

"Nothing ticked leaves the archive byte-identical" cannot be shown by running a
mirror: the mirror would simply not be called, and comparing an untouched buffer
with itself proves nothing. What matters is that the C++ returns before it
reaches the RNG *and* before any write, so T10 asserts the ordering of those
three points in `TrickWeaponGrant.cpp`, plus that the pass contains exactly one
`uniform_int_distribution` and exactly one `(rng)`. That is the property §3.1
actually wants ("turning this on cannot move any other roll").

### 3.7 `GRANT_PROFILE` is retyped in `starting_weapons_verify.py`

The `--granted` tolerance needs to know the grant's profile. It is retyped as
`(9, 9, 5, 6)` rather than parsed from `TrickWeaponGrant.cpp`, matching how
`STAT_PROFILES` is already retyped from the reference in the same file: parsing
it would let the tool and the code move together. That the constant *is* the real
origin minimum is `trick_weapons_verify.py` T8's job, measured from
`CharaInitParam`. `trick_weapons_verify.py` does the opposite — it parses the C++
constant and requires it to equal the paramdef-derived minimum — so both
directions are covered once each.

### 3.8 What the two `--granted` flags relax, exactly

The plan names the invariants but not the relaxation. Chosen, in both tools, to
be as narrow as could still pass a correct combined run, and each narrowing has a
negative case:

* `hunter_tools_verify.py --granted <id>`: `EquipParamWeapon.param` may differ
  (H-I1), and `equip_Wep_Right` may hold **that id** in an origin row (H-I3). A
  *different* id there is still a failure; a stray write elsewhere in the row is
  still a failure.
* `starting_weapons_verify.py --granted <id>`: `CharaInitParam.param` may differ
  (SW-I1); that weapon and its ten tiers may be restatted (SW-I6); and where it
  is also a coffin pick, SW-I7 expects the **grant's** profile — the coffin's
  profile winning instead is a failure, which is D4 as a test rather than a
  comment. An unrelated restatted weapon is still a failure.

---

## 4. Verification run

Commands as run, from the repository root, with `PYTHONIOENCODING=utf-8`.

### After M1

| Check | Command | Result |
| ----- | ------- | ------ |
| Build | `cd app && make` | `Nothing to be done for 'all'` — M1 adds a generated header nothing includes yet; the `.pkg` from the previous state was current |
| Table is current | `python app/tools/gen_weapon_table.py data/vanilla/dvdroot_ps4 --check` | `PASS: TrickWeaponTable.h matches the vanilla tree` |
| The mirror (T1–T9) | `python app/tools/trick_weapons_verify.py selftest data/vanilla/dvdroot_ps4` | `selftest 20/20` |
| Table listing | `python app/tools/trick_weapons_verify.py list data/vanilla/dvdroot_ps4` | 78 rows in the frozen order (`AMYGDALAN ARM` / `UNCANNY` / `LOST`, then `BEAST CLAW`, …), plus all seven excluded ids with a reason each. Not a pass/fail gate |

### After M2

| Check | Command | Result |
| ----- | ------- | ------ |
| Clean build | `cd app && make clean && make` | `.pkg` built, no compiler errors or warnings |
| Config and picker strings | `python app/tools/pool_verify.py selftest data/vanilla/dvdroot_ps4` | `94/94 passing` (was 89/89; the five new are the 686/737 pair rewritten, the new-key case and three `showRowId` cases) |
| Settings model | `python app/tools/settings_ui_verify.py` | `113/113 passing` (was 112/113 mid-edit, then green) — model reports 20 settings, 15 toggles, 5 drill-ins, `WeaponsGear` 5 |
| Picker geometry | `python app/tools/ui_scroll_verify.py` | `all geometry and scroll properties PASSED`; `Weapon picker 11 rows visible of 78` → 8 pages |
| Worlds parity | `python app/tools/worlds_verify.py selftest` | `all checks passing` |
| M1 still green | `python app/tools/trick_weapons_verify.py selftest …` | `selftest 20/20` |

### After M3

| Check | Command | Result |
| ----- | ------- | ------ |
| Clean build | `cd app && make clean && make` | exit 0, **no `error` or `warning` anywhere in the log**, `IV0000-BBRD00001_00-BBRANDOMIZERMAIN.pkg` (7 143 424 bytes) produced; `TrickWeaponGrant.o` present |
| Table is current | `python app/tools/gen_weapon_table.py data/vanilla/dvdroot_ps4 --check` | `PASS` |
| The mirror (T1–T14) | `python app/tools/trick_weapons_verify.py selftest data/vanilla/dvdroot_ps4` | `selftest 50/50` |
| Row 34 unaffected | `python app/tools/hunter_tools_verify.py selftest data/vanilla/dvdroot_ps4` | `selftest 19/19` (was 13/13; **no existing case changed state** — the 11 behavioural cases still pass, the two constant cases now read `CharaInitRows.h`, two more assert the move left nothing behind, four are the `--granted` tolerance) |
| Row 5 unaffected | `python app/tools/starting_weapons_verify.py selftest data/vanilla/dvdroot_ps4` | `selftest 17/17` (was 12/12; **no existing case changed state**; five are the `--granted` tolerance, two of them D4 both ways) |
| Config and picker strings | `python app/tools/pool_verify.py selftest data/vanilla/dvdroot_ps4` | `94/94 passing` |
| Settings model | `python app/tools/settings_ui_verify.py` | `113/113 passing` |
| Picker geometry | `python app/tools/ui_scroll_verify.py` | `all geometry and scroll properties PASSED` |
| Worlds parity | `python app/tools/worlds_verify.py selftest` | `all checks passing`; full run `97/97 passing`, with `the options mapping is field for field the wizard's (19 fields)` and `the run decision is the wizard's || chain, field for field (14)` |
| `verify` smoke test | `python app/tools/trick_weapons_verify.py verify data/vanilla/dvdroot_ps4 data/vanilla/dvdroot_ps4` | exit 1 with `G4 0 of 22 origin rows were written` — correct: a tree that granted nothing is not a granted run |

**Not run, and why:** `trick_weapons_verify.py verify <vanilla> <output>` against
a real granted output tree, and
`hunter_tools_verify.py / starting_weapons_verify.py verify … --granted <id>`
against one. There is no such tree yet — producing one needs the app to run on the
console. `data/runs/` holds only pre-feature runs. The `verify` code path was
smoke-tested as above and its G1–G6 logic is exercised by twelve positive and
negative T10–T13 cases against real vanilla bytes, but it has not seen output the
PS4 actually wrote.

---

## 5. What this does not prove

**Nothing here shows the feature works.** `CLAUDE.md` §3: the console is the
final authority and the implementer has no access to it. Specifically unproven:

* **Whether Bloodborne reads `equip_Wep_Right` at all.** This is the whole risk
  of the feature (spec §4.5). The field is `-1` on all 1700 rows of the shipped
  param, so the route the feature wants is the one the game's own data never
  exercises. A byte landing in the right place says nothing about what the engine
  does with it — the standing lesson of `docs/known-traps.md` and row 8.
* **Whether the inventory route grants a weapon either.** Weapon ids in ordinary
  item slots are *exercised* in shipped data (three rows), which is stronger
  ground than route A, but not proof.
* **Whether the requirement rewrite makes the weapon wieldable in game.** The
  bytes are 9/9/5/6 on eleven rows; that every origin's four stats meet those
  numbers is measured from `CharaInitParam`. Whether the game re-reads the
  requirement at equip time is a console question.
* **Determinism.** The pass draws once, last, and the mirror deliberately does
  not predict which weapon a seed picks (plan §E7). That two runs on one seed
  grant the same weapon is a hardware check.
* **That the *right version* arrives.** The table is pinned and every name is the
  game's own text for that exact id, but the inventory naming a ticked Lost or
  Uncanny row as that version is only observable on the TV.
* **The picker on screen.** Geometry is asserted arithmetically against the baked
  atlas; nobody has looked at 78 weapon rows over 8 pages on a 1080p panel.

---

## 6. Hardware test handoff

### The probe — do this first, and nothing else with it

1. Install the `.pkg`. In a world's settings, under **WEAPONS & STARTING GEAR**,
   open **START WITH A TRICK WEAPON** and tick exactly one row: plain
   **`SAW CLEAVER`** (not Uncanny, not Lost). Leave every other setting off.
   `SAW CLEAVER` is the right choice because it is one of the four weapons every
   origin can already wield, so a failure cannot be a requirements failure in
   disguise.
2. Activate the world. **Start a new character** — an existing save gains nothing
   by design.
3. Look at the character in Iosefka's Clinic, and look in the inventory.

| Observed | Meaning | M4 becomes |
| --- | --- | --- |
| Saw Cleaver **in hand** | route A works as intended | keep A, delete B |
| Saw Cleaver **in the inventory**, no Threaded Cane | route A grants, the equip does not — accepted under D5 | keep A, delete B, reword "in hand" to "in the inventory" |
| Saw Cleaver in hand **and** a Threaded Cane in the inventory | both routes work | keep A, delete B |
| **Threaded Cane** in the inventory, no Saw Cleaver anywhere | route A is dead, route B works — accepted under D5 | keep B, delete A, grant the drawn weapon through B, reword to "in the inventory" |
| **Nothing at all** | the game reads neither route on these rows | **report before deleting anything.** Spec §7's deletion rule applies but the decision is the developer's |
| Character creation fails, or the character is broken | the two writes cannot be told apart | halt and report. Re-probe with one write at a time |

A **Threaded Cane** is the signal for route B and is deliberately a different
weapon from the one ticked. It is a fixed probe constant, not a bug, and it goes
away in M4.

### Once the probe is positive

Spec §8's hardware list verbatim, in its order. Two are worth doing first because
they are the ones that fail silently:

* Tick a **Lost** or **Uncanny** row and confirm *that version* arrives, named as
  that version in the inventory. This is the check that the 78-row table has not
  quietly collapsed to 26.
* Tick **`LOGARIUS' WHEEL`**, then **`KOS PARASITE`**, and confirm each is usable
  without levelling. Those are the two extremes — `(20, 12, 0, 10)` and
  `(0, 0, 0, 20)` as shipped — so they are what proves the requirement rewrite
  covers the hard end of the list and not just the easy end.

Then: the weapon can be swung and transformed and kills the clinic beast; every
origin gets it including Waste of Skin at level 4; several ticked gives the same
weapon on the same seed and the rest of the world identical; nothing ticked spawns
empty-handed exactly as vanilla; with `RANDOMIZE STARTING WEAPONS` also on both
the grant and three randomized coffins are present; an existing save loaded with
the setting on gains nothing and is undamaged.

### Capture, so the offline mirrors can be run

Save `live.log` and the **output tree** of at least one granted run and one
nothing-ticked run into `data/runs/`. Then:

```
python app/tools/trick_weapons_verify.py verify data/vanilla/dvdroot_ps4 <output> \
       --ticked <the 78-character line from that world's config>
python app/tools/hunter_tools_verify.py     verify data/vanilla/dvdroot_ps4 <output> --granted 7000000
python app/tools/starting_weapons_verify.py verify data/vanilla/dvdroot_ps4 <output> --granted 7000000
```

The `--granted` flag is only needed on a run that had this feature on; without it
those two tools correctly report a combined run as FAILED.

---

## 7. Stop point

**The Required gate after M3**, exactly where the plan's Execution Strategy puts
it. M3 completed; no stop condition fired.

M4 has **not** been started, and could not be: §6's table makes its content a
function of what the console shows, and writing it now would mean writing three
branches and throwing two away.

The tree is left with M1–M3 in the working directory, a clean `make clean && make`
behind it, the `.pkg` built and every §6 automated check green. No branch, no
commit. `plan.md` §10 is the only part of the plan edited.

Next: run the probe of §6, then `/implement 37` for M4 with the result.

---

# Milestone 4 — settle the route

**Implemented:** 2026-09-27, immediately after the gate.

## 8. The gate's answer, and which branch it selected

The probe of §6 ran on hardware on 2026-09-27. **Two** rows were ticked —
`AMYGDALAN ARM` and `CHIKAGE` — everything else off, and a new character was
started. The character spawned in Iosefka's Clinic **holding the Amygdalan Arm**.

That is `equip_Wep_Right` being read at character creation, and being *equipped*
rather than merely granted. **Route A wins.** It is the first row of §6's table —
"Saw Cleaver in hand" — so the branch is **keep A, delete B**, with no rewording
of "in hand" needed. It is also the outcome spec §10 D5 preferred.

Two things the probe did not settle, and neither changes M4:

* **Whether route B also works** was not reported either way (a Threaded Cane in
  the inventory was not looked for, or not mentioned). §6's table gives the same
  M4 for "A only" and for "A and B" — keep A, delete B — so the answer is not
  needed. It is recorded here as unknown rather than as negative.
* **The probe used two ticked rows, not the one row §6 specified.** Ticking two
  exercises the draw as well as the write, which is strictly more than the probe
  asked for; it cannot produce a false positive, since a weapon arriving at all
  requires the write to be read. No stop condition covers it and it was not
  treated as one.

## 9. What was deleted and what was kept

**Kept — route A, entire.** One `WriteI32LE` of the **drawn** weapon into
`equip_Wep_Right` (offset 16) of each of the 22 origin rows, the single draw, the
`ReqOwner::Grant` profile `{9, 9, 5, 6}` through the shared
`WeaponRequirementWriter`, the full-row bounds check, `kOriginRows` and `FindRow`
from `CharaInitRows.h`, and the log line.

**Deleted — route B, entire.**

| Gone | Was |
| ---- | --- |
| `kProbeInventoryWeapon` | the fixed `22000000` Threaded Cane |
| the inventory-slot write block | `RowHasItem` / `FirstEmptySlot` / `kItemIdBase` / `kItemNumBase` and the `itemNum` count byte |
| `TrickWeaponGrantResult::slotsWritten`, `::rowsFull` | counters of that write only; both could now only report 0 |
| the `probe slots written` clause of the log line | — |
| the mirror's route-B branch, its `probe` argument and its `write_route_a` / `write_route_b` flags | `apply_grant(plain, members, offs, weapon, profile)` now |
| the item fields from `compare()`'s `allowed` set | G3 now permits only `equip_Wep_Right`'s four bytes |

`CharaInitRows.h` keeps `RowHasItem`, `FirstEmptySlot`, `kItemIdBase` and
`kItemNumBase` **unchanged** — `HunterTools.cpp` still uses all four, and the
header was carved out to be shared. Nothing in it was touched in M4.

`equip_Wep_Right_GenId` and the three sibling `equip_*` fields remain at `-1`,
as §4.5 and §3.2 require. The probe granted the weapon without them.

## 10. Every file changed in M4

| File | Change |
| ---- | ------ |
| `app/src/Randomizer/TrickWeaponGrant.h` | the "UNVERIFIED, AND THE WHOLE FEATURE RIDES ON IT" header block replaced by the hardware finding and what it means; "the character starts with it" → "wakes HOLDING it, equipped in the right hand"; `slotsWritten` and `rowsFull` removed from the result struct |
| `app/src/Randomizer/TrickWeaponGrant.cpp` | `kProbeInventoryWeapon` deleted; the route-B write block deleted; `kEquipWepRight`'s comment rewritten from "the unexercised route" to the confirmed one; "Step 4: both routes" → "Step 4: the write"; the log line's probe clause dropped; the bounds-check comment no longer claims "the fields written below sit up to 213 bytes in" (only four bytes are written now; the whole-row check is kept and its reason restated) |
| `app/tools/trick_weapons_verify.py` | module docstring records the console answer; the two route-shape cases become four M4 cases; `apply_grant` loses route B and its two flags; `compare()`'s G3 `allowed` set narrowed to `equip_Wep_Right`; T11 gains a negative case for the deleted write; T12 tightened to an exact item array; the `WORKSHOP_TOOLS` note rewritten |
| `docs/features/037-start-with-trick-weapon/plan.md` | **§10 only** — six M4 rows appended |
| `docs/features/037-start-with-trick-weapon/implementation-report.md` | this section; the status line and a pointer at the top |

**Deliberately not changed**, and each is a decision:

* **No user-facing string moved.** M4's step 2 asks for the help text and the
  progress line to say what actually happens. They already did: the help reads
  *"Start holding one of the trick weapons you tick here…"* and the progress line
  reads `STARTING WITH <NAME>`. Route A won *in hand*, so nothing promised the
  losing route. Rewording would have been churn against a correct string.
* `docs/features/randomizer-settings-ui/spec.md`, `SettingsModel.cpp`,
  `ModelPicker.h`, `WorldEditorScreen.cpp`, `WorldActivation.cpp`,
  `EnemyRandomizer.{h,cpp}` — untouched. The route change is invisible to all of
  them, and none is listed for M4 in §5.
* `hunter_tools_verify.py`'s `--granted` tolerance — it already modelled route A
  only, so it needed nothing. Its header note is now stale; see §12.

## 11. Decisions the plan left open

### 11.1 The two route-B counters go rather than stay at zero

§4.4 names two result counters; M3 added four, mirroring `HunterToolsResult`
field for field (plan §10, M3 entry 4). `slotsWritten` and `rowsFull` counted only
the inventory write. Leaving them would be a field that can never be non-zero and
a log clause that always reads `0 probe slots written` — kept-alive symbols of
exactly the kind `CLAUDE.md` §4 rules out. They were deleted.
`EnemyRandomizerResult` never carried either, so nothing outside this file moved.

### 11.2 G3's narrowing costs the combined-run case, and that is accepted

M4's step 3 done-condition is "G3 rejects a tree carrying the deleted write". The
deleted write put a weapon id into a free `item_NN` slot. `START WITH HUNTER
TOOLS` puts tool ids into free `item_NN` slots. From the output bytes alone those
are the same kind of difference, so one predicate cannot both reject the first and
tolerate the second.

Taken: reject. `trick_weapons_verify.py verify` now describes a **grant-only**
output tree, and a tree that also had row 34 on is reported as a G3 failure by it.
The combined direction is already covered the other way round — by
`hunter_tools_verify.py verify … --granted <id>`, whose tolerance permits exactly
`equip_Wep_Right` holding that id and nothing else. The reasoning is written into
the tool at the predicate, not left to this report. The alternative — a
`--with-hunter-tools` flag on `verify` — is a flag §5 does not list, for a
combination the other tool already checks.

### 11.3 "No fixed weapon id" is asserted as a digit pattern

Step 1's done-condition includes "no fixed weapon id". Asserting the absence of
one named constant would pass the moment someone spelled a different literal, so
the case asserts that **no 7-or-8-digit decimal literal occurs anywhere in
`TrickWeaponGrant.cpp`** — which is the shape of every Bloodborne weapon id. It
holds today (the remaining numerals are `16`, `4`, `300` and prose figures), and
it fails on any re-introduced id rather than on one particular name.

### 11.4 The probe's "two ticked rows" was not treated as a stop condition

§6 specified one ticked row, plain `SAW CLEAVER`. The developer ticked two. The
observed outcome still lands exactly on row 1 of §6's table, and the stop
condition is "the probe result is not one of the six rows of §6's table" — it is
row 1. Two ticked rows can only make the observation *stronger* (the draw ran as
well as the write). Recorded, not escalated.

## 12. Verification run

Commands exactly as run, from the repository root, with `PYTHONIOENCODING=utf-8`.

| Check | Command | Result |
| ----- | ------- | ------ |
| Clean build | `cd app && make clean && make` | **exit 0**, no `error` and no `warning` anywhere in the log, `IV0000-BBRD00001_00-BBRANDOMIZERMAIN.pkg` produced at 7 143 424 bytes |
| Table is current | `python app/tools/gen_weapon_table.py data/vanilla/dvdroot_ps4 --check` | `PASS: TrickWeaponTable.h matches the vanilla tree` |
| Table listing | `python app/tools/trick_weapons_verify.py list data/vanilla/dvdroot_ps4` | 78 rows in the frozen order plus the seven excluded ids with a reason each. Not a pass/fail gate |
| The mirror, T1–T14 | `python app/tools/trick_weapons_verify.py selftest data/vanilla/dvdroot_ps4` | **`selftest 53/53`** (was 50/50: −2 route-shape cases, +4 M4 shape cases, +1 T11 negative for the deleted write). No pre-existing case changed state |
| Row 34 unaffected | `python app/tools/hunter_tools_verify.py selftest data/vanilla/dvdroot_ps4` | `selftest 19/19` — unchanged from M3 |
| Row 5 unaffected | `python app/tools/starting_weapons_verify.py selftest data/vanilla/dvdroot_ps4` | `selftest 17/17` — unchanged from M3 |
| Config and picker strings | `python app/tools/pool_verify.py selftest data/vanilla/dvdroot_ps4` | `94/94 passing` |
| Settings model | `python app/tools/settings_ui_verify.py` | `113/113 passing` |
| Picker geometry | `python app/tools/ui_scroll_verify.py` | `all geometry and scroll properties PASSED` |
| Worlds parity | `python app/tools/worlds_verify.py selftest` | `all checks passing` |
| `verify` smoke test | `python app/tools/trick_weapons_verify.py verify data/vanilla/dvdroot_ps4 data/vanilla/dvdroot_ps4` | exit 1 with `G4 0 of 22 origin rows were written` — correct: a tree that granted nothing is not a granted run |

**Not run, and why.**

* `trick_weapons_verify.py verify <vanilla> <output>` against a **real granted
  output tree**, and the `--granted` runs of the two older tools against one.
  There is still no such tree: producing one needs the app to run on the console,
  and `data/runs/` holds only pre-feature runs. G1–G6's logic is exercised by one
  positive and ten negative T11 cases against real vanilla bytes, but it has not
  seen output the PS4 actually wrote.
* Nothing on hardware. `CLAUDE.md` §3 — the implementer has no console.
* `settings_ui_verify.py` case 9's picker alignment band still does not measure
  the weapon table (M3 deviation 8, unchanged and still reported).

**Known-stale, deliberately not fixed** — reported rather than tidied, because §5
does not scope M4 to these files:

* `app/tools/hunter_tools_verify.py`'s header note still reads *"Feature 037
  writes into a FREE slot found by search rather than a fixed index"*. That is now
  false: feature 037 writes no slot at all. Its actual tolerance code was always
  route A only and is correct.
* `RandomizerDefaultsStore.cpp`'s "Worst case today is 636 bytes" comment (M3
  deviation 7, unchanged).

## 13. What M4 does not prove

The route question is **answered** — that was the one thing the previous report
listed first, and the console settled it. What is still unproven:

* **The requirement rewrite actually making a weapon wieldable in game.** The
  bytes are 9/9/5/6 on eleven rows and every origin's four stats meet them, but
  whether Bloodborne re-reads the requirement at equip time is a console question.
  The probe granted an `AMYGDALAN ARM` — `(0, 0, 0, 15)` as shipped, so no origin
  can wield it unmodified — and it arrived *in hand*, which is suggestive but not
  a test: the game may equip a template weapon without checking, and what matters
  is whether it can be **swung**.
* **Determinism.** One draw, last in the run, and the mirror deliberately does not
  predict which weapon a seed picks.
* **That the right *version* arrives.** `AMYGDALAN ARM` is a version-0 row, so the
  probe says nothing about Uncanny or Lost naming.
* **The picker on screen.** 78 rows over 8 pages has still not been looked at.
* **Interaction with `RANDOMIZE STARTING WEAPONS`** (spec D4 / B6) on hardware.

## 14. Hardware test handoff

The probe is done and must not be repeated. What remains is **spec §8's hardware
list verbatim, in its order**, with these done first because they fail silently:

1. **A `Lost` or `Uncanny` row grants that version, named as that version.** The
   check that the 78-row table has not quietly collapsed to 26.
2. **`LOGARIUS' WHEEL` and `KOS PARASITE` are usable without levelling** —
   `(20, 12, 0, 10)` and `(0, 0, 0, 20)` as shipped, the two extremes of the list.
   This is the requirement rewrite's real test.
3. **The probe's own weapon can be swung and transformed.** The `AMYGDALAN ARM`
   arrived in hand but nobody has confirmed it is usable, and it is one of the 39
   rows no origin can wield unmodified.

Then the rest of spec §8: every origin gets it, including Waste of Skin at level
4; several ticked gives the same weapon on the same seed with the rest of the
world identical; nothing ticked spawns empty-handed exactly as vanilla; with
`RANDOMIZE STARTING WEAPONS` also on, both the grant and three randomized coffins
are present and the grant's profile is what a shared weapon carries; an existing
save loaded with the setting on gains nothing and is undamaged.

**There should be no Threaded Cane anywhere now.** If one appears in a new
character's inventory, the build being tested is the M3 probe build, not this one.

### Capture, so the offline mirrors can be run

Save `live.log` and the **output tree** of at least one granted run and one
nothing-ticked run into `data/runs/`. Then:

```
python app/tools/trick_weapons_verify.py verify data/vanilla/dvdroot_ps4 <output> \
       --ticked <the 78-character line from that world's config>
python app/tools/starting_weapons_verify.py verify data/vanilla/dvdroot_ps4 <output> --granted <id>
```

Use a **grant-only** world for the first command — with `START WITH HUNTER TOOLS`
also on it will report G3 failures on the item slots, by design (§11.2). For a
combined run use `hunter_tools_verify.py verify … --granted <id>` instead.

## 15. Stop point

**M4 completed. No stop condition fired.** All four milestones of plan 037 are
built, the `.pkg` builds clean from `make clean`, and every automated check in §6
is green.

The tree is left in the working directory. No branch, no commit. `plan.md` §10 is
the only part of the plan edited. `docs/features/README.md` and
`docs/randomization-feature-spec.md` remain modified from before this pass and
were not touched.
