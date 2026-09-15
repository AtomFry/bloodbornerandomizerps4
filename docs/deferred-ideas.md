# Deferred ideas

**Status: documentation only. Nothing here is authorized work, and no current
behaviour has been changed to address any of it.** These are observations worth
not losing, recorded so that a future investigation starts from evidence rather
than from memory. Each one deliberately stops short of choosing a solution.

Merged from three separate `future-*.md` documents on 2026-09-14. Two items that
were once in that set have since been resolved and moved: progress-screen
scrolling is now [plans/ui-scrolling.md](plans/ui-scrolling.md), and seed entry
is [plans/seed-entry.md](plans/seed-entry.md).

---

# Part 1 — Randomization behaviour

## 1. Caged enemy compatibility

Certain enemy spawn locations carry environmental or behavioural constraints
that make arbitrary randomization unsafe. Both the Windows reference tool and
this port treat every eligible placement as interchangeable: any enemy from the
global pool may be written into any non-excluded placement, subject only to a
model-size budget.

That assumption does not hold for placements that are physically confined or
that depend on surrounding level geometry, scripting, or navigation. When an
incompatible enemy lands in such a location, the result ranges from cosmetic
oddity to an enemy that cannot be killed.

### 1.1 The known case — caged dogs in Central Yharnam

**Observed in both implementations**, so not a port regression — consistent with
the two tools sharing the same placement-swap model and exclusion lists.

Direct in-game observations, recorded as reported. None has been traced to a
mechanism:

- A Maneater Boar appeared inside one of the cages.
- An Old Hunter appeared inside one of the cages.
- Some randomized enemies in these cages caused lag or abnormal behaviour.
- Some became effectively unkillable — the normal cage dogs die very quickly,
  whereas certain replacements survived many attacks.

### 1.2 Evidence gathered

- **Neither implementation has any cage-aware handling.** Case-insensitive
  searches across `reference/Randomizer/**/*.cs` and `app/src/` for `cage`,
  `caged`, `kennel`, `penned` return no matches. No exclusion, no compatibility
  check, no special case anywhere in either codebase.
- **The only `c1050`-specific handling in the reference is unrelated.**
  `RandomizeFunctions.cs:289-320` force-*includes* six specific placements
  (`c1050_0117`, `_0115`, `_0119`, `_0110`, `_0112`, `_0114`), but gated on
  `currentMap.Contains("m28")` — Yahar'gul, not Central Yharnam. Evidence the
  original author hand-tuned individual `c1050` placements in at least one map;
  **not** cage handling, and must not be mistaken for it.
- **The size filter does not prevent this.** Replacement is constrained only by
  a model file-size budget (`RandomizeFunctions.cs:379-466`;
  `kSizeToleranceMultiplier` in `EnemyRandomizer.cpp`). A Maneater Boar passing
  that budget is expected — the filter has no notion of physical space,
  navigation, or scripted death conditions.

**Not established:** which MSB placements are the caged dogs, whether the cage
is an Object part / collision / region / scripted event, and whether
"unkillable" means true invulnerability or merely very high HP from a
mis-scaled `NPCParamID`.

### 1.3 Candidate approaches — recorded, not chosen

**Approach A — blacklist.** Identify the constrained placements and either
exclude them from randomization entirely, or exclude known-incompatible enemy
types from being selected for them. Cheap, and fits the existing architecture
directly: the port already has a name-substring exclusion mechanism
(`EnemyExclusionList.h` / `IsExcludedEnemyName`) a placement blacklist could
reuse nearly as-is. Costs a little variety. Treats the symptom, which is also
its weakness.

**Approach B — compatibility list.** Define, per constrained location or class
of location, the set of enemy types known to work there. Preserves more variety
and degrades gracefully as new constrained locations are found. Substantially
more work: a per-location data table, a way to attach it to specific placements,
and empirical per-enemy verification to populate it. Risks becoming the same
kind of large hand-maintained dataset that already makes the reference tool's
exclusion lists hard to reason about.

