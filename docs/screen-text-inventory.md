# Screen text inventory — WORLDS and DEFAULTS

**Generated** by `app/tools/text_inventory.py` from the source. Do not
hand-edit the strings here expecting them to ship — mark this file up, and
the edits get applied to the source and the file regenerated.

Counts are characters, words, and **how many lines the string wraps to in
the 440px help column at scale 3**. The help pane allows **11 lines**; the
title above it allows 2. Line count is the number that matters — a long
string that still fits in 3 lines reads fine, and a short one that spills
to 12 is truncated on screen.

---

## How the three columns work

Both screens share one layout: a **rail** on the left, a **pane** in the
middle, a **help column** on the right.

| Column | WORLDS | DEFAULTS |
| --- | --- | --- |
| **Rail** | `+ NEW WORLD`, `VANILLA`, then the player's worlds | `TITLE ID`, then six category names |
| **Pane** | label/value detail rows for the highlighted world, then at most one note | the settings in the highlighted category, each a label and a value |
| **Help** | a title and a body, three variants by row type | the highlighted setting's own label and help text |

**There is no per-category help.** On DEFAULTS the help column always shows
a *setting's* help — the one under the cursor — except on rail row 0, where
it shows the title-ID help. So every category's worth of help text is the
settings' help text, listed below.

---

# 1. The WORLDS screen

## 1.1 Rail rows

| Row | Text | Source |
| --- | --- | --- |
| Row 0 | `+ NEW WORLD` | fixed |
| Row 1 | `VANILLA` | the world's name |
| Row 2+ | the world's name, up to 16 chars of A–Z, 0–9, space | player-entered |

## 1.2 Help column — three variants

### + NEW WORLD — *cursor on `+ NEW WORLD`*

**Title:** `+ NEW WORLD`

**Body** — 47 chars, 8 words, **2 lines of 11**

> Create a new world using your default settings.

### VANILLA — *cursor on `VANILLA`*

**Title:** `VANILLA`

**Body** — 59 chars, 10 words, **2 lines of 11**

> Play Bloodborne with the original game files and save data.

### WORLD — *cursor on any other world*

**Title:** `WORLD`

**Body** — 43 chars, 8 words, **2 lines of 11**

> A saved randomizer setup and its save data.

## 1.3 Pane — detail row labels

Label/value pairs. The labels are fixed; the values are computed.

| Row shown for | Labels, in order |
| --- | --- |
| `+ NEW WORLD` | `SETTINGS` |
| `VANILLA` | `STATUS`, `SAVE DATA`, `LAST PLAYED`, `CREATED` |
| a world | `STATUS`, `SEED`, `SETTINGS`, `REVISION`, `SAVE DATA`, `SAVED ON`, `LAST PLAYED` |

| Value | Form |
| --- | --- |
| `STATUS` | `ACTIVE` or `NOT ACTIVE` |
| `SAVE DATA` | e.g. `14.5 MB IN 17 FILES`, or `NONE` |
| `LAST PLAYED` | a timestamp, or `NEVER` |
| `SETTINGS` | e.g. `3 OF 16 ON` |
| `SAVED ON` | e.g. `REVISION 2`, or `-` |

## 1.4 Pane — notes

At most one note is shown beneath the detail rows: the row's own
explanation, or why the world cannot be activated.

**kNoteNewWorld** — *shown on `+ NEW WORLD`* — 33 chars, 5 words

> STARTS FROM YOUR DEFAULT SETTINGS

**kNoteVanilla** — *shown on `VANILLA`* — 31 chars, 6 words

> NO SETTINGS - CANNOT BE DELETED

**kNoteNoContainer** — *shown when Bloodborne has never made save data* — 48 chars, 7 words

> RUN BLOODBORNE ONCE BEFORE ACTIVATING THIS WORLD

**kNoteContainerTooSmall** — *shown when the world's save will not fit* — 47 chars, 9 words

> THIS WORLD'S SAVE IS TOO LARGE FOR THIS CONSOLE

## 1.5 Delete confirmation

