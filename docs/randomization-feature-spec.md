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

## 3. Enemy and boss pickers and placement protection (1 remaining — all three pickers shipped)

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
| 32 | Bypassed enemies | A checklist of every creature that has a randomizable placement, **none ticked by default**. Tick one and it leaves the randomizer entirely: its own placements keep their vanilla identity, and it is never used as a replacement anywhere | **DONE** — shipped as the `ENEMIES SKIPPED` drill-in, 85 rows, same component as rows 9 and 10; hardware-tested 2026-09-19. **Retired row 16.** Two documented exceptions, both deliberate: the six Yahar'gul maidens still change, and with `RANDOMIZE BOSSES` on the boss pool is not filtered. See `docs/features/032-bypassed-enemies/` |
| 33 | Protect caged dogs | A **placement** protection, not an enemy one: the ten **Shaggy Hunting Dogs** (`c1240`) wired into the cage scripts in Central Yharnam (six) and the Forbidden Woods (four) keep their vanilla identity, while `c1240`'s other 86 placements stay eligible and the model stays in the pool. Replacements dropped into the Central Yharnam cages misbehave — a Boom Hammer Hunter and a Maneater Boar each caused severe lag, and other replacements take damage but cannot be killed | **TODO** — **NEW**, no reference equivalent. Shipping as `DO NOT RANDOMIZE CAGED DOGS`; spec **APPROVED** 2026-09-19, see `docs/features/033-protect-caged-dogs/` |

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

### Row 32 — bypassed enemies, and what it replaces

**This one is new capability, not just a front-end** — the claim at the top of
this section does not extend to it. `excludeEnemiesBool`
(`StartFunctions.cs:536-563`) removes the listed models from `enemyData`, the
**pool**, and nothing else; those models' own placements are still overwritten
like anything else. The only placement protection the reference has is its
hard-coded `unusedPlusBossList` and the three strings `bellMaidenBool` appends to
it. Row 32 makes that list editable.

**One list, both halves — deliberately.** The exclusion list feeds pool
contribution (`EnemyRandomizer.cpp:375`) and placement overwriting (`:629`), so a
ticked creature is out of the randomizer entirely. That coupling is exactly what
makes row 32 reproduce `bellMaidenBool`: tick `CHIME MAIDEN` and `CHIME MAIDEN
(LIGHT)` and you have row 16, all 54 of its placements (`c1050` 31, `c1051` 23).
Decoupling the two halves was considered and rejected — with the placement half
alone, freezing the maidens would still scatter ~98 new ones across the game per
run, which is not what anyone means by "unchanged".

**85 rows, not 82 — `EnemyPoolTable.h` must not be reused.** Measured against
`data/vanilla/dvdroot_ps4`: 85 distinct models sit on randomizable placements,
covering 2,269 of them, but only 82 can be drawn as replacements. The three that
can be overwritten yet never placed are `c1130` (2 placements, `ThinkParamID` 0),
`c2121` (8, `ThinkParamID` 1) and `c2561` (9, skipped outright at `:380`).
Reusing the 82-row table would silently make those three unfreezable. This wants
its own generated table, its own frozen order, and its own `pool_verify.py table`
check.

The two tables also report different numbers for the same creature: row 9's
column is **draw weight**, row 32's is **placements protected**. Huntsman
(Transformed) is 41 pool entries and 283 placements.

**The default inverts, and so must the fail-safe.** All ticked is the no-op for
row 9; **none** ticked is the no-op here. `ModelPoolSelection` constructs to
`EnableAll()` and leaves a wrong-length `Decode()` sitting there — right for a
pool picker, exactly wrong for this one, where a stale config would freeze the
whole game and produce a run that changes nothing. The safe default belongs in
the type rather than baked into it.

**It can starve the pool, and the fix shipped with it — but not as the
commit-time check this section first proposed.** Row 16 already reached that
state: with the flag on and only the two `CHIME MAIDEN` rows ticked in
`ENEMIES INCLUDED`, nothing survived both filters and the run failed after the
mirror phase, leaving a half-built tree. Row 32 widens that from one
combination to many.

The developer chose a **non-failing** resolution instead (spec 032 D4): a
starved run completes, drawing from `ENEMIES INCLUDED` for that run while the
skipped placements stay frozen, and says so on screen. `NoneEnabled()`'s two
existing refusals are untouched (D5). Separating that from the *other* cause of
an empty pool — an unreadable vanilla source, which must still fail — was the
bulk of the work and shipped as its own milestone.

**Two things that do not generalise, and must be preserved rather than tidied
away.** `c1055` is one of `bellMaidenBool`'s three strings, matches zero
placements in any map, and has no model to hang a row on — inert, but it is
transcription fidelity and a selftest pins it. And the six forced `c1050_011x`
placements in Yahar'gul bypass both gates unconditionally
(`RandomizeFunctions.cs:288-320`), so ticking `CHIME MAIDEN` will still leave six
maidens randomized in m28.

**Config cost, as shipped:** one more positional string, 85 characters, and
row 16's line retired with its UI row. `defaults.cfg` goes from 478 to **555**
bytes of its 1024-byte buffer — the estimate of 556 above was one byte out, and
`pool_verify.py selftest` now pins the real figure. The file is `key=value` with
unknown keys ignored, so dropping a line costs nothing: only the selection
*values* are positional.

**Answered — row 16's wizard row went.** Recorded as spec 032 D1 and shipped on
2026-09-19. Reference parity survives as a configuration rather than as a
checkbox: ticking the two chime maiden rows reproduces the setting, Yahar'gul
override included. The saved value was deliberately **not** migrated, so anyone
who had the flag on loses it silently once and has to re-tick — accepted as the
cheaper of the two options, and called out in `docs/user-guide.md`.

