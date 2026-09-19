# Plan 016 — Unchanged Bell Maidens

**Status: IMPLEMENTED — awaiting hardware test.** *(built 2026-09-16; §9.3
deferred out of this feature, see §10.1. Decisions in §9, 2026-09-15.)*

**Spec:** `docs/features/016-unchanged-bell-maidens/spec.md` — **APPROVED** (developer, 2026-09-15)
**Backlog row:** `docs/randomization-feature-spec.md` §5, row **16**
**Plan review:** `docs/features/016-unchanged-bell-maidens/plan-review.md` *(once stage 2 has run)*

---

## 1. What this builds

One new boolean setting, `UNCHANGED BELL MAIDENS`, default off, carried through
the existing chain (defaults struct → `defaults.cfg` → Setup Defaults row →
Enable wizard row → `EnemyRandomizerOptions` → engine), plus **the reference's
m28 six-placement override, which the port does not implement at all today.**

Spec §10.1 is the decision that makes this more than a three-string change: the
override is built unconditionally, exactly as the reference has it, so it takes
effect **with the new flag off as well**. Spec §10.2 fixes the label, §10.3
forbids touching the enemy picker, and §10.4 keeps the wrong justification
comment's *correction* out of row 16 — though the lines themselves are being
replaced regardless, because they describe code that no longer exists after
this change.

Everything about what the feature *does* is in the spec. This document is how.

## 2. What already exists

### 2.1 The settings chain to copy

`randomizeWorkshopTools` is the closest neighbour and the exact template: a
boolean, default off, that modifies another randomizer rather than being one.
Its full chain, which this feature mirrors file for file:

| Layer | File | What it holds |
|---|---|---|
| Randomizer | `app/src/Randomizer/RandomizerDefaults.h:69` | the persisted default |
| Randomizer | `app/src/Randomizer/RandomizerDefaultsStore.cpp:57,108` | `randomize_workshop_tools` key, load and save |
| Randomizer | `app/src/Randomizer/EnemyRandomizer.h:70` | the per-run option |
| UI | `app/src/UI/SetupDefaultsScreen.h:43` / `.cpp:84,217` | row constant, toggle branch, list entry |
| UI | `app/src/UI/EnableWizardScreen.h:108` / `.cpp:28,83,171,231,513,549,640,724` | row constant, ctor init, two toggle branches, option assignment, progress line, two list entries |

Two precedents inside that chain matter here and are followed below:

- **It is deliberately absent from `StartCommit`'s "does anything need doing"
  `||`** (`EnableWizardScreen.cpp:504-509`), because ticking a modifier alone
  must not launch a run that does nothing. `UNCHANGED BELL MAIDENS` is the same
  shape — it is meaningless without `RANDOMIZE ENEMIES`.
- **It reports itself by extending an existing progress line** rather than
  adding a `SKIPPING ...` line (`workshop-tools.md` D5), and with no parentheses,
  because `Font8x8.cpp` renders unknown glyphs as blank.

### 2.2 The engine, and the two places the exclusion list is consulted

`app/src/Randomizer/EnemyRandomizer.cpp` uses `IsExcludedEnemyName` in exactly
two places, and they are the two halves of this feature:

- **`:373`** — pool contribution, inside the per-map read loop. Excluded
  placements never become candidates.
- **`:611`** — the mutation loop, inside `StepWriteMap`. Excluded placements are
  `continue`d before anything else happens.

`:611-614` is the whole placement decision today:

    if (IsExcludedEnemyName(name)) continue;
    int roll = RandInt(0, 100);
    if (roll >= lm.zoneChance) continue;

Note the shape: **the port `continue`s on exclusion before the zone roll is
drawn.** The reference instead sets a `changeData` flag and lets every enemy
fall through the roll. That difference is pre-existing, load-bearing, and §3.3
explains why it must not be "fixed" as part of this work.

`lm.zoneChance` for both m28 maps is **100** (`EnemyRandomizer.cpp:114-115`),
matching the reference's `YahargulChance = 100`
(`reference/Randomizer/MainWindowComponents/FieldContainer.cs:24`). `RandInt(0,
100)` is inclusive at both ends (`:230-233`), so `roll >= 100` is true for
exactly one of 101 outcomes. That single fact sizes the entire flag-off
behaviour change — see §5.1.

### 2.3 The exclusion list itself