* **kDeleteTitle** (12 chars) — `DELETE WORLD`
* **kDeleteLine1** (40 chars) — `REMOVES THIS WORLD AND ALL ITS REVISIONS`
* **kDeleteLine2** (21 chars) — `ITS SAVE DATA IS KEPT`
* **kDeleteKeptLine** (29 chars) — `SAVE DATA KEPT IN SAVEBACKUPS`
* **kDeleteFooter** (19 chars) — `X DELETE   O CANCEL`
* **kDeleteRefusedFooter** (6 chars) — `O BACK`
* **kRefuseVanilla** (25 chars) — `VANILLA CANNOT BE DELETED`
* **kRefuseActive** (31 chars) — `ACTIVE WORLDS CANNOT BE DELETED`

## 1.6 Startup screen

* **kWordmark** (21 chars) — `BLOODBORNE RANDOMIZER`
* **kLoadingWord** (7 chars) — `LOADING`
* **kProblemTitle** (15 chars) — `STARTUP PROBLEM`
* **kProblemReconcileFailed** (47 chars) — `AN INTERRUPTED ACTIVATION COULD NOT BE FINISHED`
* **kProblemJournalNotUnderstood** (63 chars) — `AN INTERRUPTED ACTIVATION COULD NOT BE READ - IT WAS LEFT ALONE`
* **kProblemCaptureFailed** (42 chars) — `YOUR SAVE COULD NOT BE COPIED INTO VANILLA`
* **kProblemNoSignedInPlayer** (19 chars) — `NO PLAYER SIGNED IN`
* **kProblemScrollHint** (14 chars) — `UP DOWN SCROLL`
* **kPromptContinue** (10 chars) — `X CONTINUE`
* **kPromptExit** (6 chars) — `O EXIT`

---

# 2. The DEFAULTS screen

## 2.1 Rail rows

| Row | Text |
| --- | --- |
| Row 0 | `TITLE ID` |
| Row 1 | `ENEMIES` |
| Row 2 | `BOSSES` |
| Row 3 | `ITEMS & TREASURE` |
| Row 4 | `WEAPONS & STARTING GEAR` |
| Row 5 | `DIFFICULTY` |
| Row 6 | `WORLD` |
| Row 7 | `SAVE`  ← **never shown on DEFAULTS**, editor only |

## 2.2 Title ID

**Rail label:** `TITLE ID`  **Help title:** `BLOODBORNE TITLE ID`  **Value:** the id, or `NOT SET`

**Help** — 127 chars, 22 words, **5 lines of 11**

> The PS4 title ID of the Bloodborne installation the randomizer writes to. CUSA03173 is the Europe and Game of the Year release.

---

# 3. Every setting

Each appears on DEFAULTS (except `SAVE DATA`) and again in the world
editor. Rail label, pane value, and help body are all listed.

## 3.1  ENEMIES

### RANDOMIZE ENEMIES

| | |
| --- | --- |
| **Rail label** | `RANDOMIZE ENEMIES` (17 chars) |
| **Pane value** | `YES` / `NO` |
| **Help title** | `RANDOMIZE ENEMIES` — same as the label |
| **Help length** | 72 chars, 9 words, **3 lines of 11** |

> Randomizes enemy placements throughout the world. Bosses are unaffected.

### ENEMIES INCLUDED

| | |
| --- | --- |
| **Rail label** | `ENEMIES INCLUDED` (16 chars) |
| **Pane value** | `n OF 82` |
| **Help title** | `ENEMIES INCLUDED` — same as the label |
| **Help length** | 48 chars, 7 words, **2 lines of 11** |

> Choose which enemies can appear as replacements.

### ENEMIES SKIPPED

| | |
| --- | --- |
| **Rail label** | `ENEMIES SKIPPED` (15 chars) |
| **Pane value** | `n OF 85` |
| **Help title** | `ENEMIES SKIPPED` — same as the label |
| **Help length** | 100 chars, 14 words, **4 lines of 11** |

> Choose enemies that keep their original placements. They may still appear as replacements elsewhere.

### DO NOT RANDOMIZE CAGED DOGS

| | |
| --- | --- |
| **Rail label** | `DO NOT RANDOMIZE CAGED DOGS` (27 chars) |
| **Pane value** | `YES` / `NO` |
| **Help title** | `DO NOT RANDOMIZE CAGED DOGS` — same as the label |
| **Help length** | 136 chars, 22 words, **5 lines of 11** |

> Keeps the caged dogs of Central Yharnam and the Forbidden Woods in their original placements. Other enemies behave badly in these cages.

## 3.2  BOSSES

### RANDOMIZE BOSSES

