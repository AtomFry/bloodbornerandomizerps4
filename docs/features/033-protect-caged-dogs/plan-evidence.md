# Plan Evidence 033 — Protect Caged Dogs

**Plan:** `docs/features/033-protect-caged-dogs/plan.md`

**Spec:** `docs/features/033-protect-caged-dogs/spec.md` — APPROVED 2026-09-19

---

> **This document is the investigation behind the plan, not instructions.** The
> implementer consults it when an implementation question needs context the
> contract left out. It is revisable; `log.md` holds the chronology.

Everything numeric below was measured against `data/vanilla/dvdroot_ps4` on
2026-09-19 using the tools in `app/tools/`, or read out of the working tree.
Nothing was carried over from the spec without re-measuring it; §E6 says where
the spec's own figures were confirmed and where they were not.

Most snippets share this preamble, referred to below as **P**:

```python
import sys, math, struct, collections
sys.path.insert(0, 'app/tools')
from enemy_lookup import BASE_MAPS, load_base_maps, load_map, enemies, \
     engine_pool, overwritable_placements, exclusion_reason, zone_scaled_npc
from boss_verify import read_dcx, _i32, _i64, _utf16z
root = 'data/vanilla/dvdroot_ps4'
maps = load_base_maps(root)
PROT = {2410271, 2410272, 2410275, 2410277, 2410278, 2410279,
        2700301, 2700302, 2700308, 2700309}
def objects(m):
    for blob in m.parts.entries:
        if int.from_bytes(blob[0x14:0x18], 'little') != 1: continue
        yield (m.model_name(_i32(blob, 0x1C)) or '',
               _utf16z(blob, _i64(blob, 0x08)),
               _i32(blob, _i64(blob, 0xB0)),
               struct.unpack_from('<fff', blob, 0x28))
```

---

## E1. Reference trace

**The Windows tool has no placement protection a user can turn on, and the one
piece of placement-level hand-tuning it does have is the opposite operation.**
This feature is a new capability, so there is no reference behaviour to match
(`CLAUDE.md` §7 defaults to the reference only where the reference has an
opinion). Three sites were read in full.

**`RandomizeFunctions.cs:288-320` — the only placement-specific code in the
tool.** Inside `if (currentMap.Contains("m28"))`, six `Name.Contains("c1050_01xx")`
tests each set `changeData = true`, forcing those Yahar'gul chime maidens
*into* randomization past every gate. It confirms three things the plan relies
on: the author did hand-pick individual placements when a map needed it; the
mechanism was a name substring scoped by a map test; and the port already
reproduces it (`EnemyExclusionList.h`'s `M28ForcedMaidenList()` plus the
`isM28` test in `StepWriteMap`). It is the shape the new gate is modelled on
— a map-scoped placement test — with the polarity reversed and the matching
key changed from name to entity ID (§E3).

**`RandomizeFunctions.cs:499-510` — the mutation.** The reference writes
exactly `NPCParamID`, `ThinkParamID` and `ModelName`. `EntityID` is read two
lines earlier into a local used only by the optional text log
(`:505`, `:511-521`). So the reference reads the field this plan gates on, and
has never used it as a gate. Spec §4 F8's claim that randomization keeps the
entity ID, and that the cage script therefore keeps addressing whatever
creature was written in, is confirmed in the reference as well as the port.

**`StartFunctions.cs:536-563` — `excludeEnemiesBool` / `oopsAllList`.** It
walks `enemyData` — the candidate pool — backwards and removes any entry
containing a user-supplied five-character model id. It never touches a
placement. This is the reference's nearest relative to this feature and it is
the wrong half: excluding a model there leaves that model's own spots free to
be overwritten by something else.

**`unusedPlusBossList`** (`MainWindow.xaml.cs:94-198`, transcribed verbatim
into `EnemyExclusionList.h`) does protect placements, but it is fixed, not
user-facing, and none of its 103 patterns matches `c1240`. Confirmed by
`exclusion_reason` returning `None` for all 26 protected placements (§E4 M3).

