# Feature 018 — Easy Shadows (and the three sibling difficulty helpers)

**Status: APPROVED** — read and approved by the developer on 2026-09-16.
All four questions were answered the same day and are recorded in §10; the body
has been reconciled against them. Ready for stage 1 (`/plan 18`).

**Backlog rows:** `docs/randomization-feature-spec.md` §6, rows **18, 19, 20, 21**
— the developer confirmed the grouping on 2026-09-16 (§10, decision 1); §6 gives
the evidence for it.

**Reference flags:** `easyMultiBossesBool`, `easyRomBool`, `easyFailuresBool`,
`easyWitchesBool` (`reference/Randomizer/MainWindowComponents/BooleanHandler.cs:55-68`)

**Reference:** `reference/Randomizer/MainWindowComponents/MainWindow.xaml.cs` — `EasyModes`

**Plan:** `docs/features/018-easy-shadows/plan.md` *(once stage 1 has run)*

---

## 1. What this feature does

Four of Bloodborne's boss fights put more than one body in the arena at once.
The Shadows of Yharnam are three hunters who fight as a trio. Rom summons waves
of spider children around her. The Living Failures come at you as a group. The
Celestial Emissary fight begins as a crowd of small aliens, one of which swells
into the boss.

Each of these four settings turns one of those fights into a fight against a
single opponent. The duplicate bodies are still there and still have to be
killed, but each is swapped for a tiny, harmless creature that dies to one hit —
so the fight plays as a duel instead of a brawl.

They are four independent switches, one per fight, all off by default. None of
them is a randomizer: they draw no randomness, and the same seed gives the same
world whether they are on or off. They apply whether or not enemy, boss or
treasure randomization is on.

---

## 2. What does the player experience?

Every count below is measured against the map the retail game actually loads.
§4 holds the measurements and the "how many are left" numbers.

| Setting | Off | On |
| --- | --- | --- |
| **EASY SHADOWS** | All three Shadows of Yharnam fight you at once | **One Shadow remains** — the strongest of the three. The other two are replaced by a harmless larva that dies in one hit. The fight still ends only when all three bodies are dead |
| **EASY ROM** | Rom teleports away and floods the lake with waves of her spider children | **None of her thirty children remain.** Every one of them is that same harmless larva. Rom herself is completely unchanged, so the fight becomes Rom alone |
| **EASY FAILURES** | Four Living Failures share the arena | **One Living Failure remains**, at full strength. The other three become larvae |
| **EASY EMISSARY** | A crowd of eight small emissaries fills the arena, and one of them grows into the Celestial Emissary | **One small emissary remains** — the one that becomes the boss — so the Celestial Emissary fight itself is untouched, just without its escort. The three small emissaries elsewhere in Upper Cathedral Ward are unaffected |

**The replacement is not a statue.** The backlog describes these settings as
swapping in a "harmless statue (stone guy)", and the reference tool's own
variable is named after that idea, but the creature it actually places is the
small pale larva from Iosefka's Clinic (§4). It has 2 health, is worth 18 blood
echoes, and belongs to the same non-hostile group as the Doll and Gehrman in his
chair. Whether it attacks at all is a hardware observation (§8).

**It moves a unique item, and this is the most surprising thing about the
feature.** That larva is the only creature in the game that drops *One Third of
Umbilical Cord*, and every copy these settings create is the same creature with
the same drop. Expect the cord to come from the first easy-mode larva you kill —
in Rom's lake, in the Research Hall, wherever you get there first — rather than
from the clinic. §4 separates what is measured from what is inferred. This
relocation is accepted deliberately, as the price of matching the reference
(§10, decision 2).

**Combined with the other settings.**

* With **RANDOMIZE BOSSES** on, these fights already get randomized bodies: all
  three Shadow slots, the Living Failures group, Rom herself and the Celestial
  Emissary pair are already replaced by other bosses. Turning an easy setting on
  as well means the *duplicates* become larvae while the surviving slot keeps
  whatever boss the seed drew — so you fight one random boss instead of three.
