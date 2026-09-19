# Feature 033 — Protect Caged Dogs

*The work item was named "Protect Central Yharnam Caged Dogs" until the
Forbidden Woods cages were measured (§4 F13–F15). The setting a player sees is*
***DO NOT RANDOMIZE CAGED DOGS*** *(§10 D7), which was already area-neutral.*

**Status: APPROVED**

**Backlog row:** `docs/randomization-feature-spec.md` §3, row **33**, and its
narrative subsection "Row 33 — the caged dogs, and why row 32 does not cover
them". The developer's full draft feature request, which predates the backlog
row, is reproduced verbatim in `log.md` and is this spec's primary input. Where
this spec departs from either, §4 says why.

**Reference:** **New capability.** The reference tool has no placement-level
protection a player can turn on, and it shows the same problem — see §3.

**Plan:** `docs/features/033-protect-caged-dogs/plan.md` *(once stage C has run)*

---

## 1. What this feature does

Two places in the game shut dogs in cages. In Central Yharnam there is a kennel
yard: a walled courtyard of wooden cages with dogs inside them, a couple of
which get let out at you as you pass. In the Forbidden Woods there is a cluster
of six cages, four of them with a dog inside and two standing empty.

The dogs in those cages are not ordinary placements. The game runs a small
script for each one that holds it in its cage and releases it on cue, and that
script is attached to *the spot*, not to the dog.

**RANDOMIZE ENEMIES** does not know that. It treats every eligible spot in the
game as interchangeable, subject only to a rough model-size budget, so those
cages can end up holding anything the seed draws — and in Central Yharnam the
results have been bad. A Boom Hammer Hunter and a Maneater Boar each dropped the
frame rate severely; other replacements could be hit but not killed. The
original dogs are fine, and the same creature behaves normally everywhere else
in the game.

This feature is a **placement** protection, not an enemy one. Switch it on and
the dogs in those particular cages keep their vanilla identity. Every other dog
of the same kind — loose in the same courtyard, elsewhere in Central Yharnam,
elsewhere in the Forbidden Woods, and in the two other areas the game puts them
— carries on being randomized, and the creature itself carries on being used as
a replacement everywhere.

That distinction is the whole point. **ENEMIES SKIPPED** (row 32) already
answers "leave this creature alone", but it is keyed on the creature: ticking
its row freezes all ninety-six of its spots across the game and takes it out of
the replacement pool. What is broken here is ten spots, not a creature, and this
setting is the narrow tool for that.

It does not claim to fix the underlying incompatibility, and it does not touch
the replacement creatures that misbehaved; it removes those spots from the draw
and leaves everything else alone.

---

## 2. What does the player experience?

One new on/off setting, off by default, on the same settings list as
**RANDOMIZE ENEMIES**. It only means anything when **RANDOMIZE ENEMIES** is on.

Counts below are what a player walking the game would find; §4 holds the
measurements and separates them from the duplicate map variants.

| Setting | Off | On |
| --- | --- | --- |
| **DO NOT RANDOMIZE CAGED DOGS** | Today's behaviour. All **forty-two** Shaggy Hunting Dogs a player meets are fair game, the ten in cages included, and a cage can end up holding any creature the seed draws — including ones that have lagged the game badly or could not be killed | The **ten** dogs shut in cages — **six** in the Central Yharnam kennel yard, **four** in the Forbidden Woods cage cluster — stay Shaggy Hunting Dogs and behave as they do in the unmodified game. The other **thirty-two** still change: the **six** loose Central Yharnam dogs (**including the seventh dog standing in the same kennel yard**, §10 D5), the **four** loose Forbidden Woods dogs, and all **twenty-two** in Cathedral Ward and Yahar'gul. The creature is still drawn as a replacement across the whole game |

Two further things a player can check in the arena, and neither changes with the
setting: the Forbidden Woods cluster has **six** cages, of which **two are empty
in the unmodified game** and stay empty; and the Central Yharnam yard holds a
**seventh** dog that is in no cage and is not protected (§10 D5).

**What "keeps its vanilla identity" means precisely.** A protected dog is not
re-targeted: it stays the same creature with the same behaviour. Its stats are
still re-tuned for the zone by the existing scaling pass, exactly as a creature
ticked in **ENEMIES SKIPPED** is today (§4 F11; confirmed by §10 D8).

**Combined with the other settings.**

