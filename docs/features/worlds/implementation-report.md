# Implementation Report — Worlds — milestone 6

**Status: HARDWARE TESTED — PASSED** (2026-09-25)

**Plan:** `docs/features/worlds/plan.md` — milestone 6, "activation from the UI,
and retirement"

**Spec:** `docs/features/worlds/spec.md`

**Implemented:** 2026-09-24

> Milestone 5's report is preserved unchanged below the rule at the end of this
> one, milestone 4's beneath that, and so on. One report per milestone, newest
> first, in the folder's one report file.
>
> **No hardware test has been run for any milestone of this feature.** H4, H5,
> H6–H9, H10 and H11 are all outstanding. The developer directed that
> milestones 5 and 6 be built back to back before testing, so this milestone is
> built on milestones 1–5 exactly as written and re-verifies none of them on a
> console.
>
> **This is the last milestone.** The feature is code-complete and every
> automated check in the plan's §6 passes. What that means, and what it does
> not, is §5.
>
> **P26 is implemented here.** The developer took it on 2026-09-24, after §7's
> step list was written, so the six steps below do not mention it. It is
> deviation §2.1, and it closes the B37/B8 contradiction milestone 3's report
> §2.4 raised as blocking this milestone.

---

## 1. What was built

The editor activates. `OPTIONS` on the world editor writes the world and opens
**Confirm**, which is now the activation confirmation B10 describes: which world
is being deactivated and where its save goes, which world is being activated,
which row of the plan's §4.3 phase-6 table applies in the player's words, and
roughly how long it takes — or, when the transaction refuses, the
`RefusalReason` and its sentence, with `OPTIONS` withdrawn from the footer.
Every clause is built from `PlanActivation`, phase 1 itself, run once on
Confirm's first frame and writing nothing. `OPTIONS` again runs
`WorldActivationJob` through the existing progress log, one phase per frame.

`X` on the `WORLDS` rail's `VANILLA` row routes to the same confirmation (B8):
the editor is opened on the id `vanilla`, which has no settings and no
revisions, so it starts in Confirm and `O` returns to the `WORLDS` tab.

Everything the worlds model replaces is gone. `UI/MenuScreen.{h,cpp}`,
`UI/PlaceholderScreen.{h,cpp}`, `Platform/SaveDataProbe.{h,cpp}` and
`UI/SaveProbeScreen.{h,cpp}` are deleted, with `ScreenId::Menu`,
`::EnableWizard`, `::DisableWizard` and `::SaveDataProbe`. `ScreenId` is three
entries now — `Worlds`, `Defaults`, `WorldEditor` — and the `DEFAULTS` tab's `O`
returns to `WORLDS` rather than to a menu that no longer exists.
`-lSceSaveData` stays.

The randomizer run moved with the rest. `UI/WorldEditorScreen.cpp` no longer
builds an `EnemyRandomizerOptions`, no longer evaluates the run decision, and no
longer names `/data/GoldHEN/AFR` or the vanilla source: there is one copy of
each, in `Game/WorldActivation.cpp`'s phase 4, built from the world's **current
revision** rather than from the screen's working copy. `/data/GoldHEN/AFR` now
appears in two files, `AfrManager.cpp` and the older `GameInfo.cpp`, where it
appeared in three.

| # | Planned change | Status | Where |
| - | -------------- | ------ | ----- |
| 1 | Confirm: the B10 statement, or the §4.4 refusal, plus a coarse duration | done | `WorldEditorScreen.cpp:836-893` (plan), `:1472-1541` (rows), `:1542-1614` (draw) |
| 2 | Progress runs `WorldActivation`, reporting each phase into the existing log | done | `WorldEditorScreen.cpp:895-914,943-1007,1009-1100`; `.h:282` |
| 3 | `X` on `VANILLA` routes to the same confirmation | done | `WorldsScreen.cpp:476-488`, `WorldEditorScreen.cpp:325-334,391-400` |
| 4 | Surface phase 7's revert (B12) | done | `WorldEditorScreen.cpp:1060-1080` |
| 5 | Delete `MenuScreen`, `PlaceholderScreen`, `ScreenId::EnableWizard`, `::DisableWizard` | done — `ScreenId::Menu` went too, §2.2 | `UI/Screen.h:18-32`, `Application.cpp:1-45`, `SetupDefaultsScreen.cpp:135,172` |
| 6 | Delete `SaveDataProbe`, `SaveProbeScreen`, `ScreenId::SaveDataProbe`, the menu row | done — `grep -r SaveDataProbe app/src` is empty | deleted; `Makefile` untouched |
| 7 | Run the full parity suite | done — every verifier passes | §4 |
| — | **P26**: remove the `AFR title` refusal | done — not in §7's list, §2.1 | `WorldActivation.h:88-103,150-155`, `.cpp:118-133,205-233,1140-1160`, `worlds_verify.py` |

---

## 2. Deviations from the plan

**Six, all recorded in the plan's §10.**

### 2.1 P26 was implemented in this milestone, and §7's step list predates it

**The plan says**, in §2's B37 and §4.4's `AFR title` row, that a configured
`BLOODBORNE TITLE ID` which `GameInfo::DetectAll` did not find refuses, listing
what was detected. **P26 (§9, 2026-09-24) supersedes the second sentence of
P25/D27**: the setting is used exactly as entered, and AFR handling never infers
or validates it.

**What was done.** Removed, in one pass:

* `RefusalReason::AfrTitleNotDetected` and its `RefusalReasonName` case;
* the `CheckActivation` row and its sentence;
* `ActivationFacts::afrTitleDetected` and `::detectedAfrTitles`;
* the `GameInfo::DetectAll` sweep in `GatherFacts`, and with it the
  `#include "GameInfo.h"` in `WorldActivation.cpp`;
* `worlds_verify.py`'s mirror of all of it — the reason in `REFUSAL_REASONS`,
  the `afr_detected` fact, the `check_activation` row, the refusal case, and the
  "vanilla still refuses on an undetected AFR title" case.

`AfrNotWritable` stays: whether the folder the setting names can be written to
is a fact about the folder, not an inference about the title. Save-title
discovery is untouched and still inspects `param.sfo`, which is §3.1's "the AFR
title and the save-data title are independent values".

Two replacements went in rather than leaving holes. `worlds_verify.py` now
asserts that Vanilla still refuses on an **unwritable** AFR folder, and that
nothing refuses on the configured title itself — stated as a case so the absence
is deliberate rather than an editing accident.

**Why it matters.** `GameInfo::DetectAll` finds a title by looking for
`<title>/dvdroot_ps4/event/common.emevd.dcx` **inside its AFR folder**. AFR is
an overlay, so that tree exists only because the randomizer wrote it. Activating
Vanilla removes it (B8, §4.3 phase 5), after which the check refused every
subsequent activation for that title — the one-way door milestone 3's report
§2.4 set out. H11 can now run.

**Scope note.** This required editing `Game/WorldActivation.{h,cpp}`, which §5
assigns to milestone 3, and `worlds_verify.py`, which §5 assigns to milestones
1–3. §1–§9 of the plan are unedited, as P26 instructs; §2's B37 and §4.4's row
therefore still describe a check the code no longer has, and §10 says so.

### 2.2 `ScreenId::Menu` was removed with `MenuScreen`

**The plan says** (§7 step 5) to delete `MenuScreen.{h,cpp}`,
`PlaceholderScreen.{h,cpp}`, `ScreenId::EnableWizard` and
`ScreenId::DisableWizard`, "done when nothing references them". It does not name
`ScreenId::Menu`.

**What was done.** `Menu` was removed too. **Why.** Its only screen was
`MenuScreen`; keeping it would have left an id `MakeScreen` could not build and
no screen could usefully request — precisely the dangling id the step's own
done-condition is about. `ScreenId` is now `None`, `Worlds`, `Defaults`,
`WorldEditor`.

### 2.3 `UI/SetupDefaultsScreen.cpp` was edited, and §5 assigns it to milestone 4

**What was done.** Its two `requestedScreen_ = ScreenId::Menu` lines — after an
`OPTIONS` save, and on `O` from the rail — became `ScreenId::Worlds`.

**Why.** They were the last two references to the deleted id. `WORLDS` is the
only other screen there is, and it is the tab this one sits beside, so `O` on
`DEFAULTS` crosses back to it and `O` there exits the app. The footer already
said `O BACK` and still does. Flagged rather than buried because it is a file
this milestone was not given; milestone 4's record made the same flag about the
same file's `ScreenId::Defaults` rename.

### 2.4 Confirm's geometry moved, and both UI verifiers were edited here

**The plan says** (§5) that `settings_ui_verify.py` and `ui_scroll_verify.py`
belong to milestones 4 and 5.

**What was done.** B10's statement is five more head rows on Confirm's review
list, three to eight, and two wrapped rows of sentence above it. At
`kSettingsLayout`'s old `{420, 90, 870, 60}` the list's `MORE ABOVE` hint at
y=360 drew its ink from 369, inside the second sentence row's 375–410 — which
`ui_scroll_verify.py` caught on the first run. The layout is now
`{470, 80, 870, 60}`: the same six visible rows of a list that is 27 rows long,
with the band cleared by 23px.

`settings_ui_verify.py` gained eleven cases and a parser for
`Game/WorldActivation.cpp`'s two switch tables and every `Refuse(...)` sentence,
so the strings Confirm draws but does not own are measured too.
`ui_scroll_verify.py`'s `Editor Confirm` entry is now
`470, 80, 870, 4, 60, 27, 352, 3` — the thing above the list is the second
sentence row, not the state line.

**Why it is a deviation and not just work.** Neither verifier is assigned to
this milestone in §5, and both were changed. Neither was relaxed: 81 of 81 and
all geometry passing, with every new assertion measured off the atlas.

### 2.5 `worlds_verify.py`'s output-parity check lost what it compared against

**The plan says** (§3.1) that output parity means `EnemyRandomizerOptions` is
still populated field by field from the same values, and the run decision keeps
its `||` chain. `worlds_verify.py` enforced that by comparing
`WorldActivation.cpp`'s copy with `WorldEditorScreen.cpp`'s, under a comment
reading "Milestone 6 deletes the wizard's copy; until then the two must say the
same thing".

**What was done.** Step 2 deleted the wizard's copy, so the comparison has one
side. The wizard's two lists are now **frozen into the verifier** — the eighteen
`options.<field> = run.<field>` pairs in order, and the thirteen fields of the
`||` chain — and the surviving copy is compared against those. Two new cases
assert the editor has no second mapping and no `EnemyRandomizerJob` of its own,
so the parallel structure cannot grow back.

**Why.** A frozen list is weaker than a live comparison — it is a
hand-transcribed baseline rather than two things agreeing — but it is what is
left once there is one copy, and it makes adding a field to
`EnemyRandomizerOptions` a deliberate act with the output question asked out
loud. Deleting the check would have left B28 unpinned entirely.

One more case in the same file read the now-deleted `SaveDataProbe.cpp` to
assert the harness refused more than one save directory. It now reads the same
rule out of phase 1, which is where the production code enforces it.

### 2.6 The `SKIPPING` lines and the generation's result lines stayed in the editor

**The plan says** (§7 step 2) that Progress runs `WorldActivation` rather than
`EnemyRandomizerJob`, and (§3.1) that the four progress-log constants keep their
names and `const char* const` form because `pool_verify.py` parses them out of
the file by name, and that the run decision keeps its `SKIPPING …` lines.

**What was done.** The options mapping, the `||` chain and the output path left
`WorldEditorScreen.cpp`. The **reporting** did not:

* `StartCommit` still emits the seven `SKIPPING …` lines from `run_`, before
  starting the job. §3.1 keeps them and `WorldActivation` has never had them —
  its phase 4 says `NO RANDOMIZER SETTINGS ARE ON - THE TREE IS A PLAIN MIRROR`
  instead, which is a different statement.
* `FinishCommit` still reports the generation — enemy, boss, treasure, starting,
  shop, drop, Mergo, hunter-tools, easy-mode, pool, skip and item-data lines,
  `kPoolFellBackLine1/2`, `kNothingRandomizedLine` and `kEnemyFailPrefix` — out
  of `WorldActivationJob::Result().generation`.

Moving that reporting into `WorldActivation.cpp` would have taken the four
constants with it, out of the file `pool_verify.py:533` parses.

**The one seam this leaves.** Those lines are driven by `run_`, the editor's
working copy, while the tree is generated from the world's current revision.
They are the same thing because `OPTIONS` writes the revision before Confirm
opens — and `OPTIONS` on Confirm is **refused whenever `storeError_` is set**,
which is the only way that write can have failed. Vanilla emits neither: it has
no recipe.

---

## 3. Decisions the plan left open

### 3.1 Vanilla routes through the editor, opening on Confirm

§7 step 3 says `X` on `VANILLA` "routes to the same confirmation" and does not
say which screen owns it. The alternative was a fifth `WorldsScreen` mode
duplicating the statement.

`WorldsScreen` now requests `ScreenId::WorldEditor` with the id `vanilla`, like
any other row. The editor sets `isVanilla_` in its initialiser list, skips
`LoadWorld`'s current-revision read (Vanilla has none, and asking for one
reported a missing revision as an error), and calls `OpenConfirm()` from its
constructor. `O` on Confirm returns to `WORLDS` for Vanilla instead of to a
Settings step it never had.

One confirmation, one transaction, one progress log. The cost is a screen titled
`WORLD EDITOR` briefly heading a Vanilla confirmation; the state line under it
says `CONFIRM ACTIVATION` and the rows name Vanilla throughout.

### 3.2 Eight fixed head rows, not a variable statement block

B10 lists five things the confirmation must state. They are label/value rows at
the head of the same review list the three existing ones (`NAME`, `SEED`,
`TARGET`) sit at, and there are always exactly eight, refused or not.

A block that appears only sometimes is a geometry to measure twice and a screen
whose shape moves under the player. The refused case fills the two save rows
with `-` rather than dropping them: a refusal changes neither save, and
describing a copy that will not happen is worse than saying nothing.

Vanilla contributes **no settings rows at all** below the eight. It has no
recipe (B20), and listing the `DEFAULTS` tab's settings beside `VANILLA` would
state a randomization that is about to be deleted.

### 3.3 The sentence band: one place, two purposes, two lines, the row scale

The phase-6 row in the player's words (`SaveActionSentence`) and the refusal's
own sentence are both far too wide for scale 4 — the longest refusal is 102
characters with its runtime half at worst case. They share one wrapped band of
`kConfirmSentenceMax = 2` lines at `kConfirmSentenceW = 1800`, row scale, under
the state line: `Palette::Dim` when the activation will run, `Palette::Bad` when
it will not.

Two lines is what fits between the state line and the list, and
`settings_ui_verify.py` asserts every one of the nineteen sentences wraps into
them. A `storedSaveError` longer than the 24-character stand-in the verifier
measures would be truncated on screen rather than overflowing it; §3.15 says why
that stand-in.

### 3.4 The coarse duration is three phrases, and here is the arithmetic

B10 asks for "roughly how long it takes". `DurationText()` returns one of
`ABOUT A MINUTE` (anything that generates a tree), `UNDER A MINUTE` (Vanilla
with a save to copy), `A FEW SECONDS` (Vanilla with nothing to copy) or
`A MOMENT` (the plan is still being built).

From `technical-findings.md`'s measured ~15 MB/s on a ~27 MB save: a safety
backup and its capture are ~8 s together, a restore ~4 s, and a full generation
is the 10–20 s `plan-evidence.md` records — a worst case around half a minute. A
number would be false precision on a console whose disk speed has been measured
once.

### 3.5 `OPTIONS` is gated on `storeError_` as well as on the refusal

`OPTIONS` on Confirm activates only when the plan is ready, `plan_.ok` is true
**and** `storeError_` is empty. The third is not a refusal — the world may exist
and be perfectly activatable — but if the store could not write this visit's
edits, the recipe on screen is not the recipe on disk, and activating would
generate something the player never approved. The footer drops to `O BACK` in
that case rather than offering a button that silently declines.

### 3.6 Phase 1 runs on Confirm's first frame, not on the `OPTIONS` press

`PlanActivation` reads the whole save container and walks the incoming world's
stored save — a second or two of blocking work in a strictly serial frame loop.
`OpenConfirm()` sets `planPending_` and switches step; `UpdateConfirm` does the
work on the next frame, by which time `CHECKING` has already been drawn. This is
the same "say it one step before you do it" shape phase 1 uses inside the job.

### 3.7 The state line's precedence

`storeError_`, then the refusal, then the last `OPTIONS` outcome, then
`CONFIRM ACTIVATION`. The store error wins because it is the cause — a refusal
about a world that was not written is a symptom. `saveNote_` keeps the slot it
had in milestone 5, so B14's rename-versus-revision distinction is still stated
where it was.

### 3.8 Drained job lines are pushed into the log without a second `Log()`

`WorldActivationJob::Say` already writes every line to `live.log` under
`activation:`. `UpdateProgress` pushes `TakeLines()` straight into
`progressLines_` rather than through `AddProgressLine`, which would log each one
again under `world editor:`. The log a hardware test comes back with is the
whole diagnostic surface, and two copies of every line makes it harder to read.

### 3.9 `LoadWorld()` is re-read after a successful activation

Phase 7 appends a revision putting `SAVE DATA` back to `KEEP EXISTING` (B12),
which changes the world on disk under a screen still holding the old copy.
Nothing navigates from Progress back into Settings today — `O` goes to
`WORLDS` — so this is belt and braces, but a screen holding a recipe that is no
longer the world's is a bug waiting for the first edit that does. The revert is
also stated on the log: `SAVE DATA IS BACK TO KEEP EXISTING - rev-NNNN`.

### 3.10 `kSettingsLayout` is `{470, 80, 870, 60}`

The band had to move down for the sentence rows and could not afford to lose a
visible row: 27 rows in a six-row window is already four screenfuls. Tightening
the pitch from 90 to 80 — against a scale-4 line box of 59 — keeps six. The
layout is Confirm's alone now; nothing else draws it.

### 3.11 Two Confirm footers, chosen by whether `OPTIONS` would do anything

`OPTIONS ACTIVATE   O BACK` when it would, `O BACK` when it would not. The verb
changed from `COMMIT` because the button no longer commits a randomization; it
runs a transaction that swaps save data too.

### 3.12 A refusal and a failure end the log differently

`ACTIVATION REFUSED - NOTHING WAS CHANGED` and
`ACTIVATION FAILED - CHECK THE LOG FOR DETAILS`. They mean opposite things about
the console — one changed nothing at all, the other stopped partway and left a
journal for the next launch — and one line for both would have hidden that. The
three-line flourish is gated on neither having happened.

### 3.13 `Game/GameInfo.{h,cpp}` is left in place with no caller

P26 removed its last call. §5 does not list it as deleted, and §7's stop
conditions make "the change needs a file not listed in §5" a halt, so it stays:
it compiles, it links, and it costs nothing but its own object file. It is §7's
first "noticed and not fixed", and it is the second of the two files
`worlds_verify.py`'s AFR-path allowlist still permits.

### 3.14 The AFR-path allowlist is down to two files

`worlds_verify.py` pinned `/data/GoldHEN/AFR` to `AfrManager.cpp`,
`GameInfo.cpp` and `WorldEditorScreen.cpp` — the last two predating §3.1's
one-file rule. Step 2 removed the editor's, because the screen stopped building
its own output path when it stopped running the randomizer. The allowlist is now
the two, and the case's wording says so.

### 3.15 `settings_ui_verify.py` measures a refusal's runtime half as 24 characters

Three refusal sentences concatenate runtime text: a 9-character title id, up to
two 10-digit block counts, and `storedSaveError`, which is whatever the manifest
walk said. The parser substitutes `RUNTIME_STANDIN`, 24 `X`s, for each — longer
than either of the first two, and a stated budget for the third. A stored-save
error longer than that wraps past the two-line band and is truncated on screen;
the sentence's first line still names the world and the problem.

---

## 4. Verification run

| Check | Command | Result |
| ----- | ------- | ------ |
| Clean build | `rm -rf app/src/x64 && cd app && make` | `IV0000-BBRD00001_00-BBRANDOMIZERMAIN.pkg`, **no warnings** |
| Worlds mirror | `python app/tools/worlds_verify.py` | 29/29, 54/54, 54/54, **97/97** — all checks passing |
| Settings UI | `python app/tools/settings_ui_verify.py` | **81/81** passing |
| Scroll geometry | `python app/tools/ui_scroll_verify.py` | all geometry and scroll properties PASSED |
| Pool / parity | `pool_verify.py selftest ../../data/vanilla/dvdroot_ps4` | 89/89 passing |
| Boss | `boss_verify.py selftest …` | selftest 5/5 |
| Treasure | `treasure_verify.py selftest …` | selftest 8/8 |
| Drops | `drops_verify.py selftest …` | selftest 6/6 |
| Starting weapons | `starting_weapons_verify.py selftest …` | selftest 12/12 |
| Caged dogs | `caged_dogs_verify.py selftest …` | 23/23 passing |
| Easy modes | `easy_modes_verify.py selftest …` | 25/25 passing |
| Hunter tools | `hunter_tools_verify.py selftest …` | selftest 13/13 |
| Mergo darkness | `mergo_darkness_verify.py selftest` | 9/9 passing |
| Item data | `itemdata_verify.py roundtrip ../../data/vanilla/dvdroot_ps4` | PASS — 65 entries, byte-identical round trip |
| Hardware | H4, H5, H6–H9, H10, H11 | **not run** — §6 |

Nothing failed. The clean was `rm -rf app/src/x64` rather than `make clean`:
`clean` removes the objects its own `find` currently produces, which no longer
includes the four deleted pairs, so stale objects would have survived it. §3.3's
hazard row asks for a clean rebuild in every milestone, and this one deletes
eight files and changes a screen's member layout.

`worlds_verify.py`'s source-invariant section went from 89 cases to 97: two lost
(the AFR-title refusal, the probe's directory check), ten gained (the two frozen
parity lists, the two "no second copy" cases, four in-use cases for the
progress-log constants, and the seven `SKIPPING` lines, less the re-pointed
directory check). Its refusal section is 54 where it was 56.

---

## 5. What this does not prove

Everything in §4 is a cross-compile and a set of Python mirrors. `CLAUDE.md` §3
is unchanged by any of it: **the PS4 is the only authority on runtime
behaviour**, and this feature has never run on one.

Specifically, nothing here establishes:

* that an activation completes on hardware at all. No phase of the transaction
  has ever run on a console, in this milestone or in 1 through 5;
* that the confirmation's figures are the figures the console produces.
  `PlanActivation` is a second or two of blocking work, and whether it reads as
  a pause or as a hang is a thing to watch for;
* that the duration phrases in §3.4 are right. They are arithmetic on one
  throughput measurement;
