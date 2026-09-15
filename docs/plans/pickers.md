# Features 9 and 10 — Enemies Included / Bosses Included (the pickers)

**Both are implemented.** This plan was written for feature 9; feature 10 was
built afterwards to the same design and is documented in §7 rather than in its
own file, because it is the same component pointed at a second table.

**Spec reference:** `docs/randomization-feature-spec.md` §3, row 9
(`oopsAll` / `excludeEnemiesBool`).
**Status: implemented, builds clean with no warnings, awaiting hardware test
(§5.3).** All files in §4 are done as specified, plus the new verifier in §5.1
(15/15 selftests passing) and the deviations below.

Deviations from the plan as written:

- **The confirm prompt is full-screen, not a panel over the list.** `Renderer`'s
  entire API is `Clear`, `Present`, `DrawText`, `TextWidth` — there is no filled
  rectangle, so a panel would have left list rows showing through the text.
  Adding `FillRect` to the Platform layer for one modal was not worth it, and a
  question that takes over the screen is harder to answer without reading.
- **The prompt's second line names what is being replaced** (`THIS REPLACES YOUR
  CURRENT 74 OF 82`) rather than the planned generic warning, so the number the
  user spent time building is in front of them at the moment they confirm.
- **`ContainsAny` became a template.** The banned-model lists moved out of the
  two inline `initializer_list`s into named `kBannedM2402` / `kBannedM35` arrays
  so the new precompute and the reroll loops cannot drift apart; the helper had
  to accept both forms.
- **`ui_scroll_verify.py` needed three stale counts corrected** as well as the
  new screen: the two settings lists are 13 rows now, and the progress log's
  worst case is 15 lines rather than 14 (the pool-limited line).
- **`EnemyPoolSelection`'s semantics are verified by a Python mirror**, not a
  C++ unit test. A host clang exists but ships no standard library, so the
  project's standing substitute applies — the same one `ui_scroll_verify.py`
  uses. Stated plainly because it is weaker than a real unit test: it pins the
  rules, not the C++ implementation of them.

Measurements from the finished build: the longest row is exactly 39 characters
of name field (no overflow, no slack), rows render 1215px of 1920 at scale 3, and
the config's worst case is 421 bytes of the 1024-byte buffer.

**Scope, as asked for:** one flat list of every enemy in the randomization pool,
each individually toggleable, plus **Select All** and **Select None**. Global —
**not per-zone**. Enemies only; the boss picker (row 10) is a separate feature
and is not part of this.

---

## 1. What this setting does, in plain terms

`RANDOMIZE ENEMIES` currently draws every replacement from one fixed pool of
everything the game has. This lets you decide what is in that pool.

| | |
|---|---|
| **All ticked** (default) | Exactly today's behaviour — nothing changes |
| **Some unticked** | Those creatures never appear as replacements |
| **One ticked** | Every randomized enemy in the game becomes that creature |

That last row is the "Oops, All Huntsmen" mode the reference tool has, reached
here by using **Select None** and then ticking one.

### The thing to be clear about

**Unticking an enemy does not protect it — it stops it being used.**

Untick Carrion Crow and you are saying *"never turn anything into a Carrion
Crow."* You are **not** saying *"leave the crows where they are."* The crows
already in the world are still placements like any other and will still be
replaced by something else.

This matches the reference tool's own semantics, and it is the one thing about
this feature people will get backwards. Protecting specific enemies *in place*
is a different feature and is not on the list.

### Two things worth knowing

**Unticking is not evenly weighted.** The pool holds one entry per distinct
stat/AI variant, not one per creature, so creatures with many variants are drawn
far more often. Huntsman (Transformed) alone is **12.3%** of every roll, while 30
of the 82 entries are worth 0.3% each. Untick the Huntsman and the run changes
noticeably; untick a dozen rare things and you may not be able to tell.

**A very small selection weakens the size guard.** Replacements are normally
kept to roughly the physical size of what they replace, so nothing ends up stuck
in a corridor. If nothing you leave ticked is small enough for a given spot, the
randomizer gives up after a few thousand tries and places an oversized enemy
anyway. It will not hang or fail — but with only one or two large creatures
ticked, expect some of them wedged in places they do not fit.

---

## 2. What the real data says

Measured against `data/vanilla/dvdroot_ps4` by replaying
`EnemyRandomizer.cpp`'s pool construction exactly.

### 2.1 The list is 82 rows, not 442

| | |
|---|---|
| Pool entries (npc/think/model triples) | **333** |
| Distinct models | **82** |

The picker is **per model** — `c2630` "Huntsman (Transformed)" is one row, not 41.
That matches the reference's own `oopsAll`, which also works on five-character
model IDs, and it turns an unusable 333-row list into a merely long 82-row one.

**Every one of the 82 has a real name** in `tools/data/Characters.json`, so no row
has to show a bare ID.

### 2.2 The existing tool disagrees with the engine — fix this first

`tools/enemy_lookup.py pool` reports **442 entries across 83 models**. The engine
builds **333 across 82**. The tool is wrong, in three ways:

| | Engine | `cmd_pool` |
|---|---|---|
| `c2561` placements | excluded (`EnemyRandomizer.cpp:358`) | included — this is the whole 83rd model |
| `c1110_0000` with `talk != 111010` | excluded (`:357`) | included |
| Duplicate suppression | by **NPC id, per map** | by `(npc, think, model)`, globally |

This matters because the picker's baked list has to *be* the engine's pool. If
the list is generated from the tool, it ships a row for a creature that can never
be drawn, and the counts on screen are wrong. **The tool is what needs fixing**,
and the fix is the same replay this plan's generator uses — so do it once, in one
place, and have both read it.

Incidentally `c2561` is also the only model with no name, which is a small piece
of corroboration that excluding it is correct.

### 2.3 Weighting, measured

| Model | Name | Entries | Share |
|---|---|---|---|
| c2630 | Huntsman (Transformed) | 41 | 12.3% |
| c2700 | Church Servant | 13 | 3.9% |
| c4020 | Enlarged Head Patient | 13 | 3.9% |
| c1051 | Chime Maiden (Light) | 12 | 3.6% |
| c1170 | Carrion Crow | 12 | 3.6% |
| c2620 | Wheelchair Huntsman (Gatling Gun) | 12 | 3.6% |

**Out of scope here, and now written up separately** as
`docs/deferred-ideas.md §2`, which records the full distribution (29 of the
82 models have a single entry; 13 models account for half of all draws) and the
recommended shape: a separate `EVEN ENEMY MIX` toggle rather than a change to
how `RANDOMIZE ENEMIES` already behaves, so existing seeds keep reproducing.

### 2.4 Seven names are ambiguous — rows need the model ID

Seven names are shared by two models each: Abhorrent Beast, Bloodlicker,
Celestial Larvae, Cloaked Beast Patient, Enlarged Head, Enlarged Head Patient,
Lightning Summoner. So a row shows both:

```
C2620 WHEELCHAIR HUNTSMAN (GATLING GUN)   YES
```

At 45 characters that is the longest possible row. At the settings screens' usual
item scale that is 1620px of 1920 — it fits, but only just. The picker uses scale
3 instead (see "The picker screen, concretely" below), where the same row is
**1215px**, leaving room to spare.

### 2.5 The font cannot currently render these names

`Font8x8.cpp` has **space, A-Z and 0-9 and nothing else** — an unknown character
renders as a blank gap. The 82 names between them use `'`, `(`, `)` and `-`:

- `Mergo's Attendant` → `MERGO S ATTENDANT`
- `Huntsman (Transformed)` → `HUNTSMAN  TRANSFORMED `
- `Shark-Giant` → `SHARK GIANT`

**Add the four glyphs** (D4). This is four rows of eight bytes in an existing
table, it is the second time punctuation has been worked around rather than
fixed — `docs/plans/workshop-tools.md` dropped parentheses from a progress
line for the same reason — and every future screen benefits.

---

## 3. Design decisions

**D1 — One row on the settings screens, opening a sub-screen.** 82 rows cannot
live inline. Both screens gain:

```
ENEMIES INCLUDED   82 OF 82
```

X opens the picker. The count is the whole status at a glance, and `82 OF 82`
reads unmistakably as "nothing filtered".

**D2 — The picker is an internal mode, not a new `ScreenId`.** `Application.cpp`
constructs screens fresh on every switch, so navigating away and back would
destroy the wizard's in-progress toggles. That is exactly why the title-ID editor
is an internal mode of `SetupDefaultsScreen`, and the same reasoning applies
here.

To avoid writing it twice, the picker goes in `UI/EnemyPicker.h/.cpp` as a small
reusable component owning its own cursor and scroll, with `Update(input,
selection)` and `Draw(renderer)`. Each hosting screen owns one and delegates to
it while in that mode.

