# Plan 032 — Bypassed Enemies (`ENEMIES SKIPPED`)

**Status: QUESTIONS ANSWERED — awaiting developer approval.** Q1–Q5 were
answered on 2026-09-18 (§9 P12–P16, with P17 recording a spec gap the developer
resolved in the same pass). The stage D review response of 2026-09-19 took three
new planning decisions (§9 P18–P20) and opened one new question, **Q6**, which
the developer answered the same day — recorded as **§9 P21**, and the baseline
tree it asked about now exists. **P18 was also put to the developer and
accepted.** §8 is kept as an empty heading so the numbering is stable; §9 is the
only place a decision lives.

**Spec:** `docs/features/032-bypassed-enemies/spec.md` — **APPROVED** (developer,
2026-09-18). Six binding decisions, §10 D1–D6 (D6 added 2026-09-18 as a
post-approval amendment; the spec keeps its `APPROVED` status because D6 added
rather than altered).

**Backlog row:** `docs/randomization-feature-spec.md` §3, row **32**.

**Plan review:** `docs/features/032-bypassed-enemies/plan-review.md` — verdict
**CHANGES REQUESTED**, 2026-09-19. This revision answers findings 1–7; finding 8
is about `log.md`, which this document does not own. §10 lists what each finding
changed and whether it was a correction or a decision. **One thing the review
raised is not answered here because it is not the planner's to answer:** whether
`ENEMIES SKIPPED` should also filter the boss pool, which bears on spec §10 D4's
"only case" claim. §5.10 records the measurement, plans for the spec as
approved, and says what would change if the spec is refined.

Every number in this document was measured against `data/vanilla/dvdroot_ps4`
with the tools in `app/tools/`, or read out of the working tree (which has
uncommitted modifications — line numbers below are working-tree lines, not
`HEAD`). §2.6 lists what the spec's appendix claimed and what the trace found.
Every number the review challenged was **re-measured here rather than taken from
the review**; where the two agree the plan now carries the review's number and
says so, and where they do not, §10 records the difference.

---

## 1. What are we building?

A second creature checklist, `ENEMIES SKIPPED`, nothing ticked by default, on
both settings screens. A ticked creature leaves the run in both directions: its
own placements are never overwritten, and it never appears in the replacement
pool. Plus two things the spec attaches to it that are not list work at all:

* **D4 — a starved selection must finish, not error.** When every creature
  ticked in `ENEMIES INCLUDED` is also ticked here, the pool is empty. Today
  that ends the run in `EnemyRandomizer.cpp:419`'s `Fail`, *after* the mirror
  phase has copied most of the game into the AFR folder. D4 says the run must
  proceed instead, drawing from `ENEMIES INCLUDED` while the skipped placements
  stay frozen.
* **The empty-pool error does a second job.** It is also the only hard failure
  the enemy path has for an unusable `VanillaSource`. Removing it without
  separating the two causes turns a broken installation into a run that reports
  success having randomized nothing. This is blocking finding 2.1 of
  `docs/features/016-unchanged-bell-maidens/plan-review.md`, it is why the same
  change was deferred out of row 16 on 2026-09-16, and spec §7 and §8 assertion
  8 now make the separation a requirement here.

* **D1 — `UNCHANGED BELL MAIDENS` retires**, from both screens, from
  `defaults.cfg`, and from the engine, with no migration of a saved value. Its
  effect survives as two ticks on the new list, Yahar'gul exception included.

The technical shape, in one paragraph. A new generated table
`EnemySkipTable.h` (85 rows, own frozen order, **not** derived from the shipped
82-row `EnemyPoolTable.h`) pairs with a new selection type whose defaults are
the mirror image of the picker's — constructs to *nothing selected*, unknown
model reads as *not skipped*, a wrong-length saved value leaves *nothing
skipped*. The engine builds **two** pools per run: one with the skip list
applied and one without. It normally uses the first; if the first is empty and
the second is not, it uses the second and records that the skip list's pool half
yielded (D4). If **neither** has anything — because no map could be read — the
run fails, with a message that names the vanilla source rather than the
selection. The skip test joins the existing exclusion test at the two call sites
that already exist (`:375` pool contribution, `:629` placement decision), before
the zone roll, so a skipped placement consumes no randomness and the seed stream
shifts exactly as spec §2 describes.

Sequenced as **two milestones** (§7), with **one thing that had to happen before
either of them**: a baseline output tree from the build in hand, captured before
the first code change of milestone 1, because that is the only build that can
produce one and the relevant sources are uncommitted (§6.4 step 0, §9 P19).
**That tree now exists** — the developer captured it on 2026-09-19 at
`data/runs/20260919-Enemies Only` (§9 P21). One thing about it is still
outstanding and blocks the first code change: **its seed is recorded nowhere and
must be written into §6.4 step 0.** Then the failure-path work, using row 16's
flag as the reachable test case, then the list on top of it. Rationale in §7.

---

## 2. What already exists?

### 2.1 The settings chain to copy

`enemiesIncluded` (row 9, `docs/plans/pickers.md`) is the exact template — a
drill-in list, persisted as one character per row, per-run copy in the wizard,
working copy in Setup Defaults:

| Layer | File | What it holds |
|---|---|---|
| Randomizer | `app/src/Randomizer/EnemyPoolTable.h` | the generated 82-row table + `kEnemyPoolModelCount` |
| Randomizer | `app/src/Randomizer/ModelPoolSelection.h` | the selection template, `Encode`/`Decode`/`IsModelEnabled` |
| Randomizer | `app/src/Randomizer/EnemyPoolSelection.h` | binds the sizes to typedefs |
| Randomizer | `app/src/Randomizer/RandomizerDefaults.h:95` | the persisted default |
| Randomizer | `app/src/Randomizer/RandomizerDefaultsStore.cpp:71-74, 103, 119` | the `enemies_included` key, load and save |
| Randomizer | `app/src/Randomizer/EnemyRandomizer.h:89` | the per-run option |
| Randomizer | `app/src/Randomizer/EnemyRandomizer.cpp:389` | the single engine consultation |
| UI | `app/src/UI/ModelPicker.h/.cpp` | the shared drill-in component |
| UI | `app/src/UI/SetupDefaultsScreen.h:58` / `.cpp:41, 53, 144, 196, 229` | row constant, picker `Update` delegation, `EnemiesIncludedText()`, drill-in branch, picker `Draw`, list entry |
| UI | `app/src/UI/EnableWizardScreen.h:118` / `.cpp:44, 97, 271, 365-366, 539, 675, 731, 762` | member; row constant, ctor init, X branch, picker `Update` step, option assignment, `DrawSaveData` entry, picker `Draw`, `DrawConfirm` entry |

`unchangedBellMaidens` (row 16) is the same chain for a boolean, and is the
thing being pulled back out. Every one of its sites is listed in §4.

### 2.2 The engine, and where the two halves land

`app/src/Randomizer/EnemyRandomizer.cpp` consults `IsExcludedEnemyName` in
exactly two places, and they are the feature's two halves:

* **`:375`**, inside `StepReadMap`'s contribution loop — an excluded placement
  never becomes a pool candidate.
* **`:629`**, inside `StepWriteMap` — an excluded placement is `continue`d
  before the zone roll is drawn.

`:627-637` is the whole placement decision today:

    bool forced = isM28 && IsM28ForcedMaidenName(name);
    if (!forced && IsExcludedEnemyName(name, options.unchangedBellMaidens)) continue;
    int roll = RandInt(0, 100);
    if (!forced && roll >= lm.zoneChance) continue;

Two properties of that shape are load-bearing here and must not be disturbed:

* the exclusion `continue`s **before** `RandInt` is called, so an excluded
  placement draws nothing. Adding the skip test alongside it is what makes spec
  §2's "the seed's meaning changes" true, and what keeps a no-op selection
  byte-identical to today;
* `forced` bypasses the exclusion. Spec §4 F5 requires that to keep applying, so
  the skip test must sit inside the same `!forced &&`.

`StepBuildPool` (`:399-485`) is where §4 F10 and §7 land. The `Fail` at `:418`
runs after `StepMirror` has copied all six folders — confirmed on real hardware:
`data/runs/Error Log - Chime Maidens/live.log:26394-26400` shows the six
`mirroring ...` lines and then the failure, which is the developer's own
encounter with this.

`DrawCandidate()` (`:237-241`) is the only function that indexes `pool` by a
random index, and `RandInt(0, -1)` is undefined behaviour — this is why the
`Fail` exists at all and why §3.3's design never lets an empty pool reach it.

### 2.3 The picker component, and what it cannot express yet

`ModelPicker::Draw` takes **one** heading string and has no slot for a second
line (`ModelPicker.cpp:110-147`). Its geometry is one shared constant
`kPickerLayout = { 280, 52, 900 }` (`:20`) used by `Draw` *and* by `Update` via
`VisibleRowCount`. Its vocabulary is written for an inclusion list throughout:
`YES`/`NO` in the flag column (`:137`), `ENABLE ALL n` / `DISABLE ALL n` in the
confirm prompt (`:158-159`), and `SQUARE ALL   TRIANGLE NONE   O BACK` in the
footer (`:146`). Spec §6 puts changing the two shipped pickers' wording and
layout out of scope, so the component has to become parameterisable rather than
re-worded — §3.5.

Text geometry, measured from the code rather than estimated:
`Font8x8.cpp:68` `kAdvance = 9`, so one character at scale 3 is **27px**; the
screen is 1920 wide, giving **71 characters** on a line and 1215px for the
current 45-character picker row. `DrawCenteredLabel` computes
`x = (1920 - width)/2` with no clamp (`Controls.cpp:5-8`), so an over-long
string runs off both edges rather than wrapping.

**The font has a character set as well as a width, and it is the narrower
constraint.** `Font8x8.cpp`'s glyph table is **42 entries**: space, `A`–`Z`,
`0`–`9` and exactly five punctuation marks — `'`, `(`, `)`, `-`, `,`. There is
**no lowercase and no `:`**. `FindGlyph` (`:70-74`) returns `nullptr` for
anything else, `DrawChar8x8` (`:77-78`) returns immediately on that, and
`DrawText8x8` (`:95-98`) advances the cursor anyway — so an unrenderable
character is a **blank column of full width**, not a missing one and not an
error. Counted in the working tree: **12 `Fail()` call sites** in
`EnemyRandomizer.cpp`, every one of them with a lowercase message, and the
progress line that shows them (`EnableWizardScreen.cpp:619`) prefixes them with
`"ENEMY RANDOMIZATION FAILED: "`, whose `:` is also blank. This is a live defect
in today's build, not a hypothetical, and §3.4 is where this feature deals with
it. Found by the stage D review (finding 1); re-verified here by reading the
glyph table and both draw functions.

### 2.4 The verification tools that already exist

* **`app/tools/enemy_lookup.py`** — parses the exclusion headers directly, so it
  cannot drift from the engine. `engine_pool()` is the documented single source
  of truth for the contribution loop ("Do not reimplement it"), `randomizes()`
  mirrors the four-line placement decision, `exclusion_reason()` mirrors
  `IsExcludedEnemyName`. All three need one new parameter; none needs rewriting.
* **`app/tools/pool_verify.py`** — owns the `selftest` harness (currently
  **43/43 passing**), the `table` check that pins each baked table against a
  recomputed pool, and `filter`, which asserts from an output tree that no
  placement was changed to a non-selected model. `table` generalises to a third
  table for free; `filter` is the shape assertion 2 needs.
* **`app/tools/gen_pool_table.py`** — the generator, with the order-is-load-
  bearing warning in its header. Adding a third `Kind` is a data change, not a
  code change. It already owns the right predicate for §2.3's font constraint:
  `RENDERABLE` (`:34`) is exactly `Font8x8.cpp`'s 42-character set, verified
  character for character against the glyph table, and `display_name()`
  (`:37-45`) raises rather than emits when a name falls outside it. §6.2 reuses
  that set for the new user-visible strings instead of restating it.
* **`app/tools/ui_scroll_verify.py`** — the geometry mirror. Currently **PASS**,
  with the two pickers at 12 visible rows.
* **`app/tools/boss_verify.py`** — supplies the MSBB reader. **Do not reuse its
  `load_scaling_ids`**: at `:403` it looks for
  `<repo>/PS4/bbrandomizer/src/Randomizer/NpcScalingTable.h`, a path that
  predates the repository layout, and returns an **empty set** on `OSError`
  without saying so. Verified by running it. Pre-existing, out of scope, noted
  here so nothing in this feature builds on it (§5.6).

### 2.5 Related plans and prior work

* `docs/plans/pickers.md` — the house design for a drill-in list; D12 is the
  commit-time refusal for an empty selection that D5 keeps untouched.
* `docs/features/016-unchanged-bell-maidens/` — `plan.md` §5.2/§9.3 designed a
  non-failing empty pool (the reference's "randomize nothing", not D4's
  "randomize from the included list"), `plan-review.md` findings 2.1 and 2.2 are
  why it was not shipped, `log.md` (2026-09-16) records the deferral. The
  phase-order analysis and the UB hazard carry over verbatim; the chosen
  *behaviour* does not.
* `docs/enemy-exclusion-history.md` — governs the fixed list, and is the source
  for what `c1130_0000` actually is (§5.5).

### 2.6 What the spec's appendix claimed, and what the trace found