* **With `ENEMIES SKIPPED`, the wider setting wins and the two do not fight.**
  Ticking **SHAGGY HUNTING DOG** there already freezes all ninety-six of that
  creature's spots — the cages among them — and removes it from the replacement
  pool. This setting never weakens that; it only adds protection where
  **ENEMIES SKIPPED** is not applying it. Note the row to tick is
  **SHAGGY HUNTING DOG**, not **HUNTING DOG**: they are different creatures and
  the one in every cage in the game is the shaggy one (§4, F1).

  | `ENEMIES SKIPPED` has the creature ticked | This setting | The ten caged dogs | The other 70 spots of that kind | The creature as a replacement elsewhere |
  | --- | --- | --- | --- | --- |
  | No | Off | Randomized | Randomized | Used |
  | No | On | **Kept** | Randomized | Used |
  | Yes | Off | Kept | Kept | Not used |
  | Yes | On | Kept | Kept | Not used |

* **With `ENEMIES INCLUDED`, nothing changes.** That list decides what may be
  drawn as a replacement; this one decides which spots are drawn for at all.
* **With `RANDOMIZE BOSSES`, nothing changes.** No boss placement is among the
  protected set.
* **With `RANDOMIZE ENEMY DROPS`, nothing changes.** A protected dog still
  drops randomized loot; drops follow the creature, not the spot.
* **The seed's meaning changes when you turn it on.** Taking these spots out of
  the run removes rolls from the sequence, so the same seed with this setting on
  is a different game everywhere, not the same game with ten dogs pinned. That
  is true of every exclusion setting the port has and is recorded here so it is
  not later mistaken for a defect. With the setting **off** the run is unchanged
  from today, roll for roll.

**Whether this actually cures the lag and the unkillable replacements can only
be settled on hardware.** What the setting is specified to do is stop those
spots being re-targeted; that the observed symptoms then go away is the
expectation, not a proven consequence (§8, and §4 H1). Note also that the
symptoms have only ever been reported in Central Yharnam; the Forbidden Woods
cages are included on structural evidence rather than on an observation
(§4 F14). The developer confirmed their inclusion on that basis (§10 D9).

---

## 3. What does the existing randomizer do?

**Nothing, in either implementation, and both show the problem.**
`docs/deferred-ideas.md` §1.1 records the same failure observed in the Windows
reference tool as well as in this port, which is expected: the two share the
same placement-swap model and the same exclusion lists.

The reference tool's three nearest relatives are all the wrong shape:

* `excludeEnemiesBool`
  (`reference/Randomizer/MainWindowComponents/StartFunctions.cs:536-563`)
  removes models from the candidate pool. It is keyed on the model and it does
  not protect placements at all — those models' own spots are still overwritten.
* `unusedPlusBossList` is a fixed, hard-coded list of name substrings that does
  protect placements, but it is not user-facing and is transcribed into this
  port as `EnemyExclusionList.h`. None of its entries match any dog.
* The only *placement-specific* hand-tuning in the reference
  (`RandomizeFunctions.cs:288-320`) forces six named Yahar'gul chime-maiden
  placements **into** randomization past every gate. It is evidence that the
  original author hand-picked individual placements when a map needed it, but it
  is the opposite operation and it is gated on a different map.

`docs/deferred-ideas.md` §1 already anticipated this feature, listed it as
"Approach A — blacklist", and set out in §1.5 the exact measurement this spec
performs. That document is explicitly not authorization; backlog row 33 is.

---

## 4. What do we know?

Measured against `data/vanilla/dvdroot_ps4` by reusing `app/tools/enemy_lookup.py`
(`load_base_maps`, `enemies`, `engine_pool`, `overwritable_placements`) and
`app/tools/boss_verify.py`'s MSBB reader, so no new map parsing was written.
Script data was read from the same tree's `event/m24_01_00_00.emevd.dcx` and
`event/m27_00_00_00.emevd.dcx` with a throwaway EMEVD reader whose framing was
validated two independent ways **for each file**: the header's section offsets
reconcile exactly, and the per-event instruction counts sum to the file's own
instruction total. `tools/enemy_lookup.py pool` was **not** trusted for pool
figures; `engine_pool()` was, because it is the line-for-line mirror of the
engine's own contribution loop and is what `pool_verify.py` uses.

**F1 — fact, and it contradicts the backlog row and the draft: the creature in
every cage in the game is the Shaggy Hunting Dog, not the Hunting Dog.** The
plain Hunting Dog has **14** placements in the entire game and **all 14 are in
Hemwick Charnel Lane**; it appears in neither Central Yharnam nor the Forbidden
Woods. This was re-measured for this revision, not cited from the earlier pass.
The creature in both sets of cages is the Shaggy Hunting Dog, which has **96**
placements across nine map files — Cathedral Ward, Central Yharnam, the
Forbidden Woods and Yahar'gul. The randomizer's own **ENEMIES SKIPPED** list
names them `HUNTING DOG` and `SHAGGY HUNTING DOG` respectively. This matters
beyond pedantry: a player told to tick "Hunting Dog" instead of using this
setting would tick the wrong row and nothing would happen. Three further
dog-shaped creatures were checked and none is caged or in either area: the
Keeper's Hunting Dog has **no** placement in any base map, the crow-faced Shaggy
Hunting Dog has **4** and all are in Nightmare of Mensis, and the Deep Sea Hound
has **6** and all are in the Fishing Hamlet.

