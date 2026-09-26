# Hardware Test Plan — Worlds

**Status: EXECUTED — PASSED IN FULL** — written 2026-09-24, run by the developer
2026-09-25 on account `acct-4a17f1993020e604`, save title `CUSA00207`. Every part
passed. The result sheet at the end is filled in; the log entry for 2026-09-25 in
`log.md` records what was checked independently and three things the run
established that the plan did not predict.

**Covers:** the whole worlds feature, milestones 1–6. This consolidates plan §6's
H4–H11 and the per-milestone handoffs in `implementation-report.md` into one
ordered run.

**Package:** `app/IV0000-BBRD00001_00-BBRANDOMIZERMAIN.pkg` (7,143,424 bytes,
built 2026-09-24).

**Written before any of this had run on a PS4.** Milestones 1–6 were all built
before any hardware test, at the developer's direction, so six milestones'
changes sat under every step below and a failure would not have been narrowed the
way one-milestone-at-a-time testing narrows it. In the event, nothing failed.

---

## How this is ordered, and why

Each part is safe before the next one is attempted. Three rules set the order:

1. **Everything that writes nothing comes first.** Parts B–F never touch your
   save. If the UI is broken you find out before anything is at risk.
2. **First-run capture cannot be deferred.** The app runs it automatically when
   the account has no worlds folder, so it happens on first launch whether or
   not you are ready. It is Part A for that reason, not by choice.
3. **The destructive tests are ordered by how much they can cost.** A normal
   activation, then `START FRESH`, then Vanilla, then a deliberate power cut.

Do not skip ahead. In particular, **do not run Part I (Vanilla) before Part H
(`START FRESH`)** — Vanilla is the leg that used to leave the console unable to
activate anything again, and while P26 fixed that, Part I is where you confirm
the fix rather than where you rely on it.

---

## Part 0 — Before you touch the console

### 0.1 Take a manual FTP copy of your save. This is not optional.

```
/user/home/<user id>/savedata/CUSA00207/     (the SAVE title, not the AFR one)
```

Copy the whole directory to your PC. The app cannot do this for you, and every
recovery path below assumes you have it.

**This path is not reachable over plain FTP**, as the 2026-09-25 run found — use
the PS4Explorer homebrew app to copy it somewhere FTP can see first.

**Note the title id carefully.** The save title and the `BLOODBORNE TITLE ID`
setting are independent (D25/P23) and on the reference console they genuinely
differ: AFR is `CUSA03173`, the save lives under `CUSA00207` in directory
`SPRJ0005`. Look for the directory that exists rather than assuming it matches
the setting.

### 0.2 Take a copy of the randomizer's data directory too

```
/data/bbrandomizer/
```

What tells you afterwards whether the app wrote what it said it wrote. Not small
— it was 490 MB by the 2026-09-25 run, having accumulated across the whole
build.

### 0.3 Confirm the preconditions over FTP

| Check | Path | If it is missing |
| ----- | ---- | ---------------- |
| Vanilla source tree | `/data/bbrandomizer/VanillaSource/dvdroot_ps4/` | Activation refuses at phase 1. Put it in place first. |
| AFR root is writable | `/data/GoldHEN/AFR/` | Activation refuses. Check GoldHEN's AFR plugin is loaded. |
| No leftover journal | `/data/bbrandomizer/activation.journal` should **not** exist | If it does, a previous run was interrupted. Note it — Part A will reconcile it and that is worth watching. |
| No worlds folder yet | `/data/bbrandomizer/Worlds/` should **not** exist, for a clean run of Part A | If it does exist from an earlier build, move it aside rather than deleting it. |

### 0.4 Note what your live save looks like

Write down the **file count** and **total bytes** of the savedata directory you
copied in 0.1, ignoring `sce_sys`. The save files are encrypted and opaque, so
count and size is all you get — which is exactly what the manifest records, so it
is enough. You will compare against these numbers
repeatedly, and it is far easier to have them written down than to re-derive
them at 2am.

