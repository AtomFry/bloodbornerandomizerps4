# Implementation Report — Randomizer Settings UI — milestone 1

**Status: AWAITING HARDWARE TEST**

**Plan:** `docs/features/randomizer-settings-ui/plan.md` — milestone 1,
"remove the save-data handling"

**Spec:** `docs/features/randomizer-settings-ui/spec.md`

**Implemented:** 2026-09-20

---

## 1. What was built

The save-data handling is gone in full, to the spec §4.8 inventory. Both
settings lists are 19 rows instead of 21, `Step::SelectReplace` and everything
behind it is deleted, `StartCommit`'s first progress line is now
`USING SEED …`, and neither `defaults.cfg` key is read or written any more. No
randomizer behaviour was touched: the run decision, the
`EnemyRandomizerOptions` assignments and every `SKIPPING …` line other than the
backup one are byte-for-byte what they were.

| # | Planned change | Status | Where |
| - | -------------- | ------ | ----- |
| 1 | Remove the two fields from `RandomizerDefaults` | done | `app/src/Randomizer/RandomizerDefaults.h:33-39` — replaced by a tombstone comment in the house style of the `unchanged_bell_maidens` and `CUSA03175` notes |
| 2 | Remove both keys from the store's load chain, the save format string and its argument list; extend the tolerance comment | done | `app/src/Randomizer/RandomizerDefaultsStore.cpp:43-46, 108-157`. Conversions and arguments still correspond one for one: 1 `%s`, 16 `%d`, 3 `%s`, 1 `%u` against title ID, 16 bools, 3 encodes, `lastSeed` |
| 3 | `SetupDefaultsScreen.h` constants as a sequential block in `DrawList`'s order, two rows dropped from both, `kItemCount` 19 | done | `app/src/UI/SetupDefaultsScreen.h:33-83`, `app/src/UI/SetupDefaultsScreen.cpp:76-80, 243-270` |
| 4 | The same for `EnableWizardScreen.cpp`'s constants and **both** `items` vectors, `kSaveDataRowCount` 19 | done | `app/src/UI/EnableWizardScreen.cpp:21-68`, both vectors. Verified mechanically: all three vectors are 19 entries, identical in shape, and each index matches its constant (see §4) |
| 5 | Delete `Step::SelectReplace`, `Update/DrawSelectReplace`, `ReplaceChoice`, `ReplaceDisplayText`, `kReplaceOptions` and friends, the three members | done | `app/src/UI/EnableWizardScreen.h`, `app/src/UI/EnableWizardScreen.cpp`. The `Step` switch has no missing case — the build is clean with no `-Wswitch` diagnostic. `kDetailScale` went with `DrawSelectReplace`, its only user |
| 6 | Delete `StartCommit`'s backup line and three-way replace block | done | `app/src/UI/EnableWizardScreen.cpp` — a run's first progress line is now `USING SEED …` |
| 7 | Replace the `Application.cpp` startup log line | done | `app/src/Application.cpp:47` — `defaults: loaded (bloodborne title id = <id>)` |
| 8 | Update `ui_scroll_verify.py` and `pool_verify.py` | done | three 21-row entries → 19, progress-log budget 20 → 18 and its running-state loop with it; worst-case config 669 → 616, one new case. No other case changed state |
| 9 | Update `docs/user-guide.md` | done | the two at-a-glance rows are gone and `## Save data settings` is now `## Save data`, stating plainly that the app does not touch save data at all |

### Invariants checked

* **`StartCommit`'s run decision** — unchanged. The big `||` still omits
  `randomizeWorkshopTools` and `doNotRandomizeCagedDogs`; the diff touches no
  line inside it.
* **`EnemyRandomizerOptions` populated field by field from the same values** —
  not one `options.` line appears in the diff.
* **Only `lastSeed` is written back** — `defaults_.lastSeed = seed;` is still
  the only assignment into `defaults_`.
* **The four progress-log constants** — `kEnemyFailPrefix`,
  `kPoolFellBackLine1`, `kPoolFellBackLine2`, `kNothingRandomizedLine` keep
  their names, their `const char* const kName = "...";` form and their file.
  `pool_verify.py` still parses all four (cases green).
* **`defaults.cfg` tolerance** — the loader is still an `if/else if` chain over
  known keys with everything else ignored, so a file carrying
  `backup_existing_save` or `replace_save_default_is_new` loads with those
  lines dropped and every other key honoured (B14).
* **Pool tables not regenerated** — `EnemyPoolTable.h`, `EnemySkipTable.h`,
  `BossPoolTable.h` untouched; `pool_verify.py table both` still passes.
* **Layering / `LIBS`** — no new SDL2 call, no `Makefile` change.

---

## 2. Deviations from the plan

None. Every §7 step was implemented as written, in the order given.

Two choices the plan did not settle are in §3 below; neither changes what §7
asked for.

---

## 3. Decisions the plan left open