**F2 — fact: Central Yharnam is three map files holding an identical set of
twelve dogs.** The randomizer writes `m24_01_00_00` (loaded by the base game),
`m24_01_00_01` (loaded with the DLC installed) and `m24_01_00_11` (never
loaded). All three hold the same twelve dog placements with the same names,
entity IDs, stat rows and behaviour rows, in the same order. A player sees
twelve; the map data holds thirty-six. The protection has to be applied to all
three, because which one the game loads depends on the player's install.

**F3 — fact: in Central Yharnam, exactly six of the twelve are wired into one
shared cage script, and nothing else in the map is.** Central Yharnam's script
file initialises one parameterised event **six times, once per dog, and never
for anything else**:

| Placement | Entity ID | Stat row | In the kennel yard |
| --- | --- | --- | --- |
| `c1240_0004` | 2410271 | 124400 | yes |
| `c1240_0005` | 2410272 | 124400 | yes |
| `c1240_0008` | 2410275 | 124401 | yes |
| `c1240_0010` | 2410277 | 124401 | yes |
| `c1240_0011` | 2410278 | 124401 | yes |
| `c1240_0012` | 2410279 | 124401 | yes |

Two further parameterised events split the same six: one is initialised
**exactly twice**, once for `c1240_0004` and once for `c1240_0005`; the other
**exactly four times**, once for each of the remaining four. Every one of those
instructions addresses the dog by its **entity ID**.

**F4 — fact: the four in the second group each stand on top of a cage prop, and
carry a stat row that exists nowhere else in the game.** Each of `c1240_0008`,
`_0010`, `_0011` and `_0012` is within **0.3 units** of a distinct instance of
one cage model; no other dog in the map is nearer than 1.6 units to one. All
four use stat row `124401`, and a sweep of all twenty-four base maps finds that
row on **no other placement anywhere**. These are the permanently penned dogs.

**F5 — fact: the other two are the ones that can get out.** `c1240_0004` and
`c1240_0005` each stand within 0.9 units of one of only **two** instances of a
*different* cage model, and those two cage objects are the only cage objects in
the map that both carry an entity ID and are referenced by the script. The
two-instance event in F3 is initialised once per (dog, cage-object) pair and is
the only place those object IDs appear. The six-instance event of F3 is given a
third argument for these two dogs and zero for the other four. So the
"breakout" dogs **are** represented differently from the penned ones — by which
script instances they belong to and by the cage model beside them, but **not**
by any field in their own placement record.

**F6 — fact: a seventh dog stands in the same yard, in no cage and in no cage
script.** `c1240_0007` sits on the same collision surface as the six (the
kennel-yard floor: seven enemy placements stand on it, all of them dogs), but it
is **2.7 units** from the nearest cage prop — roughly ten times the four penned
dogs' distance — and it is not an argument to any of the three events. This is
almost certainly the source of the draft's "approximately 6–7". **§10 D5 leaves
it unprotected.**

**F7 — fact: no single field in the map data isolates the protected set, and
only an entity-ID list gets it exactly.** Every 4-byte field of all twelve
Central Yharnam dog records was compared; none has one value shared by the six
and by nothing else, and adding the Forbidden Woods four does not change that
conclusion. The candidate schemes, each measured across all twenty-four base
maps:

| Scheme | What it catches | Verdict |
| --- | --- | --- |
| Entity ID, ten values | **26 placements — the six Central Yharnam dogs in each of that area's three map files, the four Forbidden Woods dogs in each of that area's two, and nothing else in the game** | Exact. Entity IDs are unique within every map the randomizer touches (one unrelated duplicate pair exists, in Nightmare Frontier) |
| Stat rows `124401` + `124501` | 20 placements — four Central Yharnam dogs × three files, four Forbidden Woods dogs × two files | Exact but **incomplete**: misses the two Central Yharnam breakout dogs. Note the asymmetry — `124501` alone *is* exact for the Forbidden Woods four (§4 F14), which is why "use the stat row" is a tempting and wrong generalisation |
| Placement name as a substring, the way the existing gates match | **58 placements across nine map files** — Cathedral Ward, Yahar'gul and the unprotected half of the Forbidden Woods and Central Yharnam all reuse the eight names in question for unrelated dogs | **Wrong on its own.** Would freeze 32 placements it must not touch unless combined with a map test. (The earlier Central-Yharnam-only form of this row said "44 placements across six maps"; 44 was right for the six-name set, "six maps" was a slip for nine map files) |
| Kennel-yard collision surface | 21 placements — seven dogs × three files, all of them dogs | Exact for the seven-dog Central Yharnam set, not for the six. **§10 D5 chose the six, so this is not the scheme to use.** It also does not generalise: the Forbidden Woods cages sit on a surface shared with eleven enemies of four different creatures (§4 F14) |
| Position clustering | The set, with hand-chosen radii | Works, and is the most fragile of the five; nothing pins it |