**D3 — It appears in both Setup Defaults and the Enable wizard**, like every
other setting. Setup Defaults persists the selection; the wizard starts from it
and can change it for one run without persisting, which is exactly how the other
toggles already behave.

**D4 — Add `'`, `(`, `)` and `-` to the 8x8 font.** See §2.5. Non-negotiable for
this feature; a picker whose labels read `MERGO S ATTENDANT` is not finished.

**D5 — Controls.**

| Input | Action |
|---|---|
| Up / Down | Move one row (wraps) |
| **R1 / L1** | **Page forward / back** |
| X | Toggle the highlighted enemy |
| **Square** | **Select all** |
| **Triangle** | **Select none** |
| O | Back to the settings screen |
| Left / Right | Also page, as a fallback — see below |

**L1 and R1 are safe to use, and this is now established rather than assumed.**
`Input.cpp` currently maps only the four face buttons and calls its `OPTIONS = 9`
an "educated guess", warning that the standard PS4 HID order cannot be trusted
because the d-pad turned out to sit at 13/14. The SDK's own header settles it —
`samples/SDL2/SDL2/Game.h`:

```
CROSS=0, CIRCLE=1, SQUARE=2, TRIANGLE=3,
L1=4, R1=5, OPTIONS=9, L3=11, R3=12,
UP=13, DOWN=14, LEFT=15, RIGHT=16,
TOUCH_PAD=17, L2=18, R2=19
```

`UP=13, DOWN=14` is exactly what this project's own hardware logs recorded, so
the enum is corroborated, not just quoted. **L1=4 and R1=5 are correct**, and so
is the `OPTIONS` guess — worth promoting that comment from a guess to a fact
while we are in there.

D-pad left/right does the same paging, kept as a belt-and-braces fallback: the
d-pad is confirmed working through `SDL_JOYHATMOTION`, so the picker stays fully
usable even if L1/R1 misbehave on some pad. It costs one extra `if`.

**D6 — Select All / Select None are buttons, not list rows.** Two action rows
pinned at the top of a scrolling list either scroll away or need special-casing
out of the scroll window, and both are worse than a footer hint. Square and
Triangle are already plumbed in `ButtonEdges` and have been unused since d-pad
navigation replaced them in M4, and being one press from anywhere in an 82-row
list is the point.

### The picker screen, concretely

```
                      ENEMIES INCLUDED                        <- scale 5, y=120
                    74 OF 82     PAGE 3 OF 7                  <- scale 3, y=185

                          MORE ABOVE                          <- y=234

        C1170 CARRION CROW                        YES         <- list starts y=280
        C1171 CARRION CROW (DOG FACE)             NO
        C2330 CAINHURST SERVANT                   YES
        C1220 CAINHURST GARGOYLE                  YES
        C2520 CELESTIAL LARVAE                    YES
        C2521 CELESTIAL LARVAE                    NO
      > C2700 CHURCH SERVANT                      YES <       <- highlighted row
        C1050 CHIME MAIDEN                        YES
        C1051 CHIME MAIDEN (LIGHT)                YES
        C1090 CLOAKED BEAST PATIENT               YES
        C4140 CLOAKED BEAST PATIENT               YES
        C1290 CLOAKED BEAST PATIENT (FEMALE)      YES         <- 12th row, y=852

                          MORE BELOW                          <- y=898

         UP DOWN MOVE   L1 R1 PAGE   X TOGGLE                 <- scale 3
       SQUARE ALL   TRIANGLE NONE   O BACK
```

