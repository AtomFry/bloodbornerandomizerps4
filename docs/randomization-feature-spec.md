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
| **BUILT** | Implemented and green off-console — clean build plus its verifiers — but **not yet hardware-confirmed**. Not DONE: on this project only the console proves runtime behaviour (`CLAUDE.md` section 3) |
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

## 3. Enemy and boss pickers and placement protection (2 remaining — all three global pickers shipped)

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
| 33 | Protect caged dogs | A **placement** protection, not an enemy one: the ten **Shaggy Hunting Dogs** (`c1240`) wired into the cage scripts in Central Yharnam (six) and the Forbidden Woods (four) keep their vanilla identity, while `c1240`'s other 86 placements stay eligible and the model stays in the pool. Replacements dropped into the Central Yharnam cages misbehave — a Boom Hammer Hunter and a Maneater Boar each caused severe lag, and other replacements take damage but cannot be killed | **DONE** — shipped as `DO NOT RANDOMIZE CAGED DOGS`; **NEW**, no reference equivalent. Hardware-tested 2026-09-19 against a byte-identical seed-1234567890 run. See `docs/features/033-protect-caged-dogs/` |

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

### Row 35 — per-map enemy pools

| # | Feature | What it does | Status | Cost |
|---|---|---|---|---|
| 35 | Per-map enemy pools | A per-map version of row 9's `ENEMIES INCLUDED`. For a chosen map, the pool becomes that map's own list rather than the global one — an **override**, not an intersection. Leaves every other map on the global list | **TODO — NEW**, no reference equivalent | Medium. The picker component exists (rows 9, 10, 32); the map axis, the override/inherit state, its config encoding and the engine lookup do not |

**NEW capability, not a front-end.** The reference has one pool for the whole
run: `oopsAll`/`excludeEnemiesBool` filter `enemyData` once, before any map is
touched. There is no per-map pool anywhere in it, so this row has no reference
behaviour to match and §7 of `CLAUDE.md` does not constrain its shape.

Open questions for its spec, all unanswered:

- **Override or intersect.** The developer's statement is *override* — a map's
  list wins over the global one. That is the simpler rule to explain and the
  easier one to show on screen. Intersection would be the safer default but it
  makes an empty result easy to reach by accident.
- **How a map says "inherit".** A map with no override must be distinguishable
  from a map whose override happens to have everything ticked, or the screen
  cannot show which maps the player has touched. That is a third state, not a
  bit per model.
- **Config cost is the real constraint.** Row 32 took `defaults.cfg` to 555 of
  its 1024 bytes. A naive encoding — 82 characters per map across the map
  table — does not fit, so this needs a sparse encoding storing only overridden
  maps, and possibly a larger buffer. Measure before designing the screen.
- **Which map list.** The map table, the zone list used by the per-zone chance
  gate, and the 14 zones of row 17's boss toggles are three different
  granularities. Picking the wrong one makes the screen either unusably long or
  too coarse to be worth having.
- **Pool starvation, again.** Row 32's resolution (spec 032 D4) was a
  non-failing run that says so on screen. A per-map override can starve one map
  while the rest are fine, which is a narrower case than anything that gate has
  seen.

Its front-end is `ui-backlog.md` row U3, which should share a map-axis component
with row 17 if the two are built near each other.

---

## 4. Items and shop (6 remaining)

