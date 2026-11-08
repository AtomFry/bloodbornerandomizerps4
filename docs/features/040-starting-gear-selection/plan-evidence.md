# Plan Evidence 040 — Starting Gear Selection

**Plan:** `docs/features/040-starting-gear-selection/plan.md`

**Spec:** `docs/features/040-starting-gear-selection/spec.md`

---

> **This document is the investigation behind the plan, not instructions.**
> There is no length budget. Depth here is the point.

**Every command below is run from `app/tools/` with `PYTHONIOENCODING=utf-8`** —
this console is cp1252 and printing an armour name or a Japanese FMG member name
raises `UnicodeEncodeError` that reads like a parse failure
(`docs/known-traps.md`). `V` is `../../data/vanilla/dvdroot_ps4`. Several
commands share a preamble, written once here and referred to as **[P]**:

```python
import sys,re,collections,struct; sys.path.insert(0,'.')
from param_offsets import load_defs, field_offsets, load_param, param_rows
from fmg import load_fmg
V = r'../../data/vanilla/dvdroot_ps4'
PROT = '防具名.fmg'     # armour names
ITEM = 'アイテム名.fmg'  # goods names
p = load_param(V,'EquipParamProtector.param'); N,_ = load_fmg(V, PROT)
I = re.compile(r'^en\d+$')
nm = lambda i: (lambda n: None if (not n or n=='*' or I.match(n) or n=='NOT NEEDED') else n)((N.get(i) or '').strip())
SEL = [(r,o) for r,o in param_rows(p) if p[o+210] in (0,1,2,3) and nm(r) and p[o+209]==3]
```

---

## E1. Reference trace

**There is nothing to trace, and that was re-verified rather than taken from the
spec.** The reference tool has no code path that touches either half of this
feature.

```
$ grep -ril "CharaInitParam"       reference/Randomizer/
$ grep -ril "EquipParamProtector"  reference/Randomizer/
```

Both empty. The only `CharaInitParam` matches anywhere in `reference/` are inside
`reference/SoulsFormats/`, in the map-part definitions of four other FromSoftware
games — the same finding spec 037 §3 recorded. The reference's nearest behaviour
is the shop shuffle, which this port already ships as rows 5 and 11 and which
changes what the Bath Messengers and the Dream *sell*, never what a character
starts with. So `CLAUDE.md` §7's rule is satisfied trivially: every part of this
is an addition, with no reference behaviour to match and no quirk to preserve.

### E1.1 What stands in for a reference trace: the route is exercised on the very rows this feature writes

Row 37's whole risk was that `equip_Wep_Right` is `-1` on all 1700 rows, so the
route it needed was one the game's own data never exercises. **The opposite holds
here**, and it is why there is no probe milestone:

```
$ python -c "<[P]>; d=dict(param_rows(load_param(V,'CharaInitParam.param')));
  q=load_param(V,'CharaInitParam.param');
  print({rid: [struct.unpack_from('<i',q,d[rid]+x)[0] for x in (32,36,40,44)] for rid in (2000,3000,3500)})"
{2000: [230000, 231000, 232000, 233000],
 3000: [-1, 231000, 232000, 233000],
 3500: [-1, 231000, 232000, 233000]}
```

Those ids are Black Hood, Foreign Garb, Sullied Bandage and Foreign Trousers —
exactly what a new Bloodborne hunter is visibly wearing. The four fields this
feature writes are therefore populated with meaningful values on the 22 rows it
writes, and the values are observably the ones the engine acts on. That is as
close to proof as offline work gets, and it is a strictly stronger argument than
the spec's own "set on 1557 rows" framing.

### E1.2 Pass ordering in this port, and where the two new passes go

`EnemyRandomizer.cpp` `StepItemData` runs the param passes in a fixed order and
the comments state which positions are behavioural:

| Order | Pass | Draws? | Positional constraint |
| --- | --- | --- | --- |
| 1 | enemy drops | yes | — |
| 2 | starting weapons / guns / shop weapons (row 5) | yes | must precede row 11 so its rolls do not move |
| 3 | shop armour / shop items (row 11) | yes | after row 5, before the grants (plan 011 D1) |
| 4 | no team type (row 27) | no | commutes; placed for reference fidelity |
| 5 | hunter tools (row 34) + rune probe | no | commutes |
| 6 | trick weapon grant (row 37) | yes, once | **last-but-one**; last RNG consumer |
| 7 | left-hand weapon grant (row 38) | yes, once | **last**; same reason |

The plan puts the supplies grant at position 5b (it draws nothing, so it
commutes) and the armour grant at position 8, after the left-hand grant, so its
four draws remain the run's last and cannot move an earlier roll. The accepted
consequence — already accepted for rows 37/38 by the comment at position 3 — is
that toggling an *earlier* pass moves which pieces a fixed seed draws. Spec §7
requires independence between the four armour slots, not across features.

---

## E2. What exists in the port

### E2.1 The settings chain, end to end

Traced for `START WITH A TRICK WEAPON` (row 37) and `START WITH A LEFT WEAPON`
(row 38), which between them exercise every site a new picker needs:

| Site | What row 37/38 did | What row 40 needs |
| --- | --- | --- |
| `ModelPoolSelection.h` | nothing — generic over `<N, DefaultSelected>` | nothing |
| `EnemyPoolSelection.h` | one typedef each, `false` polarity | four typedefs |
| `RandomizerDefaults.h` | one selection field each | four selections + one bool |
| `RandomizerDefaultsStore.cpp` | one key in `FormatSettings` **and** `ApplySettingKey` | five keys |
| `SettingsModel.h` | one `SettingId` + one `SettingKind` each | five ids, four kinds |
| `SettingsModel.cpp` | one `kSettings` entry + five `switch` cases each | five entries, four cases at each of five sites |
| `ModelPicker.h` | one `PickerStrings` each | four |
| `ModelPicker.cpp` | `showRowId` added in 037; unchanged since | nothing |
| `SetupDefaultsScreen.*`, `WorldEditorScreen.h` | unchanged | nothing |
| `WorldActivation.cpp` | options mapping + `anythingOn` term | five of each |
| `WorldEditorScreen.cpp` | one progress line + item-data condition | up to five lines |
| `EnemyRandomizer.h` | option, `AnyParamFeature()`, two result fields | five options, result fields |
| `worlds_verify.py` | two frozen lists | five entries in each |

