# Randomization Feature Spec — Full Inventory and Status

The single running list of every randomization setting: what it does, where the
reference implements it, and where this port stands. Worked through one at a
time; update the Status column as each lands.

Derived from **`reference/Randomizer/MainWindowComponents/BooleanHandler.cs`**, which is the
authoritative settings list — the XAML checkbox labels are incomplete and
sometimes misleading.

> **Do not misread `SetBooleans()`.** It ends with a long tail of
> `SomeBox.IsEnabled = false`, including `RandomizeEnemiesCheck`. That is **not**
> "these features are disabled" — `SetBooleans()` runs at the start of a run and
> those lines lock the UI while it works.

## Status legend

| Label | Meaning |
|---|---|
| **DONE** | Implemented, on hardware, confirmed working |
| **READY** | Code already exists in this port; only a UI row and a flag are missing |
| **TODO** | Not implemented |
| **NEW** | Our own idea; no reference equivalent, or a better front-end for one |
| **DEAD** | The reference has a checkbox but it is wired to nothing |
| **OUT** | Deliberately out of scope |

---

## 1. Done (6 reference settings → 8 toggles)

| # | Feature | Reference flag | What it does | Ours |
|---|---|---|---|---|
| 1 | Randomize Enemies | `randomizeEnemiesBool` | Replaces each eligible world enemy placement with a random one from the pool, gated by a per-zone chance and a model-size limit | `RANDOMIZE ENEMIES` |
| 2 | Randomize Bosses | `includeBosses` | Shuffles boss identities between boss arenas, with per-zone scaling applied afterwards | `RANDOMIZE BOSSES` |
| 3 | Non-Key Overworld Items | `randomizeItemLots` | Permutes which item lot sits at which world pickup (chests, corpses, ground items) | `RANDOMIZE TREASURE` |
| 4 | Randomize Enemy Drops | `enemyDropBool` | Reassigns `NpcParam.itemLotId_1` — what each enemy type drops on death | `RANDOMIZE ENEMY DROPS` |
| 5 | Randomize Starting Weapons | `startingWeaponsOnlyBool` + `keepGuns` | Reference: randomizes all 644 shop weapon rows, with a negative toggle to protect guns | Split into three positive toggles: `STARTING WEAPONS`, `STARTING GUNS`, `SHOP WEAPONS` |
| 6 | Seed | `seed` textbox | Fixes the RNG so a run can be reproduced | `SEED` row: roll, edit, persisted as `last_seed` |

---

## 2. Ready — already written, needs a row (0 — both landed)

**This category is now empty.** Both rows landed; they are kept here for their
history, and for the warning below, which applies to every row still on the
list further down.

| # | Feature | Reference flag | What it does | Status |
|---|---|---|---|---|
| 7 | Randomize Workshop Tools | `workshopBool` | Two workshop item lots (`2411000` blood gem tool, and its sibling) are excluded from the treasure pool by default; this opts them in | **DONE** — wired end to end (defaults, store, both screens, options, engine); hardware-tested. See `docs/plans/workshop-tools.md` |
| 8 | Nurse Perma-Darkness → **Enable Mergo Darkness** | `permaDarknessBool` | Leaves one event in `event/common.emevd.dcx` at its shipped values, which makes the whole world permanently dark. The app's poke is what produces the normal lit game | **DONE** — shipped as `ENABLE MERGO DARKNESS`, hardware-tested; see `docs/plans/mergo-darkness.md` |

> **Row 8 is the cautionary tale on this list — read it before doing another.**
> The reference's checkbox name is accurate: checking it gives you
> perma-darkness. We decoded the event, concluded from the bytes that the
> checked branch was a no-op restore, renamed the setting to the opposite, and
> shipped a build whose default was a permanently dark game. Hardware testing
> caught it; nothing in the byte-level analysis could have.
>
> **The lesson is not "check what each branch writes" — we did that, correctly.**
> It is that knowing which bytes change tells you nothing about what the game
> does with them. For the rows still on this list, treat a decode as a way to
> find the switch, and the console as the only way to learn what the switch
> does. See `docs/plans/mergo-darkness.md` §2.7.

---

## 3. Enemy and boss pickers (0 remaining — both done)

**Our design, but not new capability.** The reference already has all four
semantics; what it lacks is a usable front-end. Today you type raw five-character
model IDs, concatenated, into a text box (`oopsAllString` / `oopsBossString`),
and the code chops the string into five-character chunks:

| Reference flag | Semantics | Reads |
|---|---|---|
| `oopsAll` | **Whitelist** enemies — the pool becomes *only* the listed models | `oopsAllString` |
| `excludeEnemiesBool` | **Blacklist** enemies — remove the listed models from the pool | `oopsAllString` |
| `oopsAllBosses` | **Whitelist** bosses | `oopsBossString` |
| `excludeBossesBool` | **Blacklist** bosses | `oopsBossString` |