**Worth keeping open — a property-based rule.** Derive compatibility from model
size, navigation type, or AI class rather than enumerating placements. Would
generalize without a hand-maintained matrix, *if* such a property turns out to
correlate with the failures.

### 1.4 Questions the investigation must answer

1. **How are cage locations represented in the game data?** Object part,
   Collision part, region, `ObjAct` event, or a combination? Are the caged dogs
   distinguishable in the MSB from ordinary placements — by name prefix,
   `EntityID`, `collisionPartIndex` in the enemy's type data, or only by
   position?
2. **What actually causes the failure?** None ruled out: cage collision geometry
   trapping an oversized model; navmesh absence preventing AI activation; the
   replacement's `ThinkParamID` expecting navigation that isn't there; a
   scripted death/opening condition attached to the original dog's `EntityID`;
   or `BossParamScaling` assigning a scaled `NPCParamID` with far higher HP.
3. **Is "unkillable" literal?** Genuine invulnerability, or just very high HP
   from a scaled `NPCParamID`? Completely different fixes, and the distinction
   is cheap to establish from generated data plus one in-game test.
4. **Does it affect all non-dog enemies or only some?** Both named observations
   (Maneater Boar, Old Hunter) are large. Whether small enemies behave correctly
   in the cages is **the single most useful data point** for choosing between A
   and B.
5. **Are there other constrained locations?** Likely candidates to survey: other
   confined or scripted spawns, enemies in windows and doorways, enemies on
   narrow ledges, and anything whose vanilla occupant never moves.
6. **Is a compatibility matrix ultimately more appropriate than a blacklist?**
   Answer from the outcome of (4) and (5), not up front.
7. **How does the reference handle these?** Established: **it does not.** The
   open part is whether its community documented a workaround (e.g. on the Nexus
   Mods page linked from the repo README) worth adopting rather than re-deriving.

### 1.5 Cheapest first step

Identify the placements before any design work. Parse Central Yharnam
(`map/mapstudio/m24_01_00_01.msb.dcx`, plus `m24_01_00_00` and `m24_01_00_11`)
from a clean vanilla tree, list every `Part.Enemy` whose model is the cage dog,
and record each one's `Name`, `EntityID`, `NPCParamID`, `ThinkParamID` and
`collisionPartIndex`. Cross-reference against the `Object` parts in the same map
to see whether a cage object is co-located.

If those placements share a distinguishing field, Approach A becomes nearly
free. If they are indistinguishable except by position, **that finding is itself
important** — it would mean any fix needs a hardcoded placement list, raising the
cost of both approaches and making the property-based rule more attractive.

---

## 2. Even enemy mix — flattening the pool weighting

Noticed while planning the enemy picker ([plans/pickers.md](plans/pickers.md)),
which is what made it visible for the first time. Deliberately *not* part of that
feature: the picker adds a choice, this would change what `RANDOMIZE ENEMIES`
does for everyone.

### 2.1 The observation

The enemy pool is not one entry per creature. It is one entry per distinct
**stat/AI variant** — a `(NpcParamID, ThinkParamID, model)` triple — and every
entry is an equally likely draw. A creature the game reuses with many variants
is therefore picked far more often than one that appears once.

Measured against `data/vanilla/dvdroot_ps4`, replaying
`EnemyRandomizer.cpp`'s own pool construction: **333 entries across 82 models.**

| Entries a model has | How many models |
|---|---|
| 1 | **29** |
| 2 | 15 |
| 3 | 9 |
| 4 | 8 |
| 5–13 | 20 |
| **41** | **1** (Huntsman (Transformed)) |

Consequences, all measured:

- **Huntsman (Transformed) alone is 12.3% of every replacement rolled.**
- **13 of the 82 models account for half of all draws.**
- 29 models are worth **0.30%** each — roughly one appearance per three hundred
  randomized placements.

### 2.2 What flattening would mean