### 0.5 Install

Install the `.pkg` as in `docs/build.md`. **Do not launch it yet** — read Part A
first, because the first launch does something on its own.

---

## Part A — First launch and first-run capture (H5)

**This happens automatically.** On an account with no worlds folder, the app
begins capturing before you press anything. There is no way to open this build,
look around, and capture later.

### Do

1. Launch the app.
2. Read the startup log before pressing anything.

### Expect

* The app opens on a **startup log**, not a menu. The old main menu is gone.
* `FIRST RUN - CAPTURING THE LIVE SAVE INTO VANILLA`
* `VANILLA CREATED`
* A captured file count and a backup path.
* It waits for `X`. Press it only after you have read the whole log.
* The `WORLDS` tab then appears, with `+ NEW WORLD` and `VANILLA` in the rail.

### Then check over FTP

| Check | Where |
| ----- | ----- |
| The account directory exists | `/data/bbrandomizer/Worlds/acct-<16 hex>/` |
| Vanilla exists with a stored save | `…/acct-…/vanilla/world.cfg` and `…/vanilla/save/manifest.txt` + `save/data/` |
| The manifest agrees with reality | `manifest.txt`'s `files` and `bytes` match the count and total you noted in 0.4 |
| A safety backup was taken | `/data/bbrandomizer/SaveBackups/…_firstrun/` (or similarly suffixed), holding `manifest.txt` and `data/` |
| **Your live save is untouched** | Compare the savedata directory against your 0.1 copy. Nothing in this step should have changed it. |

### Failure looks like

* Any `FAILED - …` line.
* A `.partial` directory left in `SaveBackups/`.
* The live save changed in any way.
* A freeze of more than a few seconds — that is the container probe. Note how
  long, and pull `live.log`; the `SAVE DATA … BLOCKS` line gives the timing.
* A black screen or crash at launch. Pull `live.log`; the last `worlds:` line
  says how far it got.

### If the console had no save at all

Then there was nothing to capture. Expect Vanilla to be created with **no**
`save/` directory and no safety backup, and no error. That is D26 behaving
correctly, not a failure.

---

## Part B — Tabs and navigation (H10, first half)

Nothing here writes anything.

### Do and expect

1. **Tabs.** On the `WORLDS` rail press Left, then Right. Each switches to
   `DEFAULTS` and back. Do it from the `DEFAULTS` rail too.
2. **Left/Right means two things.** On `DEFAULTS`, `X` into a category, move to a
   toggle, press Left/Right — it must **change the value**, not switch tabs. `O`
   back to the rail, press Left — now it switches tabs.
3. **The rail.** Up/Down over `+ NEW WORLD` and `VANILLA`. Both are always
   present.
4. **The details pane.** On `VANILLA`, the middle column should show its save
   size and when it was last played.
5. **The help column.** It changes with the row — `+ NEW WORLD` and `VANILLA`
   each have their own text.
6. **The active marker.** With nothing activated yet, **no row** should be marked
   `ACTIVE`. The header should say `ACTIVE  FIRST RUN` or `ACTIVE  UNMANAGED`
   instead of marking a row.
7. **The footer.** On `DEFAULTS` it reads `LEFT RIGHT CHANGE` inside a pane and
   `LEFT RIGHT TABS` on the rail.
8. **`O` on the `WORLDS` rail** exits the app. (There is no menu behind it any
   more — that is expected.)

### Failure looks like

* Left/Right changes a value while on the rail, or switches tabs while in a pane.
* Two rows marked `ACTIVE`, or a row marked while the header names a state.
* Text clipped, two things overlapping, anything running off an edge — photograph
  it, the geometry verifiers say this cannot happen.
* Switching tabs moves the columns or the footer. Only the content should change.

---

## Part C — Create a world (H10, second half)

Still writes nothing outside `/data/bbrandomizer/Worlds/`. Your save is not
involved.