### Row 33 — the caged dogs, and why row 32 does not cover them

**The creature is the Shaggy Hunting Dog, `c1240` — not `HUNTING DOG`.** This row
was first written saying "Hunting Dog", which is wrong in a way that matters:
`HUNTING DOG` is `c1140`, 14 placements, all in Hemwick Charnel Lane and none in
either cage area, so ticking
that row in `ENEMIES SKIPPED` would do nothing at all to the caged dogs. The row that
would is `SHAGGY HUNTING DOG`, 96 placements. Spec 033 §4 F1 established this.

**Two areas, not one.** The row was first written for the Central Yharnam kennel yard
alone, because that is where the breakage was observed. A game-wide sweep for caged
dogs found one other cluster — six cages in the Forbidden Woods, four of them holding a
dog — with structurally identical map and script data, and nothing anywhere else. The
approved scope covers both: **ten** protected placements, **twenty-six** rows across
the five map files that hold them. Whether the Forbidden Woods cages actually misbehave
when randomized is unobserved and is a hardware question by construction; they are in
scope as a precaution, at a cost of four identifiers and one extra hardware walk.

**Row 32 is keyed on the model; this one is keyed on the placement.** Ticking
`SHAGGY HUNTING DOG` in `ENEMIES SKIPPED` freezes all 96 of its placements and pulls
the model out of the pool entirely. The problem being solved is far narrower: **ten**
placements are wired into the cage scripts, and only those are broken by substitution.
The other 86 should keep randomizing and the model should stay in the pool.

The protected set comes from **verified placement identifiers in the map data**, and
spec 033 found that only one identifier gets it exactly: a list of ten entity IDs.
Placement *names* are not usable — `c1240_0004` and its siblings are reused in
Cathedral Ward, Yahar'gul and the unprotected halves of both cage areas, so the
substring match the existing skip gate uses would catch 58 placements across nine map
files. In each area the dogs that break out of their cages are driven by a different
event from those that stay penned, but every one of them is addressed by entity ID, so
one list covers all ten.

The two settings compose rather than compete: the global exclusion still wins where
it applies, and this one only adds protection where it does not.

---

## 4. Items and shop (2 remaining)

| # | Feature | Reference flag | What it does | Status | Cost |
|---|---|---|---|---|---|
| 11 | Randomize Shop Items | `shopBool` | Shuffles shop **armour** (`equipType 1`) and **consumables** (`equipType 3`). Note it does *not* touch weapons — that is setting 5 | **TODO** | **Low.** Same `ShopLineupParam`, same `equipId` field, same loop we already run for weapons — two more type buckets |
| 12 | Key Overworld Items (With Logic) | `keyItemRandomizeBool` | Nothing. The flag is assigned from the checkbox and **never read anywhere**; `keyitemRand` is constructed and never used. Key items are only ever *excluded* from the treasure pool | **DEAD** in the reference | Building it for real is net-new design, not a port — see §9 |

---

## 5. Enemy and boss pool (4 remaining)

| # | Feature | Reference flag | What it does | Status | Cost |
|---|---|---|---|---|---|
| 13 | Bosses Can Replace Enemies | `insertBossesBool` | Lets boss identities into the ordinary enemy pool, so bosses appear as world mobs | **TODO** | Medium. Tangled with `excludeBossesBool` and the oops-all lists; needs a fairness decision |
| 14 | Include Lesser Bosses in Boss Pool | `lesserBossesBool` | Adds sub-boss / mini-boss placements (e.g. `c5070`, `c4030_0000`) to the boss pool | **TODO** | Low. Three call sites in the boss eligibility checks |
| 15 | Randomize NPCs | `includeNPCs` | Treats hostile human NPCs (`c0000` models with a real think ID) as randomizable placements | **TODO** | Medium. Own MSB pass, keyed off `UnkT07` against a hostile-NPC list |
| 16 | Unchanged Bell Maidens | `bellMaidenBool` | Added `c1050`, `c1051`, `c1055` to the exclusion list so bell maidens stayed put | **OUT** — shipped and hardware-verified, then **retired by row 32** on 2026-09-19. Its effect survives as two ticks on `ENEMIES SKIPPED`, Yahar'gul exception included. **The saved value was deliberately not migrated** (spec 032 D1): an existing `defaults.cfg` keeps its `unchanged_bell_maidens` line, the key is ignored, and the maidens randomize again until the two rows are ticked | The reference deliberately re-randomizes six Yahar'gul maidens even with the flag on; reproducing that override was the bulk of the work, and it survives row 32 unchanged |
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
| Enemy / boss pickers and placement protection (new) | 1 |
| Items and shop | 1 real + 1 dead |
| Enemy and boss pool | 4 |
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
4. **MSB pokes block** (~~16~~, 18, 19, 20, 21) — bell maidens (shipped, then
   retired into row 32) and all four easy modes, all the same three-field write.
5. **Pool scope block** (~~9~~, ~~10~~, ~~32~~, 33, 13, 14, 17) — the pickers and
   the pool-shape flags, which all touch the same eligibility code and want the
   same sub-screen. All three pickers are done; 33 reuses row 32's placement gate
   from the other end, and 13, 14 and 17 remain.
6. **Scaling** (22, 23).
7. **Cosmetic params** (26, 28, 29, 30, 31).
8. **Key items with logic** (12) — own spec, last.
