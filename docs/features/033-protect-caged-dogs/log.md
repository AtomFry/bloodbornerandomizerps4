# Feature 033 — Protect Caged Dogs — log

Append-only. Every stage adds an entry; none rewrites an existing one.

---

## 2026-09-19 — Origin, developer-supplied draft

This row did not start as a one-line backlog entry. The developer wrote a full
draft feature request first; backlog row 33 was added from it, and stage B then
ran against both. The draft is reproduced verbatim below and is the primary
input to `spec.md`. Where the spec departs from it — because investigation
contradicts it, or because it prescribes implementation that stage B does not
own — the spec says so and why.

---

### Developer's draft, verbatim

> # Caged Hunting Dog Randomization Exclusion
>
> ## 1. Summary
>
> Add an optional randomizer setting that prevents the specific Hunting Dog enemy placements contained in cages in Central Yharnam from being randomized.
>
> The existing enemy exclusion feature operates at the enemy-type level: excluding Hunting Dog prevents every Hunting Dog placement in the game from being randomized. This feature operates at the placement level instead, allowing Hunting Dogs elsewhere in the game to remain eligible for randomization.
>
> When enabled, the identified caged Hunting Dog placements in Central Yharnam are always excluded from enemy randomization, regardless of the user's other enemy-randomization selections.
>
> When disabled, those placements participate in randomization normally and are subject to the existing randomizer rules.
>
> ## 2. User-facing behavior
>
> Add a toggleable setting for the Central Yharnam caged Hunting Dogs.
>
> Suggested setting name:
>
> **Protect Central Yharnam Caged Dogs**
>
> The setting has two states:
>
> * **On** — the identified caged Hunting Dog placements are never randomized.
> * **Off** — the identified caged Hunting Dog placements are eligible for randomization according to the normal randomizer rules.
>
> The setting should default to **Off** unless existing project conventions indicate otherwise.
>
> The setting applies specifically to the known caged Hunting Dog placements. It must not globally exclude the Hunting Dog enemy type.
>
> ## 3. Existing behavior that must remain unchanged
>
> The randomizer already provides an enemy-type exclusion mechanism where the user can select enemies from the list of distinct enemy types and prevent those enemy types from being randomized.
>
> That existing behavior must remain unchanged.
>
> For example:
>
> * If the user excludes **Hunting Dog** using the existing enemy exclusion setting, all Hunting Dogs should continue to be excluded as they are today.
> * If the user does not exclude Hunting Dog and the new **Protect Central Yharnam Caged Dogs** setting is Off, the caged Hunting Dogs may be randomized normally.
> * If the user does not exclude Hunting Dog and the new setting is On, only the identified Central Yharnam caged Hunting Dogs are excluded. Hunting Dogs elsewhere remain eligible for randomization.
> * If both settings are enabled, the existing global Hunting Dog exclusion and the placement-specific exclusion both apply.
>
> The new setting must not alter the semantics of the existing enemy-type exclusion feature.
>
> ## 4. Problem being addressed
>
> Central Yharnam contains a group of Hunting Dogs positioned inside cages. There are approximately 6–7 relevant dog placements in this area, with some dogs able to break out of their cages during normal gameplay.
>
> These specific placements have demonstrated problematic behavior when replaced by other randomized enemies.
>
> Observed examples include:
>
> * A Boom Hammer Hunter replacing a caged dog caused severe game lag.
> * A Maneater Boar replacing a caged dog caused severe game lag.
> * Other randomized enemies placed in these cages can receive damage but cannot be killed.
> * The original Hunting Dogs behave correctly in these placements and can be killed normally.
>
> These observations suggest that the caged placements have placement/environment-specific behavior or constraints that are incompatible with arbitrary enemy replacement.
>
> The purpose of this feature is to provide a targeted workaround without preventing Hunting Dogs from being randomized elsewhere.
>
> ## 5. Placement identification
>
> The implementation must identify the specific enemy placements corresponding to the Central Yharnam caged Hunting Dogs.
>
> Do not implement this by simply checking whether the enemy type is Hunting Dog.
>
> The implementation should determine the appropriate placement-level identifier(s) from the game data and use those identifiers to recognize the affected placements.
>
> The exact identifiers are an implementation detail to be established during investigation.
>
> Claude should first investigate and document:
>
> 1. The enemy type ID for Hunting Dog.
> 2. The map/area containing the affected placements.
> 3. The specific identifiers used to distinguish each affected caged Hunting Dog placement.
> 4. Whether all relevant caged dogs share a reliable placement property or identifier pattern.
> 5. Whether any of the dogs that can escape their cages are represented differently from the permanently caged placements.
> 6. Whether the identified placements can be reliably distinguished from other Hunting Dog placements in Central Yharnam and elsewhere.
>
> Do not assume that every Hunting Dog in Central Yharnam belongs to this exclusion set.
>
> ## 6. Randomization behavior
>
> During enemy randomization, determine whether the current enemy placement is one of the protected caged Hunting Dog placements.
>
> Conceptually:
>
> * If the setting is Off, continue through the existing randomization logic unchanged.
> * If the setting is On and the current placement is one of the identified protected placements, leave the original enemy assignment unchanged and skip randomization for that placement.
> * If the setting is On but the current placement is any other enemy placement, continue through the existing randomization logic unchanged.
>
> The exclusion should occur at the appropriate point in the existing randomization pipeline so that a protected placement is never assigned a randomized replacement.
>
> The implementation should avoid duplicating or bypassing unrelated randomizer rules.
>
> ## 7. Scope of exclusion
>
> The protected set consists only of the specifically identified caged Hunting Dog placements in Central Yharnam.
>
> The following must remain eligible for randomization when otherwise permitted:
>
> * Hunting Dogs elsewhere in Central Yharnam.
> * Hunting Dogs in other maps.
> * Other enemy types in Central Yharnam.
> * Other enemy placements in the same general area.
> * Any non-caged Hunting Dog placements.
>
> The feature should therefore be considered a **specific placement blacklist**, rather than an enemy blacklist.
>
> ## 8. Configuration persistence
>
> The setting should use the existing configuration/settings mechanism used by the randomizer.
>
> The On/Off state must persist according to the project's existing configuration behavior.
>
> Do not introduce a separate configuration mechanism solely for this feature.
>
> ## 9. User interface
>
> Add the setting to the existing appropriate enemy-randomization/settings UI.
>
> Use terminology that describes the behavior rather than the implementation.
>
> Recommended label:
>
> **Protect Central Yharnam Caged Dogs**
>
> Recommended description/help text:
>
> **Prevents the caged Hunting Dogs in Central Yharnam from being randomized. Hunting Dogs elsewhere remain eligible for randomization.**
>
> The setting should clearly communicate that this is a location-specific protection.
>
> ## 10. Interaction with existing enemy exclusions
>
> The following behavior must be preserved:
>
> | Hunting Dog exclusion | Caged-dog protection | Caged Hunting Dogs   | Other Hunting Dogs   |
> | --------------------- | -------------------- | -------------------- | -------------------- |
> | Off                   | Off                  | Normal randomization | Normal randomization |
> | Off                   | On                   | Protected            | Normal randomization |
> | On                    | Off                  | Excluded             | Excluded             |
> | On                    | On                   | Excluded             | Excluded             |
>
> The new setting must not weaken or override the existing global enemy exclusion.
>
> ## 11. Validation requirements
>
> The implementation should be validated using black-box tests against an actual randomized game/data output.
>
> At minimum, verify:
>
> ### Protection Off
>
> With Hunting Dog eligible for randomization and the new setting Off:
>
> * The caged Hunting Dog placements are eligible for randomization.
> * Other Hunting Dogs remain eligible for randomization.
> * Existing randomization behavior is unchanged.
>
> ### Protection On
>
> With Hunting Dog eligible for randomization and the new setting On:
>
> * Every identified protected caged Hunting Dog remains a Hunting Dog.
> * Hunting Dogs elsewhere can still be randomized.
> * No unrelated enemy placement is accidentally protected.
>
> ### Global Hunting Dog exclusion
>
> With the existing Hunting Dog enemy exclusion enabled:
>
> * All Hunting Dogs remain excluded according to existing behavior.
> * The new setting does not change that behavior.
>
> ### Regression
>
> Run randomization with the new setting disabled and compare behavior against the pre-feature implementation to ensure the default path remains unchanged.
>
> ## 12. Important implementation constraint
>
> Do not solve this by adding Hunting Dog to the existing global exclusion list.
>
> The purpose of this feature is specifically to support:
>
> > "Do not randomize these particular Hunting Dog placements."
>
> rather than:
>
> > "Do not randomize Hunting Dogs."
>
> The distinction between **enemy type** and **enemy placement** is fundamental to this feature.
>
> ## 13. Investigation before implementation
>
> Before writing production code, investigate the existing randomization implementation and game data sufficiently to establish the exact placement-level identifiers for the affected cages.
>
> The investigation should produce a concise record of:
>
> * Hunting Dog enemy ID.
> * Relevant map ID.
> * Number of affected placements.
> * Placement identifiers/properties used to recognize them.
> * Evidence that the identified placements correspond to the caged dogs.
> * Evidence that the identification does not unintentionally include other Hunting Dogs.
>
> If the game data does not provide a clean placement identifier, document the available alternatives and determine the least fragile way to identify the affected placements before implementation.
>
> Do not begin implementation based solely on the approximate count of dogs or their visual location in the game.
>
> ## 14. Acceptance criteria
>
> The feature is complete when:
>
> 1. A user can enable or disable **Protect Central Yharnam Caged Dogs**.
> 2. When enabled, every identified Central Yharnam caged Hunting Dog remains unchanged during enemy randomization.
> 3. When disabled, those placements follow the existing randomization behavior.
> 4. Hunting Dogs outside the protected placement set remain independently randomizable.
> 5. The existing global enemy exclusion behavior remains unchanged.
> 6. The protected placement set is based on verified game-data placement identifiers rather than the Hunting Dog enemy ID alone.
> 7. The setting persists using the existing randomizer configuration mechanism.
> 8. Black-box testing demonstrates both the protected and unprotected behaviors.
> 9. No unrelated enemy placements are accidentally excluded from randomization.
> 10. The implementation does not require special handling of the replacement enemy types such as Boom Hammer Hunter or Maneater Boar; the protection is based on the problematic source placements.

