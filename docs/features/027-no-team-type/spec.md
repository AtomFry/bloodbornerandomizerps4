# Feature 027 — No Team Type

**Status: APPROVED** — the developer read and approved this spec on 2026-09-28.
Human gate 0h is passed; it is ready for `/plan 27`. §9 is deleted; its three
questions were answered the same day and are recorded as D1–D3 in §10, D3 having
been taken against this spec's own recommendation.

**Reference:** `reference/Randomizer/MainWindowComponents/RandomizeFunctions.cs` — `TeamTypeRando()`, gated by `teamTypeBool` from the `AllDmgAll` checkbox labelled "No Team Type"

**Plan:** `docs/features/027-no-team-type/plan.md` — added after the spec is approved

---

## 1. What does this feature do?

Every creature in Bloodborne belongs to an allegiance — a group the game uses to
decide who is willing to fight whom. Ordinary enemies share one allegiance. The
Hunter's Dream residents, the window NPCs and the other talkers share another.
A handful of creatures sit on allegiances of their own, among them Gehrman, the
Witch of Hemwick, the Blood-starved Beast, the Brain of Mensis and every
Maneater Boar in the game.

This setting collapses all of that into one. With it on, every creature in the
game — trash, boss, NPC, the Doll, the Messengers — is put on the single
allegiance that Gehrman and the Maneater Boars already use. Nothing keeps an
allegiance of its own.

The reference tool's author intended this to make everything willing to attack
everything: the checkbox's internal name is `AllDmgAll`, and the backlog row
describes it as "everything is hostile to everything — enemies fight each
other".

**That intended outcome is not established, and the evidence is against the
simplest reading of it.** The value the reference writes is not a special
"attack everyone" marker — 378 of the game's creature records already carry it,
and the creatures that carry it do not attack each other in the vanilla game.
Collapsing every creature onto one shared allegiance is, on the face of it, the
opposite of making them enemies of one another. §4 records what is measurable;
D1 ships it anyway, off by default, and §8 settles it on hardware.

This is a port of an existing reference behaviour. It adds no new idea of its
own.

---

## 2. What does the player experience?

| Setting | Off | On |
| ------- | --- | -- |
| **ENEMIES ON SAME TEAM** (D4, superseding D3) | Creatures keep the allegiances the game shipped: ordinary enemies on one, the Hunter's Dream residents and the game's talkers on another, and a few singletons — Gehrman, the Witch of Hemwick, the Blood-starved Beast, the Brain of Mensis, the Maneater Boars of the Central Yharnam sewers, the Forbidden Woods and Yahar'gul — on a third. | Every creature in the game is moved onto that third allegiance, the one Gehrman and the Maneater Boars already share. **Nothing is left on a different one** — not the Doll, not Gehrman in his wheelchair, not the Messengers, not a single boss or trash mob. No creature is added, removed, replaced or moved; only who they belong to changes. |