`app/src/Randomizer/EnemyExclusionList.h` holds a fixed
`std::array<const char*, 103>` transcribed verbatim from the reference, plus
`IsExcludedEnemyName` (plain substring test, mirroring `Name.Contains`). The
count is in the type, the file header documents the `RemoveAt(3)` reasoning, and
`app/tools/enemy_lookup.py:56-75` parses the array straight out of this header
so the mirror cannot drift from the port. All three properties are preserved
below.

### 2.4 Verifiers that already parse what this needs

- **`app/tools/enemy_lookup.py`** — `engine_pool()` is a deliberate
  line-for-line replay of the engine's contribution loop and is documented as
  the single source of truth ("Do not reimplement it"). `exclusion_reason()` is
  the mirror of `IsExcludedEnemyName`. Both need one optional parameter; neither
  needs rewriting. It has **no `selftest` subcommand** — its commands are
  investigative.
- **`app/tools/pool_verify.py`** — has the `selftest` harness, already imports
  `engine_pool`, `engine_pool_models`, `exclusion_reason`, `enemies` and
  `load_map`, and already owns the assertion that `EnemyPoolTable.h` *is* the
  engine's pool. This is where the new assertions go.
- **`app/tools/ui_scroll_verify.py:48-52`** — pins `kItemCount` and
  `kSaveDataRowCount` at 14 for three screens. Adding a settings row makes these
  15.
- **`app/tools/gen_pool_table.py`** — the generator for `EnemyPoolTable.h`.
  **It is not run in this change.** See §5.3.

### 2.5 Layering

Everything here lives in `Randomizer` (data + engine) and `UI` (two screens).
Nothing in the UI learns an AFR path, and nothing in `Randomizer` gains an SDL2
dependency. The new data tables are `Randomizer`-local headers. No new layer
crossing.

### 2.6 Prior plans worth reading first

`docs/plans/pickers.md` (§2.2 pool rules, D12 empty-pool guard),
`docs/plans/workshop-tools.md` (§4.4 config-buffer clamp, D3/D5 modifier
conventions), `docs/enemy-exclusion-history.md` (why the exclusion list is not
casually extended).

## 3. Approach

### 3.1 Two data tables, conditionally applied

Add two arrays to `EnemyExclusionList.h`, beside the existing one, each wrapped
in its own accessor and each using the local name `kList` so `enemy_lookup.py`'s
existing `_load_list()` parses them unchanged:

- `BellMaidenExclusionList()` — `"c1050"`, `"c1051"`, `"c1055"`, in the
  reference's own order (`StartFunctions.cs:45-49`).
- `M28ForcedMaidenList()` — `"c1050_0117"`, `"c1050_0115"`, `"c1050_0119"`,
  `"c1050_0110"`, `"c1050_0112"`, `"c1050_0114"`, in the reference's own order
  (`RandomizeFunctions.cs:288-320`).

`IsExcludedEnemyName` gains a second parameter, `bool unchangedBellMaidens`,
**with no default argument**. Both call sites are updated explicitly. A default
would let a future third call site silently get flag-off behaviour, which is the
one way this could go wrong quietly.

A new `IsM28ForcedMaidenName(const std::string&)` does the substring test
against the six.

Kept in `EnemyExclusionList.h` rather than a new header because it is the same
kind of data from the same reference source, and because it gives the Python
mirror one file to parse instead of two.

### 3.2 The placement decision — the exact new shape

`StepWriteMap`'s loop becomes:

    bool forced = isM28 && IsM28ForcedMaidenName(name);
    if (!forced && IsExcludedEnemyName(name, options.unchangedBellMaidens)) continue;
    int roll = RandInt(0, 100);
    if (!forced && roll >= lm.zoneChance) continue;

with `bool isM28 = lm.name.find("m28") != std::string::npos;` hoisted beside the
existing `isM2402` / `isM35` flags at `:600-601`.

Three properties make this the right shape, and they are the reason not to write
it any other way:

1. **The roll is still drawn for every non-excluded placement, exactly as
   today.** With the flag off, the RNG stream is bit-identical to the current
   build up to the first moment a forced placement actually rolls 100. Nothing
   else in any map shifts.
2. **`forced` bypasses both gates**, which is what the reference does: its
   override is the last writer to `changeData` before the write gate, after the
   exclusion test at `:26` and after the zone roll at `:193`.
3. **The pool is untouched by the override.** The reference's override lives in
   `Randomize()` only; `GenerateEnemyList()` applies `noNoList` plainly. So with
   the flag on, `c1050`/`c1051` leave the pool *including* for the six forced
   placements — which is precisely why §6.4's headline assertion is
   deterministic rather than probabilistic.

### 3.3 Considered and rejected

