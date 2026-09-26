# Feature 032 — Bypassed Enemies

**Status: APPROVED**

**Backlog row:** `docs/randomization-feature-spec.md` §3, row **32**. The narrative
subsection "Row 32 — bypassed enemies, and what it replaces" is the origin of
this spec; every claim it makes was re-measured here, and one was found to be
missing rather than wrong (§4, F7).

**Reference:** **New capability.** The reference tool has no user-facing control
that protects a creature's own placements. Its nearest relatives — the
`excludeEnemiesBool` text box and the `bellMaidenBool` checkbox — are described
in §3.

**Plan:** `docs/features/032-bypassed-enemies/plan.md` *(once stage C has run)*

---

## 1. What this feature does

With **RANDOMIZE ENEMIES** on, every creature in Bloodborne plays two roles at
once. It is a set of *places it stands* — the crows on the Central Yharnam
rooftops, the maidens kneeling by their bells — and it is an *identity that can
be handed to somewhere else*, so that a crow can turn up in the Research Hall.

The port already lets you switch off the second role. **ENEMIES INCLUDED** (row
9) decides which creatures may be used as replacements; untick Carrion Crow and
no placement in the game becomes a crow. It does nothing at all about the first
role — the crows themselves are still overwritten by whatever the seed draws.

This feature is the missing half. It is a second checklist, one row per creature
that has a placement the randomizer can overwrite, with **nothing ticked by
default**. Tick a creature and it leaves the randomizer entirely: the ones the
game placed stay exactly where they are and stay what they are, *and* it stops
being used as a replacement anywhere else.

That is a single idea with two effects on purpose. Protecting a creature's own
places while still letting the seed scatter new copies of it around the world is
not what anyone means by "leave this alone" (§4, F7).

**The second effect has exactly one exception, and it is deliberate.** If the
player narrows **ENEMIES INCLUDED** until every creature left in it is also
ticked here, there is nothing the randomizer may place anywhere. Rather than
refuse that run, it lets **ENEMIES INCLUDED** win the draw for that run while
keeping every skipped placement frozen (§10, D4). So in that one configuration a
skipped creature *does* appear somewhere new. Everywhere else, and in every
configuration a player reaches without deliberately narrowing the other list onto
this one, the promise holds.

It exists for three kinds of player. The one who wants one landmark encounter
left intact. The one who found a creature that behaves badly when it is moved —
or that breaks when something else is moved into its spot — and wants it out of
the way without giving up randomization. And the one who wants the hub and its
quiet, non-fighting inhabitants left as the game shipped them.

---

## 2. What does the player experience?

Counts below are the ones a player could check by walking the world; §4 holds
the measurements and separates the retail-loaded maps from the unused map
variants.

| Setting | Off | On |
| --- | --- | --- |
| **ENEMIES SKIPPED** *(a drill-in list, nothing ticked)* | Every creature the randomizer can touch is fair game in both directions: its own places get overwritten, and it gets used to overwrite other places. Across the whole game about **2,100 of the 2,270 eligible spots** change per run | Each creature you tick is taken out of the run completely. **All of its own places keep that creature**, with the same behaviour it has in the vanilla game, and **no other place in the game becomes that creature** — with the single exception of a starved selection (§10, D4). Everything you leave unticked is randomized exactly as before |

**The list is 85 creatures long, and nothing is ticked to begin with**, so
turning the feature on changes nothing until you make a choice. The rows are
sorted by name and show the creature's internal ID alongside it, because seven
names in the list belong to two different creatures each.

**The screen says what ticking a row does.** The list carries one line of
instruction under its heading, reading *SELECT ENEMIES THAT WILL NOT BE
RANDOMIZED*. It is there because this list is the opposite of the one it sits
next to on the same screen — ticking a row in **ENEMIES INCLUDED** puts a
creature *into* the run, ticking a row here takes one *out* — and that is the
single thing about these lists a player is most likely to get backwards. For
the same reason the ticked/unticked column and the select-all and select-none
prompts have to read in terms of skipping rather than enabling. The line costs
nothing a player would notice: the list still shows at least eleven creatures at
a time and is eight pages long either way (§4, F13).

**What "keeps that creature" does and does not promise.** A skipped creature
keeps its appearance and its behaviour where the game put it. It does *not* keep
its exact stat block: the randomizer re-tunes enemy stats for the zone as a
separate final pass that runs on every seed, and that pass reaches skipped
creatures too, in fifteen of the game's areas (§4, F6). The creature you meet is
the same creature, at the difficulty that pass gives it. This is existing
behaviour that already applies to everything the randomizer leaves alone today,
not something this feature introduces.

**One creature has an exception, and it is the chime maidens.** The randomizer
deliberately re-randomizes six specific bell-ringing maidens in Yahar'gul on
every run, whatever else is set — behaviour inherited from the reference tool,
already shipped here, and kept. So ticking the two
chime maiden rows freezes **42 of the game's 54 maiden placements**, and in
Yahar'gul specifically **9 of its 15 maidens stay put while 6 still change**.
Every other creature on the list has no exceptions: tick it and every one of its
placements is spared. See §4, F5.

**How many are left is per creature, and the spread is wide.** The largest row on
the list spares 283 places; the median row spares 14; five rows spare exactly one
each (§4, F4). Ticking a rare creature is close to invisible in play; ticking the
commonest one visibly changes about an eighth of the world.

**Combined with the other settings.**