**Twelve rows visible, so seven pages rather than fourteen.** The settings
screens use item scale 4 at 90px spacing and fit 6 rows; the picker uses **scale
3 at 52px spacing** in the band `firstY=280 … bottomLimit=900`, which
`VisibleRowCount` turns into 12. Scale 3 is already proven on this screen — it is
what the progress log uses. Halving the page count is worth more here than
matching the settings screens' type size.

**Columns line up without any new drawing code.** Every row is padded to exactly
45 characters — `%-39s   %-3s`, where 39 is the longest `MODEL NAME` string
(`C2620 WHEELCHAIR HUNTSMAN (GATLING GUN)`) — so all rows are the same pixel
width and the existing centred `DrawScrollableList` puts the YES/NO in a fixed
column for free. Left-aligning the list would have meant a new draw path; this is
a format string. At scale 3 a full-width row is 1215px of 1920.

**The count is in the heading, not in the list.** `74 OF 82` updates as you
toggle, so the effect of Square/Triangle is visible immediately and there is no
doubt about what state you left the screen in. `PAGE 3 OF 7` is there because
`MORE ABOVE` / `MORE BELOW` tell you there is more but not how much.

**Sorted alphabetically by name.** People look for a creature by name, so
alphabetical is the order that makes a specific enemy findable. It also puts the
seven ambiguous pairs next to each other — both Celestial Larvae adjacent, both
Cloaked Beast Patients adjacent — which is exactly when you need the model ID
next to the name to tell them apart.

*The alternative is model-ID order*, which groups variants of one creature
(`c2630`/`c2631`/`c2632` — Huntsman, Large Huntsman, Forest Huntsman) together
and is better for "get rid of all the huntsmen". Alphabetical splits those across
F, H and L. I do not think that outweighs being able to find things, but it is a
one-line change to the generator if it plays worse than it reads.

**D7 — Default is everything ticked.** The no-op, so an existing `defaults.cfg`
behaves exactly as it does today. A missing or malformed key also reads as
all-ticked (D9).

**D8 — Zero ticked is allowed in the picker but blocked at commit.** Select None
followed by ticking three things is the natural way to build a small list, so the
picker must be allowed to pass through zero. But an empty pool makes
`StepBuildPool` fail the run (`EnemyRandomizer.cpp:374`), and a failed run is a
bad way to learn this.

So `StartCommit` checks it: if `RANDOMIZE ENEMIES` is on and nothing is ticked,
it refuses with `SELECT AT LEAST ONE ENEMY` instead of starting. If `RANDOMIZE
ENEMIES` is off, the selection is irrelevant and is not checked.

**D9 — Persist as a fixed-length 0/1 string.** `enemies_included=0110111…`, 82
characters, one per model in the generated table's order.

- Compact enough for the existing config: worst case goes from 320 bytes to
  ~414 of `char buf[512]`.
- **But that leaves only ~98 bytes of headroom**, and the spec has 21 more
  settings queued. Bump the buffer to 1024 at the same time — one line, and the
  existing clamp (added by the workshop-tools work) keeps it safe either way.
- **A length mismatch means ignore it and default to all-ticked.** If the pool
  table is ever regenerated with a different number of models, an old string
  would silently map to the wrong creatures. Length is a cheap, sufficient guard
  against exactly that.

Rejected: a list of excluded model IDs (`c2630,c4040,…`). Readable, but 82
exclusions is ~500 bytes on its own — the one case that most needs to work is
the one that overflows.