**A1 — gate the m28 override on the new flag.** This would avoid changing
shipped behaviour entirely. Rejected: spec §10.1 is binding and the reference
applies the override unconditionally; gating it would preserve the exact
deviation §10.4 documents as wrong.

**A2 — restructure the loop into the reference's `changeData` shape.** Tempting,
because it would read like the C# and make the override obvious. **Rejected, and
this is the most dangerous idea in the feature.** Measured against
`data/vanilla/dvdroot_ps4`: of 2877 enemy parts in the 24 base maps, **608 are
matched by `EnemyExclusionList()` today and 2269 reach the zone roll.** Under
`changeData` all 2877 would draw a roll, so 608 extra `RandInt` calls would enter
the stream and **every seed would produce a different world in every map** — a
total regression of shipped output, dressed up as a readability improvement.

**A3 — skip the roll for forced placements** (`if (!forced) { roll; ... }`).
Rejected: it removes 12 `RandInt` calls from every run that touches m28,
shifting the stream for both m28 maps and the six maps after them on *every*
seed, where §3.2's form shifts it on about 11% of seeds. Strictly worse for no
gain.

**A4 — hide, grey or reorder the two `CHIME MAIDEN` picker rows.** Rejected:
spec §10.3 decided the picker is left exactly as it is, and spec §7 explains
that row order is the meaning of every saved config.

**A5 — regenerate `EnemyPoolTable.h` when the flag is on.** Rejected for the
same reason, more forcefully: an 80-row table silently remaps all 82 saved
selection characters. See §5.3.

**A6 — append the three patterns into the existing 103-entry array and filter at
use.** Rejected: the array's size is part of its type, its header comment
documents the exact 66/38/-1 provenance, and `enemy_lookup.py` parses it as the
reference's list. A separate three-entry list keeps "what the reference always
excludes" and "what this checkbox adds" visibly distinct.

### 3.4 Scope decisions taken by precedent, not reopened

- The setting appears on **both** the Setup Defaults screen and the Enable
  wizard, like every other setting.
- It is **excluded from `StartCommit`'s `||`**, like `randomizeWorkshopTools`
  (D3): ticking it alone must not start a run.
- It reports by **extending the existing enemy line** (D5):
  `RANDOMIZED n ENEMIES ACROSS m MAPS BELL MAIDENS UNCHANGED`. At 61 characters
  and progress scale 3 (27 px per character of 1920) this fits with room to
  spare. No parentheses; no mention of the Yahar'gul six, which does not fit on
  one line and belongs in `docs/user-guide.md` at documentation stage.
- The comment at `EnemyRandomizer.cpp:20-23` is **rewritten to describe what the
  code now does**, with no commentary on why the old reasoning was wrong. That
  is the minimum the change forces, and it respects spec §10.4, which reserves
  *fixing the record* for separate work.

## 4. Changes, file by file

No new files. No file-format layout change, but `RandomizerDefaults` and
`EnemyRandomizerOptions` both gain a member, so **CLAUDE.md §2's full clean
rebuild applies** — see §6.1.