**Both hosting screens genuinely need no change.** They reach a picker entirely
through `IsDrillIn` and the four `Selection*` accessors in `SettingsModel.cpp`,
which is why row 38 cost no screen edit and row 40 will not either.

**`ToggleCount()` is computed, not pinned.** It counts `kSettings` entries whose
kind is `SettingKind::Toggle`, so adding `START WITH SUPPLIES` moves the readiness
summary from `n OF 15 ON` to `n OF 16 ON` with no edit and no verifier case to
update, and the four pickers stay out of it automatically. That is B15 for free,
and it is the same outcome plan 037 D2 chose deliberately.

### E2.2 `CharaInitRows.h` and the four features that use it

The header holds only the shape of the row — `kItemIdBase` 124, `kItemNumBase`
204, `kItemSlots` 10, `kRowBytes` 300, `kEmptySlot` -1, `kOriginRows` (22), and
`FindRow` / `RowHasItem` / `FirstEmptySlot`. Its own comment states the rule:
"It carries no feature's own constants: no item id, no weapon field, nothing
about what is being granted." The four clothing offsets are row shape by that
test, so the plan adds them here; the armour ids and the goods ids stay in their
own features' files.

Four callers today: `HunterTools.cpp`, `TrickWeaponGrant.cpp`,
`LeftHandWeaponGrant.cpp`, `RuneProbe.cpp`. Adding two more makes six writers of
these 22 rows, which is what spec §7 anticipates.

**A correction the header needs.** Its block comment says the two ten-row blocks
are "identical in every field either feature touches, differing only in
`equip_Helm` (230000 vs -1)". True as scoped, but the blocks differ in a second
field:

```
$ python -c "<[P]>; q=load_param(V,'CharaInitParam.param'); d=dict(param_rows(q));
  print({r: [struct.unpack_from('<i',q,d[r]+268+4*s)[0] for s in range(6)] for r in (2000,3000,3500)})"
{2000: [-1,-1,-1,-1,-1,-1], 3000: [0,0,0,0,0,0], 3500: [0,0,0,0,0,0]}
```

`secondaryItem_01..06` is `-1` on the 2000 block and `0` on the 3000 block. This
changes nothing about how the feature is built — all 22 rows are written either
way — but a reader looking for the whole difference will not find it in the
header, and the `0`-means-empty case is exactly the trap the first rune probe
fell into (§E5.2).

### E2.3 The two generators, and which to copy

`gen_weapon_table.py` (row 37) and `gen_left_hand_table.py` (row 38) differ in
exactly one thing, and it is the thing that decides which the armour generator
copies:

* **row 37** asserts the name test and the obtainability test **agree on every
  candidate** and refuses to write if they do not. Correct there: all 78 included
  rows pass both, all 7 excluded fail both.
* **row 38** cannot do that — 16 named against 14 obtainable — so it asserts one
  direction only (`obtainable ⊆ named`) and treats membership as a conjunction,
  in its own file rather than by loosening row 37's guard into a flag.

Armour is a third case: **neither direction holds** (§E4 M4). So the armour
generator is a third file again, asserting both differences by their exact
contents. Row 38's docstring already states the principle the plan follows — a
weaker guard in a shared file is worse than a precise guard in a separate one.

### E2.4 Verification tools that this feature perturbs

| Tool | Why it notices | What it needs |
| --- | --- | --- |
| `hunter_tools_verify.py` | `H-I3` rebuilds the vanilla row with only the item arrays and the two tolerated weapon fields patched in; an armour write is then a stray write | `--armour h,b,g,l`. **No flag needed for supplies** — H-I3 already tolerates any item-slot difference, and H-I4/H-I5 (both tools present once, slot 0 is the Mark) still hold with two more items present |
| `trick_weapons_verify.py` | `G3` was narrowed in row 37's M4 to the one field it writes | `--armour h,b,g,l` |
| `left_hand_weapons_verify.py` | same shape, `--granted-right` already exists | `--armour h,b,g,l` |
| `pool_verify.py` | config-size cases are exact equalities; a progress-line case asserts 71 characters | re-measured arithmetic, five keys in load and save, the new lines |
| `settings_ui_verify.py` | case 1 pins "the seven selection fields"; case 3 compares the model to the settings-UI spec §7.1; case 5 is a pixel budget | eleven selections, five spec rows, four `KIND_TO_COUNT` entries |
| `ui_scroll_verify.py` | one `SCREENS` entry per scrolling list, by convention, even when the numbers repeat | four entries |
| `starting_weapons_verify.py` | writes a different param entirely | nothing |

### E2.5 The rune probe

`RuneProbe.{h,cpp}` is still in the build and still enabled, riding on
`START WITH HUNTER TOOLS`. It writes `equip_Accessory01..05` (offset 64, five
s32s) unconditionally on all 22 origin rows, and `EnemyRandomizer.cpp` fails the
run if it did not write exactly 22. The plan does not touch it, does not plan
around an accessory slot, and does not assume it will be gone.

**A pre-existing defect found while surveying, not caused by this feature and not
fixed by it.** No verifier tolerates those accessory writes, so
`hunter_tools_verify.py verify <vanilla> <output>` against any real output tree
built with `START WITH HUNTER TOOLS` on already reports `H-I3 row N changed
outside item_* / itemNum_*`, 22 times. The plan's M3 combined check (A12) is a
simulated selftest and is unaffected; the real-tree direction for that
combination is already unusable and remains so. Reported rather than absorbed.

---

## E3. Alternatives considered

