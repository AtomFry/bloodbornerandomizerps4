# Implementation Report 033 — Protect Caged Dogs — milestone 1

**Status: HARDWARE TEST PASSED** — 2026-09-19, the byte-identical
seed-1234567890 run of §6, confirmed by the developer. *(Status line updated
when milestone 2 was implemented; nothing else in this milestone-1 report has
been changed. Milestone 2's report follows it below.)*

**Plan:** `docs/features/033-protect-caged-dogs/plan.md` — milestone 1

**Spec:** `docs/features/033-protect-caged-dogs/spec.md`

**Implemented:** 2026-09-19

---

## 1. What was built

The engine can now leave the twenty-six caged-dog placements alone, and a new
verifier proves the ten shipped identifiers still describe the vanilla data.
Nothing can turn the protection on yet — there is no setting, no configuration
key and no UI row until milestone 2 — so this build's behaviour is identical to
the previous one, which is exactly what its hardware test checks.

| # | Planned change | Status | Where |
| - | -------------- | ------ | ----- |
| 1 | `CagedDogList.h` — the ten identifiers and the predicate | done | `app/src/Randomizer/CagedDogList.h` (new, 101 lines) |
| 2 | `doNotRandomizeCagedDogs` on the options, `cagedDogsProtected` on the result | done | `app/src/Randomizer/EnemyRandomizer.h:38-43` (result), `:119-131` (option) |
| 3 | The gate in `StepWriteMap`, incrementing the counter | done | `app/src/Randomizer/EnemyRandomizer.cpp:762-778` |
| 4 | The per-run count to the text log when the option is on | done | `app/src/Randomizer/EnemyRandomizer.cpp:934-941` |
| 5 | `caged_dogs_verify.py` with `list`, `protected`, `selftest` | done | `app/tools/caged_dogs_verify.py` (new, 494 lines) |

The gate, in full, sits between the `ENEMIES SKIPPED` test and the roll:

```cpp
if (!forced && IsSkippedName(name, skipPatterns)) continue;

// ... comment ...
if (!forced && options.doNotRandomizeCagedDogs &&
    IsProtectedCagedDog(lm.name, part_fields::GetEntityID(partBlob))) {
    result.cagedDogsProtected++;
    continue;
}

int roll = RandInt(0, 100);
```

Nothing else in the placement loop moved, and no `RandInt` call sits above the
new test that did not before.

**Invariants checked before calling this done (§3.1):**

* `StepReadMap`'s contribution loop is untouched — the engine pool still
  recomputes as 333 entries across 82 models, and `pool_verify.py table enemy`
  still matches the baked table.
* The RNG stream with the setting off: the option is a plain `bool` tested
  before the gate reads the entity ID or walks the table, and it short-circuits
  the whole gate. An off run draws the same `RandInt` sequence. (The proof is
  the hardware diff below; the code guarantees only that nothing new is drawn.)
* The existing test order — `forced`, fixed exclusion list, `ENEMIES SKIPPED`,
  roll — is unchanged; the new test was inserted, nothing was reordered.
* The m28 forced-maiden override still outranks everything: the new test is
  inside the same `!forced &&` guard as the two above it.
* `ENEMIES SKIPPED` was not edited in any way.
* The gate reads `GetEntityID` and writes nothing. No name, entity ID, position
  or collision index is written anywhere in this change.
* `EnemyPoolTable.h` and `EnemySkipTable.h` were not regenerated or touched.
* `defaults.cfg` is untouched — no key was added this milestone, so every
  existing file still loads unchanged.
* Layering: the new header and the gate are both under `app/src/Randomizer/`,
  the header includes only `<array>`, `<cstdint>` and `<string>`, and no UI
  file was opened.

---

## 2. Deviations from the plan

**None.** Every change is where §5 and §7 put it, with the names §4 gave, and no
file outside §5's milestone-1 rows was modified.

---

## 3. Decisions the plan left open

### 3.1 The order of `!forced` and the option test in the gate

§4.2 says the gate "reads the option first, then `part_fields::GetEntityID`",
and §3.1 says it "sits inside the same `!forced &&` guard as the two tests
above it". Both are satisfiable either way round, since `forced` is already
computed by then and the two are plain booleans, but one of them has to be
written first.

Written as `!forced && options.doNotRandomizeCagedDogs && IsProtected...`, so
the line reads like its two siblings directly above it. The option is still the
first thing *the new gate itself* does — nothing is read, allocated or compared
before it — which is what §3.1's RNG-stream requirement and §4.2's "off costs
one boolean compare" both turn on. Reversing the two would change no behaviour
and no timing worth measuring.

### 3.2 Where the per-run log line goes

§4.2 asks for "one per-run text-log line naming the count when the option is
on" without naming a site. It is emitted immediately after the existing
`enemy randomizer: done - …` summary in `StepEmevd`, which is the run's
established summary point, and it is inside `if (options.doNotRandomizeCagedDogs)`
so an off run logs nothing new:

```
enemy randomizer: caged dogs protected - 26 placement(s) left unrandomized
```

It names placements rather than dogs, deliberately: 26 is the honest figure and
"26 dogs" would contradict the ten a player meets. §9 P12 keeps that number off
both screens for the same reason, so the wording only ever reaches `live.log`.

### 3.3 Two identifiers the verifier needs that the plan did not name

`caged_dogs_verify.py` hard-codes the three cage object models
(`o243010`, `o243011`, `o273011`) as derivation A's input, because §6 case 3
defines that derivation in terms of them. It hard-codes nothing else about the
protected set: the ten entries, the eight placement names for the substring
trap, and the dog↔cage pairing derivation C cross-checks are all recomputed or
parsed out of `CagedDogList.h`.

It also asserts two figures the plan mentions only in §3.3's hazard table — 13
Central Yharnam cage props against 6 caged dogs, and 6 Forbidden Woods props
against 4 — so that the "an empty cage is not a miss" line in the hardware
handoff is backed by a check rather than by memory.

### 3.4 Naming inside the new header

The plan names the file and the shape but not the symbols. They are
`CagedDogEntry`, `CagedDogList()` and `IsProtectedCagedDog(mapName, entityID)`,
following `EnemySkipTable()`/`IsSkippedName` and `BossNameList()` next door.
The verifier parses the table by the same `kList = {{ … }}` marker
`enemy_lookup.py` already uses for the other headers.

---

## 4. Verification run

| Check | Command | Result |
| ----- | ------- | ------ |
| Clean build | `cd app && make clean && make` | `.pkg` built — `IV0000-BBRD00001_00-BBRANDOMIZERMAIN.pkg`, 7,143,424 bytes |
| No new warnings | rebuild of `EnemyRandomizer.cpp` under `-Wall` | no warnings, no errors |
| Header compiles standalone | `clang++ … -fsyntax-only` on a TU that includes only `CagedDogList.h` | ok |
| Protected set recomputation | `python app/tools/caged_dogs_verify.py selftest data/vanilla/dvdroot_ps4` | **23/23 passing** — the eight §6 cases, expanded |
| Protected set listing | `python app/tools/caged_dogs_verify.py list data/vanilla/dvdroot_ps4` | 26 rows, 6/6/6/4/4 across the five map files |
| Failing direction | `python app/tools/caged_dogs_verify.py protected data/vanilla/dvdroot_ps4 "data/runs/20260919-Enemies Only/dvdroot_ps4"` | **FAIL, as required** — `26 of 26 protected placement(s) were not frozen` |
| Pool and config mirrors | `python app/tools/pool_verify.py selftest data/vanilla/dvdroot_ps4` | 85/85 passing — unchanged, no case changed state |
| Pool unchanged | `python app/tools/pool_verify.py table enemy data/vanilla/dvdroot_ps4` | PASS — 82 models; `engine_pool` recomputes 333 entries across 82 models |
| Settings-list geometry | `python app/tools/ui_scroll_verify.py` | all geometry and scroll properties PASSED (unchanged; no UI edit this milestone) |

The selftest's 23 lines cover the plan's eight cases as follows: header shape
and the `<= 0` rejection (case 1, 4 lines), the 26 and their per-file split
(case 2, 2 lines), derivation A (case 3), derivation B (case 4), the two
distance bounds and the radius' independence of them (case 5, 3 lines), the
EMEVD framing checks and the two script families (case 6, 6 lines), model,
stat rows and exclusion-list independence (case 7, 3 lines), the name-substring
trap in the failing direction (case 8), and the two cage-prop counts of §3.3.

Selected output, for the two that matter most:

```
033: derivation A (cage proximity) gives the same 26           ok
033: derivation B (nearest object) gives the same 26           ok
033: largest protected distance is 0.899                       ok
033: smallest unprotected distance is 2.671                    ok
033: m24_01_00_00 framing - 5 sections reconcile, 208760 bytes ok
033: m24_01_00_00 framing - 250 events' instruction counts sum to 4133 ok
033: m27_00_00_00 framing - 115 events' instruction counts sum to 1927 ok
033: one m24_01 event family names exactly the six, x6         ok
033: one m27 event family pairs each of the four with its cage ok
```

```
protected placements  : 26 of 26 expected
                        0 frozen, 26 NOT frozen
other c1240 placements: 70, of which 69 changed (98.6%)
  NOT FROZEN: m24_01_00_01 c1240_0005: c1240/124400/124400 -> c1240/124551/900008215
FAIL: 26 of 26 protected placement(s) were not frozen
```

That last `NOT FROZEN` line is the one worth reading: in the pre-feature run one
caged dog became a *different variant of the same creature*, so a freeze test
comparing only the model name would have passed a re-targeted placement. The
check compares model and behaviour row, and allows the stat row to be either the
vanilla value or its zone-scaled variant (spec D8).

**Not run:** anything requiring a console, and anything belonging to milestone 2
(`pool_verify.py`'s config-size arithmetic is still at 555 bytes and still
passes, because no `defaults.cfg` key was added yet).

---

## 5. What this does not prove

* **That the engine consults the header correctly.** The mirrors pin the rules,
  not the C++ that implements them. `caged_dogs_verify.py` proves the ten
  identifiers still match the twenty-six placements they were derived from; it
  says nothing about whether `StepWriteMap` calls the predicate with the right
  map name, or at all. Only a run on the PS4 with the setting on can show that,
  and there is no way to turn it on until milestone 2.
* **That the RNG stream is unchanged with the setting off.** The code cannot
  draw randomness it does not reach, but "cannot" is an argument, and this
  project has had struct-layout and stale-object surprises that no reading of
  the source predicted. The byte-identical seed-1234567890 diff below is the
  only thing that settles it, and it is the entire point of stopping here.
* **That protecting these ten cures the lag and the unkillable replacements.**
  Spec §4 H1 is explicit that the cause is untraced. That is a milestone 2
  question and may yet come back negative.
* **That the `.pkg` runs at all.** A clean cross-compile is not a boot.

---

## 6. Hardware test handoff

One run, no gameplay needed. This is the off-is-a-no-op test, and it is the only
check that can catch an RNG-stream shift.

1. **What to enable.** Install this build's
   `app/IV0000-BBRD00001_00-BBRANDOMIZERMAIN.pkg`. Seed **1234567890**,
   `RANDOMIZE ENEMIES` **on**, every other setting **off**, all 82 enemies
   included in `ENEMIES INCLUDED`, nothing ticked in `ENEMIES SKIPPED`.
2. **What to do.** Run the randomizer to completion. No need to launch
   Bloodborne. Copy the resulting `dvdroot_ps4` tree and `live.log` off the
   console.
3. **What to look for.** The tree must be **byte-identical** to
   `data/runs/20260919-Enemies Only/dvdroot_ps4`. On Windows:

   ```
   robocopy /L /MIR /NJH /NJS /NDL "data\runs\20260919-Enemies Only\dvdroot_ps4" "<new tree>"
   ```

   or compare per-file hashes. `live.log` should contain **no** new line — in
   particular no `caged dogs protected` line, since the option cannot be on.
4. **What a failure looks like.** Any differing map file. That would mean the
   new gate changed the roll stream despite being unreachable, and it is a stop
   condition: report it rather than adjusting the gate, because the only way it
   can happen is something this plan did not model.

Nothing about the cages themselves is testable yet. The six Central Yharnam and
four Forbidden Woods walks in plan §6 belong to milestone 2. When they come:
Central Yharnam's kennel yard has **13** cage props for **6** caged dogs and the
Forbidden Woods cluster **6** props for **4** dogs, so empty cages are vanilla in
both areas and are not a miss.

---

## 7. Stop point

**Completion gate of milestone 1**, reached in full: the five §7 changes are
built, the clean build produced the `.pkg`, `caged_dogs_verify.py selftest`
passes all eight cases, `protected` fails against the pre-feature tree with 26
of 26 not frozen, and `pool_verify.py`'s selftest and enemy table are unchanged.
No stop condition fired.

Milestone 2 — the setting itself: the `RandomizerDefaults` field, the
`do_not_randomize_caged_dogs` key in both halves of the store, row 15 appended
last on both settings screens with no existing row constant touched, the wizard
member and its `StartCommit` assignment, and the `ui_scroll_verify.py` and
`pool_verify.py` arithmetic updates. **Not started**, and per `CLAUDE.md` §4 it
waits on the hardware test above and explicit approval.

---
---

# Implementation Report 033 — Protect Caged Dogs — milestone 2

**Status: AWAITING HARDWARE TEST**

**Plan:** `docs/features/033-protect-caged-dogs/plan.md` — milestone 2

**Spec:** `docs/features/033-protect-caged-dogs/spec.md`

**Implemented:** 2026-09-19

---

## 1. What was built

The setting itself. `DO NOT RANDOMIZE CAGED DOGS` is now a row on both settings
screens, it persists in `defaults.cfg`, and the value reaches the gate milestone
1 put in `StepWriteMap`. Nothing about the engine, the pool or the protected set
changed this milestone — this is the wiring that makes the existing gate
reachable, and it is the first build in which the protection can be turned on.

| # | Planned change | Status | Where |
| - | -------------- | ------ | ----- |
| 1 | `doNotRandomizeCagedDogs` on `RandomizerDefaults`, default `false` | done | `app/src/Randomizer/RandomizerDefaults.h:116-128` |
| 2 | `do_not_randomize_caged_dogs` in both halves of the store | done | `app/src/Randomizer/RandomizerDefaultsStore.cpp:81-84` (load), `:114` and `:131` (save) |
| 3 | The row constant at 15 on both screens, both counts 15 to 16 | done | `app/src/UI/SetupDefaultsScreen.h:36,65-71`, `app/src/UI/EnableWizardScreen.cpp:46-51` |
| 4 | The row string appended last in all three `items` vectors | done | `SetupDefaultsScreen.cpp:246-249`, `EnableWizardScreen.cpp` `DrawSaveData` and `DrawConfirm` |
| 5 | The toggle branches — `ToggleRow`, and left/right and X in `UpdateSaveData` | done | `SetupDefaultsScreen.cpp:121-125`, `EnableWizardScreen.cpp:233-237` (left/right), `:302-306` (X) |
| 6 | The wizard member, its initialiser, and the `StartCommit` assignment | done | `EnableWizardScreen.h:117`, `.cpp:119` (ctor), `:573-577` (`StartCommit`) |
| 7 | `ui_scroll_verify.py`'s three 15-row entries to 16 | done | `app/tools/ui_scroll_verify.py:49-56` |
| 8 | `pool_verify.py` worst-case config 555 to 585 bytes, plus a new key case | done | `app/tools/pool_verify.py:683-697`, `:704-709` |

The three settings lists were re-extracted from the source after the edit and
compared entry by entry against the row constants. All three are sixteen
entries, identical from row 3 down, and index 15 in each is the new row —
matching `kDoNotRandomizeCagedDogsRow` on both screens:

```
  11 ENABLE MERGO DARKNESS
  12 ENEMIES INCLUDED
  13 ENEMIES SKIPPED
  14 BOSSES INCLUDED
  15 DO NOT RANDOMIZE CAGED DOGS
```

**Invariants checked before calling this done (§3.1, §7):**

* **No existing row constant moved.** `git diff` on the two screen files shows
  exactly these changed or added constant lines and no others:

  ```
  -const int kSaveDataRowCount    = 15;
  +const int kDoNotRandomizeCagedDogsRow = 15;
  +const int kSaveDataRowCount    = 16;
  -    static const int kItemCount = 15;
  +    static const int kItemCount = 16;
  +    static const int kDoNotRandomizeCagedDogsRow = 15;
  ```

* **Row constants and list positions agree** — the extraction above.
* **`ENEMIES SKIPPED` untouched** — not a line of its selection type, its
  picker, its table or its branches was edited; `pool_verify.py`'s 032 cases all
  still pass, unchanged.
* **`defaults.cfg` compatibility.** The new key is one more `strcmp` arm in a
  loader that ignores unknown keys, and the struct's own `false` stands when it
  is absent, so a file written by any earlier build loads with the protection
  off and every other setting keeping its meaning. Only the *selection values*
  are positional, never the lines themselves.
* **Layering.** No UI file names a map, an entity ID or a placement; both
  screens only pass a `bool`. `CagedDogList.h` is included by
  `EnemyRandomizer.cpp` alone, and nothing under `app/src/UI/` includes it.
* **The engine was not touched this milestone.** `EnemyRandomizer.cpp`,
  `EnemyRandomizer.h` and `CagedDogList.h` are exactly as milestone 1 left them,
  so the gate the hardware test just cleared is the same gate.
* **The setting is absent from `StartCommit`'s big `||`**, like
  `randomizeWorkshopTools` and unlike `enableMergoDarkness`: ticking it alone
  must not start a run that does nothing. It also gets no `SKIPPING …` progress
  line and no count line (§9 P12).

---

## 2. Deviations from the plan

**None.** All eight §7 changes are in the files §5 names, with the field name
and configuration key §4.4 gives, at the row index §4.4 fixes. No file outside
§5's milestone-2 rows was modified.

---

## 3. Decisions the plan left open

### 3.1 Where the new field sits inside `RandomizerDefaults`

§4.4 says "following `enableMergoDarkness` site for site" but not where in the
struct. Placed immediately after `enableMergoDarkness` and before `lastSeed`,
which keeps `lastSeed` — the one remembered run artifact rather than a
preference — last, and puts the new field in the same relative position it takes
in the UI lists and in the `defaults.cfg` line order. The struct is not
serialised by layout, so this is presentation only.

### 3.2 The stale byte count in `RandomizerDefaultsStore.cpp`'s comment

That file carries a comment reading "Worst case today is 555 bytes, pinned by a
pool_verify selftest case". §5 lists only load and save for this file, but adding
the key makes that sentence false, and the number it quotes is the one
`pool_verify.py` now asserts as 585. The comment was updated to 585 rather than
left contradicting the check that guards it. No behaviour, no format string and
no buffer size affected.

### 3.3 The log wording for the two new toggle branches

Both screens log every toggle, in the form `defaults: <setting> = YES` and
`enable wizard: <setting> = YES`. The new branches follow that exactly, using
the row's own words lowercased:

```
defaults: do not randomize caged dogs = YES
enable wizard: do not randomize caged dogs = NO
```

Neither uses the words "hunting dog" (B13), and neither mentions a count — the
per-run `caged dogs protected - 26 placement(s)` line from milestone 1 remains
the only place 26 appears, and only in `live.log`.

### 3.4 One report file, two milestones

The template says "one report per milestone" and `CLAUDE.md` §10 names one
`implementation-report.md` per feature folder. Milestone 2's report is appended
to that same file under its own title rather than put in a second file, so the
folder keeps the one artifact name the pipeline indexes. Milestone 1's report is
unchanged below its status line, which was updated to record the pass the
developer confirmed; leaving it reading "AWAITING HARDWARE TEST" would have been
stale from this milestone's first line.

---

## 4. Verification run

| Check | Command | Result |
| ----- | ------- | ------ |
| Clean build | `cd app && make clean && make` | `.pkg` built — `IV0000-BBRD00001_00-BBRANDOMIZERMAIN.pkg`, 7,143,424 bytes |
| No new warnings | full rebuild under `-Wall` | no warnings, no errors — which also covers `-Wformat` on the extended `snprintf` |
| Settings-list geometry | `python app/tools/ui_scroll_verify.py` | **PASSED** — the three 16-row lists still scroll and fit: Setup Defaults 7 rows visible of 16, both wizard lists 6 of 16 |
| Pool and config mirrors | `python app/tools/pool_verify.py selftest data/vanilla/dvdroot_ps4` | **86/86 passing** — the 85 existing cases unchanged, the config-size case now asserting 585, plus one new case |
| Pool unchanged | `python app/tools/pool_verify.py table enemy data/vanilla/dvdroot_ps4` | **PASS** — `EnemyPoolTable.h` matches the enemy pool, 82 models |
| Protected set recomputation | `python app/tools/caged_dogs_verify.py selftest data/vanilla/dvdroot_ps4` | **23/23 passing**, unchanged from milestone 1 |
| Failing direction still fails | `python app/tools/caged_dogs_verify.py protected data/vanilla/dvdroot_ps4 "data/runs/20260919-Enemies Only/dvdroot_ps4"` | **FAIL, as required** — `26 of 26 protected placement(s) were not frozen` |
| Row constants unmoved | `git diff` on both screen files, filtered to constant definitions | the lines quoted in §1, and nothing else |
| List and constant agreement | the three `items` vectors re-extracted and indexed | 16 / 16 / 16, new row at index 15 in each |

The two new `pool_verify` lines:

```
  033: worst-case defaults.cfg is 585 bytes and fits char buf[1024] ok
  033: do_not_randomize_caged_dogs is in both load and save ok
```

The second is deliberately two-sided: a key written but never read loads as off
forever, and a key read but never written is silently forgotten on save. Both
compile, and both look correct in a diff.

**Not run:** anything requiring a console — which is most of what this milestone
is about, since its only new user-visible behaviour is a row a player presses.
Specifically not run: the six hardware walks of §6; any test that the saved
`defaults.cfg` round-trips on the PS4's own filesystem; and any observation of
what the protected cages hold in game. There is no host C++ compiler, so the
round-trip is argued from the source and pinned by the mirror case above, not
executed.

---

## 5. What this does not prove

* **That the protection works.** Milestone 1's hardware test proved the *off*
  state is byte-identical to the pre-feature run. Nothing has yet run with the
  setting **on**, anywhere. Every claim about what the cages will hold is still
  an argument from the vanilla data plus the code, and `caged_dogs_verify.py`
  pins only the first half of that.
* **That the row toggles.** `ui_scroll_verify.py` mirrors the scroll and
  geometry arithmetic, not the input handling; nothing here has pressed a
  button. The branch is a copy of the one beside it, which is evidence and not
  proof.
* **That the setting persists.** The key is in both halves of the store and the
  mirror asserts both, but no `defaults.cfg` has been written or read on a PS4
  by this build.
* **That the row is readable on a TV.** `DO NOT RANDOMIZE CAGED DOGS   YES` is
  33 characters against the 53 the list fits at scale 4, and every character is
  in the 8x8 font's 42-glyph table, so it cannot come out silently blank — but
  the only place it has been drawn is arithmetic.
* **That protecting these ten cures the lag and the unkillable replacements.**
  Spec §4 H1 says the cause is untraced. Hardware test 5 below is where that
  comes back positive or negative, and negative is a real possibility.
* **That the `.pkg` boots.** A clean cross-compile is not a boot, and this
  milestone changed the layout of two structs the UI holds — which is why it was
  a `make clean` build (§9 P10, `docs/build.md`).

---

## 6. Hardware test handoff

Six runs, from plan §6. Install this build's
`app/IV0000-BBRD00001_00-BBRANDOMIZERMAIN.pkg` first.

1. **What to enable.** In the Enable wizard: `RANDOMIZE ENEMIES` **on**, and the
   new bottom row `DO NOT RANDOMIZE CAGED DOGS` **on**. It is row 16 of 16, at
   the very end of the settings list below `BOSSES INCLUDED`, reached by
   pressing down past the three drill-in rows. Left, right and X all toggle it.
   Any seed; the rest of the settings as you like. The same row is on Setup
   Defaults, in the same last position, if you would rather it default to on.

2. **What to do.** Commit, then check `live.log` for

   ```
   enemy randomizer: caged dogs protected - 26 placement(s) left unrandomized
   ```

   before launching the game. If that line is absent, the setting did not reach
   the engine and nothing below is worth walking. Then play to Central Yharnam
   and to the Forbidden Woods.

3. **What to look for.**
   * **Central Yharnam kennel yard** — six dogs in cages, behaving as they do in
     the unmodified game, including the two that get let out at you, and
     killable in the usual couple of hits. **Empty cages in that yard are
     expected and are vanilla:** there are 13 cage props and 6 caged dogs.
   * **Forbidden Woods cage cluster** — six cages, four with a dog, two empty.
     The empty pair is vanilla.
   * **The seventh yard dog** (the loose one, spec D5), the rest of Central
     Yharnam, and the other four Forbidden Woods dogs must **still be changed**.
   * Shaggy Hunting Dogs in **Cathedral Ward and Yahar'gul still change**, and
     the creature still turns up somewhere new — the pool is untouched.
   * **No frame-rate collapse in either cage area, and nothing in a cage that
     takes damage without dying.**
   * **A sixth run with the setting off** — both cage areas behave exactly as
     they do today, replacements and all.

4. **What a failure looks like.**
   * The `caged dogs protected` line missing, or naming a number other than 26:
     the setting is not reaching the engine, or the gate is matching something
     else. Stop and report.
   * A cage holding anything but a dog with the setting on: the identification
     is incomplete.
   * The lag or the unkillable replacements persisting **with the correct six
     and four dogs in place**: spec §4 H1's assumption is wrong. Report it — it
     is a stop condition, not a reason to widen the list to the seventh dog, to
     a stat row or to a collision surface.
   * Either cage area behaving differently with the setting **off**: the gate is
     reachable when it should not be, which milestone 1's byte-identical run
     said it is not.

5. **What to capture.** `live.log` and the output `dvdroot_ps4` tree from run 1
   (on) and run 6 (off), into `data/runs/`, so that

   ```
   python app/tools/caged_dogs_verify.py protected data/vanilla/dvdroot_ps4 "<tree>"
   ```

   can be run against both. It must **pass** for the on tree — all 26 frozen —
   and **fail** for the off tree, the same way it fails against
   `data/runs/20260919-Enemies Only/dvdroot_ps4` today. Those two runs are what
   turn every argument in §5 into evidence.

---

## 7. Stop point

**Completion gate of milestone 2**, reached in full: the eight §7 changes are
built, the clean build produced the `.pkg`, `ui_scroll_verify.py` passes with
three 16-row lists, `pool_verify.py`'s selftest passes 86/86 with no case other
than the config-size one changing state, and `caged_dogs_verify.py selftest`
still passes 23/23. No stop condition fired.

This is the plan's last milestone. What follows is the developer's six hardware
tests above, and then the documentation stage — which owes `docs/user-guide.md`
the description of this setting that §9 P11 moved out of the app, naming Central
Yharnam and the Forbidden Woods and never the words "Hunting Dog", and owes spec
§10 D10 the amendment P11 records.
