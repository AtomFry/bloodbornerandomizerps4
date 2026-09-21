# Bloodborne Randomizer Manager — UI Blueprint

Status: agreed design, not yet fully implemented. This is the screen-level
specification the UI is built against going forward. Update this file
whenever a screen's behavior changes for real — it should never drift from
the actual app.

> **In progress:** the Enable wizard's settings step is being redesigned around
> a six-category Settings screen, with Confirm becoming a genuine final review.
> That work is specified in
> `docs/features/randomizer-settings-ui/spec.md` (spec stage, questions open).
> This blueprint still describes the **current** flat-list behaviour and should
> be updated from that spec only once the change is actually implemented.

## Relationship to the earlier architecture review

This supersedes the **top-level menu shape** from the earlier
"Bloodborne Randomizer Manager" architecture review (that menu had five
items: Enable Randomizer / Manage Saves / Randomizer Profiles / Settings /
Exit). It does **not** replace that review's foundation:

- The four state machines (`ApplicationState`, `AfrState`, `SaveState`,
  `RandomizerState`) are unchanged.
- The transaction model (journal-first, stage-then-atomically-activate,
  verify after every write, never delete the only known backup) is
  unchanged.
- The Save Data / Game Data (AFR Content) terminology split is unchanged.
- The data model sketch (`RandomizerSettings`, `RandomizerProfile`,
  `SaveBackup`, `OperationPlan`, `TransactionRunner`) is unchanged, refined
  below with the exact fields this screen spec needs.

What changed: **Manage Saves** and **Randomizer Profiles** are no longer
standalone main-menu destinations. They only exist as sub-screens reached
from *inside* the Enable/Disable wizards, at the exact moment a save or a
profile actually needs to be picked. The main menu now has exactly three
actions plus Exit.

## Guiding principle

**The UI represents the user's decisions, not the application's internal
operations.**

No screen ever shows: AFR activation/deactivation, staging, transactions,
journals, DCX/BND/PARAM processing, save-data container internals,
verification passes, or rollback. The user sees "Enable Randomizer"; the
application figures out everything necessary to do that safely.

## Global conventions

- **D-pad up/down** — move the highlighted row in a list.
- **D-pad left/right** — change a value in place (toggle a boolean, adjust a
  number) without leaving the row.
- **Cross (X)** — activates whatever row is highlighted (toggle a boolean,
  drill into a sub-screen/editor). Never means "save" or "advance the
  wizard" - those are separate buttons (below), specifically so X's meaning
  never changes depending on which row happens to be selected.
- **Circle (O)** — back one screen, or on a wizard's first step, cancel out
  of the wizard entirely.
- **Options** — the one "move forward" action across the whole app: SAVE on
  a plain editor (Setup Defaults), NEXT on a wizard step, and COMMIT on a
  wizard's final Confirm screen. Always pairs with Circle as BACK/cancel.
  Triangle is unused for now.
- **Nothing is written anywhere until the wizard's final CONFIRM.** Every
  screen before Confirm only edits an in-memory plan.
- No screen shows a raw filesystem path or any AFR/staging/journal
  vocabulary.

## Data model for this spec

```
RandomizerProfile
    name            // user-facing label, e.g. "Ludwig Chaos Run"
    seed
    settings        // full RandomizerSettings snapshot
    createdAt
    titleId

SaveBackup
    kind            // Vanilla | Randomizer
    label           // user-facing name/date
    createdAt
    linkedProfile   // optional - only meaningful for Randomizer-kind backups

RandomizerDefaults      // NOT a profile - see note below
    backupExistingSaveData : bool
    saveReplaceDefault     : SaveReplaceChoice
    settings               : RandomizerSettings   // pre-fills a NEW profile's settings
```

**Defaults ≠ Profile.** `Setup Defaults` edits a baseline used to pre-fill
Screen 2A when creating a *new* randomizer. It is never itself selectable
under "use existing seed," and creating a new randomizer never edits the
defaults — it only starts from a copy of them.

```
EnableOperationPlan
    backupExistingSaveData : bool                  // Screen 1
    saveReplace : { NewSaveData, LeaveExisting, SelectFromBackup }
    selectedSaveBackup : SaveBackup?               // set via Screen 1A

    seedChoice : { New, Existing }                 // Screen 2
    selectedProfile : RandomizerProfile?           // set via Screen 2B, if Existing
    newSeed : uint64                               // generated on entering Screen 2A, if New
    newSettings : RandomizerSettings               // edited on Screen 2A, pre-filled from Defaults

DisableOperationPlan
    saveRestore : { SelectFromBackup, NewSaveData, LeaveExisting }
    selectedSaveBackup : SaveBackup?               // set via Screen 1A (same control, reused)
    // Backing up the current randomizer save before disabling is automatic,
    // not a user toggle - see the open question at the top of this file.
```