**What the player will actually see is not established.** Three outcomes are
consistent with what is measurable offline: enemies turn on one another
(the reference author's intent), previously peaceful residents of the Hunter's
Dream become hostile, or nothing visibly changes at all. The evidence in §4
leans toward the third — the Hunter's Dream Messengers already share the
ordinary enemies' allegiance today and are entirely peaceful, so allegiance
alone plainly does not decide who fights — but leaning is not knowing. Deciding
between the three is a hardware test (§8), and the shipped help text cannot be
written honestly until it has been run. D3 takes an effect-based label anyway and accepts that risk; §8's first criterion is therefore whether the label is true.

**When it takes effect.** At world generation, not during play. It applies to
the whole game from the first spawn, in every zone including the Hunter's Dream
— the setting is not confined to the areas the other randomizers touch.

**Combined with other settings.** It is independent of everything else. Because
every creature record receives the same allegiance, it does not matter which
creature the enemy or boss randomizer put in a given spot: the result is the
same either way, and the same seed produces the same world with this setting on
or off.

---

## 3. What does the existing randomizer do?

`docs/windows-randomizer-technical-review.md` §5.11 already covers this function
and **its account is still accurate**: line numbers, the fixed value, and the
"live, but not random" classification all check out against
`RandomizeFunctions.cs:3264-3304` as it stands today. The review's §13 open
question 5 — why a deterministic faction-normalisation pass exists inside an
identity randomizer at all — is also still open, and §13 item 4 lists
`AllDmgAll` "No Team Type" among the UI labels whose real meaning was never
established. This spec does not close either.

What the reference does, in full:

* `MainWindow.xaml:207` declares the checkbox `AllDmgAll`, content "No Team
  Type", with no `IsChecked` — so it is **off by default**. It has a `Checked`
  handler and no `Unchecked` handler, but `SetBooleans()`
  (`BooleanHandler.cs:63`) re-reads every checkbox at the start of a run, so
  unticking it is honoured. There is no tooltip and no other explanation of the
  label anywhere in the tool.
* `UIComponents.cs:2300-2304` carries the author's only note on the feature, a
  comment reading `//cell 100 team type 25`.
* `StartFunctions.cs:1557-1560` calls `TeamTypeRando()` when the box is ticked,
  after enemy drops and before the face/blood/gem/talk passes. Nothing else
  gates it and it gates nothing else.
* `TeamTypeRando()` loads every PARAMDEF and the whole param archive, takes
  `NpcParam`, and loops **every row with no exclusion of any kind**, writing
  `Cells[100].Value = (byte)25`. It then writes the whole archive back.

Two harmless untidinesses, recorded so nobody mistakes them for behaviour: the
local is named `eight` and holds `25`, and a `List<PARAM.Row> talkList` is
declared and never used. Neither changes what is written.

No RNG is involved. The pass is deterministic and writes the same value on every
run. There is **no protected list, no NPC exemption, no boss exemption and no
map scoping** — `NpcParam` is a single global table, so the pass reaches every
creature in the game including the Hunter's Dream and the chalice dungeons.

None of the reference defects catalogued in the technical review touch this
path: it has no pool, no draw, and no off-by-one to inherit.

**The reference's own name disagrees with its mechanism.** `AllDmgAll` reads as
"all damage all", but the mechanism is "all share one allegiance". Whether the
engine turns the second into the first is exactly what is not known.

---

## 4. What do we know?

Measurements are against `data/vanilla/dvdroot_ps4` using `app/tools/param_offsets.py`
and `boss_verify.py`'s MSBB reader.

**The field. Fact.** `NpcParam` declares 192 fields in a 388-byte row. Cell
index 100 — the cell the reference writes — is `teamType`, a `u8`, at byte
offset 303. Its PARAMDEF enum type is named `NPC_TEAM_TYPE`. The reference's
`Cells[100]` therefore does hit the team field and the width matches, so nothing
is truncated or spilled.

**The table's size. Fact.** `NpcParam` holds **31,398 rows**. 24,242 of them are
the `900xxxxxx` per-zone scaling variants listed in
`app/src/Randomizer/NpcScalingTable.h`, every one of which exists in the param;
the remaining ~7,156 are the game's base creature records.

**What the values are today. Fact.** Thirteen distinct `teamType` values occur:

| Value | Rows | What carries it |
| --- | --- | --- |
| 23 | 30,076 | the ordinary-enemy allegiance — and also most of the Hunter's Dream Messengers |
| 25 | 378 | **the value the reference writes** |
| 24 | 326 | mixed: Winter Lantern, Nightmare Executioner, some beast patients |
| 26 | 312 | the NPC/talker allegiance — the Doll, Gehrman in his chair, Master Willem, the window NPCs |
| 29 | 105 | the one allegiance any SpEffect in the game can move a creature onto (row 4741 is the only `changeTeamType` in `SpEffectParam`) |
| 27 | 104 | mixed: Abhorrent Beast, Orphan of Kos sliver, Labyrinth Ritekeeper |
| 20 | 81 | the hostile NPC hunters, plus one Father Gascoigne record |
| 28 | 8 | invisible/scripted placeholders |
| 22 | 4 | scripted placeholders |
| 0, 4, 19, 21 | 1 each | one row each |

**Value 25 is not a "hostile to everything" marker. Fact, and it contradicts the
backlog row.** 378 rows already carry it in the shipped game, and in the 24 base
map files they account for 20 placements: Gehrman, both Witch of Hemwick
placements, the Blood-starved Beast, the Brain of Mensis and the dropped brain,
and every Maneater Boar in the game — the Central Yharnam sewers, three in the
Forbidden Woods, two in Yahar'gul. None of those attack one another in vanilla,
and the Witch of Hemwick cooperates with Mad Ones that sit on a *different*
allegiance (23) today. So the value is an ordinary group identifier, not a flag.

**Allegiance alone does not decide hostility. Fact, and this is the single most
useful thing measured here.** 19 of the 21 Messenger placements in the Hunter's
Dream already carry `teamType` 23 — the same value as 30,076 ordinary enemy
records — and they are completely peaceful. Whatever makes a creature attack the
player, it is not this field on its own. This is the evidence behind §2's
expectation that "nothing visibly changes" is a live possibility, and it is the
main reason a protection list looks unnecessary, and D2 takes none.

**What the pass changes. Fact.** Setting every row to 25 changes the value of
**31,020 rows**; 378 already hold it. Because the write is unconditional and
uniform, there is nothing for it to skip.

**What survives in the arena. Measured, not inferred.** Across the 24 base map
files there are **2,877** enemy-type placements, referencing **553** distinct
`NpcParam` rows. Their allegiances today: 2,431 placements on 23, 273 on 26, 60
on 22, 41 on 24, 26 on 20, 20 on 25, 14 on 27, 5 on 28, 3 on 29, 2 on 4, 2 on
21. After the pass, **2,857 placements change allegiance, 20 already have it,
and zero keep a distinct one.** The three placements that reference a row which
does not exist in `NpcParam` — a Chime Maiden, a Nightmare Apostle and a Chapel
Giant — are all cutscene dummy parts, not bodies in the world. So nothing a
player can see is spared.

**Who the affected non-enemies are. Fact.** The 273 placements on allegiance 26
are the game's non-combatants and set-piece characters: the Plain Doll in the
Hunter's Dream and in the Abandoned Old Workshop, Gehrman in his wheelchair, two
of the Hunter's Dream Messenger groups, Master Willem, Lady Maria as she kneels
before the fight, Ludwig's talking head, the snail women, the Nightmare
Apostles, the Enlarged Head patients who talk, and the invisible speakers behind
Yharnam's windows. The 26 placements on allegiance 20 are the hostile NPC
hunters. All of them move to 25.

**Map-level protections do not help here. Fact.** `m21_00_00_00` (the Hunter's
Dream) is never randomized — a deliberate reference quirk recorded in
`docs/design-decisions.md`. That protection is map-scoped and `NpcParam` is not,
so it gives the Doll, Gehrman and the Messengers no cover from this setting.

**Chalice dungeons. Inference.** `NpcParam` is one global table and chalice maps
are generated rather than shipped as files, so the pass necessarily reaches
chalice creatures too and the amount cannot be measured from the vanilla tree.
This is an unavoidable consequence of the field being global, not an expansion
of scope (§6).

**It cannot collide with the port's existing `NpcParam` work. Fact.**
`app/src/Randomizer/DropRandomizer.cpp` writes `itemLotId_1` at row offset 44
(reading 44 and 48). `teamType` is at offset 303. Different bytes of the same
388-byte row, so the two passes are independent in either order. The port's
other param features write different members entirely — `CharaInitParam`,
`ShopLineupParam`, `EquipParamWeapon`.

**It cannot collide with enemy randomization either. Inference, from the
uniformity of the write.** The enemy randomizer rewrites a placement's
`NPCParamID`, `ThinkParamID` and model to those of another creature that exists
in vanilla (`EnemyRandomizer.cpp:851-855`). Because every `NpcParam` row ends up
with the same `teamType`, the answer to "whose team type applies" is the same
whichever row the placement now points at, and the two passes commute. The same
argument covers the boss randomizer, the boss scaling pass — all 24,242 scaling
variants are rewritten too — and the easy-mode larva substitutions.

**What the semantics of the values are. Not established.** `NPC_TEAM_TYPE`'s
enumerated names are community paramdex data and are not in this repository. No
param in the game defines the relations between teams: the only three other
fields that mention teams are `SpEffectParam.changeTeamType` (used by exactly
one row, 4741, which moves a creature to 29), `NpcThinkParam.TeamAttackEffectivity`
and `BulletParam.isHitBothTeam`. The relation table lives in the executable.
Verify by hardware test only — `docs/known-traps.md` states the rule directly:
"Byte decoding does not prove game behavior."

---

## 5. Terminology

**Allegiance / team.** The group the game assigns a creature to, held per
creature record. This spec says "allegiance" in player-facing text and
`teamType` when naming the field.

**Creature record.** One `NpcParam` row. Many placements can share one; one
creature model can use several.

---

## 6. Scope

### In scope

* Backlog row 27 only.
* One off-by-default setting that, when on, writes the reference's fixed
  `teamType` value to every `NpcParam` row, with no exclusions.
* Its config key, UI row, help text and progress reporting.
* Automated verification that the write reaches every row and touches no other
  byte.
* A hardware test whose purpose is to establish what the setting actually does
  in play, and to settle the shipped label and help text.

### Out of scope

* **Chalice dungeons.** Out of scope per `CLAUDE.md` §7. The field is global, so
  chalice creatures are reached; no chalice-specific behaviour is added,
  measured or supported.
* Any protection list, NPC exemption or boss exemption. D2 takes none, and if
  the hardware test shows the Hunter's Dream breaks, the smallest possible
  exemption is added afterwards with that observation as its justification.
* Writing any value other than the reference's. D1 ports it verbatim.
* Any other `NpcParam` field, including `npcType`, and any change to enemy
  drops.
* The remaining reference settings that share this pass's "deterministic
  normalisation mislabelled as a randomizer" shape — `AiSoundParamRandomizer`
  (review §5.13) in particular. Separate backlog rows.
* Renaming or re-describing the backlog row in
  `docs/randomization-feature-spec.md`; that belongs to the documentation stage,
  which should correct it (the finding in §4).

---

## 7. Constraints and decisions

* **Match the reference.** `CLAUDE.md` §7. The value written is the reference's
  fixed one, and the row set is every row with no exclusions. A protection list
  would be a deviation; `docs/design-decisions.md` and
  `docs/enemy-exclusion-history.md` record what the last protection list on this
  project cost — 45 heuristic patterns that froze 119 placements, added to
  explain a symptom whose real cause was a contaminated vanilla source, and
  removed as a deviation with no evidence behind it. Any protection here needs
  its own evidence, not a plausible fear.
* **Byte decoding does not prove behaviour.** `docs/known-traps.md`. The setting
  must not be given a label or help text that asserts an in-game effect the
  project has not observed.
* **Off by default**, matching the reference and every other setting in the port.
  An older `defaults.cfg` without the key must read as off.
* **Determinism.** The pass draws no randomness. The same seed must produce the
  same world with the setting on or off, as with `ENABLE MERGO DARKNESS` and
  `START WITH HUNTER TOOLS`.
* **One archive pass.** The port loads and writes
  `gameparam.parambnd.dcx` once per run for whichever param features are on.
  This setting joins that set, which means turning it on alone still costs the
  full archive rewrite — and this port's compressor writes stored blocks, so
  that archive lands roughly 15× larger than the original. That cost is already
  accepted for the existing param features.
* **The settings block is pinned to an exact byte count.**
  `app/tools/pool_verify.py` asserts equality, currently a 728-byte settings
  block and a 779-byte worst-case `defaults.cfg`, with `char buf[1024]`. A key
  `no_team_type=1` costs 15 bytes with its newline, taking those to 743 and 794.
  **Feature 011's two keys are a separate, independent 47 bytes** (728 → 775,
  779 → 826); whichever feature lands second adds its own delta to the other's
  figure, and neither spec's arithmetic depends on the other's. Both fit the
  buffer with room to spare.