| # | Feature | Reference flag | What it does | Status | Cost |
|---|---|---|---|---|---|
| 11 | Randomize Shop Items | `shopBool` | Shuffles shop **armour** (`equipType 1`) and **consumables** (`equipType 3`). Note it does *not* touch weapons — that is setting 5 | **TODO** | **Low.** Same `ShopLineupParam`, same `equipId` field, same loop we already run for weapons — two more type buckets |
| 12 | Key Overworld Items (With Logic) | `keyItemRandomizeBool` | Nothing. The flag is assigned from the checkbox and **never read anywhere**; `keyitemRand` is constructed and never used. Key items are only ever *excluded* from the treasure pool | **DEAD** in the reference | Building it for real is net-new design, not a port — see §9 |
| 40 | Starting gear selection | *none* | **NEW.** The gap rows 37/38 leave: they grant a *weapon*, and row 5 randomizes what the Dream's coffins sell, but nothing lets the player choose **armour, or the starting consumables and bullets**, and nothing lets them fix the Dream's five coffin picks rather than randomizing them. `CharaInitParam` has the slots already — `equip_Helm/Armer/Gaunt/Leg` at offsets 32-44, set on 1557 rows so plainly read, and `item_01..10` which rows 34/37 already write | **TODO** | Medium. The fields are known and the picker machinery exists; the work is scope, not mechanism |
| 38 | Start with a left-hand weapon | *none* | **NEW.** Row 37 for the other hand: `equip_Wep_Left`, s32 at **offset 24** of the same `CharaInitParam` origin rows row 37 writes at offset 16. 16 candidates — 12 firearms, 2 shields, 2 torches — selected by the `leftHandEquipable` bit, the sibling of the bit row 37 already filters on. Same picker, same config shape, same requirement writer. Shipped as **14 rows** - the obtainability rule cuts 18 to 14 - and flat, because firearms have no Uncanny/Lost versions | **DONE** - shipped as `START WITH A LEFT WEAPON`; spec and plan deliberately skipped at the developer's request. Hardware-tested 2026-09-27. See `docs/features/038-start-with-left-hand-weapon/` | **Low.** Row 37's mechanism is hardware-proven; this is its other half |
| 39 | Start with a Caryll rune | *none* | **NEW.** `equip_Accessory01..05`, s32 at offsets 64-80 of the same rows. Precedent: `CharaInitParam` row 9001 sets `equip_Accessory01 = 100`, a real `EquipParamAccessory` row — evidence row 37's field never had. **Blocked on identification, not mechanism** — see below | **TODO — BLOCKED** | Unknown |
| 37 | Start with a trick weapon | *none* | **NEW, no reference equivalent.** Grants a weapon at character creation, so a new game starts in the clinic already holding one. A **pool picker**, not a single choice: none ticked grants nothing, one ticked always grants that weapon, several ticked draws one at random per run — the same none/one/many semantics `ENEMIES INCLUDED` already has | **DONE** - shipped as `START WITH A TRICK WEAPON`, 78 rows. The probe ran 2026-09-27 and **route A won**: `equip_Wep_Right` is read at character creation. Hardware-tested. See `docs/features/037-start-with-trick-weapon/` | Small - the field behaved |
| 34 | Start with hunter tools | *none* | **NEW, no reference equivalent.** Grants both workshop key items — Blood Gem Workshop Tool (goods `4103`) and Rune Workshop Tool (goods `4104`) — at character creation, by writing them into the free `item_*` slots of every player-origin row in `CharaInitParam`. The randomizer hands out gems and runes from the first area but vanilla gates fitting either one behind two mid-early-game chests, so without this they are dead weight | **BUILT** — shipped as `START WITH HUNTER TOOLS`; spec and plan deliberately skipped at the user's request. **NOT yet hardware-tested**, and it rests on an unverified assumption — see `docs/features/034-start-with-hunter-tools/implementation-report.md` | **Low.** One new engine file plus the usual settings chain |

### Row 37 — start with a trick weapon

**The field exists and is free.** Measured against `data/vanilla/dvdroot_ps4`
rather than assumed:

| Offset | Field | Type | Vanilla value on all 22 player-origin rows |
|---|---|---|---|
| 16 | `equip_Wep_Right` | s32 | `-1` |
| 20 | `equip_Subwep_Right` | s32 | `-1` |
| 24 | `equip_Wep_Left` | s32 | `-1` |
| 28 | `equip_Subwep_Left` | s32 | `-1` |

Those are the **same rows** row 34 already writes — `CHARACTER_INIT_PARAM`,
300-byte rows, the origin list in `HunterTools.cpp`. Row 34 writes `item_*` at
offset 124; this writes a weapon id at offset 16. One more 4-byte poke into a
file the pass already has open.

**The requirement nerf already exists.** `StartingWeapons.cpp`'s
`ApplyStatProfile()` rewrites the four u8 requirements — `properStrength`,
`properAgility`, `properMagic`, `properFaith` at offsets 237–240 — across all
eleven upgrade tiers (`kUpgradeStride` 100, `kUpgradeTiers` 10). Without it a
level-4 character cannot wield most of the list. It is reusable; what it needs
is one starting-weapon profile rather than the five per-slot ones it carries.

