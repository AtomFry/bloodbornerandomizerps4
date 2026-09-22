# Feature — Worlds

**Status: APPROVED** — approved 2026-09-21 and re-approved 2026-09-22 after the
D20–D27 amendments from the hardware probe's results and from two gaps the plan
raised (D13 superseded).

**Reference:** New feature. No reference-tool equivalent — the Windows tool
randomizes in place and has no concept of managing more than one configuration.

**Technical evidence:** `docs/features/worlds/technical-findings.md` — what the
platform permits, and how we know. This document is **functional only**: it says
what the app does and what the player sees. Where behaviour depends on a
platform fact, it cites that document rather than restating it.

**Plan:** `docs/features/worlds/plan.md` — added after this spec is approved.

**Folder naming:** unnumbered, like `font-atlas/` and `randomizer-settings-ui/`.
Platform/UI work with no backlog row. See `docs/features/README.md`.

---

## 1. What does this feature do?

It replaces "enable and disable the randomizer" with **worlds**: named,
independently configured playthroughs that the player switches between, each
keeping its own save data.

Today the app has exactly one randomizer configuration, applied or not applied.
Changing anything overwrites what came before, and save data is entirely
unmanaged — a player who wants a second run either loses their first or manages
files by hand.

A world pairs a **randomizer configuration** with the **save data produced by
playing it**. Activating a world makes both current together. The player's
original, un-randomized playthrough is itself a world — **Vanilla** — so it is
protected by the same machinery rather than being an unmanaged special case.

This subsumes the Enable and Disable wizards. `Disable Randomizer` is currently
an unimplemented `PlaceholderScreen`; in this model "disable" is activating
Vanilla.

---

## 2. What does the player experience?

### The main screen

Two text tabs across the top, the active one boxed: **`WORLDS`** and
**`DEFAULTS`**. **Left/Right switches tabs while focus is on the rail** — those
inputs are already unused there on both screens, so no new button is needed and
nothing gains a second meaning. Inside the settings pane Left/Right keeps
changing a value, as it does today. The active tab's name appears as a heading
beneath them.

**`WORLDS`** lists, in the left rail:

```
+ NEW WORLD
VANILLA
  <the player's worlds, most recently played first>
```

`+ NEW WORLD` and `VANILLA` are always present regardless of how many worlds
exist. Up/Down moves between rows; the middle pane shows the highlighted world's
details — name, seed, a summary of its settings, its save data's size and when it
was last played, and whether it is currently active. The right pane explains what
the highlighted row does.

Exactly one row is marked **ACTIVE**, or none is if the randomizer has never been
used.

**`DEFAULTS`** is today's Setup Defaults screen: the settings a **new** world
starts from, plus the Bloodborne title ID. Renamed from "Settings" because with
per-world settings that word is now ambiguous.

### Creating and editing

`X` on `+ NEW WORLD` opens the world editor — today's categorised settings screen
— pre-filled from Defaults. The player names it, sets a seed, chooses settings,
and activates.

`X` on an existing world opens the same editor with that world's current
settings. The player can change anything, activate, or back out.

`X` on `VANILLA` activates vanilla: the randomizer's files are removed and the
game runs unmodified.

### Activation

Activating is the only operation that changes what the game will do, and it
always does the same three things as one unit:

1. The **currently active world's save data is backed up to that world.** The
   active world's save is always whatever is live on the console, so this is how
   progress is kept.
2. The **new world's randomizer files are generated** and made current.
3. The **new world's save data becomes live** — restored from that world, or
   started fresh if the player chose that.

A confirmation screen states exactly this before anything happens: which world is
being deactivated and where its save is going, which is being activated, whether
its save is being restored or started fresh, and roughly how long it will take.

Activation then shows the existing progress log.

### Save handling

A **`SAVE`** category appears in the world editor with one setting for now:

| Setting | Meaning |
| --- | --- |
| **Save Data** | `KEEP EXISTING` (**always the default**) — the world uses its own stored save if it has one, or adopts the currently live save if it does not. `START FRESH` — the live save is backed up, then the game's own files are removed from it, so the game starts a new playthrough. |

Regardless of the choice, **the outgoing save is always backed up first.**

`START FRESH` **empties** the container rather than deleting it: the game's
`userdata*` and `backup*` files are removed and `sce_sys` is left alone (D20).
The game then behaves as it would with no save at all. The container itself is
never destroyed, because this app cannot create one (D21).