| File | Change |
|---|---|
| `app/src/Randomizer/EnemyExclusionList.h` | Add `BellMaidenExclusionList()` (3 entries) and `M28ForcedMaidenList()` (6 entries), each an `inline const std::array<const char*, N>&` returning a local `kList`, each with a header comment citing its reference line numbers. Add `IsM28ForcedMaidenName()`. Give `IsExcludedEnemyName` a second parameter `bool unchangedBellMaidens`, **no default**, checking the three extra patterns only when true. |
| `app/src/Randomizer/EnemyRandomizer.h` | Add `bool unchangedBellMaidens = false;` to `EnemyRandomizerOptions`, with a comment stating that it gates the exclusion only and **not** the m28 override, which is unconditional. |
| `app/src/Randomizer/EnemyRandomizer.cpp` | (a) Rewrite the m28 bullet in the file header comment (`:20-23`) to describe the implemented override. (b) `:373` pass `options.unchangedBellMaidens`. (c) `:600` hoist `bool isM28`. (d) `:611-614` replace with §3.2's four lines. (e) **§9.3:** remove the up-front `Fail` at `:407-409`, guard every site that indexes `pool` instead, and rewrite `:401-406`'s "Do not relax this" comment to say where the guard moved. |
| `app/src/Randomizer/RandomizerDefaults.h` | Add `bool unchangedBellMaidens = false;` with the standard absent-key-means-false comment. |
| `app/src/Randomizer/RandomizerDefaultsStore.cpp` | Add `unchanged_bell_maidens` to the load chain (`:57` area) and one `%d` plus argument to the save `snprintf` (`:95` area). The existing `len > sizeof(buf)` clamp already covers the ~25 extra bytes; the buffer is 1024 and the current worst case is about 450. |
| `app/src/UI/SetupDefaultsScreen.h` | `kItemCount` 14 to 15; insert the new row constant at **index 4**, under `RANDOMIZE ENEMIES`, and renumber the ten constants below it (§9.1). |
| `app/src/UI/SetupDefaultsScreen.cpp` | Add the `ToggleRow` branch with its `defaults: unchanged bell maidens = YES/NO` log, and the `"UNCHANGED BELL MAIDENS   "` entry to `DrawList`'s `items`. |
| `app/src/UI/EnableWizardScreen.h` | Add `bool unchangedBellMaidens_;` beside the other per-run toggles. |
| `app/src/UI/EnableWizardScreen.cpp` | New row constant; `kSaveDataRowCount` 14 to 15; ctor init from defaults; left/right branch; X branch; entry in **both** `DrawSaveData` and `DrawConfirm` item lists (they must stay identical — `ui_scroll_verify` treats them as one shape); `options.unchangedBellMaidens = unchangedBellMaidens_;` in `StartCommit`, **not** added to the big or-chain that decides whether a run starts; extend the enemy result line in `FinishCommit`; leave the D12 `NoneEnabled()` selection guard untouched — §9.3 replaced the guard-extension approach with matching the reference instead. |
| `app/tools/enemy_lookup.py` | Parse `BellMaidenExclusionList()` and `M28ForcedMaidenList()` out of the header with the existing `_load_list`. Add `bell=False` to `exclusion_reason`, `engine_pool`, `engine_pool_models`. Add an optional `--bell` argument to `frozen` and `diff`. |
| `app/tools/pool_verify.py` | New selftest cases (§6.2) and a new `maidens` subcommand taking vanilla, output and the flag state (§6.4). |
| `app/tools/ui_scroll_verify.py` | Row counts 14 to 15 for `Setup Defaults`, `Wizard SaveData`, `Wizard Confirm`; update the comment that states why. |

**Explicitly not changed:** `EnemyPoolTable.h`, `gen_pool_table.py`,
`ModelPoolSelection.h`, `BossList.h`, `BossPoolTable.h`, `ModelSizeTable.h`,
`docs/randomization-feature-spec.md`, `docs/user-guide.md`, anything under
`reference/`.

## 5. Risks and unknowns

### 5.1 Building the override changes shipped output with the flag off — sized

This is the risk the developer needs before approving, so it is stated with
numbers rather than adjectives.

**What changes.** Today, each of the 12 forced placements (6 in
`m28_00_00_00`, 6 in `m28_00_00_01` — the lists are identical, measured) is
skipped when its zone roll returns 100. After this change it randomizes anyway.

**How often.** `roll >= 100` on `RandInt(0, 100)` fires on 1 outcome of 101.
Per run: expected **0.119** forced placements fire; **P(at least one) =
1 - (100/101)^12, about 11%**.

**Blast radius when one fires.** The forced six sit at enemy-part positions
17-22 of 145 in `m28_00_00_00` and 19-24 of 147 in `m28_00_00_01`, and those
maps are 17th and 18th of the 24 the engine walks. A fire consumes extra
`DrawCandidate` draws, so the remainder of that map and every map after it draw
a different stream. So the honest statement is: **with the flag off, roughly one
seed in nine now produces a different — still valid — world than the same seed
produced before this change.** Same-seed reproducibility across app versions has
never been a promised property (the pickers already broke it), but it should not
come as a surprise.

**Why it is not hardware-observable, and what is done instead.** With the flag
off the pool still contains `c1050` (weight 4 of 333), so a forced placement can
be redrawn as a chime maiden by chance either way; and in about 99% of rolls the
old and new code take the same path. **No single output tree can distinguish the
two builds.** So this change is verified by (a) the Python mirror pinning the
rule, (b) code review of four lines, and (c) a flag-off hardware run treated as
a *regression* check — Yahar'gul still populates, the run completes, counts are
normal — not as proof the override fired. §6.5 step 7 says so in the test steps
rather than letting a passing run be mistaken for confirmation.

### 5.2 A new way to reach an empty pool — resolved by matching the reference

With the flag on, `c1050` and `c1051` leave the pool. If the enemy picker has
**only** those two rows ticked, the pool becomes empty. Today that reaches
`StepBuildPool`'s `Fail("enemy pool is empty ...")` *after* the mirror phase has
copied most of the game into the AFR folder, leaving a half-built tree.