**So the honest answer is a hard-coded list of ten identifiers.** It is derived
from the data rather than guessed, it catches exactly the intended placements and
nothing else, and it is checkable by a verifier that recomputes it from the
vanilla tree. The map data offers no property that means "this dog is in a cage".

**F8 — fact: randomization keeps the entity ID, and that is why the script stays
attached.** When a placement is re-targeted the port rewrites three things — the
model, the behaviour row and the stat row — and nothing else
(`EnemyRandomizer.cpp`, the placement write). The placement's name, entity ID,
position and collision surface are left as the game shipped them. The cage
script therefore goes on addressing that entity ID and now drives whatever
creature was written in.

**H1 — hypothesis, not established: that inheritance is what breaks the cages.**
F3 and F8 together say a replacement dropped into one of these spots inherits a
script written for a dog — one that holds it in place and releases it. A
creature that does not respond to those instructions as a dog does is a
plausible cause of both reported symptoms. **This has not been traced, and it is
not why the feature is specified this way**; the feature is specified on the
observation that the Central Yharnam spots misbehave. Anyone tempted to treat H1
as settled should read the row 8 cautionary tale in the backlog's §2 first.

**F9 — fact: the existing size gate is no protection here, so this cannot be
left to fix itself.** A replacement is rejected only when its model is more than
twice the original's size. The dog's model is 7,031,500 and the Maneater Boar's
is 1,314,801, so the boar passes the gate by a wide margin. The gate has no
notion of physical space or scripting.

**F10 — fact: what the alternatives would cost.** Ticking **SHAGGY HUNTING DOG**
in **ENEMIES SKIPPED** instead of building this freezes **96** placements (54 of
them in maps `names.map_is_unused` does not flag as unused) and removes **9** of
the pool's **333** entries, so the creature stops appearing anywhere new. This
feature freezes **26** placements (16 of them reachable by that same rule: the
six Central Yharnam dogs in the two loaded variants of that area, and the four
Forbidden Woods dogs in the one loaded variant of theirs) and, as specified,
removes nothing from the pool.

**F11 — fact: the zone scaling pass rewrites the protected dogs' stat rows
anyway, and already does so for skipped creatures.** `ApplyBossParamScaling`
runs after the enemy pass and rewrites every tracked stat row in the map,
randomized or not. In the DLC Central Yharnam file it turns `124400` into
`900008091` and `124401` into `900008122`; in the DLC Forbidden Woods file it
turns `124500` into `900008223` and `124501` into `900008254`. So "unchanged"
means "not re-targeted", and a verifier must not assert byte equality with
vanilla on that field. This is existing behaviour, identical to what a creature
ticked in **ENEMIES SKIPPED** gets today, and §10 D8 leaves it that way.

**F12 — fact: removing the protected set from pool contribution would cost
exactly two pool entries.** If protected placements also stopped feeding the
replacement pool — which is what **ENEMIES SKIPPED** does — the pool would drop
from **333** entries to **331**, still across 82 creatures. The entries lost are
the caged dogs' own stat variants: `124401`, which only the Central Yharnam
penned dogs supply, and `124501`, which only the Forbidden Woods caged dogs
supply. Each area costs exactly one entry on its own; together they cost two.
Those versions of the dog would then never appear anywhere. **§10 D6 keeps them
contributing, so the pool stays at 333.**

**F13 — fact: the Forbidden Woods is two map files holding an identical set of
eight dogs.** The randomizer writes `m27_00_00_00` and `m27_00_00_01`. Both hold
the same eight dog placements with the same names, entity IDs, stat rows and
behaviour rows, in the same order. A player sees eight; the map data holds
sixteen. As with Central Yharnam (F2), the protection has to be applied to both
files. One inconsistency in the repo's alias data is worth recording so it is
not mistaken for a finding: `names.map_is_unused` flags the pre-DLC Forbidden
Woods file as never loaded while flagging the pre-DLC Central Yharnam file as
loaded. Nothing measurable supports treating the two areas differently, and the
conservative reading — cover every map file that holds a protected placement —
costs nothing.

**F14 — fact: exactly four of the eight Forbidden Woods dogs are caged, and the
evidence is stronger than Central Yharnam's, not weaker.** Four independent
measurements give the same four placements and no others:

| Placement | Entity ID | Stat row | Distance to its cage |
| --- | --- | --- | --- |
| `c1240_0001` | 2700301 | 124501 | 0.25 |
| `c1240_0002` | 2700302 | 124501 | 0.13 |
| `c1240_0004` | 2700308 | 124501 | 0.26 |
| `c1240_0005` | 2700309 | 124501 | 0.34 |