* With **ENEMIES INCLUDED**, skipping wins — until it would leave nothing at
  all. A creature ticked here is out of the replacement pool whether or not it
  is also ticked there, and its own places are frozen either way, so this list
  is the stronger of the two in every configuration but one. That one is the
  next bullet.
* **Narrowing both lists onto the same creatures no longer stops the run, and
  what happens instead is worth understanding.** If every creature ticked in
  **ENEMIES INCLUDED** is also ticked here, the run does not refuse and does not
  fail. Instead **ENEMIES INCLUDED** wins the draw for that run: the world is
  randomized from exactly those creatures, while all of their own placements
  stay frozen (§10, D4). Tick only the two chime maidens in **ENEMIES INCLUDED**
  and tick them here as well, and you get a world in which every other enemy is
  a chime maiden and the maidens the game placed are still at their bells. The
  output tree is complete and everything else you turned on is applied. **This
  is the one case where ticking a creature here does not stop it appearing
  somewhere new**, and the screen should say so rather than let it be discovered
  in play.
* **Ticking the chime maidens is not that case.** Ticking the two chime maiden
  rows and leaving the rest of the list alone is the ordinary use of the
  setting: the maidens the game placed stay as they are, and the other eighty
  creatures go on being drawn as replacements everywhere else, exactly as they
  do today.
* With **RANDOMIZE BOSSES**, nothing changes. Boss arenas are a separate list
  with a separate picker, and no boss placement is one of the 2,269 this feature
  can protect (§4, F8).
* With **RANDOMIZE ENEMY DROPS**, a skipped creature still drops randomized
  loot. Drops are attached to the creature type, not to the place it stands
  (§4, F9).
* **The seed's meaning changes.** Skipping a creature removes its places from
  the sequence of rolls the run makes, so the same seed with a different skip
  list is a different game everywhere, not the previous game with some creatures
  pinned. That is true of any exclusion change and is recorded so it is not later
  mistaken for a defect.

**One existing setting disappears from the wizard.** **UNCHANGED BELL MAIDENS**
goes away when this list arrives (§10, D1). What it did is now two ticks on this
list — the two chime maiden rows — and the result is identical, Yahar'gul
exception included (§4, F5). A player who had it switched on **does not** get it
carried across: their saved configuration loses the setting and the maidens are
randomized again until they tick the two rows here. That is a deliberate choice
(§10, D1), and the user guide has to say so rather than let it be discovered in
a run.

**When it takes effect.** At commit, like every other setting: the list is read
once when the run starts and applies to the whole run. Changing it has no effect
on an output tree that has already been written.

---

## 3. What does the existing randomizer do?

Nothing equivalent, and the gap is precise rather than approximate.

**The reference's enemy blacklist is pool-only.** `excludeEnemiesBool`
(`StartFunctions.cs:536-563`) chops the `oopsAllString` text box into
five-character model IDs and removes matching entries from `enemyData` — the list
of identities that can be drawn as replacements. It never touches placements.
Type a creature into that box and the game still overwrites every one of that
creature's own spots; it simply never puts it anywhere new. That is the same
semantics the port already ships as **ENEMIES INCLUDED**, and it is the half this
feature is not about.

**The reference's only placement protection is hard-coded.** `unusedPlusBossList`
(`MainWindow.xaml.cs:201-208`) is 104 entries of name substring, built at
startup, with one entry removed at `StartFunctions.cs:580`. It is consulted both
when building the pool and when deciding what to overwrite
(`RandomizeFunctions.cs:26-33`), which is what makes an entry on it a true
skip — but the list is fixed in the source and the user cannot see or change
it. The port transcribed it verbatim as `EnemyExclusionList.h`.

**One checkbox edits that list, by exactly three strings.** `bellMaidenBool`
(`StartFunctions.cs:45-49`) appends `c1050`, `c1051` and `c1055` to
`unusedPlusBossList`. That is the entire mechanism of "Unchanged Bell Maidens",
and it is the reference's only precedent for a user deciding what gets protected
in place. The port ships it as row 16, and this feature retires that row
(§10, D1) while keeping every one of its effects.

**And one block deliberately un-protects six placements.**
`RandomizeFunctions.cs:288-320` forces six named Yahar'gul maiden placements back
into randomization after both the exclusion test and the per-zone roll, whatever
the checkbox says. The port reproduces this (row 16, decision 1), and it is the
source of §2's single exception.

**So the reference can express this feature only for one creature, by checkbox,
and only because someone hard-coded that creature's ID.** Row 32 is the general
form of the same idea: the same list, editable, one row per creature. Because the
list is the reference's own and is consulted at both sites, an entry added by the
user behaves identically to an entry the reference ships — there is no new
mechanism to justify against the reference, only a new way to add to an existing
one.

**And the reference never fails a run for having nothing to place.** Each
placement's rewrite is gated on the candidate list being non-empty
(`RandomizeFunctions.cs:322`); when it is empty the reference silently leaves
that placement as the game shipped it and the run finishes normally, having
changed nothing. It has no up-front validation of the enemy blacklist at all.
The port differs (§4, F10). This feature stops it erroring, but does **not**
adopt the reference's answer wholesale: where the reference would leave the
world alone, §10 D4 randomizes it from the included list instead. The half taken
from the reference is that a run with nothing to place finishes normally; the
half deliberately not taken is that it changes nothing.

**Relevant reference quirks that carry forward.** The third string `c1055`
matches no placement in any map file in the game (§4, F5); the six-placement
Yahar'gul override is unconditional; and both of the reference's consultation
sites are plain substring tests rather than exact matches (§4, F3).