* With **RANDOMIZE ENEMIES** on, Rom's children and the small emissaries are
  ordinary randomizable placements today, so without these settings they can
  already be anything. With an easy setting on, its own targets are larvae
  regardless of what the seed drew for them.
* With **RANDOMIZE ENEMY DROPS** on, nothing changes here: the larva's drop is
  one of only two the drop randomizer refuses to touch, in the reference and in
  the port (§4).

---

## 3. What does the existing randomizer do?

One function, `EasyModes` (`MainWindow.xaml.cs:934-1055`), called from four
independent `if` blocks at the top level of the run
(`StartFunctions.cs:1239-1256`). Each block passes the map files for its own
fight; the function branches on which map it was given and, for a hard-coded list
of placement names, overwrites three fields — `NPCParamID`, `ThinkParamID` and
`ModelName` — with the values of one particular creature, then writes the map
back.

The four branches and their target lists:

| Row | Flag | Maps passed | Names matched |
| --- | --- | --- | --- |
| 18 | `easyMultiBossesBool` | `m27_00_00_01` | `c2120_0001`, `c2120_0002` |
| 19 | `easyRomBool` | `m32_00_00_00`, `m32_00_00_01` | anything containing `c1400` |
| 20 | `easyFailuresBool` | `m35_00_00_00` | `c4030_0001`, `c4030_0002`, `c4030_0003` |
| 21 | `easyWitchesBool` | `m24_02_00_00`, `m24_02_00_01` | `c2500_0001` … `c2500_0010` |

Rows 18, 20 and 21 enumerate placement names. Row 19 matches a model family
prefix, which is why it takes all thirty of Rom's children rather than a chosen
few — §4 measures both.

**Where the replacement creature comes from.** The three values are not
constants. They are captured at `MainWindow.xaml.cs:748-762`, inside
`GenerateEnemyList`: while building the enemy pool, a placement whose model is
`c2521` has its `NPCParamID`, `ThinkParamID` and model name copied into
`stoneGuyParam` / `stoneGuyThink` / `stoneGuyModelName`. The guard around that
capture tests `addedStoneGuyBool`, a field that is **declared and never assigned
anywhere in the reference**, so the capture re-runs on every match and the last
one wins. Harmless in practice: all three `c2521` placements in the tree carry
identical values (§4).

**Three reference behaviours worth recording, none of which should be reproduced
blindly:**

1. **The target maps do not declare the replacement's model.** `EasyModes` sets
   `ModelName` to a model none of the six target maps contains, and SoulsFormats'
   `MSBB.Write` throws `KeyNotFoundException` for a model name it cannot resolve
   (`MSBBB.cs:180-190`). It works only because an unconditional earlier pass
   (`StartFunctions.cs:317-382`) merges every enemy model declaration from every
   map into every map. The port has the equivalent guarantee already (appendix).
2. **With the enemy picker ("oops all") in use, the capture never runs.**
   `GenerateEnemyList` is skipped on that path, so the three values stay at
   `0` / `0` / `null`, and `EasyModes` writes a null model name (resolved to
   index −1) and zeroed params onto the target placements. That is a reference
   defect, not a behaviour to match.
3. **The easy pass runs late.** It is after enemy randomization, boss
   randomization and NPC randomization, and *before* the zone scaling pass
   (`StartFunctions.cs:2457`). Running last against the boss passes is what makes
   the setting win over a randomized duplicate; running before scaling means the
   replacement is itself zone-scaled, which §4 measures as a no-op.

**The port today** implements none of the four. It does already replace the same
duplicate bodies when boss randomization is on — `AddTheRestInMap` in
`BossRandomizer.cpp` targets exactly `c2120_0001/0002` and `c4030_0001/0002/0003`
— so these settings and that code path aim at the same placements, and their
order relative to one another decides the outcome.

---

## 4. What do we know?

Measured against `data/vanilla/dvdroot_ps4` by reusing `app/tools/boss_verify.py`'s
MSBB reader and `app/tools/param_offsets.py`'s PARAM/PARAMDEF reader, with
`app/tools/names.py` for identities. No new format parsing was written except a
~40-line FMG reader to resolve one item name.

