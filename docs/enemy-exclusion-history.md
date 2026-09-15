# Enemy Exclusion History — the removed `EnemyExclusionListExtra` list

**Status: removed from the code on 2026-09-13.** This document preserves what
it contained, what each entry actually was, and why it went away, so that
individual entries can be re-added with justification if hardware testing shows
a real problem. Nothing here is active behavior.

## What it was

45 name-substring patterns layered on top of the reference Windows tool's own
`unusedList` + `bossList`. Any enemy placement whose name contained one of them
was excluded from enemy randomization — both from the candidate pool and from
being overwritten.

## Why it was added (2026-09-09)

While debugging a report of "bosses randomizing and enemies not attacking with
only Randomize Enemies on", a byte-diff showed that `c1130_0000` in Cathedral
Ward was being overwritten — in this port's output *and* in the reference
Windows tool's own output. I generalised that single observation into 45
entries with a heuristic: *a placement with a real (scripted) EntityID, plus
either a rare model (≤3 placements across the base maps) or a name prefix
appearing nowhere else.* It deliberately erred toward over-protecting, on the
reasoning that a spared placement costs only variety while an unprotected boss
costs a broken fight.

## Why it was removed (2026-09-13)

Three reasons, in order of weight:

1. **The symptom that motivated it had a different cause.** The bosses and
   items changing were later traced (2026-09-12) to a contaminated
   `VanillaSource` — the `.bak` polarity problem, where a previously-randomized
   tree was being read as vanilla. That explained the original report. The
   exclusion list was never re-justified afterwards.
2. **It was over-reaching badly.** Patterns are matched as substrings with no
   map scoping, so an entry derived from one placement froze every same-named
   placement in every map. 45 patterns froze **119 placements**.
3. **Most entries protect common trash mobs.** Naming the models (via the
   Smithbox alias data, see `tools/data/`) showed 18 of the 45 protect
   creatures with 40+ placements game-wide — the worst being Huntsman
   (Transformed) at 283. That produced the user-visible symptom of specific
   enemies near the start of the game never changing across runs.

## Independent finding: the reference tool does NOT protect these

Verified against three saved Windows enemies-only runs: `c1130_0000` matches
nothing in the Windows tool's lists and was overwritten in all three runs (to a
Slime Scholar, a Cloaked Beast Patient, and a Huntsman). So this list was never
a reproduction of reference behavior — it was a deviation.

## The full list, with identities

Names come from the Smithbox alias data and are community-curated, not
extracted from the game's own text — good for orientation, not authoritative.

