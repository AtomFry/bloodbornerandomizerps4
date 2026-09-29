# Plan Evidence 027 — No Team Type

**Plan:** `docs/features/027-no-team-type/plan.md`

**Spec:** `docs/features/027-no-team-type/spec.md`

---

> **This document is the investigation behind the plan, not instructions.** It is
> revisable. There is no length budget.

---

## E1. Reference trace

### E1.1 The function

`reference/Randomizer/MainWindowComponents/RandomizeFunctions.cs:3264-3304`,
`TeamTypeRando()`. Read in full. It does, in order:

1. Loads every PARAMDEF from `paramDefPath` into a `List<PARAMDEF>`.
2. Reads the whole param archive from `paramPath` with `BND4.Read`, parses every
   member with `PARAM.Read`, and applies the matching paramdef to each.
3. Takes `parms["NpcParam"]`.
4. Declares `List<PARAM.Row> talkList = new List<PARAM.Row>();` and **never uses
   it**.
5. Loops `for (int i = 0; i < currentParam.Rows.Count; i++)`, declaring
   `byte eight = 25;` **inside the loop body** and assigning
   `currentParam.Rows[i].Cells[100].Value = eight;`.
6. Re-serialises **every** member it read (`file.Bytes = parms[name].Write()`)
   and writes the archive back.

There is no `if`, no row-id test, no name test, no map scoping and no pool. The
loop bound is the row count. The local named `eight` holding `25` and the unused
`talkList` are untidiness, not behaviour — the spec's §3 account of both is
accurate.

Step 6 is the only part the port deliberately does **not** reproduce. The
reference round-trips every param through `PARAM.Write()`; the port pokes one
byte in the decompressed buffer and re-emits it. The port's approach is strictly
narrower — it cannot reflow a row table or change a member's size — and it is the
approach every shipped param feature here uses (`docs/plans/param-features.md`).
Reproducing a full re-serialisation would need a param writer the port does not
have and has no reason to acquire.

### E1.2 The gate and the call site

* `MainWindow.xaml:207` —
  `<CheckBox x:Name="AllDmgAll" Content="No Team Type" … Checked="AllDmgAll_Checked"/>`.
  No `IsChecked`, so **off by default**. A `Checked` handler and no `Unchecked`
  handler.
* `BooleanHandler.cs:63` — `teamTypeBool = AllDmgAll.IsChecked == true;` inside
  `SetBooleans()`, which re-reads every checkbox at the start of a run. Unticking
  is therefore honoured despite the missing `Unchecked` handler.
* `UIComponents.cs:2300-2304` — `AllDmgAll_Checked`, carrying the author's only
  note on the feature: `//cell 100 team type 25`.
* `StartFunctions.cs:1557-1560` — `if (teamTypeBool) { TeamTypeRando(); }`.

Confirmed: the spec's line citations are all correct.

### E1.3 Ordering in the reference

`StartFunctions.cs`, in source order within the same block:

| Line | Gate | Pass |
| ---: | ---- | ---- |
| 1536 | `enemyDropBool` | enemy drops |
| 1541 | `shopBool \|\| startingWeaponsOnlyBool` | `RandomizeShopItems()` |
| 1546 | `vfxBool` | `AiSoundParamRandomizer()` |
| **1557** | **`teamTypeBool`** | **`TeamTypeRando()`** |
| 1562 | `faceBool` | face/facegen |
| 1568 | `bloodBool` | decals |
| 1573 | `gemBool` | gem gen |
| 1578 | `talkBool` | talk |

So the reference's position is: after enemy drops, after shop items, before the
face/blood/gem/talk passes. The port has analogues of enemy drops and shop
items only, which is why plan §4.2 places the call after the starting-weapon /
shop block and before hunter tools.

**Ordering is not load-bearing here, and this is worth stating precisely because
ordering usually is.** Each reference pass re-reads the archive from disk and
writes it back, so an earlier pass's writes are visible to a later one. The two
passes that both touch `NpcParam` are the drop pass (`Cells[11]`, `Cells[12]`)
and this one (`Cells[100]`). They contest no byte. And because this pass's write
is unconditional and uniform, no later pass can observe a difference that
depends on when it ran: there is no value it could read that varies.

### E1.4 What the reference does *not* do

* It does not exclude `252100` or `6071`, the two rows its own drop pass skips
  (`RandomizeFunctions.cs:3125-3127, 3145-3147`).
* It does not exempt the Hunter's Dream, NPCs, bosses or talkers.
* It draws no randomness. `TeamTypeRando()` contains no `Next`, no `Random`, no
  reroll.
* None of the reference defects catalogued in
  `docs/windows-randomizer-technical-review.md` touches this path: no pool, no
  draw, no off-by-one to inherit or to fix.