**D10 — Bake the list as a generated header**, `Randomizer/EnemyPoolTable.h`,
holding the 82 model IDs and display names in a frozen order. Same pattern as
`ModelSizeTable.h` and `NpcScalingTable.h`, and the same reason: the app never
parses `Characters.json` or rebuilds the pool to draw a menu.

The generator is a new tool command, and §5.1's check is what stops the table
drifting from the engine.

**D11 — Select All and Select None both ask first.**

```
                  DISABLE ALL 82 ENEMIES

            THIS CLEARS EVERY TICK ON THE LIST

                X CONFIRM        O CANCEL
```

A third mode inside the picker: `Draw` paints this over the list, `Update`
accepts only X and O. Around fifteen lines, and it reuses `DrawCenteredLabel`.

**I have extended this to Square as well, which was not asked for.** The reason:
the thing being protected is the hand-built list, and Select All destroys it just
as completely as Select None does. Confirming one and not the other would protect
half the cases and teach the habit of pressing the other one without reading.
Square's prompt reads `ENABLE ALL 82 ENEMIES`. Say if you would rather Square
stayed instant — it is one `if`.

The prompt names the count so it also serves as a "you have 8 ticked, are you
sure" warning rather than a generic are-you-sure.

**D12 — Every enemy disabled: what actually happens.**

Short answer: **it cannot crash today, and it could never have produced
enemy-free maps either.** Traced rather than assumed.

*Why empty maps are not a thing.* The randomizer **replaces** enemies in place —
it rewrites a placement's model/NpcParam/ThinkParam. It never removes a
placement. An empty pool means "no replacement available", not "no enemy", so
every placement simply keeps the enemy it already had. Emptying the picker can
never thin out the world; the most it can ever do is leave it vanilla.

*Why it does not crash.* Two guards already exist, and between them they cover
every path:

1. `StepBuildPool` (`EnemyRandomizer.cpp:374`) fails the run outright if the pool
   is empty **and** `randomizeEnemies` is on.
2. All five `DrawCandidate()` call sites (`:575–603`) sit inside
   `if (options.randomizeEnemies)` (`:561`).

So an empty pool with enemies on is stopped before anything draws, and with
enemies off nothing ever draws. The boss randomizer is unaffected either way — it
builds its own `BossPool` from boss placements and never reads this one.

*What would break if those guards were removed.* `DrawCandidate()` calls
`RandInt(0, (int)pool.size() - 1)`, which on an empty pool is `RandInt(0, -1)` →
`std::uniform_int_distribution<int> d(0, -1)`. That violates the distribution's
`a <= b` precondition, so it is undefined behaviour, and the garbage it returns
then indexes an empty vector. On a console that is a crash, not a graceful
failure. **The guards are load-bearing — do not relax them**, and D8's
commit-time check is a third layer in front of both, not a replacement for them.

*So why block it at commit at all (D8)?* Because guard 1 fails the run **after**
the mirror phase has already copied most of the game into the AFR folder. The
user waits, watches it fail, and is left with a stray half-built tree. Refusing
at the confirm screen with `SELECT AT LEAST ONE ENEMY` costs nothing and says
what to do.

*The one alternative worth naming:* treat an empty pool as "leave every enemy
alone" — a silent no-op instead of a refusal. It is coherent, and it is what the
data would do if nothing stopped it. Rejected because it is indistinguishable
from the feature being broken: the run reports success, takes the usual twenty
seconds, and the world is unchanged.

---

## 4. Implementation

### 4.1 Generated data

| File | Change |
|---|---|
| `tools/enemy_lookup.py` | **Fix `cmd_pool`** to match the engine (§2.2): apply the `c2561` and `c1110_0000` rules and dedupe by NPC per map. One shared `engine_pool(root)` function, used by both the fixed command and the generator below |
| `tools/gen_enemy_pool_table.py` *(new)* | Emits `EnemyPoolTable.h` from the vanilla tree plus `data/Characters.json`. Uppercases names and asserts every character is renderable by the font after D4 |
| `src/Randomizer/EnemyPoolTable.h` *(new, generated)* | 82 `{ "c2630", "HUNTSMAN (TRANSFORMED)" }` pairs in frozen order, plus `kEnemyPoolModelCount` |