**§9.3 resolves this by removing the failure rather than guarding it.** The
reference has no up-front check at all: `RandomizeFunctions.cs:322` gates each
write on `enemyDataRandomized.Count > 0` and skips. An empty pool there means
the run finishes normally having randomized nothing. Matching that means nothing
fails, so nothing can fail halfway, and the half-built tree stops being
reachable by this route or any other.

**What survives as a risk**, and it is a real one: the port's draw is
`pool[RandInt(0, size-1)]`, and `RandInt(0, -1)` is undefined behaviour. The
reference is safe here only because C# throws rather than corrupting. Removing
the up-front `Fail` without guarding **every** site that indexes `pool` converts
a clean error message into memory corruption — the worst possible trade. The
comment at `EnemyRandomizer.cpp:401-406` currently says "Do not relax this";
it must be rewritten to explain where the guard moved, not deleted.

This is a change to already-shipped behaviour reversing a deliberate decision
from `pickers.md` D12. Like the m28 override it is sequenced inside the single
milestone, and like the override it needs its own hardware step (§6.5) and its
own entry in §10.

### 5.3 The picker table must not be regenerated — the one way this does real damage

`EnemyPoolTable.h`'s 82 rows are positional, and a saved selection is one
character per row (`pickers.md`). With the flag on the engine pool computes to
80 models, so anyone running `gen_pool_table.py` in a flag-on frame of mind
would produce an 80-row table and silently remap every saved config. The table
stays at 82 rows unconditionally; `pool_verify.py table both` must keep passing
against the flag-off pool, and §6.2 adds a case asserting the table is **not**
compared against the flag-on pool.

### 5.4 Smaller risks

- **`c1055` is inert.** Zero placements across all 43 `.msb.dcx` files; kept for
  fidelity. Pinned by a selftest case so a future data set containing `c1055`
  is noticed rather than silently changing behaviour.
- **`IsExcludedEnemyName`'s new parameter.** No default argument, so a future
  call site that forgets it fails to compile instead of quietly disabling the
  feature.
- **Boss identification is untouched.** `BossList.h` is a separate array;
  `c1050`/`c1051`/`c1055` are not bosses, and the boss pass does not consult
  `IsExcludedEnemyName`.
- **The model merge is unchanged.** `allEnemyModelNames` is gathered from map
  *model* tables, not parts, so `c1050`/`c1051` still get merged into every map
  and no MSB model list shifts. Only which placements point where changes.
- **A Python mirror pins the rules, not the C++ implementation of them.** The
  two can drift; this is the standing, accepted weakness of this setup.

### 5.5 Nothing here can make a run unwinnable

Bell maidens hold no items and gate no progression. A frozen maiden is
byte-identical to vanilla in all three written fields, so the worst case for the
frozen half is "the game is slightly more like vanilla". The one genuinely
unknown behaviour — whether a frozen maiden's bell still summons and resurrects
— is spec §4's stated assumption and is a hardware observation (§6.5 step 5),
not something bytes can settle (CLAUDE.md §5).

## 6. Verification

### 6.1 Build

    cd app
    rm -rf src/x64
    make

**The clean rebuild is required, not optional.** `RandomizerDefaults` and
`EnemyRandomizerOptions` both gain a member, which is a struct layout change,
and CLAUDE.md §2 records that a stale partial rebuild once produced a real
heap-corruption SIGSEGV on hardware.

### 6.2 `pool_verify.py selftest` — new cases

Run as `python tools/pool_verify.py selftest ../data/vanilla/dvdroot_ps4`. Each
case asserts a rule, in both directions where a direction exists.

1. **flag-off pool is unchanged** — 333 entries, 82 models (this is the existing
   "baked table matches the engine pool" case; it must keep passing untouched).
2. **flag-on pool is 317 entries and 80 models.**
3. **the two models lost are exactly `c1050` and `c1051`.**
4. **333 - 317 == 16 == the baked weights of those two rows**
   (`EnemyPoolTable.h:49-50` gives `c1050` = 4, `c1051` = 12) — cross-checks the
   mirror against the baked table.
5. **the baked table still has 82 rows, and is NOT compared against the flag-on
   pool** — the §5.3 hazard, asserted rather than assumed.
6. **all 54 maiden placements are matched with the flag on, and none with it
   off.**
7. **`c1055` matches zero placements** across the 24 base maps — records the
   pattern as deliberately inert.