Draw in two stages: pick a **model** uniformly from those enabled, then pick a
variant within that model.

| | Today | Flattened |
|---|---|---|
| Any given model | 0.30% – 12.3% | **1.22%** each |
| Huntsman (Transformed) | 12.3% | 1.22% — **10x rarer** |
| A single-variant creature | 0.30% | 1.22% — **4x more common** |

The character of a run would change noticeably. Today's runs are dominated by a
handful of common Yharnam enemies, which arguably reads as "Bloodborne,
shuffled". Flattened, rare creatures show up as often as the mooks — "everything,
everywhere". A different thing, not obviously a better one.

**Stats would still vary.** Flattening changes only which *creature* is drawn;
the variant chosen within it still carries its own NpcParam and ThinkParam, so
two flattened Huntsmen differ exactly as they do now.

### 2.3 This is a deviation, not a bug fix

**The reference tool has the same weighting**, for the same reason — it builds
the same list of triples and draws uniformly. So flattening is a deliberate
deviation and belongs on the record as one.

It is also not clear the weighting is *wrong*. A creature with many variants is
usually one the game uses heavily, so the weighting loosely tracks "how much of
Bloodborne is this enemy" — a defensible thing for a shuffle to preserve.

### 2.4 Recommended shape, when it happens

**Do it as a setting, not a behaviour change.** `EVEN ENEMY MIX`, default No.

- Nobody's existing runs change, and the same seed keeps producing the same
  world unless it is turned on. That matters more than usual here: seeds are
  shareable, and silently changing the draw would invalidate every seed anyone
  has written down.
- It offers the two genuinely different flavours in §2.2 as a choice rather than
  picking one on the player's behalf.
- Same defaults/store/two-screens/options plumbing as every other toggle.

**Where the change goes:** `DrawCandidate()` in `EnemyRandomizer.cpp`. Today it
indexes `pool` directly; flattened, it would index a per-model bucket list built
once in `StepBuildPool`. Roughly twenty lines, entirely inside the draw, with no
change to the size gate, the per-zone chance, or anything downstream.

**Interaction with the picker:** they compose cleanly. The picker decides *which*
models are in play; this decides *how* they are weighted among themselves.
Flatten over the enabled set only — `1/enabled_count`, not `1/82`.

**Verification:** a run with the setting on, over a large sample of placements,
should show model frequencies within sampling noise of uniform — something
`tools/enemy_lookup.py diff` could report directly, since it already walks every
changed placement.

### 2.5 Open question

**Should the picker's UI show the weighting?** Once both exist, a player
unticking "a dozen things I'm sick of" has no way to know that unticking the
Huntsman does more than the other eleven combined. A share percentage per picker
row would tell them — and would become meaningless the moment Even Enemy Mix is
on, since every enabled row would read the same number. Not resolved.

---

## 3. Randomly hide model parts for visual variety

**Carried over from the original README's "Personal Notes" when it was rewritten
2026-09-15. Idea only — nothing investigated.**

When randomizing, randomly switch off individual model parts on an enemy. The
worked example: Garden of Eyes wear clothes, so you could spawn two visually
distinct versions of the same creature — one clothed, one not — from a single
pool entry.