### 4.2 Font

| File | Change |
|---|---|
| `src/Platform/Font8x8.cpp` | Four glyphs: `'`, `(`, `)`, `-` (D4) |

### 4.3 Selection type and settings chain

| File | Change |
|---|---|
| `src/Randomizer/EnemyPoolSelection.h` *(new)* | A fixed-size `bool[kEnemyPoolModelCount]` wrapper with `AllOn()`, `AllOff()`, `Toggle(i)`, `CountOn()`, `IsModelEnabled(const std::string&)`, and encode/decode for the D9 string. Header-only; no SDL, no filesystem |
| `src/Randomizer/RandomizerDefaults.h` | Add an `EnemyPoolSelection enemiesIncluded;` defaulting to all-on |
| `src/Randomizer/RandomizerDefaultsStore.cpp` | Read/write `enemies_included`; length mismatch → leave the default (D9). Buffer `512` → `1024` |

### 4.4 Engine

| File | Change |
|---|---|
| `src/Randomizer/EnemyRandomizer.h` | Add `EnemyPoolSelection enemiesIncluded;` to `EnemyRandomizerOptions`, defaulting to all-on so every existing call site is unaffected |
| `src/Randomizer/EnemyRandomizer.cpp` | **One line** in the pool-contribution loop (after the existing `modelName.empty()` check, ~`:360`): `if (!options.enemiesIncluded.IsModelEnabled(modelName)) continue;`. Plus the §4.5 guard |

That single `continue` is the whole feature, engine-side. Everything downstream —
the shuffle, the dedupe, the size gate, the per-zone chance — already works
against whatever the pool contains.

### 4.5 One guard worth adding while we are here

`m24_02` and `m35` reroll the candidate while its model is in a banned list,
capped at 100000 tries (`EnemyRandomizer.cpp:593,600`). With the full pool that
loop exits immediately. With a selection of only banned models — ticking just
`c2630` is enough, since it is banned in both — it spins the full 100000 draws
**per placement**.

Not a hang, and not even slow enough to notice on the low hundreds of placements
involved, but it is pointless work in the exact configuration the picker makes
easy to reach. Precompute once per map whether the filtered pool contains any
non-banned model, and skip the loop entirely when it does not.

The size-gate loop (`tries < 30000`) needs no such guard — it already terminates
and falls back to placing the oversized candidate, which is the documented
degradation in §1.

### 4.6 UI

| File | Change |
|---|---|
| `src/Platform/Input.h/.cpp` | Add `l1` / `r1` to `ButtonEdges` and map indices **4 / 5**. Transcribe the rest of the SDK enum while there and promote the `OPTIONS = 9` comment from "educated guess" to confirmed (D5) |
| `src/UI/EnemyPicker.h/.cpp` *(new)* | The picker: cursor, scroll, paging, D5 controls, row formatting, heading counter, footer, and the D11 confirm mode. Reuses `ListLayout` / `VisibleRowCount` / `ScrollToShow` / `DrawScrollHints` / `DrawScrollableList` / `DrawCenteredLabel` from `Controls.h` — no new scrolling or drawing code |
| `tools/ui_scroll_verify.py` | Add the picker as a fourth screen (`firstY=280, spacing=52, bottomLimit=900, scale 3, 82 rows`) and assert it yields 12 visible rows with every row and both hints clearing the heading, footer and screen edges. **Run this before building the screen** — it is how the layout numbers above get checked without a console |
| `src/UI/SetupDefaultsScreen.h/.cpp` | `kItemCount` 12 → 13; `kEnemiesIncludedRow = 12`; a third `Mode`; delegate while in it; summary row in `DrawList` |
| `src/UI/EnableWizardScreen.h/.cpp` | `kSaveDataRowCount` 12 → 13; `kEnemiesIncludedRow = 12`; a `Step::EnemyPicker`; delegate; summary row in `DrawSaveData` and `DrawConfirm`; pass `options.enemiesIncluded` in `StartCommit`; the D8 refusal; a `FinishCommit` line when the selection is not all-on |