8. **the six forced names resolve to exactly 12 placements**, all in the two m28
   maps, all `NPCParamID 105810`, and zero placements outside m28.
9. **all six are matched by the flag-on exclusion test**, so the override is
   doing real work rather than being a no-op.
10. **the six names appear verbatim in
    `reference/Randomizer/MainWindowComponents/RandomizeFunctions.cs`** — a
    fidelity pin, following the precedent of `starting_weapons_verify.py`
    parsing the reference C# directly. Note this couples the selftest to that
    path, which CLAUDE.md §1 already fixes in place.

### 6.3 Other existing verifiers

    python tools/pool_verify.py table both ../data/vanilla/dvdroot_ps4    # must still PASS
    python tools/ui_scroll_verify.py                                      # 15 rows on three screens
    python tools/enemy_lookup.py frozen ../data/vanilla/dvdroot_ps4           # 608 frozen, unchanged
    python tools/enemy_lookup.py frozen ../data/vanilla/dvdroot_ps4 --bell    # 662 frozen

`608 + 54 = 662` is the arithmetic the last line checks by inspection.

### 6.4 New subcommand, for after a hardware run

    python tools/pool_verify.py maidens <vanilla_dvdroot> <output_dvdroot> on|off

With `on`, it asserts the feature's whole contract from the output tree alone:

- **all 12 forced placements differ from vanilla in model.** This is
  deterministic, not probabilistic: `c1050` is not in the flag-on pool, so a
  forced placement cannot be redrawn as a maiden.
- **the other 42 maidens** (54 - 12; **28 of them in maps the retail game
  loads**) are identical to vanilla in all three written fields.
- **no placement anywhere in the output uses model `c1050` or `c1051`** except
  those 42 — the pool half of the feature.

With `off`, it reports the same counts but asserts only that the trees are
structurally comparable, and **prints plainly that it cannot prove the override
fired** (§5.1).

### 6.5 Hardware — what the developer runs

Three trees: vanilla `V`, seed `S` with the flag **off** giving `A`, the same
seed `S` with the flag **on** giving `B`. Vanilla as the third input is mandatory
(CLAUDE.md §3): two randomized trees alone cannot tell "frozen" from "redrew the
same model".

| # | Step | Pass looks like |
|---|---|---|
| 1 | `pool_verify.py maidens V B on` | PASS |
| 2 | `enemy_lookup.py diff V B m28_00_00_01` | the 6 `c1050_011x` changed, the other 9 maidens unchanged |
| 3 | Play `B` in Yahar'gul | maidens are still maidens **except about six**, which are something else. Six changed is the **correct** result, not a failure (spec §10.1) |
| 4 | Play `B` elsewhere — Central Yharnam, Research Hall | **no** kneeling bell-ringing woman anywhere she was not in vanilla |
| 5 | Play `B`, stand near a frozen maiden | her bell still summons or resurrects as in vanilla — spec §4's assumption, and only the console answers it |
| 6 | Play `A` (flag off) | maidens **are** replaced, and maidens **do** appear elsewhere. This is what makes steps 3 and 4 mean anything |
| 7 | Regression for §5.1, on `A` | the run completes, Yahar'gul populates normally, the enemy count is in its usual range. **This cannot confirm the override fired** — record it as a regression check, not as proof |
| 8 | With the flag **on**, tick **only** the two `CHIME MAIDEN` rows in `ENEMIES INCLUDED` and run | §9.3's case: the run **completes normally** having randomized no enemies — no error, no half-built tree. Previously this failed after the mirror phase |
| 9 | Reopen Setup Defaults after saving | the new row persists, and `ENEMIES INCLUDED` still reads `82 OF 82` — a changed count would mean the config string was misread |

**Failure would look like:** maidens still replaced with the flag on (the
exclusion did not reach `:611`); a saved picker selection meaning something
different afterwards (§5.3); any failure at all from the only-maidens-selected
case, which should now complete as a no-op (§9.3) — and above all a crash or hang
there, which would mean the draw-site guard was missed and `RandInt(0, -1)` ran; or a maiden frozen in place but no longer ringing, which would mean
something beyond the three written fields matters and would invalidate more than
this feature.

Building cleanly and passing every selftest means **ready for hardware test**,
never done.

## 7. Milestones

One milestone. The feature is a boolean through a chain with eight existing
precedents plus four lines in one loop, and CLAUDE.md §4 says one milestone is a
fine answer.