---

## 4. What do we know?

Measured against `data/vanilla/dvdroot_ps4` by reusing `app/tools/enemy_lookup.py`
(which parses the port's own exclusion headers, so the mirror cannot drift from
the engine) and `boss_verify.py`'s MSBB reader. No new format parsing was
written. F6 is additionally confirmed against a saved randomized output tree in
`data/runs/`.

**F1 — fact: 85 creatures sit on 2,269 overwritable placements.** The 24 base
maps hold **2,877** enemy placements. **608** of them are already matched by the
port's transcribed reference exclusion list, leaving **2,269** that the
randomizer can overwrite. Those carry **85** distinct creatures. This is the row
count and the total the feature can protect.

**F2 — fact: only 82 of the 85 can ever be drawn as a replacement, so the
existing picker's list is the wrong list to reuse.** Three creatures have
placements that get overwritten but are never used to overwrite anything, because
the pool-building rules reject them:

| Creature | Placements | Retail-loaded | Where |
| --- | --- | --- | --- |
| Labyrinth Ritekeeper | 2 | 1 | Cathedral Ward |
| Shadow of Yharnam (Snake) | 8 | 4 | Forbidden Woods |
| *(no community name — internal ID `c2561`)* | 9 | 9 | Nightmare of Mensis 4, Nightmare Frontier 3, Fishing Hamlet 2 |

**inference, well supported:** these three are exactly the creatures this feature
is most likely to be used on. `docs/enemy-exclusion-history.md` names the
Cathedral Ward one as the **Oedon Chapel dweller** — the non-combat NPC you send
survivors to, identified from an external data sheet, with no combat AI at all,
and the single observation that started the removed 45-entry protection list. The
Forbidden Woods one is the snake body belonging to a boss the reference protects
only partially. And the unnamed one sits within a couple of feet of a Winter
Lantern at every one of its nine positions, one per lantern, which is strong
evidence it is a paired body of that creature rather than a thing of its own.

**Reusing the 82-row list would silently make all three unfreezable**, which
removes the three best reasons to build the feature.

**F3 — fact: ticking a creature spares exactly its own placements, no more and no
fewer.** The protection works by matching the creature's five-character ID
against placement names as a substring, which invites two failure modes: taking
placements that only look like they belong to that creature, and missing ones the
map really holds. Both were measured and neither occurs. Across all 2,877
placements, the first five characters of every placement's name equal its model
without exception, and no creature's ID appears anywhere inside any other
creature's placement name. So the count in F4 is what a player would actually
find left standing.

**F4 — fact: the spread of "how many are left" is wide.** Of the 2,269,
**1,602** are in maps the retail game loads; the rest are in six pre-DLC or
unused map variants a player never sees. Per creature: minimum 1, median 14,
maximum 283. **17 creatures have 3 placements or fewer and 5 have exactly one.**
The largest rows are Huntsman (Transformed) 283 (171 retail), Carrion Crow 202
(131), Cloaked Beast Patient 104 (52), Shaggy Hunting Dog 96 (54) and Viper Pit
Hatchling 88 (44). Five single-placement rows — including Matyr Logarius's sword,
the dropped Mensis brain and one lightning summoner — are sub-entities of larger
fights that the reference's own list protects only in part.

**F5 — fact: ticking the two chime maiden rows reproduces row 16 exactly.**
Chime maidens have **54** placements (31 + 23), **34** of them in retail-loaded
maps. **12** of those 54 carry the reference's forced override, six in each of
the two Yahar'gul map variants, of which only the six in the retail map are
reachable — so **42 are frozen and 6 visibly still change**. Yahar'gul holds
15 maidens, so 9 of its 15 stay. Separately, the third pattern row 16 adds
(`c1055`) matches **zero** placements across all **43** map files in the game,
base and chalice alike: it is inert. **inference:** because the two features add
to the same list at the same two sites and the third pattern is inert, the same
seed produces the same output either way — the sequence of rolls is identical
because the same placements are skipped at the same points.

**F6 — fact, measured and then confirmed in a real run: a skipped creature is
not byte-identical to vanilla, because stat re-tuning still reaches it.** After
randomization the run applies a per-zone stat-scaling pass to fifteen
representative maps, one per area. That pass rewrites the stat-block ID of any
tracked creature in those maps whether or not it was randomized. **1,369 of the
2,269** overwritable placements are affected; their model and AI are untouched
and only the stat block changes. Confirmed against
`data/runs/20260912-enemy_and_boss_randomized`: in unscaled maps, placements the
per-zone roll skipped come out identical to vanilla, while in scaled maps the
same kind of placement comes out with its model and AI intact and its stat block
changed — 64 such in the Research Hall alone.

This bounds what §2 may promise and what §8 may assert. **It also means an
existing expectation in the tree is wrong:** `pool_verify.py maidens` asserts
that 42 frozen maidens match vanilla in all three written fields, and **26 of
those 42 are in scaled maps**, so it would report 16. The correct statement of
"frozen" is *same model and same AI, with the stat block either unchanged or the
zone-tuned variant of the vanilla one*.

**F7 — fact: coupling the two halves is what makes the feature mean what it
says.** Skipping also removes the creature from the replacement pool. Were it
placement-only, freezing the chime maidens would still scatter an expected
**101** newly-created maidens across the game per run (**70** of them in
retail-loaded maps), computed from the maidens' 4.8% share of the 333-entry pool
against the expected 2,097 randomized placements. That is the measurement behind
§1's claim, and it is the number the backlog's narrative asserted without
showing.