Every pointer in the spec's appendix was checked against the working tree.

| Appendix claim | Verdict |
|---|---|
| `EnemyExclusionList.h` holds `IsExcludedEnemyName` and the row-16/forced lists | **Confirmed** (`:117`, `:81`, `:101`) |
| `EnemyRandomizer.cpp` has exactly two call sites, ~375 and ~629 | **Confirmed exactly** — `:375` and `:629` |
| `ModelPoolSelection.h`'s ctor, fallback and `Decode` are all "enabled" | **Confirmed** — `:34` `EnableAll()`, `:61` `return true`, `:79` returns false and leaves the object untouched |
| `BossParamScaling` runs outside the enemy branch and lists fifteen scaled maps | **Confirmed** — the array is **16** entries, one of which (`m21_01_00_00`) has `zoneScale = 0` and is an explicit no-op, so fifteen scale. It runs at `:704-716`, outside the `if (options.randomizeEnemies)` block |
| `ModelPicker::Draw` has no slot for a second line; `kPickerLayout` is the constant to measure against | **Confirmed**, and `Update` uses the same constant, which is why §3.5 passes the layout to both |
| `pool_verify.py maidens` encodes an expectation F6 shows is wrong | **Confirmed by measurement**: of the 42 maidens it expects to be byte-identical, **26 sit in stat-scaled maps**, so the check would report 16 and fail. §3.6 says how this plan avoids inheriting it |
| One of the 85 has no `Characters.json` entry, one more carries a misleading name | **Confirmed**: `c2561` has no entry and falls back to `C2561 C2561` (D3 satisfied by the generator as it stands); `c1130` is named "Labyrinth Ritekeeper" in the data but `enemy-exclusion-history.md:118-122` identifies the placement as the Oedon Chapel dweller. See §5.5 and the report — this is a spec gap, not a plan decision |
| `enemy_lookup.py randomizes()` already mirrors the placement decision; extend it rather than start a third mirror | **Confirmed** (`:110-125`), and §6.2 case 20 does exactly that — **with one boundary the appendix does not mention and §5.3 now depends on**: `randomizes()` mirrors the *decision* (the m28 override, the exclusion test, the zone roll) and stops there. It models **neither** of `StepWriteMap`'s two size gates (`:645-657`). Anything extended into it for the starved case must carry both gates or neither; carrying only the `× 2.0` one is exactly how this plan first arrived at a wrong number (§5.3) |
| The port's `c1110_0000` pool rule is inverted with respect to the reference's; it changes weights only | **Confirmed present** (`:381`); not touched |
| Dead end: matching by model rather than name substring buys nothing | **Confirmed by re-measurement**: across all 2,877 placements, `name[:5] == model` **without exception**, and no model id occurs inside another creature's placement name — **0 cross hits**. F3 stands |
| Dead end: deriving the 85 from the 82 plus three extras | **Confirmed as a dead end**, with a correction: the three fail *two* rules, not three. `c1130` has `ThinkParamID = 0` and `c2121` has `ThinkParamID = 1`, so both fail the `think <= 1` pool rule; `c2561` is dropped by the explicit name test at `:382`. The lists also carry different per-row numbers, which is the stronger reason |

---

## 3. Implementation approach

### 3.1 The 85-row table is generated, not derived

`gen_pool_table.py` gains a third `Kind`, `skip`, writing
`app/src/Randomizer/EnemySkipTable.h`:

* **rows** — the distinct models of every placement in the 24 base maps that
  `IsExcludedEnemyName` does *not* match. Measured: **2,877 placements, 608
  excluded, 2,269 overwritable, 85 distinct models**, and every one of the 82
  pool models also has placements (so the 85 are a strict superset).
* **order** — the same rule the other two tables use: display name upper-cased,
  then model id to break ties. Frozen from the moment it ships. The order is the
  meaning of a saved selection and only a changed *count* is detectable
  (`ModelPoolSelection::Decode`), so `pool_verify.py table skip` pins it, exactly
  as the 82-row table is pinned today.
* **`poolEntries`** — this table is never drawn from, so the field carries the
  number of placements that row protects instead of a draw weight, documented in
  the generated header's blurb. That buys a real cross-check: the weights must
  sum to 2,269. Measured spread: minimum 1, median 14, maximum 283; 17 rows have
  three placements or fewer and 5 have exactly one; 1,602 of the 2,269 are in
  maps the retail game loads.
* **names** — `display_name()` already raises on a character `Font8x8.cpp`
  cannot render. Re-measured across all 85: **none**. The longest row label is
  **39 characters** (`C2620 WHEELCHAIR HUNTSMAN (GATLING GUN)`), which is also
  the longest row of the shipped 82-row list, so the row itself needs no new
  width. `c2561` has no `Characters.json` entry and renders `C2561 C2561` — D3
  with no special case.
* **one name override, and it is required work, not a note** (spec D6, §9 P17).
  `gen_pool_table.py` gains a small project-owned override map consulted before
  `names.model_name()`, holding exactly one entry today:
  `c1130` → `OEDON CHAPEL DWELLER`, so the row reads
  `C1130 OEDON CHAPEL DWELLER` rather than `Characters.json`'s
  `Labyrinth Ritekeeper` (§5.5). The generated header must show it as **ours** —
  a marked comment on that row naming spec D6 — so a later reader does not
  "correct" it back to the data. **Applying the override globally is safe and
  the fact is worth writing down:** `c1130` occurs **zero** times in
  `EnemyPoolTable.h` and **zero** times in `BossPoolTable.h` (measured; its only
  other appearance anywhere in `app/src/Randomizer/` is a size entry in
  `ModelSizeTable.h:49`, which is keyed by prefix and carries no name), so P1's
  promise that `EnemyPoolTable.h` is untouched does not depend on the override
  being scoped to the `skip` kind. §6.2 case 26 pins the row text and the
  marking. Raised by the stage D review (finding 5), which was right that §5.5
  was the only place this appeared.

`EnemyPoolTable.h` is **not read, not altered and not regenerated** by any of
this. `gen_pool_table.py skip` and `gen_pool_table.py enemy` share only the
generator's plumbing.

### 3.2 The selection type, and the inverted default

Spec §7 requires three behaviours that are the mirror image of
`ModelPoolSelection`'s: construct to *nothing selected*, an unknown model reads
as *not skipped*, and a stale/truncated/absent saved value reads as *nothing
skipped*.

**Chosen: one template parameter with the current behaviour as its default.**

    template <int N, bool DefaultSelected = true>
    struct ModelPoolSelection { ... };

Only two lines inside the template change — the constructor calls
`DefaultSelected ? EnableAll() : DisableAll()`, and `IsModelEnabled`'s
not-in-the-table fallback returns `DefaultSelected`. `Encode`, `Decode` and
every other member are untouched, and `ModelPoolSelection<82>` still spells and
behaves exactly as it does today, so neither shipped picker changes. A
`bool IsModelSkipped(model, table) const { return IsModelEnabled(model, table); }`
alias is added so the engine call site reads correctly.

`Decode` needs no change: it returns false and leaves the object at whatever it
was, and every load path decodes into a freshly constructed
`RandomizerDefaults` (`RandomizerDefaultsStore.cpp:24`), so "leave untouched"
resolves to the constructor's default — which for this type is *nothing
skipped*. That is the fail-safe §7 asks for, and §6.2 asserts it in both
directions rather than leaving it to reading.

**Rejected — a separate `ModelSkipSelection` type.** It duplicates ~60 lines of
`Encode`/`Decode`/counting whose only job is to be identical, and two copies of
a positional encoder is a worse hazard than one parameterised one. Recorded
because it is the conservative choice and a reviewer may prefer it; the argument
against is duplication, not effort.

**Rejected — reusing `ModelPoolSelection<85>` with "enabled means skipped".**
It constructs to *everything skipped*, which freezes the entire game, and every
one of the three fail-safes then depends on a call site remembering to invert.
This is precisely what spec §7 forbids.

`EnemyPoolSelection.h` gains
`typedef ModelPoolSelection<kEnemySkipModelCount, false> EnemySkipSelection;`
and an include of the new table. No new include edges appear anywhere:
`RandomizerDefaults.h` and `ModelPicker.h` already pull that header in.

### 3.3 Two pools, one choice, and the separated failure

The skip list has a pool half and a placement half, and D4 makes them disagree
in one configuration. The pool half is decided while the pool is being built, so
that is where the fallback has to be expressible.

`StepReadMap` contributes to **two** vectors instead of one, with its own
per-map `contributedNpcIds` dedupe set for each:

* `pool` — today's rules **plus** the skip test;
* `poolIgnoringSkips` — today's rules exactly (the reference's fixed list and
  `ENEMIES INCLUDED` still apply; only the user's skip list does not).

`StepBuildPool` then decides, before the shuffle, in this order. **Every case is
tests `options.randomizeEnemies` explicitly, exactly as today's single `Fail`
does** — a bosses-only, treasure-only or drops-only run must reach neither
failure nor the D4 fallback (§9 P15, and the reachability argument below):

1. `if (maps.empty() && options.randomizeEnemies)` → **`Fail`**, with a message
   that names the vanilla source. This is the unusable-`VanillaSource` case and
   the only new hard failure.
2. `if (poolIgnoringSkips.empty() && options.randomizeEnemies)` → **`Fail`**,
   with a message that says nothing was eligible. Maps were read but yielded no
   candidates at all, which no reachable selection can cause: this is the
   residual broken-source case (wrong game, contaminated tree).
3. `if (pool.empty() && options.randomizeEnemies)` → use `poolIgnoringSkips`,
   set `result.poolFellBack`, log it. **This is D4**, and it is the only path
   that changes a run's outcome rather than its error message.
4. `if (pool.empty() && !options.randomizeEnemies)` → keep today's
   `Log("enemy randomizer: enemy pool empty and enemies disabled - skipping
   enemy shuffle")` at `:423`, unchanged and still reachable. It is the only
   thing that happens on that path today and nothing about it changes.
5. otherwise use `pool` as today.

**Why case 3's gate is load-bearing rather than tidy.** `StartCommit`'s
empty-selection refusal fires only when `randomizeEnemies_` is true
(`EnableWizardScreen.cpp:480`), so a bosses-only run committed with an empty
saved `ENEMIES INCLUDED` is reachable from the shipped screens. Without the
gate, both vectors are empty in that run, case 3 fires, `poolFellBack` is set,
and §3.4's fallback line appears on a run that never intended to randomize an
enemy. Found by the stage D review (finding 6); the reachability was re-verified
here against `:480`.

Then the existing shuffle, dedupe, `poolHasUnbanned*` scan and logging run
against whichever pool was chosen, unchanged. The other vector is freed.

Why this separation is sound, measured rather than assumed:

* **Every drawable creature has placements of its own.** The 82 pool models are
  a subset of these 85 rows, with no model in the pool that has no placements.
  So `pool` is empty if and only if every model that `ENEMIES INCLUDED` selects
  is also ticked here — spec §4 F10's "nothing to place has exactly one shape".
* **`poolIgnoringSkips` is empty only if no map produced a candidate.** The
  commit-time refusal (D5, unchanged) already rejects an empty `ENEMIES
  INCLUDED`, and `IsModelEnabled` lets an unknown model through, so a non-empty
  selection against a readable tree always yields candidates.
* **The UB hazard is closed by construction, not by a guard.** Cases 1 and 2
  fail before any draw; cases 3 and 5 both select a non-empty vector. The one
  remaining way to reach `StepWriteMap` with an empty pool is case 4,
  `randomizeEnemies == false`, and the write loop's
  `if (options.randomizeEnemies)` at `:618` already skips the whole body. Row
  16's plan-review finding **2.2** — "which sites index `pool`" — therefore does
  not recur: no draw-site guard is introduced, because no empty pool reaches a
  draw site. `DrawCandidate`'s comment says so.
* **A no-op selection is byte-identical.** With nothing ticked,
  `pool == poolIgnoringSkips`, case 5 fires, and not one extra `RandInt` is
  drawn anywhere — building the second vector consumes no randomness. Spec §8
  assertion 4 holds by construction. **By construction is not a check, and this
  plan no longer treats it as one:** the failure mode is a stray `RandInt`, a
  reordered contribution or a dedupe set built in the wrong scope, and every one
  of those would be invisible to every automated check here. §6.4 step 0 is the
  tree that makes assertion 4 testable — captured 2026-09-19, before this work
  starts (§9 P21) — and §6.4 step 1 and §6.5 step 9 diff against it. Raised by
  the stage D review (finding 2), which was right that the previous §6.5 step 9
  compared a tree against itself.

Cost: a second vector of at most a few hundred short strings, and one extra
substring pass per placement during the read phase. Both negligible against a
10–20 second run.

**Rejected — build one pool and filter it afterwards.** Simpler, and measured to
give the identical result for every single-model skip on this data (0 of 85
models differ). Rejected anyway: the per-map `contributedNpcIds` dedupe makes
contribution-time filtering and after-the-fact filtering *different rules* in
general — dropping a placement can let a later placement with the same NPC id
contribute — and the reference filters at contribution. Matching the reference's
mechanism costs one extra vector.

**Rejected — deciding starvation in the UI at commit time and passing a flag.**
It puts a pool rule in the UI layer where it can drift from the engine, and it
needs both baked tables to agree with the map data. The engine knows the answer
for free.

