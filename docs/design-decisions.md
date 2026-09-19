# Design Decisions

This document records deliberate behavioral decisions, rejected approaches, and the reasoning behind them.

## Reference fidelity

The Windows reference tool is the behavioral baseline for the PS4 implementation.

"Improving" behavior without comparing it against the reference has repeatedly produced unintended compatibility differences. New behavior should therefore be treated as a deliberate design change rather than an automatic improvement.

### Enemy exclusions

`EnemyExclusionListExtra` previously caused 119 placements to become frozen. It was removed so the PS4 implementation could match the reference exclusions exactly.

Do not restore the list wholesale.

If an individual exclusion is later determined to be necessary, add it deliberately with a documented reason. See `docs/enemy-exclusion-history.md`.

### ThinkParamID filtering

Applying the `ThinkParamID <= 1` test symmetrically remains a parked idea.

It is intentionally unimplemented because doing so would introduce behavior that differs from the reference.

Do not implement it without an explicit decision to change the compatibility behavior.

## Divergences from the reference, taken deliberately

The reference has no per-creature "leave this one alone" list. `ENEMIES SKIPPED`
(feature 032) is the port's own setting, and three decisions inside it depart
from reference behaviour on purpose. All three were the developer's, are
hardware-tested, and are documented in `docs/user-guide.md`.

### A starved selection completes instead of failing

When every creature `ENEMIES INCLUDED` allows is also ticked in
`ENEMIES SKIPPED`, the candidate pool is empty. The run does **not** error: the
pool half of the skip instruction yields for that run and the run draws from
the selection, while the skipped placements stay frozen. The screen says so.

This is spec 032 D4 and it is the **one** case where a skipped creature appears
somewhere new. The reference's equivalent is to randomize nothing; this was
chosen over that.

Do not restore the failure. The error it replaced fired *after* the mirror
phase had copied the whole game, leaving a half-built output tree and naming a
cause the player could not act on.

### An unreadable vanilla source must still fail, and differently

The empty-pool error used to do two jobs. Separating them was a prerequisite
for the change above, not a tidy-up: without it, a broken installation reports
a successful run that randomized nothing. Three causes, two messages:

| Cause | Outcome |
|---|---|
| No map could be read | `VANILLA SOURCE UNREADABLE - NO MAPS FOUND` |
| Maps read, no candidate at all | `NO ENEMIES FOUND IN THE VANILLA MAPS` |
| Everything selected was also skipped | the run completes (D4) |

The two messages must stay distinct strings, and both must stay inside
`Font8x8.cpp`'s 42-character set — there is no lowercase and no `:`, and an
unrenderable character draws as a full-width blank column, so a message can
meet every width budget and still show the player nothing.

### `ENEMIES SKIPPED` does not filter the boss pool

`c2090` (Blood Starved Beast) and `c2710` (Father Gascoigne) are in both
`BossPoolTable.h` and the 85-row skip table. With `RANDOMIZE BOSSES` on,
ticking either here still leaves a boss arena able to become one; their
ordinary overwritable placements freeze as promised.

Accepted and documented rather than fixed, and observed on hardware. Changing
it means passing the skip selection into `BossRandomizer` and applying it at
its pool build — a contained change, but a change to what the feature *does*,
so it needs a spec decision first.

## Chalice dungeons

Chalice dungeons are intentionally out of scope.

Three randomizer settings are affected by this decision. Do not expand those settings to support Chalice dungeons unless the scope decision is explicitly revisited.

## Reference quirks

Some reference behavior is intentional compatibility behavior even when it appears unusual.

### `m21_00_00_00`

This map is never randomized. It functions as a free in-game control group and is intentionally preserved.

### `m21_01_00_00`

This map appears twice in the reference's treasure randomization list. The duplicate is intentionally preserved.

These behaviors are encoded as invariants in `treasure_verify.py`.

When implementing or refactoring randomization logic, preserve these quirks unless the compatibility behavior is explicitly being changed.
