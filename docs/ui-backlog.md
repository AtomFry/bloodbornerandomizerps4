# UI and platform backlog — inventory and status

The running list of work items that are **not randomization settings**: screens,
navigation, presentation, packaging and platform behaviour. It is the sibling of
`randomization-feature-spec.md`, which stays authoritative for anything derived
from the reference tool's settings list.

**Why this file exists.** `docs/features/README.md` has been recording that
platform/UI items have no backlog row to be numbered after, and that three such
folders (`font-atlas/`, `randomizer-settings-ui/`, `worlds/`) had crossed the
stated threshold for giving them one. This is that backlog. Existing unnumbered
folders keep their names; new items get a `U`-prefixed row.

## Status legend

Same meanings as the randomization backlog, so the two read alike:

| Label | Meaning |
|---|---|
| **DONE** | Implemented, on hardware, confirmed working |
| **BUILT** | Implemented and green off-console, **not yet hardware-confirmed** |
| **TODO** | Not implemented |
| **IDEA** | Captured, not yet specified — no commitment that it ships |
| **OUT** | Deliberately out of scope |

---

## 1. Shipped

| # | Item | What it did | Status | Folder |
|---|---|---|---|---|
| U0a | Font atlas | Replaced the 8×8 bitmap font with an EB Garamond atlas | **DONE** | [features/font-atlas/](features/font-atlas/) |
| U0b | Randomizer Settings UI | The six-category settings screen; re-modelled Setup Defaults | **BUILT** — none of the four milestones hardware-tested | [features/randomizer-settings-ui/](features/randomizer-settings-ui/) |
| U0c | Worlds | Named playthroughs with their own save data; retired the Enable/Disable wizards | **DONE** — hardware test passed in full 2026-09-25 | [features/worlds/](features/worlds/) |
| U0d | Startup screen | Replaced the streaming boot log with a loading screen | **BUILT** — awaiting hardware test | [features/startup-screen/](features/startup-screen/) |

U0d is the **precedent for U1 and U2 below**: same complaint (a screen that
reads as a log), same intended fix (state what is happening, not every step
taken). Read its spec before specifying either.

---

## 2. The screens that read as logs — both done (0 open)

| # | Item | What it does | Status |
|---|---|---|---|
| U1 | **Simplify the activation screens** | Replaced the streaming per-phase log with the startup screen's loading shape, and success now hands straight to the `WORLDS` tab instead of a result screen | **BUILT** 2026-09-26 — **not yet hardware-tested**. See [features/U1-activation-screens/](features/U1-activation-screens/) |
| U2 | **Condense the activation confirmation** | Replaced the 27-row scrolling review with two fixed tiers — five rows of what is about to happen, three of what this world is — that never scroll | **BUILT** 2026-09-26 — **not yet hardware-tested**. See [features/U2-confirm-screen/](features/U2-confirm-screen/) |

Both were built directly from these rows, without a spec or a plan, at the
developer's request. Each folder holds an `implementation-report.md` and nothing
else.

**U1's shaping decision.** Full startup-screen parity over a condensed result
screen: a successful activation draws nothing and returns to the rail, and the
generation's counts live in `live.log` alone. `Step::Problem` is the only state
that holds, and it keeps the log, because a refusal and a failure are the one
case where the detail is what the player is there for. The three-line closing
flourish (`A HUNTER MUST HUNT`) had nowhere left to live and was removed — if it
is wanted back, the `WORLDS` rail is the place for it.

**U2's shaping decision.** Of the four options put to the developer, the chosen
one was the two-tier split **with no expansion** — so the per-setting list is
gone from that screen entirely rather than collapsed behind a drill-in. The
tension the row recorded ("keep all the information" versus "never scroll") was
therefore resolved in favour of never scrolling: the screen states the
consequence, and the recipe is a count.

**The startup bar was changed to match the activation bar** in the same pass
(2026-09-26), replacing its five discrete cells with one continuous fill. Two
loading screens in one session that disagree about what a bar looks like is a
seam the player can see, and removing that seam is why U1 copied the startup
geometry to the pixel in the first place. The five-stage machine behind it is
unchanged, so the fill advances in fifths.

### U2 — the tension to resolve in the spec

The developer's two requirements pull against each other: *keep all the
information* and *never scroll*. At the current row count those cannot both hold
literally, so the spec has to choose how the information compresses. Candidates,
none chosen:

- **Non-default only.** List the settings that differ from vanilla defaults and
  summarise the rest as a count. Makes the common case short and the unusual
  case honest.
- **Counts with drill-in.** One line per category (`ENEMIES — 4 of 9 on`), with
  the full list still reachable by opening the category.
- **Grouped ticks.** A compact grid of on/off marks rather than a label/value
  row each.

Whichever wins, the screen must still state the things that are not settings and
cannot be summarised away: which world, which seed, whether the save is kept or
started fresh, and any refusal or store error (`storeError_`).

---

## 3. Open — new screens wanted by randomization rows (1)

| # | Item | What it does | Status | Cost |
|---|---|---|---|---|
| U3 | **Per-map enemy pool sub-screen** | The front-end for randomization row 35. Reuses the `ModelPicker` component behind the three existing drill-ins, but adds a map dimension: pick a map, then pick that map's pool. Needs a per-map override state distinct from "inherits the global list", or the screen cannot show the difference | **IDEA** — blocked on row 35's spec | Medium. The picker exists; the map axis, the override state and its config encoding do not |

Randomization row 17 (per-zone boss toggles) also wants a sub-screen, and the
randomization backlog has said for a while that it does. If U3 and row 17 are
built near each other, they should share one map/zone-axis component rather than
growing two.

---

## 4. Related

- `randomization-feature-spec.md` — the randomization settings backlog; rows 35
  and 36 are the two new randomization ideas captured alongside U1–U3
- `features/README.md` — artifact index; authority on document state, not feature state
- `screen-text-inventory.md` — generated per-string character, word and wrapped-line
  counts for the WORLDS and DEFAULTS screens. The measuring tool for U1 and U2;
  extend it to the Confirm and Progress steps as part of that work
- `deferred-ideas.md` — ideas deliberately declined; check before proposing more
- `plans/ui-blueprint-wizards.md` — **frozen and retired**, describes the Enable/Disable
  wizards removed on 2026-09-24. Not current UI