---

## E2. What exists in the port

### E2.1 The `NpcParam` neighbour

`app/src/Randomizer/DropRandomizer.{h,cpp}`. Read in full.

* Its header comment already records the paramdef mapping the same way this
  feature needs: `cell 11 = itemLotId_1, s32, at byte 44 of a 388-byte NpcParam
  row`, `cell 12 = itemLotId_2, s32, at byte 48`, resolved by
  `tools/param_offsets.py`.
* It takes `(std::vector<uint8_t>& plain, const ParamMember& npcParam,
  std::mt19937& rng, DropRandomizerResult&, std::string* error)`, calls
  `ParseParamRows` once, and pokes fields in place with
  `ReadI32LE` / `WriteI32LE` from `Msb/BinUtil.h`.
* Its two offsets live in an anonymous namespace as `const size_t`. There is **no
  shared "NpcParam offsets" header** anywhere in the port.
* It carries two deliberate deviations from the reference (a bounded reroll cap
  and the fixed `Next(0, count-1)` off-by-one), both recorded in
  `docs/plans/param-features.md`. Neither is relevant here — this feature has no
  draw and no reroll.

**Conclusion: reuse the shape, not the file.** Nothing in `DropRandomizer.cpp` is
needed by this feature. A shared offsets header would move two constants out of a
shipped, hardware-proven file for the sake of a third constant that is not
related to them. Compare `HunterTools.cpp`, `TrickWeaponGrant.cpp` and
`LeftHandWeaponGrant.cpp`: all three write `CharaInitParam` and each names its
own offsets, with a carve-out (`WeaponRequirements.h`) only where two features
genuinely needed the *same* mutable state. That is the house rule, and it says
"own file, own constant" here.

### E2.2 `StepItemData`

`app/src/Randomizer/EnemyRandomizer.cpp:980-1170`. A three-step state machine:
step 0 reads and decompresses the archive, step 1 runs every param feature
against one in-memory buffer, step 2 compresses and writes it. Step 1's blocks,
in source order:

1. `options.randomizeEnemyDrops` → `RandomizeEnemyDrops` (`NpcParam`).
2. `WeaponRequirementWriter reqWriter;` — created here deliberately so the
   requirement-lowering precedence holds across the three passes that use it.
3. `weaponOpts.Any()` → `RandomizeStartingWeapons` (`ShopLineupParam`,
   `EquipParamWeapon`).
4. `options.startWithHunterTools` → `GrantHunterTools` (`CharaInitParam`) and the
   temporary `RunRuneProbe`.
5. `options.trickWeapons.CountEnabled() > 0` → `GrantTrickWeapon`.
6. `options.leftHandWeapons.CountEnabled() > 0` → `GrantLeftHandWeapon`.

Blocks 5 and 6 are last **as a requirement**, with the in-source comment
explaining why: they are the only remaining passes that draw, so putting them at
the end means turning either on cannot move an earlier roll. Block 4's comment
states the opposite property for `startWithHunterTools`: "Order doesn't matter
for it… it writes the same two item ids every run."

This feature is block 4's shape exactly — a non-randomizer writing fixed bytes —
except that it touches a field no other pass reads at all, which makes its
position even freer. Plan §4.2 puts it between blocks 3 and 4, which is the
reference's relative position and leaves both the "grants are last" comment and
the drop block untouched.

Each block locates its own members with `FindParamMember` and `Fail`s with its
own message, so a missing member is fatal only to the feature that wants it.
`AnyParamFeature()` in `EnemyRandomizer.h` decides whether the archive is loaded
at all, and each feature is an `||` term in it.

### E2.3 What else writes `NpcParam`, and what does not

`BossParamScaling.{h,cpp}` reads `NpcScalingTable.h` and rewrites a **map
placement's** `NPCParamID` to a zone-tuned variant of the same creature. It never
opens the param archive. `EasyModes.cpp` writes `part_fields::SetEnemyNPCParamID`
— again a placement field. `EnemyRandomizer.cpp:851-855`'s enemy write is
`NPCParamID` / `ThinkParamID` / model index on a placement.

So inside `gameparam.parambnd.dcx`, `RandomizeEnemyDrops` is the **only** writer
of `NpcParam` today. This feature is the second, and they are byte-disjoint.

The consequence for the commute argument: every map-side pass chooses *which row*
a placement points at. After this pass every row carries the same `teamType`, so
the choice cannot affect the resulting team type. The argument the spec makes in
§4 is sound and was checked against all four map-side passes, not just the enemy
one.

### E2.4 The settings chain, site by site