---

### Backlog row added from it

Row 33 was added to `docs/randomization-feature-spec.md` §3 on 2026-09-19,
alongside row 32, with a short subsection explaining why row 32's model-level
gate does not cover this case.

---

## 2026-09-19 — Stage B, spec written (`/spec 33`)

`spec.md` created from the draft above and from backlog row 33. Four developer
decisions recorded as §10 D5-D8: protect the **six** scripted dogs rather than
all seven in the yard, keep protected placements **contributing to the
replacement pool**, name the setting **`DO NOT RANDOMIZE CAGED DOGS`** (the
developer's own wording, chosen over both the draft's label and the spec's
recommendation), and **leave zone stat-scaling alone**. D1-D4 were drafted by
the spec author from the draft's own hard constraints.

**Two corrections to the material this stage was given, both of them factual.**

1. **The creature is `c1240` SHAGGY HUNTING DOG, not `c1140` HUNTING DOG.** The
   draft and the backlog row both said "Hunting Dog". `HUNTING DOG` is a
   different creature with 14 placements, none of them in Central Yharnam, so
   ticking that row in `ENEMIES SKIPPED` would do nothing whatever to the caged
   dogs. Verified against `EnemySkipTable.h` (`c1140` 14 placements, `c1240`
   96). **Backlog row 33 and its subsection were corrected the same day.**