**The list is 78 rows — this paragraph first said 37, then 80, and was wrong both times.**
The id is `family*1e6 + weapon*1e5 + variant*1e4 + tier*100`, so dividing by
1,000,000 merges distinct weapons: it puts Saw Cleaver with Saw Spear, Hunter Axe
with Burial Blade, Kirkhammer with Ludwig's Holy Blade. The real decomposition of
`EquipParamWeapon`'s 1090 rows is **47 distinct player weapons**, of which **26
are named right-hand trick weapons**. Spec 037 D2 then puts each *version* on its
own picker row so a player can choose Uncanny or Lost deliberately: 26 × 3 =
**78 rows**, beside `ENEMIES INCLUDED`'s proven 82.

The 80 figure came from counting two unnamed rows — `12080000` and `38090000` —
as fourth versions. They are not: spec 037 §4.2 establishes the exclusion by
**obtainability** rather than by name, and of the 84 right-hand tier-0 rows
exactly 78 appear in a shop or an item lot while those six appear in neither.

The Hunter's Dream's own five are `7000000` Saw Cleaver, `5000000` Hunter Axe,
`22000000` Threaded Cane, `14000000` Hunter Pistol, `6000000` Blunderbuss.

**THE PROBE, and why this row is not ready to plan.** Everything above is
byte-level fact. What none of it establishes is the only thing that matters:
**whether the game grants the weapon.** In other FromSoftware titles these
fields grant and equip at character creation; whether Bloodborne's clinic spawn
honours them is unobserved, and this project has a scar exactly here — see
`docs/plans/mergo-darkness.md` §2.7, where the byte analysis was right and the
conclusion about game behaviour was backwards.

One probe settles it: write `7000000` into `equip_Wep_Right` on the 22 origin
rows, start a new game, and see whether the character is holding a Saw Cleaver.
If it works the feature is small. If it does not, the reason is the feature, and
the scope is a different question entirely.

**Four decisions the spec has to make, none of them blocked by the probe:**

Four things this row first got wrong, corrected by spec 037's investigation and
left here so the errors are not repeated:

- **Names are in the tree, not missing.** `data/vanilla/dvdroot_ps4/msg/engus/item.msgbnd.dcx`
  carries every weapon name (BND4 member `武器名.fmg`, wide FMG v2, UTF-16LE), so
  the table is generated and pinned like the enemy tables, not hand-authored.
- **Hand IS derivable.** `rightHandEquipable` / `leftHandEquipable`, packed bits at
  offset 256 of `EQUIP_PARAM_WEAPON_ST`, split all 47 player weapons with no
  ambiguous case: 26 right-hand trick weapons, 16 left-hand (12 firearms, 2
  shields, 2 torches), 5 neither. `weaponCategory` at offset 226 does *not* work —
  value 0 covers both the Threaded Cane and the Hunter Blunderbuss.
- **Lost and Uncanny were the wrong way round here.** The game's own text says
  `7010000` is **Uncanny** and `7020000` is **Lost**, consistently across all 26.
  Under D2 this stops being trivia: it would mislabel 52 of the 78 picker rows.
  Nor can the names be composed from a prefix — the game says "Ludwig's Uncanny
  Holy Blade" and "Logarius' Uncanny Wheel", so eight rows would be wrong.
- **Precedence against row 5 is decided, not open.** Spec 037 D4: the grant wins,
  as an explicit rule rather than inherited from the order of two calls in
  `StepItemData`.

### Rows 38 and 39 — the other hand, and the runes

**Row 38 is row 37's other half** and needs no new investigation: `equip_Wep_Left`
sits eight bytes from the field the 2026-09-27 hardware test proved the game
reads, `gen_weapon_table.py` already filters on `rightHandEquipable` and the
left-hand bit is its sibling in the same byte, and the requirement writer, picker,
config encoding and engine pass all exist.

**Row 39 — where the rune data actually lives.** Established 2026-09-27 after the
first three searches failed:

* **Bloodborne calls runes and blood gems the same thing — 魔石, "magic stone".**
  That is why the reference tool has one checkbox, `Gems + Runes` / `gemBool`, for
  both.
* **The names are in `msg/enggb/menu.msgbnd.dcx`, member `魔石名.fmg`** — 3148
  strings. Not in `item.msgbnd.dcx`, and not in `msg/engus/`, which holds only
  `item.msgbnd.dcx`. `アクセサリ名.fmg` is an empty 128-byte FMG in both locales,
  which is what made the first search look conclusive when it was not.
* **The 6 covenant runes are exactly `12000004..12000009`** — Radiance,
  Corruption, Hunter, Impurity, **Beast's Embrace (12000008)**, **Milkweed
  (12000009)**. That is the wiki's count of 6, confirmed against the game's text.