* **Proximity.** Each stands within **0.34 units** of a *distinct* instance of
  one cage model. The nearest any other Forbidden Woods dog comes to a cage is
  **32.6 units** — a hundredfold gap, not the tenfold one that made the seventh
  Central Yharnam dog a judgment call in F6. **The in-cage/near-cage question
  that §10 D5 had to settle does not arise here.** No enemy of any kind other
  than these four comes within 1.0 unit of a cage.
* **Script.** The Forbidden Woods script file initialises a family of five
  parameterised events over these dogs, every instruction addressing the dog by
  **entity ID**: one event four times (once per dog, each paired with the cage
  object it stands on — and the pairing matches the proximity measurement
  exactly), two more four times each, and two that split the four into pairs.
  The other four Forbidden Woods dogs appear in none of them; two of them appear
  only in a general-purpose event the map initialises 43 times.
* **Stat row.** All four use `124501`, and a sweep of all twenty-four base maps
  finds that row on **no other placement anywhere** — 8 placements in total,
  being these four in each of the two map files. The other four Forbidden Woods
  dogs use `124500`. Unlike Central Yharnam's `124401` (F4), this row is exact
  for the whole caged set of its area.
* **Collision surface.** Not a discriminator here, and this is the one place the
  two areas differ in kind. Central Yharnam's kennel-yard floor carries seven
  enemies, all of them dogs (F6); the Forbidden Woods cages sit on a surface
  carrying eleven enemies of four different creatures, only four of them dogs.

Two further details a hardware tester will see. The cluster holds **six** cages
within about fifteen units of one another, and **two of them are empty** in the
unmodified game: their cage objects exist, carry entity IDs, and are referenced
by no script instruction at all. The script family is initialised six times
naming six dogs, but two of those names — entity IDs 2700303 and 2700310 — match
**no placement in any base map**. The area was authored for six caged dogs and
ships four.

**F15 — fact: a game-wide sweep finds caged dogs in exactly these two areas and
nowhere else.** Three independent sweeps across all twenty-four base maps agree:

* The three cage models used in Central Yharnam have **no instance in any map
  outside that area's three files**. The Forbidden Woods uses a different cage
  model, which has no instance outside that area's two files. Object model
  numbering is per-area in this game, so a shared trailing number across areas
  is not evidence of a shared asset and was not relied on.
* Of the **120** dog placements in the game (every one of the five dog-shaped
  creatures of F1), the twenty-six closest to an object are exactly the
  protected set, all within **0.90 units**. The twenty-seventh is at **1.31
  units** — a Hemwick Hunting Dog beside a generic prop the game uses four times
  in that one map, and in no script.
* Across every map's script file, the only dog placements paired with an object
  entity in an event initialiser are the two Central Yharnam breakout dogs and
  the four Forbidden Woods dogs.

So the protected set is complete as far as the vanilla data can show: there is
no third area of caged dogs waiting to be found.

---

## 5. Terminology

**Shaggy Hunting Dog** — the creature actually in the cages, as the randomizer's
own **ENEMIES SKIPPED** list names it. The backlog row and the developer's draft
both say "Hunting Dog", which in this game's data is a *different* creature
found only in Hemwick Charnel Lane (§4, F1). This spec uses "dog" loosely in §1
and §2 and the precise name wherever the distinction could mislead.

**Placement** — one spot in one map where the game puts one creature. The unit
this feature protects.

**Enemy / creature** — the identity written into a placement. The unit
**ENEMIES SKIPPED** and **ENEMIES INCLUDED** work on.

**Kennel yard** — the walled Central Yharnam courtyard of cages. Not a term the
game data uses; used here for the area that holds seven dogs, six of them caged.

**Cage cluster** — the Forbidden Woods group of six cages within about fifteen
units of one another, four of them holding a dog and two empty. Also not a term
the game data uses.

---

## 6. Scope

### In scope

* Backlog row **33** only.
* One new on/off setting, off by default, persisted with the existing
  configuration, shown on the existing settings screens.
* Protection of the identified Central Yharnam caged-dog placements in **all
  three** Central Yharnam map files (§4, F2) and of the identified Forbidden
  Woods caged-dog placements in **both** Forbidden Woods map files (§4, F13) —
  twenty-six placements in five map files (§10 D9).
* A check that recomputes the protected set from the vanilla tree, so the
  hard-coded identifiers cannot silently stop matching what they were derived
  from.

### Out of scope

* **Any change to `ENEMIES SKIPPED` or `ENEMIES INCLUDED`.** Row 32 shipped and
  was hardware-tested on 2026-09-19; its semantics must be untouched.
* **Adding the dog to any global exclusion.** Explicitly ruled out by the draft
  and by the backlog row; it is the failure mode this feature exists to avoid.