### 3.4 Telling the player (spec §7, fourth bullet)

Two new result fields and three new progress lines, all in `FinishCommit`,
**all gated on `randomizeEnemies_`** like every other enemy line there:

* when `randomizeEnemies_ && result.poolFellBack` — two lines,
  `ALL SELECTED ENEMIES WERE ALSO SKIPPED` (38 characters) and
  `SKIPPED ENEMIES WERE USED AS REPLACEMENTS FOR THIS RUN` (54). Two short lines
  rather than one long one, matching the shape `StartCommit` already uses for
  `NO ENEMIES SELECTED` / `SELECT AT LEAST ONE ENEMY...` (`:481-482`). Without
  them the run silently contradicts the list's stated promise;
* when `randomizeEnemies_ && result.enemiesRandomized == 0` —
  `NO ENEMIES WERE RANDOMIZED - EVERY ENEMY WAS SKIPPED` (52). This is the
  every-row-ticked case, the one starved configuration that genuinely changes
  nothing, and spec §7 requires it not to report a bare success.

#### The messages are constrained by the font, not only by the width

This is the part the first draft of this plan got wrong, and the stage D review
(finding 1) was right to block on it. §2.3 measures the mechanism: `Font8x8.cpp`
has no lowercase and no `:`, and an unrenderable character draws as a
**full-width blank column**. A message can therefore satisfy every length budget
in this document and still show the player an empty line.

**Re-measured rather than taken from the review, and the review's counts are the
correct ones.** The message being replaced is
`"enemy pool is empty - every enemy is excluded, or nothing was eligible"` —
**70** characters, not the 69 this plan first claimed, so the whole line is
**98** characters and **2,646px** against a 1,920px screen. But clipping is only
half the story: all 70 characters are lowercase and draw as blanks. Simulating
`DrawCenteredLabel` exactly — `x = (1920 − 2646)/2 = −363`, 27px per character,
so characters 14 through 83 are the ones inside the screen — today's failure
screen literally reads

    ATION FAILED                      -                        ,

**Both ends of the label are off-screen *and* five sixths of what is left is
blank**, with a stray hyphen and a stray comma from the lowercase message
floating in the gap. The first draft attributed it to clipping alone, which is
why it produced a length budget and no character constraint. This is also one
place where my measurement differs in detail from the review's, which described
the visible text as `ENEMY RANDOMIZATION FAILED` followed by about 44 blank
columns: the blank count is right (45 in the visible window), the leading text is
not, because the line is centred and its left end is clipped too. The conclusion
is unchanged and if anything worse than the review stated.

**P18 — the uppercasing happens in the `Fail()` strings themselves.** The same
string goes to the text log (`Fail` at `:245-250` logs
`"enemy randomizer: " + message`) and to the screen, so there are three places
the case could be fixed, and the plan has to pick one:

* **Chosen — write the two new messages inside `Font8x8.cpp`'s character set.**
  The cost is that two log lines read `enemy randomizer: VANILLA SOURCE
  UNREADABLE - NO MAPS FOUND` instead of all-lowercase. Nothing else in the
  build changes, and the constraint is checkable from the source text alone
  (§6.2 case 6), which is what makes it stick.
* **Rejected — uppercase at the UI**, `Upper(result.error)` in `FinishCommit`.
  It changes how the **11 other** `Fail()` messages render, on paths none of
  this feature's hardware steps exercise, and it does not actually fix them:
  `"failed to decompress gameparam.parambnd.dcx: " + err` still carries `:` and
  `.`, which stay blank however they are cased. It looks like a general fix and
  is not one. It would also need a shared uppercase helper in the UI layer —
  `Upper()` is private to `EnemyRandomizer.cpp`'s anonymous namespace (`:195`).
* **Rejected — a separate display string on `EnemyRandomizerResult`.** Its
  invariant is "every future `Fail` remembers to set me", and a forgotten one
  falls back silently to exactly the defect being fixed. A field whose failure
  mode is invisible is the wrong instrument for a problem whose whole symptom is
  invisibility.

**The fixed prefix loses its colon.** `"ENEMY RANDOMIZATION FAILED: "`
(`EnableWizardScreen.cpp:619`) becomes `"ENEMY RANDOMIZATION FAILED - "`, 29
characters — the hyphen is in the glyph table and is already this screen's
separator (`COMMIT CANCELLED - NOTHING WAS WRITTEN`,
`MERGO DARKNESS ENABLED - THE WORLD WILL BE DARK`). This is a one-character
change to a shipped line, and it is in scope because spec §7 and §8 assertion 8
make that line's readability a requirement of this feature.

**The budget is therefore 42 characters, not 43.** 71 characters is the whole
line at scale 3; 71 − 29 = **42**. Both new messages are written to it and both
are inside the character set:

| Case | Message | Length |
|---|---|---:|
| 1 — no map could be read | `VANILLA SOURCE UNREADABLE - NO MAPS FOUND` | 41 |
| 2 — maps read, no candidate at all | `NO ENEMIES FOUND IN THE VANILLA MAPS` | 36 |

The implementer may reword within the constraint; §6.2 case 6 pins the
**constraint** — length and character set, parsed out of the source — not the
wording. What it must not do is relax it.

**What this feature deliberately does not fix** is recorded in §5.11: the other
11 lowercase `Fail()` messages and the `STARTING CHOICES: ` progress line at
`:578` have the same defect and are out of scope.

### 3.5 The picker: one instruction line, and a vocabulary

`ModelPicker` gains a `PickerStrings` struct — heading, optional instruction,
the two flag-column words, the two confirm-prompt verbs and the second footer
line — passed **by reference to both `Update` and `Draw`**. Not stored in the
component: `Update` needs it too (the visible row count depends on whether the
instruction line is present), and a parameter that both calls must supply means
a missed call site fails to compile rather than silently drawing the wrong
vocabulary. The two shipped pickers pass a constant whose values are their
current strings with `instruction = nullptr`, so their rendering is unchanged
character for character.

**Layout — the spec's F13 option B ("give the list one row of room") is
chosen.** Measured against `ui_scroll_verify.py`'s model, not by eye:

| | Option A — keep 12 rows | **Option B — 11 rows** |
|---|---|---|
| Heading / count line | both move up; gaps tighten 25px → ~16px | **unchanged at y=120 / y=185** |
| Instruction line | y≈196, bottom 220 | **y=235, bottom 259** |
| List band | **unchanged** `{280, 52, 900}` → 12 rows | **`{332, 52, 900}` → 11 rows** |
| `MORE ABOVE` clearance | 234 − 220 = **14px** | 286 − 259 = **27px** |
| Last row bottom / `MORE BELOW` bottom | 852+24 / 922, against a 950 footer | **852+24 / 922, against a 950 footer** |
| Pages for 85 rows | 8 | **8** |

**Correction to this table, from the stage D review (§5, third non-reproducing
number), re-measured here.** The first draft wrote option A's list band as
`{260, 52, 900}`. That is wrong twice over. Arithmetically it gives
`(900−260)/52 + 1 = ` **13** rows and a `MORE ABOVE` hint at `260 − 46 = ` 214,
not the 12 rows and 234 the same row quoted — and geometrically it is not even
viable: 214 sits **above** the instruction line's 220 bottom, so
`ui_scroll_verify.py`'s G3 clearance check would fail. The reviewer diagnosed
the arithmetic and proposed 13/214; my measurement agrees with the arithmetic
but places the error one step earlier. Spec §4 F13 defines option A as "keeping
all twelve visible rows... no row is lost, nothing moves below the list", i.e.
the list band **does not move at all** and the room comes from above it. The
cell is corrected to the band the spec's option A actually implies, which makes
the rest of the row — 12 rows, hint at 234, 14px clearance, 8 pages — correct
and self-consistent. Option B is unaffected; every number in its column
reproduced exactly.

Both fit. B wins on three counts: it leaves the heading and count-line constants
**shared and untouched**, so the two pickers this feature must not disturb keep
their exact pixels; it keeps the `MORE ABOVE` clearance in the same range as
every other screen instead of halving it; and the cost — one visible row — buys
nothing back in option A, because 85 rows is 8 pages at 11 *and* at 12. The
list-band constant becomes the only thing that varies, selected by
`strings.instruction != nullptr`.

The instruction is `SELECT ENEMIES THAT WILL NOT BE RANDOMIZED` — 42 characters,
1,134px, well inside the 71-character line. Per §9 P13 the flag column becomes
`SKIPPED` / `-` (row width 39 + 3 + 7 = 49 characters = 1,323px, still centred
with 298px margins), the confirm prompt becomes `SKIP ALL 85` / `SKIP NONE 85`,
and the footer's second line becomes
`SQUARE SKIP ALL   TRIANGLE SKIP NONE   O BACK` (45 characters). Every one of
those six strings is inside `Font8x8.cpp`'s character set (§2.3) as well as
inside the 71-character line; §6.2 case 25 asserts both rather than leaving it
to reading.

### 3.6 The Python mirrors, and not inheriting the maidens error

`pool_verify.py maidens` asserts that 42 frozen maidens match vanilla in all
three written fields. Re-measured here: **26 of those 42 sit in stat-scaled
maps** and have their `NPCParamID` rewritten by the pass at `:704-716`, so the
check would report 16 and fail. Spec §6 puts *fixing* it out of scope and §8
declines to repeat it.

This plan avoids inheriting the error by defining "frozen" correctly once, in
one place, and never writing the plain-equality form:

> A placement is **frozen** when its `ModelName` and `ThinkParamID` are
> identical to vanilla and its `NPCParamID` is either identical **or** the
> zone-tuned variant of the vanilla value for that map.

The zone-tuned variant is computable exactly, not approximately. A new
`zone_scaled_npc(mapname, npc)` in `enemy_lookup.py` mirrors
`ApplyBossParamScaling`: parse `BossScalingMaps()` out of `BossParamScaling.h`
and the two arrays out of `NpcScalingTable.h`, find the tracked value's position,
add the zone scale, read the result. Two facts make a plain dictionary an exact
mirror rather than an approximation, both measured:

* **every one of the 1,171 tracked values appears exactly once** in the
  26,591-entry scaling table (0 exceptions), so "find the position" is
  single-valued;
* **no sequential-rewrite chaining occurs** — simulating `ApplyBossParamScaling`'s
  in-place loop (`BossParamScaling.cpp:40-53`) gives identical results to a
  single-shot mapping.

**The second measurement was strengthened after the stage D review (finding 7),
and the review was right.** The first draft measured it over the fifteen scaled
maps' *vanilla* contents, which is the wrong value set: the mirror's job is to
validate *randomized* output trees, where a scaled map can hold any NPCParamID
the pool can place. Re-measured over the full reachable set — the union of every
vanilla enemy NPCParamID in the 24 base maps (553 distinct) and every pool value
(332 distinct, a subset), **553 values**, of which **414 are tracked** — at all
fifteen zone scales: **zero chaining**, so the property holds more strongly than
the plan first claimed.

**And it is a data accident, not a structural guarantee, which is why the check
must stay.** The table structurally permits chaining: per zone scale, between
**151** (m36, scale 29) and **351** (m24_01, scale 1) of the 1,171 tracked values
scale to a value that is itself a tracked value processed later in the loop. Not
one of those lands on a value any map can actually contain. §6.2 case 22 is
restated over the reachable set so that if that ever stops being true, the
mirror fails rather than drifting.

Cross-check: the mirror reports **1,369 of the 2,269** overwritable placements
affected, reproducing spec §4 F6 exactly.

The row-16 `maidens` subcommand is then **retired together with the setting it
verifies** and replaced by a general
`pool_verify.py skipped <vanilla> <output> <models> [--starved]`, which is the
same contract expressed against the new list and with the corrected frozen
definition. This is a retirement, not a correction: the command checks a setting
that D1 deletes. Recorded as a decision in §9 rather than taken silently.

### 3.7 Layering

Everything new lives in `Randomizer` (the table, the selection type, the skip
predicate, the two-pool build) and `UI` (the picker parameterisation and two
screens). The UI learns no AFR path — it hands `EnemySkipSelection` to
`EnemyRandomizerOptions` exactly as it hands `EnemyPoolSelection` today — and
nothing in `Randomizer` gains an SDL2 dependency. `Msb` and `Param` are
untouched.

---

## 4. Files and changes

### 4.1 Milestone 1 — the starved run finishes, and a broken source still fails

No new files. `EnemyRandomizerResult` gains two members, so **a clean rebuild is
required** (§6.1).