### What each setting changes, and what it leaves

**fact — Easy Shadows: 3 Shadows in the arena, 2 changed, 1 left.**
`m27_00_00_01` holds exactly three `c2120` placements, clustered within five
metres of each other. The rule takes `c2120_0001` and `c2120_0002`; `c2120_0000`
is untouched. The survivor is the strongest of the three: 1425 health against 900
and 800.

**fact — Easy Rom: 30 children, 30 changed, 0 left.** Both `m32_00_00_00` and
`m32_00_00_01` hold exactly thirty `c1400` placements, all sharing one stat
block, all at Rom's own elevation. The family-prefix match takes every one of
them, so **no child remains**. Rom herself is a different model and is not
touched. This is the one of the four where the rule empties the group rather than
thinning it, and it is why §2 states a count rather than "fewer".

**fact — Easy Failures: 4 in the arena, 3 changed, 1 left; a 5th placement
elsewhere is untouched.** `m35_00_00_00` holds five `c4030` placements. Four
cluster in the arena (`c4030_0000`–`_0003`); the rule takes three of them and
leaves `c4030_0000` (320 health). The fifth, `c4030_0004`, sits 87 metres below
the arena and carries `ThinkParamID` 1 — the value the port's own pool rules
treat as "not a live enemy" (`BossRandomizer.cpp:69`). It is outside the rule's
list and stays as it is.

**fact — Easy Emissary: 8 in the arena, 7 changed, 1 left; 3 elsewhere
untouched.** Both `m24_02` variants hold eleven small-emissary placements plus
the Celestial Emissary itself. Eight cluster in the arena with the boss directly
below them; three sit 50–100 metres away with a different stat block. The rule's
enumerated list `_0001`…`_0010` covers exactly seven of the eight arena bodies —
`_0004`, `_0005` and `_0008` do not exist in the map — and reaches none of the
three outside it. **inference:** the survivor `c2500_0000` is the one that becomes
the boss, since it shares the boss's entity-ID block and is the placement the
port's boss randomizer already treats as the Celestial Emissary pair's leader
(`BossRandomizer.cpp` `kFixups`).

**fact — the replaced bodies keep their identity in the map.** `EasyModes` writes
three fields and nothing else; the placement's EntityID, position and every
index-based cross-reference are left alone. **inference — so the fight still
counts them.** All three Shadow entity IDs appear 37–39 times each in the
Forbidden Woods event script, and all five Living Failure IDs 34–37 times each,
so the fight logic tracks the individual bodies; leaving those IDs alone leaves
that logic intact. Verifiable only on hardware (§8).

### What the replacement actually is

**fact — it is the Iosefka's Clinic larva, not a statue.** Model `c2521`,
community-named *Celestial Larvae*; three placements exist in the whole tree, all
in the Central Yharnam map family, all carrying identical values (NpcParam
252100, Think 252100). Its stat block: **2 health**, 18 blood echoes, hit height
1.0 and radius 0.2 — the smallest hitbox of anything involved here. The name
comes from `app/tools/data/Characters.json`, which carries a stated accuracy
caveat; the stats come from the game's own `NpcParam`.

**inference — it is non-hostile.** Its team type is 26. Across all 43 map files,
the creatures sharing team type 26 are the Plain Doll, Gehrman in his chair,
Master Willem, the Messengers, the strapped-down patients and the friendly
`c0000` NPCs. No ordinary hostile enemy in the base maps uses it. That is strong,
but it is a byte rather than observed behaviour — `CLAUDE.md` §5 says so — hence
§8.

**fact — zone scaling cannot make it dangerous.** NpcParam 252100 is one of the
~1170 tracked identities the scaling pass rewrites, so a larva placed in a scaled
zone becomes one of 31 pre-tuned variants. All 31 variants have **2 health, 18
echoes and the same behaviour id**. Whether the easy pass runs before or after
scaling therefore cannot change what the player fights.