* **Special-casing the replacement creatures.** Nothing about Boom Hammer
  Hunters or Maneater Boars is encoded; the protection is on the source spots.
* **Other constrained placements anywhere in the game that are not caged dogs.**
  `docs/deferred-ideas.md` §1.4 Q5 lists likely candidates — enemies in windows,
  on ledges, in doorways, and anything whose vanilla occupant never moves. None
  has been surveyed and none is included here. Caged *dogs* specifically are no
  longer in that bucket: §4 F15 surveyed the whole game for them and found the
  two areas this feature covers, so there is no third area left to defer.
* **A per-location enemy compatibility matrix** (`deferred-ideas.md` §1.3
  Approach B), and any property-based rule derived from model size or AI class.
* **Diagnosing the lag and the unkillable replacements.** The cause remains
  unknown (§4, H1).
* Chalice dungeons, as always.

---

## 7. Constraints and decisions

* **Off must be a no-op.** With the setting off the run must be identical to
  today's, roll for roll. This is the standing rule for every new setting in
  `RandomizerDefaults.h` and it is what makes an older `defaults.cfg` safe.
* **Default off**, following the struct-wide convention that an absent
  configuration key never silently turns a setting on.
* **The protected set must come from verified placement identifiers**, not from
  the creature's ID and not from a stat row, and must be pinned by a check that
  recomputes it from `data/vanilla/dvdroot_ps4`. A list transcribed by hand and
  checked by nobody is the failure mode §4 F7 exists to prevent.
* **Name-substring matching alone is unsafe here.** The existing gates match
  placement names as substrings game-wide; all eight of the names in question
  are reused by unrelated dogs in other areas (§4, F7).
* **Every map file that holds a protected placement must be covered** — three
  Central Yharnam files and two Forbidden Woods files (§4, F2 and F13).
* **`ENEMIES SKIPPED` wins wherever it applies**, and this setting must not
  weaken, override or reorder it (§2 truth table).
* **The creature must stay in the replacement pool, and so must these
  placements' contribution to it** (§10 D6). Turning this on must not stop
  Shaggy Hunting Dogs appearing elsewhere in the world, and must not change the
  pool's size or contents — it stays at 333 entries. This is the deliberate
  difference from `ENEMIES SKIPPED`, which couples protection to pool removal.
* **UI budget.** The settings list draws at scale 4, which fits roughly fifty
  characters across; the longest row today is 32 characters including its value.
  `DO NOT RANDOMIZE CAGED DOGS` (§10 D7) is 27 characters, so with its value it
  sits alongside the existing longest row rather than beyond it. The list also
  grows from fifteen rows to sixteen, which `app/tools/ui_scroll_verify.py`
  tracks per screen.
* **Help text must not name "Hunting Dog"** (§10 D7), and must name both
  areas if the character budget allows, falling back to area-neutral wording if
  it does not (§10 D10).
* **Hardware is the only authority on whether the symptoms go away**
  (`CLAUDE.md` §3).

---

## 8. How will we know it works?

### Automated testing

What must be demonstrable from the vanilla tree and from a generated output
tree, without running the game:

1. **The protected set is exactly the six placements of §4 F3 in each of the
   three Central Yharnam map files and the four placements of §4 F14 in each of
   the two Forbidden Woods map files — twenty-six in total — and nothing else in
   any of the twenty-four base maps.** Recomputed from the vanilla tree rather
   than restated, so a divergence between the shipped identifiers and the data
   they were derived from is a failure.
2. **With the setting on, every protected placement in the output tree still
   carries the creature and the behaviour row it has in vanilla.** Its stat row
   must be allowed to be the zone-scaled value rather than the vanilla value
   (§4, F11).
3. **With the setting on, protection is the only thing that changed.** No
   placement outside the protected twenty-six is frozen by it, and the Shaggy
   Hunting Dog's other 70 placements are still randomized at the normal rate.
4. **With the setting on, the creature is still drawn as a replacement** — it
   still appears somewhere it does not appear in vanilla.
5. **With the setting off, the output is identical to a run made before the
   feature existed**, for the same seed and the same other settings.
6. **`ENEMIES SKIPPED` behaviour is unchanged in all four combinations of the §2
   truth table**, including the two where the creature is ticked there.
7. **The configuration round-trips**, and an older `defaults.cfg` without the new
   key loads with the setting off.

### Hardware testing

Claude cannot run any of this; the PS4 is the authority.

1. **Walk the Central Yharnam kennel yard with the setting on.** Expected: six
   dogs in cages, behaving as they do in the unmodified game — including the two
   that come out at you — and killable in the usual couple of hits. Most
   important failure: a cage still holding something else, which would mean the
   identified set is wrong or incomplete.