**`START FRESH` never persists.** It applies to one activation and then the
setting returns to `KEEP EXISTING`, recorded as a new revision. Choosing it is
always a deliberate act for the activation in front of the player, never a
standing property of the world. A stored `START FRESH` would make a world
discard its own progress on every future activation — the silently-incoherent
failure §7.1 exists to prevent — and no amount of careful confirmation wording
reliably stops a repeat.

**`KEEP EXISTING` means two different things**, and the confirmation screen must
say which one applies:

* the world **has** a stored save → that save is restored;
* the world **has none** → the currently live save is adopted into it.

It never means overwriting a world's stored save with another world's. That is
the aliasing D4 forbids.

Separately and automatically, every destructive save operation first writes a
**timestamped safety backup** that is never overwritten or auto-deleted. World
saves are working copies that get swapped; safety backups are the recovery path
when something goes wrong.

### Revisions

A world's settings are **never overwritten**. Editing a world adds a new
**revision** and makes it current; earlier revisions remain and can be made
current again (which itself becomes a new revision, so nothing is ever lost).

This exists because re-rolling a world's enemies while keeping progress is a
thing players actually want — testing has shown it works and does not harm save
data, and it is the practical fix for an impassable area. It is distinct from
creating a new world: a new world means new (or copied) save data, whereas a
revision keeps the same playthrough and changes only what is in the world.

The world's save is paired with the **world**, not with a revision. The app
records which revision was current when the save was last written, so a player
can see "last played on revision 2, current is revision 3."

### Deleting

Deleting a world asks for explicit confirmation, names what is being destroyed,
and **keeps the world's save data as a safety backup anyway**. The active world
cannot be deleted; it must be deactivated first.

---

## 3. What does the existing randomizer do?

* **One configuration, applied or not.** The Enable wizard writes randomizer
  output for the configured title; there is no second configuration and no way to
  return to a previous one.
* **`Disable Randomizer` is a `PlaceholderScreen`.** Never implemented.
* **Save data is untouched.** The simulated backup/replace handling was removed
  in `randomizer-settings-ui` milestone 1 precisely so the real thing could be
  designed from a clean slate.
* **Per-run settings are not persisted.** Only `lastSeed` is written back.
* **Status is derived from disk, never from a stored flag.** `AfrManager` checks
  for `.bbrandomizer_manifest` inside `dvdroot_ps4`, and its header states the
  reasoning: the marker travels with the content it describes, so "there is no
  separate app-level 'is it on' flag to fall out of sync." **Nothing writes that
  marker yet.**
* **The categorised settings screen, the settings model and the confirm/progress
  screens all exist** (`randomizer-settings-ui`), and this feature reuses them.

---

## 4. What do we know?

Everything below was proven on hardware. The mechanics — the exact calls, the
file sets, the measured timings — are written up in `technical-findings.md`,
and that is where planning and implementation should go for detail. What
belongs here is only what those results allow and forbid.

1. **Backing up, emptying and restoring a save all work**, using only the
   official SDK and no privilege escalation. The feature is possible.
2. **The app can never create a save container** — only the game can. A world's
   stored save can therefore only be written into a container Bloodborne itself
   has already made, and a console where the game has never run cannot have a
   world activated onto it. Activation must detect that and say so (D21).
3. **A container is emptied, never destroyed.** The game's own save files are
   removed and the container's metadata is left untouched; the game then starts
   a new playthrough, and a later restore brings the old one back (D20, D22).
4. **The save-data title ID is not the application title ID** — here the game is
   `CUSA03173` and its saves are under `CUSA00207`. The app must discover this
   by searching, and cannot read the game's own metadata to be told.
5. **Free space cannot be measured.** The app does not check capacity in
   advance; it finds out that the disk is full when the backup fails, and stops
   there (D24).
6. **A failed save-data operation is not a no-op.** One refused operation
   preceded a live save that would no longer load. Every operation must be
   treated as capable of damage even when it reports failure.
7. **Randomizer output is reproducible** from seed plus settings — an existing
   acceptance criterion. A world therefore stores its *recipe*, not its
   generated files.

---

## 5. Terminology