| # | Feature | What it does | Status |
|---|---|---|---|
| 9 | Enemies to include | A checklist of every pool creature, **all ticked by default**. Untick a few to keep them out of the run; untick everything but one and every enemy becomes that creature | **DONE** — shipped as the `ENEMIES INCLUDED` drill-in, 82 rows; see `docs/plans/pickers.md` |
| 10 | Bosses to include | The same for the boss pool. Leave only Ludwig ticked and every boss arena is Ludwig | **DONE** — shipped as the `BOSSES INCLUDED` drill-in, 17 rows; same component as row 9 |

Why this is worth building rather than porting the text box:

- **We have the names.** `tools/data/Characters.json` (Smithbox, MIT) turns
  `c2630` into "Huntsman (Transformed)". A picker can show creature names; the
  reference's box cannot.
- **A whitelist and a blacklist are the same control.** "All ticked" is the
  no-op, "one ticked" is Oops-All, "most ticked" is an exclusion. One list
  replaces four flags and two text boxes.
- **It supersedes our removed protection list.** `docs/enemy-exclusion-history.md`
  records the 45-entry heuristic we deleted for being a deviation. A user-driven
  picker gives the same protection without guessing on the player's behalf.

**Row 9 is now planned in full — `docs/plans/pickers.md`.** It answers
the three questions that used to sit here, and corrects the premise of the first:

- **"~425 pool entries" was wrong.** Per *model*, which is what `oopsAll` keys on
  anyway, the list is **82 rows** — and the engine's real pool is 333 entries
  across those 82, not the 442/83 `tools/enemy_lookup.py pool` reports. That tool
  is wrong and is fixed as part of the feature.
- **Selections persist** as a fixed-length 0/1 string in `defaults.cfg`, with a
  length mismatch falling back to all-enabled.
- **A too-small whitelist does not break anything** — the size gate degrades to
  placing an oversized enemy rather than hanging, and an *empty* selection is
  blocked at commit, behind two existing engine guards.

Pool weighting turned out to be the interesting discovery: one model is 12.3% of
all draws and 13 models are half of them. Deliberately left alone here and
written up as `docs/deferred-ideas.md §2`.

---

## 4. Items and shop (2 remaining)

| # | Feature | Reference flag | What it does | Status | Cost |
|---|---|---|---|---|---|
| 11 | Randomize Shop Items | `shopBool` | Shuffles shop **armour** (`equipType 1`) and **consumables** (`equipType 3`). Note it does *not* touch weapons — that is setting 5 | **TODO** | **Low.** Same `ShopLineupParam`, same `equipId` field, same loop we already run for weapons — two more type buckets |
| 12 | Key Overworld Items (With Logic) | `keyItemRandomizeBool` | Nothing. The flag is assigned from the checkbox and **never read anywhere**; `keyitemRand` is constructed and never used. Key items are only ever *excluded* from the treasure pool | **DEAD** in the reference | Building it for real is net-new design, not a port — see §9 |

---

## 5. Enemy and boss pool (5 remaining)

| # | Feature | Reference flag | What it does | Status | Cost |
|---|---|---|---|---|---|
| 13 | Bosses Can Replace Enemies | `insertBossesBool` | Lets boss identities into the ordinary enemy pool, so bosses appear as world mobs | **TODO** | Medium. Tangled with `excludeBossesBool` and the oops-all lists; needs a fairness decision |
| 14 | Include Lesser Bosses in Boss Pool | `lesserBossesBool` | Adds sub-boss / mini-boss placements (e.g. `c5070`, `c4030_0000`) to the boss pool | **TODO** | Low. Three call sites in the boss eligibility checks |
| 15 | Randomize NPCs | `includeNPCs` | Treats hostile human NPCs (`c0000` models with a real think ID) as randomizable placements | **TODO** | Medium. Own MSB pass, keyed off `UnkT07` against a hostile-NPC list |
| 16 | Unchanged Bell Maidens | `bellMaidenBool` | Adds `c1050`, `c1051`, `c1055` to the exclusion list so bell maidens stay put | **TODO** | **Trivial.** Three strings appended to the exclusion list |
| 17 | Per-zone boss toggles | `bossHemwick`, `bossCentralYharnam`, … (14 zones) | Chooses which zones' bosses take part, one flag per zone | **TODO** | Low logic, but 14 rows of UI — wants a sub-screen, same as the pickers in §3 |

---

## 6. Difficulty helpers (4 remaining)

All four do the same thing: replace a duplicated boss add with a harmless statue
("stone guy") so multi-phase or multi-body fights become single-target. That is
the **same three-field poke** (`NPCParamID`, `ThinkParamID`, model) the enemy and
boss randomizers already perform, so all four are one small table-driven pass.

| # | Feature | Reference flag | Target | Status |
|---|---|---|---|---|
| 18 | Easy Shadows | `easyMultiBossesBool` | `m27_00_00_01` — Shadows of Yharnam (`c2120_0001/0002`) | **TODO** |
| 19 | Easy Rom | `easyRomBool` | `m32_00_00_00/01` — Rom's children (`c1400`) | **TODO** |
| 20 | Easy Failures | `easyFailuresBool` | `m35_00_00_00` — the Failures | **TODO** |
| 21 | Easy Emissary | `easyWitchesBool` | `m24_02_00_00/01` — Celestial Emissary (`c2500_0001`) | **TODO** |

