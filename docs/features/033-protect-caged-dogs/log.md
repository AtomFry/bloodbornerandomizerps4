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