Both rows go last again, so no existing row index moves.

---

## 5. Verification

### 5.1 The check that matters — the baked table must equal the engine's pool

A new `tools/enemy_pool_verify.py`:

```
python enemy_pool_verify.py table  <vanilla_dvdroot>   # baked table vs. replayed pool
python enemy_pool_verify.py filter <vanilla_dvdroot> <output_dvdroot> --included <ids>
python enemy_pool_verify.py selftest
```

`table` rebuilds the pool from the vanilla tree and asserts it matches
`EnemyPoolTable.h` exactly — same models, same count, same order. **This is the
one that stops the silent failure**: if an exclusion rule changes and the table
is not regenerated, the config string silently maps to the wrong creatures, and
nothing else in the system would notice.

`filter` is the real end-to-end check: given an output tree and the set that was
ticked, assert that **no placement anywhere was changed to a model outside that
set**. That is the feature's actual contract, and it is checkable from the output
alone.

### 5.2 Test matrix

| Run | Expect |
|---|---|
| All 82 ticked | **Byte-identical to a pre-change run of the same seed.** The regression guard: the filter must not perturb the RNG stream when it excludes nothing |
| A few unticked | `filter` passes; the unticked models appear nowhere as replacements; placements *of* those models are still replaced (§1) |
| One ticked (Oops-All) | Every randomized placement is that model; run completes; no hang in `m24_02`/`m35` (§4.5) |
| One ticked, large model | Completes, with oversized placements — the documented degradation, not a failure |
| Zero ticked, enemies ON | Commit refused with `SELECT AT LEAST ONE ENEMY`; no run starts (D8). Confirms the third guard layer in front of D12's two |
| Select All / Select None | Each prompts; O cancels leaving the selection untouched; X applies (D11) |
| Zero ticked, enemies OFF | Commit proceeds normally; selection ignored |
| Selection changed, bosses/treasure on | Boss and treasure output byte-identical across the two runs — this must not touch them |

### 5.3 Hardware check

Navigation is the risk, not the randomization. 82 rows through a 6-row window:
confirm the cursor stays visible throughout, paging lands where expected at both
ends, Triangle/Square do what the footer says, and the summary row count matches
what was ticked. Then one Oops-All run, which is the most visible possible proof
that the filter works.

---

## 6. Resolved questions

All five answered. Kept as a record of what was decided and why.

**Q1 — Is 82 rows of scrolling tolerable? RESOLVED: yes, for now.** Seven pages
at 12 rows a page, with L1/R1 paging and one-press select-all/none. Ship the flat
list. If it drags in practice, jump-to-letter is the cheapest next step and the
alphabetical order already sets it up.

**Q2 — Confirm before wiping a selection? RESOLVED: yes, a confirmation dialog.**
See D11 for the design.

**Q3 — Summary row: count or names? RESOLVED: the count.** `ENEMIES INCLUDED
74 OF 82`, for predictable width.

**Q4 — What happens with every enemy disabled? RESOLVED: it cannot crash, and it
cannot produce empty maps either.** See D12 — this turned out to be worth writing
up properly rather than answering in a line.

**Q5 — Flatten the pool weighting? RESOLVED: deferred, written up separately.**
`docs/deferred-ideas.md §2` records the measurements and the recommended
shape: a separate `EVEN ENEMY MIX` toggle rather than a change to how
`RANDOMIZE ENEMIES` already behaves, so existing seeds keep reproducing.


---

## 7. Feature 10 — the boss picker

Built after the enemy picker, to be identical in use. No new design decisions:
D1-D12 all carry over unchanged.