* **The memory runes are `11000000..11000029`**, 25 ids present with gaps,
  including Clawmark (11000004) and Guidance (11000029). 31 rune names in total;
  the wiki's 73 counts tiers, which repeat names — `Lake` appears five times.

**What is NOT yet established, and what blocks the row:** the link from a rune
name id to whatever `equip_Accessory01` actually takes. `EquipParamAccessory` has
41 rows (ids 100-150, `sortId` 1-41 contiguous, all `accessoryCategory` 0) and
nothing found so far maps those 41 onto the 31 names — not `refId` (23 of 41 are
absent from `SpEffectParam`), not `iconId`, not `gemNameIdOffset` (almost always
0), and not `qwcId` (0 on every row). 41, 31 and 73 are three different counts and
the relationship between them is unresolved.

**The reference tool does not help here.** `GemGenParamRandomizer()`
(`RandomizeFunctions.cs:5253`) shuffles `GemGenParam`'s effect-slot columns —
gem *generation recipes*. It never touches `CharaInitParam`, `EquipParamAccessory`
or a rune's identity, so there is no behaviour to match.

---

## 5. Enemy and boss pool (5 remaining)

| # | Feature | Reference flag | What it does | Status | Cost |
|---|---|---|---|---|---|
| 41 | Cut content pool | *none* | **NEW.** Include cut/unused entities and items in the pools where they are safe. Row 37's derivation already met the shape of this problem: of 84 right-hand tier-0 weapon rows, six are unnamed and appear in no shop and no item lot — cut content, excluded by an obtainability rule rather than a hand list. The same question applies to enemies (`EnemyPoolTable` excludes `c1130`, `c2121`, `c2561` today) and to the four unnamed weapon families. **What "safe" means is the whole feature**: an entity with no `ThinkParamID` does not fight, and one with no name reads as a bug | **TODO** | Medium, and mostly investigation. Every inclusion needs its own evidence |
| 13 | Bosses Can Replace Enemies | `insertBossesBool` | A **second pass** over each map that converts ordinary creature placements to bosses, so bosses appear as world mobs. **Not** the enemy pool: the reference harvests a separate `insertBossesString` list and never gives the enemy pool a boss entry, and spec 013 D-N keeps that structure — this row's original "lets boss identities into the ordinary enemy pool" was wrong, as are `docs/plans/pickers.md` §7.5 and spec 032 §8, which repeat it | **TODO** | Medium. **Eight settings** — see `docs/features/013-bosses-replace-enemies/spec.md`. Grew well past the row's estimate: four new `SettingKind` values, two new pickers, and it forces the settings buffer to 2,048 |
| 14 | Include Lesser Bosses in Boss Pool | `lesserBossesBool` | Adds sub-boss / mini-boss placements (e.g. `c5070`, `c4030_0000`) to the boss pool | **TODO** | Low. Three call sites in the boss eligibility checks |
| 15 | Randomize NPCs | `includeNPCs` | Treats hostile human NPCs (`c0000` models with a real think ID) as randomizable placements | **TODO** | Medium. Own MSB pass, keyed off `UnkT07` against a hostile-NPC list |
| 16 | Unchanged Bell Maidens | `bellMaidenBool` | Added `c1050`, `c1051`, `c1055` to the exclusion list so bell maidens stayed put | **OUT** — shipped and hardware-verified, then **retired by row 32** on 2026-09-19. Its effect survives as two ticks on `ENEMIES SKIPPED`, Yahar'gul exception included. **The saved value was deliberately not migrated** (spec 032 D1): an existing `defaults.cfg` keeps its `unchanged_bell_maidens` line, the key is ignored, and the maidens randomize again until the two rows are ticked | The reference deliberately re-randomizes six Yahar'gul maidens even with the flag on; reproducing that override was the bulk of the work, and it survives row 32 unchanged |
| 17 | Per-zone boss toggles | `bossHemwick`, `bossCentralYharnam`, … (14 zones) | Chooses which **areas row 13's insertion pass visits**, one flag per area. This row's original "chooses which zones' bosses take part" was wrong: the fourteen flags gate `InsertBossesVoid` and **nothing else in the reference tool** — they have no effect on boss randomization | **TODO** | **Not separately implementable — delivered by row 13.** Ships as `AREAS INCLUDED`, setting 5 of `docs/features/013-bosses-replace-enemies/spec.md` (D-A). Do not spec separately |

---

## 6. Difficulty helpers (4 remaining)