### Do and expect

1. `X` on `+ NEW WORLD`. The editor opens, titled `WORLD EDITOR`. The rail reads
   `NAME`, `SEED`, a rule, `ENEMIES` … `WORLD`, `SAVE`, a rule, `HISTORY`.
2. `X` on `NAME`. Sixteen slots with a bar under the cursor. Left/Right moves,
   Up/Down changes the character, over `A–Z`, `0–9` and space. `X` accepts.
   Name it something you will recognise — `ALPHA`.
3. Move to `SAVE`, `X` into the pane. One row: `SAVE DATA   KEEP EXISTING`.
   * Left/Right flips it to `START FRESH` and back.
   * **`X` must do nothing on it.** This is the one that matters — it is a new
     kind of setting and `X` opens a picker for the other kinds.
   * Leave it on `KEEP EXISTING`.
4. **Check `SAVE` is not on the `DEFAULTS` tab.** Back out to `WORLDS`, switch to
   `DEFAULTS`, and confirm its category list has no `SAVE`. The two lists are
   deliberately different.
5. Back in the editor, turn on `RANDOMIZE ENEMIES` and at least one more setting.
   With everything off the tree is a plain mirror and later steps have nothing to
   look at.
6. Press `OPTIONS`. Confirm opens; its state line reads `CREATED` followed by the
   world id.
7. `O` back, `O` again to the rail. `ALPHA` is listed.

### Failure looks like

* `X` on `SAVE DATA` changes it — `IsDrillIn` is wrong for the new kind.
* `SAVE` appears on the `DEFAULTS` tab — the two category lists have converged.
* A rail row sits under another, or `HISTORY` is off the bottom — the editor's
  70px grid is wrong. Photograph it.
* A new world opens on `START FRESH`. It must always open on `KEEP EXISTING`,
  regardless of what `defaults.cfg` holds.

---

## Part D — Revisions and history (H10)

This is where the append-only rule is proved. Nothing destructive.

### Do and expect

1. **A rename appends nothing.** `X` on `ALPHA`, `X` on `NAME`, change it, `X`,
   `OPTIONS`. The state line reads `RENAMED - NO NEW REVISION`. `O`, then `X` on
   `HISTORY`: **one** row, `REVISION 1` with the seed and `CREATED`.
2. **Nothing changed appends nothing.** `OPTIONS` again without touching
   anything: `NO CHANGES - STILL` followed by the revision id.
3. **A settings change appends one.** Flip a setting, `OPTIONS`. State line
   `APPENDED` with the new revision id. `HISTORY` now has **two** rows, newest
   first, the top one reading `REVISION 2`, its seed, and `1 CHANGED`.
4. **History grows, it never rewinds.** In `HISTORY`, select `REVISION 1` with
   `X`. Back on the editor the `NAME` pane says `REV-0001 IS NOW REV-0003`, and
   the settings show revision 1's values. Re-open `HISTORY`: **three** rows.

### Then check over FTP

`/data/bbrandomizer/Worlds/acct-…/w-0001/` holds `world.cfg`, `rev-0001.cfg`,
`rev-0002.cfg` and `rev-0003.cfg`. No revision file was rewritten — compare
`rev-0001.cfg`'s timestamp against the others.

### Failure looks like

* **`HISTORY` shrinks after selecting an old revision.** This is the one thing
  D2 forbids outright. Stop and report it.
* A rename appends a revision, or a settings change does not.
* The editor opens an existing world showing the Defaults settings rather than
  its own — check `live.log` for `opened w-000N on rev-000M`.

---

## Part E — Delete keeps the save (H10)

### Do and expect

1. Create a throwaway second world, `SPARE`, so you are not deleting `ALPHA`.
2. On the `WORLDS` rail, `TRIANGLE` on `SPARE`. The confirmation names it and
   says `ITS SAVE DATA IS KEPT AS A SAFETY BACKUP AND IS NOT DELETED`.