| Approach | Why rejected |
| -------- | ------------ |
| **One 124-row outfit or piece picker** | Spec §10 D1 settles four per-slot pickers. Not the planner's to reopen |
| **Four separate generated headers** (`HeadwearTable.h`, …) | The sum-to-124 invariant — the one assertion that says no wearable piece was dropped between the four tables or counted twice — cannot be stated in C++ anywhere unless all four counts are in one place. A `static_assert` in the generated header makes B2 a compile-time fact |
| **One `ArmourPool` `SettingKind` with a slot discriminator** | The five `switch` sites in `SettingsModel.cpp` (`SelectionArray`, `SelectionTable`, `SelectionCount`, `SelectionStrings`, `SettingValueText`) dispatch on kind alone and have no access to the entry; a single kind cannot answer "which table". Four kinds is also what makes `settings_ui_verify.py`'s "one entry per selection kind" case meaningful |
| **`protectorCategory` from the id (`id/1000 % 10`) instead of the field** | The two agree on all 252 wearable rows (M10), so the field is free and the arithmetic is a derived fact. Deriving the slot from the id would make a single mis-divided id place a hat in the glove field — the highest-consequence silent failure spec §8 names. The id relation is kept as an *assertion*, not a rule |
| **Filter the 21 `NOT NEEDED` placeholders by name** | 58 further rows in the four wearable slots are *also* named `NOT NEEDED` (M2). A name filter would drop them too and land on the same 124 by luck; `protectorCategory` is what actually separates "a fifth category" from "a wearable slot", and the two tests answer different questions |
| **Require `equipModelGender != 0` rather than `== 3`** | Gives 148 rows, not 124: 6 male-only and 12 female-only pieces would join, and a piece the player's character cannot wear is not a piece the picker may offer. `== 3` is what spec §4.2's "wearable by both sexes" means and what reproduces its counts |
| **Sanitise the three `/` names** | `gen_weapon_table.py`'s own error message states the house rule: a renamed row stops matching the inventory. Adding one glyph to the fallback font is smaller than three edited game strings, and the atlas path already renders `/` (M6) |
| **Suffix the duplicate names to disambiguate** | Invents a label, which spec §7 forbids. See §8 Q1 |
| **`showRowId = true` for the armour pickers** | Disambiguates 4 rows by printing a meaningless five-digit id beside all 124. Offered as the alternative in §8 Q1 because it costs one field and no table change, so it can be taken later |
| **One engine pass writing both clothing and inventory** | The two halves write disjoint fields, one draws and one does not, and one must run last while the other commutes. A single pass would have to be ordered by its stricter half and would put a deterministic write behind an RNG guard |
| **Extend `HunterTools.cpp` to also grant the supplies** | Row 34's output must stay byte-identical when its own toggle is on and this one is off. Two toggles in one pass means one `if` per item inside a shared loop and a shared result struct, for no gain over a 60-line sibling file |
| **A shared sequential draw: one `uniform_int_distribution` per non-empty slot** | The obvious implementation, and it breaks B6. See §E5.1 |
| **Per-slot `std::mt19937` seeded from the run seed** | Would also satisfy B6, but the pass is handed an `std::mt19937&` and not the seed; plumbing the seed through `EnemyRandomizerOptions` to reach one pass is a wider change than four raw `rng()` calls, and four independent generators are four more things to keep in step |
| **Derive the supplies count from `EquipParamGoods.maxNum` at run time** | Would need the pass to locate and parse a second param member to learn a number that is 20 for both ids and has been 20 since the game shipped. Pinning the constant *against* `maxNum` in the verifier (A9) gets the same guarantee for no C++ |
| **Grant the supplies into `secondaryItem_01..06`** | Empty on all 22 origin rows, used on 1376 others, and the ten ordinary slots already leave six spare in the worst case. Also the array whose "empty" is `0` on the 3000 block, which is the trap of §E5.2 |
| **Fix the four displaced garments by re-granting them into item slots** | Spec §2 states the four starting garments remain owned; whether they do is a hardware question (§E7), and pre-emptively writing them into item slots would spend four of the nine free slots on a problem that may not exist |

---

## E4. Measurements

All against `data/vanilla/dvdroot_ps4`. Commands assume the **[P]** preamble.

| # | Quantity | Value | How measured |
| - | -------- | ----: | ------------ |
| M1 | `EquipParamProtector` row size / `CHARACTER_INIT_PARAM` row size | 268 / 300 | `field_offsets(load_defs(V)['EQUIP_PARAM_PROTECTOR_ST'])[1]`, ditto `['CHARACTER_INIT_PARAM']` |
| M2 | Protector rows, and the `protectorCategory` split | 273 → 76 / 76 / 48 / 52 / 21 | below |
| M3 | The four picker sets | **33 / 34 / 26 / 31 = 124** | below |
| M4 | Obtainability cross-check | 122 obtainable; two exceptions each way | below |
| M5 | `lotItemCategory` census | cat 0 = weapons, cat 1 = armour | below |
| M6 | Unrenderable characters in the 124 names | `{'/'}`, in 3 names | below |
| M7 | The five new labels against the 700 px pane | widest 609 px | below |
| M8 | The 22 origin rows' clothing and inventory | see below | §E1.1 and below |
| M9 | Config arithmetic | +239 bytes → 1277 / 1328 | below |
| M10 | `id/1000 % 10` vs `protectorCategory` | 0 mismatches in 252 | below |
| M11 | Goods 1000 / 900 | named, `maxNum` 20 each | below |
| M12 | Picker pages at 11 visible rows | 3 / 4 / 3 / 3 | arithmetic on `ui_scroll_verify.py`'s instruction-layout band |

### M2 — 273 rows, and what the fifth category is

```
$ python -c "<[P]>; print(collections.Counter(p[o+210] for r,o in param_rows(p)));
  print(collections.Counter(p[o+209] for r,o in param_rows(p)))"
Counter({0: 76, 1: 76, 3: 52, 2: 48, 4: 21})
Counter({3: 133, 0: 91, 2: 27, 1: 22})
```

All 21 category-4 rows are named `NOT NEEDED`. **But so are 58 rows inside the
four wearable categories** — which is why the plan filters on the category for
"is this a wearable slot" and on the name for "does the game name this piece",
and never uses one to do the other's job:

```
$ python -c "<[P]>; W=[(r,o) for r,o in param_rows(p) if p[o+210] in (0,1,2,3)];
  print(len(W), sum(1 for r,o in W if (N.get(r) or '').strip()=='NOT NEEDED'))"
252 58
```

### M3 — the four picker sets

```
$ python -c "<[P]>; print(collections.Counter(p[o+210] for r,o in SEL), len(SEL))"
Counter({1: 34, 0: 33, 3: 31, 2: 26}) 124
```

33 head / 34 body / 26 gloves / 31 legwear, summing to 124 — spec §2's counts
exactly, and reached independently.

**The three filter steps, each worth its own number.** 252 rows in the four
wearable categories → 208 with any FMG string that is not `*`/`enNNNN` → 150 once
the 58 `NOT NEEDED` rows are dropped → **124** once `equipModelGender == 3` is
required. The 26 dropped at the last step are 6 male-only, 12 female-only, and 8
marked wearable by neither — of which 2 are named (§E4 M4) and 6 are not.

**Duplicate display names — a blocker the spec does not mention.**

```
$ python -c "<[P]>; c=collections.Counter(nm(r).upper() for r,o in SEL);
  print({k:v for k,v in c.items() if v>1});
  d=dict(param_rows(p));
  print([(i, nm(i), struct.unpack_from('<f',p,d[i]+32)[0], struct.unpack_from('<H',p,d[i]+160)[0], struct.unpack_from('<i',p,d[i]+0)[0]) for i in (71000,281000,191000,301000)])"
{'HUNTER GARB': 2, 'STUDENT UNIFORM': 2}
[(71000,'Hunter Garb',4.7,2200,10010), (281000,'Hunter Garb',4.7,2300,10011),
 (191000,'Student Uniform',3.0,6300,220010), (301000,'Student Uniform',4.7,6310,220011)]
```

Four rows, two names, both pairs in the body slot. They are genuinely distinct
rows — different `equipModelId`, adjacent `sortId` — and all four are obtainable
in vanilla, so a player really can own two garments the game calls Hunter Garb.
`gen_weapon_table.py`'s `build()` refuses to write when two rows share a display
name ("the picker would be ambiguous"), so an armour generator inheriting that
guard would not produce a table at all. Plan §8 Q1.

**Longest name.** `DECORATIVE OLD HUNTER TROUSERS`, 30 characters, 607 px at
picker scale 3 — well inside the 71-character fallback line (39 characters with
the widest flag word) and inside the band plan 037 measured at 791 px.

### M4 — obtainability, and the two exceptions in each direction

```
$ python -c "<[P]>; L=load_param(V,'ItemLotParam.param'); S=load_param(V,'ShopLineupParam.param')
  A={r for r,_ in param_rows(p)}; sold={struct.unpack_from('<i',S,o)[0] for _r,o in param_rows(S) if S[o+23]==1}&A
  lot=set()
  for _r,o in param_rows(L):
      for s in range(8):
          if struct.unpack_from('<i',L,o+32+4*s)[0]==1:
              i=struct.unpack_from('<i',L,o+4*s)[0]
              if i in A: lot.add(i)
  obt=sold|lot; inc={r for r,o in SEL}
  print(len(sold), len(lot), len(obt)); print(sorted(inc-obt)); print(sorted(obt-inc))"
44 93 122
[230000, 231000, 232000, 233000]
[250000, 430000]
```

* **120 of the 124 are obtainable.** The four that are not are exactly Black
  Hood, Foreign Garb, Sullied Bandage and Foreign Trousers — the clothes the
  character already wears, which need no shop and no pickup. Spec §4.2 says this
  and it is confirmed.
* **Two obtainable rows are excluded**, ids 250000 and 430000 — Crown of
  Illusions and Enlarged Head, the two rows `equipModelGender == 0` marks as
  wearable by neither sex.

**This contradicts spec §8 and §4.2.** The spec asserts "not one of the 126
unnamed rows and neither of the two cut rows is in any shop or any pickup
anywhere" and makes the verification criterion "every excluded row is both
unnamed and absent from every shop and every pickup". Both are false for those
two ids. The plan therefore states the cross-check as two explicit set
equalities rather than as a subset relation in either direction. **No included
row changes** — the 124 are unaffected — so the correction is to a stated
justification and a verification criterion, not to the feature.

### M5 — the lot-category census, and that fixing the constant is a no-op for row 37

```
$ python -c "<[P]>; L=load_param(V,'ItemLotParam.param')
  P={r for r,_ in param_rows(p)}; W={r for r,_ in param_rows(load_param(V,'EquipParamWeapon.param'))}
  G={r for r,_ in param_rows(load_param(V,'EquipParamGoods.param'))}
  h=collections.defaultdict(collections.Counter)
  for _r,o in param_rows(L):
      for s in range(8):
          c=struct.unpack_from('<i',L,o+32+4*s)[0]; i=struct.unpack_from('<i',L,o+4*s)[0]
          if i<=0: continue
          h[c]['n']+=1; h[c]['prot']+= i in P; h[c]['wep']+= i in W; h[c]['goods']+= i in G
  print({c:dict(v) for c,v in sorted(h.items())})"
{0:  {'n': 72,   'wep': 63,  'prot': 3,    'goods': 1},
 1:  {'n': 101,  'prot': 101,'wep': 3,     'goods': 3},
 4:  {'n': 7456, 'goods': 7283,'prot': 1696,'wep': 1307},
 8:  {'n': 1892, 'prot': 33, 'goods': 13},
 15: {'n': 451,  'prot': 6,  'wep': 2}}
```

**Category 0 is weapons and category 1 is armour**, unambiguously — 63 weapon ids
against 3 protector in category 0, and 101 protector against 3 weapon in
category 1. (Ids collide across param tables, a fact spec 011 established, which
is why the minority counts are not zero. The three "weapons" in category 1 and
the three "protectors" in category 0 are collisions, not misfilings.)
`gen_weapon_table.py:74`'s `LOT_CATEGORY_WEAPON = 1` is therefore wrong, and its
docstring's claim of "two independent tests" is one and a half.