---

## 7. Scaling (2 remaining)

| # | Feature | Reference flag | What it does | Status |
|---|---|---|---|---|
| 22 | No Scaling | `noScalingBool` | Skips `ParamScalingForBosses` entirely, so a randomized enemy keeps its native stats instead of being retuned for the zone it landed in | **TODO** — we always scale; this is the opt-out |
| 23 | Custom Scaling | `customScaling` | Replaces the fifteen baked per-zone offsets (`dreamScale=20`, `centralScale=1`, …) with values chosen in the UI | **TODO** — offsets already live in `BossParamScaling.h`; needs per-zone numeric entry |

---

## 8. Combat and cosmetic params (8 remaining)

| # | Feature | Reference flag | What it does | Status | Cost |
|---|---|---|---|---|---|
| 24 | Melee Movesets | `meleeMoveset` | Permutes **48 cells** of `EquipParamWeapon` between melee weapons, so a weapon swings like a different one | **TODO** | **Low.** Same param we already parse and edit; `tools/param_offsets.py` resolves all 48 cell indices mechanically |
| 25 | Gun Movesets | `gunMoveset` | The same for firearms | **TODO** | **Low** — shares everything with 24 |
| 26 | Gems + Runes | `gemBool` (+ `threeGemBool`, `sixGemBool`) | Randomizes gem/rune effects; the two sub-flags force 3 or 6 effects per item | **TODO** | Medium — new param (`GemGenParam`) |
| 27 | No Team Type | `teamTypeBool` | Rewrites `NpcParam` team types so everything is hostile to everything — enemies fight each other | **TODO** | Low — `NpcParam` is already parsed for drops |
| 28 | Blood Decals | `bloodBool` | Randomizes blood decal appearance | **TODO** | Low, cosmetic |
| 29 | Face Data | `faceBool` | Randomizes `FaceGenParam` / `FaceParam` | **TODO** | Low, cosmetic |
| 30 | Talk Data | `talkBool` | Randomizes dialogue data | **TODO** | Unknown — least traced of the set |
| 31 | VFX / AI Sound | `vfxBool` | Calls `AiSoundParamRandomizer`; the other VFX calls beside it are commented out in the reference | **TODO** | Low, but the reference's own version is mostly disabled |

---

## 9. Out of scope

### Chalice dungeons — OUT, by earlier decision

Three settings, and they stay out:

| Reference flag | Checkbox |
|---|---|
| `chaliceEnemies` | Include Chalice Enemies |
| `chaliceBosses` | Include Chalice Bosses |
| `bossChalices` | Bosses Can Replace Enemies (Chalice Dungeons) |

**The decision and why it stands:** chalice dungeons were ruled out at the start
of the port and the reasoning has not changed — they are a separate map set
(`m29_*`) that our map tables do not cover, procedurally assembled, and reaching
them requires deep progression, so they are the least-played content per unit of
work. Our UI has never had a chalice setting. Revisit only on an explicit
request; do not quietly pull them in while implementing something adjacent.

### Key items with logic — a correction

I previously told you key-item randomization was the highest-impact remaining
feature. **That was wrong in an important way, and the error was mine:** I ranked
it from the checkbox label without checking whether it was wired up. It is a
**dead checkbox** — `keyItemRandomizeBool` is written and never read.

So it is not a port of anything. Building it means designing reachability logic
from scratch, and getting it wrong hands someone an unwinnable save hours in. It
remains the highest *ceiling* on the list and it is genuinely appealing, but it
belongs at the end, not the front, and it needs its own spec.

---

## 10. Tally

| Group | Remaining |
|---|---|
| Ready (code exists, needs a row) | 0 |
| Enemy / boss pickers (new) | 0 |
| Items and shop | 1 real + 1 dead |
| Enemy and boss pool | 5 |
| Difficulty helpers | 4 |
| Scaling | 2 |
| Combat and cosmetic params | 8 |
| **Total actionable** | **20** |
| Out of scope (chalice) | 3 |

## 11. Suggested order

Grouped so each block reuses one piece of machinery and can share a validator:

1. ~~**The two freebies** (7, 8)~~ — both done.
2. **`EquipParamWeapon` block** (24, 25) — movesets, in a param already proven.
3. **`ShopLineupParam` / `NpcParam` block** (11, 27) — shop armour and
   consumables, plus no-team-type.
4. **MSB pokes block** (16, 18, 19, 20, 21) — bell maidens and all four easy
   modes, all the same three-field write.
5. **Pool scope block** (~~9~~, ~~10~~, 13, 14, 17) — the pickers and the pool-shape
   flags, which all touch the same eligibility code and want the same sub-screen.
6. **Scaling** (22, 23).
7. **Cosmetic params** (26, 28, 29, 30, 31).
8. **Key items with logic** (12) — own spec, last.