2. **Six placements, not "approximately 6-7".** The kennel yard holds seven
   dogs; six are addressed by the cage script and one, `c1240_0007`, is not and
   sits ten times further from the nearest cage prop. The draft's range was the
   developer seeing the seventh dog in the yard.

**The identification finding, which the feature turns on.** Only a hard-coded
list of six entity IDs isolates the set exactly. Every 4-byte field of all
twelve Central Yharnam dog records was diffed and none has a value shared by the
six and nothing else. Critically, **placement-name substring matching — how the
existing `ENEMIES SKIPPED` gate works — catches 44 placements across six maps**,
because Cathedral Ward, the Forbidden Woods and Yahar'gul all reuse the names
`c1240_0004`, `_0005`, `_0008`, `_0010`, `_0011` and `_0012`. Stage C cannot
reuse the existing gate's matching strategy.

**Not established, and deliberately not guessed.** The cause of the lag and the
unkillable replacements. The spec carries it as hypothesis H1 — `StepWriteMap`
rewrites only model, behaviour and stat rows, leaving name, entity ID and
position untouched, so the cage events keep addressing the same entity IDs and
drive whatever creature was written in. Hardware is the only authority
(`CLAUDE.md` §3). The spec author also left the individual EMEVD instruction
opcodes uninterpreted, there being no EMEDF in the repo; nothing in the spec
depends on them.