| Pattern | Creature | Placements of that model | Placements this froze | Zones |
|---|---|---|---|---|
| `c2630_0000` | Huntsman (Transformed) | 283 | 8 | Cathedral Ward, Central Yharnam, Hemwick Charnel Lane, Yahar |
| `c2630_0008` | Huntsman (Transformed) | 283 | 8 | Cathedral Ward, Central Yharnam, Hemwick Charnel Lane, Yahar |
| `c1090_0010` | Cloaked Beast Patient | 104 | 2 | Old Yharnam |
| `c1240_0004` | Shaggy Hunting Dog | 96 | 7 | Central Yharnam, Forbidden Woods, Yahar'gul, Unseen Village |
| `c1240_0014` | Shaggy Hunting Dog | 96 | 3 | Central Yharnam |
| `c2632_0055` | Forest Huntsman (Transformed) | 66 | 2 | Forbidden Woods |
| `c1120_0001` | Rotted Corpse | 63 | 2 | Forbidden Woods |
| `c1120_0006` | Rotted Corpse | 63 | 2 | Forbidden Woods |
| `c1100_0001` | Rat | 57 | 4 | Central Yharnam, Research Hall |
| `c2500_0004` | Small Celestial Emissary | 57 | 5 | Central Yharnam, Forbidden Woods |
| `c4040_0020` | Fishwitch | 50 | 1 | Fishing Hamlet |
| `c4040_0046` | Fishwitch | 50 | 1 | Fishing Hamlet |
| `c4040_0047` | Fishwitch | 50 | 1 | Fishing Hamlet |
| `c2600_0000` | Large Huntsman | 48 | 5 | Central Yharnam, Forbidden Woods |
| `c2600_0005` | Large Huntsman | 48 | 5 | Central Yharnam, Forbidden Woods |
| `c2600_0007` | Large Huntsman | 48 | 3 | Central Yharnam |
| `c2610_0008` | Hemwick Grave Woman | 46 | 3 | Hemwick Charnel Lane, Yahar'gul, Unseen Village |
| `c4020_0065` | Enlarged Head Patient | 41 | 1 | Research Hall |
| `c2700_0002` | Church Servant | 34 | 4 | Cathedral Ward, Upper Cathedral Ward |
| `c1000_0003` | Wolf Beast | 31 | 4 | Old Yharnam, Upper Cathedral Ward |
| `c2620_0004` | Wheelchair Huntsman (Gatling Gun) | 25 | 3 | Cathedral Ward, Research Hall |
| `c2620_0005` | Wheelchair Huntsman (Gatling Gun) | 25 | 2 | Cathedral Ward |
| `c2020_0000` | Snatcher | 22 | 6 | Cathedral Ward, Forbidden Woods, Yahar'gul, Unseen Village |
| `c2020_0002` | Snatcher | 22 | 4 | Cathedral Ward, Yahar'gul, Unseen Village |
| `c2170_0003` | Vermin Host | 22 | 2 | Forbidden Woods |
| `c2170_0007` | Vermin Host | 22 | 2 | Forbidden Woods |
| `c2170_0008` | Vermin Host | 22 | 2 | Forbidden Woods |
| `c3100_0004` | Cramped Casket | 22 | 2 | Yahar'gul, Unseen Village |
| `c2730_0010` | Chapel Giant | 20 | 2 | Cathedral Ward |
| `c2000_0002` | Executioner | 18 | 1 | Hemwick Charnel Lane |
| `c2110_0002` | Garden of Eyes | 16 | 2 | Byrgenwerth |
| `c2330_0007` | Cainhurst Servant | 16 | 1 | Forsaken Castle Cainhurst |
| `c4110_0009` | Large Nightmare Huntsman | 15 | 1 | Hunter's Nightmare |
| `c4110_0012` | Large Nightmare Huntsman | 15 | 1 | Hunter's Nightmare |
| `c4000_0025` | Nightmare Huntsman | 14 | 1 | Hunter's Nightmare |
| `c2400_0001` | Giant Lost Child | 13 | 2 | Nightmare Frontier, Nightmare of Mensis |
| `c2400_0003` | Giant Lost Child | 13 | 1 | Nightmare of Mensis |
| `c2560_0000` | Winter Lantern | 10 | 3 | Fishing Hamlet, Nightmare Frontier, Nightmare of Mensis |
| `c2100_0002` | Witch of Hemwick | 9 | 2 | Yahar'gul, Unseen Village |
| `c2121_0000` | Shadow of Yharnam (Snake) | 8 | 2 | Forbidden Woods |
| `c1130_0000` | Labyrinth Ritekeeper | 2 | 2 | Cathedral Ward |
| `c1300_0000` | Nightmare Apostle | 1 | 1 | Nightmare of Mensis |
| `c2321_0000` | Matyr Logarius (Sword) | 1 | 1 | Forsaken Castle Cainhurst |
| `c4051_0000` | Lightning Summoner | 1 | 1 | Fishing Hamlet |
| `c5160_0000` | Mensis Brain (Dropped) | 1 | 1 | Nightmare of Mensis |

## The entries most worth reconsidering

If testing shows something broken, these are the candidates — each protects a
boss sub-entity that the reference tool's list genuinely misses, which is the
same class of problem as the Emissary desync already fixed in boss
randomization:

| Pattern | What it is | What the reference protects instead |
|---|---|---|
| `c2321_0000` | Matyr Logarius (Sword) | Logarius himself (`c2320_0000`), not the sword |
| `c2121_0000` | Shadow of Yharnam (Snake) | the three Shadows (`c2120_*`), not the snakes |
| `c2100_0002` | Witch of Hemwick (third placement) | witches `c2100_0000`/`0001` only |
| `c5160_0000` | Mensis Brain (Dropped) | nothing |
| `c1130_0000` | Cathedral Ward chapel figure | nothing |

`c1130_0000` is the best-evidenced of them: its vanilla `ThinkParamID` is **0**,
meaning no combat AI, and the user identified it from an external data sheet as
the Oedon Chapel dweller (internal name デーモンの狂信者 / DemonsFanatic).
Replacing a non-combat hub NPC with a hostile monster is a plausible way to
break the NPC-rescue mechanic, though that has not been observed in game.

## A principled alternative, parked

The reference tool's `GenerateEnemyList` already refuses to put any placement
with `ThinkParamID <= 1` **into** the pool — it treats them as non-enemies. But
`Randomize()` never applies the same test when choosing what to **overwrite**.
Applying that existing rule symmetrically would protect non-combat placements
on principle rather than by name.

Measured: 276 placements have `think <= 1`; 255 are already covered by the
reference's own list, 4 were covered only by this removed list (including
`c1130_0000` and a Shadow of Yharnam snake), and **17 are protected by nothing
at all today** — including six more Shadow of Yharnam snakes and an Orphan of
Kos (Shell).

This is deliberately **not** implemented. It is still a deviation from the
reference, and the project's standing preference is fidelity unless a concrete
problem justifies otherwise.