Traced from `enableMergoDarkness`, the closest sibling (a non-randomizer, WORLD
category, off by default, fixed edit):

| Site | Line | What is there |
| ---- | ---- | ------------- |
| `RandomizerDefaults.h` | 113 | `bool enableMergoDarkness = false;`, with a long comment on why "no" genuinely means "do nothing" |
| `RandomizerDefaultsStore.cpp` | 39, 58 | `enable_mergo_darkness=%d` in `FormatSettings`' format string and its argument list |
| `RandomizerDefaultsStore.cpp` | 127 | `else if (strcmp(key, "enable_mergo_darkness") == 0)` in `ApplySettingKey` |
| `SettingsModel.h` | `SettingId::EnableMergoDarkness` | last before `SaveData` |
| `SettingsModel.cpp` | 120-123 | the one `World` entry, kind `Toggle`, with its help string |
| `WorldActivation.cpp` | 771 | `options.enableMergoDarkness = run.enableMergoDarkness;` |
| `WorldActivation.cpp` | 795 | a term in the `anythingOn` chain |
| `WorldEditorScreen.cpp` | 1291 | a report line when on, and the in-source note that there is deliberately no `SKIPPING` counterpart |
| `worlds_verify.py` | 1391, 1407 | one entry in `WIZARD_OPTIONS_MAPPING` and one in `WIZARD_RUN_DECISION`, both pinned as ordered lists |

Neither settings screen has per-setting code any more: `SettingsModel.h`'s
header comment records that the old row-index design was removed and that
"adding a setting is one entry in one place". Confirmed by reading both screens —
they render any `Toggle` generically. `ui_scroll_verify.py` no longer pins
per-screen row counts for these screens, so unlike plan 033 this feature does
not touch it.

### E2.5 The verification tools

| Tool | What it owns | Verdict |
| ---- | ------------ | ------- |
| `drops_verify.py` | `NpcParam` inside the archive; D-I1–D-I7 | **Extend and correct** — see §E5.1. It is the tool that would otherwise silently pass a wrong tree |
| `hunter_tools_verify.py` | `CharaInitParam`; the `--granted` / `--granted-left` declaration-flag pattern | **The model to copy.** Its docstring is also the model for saying plainly what a mirror cannot prove |
| `mergo_darkness_verify.py` | the one other non-randomizer's output; `show` / `verify dark\|lit` / `selftest` | Read for shape. Its warning — "What the game then DOES with either state is the part that is not understood - do not infer it from here" — is the exact posture this feature needs |
| `param_offsets.py` | paramdef → field name, type, byte offset; row tables | **Reuse as-is**, and it is where every offset in this plan came from |
| `pool_verify.py` | the settings-block byte arithmetic, as an exact equality | **Extend** — one delta and one key case |
| `settings_ui_verify.py` | one model entry per bool, category membership against the settings-UI spec §7.1, row and help geometry against the baked atlas | **Extend** — `PROSE_TO_LABEL` plus the spec table edit |
| `worlds_verify.py` | the options mapping and `anythingOn` chain, field for field | **Extend** — one insertion in each pinned list |
| `itemdata_verify.py` | the archive round trip at 28 MB scale | **Reuse as-is** |
| `enemy_lookup.py`, `boss_verify.py`, `names.py` | base-map loading, the MSBB reader, model names | Reuse as-is; they produced §E4's placement census |

### E2.6 Prior decisions checked

* `docs/deferred-ideas.md` — no entry about teams, factions or allegiances.
* `docs/design-decisions.md` — records the `m21_00_00_00` never-randomized quirk.
  That protection is **map-scoped**, and `NpcParam` is not a map, so it gives the
  Doll, Gehrman and the Messengers no cover here. No entry about teams.
* `docs/enemy-exclusion-history.md` — the standing record behind D2. The removed
  `EnemyExclusionListExtra` was 45 substring patterns that froze **119
  placements**; it was added to explain bosses and items changing, a symptom
  later traced to a contaminated `VanillaSource` (the `.bak` polarity problem);
  18 of the 45 protected creatures with 40+ placements game-wide, the worst being
  Huntsman (Transformed) at 283; and three saved Windows runs proved the
  reference protects none of them, so the list was a **deviation**, not a port.
  The lesson D2 applies: a protection needs its own evidence, not a plausible
  fear.
* `docs/known-traps.md` §"Byte decoding does not prove game behavior" — the rule
  D3(a) and §6's verdict table implement.
* `CLAUDE.md` §7 — chalice dungeons out of scope; match the reference before
  improving it.

---

## E3. Alternatives considered