**The fix is provably free for row 37:**

```
$ python -c "import sys; sys.path.insert(0,'.'); import gen_weapon_table as g
  V=r'../../data/vanilla/dvdroot_ps4'
  c,_=g.right_hand_tier0(V); named,_=g.named_ids(V,c)
  o1=g.obtainable_ids(V,c); g.LOT_CATEGORY_WEAPON=0; o0=g.obtainable_ids(V,c)
  print(len(c), len(named), len(o1), len(o0), named==o1==o0)"
85 78 78 78 True
```

Both categories give the identical 78 ids, because every one of the 78 is in a
shop listing and the lot half contributes nothing either way. So flipping the
constant cannot move a row of `TrickWeaponTable.h`, and `--check` passing is the
assertion that says so. The log's counts (99/2 and 63/2) differ from these
(101/3 and 63/3) because they were taken over distinct ids rather than slot hits;
the split is stark either way and the conclusion is the same.

### M6 — the three slashed names, and what renders them

```
$ python -c "<[P]>
  from gen_pool_table import RENDERABLE
  bad=collections.Counter(ch for r,o in SEL for ch in nm(r).upper() if ch not in RENDERABLE)
  print(dict(bad)); print([nm(r) for r,o in SEL if '/' in nm(r)])
  import settings_ui_verify as s; print('/' in s.ADV[3])"
{'/': 3}
["Knight's Trousers/Dress", 'Black Church Trousers/Dress', 'White Church Trousers/Dress']
True
```

`/` is the only unrenderable character in all 124 names, it occurs in exactly
three, **the font atlas already has it**, and only `Font8x8.cpp`'s 24-entry
fallback `kGlyphs` table lacks it. `FindGlyph` returns `nullptr` for an unknown
character and `DrawChar8x8` then draws nothing while still advancing the cursor —
a blank column, silently. One `kGlyphs` entry and one character in
`gen_pool_table.py`'s `RENDERABLE` closes it.

### M7 — the five labels against the pane budget

```
$ python -c "import sys; sys.path.insert(0,'.'); import settings_ui_verify as s
  w=lambda t: s.width(t,s.ROW_SCALE)
  for lab,val in [('START WITH HEADWEAR','12 OF 33'),('START WITH BODY ARMOUR','12 OF 34'),
                  ('START WITH GLOVES','12 OF 26'),('START WITH LEGWEAR','12 OF 31'),
                  ('START WITH SUPPLIES','YES'),('START WITH A TRICK WEAPON','12 OF 78'),
                  ('BOSS BEHAVIOUR','REPLACED ENEMYS AI')]:
      print('%-26s %4d' % (lab, w(lab)+s.VALUE_GAP+w(val)))"
START WITH HEADWEAR          556
START WITH BODY ARMOUR       609
START WITH GLOVES            492
START WITH LEGWEAR           526
START WITH SUPPLIES          443
START WITH A TRICK WEAPON    649
BOSS BEHAVIOUR               656
```

Budget is `PANE_W` = 700 px. All five fit; the widest is
`START WITH BODY ARMOUR` at 609 px, confirming the figure spec §10 D1 records.
The existing worst rows are `BOSS BEHAVIOUR` at 656 and
`START WITH A TRICK WEAPON` at 649, so this feature does not become the pane's
constraint. (Spec §4.7 quotes 663 px for `BOSS BEHAVIOUR`; measured here at 656.
Immaterial — both are under budget and neither is the row this feature adds.)
Picker headings at scale 4 measure 533 / 608 / 448 / 491 px against the same
700 px.

### M8 — the 22 origin rows

```
$ python -c "<[P]>; q=load_param(V,'CharaInitParam.param'); d=dict(param_rows(q))
  R=list(range(2000,2010))+list(range(3000,3010))+[3500,3501]
  print(len({tuple(struct.unpack_from('<i',q,d[r]+124+4*s)[0] for s in range(10)) for r in R}),
        {tuple(struct.unpack_from('<i',q,d[r]+124+4*s)[0] for s in range(10)) for r in R})
  print({tuple(q[d[r]+204+s] for s in range(10)) for r in R})
  print({tuple(struct.unpack_from('<i',q,d[r]+248+4*s)[0] for s in range(4)) for r in R})"
1 {(100, -1, -1, -1, -1, -1, -1, -1, -1, -1)}
{(1, 0, 0, 0, 0, 0, 0, 0, 0, 0)}
{(-1, -1, -1, -1)}
```