| | |
| --- | --- |
| **Rail label** | `RANDOMIZE BOSSES` (16 chars) |
| **Pane value** | `YES` / `NO` |
| **Help title** | `RANDOMIZE BOSSES` — same as the label |
| **Help length** | 48 chars, 6 words, **2 lines of 11** |

> Randomizes boss placements throughout the world.

### BOSSES INCLUDED

| | |
| --- | --- |
| **Rail label** | `BOSSES INCLUDED` (15 chars) |
| **Pane value** | `n OF 17` |
| **Help title** | `BOSSES INCLUDED` — same as the label |
| **Help length** | 47 chars, 7 words, **2 lines of 11** |

> Choose which bosses can appear as replacements.

## 3.3  ITEMS & TREASURE

### RANDOMIZE TREASURE

| | |
| --- | --- |
| **Rail label** | `RANDOMIZE TREASURE` (18 chars) |
| **Pane value** | `YES` / `NO` |
| **Help title** | `RANDOMIZE TREASURE` — same as the label |
| **Help length** | 44 chars, 6 words, **2 lines of 11** |

> Randomizes items found throughout the world.

### RANDOMIZE WORKSHOP TOOLS

| | |
| --- | --- |
| **Rail label** | `RANDOMIZE WORKSHOP TOOLS` (24 chars) |
| **Pane value** | `YES` / `NO` |
| **Help title** | `RANDOMIZE WORKSHOP TOOLS` — same as the label |
| **Help length** | 93 chars, 15 words, **4 lines of 11** |

> Adds the Blood Gem and Rune Workshop Tools to the treasure pool. Requires Randomize Treasure.

### RANDOMIZE ENEMY DROPS

| | |
| --- | --- |
| **Rail label** | `RANDOMIZE ENEMY DROPS` (21 chars) |
| **Pane value** | `YES` / `NO` |
| **Help title** | `RANDOMIZE ENEMY DROPS` — same as the label |
| **Help length** | 41 chars, 6 words, **2 lines of 11** |

> Randomizes what enemies drop when killed.

## 3.4  WEAPONS & STARTING GEAR

### RANDOMIZE STARTING WEAPONS

| | |
| --- | --- |
| **Rail label** | `RANDOMIZE STARTING WEAPONS` (26 chars) |
| **Pane value** | `YES` / `NO` |
| **Help title** | `RANDOMIZE STARTING WEAPONS` — same as the label |
| **Help length** | 59 chars, 9 words, **2 lines of 11** |

> Randomizes the trick weapons offered in the Hunter's Dream.

### RANDOMIZE STARTING GUNS

| | |
| --- | --- |
| **Rail label** | `RANDOMIZE STARTING GUNS` (23 chars) |
| **Pane value** | `YES` / `NO` |
| **Help title** | `RANDOMIZE STARTING GUNS` — same as the label |
| **Help length** | 54 chars, 8 words, **2 lines of 11** |

> Randomizes the firearms offered in the Hunter's Dream.

### RANDOMIZE SHOP WEAPONS

| | |
| --- | --- |
| **Rail label** | `RANDOMIZE SHOP WEAPONS` (22 chars) |
| **Pane value** | `YES` / `NO` |
| **Help title** | `RANDOMIZE SHOP WEAPONS` — same as the label |
| **Help length** | 51 chars, 8 words, **2 lines of 11** |

> Randomizes the weapons sold by the Bath Messengers.

### START WITH HUNTER TOOLS

| | |
| --- | --- |
| **Rail label** | `START WITH HUNTER TOOLS` (23 chars) |
| **Pane value** | `YES` / `NO` |
| **Help title** | `START WITH HUNTER TOOLS` — same as the label |
| **Help length** | 99 chars, 19 words, **4 lines of 11** |

> Start with the Blood Gem and Rune Workshop Tools. Gems and runes can be fitted from the first area.

## 3.5  DIFFICULTY

### EASY SHADOWS

| | |
| --- | --- |
| **Rail label** | `EASY SHADOWS` (12 chars) |
| **Pane value** | `YES` / `NO` |
| **Help title** | `EASY SHADOWS` — same as the label |
| **Help length** | 87 chars, 15 words, **3 lines of 11** |

> Replaces the duplicate Shadows of Yharnam with harmless larvae, so the fight is a duel.

### EASY ROM