Attractive because it multiplies apparent variety without needing new creatures,
and because [§2](#2-even-enemy-mix--flattening-the-pool-weighting) shows variety
is genuinely limited: 29 of 82 models are worth 0.30% of draws each, and the
size gate leaves some placements with fewer than 20 candidates.

**Completely unexamined.** Nothing is known about whether model part visibility
is addressable from the data this project already touches. The randomizer works
in MSB placements and PARAM rows; part visibility is more likely a property of
the FLVER model or the `chr` archives, neither of which this port parses at all.
That makes this potentially much larger than it sounds — establish where part
visibility actually lives before treating it as a feature.

---

# Part 2 — New capability

## 4. Merge other mods into the output

**Raised 2026-09-14. Not designed, not scoped — recorded so the shape is not
re-derived later.**

The idea: let the user drop third-party Bloodborne mods alongside the
randomizer, show one toggle per mod, and merge the enabled ones into the output.

### 4.1 Why this is cheap

A mod for this game is **already the same shape as everything else here.**
`app/tools/data/mods/no-logo/` is the committed example: three `.gfx` files
under `dvdroot_ps4/menu/`. The vanilla source is a `dvdroot_ps4` tree, the AFR
output is a `dvdroot_ps4` tree, and a mod is a partial `dvdroot_ps4` tree.

So merging is an **overlay onto the AFR output after randomization** — copy the
mod's files over the top, last. That is the mirror code path `FileIo` already
runs at the start of every job, pointed at a different source and run at the
other end.

### 4.2 Where mods live

**On the console:** `/data/bbrandomizer/Mods/<name>/dvdroot_ps4/`, parallel to
the existing `VanillaSource/`. The app enumerates that directory and builds the
toggle list from what it finds, so adding a mod is an FTP copy — no rebuild, no
hardcoded list.

**Not in the `.pkg`.** Same reasoning that keeps vanilla data out: package size
and build iteration. A logo mod is 4 KB but a texture pack is hundreds of MB.

**In the repo:** `app/tools/data/mods/` holds small fixtures only, for
developing and verifying the merge. Real mods belong in the gitignored
`../data/`.

### 4.3 Open questions

1. **Conflict with randomization.** A mod that ships `map/mapstudio/*.msb.dcx`
   or `param/gameparam/*` would overwrite exactly what the randomizer just
   wrote. Merging last means the mod wins and the randomization is silently
   discarded for those files. Options: refuse such mods, warn, let the user
   order them, or merge before randomizing instead. **This is the real design
   question** — a logo mod is trivial precisely because it touches nothing the
   randomizer touches.
2. **Mod-versus-mod conflicts** when two enabled mods ship the same file.
3. **What counts as a mod directory?** Presence of `dvdroot_ps4/` is the
   obvious test. Some Nexus downloads nest it under a version folder
   (`5.2/dvdroot_ps4`), so a shallow hunt may be needed.
4. **Display names.** Directory names off Nexus are like
   `no-logo mod-10-1-0-1629746408`. The 8x8 font is uppercase and digits only
   (CLAUDE.md §5), so names need cleaning before they can be shown at all.
5. **Persistence.** Which mods are enabled has to survive in `defaults.cfg`, but
   the set of installed mods can change between runs — so it cannot be a
   positional bit string the way the enemy picker's selection is. Name-keyed.
6. **Does the progress screen report it?** Probably one line per merged mod.

### 4.4 Not yet decided

Whether this is one setting with a drill-in (like the enemy picker) or something
that lives on its own screen. Defer until the conflict question in (1) is
answered, because that determines whether merging is a simple toggle or needs
ordering and conflict resolution in the UI.

---

# Part 3 — UI and UX

## 5. Progress screen appears frozen during the item-data phase

When enemy drops is enabled, the run decompresses a 28 MB archive inside a
single `Step()` call. The frame loop cannot draw during that, so the screen sits
motionless for several seconds on `RANDOMIZING ENEMY DROPS`.

Not a hang, and the phase machine handles everything else in small steps — but
it is indistinguishable from a crash to anyone watching.

Options: split the decompression into chunks (awkward — `puff.c` is monolithic),
or show an explicit "this step takes a while" hint before entering it.

---

## 6. Save Data backup and restore are still simulated

`StartCommit` prints `BACKING UP EXISTING SAVE (SIMULATED)`, `REMOVING EXISTING
SAVE DATA (SIMULATED)` and `RESTORING SAVE DATA … (SIMULATED)`, and the backup
entries on the replace-save screen are stubs with hardcoded dates. Nothing
touches real save data.

Deliberately deprioritized in favour of the randomizer path, but **it is the one
remaining place where the UI claims to do something it does not.** Worth doing
before anyone runs this against a save they care about.