### 3.1 The wizard's on-screen sub-heading, `SAVE DATA` → `SETTINGS`

**What the plan says.** §5 and §7 do not mention it. §4.6 and milestone 4 step 2
rename `Step::SaveData` to `Step::Settings`, which is milestone 4 work.

**What was done.** The C++ identifier `Step::SaveData` is untouched — that
rename stays in milestone 4. Only the drawn sub-heading at
`EnableWizardScreen.cpp` `DrawSaveData`, y 260, changed from `"SAVE DATA"` to
`"SETTINGS"`, with a comment saying why and noting the identifier rename is
still to come.

**Why.** With both save rows gone the step is nothing but the settings list
(spec §4.8: "the wizard's entry step becomes the Settings screen"), and the
milestone-1 hardware test asks the developer to read 19 rows with the right
labels on a screen headed `SAVE DATA` that contains no save data. Leaving it
would have made the test screen actively misleading for one milestone. The
alternative — leaving the string and noting it — was rejected as costing the
developer a confusing screen for no gain, since milestone 4 replaces the
heading entirely either way.

**Risk if wrong.** Reverting is a one-string change.

### 3.2 Stale header and tombstone comments

**What the plan says.** Nothing about comments beyond §4.5 (a milestone-2 item)
and §7 step 2's "extend the tolerance comment to name them".

**What was done.** In the three files the milestone already edits:

* `EnableWizardScreen.h`'s file-header comment described Screen 1A, the stub
  backup list and the simulated save-data steps in detail. It was rewritten to
  describe the flow that now exists and to record what was removed and why.
* `SetupDefaultsScreen.h`'s first line named `BACKUP EXISTING SAVE` as one of
  the screen's two things. Rewritten.
* `RandomizerDefaults.h` gained a tombstone comment where the two fields were,
  matching how this file already records the retired `unchanged_bell_maidens`
  and the `CUSA03175` correction.
* Both row-constant blocks gained a paragraph recording that the two save rows
  went and every row below moved up two — the same note feature 032 left when
  it retired a row. Stale "kItemCount goes with it, 15 -> 16" arithmetic in the
  older notes was trimmed where it described a count that no longer exists.
* The `// Position N, matching k…Row` comments inside all three `items` vectors
  were renumbered with their rows.

**Why.** Every one of these is inside a file §5 already assigns to milestone 1,
and each would otherwise describe code that no longer exists — the exact
condition the row-constant hazard (§3.3) exists to prevent.

**Deliberately not done:** the tombstone comments name the retired **keys** in
the store (§7 step 2 requires it) but avoid spelling the retired **field**
identifiers, so a future `settings_ui_verify.py` case 7 that greps `app/src`
for the §4.8 inventory is not tripped by prose. The one file that does name the
keys is the one the plan told me to make name them.

### 3.3 Shape of the new `pool_verify.py` case

**What the plan says.** §5: "new case asserting neither key survives in load or
save."

**What was done.** The case checks the quoted key literal (`"backup_existing_save"`),
the `key=%d` fragment of the save format string, and the struct field name the
argument list would have used — the same three-part shape as the existing
`start_with_hunter_tools` and `do_not_randomize_caged_dogs` cases, inverted.

**Why.** A plain "the string does not appear in the file" check would fail
against the tolerance comment that §7 step 2 requires to name both keys. The
case is written so prose cannot make it pass or fail, and says so in a comment.

---

## 4. Verification run

| Check | Command | Result |
| ----- | ------- | ------ |
| Clean build | `cd app && make clean && make` | `IV0000-BBRD00001_00-BBRANDOMIZERMAIN.pkg` built, 7 143 424 bytes. No warnings or errors |
| Scroll and geometry | `python app/tools/ui_scroll_verify.py` | `all geometry and scroll properties PASSED` — Setup Defaults 7 of 19, Wizard SaveData 6 of 19, Wizard Confirm 6 of 19, Progress log 9 of 18 |
| Pool, strings, config | `python app/tools/pool_verify.py selftest data/vanilla/dvdroot_ps4` | `88/88 passing` (87 before, plus the new save-data-removal case). The config case reads `worst-case defaults.cfg is 616 bytes and fits char buf[1024] ok`. No other case changed state |
| Pool tables unchanged | `python app/tools/pool_verify.py table both data/vanilla/dvdroot_ps4` | `PASS: EnemyPoolTable.h matches the enemy pool - 82 models`, `PASS: BossPoolTable.h matches the boss pool - 17 models` |
| Boss engine mirror | `python app/tools/boss_verify.py selftest data/vanilla/dvdroot_ps4` | `selftest 5/5`, unmodified tree CLEAN |
| Caged dogs mirror | `python app/tools/caged_dogs_verify.py selftest data/vanilla/dvdroot_ps4` | `23/23 passing` |
| Easy modes mirror | `python app/tools/easy_modes_verify.py selftest data/vanilla/dvdroot_ps4` | `25/25 passing` |
| Hunter tools mirror | `python app/tools/hunter_tools_verify.py selftest data/vanilla/dvdroot_ps4` | `selftest 13/13` |
| Font atlas | `python app/tools/font_atlas_verify.py` | `PASS - baked metrics match the typeface and the draw maths lands where PIL puts it` |
| Row/label correspondence | ad-hoc parse of the three `items` vectors | all three are 19 entries, identical in shape, index *n* matches constant *n* for all 19 |