**F8 — fact: this cannot affect boss randomization.** None of the 2,269
overwritable placements is matched by the port's boss-name list, so the two
features act on disjoint sets of placements, and the boss picker filters a
separately built pool. Two creatures appear in both pickers' tables under the
same name but are different identities of the same model, which is existing
documented behaviour.

**F9 — fact: skipping does not protect drops.** Drop randomization rewrites what
a creature *type* drops and never consults the exclusion list, so a frozen
creature still drops randomized loot when that setting is on.

**F10 — fact: a selection can leave the randomizer nothing to place, and today
that state ends the run in an error.**

*When it happens.* Every creature that can be drawn as a replacement also has
placements of its own: the 82 drawable creatures are a subset of these 85 rows,
and the three rows that are not drawable are F2's. Measured, not assumed. So
"nothing to place" has exactly one shape — **every creature ticked in ENEMIES
INCLUDED is also ticked here** — and ticking only F2's three creatures can never
cause it.

*How easily it is reached.* Two ways, both from the shipped screens. Select-all
on this list is one button and a confirmation, and it starves every possible
**ENEMIES INCLUDED**. Otherwise it needs an inclusion list narrowed to
creatures that are all skipped, and the smallest of those is two ticks against
two ticks: the chime maiden case. That one is reachable today through row 16's
flag instead of this list; it was found when row 16 shipped and deliberately
left unfixed (`docs/features/016-unchanged-bell-maidens/log.md`, 2026-09-16). The ordinary
chime maiden configuration is nowhere near it: skipping both maiden rows with
everything else included leaves **317 of the 333** pool entries and **80 of the
82** drawable creatures.

*What happens today.* The commit-time check counts ticks in **ENEMIES INCLUDED**
and refuses only when that count is zero, so a starved selection passes it. The
run then stops when it builds its candidate list — after all six game folders
have been copied into the output folder and before any map is written. The
player is left with a complete but entirely unrandomized copy of the game that
the wizard has reported as a failure, and no statement of what to change.
**inference, from the order of the run's phases:** nothing is half-written in
this particular failure; the damage is a useless output folder and a player with
no way to tell a starved selection from a broken installation.

*The same error does a second job, which is why it is still there.* An unusable
vanilla source — a wrong tree, or a half-finished transfer —
makes no map load, which produces exactly the same empty candidate list, and
this is the only hard failure the enemy path has for it; a missing folder and a
missing map are both logged and stepped over. Removing the error without
separating the two causes would turn a bad installation into a run that reports
success having randomized nothing. This is the finding that deferred the same
change out of row 16 (`docs/features/016-unchanged-bell-maidens/plan-review.md`,
blocking finding 2.1), and §10's D4 does not relieve it — D4 gives the starved
*selection* a defined outcome, which makes separating it from the broken-source
case a requirement rather than a nicety, since the two would otherwise both end
in a run that reports success.

*Partial starvation is reachable too, and is already harmless.* A selection that
leaves few creatures rather than none needs nothing new: a map whose every
eligible placement is skipped simply randomizes nothing, and the two maps that
refuse certain creatures already check once, before placing anything, whether
the surviving creatures are all refused there, and stop re-drawing when they
are. The visible degradation is the documented one — with very few creatures
left, some end up in spots they do not fit (`docs/plans/pickers.md` §1). This
list adds a second route to those, not a new failure.

**F11 — fact: the list costs one more line of configuration and fits.**
Selections persist as one character per row. Worst case grows from **478** bytes
to **556** of the 1,024-byte buffer: **+103** for this list, **−25** for row 16's
line, which retires with its row (§10, D1).

**F12 — fact: the list renders and navigates within what is already built.** 85
rows is **8 pages** at the picker's 12 visible rows. The longest label is 39
characters — the same row that is already the longest in the 82-row list, so the
list itself needs no new layout number; the instruction line of F13 is the one
thing in this feature that does. Every name is renderable by the current font. The one
exception is the unnamed creature of F2, which has no name to render at all and
keeps the generator's `C2561 C2561` fallback (§10, D3).

**F13 — fact: a line of instruction fits on the picker screen, and there are
two ways to fit it.** The screen is 1920x1080. Above the list sit a heading and
a line giving the count and the page number; below it sit two lines of control
hints. The band between the count line and the top of the list is what a third
line has to come out of. Measured against the geometry the layout check already
asserts:

* **Keeping all twelve visible rows.** The heading moves up 20px and the gaps
  between the three lines above the list tighten from 25px to about 16px. No row
  is lost, nothing moves below the list, and every clearance the layout check
  asserts still holds.
* **Giving the list one row of room.** The list starts one row lower, which
  leaves **11 visible rows** instead of 12 and about 26px of clear space around
  the new line. The bottom of the list, the scroll hints and the footer do not
  move.

**85 rows is 8 pages either way** — 12 and 11 visible rows both give 8 — so the
visible cost of the second option is one row of the window and nothing else. The
instruction itself is well within the width: *SELECT ENEMIES THAT WILL NOT BE
RANDOMIZED* is 42 characters and renders 1,134px wide against the screen's 1,920
and the list's own 1,215px rows, and up to **71 characters** fit on one line at
that size. Every character in it is renderable by the current font.

**inference, well supported:** the same line should not be given to the two
shipped pickers as part of this work. The 82-row **ENEMIES INCLUDED** list is 7
pages at 12 visible rows and **8 at 11**, so the room-making option would cost
that list a page for a feature that is not changing it (§6).

