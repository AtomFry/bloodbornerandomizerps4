# Feature 013 — Bosses Can Replace Enemies

**Status: APPROVED** — 2026-09-28

**Reference:** `reference/Randomizer/MainWindowComponents/RandomizeFunctions.cs` — `InsertBossesVoid`, plus the pass's driver and gates in `StartFunctions.cs`. Six of this feature's eight settings have no reference equivalent; §7 carries each deviation's justification.

**Covers backlog rows 13 and 17.**

**Plan:** `docs/features/013-bosses-replace-enemies/plan.md` — added after the spec is approved

---

## 1. What does this feature do?

Today a run can shuffle the ordinary creatures of the world among themselves, and
separately shuffle which boss stands in which boss arena. The two never mix: a
boss only ever appears where a boss already was.

This feature lets bosses out of their arenas. With it on, some of the ordinary
creatures in each area — the huntsmen on the bridge, the crows in the woods, the
church servants in the ward — are replaced by bosses instead. The boss keeps its
own body and moves; it simply stands where a mob stood, with no fog gate, no
arena, no health bar and no music.

The player controls it in six ways. They say **how many** bosses each map gets
and whether that number is a ceiling or an exact quota; which **areas** take
part; which **bosses** may arrive; whether the bosses in a map are **spread out**
across it or allowed to land wherever chance puts them; and whether an arriving
boss uses **its own AI or the AI of the creature it replaced**. A seventh control
replaces *every* eligible creature at once, and exists only to find out whether
the game survives that — it is expected not to, and it is meant to be removed.

The bosses that arrive are still the bosses of the same playthrough — nothing new
is invented — and their own arenas are untouched, so every boss fight the game
normally gives you is still there.

This is a port of a setting the reference tool already has, and whose own
interface warns the player that it may make the game unstable and crash.

---

## 2. What does the player experience?

Eight settings, all in the **Bosses** group. The first is the master switch: with
it off, none of the other seven does anything.

| # | Setting | Off / low | On / high |
| - | ------- | --------- | --------- |
| 1 | **BOSSES CAN REPLACE ENEMIES** | Bosses appear only in boss arenas. All 1,381 ordinary creatures across the fourteen explorable areas are ordinary creatures. | Bosses stand loose in the world, with no fog gate, no health bar and no music, in the numbers and places the settings below decide. Everything not replaced is an ordinary creature still, shuffled or not according to **RANDOMIZE ENEMIES**. **This may make the game unstable or cause crashes** — the reference tool interrupts the user to say so, and §10 D-O carries that warning into this row's help text. |
| 2 | **BOSS REPLACEMENTS** | **UP TO** — the number below is a ceiling each map approaches but need not reach, so some maps get their full share and others get one or none. Variety between areas, and between runs on different seeds. How many bosses you tick changes which ones arrive, never how many (§10 D-M). | **EXACTLY** — every participating map gets the number below, no more and no fewer. Predictable, and the setting to use when you want to know what you are walking into. |
| 3 | **BOSS REPLACEMENTS PER MAP** | **0** — no bosses are placed at all, which is how to arm setting 4 without also asking for a count. | **1 to 15**, default **1**. At the default that is 14 bosses in a full run, leaving **1,367 of the 1,381 ordinary**. At 15 it is up to 210, leaving **1,171 ordinary**. A map with fewer creatures available than the number asked for gets as many as it can and the run says so; it never fails. |
| 4 | **REPLACE ALL ENEMIES** | Normal behaviour: the count above applies. | **Every** eligible creature in every participating area becomes a boss — all 1,381 of them, and nothing ordinary is left anywhere. This is a test control, not a way to play: it is expected to make the game unstable or crash, and it is meant to be removed once it has answered that question. See §6. |
| 5 | **AREAS INCLUDED** | — (a list; all fourteen ticked by default) | Which of the fourteen explorable areas take part. Unticking one leaves every creature in it ordinary. With nothing ticked, nothing happens. |
| 6 | **INSERTED BOSSES** | — (a list; all 22 ticked by default) | Which bosses may arrive in the world. **With nothing ticked, the whole feature quietly does nothing** — no error, no warning, no refusal. The list is **not** the same as **BOSSES INCLUDED**: it is six longer, because it also holds the individual bodies of the group fights. Unticking a boss here does not stop it holding its own arena, and unticking one under **BOSSES INCLUDED** does not stop it appearing in the world. Two separate choices about two separate things. |
| 7 | **SPREAD BOSSES OUT** | **Default.** The bosses in a map land wherever chance puts them, so two can end up in the same room. Measured, at ten per map the closest two are typically **4 to 17 paces** apart — near enough to fight at once. | The bosses in a map are pushed apart, so each tends to hold its own part of it. At ten per map that raises the closest pair to **35 to 89 paces**; at five, to **59 to 163**. At a count of one it changes nothing, because one boss has nobody to be spread from. |
| 8 | **BOSS BEHAVIOUR** | **OWN AI** (default) — an arriving boss fights the way it fights in its arena. | **REPLACED ENEMY'S AI** — an arriving boss wears the boss's body but is driven by the script of the creature it replaced, so a Cleric Beast may patrol a bridge like a huntsman. **This may simply not work**: AI scripts refer to animations and behaviours that belong to a particular body, and a boss running a huntsman's script may stand inert, flail, or crash the game. Only the console can say. |

What the feature leaves alone, in every case:

* **Every boss arena.** Every boss fight the game normally gives you is still
  fought where it always was, subject only to **RANDOMIZE BOSSES**.
* **The Hunter's Dream and the Abandoned Old Workshop** entirely. Neither holds a
  single creature this feature may touch, and neither is one of the fourteen
  areas.
* **360 further creatures** across the fourteen areas that the randomizer never
  touches at all — friendly and scripted bodies, and the placements the port
  already protects.
* **Anything ticked under ENEMIES SKIPPED**, and the ten caged dogs when **DO NOT
  RANDOMIZE CAGED DOGS** is on. A creature the player has told the run to leave
  alone is not replaced by a boss either. This holds even under **REPLACE ALL
  ENEMIES**.

Two combinations are worth stating up front:

* With **RANDOMIZE ENEMIES** off, this setting still works. It is not a modifier
  on the enemy shuffle; it is its own pass, and a run may have bosses in the
  world without a single mob having moved.
* With **RANDOMIZE ENEMIES** on, this pass writes last. A spot the enemy shuffle
  filled with a crow can still become Ludwig.

Across a whole run the bosses are dealt rather than rolled: each of the enabled
bosses is used once before any is used twice, so a run at a high count shows the
player the whole roster rather than three Living Failures and nothing else.

Every one of the eight is remembered on the **DEFAULTS** tab and a new world
starts from whatever is set there, exactly as every other setting in the app
already does.

The feature takes effect when the world is built, not during play, so a world
already generated does not change.

---

## 3. What does the existing randomizer do?

The reference tool has this setting as `insertBossesBool`, checkbox
**"Bosses Can Replace Enemies"**. Ticking it pops a modal dialog reading *"This
may make the game unstable and cause crashing. Use this feature at your own
risk."* (`UIComponents.cs:19-23`).

The work is `InsertBossesVoid` (`RandomizeFunctions.cs:1460-1552`), called once
per eligible map from `StartFunctions.cs:1013-1133`, **after** enemy
randomization and after boss randomization have both finished. For each enemy
placement in the map that does not match the fixed unused+boss name list, it
rolls `universalRand.Next(0, 101) <= bossPercentage` and, on success, overwrites
that placement's `NPCParamID`, `ThinkParamID` and `ModelName` with a boss
identity drawn uniformly from `insertBossesString`.

Five things about that are not what the backlog rows describe, and all five are
load-bearing:

1. **It is not the enemy pool.** `insertBossesString` is a separate list,
   harvested inside `GenerateBossList` (`:1843`) from boss placements. The
   ordinary enemy pool is never given a boss entry. Row 13's "lets boss
   identities into the ordinary enemy pool" describes an implementation the
   reference does not have — and, under §10 D-N, not one this port has either:
   the pass is a **second pass** over each map, exactly as the reference orders
   it. §4 measures what mixing bosses into the enemy pool would actually have
   cost, which is what settled it.
2. **It is gated by a percentage**, `bossPercentage`, which has no field
   initializer (`FieldContainer.cs:169`) and is only ever written by the
   slider's own handlers (`UIComponents.cs:194-309`). On a run where the user
   never touches it, it is `0.0` — and because the roll is `<=` against an
   inclusive `Next(0, 101)`, a 0.00 setting still converts one eligible
   placement in 101. This is the defect §7 cites as the justification for
   replacing the percentage with a count.