| Approach | Why rejected |
| -------- | ------------ |
| Extend `DropRandomizer.{h,cpp}` with a second function | The two features share a table and nothing else: different fields, one with RNG and one without, one with exclusions and one deliberately without. Merging them would put an unconditional write next to an excluded one in the same file, which is exactly the confusion `docs/enemy-exclusion-history.md` warns about. It also reopens a shipped, hardware-proven file for no gain |
| Add the write to the drop pass's existing row loop | Would tie the feature's execution to `randomizeEnemyDrops`, so it could not run alone, and would make the two settings' outputs entangled in a file where they are provably independent |
| Carve the `NpcParam` offsets into a shared header | Nothing is shared. Offsets 44/48 and 303 are unrelated fields of the same struct; a shared header would be a bag of constants, not an abstraction. `HunterTools`/`TrickWeaponGrant`/`LeftHandWeaponGrant` each name their own `CharaInitParam` offsets and only carved out `WeaponRequirements.h`, where two features needed the *same* mutable state |
| Run the pass first in `StepItemData`, before the drop block | Slightly clearer as a statement of "draws nothing", but it contradicts the reference's own order for no behavioural benefit, and `CLAUDE.md` §7 makes reference order the default when nothing else decides |
| Run the pass last, after the two grants | Would sit past the "these are the last rolls of the run" comment and invite a future reader to think the grants are no longer last. Harmless but misleading |
| Skip `252100` and `6071` because the drop pass skips them | A deviation with no evidence. The reference writes them; D2 and `CLAUDE.md` §7 forbid the exemption. The larva interaction it would "fix" is recorded as §E5.2 instead, which is the honest handling |
| Protect the Hunter's Dream rows, or `teamType` 26 rows generally | D2 settled it, and `docs/enemy-exclusion-history.md` records the cost of the last speculative protection on this project. Also weakly motivated: 19 of the 21 Messenger placements already sit on allegiance 23 with 30,076 enemy records and are perfectly peaceful, so the field is demonstrably not sufficient to make something hostile |
| Only write rows referenced by a base map (553 of 31,398) | A deviation, and it would leave the 24,242 scaling variants inconsistent with their base rows — which is worse than either uniform state, and unmeasurable without hardware |
| Extend `drops_verify.py` to own this feature's positive assertions too | Its `compare()` is built around the drop rules (pool membership, the two exclusions, the `-1` rule) and would have to grow a mode switch. A separate tool per feature is the established pattern: four param features, four tools. `drops_verify.py` still gets the D-I2 correction and a tolerance flag, because it must be able to judge a combined tree |
| Leave `drops_verify.py` alone, since the byte ranges are disjoint | Would leave a measured hole: D-I2 today passes a tree in which every `teamType` was rewritten, and also passes a stray byte at offset 100 (§E5.1). "Disjoint" is why there is no *collision*; it is not why there is no *gap* |
| Report the setting as a state line (`ALL CREATURES ON ONE ALLEGIANCE`) rather than a count | A fixed count makes a wrong figure legible, which is the reason the four easy-mode lines carry theirs. `START WITH HUNTER TOOLS` reports a state because its honest count (22 origin rows) invites the wrong question; 31,398 invites no question at all |
| Name the C++ field `enemiesHostileToEachOther`, matching the label | D3 fixed the key as `no_team_type` precisely so a label revision touches no persistence. Naming the field for the label would undo half of that |
| One milestone | The verifier half is a new tool with eleven cases plus a correction to an existing tool with its own selftest; the settings half is nine sites across four source files, three tools and a spec. Two passes, split where the halves are each meaningful functionality |
| Three or more milestones, or a gate after M1 | Would be splitting to create a checkpoint, which `CLAUDE.md` §4 forbids — and the checkpoint would not even exist, because the setting is unreachable until M2 |

---

## E4. Measurements

All against `data/vanilla/dvdroot_ps4`. Commands run from the repository root
unless noted.