2. **Walk the Forbidden Woods cage cluster with the setting on.** Expected: six
   cages, four holding Shaggy Hunting Dogs that behave as they do in the
   unmodified game, and two empty (§4 F14) — the empty pair is expected and is
   not a sign the protection missed something.
3. **Confirm the seventh Central Yharnam yard dog, the rest of Central Yharnam
   and the other four Forbidden Woods dogs still changed**, so the protection is
   not quietly wider than specified.
4. **Confirm Shaggy Hunting Dogs in Cathedral Ward and Yahar'gul still changed**,
   and that the creature still turns up somewhere new.
5. **The symptoms.** No frame-rate collapse in either cage area, and nothing in
   a cage that takes damage without dying. **If either symptom persists with the
   setting on, the identification is incomplete or H1 is wrong** — and that is
   the most valuable result this test can produce.
6. **Run once with the setting off** and confirm both cage areas behave exactly
   as they do today, replacements and all. This run is also the first
   observation anyone will have of what a randomized Forbidden Woods cage
   actually does, which §4 F14 could not establish from data.

---

## 10. Decisions

| Date | Decision |
| ---------- | -------- |
| 2026-09-19 | **D1 — This is a placement protection, never an enemy one.** Adding the dog to any global exclusion list is ruled out by the developer's draft §12 and by backlog row 33. The creature stays in the replacement pool and its other placements stay eligible. |
| 2026-09-19 | **D2 — Default off.** Follows the draft §2 and the convention in `RandomizerDefaults.h` that an absent configuration key never turns a setting on. |
| 2026-09-19 | **D3 — The protected set is derived from the vanilla map data and pinned by a check that recomputes it, not transcribed by hand.** §4 F7 established that only a hard-coded list of identifiers gets the set exactly; recomputation is the mitigation for that. |
| 2026-09-19 | **D4 — `ENEMIES SKIPPED` is untouched and wins wherever it applies.** The §2 truth table is the contract. |
| 2026-09-19 | **D5 — Six dogs, not seven.** The protected set is the six placements the cage script addresses by entity ID. `c1240_0007`, the seventh dog in the same yard, is **not** protected: it is in no cage event and sits ten times further from the nearest cage prop than the penned dogs (§4 F6). The evidence for "in a cage" is the script and the prop, and both give the same six. If hardware testing shows the loose dog's spot also misbehaves, adding it is one more identifier — it is not a reason to switch the whole scheme to the collision surface. |
| 2026-09-19 | **D6 — Protected placements still feed the replacement pool.** This is a placement blacklist and nothing else; the pool is not what is broken. The pool stays at 333 entries, and the caged dogs' stat variant `124401` — which no other placement in the game supplies (§4 F12) — stays in circulation. **This is the deliberate difference from `ENEMIES SKIPPED`, which couples the two halves on purpose.** |
| 2026-09-19 | **D7 — The setting is called `DO NOT RANDOMIZE CAGED DOGS`.** The developer chose this over the recommended `PROTECT CAGED DOGS` and over the draft's `PROTECT CENTRAL YHARNAM CAGED DOGS`. At 27 characters it is in line with the longest existing row. The name is deliberately **narrow**: a future survey of other constrained placements (`docs/deferred-ideas.md` §1.4) gets its own row and its own decision rather than inheriting a name chosen before the evidence existed. Help text must **not** use the words "Hunting Dog" — that points at the wrong `ENEMIES SKIPPED` row (§4 F1). |
| 2026-09-19 | **D8 — Zone stat-scaling is left alone.** Protected placements are still re-tuned for the zone, exactly as creatures ticked in `ENEMIES SKIPPED` are today (§4 F11). "Unchanged" therefore means "not re-targeted", never "byte-identical to vanilla", and §8 must not assert byte equality on the stat field. If the protected dogs feel wrong on hardware, scaling is the next suspect and gets its own change. |
| 2026-09-19 | **D9 — The Forbidden Woods cages are in scope.** The four caged Shaggy Hunting Dogs of the Forbidden Woods cluster are protected alongside the Central Yharnam six, making the protected set **ten** placements, **twenty-six** across the five map files that hold them. The developer confirmed this knowing the asymmetry in the evidence: the Central Yharnam six are in the feature because their failure was *observed*, while the Forbidden Woods four are in it because the data is *structurally identical* (§4 F14) and a game-wide sweep finds caged dogs in these two areas and nowhere else (§4 F15). Including them is a precaution, taken because the setting's name (D7) is already area-neutral, the marginal cost is four identifiers and one hardware walk, and picking them up later would cost a second pass through every stage. §8's hardware test 6 supplies the observation that is currently missing. |
| 2026-09-19 | **D10 — Help text names both areas, budget permitting.** It must not use the words "Hunting Dog" (D7), and should name Central Yharnam and the Forbidden Woods so a player knows precisely what they are giving up. If the wording does not fit the §7 UI budget, stage C falls back to area-neutral phrasing rather than naming one area and omitting the other. Vagueness here is what produced the draft's wrong-creature instruction in the first place. |