**fact — that creature is the game's only source of One Third of Umbilical
Cord.** Its stat block points at one item lot, which grants item 4321 — *One
Third of Umbilical Cord* — at 100 base point. That lot is the only lot in
`ItemLotParam` granting the item, and it is referenced only by NpcParam 252100
and its 31 scaled variants. **fact — the reference and the port both protect this
exact row**: it is one of the two NpcParam rows the drop randomizer skips when
building its pool and when assigning (`DropRandomizer.cpp:24`,
`RandomizeFunctions.cs:3125`, invariant D-I4 in `app/tools/drops_verify.py`).

**inference — the cord is relocated, not multiplied.** The lot carries a single
`getItemFlagId`, the field FromSoftware uses to mark a lot as already taken, so
the expected outcome is that the first easy-mode larva killed yields the cord and
every other larva — including the clinic one — yields nothing. The alternative,
that each larva drops its own copy, would put up to thirty cords in the world
from Easy Rom alone. Only hardware separates the two (§8). The developer accepted
the relocation and chose to match the reference either way (§10, decision 2),
with §8's hardware item 4 as the check on which reading is true.

### Maps and coverage

**fact — the reference's map lists are inconsistent between the four settings,
but each covers the map the retail game loads.** Rows 19 and 21 patch both the
live and the pre-DLC variant of their area; row 18 patches only the live one,
even though the pre-DLC Forbidden Woods holds the same three Shadow placements
with the same values. The Smithbox map aliases tag the pre-DLC Forbidden Woods
and Upper Cathedral Ward `unused`, the tag `app/tools/data/README.md` says marks
content the retail game never loads. **inference:** the omission on row 18 is
harmless and the extra work on rows 19 and 21 is wasted, so no player-visible
difference follows from either. That rests on community data, and the developer
chose to match the reference map-for-map regardless (§10, decision 3), so the
asymmetry is agreed behaviour rather than an oversight to correct.

**fact — 42 larvae in a loaded-map run with all four on**: 2 in the Forbidden
Woods, 30 at Byrgenwerth, 3 in the Research Hall, 7 in Upper Cathedral Ward.

### Where the port already stands

**fact — the port's map pipeline already guarantees the replacement model is
available.** `EnemyRandomizerJob` has a `MergeModels` phase that appends every
enemy model seen in any loaded map to every loaded map, and the Central Yharnam
maps that declare `c2521` are all in its 24-map list. It runs on every run
regardless of which settings are on.

**fact — the port already writes every map on every run**, including runs with
nothing enabled, so an easy-mode-only run has a natural place to live. `ENABLE
MERGO DARKNESS` is the existing precedent for a non-randomizing toggle inside
this job.

**fact — the port already targets the same placements when bosses are on.**
`AddTheRestInMap` replaces `c2120_0001/0002` and `c4030_0001/0002/0003` with
random bosses, and the arena slots `c2120_0000`, `c4030_0000`, Rom and the
Celestial Emissary pair are assigned by the main boss pass. Which of the two
passes writes last decides §2's combined-setting row.

---

## 5. Terminology

*Deliberately empty — §1 and §2 use the game's own vocabulary and nothing here
needs a definition. The heading stays so section numbers remain stable across
documents.*

---

## 6. Scope

### In scope

Backlog rows **18, 19, 20 and 21** — four user-facing toggles, all default off,
each replacing the duplicate bodies in one boss arena, exactly as the reference
selects them.

**Why one spec covers four rows.** The evidence for grouping, rather than an
assumption:

* They are **one function** in the reference. `EasyModes` is a single method with
  a four-way branch; rows 18–21 are four `if` statements choosing which map
  filename to hand it.
* They share **one prerequisite that cannot be specified per row**: the
  replacement creature's identity, where it is captured from, and the guarantee
  that its model is declared in the target map. Specifying row 18 alone would
  either leave that undescribed or repeat it in three more specs.
* They share **one player-facing consequence** — the umbilical cord (§4) — which
  is only visible if you count all 42 replacements rather than row 18's two.