Nothing failed, so there is no failure output to paste.

**Not run:** `settings_ui_verify.py` does not exist yet — it is written in
milestone 3. `starting_weapons_verify.py` and the other verifiers not listed in
the plan's §6 were not run; nothing in this milestone touches what they mirror.

---

## 5. What this does not prove

Per `CLAUDE.md` §3, a clean cross-compile and green Python mirrors mean **ready
for hardware testing** and nothing stronger.

Specifically unproven until the PS4 runs it:

* that an existing `defaults.cfg` written by the shipped build still loads with
  every remaining setting correct — the store is not exercised off-console;
* that both settings screens draw 19 rows with each label against the right
  value — the verifiers check the arithmetic and the index correspondence, not
  the pixels;
* that a commit's progress log contains none of the five save-data lines and is
  otherwise unchanged;
* that nothing in the removal disturbed a run — the strongest statement
  available here is that no line of the run decision or the option assignments
  appears in the diff. The byte-identical output-parity run is a milestone-4
  test.

---

## 6. Hardware test handoff

Install `app/IV0000-BBRD00001_00-BBRANDOMIZERMAIN.pkg`.

**Before anything else:** confirm `live.log` says `FontAtlasInit ok`. If it
says `FontAtlasInit: texture build failed`, stop — the app has fallen back to
`Font8x8` and every later milestone's measurements assume the atlas is the live
text path (plan §3.3, stop condition 3).

1. **Old config still loads.** Start with a `defaults.cfg` from the shipped
   build — one that still carries `backup_existing_save=` and
   `replace_save_default_is_new=`, ideally with several randomizer settings
   turned on. Launch the app.
   * `live.log` should now say `defaults: loaded (bloodborne title id = <id>)`
     rather than the old backup line.
   * Open **SETUP DEFAULTS** and check every setting reads back what the file
     said. **Failure looks like:** a setting reading `NO` that the file has as
     `1`, or the title ID reverting to `CUSA03173`.

2. **Both screens show 19 rows.** In **SETUP DEFAULTS**, scroll top to bottom.
   The list must be `BLOODBORNE TITLE ID`, then `RANDOMIZE ENEMIES` through
   `EASY EMISSARY` — 19 rows, with **no** `BACKUP EXISTING SAVE` and no
   `DEFAULT REPLACE SAVE`. Do the same in **ENABLE RANDOMIZER**, where row 0 is
   `SEED` and there is no `REPLACE SAVE`.
   * Check every label sits against a sensible value — this is the renumbering
     test, and **failure looks like** a correct-looking list where toggling one
     row changes a different row's value, or a row showing `2 OF 82` style text
     against a `YES`/`NO` label.
   * The wizard's sub-heading now reads `SETTINGS`, not `SAVE DATA`.
   * Press `X` on `BLOODBORNE TITLE ID` (defaults) and on `SEED` (wizard) —
     both editors must still open and return to the same row.

3. **Save the defaults.** Press `OPTIONS` in Setup Defaults, then FTP
   `/data/bbrandomizer/defaults.cfg` off the console. It must have no
   `backup_existing_save` line and no `replace_save_default_is_new` line, and
   every other key must be present and correct.

4. **Commit a run.** In the wizard, turn on `RANDOMIZE ENEMIES` (anything else
   is fine too), press `OPTIONS` twice and let it finish.
   * The **first** progress line must be `USING SEED <n>`.
   * None of these five may appear anywhere in the log:
     `BACKING UP EXISTING SAVE (SIMULATED)`, `SKIPPING BACKUP PROCESS`,
     `REMOVING EXISTING SAVE DATA (SIMULATED)`, `LEAVING EXISTING SAVE DATA`,
     `RESTORING SAVE DATA … (SIMULATED)`.
   * Everything else — the `SKIPPING …` lines, the counts, the three closing
     lines — must read exactly as it did before.
   * **Failure looks like:** a missing `SKIPPING …` line, a changed count, or
     the commit not starting at all.

5. Launch Bloodborne and confirm the run is there, as a sanity check that the
   struct-layout change did not disturb the engine.

---

## 7. Stop point

Milestone 1's **completion gate** ended this pass: every §6 check for the
milestone passes and the `.pkg` builds. No stop condition fired.

Milestone 2 — renderer primitives and the honest geometry model — has **not**
been started, per `CLAUDE.md` §4. It waits on the hardware test above.