| # | Quantity | Value | How measured |
| - | -------- | ----: | ------------ |
| M1 | `NpcParam` row count and stride | **31,398** rows, single stride **388** | `python app/tools/param_offsets.py rows data/vanilla/dvdroot_ps4 NpcParam.param` → `rows=31398  row stride(s) seen=[388]` |
| M2 | Cell 100's identity | `teamType`, `u8`, byte offset **303**; neighbours `npcType` at 302 and `moveType` at 304; 192 fields, computed row size 388 | `python app/tools/param_offsets.py fields data/vanilla/dvdroot_ps4 NPC_PARAM_ST 99 100 101` |
| M3 | `teamType` distribution | 13 distinct values: 23→30,076 · 25→**378** · 24→326 · 26→312 · 29→105 · 27→104 · 20→81 · 28→8 · 22→4 · 0/4/19/21→1 each | `python -c` over `drops_verify.load_archive` + `npc_rows`, reading `plain[at+303]` (script in §E4.1) |
| M4 | Rows the pass changes | **31,020** (31,398 − 378) | same script |
| M5 | Scaling variants present | **24,242** row ids in `[900000000, 910000000)`; all row ids distinct | same script |
| M6 | Base-map placement census | 24 base maps · **2,877** enemy placements · **553** distinct `NpcParam` rows referenced · by allegiance 23→2,431, 26→273, 22→60, 24→41, 20→26, **25→20**, 27→14, 28→5, 29→3, 4→2, 21→2 · **2,857** change, 20 already hold 25, **0** keep a distinct one · **0** placements reference a row absent from `NpcParam` | `enemy_lookup.load_base_maps` + `enemies()`, joined to the `teamType` map (§E4.1) |
| M7 | `drops_verify.py`'s blind spot | Rewriting `teamType` in **every** row produces **0** failures; mutating byte 100 of one row produces **0**; mutating byte 46 is caught (by D-I3/D-I5, not D-I2) | §E4.2 |
| M8 | Settings-block arithmetic | `no_team_type=1` + `\n` = **15** bytes. Settings block **728 → 743**; worst-case `defaults.cfg` **779 → 794**; `char buf[1024]`, 281 bytes spare | `app/tools/pool_verify.py:688-734` read directly; the two pinned equalities are `worst == 779` and `settings_block == 728` |
| M9 | Pinned mapping lists | `WIZARD_OPTIONS_MAPPING` 21 entries, `WIZARD_RUN_DECISION` 15 entries, both ordered; `enableMergoDarkness` at index 8 / 7 | `app/tools/worlds_verify.py:1382-1416` |
| M10 | Label geometry | `ENEMIES HOSTILE TO EACH OTHER` = **558 px** at row scale; label + 40 px gap + `YES` = **651 px** against `PANE_W` 700. Current widest pane row is 649 px, so this becomes the widest. All characters in the atlas | §E4.3 |
| M11 | Help geometry | The §4.3 help wraps to **7** lines at `HELP_W` 440 against `HELP_BODY_MAX_LINES` 11; current worst is 6. The label wraps to **2** help-title lines against `HELP_TITLE_MAX_LINES` 2 | §E4.3 |
| M12 | Progress line | `SET 31398 CREATURE RECORDS TO ONE ALLEGIANCE` = **44** characters against `pool_verify.LINE_CHARS` 71 | character count; budget from `app/tools/pool_verify.py:58` |
| M13 | Hunter's Dream census | `m21_00_00_00` enemy placements by (model, `teamType`): `c9020`(Messenger) **19 on 23** and 2 on 26; `c9010` 4 on 26; `c8030`/`c8040` 1 each on 26; `c8050` 1 on **25**; `c5400`, `c9040`, `c9050` 1 each on 23 | §E4.1 |
| M14 | Easy-mode larva | `NpcParam` **252100** carries `teamType` **26**; so does **6071**; all **31** scaled variants `900014601`–`900014631` carry **26** | §E4.1 |
| M15 | Tool baselines before any change | `drops_verify.py selftest` **6/6**; `pool_verify.py selftest` **95/95**; `worlds_verify.py selftest` **29/29**; `settings_ui_verify.py` all passing, reporting "widest rail row 552/580 px, widest pane row 649/700 px, worst help body 6/11 lines" | the four commands, run as written in plan §6 |

### E4.1 The census script

Run from `app/tools/`:

```
python -c "
import collections, drops_verify as d, enemy_lookup as el
R='../../data/vanilla/dvdroot_ps4'
plain,mem=d.load_archive(R)
tt={rid: plain[at+303] for rid,at in d.npc_rows(plain,mem)}
print('rows', len(tt), 'distribution', collections.Counter(tt.values()).most_common())
print('scaled', sum(1 for r in tt if 900000000<=r<910000000))
maps=el.load_base_maps(R); c=collections.Counter(); missing=[]
for name,m in maps.items():
    for e in el.enemies(m):
        if e['npc'] in tt: c[tt[e['npc']]]+=1
        else: missing.append((name, e['name'], e['npc']))
print('placements', sum(c.values()), c.most_common(), 'missing', missing)
print('larva', tt.get(252100), tt.get(6071),
      set(tt[r] for r in range(900014601,900014632) if r in tt))
dc=collections.Counter()
for e in el.enemies(maps['m21_00_00_00']): dc[(e['model'], tt.get(e['npc']))]+=1
print(sorted(dc.items()))
"
```

### E4.2 The `drops_verify.py` blind-spot probe

Run from `app/tools/`:

```
python -c "
import drops_verify as d
R='../../data/vanilla/dvdroot_ps4'
van,mem=d.load_archive(R); rows=dict(d.npc_rows(van,mem)); rid=next(iter(rows))
out=bytearray(van)
for r,at in rows.items(): out[at+303]=25
print('every teamType rewritten ->', len(d.compare(van,mem,out,mem)[0]), 'failures')
o2=bytearray(van); o2[rows[rid]+100]=0xEE
print('byte 100 mutated ->', len(d.compare(van,mem,o2,mem)[0]), 'failures')
o3=bytearray(van); o3[rows[rid]+46]=0xEE
print('byte 46 mutated ->', d.compare(van,mem,o3,mem)[0][:2])
"
```

Output: `0 failures`, `0 failures`, and two D-I3/D-I5 failures respectively.

### E4.3 The geometry probe

Run from `app/tools/`:

```
python -c "
import settings_ui_verify as s
L='ENEMIES HOSTILE TO EACH OTHER'
H=(\"Puts every creature in the game onto one shared allegiance - enemies, \"
   \"bosses and the Hunter's Dream residents alike. What this changes in play \"
   \"is not yet established, so try it on a spare save.\")
w=s.width(L,s.ROW_SCALE)
print('label px', w, 'row px', w+s.VALUE_GAP+s.width('YES',s.ROW_SCALE), 'budget', s.PANE_W)
print('help lines', len(s.wrap(H,s.ROW_SCALE,s.HELP_W)), 'of', s.HELP_BODY_MAX_LINES)
print('title lines', len(s.wrap(L,s.ROW_SCALE,s.HELP_W)), 'of', s.HELP_TITLE_MAX_LINES)
print('renderable', all(c in s.ADV[s.ROW_SCALE] for c in L+H))
"
```

---

## E5. Risk analysis

### E5.1 `drops_verify.py`'s D-I2 does not check what it says it checks

`drops_verify.py`'s docstring declares
`D-I2  within NpcParam, only itemLotId_1 differs - row ids and everything else intact`.
The implementation compares two slices per row:

* `van_plain[vat : vat+44]` versus the output — bytes 0–43, the bytes *before*
  `itemLotId_1`;
* `van_plain[vat+48 : vat+48]` versus the output — which is the **empty slice**,
  intended as "between the two lot fields" but computed as
  `[44+4, 48) == [48, 48)`.

Bytes **52 to 387** of every row are compared against nothing. `teamType` at 303
sits in that range, and so does most of the row. Measured (M7): a tree in which
every one of the 31,398 rows had `teamType` rewritten passes with zero failures,
and so does a tree with an arbitrary byte corrupted at offset 100.

**Likelihood: certain** — it is the present state of the tool. **Impact:** if it
is left alone, this feature and the drop feature do not collide; they leave a
hole. `drops_verify.py verify` would report a clean pass on a tree where the new
pass had written the wrong offset in every row, and the developer would have no
signal short of hardware.

**What the plan does:** M1 change 4 corrects D-I2 to compare bytes 0–43 and
52–387 (the whole row outside `itemLotId_1`), and then adds `--team-type` as a
*declaration*, tolerating byte 303 only when the flag is given and asserting that
every row holds 25 when it is. That follows `hunter_tools_verify.py`'s
`--granted`, which tolerates the weapon member only while asserting the tolerated
id. D-S1 pins the correction in the failing direction — the two mutations that
pass today must fail afterwards — so the correction cannot silently regress.

Note the corrected D-I2 subsumes the empty-slice line; it is a strictly stronger
check, and every one of the six existing cases still passes against vanilla
(M15 is the baseline to compare against).

### E5.2 The easy-mode larva loses `teamType` 26

`docs/features/018-easy-shadows/plan-evidence.md` §E5.7 lists "teamType 26 means
non-hostile" as an assumption the easy-mode feature rests on, and its plan §3.25
cites the larva's profile including `teamType 26`. Measured (M14): `NpcParam`
252100 and all 31 of its scaled variants carry 26 today. This pass moves them to
25.

**What could happen:** with `ENEMIES HOSTILE TO EACH OTHER` and any easy mode on
together, the larvae that replace duplicate boss bodies might stop being
harmless — which would defeat the easy mode, in a boss arena, mid-fight.

**Likelihood: unknown, and unknowable offline.** The whole feature turns on what
the engine does with this field, and §4's evidence (19 Messenger placements on
allegiance 23, peaceful) says allegiance alone does not decide hostility. If that
holds, the larvae are unaffected. If it does not, 018's assumption was already
resting on the same unknown.

**What bounds it:** both settings are off by default, so the combination requires
two deliberate choices; and the larva is the *replacement* body, so the failure
mode is "the easy mode did not help", not "the save is lost".