**assumption — freezing a placement preserves whatever depends on it.** The
randomizer writes only three fields, and leaves entity IDs and every event hook
alone, so a skipped placement's scripting is untouched by construction. Whether
the creature then *behaves* as it does in vanilla is behaviour, and `CLAUDE.md`
§5 forbids concluding that from bytes — §8 makes it a hardware observation. Row
16 is the precedent and its hardware test passed.

---

## 5. Terminology

**Skipped** — a creature the randomizer ignores in both directions: its own
placements are not overwritten, and it is not used to overwrite anything. Used
throughout this document in preference to "excluded", which in this codebase
already means the reference's fixed internal list, and "unticked", which in the
**ENEMIES INCLUDED** list means only the pool half. The backlog row, this
document's title and the folder name say *bypassed*; the setting itself is
**ENEMIES SKIPPED** (§10, D2) and the two words mean the same thing here.

**Starved** — a combination of **ENEMIES INCLUDED** and **ENEMIES SKIPPED**
that leaves the randomizer no creature it is allowed to place anywhere. Distinct
from an empty **ENEMIES INCLUDED**, which is a list with nothing ticked in it and
which the wizard already refuses at commit today.

**Placement** — one spot in one map where the game puts a creature. The unit this
feature protects, and the unit the counts in §4 are in.

**Eligible / overwritable** — a placement the randomizer is allowed to change,
i.e. one the reference's fixed list does not already protect. 2,269 of the
game's 2,877.

---

## 6. Scope

### In scope

* Backlog row **32** only.
* One drill-in list of **85** creatures, nothing ticked by default, on both the
  Setup Defaults screen and the Enable wizard, persisted like the existing two
  pickers.
* Both halves of the effect for a ticked creature: its placements are spared, and
  it leaves the replacement pool.
* **Defined, non-failing behaviour for a starved selection** (§4, F10; §10, D4).
  This ships with the feature rather than after it, because the feature widens a
  reachable failure from one combination to many, and because leaving it as an
  error is the thing this round of feedback asked to be changed.
* **One line of instruction on the picker screen** saying what ticking a row
  does (§4, F13), and screen wording — the ticked column and the select-all and
  select-none prompts — that reads for a list of things being taken out rather
  than a list of things being put in.
* Reproducing row 16's behaviour, including its Yahar'gul exception, as a
  configuration of this list (§4, F5).
* **Removing the `UNCHANGED BELL MAIDENS` row** from both screens and its line
  from the configuration, with no carry-over of a saved value (§10, D1). The
  engine behaviour it drove stays, reachable as two ticks on this list.

### Out of scope

* **Per-placement protection.** This list is per creature. "Freeze the crows on
  this one roof" is a different feature with a different user interface and is
  not proposed.
* **Changing the reference's fixed 103-entry list, or re-adding anything from the
  removed `EnemyExclusionListExtra`.** `docs/enemy-exclusion-history.md` governs
  that and this feature is not an excuse to reopen it. §4's F2 argument is that
  the *user* should be able to reach those creatures, not that the port should
  protect them by default.
* **Changing the per-zone stat-scaling pass** so that skipped creatures keep
  their exact vanilla stats (§4, F6). That is the reference's behaviour, its
  opt-out is backlog row 22 (`No Scaling`), and turning it off selectively here
  would be a deviation invented by this feature.
* **Correcting `pool_verify.py maidens`' wrong frozen-maiden expectation**
  (§4, F6). Found here; it belongs to whoever owns that check, and §8 simply
  declines to repeat the error.
* **Backlog row 13 (`Bosses Can Replace Enemies`).** Unimplemented; when it
  lands it will draw from the enemy pool, and how a skip interacts with a boss
  arriving in the world is that feature's question.
* **Backlog rows 9 and 10.** Shipped. This feature reuses their component and
  their persistence shape and changes neither. In particular their two screens
  keep their present layout and page counts (§4, F13), and their commit-time
  refusal for a list with nothing ticked stays exactly as it is (§10, D5).
* **Giving `ENEMIES INCLUDED` and `BOSSES INCLUDED` the same line of
  instruction.** They would benefit from one for the same reason this list needs
  one, and the component is shared, so it is a small change — but it changes two
  shipped screens for a feature that is not about them, and one of the two
  layouts that fits the line would cost the 82-row list a page (§4, F13). Noted
  here so it is a decision rather than an oversight.
* **Chalice dungeons** — out by standing decision. Noted only because the game's
  other 19 map files contain many more placements of these same 85 creatures,
  and none of them is touched.

---

## 7. Constraints and decisions

**The default must be the no-op, and the fail-safe must be too.** Nothing ticked
means nothing changes. The existing selection type constructs to *everything
enabled* and leaves a wrong-length saved value at that default — correct for a
pool picker, exactly backwards here, where it would freeze the entire game and
produce a run that changes nothing while reporting success. That outcome is a
legitimate one when a player has asked for it and a silent failure when a
decoding mistake has, and the two are indistinguishable from the outside — which
is why a stale, truncated or absent saved value for this list must read as
**nothing skipped**.

**Row order is the meaning of a saved selection.** The existing lists persist as
one character per row against a frozen order; a regenerated order silently remaps
every saved selection onto the wrong creatures, and only a changed *count* is
detectable. This list is a different length from the existing one (85 vs 82) and
must not be stored against it or derived from it.