### 7.1 What is different

| | Enemies | Bosses |
|---|---|---|
| Rows | 82 | **17** |
| Pages at 12 visible | 7 | **2** |
| Pool entries | 333 | 18 |
| Longest label | 39 chars | **41 chars** |
| Config key | `enemies_included` | `bosses_included` |
| Filter site | `EnemyRandomizer.cpp` pool loop | `BossRandomizer.cpp` `CollectBossCandidates` |

### 7.2 Three things the boss list forced

**A comma glyph.** Four boss names contain one — *Ebrietas, Daughter of the
Cosmos*; *Laurence, the First Vicar*; *Ludwig, the Holy Blade*; *Rom, the
Vacuous Spider*. The generator's font check caught it and refused to emit the
table, which is exactly what that check exists for. `Font8x8.cpp` now has five
punctuation glyphs.

**A measured field width instead of a constant.** The enemy picker hardcoded a
39-character name field, which is its own longest label. The boss list's longest
is 41 (*Lady Maria of the Astral Clocktower*), so a shared constant would have
silently clipped it. `ModelPicker` now measures the widest label in whichever
table it is handed. Rows are still padded to a common width so the centred draw
puts YES/NO in a fixed column.

**A sharper empty-pool guard.** The enemy pool fails cleanly when empty. The
boss pool is *drained* as arenas are assigned and refilled from a model-distinct
copy when it empties — so with nothing selected both are empty, the refill never
helps, and the next draw calls `RandIndex(rng, 0)`, i.e.
`uniform_int_distribution(0, -1)`. That is undefined behaviour, not a failure.
`BossRandomizer.cpp`'s own header already warned about it. The commit-time
refusal (D8) is therefore load-bearing here in a way it is not for enemies.

### 7.3 What the pickers share

`ModelPicker` (renamed from `EnemyPicker`) takes a table pointer, a count and a
flag array, so one implementation drives both. `ModelPoolSelection<N>` does the
same for the selection logic, with `EnemyPoolSelection` and `BossPoolSelection`
as typedefs. `tools/gen_pool_table.py` emits both headers and
`tools/pool_verify.py` checks both, running every invariant over each.

### 7.4 The two pools share models, but not identities

`c2090` (Blood Starved Beast) and `c2710` (Father Gascoigne) appear in both
tables. That is not a leak between the pools — each creature genuinely exists
twice in the game, with different stats and AI:

| Model | Enemy pool holds | Boss pool holds |
|---|---|---|
| `c2090` | `209010` — the Hunter's Nightmare enemy (`c2090_0000`, entity 3400650) | `209000` — the Old Yharnam boss (`c2090_0003`, entity 2300800) |
| `c2710` | `271010` — the NPC (`c2710_0001`, entity 2410158) | `271000` — the boss (`c2710_0000`, entity 2410810) |

The reference's boss-name list keys on the **placement suffix**, which is what
separates them: `c2710_0000` is boss-named and therefore excluded from the enemy
pool, while `c2710_0001` is not boss-named and therefore never reaches the boss
pool. The identity sets are disjoint — verified.

So a picker row named "Father Gascoigne" means a different creature depending on
which list it is in, and unticking one has no effect on the other. That is
correct behaviour rather than a wart, but it is worth knowing before someone
reports it as one.

`pool_verify.py` pins both halves: the model overlap is exactly those two, and
no identity is shared. An earlier version of that check asserted the model lists
were disjoint — a guess, which failed on first run and was replaced with the
measurement.

### 7.5 What is NOT in scope

`insertBossesBool` — "Bosses Can Replace Enemies", feature 13 — is still
unimplemented, and nothing here touches it. The boss picker filters
`BossPool`, which feeds only `AssignBossesInMap`, `AddTheRestInMap` and the
Orphan phase-one fixup: which boss stands in which *boss arena*, and nothing
else. When feature 13 is built it will draw from the enemy pool, which is a
separate list with a separate picker.