Ended at `QUESTIONS ANSWERED — awaiting developer approval`.

---

## 2026-09-19 — Stage B, spec refined (`/refine-spec 33`)

**The feedback that prompted it**, verbatim:

> i believe there are also caged dogs in forbidden woods.  Please review the
> forbidden woods map and see if you can identify caged dogs, either hunting
> dogs or shaggy dogs in cages that we should also include in this feature

**Classification: evidence-driven scope extension under a pre-existing
conditional instruction.** The developer's message was itself conditional — "see
if you can identify … that we should also include" — so the widening executes an
instruction rather than overriding one. **No previously recorded decision was
altered. D1–D8 survive verbatim and every one still holds under the widened
scope.** The spec had never reached `APPROVED`, so no approval was broken; it
re-entered `QUESTIONS OPEN` while Q1–Q2 were outstanding and returned to
`QUESTIONS ANSWERED — awaiting developer approval` once they were answered.
**Nothing was marked `STALE`** — the feature folder holds no `plan.md` and
nothing derived from one.

**The finding: the Forbidden Woods does hold caged dogs, and the evidence is
stronger than Central Yharnam's.** Four `c1240` Shaggy Hunting Dogs, in a
cluster of six cages of which two ship empty. The Forbidden Woods is
**`m27_00_00_00` / `m27_00_00_01`** (two identical files, eight dogs each);
`m32` is Byrgenwerth. Four independent measurements pick out the same four
placements and nothing else:

* **Proximity** — each within 0.34 units of its own `o273011` cage instance,
  against **32.64** units for the nearest other dog in the area. A hundredfold
  break, where Central Yharnam's `c1240_0007` sat at a tenfold one.