**The existing 82-row list must not be reused or altered.** §4, F2: three
creatures are missing from it by design. Whatever this feature does, the shipped
list and every saved selection against it keep their current meaning.

**A starved selection must produce a finished run, not an error.** This reverses
the position an earlier draft of this spec took, on the developer's instruction
of 2026-09-18: the run is to go through and do something sensible rather than be
refused. What it draws from is settled by §10, D4 — the **ENEMIES INCLUDED**
selection, with every skipped placement still frozen. The constraints below hold
alongside it.

* **Nothing may fail once the output folder has been touched.** That hazard is
  the reason the refusal was proposed, and it does not go away with it (§4, F10).
* **No replacement may ever be drawn from an empty set.** In the port that is
  undefined behaviour rather than a clean failure, which is why the in-run guard
  exists at all. Whatever replaces the guard's *failure* must keep its
  *protection*.
* **An unusable vanilla source must still fail, loudly and distinguishably.** It
  produces the same empty candidate list as a starved selection and is the one
  case where reporting success is worse than stopping (§4, F10). Separating the
  two causes is a requirement of this feature, not an optional tidy-up.
* **The player must be told when the skip list stops applying in full.** A
  starved selection still randomizes — from the included list, with the skipped
  placements frozen (§10, D4) — so the thing the player cannot otherwise know is
  that their skipped creatures are being used as replacements after all. That
  must be said where they are already looking, at the point the run is committed
  or while it reports progress. A run that silently contradicts the list's stated
  promise is indistinguishable from the feature being broken. The one starved
  configuration that genuinely randomizes nothing — every row ticked, so every
  placement is frozen — must likewise say that nothing was randomized rather
  than report a bare success.

**The screen must say what ticking a row does.** One line of instruction, in the
sense "select enemies that will not be randomized", on the picker itself rather
than only in the user guide (§2, §4 F13). The two shipped pickers keep their
current layout and page counts (§6).

**Reference fidelity, `CLAUDE.md` §7.** Two quirks are reproduced deliberately
rather than tidied: the six forced Yahar'gul maiden placements keep randomizing
(§2, §4 F5), and the reference's inert third maiden pattern stays inert. Neither
is improved into "all 15 maidens" or "three creatures protected".

**Existing saved configurations keep their meaning, with one deliberate
exception.** An existing defaults file has no entry for this list, so it must
read as *nothing skipped* and otherwise behave exactly as it does today. The
exception is row 16's line, which retires unmigrated (§10, D1): a file that had
the setting on comes back with bell maidens randomized. Every other setting in
that file must survive the layout change untouched — a positional format means
removing a line is not a free operation.

**The per-zone stat pass is left alone** (§4, F6, §6). §2 and §8 are written to
what it actually does.

**No emulator.** Nothing in this loop can run the game; §8 splits accordingly.

---

## 8. How will we know it works?

### Automated

What must be true of an output tree produced with a given skip list:

1. **Every placement of a skipped creature keeps that creature.** Its model and
   its AI match vanilla exactly. Its stat block matches vanilla *or* is the
   zone-tuned variant of the vanilla value — both are correct, and asserting
   plain equality is wrong (§4, F6). The one permitted exception is the six
   forced Yahar'gul maiden placements when the chime maiden rows are ticked.
2. **No placement anywhere becomes a skipped creature.** Checkable from the
   output tree alone, and this is the pool half of the feature.
3. **Placements of creatures that were not skipped are unaffected** — they are
   randomized at the same rate as a run with nothing skipped.
4. **Nothing skipped is byte-identical to a run of the same seed made before the
   feature existed.** The no-op default must not perturb the sequence of rolls.
5. **Ticking the two chime maiden rows reproduces what `UNCHANGED BELL MAIDENS`
   did**, for the same seed — including the six forced Yahar'gul placements
   changing and the other 42 maidens not (§4, F5). This is the concrete meaning
   of "supersedes row 16", and because that setting is being removed (§10, D1)
   the comparison is against a tree from a build that still has it, captured
   before the change rather than produced after it.
6. **The baked 85-row list equals the set of creatures that actually have
   overwritable placements in the vanilla tree**, in its recorded order. This is
   the silent failure that nothing else in the system would notice, and it is the
   same class of check the existing 82-row list already has.
7. **A starved selection produces a complete, finished output tree, randomized
   from the included list** (§10, D4). It holds the same set of files a normal
   run writes and every other enabled randomizer has been applied to it. Every
   placement of a skipped creature still keeps that creature — assertion 1 holds
   unchanged here — while every *other* eligible placement is drawn from the
   **ENEMIES INCLUDED** selection. Assertion 2 is the one that does **not** hold
   in this configuration, and a check that asserts it unconditionally is wrong.
   No run stops after the output folder has been touched, for any selection
   reachable from the two lists.
8. **An unusable vanilla source still fails.** A source tree that yields no
   readable maps must be reported as a failure, and must not be reported the
   same way as a starved selection (§4, F10; §7).

Note that assertions 1–5 and 7 pin the rules, not the C++ that implements them —
the project's standing limitation.

### Hardware

Three trees are needed, not two: vanilla, one run with nothing skipped, one run
with the same seed and a chosen list. Two randomized trees alone cannot tell
"frozen" from "redrawn as the same creature".

1. **Pick a creature with many placements and tick it.** Are they all still that
   creature, in the places you remember them? Is it absent from everywhere else
   it used to turn up in the unskipped run? This is both halves of the feature
   in one observation.