| File | Change |
|---|---|
| `app/src/Randomizer/EnemyRandomizer.h` | `EnemyRandomizerResult` gains `bool poolFellBack = false;` and `int poolSize = 0;`, with a comment naming D4 as the reason |
| `app/src/Randomizer/EnemyRandomizer.cpp` | (a) `State` gains `std::vector<std::string> poolIgnoringSkips;` — in this milestone, the pool built without the row-16 exclusion. (b) the contribution loop at `:366-393` contributes to both vectors, each with its own `contributedNpcIds`. (c) `StepBuildPool:399-425` replaces the single `Fail` with §3.3's five-case decision and rewrites the comment block at `:400-417` to say where each cause now goes; **the enemies-disabled `Log` at `:423` survives verbatim as case 4**, with its condition made explicit (`pool.empty() && !options.randomizeEnemies`) rather than implied by the `Fail`'s early return. (d) `DrawCandidate:237` gains a comment stating that the pool is guaranteed non-empty by `StepBuildPool` and why `RandInt(0, -1)` is therefore unreachable. (e) the two new `Fail` messages are written to §3.4's 42-character, renderable-character budget (P18). (f) the file header comment's row-16 bullet gains the D4 behaviour |
| `app/src/UI/EnableWizardScreen.cpp` | `FinishCommit:547` adds §3.4's three lines, all gated on `randomizeEnemies_`; the failure prefix at `:619` changes from `"ENEMY RANDOMIZATION FAILED: "` to `"ENEMY RANDOMIZATION FAILED - "` (P18). No change to `StartCommit`'s D5 refusals at `:480` and `:496` |
| `app/tools/enemy_lookup.py` | `engine_pool(root, bell=False, included=None)` — a model-id set that mirrors `IsModelEnabled`, so the mirror can express "this selection starves the pool" |
| `app/tools/pool_verify.py` | selftest cases for §6.2's milestone-1 group |

### 4.2 Milestone 2 — the list

| File | Change |
|---|---|
| `app/src/Randomizer/EnemySkipTable.h` | **New, generated.** 85 rows + `kEnemySkipModelCount = 85` |
| `app/src/Randomizer/ModelPoolSelection.h` | Second template parameter `bool DefaultSelected = true`; ctor and unknown-model fallback keyed off it; `IsModelSkipped` alias; header comment gains a paragraph on why the polarity is a parameter and not a convention |
| `app/src/Randomizer/EnemyPoolSelection.h` | Include the new table; `typedef ModelPoolSelection<kEnemySkipModelCount, false> EnemySkipSelection;` |
| `app/src/Randomizer/EnemySkipList.h` | **New.** `BuildSkipPatterns(const EnemySkipSelection&)` → the ticked rows' model ids, built once per run; `IsSkippedName(name, patterns)`, the substring test. Kept out of `EnemyExclusionList.h` for the reason row 16 kept its three patterns separate: "what the reference always excludes" and "what the user chose" must stay visibly distinct |
| `app/src/Randomizer/EnemyExclusionList.h` | **Remove** `BellMaidenExclusionList()` and `IsExcludedEnemyName`'s second parameter (back to one argument). `M28ForcedMaidenList()` and `IsM28ForcedMaidenName` stay — the override is unconditional and survives D1 untouched. The header comment records that `c1055` was inert and where the fact is now pinned |
| `app/src/Randomizer/EnemyRandomizer.h` | `EnemyRandomizerOptions`: **remove** `unchangedBellMaidens`, **add** `EnemySkipSelection enemiesSkipped;` with a comment stating that it filters the pool *and* protects placements, i.e. the opposite of `enemiesIncluded` |
| `app/src/Randomizer/EnemyRandomizer.cpp` | `State` builds the skip pattern list once in its constructor. `:375` and `:629` gain `IsSkippedName(...)` beside the exclusion test — at `:629` **inside** the same `!forced &&`, so the m28 override still wins. Both `IsExcludedEnemyName` calls lose their second argument. Header comment's row-16 bullet rewritten for the list |
| `app/src/Randomizer/RandomizerDefaults.h` | **Remove** `unchangedBellMaidens`, **add** `EnemySkipSelection enemiesSkipped;` with the standard absent-key comment |
| `app/src/Randomizer/RandomizerDefaultsStore.cpp` | **Remove** the `unchanged_bell_maidens` load branch (`:59-60`) and its `snprintf` line and argument (`:98`, `:112`); **add** `enemies_skipped` in both. Worst-case buffer use measured: **478 → 555 bytes** of 1,024 (−25 for the retired line, +102 for the new one). The existing clamp at `:126` is unchanged |
| `app/src/UI/ModelPicker.h` / `.cpp` | `PickerStrings` struct; `Update` and `Draw` both take it; a second `ListLayout` chosen by `instruction != nullptr`; one `DrawCenteredLabel` for the instruction; the flag column, confirm prompt and second footer line read from the struct |
| `app/src/UI/SetupDefaultsScreen.h` | `kItemCount` **stays 15** (one row out, one row in). Delete `kUnchangedBellMaidensRow`, renumber the ten below it, add `kEnemiesSkippedRow` **directly after** `kEnemiesIncludedRow` (§9, P16). `Mode` gains `SkipPicker` |
| `app/src/UI/SetupDefaultsScreen.cpp` | Delete the bell-maidens `ToggleRow` branch and list entry; add the drill-in branch, the `EnemiesSkippedText()` summary, the picker delegation in `Update`/`Draw`, and the new row to the left/right drill-in exclusion at `:126-128` |
| `app/src/UI/EnableWizardScreen.h` | Delete `unchangedBellMaidens_`, add `EnemySkipSelection enemiesSkipped_;`; `Step` gains `SkipPicker`; `kSaveDataRowCount` **stays 15**; row constants as above |
| `app/src/UI/EnableWizardScreen.cpp` | Delete the bell-maidens ctor init, both toggle branches, the option assignment and the ` BELL MAIDENS UNCHANGED` suffix at `:559`. Add the member init, the X branch, `UpdateSkipPicker`/`DrawSkipPicker`, `options.enemiesSkipped = enemiesSkipped_;`, the entry at the same position in **both** `DrawSaveData` and `DrawConfirm` item lists, and a "N OF 85 ENEMIES SKIPPED" progress line when the count is non-zero. **Not** added to `StartCommit`'s big `\|\|`: it is a modifier on `RANDOMIZE ENEMIES`, like `randomizeWorkshopTools` and both pickers |
| `app/tools/gen_pool_table.py` | Third `Kind`, `skip`; `KINDS`/`main` accept it; `both` stays enemy+boss so no existing invocation changes. **Plus the D6 name override** (§3.1, §5.5, §9 P17): a project-owned `NAME_OVERRIDES = {"c1130": "OEDON CHAPEL DWELLER"}` consulted by `display_name()` before `names.model_name()`, and a marked comment on the generated row naming spec D6 as its source. Safe to apply for every `Kind` — `c1130` is in neither `EnemyPoolTable.h` nor `BossPoolTable.h` (measured, 0 occurrences in each), so P1's promise is not resting on the override's scope |
| `app/tools/enemy_lookup.py` | `overwritable_models(root)` and `overwritable_counts(root)` for the generator; **all six functions that carry `bell` today** — `exclusion_reason` (`:82`), `is_m28_forced`'s caller `randomizes` (`:110`), `engine_pool` (`:147`), `engine_pool_models` (`:195`), `cmd_frozen` (`:216`) and `cmd_diff` (`:264`) — take `skipped=()` in its place; `zone_scaled_npc(mapname, npc)`; `--bell` becomes `--skip c1050,c1051`. The first draft listed only three of the six; `pool_verify.py:347-349` calls two of the three it missed, so a partial rename would not even import. Caught by the stage D review (finding 5) and re-verified here |
| `app/tools/pool_verify.py` | `TABLES` gains `skip`; `maidens` retired and replaced by `skipped`; the 18 `bell:` selftest cases retargeted to the skip list with their measured numbers intact; new cases per §6.2 |
| `app/tools/ui_scroll_verify.py` | New `("Skipped picker", 332, 52, 900, 3, 85, 235 + glyph_h(3), SCREEN_H - 130)` row. The two existing picker rows and all three 15-row screens are **unchanged** |
| `docs/user-guide.md`, `docs/randomization-feature-spec.md` | **Not touched.** They belong to the documentation stage, which must record D1's un-migrated setting and D4's exception |

**Saved-configuration impact.** `defaults.cfg` is `key=value` per line, parsed by
`strtok` with unknown keys ignored (`RandomizerDefaultsStore.cpp:38-83`). So
removing the `unchanged_bell_maidens` line is genuinely free: an existing file
that still contains it loads fine with the key ignored, every other setting
keeps its meaning, and a file with no `enemies_skipped` line leaves the
selection at its constructor default of nothing skipped. This corrects spec §7,
which describes the format as positional — only the *selection values* are
positional (§5.1). The change is strictly safer than the spec assumed.

**A second, smaller correction to the spec in the same area.** Spec §4 F11 gives
the growth as **478 → 556** bytes, `+103 / −25`. Re-measured from
`RandomizerDefaultsStore.cpp:94-120`'s format string with worst-case values, the
added line `enemies_skipped=` + 85 characters + `\n` is **102** bytes, not 103,
and the retired `unchanged_bell_maidens=1\n` is 25, so the total is **555**. Off
by one, no consequence — 555 and 556 both sit far inside `char buf[1024]` and
the clamp at `:126` is unchanged either way. Recorded so the plan and the spec
are not quietly one apart. §6.2 case 24 pins 555.

---

## 5. Risks and unknowns

### 5.1 Row order is the meaning of a saved selection — three tables now

`EnemySkipTable.h` joins two other positional tables. A regenerated order
silently remaps every saved selection onto the wrong creatures, and `Decode`'s
length check catches only a changed count. Handled the way the other two are:
`pool_verify.py table skip` compares the baked order against the recomputed one
row by row and prints `ORDER MISMATCH - every saved selection is now wrong`, and
the generated header repeats the warning. The 82-row table is not read,
regenerated or compared against anything new, so rows 9 and 10 cannot be
affected by this work — asserted in §6.2 rather than assumed.

### 5.2 The polarity change touches a type two shipped features use

§3.2 adds a template parameter to `ModelPoolSelection`, which both pickers
instantiate. A mistake there breaks `ENEMIES INCLUDED` and `BOSSES INCLUDED` at
once. Mitigations: the parameter defaults to today's behaviour so both existing
spellings are unchanged; only two lines inside the template are touched;
`pool_verify.py selftest`'s encode/decode mirror is extended to run **both**
polarities, including the three fail-safes in the failing direction (wrong
length, unknown model, fresh construction). A Python mirror pins the rules and
not the C++ that implements them — the standing limitation.

### 5.3 A starved run is slow, and slow must not be read as hung

Measured for the chime-maiden starved case (tick the two maiden rows in both
lists): the fallback pool is **16 entries** (12 `c1051` + 4 `c1050`).
`StepWriteMap` has **two** size gates, not one (`EnemyRandomizer.cpp:645-657`),
and the count depends on which:

| Gate | Where | Cap | Placements with no maiden candidate that passes |
|---|---|---:|---:|
| `size > originalSize * 2.0` | every map except `m24_02` and `m35` | 30,000 | **162** |
| `size > originalSize` | `m24_02` and `m35` only (`:646`) | 30 | **73** |

**235 placements exhaust their loop**, not the 168 this plan first claimed. The
stage D review found the error (§5, second non-reproducing number) and I
reproduce its numbers exactly: **168** is what you get by applying the ×2.0 gate
uniformly to all 2,215 eligible non-maiden placements, i.e. from a mirror that
drops the `m24_02` / `m35` branch. **This matters beyond the arithmetic**,
because §6.2 proposes extending `enemy_lookup.randomizes()` as the single mirror
of `StepWriteMap`'s placement decision — a mirror that omits a branch of the
code it mirrors is the exact failure this plan is trying to avoid. Whatever
`randomizes()` grows must carry both gates or carry neither.

The timing conclusion is unchanged and if anything understated. Each try costs a
`ParsePoolString` plus a `LookupModelSize` linear scan of up to 245 prefixes, so
the cost is dominated by the 162 × 30,000 draws; the 73 × 30 draws are noise.
That is on the order of ten to thirty seconds spread across the write phase,
inside per-map steps that block the frame — an estimate only the console can
settle (§6.4 step 2).

This is pre-existing behaviour reachable today with a narrow `ENEMIES INCLUDED`,
not something D4 introduces — but D4 makes it reachable from a configuration the
developer will deliberately test. Handled by **telling the tester the number**
(§6.4, milestone 1 step 2) rather than by changing the loop: capping or
short-circuiting the reference's reroll is a behaviour change to a shipped pass
and is not in this feature's scope.

### 5.4 The m28 override interacts with a starved selection

With the chime maidens ticked in both lists, the pool falls back to containing
maidens, so the twelve forced Yahar'gul placements may be redrawn *as* maidens
and look frozen. Under the ordinary configuration (maidens skipped, the rest
included) the pool contains no maidens and all twelve deterministically change,
exactly as row 16 shipped. §6.3's `skipped --starved` mode therefore stops
asserting anything about the forced twelve, and says so.

### 5.5 One of the 85 rows is named for something it is not — resolved

`c1130`'s `Characters.json` name is "Labyrinth Ritekeeper", but
`docs/enemy-exclusion-history.md:118-122` identifies `c1130_0000` — the only
retail-loaded one of its two placements — as the Oedon Chapel dweller, with
`ThinkParamID = 0` and no combat AI, from an external data sheet
(デーモンの狂信者 / `DemonsFanatic`). This was raised as a spec gap and the developer
resolved it on 2026-09-18: the row reads `C1130 OEDON CHAPEL DWELLER` (spec D6,
§9 P17). The generator therefore needs a small project-owned name-override map
rather than taking `Characters.json` as the only source, and that override must
be visible as ours in the generated header. **That is work, not a note, and it
lives in §3.1 and §4.2's `gen_pool_table.py` row with §6.2 case 26 pinning it** —
this section is only the reasoning. It sat here alone in the first draft, which
the stage D review caught (finding 5), and an implementer working from §4 would
have shipped `C1130 LABYRINTH RITEKEEPER`. Note this does **not** reopen D3:
`c2561` still has no name and still renders as `C2561 C2561`, because there the
evidence was a position match rather than an identification.