3. `O` cancels. Nothing changes.
4. `TRIANGLE` again, then `X`. It reports `DELETED SPARE` and stays to say so.
   `O` returns, and `SPARE` is off the rail.
5. **`TRIANGLE` on `VANILLA`** must refuse: `VANILLA IS ALWAYS PRESENT AND CANNOT
   BE DELETED`.

### Then check over FTP

The `w-000N` folder for `SPARE` is gone from `Worlds/acct-…/`, and if it had a
stored save, that save is now under `SaveBackups/`.

### Deferred to Part G

**`TRIANGLE` on the active world** must also refuse — but no row is `ACTIVE`
yet, so that half cannot be exercised here. It is revisited after the first
activation. Do not mark it passed now.

---

## Part F — Read a confirmation without acting on it

The last step that writes nothing. Phase 1 runs on Confirm's first frame, so
this exercises every refusal check without committing to anything.

### Do

`X` on `ALPHA`, then `OPTIONS` to open Confirm.

### Expect

* `CONFIRM ACTIVATION` as the heading, and briefly `CHECKING` while phase 1 runs.
* `DEACTIVATING VANILLA`
* `OUTGOING SAVE   BACKED UP AND FILED INTO IT`
* `ACTIVATING ALPHA`
* `INCOMING SAVE   ADOPT THE LIVE SAVE` — because `ALPHA` has no stored save yet.
  This is the phase-6 row that applies, stated in the player's words.
* `HOW LONG   ABOUT A MINUTE` (or `UNDER A MINUTE` / `A FEW SECONDS` — the
  estimate is coarse by design).
* Footer `OPTIONS ACTIVATE   O BACK`.

### Then press `O`. Do not activate yet.

### Failure looks like

* A `CANNOT ACTIVATE` state line. Read the sentence beneath it — that is the
  §4.4 row that fired, and **nothing was written**. The likely ones at this point
  are a missing `VanillaSource`, an unwritable AFR root, or
  `NO BLOODBORNE SAVE DATA FOR THIS PLAYER - RUN BLOODBORNE ONCE FIRST`.
* The confirmation naming the wrong outgoing world, or claiming `ALPHA` has a
  stored save when it does not.

---

## Part G — First activation (H6, first half; H4 by proxy)

**From here on, things are written.** Your 0.1 FTP copy is the recovery path.

### Do

`OPTIONS` on Confirm.

### Expect, in the progress log

```
CHECKING
ACTIVATING ALPHA
BACKING UP THE LIVE SAVE
SAFETY BACKUP VERIFIED - …
FILING THE BACKUP INTO VANILLA
GENERATING WITH SEED …
GENERATED n MAP(S)
RANDOMIZED …                (one line per feature that is on)
THE NEW TREE IS LIVE
```

### Then check over FTP — this is where H4's coverage now lives

| Check | Where |
| ----- | ----- |
| A fresh AFR tree | `/data/GoldHEN/AFR/<title>/dvdroot_ps4/` |
| Stamped with this world | `…/dvdroot_ps4/.bbrandomizer_manifest` names `ALPHA` and its revision |
| Nothing left behind | No `dvdroot_ps4.staging`, no `dvdroot_ps4.old`, no `/data/bbrandomizer/activation.journal` |
| The safety backup verifies | `SaveBackups/…_preactivate/manifest.txt`'s file count and total match the `data/` beside it |
| **Vanilla now holds your original save** | `Worlds/acct-…/vanilla/save/` — its manifest should match the numbers from 0.4 |
| No `.partial` anywhere | `SaveBackups/` |

Compare `Worlds/acct-…/vanilla/save/data/` against your 0.1 copy **file for file
and byte for byte**. This is the closest you can now get to H4's round-trip
check — see "Coverage this plan cannot reach" at the end.

### Then launch Bloodborne

Your save is there, your progress is intact, and the enemies are randomized.
**Play far enough to be recognisable** — kill something distinctive, move to a
different lamp. You need to be able to tell this playthrough from another one
later.