2. **Tick `C1130 OEDON CHAPEL DWELLER`** (§4, F2; §10, D6). Is Oedon Chapel as the
   game shipped it, and do rescued survivors still gather there? This is the
   clearest case the feature exists for, and it is one of the three creatures the
   existing list cannot reach.
3. **Does a skipped creature still work?** A frozen chime maiden must still ring
   her bell and summon; a frozen non-combat NPC must still be talked to. This is
   §4's assumption and only the console answers it.
4. **Tick the two chime maiden rows and confirm Yahar'gul.** Six of its fifteen
   maidens are *expected* to have changed. "Some Yahar'gul maidens were replaced"
   is the correct result, not a failure.
5. **Navigation.** 85 rows through a twelve-row window — eleven if the
   instruction line is given room of its own (§4, F13), and 8 pages either way:
   the cursor
   stays visible, paging lands where expected at both ends, select-all and
   select-none do what the footer says, and the summary row's count matches what
   was ticked.
6. **Leave it empty and confirm nothing changed** — the default must be
   indistinguishable from today.
7. **Read the screen.** Does the instruction line say what ticking a row does,
   is it legible and clear of the heading, the count line and the list, and do
   the ticked column and the select-all and select-none prompts agree with it
   rather than reading as an inclusion list? (§4, F13)
8. **Starve it deliberately and commit.** Tick here the only creatures ticked in
   **ENEMIES INCLUDED** — the two chime maidens against the two chime maidens is
   the case from the 2026-09-18 feedback. The run must finish rather than error,
   the output folder must be complete, and the game must load and play. What you
   should find is a world made of chime maidens with the original maidens still
   at their bells (§10, D4). Tick *every* row as well and confirm that run also
   finishes — with every placement in the game frozen, it is the one starved
   configuration that has nothing to draw *and* nothing to draw for, and it must
   not fail or hang.

**Failure would look like:** a skipped creature still being replaced (the
selection did not reach the placement decision); a skipped creature still
appearing somewhere new in any configuration other than the starved one of
§10 D4 (it did not reach the pool); a saved selection from
either existing picker meaning something different after the change (the row-order
hazard in §7); a run that fails part-way and leaves a half-written output folder;
a creature frozen in place but no longer doing what it does, which would mean
something beyond the three written fields matters and would invalidate more than
this feature; or a run that reports success having randomized nothing when the
cause was a bad vanilla source rather than the selection (§4, F10).

---

## 10. Decisions

| Date | Decision |
| --- | --- |
| 2026-09-17 | **D1 — `UNCHANGED BELL MAIDENS` retires when this feature ships, and its saved configuration is not migrated.** Row 16's wizard row is removed; reference parity survives as a configuration of this list (tick the two chime maiden rows) rather than as its own checkbox. There is deliberately **no carry-over** of an existing saved value: a defaults file that has the old setting on loses it, and the player re-expresses it here. Developer's call, made with the silent behaviour change understood and accepted. |
| 2026-09-17 | **D2 — the setting is called `ENEMIES SKIPPED`.** Not `ENEMIES BYPASSED` (the backlog's word) and not `ENEMIES UNCHANGED` (the spec's recommendation). This is the screen heading, the summary row on both screens, the configuration key, and the wording used in the user guide. This document uses *skipped* as its term for the concept throughout; the backlog row, this feature's title and its folder name keep the word *bypassed* because that is what the backlog row is called. |
| 2026-09-17 | **D3 — the unnamed creature's row keeps the generator's existing fallback.** `c2561` has no community name, so its row reads `C2561 C2561`. No label is invented for it and none is inferred from its position next to Winter Lanterns. The row is honest about what is known and is expected to be little used. |
| 2026-09-18 | **D4 — a starved selection must not error, and in that case `ENEMIES INCLUDED` wins the draw while the skipped placements stay frozen.** When every creature ticked in `ENEMIES INCLUDED` is also ticked in `ENEMIES SKIPPED`, the run proceeds: the pool for that run is the `ENEMIES INCLUDED` selection, the skip list's *placement* half still holds in full, and the skip list's *pool* half yields. The result is a fully randomized world drawn from creatures whose own placements are all frozen — in the chime maiden example, every other enemy becomes a chime maiden while the original maidens stay at their bells. Chosen over matching the reference's "randomize nothing" (the spec's recommendation) because the developer wants a usable run out of that configuration rather than a no-op. **The consequence is accepted and must be documented, not hidden: in this one configuration the feature's promise that a skipped creature never appears anywhere new does not hold.** It is the only case in which it does not. |
| 2026-09-18 | **D6 — `c1130`'s row reads `OEDON CHAPEL DWELLER`, overriding the community name data.** `Characters.json` calls it "Labyrinth Ritekeeper"; `docs/enemy-exclusion-history.md:118-122` identifies the retail-loaded placement `c1130_0000` as the Oedon Chapel dweller — `ThinkParamID` 0, no combat AI, internal name デーモンの狂信者 / `DemonsFanatic`. The generated table therefore carries a small project-owned name override, marked as ours. **This does not reopen D3:** `c2561` still renders as `C2561 C2561`, because there the evidence was a position match rather than an identification. Taken 2026-09-18, after the spec was approved. |
| 2026-09-18 | **D5 — the existing commit-time refusals for an empty `ENEMIES INCLUDED` and an empty `BOSSES INCLUDED` stay exactly as they are.** They fire at the confirm screen before anything is written, they name the fix, and the boss one guards a path with no safe no-op. The resulting asymmetry — nothing ticked on the included list refuses the run, everything ticked on the skipped list does not — is accepted. Rows 9 and 10 are not changed by this feature. |