| # | Milestone | Ends with |
|---|---|---|
| 1 | `UNCHANGED BELL MAIDENS`: the two data tables, the conditional exclusion at both engine call sites, the unconditional m28 override, the defaults/store/two-screen chain, the empty-pool change of §9.3, and the three verifier updates. Clean rebuild, §6.2-6.4 green. | `.pkg` built, awaiting the §6.5 hardware test |

**Why the m28 override is not its own milestone**, given that it changes shipped
flag-off behaviour: a milestone must end in something the developer can
hardware-test, and this one cannot. Built alone it has no UI to exercise, and its
effect is invisible on any single seed (§5.1). A dedicated stop would buy a
`.pkg` and a hardware cycle to learn nothing. Instead it is sequenced inside this
milestone, verified by mirror and review, and given an explicit named regression
step (§6.5 step 7) that is labelled as a regression rather than a confirmation.

§9.2 records this as taken on the developer's behalf: their answer, "do what the
Windows version does", has no analogue here — the Windows tool is one desktop
application with no milestone structure. Revisitable at no cost before
implementation begins.

## 8. Questions for the developer

*All answered and moved to §9. Section kept as a heading only so the numbering
stays stable, since this document and `.claude/commands/plan.md` cite sections
by number.*

## 9. Decisions taken

All three answered by the developer on **2026-09-15**.

**1. `UNCHANGED BELL MAIDENS` is inserted as row 4**, directly under
`RANDOMIZE ENEMIES`, grouping the modifier with what it modifies — the way
`RANDOMIZE WORKSHOP TOOLS` sits under `RANDOMIZE TREASURE`.

Ten row-index constants renumber across the two screen files. The indices are
compile-time constants carrying no saved meaning — only `EnemyPoolTable.h`'s row
order is load-bearing for saved configs (§5.3) — so this is a mechanical rename
with no compatibility consequence. The plan's §4 previously assumed the append
option; it has been corrected.

**2. One milestone, both changes in one build.**

The developer's answer was "do what the Windows version does", which does not
resolve this one: the Windows tool is a single desktop application with no
milestone structure, so there is nothing to copy. The question is about this
project's build cadence, not the randomizer's behaviour. Taken as **one
milestone** on the reasoning already in §7 — a stop must end in something the
developer can hardware-test, and the override built alone has no UI to exercise
and no single-seed observable effect (§5.1). Flagged to the developer as a call
made on their behalf, revisitable at no cost before implementation starts.

**3. The empty-pool case now matches the reference exactly: no up-front guard,
skip at the point of use.**

This reverses §5.2's original mitigation, and it is the largest change any
decision made to this plan.

*What the reference does*, checked directly: `RandomizeFunctions.cs:322` gates
every placement write on `enemyDataRandomized.Count > 0` and simply skips when
the pool is empty. There is no pre-run validation anywhere in
`StartFunctions.cs` — no message box, no abort. An empty pool means the run
completes normally having randomized nothing.

*What the port does today:* `EnemyRandomizer.cpp:407-409` calls
`Fail("enemy pool is empty ...")` up front, and the comment at `:401-406` says
**"Do not relax this"**, because the port's draw is `pool[RandInt(0, size-1)]`
and `RandInt(0, -1)` is undefined behaviour.

*The decision:* remove the up-front `Fail` and guard the draw instead, so an
empty pool randomizes nothing and the run finishes cleanly. This dissolves
§5.2's half-built-tree problem rather than guarding against it — nothing fails,
so nothing can fail halfway.

Three things the implementer must carry, because this is not a like-for-like
swap:

- **The UB hazard is real and does not go away.** The reference is safe only
  because C# would throw rather than corrupt; the port must guard the draw site
  itself. Every call that indexes `pool` needs the check, not just the one in
  `StepBuildPool`. `:401-406`'s comment should be rewritten to say why the guard
  moved, not deleted.
- **This reverses a shipped, deliberate decision** made in `pickers.md` D12 and
  documented in-code. It is a change to already-shipped behaviour arriving
  inside row 16, exactly like the m28 override, and it wants its own line in
  §6.5's hardware run and its own entry in §10.
- **`NoneEnabled()` at commit time is untouched by this.** It refuses an empty
  *picker selection*, which is a different condition from an empty *pool*. It
  stays as it is.

## 10. Deviations from the plan as written

Implemented 2026-09-16. Clean rebuild, `.pkg` built, every pre-hardware
verifier green. Seven deviations, one of them large.

### 10.1 §9.3 (the empty-pool change) was DEFERRED out of row 16 — developer decision, 2026-09-16

Asked in response to plan-review finding 2.1, which showed that the `Fail` §9.3
removes is also **the only hard error the enemy path has for an unusable
`VanillaSource`**. The developer chose to defer rather than to split the two
causes or to accept the loss.