| Term | Meaning |
| --- | --- |
| **World** | A named playthrough: a settings history plus its save data. |
| **Revision** | One version of a world's settings. Worlds accumulate revisions; the newest is current. |
| **Vanilla** | The un-randomized world. Always present. Holds the player's original save. |
| **Active** | The world whose randomizer files and save data are currently live. At most one. |
| **Activation** | Switching which world is active — the one operation that swaps files and save data. |
| **World save** | A world's copy of save data. Swapped on activation. |
| **Safety backup** | A timestamped, never-overwritten snapshot taken before any destructive save operation. |

---

## 6. Scope

### In scope

* The tabbed main screen (`WORLDS` / `DEFAULTS`) and the worlds rail.
* Creating, editing, activating and deleting worlds.
* Settings revisions with rollback.
* Vanilla as a first-class world, including capturing the player's existing save
  into it on first run.
* Save data backup and restore on activation, and the `START FRESH` option.
* Automatic safety backups before destructive save operations.
* Discovering the save-data title ID rather than assuming it.
* Storage layout for worlds, revisions, saves and safety backups.
* Writing `.bbrandomizer_manifest` so the active world is derivable from disk.
* Retiring the Enable and Disable wizards into this model.

### Out of scope

* **Any change to randomization behaviour or output.** Output parity still holds.
* Adding or removing randomizer settings, beyond the one `SAVE` setting above.
* Sharing one save between worlds by reference. Copying is in scope; linking is
  not (see §7).
* Importing or exporting worlds off-console.
* Re-binding a save to a different account. Worlds belong to the account that
  created them (D12); moving one between accounts is Apollo's `patch_sfo`
  territory and is not attempted.
* Artwork and tab icons — text tabs for now.
* Chalice dungeon settings.

---

## 7. Constraints and decisions

### 7.1 Safety rules

These exist because the feature can destroy a player's progress.

* **Back up before destroying.** A safety backup is written and **verified** —
  file count and sizes against a manifest — before any operation that overwrites
  or removes save data. Only then may the live save be touched.
* **Activation is one transaction.** Randomizer files and save data switch
  together or not at all. A world's save running on another world's randomization
  is not corrupt, it is *silently incoherent*, which is worse.
* **Never link saves, only copy.** A save shared by two worlds has no defined
  owner: activating either silently changes the other. Copying gives the same
  capability with none of the aliasing.
* **Refuse rather than guess.** A restore checks the owning account, the target
  title and directory, and that the container is large enough — **before writing
  a byte**. Any mismatch refuses.
* **Everything is scoped to one player.** Save searches, worlds and backups all
  belong to the signed-in user. The app never reads another user's save data and
  never lists another user's worlds.
* **Never destroy a container, only its contents.** The app cannot create one
  (D21), so destroying a container would be unrecoverable without the game.
  `START FRESH` empties; it does not delete.
* **A failed backup stops everything.** The backup runs first and is verified;
  if it fails — disk full, or any other reason — the activation aborts before
  anything else is touched. An incomplete backup is never mistaken for a good
  one (D24).
* **A failure is not a no-op.** Any save-data operation that reports an error
  must be treated as potentially having changed something. Bracket risky
  operations with a before-and-after reading of the container.
* **Never index into an unordered result.** Save directory search order is not
  guaranteed; selection must be explicit.
* **Deleting keeps the save.** Deleting a world keeps its save as a safety
  backup.
* **First run captures the existing save into Vanilla before anything else.** A
  fresh install already has a real playthrough on the console; if the first action
  is creating a world, that save must already belong to Vanilla.

### 7.2 Decisions taken