### 5.6 `boss_verify.load_scaling_ids` silently returns nothing

Verified by running it: its path (`:403`) predates the repository layout and its
`except OSError` returns an empty set, so any check built on it passes
vacuously. Nothing in this feature uses it — §3.6's mirror parses the header
itself and cross-checks against F6's 1,369 — but it is the obvious thing for a
future reader to reuse. Out of scope; recorded so it is not rediscovered as this
feature's bug.

### 5.7 Assertion 3 is a rate check, not an equality check

Spec §8 assertion 3 asks that placements of un-skipped creatures be "randomized
at the same rate as a run with nothing skipped". Skipping removes placements
from the roll stream, so the two runs are different worlds and only the
*proportion* is comparable. §6.3 implements it as a reported rate with a
tolerance rather than an assertion, and says so in the output. Flagged so the
weaker form is a stated choice and not a silent one.

### 5.8 Nothing here can leave a half-written tree

After milestone 1, the only `Fail`s that can fire after `StepMirror` are a DCX
decompress failure, an MSBB parse failure, a `MakeDirsRecursive` failure and a
`WriteWholeFile` failure — none reachable from a selection. Spec §8 assertion
7's "no run stops after the output folder has been touched, for any selection
reachable from the two lists" therefore holds. `Fail` still does no cleanup, so
a genuine I/O failure still leaves a partial tree; that is unchanged and out of
scope.

### 5.9 Run-breaking risk is low and bounded

A skipped placement keeps all three written fields, so its entity ID and every
event hook are untouched by construction. Whether the creature then *behaves* as
in vanilla is behaviour and `CLAUDE.md` §5 forbids concluding it from bytes —
§6.5 makes it a hardware observation, as row 16 did, and row 16's equivalent
test passed. The worst case for the frozen half is a world slightly more like
vanilla.

Boss randomization cannot touch a skipped **placement**, and this was
re-verified rather than taken from the spec: `BossNameList()`'s 38 entries are
the tail of `EnemyExclusionList()`, and every `AddTheRestInMap` target and every
`kFixups` companion (`c2570_0001`, `c5510_0001`, `c5510_0002`, `c4520_0000`,
`c4030_0000`) is matched by the fixed exclusion list, so none of them is one of
the 2,269. The stage D review re-derived the same thing independently. **That
settles the placement half only; the pool half is §5.10.**

### 5.10 Two of the 85 are also in the boss pool, and six have reference-excluded placements too

Both were found by the stage D review (finding 3) and both reproduce exactly.

**Six of the 85 rows have placements on *both* sides of the fixed exclusion
list.** Measured across the 24 base maps:

| Model | Name | Overwritable | Already excluded |
|---|---|---:|---:|
| `c1060` | Brainsucker | 19 | 2 |
| `c2090` | Blood Starved Beast | 1 | 2 |
| `c2100` | Witch of Hemwick | 2 | 7 |
| `c2120` | Shadow of Yharnam | 7 | 8 |
| `c2500` | Small Celestial Emissary | 50 | 7 |
| `c2710` | Father Gascoinge | 3 | 3 |

Tick any of those six and the output tree **still contains placements of that
model** — the frozen, reference-protected ones the feature never claimed to
remove. An automated check phrased as "no placement anywhere uses a skipped
model" therefore fails on a *correct* implementation. That is worse than a
wrong number: a check that cries wolf on the happy path teaches its reader to
ignore it. §6.3 assertion 2 is restated (§9 P20) to say what it actually means.

**Two of the six are in the boss pool: `c2090` and `c2710`.** Measured as the
intersection of `BossPoolTable.h`'s 17 models with the 85 rows; it is exactly
those two and no others. With `RANDOMIZE BOSSES` on, ticking
`C2090 BLOOD STARVED BEAST` here does **not** stop a boss arena from becoming a
Blood Starved Beast, because boss randomization draws from its own separately
built pool and this list does not filter it.

**This plan does not change that, and cannot.** Whether `ENEMIES SKIPPED` should
also filter the boss pool is a question about what the feature *does*, and it
bears directly on spec §10 D4's claim that its starved-selection exception is
"the only case in which" a skipped creature appears somewhere new — with bosses
on, that claim is not true. **That is a spec decision and not the planner's.**
It has been reported to the developer rather than answered here. The plan is
written for the spec as approved: the overlap is **accepted and documented** —
§6.3 assertion 2 scopes boss-written placements out, §6.5 step 2 tells the
tester to leave `RANDOMIZE BOSSES` off so the observation is clean, and the
documentation stage inherits a second exception to record alongside D4's.

**If a `/refine-spec 32` answers it the other way**, the plan changes in three
places and nowhere else: `BossRandomizer` would need the skip selection passed
to it and applied at its pool build; §6.3 assertion 2 would lose its
boss-placement exemption; and milestone 2 would grow a boss-pool case. It is a
contained change, which is why waiting for the answer costs little.

### 5.11 The font defect is wider than this feature, and this feature fixes only its own part

§2.3 measures it: `Font8x8.cpp` has no lowercase and no `:`, and unrenderable
characters draw as blank columns. In the working tree that reaches **12**
`Fail()` messages in `EnemyRandomizer.cpp`, all lowercase, and the
`STARTING CHOICES: ` progress line at `EnableWizardScreen.cpp:578`.

This feature fixes exactly two of them, both because spec §7 and §8 assertion 8
require it: the message at `:419`, which it replaces with two of its own written
to the constraint, and the `"ENEMY RANDOMIZATION FAILED: "` prefix that frames
them (§3.4, P18). **The other 11 messages and `STARTING CHOICES: ` are
left exactly as they are.** They are reachable only through I/O and param
failures that none of this feature's hardware steps exercise, and fixing them
would be a cosmetic change to eleven shipped error paths inside a feature about
an enemy list. Recorded here so it reads as a scope line rather than an
oversight, and so a future reader does not conclude from `:419`'s new message
that the rest were checked.

---

## 6. Verification

### 6.1 Build

    cd app
    rm -rf src/x64
    make

**The clean rebuild is required, not optional, in both milestones.** Milestone 1
adds members to `EnemyRandomizerResult`; milestone 2 changes
`RandomizerDefaults` and `EnemyRandomizerOptions` (a `bool` out, 85 `bool`s in)
and the shape of a template two other types instantiate. `CLAUDE.md` §2 records
that a stale partial rebuild once produced a real heap-corruption SIGSEGV on
hardware.

Baseline before any change, re-run and confirmed green while writing this plan:
`pool_verify.py selftest` **43/43**, `pool_verify.py table both` **PASS** at
82/17, `ui_scroll_verify.py` **PASS**.

### 6.2 `pool_verify.py selftest` — new cases

Run as `python tools/pool_verify.py selftest ../data/vanilla/dvdroot_ps4`. Each
case pins a rule, in the failing direction where one exists.

**Milestone 1 — the pool decision.**

1. the flag-off pool is unchanged: 333 entries, 82 models (existing case, must
   keep passing untouched);
2. with the row-16 exclusion on and `ENEMIES INCLUDED` narrowed to
   `{c1050, c1051}`, the skip-filtered pool is **empty** — the starved shape;
3. …and the fallback pool for that same selection is **16 entries**, which is
   the sum of the two rows' baked weights. This is what D4 draws from;
4. every model in the baked 82-row table has at least one placement of its own,
   so a starved pool has exactly one cause (the measurement §3.3 rests on);
5. no selection of the 82 rows can empty the fallback pool — sampled over the
   single-row selections, all 82 non-empty;
6. **every string this milestone puts on screen is renderable as well as short.**
   Parsed out of the source, not retyped into the test: the two new `Fail`
   messages from `EnemyRandomizer.cpp`, and the
   `"ENEMY RANDOMIZATION FAILED - "` prefix plus §3.4's three progress lines
   from `EnableWizardScreen.cpp`. For each — **every character is in
   `gen_pool_table.RENDERABLE`** (§2.4: `Font8x8.cpp`'s own 42-character set,
   reused rather than restated), prefix + message is **≤ 71 characters**, and
   each failure message alone is **≤ 42**. The character assertion is the one
   that matters: length alone passed the first draft of this plan while the
   messages would still have drawn blank (§3.4). Pinned in the failing direction
   too — a lowercase probe and a probe containing `:` must both be rejected by
   the predicate, so the case cannot pass vacuously;
7. the two messages are **different strings** — a starved selection and a broken
   source must not report identically (spec §8 assertion 8).

**Milestone 2 — the table, the polarity, the two halves.**

8. the baked 85-row table **is** the set of models with overwritable placements,
   in order (`cmd_table("skip")`, the spec's assertion 6);
9. `kEnemySkipModelCount == 85` and matches the row count;
10. the table's weights sum to **2,269**;
11. every row has a renderable display name; `c2561`'s row is `C2561`, i.e. the
    generator's fallback and not an invented label (D3);
12. the 85 rows are a strict superset of the 82, and the three extras are exactly
    `c1130`, `c2121`, `c2561`;
13. the 82-row table is **still 82 rows and byte-identical**, and is not compared
    against anything the skip list computes (§5.1);
14. mirror, both polarities: a default-constructed skip selection encodes to 85
    zeros, the pool selection to 82 ones;
15. mirror: a wrong-length, empty or absent skip value leaves **nothing
    skipped** — three cases, the fail-safe spec §7 demands;
16. mirror: a model absent from the skip table reads as **not skipped**, while
    one absent from the pool table reads as **enabled**;
17. skipping `{c1050, c1051}` yields the 317-entry, 80-model pool and the 16 lost
    weights — the retargeted row-16 cases, numbers intact;
18. all 54 maiden placements match the skip test with those two rows ticked, none
    with them unticked;
19. the six forced names resolve to 12 placements, all in m28, all
    `NPCParamID 105810`, all skip-matched, all appearing verbatim in
    `reference/.../RandomizeFunctions.cs`, and inert outside m28 (existing cases,
    retargeted);
20. `randomizes()` over all 101 rolls: a forced m28 placement randomizes at every
    roll; a non-forced m28 maiden randomizes at no roll when skipped and at every
    roll but 100 when not;
21. **no placement in the 24 base maps contains `c1055`** — the fact row 16's
    inert third pattern recorded, kept pinned after the pattern itself is
    deleted;
22. `zone_scaled_npc` cross-check: **1,369 of the 2,269** overwritable placements
    are reached by the stat pass, and **1,171** tracked values each appear
    exactly once in the 26,591-entry scaling table. **The no-chaining assertion
    runs over the reachable value set, not over the fifteen vanilla maps**
    (§3.6): the sequential in-place mirror must equal the single-shot mapping
    for all **553** values a scaled map can hold after randomization — every
    vanilla enemy `NPCParamID` in the 24 base maps ∪ every pool value — at all
    fifteen zone scales. The vanilla-map form of this check does not cover the
    values the mirror will actually meet. The case also **records that between
    151 and 351 of the 1,171 tracked values structurally could chain at each
    zone scale** and that none of them lands on a reachable value, so the next
    reader knows the property is a data accident rather than a guarantee;
23. **26 of the 42 non-forced maiden placements are in scaled maps** — the
    measurement that makes plain equality the wrong assertion, pinned so the
    corrected definition cannot quietly regress;
24. the worst-case `defaults.cfg` string is **555 bytes** and fits `char buf[1024]`;
25. **every string the picker adds is renderable and fits**, parsed out of
    `ModelPicker.cpp` and the two screens rather than retyped: the instruction
    line (42 characters), the second footer line (45), `SKIP ALL 85` and
    `SKIP NONE 85`, the `SKIPPED` / `-` flag column, and the
    `N OF 85 ENEMIES SKIPPED` progress line. Same predicate as case 6, same
    failing-direction probes. Also: the widest row the new list can draw is
    39 + 3 + 7 = **49** characters = 1,323px, inside the 1,920px screen;
26. **`c1130`'s row reads `C1130 OEDON CHAPEL DWELLER`** — not
    `Characters.json`'s `LABYRINTH RITEKEEPER` — and the generated header marks
    that row as a project-owned override naming spec D6 (§3.1, §5.5, §9 P17).
    Paired with case 11, which pins that `c2561` is *not* given an invented
    name, so the two decisions cannot drift into each other.

### 6.3 Other verifiers

    python tools/pool_verify.py table both  ../data/vanilla/dvdroot_ps4   # must still PASS at 82/17
    python tools/pool_verify.py table skip  ../data/vanilla/dvdroot_ps4   # new, 85 rows
    python tools/ui_scroll_verify.py                                      # + the 11-row skipped picker
    python tools/enemy_lookup.py frozen ../data/vanilla/dvdroot_ps4       # 608, unchanged
    python tools/gen_pool_table.py skip ../data/vanilla/dvdroot_ps4 --check

New `pool_verify.py skipped <vanilla> <output> <models> [--starved]`, run after a
hardware run, asserts the feature's contract from the output tree alone:

* **assertion 1** — every placement of a skipped model is frozen by §3.6's
  definition: same `ModelName`, same `ThinkParamID`, `NPCParamID` either
  unchanged or `zone_scaled_npc` of the vanilla value. The only permitted
  exception is the twelve forced m28 placements when the maiden rows are ticked;
* **assertion 2** — **no *eligible* placement was *changed* to a skipped
  model.** Stated as a diff against vanilla over the 2,269 overwritable
  placements, which is the shape `pool_verify.py filter` already has, and
  **not** as "no placement anywhere in the output uses a skipped model", which
  the first draft wrote and which is false on a correct implementation: six of
  the 85 rows also own reference-excluded placements that stay put (§5.10).
  **Boss-written placements are explicitly out of this assertion's scope** — the
  boss pass writes only into arenas the fixed exclusion list already protects,
  so none of them is one of the 2,269, but `c2090` and `c2710` can still arrive
  in an arena from the boss pool and that is not this check's business (§5.10).
  Suppressed by `--starved`, where D4 makes even the restated form deliberately
  false;
* **assertion 3** — the proportion of non-skipped eligible placements that
  changed, reported against the unskipped run's proportion with a tolerance
  (§5.7);
* **assertion 7** — with `--starved`, every file a normal run writes is present
  and every other enabled randomizer has been applied.

**Assertion 5 has a sequencing requirement.** "Ticking the two chime maiden rows
reproduces what `UNCHANGED BELL MAIDENS` did, for the same seed" can only be
checked against a tree produced by a build that still has the setting. Because
the same placements are skipped at the same two points and `c1055` is inert, the
two trees should be **byte-identical**, so the check is a file comparison of the
24 `.msb.dcx` files rather than a new tool. That tree must be captured during
milestone 1's hardware test, while such a build is still in hand — §6.4 step 4,
confirmed by the developer (§9, P14).

**Assertion 4 has the same shape and a tighter deadline, and the first draft of
this plan missed it.** "Nothing skipped is byte-identical to a run of the same
seed made before the feature existed" also needs a tree, and its build expires
one milestone earlier — at milestone 1's *first code change*, not at milestone
2's. **That tree was captured on 2026-09-19** (`data/runs/20260919-Enemies Only`,
§9 P21) and §6.4 step 1 / §6.5 step 9 compare against it, by the same 24-file
comparison. Neither assertion needs a new tool; both need a tree captured at the
right moment, and until this pass only one of the two had one. Both comparisons
are at the same seed `S` — step 0's — for the reason §6.5's preamble gives.