* **UI plumbing must follow the existing shape**, not invent a new one: one
  `SettingDef` entry, one `RandomizerDefaults` field, one config key written and
  read, the `WorldActivation` options mapping, the run decision, and a progress
  line.
* **Map-scoped protections do not apply.** The `m21_00_00_00` quirk protects
  nothing here (§4).

---

## 8. How will we know it works?

### Automated testing

What must be observably true of the generated tree:

* Every one of `NpcParam`'s 31,398 rows carries the reference's `teamType` value
  after a run with the setting on, and no row carries any other value.
* No byte outside `teamType` changes in any row — in particular `itemLotId_1`
  still matches whatever the drop pass did or did not do to it, and the row
  count, row IDs and row stride are unchanged.
* With the setting off, `teamType` matches the vanilla tree in every row.
* Turning the setting on does not change any map file, and does not change the
  world produced for a given seed by any other setting.
* The setting round-trips through `defaults.cfg` and a world revision; an older
  config without the key loads as off; the pinned settings-block byte count is
  updated to match reality rather than tolerated.
* The setting's row fits the settings pane's label-plus-value budget and its
  help text is drawable in the shipped font.

### Hardware testing

The hardware test is not a confirmation here — **it is the only way to learn
what the setting does**, and its result determines the shipped label and help
text. Per D3 its FIRST question is whether the label `ENEMIES HOSTILE TO EACH
OTHER` is true. It must establish, with the setting on and nothing else on:

* **The Hunter's Dream is intact.** The Doll can be spoken to and does not
  attack; Gehrman in his wheelchair can be spoken to and does not attack; the
  Messengers behave normally; levelling up, the shop and travel all work. This
  is the failure case that would end a playthrough, and it is the first thing to
  look at.
* **Whether enemies fight each other.** Somewhere densely populated with mixed
  creature types — Central Yharnam's mob, the Forbidden Woods, Yahar'gul — watch
  for creatures attacking one another unprompted or taking damage from one
  another's attacks.
* **Whether quest NPCs still work.** A window NPC conversation, and at least one
  of Master Willem, the kneeling Lady Maria, or Ludwig's head, all of which move
  off the NPC allegiance.
* **Whether boss fights with cooperating adds still behave.** The Witch of
  Hemwick and her Mad Ones is the sharpest case, because those two are on
  different allegiances today and end up on the same one.
* **Whether anything changes at all.** "No observable difference" is a real and
  likely result, and recording it is as valuable as recording a dramatic one.

---

## 10. Decisions

All three questions were put to the developer on 2026-09-28 and answered the same
day. Two took the spec's recommendation; the third did not, and is marked.

| Date | Decision |
| ---- | -------- |
| 2026-09-28 | **D1 — Port it verbatim now, off by default.** One fixed byte written to every `NpcParam` row, exactly as the reference does, with no attempt to guess the effect first. This is the cheapest route to an answer and the only option that matches the reference; the setting is off by default and touches one byte per row, so the risk of shipping it is bounded. The hardware test in §8 is framed as **discovery**, not confirmation — it closes the last reference setting whose meaning `docs/windows-randomizer-technical-review.md` §13 left open |
| 2026-09-28 | **D2 — No protection. All 31,398 rows are written**, matching the reference, which excludes nothing. Two things carried this: 19 of the 21 Hunter's Dream Messenger placements already sit on the ordinary-enemy allegiance and are perfectly peaceful, so the field is demonstrably not sufficient to make something hostile; and `docs/enemy-exclusion-history.md` records what this project's last speculative protection list cost — 45 patterns and 119 frozen placements, added to explain a symptom that had a different cause, then removed as an unjustified deviation. The Hunter's Dream is the first item of the hardware test precisely so this can be revisited with evidence rather than guessed at |
| 2026-09-28 | ~~**D3 — The setting is called `ENEMIES HOSTILE TO EACH OTHER`.**~~ **Superseded by D4.** *Taken against this spec's recommendation*, which was a mechanism-flavoured name (`ONE FACTION FOR EVERYTHING`) on the grounds that `docs/known-traps.md` forbids naming a setting for a runtime effect nobody has observed. **The risk is explicit and accepted:** if the hardware test shows the write has no such effect, the label asserts something false, which is the `ENABLE MERGO DARKNESS` failure repeated. Two things follow and are binding rather than optional — the help text must not compound the claim, and **§8's first acceptance criterion is whether the label tells the truth**. The config key stays `no_team_type`, tied to the backlog row and the reference, so the label can be revised after the test without touching persistence. Category remains **WORLD**: the reasoning for it is unaffected by the label, since this setting draws no randomness and writes the same bytes every run, which makes it a sibling of `ENABLE MERGO DARKNESS` rather than of anything in ENEMIES |
| 2026-09-28 | **D4 — the setting is renamed `ENEMIES ON SAME TEAM`, superseding D3, and the three-state enum considered alongside it is declined.** Two hardware runs (setting on alone, then off) showed no difference, and the audit that followed explains why and corrects the record on three points. **(a) The reference's own label is "No Team Type".** `MainWindow.xaml:207` reads `<CheckBox x:Name="AllDmgAll" Content="No Team Type" .../>` — `AllDmgAll` is a stale *variable* name and `Content` is what the author shipped on screen, so D3's central premise (that the author's intent was mutual hostility) rested on an identifier rather than on the UI. **(b) Uniform to uniform cannot change anything.** 451 of the 553 placed `NpcParam` rows — **81.6%** — are already `teamType` 23, so replacing one shared value with another shared value is a no-op in kind, whatever 25 means. **(c) The real effect is suppression, not creation.** The enemy pool spans **six** teams (20, 23, 24, 25, 26, 27), so shuffling stands rival teams side by side and infighting *emerges*; forcing one team removes it. Cross-team adjacency measures **12.1%** with the setting off against **0%** on. Corroborated in vanilla data: Shadow of Yharnam is 23 and Maneater Boar is 25 — different teams, and reported as mutually hostile. A randomize-the-team third state was considered and **declined by the developer**: it would have raised cross-team adjacency to ~80% over a curated set, but it needs a new multi-state `SettingKind`, it would consume randomness where the other two states consume none, and the available values are unsafe to draw from freely — 26 is the passive team (the Doll, Messengers, larvae) and 20/21 are Player teams. The rename touches no identifier and no saved file, because D3 already kept the key `no_team_type` for exactly this case. |

---

## Appendix — research notes for stage C

*Not part of the specification, and not covered by the developer's approval.
These are pointers from the spec investigation to save stage C a search. Verify
anything here before relying on it.*

**Where the behaviour lives**

* `reference/Randomizer/MainWindowComponents/RandomizeFunctions.cs` —
  `TeamTypeRando()`, the whole feature, about forty lines including the param
  load and save.
* `reference/Randomizer/MainWindowComponents/UIComponents.cs` —
  `AllDmgAll_Checked`, which carries the author's only comment about the field.
* `app/src/Randomizer/DropRandomizer.cpp` — the pattern to follow exactly: find
  the member, `ParseParamRows`, read and write one field per row in place, no
  rebuild. This feature is that file with one offset and no RNG.
* `app/src/Randomizer/EnemyRandomizer.cpp` — `StepItemData()`, where a new param
  pass is hung, and `AnyParamFeature()` in `EnemyRandomizer.h`, which decides
  whether the archive is loaded at all.
* `app/src/Param/ParamBnd.h` — `ParseParamRows` and `FindParamMember`.
* `app/src/Game/WorldActivation.cpp` around the options mapping, and
  `app/src/UI/WorldEditorScreen.cpp` for the SKIPPING line and the result line;
  `app/tools/worlds_verify.py` pins the mapping field for field.

**Worth checking early**

* `app/tools/param_offsets.py fields <dvdroot> NPC_PARAM_ST 100` gives the field
  name, type and byte offset directly; do not hand-count it.
* `app/tools/pool_verify.py` asserts the settings-block size as an **exact**
  equality and will fail the moment a key is added. The comment above it narrates
  the arithmetic and expects to be extended, not replaced. Feature 011's plan
  moves the same numbers; whichever lands second rebases on the other.
* `app/tools/settings_ui_verify.py` checks the label-plus-value width budget and
  the drawable character set for a new settings row.
* `app/tools/boss_verify.py`'s `Msbb` reader plus `enemy_lookup.py`'s
  `BASE_MAPS` and `names.py`'s `model_name` is the three-line way to turn param
  rows into named creatures and placement counts. That combination produced
  every count in §4.
* `app/src/Randomizer/NpcScalingTable.h` contains roughly three quarters of
  `NpcParam`'s row IDs as `900xxxxxx` scaling variants; any per-row assertion
  needs to expect them.
* The chalice map entries under `map/mapstudio` are **directories**, not `.msb.dcx`
  files, so a naive listing loop will hit a `PermissionError`. Filter on the
  extension.

**Dead ends already walked**

* Looking for `NPC_TEAM_TYPE`'s enumerated value names anywhere in the repo or
  the shipped data: they are not there. The PARAMDEF names the enum type and
  stops.
* Looking for a param that defines which teams are hostile to which: there is
  none. `SpEffectParam.changeTeamType`, `NpcThinkParam.TeamAttackEffectivity`
  and `BulletParam.isHitBothTeam` are the only other team-related fields in the
  whole paramdef set, and none of them carries a relation table.
* Trying to derive the value's meaning from which creatures carry it in vanilla:
  it produces a suggestive list and no conclusion. See §4.
* Looking for a prior decision about this row in `docs/deferred-ideas.md` or
  `docs/design-decisions.md`: neither mentions teams or factions.

---