All four do the same thing: replace a duplicated boss add with a harmless statue
("stone guy") so multi-phase or multi-body fights become single-target. That is
the **same three-field poke** (`NPCParamID`, `ThinkParamID`, model) the enemy and
boss randomizers already perform, so all four are one small table-driven pass.

| # | Feature | Reference flag | Target | Status |
|---|---|---|---|---|
| 18 | Easy Shadows | `easyMultiBossesBool` | `m27_00_00_01` — Shadows of Yharnam (`c2120_0001/0002`) | **BUILT** — shipped as `EASY SHADOWS`, verified by `easy_modes_verify.py`; **not yet hardware-tested**. See `docs/features/018-easy-shadows/` |
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

## 8. Combat and cosmetic params (9 remaining)

| # | Feature | Reference flag | What it does | Status | Cost |
|---|---|---|---|---|---|
| 24 | Melee Movesets | `meleeMoveset` | Permutes **48 cells** of `EquipParamWeapon` between melee weapons, so a weapon swings like a different one | **TODO** | **Low.** Same param we already parse and edit; `tools/param_offsets.py` resolves all 48 cell indices mechanically |
| 25 | Gun Movesets | `gunMoveset` | The same for firearms | **TODO** | **Low** — shares everything with 24 |
| 26 | Gems + Runes | `gemBool` (+ `threeGemBool`, `sixGemBool`) | Randomizes gem/rune effects; the two sub-flags force 3 or 6 effects per item | **TODO** | Medium — new param (`GemGenParam`) |
| 27 | No Team Type | `teamTypeBool` | Writes one shared team id (25) into `NpcParam.teamType` on every row, so nothing is on a different team from anything else. **The original description here — "everything is hostile to everything" — was wrong**, and so was the shipped label: the reference's own checkbox text is `Content="No Team Type"`, and `AllDmgAll` is only a stale variable name. Vanilla already has 81.6% of placed creatures on one team, so uniform-to-uniform changes nothing; what the pass actually does is **suppress the infighting that shuffling creates** by mixing six teams into one area. Ships as `ENEMIES ON SAME TEAM` | **BUILT** | Low — `NpcParam` is already parsed for drops. Both milestones built 2026-09-28; clean build, `team_type_verify` 20/20, `drops_verify` 12/12 (its D-I2 hole corrected as part of this), `pool_verify` 96/96, `settings_ui_verify` 113/113. **Awaiting hardware** — and "nothing visibly changed" is a valid result: 378 rows already ship at 25 |
| 28 | Blood Decals | `bloodBool` | Randomizes blood decal appearance | **TODO** | Low, cosmetic |
| 29 | Face Data | `faceBool` | Randomizes `FaceGenParam` / `FaceParam` | **TODO** | Low, cosmetic |
| 30 | Talk Data | `talkBool` | Randomizes dialogue data | **TODO** | Unknown — least traced of the set |
| 42 | Contextual boss scream | *none* | **NEW.** When `RANDOMIZE BOSSES` puts a different boss in an arena, the arena's scripted scream and intro still belong to the vanilla occupant — the Cleric Beast's roar plays for whatever is actually there. Replace it with the incoming boss's own. Lives in `event/*.emevd`, which **this port has never edited** — `ENABLE MERGO DARKNESS` reads one event and pokes it, and that is the extent of our emevd work | **TODO** | High, and the emevd dependency is the reason. Investigate before costing |
| 31 | VFX / AI Sound | `vfxBool` | Calls `AiSoundParamRandomizer`; the other VFX calls beside it are commented out in the reference | **TODO** | Low, but the reference's own version is mostly disabled |

---

## 8A. Modes — our own ideas (1)

Numbered 8A rather than 9 on purpose: §9, §10 and §11 are referred to by number
from other documents, so this section is inserted without renumbering them.

Everything above is a **setting** — a flag that changes how randomization
behaves. This section is for items that change what a run *is*.

| # | Feature | What it does | Status | Cost |
|---|---|---|---|---|
| 36 | Boss rush mode | Clears a map's ordinary enemy placements entirely and places **bosses** at chosen points along it, spaced far enough apart that only one is ever engaged at a time | **IDEA — NEW**, no reference equivalent | **High**, and the highest-uncertainty item on this list |

**Why this is the largest item here despite sounding small.** Every shipped
feature substitutes one identity for another at a placement that already exists
and already works — the game's own scripts, collision, navmesh and triggers stay
intact. This one does the opposite: it removes placements and invents new ones.
That crosses into territory nothing in this port has touched:

- **Deleting placements is not a poke.** The existing engine overwrites three
  fields of an existing MSB part. Removing parts means editing the MSB's part
  lists and every index that refers into them, and `Msb` was written for
  in-place edits.
- **Bosses are not just enemies.** A boss placement in this game comes with arena
  scripting, fog gates, an emevd trigger, a health bar and usually a region — the
  mechanism `docs/features/018-easy-shadows/` had to understand to do the far
  smaller job of statue-ing a duplicate add. Dropping `c5070` at a spot in Central
  Yharnam does not give you a boss fight; the open question is what it *does*
  give you, and that is a hardware question by construction.
- **"Spaced enough" needs coordinates we do not have a source for.** Choosing
  placement points means either authoring them by hand per map, or deriving them
  from the vanilla placements' own transforms. Which is available, and whether
  the results are traversable ground rather than mid-air or inside geometry, is
  unknown.
- **The scale of the deletion decides whether the map still loads.** Scripts that
  wait on an enemy that no longer exists, and doors gated on clearing a group,
  are the obvious failure class.

**Suggested first step is an investigation, not a spec.** The cheapest way to
learn whether this is a small feature or a project is one hardware probe: a
single map, enemies left alone, one extra boss placed at a known-traversable
point. If that fights, the rest is scoping. If it does not, the reason it does
not is the actual feature. Because the uncertainty is this concentrated, this row
should stay **IDEA** until that probe has run — it is not yet a thing anyone can
write a spec against.

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
| Enemy / boss pickers and placement protection (new) | 2 |
| Enemy and boss pool - cut content (new) | 1 |
| Items and shop | 3 real + 1 dead |
| Enemy and boss pool | 4 |
| Difficulty helpers | 4 |
| Scaling | 2 |
| Combat and cosmetic params | 9 |
| Modes (new) | 1 idea |
| **Total actionable** | **25 + 1 idea** |
| Out of scope (chalice) | 3 |

Of those, **19 are unimplemented reference settings** — §4 through §8, every
checkbox the Windows tool has that this port does not. Finishing that set is a
standing goal in its own right and needs no separate row: the rows below *are*
the list, and the order in §11 is how to work through them. Rows 35 and 36 are
ours and sit outside it.

UI and platform work is tracked separately in `ui-backlog.md`.

### The 2026-09-28 feature list, and where each item went

The developer proposed sixteen items across five headings. **Four were already
on this list** and were not duplicated — that is the main thing this note
exists to record:

| Proposed | Already tracked as |
| --- | --- |
| Shop item randomization | **row 11**, `shopBool` — armour and consumables; note it does *not* cover weapons, which is row 5 |
| Rune/gem effect randomization, 3 or 6 effects | **row 26**, `gemBool` + `threeGemBool` / `sixGemBool` — the sub-flags are exactly the 3/6 request |
| Talk data randomizer | **row 30**, `talkBool` |
| Sound effect randomizer | **row 31**, `vfxBool` — `AiSoundParamRandomizer`. Partial: the reference's other VFX calls are commented out in its own source |

**Three became new rows here:** 40 starting gear selection, 41 cut content pool,
42 contextual boss scream.

**Seven went to `ui-backlog.md`** as U6-U12, because they change the app or the
save rather than what a run randomizes: NG+ level, the save-data review, the
save editor, the on-screen keyboard, title id lookup, the activation review, and
optional mod support.

**One was not filed at all.** "Review the remaining unimplemented randomization
options and validate each against the current architecture" *is this document* —
§4 through §8 are that list, §11 is the order, and every row already carries its
cost against the port's architecture. Filing it as a row would have been filing
the backlog inside itself.

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

Two later additions, both ours rather than the reference's:

- **Row 35 (per-map enemy pools)** belongs in block 5 — it is the same
  eligibility code and the same picker component as the rest of that block, and
  it should share a map-axis sub-screen with row 17.
- **Row 36 (boss rush)** is sequenced by nothing on this list. It needs a
  hardware probe before it can be specified, and that probe can happen at any
  time because it shares no machinery with anything here.
- **Row 37 (start with a trick weapon)** belongs beside row 5, whose
  `ApplyStatProfile` it reuses and whose stat writes it has to agree with. Like
  row 36 it wants a hardware probe first, but unlike row 36 the probe is one
  four-byte write and the rest of the feature is already built machinery.