| | |
| --- | --- |
| **Rail label** | `EASY ROM` (8 chars) |
| **Pane value** | `YES` / `NO` |
| **Help title** | `EASY ROM` — same as the label |
| **Help length** | 54 chars, 7 words, **2 lines of 11** |

> Replaces Rom's attendant spiders with harmless larvae.

### EASY FAILURES

| | |
| --- | --- |
| **Rail label** | `EASY FAILURES` (13 chars) |
| **Pane value** | `YES` / `NO` |
| **Help title** | `EASY FAILURES` — same as the label |
| **Help length** | 60 chars, 8 words, **2 lines of 11** |

> Replaces the duplicate Living Failures with harmless larvae.

### EASY EMISSARY

| | |
| --- | --- |
| **Rail label** | `EASY EMISSARY` (13 chars) |
| **Pane value** | `YES` / `NO` |
| **Help title** | `EASY EMISSARY` — same as the label |
| **Help length** | 73 chars, 9 words, **3 lines of 11** |

> Replaces the Celestial Emissary's lesser emissaries with harmless larvae.

## 3.6  WORLD

### ENABLE MERGO DARKNESS

| | |
| --- | --- |
| **Rail label** | `ENABLE MERGO DARKNESS` (21 chars) |
| **Pane value** | `YES` / `NO` |
| **Help title** | `ENABLE MERGO DARKNESS` — same as the label |
| **Help length** | 82 chars, 15 words, **3 lines of 11** |

> Turns the Wet Nurse's arena darkness on for the whole game, from your first spawn.

## 3.7  SAVE

*Editor only. Deliberately absent from DEFAULTS — it is a per-world
question, not a preference.*

### SAVE DATA

| | |
| --- | --- |
| **Rail label** | `SAVE DATA` (9 chars) |
| **Pane value** | `KEEP EXISTING` / `START FRESH` |
| **Help title** | `SAVE DATA` — same as the label |
| **Help length** | 195 chars, 34 words, **6 lines of 11** |

> Keep Existing restores this world's save, or uses your current save if it has none. Start Fresh begins a new playthrough, and applies to this activation only. Your save is always backed up first.

---

# 4. World editor only

Three rail rows the DEFAULTS tab does not have.

### NAME

**Help** — 88 chars, 16 words, **3 lines of 11**

> What this world is called. Up to 16 characters. Renaming does not create a new revision.

### SEED

**Help** — 96 chars, 14 words, **4 lines of 11**

> Determines this world's randomization. The same seed and settings always produce the same world.

### HISTORY

**Help** — 119 chars, 22 words, **4 lines of 11**

> Every set of settings this world has had, newest first. Choosing an older one makes it current, so the list only grows.

---

# 5. Longest first

The help strings ranked by how many lines they occupy, since that is what
makes a column look dense. Eleven is the cap.

| Lines | Words | Setting |
| --- | --- | --- |
| 6 | 34 | SAVE DATA |
| 5 | 22 | TITLE ID |
| 5 | 22 | DO NOT RANDOMIZE CAGED DOGS |
| 4 | 22 | HISTORY |
| 4 | 19 | START WITH HUNTER TOOLS |
| 4 | 15 | RANDOMIZE WORKSHOP TOOLS |
| 4 | 14 | SEED |
| 4 | 14 | ENEMIES SKIPPED |
| 3 | 16 | NAME |
| 3 | 15 | ENABLE MERGO DARKNESS |
| 3 | 15 | EASY SHADOWS |
| 3 | 9 | RANDOMIZE ENEMIES |
| 3 | 9 | EASY EMISSARY |
| 2 | 10 | WORLDS help — VANILLA |
| 2 | 9 | RANDOMIZE STARTING WEAPONS |
| 2 | 8 | WORLDS help — WORLD |
| 2 | 8 | WORLDS help — + NEW WORLD |
| 2 | 8 | RANDOMIZE STARTING GUNS |
| 2 | 8 | RANDOMIZE SHOP WEAPONS |
| 2 | 8 | EASY FAILURES |
| 2 | 7 | ENEMIES INCLUDED |
| 2 | 7 | EASY ROM |
| 2 | 7 | BOSSES INCLUDED |
| 2 | 6 | RANDOMIZE TREASURE |
| 2 | 6 | RANDOMIZE ENEMY DROPS |
| 2 | 6 | RANDOMIZE BOSSES |