---

## Amendments after approval

Changes made after the developer approved this spec on 2026-09-18. A change that
**alters** an approved decision drops the status back to
`QUESTIONS ANSWERED — awaiting developer approval`; a change that adds or
corrects without altering one keeps `APPROVED`. Each entry says which it was.

| Date | Change | Kind |
| --- | --- | --- |
| 2026-09-18 | **D6 added** — `c1130`'s row is named `OEDON CHAPEL DWELLER`. Raised by stage C: §8 hardware step 2 told the tester to tick "the Cathedral Ward chapel dweller", which was not a string that would appear on screen. §8 step 2 now names the row as it will read. | **Addition.** D3 settled only `c2561` and is untouched, so no approved decision changed and the status stands. |

---

## Appendix — research notes for stage C

*Not part of the specification, and not covered by the developer's approval.
These are pointers from the spec investigation to save stage C a search. Verify
anything here before relying on it.*

**Where the behaviour lives**

* `app/src/Randomizer/EnemyExclusionList.h` — `IsExcludedEnemyName`, the single
  substring test both halves of the feature go through; also the row-16 and
  forced-Yahar'gul lists it already carries.
* `app/src/Randomizer/EnemyRandomizer.cpp` — the pool-contribution filter and the
  placement decision are the only two call sites of that test; the order of the
  exclusion check against the per-zone roll is what makes the seed stream shift.
  `StepBuildPool`'s guard and the comment above it are where §4 F10 and §7 land,
  and the comment already names both causes and where the decision lives; the
  phase order above it is what decides what a failure leaves on disk.
* `app/src/Randomizer/ModelPoolSelection.h` — the selection type rows 9 and 10
  share. Read its constructor, its unknown-model fallback and its `Decode` length
  guard against §7's inverted-default constraint before designing anything.
* `app/src/Randomizer/BossParamScaling.cpp` and `BossParamScaling.h` — the
  per-zone stat pass of §4 F6, and its list of fifteen scaled maps. It runs
  outside the enemy-randomization branch, which is why it reaches frozen
  placements.
* `app/src/UI/ModelPicker.h/.cpp` — the shared picker component; note its
  confirm-prompt wording and its YES/NO column are written for an inclusion
  list, and that `Draw` takes one heading string with no slot for a second line
  of text. `kPickerLayout` and the two `DrawCenteredLabel` calls above it are
  the constants §4 F13 measures against.
* `app/tools/ui_scroll_verify.py` — the geometry mirror; its `SCREENS` table
  carries the picker's band and the heading-bottom value a third line changes.
  Run it before drawing anything.
* `app/src/UI/EnableWizardScreen.cpp` — the commit-time refusals, and what §4 F10
  says they cannot see.
* `app/tools/gen_pool_table.py` and `app/tools/enemy_lookup.py` — the generator
  and the shared pool replay. `enemy_lookup`'s `exclusion_reason` is the mirror of
  the engine's test; anything new should read the headers the same way rather
  than restating the rules.
* `reference/Randomizer/MainWindowComponents/StartFunctions.cs` —
  `excludeEnemiesBool` and the three maiden appends, for the §3 claims.

**Worth checking early**

* `pool_verify.py`'s `maidens` command encodes an expectation that §4 F6 shows is
  wrong. Establish the correct notion of "unchanged" before writing any new
  assertion against it, or the new check inherits the error.
* `app/tools/names.py` / `data/Characters.json` — one of the 85 has no entry, and
  one more carries a name that the exclusion history says is misleading for the
  placement it actually sits on. Both affect what the generated list says.
* `docs/features/016-unchanged-bell-maidens/` — `plan.md` §5.2/§9.3 designed a
  non-failing empty-pool behaviour (the reference's, not §10 D4's, but the
  phase-order and guard work carries over), `plan-review.md` finding
  2.1 is why it was not shipped, and `log.md` (2026-09-16) records the deferral
  and the three options put to the developer. Read all three before designing
  anything for §7; most of the thinking is already there.
* `EnemyRandomizer.cpp`'s `Fail` does no cleanup and the mirror phase runs
  first, so what a failed run leaves behind is worth confirming for yourself
  before deciding what replaces the failure.
* `app/tools/enemy_lookup.py`'s `randomizes()` already mirrors the placement
  decision line for line, and `pool_verify.py selftest` drives it; a starved-run
  assertion should extend those rather than start a third mirror.
* The port's `c1110_0000` pool rule is inverted with respect to the reference's
  (reference adds when the talk value differs, the port skips). It changes pool
  *weights* only — the set of 82 drawable creatures and the 85 rows here are both
  unaffected — so it is not this feature's problem, but do not "fix" it here and
  do not be surprised by it when replaying the pool.

**Dead ends already walked**

* Matching by model rather than by name substring. It looked like the safer
  design; §4 F3 measured both and they are identical on this data, so it buys
  nothing and departs from the reference's own mechanism.
* Deriving the 85 rows from the shipped 82-row table plus three extras. The three
  are not an addendum — they fail different pool rules for different reasons, and
  the two lists also carry different per-row numbers.
* Looking for scripting that distinguishes the six forced Yahar'gul maidens.
  Row 16's spec already established that all fifteen appear in the map's event
  script, so that is not the explanation, and the feature does not need one.
* Hunting for cage-aware or placement-compatibility handling in either codebase.
  `docs/deferred-ideas.md` §1 already records that neither has any.