| # | Decision |
| --- | --- |
| D1 | Worlds store a settings *recipe*, not generated files. Activation regenerates. |
| D2 | Settings edits create revisions; nothing is overwritten. History is append-only. |
| D3 | A world's save pairs with the world, not with a revision. The revision last played is recorded. |
| D4 | Saves are copied between worlds, never shared by reference. |
| D5 | Vanilla is a world, not a special case. |
| D6 | `START FRESH` vs `KEEP EXISTING` is a setting in a `SAVE` category in the world editor. |
| D7 | Tabs are text (`WORLDS`, `DEFAULTS`). Icons deferred. |
| D8 | "Settings" is renamed "Defaults" — it is what a new world starts from. |
| D9 | Deleting a world always keeps its save as a safety backup. |
| D10 | The Enable and Disable wizards are retired into this model. |
| D11 | The active world is recorded inside `.bbrandomizer_manifest`, so it travels with the content and cannot drift from a separate flag. |
| D12 | Worlds belong to the account that created them and are listed only for that account. Save searches are already scoped to one user, so another account's worlds are simply absent rather than shown as unusable. |
| D13 | ~~Activation refuses when there is not enough disk space to back up the outgoing save.~~ **Superseded by D24** — free space cannot be measured on this platform. |
| D14 | The world editor shows current settings; revisions live behind a `HISTORY` rail row, recent first, reachable only deliberately. |
| D15 | A world's identity is an opaque id assigned at creation; the name is editable metadata and duplicate names are permitted. |
| D16 | Vanilla has no editable settings and no revisions, and cannot be deleted. Selecting it shows its save details and an explanation. |
| D17 | `SAVE DATA` always defaults to `KEEP EXISTING`. `START FRESH` applies to a single activation and then reverts, recorded as a revision. |
| D18 | On a console whose randomizer output predates this feature — randomizer files present with no manifest — the live save is still captured into Vanilla per §7.1, because preserving it matters more than where it is filed. The app shows an `UNMANAGED` state rather than claiming the game is unmodified. |
| D19 | Tabs are switched with Left/Right **while focus is on the rail**, where those inputs are already unused on both screens. No new button is introduced. |
| D20 | `START FRESH` empties the container by unlinking `userdata*` and `backup*`, leaving `sce_sys`. `sceSaveDataDelete` is not used. Proven on hardware: the game then starts a new playthrough, and a later restore recreates the files. |
| D21 | The app never creates a save container. A world's stored save can only be written into a container the game has already made, so a world cannot be activated on a console where Bloodborne has never run. Activation must detect that and say so rather than failing midway. |
| D22 | `sce_sys` is never written. The restore file set is everything except that directory. |
| D23 | A restore requires the target container to be at least as large as the save needs. `SAVEDATA_BLOCKS` lives in the unwritable `param.sfo`, so the size is fixed by the container and must be checked before writing, not discovered partway. |
| D24 | No pre-flight disk-space check. Activation runs the backup and **aborts the whole transaction if it fails for any reason**, including running out of space. A backup that did not complete is never treated as a backup, and a partial one is marked or removed rather than left looking usable. |
| D25 | The **AFR title** is the `BLOODBORNE TITLE ID` setting on the Defaults tab, defaulting to `CUSA03173`. The **save-data title** is discovered by searching, and is a different value (`CUSA00207` on the reference console). The two are independent and neither is derived from the other. |
| D26 | When the container exists but holds no game save files — the player deleted the save through the console's own Saved Data Management — activation takes no safety backup and **leaves the outgoing world's stored save exactly as it was**. There is nothing to capture, and replacing a good stored save with an empty capture would be destroying it, with the backup that normally covers such a replacement being precisely what cannot be taken. The confirmation screen says the outgoing world was left alone. |
| D27 | A world targets the AFR title named by the `BLOODBORNE TITLE ID` setting and nothing else. If that title is not among the installs the app detects, activation **refuses** and lists what it did detect. Writing an AFR tree for a title that is not installed produces a game that launches unmodified with no error anywhere — the silent failure this project has already been bitten by once. |

### 7.3 Constraints

* **The player closes the game before using this app** — the PS4 enforces it, so
  swapping files underneath a running game is not a risk here.
* **Output parity.** Regenerating a revision must produce what it produced before.
* **Preserve the one-`Screen` pattern** and the existing layering.
* **No new libraries** beyond `libSceSaveData`, which is already proven to link.
* **Disk is finite, and unmeasurable.** Each save is ~36 MB on the same
  partition as `VanillaSource` (multiple GB) and AFR. Free space cannot be read
  (§4.5), so the app cannot warn in advance — it finds out when a backup fails,
  and stops there (D24). Safety backups accumulate and are never auto-pruned
  (§11), which makes running out a question of when.

---

## 8. How will we know it works?

### Automated

* **Output parity** — existing verifiers continue to pass; a given revision's
  output is unchanged by this feature.
* **Storage round-trip** — a world written and read back yields identical
  settings, revisions and metadata.
* **Manifest correctness** — every backup's manifest matches the files beside it
  in count and size.