---

## Screen inventory

```
Main Menu
  │
  ├── Enable Randomizer  (no-op notice if already ON)
  │     ├── Screen 1  — Save Data
  │     │     └── Screen 1A — Select Save Backup
  │     ├── Screen 2  — Randomizer (seed choice)
  │     │     ├── Screen 2A — New Randomizer Settings
  │     │     └── Screen 2B — Select Existing Profile
  │     ├── Confirm (Enable)
  │     ├── Progress (Enable)
  │     └── Result (Enable)
  │
  ├── Disable Randomizer  (no-op notice if already OFF)
  │     ├── Screen D1 — Save Data
  │     │     └── Screen 1A — Select Save Backup (same control as Enable's)
  │     ├── Confirm (Disable)
  │     ├── Progress (Disable)
  │     └── Result (Disable)
  │
  ├── Setup Defaults   (single scrolling screen, same row control as 2A)
  └── Exit
```

---

## Main Menu

**Banner:** `RANDOMIZER STATUS: ON` or `RANDOMIZER STATUS: OFF`.

**Items:** Enable Randomizer, Disable Randomizer, Setup Defaults, Exit —
always all four, regardless of current status.

**Behavior:** selecting Enable while already ON (or Disable while already
OFF) is allowed - it enters the wizard exactly as normal. No special-cased
notice; the current status doesn't gate which actions are reachable.

---

## Enable Randomizer

### Screen 1 — Save Data

**Backup existing save data?** `YES` / `NO` — defaults from
`RandomizerDefaults.backupExistingSaveData`.

**Replace save** — a single summary row showing the current choice as
plain text: `NEW SAVE DATA`, `LEAVE EXISTING SAVE DATA`, or the name of a
selected backup. X on this row drills into **Screen 1A** to change it; the
choice itself is made there, not on this screen.