### Back in the app

* `ALPHA` is now marked `ACTIVE` on the rail, and it is the only row that is.
* **`TRIANGLE` on `ALPHA` now refuses**, saying it is active. This is the half of
  Part E that could not be run then. Mark Part E complete only now.

### Failure looks like

* Any `FAILED - …` line. **Stop.** Do not activate anything else. Keep
  `/data/bbrandomizer/SaveBackups/` — the `_preactivate` backup taken at the
  start of this activation is the recovery path, and nothing prunes it.
* A `dvdroot_ps4.old` or `.staging` left on disk.
* The game launching unmodified — the AFR tree is there but not being used. Check
  the title id in `DEFAULTS` against the folder name under `/data/GoldHEN/AFR/`.

---

## Part H — A second world, and the round trip (H6)

This is the feature's central claim: two playthroughs, kept apart.

### Do

1. Create `BETA` with a **different seed** and visibly different settings.
2. Activate it. In Confirm, expect `DEACTIVATING ALPHA`,
   `OUTGOING SAVE   BACKED UP AND FILED INTO IT`, and
   `INCOMING SAVE   ADOPT THE LIVE SAVE` — `BETA` has no stored save either, so
   it adopts what is live. **Note that this means `BETA` starts from `ALPHA`'s
   progress.** That is `KEEP EXISTING` behaving as specified, not a bug.
3. Launch Bloodborne. Play a little, distinguishably.
4. Back in the app, activate `ALPHA` again. Now Confirm should read
   `INCOMING SAVE` as **restoring `ALPHA`'s own stored save**, not adopting the
   live one — `ALPHA` has a stored save this time.
5. Launch Bloodborne.

### Expect

**`ALPHA`'s progress is exactly where you left it in Part G**, and the
randomization is `ALPHA`'s, not `BETA`'s. The play you did as `BETA` is not
visible, and it is not lost — it is filed into `BETA`.

### Then check over FTP

The container holds exactly `ALPHA`'s file set. **None of `BETA`'s surplus
`backup*` files are still there.** That is the aliasing D4 forbids and the
reason phase 6 empties before it writes.

### Failure looks like

* `ALPHA`'s progress is `BETA`'s, or a mixture. Stop — this is the silent
  incoherence §7.1 exists to prevent, and it is the most serious failure this
  plan can find.
* Files in the container from the wrong playthrough.
* `LAST PLAYED` on the rail not reordering the worlds.

---

## Part I — `START FRESH` (H7)

Destructive to the live container by design, and the outgoing save is backed up
first.

### Do

1. Create `GAMMA`.
2. In its editor, `SAVE` → `SAVE DATA` → `START FRESH`.
3. `OPTIONS`. Confirm should state
   `INCOMING SAVE   THE SAVE IS BACKED UP AND THE GAME STARTS A NEW PLAYTHROUGH`.
4. Activate.

### Expect

* `BACKING UP THE LIVE SAVE`, `SAFETY BACKUP VERIFIED`, the backup filed into
  `ALPHA`.
* `EMPTYING THE SAVE CONTAINER`, with a line saying how many files were removed
  and that four were left alone.
* `SAVE DATA IS BACK TO KEEP EXISTING - rev-NNNN`.

### Then check

| Check | Where |
| ----- | ----- |
| The container holds `sce_sys` **only** | `/user/home/<id>/savedata/CUSA…/` — four entries, nothing else |
| The container still exists | It must never be deleted, only emptied. The app cannot recreate one. |
| `ALPHA` kept its progress | `Worlds/acct-…/w-0001/save/` |
| The setting reverted | Re-open `GAMMA`: `SAVE DATA` reads `KEEP EXISTING`, and `HISTORY` has the extra appended revision |

### Then launch Bloodborne

It offers a **new playthrough**. Create a character and play briefly.

### Failure looks like

* The container gone entirely rather than emptied. Serious — the app cannot make
  one, so this is unrecoverable without the game.