**What the plan does:** nothing in the implementation — D2 and `CLAUDE.md` §7
forbid the exemption, and exempting one row would be exactly the kind of
unevidenced protection `docs/enemy-exclusion-history.md` records the cost of. It
is listed as a §3.3 hazard so the implementer does not "fix" it, and §6 states
plainly that the hardware test does not cover the combination. If it turns out to
matter, the observation is the justification for a one-row exemption later.

### E5.3 The settings-UI spec puts this setting in the wrong category

`docs/features/randomizer-settings-ui/spec.md` §7.1's backlog column lists
`No Team Type` under **Enemies**. D3 puts it in **World**.
`settings_ui_verify.py` case 3 compares the model's per-category membership
against that table, in order, so the model and the spec must be changed together
or case 3 fails. `PROSE_TO_LABEL` (lines 185-206) must also gain the prose → label
mapping or the comparison produces `?No Team Type`.

**What the plan does:** M2 step 4 does both edits together, and orders the
milestone so the model entry (step 3) is immediately followed by them. The prose
name stays `No Team Type` (P8), matching the `Protect Caged Dogs` →
`DO NOT RANDOMIZE CAGED DOGS` precedent, which keeps the spec table tied to the
backlog row and unaffected by a later label revision.

`settings_ui_verify.py` case 1 fails as soon as `RandomizerDefaults` gains a bool
with no `kSettings` entry, which is why the field (step 1) and the model entry
(step 3) are in the same milestone.

Appendix A is not asserted by any tool — it has no entries for features 037 or
038 — so adding a row there is housekeeping rather than a gate. It is included
because D3(a) makes the help wording a binding matter and Appendix A is where
this project records help wording.

### E5.4 The label may be false

D3 took `ENEMIES HOSTILE TO EACH OTHER` against the spec's own recommendation,
knowing that 378 rows already carry the value and that the creatures carrying it
do not attack each other in vanilla. If the hardware test shows no such effect,
the shipped label asserts something false — the `ENABLE MERGO DARKNESS` failure
repeated (`docs/plans/mergo-darkness.md` §2.7: the polarity measured on console
is the opposite of what the event decode suggested).

**What bounds it:** the setting is off by default; the key is `no_team_type`, so
the label can be revised without touching persistence; and the write is one byte
per row, reversible by regenerating the world with the setting off.

**What the plan does:** §6's hardware section is written as a verdict with three
classified outcomes rather than a pass/fail, so a negative result is a finding
rather than a failure; the help text (§4.3) states the mechanism and explicitly
declines to assert the effect (B8); and §3.3 tells the implementer not to
"improve" the help into a claim.

### E5.5 Chalice dungeons are reached

`NpcParam` is one global table and chalice maps are generated rather than shipped
as files, so the pass necessarily reaches chalice creatures. The amount cannot be
measured from the vanilla tree. This is a consequence of the field's scope, not
an expansion of the project's: no chalice map is read, no chalice behaviour is
added, and none is tested. `CLAUDE.md` §7's scope decision is about chalice
*content*, and "a chalice creature's allegiance changed" is not a chalice
feature. Recorded here so nobody discovers it later and reads it as scope creep.

### E5.6 The archive cost

Turning this setting on alone still costs the full `gameparam.parambnd.dcx`
decompress-and-rewrite, and this port's compressor writes stored blocks, so the
archive lands roughly 15× larger than the original. Spec §7 accepts this — it is
already paid by every existing param feature, the archive is loaded once per run
regardless of how many are on, and `itemdata_verify.py roundtrip` is the standing
check that the write path stays valid at that scale. No mitigation is planned or
needed.

### E5.7 Judged low enough not to change the implementation

* **A `u8` write cannot truncate or spill.** The field is `u8` and the value is
  25; the paramdef gives 303 as a single-byte field with `npcType` at 302 and
  `moveType` at 304 (M2). No masking, no endianness, no alignment question.
* **`ParseParamRows` is proven.** It is the same call the drop pass makes against
  the same member, hardware-exercised. A structural failure returns false and is
  reported by the block's own `Fail`.
* **31,398 single-byte stores is not a performance concern.** The drop pass
  already walks the same rows twice and reads two `int32`s per row per pass
  inside one `Step()`.
* **No saved data is invalidated.** The new key is absent from existing
  `defaults.cfg` files and world revisions and reads as off; `RandomizerDefaults`
  is never serialised as a struct.

---

## E6. What the spec's appendix claimed

The appendix is a lead, not a finding. Every item was checked.