**Navigation:** O → Main Menu. Options → Screen 2 (or wherever this
milestone's landing screen is, until Screen 2 exists).

### Screen 1A — Select Replace Save

One unified, scrollable list — not a backup browser bolted onto a
separate radio choice:

```
> New Save Data
  Leave Existing Save Data
  Randomizer Save — Ludwig Run
  Randomizer Save — New Game+
  Vanilla Save — 2026-09-01
```

The first two rows are fixed actions. Everything after them is a real
randomizer-save backup — created by this app itself, at commit time,
whenever `BACKUP EXISTING SAVE` was `YES` on a previous Enable/Disable run.
Below the list, metadata for the highlighted entry (created date, etc.) -
blank for the two fixed actions, populated for a real backup.

- **X** on any row → return to Screen 1 with that choice recorded, and the
  summary row updated to match (plain text for the two fixed actions, the
  backup's name for a backup).
- **O** → return to Screen 1 with **no change** to whatever was selected
  there before entering this screen.

### Screen 2 — Randomizer

**Seed:** one of two, radio-style:
1. `New randomizer seed` → **Screen 2A** on NEXT.
2. `Select existing randomizer seed` → **Screen 2B**.

**Navigation:** BACK → Screen 1. NEXT → Screen 2A or the Confirm screen,
depending on which seed option led here and whether 2A/2B already
recorded a selection.

### Screen 2A — New Randomizer Settings

A scrolling list, one row per setting, pre-filled from
`RandomizerDefaults.settings`. Top of the list always shows the generated
**Seed** value (read-only here; regenerating it is a later concern, not
v1).

Row types:
- **Boolean** — `ON`/`OFF`, left/right toggles.
- **Numeric** — `←  3  →`, left/right adjusts by one step.
- **Drill-in (later)** — a row ending in `>` that opens a sub-screen for a
  cluster of related advanced settings. **Not built in v1** — the
  architecture must not preclude it, but no such row exists yet.

Footer: `↑↓ NAVIGATE   ←→ CHANGE   O BACK   X` (X reserved for drill-in
rows once they exist; plain rows only respond to left/right).

**Navigation:** BACK → Screen 2 (settings edited so far are kept in the
plan, not discarded — only leaving the whole wizard discards anything).
NEXT → Confirm.

### Screen 2B — Select Existing Randomizer Profile

Same list/detail/X-select/O-cancel shape as Screen 1A, browsing
`RandomizerProfile` entries instead of `SaveBackup` entries:

```
> Ludwig Chaos Run
  NG+ Nightmare
  First Playthrough
  Boss Rush-ish
```

Detail pane for the highlighted profile: seed, created date, and a short
settings summary (e.g. which major categories are ON/OFF).

- **X** → return to Screen 2, set `seedChoice = Existing`, record the
  profile, and show its name on that row instead of the generic option
  text.
- **O** → return to Screen 2, no change.

### Confirm (Enable)

Human-readable summary of the whole `EnableOperationPlan`:

```
CONFIRM ENABLE RANDOMIZER

SAVE DATA
  Backup existing save: YES
  Replace with: New save data

RANDOMIZER
  Profile: New Randomizer
  Seed: 839274
  Settings: 47 changes from defaults

THE APPLICATION WILL:
  1. Back up your current save data
  2. Create new Bloodborne save data
  3. Generate randomized game data
  4. Enable the Bloodborne Randomizer
  5. Save this randomizer configuration

  O CANCEL                              X CONFIRM
```

CANCEL returns to the Main Menu; the plan is discarded and nothing has
changed. CONFIRM begins execution — this is the one and only point of no
return in the whole wizard.

### Progress (Enable)

Non-interactive checklist, one line per user-meaningful phase (not the
finer-grained internal transaction steps from the architecture review —
those collapse into these five lines):

```
ENABLING RANDOMIZER

✓ Backing up save data
✓ Creating randomizer profile
● Generating randomized game data
○ Installing randomized game data
○ Final verification
```

### Result (Enable)

```
RANDOMIZER ENABLED

Profile: Ludwig Chaos Run
Seed: 839274

X — RETURN TO MAIN MENU
```

---

## Disable Randomizer

### Screen D1 — Save Data

**Restore save data:** one of three, radio-style:
1. `Select from backup` — selected by default → drills into **Screen 1A**
   (identical control to Enable's, same backup pool).
2. `New save data` — sub-text warning, same as Enable's.
3. `Leave existing save data`

**Navigation:** BACK → Main Menu. NEXT → Confirm.

### Confirm (Disable)

```
CONFIRM DISABLE RANDOMIZER

SAVE DATA
  Restore: Vanilla Save — Sep 1, 2026

RANDOMIZER
  Current profile: Ludwig Chaos Run
  Seed: 839274

THE APPLICATION WILL:
  1. Back up your current randomizer save
  2. Restore the selected save data
  3. Disable the Bloodborne Randomizer
  4. Preserve the randomizer profile for later use

  O CANCEL                              X CONFIRM
```

### Progress (Disable) / Result (Disable)

Same shape as Enable's, phases adjusted to match the Disable action list
above.

---

## Setup Defaults

Single screen, not a wizard — no Confirm/Progress/Result, just BACK / SAVE.

```
SETUP DEFAULTS

SAVE DATA
  Backup existing save: YES
  Replace with: New save data

RANDOMIZER DEFAULTS
  Enemy Randomization: ON
  Boss Randomization: ON
  Item Randomization: ON
  Weapon Randomization: ON
  ...

  O BACK                                 X SAVE
```

Uses the exact same row control as Screen 2A (boolean/numeric/drill-in),
minus the seed row (defaults have no seed - only a new profile does).
SAVE writes `RandomizerDefaults`; BACK discards edits.

---

## Decisions confirmed

1. **Enable/Disable while already in that state** — no special case. Both
   actions are always reachable regardless of current status.
2. **Randomizer-save backup on Disable** — automatic, not a user toggle.
   This supersedes the earlier architecture review's explicit yes/no step.

## Implementation approach

Same incremental, hardware-tested, stub-first discipline as every
milestone so far (M1–M6 and the current Enable-wizard slices): screens and
navigation get built and tested with fake/in-memory data well before any
real transaction, AFR, or save-data machinery exists underneath them. The
existing `EnableWizardScreen` (built against the *previous*, flatter
Vanilla-Save-choice spec) will be reworked to match Screen 1/1A exactly
rather than patched incrementally, since the new spec changes that
screen's shape (a separate backup toggle plus a 3-way replace choice,
instead of one flat 4-way list).