3. **It is gated per area.** Fourteen further checkboxes — `bossHemwick`,
   `bossCentralYharnam`, … — decide which areas the pass visits
   (`StartFunctions.cs:1039-1102`), each defaulting unchecked, so with the main
   checkbox on and none of the fourteen ticked the feature does nothing at all.
   Those fourteen flags are backlog **row 17**, and they gate this pass and
   nothing else in the entire tool — which is why this spec covers row 17 too.
   Row 17's backlog description, "chooses which zones' bosses take part", is
   wrong: they have no effect on boss randomization.
4. **Drawing does not consume, and placement is per-placement.** The list is
   never drained, so the same boss can arrive many times in one area; and the
   choice is an independent coin flip at each placement, so the reference has no
   notion of a count, of a quota, or of where in the area a boss lands.
5. **The boss's own AI is written, except once.** The pass writes the drawn
   identity's `ThinkParamID` — except for `NPCParamID == 551000`, where it
   computes a replacement AI id and then fails to assign it, leaving the replaced
   creature's AI in place. That accident is the only case in the reference where
   a boss body runs an ordinary enemy's script, and setting 8 in §2 is what
   completing its evident intention would look like.

Two filters apply to the list itself. `excludeBossesBool` plus its text box of
five-character model strings removes matching entries before the pass runs
(`StartFunctions.cs:786-802`) — the reference's exclusion list is the same
instrument the port ships inverted, as a picker. And `InsertBossesVoid` itself
strips Moon Presence from the list on entry (`:1462-1469`), one entry per call,
so it can never be inserted.

`oopsAll` mode suppresses the feature entirely: it skips `GenerateBossList`
(`StartFunctions.cs:705`, `RandomizeFunctions.cs:1565`), leaving
`insertBossesString` empty, and an empty list makes every insertion a no-op.
That is the reference's own precedent for **doing nothing quietly** when the
insertion list is empty, which is what §10 D-G requires.

**Reference defects on this path.** Two, both in `InsertBossesVoid`:

* `:1538-1545` — the unassigned `ThinkParamID` described above. Catalogued in
  `docs/windows-randomizer-technical-review.md` §5.4.
* `:1514` — an unguarded `Console.WriteLine(enemyData[0])` on a list that the
  exclusion filter above can empty.

The standing reference-architecture document,
`docs/windows-randomizer-technical-review.md`, **does** cover this feature, at
§5.4 and §5.5, and everything it says was re-checked against the source for this
spec and is still true. It is also the only project document that describes the
feature correctly: `docs/plans/pickers.md` §7.5 and
`docs/features/032-bypassed-enemies/spec.md` §8 both state that row 13 "will
draw from the enemy pool", which the trace above contradicts.

There is **no reference behaviour** for a count, a quota mode, a replace-all
override, spreading placements out, drawing without replacement, or choosing an
arriving boss's AI. Those are this port's own, and §7 carries their
justifications.

---

## 4. What do we know?