A Python mirror pins the rules, not the C++ implementation of them. Every check
above can pass against an implementation that does the wrong thing at runtime.
The two byte-comparison checks (assertions 4 and 5) are the only ones here that
are not mirrors — they compare real output against real output — which is
exactly why losing their baselines would matter.

### 6.4 Hardware — milestone 1, and one step before it

Nothing in this loop can run the game. Milestone 1 needs **four** trees, not the
three the first draft named: vanilla, the pre-change baseline of step 0, the
post-change run of step 1 that must match it byte for byte, and step 4's
flag-on tree that milestone 2 will be diffed against.

**Step 0 had to happen before the first code change of milestone 1 (§9 P19), and
it has happened (§9 P21).** Spec §8 assertion 4 — "nothing skipped is
byte-identical to a run of the same seed made before the feature existed" —
needs a tree from a build *without* this work, and the build currently in hand
is the only one that can produce it. §3.3 argues the property holds by
construction; an argument is not a check, and the failure it would miss (a stray
`RandInt`, a reordered contribution, a dedupe set in the wrong scope) is exactly
the kind the two-pool refactor could introduce. **The pre-change state is not
recoverable afterwards:** nine sources under `app/src/Randomizer/` and
`app/src/UI/` are uncommitted working-tree modifications, so `git` cannot
resurrect it either. Raised by the stage D review (finding 2); the developer
captured the tree the same day, at
**`data/runs/20260919-Enemies Only/dvdroot_ps4`**.

**No *older* tree could have served, which is why a fresh one was needed.** The
four that predate it — `20260912-enemy_and_boss_randomized`, `New Test Runs/*`,
`Randomize Enemies Only/PS4 1..3` — carry map files dated 2026-09-12 to
2026-09-14, i.e. **before row 16 landed in the working tree on 2026-09-16**, and
row 16's unconditional m28 override changes how many `RandInt` calls the write
loop makes, so none of them was produced by the build step 1 will be compared
against. None of them records a seed or a settings list either.

#### What the captured tree is, verified rather than assumed

The tree carries no log, no `defaults.cfg` and no note — 102 files, all `.dcx`.
So its **settings were re-derived from its contents** against
`data/vanilla/dvdroot_ps4`, and every one of them matches "`RANDOMIZE ENEMIES`
on, everything else default". Nothing found contradicts what step 1 needs.

| Setting | Evidence from the tree | Reading |
|---|---|---|
| Scope of the write | All **43** `.msb.dcx` present; exactly the **24** `kBaseMaps` differ from vanilla and all **19** chalice maps are byte-identical | The base-map write loop ran, nothing else |
| `RANDOMIZE ENEMIES` | **1,974** of the 2,269 eligible placements carry a new model; 2,226 differ in at least one of the three written fields | **ON** |
| …and at the default rate | In **unscaled** maps the change rate tracks each map's `kBaseMaps` zone chance: the 100-chance maps land at 97–100%, and `m24_02_00_00` (chance **60**) lands at **59.3%**. By model change, `m35_00_00_00` (chance **30**) is 24/84 = **28.6%** and `m24_02_00_01` is 32/54 = **59.3%** | The stock zone-chance table, full pool |
| `RANDOMIZE BOSSES` | **0** reference-excluded placements changed model. A boss pass would have rewritten arena placements, all of which are exclusion-matched | **OFF** |
| `RANDOMIZE TREASURE` | **0** of **73,125** non-enemy MSB parts differ | **OFF** |
| Drops / starting weapons / guns / shop weapons | `param/` is byte-identical to vanilla, so the `ItemData` phase never ran | **all OFF** |
| `ENABLE MERGO DARKNESS` | `event/common.emevd.dcx` decompresses to plain bytes differing from vanilla at exactly 4 offsets (60440–60444), carrying `159, 134, 1, 0` — `PermaDarkness.cpp:63-67`'s **OFF** pattern | **OFF** |
| `UNCHANGED BELL MAIDENS` | **50 of 54** maiden placements carry a new model, including all **12** m28-forced ones. With the flag on, only the 12 could have changed | **OFF** |
| `ENEMIES INCLUDED` | All **82** pool models appear as placed values somewhere, and no model outside the pool was placed | **all 82 (default)** |
| `BOSSES INCLUDED` | Not determinable, and not needed — the boss pass did not run | n/a |
| **Seed** | **Not recoverable from the tree.** Nothing in the output encodes it | **must be supplied** |

Two things worth knowing rather than being surprised by. `event/common.emevd.dcx`
is the one non-map file that differs, and it differs **on every run**:
`StepEmevd` (`:753-779`) writes both states rather than leaving the mirrored copy
alone, and the port's DCX compressor emits stored blocks, so the file grows
11,415 → 74,844 bytes while its plain content is 74,752 either way. And vanilla's
own bytes are the *darkness-on* pattern — the inversion `PermaDarkness.h` warns
about — so "OFF" here means the poke was written, which is correct and expected.