---

## Appendix — research notes for stage C

*Not part of the specification, and not covered by the developer's approval.
Pointers from the spec investigation to save a search. Verify anything here
before relying on it.*

**Where the behaviour lives**

* `app/src/Randomizer/EnemyRandomizer.cpp` — `StepWriteMap`'s placement loop is
  where a placement is accepted or skipped; `StepReadMap`'s contribution loop is
  the other half (§10 D6 keeps contribution unchanged). Read the two existing gates and the position of the roll
  relative to them before choosing where a third gate goes.
* `app/src/Randomizer/EnemySkipList.h` and `EnemyExclusionList.h` — the two
  existing name-matching gates, and the shape any new one will be compared to.
  Note §4 F7 on why plain substring matching is not safe for this set.
* `app/src/Msb/Msbb.h` — `part_fields::GetEntityID` already exists and is
  already used by `BossRandomizer.cpp`; no new field accessor is needed.
* `app/src/Randomizer/BossParamScaling.cpp` — runs after the enemy pass and
  rewrites stat rows map-wide; §10 D8 leaves it alone, which bounds what §8 may
  assert.
* `app/src/Randomizer/RandomizerDefaults.h`, `RandomizerDefaultsStore.cpp`,
  `app/src/UI/EnableWizardScreen.cpp` and `app/src/UI/SetupDefaultsScreen.cpp` —
  the four places a new on/off setting has to appear, plus
  `app/tools/ui_scroll_verify.py`, whose per-screen row counts are hard-coded and
  will be one short.

**Worth checking early**

* `app/tools/enemy_lookup.py` — `load_base_maps` + `enemies` yields every
  placement's name, entity ID, stat and behaviour rows and position in a few
  lines, and `engine_pool` is the trusted pool mirror. Reuse rather than
  reimplement; `tools/enemy_lookup.py pool` is the one that has been wrong.
* The enemy part record carries a collision-surface index a little past the
  stat/behaviour block `enemy_lookup` already reads; it resolves to a collision
  part and is how the seven-dog Central Yharnam set would have been expressed.
  **§10 D5 chose the six, and §4 F14 shows the field does not discriminate in the
  Forbidden Woods at all, so this is background rather than the identifier to
  build on.**
* Both cage areas' `.emevd` files are readable with a ~30-line reader built on
  `boss_verify.read_dcx`: the header's section counts and offsets are plain
  64-bit fields, events are 48 bytes each, instructions 32, and an event's
  instruction offset is a **byte** offset, not an index. Two cheap checks
  validate the framing per file — section offsets reconcile, and per-event
  instruction counts sum to the file total. Event initialisers are bank 2000;
  their first two arguments are the slot and the target event ID, and the rest
  are the parameters, which is how the dog/cage pairing was recovered.
* Object part records live in the same `PARTS_PARAM_ST` section as enemies with
  a part-type discriminator of 1, and carry name, model index, position and
  entity ID at the same offsets `enemy_lookup.enemies` already uses. That is all
  the cage-prop measurements needed; no new parser.
* `docs/deferred-ideas.md` §1 is the prior investigation this feature closes the
  first half of, and §1.4 lists the questions it does not.
* `app/tools/names.py`'s `map_is_unused` disagrees with itself across the two
  areas (§4 F13). Do not let it decide which map files the protection covers.

**Dead ends already walked**

* Looking for a single distinguishing field in the dog placement records. Every
  4-byte field of all twelve Central Yharnam dogs was compared; there is none,
  and the Forbidden Woods four do not supply one either.
* Expecting the stat row to be enough. It is exact for the Forbidden Woods four
  and covers only four of the Central Yharnam six, so it is exact for neither
  the whole set nor a rule that could be written once.
* Assuming a shared trailing object-model number means a shared asset across
  areas. Object model IDs are numbered per area; the Central Yharnam and
  Forbidden Woods cage models share a suffix and so do several unrelated props
  in Yahar'gul and the Research Hall. The pairing was established from the
  script and the geometry instead.
* Expecting the MSB's own event or region sections to reference the cages. They
  reference parts by index, not by entity ID, and none of them mentions these
  dogs or the cage objects.
* Looking up the cage object models to size or name them. `dvdroot_ps4/obj`
  holds a single archive, `o919900.objbnd.dcx`, and none of the relevant
  objects.
* Treating the game's own cage scripts as self-explanatory. The event framing and
  the argument wiring are recoverable and are what §4 F3–F5 and F14 rest on; the
  *meaning* of the individual character instructions is not, and nothing in this
  spec depends on it. In particular, the Central Yharnam and Forbidden Woods
  event families are **not** the same instruction sequences — the two areas were
  scripted separately — so do not expect one pattern to match both.