* The backlog itself groups them in §6 and says all four are "one small
  table-driven pass".
* Rows 19 and 21 are where the two measurement hazards actually live: a family
  prefix that empties a group, and an enumerated list that could have missed
  bodies the map holds. A row-18-only spec would ship without testing either.

The cost is that the developer approves four behaviours at once instead of one.
That choice was put to the developer and taken on 2026-09-16 (§10, decision 1).

### Out of scope

* **Any change to how the four fights are randomized.** These settings overwrite
  placements after the fact; the boss and enemy pools, eligibility rules and
  fixup groups stay exactly as they are.
* **Choosing a different replacement creature.** Out of scope unconditionally:
  decision 2 fixes the replacement as the reference's own creature, drop intact.
  Blanking or substituting the drop is equally out.
* **Reproducing the reference's picker-path defect** (§3, item 2). The port's
  model merge does not have that hole and must not grow one.
* **Any other multi-body fight.** Micolash's, the Witches of Hemwick's and the
  Blood-starved Beast's arenas are not covered by any of the four flags, and
  extending the idea is new design rather than a port.
* **Row 22 "No Scaling" and row 23 "Custom Scaling".** §4 shows scaling cannot
  change the outcome here, so there is no dependency in either direction.
* **Row 16 "Unchanged Bell Maidens".** `docs/features/016-unchanged-bell-maidens/spec.md` §6 already rules these rows
  out of its own scope, correctly: an exclusion-list change and a placement
  overwrite share nothing but the file format.
* **Chalice dungeons** — out by standing decision. Noted only because the
  replacement creature's model also exists in chalice map data and stays
  untouched.

---

## 7. Constraints and decisions

**Reference fidelity (`CLAUDE.md` §7).** The name lists, the map lists and the
replacement creature come from the reference and are matched rather than
improved. Decisions 2 and 3 confirmed that fidelity for the two places where it
was in question — the replacement creature's drop and the asymmetric map lists —
so the only thing not matched is the picker-path defect in §3, which is a defect
rather than a behaviour.

**Four independent switches.** The reference has four checkboxes with no
interaction between them, and each writes to a different map. Any combination
must be valid, including all four with every randomizer off.

**Not a randomizer.** These draw no randomness. The same seed with an easy
setting on must produce the same world as with it off, except for the affected
placements. The implementation must not consume from the RNG stream, or every
seed's meaning changes — the trap `docs/features/016-unchanged-bell-maidens/spec.md` recorded for exclusion changes
applies here in reverse.

**The replacement's model must be declared in the map being written.** A model
name the map's model list does not contain cannot be referenced by a placement.
The port's existing model merge already satisfies this for all 24 maps; the
constraint is recorded so nothing removes or conditionalises that pass without
noticing.

**Order against the boss passes.** These settings and the port's existing
`AddTheRestInMap` target the same placements. The reference's outcome is that the
easy setting wins. Whatever order the implementation picks must produce that
outcome.

**Defaults-file compatibility.** Every `RandomizerDefaults` field treats an
absent key as `false`, and `false` here means off, which means the maps are
written exactly as they are today. An older `defaults.cfg` keeps meaning what it
meant, with no special handling.

**Font.** 8x8, uppercase, digits, space and a small punctuation set — check
`app/src/Platform/Font8x8.cpp` rather than any list written elsewhere.
`EASY SHADOWS`, `EASY ROM`, `EASY FAILURES` and `EASY EMISSARY` are all clean.

**UI row growth is not a constraint.** The Enable wizard's settings and confirm
lists and the Setup Defaults list all scroll since `docs/plans/ui-scrolling.md`
shipped, so four more rows are safe on every screen they touch.

**No emulator.** Nothing in this loop can run the game; §8 splits accordingly.

---

## 8. How will we know it works?

### Acceptance criteria

With a setting on, in the map the retail game loads:

1. Exactly the placements §4 names carry the replacement creature's identity —
   its stat block, its AI and its model — and **no other placement in the map
   differs from what the same seed and settings produce with the setting off**.