| # | Step | Pass looks like |
|---|---|---|
| 0 | **Done — `data/runs/20260919-Enemies Only`, captured 2026-09-19 before any code change (§9 P21).** Settings verified from the tree's contents, above. **`SEED = 1234567890`** — transcribed 2026-09-19 from the run's own `USING SEED 1234567890` line, after the developer recovered `live.log` and added it to the tree. That log also confirms the settings the table above re-derived from the bytes: bosses, treasure, workshop tools, drops, starting weapons, starting guns, shop weapons and Mergo darkness all `NO`. A `README.txt` beside the tree records the same | The tree exists and steps 1 and §6.5 step 9 have something to diff against. **The seed is not optional bookkeeping:** step 1 has to reproduce this exact run, and a tree whose seed is guessed produces a diff whose failures cannot be interpreted — a mismatch would be indistinguishable from the two-pool refactor perturbing the roll stream, which is the one thing step 1 exists to detect |
| 1 | A normal run, `RANDOMIZE ENEMIES` on, everything else default, **at step 0's seed `S`** | Completes with the usual counts, **and the 24 `.msb.dcx` files are byte-identical to step 0's tree.** That is the regression check "the two-pool build must change nothing" actually means, and spec §8 assertion 4 |
| 2 | **The starved case.** `UNCHANGED BELL MAIDENS` on, and only the two rows `C1050 CHIME MAIDEN` and `C1051 CHIME MAIDEN (LIGHT)` ticked in `ENEMIES INCLUDED` | The run **completes** instead of failing after the mirror phase. A world of chime maidens with the original maidens still at their bells. **Expect the write phase to take noticeably longer** — measured cause in §5.3, 162 placements burning a 30,000-try loop; that is not a hang |
| 3 | **The broken source.** Rename or empty `/data/bbrandomizer/VanillaSource/dvdroot_ps4/map` and run with enemies on | Fails, reading `ENEMY RANDOMIZATION FAILED - VANILLA SOURCE UNREADABLE - NO MAPS FOUND` — visibly different from step 2's outcome and from the old text. **Every word must be legible.** Today's line is 98 characters, so it runs off both edges, *and* its message is entirely lowercase, so what is on screen now is `ATION FAILED` followed by 45 blank columns with a stray `-` and `,` in them (§3.4). Anything short of a fully readable sentence here means the character constraint was not applied |
| 4 | **Capture the baseline for assertion 5.** With this same build, run **seed `S`** (step 0's seed, not the `1234567` P14 named — see the note below), `RANDOMIZE ENEMIES` on and `UNCHANGED BELL MAIDENS` on, everything else default, and keep the output tree under `data/runs/` | A tree that milestone 2 can be diffed against. This build is the last one that can produce it. **Confirmed by the developer (§9, P14) — this step is required, not optional.** Distinct from step 0: that one is flag-*off* and pre-change, this one is flag-*on* and post-milestone-1 |
| 5 | Reopen Setup Defaults after saving | Every setting persists and `ENEMIES INCLUDED` still reads `82 OF 82` |

**Note on the seed `1234567`.** P14 named that number for step 4 when step 0 was
still hypothetical, precisely so the two would share one seed. Step 0's tree now
exists and was rolled at some other, unrecorded seed, so the number moves and the
principle does not: **every tree in §6.4 and §6.5 uses the seed of
`20260919-Enemies Only`.** P14's substance — the assertion-5 baseline is captured
during milestone 1's hardware test, required and not optional — is untouched;
only the literal it quoted changes, because keeping it would put the five trees
on two seeds and make steps 2, 3 and 9 read against different worlds.

**Failure would look like:** step 1 differing from step 0 by a single byte in any
map (the two-pool build perturbed the roll stream — this is the whole reason
step 0 exists); step 2 crashing or hanging (the pool decision let an empty pool
reach a draw); step 3 reporting success with
`RANDOMIZED 0 ENEMIES ACROSS 0 MAPS` (the two causes were not separated — this is
blocking finding 2.1 recurring), or showing a blank line where the message should
be (the character-set constraint was not applied); step 1 producing a visibly
different kind of world (the fallback fired when it should not have).

### 6.5 Hardware — milestone 2

**`S` is the seed of the `data/runs/20260919-Enemies Only` tree**, whatever
§6.4 step 0 records it to be, and **every run below uses it**. It is not
`1234567`; that number was chosen while step 0 was still hypothetical and is
superseded (see the note at the end of §6.4). One seed throughout is what makes
every comparison below a comparison of *settings* rather than of worlds. Five
trees are in play, not three:

* `V` — vanilla, `data/vanilla/dvdroot_ps4`;
* `A` — seed `S`, this build, nothing skipped. The reference for "did this
  creature move?" in steps 2 and 3;
* `B` — seed `S`, this build, a chosen skip list. What steps 1–5 examine;
* the **§6.4 step 0** tree — `20260919-Enemies Only`, seed `S`,
  *pre-implementation*, `RANDOMIZE ENEMIES` on and everything else default.
  Step 9's target, and spec §8 assertion 4;
* the **§6.4 step 4** tree — seed `S`, post-milestone-1, `UNCHANGED BELL
  MAIDENS` on. Step 6's target, and spec §8 assertion 5.

`A` and the step 0 tree are the *same configuration* on either side of the whole
feature — "seed `S`, nothing skipped, everything else default" — so step 9 is
their comparison and needs no extra run. If they differ, assertion 4 has failed
and steps 2 and 3 are being read against the wrong world.

| # | Step | Pass looks like |
|---|---|---|
| 1 | `pool_verify.py skipped V B <models>` | PASS — both halves, with the corrected frozen definition |
| 2 | **Many-placement creature.** Tick the row reading `C1170 CARRION CROW` (202 placements, 131 in retail-loaded maps) and play, with `RANDOMIZE BOSSES` **off** so the observation is clean (§5.10) | Every crow is still a crow where you remember it, and no crow appears anywhere it did not in `A`. Both halves in one observation |
| 3 | **The chapel dweller.** Tick the row reading `C1130 OEDON CHAPEL DWELLER` (spec D6, §9 P17, §5.5) | Oedon Chapel is as the game shipped it and rescued survivors still gather. One of the three creatures the shipped list cannot reach |
| 4 | **Does a skipped creature still work?** Stand by a frozen chime maiden; talk to a frozen non-combat NPC | The bell still summons; the NPC still talks. Spec §4's assumption; only the console answers it |
| 5 | **Yahar'gul.** Tick the rows `C1050 CHIME MAIDEN` and `C1051 CHIME MAIDEN (LIGHT)` | Six of its fifteen maidens **have** changed. That is the correct result, not a failure |
| 6 | **Assertion 5.** Seed `S`, the two chime maiden rows ticked, everything else default; diff the 24 `.msb.dcx` files against §6.4 step 4's tree | Byte-identical. Any difference means the two mechanisms are not equivalent |
| 7 | **Navigation.** 85 rows through an eleven-row window, 8 pages | Cursor stays visible, paging lands where expected at both ends, select-all and select-none do what the footer says, the summary row's count matches |
| 8 | **Read the screen.** | The instruction line is legible and clear of the heading, the count line and the list; the flag column and both prompts read as skipping, not enabling; no word on the screen shows a gap where a character should be (§2.3) |
| 9 | **Assertion 4 — leave it empty.** No new run needed: tree `A` *is* "seed `S`, nothing ticked, everything else default". Diff its 24 `.msb.dcx` files against **§6.4 step 0's pre-implementation tree** | Byte-identical. The default must perturb nothing, across both milestones. The first draft of this plan diffed this run against `A` itself — same build on both sides, so it could not fail; corrected after the stage D review (finding 2) |
| 10 | **Starve it through the new list.** Tick only the two chime maiden rows in `ENEMIES INCLUDED` and the same two here; then tick **every** row as well | Both complete. The first gives a world of maidens with the originals at their bells and shows `ALL SELECTED ENEMIES WERE ALSO SKIPPED`; the second randomizes nothing and shows `NO ENEMIES WERE RANDOMIZED - EVERY ENEMY WAS SKIPPED` rather than reporting a bare success |
| 11 | **D1.** Open both screens with a `defaults.cfg` that has `unchanged_bell_maidens=1` | The row is gone, every other setting survived, and the maidens randomize again until the two chime maiden rows are ticked here. This is the deliberate un-migrated loss |
| 12 | **The boss-pool overlap, observed rather than fixed (§5.10).** Tick `C2090 BLOOD STARVED BEAST` and `C2710 FATHER GASCOINGE` here with `RANDOMIZE BOSSES` **on** — those two are the whole intersection of the 85 rows with `BossPoolTable.h` | A boss arena may still become one of them, while their own overwritable placements (1 and 3 respectively) stay frozen. **This is the expected result under the spec as approved** — recorded so it is not reported as a defect, and so the developer sees the case the spec question in §5.10 is about |

**Failure would look like:** a skipped creature still being replaced (the
selection did not reach `:629`); a skipped creature appearing somewhere new
outside step 10's starved case and step 12's boss case (it did not reach
`:375`); step 9 differing from the step 0 baseline (the default is not a no-op);
a saved selection from either shipped picker meaning something different
afterwards (§5.1); a creature frozen in place but no longer doing what it does,
which would invalidate more than this feature.

Building cleanly and passing every check means **ready for hardware testing**,
never done.

---

## 7. Milestones

Two milestones, in this order, with a full stop between them — and **one
required step before milestone 1 writes a line of code**.

| # | Milestone | Result |
|---|---|---|
| **0** | **The assertion-4 baseline** (§6.4 step 0, §9 P19, P21). No code change at all. **Already done:** the developer captured it on 2026-09-19 at `data/runs/20260919-Enemies Only`, and its settings were verified from the tree's contents to be `RANDOMIZE ENEMIES` on with everything else default. **What remains is one line of bookkeeping — record its seed in §6.4 step 0 — and that must happen before milestone 1 edits a file** | The tree exists. **Not a milestone in the `CLAUDE.md` §4 sense** — nothing is built for the feature and nothing is approved; it was a precondition of milestone 1 that happened to need the console |
| 1 | **The starved run finishes, and a broken source still fails.** The two-pool build, the five-case decision in `StepBuildPool`, the two distinct failure messages within the **42**-character, renderable-character budget and the prefix change that sets it (P18), the three progress lines, the result fields, and the mirror changes behind §6.2's cases 1–7. No UI row is added or removed; the reachable test case is row 16's existing flag | `.pkg` built, §6.2 cases 1–7 and the existing 43 green, awaiting the §6.4 hardware test — which also captures the baseline tree milestone 2 needs |
| 2 | **`ENEMIES SKIPPED`.** The generated 85-row table including the D6 name override, the polarity parameter and the new selection type, the skip predicate at both engine call sites, the picker's instruction line and vocabulary, both screens, the config key, D1's removal of `UNCHANGED BELL MAIDENS`, and the four tool updates | `.pkg` built, §6.2 cases 8–26 green plus `table skip` and `ui_scroll_verify`, awaiting the §6.5 hardware test |

**Why this order.** The failure-path work is a dependency of the list, not a
follow-up to it. Spec §4 F10 says the list widens a reachable failure "from one
combination to many" — after milestone 2, select-all on the new list reaches it
with one button and a confirmation. Landing the list first would mean handing a
hardware tester a build in which the easiest thing to try leaves a half-built
tree; landing the fix first removes the hazard before it is widened. It is also
the only order in which milestone 1 is independently testable: the starved case
is reachable **today** through `UNCHANGED BELL MAIDENS`, which milestone 2
deletes.

**Why not one milestone.** Milestone 1 changes shipped failure behaviour,
reverses a documented decision (`pickers.md` D12's backstop), and answers a
blocking review finding from another feature. It deserves its own build, its own
focused hardware test — including a deliberately broken vanilla source, which
nothing else in this plan exercises — and its own approval. Chaining it into the
list's milestone would bury a behaviour change to a shipped path inside a new
feature, which is what `CLAUDE.md` §4 exists to prevent.

**Why step 0 is not folded into milestone 1.** Because it had to happen on the
*other side* of the first edit. Once `EnemyRandomizer.cpp` changes, the tree it
would have produced no longer exists anywhere — not on disk, and not in `git`,
because the relevant sources are uncommitted working-tree modifications. Making
it step 1 of the milestone would have put it after the code change in practice,
which is precisely the mistake the stage D review caught (finding 2). The same
argument now applies to the one part of it still outstanding: **the seed has to
be written down while the run that produced it is still the run in front of
you.** A tree whose seed is guessed is a tree that cannot fail a diff for a
reason anyone can read.

**Why not three.** The obvious third split — table and tools first, then engine
and UI — produces a milestone with nothing to hardware-test: a generated header
that nothing includes changes no behaviour and no `.pkg`. Row 16's plan §7 made
the same call for the same reason.

Milestone 2 is not small, but it does not divide into hardware-testable halves:
the table without the picker cannot be exercised, and the picker without the
engine is a list that does nothing.

**Note on spec §6.** Scope says D4 "ships with the feature rather than after
it". Two milestones satisfy that — both land before the feature is documented or
declared done — but milestone 1 alone is a build-and-test stop, **not a
shippable state of the feature**. The developer confirmed the split and the
order on 2026-09-18 (§9, P12).

---

## 8. Open questions

*Deliberately empty. Q1–Q5 were answered on 2026-09-18 and moved to §9 as
P12–P16; P17 records a spec gap the developer resolved in the same pass. Q6,
raised by the stage D review response of 2026-09-19, was answered by the
developer the same day and moved to §9 as **P21** — the pre-implementation
baseline tree was captured rather than declared to exist already. **P18 was put
to the developer in the same pass and accepted**, so it is a confirmed decision
rather than a planner's call. The heading is kept so the numbering matches every
other plan and every cross-reference to §9.*

*Nothing is outstanding. §6.4 step 0's seed — the one transcription this plan
still owed — is `1234567890`, recovered from the run's `live.log` and written
into §6.4 on 2026-09-19.*

---

## 9. Decisions

| Date | Decision |
|---|---|
| 2026-09-18 | **P1 — the 85-row table is generated fresh, never derived from the shipped 82-row one.** `gen_pool_table.py` gains a `skip` kind; `EnemyPoolTable.h` is not read, altered or regenerated by any part of this work, and `pool_verify.py selftest` asserts that it is still 82 rows and is not compared against anything the new list computes (spec §7, §4 F2) |
| 2026-09-18 | **P2 — the polarity is a template parameter, not a second type.** `ModelPoolSelection<N, bool DefaultSelected = true>`; the default preserves both shipped pickers exactly, and the new type is `ModelPoolSelection<85, false>`. Rejected: a duplicated `ModelSkipSelection` (two copies of a positional encoder), and reusing the existing polarity with inverted call sites (spec §7 forbids it) |
| 2026-09-18 | **P3 — D4 is implemented as two pools built during the read phase and one choice made before the shuffle.** The pool half of the skip list yields; the placement half is untouched. A no-op selection draws no extra randomness, so spec §8 assertion 4 holds by construction |
| 2026-09-18 | **P4 — the empty-pool causes are separated into three, not two.** No maps read, no eligible candidates at all, and a starved selection. The first two fail with distinct messages; only the third proceeds. No draw-site guard is introduced, because no empty pool reaches a draw site — row 16's review finding 2.2 does not recur |
| 2026-09-18 | **P5 — the picker gets an optional instruction line via spec F13's option B (11 visible rows).** Chosen over option A because it leaves the heading and count-line constants untouched — so the two pickers this feature must not disturb keep their exact pixels — and because it keeps the `MORE ABOVE` clearance at 27px instead of 14px. 85 rows is 8 pages either way, so the cost is one row of the window and nothing else |
| 2026-09-18 | **P6 — the picker's vocabulary becomes a `PickerStrings` parameter passed to both `Update` and `Draw`.** Not stored in the component: `Update` needs the layout too, and a required parameter makes a missed call site a compile error |
| 2026-09-18 | **P7 — "frozen" is defined once, correctly, and the plain-equality form is never written.** Same model and think; npc either unchanged or the zone-tuned variant. `enemy_lookup.py` gains an exact mirror of `ApplyBossParamScaling`, justified by two measurements (each tracked value appears exactly once; no sequential chaining on real data) and cross-checked against spec F6's 1,369 |
| 2026-09-18 | **P8 — `pool_verify.py maidens` is retired with the setting it verifies, not corrected.** D1 deletes `UNCHANGED BELL MAIDENS`, so a checker for it is dead weight; the general `skipped` command supersedes it with the corrected definition. This is not the out-of-scope fix spec §6 declines — the wrong expectation is deleted along with its subject, and the measurement behind it (26 of 42 in scaled maps) is pinned as a selftest case so it cannot be silently reintroduced |
| 2026-09-18 | **P9 — `BellMaidenExclusionList()` and `IsExcludedEnemyName`'s second parameter are deleted with D1.** `M28ForcedMaidenList()` stays: the override is unconditional and survives. `c1055` matched zero placements across all 43 map files, so deleting it changes nothing measurable; the fact is kept as a selftest case rather than as dead data |
| 2026-09-18 | **P10 — `ENEMIES SKIPPED` is not added to `StartCommit`'s commit `\|\|` chain, and D5's two refusals are untouched.** It is a modifier on `RANDOMIZE ENEMIES`, like `randomizeWorkshopTools` and both pickers; ticking it alone must not launch a run that does nothing |
| 2026-09-18 | **P12 — developer's answer: two milestones, failure path first.** The order in §7 stands. The starved case is only independently testable while `UNCHANGED BELL MAIDENS` still exists, and the fix lands before the list makes starvation a one-button action. Milestone 1 remains a build-and-test stop rather than a shippable state of the feature, and §7's note says so |
| 2026-09-18 | **P13 — developer's answer: the flag column reads `SKIPPED` / `-`.** The confirm prompt is `SKIP ALL 85` / `SKIP NONE 85` and the footer `SQUARE SKIP ALL   TRIANGLE SKIP NONE   O BACK`. `YES`/`NO` under a heading reading `ENEMIES SKIPPED` is the ambiguity the instruction line exists to remove. Measured: the row grows 45 → 49 characters, 1,323px of 1,920, still centred, every glyph in `Font8x8.cpp` |
| 2026-09-18 | **P14 — developer's answer: the assertion-5 baseline tree is captured during milestone 1's hardware test.** §6.4 step 4 is confirmed, not optional: seed `1234567`, `RANDOMIZE ENEMIES` and `UNCHANGED BELL MAIDENS` on, everything else default, tree kept under `data/runs/`. That build is the last one that can produce it, and without the tree spec §8 assertion 5 needs an old build resurrected from git |
| 2026-09-18 | **P15 — developer's answer: the broken-source failure keeps today's `options.randomizeEnemies` gate.** A bosses-only, treasure-only or drops-only run against an unusable vanilla source stays as quiet as it is today. Milestone 1 changes no run outside this feature's scope. The wider gap — a broken source is silent for every non-enemy randomizer — is recorded as a separate backlog item, not fixed here |
| 2026-09-18 | **P16 — developer's answer: the `ENEMIES SKIPPED` row sits directly after `ENEMIES INCLUDED`** on both the Setup Defaults screen and the Enable wizard. The two enemy lists a player must not confuse are adjacent and differ by one word; `BOSSES INCLUDED` stays last, so the drill-in rows stay grouped. Both screens remain at 15 rows (one out for D1, one in), so no geometry changes and `ui_scroll_verify.py`'s three 15-row entries stand |
| 2026-09-18 | **P17 — developer's answer: `c1130`'s row reads `C1130 OEDON CHAPEL DWELLER`**, not the `Characters.json` name "Labyrinth Ritekeeper". Recorded in the spec as D6; the generator carries the override as project-owned data, marked as ours. This makes spec §8 hardware step 2 refer to a string that actually appears on screen |
| 2026-09-18 | **P11 — the starved-selection warning is a progress line, not a commit-time check.** Spec §7 allows either. The engine already knows the answer for free once the pool is built; computing it in the UI would put a pool rule in the UI layer where it can drift from the engine |
| 2026-09-19 | **P18 — the new failure messages are written inside `Font8x8.cpp`'s character set, in the `Fail()` strings themselves, and the fixed prefix loses its colon.** `Font8x8.cpp` has no lowercase and no `:`, and an unrenderable character draws as a full-width blank column (§2.3), so a message can meet every length budget and still show the player nothing. Of the three places the case could be fixed, uppercasing at the UI was rejected because it changes 11 other error paths without actually fixing them (`:` and `.` in filenames stay blank) and needs a new UI-layer helper; a separate display string on `EnemyRandomizerResult` was rejected because its invariant is "every future `Fail` remembers to set me", whose failure mode is the very invisibility being fixed. `"ENEMY RANDOMIZATION FAILED: "` becomes `"ENEMY RANDOMIZATION FAILED - "` (29 characters, the separator this screen already uses elsewhere), which **tightens the message budget from 43 to 42 characters**. §6.2 case 6 asserts the character set and the length together, in the failing direction, parsed out of the source. Taken by the planner in response to stage D finding 1, which was blocking and which identified the decision but explicitly left it to the plan. **Put to the developer on 2026-09-19 and accepted** — the prefix change was the part with a visible cost, and the developer chose ` - ` over keeping the colon, so this is a confirmed decision rather than a planner's call. The 42-character budget stands |
| 2026-09-19 | **P19 — an assertion-4 baseline tree is captured before milestone 1's first code change, as required work.** §6.4 step 0. Spec §8 assertion 4 had no check at all and §6.5 step 9 compared a tree against itself; §3.3's "holds by construction" argument cannot detect a stray `RandInt` or a mis-scoped dedupe set. The baseline can only come from the build in hand, because the pre-change sources are uncommitted and `git` cannot restore them. I checked every tree under `data/runs/` and none can serve — all predate row 16 and none has a recorded seed or settings — so the step is written as work to do, with §8 Q6 asking the developer whether a tree exists outside the repository. Taken in response to stage D finding 2, which was blocking. **Answered the same day — see P21: the tree was captured rather than found** |
| 2026-09-19 | **P20 — the output-tree pool assertion is "no *eligible* placement was *changed* to a skipped model", and boss-written placements are out of its scope.** The literal form ("no placement anywhere uses a skipped model") is false on a correct implementation: six of the 85 rows — `c1060`, `c2090`, `c2100`, `c2120`, `c2500`, `c2710` — also own placements the fixed exclusion list already protects, which stay put when the row is ticked (§5.10). A check that fails on the happy path is worse than no check, because it trains its reader to ignore it. **This decides only what the checker asserts.** The behavioural question underneath it — whether `ENEMIES SKIPPED` should also filter the boss pool, given that `c2090` and `c2710` are in both `BossPoolTable.h` and the 85 rows, and that spec §10 D4 calls its exception "the only case" — is a **spec** decision and is not taken here; it has been reported to the developer, and §5.10 records what would change in the plan if it is answered the other way. Taken in response to stage D finding 3 |
| 2026-09-19 | **P21 — developer's answer to Q6: the pre-implementation baseline was captured rather than found.** No usable older tree existed, so the developer made one the same day: `data/runs/20260919-Enemies Only`, before any milestone 1 code change. This is P19 satisfied and the recommended answer taken. **Its settings were verified from the tree's contents rather than trusted** (§6.4 step 0's table): 43 `.msb.dcx` present with exactly the 24 base maps differing and all 19 chalice maps identical; 1,974 of 2,269 eligible placements re-modelled at rates that track each map's zone chance; 0 reference-excluded placements re-modelled, so bosses were off; 0 of 73,125 non-enemy MSB parts changed, so treasure was off; `param/` byte-identical, so every param feature was off; `common.emevd.dcx` carrying `PermaDarkness`'s OFF pattern; 50 of 54 maidens re-modelled, so `UNCHANGED BELL MAIDENS` was off; all 82 pool models placed, so `ENEMIES INCLUDED` was full. That is exactly "`RANDOMIZE ENEMIES` on, everything else default". **The one thing the tree does not carry is its seed**, which is recorded nowhere and which §6.4 step 0 leaves a blank for; filling it in is a precondition of milestone 1's first code change, and it also replaces the `1234567` P14 named, so that all five trees in §6.4 and §6.5 share one seed |

---

## 10. Changes from the plan

*No implementation has begun, so this section records only changes to the plan
document itself. It stays available for implementation deviations from the first
code change onward.*

### 2026-09-19 — stage D review response (`/refine-plan 32`)

`plan-review.md` returned **CHANGES REQUESTED**. This pass answers findings 1–7;
finding 8 concerns `log.md`, which the plan does not own. Every number the review
challenged was re-measured against `data/vanilla/dvdroot_ps4` and the working
tree rather than taken on trust.

| Finding | Where it landed | Kind |
|---|---|---|
| **1 (blocking)** — failure messages budgeted by width, not by the font | New §2.3 paragraph on the glyph table and the blank-column mechanism; §2.4 names `gen_pool_table.RENDERABLE` as the predicate; §3.4 rewritten with the uppercasing decision, its two rejected alternatives, the prefix change and the concrete strings; §5.11 scopes out the 11 other messages; §6.2 case 6 restated over characters as well as length; §7 and §4.1 follow the 43 → **42** budget | **New planning decision (P18).** No approved decision altered |
| **2 (blocking)** — assertion 4 has no check and its baseline expires | New §6.4 step 0 and a step-0 row in §7; §6.4 step 1 and §6.5 step 9 now diff against it; §3.3's by-construction bullet says why an argument is not a check; §6.3 gains the assertion-4 paragraph; §8 Q6 asked whether a usable tree already existed | **New planning decision (P19) plus one question, answered the same day — see the second pass below.** P14's substance is untouched: §6.4 step 4 still captures assertion 5's baseline during milestone 1, exactly as the developer answered |
| **3 (should fix)** — six rows keep reference-excluded placements; two are in the boss pool | New §5.10 with both measurements; §6.3 assertion 2 restated; §5.9 re-scoped to the placement half; §6.5 step 2 turns bosses off and new step 12 observes the overlap | **New planning decision (P20)** on what the checker asserts. **The behavioural question is left to the spec** and reported to the developer; spec §10 D4 is not touched |
| **4** — hardware step named a row that will not exist | §6.5 step 3 now reads `C1130 OEDON CHAPEL DWELLER`. Swept the rest: steps 2, 5, 10 and 12 now name rows as they will appear (`C1170 CARRION CROW`, `C1050 CHIME MAIDEN`, `C1051 CHIME MAIDEN (LIGHT)`, `C2090 BLOOD STARVED BEAST`, `C2710 FATHER GASCOINGE`), and §6.4 step 2 likewise | **Correction.** Brings the plan into line with spec D6 / P17 |
| **5** — the D6 name override was only in the risks section | Moved into §3.1 as required work and into §4.2's `gen_pool_table.py` row; §6.2 case 26 pins it; `c1130` measured absent from both pool tables; §4.2's `enemy_lookup.py` row now lists all six `bell`-carrying functions instead of three | **Correction.** No decision changed; P17 already required it |
| **6** — fallback and progress line not gated on `randomizeEnemies` | §3.3 cases 3 and 4 gated explicitly, with the reachability argument against `EnableWizardScreen.cpp:480`; §3.4's lines gated in `FinishCommit`; §4.1 states that the enemies-disabled `Log` at `:423` survives verbatim | **Correction** |
| **7** — chaining measured over the wrong value set | §3.6 restated over the 553 reachable values (414 of them tracked) at all fifteen scales, zero chaining; the structural 151–351 figure recorded; §6.2 case 22 restated to match | **Correction**, and the claim is now stronger than it was |
| **§5, number 1** — old message 70 characters, not 69 | §3.4 and §6.4 step 3: **70** characters, 98 on screen, 2,646px. The review's count is confirmed; its description of what is on screen is not. Simulating `DrawCenteredLabel` at `x = −363` gives the visible window as characters 14–83, so the line reads `ATION FAILED` plus 45 blanks — the left end is clipped too, not just the right | **Correction (count confirmed, description corrected)** |
| **§5, number 2** — 162 + 73, not 168 | §5.3 rebuilt around `StepWriteMap`'s two size gates (`:645-657`): **162** at the 30,000 cap and **73** at the 30-try cap, 235 in total. 168 is what a mirror gives if it drops the `m24_02`/`m35` branch — noted in §5.3 because §6.2 extends `randomizes()` as the single mirror of that decision | **Correction (review's numbers confirmed exactly)** |
| **§5, number 3** — option A's list band | §3.5's comparison table: option A's band is **unchanged at `{280, 52, 900}`**, 12 rows, hint at 234. I agree the row was not reproducible and disagree slightly about the fix: `{260, 52, 900}` would give 13 rows and a hint at 214, as the review says, but 214 sits above the instruction line's 220 bottom and would fail `ui_scroll_verify.py`'s G3 — and spec §4 F13's option A moves the *heading*, not the list. The wrong cell was the band | **Correction, with a noted difference from the review** |
| — | Line-number drift swept across the whole document, not only where the review pointed: `:390` → `:389`, `:626-632` → `:627-637`, `:399-484` → `:399-485`, `:616` → `:618`, `:652-657` → `:645-657`, `:157` → `:158-159`, `:557` → `:559`, `FinishCommit` at `:547`, and §2.1's two UI rows re-derived site by site (`.cpp:43, 96, 265, 362, 539, 729, 761` → `:44, 97, 271, 365-366, 539, 675, 731, 762`; `.cpp:40, 144, 195, 229` → `:41, 53, 144, 196, 229`). Two `` `\|\|` `` code spans inside tables escaped so the rows render | **Correction** |
| — | Header: spec now cited as **six** decisions D1–D6, the review linked with its verdict, status `QUESTIONS ANSWERED` → `QUESTIONS OPEN` for Q6 — **superseded within the day by the second pass below, which returns it** | **Correction** |
| — | §4.2: spec §4 F11's `478 → 556 (+103)` re-measured as `478 → 555 (+102)`; no consequence, recorded so plan and spec are not quietly one apart | **Correction to a spec number**, reported rather than changed in the spec |

### 2026-09-19 — second pass: Q6 answered, P18 confirmed

Same day, same `/refine-plan 32` dispatch. The developer answered the one
question the first pass opened and confirmed the one decision it took alone, so
the status returns to `QUESTIONS ANSWERED — awaiting developer approval` with §8
empty again.

| Change | Where | Kind |
|---|---|---|
| **Q6 answered: the baseline tree was captured, not found.** `data/runs/20260919-Enemies Only`, made by the developer on 2026-09-19 before any milestone 1 code change — the recommended answer, taken | New **§9 P21**; §8 emptied; header, §1, §6.3, §6.4, §7 reworded from "must be captured" to "has been captured" | **Developer answer recorded.** P19 is satisfied, not altered |
| **§6.4 step 0 rewritten from "make one" to "use this one"** — and given the one thing the tree does not carry. The tree holds 102 files, all `.dcx`: **no log, no cfg, nothing recording the seed or the settings.** The step now carries an explicit `SEED = ____________` blank to be transcribed from the run's `USING SEED <n>` line before the first code change, with the reason stated: a tree whose seed is guessed produces a diff whose failures cannot be interpreted, and a mismatch would be indistinguishable from the two-pool refactor perturbing the roll stream | §6.4 step 0; §7 step-0 row; §7's "why step 0 is not folded in"; §8's closing note | **Addition.** No decision changed |
| **The settings were re-derived from the tree rather than assumed**, and a new table in §6.4 records each one with its evidence. All nine determinable settings read as "`RANDOMIZE ENEMIES` on, everything else default" — exactly what step 1 needs. **Nothing found contradicts the plan** | New table in §6.4 step 0's preamble; summarised in P21 | **Addition** |
| **`S` restated as the seed of the `20260919-Enemies Only` tree**, replacing the literal `1234567` everywhere it appeared (§6.4 steps 1 and 4, §6.5 preamble and step 6, §7 step-0 row) | §6.4, §6.5, §7, plus an explicit note at the end of §6.4 | **Detail change to a developer-answered decision (P14), flagged not buried.** P14's substance — assertion 5's baseline is captured during milestone 1's hardware test, required and not optional — is untouched; only the seed literal moves, because step 0's tree exists at some other seed and keeping `1234567` would put the five trees on two seeds |
| **P18 confirmed by the developer**, who chose `"ENEMY RANDOMIZATION FAILED - "` over keeping the colon. The 42-character budget stands | §9 P18 gains the confirmation; header and §8 say so | **Planner decision → developer-confirmed decision.** No content change |

---

<!--
Not in this document, on purpose:

  - what the feature is and why it exists (that is docs/features/032-bypassed-enemies/spec.md)
  - the plan review's findings (stage D, its own file)
  - production code
-->