* `sce_sys` disturbed.
* `GAMMA` still reading `START FRESH` when re-opened. It must never persist; a
  stored `START FRESH` discards progress on every future activation.

---

## Part J — Vanilla, and back again (H11, and the P26 fix)

**The most interesting step in this plan.** Before P26 this was a one-way door:
activating Vanilla deleted the AFR tree, and every later activation refused with
`BLOODBORNE TITLE ID … WAS NOT DETECTED` because the check read the AFR overlay
as evidence the game was installed. That check is gone. This is where you find
out whether removing it was right.

### Do

1. `X` on `VANILLA` from the rail. The same confirmation opens, with `SEED NONE`
   and no settings rows.
2. Activate it.

### Expect

* The fresh `GAMMA` save backed up and filed into `GAMMA`.
* `VANILLA - NOTHING IS GENERATED`
* `ACTIVATING VANILLA - THE RANDOMIZER FILES ARE REMOVED`
* Vanilla's save restored.

### Then check

* `/data/GoldHEN/AFR/<title>/dvdroot_ps4/` is **gone**.
* The container holds exactly Vanilla's file set — the one from 0.1.
* Launch Bloodborne: your original save, unmodified game.

### Then — the actual test

**Activate `ALPHA` again.** It must run. Before P26 it refused here.

* Launch Bloodborne: `ALPHA`'s progress and `ALPHA`'s randomization, both back.

### Failure looks like

* `ALPHA`'s activation refusing with anything about a title not being detected.
  That would mean P26 was not fully removed; report the exact sentence.
* The AFR tree surviving the Vanilla activation.
* Vanilla's save not coming back — check `SaveBackups/` for the `_preactivate`
  backup taken at the start of this activation.

---

## Part K — Interrupted activation (H8)

Deliberately breaking things. Do this only once everything above has passed.

### Do

1. Start an activation of any world.
2. **Pull the power** partway through. The two interesting windows are during
   `GENERATING WITH SEED …` (phase 4) and immediately after
   `THE NEW TREE IS LIVE` (phase 6).
3. Power back on and launch the app.

### Expect, in the startup log

* The partial sweep, then a line naming the journal and the phase it reached:
  `AN ACTIVATION WAS INTERRUPTED AT PHASE n`.
* One of: `THE STAGED TREE WAS DISCARDED - NOTHING ELSE CHANGED`,
  `THE FILE SWAP DID NOT COMPLETE - DOING IT AGAIN`,
  `THE FILE SWAP HAD ALREADY COMPLETED`, `RESUMING THE SAVE SWAP`, or
  `THE INTERRUPTED ACTIVATION WAS FINISHED`.
* No `.partial` directory left in `SaveBackups/`.
* No `activation.journal` left.
* **No save lost.** Whichever world ends up active, its save is coherent with its
  randomization.

### Worth doing twice

Once during phase 4 and once after phase 6 — they take different reconciliation
paths, and the phase-5 rule was rewritten during implementation precisely because
the plan's original version was wrong in two of the four ways it can be
interrupted.

### Failure looks like

* `THE INTERRUPTED ACTIVATION COULD NOT BE FINISHED`. Read the rest of the log —
  it should then have put the safety backup back and said so.
* A `.partial` surviving the sweep.
* A save that does not match the active world's randomization.

---

## Part L — Refusals and account scoping (H9, partial)

### L1 — A second account sees nothing of the first's

1. Sign in as a second PS4 account.
2. Launch the app.
3. **Expect** first-run capture again, for that account, and a `WORLDS` rail
   holding only `+ NEW WORLD` and `VANILLA`. None of `ALPHA`, `BETA` or `GAMMA`.
4. Over FTP: a second `Worlds/acct-<different hex>/` directory, and the first
   one untouched.
5. **Failure looks like** the second account seeing the first's worlds at all.

### L2 — A world with a stored save on a console with no Bloodborne save