* **Script** — a five-event family in `m27_00_00_00.emevd.dcx` (framing
  validated both ways, per-event instruction counts summing to the file's 1927),
  every instruction addressing the dog by entity ID, and event `12705500`
  pairing each dog with the exact cage object the proximity measurement put it
  on.
* **Stat row** — `124501`, carried by these four in each file and by no other
  placement in the game. Unlike Central Yharnam's `124401`, it is exact for the
  whole caged set of its area.
* **A game-wide sweep** — of the 120 dog placements in the game, the 26 closest
  to any object are exactly the protected set, all ≤ 0.90 units; the 27th is at
  1.31. Across every map's EMEVD the only dogs paired with an object entity in
  an initialiser are Central Yharnam's two breakout dogs and these four.

**Two things the Forbidden Woods has that Central Yharnam does not.** Event
`12705480` is initialised six times, naming entity IDs **2700303 and 2700310
that match no placement in any base map**, and cage objects 2701111 and 2701115
carry entity IDs but appear in no instruction argument anywhere. The area was
authored for six caged dogs and ships four. Separately, the collision surface
does **not** discriminate here — `h000011_0000` carries eleven enemies of four
creatures — so the surface-based scheme that was exact for Central Yharnam's
seven-dog set does not generalise.

**No D5-analogue arose.** There is no near-cage-but-not-in-cage Forbidden Woods
dog; the break at 0.34 / 32.64 is clean, so the judgment call D5 settled for
Central Yharnam simply does not recur.

**Creature check, re-measured rather than cited.** `c1140` Hunting Dog: 14
placements, still all Hemwick. Three further dog-shaped creatures were checked
against the developer's "either hunting dogs or shaggy dogs" — `c1160` Keeper's
Hunting Dog (zero base-map placements), `c1241` Shaggy Hunting Dog (Crow Face)
(4, all Nightmare of Mensis), `c4080` Deep Sea Hound (6, all Fishing Hamlet).
None in the Forbidden Woods, none caged.

**Two corrections of fact made on the spec author's own authority**, neither
reversing a developer choice. F7's substring result was "44 placements across
**six** maps"; the 44 was right and the map count was not — it is **nine map
files**, now 58 across nine for the widened set. And F10's "reachable" figure
silently depended on a `names.map_is_unused` rule that is inconsistent between
the two areas' pre-DLC files, so the rule is now stated where the arithmetic
uses it.

**Recomputed figures.** Protected set 18 → **26** placements across five map
files; reachable 12 → **16**. The pool counterfactual in F12 doubles: if
protected placements stopped contributing the pool would lose `124401` *and*
`124501` and fall to **331**, not 332 — but D6 keeps them contributing, so the
pool stays at **333** across 82 models, and D6's own claim is untouched.

**Two new questions, both answered the same day.**

* **D9 — the Forbidden Woods cages are in scope.** Asked because the two halves
  of the feature rest on different kinds of evidence: Central Yharnam's six were
  *observed* to break, the Forbidden Woods' four merely *look identical*, and no
  static analysis can close that gap. The developer confirmed inclusion knowing
  that asymmetry. Hardware test 6 is written to supply the missing observation.
* **D10 — help text names both areas, budget permitting**, falling back to
  area-neutral wording if it does not fit the §7 UI budget. D7 had constrained
  the help text only negatively.

**Still not established.** Whether the Forbidden Woods cages actually misbehave
when randomized — the substance of D9, and a hardware question by construction.
Which of the four are "breakout" dogs: the script splits them 2/2 in the same
shape Central Yharnam's does, but all four have entity-ID'd cage objects where
only Central Yharnam's breakout pair did, and with no EMEDF in the repo the
opcodes stay uninterpreted. It does not affect the protected set — all four are
protected either way.

**Also recorded for stage C:** object model IDs are numbered per area, so the
shared `3011` suffix across `o243011`, `o273011`, `o283011` and `o353011` is
**not** evidence of a shared asset. That trap is now in the spec's dead-ends
appendix.

Ended at `QUESTIONS ANSWERED — awaiting developer approval`.

---

## 2026-09-19 — Stage B, spec approved

The developer read the refined spec and approved it. Status
`QUESTIONS ANSWERED — awaiting developer approval` → `APPROVED`.

This is human gate 0h. §10 holds ten decisions (D1–D10) and §9 is gone.
Ready for stage C (`/plan 33`).

The approved scope is the widened one: **ten** protected placements — Central
Yharnam's six and the Forbidden Woods' four — **twenty-six** across the five map
files that hold them (D9). The backlog row and its narrative subsection in
`docs/randomization-feature-spec.md` still described the Central-Yharnam-only
set they were written for, and were corrected to the approved scope in the same
pass; the spec, not the backlog, was the thing approved.

Any change from here that alters an approved decision drops the status back to
`QUESTIONS ANSWERED — awaiting developer approval` and requires re-approval; a
correction that alters none keeps `APPROVED` and is recorded as an amendment.

---

## 2026-09-19 — Stage C: the plan

`/plan 33` ran the `planner` subagent against the APPROVED spec and produced
`plan.md` (the implementation contract, §1–§7) and `plan-evidence.md` (the
investigation behind it). Two milestones: the protected set plus the engine
gate plus a new `caged_dogs_verify.py`, then the setting and its two screens.

The planner re-derived the spec's evidence independently rather than citing it
— all 26 placements, the entity IDs, the stat rows, the 96/70 and 42/32
placement arithmetic, the pool at 333 across 82 models, the zone-scaled stat
values and the cage-script families reproduced exactly, and the EMEVD
behind spec F3/F5/F14 matched object for object.

### Questions put to the developer, and what was decided

**Q1 — on-screen help text.** Spec D10 requires help text naming both areas,
but the planner found the port has no per-row help facility on either settings
screen; the only precedents are a detail line under a three-item menu and an
instruction line above the pickers. Adding one to a 16-row scrolling list is a
geometry change to every row. The planner flagged this as a **spec gap**
rather than a plan question, so it was put to the developer as one.
**Decided: no in-app help text**; D10 is satisfied in `docs/user-guide.md` at
the documentation stage, naming Central Yharnam and the Forbidden Woods and
never the words "Hunting Dog". Recorded as P11. **This supersedes spec D10 as
written, and the spec still needs amending to match** — noted in the pipeline
index against the spec, not silently absorbed into the plan.

**Q2 — reporting the protection count on screen.** The honest figure is 26,
where a player expects 10, because the five map files include two variants the
retail game never loads. **Decided: text log only**, no progress-screen line.
Recorded as P12, confirming the planner's recommendation.

**Q3 — where the new settings row goes.** Not a §8 question; raised because
the planner listed its own choice as a risk and offered the alternative. It
had recommended index 4, directly below `RANDOMIZE ENEMIES`, which renumbers
eleven constants and three parallel `items` vectors — the failure both screens
carry a warning comment about, since a constant and its entry that disagree
compile, pass `ui_scroll_verify.py` and mislabel every row below.
**Decided: append the row last instead**, accepting the worse list order to
remove the renumbering hazard entirely. Recorded as P5 (rewritten) and P13.

### What step 7 reconciled

Q3 reversed a planner decision, so most of the reconciliation was its
consequences:

* `plan.md` §3.3 — the renumbering hazard rewritten as the much narrower check
  that survives appending.
* §4.4 — the row-position paragraph rewritten: index 15, no constant changes,
  the full row list corrected, and the accepted cost stated.
* §4.2 and §4.4 — `(§8 Q2)` and `(§8 Q1)` repointed to `(§9 P8)` and
  `(§9 P11)`.
* §5 — the three UI rows of the files table: no renumbering, entries appended.
* §7 — milestone 2 steps 3 and 4 rewritten, with "no existing row constant
  changes" made checkable by `git diff`.
* §8 — emptied to one line pointing at P11 and P12; heading kept.
* §9 — P5 rewritten and reattributed to the developer; P8 marked confirmed;
  P11–P13 added.
* `plan-evidence.md` — the rejected-alternatives table entry inverted, since
  appending is now the decision and index-4 insertion the rejected branch; and
  §E5.1 rewritten as a hazard the decision retired rather than one to accept.

Two §8 references were left alone deliberately: §1's note that §8–§10 are the
record, and B12's citation of the **spec's** §8.

Contract length after reconciliation: 419 lines for §1–§7.

### Spec issues the planner found

1. **D10 asks for a facility the port does not have** — the Q1 spec gap above.
   The only place the approved spec asks for something the codebase cannot
   deliver as written.
2. **Spec §8's "Automated testing" list is misclassified.** Items 2–6 all need
   a randomized output tree, and the engine only runs on the PS4. Only items 1
   and 7 are runnable by an implementer. The plan's §6 moves the rest to
   post-hardware checks and gives the command that runs them against a
   captured tree.
3. **Spec §4 F7's parenthetical is wrong in detail** (harmless). Among enemy
   parts `m33_00_00_00` has two duplicate entity-ID pairs, not one, and across
   all part types several maps repeat an ID between an enemy and a non-enemy
   part. F7's conclusion holds: the ten protected IDs are unique wherever the
   gate can see them.
4. **Spec §7's UI budget arithmetic is one character off** — the new row is 33
   characters including its value against 32 for the current longest, so it is
   marginally the longest rather than "alongside". The real budget is 53 at
   scale 4, so nothing is affected.
5. **Spec §2 and hardware test 1 omit that Central Yharnam has 13 cage props
   for 6 caged dogs** — the Forbidden Woods' 6/4 asymmetry is documented but
   Central Yharnam's is not. A tester told "six dogs in cages" will see empty
   cages and could report a false failure. The correction is in the plan's
   hardware handoff.

Status left at **QUESTIONS ANSWERED — awaiting developer approval**: the
developer answered from the summary above, not from the plan itself. Stage D
(`/review-plan 33`) is theirs to call.

---

## 2026-09-19 — Stage E, milestone 1 implemented (`/implement 33`)

Dispatched with `plan.md` as the contract, alongside `plan-evidence.md`,
`spec.md` and this log. Plan status on dispatch: **Approved**, §8 empty. Note
that `plan-review.md` does not exist — stage D was never run for this feature;
the developer approved the plan directly, which is their gate to hold.

**Milestone 1 — the protected set and the engine gate. Completed.** No stop
condition fired.

### Changed

Exactly the four files §5 assigns to milestone 1, and no others:

* `app/src/Randomizer/CagedDogList.h` — **new**. Ten `(map prefix, entity ID)`
  entries and `IsProtectedCagedDog`, which returns false for entity IDs `<= 0`
  before comparing anything.
* `app/src/Randomizer/EnemyRandomizer.h` — `doNotRandomizeCagedDogs` (default
  `false`) on the options struct, `cagedDogsProtected` on the result struct.
* `app/src/Randomizer/EnemyRandomizer.cpp` — the include, the gate in
  `StepWriteMap`, and the on-only log line in `StepEmevd`.
* `app/tools/caged_dogs_verify.py` — **new**; `list`, `protected`, `selftest`.

Plus `implementation-report.md`, and `plan.md` §10 — §1–§7 verified unchanged
against a pre-dispatch copy.

### Deviations

**None.** Four choices the plan left open were taken by the implementer and
recorded in `plan.md` §10 and report §3:

1. The gate reads `!forced && options.doNotRandomizeCagedDogs &&
   IsProtectedCagedDog(...)`, matching the shape of its two siblings. The
   option is still the first thing the gate itself evaluates, so §3.1's
   RNG-stream invariant holds by short-circuit.
2. The per-run log line is emitted after the existing `enemy randomizer: done
   - …` summary in `StepEmevd`.
3. `caged_dogs_verify.py` hard-codes the three cage object models as
   derivation A's input, and also asserts the two cage-prop counts from §3.3.
4. Header symbols: `CagedDogEntry` / `CagedDogList()` / `IsProtectedCagedDog()`.

### Verification run, and independently re-run by the orchestrator

| Check | Result |
| ----- | ------ |
| `make clean && make` | `.pkg` built, 7,143,424 bytes. Re-run from scratch after the report, producing a byte-size-identical `.pkg` — the shipped artifact provably matches the sources |
| `caged_dogs_verify.py selftest data/vanilla/dvdroot_ps4` | 23/23, exit 0; covers all eight §6 cases. Both geometric derivations return the same 26; bounds 0.899 / 2.671; both EMEVD files framed two ways each; both script families confirmed |
| `caged_dogs_verify.py protected … "data/runs/20260919-Enemies Only/dvdroot_ps4"` | FAIL as required — 26 of 26 not frozen, exit 1. The check has teeth |
| `caged_dogs_verify.py list` | 26 rows, 6/6/6/4/4 |
| `pool_verify.py selftest` | 85/85, no case changed state; config case still 555 bytes, correctly untouched until milestone 2 |
| `pool_verify.py table enemy` | 82 models, unchanged; selftest confirms the pool is still 333 entries across 82 models |
| `ui_scroll_verify.py` | passes, unchanged — no UI file was touched this milestone |

§3.1 invariants checked against the diff: `StepReadMap` not touched; the gate
sits after the `ENEMIES SKIPPED` test and before `RandInt`, inside the same
`!forced &&`, so the m28 override still outranks it and no existing test moved;
the gate only `continue`s, writing no name, entity ID, position or collision
index; `EnemyPoolTable.h` and `EnemySkipTable.h` not regenerated; no SDL2 and
no UI file learns a map name or an entity ID.

### Not run

All hardware testing, and everything belonging to milestone 2.

### Noted for stage F, not acted on

* In the pre-feature baseline, `m24_01_00_01 c1240_0005` was re-targeted to a
  different variant of the *same* creature. A freeze test comparing only the
  model name would pass it; `protected` compares the behaviour row too, so it
  catches it.
* `app/tools/names.py`'s `map_is_unused` is internally inconsistent across the
  two areas (evidence F13). Not touched; the feature covers all five map files
  regardless.

### Awaiting

Hardware test 1 of §6: seed **1234567890**, `RANDOMIZE ENEMIES` on, everything
else off, all 82 included, nothing skipped. The output `dvdroot_ps4` must be
byte-identical to `data/runs/20260919-Enemies Only/dvdroot_ps4`, and `live.log`
must gain no new line. Milestone 2 does not start until that passes.

---

## 2026-09-19 — Stage E, milestone 2 implemented (`/implement 33 2`)

**Milestone 1's hardware test: PASSED.** The developer confirmed the
byte-identical seed-1234567890 run when `/implement 33 2` was invoked, before
milestone 2 was dispatched. The orchestrator stopped and asked rather than
assuming, because no new capture had appeared in `data/runs/` and the milestone
gate is what `CLAUDE.md` §4 protects. Recorded here because the confirmation
was verbal — the captured tree is not in `data/runs/`.

**Milestone 2 — the setting. Completed.** No stop condition fired.

### Changed

Exactly the eight files §5 assigns to milestone 2, and no others:

* `app/src/Randomizer/RandomizerDefaults.h` — `doNotRandomizeCagedDogs`,
  default `false`.
* `app/src/Randomizer/RandomizerDefaultsStore.cpp` — the key in the loader's
  `strcmp` chain, in the save format string and in its argument list.
* `app/src/UI/SetupDefaultsScreen.h` — `kDoNotRandomizeCagedDogsRow = 15`,
  `kItemCount` 15 → 16.
* `app/src/UI/SetupDefaultsScreen.cpp` — `ToggleRow` branch, row appended last
  in `DrawList`.
* `app/src/UI/EnableWizardScreen.h` — the per-run member.
* `app/src/UI/EnableWizardScreen.cpp` — row constant at 15,
  `kSaveDataRowCount` 16, ctor initialiser, left/right and X branches, row
  appended last in `DrawSaveData` and `DrawConfirm`, `StartCommit` assignment.
* `app/tools/ui_scroll_verify.py` — the three 15-row entries → 16.
* `app/tools/pool_verify.py` — worst case 555 → 585, one new case.

Plus `implementation-report.md` and `plan.md` §10 — §1–§7 verified unchanged
against a pre-dispatch copy.

### Deviations

**None.** Four choices the plan left open, recorded in `plan.md` §10 and the
report:

1. The `RandomizerDefaults` field sits after `enableMergoDarkness` and before
   `lastSeed`, keeping the remembered run artifact last. Presentation only —
   the struct is not serialised by layout.
2. `RandomizerDefaultsStore.cpp`'s comment quoting the worst-case config size
   was updated 555 → 585. Adding the key made the sentence false and it quotes
   the number `pool_verify.py` now asserts. A fourth touched line in a file §5
   already names.
3. The two toggle log lines read `defaults: do not randomize caged dogs = YES`
   / `enable wizard: … = NO`, the form every other toggle uses.
4. Milestone 2's report is appended to the same `implementation-report.md`
   under its own title, and milestone 1's status line there was updated to
   record the hardware pass.

### Verification run, and independently re-run by the orchestrator

| Check | Result |
| ----- | ------ |
| `make clean && make` | `.pkg` built, 7,143,424 bytes, no errors and no `-Wall` warnings. Re-run from scratch by the orchestrator after the report |
| `ui_scroll_verify.py` | PASSED — Setup Defaults 7 of 16, Wizard SaveData 6 of 16, Wizard Confirm 6 of 16. Diff confirms only those three entries changed; "Progress log" was already 16 |
| `pool_verify.py selftest` | 86/86 — the 85 existing cases unchanged, the config case now 585, one new case asserting the key is in load *and* save |
| `pool_verify.py table enemy` | PASS, 82 models |
| `caged_dogs_verify.py selftest` | 23/23, unchanged |
| `caged_dogs_verify.py protected …` vs the pre-feature tree | FAIL as required, 26 of 26 |

§3.1 invariants checked against the diff: no pre-existing `k*Row` constant was
edited on either screen — rows 0–14 are untouched and the new row is 15 with
both counts at 16; all three `items` vectors carry the new entry last, at the
index its constant names; `defaults.cfg` stays compatible, since an absent key
falls through the `strcmp` chain to the struct's own `false`; `ENEMIES SKIPPED`
is untouched; and no UI file names a map or an entity ID — the UI only sets a
bool, and `CagedDogList.h` is included by the engine alone. B13 confirmed: the
words "HUNTING DOG" appear in no user-facing string.

The setting is **absent from `StartCommit`'s big `||`** (verified by reading
the condition), so ticking it alone cannot start a run that does nothing — B10.

### Not run

Everything needing a console. In particular the `defaults.cfg` round-trip is
argued from source and pinned by the mirror case, not executed; there is no
host C++ compiler.

### Noted for stage F, not acted on

* `EnableWizardScreen.cpp`'s anonymous-namespace comment and
  `SetupDefaultsScreen.h`'s equivalent still narrate feature 032 keeping the
  count at 15. They describe 032's change rather than today's total, so they
  were left; a reviewer may prefer them reworded.
* `StartCommit` carries two stacked "Meaningless without …" comments above
  `options.randomizeWorkshopTools`, one of which reads as a leftover.

### Awaiting

The six hardware tests of §6, with the setting on and then off. Both captures
go into `data/runs/` so `caged_dogs_verify.py protected` can be run on each: it
must pass on the "on" tree and fail on the "off" one.