2. The survivors §4 names are byte-identical to what the same run produces with
   the setting off: one Shadow, Rom herself, one Living Failure, one small
   emissary, and the Celestial Emissary.
3. Every replaced placement keeps its EntityID and its position.
4. With a setting off, the map is byte-identical to today's output for the same
   seed and settings.
5. The four settings are independent: turning one on changes only its own map.
6. `NpcParam` is unchanged by this feature — in particular the replacement
   creature's own stat block and its item lot are untouched, whether or not
   enemy-drop randomization is on.

### Automated testing

All of the above are properties of a generated map tree measured against the
vanilla tree, which is what the existing verifiers already do.
`boss_verify.py`'s MSBB reader and `enemy_lookup.py`'s diffing already read
exactly the fields involved, and the assertion-harness pattern already exists as
`boss_verify.py selftest` / `drops_verify.py selftest`. Which tool grows the
assertions, and whether the "unchanged with the setting off" comparison runs
against a second generated tree or against vanilla, belongs to the stages that
own building and testing.

What automation **cannot** establish: that the replacement is harmless, that the
fights still end, and where the umbilical cord ends up. Those are behaviour.

### Hardware testing

Two runs of the same seed, one with the four settings off and one with them on,
plus vanilla as the third reference point.

1. **Shadows of Yharnam.** One Shadow fights; two small creatures stand in for
   the others. Do they attack? Do they die in one hit? **Does the fight end**
   after the third body dies, and does the arena fog lift? This is the §4
   inference that matters most — if the fight does not end, the feature is broken
   in a way no verifier can see.
2. **Rom.** With every child replaced, does Rom's fight still progress through her
   teleports, and does she die normally? Her phases are the most heavily scripted
   of the four.
3. **Living Failures** and **Celestial Emissary.** One opponent each, the fight
   ends normally, and the Emissary still grows out of the surviving small body.
4. **The umbilical cord.** Kill an easy-mode larva and check what you get. Then go
   to Iosefka's Clinic and check whether the larva there still drops one. One cord
   in total is the expected result (§4). A cord per larva would contradict the
   reading §4 records, and is the trigger for revisiting decision 2 with evidence.
5. **With the settings off**, confirm the four fights are exactly as they were.
   This is what makes runs 1–4 mean anything.

**Failure would look like:** a fight that never ends because a replaced body no
longer registers as dead; a replacement that is hostile, or tough enough to
matter; an easy setting losing to boss randomization so the duplicates are still
bosses; or a map that fails to load at all, which would point at the model
declaration (§7).

---

## 10. Decisions

*§9 Open questions is deliberately absent. All four questions it held were put to
the developer and answered on 2026-09-16 and are recorded below. The heading is
not reused and nothing is renumbered, so other documents can keep citing these
sections by number. §1–§8 have been reconciled against the answers.*

| Date | Decision |
| --- | --- |
| 2026-09-16 | **1. Scope: rows 18, 19, 20 and 21 together, as one feature over one shared pass.** The spec's recommendation, taken on the grounds in §6: one reference function, one shared prerequisite, one shared player-facing consequence. Four behaviours are approved at this gate rather than one. |
| 2026-09-16 | **2. Match the reference on the replacement creature, and accept that it relocates One Third of Umbilical Cord.** `c2521`/NpcParam 252100 is planted as-is, drop intact. Recorded as deliberate reference fidelity per `CLAUDE.md` §7, not an oversight. §8's hardware item 4 is the check on which of the two readings of `getItemFlagId` is true; if hardware shows a cord per larva, the decision is revisited with evidence. |
| 2026-09-16 | **3. Match the reference's map lists exactly, asymmetry included.** Easy Shadows patches `m27_00_00_01` only, while Easy Rom and Easy Emissary patch both variants of their area. The asymmetry is deliberate and must be carried into the plan as such, not silently normalised. |
| 2026-09-16 | **4. Four independent toggles, one per fight, each default off**, labelled from the reference's own checkbox text uppercased: `EASY SHADOWS`, `EASY ROM`, `EASY FAILURES`, `EASY EMISSARY`. |