* that the tree an activation generates is byte-for-byte the tree the Enable
  wizard generated for the same seed and recipe. The options mapping and the
  `||` chain are pinned as source text; the bytes are not, and only a run on
  hardware followed by a verifier pass against the output tree shows that;
* that removing four screens left nothing unreachable in a way a player would
  notice. The build proves no dangling reference; it does not prove the
  navigation still makes sense at the controller.

---

## 6. Hardware test handoff

The package is `app/IV0000-BBRD00001_00-BBRANDOMIZERMAIN.pkg`. **Every hardware
test in the plan's §6 is outstanding.** H4 through H10 were written against the
probe harness — **which this milestone deleted** — so they now have to be run
through the production UI, which is what H11 exercises end to end. They are
ordered below so each is safe before the next.

**Before anything: back the console's Bloodborne save up over FTP by hand.**
`/user/home/<id>/savedata/CUSA*/` cannot be copied out of the app, and
milestones 1–5 have never run on hardware.

1. **First launch (H5).** Install and run. The `WORLDS` tab opens on the startup
   log. On a console with a Bloodborne save and no `/data/bbrandomizer/Worlds/`,
   expect `FIRST RUN - CAPTURING THE LIVE SAVE INTO VANILLA`, then
   `VANILLA CREATED`, a captured file count, and a backup path. **Look for:**
   that path exists over FTP, holds `manifest.txt` and `data/`, and its file
   count matches the live save's. **Failure looks like:** a `FAILED - …` line, a
   `.partial` directory left behind, or the live save changed at all — nothing
   in this step should change it.
2. **Navigate, before activating anything (H10).** Left/Right on either rail
   switches tabs; Left/Right inside a settings pane still changes a value;
   `TRIANGLE` on `VANILLA` and on the active world both refuse by name. Create a
   world with `+ NEW WORLD`, name it, set a seed, press `OPTIONS` — Confirm
   should appear with `CREATED w-0001` on the state line. `O`, then `OPTIONS`
   again with nothing changed: `NO CHANGES - STILL rev-0001`. Rename it and
   press `OPTIONS`: `RENAMED - NO NEW REVISION`. Change a setting:
   `APPENDED rev-0002`. `HISTORY` lists newest first.
3. **Read a confirmation without acting on it.** On that world, `OPTIONS` →
   Confirm. **Look for:** `DEACTIVATING VANILLA`,
   `OUTGOING SAVE   BACKED UP AND FILED INTO IT`, `ACTIVATING <name>`,
   `INCOMING SAVE   ADOPT THE LIVE SAVE` (this world has no stored save yet),
   `HOW LONG   ABOUT A MINUTE`, and the phase-6 sentence under the state line.
   Press `O`. **Failure looks like:** a `CANNOT ACTIVATE` state line — read the
   sentence under it; it is the §4.4 row that fired, and nothing was written.
4. **Activate it (H6, first half).** `OPTIONS` on Confirm. The log should run
   `CHECKING`, `ACTIVATING …`, `BACKING UP THE LIVE SAVE`,
   `SAFETY BACKUP VERIFIED`, `FILING THE BACKUP INTO VANILLA`,
   `GENERATING WITH SEED …`, `GENERATED n MAP(S)`, the per-feature
   `RANDOMIZED …` lines, then the three closing lines. **Look for:**
   `/data/GoldHEN/AFR/<title>/dvdroot_ps4/` holds a fresh tree with
   `.bbrandomizer_manifest` naming this world, no `dvdroot_ps4.staging` or
   `.old` left, and no `activation.journal`. Launch Bloodborne: randomized, on
   the save you were playing.
5. **Play, activate a second world, come back (H6).** The returning world's
   progress must be its own and its randomization must be its own.
6. **`START FRESH` (H7).** Set `SAVE DATA` to `START FRESH` on a world and
   activate it. The container should be left holding `sce_sys` only, the game
   should start a new playthrough, the log should say
   `SAVE DATA IS BACK TO KEEP EXISTING - rev-NNNN`, and re-opening the world
   should show `KEEP EXISTING` with the extra revision in `HISTORY`.
7. **Vanilla, and back (H11).** `X` on `VANILLA` opens the same confirmation,
   with `SEED   NONE` and no settings rows. Activate it: the AFR tree should be
   gone and the game should launch unmodified on Vanilla's save. Then activate
   the randomized world again — **this is the case P26 unblocked.** Before it,
   this second activation refused with `BLOODBORNE TITLE ID … WAS NOT DETECTED`.
   It must now run.
8. **Interrupt one (H8).** Power the console off during a generation. On the
   next launch the startup log should name the journal, say what it did, leave
   no `.partial`, and lose no save.
9. **Refusals (H9).** A second account must see none of the first's worlds, and
   a world with a stored save must refuse on a console where Bloodborne has
   never run, saying to run the game once.

**What a failure looks like anywhere in 4–9:** any `FAILED - …` line. Stop, do
not activate anything else, and keep `/data/bbrandomizer/SaveBackups/` — the
`_preactivate` backup taken at the start of that activation is the recovery
path, and nothing prunes it.

---

## 7. Stop point

**The milestone completed.** All six of §7's steps are done, P26 with them,
every check in the plan's §6 that can run without a console passes, and a clean
rebuild produces the `.pkg`. No stop condition fired.

**This is the last milestone of the worlds feature.** What comes next is not
another milestone — it is the hardware tests in §6 above, which are the first
time any of this runs on a PS4.

### Noticed, and deliberately not fixed

* **`Game/GameInfo.{h,cpp}` has no caller.** P26 removed the last one. Deleting
  it would need a file §5 does not list; §3.13.
* **`UI/Controls.h`'s opening comment still names `MenuScreen`, `OptionsScreen`
  and `StatusScreen`** as the screens it was factored out of. Two of the three
  never existed and the third is gone. It is a historical note, not a reference.
* **The editor's Settings footer still reads `OPTIONS SAVE`.** `OPTIONS` now
  saves the world *and* opens the activation confirmation. The first half is
  still exactly true, and the string is pinned by `settings_ui_verify.py`;
  changing it is a wording decision, not an implementation one.
* **`app/UI_BLUEPRINT.md` still describes the Enable/Disable wizards and the
  main menu.** §3.2 puts it out of scope — the documentation stage owns it.
* **`make clean` no longer removes the objects of deleted sources**, because
  `OBJS` is derived from a live `find`. Harmless — they are not linked — but a
  developer expecting `clean` to empty `src/x64` will be surprised. The
  `Makefile` is explicitly unchanged by this plan (§5).

---
---

# Implementation Report — Worlds — milestone 5

**Status: AWAITING HARDWARE TEST**

**Plan:** `docs/features/worlds/plan.md` — milestone 5, "the world editor"

**Spec:** `docs/features/worlds/spec.md`

**Implemented:** 2026-09-24

> Milestone 4's report is preserved unchanged below the rule at the end of this
> one, milestone 3's beneath that, and so on. One report per milestone, newest
> first, in the folder's one report file.
>
> **H4, H5, H6–H9 and H10 have not been run.** The developer directed that
> milestones 5 and 6 be built back to back before testing. This milestone is
> built on milestones 1–4 exactly as written and re-verifies none of them.
>
> **`settings_ui_verify.py` is green again, 71 of 71.** Milestone 2's
> carried-over failure — check 1, `startFreshSave` with no `SettingsModel`
> entry — is closed by step 1's entry, not by an exemption. The field is still
> in the check and now passes it.
>
> **The B37/B8 contradiction milestone 3 raised is settled** by the developer's
> **P26** (plan §9, 2026-09-24), and is milestone 6's to implement. Nothing
> built here activates anything, and nothing here was changed for it.

---

## 1. What was built

The Enable wizard is now the **world editor**. `UI/EnableWizardScreen.{h,cpp}`
is renamed `UI/WorldEditorScreen.{h,cpp}` and extended into the screen that
creates and edits one world: a `NAME` row above `SEED`, a seventh settings
category `SAVE` holding the one `SAVE DATA` setting, and a `HISTORY` row that
lists every recipe the world has ever had, newest first. `OPTIONS` writes the
world through `Randomizer/WorldStore` — creating it on the first press — and
opens the existing Confirm screen. A rename rewrites `world.cfg` and appends no
revision; a settings or seed change appends one; selecting an older revision in
`HISTORY` makes it current by appending it again, so the list grows and never
rewinds.

`TRIANGLE` on the `WORLDS` rail deletes a world, after a confirmation that names
it and states its save is kept — and refuses outright on `VANILLA` and on the
active world. That is §4.5's fourth `WorldsScreen` mode, `ConfirmDelete`, which
milestone 4 deliberately left to this step.

Nothing about randomizer output changed. The editor still commits through
`EnemyRandomizerJob` on the old wizard's path; replacing that with
`WorldActivation` is milestone 6 step 2.

| # | Planned change | Status | Where |
| - | -------------- | ------ | ----- |
| 1 | `SettingCategory::Save`, `SettingKind::SaveChoice`, the `SAVE DATA` entry and help, the editor-only category list | done | `SettingsModel.h:40-66,113-137`, `SettingsModel.cpp:112-133,213-218,236-247` |
| 2 | Rename `EnableWizardScreen.{h,cpp}`, update the three verifier parses | done — plus three more in `worlds_verify.py`, see §2.1 | `UI/WorldEditorScreen.{h,cpp}`, `pool_verify.py:533`, `settings_ui_verify.py`, `worlds_verify.py` |
| 3 | The §4.5 editor rail, adjusted until the verifier reports no overlap | done — the rail's pitch is now the editor's own, see §2.2 | `WorldEditorScreen.cpp:99-131,171-176`, `.h:95-127` |
| 4 | `EditName`: a 16-character editor over `A–Z`, `0–9`, space | done | `WorldEditorScreen.cpp:430-478,1285-1322` |
| 5 | Create and edit; a revision only when the recipe differs | done | `WorldEditorScreen.cpp:266-300,671-736` |
| 6 | `History`: revisions newest first; selecting one appends | done | `WorldEditorScreen.cpp:547-668,1399-1428` |
| 7 | Delete on `TRIANGLE`, with a confirmation; refused on the active world | done — in `UI/WorldsScreen.{h,cpp}`, as milestone 4 flagged | `WorldsScreen.cpp:506-566,664-706` |
| 8 | Extend `ui_scroll_verify.py` with the revision list | done | `ui_scroll_verify.py` — `Editor history` |

---

## 2. Deviations from the plan

**Four, all recorded in the plan's §10.**

### 2.1 The rename touched `worlds_verify.py` as well as the three files the hazard table names

§3.3's hazard row names `pool_verify.py:533`, `settings_ui_verify.py:403` and
`:677` as the three parses that break when `EnableWizardScreen.{h,cpp}` is
renamed. There are six, not three. `worlds_verify.py` also names the file, in
three places milestone 3 added: the AFR-path allowlist, the output-parity
comparison that pins the activation job's options mapping and `||` chain against
the wizard's, and the four progress-log constants' location check. All three
were updated in the same pass and all three still pass.

Two smaller corrections inside the hazard row itself: the line numbers had moved
(`settings_ui_verify.py:520` and `:803`, not `:403` and `:677`), and three
source files carried the old name in prose comments — `EasyModes.h`,
`EnemyRandomizer.cpp` and `SetupDefaultsScreen.h`. Those were updated too.
`grep -r EnableWizardScreen app/src app/tools` is now empty.

### 2.2 The editor's rail no longer shares Setup Defaults' pitch or category band

§4.4's "one geometry, shared by the wizard's Settings step and
`SetupDefaultsScreen`" has held since the settings-UI feature, and
`settings_ui_verify.py` enforces it by comparing 28 constants across the two
screens.

The §4.5 rail breaks part of it. Setup Defaults' rail is seven rows —
`BLOODBORNE TITLE ID`, a rule, six categories. The editor's is **ten** — `NAME`,
`SEED`, a rule, seven categories, a rule, `HISTORY`. Ten rows on the shared
330/76 category band put the last row's ink at y 958, two pixels inside the
column rule that ends at 960, with its focus bar running past it to 968. There
is no arrangement that keeps `kRailFirstY`, `kRailRuleY` and `kRailPitch` shared
and fits ten rows.

The editor's rail is therefore its own uniform 70px grid from the still-shared
`kRailRow0Y = 230`: rows at 230…860, rules at 358 and 848, each sitting 14px
below the ink above it and 19px above the ink below it, and the column's ink
ending at 900 instead of 958. Three constants — `kRailRuleY`, `kRailFirstY`,
`kRailPitch` — left `SHARED_GEOMETRY` and are pinned by the verifier's own
mirror instead, exactly as `kHeaderY` already was; `kRailRow0Y` and everything
that decides where the three **columns** are is still shared and still compared.
The verifier now builds and checks two rail stacks rather than one, and also
pins the DEFAULTS rail's three constants, which would otherwise have become
unpinned by the same move.

The visible consequence: the editor's category rows no longer line up
horizontally with the settings pane rows beside them. Setup Defaults' still do.

### 2.3 `FINISH` and its readiness summary are gone; `Application.cpp` was edited