Only testable if you have a console, or a title id, where Bloodborne has never
run. Set `BLOODBORNE TITLE ID` in `DEFAULTS` to a title with no save data and try
to activate a world that has a stored save.

**Expect** `NO BLOODBORNE SAVE DATA FOR THIS PLAYER - RUN BLOODBORNE ONCE FIRST`,
and **nothing written**.

### L3 — What you can no longer test

The refusal table has ten rows. Milestone 3 built a harness that could force each
one synthetically and print whether it fired correctly; milestone 6 deleted that
harness, and this test pass happens after milestone 6. Through the production UI
only the naturally reachable refusals can be provoked — roughly L1, L2, a missing
`VanillaSource`, and an unwritable AFR root. **Mark the rest untested rather than
passed.** `worlds_verify.py` asserts the refusal logic against constructed cases
and passes 97/97, but that proves the rules, not the C++ wiring.

---

## Coverage this plan cannot reach

Two things were testable at milestones 1 and 3 and are not testable now, because
the harness that provided them was deleted in milestone 6 and testing was
deferred until after it. Neither is a defect; both are consequences of the
sequencing.

| Lost | What it proved | Nearest substitute here |
| ---- | -------------- | ----------------------- |
| **H4** — backup, empty, restore, back up again, compare the two backups file for file and in total bytes | The save-data service round-trips exactly, measured, in isolation | Part G's FTP comparison of `vanilla/save/data/` against your 0.1 copy. Weaker: it checks one direction, after the fact, without timings. |
| **H9's synthetic refusals** | All ten §4.4 rows fire on a constructed mismatch and on nothing else | Part L's three or four naturally reachable rows |

If either matters enough to you, restoring `Platform/SaveDataProbe` and
`UI/SaveProbeScreen` from git and rebuilding is a small change — they were
deleted, not rewritten, so `git checkout HEAD -- app/src/Platform/SaveDataProbe.*
app/src/UI/SaveProbeScreen.*` plus re-adding the `ScreenId` and a way to reach it
would bring them back. That is your call, not something to do mid-run.

---

## If something fails

1. **Stop.** Do not activate anything else.
2. **Keep `/data/bbrandomizer/SaveBackups/`.** Nothing prunes it. The
   `_preactivate` backup from the failing activation is the recovery path.
3. **Pull `live.log`** off the console. The `worlds:`, `activation:` and
   `savedata:` lines say how far each phase got.
4. **Photograph the screen** if the failure is visual — the geometry verifiers
   claim overlaps cannot happen, so a photo of one is evidence against the
   mirror, not just against the screen.
5. To recover the console: restore your 0.1 copy over FTP, and delete
   `/data/GoldHEN/AFR/<title>/dvdroot_ps4/` to get back to an unmodified game.

---

## Result sheet

| # | Test | Part | Result |
| - | ---- | ---- | ------ |
| H5 | First-run capture | A | **PASS** |
| H10a | Tabs, navigation, listing | B | **PASS** |
| H10b | Editor, name, `SAVE` category | C | **PASS** |
| H10c | Revisions and history | D | **PASS** |
| H10d | Delete keeps the save | E, G | **PASS** |
| — | Confirmation reads correctly, writes nothing | F | **PASS** |
| H6a | First activation | G | **PASS** |
| H4* | Backup round-trip, by FTP comparison | G | **PASS** |
| H6 | Two worlds, round trip, progress kept apart | H | **PASS** |
| H7 | `START FRESH` | I | **PASS** |
| H11 | Vanilla, and back — the P26 case | J | **PASS** |
| H8 | Interrupted activation, phase 4 | K | **PASS** |
| H8 | Interrupted activation, phase 6 | K | **PASS** |
| H9a | Second account sees nothing | L1 | **PASS** |
| H9b | No save data refusal | L2 | **PASS** |
| H9c | Remaining refusal rows | L3 | **NOT TESTABLE** — harness deleted in M6 |