* **Refusal logic** — account, title, directory and container-size mismatches
  each refuse, tested against constructed manifests without touching real
  saves. A backup that fails partway aborts the transaction and leaves no
  backup that could later be mistaken for complete.

### Hardware

Ordered so each step is safe before the next is attempted:

1. ~~**Restore into an existing container**~~ — **already proven on hardware**;
   the sequence is `technical-findings.md` §8.4. The tests below re-exercise it
   through the production service rather than re-prove it.
2. ~~**`START FRESH`**~~ — **already proven on hardware**; the sequence is
   `technical-findings.md` §8.3.
3. **Activation round-trip** — play world A, activate B, play, return to A, and
   confirm A's progress is intact.
4. **First-run capture** — on a console with an existing save and no worlds, that
   save becomes Vanilla's.
5. **Player identity** — the app reports the signed-in player's name, and the
   account id it reads matches the `ACCOUNT_ID` inside that player's save. On a
   console with a second account, that account sees no worlds belonging to the
   first.
6. **Interrupted activation** — power off mid-activation; confirm no save is lost
   and the app reconciles on next launch.
7. Tab navigation, world listing, delete-keeps-save, and disk-full behaviour —
   including that activation **refuses** when the outgoing backup will not fit.

---

## 9. Open questions

*Deliberately empty.* Every question this spec raised has been answered and
recorded in §10 — D14–D19 on 2026-09-21, and D20–D27 on 2026-09-22 after
the hardware probe's results and the plan's two spec gaps. The heading is kept so the gap between §8 and
§10 cannot be mistaken for an editing accident.

---

## 10. Decisions

| Date | Decision |
| ---------- | -------- |
| 2026-09-21 | D1–D11 in §7.2, taken with the developer while designing this feature. |
| 2026-09-21 | **D12** — worlds belong to the account that created them and are listed only for that account. |
| 2026-09-21 | **D13** — activation refuses when the outgoing save's backup will not fit on disk. |
| 2026-09-21 | **D14** — the world editor shows current settings; revisions live behind a `HISTORY` rail row, recent first, reachable only deliberately. |
| 2026-09-21 | **D15** — a world's identity is an opaque id assigned at creation; the name is editable metadata and duplicate names are permitted. |
| 2026-09-21 | **D16** — Vanilla has no editable settings and no revisions, and cannot be deleted. Selecting it shows its save details and an explanation. |
| 2026-09-21 | **D17** — `SAVE DATA` defaults to `KEEP EXISTING`; `START FRESH` is a one-shot the player must choose each time. Raised as a spec gap during planning. |
| 2026-09-21 | **D18** — pre-existing unmanaged randomizer output: the live save still goes to Vanilla, with an `UNMANAGED` state shown. Raised as a spec gap during planning. |
| 2026-09-21 | **D19** — tab switching is Left/Right on the rail, not a new button. |
| 2026-09-22 | **D20** — `START FRESH` empties the container by unlinking `userdata*` and `backup*`. Proven on hardware. |
| 2026-09-22 | **D21** — the app never creates a save container; `CREATE2` is refused by the platform. |
| 2026-09-22 | **D22** — `sce_sys` is never written; it refuses with EACCES and never needs changing. |
| 2026-09-22 | **D23** — a restore checks the container is large enough before writing. |
| 2026-09-22 | **D24** — no pre-flight disk check; a failed backup aborts the activation. Supersedes D13, which the platform cannot support. |
| 2026-09-22 | **D25** — the AFR title comes from the `BLOODBORNE TITLE ID` setting; the save-data title is discovered separately. |
| 2026-09-22 | **D26** — a container holding no save files means no backup and no capture; the outgoing world's stored save is left untouched. Raised by the plan as a spec gap. |
| 2026-09-22 | **D27** — the `BLOODBORNE TITLE ID` setting alone decides the AFR title, and a title that is not installed refuses at activation. Raised by the plan as a spec gap. |

---

## 11. Deferred

* **Tab icons** — text tabs ship first.
* **Importing/exporting worlds**, including off-console copies.
* **Re-binding a save to a different account** — see D12.
* **Sharing one save across worlds by reference** — copying covers the need.
* **A shared view across accounts.** Each account sees its own worlds; there is
  no combined listing and no transfer between them.