| Claim | Found |
| ----- | ----- |
| `TeamTypeRando()` is the whole feature, ~40 lines including load and save | **Confirmed.** `RandomizeFunctions.cs:3264-3304` |
| `UIComponents.cs` `AllDmgAll_Checked` carries the only comment | **Confirmed**, lines 2300-2304, `//cell 100 team type 25` |
| `DropRandomizer.cpp` is "the pattern to follow exactly… this feature is that file with one offset and no RNG" | **Confirmed as a pattern, rejected as a place to put the code.** §E2.1, §E3 |
| `StepItemData()` is where a new param pass is hung; `AnyParamFeature()` decides whether the archive is loaded | **Confirmed.** §E2.2 |
| `param_offsets.py fields <dvdroot> NPC_PARAM_ST 100` gives name, type and offset directly | **Confirmed.** M2 |
| `pool_verify.py` asserts the settings-block size as an exact equality and expects to be extended | **Confirmed.** M8; two equalities at lines 732-734 |
| `settings_ui_verify.py` checks the width budget and drawable character set | **Confirmed**, and it also checks category membership against the settings-UI spec — which this feature must edit (§E5.3). The appendix did not mention that |
| `boss_verify`'s `Msbb` + `enemy_lookup.BASE_MAPS` + `names.model_name` turns param rows into named creatures and counts | **Confirmed**; `enemy_lookup.load_base_maps` + `enemies()` was the shorter route and reproduced M6 |
| `NpcScalingTable.h` holds roughly three quarters of the row ids as `900xxxxxx` variants | **Confirmed.** 24,242 of 31,398 (M5) |
| Chalice `map/mapstudio` entries are directories, so a naive listing raises `PermissionError` | Not exercised — `enemy_lookup.load_base_maps` filters by name |
| `NPC_TEAM_TYPE`'s value names are nowhere in the repo | **Confirmed.** The PARAMDEF names the enum type and stops |
| No param defines team relations | **Confirmed.** Nothing in the plan rests on it |

### Spec figures reproduced, and one that was not

Every §4 figure was independently reproduced: 31,398 rows · 388 stride · offset
303 · the exact 13-value distribution · 378 already at 25 · 31,020 changing ·
24,242 scaling variants · 2,877 placements · 553 distinct rows · the full
per-allegiance placement breakdown · 2,857 changing and 20 already holding it ·
19 of 21 Messenger placements on allegiance 23.

**One claim did not reproduce.** Spec §4 says "The three placements that
reference a row which does not exist in `NpcParam` — a Chime Maiden, a Nightmare
Apostle and a Chapel Giant — are all cutscene dummy parts". Joining all 2,877
enemy placements to the row table found **zero** unresolved references (M6): all
553 distinct rows exist, and the per-allegiance counts sum to 2,877 with nothing
left over.

This is a factual slip in the spec and **nothing in this plan depends on it**. It
makes the spec's own conclusion stronger, not weaker: if no placement references
a missing row, then literally every enemy placement in the base maps moves to
allegiance 25 and none is spared. Reported to the developer rather than silently
corrected, since the spec is approved.

---

## E7. Anything that could not be established

* **What `teamType` 25 means to the engine.** The relation table between teams
  lives in the executable. `SpEffectParam.changeTeamType` (one row, 4741, moving
  a creature to 29), `NpcThinkParam.TeamAttackEffectivity` and
  `BulletParam.isHitBothTeam` are the only other team-related fields in the whole
  paramdef set and none carries a relation table. The entire question is the
  hardware test.
* **Whether the label is true.** §E5.4. This is the same statement as above, and
  it is why §6 is written as a discovery with three classified outcomes.
* **Whether the Hunter's Dream survives.** The evidence says probably — 19
  Messenger placements already share the ordinary-enemy allegiance and are
  peaceful — but "probably" is why the Hunter's Dream is step 1 of the procedure
  and why D2's no-protection decision is revisitable with an observation.
* **Whether the easy-mode larvae stay harmless with both settings on.** §E5.2.
  Not covered by §6's test, and stated as such.
* **How many chalice creatures are affected.** Unmeasurable from the vanilla
  tree; chalice maps are generated. §E5.5.
* **Whether the engine's loop is the one that wrote the byte.** No mirror can
  show that. `team_type_verify.py` proves the tree carries the byte everywhere and
  only there; that the C++ produced it is a hardware-only claim, and
  `docs/known-traps.md`'s rule applies to the value's meaning regardless.
* **What the reference tool actually produces on console.** No saved Windows run
  with `No Team Type` ticked exists in `data/runs/`, so the port's output could
  not be diffed against the reference's the way `docs/enemy-exclusion-history.md`
  diffed three enemies-only runs. The trace (§E1) is the whole basis for
  "verbatim", and it is a complete one: the function is forty lines with no
  branches.