**What this means, concretely:**

- `EnemyRandomizer.cpp`'s `Fail("enemy pool is empty ...")` and its
  "Do not relax this" comment are **unchanged**. §4 row (e) was not applied.
- **Plan-review finding 2.2 is moot** — there is no replacement draw-site guard
  to specify, and `RandInt(0, -1)` remains unreachable.
- **§5.2's risk stands rather than being dissolved.** With the flag on and only
  the two `CHIME MAIDEN` rows ticked in `ENEMIES INCLUDED`, the pool is empty,
  the `Fail` fires after the mirror phase, and a half-built tree is left behind.
  `NoneEnabled()` does not catch this: the selection is not empty.
- **§6.5 step 8 inverts.** It said the run "completes normally having
  randomized no enemies". It now **fails with the empty-pool message**, and
  that is the correct, expected result. It is worth running anyway, to confirm
  the failure is the clean message and not a crash.
- The comment at the `Fail` was extended (beyond the plan) to record this second
  route to an empty pool and to name this section as where the decision lives.
  That is the only edit made to that region.

Matching the reference here remains a real, open question. It is now its own
item rather than a passenger on row 16.

### 10.2 Review 2.3 addressed — the placement decision is now actually mirrored

§5.1 claimed the Python mirror pinned the flag-off rule; the reviewer showed
none of §6.2's ten cases modelled it. `enemy_lookup.py` gained `randomizes()`,
a line-for-line mirror of `StepWriteMap`'s four-line decision, and
`pool_verify.py selftest` gained five cases driving it across all 101 rolls in
both flag states — including that a forced placement randomizes at *every*
roll, that a non-forced m28 maiden randomizes at every roll *except* 100 with
the flag off, and that the override is map-scoped.

### 10.3 Review 2.4 addressed

`cmd_frozen`'s summary now prints the `bell` total and a grand total.
Measured: `frozen` → 608, `frozen --bell` → 662. The plan's `608 + 54 = 662`
is now printed rather than left to inspection.

### 10.4 Review 2.5 addressed

The new entry was **inserted at position 4** in all three parallel item vectors
(`DrawList`, `DrawSaveData`, `DrawConfirm`), not appended, and both screen files
carry a comment at the row constants stating that these indices *are* the item
positions and that an out-of-order entry compiles, passes `ui_scroll_verify.py`
and mislabels every row below.

### 10.5 Review 2.6 NOT addressed

`docs/randomization-feature-spec.md:134` still reads "Trivial. Three strings
appended". Left for the documentation stage, as §4 specified. Flagged here so
the staleness is recorded rather than silently carried.

### 10.6 Two helpers added to `enemy_lookup.py` beyond §4's list

`is_m28_forced(mapname, name)` and `randomizes(...)`. Both are mirrors of engine
rules and belong next to the pool mirror, so `pool_verify.py` imports them
rather than reimplementing the rule — the same "do not reimplement it" rule
`engine_pool` already carries.

### 10.7 `-Wreorder`

`unchangedBellMaidens_` is initialised in declaration order in
`EnableWizardScreen`'s constructor (after `randomizeEnemies_`, not next to
`randomizeWorkshopTools_` as §4's text implied). The build is `-Wall` without
`-Werror`, so this was a warning avoided rather than an error fixed.

### Verification actually run

| Check | Result |
|---|---|
| `rm -rf src/x64 && make` | clean, **zero warnings**, `.pkg` built |
| `pool_verify.py selftest` | **43/43**, including 18 new `bell:` cases |
| `pool_verify.py table both` | PASS — 82 enemy models, 17 boss models |
| `ui_scroll_verify.py` | PASS at 15 rows on all three screens |
| `enemy_lookup.py frozen` | 608 |
| `enemy_lookup.py frozen --bell` | 662 |
| `pool_verify.py maidens` | smoke-tested both directions against vanilla-vs-vanilla: `off` reports and passes, `on` correctly FAILS with 0 of 12 forced placements changed |

Every measured figure in §6.2 reproduced: pool 333/82 → 317/80, the 16 lost
weights, 54 maidens, `c1055` inert, 12 forced placements all `NPCParamID`
105810.

**This is ready for the §6.5 hardware test, not done** — with step 8 read as
§10.1 restates it.

---

<!--
Not in this document, on purpose:

  - what the feature is and why it exists (that is docs/features/016-unchanged-bell-maidens/spec.md)
  - the plan review's findings (stage 2, its own file)
  - production code
-->