Every one of the 22 rows: item slot 0 is goods 100 (Hunter's Mark) at count 1,
slots 1–9 are `-1` at count 0, and all four clothing `*_GenId` fields are `-1`.
So **nine free slots per row**, `FirstEmptySlot`'s `-1` test is correct for
`item_NN`, the supplies take two leaving seven (five with the hunter tools also
on), and leaving the `GenId`s alone matches what `TrickWeaponGrant.cpp` already
does on the same ground.

`CharaInitParam.param` carries **1700 row descriptors over 1699 distinct ids** —
id 31101 appears twice. Not an origin row, and `FindRow` returns the first match,
so it is immaterial; recorded because the spec's "1700 rows" and a `len(set(...))`
of 1699 would otherwise look like a contradiction.

### M9 — the config arithmetic

```
$ python -c "keys=[('armour_head_included',33),('armour_body_included',34),
                   ('armour_gloves_included',26),('armour_legwear_included',31)]
  add=sum(len(k)+1+n+1 for k,n in keys); sup=len('start_with_supplies=1')+1
  print(add, sup, add+sup); print(1038+add+sup, 1089+add+sup)
  print(1038+add+sup-22, 1089+add+sup-22)"
217 22 239
1277 1328
1255 1306
```

Today's pinned figures in `pool_verify.py` are a 1038-byte settings block and a
1089-byte worst-case `defaults.cfg`, inside `char buf[2048]`. The five keys add
**239 bytes** → **1277 / 1328**, leaving 771 bytes of the buffer. Spec §4.7
predicted 1285 / 1336; the 8-byte difference is key naming, and the spec's own
note that stage C should re-measure is why this figure and not that one is in the
plan.

**Row 13's milestone 5 is the live risk to this number.** It deletes
`REPLACE ALL ENEMIES`, whose key line `replace_all_enemies=1\n` is 22 bytes. If
it lands before this feature, the baseline becomes 1016 / 1067 and the figures
become **1255 / 1306**. The plan states the rule (`baseline + 239`) rather than
the number, and makes a mismatch a stop condition, so either order of landing is
safe.

### M10 — the id decomposition, kept as an assertion

```
$ python -c "<[P]>; W=[(r,o) for r,o in param_rows(p) if p[o+210] in (0,1,2,3)]
  print(len(W), sum(1 for r,o in W if (r//1000)%10 != p[o+210]))"
252 0
```

`id/1000 % 10 == protectorCategory` on all 252 wearable-slot rows with no
exception. Kept as assertion A8 rather than used as the slot rule (§E3), because
it is the cheapest available guard against the one silent failure spec §8 ranks
first: a piece written into the wrong slot's field.

### M11 — the two goods ids

```
$ python -c "<[P]>; g=load_param(V,'EquipParamGoods.param'); d=dict(param_rows(g))
  M,_=load_fmg(V, ITEM)
  print(len(d), [(i, M.get(i), struct.unpack_from('<h',g,d[i]+54)[0]) for i in (1000,900,100,4103,4104)])"
295 [(1000,'Blood Vial',20), (900,'Quicksilver Bullets',20), (100,"Hunter's Mark",1),
     (4103,'Blood Gem Workshop Tool',1), (4104,'Rune Workshop Tool',1)]
```

Blood Vial is goods **1000** and Quicksilver Bullets goods **900**, and each
carries `maxNum = 20` — so spec D2's count of 20 is the game's own carry cap read
out of the param, not a chosen number. These are also exactly the two ids
`ShopLineup.h`'s `kNeverTouchEquipIds` protects as the ones a playthrough cannot
do without, which is independent corroboration that these are the right two ids.

---

## E5. Risk analysis

### E5.1 The four draws, and why the obvious implementation breaks B6

Spec §7 requires that the four slots be drawn independently, and spec §8 makes it
a verification criterion: *changing what is ticked in one slot must not change
which piece a fixed seed draws in another*. This is the one requirement in the
feature that a natural implementation gets wrong.

The natural implementation is the shipped one, four times: for each slot with
anything ticked, `std::uniform_int_distribution<int> pick(0, k-1); id =
candidates[pick(rng)]`. It fails in two independent ways against one shared
`std::mt19937`:

1. **A slot with nothing ticked draws nothing at all**, so the *number* of draws
   before the gloves slot depends on whether headwear and body have anything
   ticked. Ticking one head piece shifts every later slot's draw.
2. **Even with a draw per slot unconditionally**, `uniform_int_distribution`
   consumes a *variable* number of raw words: its rejection-sampling loop depends
   on the range. Changing headwear's candidate count from 1 to 17 can change how
   many words it eats, which again shifts the gloves draw.

The plan's mechanism removes both: **exactly four `rng()` calls whenever the
feature does anything**, one per slot in the frozen order, and
`index = word % k`. The raw call count is then a constant independent of every
tick, and each slot's index is a pure function of its own word and its own
candidate list.

Two costs, both accepted:

* **Modulo bias.** `word` is uniform over 2³², `k` ≤ 34, so the bias is on the
  order of 10⁻⁸ per candidate. Against a picker choosing a hat, it is not
  measurable and not worth a rejection loop that would reintroduce variable
  consumption.
* **All four words are consumed even when only one slot is ticked.** That is the
  point, and it is why "nothing ticked anywhere" must return *before* the first
  `rng()` call — otherwise turning the four pickers on with nothing ticked would
  consume four words and stop being identical to leaving them off. The plan makes
  that early return step 2, ahead of the draws.

**What is deliberately not promised.** Turning `START WITH A TRICK WEAPON` on
still moves which armour a seed draws, because both share one stream and the
armour pass runs later. That is the behaviour already accepted for rows 5, 11, 37
and 38 by `EnemyRandomizer.cpp`'s own comment, and spec §7's determinism clause
asks only that turning a setting on not "reshuffle the rest of a run's world" —
which putting this pass last guarantees.

### E5.2 The namespace trap, and why it cannot be hit here

Spec §4.5 records that the first rune probe wrote a category-8 "magic stone" id
into `item_01..10` and `secondaryItem_01..06`, reached the shipped tree on all 22
origin rows, and was ignored completely, because `item_NN` resolves its id against
`EquipParamGoods` (which stops at 12140) and a category-8 id has no row there. A
well-formed write into the wrong table looks exactly like a field the game does
not read.

This feature writes two namespaces and each write has a structural guard:

* **Clothing fields** take `EquipParamProtector` ids. Every id in the four tables
  is a row id of that param by construction (the generator enumerates the param's
  own rows), and A8 additionally asserts that each id's own decomposition agrees
  with the slot it is filed under, so a head id cannot reach the glove field.
* **Inventory slots** take `EquipParamGoods` ids. The two ids are 1000 and 900,
  and A9 asserts both are rows of that param with the expected names, so the
  constants cannot drift into another table's range.

The probe's *second* lesson is the sharper one for this feature: its `RuneProbe.cpp`
comment records that an empty-slot search "silently skipped 12 rows in the first
probe, where 'empty' turned out to be 0 in some rows and -1 in others". M8 shows
where that came from — `secondaryItem_01..06` is `0` on the 3000 block and `-1`
on the 2000 block — and that `item_01..10` is uniformly `[100, -1×9]` on all 22
rows. So `FirstEmptySlot`'s `-1` test is correct for the array the supplies use,
and the plan forbids extending it to `secondaryItem_NN`, where it would be wrong.

### E5.3 Six writers of 22 rows, and what each one claims

| Feature | Fields written | Draws | Guard against losing another's write |
| --- | --- | --- | --- |
| row 34 hunter tools | 2 free `item`/`itemNum` pairs | no | `RowHasItem` + `FirstEmptySlot` |
| rune probe | `equip_Accessory01..05` | no | unconditional; disjoint fields |
| row 37 trick weapon | `equip_Wep_Right` | 1 | disjoint field |
| row 38 left weapon | `equip_Wep_Left` | 1 | disjoint field |
| **row 40 supplies** | 2 free `item`/`itemNum` pairs | no | `RowHasItem` + `FirstEmptySlot` |
| **row 40 armour** | `equip_Helm/Armer/Gaunt/Leg` | 4 | disjoint fields |

Five of the six write fields no other touches, so they commute trivially. **The
inventory pair is the only real contention**, and it is bounded by measurement:
nine free slots, four wanted, so both features on leave five spare and no row can
run out. Both use a first-free search rather than a fixed index, and both skip an
item the row already holds, so either order produces the same bytes — which A12
asserts by running the two simulated passes in both orders and comparing.

The residual risk is a *verification* risk rather than a behavioural one: each
sibling verifier asserts "only my fields differ", so a correct combined run reads
as a regression during hardware testing unless the tolerance flags land in the
same milestone as the write. Row 37 hit exactly this and solved it the same way.

### E5.4 Five settings arriving at once in the UI

`settings_ui_verify.py` couples the C++ model to `docs/features/randomizer-settings-ui/spec.md`
§7.1 in both directions: case 3 fails if the model has a setting the spec's table
does not, in the same category and the same order. Five new settings therefore
break it until the spec table, `PROSE_TO_LABEL`, `KIND_TO_COUNT` and the
"seven selection fields" count all move together. That is mechanical, but it is
five coordinated edits across two files and a doc, and doing them in one pass is
what keeps a single failing case from being ambiguous about which of the five is
wrong.

Case 9's picker-alignment band still keys its row regex on `"c\d+"` and so cannot
see any non-creature table — a pre-existing limitation row 037 recorded and did
not fix. The armour widths (607 px worst) are well inside the 791 px the band was
sized for, so nothing is newly unpinned.

### E5.5 Why no gate is the right answer

The three tests `CLAUDE.md` §4 and the planner brief set for a required gate:

* **Could data be corrupted or destroyed?** No. Every write is one value over
  another inside an existing row of a decompressed archive that is written to the
  *output* tree; `data/vanilla/dvdroot_ps4` is read-only input, and the archive is
  never rebuilt or resized. An existing save is untouched by construction —
  `CharaInitParam` is consulted at character creation only.
* **Would a later step make a failure here harder to diagnose?** No. M1's output
  is a generated header pinned by `--check`; M2 adds settings nothing reads, and
  its own verifier proves the config round-trips; M3 and M4 each add a pass whose
  simulated selftest compares whole rows byte for byte. A failure in any of them
  is localised by that milestone's own checks, which run whether or not execution
  stops.
* **Does the next milestone depend on something only the console can establish?**
  No, and this is the difference from rows 37 and 38. Their M3 *was* a probe:
  nothing offline could say whether the game reads `equip_Wep_Right`, and M4's
  content was chosen by the answer. Here the 22 rows this feature writes already
  carry meaningful values in the fields it writes (§E1.1), M4's content is fixed
  in advance, and no branch of the plan depends on an observation.

An optional gate after M3 was considered — "check the vials arrive before
building the armour half" — and rejected: nothing in M4 depends on the answer, so
it would spend a full install-and-play cycle to learn something the final test
learns anyway.

### E5.6 Risks judged low enough not to change the plan

* **A count of 20 being clamped or spilled to storage.** Not decidable offline
  (§E7), and 20 is the value that is correct either way because it *is* the carry
  cap (M11). No mitigation needed.
* **Four pickers making `WEAPONS AND GEAR` thirteen settings long.** The category
  pane scrolls and `ui_scroll_verify.py` pins the band; thirteen rows at 8 visible
  is two pages, which the component already does elsewhere.
* **`RandomizerDefaults` growing 124 bytes.** It is a value type copied by both
  screens and `WorldStore`; the only hazard is a stale object file, which
  `make clean` covers.
* **A seventh feature wanting these 22 rows.** Five slots remain free in the worst
  case. Not this plan's problem, but the number is recorded so the next one does
  not have to re-measure it.

---

## E6. What the spec's appendix claimed

The appendix is a lead, not a finding. Every item was checked.

| Appendix claim | Verdict |
| --- | --- |
| `CharaInitRows.h` has the rows, size and helpers; the clothing fields "belong beside the ones already there" | **Confirmed**, and adopted. They are row shape by the header's own stated test |
| `HunterTools.cpp` is the closest sibling for the supplies half | **Confirmed.** Same rows, same search, same single-byte count. Copied in shape, not extended (§E3) |
| `TrickWeaponGrant.cpp` is the closest sibling for the picker half | **Confirmed for the write**, and **insufficient for the draw.** Its single `uniform_int_distribution` is exactly the implementation that breaks B6 (§E5.1) — the one place the appendix's sibling must not be copied |
| `EnemyRandomizer.cpp` `StepItemData` is where the passes sit | **Confirmed**; ordering table at §E1.2 |
| `ModelPoolSelection.h`'s `DefaultSelected` is the fail-safe polarity needed | **Confirmed.** `false`, as rows 37/38 |
| `ShopLineup.h` relevant only under the rejected coffin option | **Confirmed irrelevant** — and it turned out to supply independent corroboration of the two goods ids (M11) |
| `gen_weapon_table.py` is the generator model, "but see the defect" | **Confirmed both halves.** Model for structure; its `LOT_CATEGORY_WEAPON` and its "tests must agree" guard are both wrong for armour (M5, M4) |
| `fmg.py` "needs one more member name each, spelled as an escape" | **Confirmed**, `防具名.fmg` and `アイテム名.fmg`. Note the two shipped constants in that file are spelled with literal Japanese despite its comment claiming escapes |
| `param_offsets.py` — do not write a second param reader | **Followed.** Every number in §E4 came from it |
| `gen_weapon_table.py`'s category is wrong: 1 is armour, 0 is weapons (98/3 vs 63/3) | **Confirmed**, measured independently at 101/3 and 63/3 over slot hits (M5). Also confirmed that fixing it does not move row 37's table — which the appendix did not establish |
| Armour id decomposes as outfit × 10000 + slot × 1000, verified on all 252 | **Confirmed**, 0 mismatches (M10). Used as an assertion, not as the slot rule |
| The 21 fifth-category rows are `NOT NEEDED` placeholders; filter on the category, not the name | **Confirmed and sharper than stated**: 58 *further* rows inside the wearable categories are also named `NOT NEEDED` (M2), so the advice is not merely tidier — a name filter would be wrong |
| `gen_pool_table.py`'s `RENDERABLE` has no `/`; three names need it | **Confirmed**, and `/` is the only missing character across all 124 names. The atlas already has it; only the fallback does not (M6) |
| `pool_verify.py`'s config cases are exact equalities; §4.7 has the new numbers | **Confirmed as a hazard, and the numbers are wrong.** Measured 1277 / 1328, not 1285 / 1336 (M9) |
| `settings_ui_verify.py` case 5 is a pixel budget | **Confirmed.** All five labels measured (M7) |
| cp1252 console | **Confirmed the hard way.** Every command here needs `PYTHONIOENCODING=utf-8` |
| Caryll runes at goods 1600–1612 are a lead for row 39, not for row 40 | **Not acted on**, per the appendix's own instruction and spec §6 |
| Dead end: no reference equivalent | **Confirmed** (§E1) |
| Dead end: no set name in the text, no set field | **Not re-searched.** D1 removed the need — per-slot pickers have no outfit label to invent |
| Dead end: armour has no `proper*` requirement fields | **Confirmed.** The `EQUIP_PARAM_PROTECTOR_ST` field list has none, against four on the weapon row. `WeaponRequirements.h` is not involved at all |
| Dead end: no `lotItemCategory` 2 anywhere | **Confirmed** — the census (M5) finds categories 0, 1, 4, 8 and 15 only |
| Dead end: secondary-item slots not needed | **Confirmed**, and they are the array whose "empty" is `0` (§E5.2) |
| Dead end: `equip_Arrow`/`equip_Bolt` are not where bullets go | **Not re-measured.** The goods-id route is positively established (M11), so the alternative needed no disproof |

### E6.1 What the spec got wrong

Three corrections, none of which changes what the feature does:

1. **Spec §4.2 and §8 — the two membership tests do *not* agree.** Crown of
   Illusions (250000) and Enlarged Head (430000) are both named *and* obtainable,
   and are excluded on the sex test alone. §8's criterion "every excluded row is
   both unnamed and absent from every shop and every pickup" is unsatisfiable as
   written. The plan replaces it with the measured two-sided equality (M4).
2. **Spec §4.2 — "exactly one named row carries a permanent special effect".**
   All 124 included rows carry `residentSpEffectId2 = 6300`; `residentSpEffectId`
   and `...Id3` are `-1` on all of them. The spec's *conclusion* still holds — the
   value is uniform, so no piece brings a buff another does not — but its premise
   is false, and a verifier written to the premise would fail.
3. **Spec §4.7 — "none of them reaching three pages of 12 visible rows".** The
   instruction-line layout fits **11**, not 12 (`ui_scroll_verify.py`'s
   `Skipped picker` and `Weapon picker` entries), so 33, 26 and 31 rows are three
   pages and 34 is four. Immaterial to the feature; the plan states the real page
   counts so `ui_scroll_verify.py`'s new entries are right first time.

Two smaller discrepancies, recorded so a later reader does not think they are
findings: spec §4.7 quotes `BOSS BEHAVIOUR` at 663 px against 656 measured here,
and §4.1's "1700 rows" is 1700 descriptors over 1699 distinct ids (M8).

---

## E7. Anything that could not be established

* **Whether a granted piece is *owned* as well as worn**, and whether the four
  displaced starting garments remain in the inventory. The param says what the
  character wears; nothing in it says what the character possesses. Spec §2 asserts
  the garments remain owned and spec §8 tests it; the plan neither depends on the
  answer nor pre-empts it (§E3, last row).
* **Whether the engine clamps or spills an inventory count above the carry cap.**
  4,505 shipped slot/row combinations exceed it, so the field accepts such values,
  but nothing says where the excess lands. D2's 20 sidesteps it entirely, and M11
  shows 20 *is* the cap, so the question never arises for this feature.
* **Which of the two origin blocks the game instantiates.** §E1.1 infers the 3000
  block from the empty head field against a vanilla character's bare head, which
  rests on an observation rather than on data. All 22 rows are written either way,
  as the three shipped features do. Spec §8's free pre-build observation settles
  it and answers row 34's open item O2 at the same time.
* **Which piece a given seed draws.** The Python mirror deliberately does not
  predict it: reproducing `std::mt19937`'s stream position across a whole run
  would mean mirroring every earlier pass's consumption. A15 tests the *property*
  B6 states — that the answer does not move when another slot's ticks change — by
  running the simulated pass twice and comparing, which is the part that can fail.
  Determinism across two real runs is a hardware check.
* **Whether the game draws the `/` glyph acceptably at picker scale.** The 8x8
  bitmap is authored by hand; that it is renderable is checkable offline, that it
  is legible is not.
* **Whether any of the 124 pieces is visually broken on a player body.** All 124
  are marked wearable by both sexes and 120 are obtainable in vanilla, so the game
  itself puts them on the player; the four that are not are the starting clothes.
  No offline check can say more.
* **What a mirror proves.** `starting_gear_verify.py` pins the *rules* — the
  derivation, the field offsets, the row-level effect of each pass — against real
  game data. It cannot show that the C++ implements those rules, that the engine
  consults the tables correctly, or that Bloodborne reads any of it. A clean build
  and a passing mirror mean **ready for hardware testing**.