**Notes carried from the decisions into the body:**

* Decision 1 makes §4's row-19 finding — a family prefix that empties a group of
  thirty rather than thinning it — a hazard this feature must manage, not a
  reason to defer the row.
* Decision 2 fixes the replacement creature, so §6's "choosing a different
  replacement creature" stays out of scope unconditionally.
* Decision 3 means the `m27_00_00_00` omission is agreed behaviour. Stage 1 must
  not add the map, and §8 asserts the live variant only.
* Decision 4 is recorded as a **rule** keyed to the reference's labels, so a
  fifth easy-mode setting — if the reference ever grows one — inherits the naming
  without a new decision.

---

## Appendix — research notes for stage 1

*Not part of the specification, and not covered by the developer's approval.
These are pointers from the spec investigation to save stage 1 a search. Verify
anything here before relying on it.*

**Where the behaviour lives**

* `reference/Randomizer/MainWindowComponents/MainWindow.xaml.cs` — `EasyModes`.
  The whole feature; four branches, one write per map.
* `reference/Randomizer/MainWindowComponents/StartFunctions.cs:1239-1256` — the
  four call sites and their map lists, at the top level of the run.
* `reference/Randomizer/MainWindowComponents/MainWindow.xaml.cs` —
  `GenerateEnemyList`, the `addedStoneGuyBool` block near its top. The only place
  the replacement's three values are ever set, and the reason they are unset on
  the picker path.
* `reference/Randomizer/MainWindowComponents/StartFunctions.cs` — the model-merge
  loop that runs before everything else. Explains why the reference can name a
  model the target maps do not declare.
* `app/src/Randomizer/EnemyRandomizer.cpp` — `StepMergeModels` (the port's
  equivalent guarantee) and `StepWriteMap` (the per-map mutation point, which also
  calls the scaling pass before serialising).
* `app/src/Randomizer/BossRandomizer.cpp` — `AddTheRestInMap` and `kFixups`.
  Targets the same placements this feature does; ordering between them decides
  §2's combined-setting row.
* `app/src/Randomizer/PermaDarkness.h` and `EnemyRandomizerOptions` — the existing
  pattern for a non-randomizing toggle carried through the same job.
* `app/src/Randomizer/DropRandomizer.cpp` — `IsExcludedNpcRow`. Confirms the
  replacement creature's drop row is deliberately protected on both sides.

**Worth checking early**

* Whether the easy pass runs before or after `ApplyBossParamScaling` inside the
  write step. §4 measured that it cannot change the result, so this is a
  readability choice rather than a correctness one — worth knowing before it turns
  into a debate.
* The reference's ordering guarantee is "the easy pass wins over the boss passes".
  Reproducing it is an ordering question inside one map's mutation, not a new
  phase.
* `app/tools/boss_verify.py`'s `Msbb` class and `app/tools/param_offsets.py`'s
  `load_defs` / `load_param` / `param_rows` were enough for every measurement in
  §4. Part position is the three floats at entry+0x28 if placement clustering
  needs re-checking.
* Item names resolve from `msg/engus/item.msgbnd.dcx`; its members are named in
  Japanese and the FMG inside is the wide (DS3/BB) variant. No repository tool
  reads FMG yet.

**Dead ends already walked**

* `docs/deferred-ideas.md` has nothing on these rows; nothing here reopens a
  declined decision.
* `docs/features/016-unchanged-bell-maidens/spec.md` §6 rules rows 18–21 out of row 16 on the
  grounds that the machinery is not shared with an exclusion-list change. That is
  correct, and it is not an argument about how rows 18–21 relate to each other.
* Searching the reference for a constant holding the replacement creature's
  identity finds nothing — it is captured at runtime from map data, not declared.
* `RandomizerSettings.h` is not the settings struct in use; `RandomizerDefaults.h`
  and `EnemyRandomizerOptions` are.