**Pass ordering.** The reference's own order — pool build, then the per-map
mutation loop, then `ParamScalingForBosses` — is the order the port runs
(`StepReadMap` → `StepBuildPool` → `StepWriteMap` → `ApplyBossParamScaling`
inside the write step). The new gate sits inside the mutation loop, so it
changes no pass ordering at all. This is why spec D8 (stat scaling still
applies) is the automatic outcome rather than an extra decision.

---

## E2. What exists in the port

### E2.1 The placement decision, as it stands

`EnemyRandomizer.cpp`'s `StepWriteMap`, inside `if (options.randomizeEnemies)`,
per enemy part:

1. `forced = isM28 && IsM28ForcedMaidenName(name)`
2. `if (!forced && IsExcludedEnemyName(name)) continue;` — the reference's
   fixed 103-pattern list
3. `if (!forced && IsSkippedName(name, skipPatterns)) continue;` — `ENEMIES
   SKIPPED`, feature 032
4. `int roll = RandInt(0, 100); if (!forced && roll >= lm.zoneChance) continue;`
5. size-gate reroll loops, the `m24_02`/`m35` banned-model guards, then the
   three-field write.

The comment above step 4 is load-bearing and was read before choosing the gate
position: the roll is drawn for *every* non-excluded placement, forced ones
included, because skipping the draw would remove `RandInt` calls from the
stream and reshuffle every later map. Tests 2 and 3 sit above the draw, so a
placement they catch consumes no randomness — which is exactly the semantics
spec §2 gives this setting ("taking these spots out of the run removes rolls
from the sequence"). Placing the new test beside them is therefore the only
position that matches the approved behaviour; placing it *after* the roll would
preserve the RNG stream but contradict spec §2, and placing it before test 3
would let it pre-empt `ENEMIES SKIPPED` in a way the §2 truth table does not
describe (the outcome is identical, but the ordering claim in B6 would stop
being literally true).

### E2.2 The pool contribution loop

`StepReadMap` builds two pools in one pass (`pool` and `poolIgnoringSkips`),
deduping by NPC id per map. Spec D6 keeps protected placements contributing, so
this function is not touched at all — which also means the feature cannot
change `result.poolSize`, the fallback logic, or the banned-model flags.

### E2.3 The boolean settings chain

`enableMergoDarkness` is the closest neighbour: a plain bool, default false, no
drill-in. Its sites, all of which the new setting copies:

| Layer | File | Site |
| ----- | ---- | ---- |
| Randomizer | `RandomizerDefaults.h` | the field and its default |
| Randomizer | `RandomizerDefaultsStore.cpp` | key in `LoadRandomizerDefaults`, key in `SaveRandomizerDefaults`'s format string and argument list |
| Randomizer | `EnemyRandomizer.h` | `EnemyRandomizerOptions` field |
| UI | `SetupDefaultsScreen.h` | row constant, `kItemCount` |
| UI | `SetupDefaultsScreen.cpp` | `ToggleRow` branch, `DrawList` entry |
| UI | `EnableWizardScreen.h` | per-run member |
| UI | `EnableWizardScreen.cpp` | row constant, ctor initialiser, left/right branch, X branch, `DrawSaveData` entry, `DrawConfirm` entry, `StartCommit` assignment |

`SetupDefaultsScreen` persists with `defaults_ = working_;`, so no per-field
save code exists or is needed.

`randomizeWorkshopTools` is the closer neighbour in one respect: it is a
*modifier* on another setting, and is deliberately absent from `StartCommit`'s
`||` that decides whether a run has anything to do, and gets no `SKIPPING …`
progress line. The new setting is a modifier on `RANDOMIZE ENEMIES` and
follows it.

### E2.4 Verification tools

* `enemy_lookup.py` — `load_base_maps`, `enemies` (name, entity, npc, think,
  talk, model, pos), `engine_pool` (the trusted pool mirror),
  `overwritable_placements`, `zone_scaled_npc`. Reused as-is.
* `pool_verify.py` — `_frozen` is the corrected "untouched by the enemy pass"
  test (same model, same think, npc either unchanged or the zone-scaled
  variant). Its selftest currently reports **85/85 passing** in 23 s.
* `boss_verify.py` — `read_dcx` and the little-endian readers, used for both
  MSB and EMEVD.
* `ui_scroll_verify.py` — per-screen geometry; its row counts are hard-coded.
* `names.py` — `map_name`, `map_is_unused`. Used for reporting only; spec F13
  is right that `map_is_unused` is inconsistent between the two areas'
  pre-DLC files and must not decide which map files are covered.

**Where the new tool goes.** `pool_verify.py` is 737 lines and owns the
pickers. The recomputation here needs object parts, 3-D distances and an EMEVD
reader, none of which belongs to the pickers, so `caged_dogs_verify.py` is a
new file — consistent with `treasure_verify.py`, `drops_verify.py`,
`mergo_darkness_verify.py` each owning one feature.

---

## E3. Alternatives considered

| Approach | Why rejected |
| -------- | ------------ |
| Placement-name substring, the way `IsExcludedEnemyName` and `IsSkippedName` match | Catches **58 placements across nine map files** (M6). The eight names are reused by unrelated dogs in Cathedral Ward, Yahar'gul and the unprotected halves of both areas. Would freeze 32 placements it must not touch |
| Name substring **plus** a map-name test | Correct for the Forbidden Woods, wrong for Central Yharnam: `c1240_0004` and `c1240_0005` name both a protected breakout dog in `m24_01` and an unprotected dog in `m27`, and within `m24_01` the six names are unique but nothing guarantees that for a future map. Two keys where one suffices |
| Stat row (`124401` + `124501`) | Exact for the Forbidden Woods four and for the Central Yharnam *penned* four, but misses the two breakout dogs, which share row `124400` with six unprotected Central Yharnam dogs (M5). Twenty of twenty-six |
| Collision surface | Exact for Central Yharnam's seven-dog set, which spec D5 explicitly rejected in favour of six, and does not discriminate at all in the Forbidden Woods, where the cage surface carries eleven enemies of four creatures |
| Position clustering with hand-chosen radii | Works (M4 shows a wide margin) but nothing pins the radii in the shipped code, and a float comparison in the engine's hot loop buys nothing over ten integer comparisons |
| Entity ID alone, no map test | Already exact today — the ten IDs match exactly 26 enemy placements game-wide (M2). Rejected anyway: the map prefix costs one `find` on a string the loop already holds and makes "26 and no more" structural rather than measured. It also mirrors the reference's own map-scoped override (§E1) |
| Cage-object entity ID, resolved through the script | The four Central Yharnam penned dogs' cage props carry entity ID `-1` (M10), so six of the ten cannot be reached this way |
| A new list in `EnemyExclusionList.h` or `EnemySkipList.h` | Both match names game-wide and both have a documented reason for existing. Feature 016 and 032 each kept a new gate in its own header for the same reason: provenance and lifetime differ |
| Removing protected placements from pool contribution too | Spec D6 forbids it. Measured cost if it were allowed: the pool would drop from 333 to 331, losing stat variants `124401` and `124501`, which no other placement supplies (M11) |
| Gate after the zone roll, preserving the RNG stream | Contradicts spec §2's explicit statement that turning the setting on changes the seed's meaning, and would make the setting's effect depend on the zone chance |
| Putting the recomputation in `pool_verify.py` | See §E2.4 |
| Inserting the new settings row at index 4, below `RANDOMIZE ENEMIES` | This was the planner's recommendation, on the grounds that the list is read top to bottom and the setting is a modifier on the row above it. **Rejected by the developer on 2026-09-19** (plan §9 P5, P13) in favour of appending last: insertion renumbers eleven constants and three parallel vectors, and the developer judged that hazard (§E5.1) not worth the better list order |

---

## E4. Measurements

| # | Quantity | Value |
| - | -------- | ----- |
| M1 | Shaggy Hunting Dog `c1240` placements in the 24 base maps | **96**, across 9 map files: `m28_*` 16+16, `m24_01_*` 12+12+12, `m27_*` 8+8, `m24_00_*` 6+6 |
| M2 | Placements matched by the ten entity IDs | **26** — 6 in each `m24_01` file, 4 in each `m27` file, none anywhere else |
| M3 | Of those 26, how many the fixed exclusion list already catches | **0** |
| M4 | Cage-proximity derivation | **26**, max protected distance **0.899**, nearest unprotected placement to a cage **2.671** |
| M5 | Stat rows | `124400` 24 placements game-wide, `124401` 12, `124500` 8, `124501` 8 |
| M6 | Name-substring hits for the eight placement names | **58** across **9** map files |
| M7 | Entity ID `-1` among Central Yharnam dogs | 2 of 12 (`c1240_0001`, `c1240_0002`) |
| M8 | Enemy-part entity-ID uniqueness | unique within every base map except `m33_00_00_00` (Nightmare Frontier), which has **two** duplicate pairs |
| M9 | Worst-case `defaults.cfg` after the new key | **585** bytes into `char buf[1024]` |
| M10 | Cage props | Central Yharnam `o243010` ×2 (entity 2411270, 2411271) + `o243011` ×11 (nine of them entity `-1`); Forbidden Woods `o273011` ×6 |
| M11 | Pool with the plan as written | **333** entries across **82** models — unchanged |
| M12 | Protected placements in map files the retail game actually loads | **16** (`m24_01_00_00` 6, `m24_01_00_01` 6, `m27_00_00_01` 4) |
| M13 | Protected placements re-targeted in the pre-feature run | **26 of 26** |
| M14 | Settings row width | new row 33 characters; 53 fit at scale 4 |

### M1–M3, M5, M6, M11 — placements, rows, names, pool

```python
# preamble P, then:
print(len([e for mn in BASE_MAPS for e in enemies(maps[mn]) if e['model']=='c1240']))       # M1 -> 96
hits=[(mn,e) for mn in BASE_MAPS for e in enemies(maps[mn]) if e['entity'] in PROT]
print(len(hits), collections.Counter(mn for mn,_ in hits))                                   # M2 -> 26
print([e['name'] for _,e in hits if exclusion_reason(e['name'])])                            # M3 -> []
for row in (124400,124401,124500,124501):
    print(row, sum(1 for mn in BASE_MAPS for e in enemies(maps[mn]) if e['npc']==row))       # M5
NAMES={'c1240_0001','c1240_0002','c1240_0004','c1240_0005','c1240_0008',
       'c1240_0010','c1240_0011','c1240_0012'}
sub=[(mn,e['name']) for mn in BASE_MAPS for e in enemies(maps[mn])
     if any(p in e['name'] for p in NAMES)]
print(len(sub), len({mn for mn,_ in sub}))                                                   # M6 -> 58 9
pool=engine_pool(root, maps=maps); print(len(pool), len({m for _,_,m in pool}))              # M11 -> 333 82
```

The 26 in full, with the fields the gate and the verifier use:

| Map | Placement | Entity | Model | Stat row | Behaviour row |
| --- | --------- | -----: | ----- | -------: | ------------: |
| `m24_01_00_00` / `_01` / `_11` | `c1240_0004` | 2410271 | c1240 | 124400 | 124400 |
| `m24_01_00_00` / `_01` / `_11` | `c1240_0005` | 2410272 | c1240 | 124400 | 124400 |
| `m24_01_00_00` / `_01` / `_11` | `c1240_0008` | 2410275 | c1240 | 124401 | 124400 |
| `m24_01_00_00` / `_01` / `_11` | `c1240_0010` | 2410277 | c1240 | 124401 | 124400 |
| `m24_01_00_00` / `_01` / `_11` | `c1240_0011` | 2410278 | c1240 | 124401 | 124400 |
| `m24_01_00_00` / `_01` / `_11` | `c1240_0012` | 2410279 | c1240 | 124401 | 124400 |
| `m27_00_00_00` / `_01` | `c1240_0001` | 2700301 | c1240 | 124501 | 124551 |
| `m27_00_00_00` / `_01` | `c1240_0002` | 2700302 | c1240 | 124501 | 124500 |
| `m27_00_00_00` / `_01` | `c1240_0004` | 2700308 | c1240 | 124501 | 124500 |
| `m27_00_00_00` / `_01` | `c1240_0005` | 2700309 | c1240 | 124501 | 124500 |

Note `c1240_0001` in the Forbidden Woods carries behaviour row `124551` where
its three neighbours carry `124500`. A freeze test that compared only the model
would pass a run that swapped one caged dog's behaviour for another's, which is
why `_frozen` compares model *and* behaviour row.

### M4 — the two geometric derivations and the margin

```python
# preamble P, then:
CAGE={'o243010','o243011','o273011'}
rows=[]
for mn in BASE_MAPS:
    cages=[o for o in objects(maps[mn]) if o[0] in CAGE]
    for e in enemies(maps[mn]):
        d=min((math.dist(e['pos'],o[3]) for o in cages), default=None)
        if d is not None and d<=1.0: rows.append((round(d,3),mn,e['name'],e['entity']))
print(len(rows), max(r[0] for r in rows))          # 26, 0.899
```

Derivation A (above) names the three cage models and lets any creature match;
derivation B names the creature and lets any object match. Both return the same
26. Full ordered output of B, which is also what makes the margin visible:

```
0.135 m27 c1240_0002   0.253 m27 c1240_0001   0.256 m24_01 c1240_0008
0.261 m27 c1240_0004   0.273 m24_01 c1240_0010  0.276 m24_01 c1240_0011/_0012
0.342 m27 c1240_0005   0.899 m24_01 c1240_0004/_0005   <- last protected
1.517 m28 c1240_0001   <- first unprotected c1240, in Yahar'gul
```

Against the cage models specifically, the nearest **unprotected** placement of
any creature in the game is `c1240_0007` at **2.671** — the seventh Central
Yharnam yard dog, the one spec D5 deliberately leaves out. So any radius in
(0.899, 2.671) produces the protected set exactly, and the verifier asserts the
two bounds rather than the radius.

### M7, M8 — entity IDs

```python
# preamble P, then:
for mn in BASE_MAPS:
    c=collections.Counter(e['entity'] for e in enemies(maps[mn]) if e['entity']>0)
    d={k:v for k,v in c.items() if v>1}
    if d: print(mn, d)          # only m33_00_00_00: {3300350: 2, 3300421: 2}
```

Two Central Yharnam dogs (`c1240_0001`, `c1240_0002`) carry entity ID `-1`, as
do many placements across the game, which is why the predicate rejects `<= 0`
before comparing. Across *all* part types the five protected map files do
contain repeated entity IDs — e.g. `2411200` appears on an object and on a
part of type 9 in `m24_01`, and `2700405` on an enemy and a part of type 10 in
`m27` — but the gate only ever sees enemy parts, so this cannot reach it. None
of the ten protected IDs is duplicated on any part of any type.

### M9, M14 — the UI and config budgets

`Font8x8.cpp`'s `kAdvance` is 9 pre-scale, the settings list draws at scale 4,
and `kScreenWidth` is 1920: 1920 / (9 × 4) = **53** characters per row.
`DO NOT RANDOMIZE CAGED DOGS   YES` is 33. The longest existing row,
`RANDOMIZE STARTING WEAPONS   YES`, is 32.

`pool_verify.py`'s worst-case `defaults.cfg` figure is 555 bytes today;
`do_not_randomize_caged_dogs=1` plus its newline adds 30, giving **585**,
comfortably inside `char buf[1024]` and inside the `snprintf` clamp.

`ui_scroll_verify.py` with the three settings screens raised from 15 rows to 16
passes unchanged: Setup Defaults shows 7 of 16, both wizard screens 6 of 16,
and every geometry and scroll property still holds.

```
python - <<'EOF'
src=open('app/tools/ui_scroll_verify.py',encoding='utf-8').read().replace('4, 15,','4, 16,')
exec(compile(src,'u','exec'))
EOF
```

### M10 — the cage props, and the empty cages

```python
# preamble P, then:
for mn in ('m24_01_00_00','m27_00_00_00'):
    print(mn, collections.Counter(o[0] for o in objects(maps[mn])
                                  if o[0] in {'o243010','o243011','o273011'}))
# m24_01_00_00 {'o243011': 11, 'o243010': 2}   m27_00_00_00 {'o273011': 6}
```

The Central Yharnam yard holds **13** cage props and **6** caged dogs; seven
`o243011` instances hold nothing. The Forbidden Woods cluster holds **6** cage
props and **4** caged dogs, which the spec already records. The two
`o243010` instances — the breakout pair's cages — are the only cage props in
Central Yharnam carrying an entity ID (2411270, 2411271); all eleven
`o243011` instances that a dog stands on carry `-1`.

This matters for the hardware handoff: a tester told "six dogs in cages" will
also see empty cages in Central Yharnam, and that is vanilla, not a miss.

### M12, M13 — reachability and the pre-feature baseline

`names.map_is_unused` flags `m24_01_00_11` and `m27_00_00_00` as never loaded,
leaving 16 of the 26 in map files the retail game can load. The plan covers all
26 regardless, per spec §6 and F13.

`data/runs/20260919-Enemies Only/` is a real output tree from the build
immediately before feature 032's first code change: seed **1234567890**,
enemies only, all 82 included, nothing skipped, pool 333, 2120 enemies
randomized, and its whole `dvdroot_ps4` tree is byte-identical to the
post-milestone run at the same seed. Against it:

```python
# preamble P, then:
out='data/runs/20260919-Enemies Only/dvdroot_ps4'
n=f=0
for mn in ('m24_01_00_00','m24_01_00_01','m24_01_00_11','m27_00_00_00','m27_00_00_01'):
    a,b=load_map(root,mn),load_map(out,mn)
    for x,y in zip(enemies(a),enemies(b)):
        if x['entity'] in PROT:
            n+=1
            if not (y['model']==x['model'] and y['think']==x['think']
                    and y['npc'] in (x['npc'],zone_scaled_npc(mn,x['npc']))): f+=1
print(n,f)   # 26 26
```

All 26 were re-targeted, to eighteen different models including an
Abhorrent Beast, a Brick Troll, a Nightmare Executioner and a Cramped Casket.
One of them — `c1240_0005` in `m24_01_00_01` — became a *different variant of
the Shaggy Hunting Dog* (`124551`/`900008215`), which is why a freeze test that
compared only the model name would pass a broken run. This tree is both the
problem the feature exists for and the failing-direction test the plan requires
(`caged_dogs_verify.py protected` must report FAIL against it).

### M15 — the cage scripts

Confirmed independently of the spec with a reader built on `read_dcx`. The
EMEVD64 header fields used: `fileSize` at `0x0C`; `(eventCount, eventOffset)`
at `0x10`; `(instructionCount, instructionOffset)` at `0x20`;
`(argLength, argOffset)` at `0x70`. Events are 48 bytes
(`id, instructionCount, instructionOffset, paramCount, paramOffset, …`) and the
instruction offset is a **byte** offset; instructions are 32 bytes
(`bank, index, argSize, argOffset, layerOffset`). Event initialisers are bank
2000 index 0, with `args[0]` the slot, `args[1]` the target event and the rest
the parameters. Two framing checks passed on both files: the section offsets
reconcile with the section sizes, and the per-event instruction counts sum to
the file's own total (4133 for `m24_01_00_00`, 1927 for `m27_00_00_00`).

`m24_01_00_00.emevd.dcx` — the three families, and nothing else names a dog:

```
event 12415420 x6 : 2410271 2410272 2410275 2410277 2410278 2410279
event 12410378 x2 : (2410271, 3021, 2411270, 2412046) (2410272, 3021, 2411271, 2412045)
event 12410380 x4 : 2410275 2410277 2410278 2410279   (each with 3020)
```

`m27_00_00_00.emevd.dcx`:

```
event 12705480 x6 : 2700301 2700302 2700303 2700308 2700309 2700310
event 12705500 x4 : (…, 2700301, 0, 2701110) (…, 2700302, 1, 2701112)
                    (…, 2700308, 3, 2701113) (…, 2700309, 4, 2701114)
events 12705510 / 12705540 x4, 12705520 x2, 12705530 x2 : the same four
```

Two things fall out. The dog↔cage-object pairing in `12410378` and `12705500`
matches the proximity measurement exactly, object for object — independent
confirmation that derivation A found cages and not scenery. And `12705480`
names two entity IDs, **2700303** and **2700310**, that match no placement in
any base map: the Forbidden Woods was authored for six caged dogs and ships
four, which is why two of its six cages are empty.

Only these six of the ten dogs are paired with an object entity in an
initialiser; the four Central Yharnam penned dogs are not, because their cages
carry entity ID `-1` (M10). That asymmetry is why the script cannot be the
primary derivation and is used as a cross-check.

---

## E5. Risk analysis

### E5.1 Renumbering the settings rows — retired by §9 P5

Both screens carry a comment warning that the row constants are also positions
in a parallel `items` vector, and that a mismatch compiles, passes
`ui_scroll_verify.py` and mislabels every row below the insertion point. The
planner's index-4 insertion would have moved eleven constants and three
vectors, and this section argued the residual risk was acceptable because the
constants are named everywhere, the three vectors are textually almost
identical, and a mismatch is immediately visible on the TV.

**The developer chose to append the row last instead (plan §9 P5).** No
existing constant moves, so this hazard no longer applies to this feature. What
survives of it is a much narrower check: the one new constant must equal the
position of the one appended `items` entry in all three vectors, and both
counts must reach 16. The cost paid for that is list order alone (§9 P13).

### E5.2 Stale objects after a header change

The Makefile compiles `.cpp` files found by `find` with no `-MMD`/`-MD`, so
there is no header dependency tracking: editing `RandomizerDefaults.h` or
`EnemyRandomizer.h` does **not** force the dependent translation units to
rebuild. `docs/build.md` records that a stale partial rebuild after a struct
layout change has already produced a heap-corruption `SIGSEGV` on hardware.
Both milestones add a field to a struct that the UI and the engine each hold,
so both require `make clean && make`. This is the most likely way for this
feature to produce a crash that looks unrelated to it.

### E5.3 An RNG-stream shift with the setting off

The gate is one `continue` in the hot loop. If the option test is not the first
term — for instance if `GetEntityID` is hoisted out of the condition, or the
gate is written above the `ENEMIES SKIPPED` test in a way that changes which
placements reach the draw — an off run could diverge from today's. Nothing in
the C++ makes this visible: the run still succeeds, still reports 2120 enemies,
and the world is silently different.

Bounded by the milestone 1 hardware test: one run at seed 1234567890 with the
setting off, diffed against `data/runs/20260919-Enemies Only/dvdroot_ps4`,
which is byte-identical-or-not with no interpretation required. That baseline
is the reason milestone 1 is worth stopping at.

### E5.4 A mis-framed EMEVD reader

A reader that computes an event's instruction range wrongly can still produce
plausible-looking output — bank 2000 instructions are common enough that a
wrong window will find some. Two checks per file close this: the header's
section offsets must reconcile with the section sizes, and the per-event
instruction counts must sum to the file's own instruction total. Both passed on
both files. If either fails during implementation the plan says to halt rather
than adjust offsets until output appears, because "adjust until it looks right"
is exactly how a mis-framed reader gets shipped.

### E5.5 The identification being incomplete on hardware

Spec §4 H1 is explicit that the *cause* of the lag and the unkillable
replacements is unknown. This feature is specified on the observation that
these spots misbehave, not on a traced mechanism, so the possibility remains
that protecting the ten does not cure the symptoms — because the cause is
elsewhere, or because more placements are involved. Low impact on the plan: the
worst case is a hardware report that the symptoms persist, which spec §8
already calls the most valuable result that test can produce. It is not
mitigated in code, and deliberately: widening the set to the seventh dog, to a
stat row or to a collision surface is listed as a stop condition precisely so
that an implementer meeting a surprise does not quietly redesign the feature.

### E5.6 The Forbidden Woods four are in on structural evidence only

Spec D9 records the asymmetry: Central Yharnam's six were observed to break,
the Forbidden Woods' four merely look identical in the data. Three
measurements now back them (proximity at a hundredfold margin, an entity-ID
script family, a unique stat row), and the marginal implementation cost is four
table entries. If the Forbidden Woods cages turn out to randomize perfectly
well, the cost of having protected them is four placements a player might have
preferred randomized — recoverable by deleting four lines.

### E5.7 Cost in the hot loop

`GetEntityID` reads an `i64` and an `i32` per placement, and the table walk is
up to ten integer comparisons plus a `find` on a short string. The loop runs
about 2,900 times per run, and the whole thing is behind the option test, so an
off run pays one boolean compare per placement. Not a concern; recorded because
"is this too slow to do per placement" is a question the shape invites.

---

## E6. What the spec's appendix claimed

The appendix is a lead, not a finding. Everything in it was checked.

| Claim | Found |
| ----- | ----- |
| `StepWriteMap`'s placement loop is where a placement is accepted or skipped; read the two gates and the roll's position first | Confirmed, and it is what decided the gate's position (§E2.1) |
| `part_fields::GetEntityID` already exists and needs no new accessor | Confirmed; declared in `Msbb.h:103`, already used by boss randomization, read-only |
| `BossParamScaling` runs after the enemy pass and bounds what §8 may assert | Confirmed; the DLC Central Yharnam file turns `124400` → `900008091` and `124401` → `900008122`, the DLC Forbidden Woods file `124501` → `900008254`. The three pre-DLC/unused files are not scaled at all |
| The four places a new on/off setting has to appear, plus `ui_scroll_verify.py` | Confirmed, and there are seven distinct edit sites in `EnableWizardScreen.cpp` alone (§E2.3). The appendix missed `pool_verify.py`'s worst-case config case, which is an equality and will fail when the key is added |
| `enemy_lookup.py`'s `load_base_maps`/`enemies`/`engine_pool` are the things to reuse; `tools/enemy_lookup.py pool` is the one that has been wrong | Confirmed; `engine_pool` reports 333/82 |
| Object parts live in the same section with a type discriminator of 1, at the same offsets | Confirmed |
| Both `.emevd` files are readable with a ~30-line reader; initialisers are bank 2000; the instruction offset is a byte offset | Confirmed, with the exact field offsets in M15 |
| The collision-surface field is background, not the identifier | Confirmed and not used |
| Object model IDs are per-area, so a shared suffix is not a shared asset | Confirmed: `o243010`, `o243011` and `o273011` are distinct models with instances only in their own area's files |
| Dead end: no single distinguishing field in the dog records | Confirmed by the stat-row measurement (M5) |

**Where the spec's own §4 is imprecise.** None of these changes the plan.

* **F7's parenthetical** says entity IDs are unique within every map the
  randomizer touches, with "one unrelated duplicate pair … in Nightmare
  Frontier". Among enemy parts `m33_00_00_00` has **two** duplicate pairs
  (3300350, 3300421), and across all part types several maps repeat an ID
  between an enemy and a non-enemy part. The conclusion stands — the ten
  protected IDs are unique wherever the gate can see them.
* **§7's UI budget** says the new row "sits alongside the existing longest row
  rather than beyond it". It is one character longer than it (33 vs 32). The
  budget is 53, so the point survives.
* **§2 and §8's hardware test 1** describe the Central Yharnam yard without
  mentioning that it contains roughly twice as many cage props as caged dogs
  (M10), which could read as a failed protection on hardware.
* **§8's "Automated testing" list** includes items 2–6, which all require a
  randomized output tree. The engine only runs on the PS4, so those are
  post-hardware checks; the plan's §6 splits them accordingly and gives the
  command that runs them against a captured tree.

---

## E7. Anything that could not be established

* **Whether the protection cures the symptoms.** Unchanged from the spec: the
  frame-rate collapse and the unkillable replacements have no traced cause
  (spec H1), and only hardware can answer it.
* **Whether the Forbidden Woods cages misbehave at all when randomized.** No
  one has walked them in a randomized run. Spec §8's hardware test 6 is
  written to produce that first observation.
* **What the cage scripts actually do.** The event framing and the argument
  wiring are recovered (M15); the meaning of the individual character
  instructions is not, and there is no EMEDF in the repo. Nothing in the plan
  depends on it.
* **Which of the Forbidden Woods four are "breakout" dogs.** The script splits
  them 2/2 in the same shape Central Yharnam's does, but all four have
  entity-ID'd cage objects where only Central Yharnam's breakout pair did. It
  does not affect the protected set.
* **Whether the engine actually consults the table correctly.** No host C++
  compiler and no emulator exist. `caged_dogs_verify.py selftest` pins the
  identifiers against the vanilla data; only a hardware run with the setting on
  shows that the gate fires. This is the standing gap
  (`CLAUDE.md` §3), not a gap in this plan.
* **Whether the twenty-six-placement count survives a different game revision.**
  Everything here is measured against one `data/vanilla/dvdroot_ps4` dump. A
  user whose dump differs would be outside every measurement in this document,
  as they already are for the pool table and the skip table.