Every number below is measured against `data/vanilla/dvdroot_ps4` by replaying
the reference's own rules through the existing readers in `app/tools/`
(`boss_verify.py`'s MSBB reader, `enemy_lookup.py`'s exclusion and
overwritable-placement oracles, and the size and scaling tables parsed from the
port's own headers). The scripts are throwaway; the rules they replay are cited
in §3.

### The fourteen areas, and what each one offers

The reference's fourteen per-area flags cover 22 map files, because most areas
have a pre-DLC or unused variant of the same file. A retail player with the DLC
installed loads exactly one file per area, and the port's own stat-scaling table
names precisely those fourteen files and no variant (`BossParamScaling.h`) —
independent corroboration of which file is the live one, from the reference's own
data rather than from a name. **The count in setting 3 is per map file**, so a
player sees it as per area; the unused variants each receive their own quota,
which is harmless because the game never loads them but which the verifier has to
expect.

| Area | Eligible targets | Distinct creature kinds | Area's extent (paces, bounding diagonal) |
| --- | --- | --- | --- |
| Hemwick Charnel Lane | 73 | 9 | 440 |
| Old Yharnam | 81 | **5** | 268 |
| Cathedral Ward | 87 | 16 | 363 |
| Central Yharnam | 149 | 17 | 474 |
| Upper Cathedral Ward | **54** | 7 | 274 |
| Forsaken Castle Cainhurst | 96 | 7 | 302 |
| Nightmare of Mensis | 108 | 17 | 391 |
| Forbidden Woods | **178** | 18 | 523 |
| Yahar'gul, Unseen Village | 118 | 14 | 625 |
| Byrgenwerth | 72 | 8 | 325 |
| Nightmare Frontier | 60 | 8 | 264 |
| Hunter's Nightmare | 106 | 12 | 465 |
| Research Hall | 84 | 9 | **184** |
| Fishing Hamlet | 115 | 9 | 274 |
| **Total** | **1,381** | — | — |

* **Fact.** Those fourteen areas hold **1,741** enemy placements, of which
  **1,381 are eligible targets and 360 are spared** by the port's existing
  103-pattern fixed exclusion list — the same test the port already applies at
  the same two places the reference does. 1,381 is therefore also exactly what
  **REPLACE ALL ENEMIES** replaces.
* **Fact.** Counted by map *file* rather than by area, which is how the pass
  iterates and which counts the unused variants again, the figure is **2,269
  across 24 files**. The two numbers describe the same creatures; 1,381 is the
  one a player could verify.
* **Fact.** **The smallest map is Upper Cathedral Ward at 54 eligible targets**,
  then Nightmare Frontier at 60. That is the floor the maximum count is set
  against: at 15 the largest quota is under a third of the smallest map's supply,
  so **EXACTLY** is satisfiable everywhere at default settings.
* **Fact.** The Hunter's Dream and the Abandoned Old Workshop contribute **zero**
  eligible targets, and the reference's per-area call list does not name them
  either. `docs/design-decisions.md` already records the Hunter's Dream as a
  deliberate control group.
* **Fact.** No boss placement is a target: all 38 boss name patterns are in the
  same fixed exclusion list (`EnemyExclusionList.h`, `BossList.h`). All **42**
  boss-named placements across the boss maps are untouched by this pass, under
  **REPLACE ALL ENEMIES** included.
* **Fact,** and the case the shortfall rule exists for: **a map can be starved.**
  Old Yharnam's 81 targets are only **five** distinct creature kinds — Wolf
  Beast, Chime Maiden (Light), Cloaked Beast Patient, Carrion Crow and Cloaked
  Beast Patient (Female) — so ticking five rows of **ENEMIES SKIPPED** takes that
  map to zero eligible targets. Upper Cathedral Ward and Cainhurst need seven,
  Byrgenwerth and Nightmare Frontier eight. It takes deliberate effort, but it is
  reachable, and it is the only way a count of 15 or less cannot be met.

### What arrives, and how it is dealt out

* **Fact.** The candidate list holds **33 boss identities across 23 models**.
  Removing Moon Presence, which the pass strips on entry, leaves **32 identities
  across 22 models** — the list **INSERTED BOSSES** offers. *(Corrected
  2026-09-28 from 30/29 during stage C; see `log.md`. The harvest at
  `RandomizeFunctions.cs:1657` runs* before *the `m28 && c2100` and
  `m34 && npc == 210030` de-eligibility tests at `:1669` and `:1677`, so three
  further Witch of Hemwick identities reach the insertion list even though the
  boss pool rejects them. The `ThinkParamID != "1"` drop at `:1841` is applied.
  The model set is unchanged at 22.)* Named: Blood-Starved
  Beast, Witch of Hemwick, Shadow of Yharnam, Martyr Logarius, Celestial Emissary
  and its small form, Ebrietas, Father Gascoigne and his beast form, Living
  Failure, Laurence, Ludwig, Lady Maria, the Orphan of Kos and its flapping form,
  Cleric Beast, Vicar Amelia, Darkbeast Paarl, Rom, Amygdala, Mergo's Wet Nurse,
  and Gehrman.
* **Fact.** That list is **not** the boss-arena pool. The boss pool is 18
  identities across 17 models; the insertion list is harvested earlier in the
  same loop, before the narrower reject list and before the per-map duplicate
  test, so it holds **six models the boss pool does not**: Witch of Hemwick,
  Shadow of Yharnam, Celestial Emissary, the small Celestial Emissary, Living
  Failure and the standing Orphan of Kos. This is why **BOSSES INCLUDED**'s
  17-row table cannot express a choice about six of these creatures, and why
  §10 D-B gives this feature its own table.
* **Fact.** The 32 identities are **not** 32 different creatures: Witch of
  Hemwick supplies 5 of them, Living Failure 4, Shadow of Yharnam 3, Ludwig 2,
  and the remaining eighteen models 1 each. Draining by identity therefore
  preserves the reference's own weighting — Witch of Hemwick is 15.6% of
  arrivals, Living Failure 12.5%, a single-variant boss 3.1% — while draining by
  model would flatten it, which is
  a change `docs/deferred-ideas.md` §2 declines to make for the enemy pool. This
  is why §10 D-G's pool drains by identity.
* **Inference,** and the answer to whether the pool is per map or per run:
  **per run.** A per-map pool of 32 identities can never be exhausted by a count
  of 15, so the refill rule would be unreachable code and every map would start
  from the same full list. Drained across the run it is exercised constantly — at
  the default count of one, 14 draws is half a cycle; at 15 per map, 210 draws is
  about seven cycles; under **REPLACE ALL ENEMIES**, 2,269 draws is about 78.
  It also matches the port's own shipped precedent exactly: `BossPool` in
  `BossRandomizer.h` is drained as arenas are assigned across the whole run and
  refilled from a second list when it empties.
* **Fact.** Draw-without-replacement-with-refill has a property that can be
  checked on generated output without predicting a single placement: across a
  whole run, the number of times any two enabled identities are used **differs by
  at most one**. §8 uses that as its pool criterion.
* **Fact.** The candidate list is identical whether it is harvested over the
  reference's 16-map list or the port's 17-map boss-collect order — 33
  identities, 23 models, the same set. The port's undocumented use of the
  assignment map order for pool collection therefore costs this feature nothing.
* **Fact.** All 23 models already exist in the union of the 24 base maps' enemy
  model tables, and the port already copies that whole union into every map. No
  new file-format capability is needed for a boss to be referenced from a world
  map.
* **Fact, and the measurement D-M rests on.** The ordinary enemy pool is **333
  entries across 82 models** — `enemy_lookup.py`'s `engine_pool`, which replays
  `EnemyRandomizer.cpp`'s contribution loop line for line. So if the insertion
  were done by giving each ticked boss one entry in that pool and drawing
  uniformly, **the boss share would be set by how many bosses the player
  ticked**, not by the count:

  | INSERTED BOSSES ticked | Share of draws | Expected in the smallest map (54) | Expected across the run (1,381) |
  | --- | --- | --- | --- |
  | 32 | 8.77% | 4.7 | 121 |
  | 15 | 4.31% | 2.3 | 60 |
  | 5 | 1.48% | 0.8 | 20 |
  | 1 | 0.30% | 0.16 | 4 |

  A ceiling cannot repair the bottom row, because a ceiling only ever removes
  bosses. **ENEMIES INCLUDED** distorts it in the opposite direction, since
  shrinking the pool raises the boss share — at 100 entries 24.2%, at 10 entries
  76.2% — though that direction *is* contained by the ceiling. This is why D-M
  derives the per-placement probability from the count and the map's
  eligible-target count, and why §8 criterion 4 measures the single-boss mean.

### Where a boss can be put, and what spacing buys

* **Fact, verified three ways.** An MSB part record carries its world position as
  three little-endian `float32` at **entry + 0x28**, followed by rotation at
  +0x34 and scale at +0x40. Derived from `MSBBB`'s `PartsParam.cs`
  `Part(BinaryReaderEx)` constructor — `descOffset` 0x00, `nameOffset` 0x08,
  `ModelLocalID` 0x10, `Type` 0x14, ID 0x18, `modelIndex` 0x1C,
  `placeholderOffset` 0x20, then `ReadVector3()` three times — and cross-checked
  against the port's own three documented offsets in `Msbb.h`, which bracket it
  exactly: `Type` at +0x14 and `modelIndex` at +0x1C below it, `baseDataOffset` at
  +0xB0 and `typeDataOffset` at +0xB8 above it, with the three vectors plus the
  three 8-word group arrays and one `int32` accounting for every byte between.
  Confirmed empirically by reading the field out of Central Yharnam: plausible
  world coordinates, rotations in degrees with the expected Y-dominant yaw, and
  every scale exactly 1.0.
* **Inference.** Reading three floats at a fixed offset inside a part record is
  the mechanism the port already uses for `NPCParamID`, `ThinkParamID`,
  `modelIndex` and `EntityID`. Nothing new is needed, and nothing is written —
  position is read-only for this feature.
* **Fact.** Placements are almost all at distinct positions: 1,381 targets across
  the fourteen areas sit at 1,367 distinct coordinates. The exceptions are real
  (Nightmare of Mensis has 101 distinct positions for 108 targets, Forbidden
  Woods 170 for 178), so any spacing rule has to tolerate a zero distance.
* **Fact.** Spreading the choices out makes a large, measurable difference, and
  it is what sets the maximum count. Mean minimum pairwise distance across the
  fourteen areas, uniform draw versus greedy farthest-point, 12 trials per area
  per count:

  | Count per map | Uniform draw | Spread out | Worst single area, spread |
  | --- | --- | --- | --- |
  | 5 | 14 – 42 paces | 59 – 163 paces | 59 (Research Hall) |
  | 10 | 4 – 17 paces | 35 – 89 paces | 35 (Research Hall) |
  | **15** | 3 – 9 paces | 21 – 65 paces | **21 (Upper Cathedral Ward)** |
  | 20 | 2 – 7 paces | 12 – 54 paces | 12 (Upper Cathedral Ward) |
  | 25 | 1 – 5 paces | 10 – 48 paces | 10 (Upper Cathedral Ward) |

  **15 is the last count at which spacing still keeps every map's closest pair
  above 20 paces.** At 20 the worst case falls to 12 and at 25 to 10 — which is
  roughly what a *uniform* draw already gives at a count of 5, so beyond 15 the
  spacing setting stops being able to deliver what it promises. That, not the
  supply of placements, is what §7 sets the maximum from.
* **Inference, and the honest limit.** This is 3D Euclidean distance over
  placement coordinates and nothing else. It has no knowledge of walls, floors,
  doors, locks or traversal. Two placements 30 paces apart may be on different
  storeys of the Research Hall — whose 84 targets fit a bounding diagonal of only
  184 paces, the tightest of the fourteen, and which is the worst case in the
  table at every count up to 15. So the guarantee is **spread out in space, not
  spread out in play**: the rule reliably stops two bosses sharing a courtyard
  and does not promise they are far apart along the route.
* **Judgement, stated as such.** It is worth shipping anyway. What a player
  notices is not the metric but its failure mode — two bosses aggroing together
  from one spot — and the measured 4-to-17-pace nearest pair at ten per map says
  that failure is the *common* case without spacing and rare with it. The
  alternative that would fix the storey problem is a navmesh or region analysis
  the port has no reader for and no need for elsewhere.

### Whether an inserted boss is a fair fight, and whether it fights at all

* **Fact.** The reference applies **no size gate** to this pass. The enemy
  shuffle's gate — reject a candidate whose model-size proxy exceeds twice the
  original's — is in the enemy loop only, and this pass does not reuse it.
* **Fact,** and the reason that matters less than it looks: the gate would barely
  bite if it were applied. Measured against the 2,269 targets' own sizes, a ×2
  tolerance would admit the smallest bosses almost everywhere (Gehrman 79% of
  targets, Vicar Amelia 79%, the small Emissary 93%) and the largest at a third
  (the Orphan of Kos 21%, Father Gascoigne 33%, Amygdala 33%, Darkbeast Paarl
  33%). Nothing is excluded outright; a gate would only concentrate the big
  bosses onto the big mobs. The table's own metric is a memory footprint, not a
  hitbox (`ModelSizeTable.h`), so it says nothing about whether a boss physically
  fits a corridor.
* **Fact.** The port's existing stat-scaling pass **does** re-tune most inserted
  bosses to the area they land in. Of the 32 insertable identities, **25 are
  tracked by the scaling table and are rewritten to that area's own variant** in
  15 of the 16 scaled areas; a Cleric Beast dropped into Central Yharnam is
  written as that zone's Cleric Beast variant, not the arena one. **Seven are not
  tracked and keep their arena stats wherever they land**: three of the five
  Witch of Hemwick identities, the small Celestial Emissary, and three of the
  four Living Failure variants.
* **Assumption.** That the scaling variants are *easier*, not merely different.
  The pass's own source calls them "pre-tuned … appropriate for the zone"
  (`BossParamScaling.h`), and the reference's opt-out is named "No Scaling", but
  no one has read the resulting `NpcParam` rows. Verifiable by dumping the scaled
  rows' HP and damage out of `gameparam.parambnd.dcx`; `docs/known-traps.md` is
  explicit that byte decoding does not prove game behaviour, so the honest claim
  is only that the variant is different.
* **The BOSS BEHAVIOUR risk, stated plainly, and it is the largest unknown in
  this feature.** `ThinkParamID` selects an AI script, and an AI script drives a
  body by naming animations and behaviours that belong to that body. A boss model
  running an ordinary enemy's script is therefore being asked to play animations
  its skeleton may not have. The plausible outcomes, none of which can be
  distinguished from map data: it works and the boss patrols like a mob; the boss
  stands inert because every animation it is told to play is missing; it
  T-poses or flails; or the game crashes on the first behaviour lookup. **This is
  an inference from what a think id is for, not a measurement** — nothing in the
  data says which. The one piece of circumstantial evidence is that the
  reference's sole accidental instance of this (§3 point 5) sits in the very
  feature whose checkbox carries the author's instability warning, which is at
  least consistent with the author having seen it misbehave. OWN AI is the
  default for exactly this reason.
* **Inference.** The fairness problem, separately from the AI question, is (a)
  the five unscaled identities, (b) the multi-body fights whose companion bodies
  this pass inserts *individually*, since each Living Failure, Shadow and
  Emissary body is its own candidate and arrives alone, and (c) the count, which
  is now the player's to choose and which the maximum of 15 holds to 15.2% of the
  world's creatures — with **REPLACE ALL ENEMIES** as the one deliberate
  exception, which is why it is temporary.

### What could break

* **Fact.** The reference tool's own interface warns of instability and crashes
  on this setting and on no other.
* **Fact.** The one insertable identity with `NPCParamID == 551000` is Mergo's
  Wet Nurse, so at the default count of one per map the reference's abandoned
  think-id edit would have been reachable in roughly one run in two. `ThinkParamID`
  551111, the value the reference computes and drops, appears nowhere in any of
  the 24 base maps — which is why §10 D-J makes the choice a general setting
  rather than resurrecting that one id.
* **Fact.** One identity the reference keeps out of this list, The One Reborn,
  carries `ThinkParamID == 0` in vanilla. Backlog row 14 would admit it, which is
  one of several reasons §6 leaves row 14 out.
* **Fact.** The port already substitutes bodies in four multi-body boss arenas
  (feature 018's easy modes) and already protects ten caged dogs because other
  creatures "behave badly in these cages" (feature 033, 26 placements, all of
  them eligible targets for this pass — which is why §10 D-C protects them, under
  **REPLACE ALL ENEMIES** included).
* **Fact.** Backlog row 36, boss rush, is recorded as an IDEA needing a hardware
  probe before it can be specified, and is "the highest-uncertainty item on this
  list" precisely because placing bosses at chosen points is not understood.
  **REPLACE ALL ENEMIES** is the most extreme version of that question anyone
  could ask, which is both why it is worth asking once and why it is not a way to
  play.
* **Inference,** and the closest thing to a safety argument available without
  hardware: every field this pass writes is one the port already writes on the
  same blobs, to the same three fields, in two shipped and hardware-tested
  passes. Nothing about the *file* is new. What is unproven is the game's
  reaction to a boss with no arena — fog gate, health-bar registration,
  multi-phase transitions, arena-scripted adds, and 1,381 boss models resident at
  once — and none of it can be established except on the console.

### Cost to the settings chain

* **Fact, and now a hard prerequisite rather than a tidy-up.**
  `RandomizerDefaultsStore.cpp`'s `FormatSettings` builds the settings block in a
  `char buf[1024]` with an `snprintf` clamp, and `pool_verify.py:700-733` pins the
  block at exactly **728** bytes today. This feature's eight settings cost **241
  bytes**: `bosses_can_replace_enemies` 29, `boss_replacements_exact` 26,
  `boss_replacements_per_map` 29 (two digits at the maximum),
  `replace_all_enemies` 22, a 14-row area picker 30, a 22-row insertion picker 46,
  `space_out_inserted_bosses` 28, and `inserted_boss_keeps_enemy_ai` 31. That
  takes the block to **969** — and with features 011 (+47) and 027 (+15), both
  APPROVED and unlanded, to **1,031, which is 7 bytes over the buffer.**
  `snprintf` clamps, so the overflow would **silently drop the last key** rather
  than crash. §10 D-I's growth to 2,048 is therefore load-bearing: at 2,048 the
  three features together use 1,031 with 1,017 spare. Worst-case `defaults.cfg`
  goes 779 → 1,020, or 1,082 with all three; the read buffer is already 4,096.
* **Fact.** The port has fifteen toggle settings today and the readiness line
  reads "N OF 15". This feature adds three plain toggles — settings 1, 4 and 7 —
  taking it to 18. The number, the two named choices and the two pickers must all
  be excluded from that count, exactly as `SaveChoice` and the three weapon
  pickers already are.
* **Fact, and the reuse question answered.** `SettingKind` today is `Toggle`,
  `SaveChoice`, `EnemyPool`, `EnemySkip`, `BossPool`, `TrickWeaponPool` and
  `LeftHandWeaponPool`. For the two **named two-state choices** — settings 2 and
  8 — `SaveChoice`'s *machinery* is exactly right and already generic: it carries
  one `bool RandomizerDefaults::*`, `AdjustSetting` flips it on either direction
  with no special case, `IsDrillIn` returns false for it, and it is already
  excluded from `ToggleCount`. **But the kind itself cannot be reused**, for two
  specific reasons found in the code rather than guessed: `SettingValueText`
  hard-codes its two labels, `"START FRESH"` and `"KEEP EXISTING"`, inside the
  switch; and `settings_ui_verify.py` asserts *"exactly one SaveChoice entry, and
  it is SAVE DATA"*. So the honest answer is **generalise, do not reuse**: add one
  new kind whose two display labels come from the `SettingDef` rather than from
  the switch, and let both settings 2 and 8 use it. Leaving `SaveChoice` itself
  untouched keeps a shipped setting and its verifier case out of this feature.
* **Fact.** Counting that generalised kind, this feature needs **four** new
  `SettingKind` values, not one: the named choice (shared by settings 2 and 8), a
  number (setting 3), a 14-row area picker and a 22-row insertion picker. The two
  pickers cannot share one kind for the reason `SettingsModel.h` already states
  about the existing three — the selection types are distinct types, so they
  cannot share a pointer-to-member. `SettingKind` goes 7 → 11, and each of the
  four `Selection*` switches gains two cases.
* **Fact.** `SettingDef` carries exactly one member pointer, a
  `bool RandomizerDefaults::*`. It needs an `int RandomizerDefaults::*` with a
  minimum and maximum for the number, and a pair of `const char*` for the named
  choice's two labels. `SettingValueText` gains a decimal case and a
  label-pair case; `AdjustSetting` gains a clamped ±1 case.
* **Fact.** Both screens reach every setting only through the shared accessors —
  the pattern `SaveChoice` already proved — so neither screen's own code needs a
  new branch for any of the four kinds. The value column must render `"15"` and
  `"EXACTLY"`, both narrower than the existing `"KEEP EXISTING"`, so
  `settings_ui_verify.py`'s width cases have slack. The world editor's
  `Step::EditSeed` is a working precedent for a full-screen numeric editor if a
  ±1 rail control proves insufficient for a 0–15 range.
* **Fact, and it confirms the developer's assumption.** A new world is
  pre-filled from the DEFAULTS tab by whole-struct copy —
  `WorldEditorScreen`'s constructor initialises `run_(defaults)` and the comment
  names it as worlds B6 — so **every setting in `RandomizerDefaults` gets
  defaults-seeding for free, and these eight need nothing special to obtain it.**
  There is exactly **one** exception in the whole app and it is deliberate:
  `startFreshSave` is forced to `false` for a new world whatever the defaults say
  (worlds B12). Nobody should generalise from that one line; it is the exception
  that proves the rule.
* **Fact,** and the constraint most likely to be underestimated: both picker
  tables are **generated, and their row order is frozen**, because a selection is
  stored as one character per row positionally and `Decode` can only detect a
  changed count, never a changed order (`ModelPoolSelection.h`,
  `gen_pool_table.py`). Row 13 does **not** force a regeneration of any existing
  table — both its lists are new. Folding row 14 in **would**, and that is the
  main reason §6 leaves it out.

---

## 5. Terminology

**Insertion pass** — the write pass this feature adds: one visit per map file,
after the enemy and boss passes, replacing some ordinary creatures with bosses.
Named to keep it distinct from *boss randomization*, which reassigns boss arenas
and is a different, shipped feature.

**Insertion list** — the boss identities the insertion pass may write, offered to
the player as **INSERTED BOSSES**. A third list, distinct from both the enemy
pool and the boss pool, and larger than either (§4).

**Identity** — one `(NPCParamID, ThinkParamID, model)` triple. The insertion list
holds 32 of them across 22 models, because several bosses have more than one stat
variant. The run's pool is drained by identity, not by model.

**Eligible target** — an ordinary creature the pass is allowed to overwrite: one
that the fixed exclusion list does not name, that **ENEMIES SKIPPED** does not
tick, and that **DO NOT RANDOMIZE CAGED DOGS** does not protect.

**Area** — one of the fourteen explorable places the per-area flags name. An area
corresponds to one map file a retail player loads, and sometimes to one or two
further variant files the retail game does not load. The count is per map file.

**Pace** — the unit MSB world positions are expressed in. Used here only for
relative comparison; nothing in this spec depends on its real-world size.

---

## 6. Scope

### In scope

**Backlog rows 13 and 17.** Row 13 is `insertBossesBool`, "Bosses Can Replace
Enemies". Row 17 is the fourteen per-area flags, folded in under §10 D-A because
the investigation established they gate `InsertBossesVoid` and nothing else in
the reference tool, so they are this feature's own gate rather than a separable
row.

* The **eight settings** of §2, each with label, help text, config key, default
  and persistence.
* Four new `SettingKind` values and the `SettingDef` fields they need, per §4.
* Growing the settings buffer to 2,048, which §4 shows is a prerequisite rather
  than an improvement.
* The insertion pass over the base maps, after the enemy and boss passes.
* The insertion list, built from the boss-candidate harvest the port already
  performs, including the Moon Presence removal.
* The run-long pool, drained by identity and refilled when empty.
* Reading placement positions, and the spacing selection built on them.
* Automated verification of the pass, both count modes, the override, the two
  pickers, the pool property, the AI choice and the spacing property against real
  vanilla data.

### REPLACE ALL ENEMIES is deliberately temporary

It is specified, built and shipped **to be removed**, and the spec says so here so
that its removal is a planned step rather than a later argument. The repository
already has this pattern: `app/src/Randomizer/RuneProbe.h` opens with *"A
TEMPORARY HARDWARE PROBE. Delete this file once it has answered its question; it
is not a feature and must never ship enabled."* and then states the question, the
design and the result that retires it. This setting should carry the same shape of
comment.

* **What it is for.** One question, unanswerable from data: does Bloodborne
  survive a world in which every ordinary creature is a boss? That covers model
  residency, health-bar registration, arena scripting fired outside an arena, and
  whatever else only shows up in bulk. Answering it at the maximum count would
  take many runs; answering it once at 1,381 takes one.
* **What it is expected to do.** Crash, or make the game unplayable. The
  developer's own expectation, recorded verbatim so nobody later reads the setting
  as a feature: *"likely this will just be for a test and we need to take this out
  because it will probably make the game crash."*
* **What retires it.** One hardware run with it on, and the result recorded in
  `log.md`. Whatever the outcome — crash, instability, or an unexpected success —
  the setting is then deleted along with its config key, its `RandomizerDefaults`
  field, its row and its help text, and `pool_verify.py`'s byte arithmetic is
  rebased down by the 22 bytes it cost. If it succeeds, the useful part of the
  result is that the *maximum count* could be raised, which is a different
  decision made with evidence.
* **What must be true while it exists.** It must be off by default, it must not
  be reachable by accident from any other setting, and its help text must say it
  is a test control that is expected to break the game.

### Out of scope

**Backlog row 14, `lesserBossesBool` — "Include Lesser Bosses in Boss Pool" — is
deliberately not covered here**, and folding row 17 in does not change that
argument. Row 17 shares this feature's pass; row 14 shares nothing with it but a
function name.

* They change **different outputs**. Row 14 changes which boss stands in a boss
  *arena* — measured, the boss pool goes from 18 identities across 17 models to
  28 across 22, adding the Witch of Hemwick, Shadow of Yharnam, Celestial
  Emissary, Living Failure and The One Reborn, and removing nothing. Row 13
  changes which creatures stand in the *world*.
* Row 14 **forces a table regeneration and row 13 does not.**
  `BossPoolTable.h` grows 17 → 22 rows, the new rows sort into the middle of a
  display-name ordering, and every saved **BOSSES INCLUDED** selection is
  silently remapped. That is the exact failure the frozen-order discipline of
  features 032 and 037 exists to prevent, and handling it — migrate, invalidate,
  or accept — deserves its own approval rather than riding along inside a feature
  that already carries eight settings and the project's largest unproven runtime
  risk.
* Their only measured interaction is **one line**. Of the five models row 14 adds
  to the boss pool, four are already in row 13's insertion list regardless. The
  single overlap is The One Reborn, which row 14 would add to both lists and which
  carries `ThinkParamID == 0` in vanilla (§4).
* Row 14 is three lines in one function and needs no new pass, no new setting
  kind, and no new verification shape. It is a clean small feature on its own.

Precedent cuts both ways and was checked: feature 024 folded row 25 in because
both rows write the same param column through the same function, and feature 018
folded rows 19–21 in because all four easy modes are literally the same
three-field write with a different name list. Those resemblances hold for row 17
and not for row 14.

Also out of scope:

* **Chalice dungeons.** The reference's chalice branch of this pass is commented
  out in its own source in any case, and its fifteenth area flag, `bossChalices`,
  goes with it. This feature's area picker has fourteen rows, not fifteen.
* `oopsAll` and `oopsAllBosses` modes, neither of which the port has.
* **A navmesh, region or traversal-aware notion of distance.** §4 states the
  limit of Euclidean spacing plainly; closing it is not this feature.
* **Writing positions.** Nothing here moves a placement. A boss stands exactly
  where the creature it replaced stood.
* **Raising the maximum count above 15** on the strength of a successful
  **REPLACE ALL ENEMIES** run. That is a later decision with its own evidence.
* **Flattening the draw weighting** so each boss model is equally likely. Draining
  by identity deliberately preserves it; `docs/deferred-ideas.md` §2 declines the
  same change for the enemy pool.
* **Adding a size gate to this pass.** The reference has none (§4), and adding one
  is a deviation nobody has asked for.
* **Changing the stat-scaling pass** so that inserted bosses scale differently, or
  so the five unscaled identities become scaled. Its opt-out is backlog row 22.
* **Re-expressing SAVE DATA as the new named-choice kind.** It would fit, but it
  changes a shipped setting and its verifier case for a feature that is not about
  it.
* **Correcting `docs/plans/pickers.md` §7.5, `032/spec.md` §8 and row 17's
  backlog description**, all three of which are wrong about this feature.
  Reported here; correcting them belongs to the documentation stage.

---

## 7. Constraints and decisions

### Reference fidelity, and the six places this feature leaves it

`CLAUDE.md` §7 requires a deviation to be justified rather than assumed. There
are six, and each is listed with what it departs from and why.

1. **A count per map replaces the reference's percentage.**
   *Departs from:* `bossPercentage`, rolled per placement.
   *Justification:* the reference's slider has **no field initializer**
   (`FieldContainer.cs:169`) and is written only by its own event handlers, so on
   any run where the user does not touch it the value is 0.0 — and because the
   test is `<=` against an inclusive `Next(0, 101)`, that still converts one
   placement in 101, about 16 per run. A control whose shipped default fires on
   1% of placements by arithmetic accident is a defect, not a design, and the port
   has direct precedent for replacing exactly this defect class with a real
   working value: `kSizeToleranceMultiplier = 2.0` exists because `sizeOfEnemy`
   is an unset slider in the reference too, and that substitution is already
   documented and accepted in `EnemyRandomizer.cpp`'s header. The count's default
   of one per map gives 14 per run, landing within two of the reference's own
   accidental default — so the deviation changes the *control*, not the shipped
   experience.
2. **UP TO / EXACTLY, a mode the reference has no notion of.**
   *Departs from:* an independent per-placement coin flip — but less than the
   first draft of this spec assumed. Under D-L, **UP TO** *is* a per-placement
   draw, with the probability calibrated from the count and a ceiling at the
   count; the reference's coin flip is the same shape with the probability taken
   from an uninitialized slider and no ceiling at all. **EXACTLY** is the genuine
   departure.
   *Justification:* the two answer different player questions — "surprise me,
   differently in each area" and "give me exactly this many so I know what I am
   walking into" — and a count with no mode has to silently pick one of them.
   The measurements also make **EXACTLY** safe to offer: at a maximum of 15
   against the smallest map's 54 eligible targets, the quota is satisfiable
   everywhere at default settings, so the mode is meaningful rather than
   aspirational.
3. **A maximum count of 15.**
   *Departs from:* the reference's unbounded 0–100 slider.
   *Justification:* measured in §4, **15 is the last count at which spreading the
   bosses out still keeps every map's closest pair above 20 paces** — at 20 the
   worst map falls to 12 and at 25 to 10, which is what a *uniform* draw already
   gives at a count of 5. Beyond 15 the feature's own spacing setting can no
   longer deliver what it promises, so the range stops meaning anything. 15 is
   also under a third of the smallest map's supply, keeping **EXACTLY**
   satisfiable, and 15 × 14 = 210 is 15.2% of the world's creatures. The extreme
   is not lost: **REPLACE ALL ENEMIES** covers it, which is precisely why the
   count no longer has to gesture at it.
4. **Spreading the chosen placements out.**
   *Departs from:* the reference's per-placement coin flip, which has no notion of
   where a boss lands.
   *Justification:* measured, a uniform draw at ten per map puts the closest two
   bosses 4–17 paces apart on average — the same room — and the feature's point is
   extra *encounters*, not one crowded fight. It is opt-in, it is provably a no-op
   at the default count of one, and it changes only which eligible placement is
   chosen, never what is written into it. It is *reachable* because the pass is a
   second pass (§10 D-N): it holds the whole map's eligible placements before it
   chooses any, which a per-placement decision taken during pass 1 could not.
5. **Drawing without replacement, with refill, across the whole run.**
   *Departs from:* the reference's uniform draw with no removal, which can put
   four Living Failures in one map and never show Gehrman at all.
   *Justification:* it matches the port's own shipped boss randomizer exactly —
   `BossPool` in `BossRandomizer.h` is drained as arenas are assigned and refilled
   from a second list when it empties — so this is internal consistency rather
   than invention. Draining **by identity** rather than by model is deliberate and
   preserves the reference's weighting (§4). Draining across the **run** rather
   than per map is what makes the refill rule reachable at all: a 29-identity pool
   cannot be exhausted by a count of 15 in one map.
6. **Choosing the arriving boss's AI.**
   *Departs from:* the reference, which always writes the boss's own AI except in
   one case where it drops the assignment by accident.
   *Justification:* the accident is the only evidence that the alternative is even
   expressible, and it is unfinished rather than chosen — it computes an id and
   discards it, on a value that appears nowhere in the game data. Making it a
   setting completes the evident intention without resurrecting an unverifiable
   id, and defaults to the boss's own AI so the reference's *effective* behaviour
   is what an untouched run gets.

### Everything else that must be respected

* **The pass writes last among the creature passes, and that is behaviour.** The
  reference runs it after both the enemy and the boss pass, and the port's easy
  modes already depend on a comparable ordering claim (`EnemyRandomizer.cpp`
  header). Its position relative to the stat-scaling pass decides whether an
  inserted boss gets its area's variant or its arena's, and §4 measures that 25 of
  32 identities turn on it — the seven untracked ones being three Witch of
  Hemwick, three Living Failure and the Small Celestial Emissary.
* **An empty INSERTED BOSSES selection skips the pass silently.** No error, no
  warning, no commit-time refusal. This is deliberately **different** from
  **ENEMIES INCLUDED**, whose empty selection *is* refused at commit, and the
  difference is correct: an empty enemy pool would leave the run unable to
  populate a world it has already started rewriting, which is why feature 032 had
  to separate that case from a genuine failure. An empty insertion list simply
  means no bosses are inserted, and a world with no inserted bosses is a perfectly
  good world. The reference agrees by accident: `oopsAll` empties the same list
  and the pass becomes a silent no-op.
* **The run must never fail because a count cannot be met.** Feature 032 D4 exists
  because an empty-pool `Fail` fired after the mirror phase had copied all six
  folders, leaving the player a half-built tree and an error they could not act
  on. A shortfall here is reported, never fatal.
* **Both picker tables are generated and their row order is frozen**, the 22-model
  insertion table and the 14-row area table alike. A selection is stored
  positionally and a reordering cannot be detected at runtime. This feature must
  not reorder or renumber `EnemyPoolTable.h`, `EnemySkipTable.h` or
  `BossPoolTable.h` either.
* **The settings buffer is a shared resource, and this feature overflows it.**
  At 1,024 bytes the block cannot hold this feature plus features 011 and 027 —
  it lands 7 bytes over, and `snprintf` would truncate the last key in silence.
  §10 D-I grows it to 2,048 here. The exact-equality assertion in
  `pool_verify.py` stays, so the real figure remains pinned; **whichever of the
  three features lands last rebases the arithmetic**, and all three must agree on
  the buffer size rather than each assuming the old one.
* **Nothing about the shipped enemy or boss passes may change.** A run with
  setting 1 off must be byte-identical to today's, roll for roll — which means the
  pass must draw no randomness when it is off, the way `doNotRandomizeCagedDogs`
  and the easy modes already do not.
* **The 103-pattern fixed exclusion list is not to be extended.**
  `docs/design-decisions.md` and `docs/enemy-exclusion-history.md` record 119
  placements frozen by a well-meant extension; if a boss must be kept out of a
  particular place, that is a rule in this feature, not a new exclusion pattern.
* **Positions are read, never written**, and read at a fixed offset without
  decoding anything else in the blob — the convention `Msbb.h`'s header states for
  every other field the port pokes.
* **`SaveChoice` and its verifier case are not to be touched.** The new named
  choice is a generalisation beside it, not a replacement for it.
* **Labels and help text are drawn in an uppercase-only 8×8 font with no colon**
  and are size-checked by `settings_ui_verify.py`; an unrenderable character draws
  as a full-width blank column. The eight labels in §2 are provisional and must
  pass those width cases. Eight new rows in the **Bosses** group also change that
  pane's row count, which the geometry cases check.
* **Hardware is the only authority on whether a loose boss is stable, and on
  whether a boss can run a mob's AI at all.** A clean build and a passing verifier
  mean ready for hardware testing, not correct (`CLAUDE.md` §3). The reference's
  own crash warning makes this the feature on the list least entitled to be
  assumed safe.

---

## 8. How will we know it works?

### Acceptance criteria

**The pass, off and on**

1. With setting 1 **off**, the generated tree is byte-identical to a tree
   generated from the same seed and settings today.
2. With setting 1 **on**, boss identities appear at ordinary-creature placements
   and at no other kind of placement, and only in map files belonging to ticked
   areas.

**How many — one criterion per configuration**

3. **EXACTLY N**, override off, list non-empty, area ticked: the number of
   placements this pass changed in a map file is **exactly N**, or exactly that
   file's eligible-target count if that is smaller. Never more. An equality, not
   a statistical bound — which is the point of a count replacing a percentage.
4. **UP TO N**, same conditions: the number changed in a map file is **between 0
   and min(N, that file's eligible targets)** inclusive — N is a ceiling and is
   never exceeded. Two further properties, both from D-L and D-M, and both
   needed because a ceiling alone would be satisfied by a pass that does
   nothing:
   * Across the fourteen areas of one run at N ≥ 5, **not every map is at N**.
     Otherwise the mode is **EXACTLY** wearing a different label.
   * Over many seeds, a map's **mean count approaches N rather than a fraction
     of it**, and that mean is **unchanged when INSERTED BOSSES is narrowed to a
     single boss**. This is the criterion that fails if the probability is taken
     from pool composition instead of from N: measured in §4, one entry per
     ticked boss would put the single-boss mean at 0.16 in the smallest map
     against 4.3 for all 29, and D-M forecloses exactly that.
5. **REPLACE ALL ENEMIES on**: the number changed in a map file is **exactly that
   file's eligible-target count** — 1,381 across the fourteen live areas, 2,269
   across all 24 files — and no eligible placement anywhere is left unreplaced.
   Settings 2 and 3 have no effect on the result.
6. **Count 0**, override off: **zero** placements changed anywhere, and the run
   succeeds.
7. **INSERTED BOSSES empty**: **zero** placements changed anywhere; the run
   succeeds; the log records that the pass was skipped; **no error is shown and
   nothing is refused at commit.**
8. **AREAS INCLUDED empty**: zero placements changed anywhere, and the run
   succeeds.

**What arrives, and how it was dealt**

9. Every replaced placement carries an identity from the insertion list of §4 —
   32 identities across 22 models, Moon Presence never among them, and only
   models the player has ticked — with its `NPCParamID` and model consistent with
   each other and with a real vanilla identity. No fabricated values.
10. **The pool property**: across a whole run, the number of times any two enabled
    identities were used differs by **at most one**. This is what
    draw-without-replacement-with-refill guarantees, and it is checkable on output
    without predicting a single placement.
11. **BOSS BEHAVIOUR = OWN AI**: each replaced placement's written
    `ThinkParamID` equals the drawn identity's own. **= REPLACED ENEMY'S AI**:
    the written `ThinkParamID` equals that placement's **vanilla** value while its
    `NPCParamID` and model are the boss's. Both are exact equalities on real
    output.

**Where it arrives**

12. **SPREAD BOSSES OUT on** — available because D-N's second pass sees every
    eligible placement in the map before choosing any: the minimum pairwise
    distance among a map file's chosen placements is at least as large as the
    uniform draw's measured expectation for that file and count, and no smaller than greedy
    farthest-point would achieve from the same first pick. **Off**: the choice is
    a plain draw without replacement over the file's eligible targets. At a count
    of 1 the two states produce the same placement and consume the same number of
    draws.

**What must not change**

13. Every vanilla boss placement is unchanged by this pass — all 42 boss-named
    placements across the boss maps — so the arenas remain exactly as **RANDOMIZE
    BOSSES** left them, under **REPLACE ALL ENEMIES** included.
14. The Hunter's Dream and the Abandoned Old Workshop are unchanged, and are not
    offered as areas.
15. The 360 creatures the fixed exclusion list spares are unchanged, as is every
    placement **ENEMIES SKIPPED** ticks and, when it is on, all 26 caged-dog
    placements — under **REPLACE ALL ENEMIES** included.
16. An inserted boss in a stat-scaled area carries that area's variant, for the 24
    identities the scaling table tracks.
17. Only the three fields the port already writes on an enemy blob are written.
    Position, rotation and scale are unchanged byte-for-byte.
18. A map file with fewer eligible targets than the quota asked for — reachable by
    ticking five skip rows against Old Yharnam (§4) — completes, inserts as many
    as it can, and reports the shortfall. The run does not fail.

**The settings chain**

19. All eight keys round-trip through `defaults.cfg`, including the number at both
    ends of its range and both states of each named choice. An existing
    `defaults.cfg` still loads. No existing picker selection is reinterpreted.
20. The settings block fits the buffer **with features 011 and 027 also present**,
    and `pool_verify.py`'s exact-equality assertion matches the real figure.
21. A new world created from the DEFAULTS tab starts with all eight settings as
    the tab shows them.

### Automated testing

Verifiable without running the game: everything above except the game's reaction.
Criterion 1 is a tree diff. Criteria 2–18 are properties of real generated output
measured against the real vanilla input, which is the shape `boss_verify.py
verify` already has and the shape that cannot be satisfied by a misread rule
shared with the C++.

Three are worth calling out as unusually strong for this project. Criteria 3 and 5
are **equalities on a count**, which the percentage design could never have given.
Criterion 10 pins the whole draw discipline without reproducing the PRNG.
Criterion 11 pins the AI choice exactly, in both states, on vanilla values the
checker reads independently.

Criterion 4 needs more than one map to be meaningful and criterion 12 needs a
measured baseline rather than a reproduction of the selection, for the same reason
`boss_verify.py` does not try to predict which boss lands where.

The oracles this needs — the eligible-target set, the insertion list, the
stat-scaling map, and the map-file reader — all already exist in `app/tools/`, and
the position field is three floats at a known offset in the record those readers
already walk. Reuse rather than rewrite.

### Hardware testing

The console must answer the questions the data cannot, and this feature's hardware
test is not a formality. It should be run in this order, so a failure can be
attributed:

1. **Default count, 1 per area, OWN AI, spacing off.** Fourteen bosses, the
   mildest configuration the feature has. Does a boss standing in the world
   **act** — aggro, attack, and die — with no fog gate and no arena trigger? Is
   there a **health bar**, **music**, or a **boss-defeated message**, and does its
   absence or presence break anything? Does a **save made in an affected area
   still load**?
2. **Maximum count, spacing off, then on.** Do two bosses the data says are four
   paces apart actually fight the player together, and is that survivable? Does
   spacing visibly change where they are?
3. **REPLACED ENEMY'S AI**, at the default count. **This is the test most likely
   to fail**, and §4 says why: the boss may stand inert, flail, or crash on its
   first behaviour lookup. A clean result here is a genuine discovery; a crash
   here retires nothing but does tell us what the reference's warning was about.
4. **REPLACE ALL ENEMIES**, last, once. Expected to crash or to make the game
   unplayable. Whatever happens is recorded in `log.md` and the setting is then
   removed (§6).

Other things the test must look at: do the **multi-phase** bosses transition, and
what happens to a boss whose second phase is an arena script? Do the **five
unscaled** identities — the Witch of Hemwick, the small Celestial Emissary, and
three Living Failure variants — arrive at arena strength in the first area?

The most important failure cases: a crash on entering an affected area; a boss
that stands inert and cannot be killed, in a doorway the player must pass; and a
boss whose death fails to fire an event the area needs.

---

## 10. Decisions

| Date       | Decision |
| ---------- | -------- |
| 2026-09-28 | **D-A (was Q2) — row 17 is folded in.** This spec covers backlog rows **13 and 17**. The fourteen per-area flags ship as part of this feature, because the investigation established they gate `InsertBossesVoid` and nothing else in the reference tool, so they are this feature's own gate rather than a separable row. Row 14 remains excluded — see §6. |
| 2026-09-28 | **D-B (was Q3) — the insertion list gets its own table and picker**, 22 rows, frozen order, its own config key. `BossPoolTable.h` and the **BOSSES INCLUDED** setting are untouched, so no saved selection changes meaning. |
| 2026-09-28 | **D-C (was Q4) — ENEMIES SKIPPED and DO NOT RANDOMIZE CAGED DOGS both protect a placement from this pass**, under **REPLACE ALL ENEMIES** included. A creature the player has told the run to leave alone is not replaced by a boss either, and the 26 caged-dog placements stay dogs. |
| 2026-09-28 | **D-D (was Q1) — the pass is governed by a count, not a percentage.** A deviation from the reference, justified in §7 item 1: the reference's percentage slider has no field initializer, so its shipped default fires on ~1% of placements by arithmetic accident rather than by design. |
| 2026-09-28 | ~~**D-E — the maximum count is 10.**~~ **Superseded by D-K.** |
| 2026-09-28 | **D-F — bosses in a map may be spread out**, under **SPREAD BOSSES OUT**, by greedy farthest-point selection over placement positions read from the map records. A deviation from the reference, justified in §7 item 4. Its guarantee is spatial and not traversal-aware, and §4 states that limit. |
| 2026-09-28 | **D-G (was Q7) — the count is three settings, and both modes ship.** **BOSS REPLACEMENTS** is a named two-state choice, **UP TO** or **EXACTLY**; **BOSS REPLACEMENTS PER MAP** is a number from 0 to the maximum; **REPLACE ALL ENEMIES** is an override that ignores both and replaces every eligible placement. D-G also settles three sub-questions: an **empty INSERTED BOSSES selection skips the pass silently** — not a refusal, not a failure, and deliberately unlike **ENEMIES INCLUDED**'s commit-time refusal, for the reason given in §7; the pass **draws without replacement and refills when the pool empties**; and that pool is **drained by identity across the whole run**, not per map, per the reasoning in §4 and §7 item 5. |
| 2026-09-28 | **D-H (was Q8) — SPREAD BOSSES OUT defaults OFF.** The developer's accompanying assumption is confirmed: a new world is pre-filled from the DEFAULTS tab by whole-struct copy, so this and all eight settings get defaults-seeding with no special work. The single deliberate exception in the whole app is `startFreshSave`, forced false for a new world regardless of the defaults (worlds B12) — see §4. |
| 2026-09-28 | **D-I (was Q9) — grow `char buf[1024]` to 2,048** as part of this feature, keeping `pool_verify.py`'s exact-equality assertion so the real figure stays pinned. Measurement taken since: this is **not optional**. The eight settings cost 241 bytes, taking the block to 969, and with features 011 and 027 to **1,031 — seven bytes over the current buffer**, which `snprintf` would truncate silently. The buffer is a shared resource and all three features land in it. |
| 2026-09-28 | **D-J (was Q5) — the arriving boss's AI is a setting.** **BOSS BEHAVIOUR**, defaulting to the boss's **own** `ThinkParamID`; the other state keeps the replaced creature's, completing the reference's abandoned edit without resurrecting its unverifiable 551111. Both ship. The risk is real and stated in §4 and §8: AI scripts name model-specific animations and behaviours, so a boss body on a mob's script may not work at all, and only hardware can say. |
| 2026-09-28 | **D-K — the maximum count is 15**, superseding D-E's 10. Two things changed: **REPLACE ALL ENEMIES** now covers the extreme, so the count's job is the middle of the range rather than gesturing at everything; and the spacing measurement in §4 identifies 15 as the last count at which spreading still keeps every map's closest pair above 20 paces, against 12 at a count of 20 and 10 at 25. Full justification in §7 item 3. |
| 2026-09-28 | **D-L (was Q10) — UP TO means "about N, and never more than N"; the pass runs as a second pass over the map, and N is the only control over how many arrive.** The developer rejected both readings the question offered. The mechanism is: **pass 1 randomizes the map's enemies as it does today; pass 2 then walks the same map and converts placements to bosses**, drawing which boss from **INSERTED BOSSES** and which placement from the map's eligible targets. **EXACTLY N** drives the map to N. **UP TO N** treats N as a ceiling that the draw approaches but need not reach, so the count varies between areas and between seeds. Stated by the developer as the governing case: *"If the number is 5 and only Amygdala is selected then we get either up to 5 Amygdalas or exactly 5 Amygdalas in the map."* **The count therefore governs the number and the selection governs only the variety** — see D-M for the measurement that makes this the only self-consistent reading, and §7 item 2. |
| 2026-09-28 | **D-M — no rate setting, and the boss share is derived from N rather than from pool composition.** The developer declined a ninth setting for the rate. This forecloses the naive form of the pool-mixing mechanism: measured, the enemy pool is **333 entries**, so one entry per ticked boss makes the boss share **8.77% with all 32 ticked but 0.30% with one**, giving 4.7 expected bosses in the smallest map against 0.16 — the Amygdala case D-L names would produce almost none, and the cap, being a ceiling, cannot raise it. Narrowing **ENEMIES INCLUDED** distorts it the other way, a 10-model pool making 74% of draws bosses. So pass 2's per-placement probability is **calibrated from N and the map's eligible-target count**, not from how many bosses are ticked. Ticking fewer bosses changes **which** bosses arrive, never **how many**. |
| 2026-09-28 | **D-N — D-F stands unchanged: SPREAD BOSSES OUT keeps greedy farthest-point selection.** The question put to the developer assumed a sequential per-placement draw, under which up-front selection is impossible; the developer's two-pass answer removes the assumption. **Pass 2 sees every eligible placement in the map before it chooses any**, so farthest-point selection over placement positions is available exactly as D-F specified, and §8 criterion 12 is unaffected. This also restores the reference's own pass ordering — `InsertBossesVoid` runs after enemy randomization (§3) — so the port's structure matches it rather than departing from it. |
| 2026-09-28 | **D-O (was Q6) — the reference's instability warning is carried as help text, on three rows rather than one.** The reference pops a modal on ticking this one checkbox (§3); the port has no modal for any setting and will not grow one for this. Instead: **BOSSES CAN REPLACE ENEMIES** says the feature may make the game unstable or crash, **REPLACE ALL ENEMIES** says it is a test control expected to break the game, and **BOSS BEHAVIOUR** says its second state may not work at all. Three rows because this feature now carries three distinct risks, where the reference had one. Help text is where the port already carries per-setting caveats, `settings_ui_verify.py` already pins it non-empty and size-checked, and the font's uppercase-only set can express all three. **Revisit after the hardware test:** if the feature proves stable, the first warning can soften — and per `docs/known-traps.md` that is a claim about observed behaviour, so it must not harden into a label. |

---

## Appendix — research notes for stage C

*Not part of the specification, and not covered by the developer's approval. These are pointers from the spec investigation to save stage C a search. Verify anything here before relying on it.*

**Where the behaviour lives**

* `reference/.../RandomizeFunctions.cs:1460-1552` — `InsertBossesVoid`, the whole
  pass, including the Moon Presence strip at the top and the two defects.
  `:1538-1545` is the abandoned think-id edit D-J generalises.
* `reference/.../StartFunctions.cs:1013-1133` — the driver: the global dedupe, the
  fourteen per-area gates in order, and the pass's position in the run. The order
  those fourteen appear in is a candidate for the area table's frozen row order.
* `reference/.../StartFunctions.cs:786-802` — where the boss-exclusion list is
  applied to the insertion list, and the only place the two interact.
* `reference/.../RandomizeFunctions.cs:1838-1846` — where the insertion list is
  serialized, and `:1650-1665` where entries are harvested. The harvest sits
  inside the `addEnemy = true` branch, which is why the list is a superset of the
  boss pool; that placement is the fact the six extra models of §4 come from.
* `app/src/Randomizer/BossRandomizer.cpp` — `CollectBossCandidates` is the port of
  the same loop and is where the insertion list would be harvested; note
  `kInsertReject` is the narrower list the harvest happens *before*. `DrainPool`
  and `BossPool::refill` are the shipped precedent D-G's pool follows, including
  the refill-on-empty step.
* `app/src/Randomizer/EnemyRandomizer.cpp` — `StepWriteMap` is the existing
  target-eligibility test and the existing three-field write; `State::Phase` is
  where a new pass gets sequenced; `IsSkippedName` and `IsProtectedCagedDog` are
  the two tests D-C needs; and the file header's easy-modes bullet is the
  precedent for treating pass order as behaviour.
* `app/src/Randomizer/BossParamScaling.h` — `BossScalingMaps()` names exactly the
  fourteen live area files plus the two Hunter's Dream files, which is how §4
  decided which map file each area corresponds to.
* `app/src/Msb/Msbb.h` — the header comment enumerates every fixed offset the port
  pokes and is what the position offset was cross-checked against; `part_fields`
  is where a position accessor belongs.
* `reference/SoulsFormats/.../MSBB/PartsParam.cs` — the `Part(BinaryReaderEx)`
  constructor is the authority for the record layout.
* `app/src/Randomizer/RuneProbe.h` — the model for how **REPLACE ALL ENEMIES**
  should be commented: first line says it is temporary, then the question, the
  design, and what retires it.

**Worth checking early**

* `app/src/UI/SettingsModel.cpp:213-252` — `SettingValueText` hard-codes
  `SaveChoice`'s two labels inside the switch, and `AdjustSetting`'s `twoState`
  test and `IsDrillIn` are already generic over it. Those three lines are the
  whole reuse question; §4 has the answer but the code is where it is visible.
* `app/tools/settings_ui_verify.py:893` — *"exactly one SaveChoice entry, and it
  is SAVE DATA"*. This is the other half of why the kind cannot simply be reused.
* `app/tools/enemy_lookup.py` — `overwritable_placements`, `exclusion_reason` and
  `zone_scaled_npc` are three of the four oracles this feature's verification
  needs, and all three already exist. `zone_scaled_npc` returns its input
  unchanged for an untracked value, which is easy to misread as "everything is
  scaled".
* `app/tools/boss_verify.py` — `build_pool` is the boss-pool oracle; the insertion
  list is a *different* projection of the same loop and needs its own function
  rather than a parameter on that one. Its `Msbb` class already walks the part
  record this feature needs three more floats out of.
* `app/tools/gen_pool_table.py` and `ModelPoolSelection.h` — read the frozen-order
  rule and `Decode`'s length-only guard before touching any table. The area picker
  is `ModelPoolSelection<14, true>` in shape; whether `ModelPoolEntry`'s `model`
  field should carry a map name or something else is a judgement the plan should
  make explicitly rather than by analogy.
* `app/tools/pool_verify.py:688-734` — the settings-block arithmetic and the
  comment narrating every previous rebase. Eight keys and a buffer change land
  here at once, and the arithmetic has to be rebased on whichever of 011 and 027
  has landed.
* `docs/plans/pickers.md` §7.4 — the Father Gascoigne case, where one name means
  different creatures in two lists. A third boss-shaped picker sets that trap
  again, and §2 already has to explain it to the player.
* `app/src/UI/WorldEditorScreen.cpp` — the constructor's `run_(defaults)` and its
  B6 comment are the confirmation behind D-H, and `Step::EditSeed` /
  `UpdateEditSeed` / `DrawEditSeed` is the existing full-screen numeric editor if a
  ±1 rail control proves insufficient for a 0–15 range.
* A few placements share exact coordinates (§4), so any spacing implementation has
  to tolerate a zero distance without dividing by it or looping forever.

**Dead ends already walked**

* The enemy pool. Row 13's backlog text, `docs/plans/pickers.md` §7.5 and
  `032/spec.md` §8 all say this feature adds bosses to it. It does not, and
  `EnemyPoolTable.h` needs no change.
* The model-size gate. It is in the enemy loop only, the reference's insertion pass
  has none, and measuring what a ×2 gate would admit shows it would exclude
  nothing outright — so it is not the fairness lever it looks like.
* A per-map pool. 32 identities cannot be exhausted by a count of 15, so the
  refill rule would never execute and every map would start from the same full
  list. Measured, not assumed.
* Reusing `SettingKind::SaveChoice` for the two named choices. The machinery fits
  and the kind does not, for the two reasons above.
* Boss randomization's map order as an explanation for anything here: the port
  collects the boss pool over the assignment order rather than the reference's
  collection order, which `docs/plans/boss-randomization.md` §6 does not record as
  a deviation — but the insertion list is bit-identical either way, measured.
  Worth someone's attention for the boss pool itself; irrelevant to this feature.
* Per-area control via `WorldStore`. "World" in this port is a playthrough, not a
  game area; there is no existing per-area machinery to reuse, which is why row 17
  needs a table.
* `docs/known-traps.md` — read in full for this feature; nothing in it sits on this
  path beyond the two general rules already cited in §4 and §7.