§4.5's rail has no `FINISH` row — `OPTIONS` activates. `FINISH` was also where
the help pane drew the readiness summary (seed, target, "N OF M SETTINGS
ENABLED"), so that summary had nowhere to live. It is now the first three rows
of the Confirm list — `NAME`, `SEED`, `TARGET` — which is the one screen that
states everything about the run at once, and Confirm is what `OPTIONS` opens.
`SettingsModel`'s `FinishHelp()` was removed and replaced by `NameHelp()` and
`HistoryHelp()`, the two rows that took its place.

`Application.cpp` was edited to pass the requested world id into the editor's
constructor. §5 assigns that file to milestones 4 and 6; milestone 4 built the
`worldId` plumbing as far as `MakeScreen` and left the constructor call to take
it, which is this milestone's step 5. Two lines, no new behaviour in that file.
The menu's `ENABLE RANDOMIZER` row now opens the editor on a new world, because
that is what the Enable wizard became (P8); both the row and
`ScreenId::EnableWizard` go in milestone 6 step 5.

### 2.4 The delete confirmation reports its own outcome rather than returning straight to the rail

§4.5 says `TRIANGLE` deletes a world "after a confirmation naming it and stating
its save is kept" and says nothing about afterwards. The first attempt returned
to the rail immediately and stated the outcome in the header band — but the
worlds screen's header band has no second line: `kStateY` is 140 and the header
rule is at 196, and a second row at scale 3 would have its ink running from 197
straight through it.

`X` therefore performs the delete and **stays** on the confirmation screen to
say what it did and that the save was kept; `O` is what returns to the rail, and
the rail is re-read from disk on the way. A world vanishing from the list with
nothing said about it reads as a crash, which is the failure this avoids.

---

## 3. Decisions the plan left open

### 3.1 `SaveChoice` is a bool-backed kind beside `Toggle`, not a third state on `Toggle`

§7 step 1 asks for `SettingKind::SaveChoice` and gives its two value strings,
and stops there. The entry carries the same `bool RandomizerDefaults::*` a
toggle does — `startFreshSave` — and `AdjustSetting` flips it the same way, so
Left/Right behaves identically and `X` still does nothing on it. What differs is
only how it is named (`KEEP EXISTING` / `START FRESH` rather than `YES` / `NO`)
and that `ToggleCount()` and `EnabledToggleCount()` deliberately exclude it:
"9 OF 15 SETTINGS ENABLED" is a statement about what the run randomizes, and a
save policy is not one of those. The alternative — making it a `Toggle` and
special-casing the value text on the id — would have put a settings identity
back inside a screen, which is the thing `SettingsModel.h` exists to prevent.

`SAVE DATA   YES` was rejected outright: it does not say which of the two things
it means, and the reading it invites — "yes, keep my save" — is the opposite of
what `true` stores.

### 3.2 The editor iterates its own `kCategories` list, as Setup Defaults does

The hazard table says "`SettingCategory::Save` is editor-only; Defaults iterates
its own category list" and leaves open what the editor iterates. It could have
ranged over the enum, since it shows all of them. It carries an explicit list of
seven instead, mirroring `SetupDefaultsScreen`'s list of six: the **difference
between the two lists is the rule**, and a verifier case can only compare two
lists if both exist. `settings_ui_verify.py` now asserts the DEFAULTS list is
the six settings categories and the editor's is all seven with `Save` last.

### 3.3 `SAVE` is last on the rail, below `WORLD`

Nothing says where in the rail order it goes. It is last because it is the only
category on the screen that is not about what the randomizer writes — it is
about what happens to save data — and grouping it with the six that are would
misfile it. It is also directly above `HISTORY`, the other row that is about the
world rather than the recipe.

### 3.4 A new world is called `WORLD` before the player names it

B21 permits duplicate names and the plan does not say what a new world starts
out called. An empty name lists as the world's own id (`W-0001`) on the rail,
which reads as a bug rather than as an invitation to rename. `WORLD` is an
ordinary name, so two unnamed worlds are two worlds called `WORLD` and not an
error state anyone has to handle.

### 3.5 A new world's `SAVE DATA` is forced to `KEEP EXISTING`, not inherited from Defaults

B6 says a new world is pre-filled from Defaults, and B12 says `SAVE DATA` always
defaults to `KEEP EXISTING`. `defaults.cfg` carries a `start_fresh_save` key —
milestone 2 added it, because the serializer is shared with a revision file and
writes every setting — so those two could in principle disagree. The
constructor forces `run_.startFreshSave = false` after copying Defaults. B12
wins, because a world that came up on `START FRESH` because a file said so is
precisely the deliberate choice B12 requires the player to make every time.

The `SAVE` category is not on the Defaults tab, so nothing in the app can set
that key to `1` — but the key is on disk and editable over FTP, and "the player
edited the file" must not be able to arm a destructive default.

### 3.6 Selecting a revision in `HISTORY` appends immediately, rather than on the next `OPTIONS`

Every other edit on this screen is local until `OPTIONS`. Step 6 says "selecting
one makes it current by appending a new revision", which reads as immediate, and
that is what was built: `X` on a revision appends it, reloads the list, loads it
into the editor and reports `rev-0002 IS NOW rev-0005`.

The reason for following the literal reading rather than the screen's own
convention is that an append is the one edit here that cannot lose anything if
the player backs out of it — the revision they came from is still in the list,
one row down (D2). A deferred append would also have needed a rule for what
happens when the player then edits a setting before pressing `OPTIONS`, which is
a second thing to specify for no gain.

### 3.7 The revision line is `REVISION n   <seed>   n CHANGED`, and the oldest says `CREATED`

Step 6 asks for "seed and changed-setting count" and does not say changed from
what. Each row counts the settings that differ from the revision **below** it in
the list — the one it was made from — so the number answers "what did this
revision do". The oldest revision was made from nothing, so it says `CREATED`
rather than reporting a count against a baseline that does not exist.

The count compares every setting through the model, and the three pool kinds
flag by flag rather than by their `N OF M` text: two selections of the same size
are still two different pools.

### 3.8 `O` from the editor's rail returns to `WORLDS`, and so does `O` from Progress

The editor used to return to `ScreenId::Menu` from both. It is reached from the
worlds rail now, so both return there. The menu is still reachable through the
`DEFAULTS` tab, as milestone 4 left it, and goes in milestone 6.

### 3.9 What the last `OPTIONS` press did is stated on the `NAME` pane and on Confirm

B14's difference — a rename records no history, a settings change does — is
invisible unless the screen says so. `SaveWorld` sets one sentence
(`CREATED w-0003`, `APPENDED rev-0004`, `RENAMED - NO NEW REVISION`,
`NO CHANGES - STILL rev-0003`, `RENAMED AND APPENDED rev-0004`) and it is drawn
under the world name in the pane and as Confirm's sub-heading. A store failure
replaces it there in `Palette::Bad`.

### 3.10 The history list is a full-screen mode, not a fourth column

`History` is a `Step` like `Confirm` and `Progress`, and draws full-screen with
its own `kHistoryLayout` band. The three-column frame has no spare column and
the details pane has no cursor; a revision list needs one, because selecting a
row is the whole point of it.

### 3.11 The name editor marks the cursor with a bar, not only with colour

A space has an advance and no ink, so the highlighted-character trick the seed
and title-ID editors use marks nothing at all on an empty slot — and a
16-character name is mostly empty slots. A 4px bar in `Palette::Selected` under
the cursor's advance is what makes the position visible. No new SDL entry point:
it is `FillRect`, which every rail row already uses.

### 3.12 `kCategoryRows` is written twice and asserted equal

The rail's geometry is arithmetic on the category count, and the count lives in
the class's private `kCategories`, which a file-scope constant in the anonymous
namespace cannot see. `kCategoryRows = 7` is declared beside the geometry and a
`static_assert` in the constructor makes the two agree. The same pattern covers
`kSeedDigitsForText` against the class's `kSeedDigits`.

---

## 4. Verification run

### 4.1 Build

| Check | Command | Result |
| ----- | ------- | ------ |
| Clean rebuild | `cd app && make clean && make` | `IV0000-BBRD00001_00-BBRANDOMIZERMAIN.pkg`, 7 143 424 bytes, **no warnings, no errors** |

Clean rather than incremental, per §6 and `docs/build.md`: `SettingCategory`,
`SettingKind` and `SettingId` all changed shape, and `WorldEditorScreen`'s
member layout changed wholesale.

### 4.2 Automated

| Check | Command | Result |
| ----- | ------- | ------ |
| Settings UI | `python app/tools/settings_ui_verify.py` | **71/71 passing** (was 60/61) |
| Scroll geometry | `python app/tools/ui_scroll_verify.py` | all geometry and scroll properties PASSED |
| Worlds mirror | `python app/tools/worlds_verify.py` | 84/84 passing |
| Pool / progress strings | `python app/tools/pool_verify.py selftest ../data/vanilla/dvdroot_ps4` | 89/89 passing |
| Boss | `python app/tools/boss_verify.py selftest …` | selftest 5/5 |
| Treasure | `python app/tools/treasure_verify.py selftest …` | selftest 8/8 |
| Drops | `python app/tools/drops_verify.py selftest …` | selftest 6/6 |
| Starting weapons | `python app/tools/starting_weapons_verify.py selftest …` | selftest 12/12 |
| Caged dogs | `python app/tools/caged_dogs_verify.py selftest …` | 23/23 passing |
| Easy modes | `python app/tools/easy_modes_verify.py selftest …` | 25/25 passing |
| Hunter tools | `python app/tools/hunter_tools_verify.py selftest …` | selftest 13/13 |
| Mergo darkness | `python app/tools/mergo_darkness_verify.py selftest …` | 9/9 passing |
| Item data | `python app/tools/itemdata_verify.py roundtrip …` | round trip byte-identical, 65 entries |
| Font atlas | `python app/tools/font_atlas_verify.py` | PASS |

**Nothing was run on hardware.** No PS4 is reachable from here (`CLAUDE.md` §3).

### 4.3 What the verifiers gained

`settings_ui_verify.py` went from 61 cases to 71. The new ones: one `SaveChoice`
entry and it is `SAVE DATA`; the flag rule now covering two bool-backed kinds;
the DEFAULTS tab iterating the six settings categories and not `SAVE`; the
editor iterating all seven with `SAVE` last; `SAVE` holding exactly one setting;
two rail stacks in place of one, each checked for order and for ending inside
its column rule; the editor rail's focus bar at its own pitch; the DEFAULTS
rail's three now-unshared constants; the name editor's widest typeable name; the
`HISTORY` row at four-digit revision numbers; the eight delete-confirmation
strings against the screen; and the B17/B20 wording — the confirmation says the
save is `KEPT`, and both refusals exist.

`ui_scroll_verify.py` gained `Editor history` and renamed `Wizard Settings` and
`Wizard Confirm` to `Editor Settings` and `Editor Confirm`; Confirm's row count
went from 19 to 22 with the three head rows. The history band's `bottomLimit`
was moved from 880 to 820 because the first attempt put `MORE BELOW`'s ink at
974 against a footer at 959 — the verifier caught it, which is what it is for.

---

## 5. What this does not prove

A clean cross-compile proves the editor links. The mirrors prove that every
setting has exactly one category, that `SAVE` is on one screen and not the
other, that every new string fits the column it is drawn into, and that no row,
rule, bar, hint or footer on either rail overlaps another. **None of it proves
the screen behaves.**

Specifically unproven until hardware:

* that `OPTIONS` actually writes `world.cfg` and `rev-NNNN.cfg` under
  `/data/bbrandomizer/Worlds/acct-…/`, and that the files come back;
* that a rename appends no revision and a settings change appends one — the
  rule is `WorldStore`'s and was hardware-untested in milestone 2 as well;
* that `HISTORY` shows what was written, in the right order, and that selecting
  an older revision grows the list;
* that the name editor produces a name the rail and the details pane show
  identically;
* that `TRIANGLE` deletes the right world, keeps its save, and refuses on
  `VANILLA` and on the active world;
* that any of this draws where the arithmetic says it does on a real TV.

Milestones 1–4 remain hardware-untested underneath this one. A failure in H10
could as easily be milestone 2's `WorldStore` as this milestone's screen.

---

## 6. Hardware test handoff

**H10 — tabs, listing, editor, delete.** The whole of it is now testable; the
navigation-and-listing half was already testable after milestone 4.

**Install.** `IV0000-BBRD00001_00-BBRANDOMIZERMAIN.pkg` as usual. The app opens
on `WORLDS`. Have `/data/bbrandomizer/VanillaSource/dvdroot_ps4` in place.

**1 — create a world.**
1. `X` on `+ NEW WORLD`. The editor opens titled `WORLD EDITOR`, rail reading
   `NAME`, `SEED`, rule, `ENEMIES` … `WORLD`, `SAVE`, rule, `HISTORY`.
2. `X` on `NAME`. Sixteen slots, a bar under the cursor. Type a name with
   Left/Right and Up/Down, `X` to accept.
3. Move to `SAVE`, `X` into the pane. One row, `SAVE DATA   KEEP EXISTING`.
   Left/Right flips it to `START FRESH` and back. **`X` must do nothing here.**
   Leave it on `KEEP EXISTING`.
4. Turn on a setting or two elsewhere, then `OPTIONS`.
5. Confirm opens. Its sub-heading should read `CREATED W-0001`, and its list
   should start `NAME`, `SEED`, `TARGET`.
6. `O` back, then `O` again to the rail. The new world is listed.

**2 — a rename appends nothing.** `X` on the world, `X` on `NAME`, change it,
`X`, `OPTIONS`. Confirm's sub-heading must read `RENAMED - NO NEW REVISION`.
`O`, `X` on `HISTORY`: **one** row, `REVISION 1   <seed>   CREATED`.

**3 — a settings change appends one.** Back in the editor, flip a setting,
`OPTIONS`. Sub-heading `APPENDED REV-0002`. `HISTORY` now has two rows, newest
first, the top one reading `REVISION 2   <seed>   1 CHANGED`.

**4 — history grows, it does not rewind.** In `HISTORY`, select `REVISION 1`
with `X`. Back on the editor, the `NAME` pane says `REV-0001 IS NOW REV-0003`,
and the settings show revision 1's values. Re-open `HISTORY`: **three** rows,
not one.

**5 — delete keeps the save.** Back on the `WORLDS` rail, `TRIANGLE` on the
world. The confirmation names it and says its save data is kept as a safety
backup. `O` cancels and nothing changes. `TRIANGLE` again, `X`: it reports
`DELETED <name>`, `O` returns and the world is off the rail. Check
`/data/bbrandomizer/Worlds/acct-…/` over FTP — the world folder is gone.

**6 — the two refusals.** `TRIANGLE` on `VANILLA` must say it cannot be deleted.
If a row is marked `ACTIVE`, `TRIANGLE` on it must say so and refuse. On a
console with no activation yet no row is `ACTIVE`, and that half cannot be
exercised — say so rather than passing it.

**7 — Left/Right still means two things.** On the `WORLDS` and `DEFAULTS` rails
Left/Right switches tabs. In either settings pane, and in the editor's, it still
changes a value. In the editor's **rail** it must do nothing at all — the editor
is not a tab.

**What a failure looks like.**

* **A rail row sits under another, or `HISTORY` is off the bottom.** The 70px
  grid is wrong; photograph it.
* **`X` on `SAVE DATA` changes it.** `IsDrillIn` is wrong for the new kind.
* **`SAVE` appears on the `DEFAULTS` tab.** The two category lists have
  converged; §3.2 is what stops that.
* **A rename appends a revision, or a settings change does not.** That is
  `WorldStore::AppendRevision`, milestone 2, not this screen.
* **`HISTORY` shrinks after selecting an old revision.** The one thing D2
  forbids. Stop and report.
* **A deleted world's save is not in `SaveBackups/`.** `WorldStore::Delete`,
  milestone 2.
* **The editor opens an existing world on the Defaults settings.** Step 5's
  `LoadWorld` is not running — check the log for `opened w-000N on rev-000M`.

---

## 7. Stop point

**Milestone 5's completion gate**, not a stop condition. Its eight steps are
done, `make clean && make` produces the `.pkg` with no warnings, and every
verifier in §6 that applies passes — `settings_ui_verify.py` back to green at
71/71, `ui_scroll_verify.py`, `worlds_verify.py`, `pool_verify.py` and the whole
output-parity suite.

Next is **milestone 6 — activation from the UI, and retirement**, which the
developer has directed be built before H10 is run. It implements **P26**: the
§4.4 `AFR title` refusal and `RefusalReason::AfrTitleNotDetected` come out,
which is what resolves the B37/B8 one-way door milestone 3 raised. It also
replaces this editor's Confirm body with the B10 statement and its Progress with
`WorldActivation`, and deletes `MenuScreen`, `PlaceholderScreen`,
`SaveDataProbe` and `SaveProbeScreen`.

---
---

# Implementation Report — Worlds — milestone 4

**Status: AWAITING HARDWARE TEST**

**Plan:** `docs/features/worlds/plan.md` — milestone 4, "the worlds screen"

**Spec:** `docs/features/worlds/spec.md`

**Implemented:** 2026-09-23

> Milestone 3's report is preserved unchanged below the rule at the end of this
> one, milestone 2's beneath that, milestone 1's beneath that, and milestone
> 0's beneath that. One report per milestone, newest first, in the folder's one
> report file.
>
> **H4, H5 and H6–H9 have not been run.** The developer directed that
> milestones 2, 3 and 4 be built before their hardware gates (`log.md`,
> 2026-09-22). This milestone is built on milestones 1–3 exactly as written and
> re-verifies none of them.
>
> **`settings_ui_verify.py` is 60 of 61, and the one failure is the carried-over
> one.** It is check 1, `startFreshSave` has no `SettingsModel` entry, and
> milestone 5 step 1 is what closes it. It was neither exempted nor closed here.
> See §4.2.
>
> **The B37/B8 contradiction milestone 3 raised is still open** and was not
> touched. It bears on milestone 6, not on this one: nothing built here
> activates anything.

---

## 1. What was built

The app now opens on the **WORLDS** tab. `UI/WorldsScreen.{h,cpp}` is the
tabbed three-column screen of plan §4.5: a scrolling rail of `+ NEW WORLD`,
`VANILLA` and the player's worlds most recently played first, a details pane of
readouts and wrapped notes for whichever row is highlighted, and the help column
explaining what that row does. Which world is active is derived from the disk
every time the screen is built, through `AfrManager::DeriveActive`, and shown
two ways — a swatch on the one rail row that is active, and a readout in the
header band that names `UNMANAGED` or `FIRST RUN` when no row can be. Before it
lists anything it runs milestone 3's `WorldReconcileJob` and milestone 2's
`FirstRunCaptureJob` on screen, drawn with the wizard's progress log.
`Controls` gained the tab strip both tabbed screens draw from, and
`SetupDefaultsScreen` became the **DEFAULTS** tab. Left/Right switch tabs on
either rail and still change a value inside a settings pane.

Nothing activates anything. `X` on `VANILLA` logs and returns — B8 is milestone
6 step 3 — and `X` on a world opens today's Enable wizard, which is what
milestone 5 renames and extends into the editor. The randomizer engine, the
pool tables, `SettingsModel`, `EnableWizardScreen`, `WorldStore`,
`WorldActivation`, `Platform/SaveData` and the Makefile are untouched.

| # | Planned change | Status | Where |
| - | -------------- | ------ | ----- |
| 1 | `Controls`: `DrawTabs` and the tab-strip constants | done | `Controls.h:72`–`:97` — `kTabCount`/`kTabWorlds`/`kTabDefaults`, the eleven geometry constants, `TabLabel`, `DrawTabs`; `Controls.cpp:15` `TabLabel`, `:20` `DrawTabs` (four `FillRect`s for the box, per §4.5's "no new glyph"). `settings_ui_verify.py` measures both labels, their boxes and the gap between them |
| 2 | `UI/WorldsScreen.{h,cpp}` `Browse` mode: rail, details pane per B3, help pane | done | `WorldsScreen.cpp:665` `DrawRail` over `kRailLayout` (`:70`), `:696` `DrawDetails` over `kDetailLayout` (`:77`), `:734` `DrawHelp`; `:324`–`:355` the rail as indices; `:480` `Details` builds B3's readouts — name as the pane heading, seed, settings summary, revision, save size, when it was last played, and whether it is active |
| 3 | The active marker, the `UNMANAGED` case and the cannot-activate reasons of §4.5 | done | `WorldsScreen.cpp:346` `RowIsActive` — Vanilla when the state is Vanilla, the named world when it is World, **no row at all** for FirstRun and Unmanaged (B4, B32); `:684` the `Palette::Good` swatch; `:636` the header readout that states the state by name; `:256` `CannotActivate` and `:540` the note it puts in the details pane (B25, B33) |
| 4 | `Reconciling` and `FirstRunCapture` modes, driven by the milestone 2 and 3 jobs, drawn with the progress-log layout | done | `WorldsScreen.cpp:369` `UpdateStartup` — `WorldReconcileJob` then, only when the account folder is absent, `FirstRunCaptureJob`; `:580` `DrawStartup` uses the wizard's `kProgressLayout` numbers verbatim (`:97`). §3.4 covers when it waits for `X` and when it goes straight through |
| 5 | `SetupDefaultsScreen`: tab strip, `DEFAULTS` heading, Defaults-only category list | done | `SetupDefaultsScreen.cpp:287` draws the strip and the heading where the `SETUP DEFAULTS` title used to be; `SetupDefaultsScreen.h:65` `kCategories[]` — a list, not a range over the enum, so milestone 5's `SettingCategory::Save` cannot appear here (§3.3 hazard row 6) |
| 6 | Tab switching on Left/Right while focus is on the rail, in both screens' `UpdateRail` | done | `SetupDefaultsScreen.cpp:156` and `WorldsScreen.cpp:442`. `SetupDefaultsScreen.cpp:193` — the pane's Left/Right still calls `AdjustSetting` and is untouched. Nothing was re-bound and `L1`/`R1` are as they were |
| 7 | `Screen.h` and `Application.cpp`: new `ScreenId`s, `RequestedWorldId`, startup reconciliation | done | `Screen.h:30`–`:32` `Worlds`, `Defaults`, `WorldEditor`; `:56` `RequestedWorldId()`; `Application.cpp:33` `MakeScreen` takes the world id and the session, `:74` owns `WorldsSession`, `:81` opens on `WorldsScreen`, `:107` reads both answers before the screen is destroyed. `MenuScreen` stays reachable — see §3.5 |
| 8 | Extend `settings_ui_verify.py` and `ui_scroll_verify.py` | done | `settings_ui_verify.py` +21 cases (39→61 total): the tab strip, the worlds screen's three-column geometry against the other two, its header stack, its rail budget beside the swatch, its details column and worst-case pane, its help column, both new footers, and the DEFAULTS tab's own category list. `ui_scroll_verify.py` +3 screens: `Worlds rail`, `Worlds details`, `Worlds startup` |

### The §7 invariants

* **The one-`Screen` pattern.** `WorldsScreen` is one `Screen` with three
  internal modes (`WorldsScreen.h:79`). No sub-step is a `ScreenId`.
* **No SDL2 or orbis call from `UI/`.** `grep -cE "sce[A-Z]|SDL_|orbis"` over
  `WorldsScreen.{h,cpp}` is **0 and 0**. The screen reaches the platform only
  through `savedata::ResolveUser`, `DiscoverSaveTitle` and `ReadContainer`,
  which are `Platform/SaveData`'s own API.
* **No raw AFR path in `UI/`.** `WorldsScreen` contains no `GoldHEN` string; it
  passes `defaults_.bloodborneTitleId` to `AfrManager`, which owns the path.
  (Two pre-existing occurrences elsewhere are noted in §3.9 — neither is mine
  and neither was touched.)
* **Left/Right inside a settings pane still changes a value.**
  `SetupDefaultsScreen::UpdatePane` is unchanged; only `UpdateRail`, which
  consumed neither input, gained the tab switch.

---

## 2. Deviations from the plan

**Two, both small, and one of them is a rename the plan's own §5 forced.**

### 2.1 `ScreenId::SetupDefaults` was renamed to `ScreenId::Defaults` rather than both existing

§5 says `Screen.h` gains "`ScreenId::Worlds`, `Defaults`, `WorldEditor`". It
does not say what becomes of `SetupDefaults`, which named the same screen. Two
ids for one screen is one more thing to keep in step, and the id that survives
is the one the plan names — so `SetupDefaults` is gone and its single caller,
`MenuScreen.cpp:74`, now asks for `Defaults`.

That touched `UI/MenuScreen.cpp`, which §5 assigns to **milestone 6** (deleted).
It is one token on one line, it is not a behaviour change, and the alternative
was to carry a duplicate enum value into a milestone that deletes the file
anyway. Flagged rather than buried because it is a file this milestone was not
given.

### 2.2 `WorldsScreen` does not have a `ConfirmDelete` mode

§4.5 lists four modes for this screen — `Reconciling`, `FirstRunCapture`,
`Browse`, `ConfirmDelete`. §7's milestone 4 step list asks for the first three
and says nothing about the fourth; **milestone 5 step 7** is the step that
defines what deleting does ("`TRIANGLE` plus a confirmation naming the world and
stating its save is kept; refused on the active world"), and its done-condition
is that both cases behave. Building the mode here would have been building
milestone 5's step with no behaviour behind it, and `TRIANGLE` has nothing to
delete until milestone 5 can create a world.

The consequence for milestone 5 is worth stating plainly: **step 7 will have to
edit `UI/WorldsScreen.{h,cpp}`**, which §5's table lists only against milestone
4. That is a table-versus-§7 gap of the same kind milestone 2's report recorded
for `AfrManager`, not a new file.

---

## 3. Decisions the plan left open

Eleven. The first four are the ones most likely to be argued with.

### 3.1 The details pane is a list of rows with at most one explanatory note

B3 fixes *what* the pane shows; §4.5 adds that it "states when a world cannot be
activated and why". Those are two different shapes — aligned label/value
readouts, and a wrapped sentence — and the pane has **no cursor**, so whatever
it builds has to fit the band rather than scroll.

`DetailRow` carries both (`WorldsScreen.h:88`): a normal row draws its label
left and its value right-aligned to `kPaneValueRight`, exactly like a settings
row; a `note` row is wrapped across the whole pane and drawn in `Palette::Dim`.
The budget is then bounded by rule rather than by hope — after the readouts the
pane appends **the row's own explanation** (Vanilla's D16 sentence, `+ NEW
WORLD`'s) and then **exactly one** of:

1. why *this* world cannot be activated (B25/B33) — the actionable one, so it
   wins; or
2. when no row is `ACTIVE` at all, `AfrActiveWorld::reason` — which is how the
   `UNMANAGED` and `FIRST RUN` states get a sentence rather than only a word.

Worst case is 10 of the 11 rows the band draws, measured by
`settings_ui_verify.py` from the atlas against the real strings, not asserted.

*Rejected:* letting the pane scroll (a pane the player cannot scroll is the
silent-truncation failure `Controls.h` says is being removed); putting the
reason in the help column (§4.5 says the details pane states it); showing both
notes (unbounded).

### 3.2 The state that has no row — `UNMANAGED` and `FIRST RUN` — is stated in the header band

B32 wants `UNMANAGED` shown, and B4 forbids marking any row in that state. So
there is a state with nothing to attach to. It is drawn right-aligned at
`kStateRight` on the heading line — `ACTIVE  <world name>`, `ACTIVE  VANILLA`,
`ACTIVE  UNMANAGED`, `ACTIVE  FIRST RUN` — which is the position and the
treatment the Enable wizard already gives its `TARGET` readout, at the same
scale and in the same dim colour. `settings_ui_verify.py` checks it clears the
heading horizontally at the widest name a player can type.

### 3.3 The container is read once per launch, not once per visit, and `WorldsSession` is where that is kept

B25 needs the container's block count and B33 needs to know whether a container
exists. The only way to get either is `savedata::ReadContainer`, which sizes
every file by reading it through to a short read because `st_size` lies on this
kernel (findings §9). At the measured ~15 MB/s that is close to **two seconds**
on a full Bloodborne save.

`Application` rebuilds a screen on every switch, so doing that in
`WorldsScreen`'s constructor would freeze the screen for two seconds *every time
the player presses Left twice*. `WorldsSession` (`WorldsScreen.h:57`) is a small
struct `Application` owns (`Application.cpp:74`) and passes by reference: the
startup jobs and the container probe run once per launch, while the progress log
is already up and a pause is expected, and every later visit reads only
`/data` (`world.cfg`, revision files, save manifests — all small).

*Rejected:* a file-scope `static` in `WorldsScreen.cpp` (Application.h says
app-wide state lives in `Application`); adding a cheap "stat only" variant to
`Platform/SaveData` (§5 assigns that file to milestone 1, and §3.1's bracketing
rule is the reason it has only the expensive read); dropping the two sentences
(they are B25 and B33).

### 3.4 The startup log waits for `X` only when something happened

Step 4's done-condition is "both complete on screen", which leaves open what
happens next. Making the player dismiss a log every launch on a console where
reconciliation is a no-op is a button press for nothing; auto-dismissing a
first-run capture would hide the one launch on which their existing playthrough
is filed into a world.

So: the log goes straight to `Browse` when the reconcile job reports a **quiet**
run — no journal, nothing swept, and `ok` — and waits for `X` otherwise, with
Up/Down scrolling it. A first-run capture **always** waits.

### 3.5 `O` exits from `WORLDS`; `MenuScreen` stays reachable through the `DEFAULTS` tab

Step 7 says `MenuScreen` stays reachable, and the app now opens on `WORLDS`.
`MenuScreen` has no `WORLDS` row, so routing `O` from the worlds rail to it
would have made the main screen unreachable again without a relaunch.

`WORLDS` is the top-level screen and `O` exits, exactly as `O` at `MenuScreen`
does today. `MenuScreen` is reached the way it always was — `DEFAULTS`'s `O`,
whose behaviour is unchanged — and from there `SETUP DEFAULTS` returns to the
`DEFAULTS` tab and Left/Right to `WORLDS`. Both the Enable wizard and the
`SAVE DATA PROBE (TEST)` harness therefore stay reachable, which matters: H4,
H5 and H6–H9 are all still unrun and all run through that harness.

### 3.6 `X` on a world opens the Enable wizard, carrying the world id

Step 7 asks for `ScreenId::WorldEditor` and `RequestedWorldId`, and milestone 5
is what builds the editor. `MakeScreen` maps `WorldEditor` to
`EnableWizardScreen` — the screen P8 says is renamed and extended into it — and
logs the id it was asked for (`Application.cpp:46`). `+ NEW WORLD` sends an
empty id, which is B6's "pre-filled from Defaults" and is exactly what that
screen already does; an existing world sends its `w-NNNN`, which nothing reads
yet. The plumbing is therefore exercised end to end this milestone and milestone
5 step 5 is what makes the id mean something.

**This is worth a sentence in the hardware handoff** (§6): opening an existing
world in the editor today shows the *Defaults* settings, not that world's.

### 3.7 `X` on `VANILLA` does nothing this milestone

B8 activates Vanilla; §7 milestone 6 step 3 is "`X` on `VANILLA` routes to the
same confirmation". There is no confirmation screen and no UI path to
`WorldActivationJob` until then, and wiring `X` straight into an activation with
no confirmation would be shipping B10's screen as "there isn't one". It logs and
returns; the row still lists, explains itself and shows its save.

### 3.8 The Defaults tab's footer changes with focus

The `DEFAULTS` tab's Left/Right now means two things depending on where focus
is, which is what D19 chose. A single footer line would have to be wrong in one
of the two places, so there are two (`SetupDefaultsScreen.cpp:88` and `:90`) and
`:306` picks by focus. `LEFT RIGHT CHANGE` in the pane, `LEFT RIGHT TABS` on the
rail. Both are measured by `settings_ui_verify.py`.

Switching tabs off the `DEFAULTS` rail **discards unsaved edits**, because those
edits are local until `OPTIONS` writes them — the same thing `O` has always
done. It is logged in the same words.

### 3.9 Two pre-existing `/data/GoldHEN/AFR` occurrences were left alone

§3.1 requires that string to appear in exactly one file. It appears in three:
`Game/AfrManager.cpp:44`, `Game/GameInfo.cpp:13` and
`UI/EnableWizardScreen.cpp:494`. All three are in `HEAD` and predate this
feature; none is mine; `WorldsScreen` adds none. `EnableWizardScreen.cpp:494` is
the wizard's own output path, which milestone 6 step 2 replaces when the editor
runs `WorldActivation` instead of `EnemyRandomizerJob`. Reported, not fixed
(§3.2 forbids tidying neighbouring code).

### 3.10 Sizes are printed in whole units, with no decimal point

`SizeText` (`WorldsScreen.cpp:150`) prints `27 MB`, not `27.4 MB`. `Font8x8` has
no `.` glyph and **advances the cursor for a character it cannot draw**, so on
the fallback path a decimal would read as `27 4 MB` rather than as a missing
dot. The atlas is the live path and would render it fine; one less thing that is
only correct on one of two paths.

### 3.11 The rail row's label is not clipped; the clear space is a verifier requirement

Nothing at draw time stops a name running into the `ACTIVE` swatch. The 16
character cap (P10) exists precisely so the band can be *checked*:
`settings_ui_verify.py` measures the widest name `NormalizeWorldName` can
produce — parsed from `WorldStore.cpp`'s own loop bound, not retyped — against
`kRailW - kSwatchW - SWATCH_MIN_GAP`. Same treatment, and same reasoning, as the
picker's flag column in case 9.

---

## 4. Verification run

### 4.1 Commands and results

| Check | Command | Result |
| ----- | ------- | ------ |
| Clean build | `cd app && make clean && make` | **`IV0000-BBRD00001_00-BBRANDOMIZERMAIN.pkg`, 7,143,424 bytes.** No errors, **no warnings** |
| Settings UI | `python app/tools/settings_ui_verify.py` | **60 of 61.** The one failure is §4.2's, carried over from milestone 2 |
| Scroll geometry | `python app/tools/ui_scroll_verify.py` | **PASSED** — 10 screens, including the three added here |
| Worlds mirror | `python app/tools/worlds_verify.py` | **all checks passing** (221 cases, untouched this milestone) |
| Font atlas | `python app/tools/font_atlas_verify.py` | PASS |
| Output parity | `pool_verify.py selftest` | **89/89** |
| Output parity | `boss_verify.py selftest` | 5/5 |
| Output parity | `treasure_verify.py selftest` | 8/8 |
| Output parity | `drops_verify.py selftest` | 6/6 |
| Output parity | `starting_weapons_verify.py selftest` | 12/12 |
| Output parity | `caged_dogs_verify.py selftest` | 23/23 |
| Output parity | `easy_modes_verify.py selftest` | 25/25 |
| Output parity | `hunter_tools_verify.py selftest` | 13/13 |
| Output parity | `mergo_darkness_verify.py selftest` | 9/9 |
| Output parity | `itemdata_verify.py roundtrip` | PASS — byte-identical, 28,309,992 bytes |

The parity suite is not in milestone 4's §6 list — this milestone changes no
randomizer code — but it was run because `pool_verify.py` parses the four
progress-log constants out of `EnableWizardScreen.cpp` by name and this
milestone edits the UI layer around it. All ten pass, against
`data/vanilla/dvdroot_ps4`.

### 4.2 The one failing check, in full

```
  1: all 16 bool fields appear in exactly one entry                        FAILED
   missing=['startFreshSave'] twice=[] unknown=[]
```

Unchanged from milestone 2, and unchanged *by* this milestone. `startFreshSave`
was added to `RandomizerDefaults` in milestone 2 step 1; its `SettingsModel`
entry is **milestone 5 step 1**, whose own done-condition is "done when
`settings_ui_verify.py` passes". §6 lists this verifier against milestone 4, so
it is red at this gate.

It was **not** closed by exempting the field, and milestone 5 step 1 was **not**
pulled forward. Both would have been decisions for the developer, and the second
is the next milestone. Every other case in the file — including all 21 added
here — passes.

### 4.3 What the new cases assert

`settings_ui_verify.py`, 39 cases → 61:

* the tab strip declares two tabs and names both; both labels fit their boxes;
  the boxes clear each other by exactly `kTabGap`; the active box contains its
  own label's ink
* the worlds screen's **24** three-column constants are identical to the
  settings screens', and all three draw their rules in the same colour
* the worlds screen's five header elements — tab box, heading band, header
  rule, column rule, pane heading — are in order and clear of each other, and
  the heading clears the `ACTIVE` readout across the screen
* every rail row fits beside the swatch at the widest typeable name; the focus
  bar contains its row's ink and the swatch sits inside the bar
* the details heading, every label-plus-widest-value pair, every note's wrap
  count, the worst-case pane (10 of 11 rows) and the widest note word all fit
* the three help titles and three help bodies wrap inside the column and no
  word overhangs it
* both new footer lines fit the screen and both say `LEFT RIGHT TABS`
* the `DEFAULTS` tab iterates its own list of the six categories

`ui_scroll_verify.py`, 7 screens → 10: `Worlds rail`, `Worlds details`,
`Worlds startup`.

`kTitleY` and `kTitleScale` left `SHARED_GEOMETRY` — the `DEFAULTS` tab has no
screen title any more — and stay pinned against the wizard by the `mirror`
dict, which is checked against the wizard alone.

---

## 5. What this does not prove

Everything here is a build and two geometry mirrors. Neither establishes runtime
behaviour (`CLAUDE.md` §3).

Specifically unproven:

* **That the tab strip, the rail, the details pane and the help column look
  right on a TV.** The mirrors prove ink boxes do not overlap and strings fit
  measured columns. They say nothing about whether the boxed tab reads as
  "you are here", whether the `Palette::Good` swatch is legible at viewing
  distance, or whether `Palette::Dim` note text is readable on a real panel.
* **That the startup modes complete on screen.** `WorldReconcileJob` and
  `FirstRunCaptureJob` have never run from a screen. Their own hardware tests —
  **H5** and **H8** — have not been run at all.
* **That the derived state renders correctly for each row of §4.2.** The
  derivation is milestone 2's and `worlds_verify.py` pins it as a function; that
  the *screen* draws the right thing for each of the four states has only been
  reasoned about.
* **That the container probe costs what it is assumed to cost.** The ~2 s
  estimate is `technical-findings.md`'s 15 MB/s applied to a 27 MB save. If it
  is much worse, the first launch is a longer pause than intended.
* **That `ReadContainer` is safe to call from a screen at all.** It mounts
  read-only and §3.1 forbids writing through such a mount, and milestone 1
  built it that way — but §3.1's "a failed save-data operation has changed
  something" applies to every call, and this is the first one made outside the
  harness.
* **That two seconds of blocking work inside one `Update()` does not trip a
  watchdog.** Nothing in this app has blocked that long in a frame before.

---

## 6. Hardware test handoff

**H10, navigation and listing only** (plan §6). The editor half of H10 is
milestone 5's.

Install the `.pkg` as usual (`docs/build.md`). **Before anything else, note
that H4, H5 and H6–H9 are still unrun**, so this console's worlds folder may not
exist and the first launch will run first-run capture.

### 1. What to expect on launch

The app opens on a **startup log**, not the main menu.

* If this account has a worlds folder already: the log flashes past and the
  `WORLDS` tab appears.
* If it does not: the log stays up, says `FIRST RUN - CAPTURING THE LIVE SAVE
  INTO VANILLA`, and waits for `X`. **This is H5 happening.** Read what it says
  before pressing `X` — it names the backup it took and how many files went into
  Vanilla, and that is the record of your existing playthrough being filed.

### 2. What to do

1. **Tabs.** On the `WORLDS` rail, press Left, then Right. Each should switch to
   `DEFAULTS` and back. Do it from the `DEFAULTS` rail too.
2. **Left/Right inside a pane.** On `DEFAULTS`, `X` into a category, move to a
   toggle, press Left/Right. It must **change the value**, not switch tabs. Back
   out with `O` and press Left — now it should switch tabs.
3. **The rail.** Up/Down over `+ NEW WORLD`, `VANILLA` and any worlds the
   harness left behind (`HARNESS A`, `HARNESS B`). Check the order: most
   recently played first, then never-played newest first.
4. **The details pane.** On each row, read the middle column. `VANILLA` should
   show its save size and when it was last played. A harness world should show
   its seed, its settings count and its revision.
5. **The active marker.** Exactly one row should carry a green swatch at its
   right edge — or none, with the header saying `ACTIVE  UNMANAGED` or
   `ACTIVE  FIRST RUN`.
6. **The help column.** It should change with the row: `+ NEW WORLD`, `VANILLA`
   and `WORLD` each have their own text.
7. **`X` on `VANILLA`.** Should do nothing visible. (B8 is milestone 6.)
8. **`O` on the `WORLDS` rail.** Exits the app.
9. **Reaching the harness.** `DEFAULTS` → `O` → the old menu →
   `SAVE DATA PROBE (TEST)`. **Confirm this still works** — H4 and H6–H9 run
   through it.

### 3. What to look for

* Both tabs drawn, the active one boxed, its name as the heading beneath.
* No text clipped, no two things overlapping, nothing running off an edge.
* The rail scrolls with `MORE ABOVE` / `MORE BELOW` once there are more than
  eight rows.
* Switching tabs does not move the columns or the footer — only the content.
* The footer on `DEFAULTS` says `LEFT RIGHT CHANGE` in a pane and
  `LEFT RIGHT TABS` on the rail.

### 4. What a failure looks like

* **The screen freezes on the first launch for more than a few seconds.** That
  is the container probe (§3.3). Report roughly how long — `live.log` has the
  `SAVE DATA ... BLOCKS` line and its timing context.
* **A black screen or a crash at launch.** The app now runs
  `savedata::ReadContainer` outside the harness for the first time (§5). Pull
  `live.log`; the last `worlds:` line says how far it got.
* **Left/Right changes a value while on the rail**, or **switches tabs while in
  a pane.** Either is step 6 wrong.
* **A row shows `ACTIVE` when another does too**, or none does when the header
  names a world. That is `RowIsActive` disagreeing with `DeriveActive`.
* **Opening an existing world in the editor shows the wrong settings.** Expected
  this milestone (§3.6) — it shows the Defaults settings until milestone 5 step
  5. Not a failure yet.
* **A note in the details pane is cut off at the bottom.** Would mean the §3.1
  budget is wrong; `settings_ui_verify.py` says it is not, so report it with a
  photo.

---

## 7. Stop point

**Milestone 4's completion gate**, not a stop condition. Its eight steps are
done, `make clean && make` produces the `.pkg` with no warnings,
`ui_scroll_verify.py` and `worlds_verify.py` pass, and `settings_ui_verify.py`
is 60 of 61 with the single carried-over failure of §4.2 — which milestone 5
step 1 closes by design.

Next is **milestone 5 — the world editor**, which starts with exactly that step.
Do not begin it before H10's navigation and listing half has been run. Note that
milestone 5 step 7 will have to edit `UI/WorldsScreen.{h,cpp}` (§2.2), and that
the B37/B8 contradiction milestone 3 raised still needs a developer decision
before milestone 6.

---
---

# Implementation Report — Worlds — milestone 3

**Status: AWAITING HARDWARE TEST**

**Plan:** `docs/features/worlds/plan.md` — milestone 3, "the activation transaction"

**Spec:** `docs/features/worlds/spec.md`

**Implemented:** 2026-09-22

> Milestone 2's report is preserved unchanged below the rule at the end of this
> one, milestone 1's beneath that, and milestone 0's beneath that. One report
> per milestone, newest first, in the folder's one report file.
>
> **Milestone 1's H4 and milestone 2's H5 have not been run.** The developer
> directed that milestones 2–4 be built before their hardware gates (`log.md`,
> 2026-09-22). This milestone was built on milestone 1's save-data service and
> milestone 2's world store exactly as written, and nothing in it re-verifies
> either.
>
> **Read §2.4 before running anything on hardware.** B37 and B8 contradict each
> other on this platform, and the contradiction is reachable in this
> milestone's own hardware tests.

---

## 1. What was built

A world can now be activated end to end. `Game/WorldActivation.{h,cpp}` is the
seven-phase transaction of plan §4.3: it checks every refusal in §4.4 before
writing anything, writes a journal that names the phase about to run and
`sceKernelFsync`s it before the phase runs, backs the live save up and files it
into the outgoing world, regenerates the incoming world's tree into
`dvdroot_ps4.staging`, swaps it in by rename, swaps the save per §4.3's
phase-6 table, and commits — reverting a one-shot `START FRESH` as a recorded
revision on the way out. `WorldReconcileJob` sweeps `*.partial` debris and
finishes or discards an interrupted activation, one documented action per
phase. `Game/AfrManager` gained the staging path, the swap and the remove-tree
that milestone 2 deferred. The harness gained two operations, on `L1` and `R1`,
so all of it can be driven on hardware before any world UI exists.

No UI beyond the harness screen, no startup wiring, no confirmation screen —
those are milestones 4 and 6. `Application.cpp`, `Screen.h`, `SettingsModel`,
`Controls`, `EnableWizardScreen`, the randomizer engine, the pool tables and
the Makefile are untouched.

| # | Planned change | Status | Where |
| - | -------------- | ------ | ----- |
| 1 | `Game/WorldActivation.h` — the job interface and a `Refusal` result carrying a reason and a human sentence | done | `WorldActivation.h` — `RefusalReason` (12 reasons, §2.1), `Refusal` with its one-line `sentence`, `ActivationFacts`, `ActivationPlan`, `SaveAction`, `ActivationJournal`, `ReconcileAction`, `ActivationResult`, `WorldActivationJob`, `ReconcileResult`, `WorldReconcileJob`. It compiles, and `worlds_verify.py` asserts it names no orbis type — it is included from `UI/` |
| 2 | Phase 1's checks, every §4.4 entry, returning a refusal and writing nothing | done | `WorldActivation.cpp:149` `CheckActivation` — a pure function of `ActivationFacts`, in §4.4's own order; `:564` `PlanActivation` gathers the facts, walks the incoming world's stored save and judges them; `:1133` `GatherFacts` does the reading. The harness `L1` fires all twelve reasons on constructed mismatches and then prints the live verdict for every world on the console. One qualification about "writing nothing" in §3.11 |
| 3 | The journal: write, fsync, advance, delete | done | `WorldActivation.cpp:286` `FormatActivationJournal`, `:299` `ParseActivationJournal`, `:330`–`:342` the file halves, `:456` `State::Advance` — stamp the phase, write, fsync, and only then move. Created at the start of phase 2, the first phase that writes anything; deleted at the end of phase 7. A failure leaves it in place for the next launch (§3.5) |
| 4 | Phases 2–7 in order, every row of the phase-6 table and phase 7's revert | done | `WorldActivation.cpp:614` phase 1, `:656` phase 2 (skipped per **D26**), `:711` phase 3, `:752` phase 4 (mapping and run decision pinned against the wizard's), `:861` phase 5, `:914` phase 6 (all five rows), `:1049` phase 7's revert and the journal's deletion. `AfrManager.cpp:317` `StagingDvdroot`, `:321` `WriteManifestAt`, `:326` `Swap`, `:372` `RemoveStaging`, `:378` `RollBackSwap` |
| 5 | Reconciliation per §4.3, callable from the harness | done | `WorldActivation.cpp:344` `ActionForPhase` — exactly one action per phase, and a phase this app never writes is left alone rather than guessed; `:1242` `WorldReconcileJob::Step` — sweep, read, act, and for phases 5–7 resume through the activation job itself. A hand-written `activation.journal` over FTP plus `L1` exercises each phase (§6) |
| 6 | Extend `worlds_verify.py` with the refusal table and the phase machine | done | A new `activation` section, 54 cases, plus 28 more in the source section — including the options mapping and the run decision compared field for field against `EnableWizardScreen.cpp`. 221 cases total, all passing |

### The §7 invariants

* **Nothing is written before phase 1 passes.** The journal — the first thing
  an activation puts on disk — is built and written only after
  `PlanActivation` has returned a plan with no refusal; `worlds_verify.py`
  pins the order in the source. The one qualification is `AfrManager::Check`'s
  writability probe, which creates and unlinks a scratch file in the AFR root:
  §3.11.
* **The safety backup is verified before phase 6, and its failure aborts the
  transaction.** Phase 2 runs `SafetyBackupJob`, whose `Result().ok` is not set
  until `savedata::VerifyBackup` has walked the copy on disk (milestone 2), and
  `!r.ok` goes straight to `Fail` — no later phase runs. Phase 6's restore
  verifies its source again before it opens the container, in milestone 1's
  `RestoreJob`.
* **The container is emptied before it is written.** Both paths that write a
  container go through milestone 1: `RestoreJob` empties first as its phase 2,
  and `START FRESH` is `EmptyContainer` with nothing restored after it.
  `worlds_verify.py`'s existing source case on `RestoreJob` still passes.
* **Output parity — the options mapping and run decision unchanged, only the
  output path differs.** `worlds_verify.py` extracts every
  `options.X = run.Y;` from `WorldActivation.cpp` and from
  `EnableWizardScreen.cpp` and requires the two lists to be identical — 18
  fields, in the same order — and does the same for the 13 identifiers in the
  run decision's `||` chain. The full parity suite passes unchanged (§4).

---

## 2. Deviations from the plan

All three are recorded in the plan's §10. §2.4 is not a deviation — it is a
contradiction between two approved documents, reported rather than resolved.

### 2.1 A thirteenth refusal, `EmptySelection`, that §4.4 does not list

**The plan said** §4.4 is the refusal table, ten rows.

**What was done:** `CheckActivation` has an eleventh row, `EmptySelection`,
which fires when `RANDOMIZE ENEMIES` is on with no enemies selected or
`RANDOMIZE BOSSES` is on with no bosses selected. It never fires for Vanilla.

**Why:** these are not new. `EnableWizardScreen::StartCommit` already refuses
both, up front, before it constructs a job, and §3.1 requires that "the run
decision" be unchanged. Its own comments say why:

> D12/D8: an empty pool would be caught by StepBuildPool, but only after the
> mirror phase has already copied most of the game into the AFR folder

and for the boss case,

> With no bosses selected BOTH are empty, so the refill never helps and the
> next draw calls RandIndex(rng, 0) -> a uniform_int_distribution(0, -1),
> which is undefined behaviour rather than a clean failure.

Dropping them would have made an activation able to reach undefined behaviour
in phase 4 — *after* phase 2 has taken a safety backup and phase 3 has
rewritten the outgoing world's stored save. Phase 1 is where this feature
refuses, so that is where they went. They read no disk and write nothing.

**If the reviewer disagrees**, the alternative is to move the two checks to the
start of phase 4 and let the transaction abort there; the console stays
coherent either way, because phase 4's failure leaves the journal at phase 4
and reconciliation discards the staged tree. Phase 1 was chosen because "every
refusal in §4.4. Nothing is written" is a stronger promise to keep whole.

### 2.2 Phase 4 generates for every non-Vanilla world, chain or no chain

**The plan said**, in §4.3: "**Generate** — `EnemyRandomizerJob` into
`…/dvdroot_ps4.staging`, then write `.bbrandomizer_manifest` inside it.
Skipped when the incoming world is Vanilla." And in §7 milestone 3's
invariants: "output parity — the options mapping and run decision unchanged,
only the output path differs."

**What was done:** phase 4 is skipped for Vanilla and runs for everything else.
The wizard's `||` chain is still evaluated, field for field, and reported —
when it is false the log says `NO RANDOMIZER SETTINGS ARE ON - THE TREE IS A
PLAIN MIRROR` — but it does not decide whether the job runs.

**Why:** the two sentences cannot both be read as "skip the run when the chain
is false". `.bbrandomizer_manifest` lives *inside* `dvdroot_ps4` (D11, B23),
and it is the only record of which world is active. A world generated with
every setting off would have no tree, so no manifest, so §4.2 would derive the
console as **Vanilla active** and B4's "exactly one row is marked `ACTIVE`"
would be false for a world the player had just activated. §4.3's own wording is
unconditional for non-Vanilla, and §4.4's `Source` row — "the incoming world is
not Vanilla and `VanillaSource/dvdroot_ps4` is missing" — only makes sense if
every non-Vanilla activation generates.

Parity is not affected. With every option off, `EnemyRandomizerJob` mirrors the
six vanilla folders and writes nothing else, so that world plays exactly as the
game shipped. For any recipe where the chain is true the run is the run the
wizard makes, from the same seed with the same options — which is now checked
rather than asserted (§1's last invariant).

### 2.3 `PlanActivation` is public rather than phase 1's private body

**The plan said**, §7 step 2: "Phase 1's checks, every §4.4 entry, returning a
refusal and writing nothing — done when the harness can trigger each one."

**What was done:** phase 1 is `PlanActivation(user, afrTitleId, toWorldId)`, a
free function in the same header. `WorldActivationJob`'s phase 1 calls it and
nothing else; the harness calls it directly for a read-only verdict.

**Why:** the done-condition needs the harness to reach phase 1 without reaching
phase 2, and a job with a "stop after checking" mode is a transaction with a
half-open door. Milestone 6 needs the same thing for real: B10's confirmation
screen states the refusal, the outgoing world, the incoming world and which
phase-6 row applies *before* anything happens, which is exactly this function's
return value.

### 2.4 B37 makes B8 a one-way door — this needs a spec decision

**Not a deviation. B37 is implemented exactly as §4.4 and B37 are written**,
and the consequence is reported here rather than designed around.

`GameInfo::DetectAll` decides a title is installed by finding
`/data/GoldHEN/AFR/<title>/dvdroot_ps4/event/common.emevd.dcx` and checking its
DCX magic. That file is there **because the randomizer put it there** — AFR is
a redirect overlay over the installed game, not a copy of it, which is why this
project keeps its own `VanillaSource` tree to randomize from.

So:

1. Activating Vanilla removes `dvdroot_ps4` for that title (B8, §4.3 phase 5).
2. `DetectAll` then no longer reports that title.
3. §4.4's `AFR title` row — "the `BLOODBORNE TITLE ID` setting names a title
   `GameInfo::DetectAll` did not find" — refuses **every** later activation for
   it, with `BLOODBORNE TITLE ID CUSA03173 WAS NOT DETECTED`.

H11's "activate Vanilla, return" cannot pass. Nor can the very first activation
on a console whose AFR folder has never held randomizer output, which is every
fresh install. The reference console happens to hide half of this: it has
`BossarenaCUSA03173` and `BossarenaCUSA03174` seeded as well, so `DetectAll`
will keep returning a non-empty list that simply does not contain the
configured title.

The failure D27 exists to prevent is real and worth keeping — an AFR tree
written for a title that is not installed produces a game that launches
unmodified with no error anywhere. What is wrong is only the *evidence*
`DetectAll` offers: it proves "this title's AFR folder currently holds
Bloodborne data", not "this title is installed".

**Three things were considered and none of them taken**, because each changes
approved behaviour:

* only apply the check when `DetectAll` returns something — does not help, the
  reference console's Bossarena entries keep the list non-empty;
* treat a title as detected when `/data/GoldHEN/AFR/<title>` exists as a
  directory — plausible, and the AFR root survives a Vanilla activation, but it
  weakens D27 to "somebody made this folder once";
* remember the titles this app has itself written a tree for — new stored
  state, which §4.2's "derived from disk, never a stored flag" argues against.

**What to do now:** milestone 3's own hardware tests are unaffected as long as
the cycle is run in order — H6/H7/H9 are `HARNESS A → HARNESS B` and the
Vanilla leg is the last of the three. Expect the activation **after** the
Vanilla one to refuse. That refusal is the bug report, not a regression. A
decision is needed before milestone 6, which is where `X` on `VANILLA` becomes
a thing a player does by choice.

---

## 3. Decisions the plan left open

### 3.1 Twelve refusal reasons, and the order they are tested in

`RefusalReason` has one value per row of §4.4, plus `None` and the carried-over
`EmptySelection`. They are tested in §4.4's own table order, and the first that
fires is the one reported: a console with three problems is told about the
first one and asked again, rather than handed a list it cannot act on.
`worlds_verify.py` mirrors the function, asserts every reason is reachable, and
reads the enum out of the header so a reason added on one side and not the
other fails rather than passes quietly.

### 3.2 Zero save titles refuses — with a sentence that says to run the game

§4.4's `Save title` row says refuse on "zero, or more than one". H9 says a world
with a stored save on a console where Bloodborne has never run must refuse "and
say to run the game once". On such a console the §8.1 sweep returns zero, so it
is the `Save title` row that fires, not the `Container` row. Both are satisfied
by wording rather than by restructuring: the zero case reads `NO BLOODBORNE
SAVE DATA FOR THIS PLAYER - RUN BLOODBORNE ONCE FIRST`. The `Container` row is
implemented and keeps its own B33 sentence for the case where a save directory
exists but its container cannot be mounted.

### 3.3 Reconciliation takes the AFR title from the setting, not the journal

§4.3 lists the journal's eight fields and the AFR title is not one of them, so
`WorldReconcileJob` is handed the `BLOODBORNE TITLE ID` setting by its caller.
That is consistent with D25/D27 — the setting alone decides the AFR title — and
the journal is left exactly as §4.3 specifies. The cost is that changing the
setting between an interrupted activation and the next launch would reconcile
the wrong title's tree. Adding a ninth field would fix that and would be a
one-line change if the reviewer prefers it.

### 3.4 The journal is created at the start of phase 2

Phase 1 writes nothing, so a journal written before it would describe a
transaction that may never begin and would have to be deleted on every refusal.
It is written when phase 2 is about to run — the first phase that changes
anything — carrying `phase=2`. `ActionForPhase(1)` still exists and discards,
because a journal could in principle be hand-written at phase 1 and because a
table with a hole in it is worse than a redundant row.

### 3.5 A failure leaves the journal on disk

`State::Fail` does not delete the journal. The next launch reconciles it: that
is the whole point of having one, and swallowing it would turn a recoverable
interruption into a console nobody can reason about. The two exceptions are
`Refused` (nothing was written, so there is no journal yet) and the resumed
phase-5 case, which completes or redoes the swap rather than abandoning it.

### 3.6 The staged tree, not `dvdroot_ps4.old`, says whether phase 5 completed

§4.3's reconciliation rule for phase 5 is: "if `dvdroot_ps4.old` exists and
`dvdroot_ps4` does not, rename it back, else the swap completed; then continue
at phase 6." Taken literally that is wrong in two of the four ways phase 5 can
be interrupted. Interrupted *before* the first rename, `dvdroot_ps4` exists and
`.old` does not, so the rule concludes "the swap completed" and phase 6 would
put the incoming world's save on the outgoing world's tree — the silently
incoherent state §7.1 exists to prevent. Interrupted *between* the renames, the
rule puts the old tree back and then still continues to phase 6, with the same
result.

What distinguishes them is the **staged tree**: phase 4 does not advance until
that tree is complete and stamped, and phase 5's second rename is what consumes
it, so a staged tree still present means the swap did not finish. The
implementation puts `.old` back if it needs to, then asks that question and
**redoes the swap** if the answer is no, then continues at phase 6 as §4.3
says. Vanilla stages nothing, so for Vanilla the question is whether the live
tree is gone; redoing is idempotent either way.

### 3.7 `SetLastPlayed` on the incoming world when phase 6 adopts the live save

§4.3 sets `last_played` and `last_played_revision` in phase 3, for the
*outgoing* world, and says nothing about the incoming one. The adopt row of the
phase-6 table writes the incoming world's save, and D3 defines
`last_played_revision` as "the revision current when the save was last
written", so that row sets both. The restore row does not: it reads the world's
stored save rather than writing it. The effect is that a world sorts to the top
of the rail when its save was last written, which is what B2 asks for.

### 3.8 `AdoptLive` collapses to nothing when the container holds no save files

The phase-6 table's adopt row copies "the verified backup" into the incoming
world. When the container exists but holds no `userdata*`/`backup*` files,
**D26** skips phases 2 and 3 and there is no verified backup to copy. That row
therefore resolves to `Nothing` in `PlanActivation`, so the confirmation screen
will say so rather than promising an adoption that cannot happen.
`worlds_verify.py` pins it as its own row of the mirrored table.

### 3.9 `TakeLines()` beside the `EnemyRandomizerJob` job shape

§4.3 asks for "a resumable job shaped like `EnemyRandomizerJob`", which is
`Step`/`Done`/`StatusText`/`Progress`/`Result`. Milestone 6 needs each phase
reported into the existing progress log as it happens, which a single
`StatusText` cannot carry. The job has all five plus `TakeLines()`, the shape
milestones 1 and 2's harness jobs already use. The four progress-log constants
stay in `EnableWizardScreen.cpp` untouched, and `worlds_verify.py` now asserts
both that they are still there and that `WorldActivation.cpp` does not name
them.

### 3.10 Reconciliation resumes through the activation job itself

`WorldReconcileJob` does the sweep, reads the journal and picks the action; for
phases 5, 6 and 7 it constructs a `WorldActivationJob` from the journal and
steps it. Phases 5–7 therefore exist once, not twice. The resuming constructor
takes the save title, directory, policy and safety backup from the journal
rather than rediscovering them — a console that has changed underneath must
finish the transaction it started, not start a different one — and phase 6
recomputes its row from the journal's own data rather than from a plan it never
built.

### 3.11 Phase 1 writes one scratch file, and it cannot not

§4.4's last row is "the AFR root for the `BLOODBORNE TITLE ID` setting is not
writable", and the only way to know that is to try. `AfrManager::Check` creates
`bbrandomizer_write_test.tmp` in the AFR root and unlinks it — the probe that
predates this feature and that `docs/ps4-homebrew-findings.md` §1 records.
Phase 1 therefore is not literally write-free; it writes and removes one empty
file outside `dvdroot_ps4`, and changes nothing a player or the game can see.
Called out because "nothing is written" is a §7 invariant and this is its one
exception.

### 3.12 The harness is two buttons, `L1` and `R1`, and a three-world cycle

Every other button on the probe screen already means something, and milestones
1 and 2's hardware instructions name them — H4 and H5 have not been run yet, so
renumbering them would have invalidated instructions the developer is still
holding. `L1` and `R1` were free.

`R1` needed to reach three targets with one button, so it is a fixed cycle:
`HARNESS A` → `HARNESS B` → `VANILLA` → back to A, with the next target derived
from which world is active now. `HARNESS A` and `HARNESS B` are created on
first use from the **Defaults** tab's current settings, with fixed seeds
1111111 and 2222222 so their trees differ; `HARNESS B` carries `START FRESH`
and is put back on it by an appended revision before each of its turns, because
a successful activation reverts it (B12). Three presses are H6's round trip and
H7's fresh start; the harness prints its target before it begins.

`L1` is the read-only half — sweep, reconcile whatever journal is on disk,
then the constructed refusal table, the live verdict for every world, and the
phase machine. It writes nothing *unless* a journal is present, which is
exactly the case that must be finished rather than looked at.

---

## 4. Verification run

| Check | Command | Result |
| ----- | ------- | ------ |
| Clean build | `cd app && make clean && make` | `IV0000-BBRD00001_00-BBRANDOMIZERMAIN.pkg`, 7,143,424 bytes. No warnings from the new files |
| Worlds mirror | `python app/tools/worlds_verify.py` | **221/221 passing** — manifest 29, world store 54, activation 54, source 84 |
| Activation rules only | `python app/tools/worlds_verify.py activation` | 54/54 |
| Enemy pool | `python app/tools/pool_verify.py selftest ../data/vanilla/dvdroot_ps4` | 89/89 passing |
| Boss pool | `python app/tools/boss_verify.py selftest ...` | selftest 5/5 |
| Treasure | `python app/tools/treasure_verify.py selftest ...` | selftest 8/8 |
| Drops | `python app/tools/drops_verify.py selftest ...` | selftest 6/6 |
| Starting weapons | `python app/tools/starting_weapons_verify.py selftest ...` | selftest 12/12 |
| Caged dogs | `python app/tools/caged_dogs_verify.py selftest ...` | 23/23 passing |
| Easy modes | `python app/tools/easy_modes_verify.py selftest ...` | 25/25 passing |
| Hunter tools | `python app/tools/hunter_tools_verify.py selftest ...` | selftest 13/13 |
| Mergo darkness | `python app/tools/mergo_darkness_verify.py selftest ...` | 9/9 passing |
| Item data | `python app/tools/itemdata_verify.py roundtrip ../data/vanilla/dvdroot_ps4` | round trip byte-identical, 28,309,992 bytes, 65 entries |
| Scroll geometry | `python app/tools/ui_scroll_verify.py` | all geometry and scroll properties PASSED |
| Settings UI | `python app/tools/settings_ui_verify.py` | **39/40 — unchanged from milestone 2** |

The one failure is milestone 2's known, deliberate one, and this milestone
neither caused nor fixed it:

```
  1: all 16 bool fields appear in exactly one entry                        FAILED
39/40 passing
```

`startFreshSave` is the seventeenth `bool` in `RandomizerDefaults` and has no
`SettingsModel` entry until milestone 5 step 1, whose done-condition is that
this verifier passes.

**Not run:** H6, H7, H8, H9 — the hardware tests, which are the handoff. No
console is reachable from here.

---

## 5. What this does not prove

`CLAUDE.md` §3: a clean cross-compile catches C++ errors and the Python mirrors
pin the rules, but neither establishes runtime behaviour. Everything below
rests on the PS4.

* **That any of the seven phases does what it says.** `worlds_verify.py` proves
  the refusal table, the phase-6 table and the phase machine are consistent and
  that the source still orders its writes the way this report claims. It cannot
  run `sceSaveDataMount`, `sceKernelRename` on an 80 MB tree, or a power cut.
* **That the swap is safe.** The rename pair, the removal of
  `dvdroot_ps4.old`, and how long each takes are all untested. The mirrored
  tree measures ~80 MB across six folders on the vanilla data here, so staging
  costs that much again while an activation is in flight, on a partition whose
  free space this platform will not report.
* **That reconciliation recovers anything.** Every branch of it is reasoned and
  none has been interrupted for real. H8 is the only thing that tests it.
* **That `START FRESH` leaves a container the game will accept as empty.**
  Milestone 1's `EmptyContainer` was proven on hardware by the probe (findings
  §8.3); its use *inside a transaction*, with a backup taken first and a policy
  reverted afterwards, has not been.
* **That output parity holds in the produced tree.** The mirror checks the
  mapping and the decision in the source. Only a real run of the same seed and
  recipe through both paths would compare trees, and the wizard's path is still
  there to compare against until milestone 6.
* **Nothing about §2.4.** The B37/B8 contradiction is read out of the source of
  `GameInfo::DetectAll` and `AfrManager::Swap`. The first Vanilla activation on
  hardware is what confirms it.

---

## 6. Hardware test handoff

Everything below is the `SAVE DATA PROBE (TEST)` row on the main menu. **Take a
copy of your save off the console over FTP before starting.** `R1` moves real
save data.

### Before anything

1. Install the `.pkg`.
2. On **Setup Defaults**, confirm `BLOODBORNE TITLE ID` is the title whose AFR
   folder currently holds randomizer output, and turn on at least
   `RANDOMIZE ENEMIES` — the harness worlds are created from these settings,
   and with everything off their trees are plain mirrors and H6's "its
   randomization is A's" has nothing to look at.
3. If milestone 2's **H5** has not been run yet, run it now: `OPTIONS` on the
   probe screen. `R1` refuses with `NO WORLDS FOLDER YET` until it has.

### H9 — refusals, and the checks (`L1`, writes nothing)

Press `L1`. Expect, in order:

* `SWEPT n PARTIAL BACKUP(S), m PARTIAL WORLD SAVE(S)` then
  `NO ACTIVATION WAS INTERRUPTED`;
* `REFUSAL TABLE - SECTION 4.4` with **nineteen `OK` lines and no `WRONG`**,
  each refusing row printing the sentence a player would see;
* `LIVE VERDICTS - NOTHING IS WRITTEN` — one block per world, naming the
  outgoing world, the revision, the seed, which phase-6 row applies, and
  whether a backup and capture will happen;
* `JOURNAL ROUND TRIP OK`, then `PHASE 0` to `PHASE 8` each with one action.

**A failure looks like** any `WRONG` line, or a live verdict refusing for a
reason you do not expect. The second account half of H9 — "a second account
sees none of the first's worlds" — is `LEFT` on a second signed-in account:
expect `NO WORLDS FOLDER FOR THIS ACCOUNT - FIRST RUN`.

### H6 and H7 — the activation cycle (`R1`, destructive)

Press `R1` **once** and read the first lines: it names the target before it
does anything. The cycle is `HARNESS A` → `HARNESS B` → `VANILLA`.

1. **First press — `HARNESS A`.** Expect `BACKING UP THE LIVE SAVE`,
   `SAFETY BACKUP VERIFIED`, the backup filed into the outgoing world,
   `GENERATING WITH SEED 1111111`, `THE NEW TREE IS LIVE`, and
   `HARNESS A HAS NO SAVE - ADOPTING THE LIVE ONE`. Then `ACTIVE NOW WORLD
   ACTIVE w-000N`. Launch Bloodborne: your save is there and the enemies are
   randomized. Play far enough to be recognisable — kill something, move to a
   lamp.
2. **Second press — `HARNESS B`, the `START FRESH` world (H7).** Expect
   `HARNESS B PUT BACK ON START FRESH`, a verified backup, the backup filed
   into `HARNESS A`, a generate with seed 2222222, `EMPTYING THE SAVE
   CONTAINER` with `REMOVED n FILE(S), LEFT 4 ALONE`, then `SAVE DATA IS BACK
   ON KEEP EXISTING - rev-000N`. The container listing at the end must show
   **only the four `sce_sys` entries**. Launch Bloodborne: it offers a new
   playthrough. Create a character.
3. **Third press — `VANILLA`.** Expect the fresh save backed up and filed into
   `HARNESS B`, `VANILLA - NOTHING IS GENERATED`, `THE RANDOMIZER'S FILES ARE
   REMOVED`, and `PUTTING VANILLA'S SAVE BACK` with `RESTORED n FILE(S) …
   AFTER REMOVING m`. **The container listing is the check for H6's second
   half**: it must hold exactly Vanilla's file set, with none of `HARNESS B`'s
   surplus `backup*` files left behind. Launch Bloodborne: your original save,
   unmodified game.
4. **Fourth press — `HARNESS A` again (H6 completes).** Expect
   `PUTTING HARNESS A'S SAVE BACK`. Launch Bloodborne: the progress you made at
   step 1 is intact and the randomization is A's.

**This is where §2.4 bites.** After step 3 the AFR tree for the configured
title is gone, so step 4 is likely to refuse with `BLOODBORNE TITLE ID
CUSA03173 WAS NOT DETECTED - FOUND …`. That refusal is the contradiction in
§2.4, not a defect in the transaction. If you want H6's round trip without it,
run the A → B → A legs first and leave the Vanilla leg until last.

**A failure looks like** `FAILED - …`, a container listing with files from the
wrong playthrough, or a `REACHED PHASE` below 7 on a run that reported no
refusal.

### H8 — interrupted activation

1. Press `R1` and **pull the power** partway through — the phase lines say
   where it is, and the interesting windows are during `GENERATING` (phase 4)
   and immediately after `THE NEW TREE IS LIVE` (phase 6).
2. Power back on, open the probe screen, press `L1`.
3. Expect `AN ACTIVATION WAS INTERRUPTED AT PHASE n`, the action for that
   phase, and then either `THE STAGED TREE WAS DISCARDED - NOTHING ELSE
   CHANGED` (phases 1–4) or the resumed save swap finishing (phases 5–7). Any
   `*.partial` directory under `SaveBackups/` must be gone.
4. Launch Bloodborne and confirm the save is one of the two worlds' saves,
   whole.

Each phase can also be exercised without a power cut: write
`/data/bbrandomizer/activation.journal` over FTP with the eight keys —
`from_world`, `to_world`, `revision`, `save_policy`, `save_title_id`,
`save_dir_name`, `safety_backup`, `phase` — and press `L1`. The file's format
is printed by `L1`'s own `JOURNAL ROUND TRIP` block.

**A failure looks like** a save that will not load, a journal still present
after a successful reconcile, or `THE JOURNAL'S PHASE IS NOT ONE THIS APP
WRITES` for a phase between 1 and 7.

### What to report back

The elapsed milliseconds printed for each activation, the container listings
after steps 2 and 3, and whether step 4 refused with the §2.4 message.

---

## 7. Stop point

**Milestone 3's completion gate.** Its six steps are done, its verification is
run, `make clean && make` produced the `.pkg`, and it is handed over for **H6**
through **H9**. No stop condition fired — §2.4 is a contradiction between the
spec and the platform, reported and left for the developer, not a decision
taken here.

Next is milestone 4, the worlds screen. Per `CLAUDE.md` §4 and the plan's §7
preamble it waits for the developer; the standing direction to build milestones
2–4 ahead of their hardware gates is the developer's to re-confirm or withdraw.
**§2.4 should be settled before milestone 6**, which is where activating
Vanilla stops being a harness button and becomes something a player does.

---

# Implementation Report — Worlds — milestone 2

**Status: AWAITING HARDWARE TEST**

**Plan:** `docs/features/worlds/plan.md` — milestone 2, "the world store"

**Spec:** `docs/features/worlds/spec.md`

**Implemented:** 2026-09-22

> Milestone 1's report is preserved unchanged below the rule at the end of this
> one, and milestone 0's beneath that. One report per milestone, newest first,
> in the folder's one report file.
>
> **Milestone 1's hardware test H4 has not been run.** The developer directed
> that milestones 2–4 be built before their hardware gates (`log.md`,
> 2026-09-22). This milestone was built against milestone 1's service as
> written, and nothing in it re-verifies that service.

---

## 1. What was built

Worlds now exist on disk. `Randomizer/WorldStore.{h,cpp}` owns an
account-scoped store of worlds, append-only revisions, world saves and
timestamped safety backups, with Vanilla created and the live save captured
into it on first run. `RandomizerDefaultsStore` gained the serializer and
parser that `defaults.cfg` and a world's revision file now share, and
`RandomizerDefaults` gained `startFreshSave` — the editor's one `SAVE`
setting, whose struct default is `KEEP EXISTING`. `Game/AfrManager` learned to
read and write `.bbrandomizer_manifest` and to derive which of the four "which
world is active" states the disk is in. The harness gained three operations so
all of it can be exercised on hardware before any world UI exists.

No activation, no journal, no reconciliation, no refusal table, no UI beyond
the harness screen — those are milestones 3 and 4. `ScreenId`,
`Application.cpp`, `SettingsModel`, `Controls`, the randomizer engine, the
pool tables and the Makefile are untouched.

| # | Planned change | Status | Where |
| - | -------------- | ------ | ----- |
| 1 | `startFreshSave` + `start_fresh_save`; extract the serializer and parser into functions shared by `defaults.cfg` and a revision file | done | `RandomizerDefaults.h` (the field and why `KEEP EXISTING` is its default); `RandomizerDefaultsStore.cpp:27` `FormatSettings`, `:76` `ApplySettingKey`, `:144` `ApplySettingsText`, with `:169`/`:185` now composing the two wrapper keys around the shared block. `defaults.cfg` round-trips every existing key — `settings_ui_verify.py` case 8 and `pool_verify.py`'s key cases both still pass |
| 2 | `WorldStore.{h,cpp}` — create, list, load, rename, delete; append and list revisions; `world.cfg` and `worlds.cfg`; account scoping | done | `WorldStore.cpp:667` the constructor (the account directory is derived from the account id and nothing else), `:683` `CreateAccount`, `:789` `List`, `:805` `Create`, `:864` `Delete`, `:921` `Revisions`, `:950` `AppendRevision`. The harness `RIGHT` operation creates, reads back, revises twice, renames, lists and deletes one |
| 3 | World save storage: store, load and verify through the milestone 1 manifest | done | `WorldStore.cpp:561` `StoreSaveJob::Step` (verify source → copy one file per step → manifest → swap → verify destination), `:1009` `VerifyStoredSave`, `SaveDir`/`HasStoredSave`/`StoredSaveManifest` beside it. A stored save is a `savedata::VerifyBackup`-shaped directory, so `worlds_verify.py verify <dir>` reads one directly |
| 4 | Safety backups into `SaveBackups/…_<reason>/`, verified before the caller may proceed, plus a startup sweep of every `*.partial` | done | `WorldStore.cpp:414` `SafetyBackupPath`, `:473` `SafetyBackupJob::Step` — `Result().ok` is not set until `VerifyBackup` has walked the copy. `:420` `SweepPartialBackups` and `:1018` `SweepPartialWorldSaves`. An interrupted backup leaves only `<name>.partial`, which the sweep removes |
| 5 | First-run capture: create the account folder and Vanilla, then take, verify and copy in a safety backup only if a container with `userdata*` exists | done | `WorldStore.cpp:1100` `FirstRunCaptureJob::Step`. Five outcomes: already done; no save data; no container; a container with no save files (spec **D26**); and the capture itself. Ambiguous discovery refuses without creating anything, and a failed capture removes what it made — see §3.8 |
| 6 | `AfrManager`: read and write `.bbrandomizer_manifest`, and expose the §4.2 derivation | done, narrower than §5 | `AfrManager.cpp:113` `FormatManifest`, `:128` `ParseManifest`, `:164`/`:172` the file halves, `:180` `DeriveActive`. The staging path, the swap and the remove-tree that §5's row also lists are deferred — §2.1. The harness prints the live derived state and then every row of the §4.2 table |
| 7 | Extend `worlds_verify.py` with recipe round-trip and world-store cases | done | A new `worlds` section, 54 cases, plus 24 more in the source section. 140 cases total, all passing |

### The §7 invariants

* **Account scoping.** There is no call in `WorldStore` that lists or touches
  a world without an account id, because `WorldStore` has one constructor and
  it takes one (`explicit WorldStore(uint64_t)`, no default). The account
  directory is built once, in that constructor, from `acct-%016llx`.
  `worlds_verify.py` asserts both. The account id itself comes only from
  `savedata::ResolveUser`, which is `sceUserServiceGetNpAccountId` — the value
  hardware showed is exactly the `ACCOUNT_ID` in that player's own save
  (findings §8.5, plan P14).
* **Verify before any destructive operation.** `SafetyBackupJob` does not
  report success until `VerifyBackup` has walked what landed on disk;
  `StoreSaveJob` verifies its source before it copies a byte and verifies the
  destination once it is in place; `VerifyStoredSave` is the same walk again.
  `worlds_verify.py` pins the ordering of all four out of the source.
* **AFR paths confined to `AfrManager.cpp`.** Every path this milestone added
  is built from the one `"/data/GoldHEN/AFR/"` literal in that file. Two older
  callers predate the rule — see §5 and the pinned allowlist in the verifier.
* **`defaults.cfg` tolerance.** `ApplySettingKey` returns false for an unknown
  key and every caller ignores that; an absent key leaves the struct's own
  default standing. `start_fresh_save` absent reads as `KEEP EXISTING`, which
  is the only safe reading of silence (spec **D17**).
* **Output parity.** No randomizer, engine, param, MSB or table file was
  touched. `startFreshSave` is in the recipe and is never read by generation
  (plan P5). The full parity suite was run anyway — §4.
* **Layering.** `WorldStore` makes no save-data or user-service call;
  `worlds_verify.py`'s app-wide check still finds none outside `Platform/`.
  `Game/AfrManager` does not depend on `Randomizer/` — the two world facts the
  derivation needs are passed in as booleans (§3.11).
* **`LIBS` gained nothing.** `app/Makefile` is unchanged, asserted.

---

## 2. Deviations from the plan

Three, plus one knock-on. All four are recorded in the plan's §10.

### 2.1 `AfrManager` gained the manifest and the derivation, not the staging path, the swap or the remove-tree

**What the plan said.** §5's `AfrManager` row: "Manifest read/write, staging
path, swap, remove tree", milestone 2. §7 milestone 2 step 6: "read and write
`.bbrandomizer_manifest`, and expose the §4.2 derivation — done when the
harness prints the derived state for each row of that table."

**What was done.** §7's two, and its done-condition. The staging path, the
swap and the remove-tree are not in this milestone.

**Why.** §7 is the ordered step list with done-conditions and is what this
stage executes; §5 is the file-level summary. The three deferred items have no
caller until §4.3's phases 4 and 5, which are milestone 3, and no milestone-2
hardware test can reach them. Writing them now would put three untested
functions in the tree between two hardware gates, which is the shape of thing
the milestone structure exists to prevent.

**What it costs.** Milestone 3 edits `AfrManager.{h,cpp}`, which §5's
milestone column does not anticipate. The file is listed; only the column
moves.

### 2.2 `app/tools/pool_verify.py` was edited in milestone 2, not milestone 5

**What the plan said.** §5 assigns `pool_verify.py` to milestone 5, for the
progress-log constants' file name after the screen rename.

**What was done.** Its worst-case `defaults.cfg` size case was updated, and
split in two.

**Why.** It was unavoidable and it was the verifier working as designed. The
case is an exact equality — `worst == 616` — with a comment saying "This is an
exact equality on purpose: it fails the moment a key is added or removed
without the buffer being thought about." Step 1 of this milestone adds
`start_fresh_save`, which is 19 bytes with its newline. The new figures are
635 for the whole file and 584 for the settings block, and they are now two
cases because the serializer is now two pieces: `char buf[1024]` holds the
settings block, and the two wrapper keys are built beside it.

**What it costs.** Nothing that was being checked stopped being checked. The
buffer bound is now asserted against the thing that actually goes in the
buffer, which is slightly tighter than before.

### 2.3 The milestone-2 harness operations live in `UI/SaveProbeScreen.{h,cpp}`

**What the plan said.** §5 lists `Platform/SaveDataProbe.{h,cpp}` and
`UI/SaveProbeScreen.{h,cpp}` together as "harness operations for each
milestone", milestones 1–3.

**What was done.** The three new operations are a `WorldHarnessJob` declared
in `UI/SaveProbeScreen.h` and implemented in its `.cpp`.
`Platform/SaveDataProbe.{h,cpp}` are **unchanged**.

**Why.** These operations drive `Randomizer/WorldStore` and
`Game/AfrManager`. `Platform/` may include neither — and `WorldStore.h`
already includes `Platform/SaveData.h`, so a `Platform/` → `Randomizer/`
include would be a cycle between two layers rather than merely an inversion.
`UI/` is the one layer from which both halves are reachable, and milestone 1
put the AFR half of the harness there for exactly this reason (its §3.10).

**What it costs.** The harness is split across two files with two job types,
and the screen holds one pointer for each. It also means milestone 1's H4
button map is untouched: `X`, `SQUARE` and `TRIANGLE` do precisely what
milestone 1's handoff says they do, which matters because H4 has not been run
yet.

### 2.4 Knock-on: `settings_ui_verify.py` case 1 now fails, and was left failing

**What happens.** `1: all 16 bool fields appear in exactly one entry` fails.
`startFreshSave` is a seventeenth `bool` in `RandomizerDefaults` with no
`SettingsModel` entry, and that case exists to catch precisely that.

```
  1: all 16 bool fields appear in exactly one entry                        FAILED
39/40 passing
```

**Why it was left.** Milestone 5 step 1 adds the `SAVE DATA` entry and its
done-condition is "done when `settings_ui_verify.py` passes" — the plan
expects this to be closed by adding the entry, not by exempting the field in
the verifier. Milestone 2's own verification list (§6) does not include that
verifier.

**What it costs, and what the developer may want to decide.** The verifier is
red from here until milestone 5. §6 lists it at **milestone 4's** gate as
well, where it will still be red for this one reason. The options are to
expect the single known failure at that gate, or to pull milestone 5's step 1
forward into milestone 4. Not decided here.

---

## 3. Decisions the plan left open

### 3.1 `WorldStore` is a class scoped to one account, not free functions

`RandomizerDefaultsStore` is free functions and `AfrManager` is static
methods, so either shape had precedent.

Taken: a class holding the account id, with no default constructor. §3.1's
invariant is "every save search and world listing is scoped to one user id",
and a class makes that structural rather than remembered — there is no way to
spell a listing call without having produced an account id first. The
alternative, free functions each taking an id, works only as long as every
future caller remembers to thread it through, and the failure it permits
(listing another account's worlds) is one spec §7.1 singles out.

### 3.2 A recipe is a seed plus a whole `RandomizerDefaults`

The plan says a revision is "`seed=` plus every setting", and calls what a
world stores a *recipe*.

Taken: `struct WorldRecipe { uint32_t seed; RandomizerDefaults settings; }` —
the same struct the editor edits and the randomizer is handed, whole. A
narrower settings type would have to be kept in step with
`RandomizerDefaults` by hand, and a recipe that dropped a field would generate
a different world from the one that was approved, silently. The cost is that
`bloodborneTitleId` and `lastSeed` ride along in the struct and are simply not
written into a revision file — `worlds_verify.py` asserts both keys are absent
from `FormatSettings`, which is what spec **D25** requires.

### 3.3 Two recipes are the same recipe when they serialize identically

The plan (P4) says a revision is appended only when the recipe differs. It
does not say how "differs" is computed.

Taken: `SameRecipe` compares the seed and `FormatSettings(a) ==
FormatSettings(b)`. A hand-written field-by-field comparison forgets whichever
field was added last, and the failure it produces is a world whose edit
silently appended no revision. Serializer equality cannot forget a field,
because the serializer is the same one that writes the file. It also makes the
rule easy to mirror in Python, which `worlds_verify.py` does.

### 3.4 The extracted serializer is three functions, and keeps its `snprintf` shape

The plan says to "extract the serializer and parser into functions shared by
`defaults.cfg` and a revision file".

Taken: `FormatSettings` (the settings block only),
`ApplySettingKey(const char*, const char*, ...)` (the `strcmp` chain, every
key including the two only `defaults.cfg` writes), and `ApplySettingsText`
(the line splitter). Three shapes were constrained by things outside the
contract:

* `FormatSettings` keeps `char buf[1024]` and one `snprintf` because that is
  what the file already does and because `pool_verify.py` reads the buffer
  size out of the source to bound the config line.
* `bloodborne_title_id=%s` and `last_seed=%u` stay `snprintf` format strings
  in `SaveRandomizerDefaults` rather than becoming string concatenation,
  because `settings_ui_verify.py` derives the store's *written* key set from
  `(\w+)=%[dsu]` and pins that every key read is also written.
* `ApplySettingKey` takes `const char*`, not `std::string`, because the same
  verifier derives the *read* key set from `strcmp(key, "...")`.

Its parameter is named `defaults` rather than `settings` for the same family
of reasons: `pool_verify.py` checks the `start_with_hunter_tools` key is bound
to `defaults.startWithHunterTools`, which is how it catches a key that is
written but never read. Contorting code to satisfy a verifier is usually
wrong; here the verifier is checking something real and the name it wants is
also the one the neighbouring function uses.

### 3.5 A world's save is swapped in through `.partial` and `.old`, and verified at both ends

§4.1 gives the `.partial`-then-rename discipline for safety backups. A world's
save is written by the same kind of copy and the plan does not say whether it
gets the same treatment.

Taken: it does, plus one more rename. `StoreSaveJob` copies into
`save.partial/`, writes the manifest last, renames an existing `save/` to
`save.old/`, renames `save.partial/` to `save/`, and only then removes
`save.old/`. A failed second rename puts `save.old/` back before failing. The
plain alternative — remove the old save, then rename — has a window in which
the world has no save at all, and this is a directory holding a playthrough.
Both `.partial` and `.old` are swept at startup.

The source is verified before the copy starts and the destination is verified
once it is in place. The first is because this job is the only way a save gets
into a world, so nothing that already disagrees with its own manifest should
be allowed to become one; the second is milestone 2 step 3's done-condition,
"done when a stored save verifies", taken literally.

### 3.6 `SafetyBackupJob` verifies itself; `savedata::BackupJob` still does not

Milestone 1 deliberately left `VerifyBackup` as a separate call the caller
makes (its §3.5), so that verification is an independent walk of what landed
on disk.

Taken: that stays true, and `SafetyBackupJob` is the caller. It runs
`savedata::BackupJob` to completion and then walks the result, and
`Result().ok` is false until that walk passes. The point is that "took a
safety backup" and "has a usable safety backup" cannot come apart at a call
site that forgot the second half — spec §7.1 makes verification a
precondition of touching the live save, and milestone 3's phase 2 will depend
on this one flag.

### 3.7 World names are normalised in the store, not only in the editor

Plan P10 caps names at 16 characters of `A–Z`, `0–9` and space, and §7
milestone 5 step 4 builds the editor for them.

Taken: `NormalizeWorldName` is applied by `Create` and `Rename` as well.
`world.cfg` is one `key=value` line per field, so a name carrying a newline
would silently corrupt the file and take the world's metadata with it. This is
file-format defence, not the editor — milestone 5 still builds the character
editor, and this is what stops anything else ever writing a name the format
cannot hold.

### 3.8 First-run capture refuses rather than spending its one run, and rolls itself back

B26 says the live save is captured into Vanilla before anything else happens.
§4.2 says "no account world directory" *is* the first-run state. Between them
sits a case the plan does not name: what happens when the save-data title or
directory is ambiguous on first run.

Taken: two rules.

* **It looks before it creates.** Discovery and the container read happen
  before the account directory exists. Zero save titles, no container, or a
  container holding no `userdata*`/`backup*` (spec **D26**) all create Vanilla
  with no save and say why — those are settled answers. More than one save
  title, or more than one directory for one title, **creates nothing** and
  reports; the next launch is still a first run.
* **A failed capture undoes itself.** If the backup or the copy fails after
  the directory was created, `DestroyAccount()` removes it, so the next launch
  is a first run again. The safety backup it may already have taken stays
  where it is — those are never deleted, and one without a world to belong to
  is still the player's save.

The alternative, creating Vanilla and moving on, ends the first run
permanently on a console whose save could not be identified, and the live save
would then never be filed. Ambiguity is a milestone 3 refusal; here it is a
reason to try again later.

### 3.9 The `.partial` sweep covers world saves as well as safety backups

§4.3 says startup "removes every `SaveBackups/*.partial`". §7 step 4 says "a
startup sweep of every `*.partial`".

Taken: both. `SweepPartialBackups()` is the `SaveBackups` half and needs no
account; `WorldStore::SweepPartialWorldSaves()` is the account's half and
removes `*/save.partial` and `*/save.old`. Splitting them means the first can
run even when `ResolveUser` fails, which is when there is no `WorldStore` to
call the second on. A `save.partial` left behind is ~27 MB of debris on a
partition whose free space cannot be measured, so leaving it out would have
been a slow leak with no way to notice it.

### 3.10 `Delete` does not know which world is active

B17 says the active world cannot be deleted. `WorldStore::Delete` refuses
Vanilla (spec **D16**) and nothing else.

Taken: the active-world refusal belongs to the screen, which is milestone 5
step 7 ("refused on the active world"). Only a caller that has read the AFR
manifest knows which world is active, and giving the store a second source for
that would be a copy of the derivation that could disagree with
`DeriveActive`. Recorded here because the store is, for now, deletable in a
way the finished feature is not.

### 3.11 `DeriveActive` is a pure function, so `Game/` does not learn about worlds

§7 step 6 says `AfrManager` should "expose the §4.2 derivation". Two of that
table's four rows are facts about the world store, not about AFR.

Taken: `DeriveActive(status, manifest, accountDirExists, worldIsKnown)` — the
two world facts arrive as booleans. `Game/AfrManager` therefore does not
include `Randomizer/WorldStore`, the derivation is a pure function of four
inputs, and every row of the table can be exercised on a console that can only
be in one of them. The harness does exactly that, and `worlds_verify.py`
mirrors the same function and runs all sixteen combinations of its inputs.

### 3.12 A `.bbrandomizer_manifest` with no `world_id` reads as absent

The plan does not say what a truncated or empty manifest means.

Taken: absent. `ParseManifest` returns false unless it saw a non-empty
`world_id`, so a half-written file derives to `Unmanaged` (randomizer files
with no manifest) rather than to a world named `""`. That is the same rule the
save manifest uses for a missing `title_id`, and it fails towards the state
whose offered fix is "activate something", which is the recoverable one.

### 3.13 `WorldStore.cpp` carries its own file helpers

§4's reuse table lists `Randomizer/FileIo.cpp`'s `CopyDirRecursive`,
`MakeDirsRecursive` and chunked read as reused as-is, and `WorldStore` is in
`Randomizer/` where `FileIo` lives.

Taken: `WorldStore.cpp` carries its own BSD flag table, dirent struct,
directory listing, `RemoveTree` and chunked copy — the same shape
`Platform/SaveData.cpp`, `Game/AfrManager.cpp`, `Game/GameInfo.cpp` and
`FileIo.cpp` itself each already carry. Two reasons beyond the house pattern:
`CopyDirRecursive` is best-effort (it continues past a failed entry) and skips
`*.bak`, neither of which is acceptable for a copy whose whole purpose is to
be verifiable; and `FileIo` has no directory listing or tree removal in its
header, both of which this needs.

### 3.14 The harness gained three buttons, and the existing three are untouched

The plan does not say how the harness presents its operations. The screen had
`X`, `SQUARE`, `TRIANGLE` and `O`.

Taken: `LEFT` (worlds report), `RIGHT` (world store test) and `OPTIONS`
(first-run capture), all on the free buttons, with the first three left
exactly as milestone 1 shipped them. A selection list would have read better
but would have invalidated milestone 1's H4 handoff, which is written as
button presses and has not been run yet.

The store test **refuses to run** before first-run capture has: creating the
account directory is what ends the first run, and a diagnostic that ended it
before the live save was filed would destroy the thing the feature exists to
protect.

### 3.15 The verifier gained a third section and a pinned AFR-path allowlist

§6 says `worlds_verify.py` pins the rules. Milestone 1 also gave it a source
section (its §3.14).

Taken: a third mode, `worlds`, holding the world-store rules in Python —
recipe format and round trip, recipe identity, id allocation, rail order, name
normalisation, both config key sets, and the §4.2 derivation run exhaustively
over all sixteen shapes of disk. The settings key set is read out of the C++
rather than retyped, so a mirror cannot go on passing after the store stops
writing a key.

One case is unusual and deliberate: §3.1 requires `/data/GoldHEN/AFR/...` in
exactly one file, and it is currently in three. The verifier pins the exact
set — `AfrManager.cpp`, `GameInfo.cpp`, `EnableWizardScreen.cpp` — so a
**new** one fails the check while the two that predate the rule are visible
rather than silently tolerated. See §5.

### 3.16 Vanilla's `last_played_revision` is empty

Spec **D3** records the revision current when a save was last written. Vanilla
has no revisions (spec **D16**).

Taken: empty. `SetLastPlayed("vanilla", "")` still records *when*, which is
what the rail sorts on, and the harness prints `LAST PLAYED ON -` for it. A
placeholder id would be a revision that does not exist.

### 3.17 The account directory is `acct-%016llx`

§4.1 says `Worlds/acct-<16 hex account id>`. Case is not specified.

Taken: lowercase, zero-padded to sixteen. Fixed width so the directory sorts
and compares as one token, lowercase because it is a path.

### 3.18 The store test burns a world id

`Create` allocates one above the highest ever used and ids are never reused
(plan P3), so the harness's scratch world consumes `w-0001` on a fresh
console and the developer's first real world is `w-0002`.

Taken: accepted. Reusing the id would break the one property P3 exists to
give, and the alternative — a separate id space for test worlds — is
scaffolding inside scaffolding for something milestone 6 deletes.

---

## 4. Verification run

| Check | Command | Result |
| ----- | ------- | ------ |
| Clean rebuild | `cd app && make clean && make` | `IV0000-BBRD00001_00-BBRANDOMIZERMAIN.pkg` built, 7,143,424 bytes; `eboot.bin` 3,839,232 bytes |
| Compiler warnings | same, `-Wall` is on | none |
| Worlds mirror | `python app/tools/worlds_verify.py` | manifest rules 29/29, **world store rules 54/54**, source invariants 57/57 — "all checks passing" |
| Pool / defaults mirror | `python app/tools/pool_verify.py selftest data/vanilla/dvdroot_ps4` | 89/89 passing |
| Scroll geometry | `python app/tools/ui_scroll_verify.py` | all geometry and scroll properties PASSED |
| Settings UI mirror | `python app/tools/settings_ui_verify.py` | **39/40 — one known failure**, see §2.4 |
| Output parity — boss | `boss_verify.py selftest …` | 5/5 |
| Output parity — treasure | `treasure_verify.py selftest …` | 8/8 |
| Output parity — drops | `drops_verify.py selftest …` | 6/6 |
| Output parity — starting weapons | `starting_weapons_verify.py selftest …` | 12/12 |
| Output parity — caged dogs | `caged_dogs_verify.py selftest …` | 23/23 |
| Output parity — easy modes | `easy_modes_verify.py selftest …` | 25/25 |
| Output parity — hunter tools | `hunter_tools_verify.py selftest …` | 13/13 |
| Output parity — Mergo darkness | `mergo_darkness_verify.py selftest` | 6/6 |
| Output parity — item data | `itemdata_verify.py roundtrip …` | round trip byte-identical, 28,309,992 bytes; 65 entries unchanged |
| Harness string widths | `settings_ui_verify.load_atlas()` / `width()` at scale 3 | widest static body line 942 px, widest composed runtime line 1,483 px, against the 120→1680 band (1,560 px); longest footer 1,223 px against 1,920 |

The one failure, in full:

```
  1: all 16 bool fields appear in exactly one entry                        FAILED
39/40 passing
```

**Not run**, and why:

* **H5** — hardware. It is this milestone's point and only the PS4 can run it.
* **H4** — milestone 1's hardware test, still outstanding by the developer's
  own direction. Nothing here re-verifies milestone 1, and nothing here
  changed it: `Platform/SaveData.{h,cpp}` and `Platform/SaveDataProbe.{h,cpp}`
  are byte-identical to what milestone 1 left.
* The `verify` and `filter` modes of the parity verifiers, which need an
  output tree from a real run. `selftest` is what runs without one, and it is
  what the milestone-2 change could plausibly break — the defaults chain.
* `worlds_verify.py verify <dir>` against a real world save. The first
  directory it can read is the one H5 produces.

---

## 5. What this does not prove

Everything about runtime, and rather more of it than milestone 1.

* **No world has ever been written on hardware.** Every path in `WorldStore`
  is new and untried: `sceKernelRename` on a directory is called here for the
  first time in this app's history outside milestone 1's backup, and the whole
  `.partial`/`.old` swap rests on it being atomic and working at all on this
  filesystem.
* **First-run capture has never run.** It reads save data through milestone
  1's service, whose own hardware test has not happened either. If `BackupJob`
  is wrong, this is where it first matters to a player's save rather than to a
  diagnostic.
* **The rollback has never fired.** `DestroyAccount()` removing a directory it
  just created is exactly the kind of code that is written once and never
  executed until the day it matters.
* **The derivation has only ever been computed on constructed inputs.** The
  harness prints the live state, but a console can only be in one row of that
  table at a time, and the console this will run on is in the `Unmanaged` row
  (evidence §E5.3) until something is activated.
* **`worlds_verify.py` proves the rules are consistent and that the source
  still says what it says.** It cannot show the C++ implements any of it.

A clean build and a green mirror mean **ready for hardware testing**.

Two pre-existing things this milestone did not fix, and deliberately did not
touch:

* `Game/GameInfo.cpp:13` and `UI/EnableWizardScreen.cpp:494` each build
  `/data/GoldHEN/AFR/...` themselves, which §3.1 says should appear in exactly
  one file. Both predate this feature; the wizard is retired in milestone 6
  and `GameInfo` detects installs by content rather than by AFR management, so
  neither is in this milestone's scope. `worlds_verify.py` now pins the set so
  a fourth cannot appear unnoticed.
* `MenuScreen` still says `SAVE DATA PROBE (TEST)` on its row while the screen
  itself says `SAVE DATA HARNESS`. Milestone 6 deletes both.

---

## 6. Hardware test handoff

Install the `.pkg` from `app/`. The harness is main menu → **SAVE DATA PROBE
(TEST)**.

**This build still contains milestone 1's H4 test, unchanged.** If H4 has not
been run, run it first — `TRIANGLE`, per milestone 1's handoff below — because
everything in milestone 2 is built on the backup and restore it exercises.

**Before anything: confirm the verified byte-exact backup is still on the PC**
(`data/Save Backups/CUSA00207_SPRJ0005_20260921-032915`, 26 files, 26,949,914
bytes).

### Order

Each visit to the screen runs one operation. Leave with `O` and re-enter
between them. **Run `OPTIONS` before `RIGHT`** — the screen says so, and
`RIGHT` refuses if you do not.

1. **`LEFT` — worlds report.** Read-only, apart from sweeping `*.partial`
   debris. On a console that has never run this build it should say
   `NO WORLDS FOLDER FOR THIS ACCOUNT - FIRST RUN`.
2. **`OPTIONS` — first-run capture.** This is **H5**. It reads save data and
   never writes it, and it writes into `/data/bbrandomizer/` only.
3. **`LEFT` again** — the same report, now showing Vanilla with a save.
4. **`RIGHT` — world store test.** Creates a scratch world, revises it,
   renames it and deletes it. Never touches save data.
5. **`OPTIONS` again** — must do nothing at all.

Pull `live.log` afterwards — every line is written there as it is produced.

### What a pass looks like

| Step | Pass |
| ---- | ---- |
| 1 `LEFT` | `SWEPT 0 PARTIAL BACKUP(S), 0 PARTIAL WORLD SAVE(S)`; `NO WORLDS FOLDER FOR THIS ACCOUNT - FIRST RUN`; the `AFR` block showing `SEEDED YES MANIFEST NO` and `STATE FIRST RUN`; then `DERIVATION TABLE` with five `OK` rows, `AFR MANIFEST ROUND TRIP OK`, `RECIPE ROUND TRIP OK`, `SAVE DATA POLICY OK` |
| 2 `OPTIONS` | one `… BYTES COPIED` line per file, `SAFETY BACKUP VERIFIED - 26 FILE(S), 26949914 BYTES`, then one `… BYTES COPIED` line per file again for the copy into Vanilla, `SAVE STORED AND VERIFIED`, and `ALREADY DONE NO   CREATED VANILLA YES   CAPTURED SAVE YES` with a `SAFETY BACKUP /data/bbrandomizer/SaveBackups/…_firstrun` path and an elapsed figure |
| 3 `LEFT` | `VANILLA` with `SAVE 26 FILE(S), 26949914 BYTES, 1136 BLOCKS`, a `CAPTURED` timestamp, `LAST PLAYED` set; `WORLDS 0`; `STATE UNMANAGED` (this console has randomizer files this app did not write — spec **D18**, and it is the correct answer) |
| 4 `RIGHT` | `CREATED w-0001`; `RECIPE ON DISK MATCHES`; `SAME RECIPE APPENDED NO`; `CHANGED RECIPE APPENDED YES`; `HISTORY 2 (NEWEST FIRST)` listing `rev-0002` then `rev-0001`; `RENAMED TO HARNESS RENAMED - REVISIONS 2`; `NAME FILTER "A VERY LONG NAME"`; `DELETED w-0001` with `SAVE KEPT AT (IT HAD NONE)`; `STILL THERE NO`; then the two `VANILLA … BLOCKED` lines |
| 5 `OPTIONS` | `THIS PLAYER ALREADY HAS WORLDS - NOTHING TO DO`, and nothing else |

**Afterwards, over FTP**, `/data/bbrandomizer/` should hold
`Worlds/acct-<16 hex>/worlds.cfg` (`next_world_id=2`), `Worlds/acct-<…>/vanilla/`
with `world.cfg` and `save/{manifest.txt,data/}`, and
`SaveBackups/CUSA00207_SPRJ0005_<stamp>_firstrun/`. No directory anywhere
should end in `.partial` or `.old`. Copy the world's `save/` off the console
and run `python app/tools/worlds_verify.py verify <that directory>` — it
should print `verified`.

**Then launch Bloodborne once** and confirm the save loads and the character
is where it was. Nothing in this milestone writes save data, so it must be
untouched; that is the claim being checked.

### What a failure looks like

* **`LEFT` reports `STATE FIRST RUN` after `OPTIONS` succeeded.** The account
  directory was not created or is not where it is looked for. Report the two
  `WORLDS IN …` paths.
* **`OPTIONS` ends `FIRST RUN CAPTURE FAILED - …`.** The account directory
  should have been removed again — confirm `Worlds/acct-<…>/` is gone and that
  the `SaveBackups/…_firstrun` directory either does not exist or has no
  `.partial` suffix. Report the whole line; it names which half failed.
* **A `.partial` survives a failure.** That is the one state the design says
  cannot exist. Report it and do not delete it.
* **Any `WRONG` line in `DERIVATION TABLE`, or a `ROUND TRIP FAILED`.** The
  C++ disagrees with the plan's own table. Report the line; milestone 3 must
  not start.
* **`RIGHT` reports `SAME RECIPE APPENDED YES`.** A rename or a no-op edit
  would then grow the history forever. Report it.
* **`DELETE FAILED - COULD NOT KEEP THE SAVE …`.** The save was not
  destroyed — that is the designed refusal — but the world is still there.
  Report it.
* **Bloodborne's save has changed.** Nothing here writes save data, so this
  would mean milestone 1's `BackupJob` is writing through a read-only mount.
  Stop, restore from the PC copy, and report.

---

## 7. Stop point

**Milestone 2's completion gate.** Its seven steps are done, its verification
is run, `make clean && make` produced the `.pkg`, and it is handed over for
**H5**. No stop condition fired.

Next is milestone 3, the activation transaction — phase 1's refusals, the
journal, phases 2–7 and reconciliation. Per `CLAUDE.md` §4 and the plan's §7
preamble it waits for the developer; the standing direction to build
milestones 2–4 ahead of their hardware gates is the developer's to re-confirm
or withdraw.

---

# Implementation Report — Worlds — milestone 1

**Status: AWAITING HARDWARE TEST**

**Plan:** `docs/features/worlds/plan.md` — milestone 1, "the save-data service"

**Spec:** `docs/features/worlds/spec.md`

**Implemented:** 2026-09-22

> Milestone 0's report is preserved unchanged below the rule at the end of this
> one. One report per milestone, newest first, in the folder's one report file.

---

## 1. What was built

`Platform/SaveData.{h,cpp}` — the app's one door to PS4 save data, as
production code: identity, save-title discovery, directory search, a container
read, an empty, a manifest with its verifier, and backup and restore as
resumable jobs. Nothing above `Platform/` names a mount, and no orbis type
appears in the header. The temporary probe was rewritten as an operations
harness that drives those calls and prints what they return, so milestone 1 can
be hardware-tested before any world UI exists. `app/tools/worlds_verify.py` is
new and mirrors the manifest rules and the readable half of the plan's §3.1
invariants.

No world store, no activation, no journal, no UI beyond the harness screen. The
Makefile, `LIBS`, `ScreenId`, `Application.cpp`, the settings model and every
randomizer file are untouched.

| # | Planned change | Status | Where |
| - | -------------- | ------ | ----- |
| 1 | `Platform/SaveData.h` — `ResolveUser`, `DiscoverSaveTitle`, `FindSaveDirs`, `ReadContainer`, `EmptyContainer`, `BackupJob`, `RestoreJob`; compiles with no orbis type in the header | done | `app/src/Platform/SaveData.h` (303 lines). All seven names are the plan's, inside `namespace bbr::savedata` — see §3.1 |
| 2 | Identity and discovery, findings §8.1 and §8.5, refusing on zero or multiple matches | done | `SaveData.cpp:466` `ResolveUser`, `:500` `FindSaveDirs`, `:543` `DiscoverSaveTitle`. The harness prints the save title, directory, account and both AFR figures — `SaveDataProbe.cpp` stage `kIdentity` |
| 3 | `ReadContainer` — the findings §8.2 walk, returning file list, per-file sizes, total and block count; the bracketing read | done | `SaveData.cpp:593`, walking via `WalkSized` (`:282`). Harness stages `kReportPresent` and `kReportAbsent` print it for a container and for one that does not exist |
| 4 | The manifest of §4.1 and its verifier | done | `SaveData.cpp:676` `FormatManifest`, `:697` `ParseManifest`, `:767`/`:771` file I/O, `:780` `VerifyBackup`. Written last inside `<name>.partial/`, then renamed (`:962-999`) |
| 5 | `EmptyContainer`, findings §8.3 | done | `SaveData.cpp:622`, bracketed by two `ReadContainer` calls; `UnlinkGameSaveFiles` (`:439`) takes only root-level `userdata*`/`backup*` |
| 6 | `BackupJob` and `RestoreJob` as resumable jobs, one file per step, findings §8.2 and §8.4 | done | `SaveData.cpp:900` `BackupJob::Step`, `:1058` `RestoreJob::Step`. The harness records `StatusText()` after every `Step()`, which is one line per file |
| 7 | Point the harness at the service — `SaveDataProbe.cpp` has no mount call of its own | done, and further than asked | `SaveDataProbe.cpp` now contains **no orbis call at all** — no `sceSaveData*`, `sceKernel*` or `sceUserService*`. See §2.1 for what that cost |
| 8 | `app/tools/worlds_verify.py` covering the manifest cases | done | 29 manifest cases + 33 source-invariant cases, all passing |

### The §7 invariants

* **findings §8.6 in full.** `CREATE2` is never set and `sceSaveDataDelete` is
  never called — `worlds_verify.py` asserts both strings are absent from every
  file under `app/src/`, comments stripped. No path under `sce_sys` is ever
  opened for writing: `RestoreJob` filters those entries out when it builds its
  write list, and counts them as `skipped`. `EmptyContainer` and `RestoreJob`
  each read the container before and after they write it. `RestoreJob` compares
  the manifest's `blocks` against the container's own before it mounts
  read-write.
* **No orbis API outside `Platform/`.** Asserted by the verifier, which walks
  `app/src/` and fails on a `sceSaveData`/`sceUserService` call anywhere else.
* **A read-only mount is never written to.** The two read-only paths
  (`ReadContainer`, `BackupJob`) only walk and read; the mount mode is chosen by
  one `bool readWrite` argument at `SaveData.cpp:391`.
* **Search results are selected by name, never by index.** `cond.key` and
  `cond.order` are never assigned (asserted), `FindSaveDirs` returns the whole
  sorted list, and a directory is used only when there is exactly one.
* **Every search is scoped to one user id** — `cond.userId` is always
  `User::userId`, and `User` only ever comes from `ResolveUser`.
* **The AFR title and the save-data title stay independent.** The service is
  never handed the AFR title; the sweep is its own six-SKU list. The harness
  prints both side by side so the difference is visible on hardware.
* **`LIBS` gained nothing** — `app/Makefile` is unchanged, asserted by the
  verifier.
* **Files are sized by reading to a short read.** `sceKernelStat` does not
  appear in `SaveData.cpp` (asserted).
* **Output parity** — no randomizer, settings or table file was touched.

---

## 2. Deviations from the plan

One. It is recorded in the plan's §10.

### 2.1 Milestone 0's write probe was deleted rather than repointed

**What the plan said.** §7 milestone 1 step 7: "Point the harness at the
service — done when `SaveDataProbe.cpp` has no mount call of its own." §5 keeps
`SaveDataProbe.{h,cpp}` and `SaveProbeScreen.{h,cpp}` as "harness operations for
each milestone", deleted in milestone 6.

**What was done.** `SaveDataProbe.{h,cpp}` were rewritten from 2,338 lines of
diagnostic into a 394-line harness that owns no save-data code. The milestone-0
write probe — its `RDWR|CREATE2` mount, its `sceSaveDataDelete`, its `statvfs`
dump, its scratch-directory name and guard — is gone, as are the read-only probe
and the two delete-by-prefix probes.

**Why.** Those steps cannot be expressed through `Platform/SaveData`, because
§3.1 forbids the service from having any of them: `CREATE2` is never set,
`sceSaveDataDelete` is never called. Keeping them would have meant either
leaving mount calls in the file (failing step 7's done-condition) or building
the two forbidden operations into the service. Everything they established is
already written up in `technical-findings.md` §§1–7, which is where the plan
sends an implementer for it, and §7's preamble describes the scaffolding as "the
harness that hardware-tests milestones 1–3" — P11 calls that a promotion to an
operations harness. The read-only probe and the prefix deletes are now
`ReadContainer` and `EmptyContainer`, exercised through the harness.

**What it costs.** The milestone-0 diagnostics cannot be re-run from this build.
If a question about `CREATE2`, `sceSaveDataDelete` or `statvfs` reopens, it
comes back through the findings document or a fresh probe, not from this file.

---

## 3. Decisions the plan left open

### 3.1 A `savedata` namespace, so the plan's names can be used exactly

The plan names `ReadContainer`, `EmptyContainer`, `BackupJob`, `RestoreJob` and
`Manifest`-shaped types. Those are too generic to sit bare in `bbr`, where
`FileIo` already has `ReadWholeFile` and the randomizer has a `Job`.

Taken: `namespace bbr { namespace savedata { … } }`, so every identifier is the
plan's word with no prefix invented for it — `savedata::BackupJob`,
`savedata::ReadContainer`. It is the first nested namespace in the app; the
alternative was renaming the plan's symbols, which §7 says not to do.

### 3.2 `ResolveUser` does not fall back to the initial user

Milestone 0's probe fell back from `sceUserServiceGetForegroundUser` to
`GetInitialUser` so it stayed runnable. §4.4's refusal table says production
refuses when there is "no foreground user, or no account id for it".

Taken: the contract. `ResolveUser` uses the foreground user only, and returns
`valid = false` with a sentence otherwise. Findings §11 records that the two
calls are still indistinguishable on this single-account console, so a fallback
would be untested code guarding an untested case, and the failure it would
produce — acting for a different signed-in player than the one holding the pad —
is the one the account scoping exists to prevent.

### 3.3 The sweep is the six SKUs, and does not include the configured AFR title

Milestone 0's probe searched the configured title first, then the six official
SKUs. Findings §8.1 sweeps "the six known Bloodborne title ids".

Taken: the six, and only the six. Including the AFR title would make the two
values interact in exactly the way §3.1 forbids — a console whose AFR title
happened to have save data would then report two save titles and refuse. The
consequence is that a save written under a retitled, non-official SKU is not
found, and the app refuses with "NO BLOODBORNE SAVE DATA FOR THIS PLAYER" rather
than guessing. That is a refusal, not a loss.

### 3.4 `ReadContainer` reads every byte; `BackupJob` does not use it

`ReadContainer` sizes by reading each file through, which is what makes it
usable as the bracketing read — it proves the file is readable as well as how
long it is. On the reference save that is ~27 MB, about a second.

A backup that called it would read the container twice. Taken: `BackupJob`
walks names only (`WalkNames`) and takes each file's size from the copy it is
already making, so a backup reads the container once. `ReadContainer` is used
where a full read is the point: the two brackets around an empty and around a
restore, and the harness's report.

### 3.5 `BackupJob` does not verify itself

§4.1 orders a backup copy → manifest → rename; milestone 1 step 4's
done-condition says "a backup produces `manifest.txt`, **a re-verify** passes".

Taken: the job writes, and `VerifyBackup` is a separate call the caller makes.
The job still checks every file as it copies (a short write fails it) and the
manifest is written last so an unfinished copy has none. This keeps the
verification an independent walk of what landed on disk rather than a restatement
of what the copy thought it wrote, which is the only version of it worth having.
Milestones 2 and 3 call `VerifyBackup` before they let anything proceed.

### 3.6 A restore confirms the **written** set, not the whole container

§4.3's phase-6 table says "verify the written set against the manifest".
`sce_sys` is in the manifest but is never written.

Taken: after the restore, the container is read back and compared against the
manifest entries **outside** `sce_sys`, in count, path and per-file size; the
`sce_sys` entries are reported (`skipped`) and not asserted. Asserting them
would make a restore fail if the system ever changed a byte count inside
`sce_sys`, which is a directory this app is not allowed to write and therefore
cannot be responsible for.

### 3.7 `RestoreJob` carries its own refusals

§4.4 puts every refusal in activation's phase 1, which is milestone 3.

Taken: `RestoreJob` re-checks the four that are about the restore itself —
owning account, target title, target directory, container large enough — in its
own phase 1, before it mounts read-write. Spec §7.1 and B25 state those as
properties of a restore ("before writing a byte"), and milestone 1 has to be
hardware-testable on its own. Milestone 3's phase 1 will check them earlier and
more loudly; this is the belt under that brace, not a replacement for it.

### 3.8 An unlink refused during an empty fails the operation

Findings §8.3 measured `REMOVED 11 / REFUSED 0` and does not say what a refusal
means.

Taken: `refused > 0` fails both `EmptyContainer` and `RestoreJob`'s empty phase,
and the restore does not go on to write. A container that is half-emptied is
exactly the §3.3 hazard — another playthrough's surplus `backup*` files
surviving into a restored save — and writing over it would bury the evidence.

### 3.9 The harness is a resumable job, stepped once per frame

The plan does not say how the harness runs. Milestone 0's probe ran inside one
`Update()`.

Taken: `SaveHarnessJob` takes the same `Step`/`Done` shape as the service's own
jobs, and `SaveProbeScreen` steps it once per frame and drains its lines. The H4
cycle moves ~110 MB across four passes; run in one frame that is a screen frozen
for ten-plus seconds, which is indistinguishable from a hang. It also exercises
the resumability the plan asks the jobs for.

### 3.10 The harness reads the AFR side, because `Platform/` may not

Step 2's done-condition includes printing the AFR title. `GameInfo::DetectAll`
lives in `Game/`, and `Platform/` calling `Game/` would invert the layering.

Taken: `SaveProbeScreen` (UI, which already calls `Game/` — `MenuScreen` does)
calls `DetectAll()` and passes the detected list and the configured
`BLOODBORNE TITLE ID` into the harness as plain strings. The harness prints both
and uses neither.

### 3.11 Backup directory naming stays in the harness

§4.1 defines `SaveBackups/<saveTitle>_<dir>_<stamp>_<reason>/`. `WorldStore`
owns that from milestone 2.

Taken: the harness builds its own names to that pattern (`_harness`, `_h4a`,
`_h4b`) and `BackupJob` takes a full destination path. No naming helper was
added to the service for a caller that does not exist yet.

### 3.12 A compile-time name for the container that is not there

Step 3 wants `ReadContainer` printed "for a container and for a missing one".

Taken: `kAbsentDirName = "BBRNOSUCHSAVE"` in the harness, mounted **read-only**.
Nothing is ever created under it — `CREATE2` is not passed anywhere in the app —
so unlike milestone 0's scratch directory it needs no guard.

### 3.13 `Platform/SaveData.cpp` does not include `Randomizer/FileIo.h`

§4's reuse table lists `FileIo`'s `CopyDirRecursive`, `MakeDirsRecursive` and
chunked read as reused as-is.

Taken: `SaveData.cpp` carries its own copies of the BSD flag table, the dirent
struct and the chunked copy, in the same shape and with the same comments
`Game/AfrManager.cpp`, `Game/GameInfo.cpp` and `Randomizer/FileIo.cpp` each
already carry their own. `Platform/` including `Randomizer/` would invert
CLAUDE.md §6's layering. `FileIo`'s own functions are also not quite right here:
`CopyDirRecursive` is best-effort and skips `*.bak`, and nothing in it can walk
a mount. Milestone 2's `WorldStore` lives in `Randomizer/` and can use `FileIo`
directly.

### 3.14 `worlds_verify.py` checks the source as well as the rules

§6 describes what the verifier asserts about manifests, refusals, recipes and
the journal. It does not say whether it reads the C++.

Taken: two sections. `selftest` is the manifest rules in Python — format, round
trip, and the four ways a copy disagrees. `source` reads `app/src/` and pins the
§3.1 invariants that are visible statically: no `CREATE2`, no
`sceSaveDataDelete`, `sce_sys` filtered out of the write list, the
`.partial`-then-rename order, `cond.key`/`cond.order` never set, no orbis call
outside `Platform/`, no `sceKernelStat`, and `-lSceSaveData` as the only
save-data library. CLAUDE.md §7 asks for intentional quirks to be documented as
invariants in the verification tools; these are the invariants whose breakage a
build would not show.

The source section strips `//` comments before matching, because this code
explains at length what it deliberately does not do and would otherwise match
its own explanations.

### 3.15 Two sentences shortened to fit the text column

Measured against the atlas at scale 3, two composed lines overran the
120→1680 px band: the harness's "more than one save directory" refusal (1,704 px)
and `DiscoverSaveTitle`'s multiple-title error (~1,670 px). The first now prints
the directory names on their own lines and a short verdict; the second ends
"CANNOT CHOOSE ONE". Worst line now 1,399 px.

---

## 4. Verification run

| Check | Command | Result |
| ----- | ------- | ------ |
| Clean rebuild | `cd app && make clean && make` | `IV0000-BBRD00001_00-BBRANDOMIZERMAIN.pkg` built, 7,143,424 bytes; `eboot.bin` 3,707,760 bytes |
| Compiler warnings | same, `-Wall` is on | none |
| Worlds mirror | `python app/tools/worlds_verify.py` | manifest rules 29/29, source invariants 33/33, "all checks passing" |
| Settings UI mirror | `python app/tools/settings_ui_verify.py` | 40/40 passing |
| Scroll geometry | `python app/tools/ui_scroll_verify.py` | all geometry and scroll properties PASSED |
| Pool / defaults mirror | `python app/tools/pool_verify.py selftest data/vanilla/dvdroot_ps4` | 88/88 passing |
| Harness string widths | `settings_ui_verify.load_atlas()` / `width()` at scale 3 | widest static body line 942 px and widest composed runtime line 1,399 px against the 120→1680 band; longest footer 1,223 px against 1,920 |

**Not run**, and why:

* **H4** — hardware. See §6. It is the whole point of this milestone and only
  the PS4 can run it.
* The output-parity suite (`boss_verify`, `treasure_verify`, `drops_verify`,
  `starting_weapons_verify`, `caged_dogs_verify`, `easy_modes_verify`,
  `hunter_tools_verify`, `mergo_darkness_verify`, `itemdata_verify`) — milestone
  1 changes no randomizer code, no settings and no tables, and the plan's
  milestone-1 verification line does not list them. `pool_verify.py`'s selftest
  was run anyway as the cheapest check that the defaults chain is untouched;
  the full suite is milestone 3's gate.
* `worlds_verify.py verify <dir>` against a real backup — the backups on disk
  were written by milestone 0's probe, which had no manifest and no `data/`
  subdirectory, so there is nothing yet for it to check. The first directory it
  can read is the one H4 produces.

---

## 5. What this does not prove

Everything about runtime. A clean cross-compile shows the calls type-check and
the binary links; `worlds_verify.py` shows the manifest rules are consistent and
that the source still says what it says. Neither shows that:

* `sceSaveDataMount` still succeeds from this build, or that the mount walks;
* a `BackupJob` driven one file per frame produces a byte-exact copy, rather than
  one that differs because the mount was held open across frames — milestone 0's
  probe copied inside a single `Update()`, and holding a mount over many frames
  is **new** in this milestone and is the single most likely place for this code
  to be wrong;
* `RestoreJob`'s empty-then-write sequence leaves a container the game will
  load — findings §5 proved the sequence by hand, not this implementation of it;
* the unmount after a read-write mount re-seals correctly when the job's phases
  are spread over frames;
* the account, title, directory and block refusals fire on a real mismatch. None
  of them can be provoked on the reference console without constructing one.

A clean build and a green mirror mean **ready for hardware testing**.

---

## 6. Hardware test handoff

Install the `.pkg` from `app/`. The harness is main menu → **SAVE DATA PROBE
(TEST)**, now titled `SAVE DATA HARNESS`.

**Before anything: confirm the verified byte-exact backup is still on the PC**
(`data/Save Backups/CUSA00207_SPRJ0005_20260921-032915`, 26 files, 26,949,914
bytes). `TRIANGLE` empties the live container and puts it back; that copy is the
recovery if it does not.

### Order

Each visit to the screen runs one operation. Leave with `O` and re-enter between
them.

1. **`X` — report.** Read-only. Nothing is written anywhere.
2. **`SQUARE` — back up and verify.** Writes only into
   `/data/bbrandomizer/SaveBackups/`.
3. **`TRIANGLE` — the H4 cycle.** Back up, empty the container, restore, back up
   again, compare. This is the destructive one.

Pull `live.log` afterwards — every line is written there as it is produced.

### What a pass looks like

| Step | Pass |
| ---- | ---- |
| `X` | `AFR TITLE (BLOODBORNE TITLE ID SETTING) CUSA03173` and the detected AFR titles, then `PLAYER`, `USER ID`, `ACCOUNT`; the sweep showing `CUSA00207 (AUSTRALIA) - 1 SAVE DIR(S)` and `NONE` for the other five; `SAVE DATA TITLE CUSA00207`, `SAVE DIRECTORY SPRJ0005`; 26 files listed with sizes, `26949914 BYTES, 1136 BLOCKS`; then the absent read printing `NO CONTAINER FOR CUSA00207 / BBRNOSUCHSAVE - MOUNT 0x…` |
| `SQUARE` | one `… BYTES COPIED` line per file, `BACKUP COMPLETE - 26 FILE(S), 26949914 BYTES`, an elapsed figure, then `VERIFY …` and `OK - 26 FILE(S), 26949914 BYTES, 1136 BLOCKS` |
| `TRIANGLE` | backup and verify as above; `EMPTYCONTAINER` showing `BEFORE 26 FILE(S)`, `REMOVED 22  REFUSED 0  LEFT ALONE 4`, `AFTER 4 FILE(S)` listing only the four `sce_sys` entries; `SOURCE VERIFIED`, `EMPTIED`, one `… BYTES WRITTEN` line per file, `RESTORE COMPLETE - 22 FILE(S)`; a second backup and verify; then `VERDICT MATCH - EVERY FILE AND EVERY BYTE OUTSIDE SCE_SYS` |

Both elapsed figures are wanted for the record — findings §4 has ~15 MB in
~1,009 ms from the single-frame probe, and one file per frame at 60 Hz puts a
floor of ~0.43 s on 26 files regardless of throughput.

`REFUSED 0` on the empty and `LEFT ALONE 4` are the two numbers that say
`sce_sys` survived and nothing else did.

### What a failure looks like

* **`X` reports more than one save directory.** The harness prints the names and
  stops. That is a plan stop condition — report it, do not pick one.
* **`X` reports save data under more than one title.** Same: `SAVE DATA UNDER n
  BLOODBORNE TITLES - CANNOT CHOOSE ONE`. Report it.
* **`SQUARE` ends `BACKUP FAILED …`.** A `.partial` directory is left under
  `SaveBackups/` and no backup. That is the designed outcome of an interrupted
  copy — confirm the `.partial` is there and that no directory without
  `.partial` was created for that stamp.
* **`VERIFY FAILED - count/path/size/total …`.** The copy and its manifest
  disagree. Report the whole line; it names which of the four checks failed.
* **`TRIANGLE` fails after `EMPTIED`.** The container has been emptied and not
  refilled. Recovery, in order: re-enter the screen and press `TRIANGLE` again
  (it will back up the emptied container, which is harmless, then restore from
  that backup — **not** what you want), so **instead** report the failure and
  restore from the PC copy through the console's own tooling. This is the case
  the PC backup exists for.
* **`VERDICT MISMATCH`.** The two backups differ; the line before it names the
  first file that does. This says the round trip is lossy and milestone 2 must
  not start.

### After the run

Record the elapsed figures and the empty's `REMOVED / REFUSED / LEFT ALONE`
counts. **Launch Bloodborne once** and confirm the save loads and the character
is intact — the restore's own verification compares sizes, and only the game can
say the bytes are the right ones.

---

## 7. Stop point

Milestone 1's completion gate: its eight steps are done, `worlds_verify.py`
passes, `make clean && make` produced the `.pkg`, and the mirrors that could
regress are green. **No stop condition fired.**

Milestone 1 is handed over for **H4**. Milestone 2 — the world store — must not
begin until H4 has run, and must not begin at all if the two backups do not
match or if the container cannot be refilled.

### Noticed, and deliberately not fixed

* **`/data/GoldHEN/AFR` is in two files, not one.** §3.1 says it appears in
  exactly one, `Game/AfrManager.cpp`; `Game/GameInfo.cpp:13` has its own
  `kAfrRoot` literal, and has since before this feature. `AfrManager` is
  milestone 2's file and this is milestone 2's or a later cleanup's to make
  true. Nothing in this milestone touched either.
* **`Randomizer/FileIo.cpp` calls `sceKernel*` from outside `Platform/`.**
  Pre-existing, and the reason `worlds_verify.py`'s layering check looks for
  `sceSaveData`/`sceUserService` specifically rather than every `sce*` symbol —
  a broader check would fail on day one for a reason this feature did not cause.
* **`ScreenId::SaveDataProbe` and its menu row still read "SAVE DATA PROBE
  (TEST)"** while the screen now says `SAVE DATA HARNESS`. Both are deleted in
  milestone 6 and `ScreenId` is a milestone 4 file; renaming a row on the way
  past is exactly what §3.2 rules out.
* **`RemoveTree` exists in `SaveData.cpp` but is private.** Milestone 2 step 4
  needs a `*.partial` startup sweep and will want it exposed. It is used here
  only to clear a stale `.partial` before a backup starts.

---
---

<!-- Below this line: milestone 0's report, preserved unchanged. -->

# Implementation Report — Worlds — milestone 0

**Status: AWAITING HARDWARE TEST**

**Plan:** `docs/features/worlds/plan.md` — milestone 0, "hardware probe"

**Spec:** `docs/features/worlds/spec.md`

**Implemented:** 2026-09-21

---

## 1. What was built

Milestone 0 is diagnostic. It extends the existing temporary scaffolding —
`Platform/SaveDataProbe.{h,cpp}` and `UI/SaveProbeScreen.{h,cpp}` — with a
second entry point, `ProbeSaveDataWrite()`, that runs the milestone's eight
steps in order and prints everything it learns to the screen and to `live.log`.
No production code was added: no `Platform/SaveData`, no `WorldStore`, no
`WorldActivation`, no world UI. `ScreenId`, the Makefile and `LIBS` are
unchanged.

The existing read-only probe (`X`) and backup probe (`SQUARE`) are untouched and
still behave exactly as before. The write probe is a third button, `TRIANGLE`.

| # | Planned change | Status | Where |
| - | -------------- | ------ | ----- |
| 1 | Compile-time scratch save-directory name and a guard aborting every write path, guard state printed | done | `app/src/Platform/SaveDataProbe.cpp:173-185` (constant), `:927-930` (name printed), `:1118-1132` (guard verdict), `:1137-1153` (`scratchAllowed`, re-checked at each of the three save-data write/mount call sites) |
| 2 | Identity: `GetInitialUser`, `GetForegroundUser`, `GetLoginUserIdList`, `GetUserName`, `GetNpAccountId`; `ACCOUNT_ID` from the live save's `param.sfo` via a raw-bytes sibling of `SfoString` | done | `app/src/Platform/SaveDataProbe.cpp:933-983` (identity), `:200-253` (`SfoRaw` / `SfoU64`), `:1158-1216` (live `param.sfo`, printed beside the NP account id with a MATCH/MISMATCH verdict) |
| 3 | `statvfs("/data")` and `statvfs("/user")`, every field raw | done — see §3 | `app/src/Platform/SaveDataProbe.cpp:1210-1256` |
| 4 | Rename a scratch directory with a file under `/data/bbrandomizer/`, confirm the file moved, remove it | done | `app/src/Platform/SaveDataProbe.cpp:1258-1315` |
| 5 | Mount the scratch directory `RDWR\|CREATE2` with `blocks` from the chosen backup's `param.sfo`; copy it in reporting each file's write result | done — backup chosen as in §3 | `app/src/Platform/SaveDataProbe.cpp:1316-1516` |
| 6 | Re-mount `RDONLY`, walk, print count, per-file sizes and total, compare against the source, print a verdict | done | `app/src/Platform/SaveDataProbe.cpp:1517-1620` |
| 7 | `sceSaveDataDelete` the scratch directory, re-search, print whether it is gone | done | `app/src/Platform/SaveDataProbe.cpp:1621-1664` |
| 8 | Time steps 5 and 6 in milliseconds | done | `app/src/Platform/SaveDataProbe.cpp:386` (`NowUs`), `:1509-1514`, `:1613-1618`, `:1665-1671` |

Supporting helpers added to the file's anonymous namespace, all shaped to the
traps `technical-findings.md` §6 records: `TreeWalk` sizes every file by reading
to a short read and recurses on `d_type == 4`; `ReadWholeFile` does the same for
a single file; `MakeParentDirs` creates `sce_sys/` inside the mount, since
`mkdir` does not make parents.

Invariants held:

* The live save directory is mounted exactly once, `RDONLY`, at
  `SaveDataProbe.cpp:1164-1180`. There is no other mount of it in the file.
* Every save-data write path names `kScratchDirName`, a compile-time constant,
  and calls `scratchAllowed()` immediately before firing. `SPRJ0005` is never
  typed anywhere.
* No privileged path. The write probe touches `/data/bbrandomizer`, save mounts,
  and nothing else. No `/system_data/`, no libjbc, no sandbox escape.
* `LIBS` gained nothing; `app/Makefile` is unchanged.
* Layering: all orbis calls stay in `Platform/`; `SaveProbeScreen` calls only
  `ProbeSaveDataWrite()`.

---

## 2. Deviations from the plan

All six are in the plan's §10. Summarised:

1. **Step order.** The steps run 1a (print the scratch name) → 2 (identity,
   then the title sweep) → 1b (guard verdict) → 2c (live `ACCOUNT_ID`) → 3 → 4 →
   5 → 6 → 7 → 8. The guard compares the scratch name against the directories
   the search returned, so its verdict cannot be printed before that search. Its
   done-condition — "the guard's state prints in the probe output" — is met;
   only the position of the line moved. Every block is labelled with its plan
   step number so the output still reads in the plan's terms.

2. **A second entry point on a third button.** The plan's §7 lists the probe's
   steps but not how the developer starts them. `ProbeSaveDataWrite()` is bound
   to `TRIANGLE`; `X` and `SQUARE` keep their existing meanings, which matters
   because `SQUARE` is what produces the backup step 5 restores from.

3. **Step 3 prints named fields plus a hex dump, into an oversized buffer.** See
   §3 below.

4. **Step 5's backup selection rule.** See §3 below.

5. **`SaveDataProbe.h`'s delete list updated** to say `-lSceSaveData` stays, per
   §7 milestone 6 step 6. Comment only.

6. **The `PARAMS` `CUSA#####` scan was not implemented.** It is the §3.3 hazard
   row for save-title discovery, and discovery is milestone 1 step 2. The piece
   milestone 1 needs from milestone 0 — a param.sfo reader that handles format
   `0x0004` — is in place as `SfoRaw`.

---

## 3. Decisions the plan left open

### 3.1 Which user id the probe scopes its searches to

Step 2 says to call all five identity functions and print them; it does not say
which id the search and mount below should use. §4.4 refuses when there is no
foreground user, but that is milestone 1's production rule and refusing here
would make the probe unrunnable on a console where the call happens to fail —
which is one of the things the probe exists to find out.

Taken: use the foreground user when `sceUserServiceGetForegroundUser` succeeds
and returns a valid id; otherwise fall back to the initial user and print
`(INITIAL - FOREGROUND UNAVAILABLE)`; abort only when neither is usable. The
chosen id is printed on its own line.

### 3.2 Step 3's "every field printed raw"

Taken: all eleven named `struct statvfs` fields, then a 128-byte dump of the
buffer as sixteen hex words, into a **1 KB zeroed arena** cast to
`struct statvfs*` rather than a bare local struct.

Why: the named fields are musl's Linux layout, which is exactly what is in
doubt. The hex words are what actually landed in memory and are the thing a
judgement can be made from. The oversized arena exists because if the kernel
writes a struct larger than musl's 112 bytes — FreeBSD's `struct statfs` is 488
— a bare local would be a stack smash rather than a result; with the arena it
shows up in the dump.

Nothing is interpreted. The probe prints and moves on.

### 3.3 Which local backup step 5 restores from

The plan says "the chosen local backup's `param.sfo`" without saying how it is
chosen.

Taken: among the subdirectories of `/data/bbrandomizer/SaveBackups` whose name
begins with `<live save title id>_`, use the lexicographically greatest. Backup
names are `<title>_<dir>_%Y%m%d-%H%M%S`, so greatest is newest, and the title
prefix keeps the restore in the same save container. The candidate count and the
chosen name are both printed. With no candidate, steps 5–7 are skipped with a
line telling the developer to press `SQUARE` first.

### 3.4 A leftover scratch directory trips the guard

The guard is specified as aborting "if it equals a discovered live directory
name", and §E5.1 as "if that constant ever equals a directory the search
returned". A scratch directory left behind by a failed delete *is* a directory
the search returns, so on the next run the guard trips and every write step
refuses.

Taken: implement it literally and leave it tripped. The guard cannot tell a
leftover from a collision and guessing is not what a guard is for. The output
names the match and points at Settings → Application Saved Data Management,
which the plan's own gate names as the recovery.

### 3.5 Selecting the live save without indexing

§3.1 forbids selecting a search result by index. The probe accepts a live
directory only when the first title with any hits has **exactly one**; anything
else prints every name found, sets a flag, and skips steps 5–7 (that is the
plan's "more than one save directory" stop condition, surfaced as probe output
rather than a crash). Step 7's re-search scans the returned names for
`kScratchDirName` and for the live name by string comparison. `cond.key` and
`cond.order` are left at their defaults, and nothing reads meaning from a
position.

---

## 4. Verification run

| Check | Command | Result |
| ----- | ------- | ------ |
| Clean rebuild | `cd app && make clean && make` | `IV0000-BBRD00001_00-BBRANDOMIZERMAIN.pkg` built, 7,143,424 bytes; `eboot.bin` 3,708,752 bytes |
| Compiler warnings | changed objects deleted and rebuilt | none (`-Wall` is on) |
| Settings UI mirror | `python app/tools/settings_ui_verify.py` | 40/40 passing |
| Scroll geometry | `python app/tools/ui_scroll_verify.py` | all geometry and scroll properties PASSED |
| Pool / defaults mirror | `python app/tools/pool_verify.py selftest data/vanilla/dvdroot_ps4` | 88/88 passing |
| New screen strings | `settings_ui_verify.load_atlas()` / `width()` at scale 3 | widest new body line 946 px against the 120→1680 band; footer 838 px against 1920; every glyph present in the atlas |

**Not run**, and why:

* `worlds_verify.py` — does not exist yet; it is milestone 1 step 7.
* The output-parity suite (`boss_verify`, `treasure_verify`, `drops_verify`,
  `starting_weapons_verify`, `caged_dogs_verify`, `easy_modes_verify`,
  `hunter_tools_verify`, `mergo_darkness_verify`, `itemdata_verify`) — milestone
  0 changes no randomizer code, no settings and no tables. `pool_verify.py`'s
  selftest was run anyway as the cheapest check that the defaults chain is
  untouched.
* H1, H2, H3 — hardware. See §6.

---

## 5. What this does not prove

Everything this milestone exists to answer. A clean cross-compile shows the
calls type-check and the binary links; it says nothing about whether
`RDWR|CREATE2` mounts, whether `sceSaveDataDelete` returns 0, whether
`sceUserServiceGetForegroundUser` returns what `technical-findings.md` §1.2
expects, or whether `statvfs` produces a usable number. Those are exactly the
unknowns in findings §7 and only the PS4 can close them.

One thing **was** established statically, and the developer should know it
before running the probe:

> `statvfs` in this toolchain's `libc.a` cannot fill the caller's struct.
> Disassembling `statvfs.lo` shows the body allocating a 0x928-byte local,
> loading `%rsi` with `%rsp` — discarding the caller's buffer pointer — calling
> `statfs`, and returning `eax >> 31`. There is no copy-out step. So step 3's
> named fields and hex dump will almost certainly read as zeros, `statvfs`'s
> return code will be the only signal, and §4.4's "if milestone 0 shows its
> fields are sane" branch looks already decided against.

That is a prediction from static reading, not a result, and the probe still runs
the call — a prediction that the hardware contradicts would itself be worth
knowing. It is **not** treated as the plan's `statvfs`-unusable stop condition,
because that condition requires the fallback capacity probe to have failed too,
and the fallback is milestone 1's work. `libc.a` also exports `statfs`, which
*does* fill a caller-supplied buffer with the kernel's FreeBSD `struct statfs`;
whether §4.4 should use it instead of the write-a-file fallback is a plan
question, not an implementation one, and was deliberately left alone.

---

## 6. Hardware test handoff

Install the `.pkg` from `app/`. The probe is main menu → **SAVE DATA PROBE
(TEST)**.

**Before anything: confirm you still have the verified byte-exact backup on the
PC** (`data/Save Backups/CUSA00207_SPRJ0005_20260921-032915`, 26 files,
26,949,914 bytes). Nothing below should need it. It is the reason this is
runnable at all.

### Order

1. **Press `SQUARE` first** — the existing read-only probe plus backup. This
   writes `/data/bbrandomizer/SaveBackups/CUSA00207_SPRJ0005_<stamp>/` and is
   what the write probe restores from. Confirm it ends
   `BACKUP COMPLETE - 0 ERROR(S)`. If you already have a backup on the console
   from the earlier session you can skip this, but a fresh one costs a minute.
2. Leave the screen (`O`) and re-enter it — each visit runs one probe.
3. **Press `TRIANGLE`.** The screen will freeze for as long as steps 5 and 6
   take; it copies ~27 MB in and reads ~27 MB back inside one frame. `live.log`
   is written line by line throughout, so a hang or a crash still leaves the
   trail.
4. Scroll with Up/Down and read the whole output, or pull `live.log`.

### What to look for, step by step

| Step | Pass looks like |
| ---- | --------------- |
| 1 | `SCRATCH SAVE DIR BBRRESTORETEST`, and later `ARMED - BBRRESTORETEST MATCHES NONE OF n DISCOVERED DIR(S)` |
| 2 | `GETFOREGROUNDUSER 0x00000000` with a valid id; `GETNPACCOUNTID` non-zero; `PARAM.SFO ACCOUNT_ID` and `GETNPACCOUNTID` printing the same 64-bit value, then `MATCH - NP ACCOUNT ID IS USABLE FOR OWNERSHIP` (this settles **P14**) |
| 3 | Numbers print, sane or not. Per §5 expect zeros; record them verbatim either way |
| 4 | `SCEKERNELRENAME 0x00000000`, `FILE MOVED WITH THE DIRECTORY - CONTENT MATCHES`, `OLD PATH GONE`, then a clean cleanup line |
| 5 | `MOUNTED AT /savedataN`, then one `... BYTES WRITTEN OK` line per file, `WROTE 26 OF 26 FILE(S)`, `DEST OPEN REFUSED 0, FAILED 0`, `UNMOUNT 0x00000000` (this settles **P15**) |
| 6 | `RE-MOUNTED AT /savedataN`, 26 files walked, `26949914 BYTES`, and `VERDICT MATCH - EVERY FILE AND EVERY BYTE ACCOUNTED FOR`. `MOUNT REPORTS 1136 BLOCKS (36352 KB)` reconciles the allocation |
| 7 | `SCESAVEDATADELETE ... 0x00000000`, `SCRATCH DIRECTORY GONE`, `LIVE DIRECTORY SPRJ0005 STILL PRESENT` |
| 8 | Two millisecond figures |

### What failure looks like — the two that halt the feature

**Step 5 fails.** Any of:

* `MOUNT FAILED 0x........ - REQUIRED BLOCKS n` — `RDWR|CREATE2` is refused for
  a cross-title container, or the container size read from the backup is too
  small. The `REQUIRED BLOCKS` figure is the useful part.
* `DEST OPEN REFUSED` on some or all files — the mount is writable but the
  service rejects the write. The *pattern* is the finding: if `sce_sys/*` is
  refused and `userdata*` accepted, P15's fallback ("restore only the files the
  game wrote plus `sce_sys/param.sfo`") is the answer; if everything is refused,
  restore does not work from homebrew.
* Files report `n OF m BYTES THEN ...` — a partial write.

**Step 7 fails.** `SCESAVEDATADELETE ... 0x........` non-zero, or
`SCRATCH DIRECTORY STILL PRESENT` after it.

In either case **stop — do not start milestone 1.** The plan's gate makes both
of these hard stops: the design depends on restore and delete working. Report
the error code.

### If the scratch directory survives

Remove `BBRRESTORETEST` under Bloodborne in **Settings → Application Saved Data
Management → Saved Data in System Storage → Delete**. It is not the game's save;
Bloodborne uses `SPRJ0005`, which the probe never names. Leaving it in place
also means the guard will trip on the next run and the write steps will refuse —
that is intended, not a bug.

### If the guard trips on a first run

`TRIPPED - BBRRESTORETEST MATCHES A DISCOVERED DIRECTORY` on a console that has
never run the write probe would mean the constant collides with a real
directory. That is a plan stop condition. Report it and do not change the
constant to route around it.

### After the run

Record the output in `technical-findings.md` §7 per the plan's gate. It settles
**P14** (account scoping, from step 2) and **P15** (which files a restore
writes, from step 5's per-file results).

---

## 7. Stop point

Milestone 0's completion gate: the eight steps are implemented, the clean
rebuild produced the `.pkg`, and the mirrors that could regress are green. No
stop condition fired during implementation.

Milestone 0 is handed over for **H1, H2 and H3**. Milestone 1 — `Platform/SaveData`
— must not begin until those have run, and must not begin at all if step 5 or
step 7 fails.
