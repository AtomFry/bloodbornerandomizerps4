# Feature 016 — Unchanged Bell Maidens

**Status: APPROVED.** *(developer, 2026-09-15 — ready for `/plan 16`.)*


**Backlog row:** `docs/randomization-feature-spec.md` §5, row **16**
**Reference flag:** `bellMaidenBool` (`reference/Randomizer/MainWindowComponents/BooleanHandler.cs:49`)
**Plan:** `docs/features/016-unchanged-bell-maidens/plan.md` *(once stage 1 has run)*

The backlog calls this **Trivial — three strings appended to the exclusion
list**. That is right about the mechanism and wrong about the decision content:
the reference deliberately randomizes six Yahar'gul bell maidens *anyway* when
the flag is on, and the port currently skips that override for a stated reason
that is incorrect. §3 and §10.1 are the whole spec; everything else is short.

---

## 1. What this does, in plain terms

Bell maidens (the game's own files call them **chime maidens**, `c1050` /
`c1051`, and the port's enemy picker shows them as `CHIME MAIDEN` and
`CHIME MAIDEN (LIGHT)`) are the kneeling women who ring a bell to summon and
resurrect things around them. With enemy randomization on they are ordinary
placements: each one can be overwritten by whatever the pool draws, and their
own identity can be drawn onto other placements elsewhere.

This setting takes them out of both halves of that. Bell maidens stay bell
maidens where the game put them, and no other placement in the game becomes a
bell maiden.

## 2. What the player experiences

| | |
|---|---|
| **Off** (default) | Bell maidens are randomized like anything else. This is today's behaviour on about **89% of seeds**; building the m28 override (§10.1) also fires with the flag off, which changes the output of the remaining ≈11% — see §10.1 for the measurement |
| **On** | **28 of the 34 live** bell maiden placements keep their vanilla identity, and bell maidens never appear as a replacement anywhere. The six exceptions are all in Yahar'gul — see below |

**The six Yahar'gul exceptions.** Per §10.1 the port reproduces the reference's
deliberate override, so six specific `c1050` placements in Yahar'gul randomize
even with this setting on. In that one area **6 of its 15 bell maidens still
change**; everywhere else in the game, every bell maiden is frozen. This is the
reference's behaviour reproduced on purpose, and the single most surprising thing
about the feature — anyone writing the user guide should say it out loud.

*Six, not twelve, from the player's side.* The same six placement names exist in
the unused `m28_00_00_00` variant as well, so **12** placements carry the
override in the data (§4) — but only the six in `m28_00_00_01`, the map the
retail game loads, are ones a player can walk up to. §8's automated assertions
count all 12; this table counts the 6.

Two further consequences worth stating up front, because neither is obvious from
the label:

- **It also shrinks the replacement pool** — measured at 333 → 317 entries and
  82 → 80 models (§4). The two `CHIME MAIDEN` rows on the `ENEMIES INCLUDED`
  picker screen become inert while this is on. That is the reference's
  behaviour, not an addition. The picker is deliberately left as it is — §10.3.
- **The seed's meaning changes.** The port's placement loop skips excluded
  placements before rolling the zone chance, so freezing 54 placements shifts
  the RNG stream. The same seed with this on is a different game everywhere, not
  a previous game with maidens pinned. True of any exclusion change, recorded so
  it is not later mistaken for a bug.

## 3. What the reference tool does

Three appends, then one override that partly undoes them.

**The appends** — `StartFunctions.cs:45-49`, at the top of `DoSomething()`:

```csharp
if (bellMaidenBool)
{
    unusedPlusBossList.Add("c1050");
    unusedPlusBossList.Add("c1051");
    unusedPlusBossList.Add("c1055");
}
```

`unusedPlusBossList` is the same list the port transcribed into
`EnemyExclusionList.h`, and it feeds both halves of the feature: pool
contribution (`MainWindow.xaml.cs:765-770`, inside `GenerateEnemyList`) and
placement overwriting (`RandomizeFunctions.cs:26-33`, inside `Randomize`). Both
are plain `Name.Contains` substring tests, matching the port's
`IsExcludedEnemyName`.

**The override** — `RandomizeFunctions.cs:288-320`. For any map whose path
contains `m28` (Yahar'gul), six named placements have `changeData` forced back
to `true`:

```
c1050_0117, c1050_0115, c1050_0119, c1050_0110, c1050_0112, c1050_0114
```

Order matters and is unambiguous in the source: the exclusion test runs at
`:26`, the Yahar'gul zone-chance roll at `:187`, and this block at `:289`, after
both. It is the last thing to touch `changeData` before the write at `:322`. So
those six are randomized **regardless of the exclusion list and regardless of
the Yahar'gul zone chance**.

**Stated plainly: in the reference, "Unchanged Bell Maidens" leaves six of
Yahar'gul's fifteen bell maidens changeable.** That is not a defect the way
spec 024's R-1 was — it is a deliberate, hand-typed list of six specific
placement names, and §4 shows the six are exactly one coherent group.

**The port's position today.** `EnemyRandomizer.cpp:20-23` records the override
as deliberately skipped:

> the M28-specific "re-include these 6 c1050_* instances" override in the
> reference `Randomize()` is skipped: it only matters when `bellMaidenBool` has
> added "c1050"/"c1051"/"c1055" to the exclusion list, and `bellMaidenBool`
> isn't a setting we expose (defaults false).

**The reason given is wrong**, and noticing that is one of this spec's two
findings. Because the block runs *after* the zone-chance roll, it also matters
with the flag off: in the reference those six always randomize even when the
Yahar'gul chance would have skipped them. The port has therefore been quietly
deviating from the reference on six placements per m28 map since the enemy
randomizer shipped. The deviation is small — measured, it surfaces on 1 roll in
101 per placement, so on ≈11% of seeds (§10.1) — and probably harmless, but the
comment claims a dependency the control flow does not have.

§10.1 resolves this by building the override, which ends the deviation in both
flag states. §10.4 keeps the *comment* correction out of row 16 as a separate
piece of work — so the wrong comment stays in the tree until then, and needs its
own backlog entry.

## 4. Evidence

Measured against `data/vanilla/dvdroot_ps4` using `app/tools/enemy_lookup.py`
(`enemies`, `engine_pool`) and `boss_verify.py`'s MSBB reader. No new parsing
was written.

**fact — 54 bell maiden placements exist in the 24 base maps**, out of 2877
enemy parts (1.9%): 31 named `c1050*`, 23 named `c1051*`. None of the 54 is
matched by any pattern already in `EnemyExclusionList.h`, so all 54 are newly
frozen by this feature.

**fact — 34 of the 54 are in maps the retail game loads.** The other 20 are in
pre-DLC or unused map variants (per `names.py`'s `map_is_unused`), 15 of them in
`m28_00_00_00` alone. The player-visible number is **34**.

**fact — 22 of the 24 base maps contain at least one.** Only `m21_00_00_00` and
`m21_01_00_00` (the Hunter's Dream pair) have none. Distribution is one or two
per map everywhere except Yahar'gul: `m28_00_00_01` holds **15 of its 147**
enemy parts.

**fact — `c1055` matches nothing.** Zero placements across **all 43** `.msb.dcx`
files in the tree, base maps and chalice alike. It is a real model
(`ModelSizeTable.h` gives it 14,334,730 bytes and `names.py` calls it *Chime
Maiden*) but it is placed nowhere. The third string is inert; keep it for
fidelity and expect it to change nothing.

**fact — the pool loses 16 of 333 entries and 2 of 82 models.** Re-ran
`engine_pool` with the three patterns appended: 333 → 317 entries, and models
`c1050` and `c1051` drop out entirely. That matches `EnemyPoolTable.h`'s baked
weights for those two rows exactly (`c1050` = 4, `c1051` = 12, sum 16), which is
a useful cross-check that the mirror and the baked table agree.

**fact — the reference's six m28 names are exactly the `NPCParamID 105810`
group.** Every bell maiden in both m28 variants, measured:

| Group | Count per map | NPCParamID | ThinkParamID | In the six? |
|---|---|---|---|---|
| `c1050_0000`–`_0009` | 7 | 105800 | 105800 / 105801 | no |
| `c1050_0110`–`_0119` | 6 | **105810** | 105810 / 105811 | **all six** |
| `c1050_0120` | 1 | 507000 | 0 | no |
| `c1051_0000` | 1 | 105280 | 105900 | no |

The six are not a scattering — they are one whole param group, complete, with
entity IDs `2800520`–`2800529`, and the list is identical in `m28_00_00_00` and
`m28_00_00_01`. **inference:** the author picked a category of maiden, not six
placements he happened to dislike.

**fact — event scripting does not distinguish them.** All 15 of Yahar'gul's
maiden entity IDs appear as little-endian int32 in
`event/m28_00_00_00.emevd.dcx`, the six included. So "these six are scripted and
the others are not" is *not* the explanation. What else separates the 105810
group is **not established**; only hardware or a full emevd decode would settle
it, and neither is needed to implement the feature.

**fact — appending cannot disturb `RemoveAt(3)`.** `unusedPlusBossList` is built
at `MainWindow.xaml.cs:201-208` (66 + 38 = 104 entries), the three appends land
at indices 104–106, and `StartFunctions.cs:580`'s `unusedPlusBossList.RemoveAt(3)`
still removes `unusedList[3]` = `"c2560"`, which the port already bakes in. The
obvious index-shift trap is not a trap here.

**assumption — freezing a maiden preserves whatever her bell does.** The
randomizer writes only `NPCParamID`, `ThinkParamID` and the model index; entity
IDs and every event hook are untouched, so leaving all three fields alone leaves
the placement byte-identical to vanilla. That is as strong as this gets without
the console. What *randomizing* one does to the bell, and whether a maiden drawn
onto some other placement rings anywhere, is behaviour — `CLAUDE.md` §5 forbids
concluding it from bytes, so §8 makes it a hardware observation instead.

## 5. Terminology

*Deliberately empty — nothing in this feature needs a definition beyond what §1
gives. The heading is kept so the numbering stays stable, since other documents
cite these sections by number.*

## 6. Scope

**In scope:** backlog row **16** only. One user-facing toggle, default off; both
halves of the reference's behaviour (placements frozen, pool shrunk); and
**building the m28 override**, which the port does not implement today (§10.1).

**Out of scope:**

- **Correcting the wrong justification comment at `EnemyRandomizer.cpp:20-23`.**
  This spec found it, but §10.4 keeps it out of row 16 as separate work. Note
  the code that comment describes is being replaced by this feature anyway, so
  the comment will need rewriting here regardless — what is out of scope is
  *fixing the record*, not touching the lines.

- **Rows 18–21, the easy-mode stone-guy pokes.** The backlog's §11 block 4
  groups them with this row, but the machinery is not actually shared: those
  four *write* three fields onto specific placements, this one *skips*
  placements. Nothing but the MSB file is common, and specifying them together
  would buy nothing.
- **Rows 9 and 10, the pickers.** Shipped. This spec only records where the
  features overlap (§2, §10.3).
- **Re-adding anything from `EnemyExclusionListExtra`.**
  `docs/enemy-exclusion-history.md` governs that, and this feature is not an
  excuse to reopen it. The three patterns here come from the reference's own
  source, under a flag, default off.
- **The caged-dog problem** in `docs/deferred-ideas.md` §1. It mentions `c1050`
  only to warn that the m28 override must not be mistaken for cage handling.
  Confirmed here: it is not.
- **Chalice dungeons** — out by standing decision. Noted only because 54 further
  `c1050` placements exist in the m29 maps and will stay untouched.

## 7. Constraints

**Font** — 8x8, uppercase A–Z, digits, space and `'` `(` `)` `-` `,`.
`UNCHANGED BELL MAIDENS` and `UNCHANGED CHIME MAIDENS` are both clean, and
**`YAHAR'GUL` is fine too** — the apostrophe glyph exists, added for the enemy
picker's creature names. Verify against `app/src/Platform/Font8x8.cpp` rather
than from memory; an earlier draft of this spec claimed the apostrophe would
render blank, which was wrong.

**Fidelity** (`CLAUDE.md` §7) — the six-placement override is exactly the kind
of reference quirk the standing preference says to reproduce deliberately and
say so, rather than tidy away. §10.1 decided to do exactly that, so the
constraint is now a commitment: the six stay changeable, and the plan says so
out loud rather than quietly improving the setting into meaning all 15.

**Defaults file compatibility** — every existing `RandomizerDefaults` field
treats an absent key as `false`, and `false` here means the setting is off. The
usual rule holds with no special handling. Note that off is *not quite* today's
behaviour: the m28 override arrives in both flag states and changes flag-off
output on ≈11% of seeds (§10.1). That is a code change, not a defaults-file
compatibility problem — an old defaults file keeps meaning exactly what it
meant.

**The picker table is positional and baked.** `EnemyPoolTable.h`'s row order is
the meaning of a saved picker config (`docs/plans/pickers.md`). Nothing in this
feature may regenerate that table conditionally, or every saved selection
silently remaps. This is the one way a "three strings" change could do real
damage.

**No emulator** — nothing in this loop can run the game.

## 8. How we will know it works

**Automated.** No new mirror needed, but only one of the two relevant tools can
hold an assertion, and the distinction matters:

- **`pool_verify.py selftest` is where every automated assertion belongs.** It
  is the assertion harness — a list of named cases, pass/fail, with a non-zero
  exit — and it already imports `enemy_lookup`'s parsing (`engine_pool`,
  `exclusion_reason`, `enemies`), so nothing needs re-parsing.
- **`enemy_lookup.py` asserts nothing.** Its subcommands are `frozen`, `diff`,
  `compare` and `pool`; they print a report and return 0 either way. It has no
  `selftest`. It is the tool for *reading* an output tree by eye — `frozen` to
  see the 54 placements listed with their reason, `diff` to see a run against
  vanilla — not for pinning a rule.

What to assert, in `pool_verify.py selftest`, given a way to model the
conditional exclusion:

- that exactly the 54 placements are excluded with the flag on, and that the
  excluded set returns to its current membership with it off;
- that the pool is 317 entries / 80 models with the flag on and **333 / 82 with
  it off**, and that `EnemyPoolTable.h`'s baked 82 rows are untouched in both
  cases.

**The m28 override can be asserted in one flag state, not both.** §10.1 makes
the behaviour definite, but only the flag-on half is checkable from an output
tree:

- **Flag on — assertable, and deterministic for every seed.** All 12 forced
  placements (the six `105810` names in each of `m28_00_00_00` and
  `m28_00_00_01`) must differ from vanilla, and the other nine maidens in each
  map must match it. This holds on every seed because `c1050` is not in the
  flag-on pool: whatever is drawn cannot be a chime maiden, so a forced
  placement cannot come back looking unchanged.
- **Flag off — not assertable. Do not assert it.** With the flag off `c1050` is
  still in the pool (4 entries of 333, §4), so a forced placement can
  legitimately be redrawn *as* a chime maiden — 1.2% per placement, and a 13.5%
  chance across the 12 that at least one is. A passing "they all differ from
  vanilla" check would therefore prove nothing, and a failing one would not be
  evidence of a defect. The flag-off change §10.1 brings is real, but it is a
  statistical property across seeds (≈11% of them, §10.1), not something a
  single output tree can show.

**State the limitation:** these pin the rules, not the C++ that implements them.

**Hardware.** Two runs, same seed, flag off then on, with vanilla as the third
input (`CLAUDE.md` §3 — two randomized trees alone cannot tell "frozen" from
"redrew the same model"):

1. In Yahar'gul with the flag **on**, are the bell maidens bell maidens? Count
   them if practical. 15 placements are in that map and **6 of them are expected
   to have changed** (§10.1) — so "some Yahar'gul maidens were replaced" is the
   correct result here, not a failure. Nine should be untouched.
2. With the flag on, does a bell maiden turn up anywhere she should not — a
   kneeling, bell-ringing woman in Central Yharnam or the Research Hall? She
   should not. That is the pool half of the feature.
3. With the flag **off**, confirm the opposite of both: maidens replaced, and
   maidens appearing elsewhere. This is what makes run 1 mean anything.
4. Does the bell still *work* — summoning or resurrecting as in vanilla — for a
   frozen maiden? This is the §4 assumption, and only the console answers it.

**What hardware cannot settle:** the override's flag-**off** effect (§10.1). On
≈89% of seeds the flag-off tree is byte-identical to today's, so a run showing
no difference is not evidence that the override was built, and one showing a
difference is not evidence that it is correct. §10.1 rightly asks for this not
to be assumed harmless — but what discharges that is the flag-on assertion
above, plus run 3 confirming the flag-off run still behaves sanely. Treat a
quiet flag-off run as uninformative rather than as confirmation.

**Failure would look like:** maidens still being replaced with the flag on (the
exclusion did not reach the placement loop); the enemy picker's saved selection
meaning something different after the change (the §7 hazard); or a maiden frozen
in place but no longer ringing, which would mean something beyond the three
written fields matters and would invalidate more than this feature.

## 10. Decisions taken

All four questions were put to the developer and answered on **2026-09-15**.
Nothing in §9 remains open, so that section has been removed.

**1. Reproduce the reference's six-placement Yahar'gul override.** *(2026-09-15)*

The six named `c1050` placements keep randomizing even with this setting on, as
in the reference. So the honest description of the feature is: **6 of Yahar'gul's
15 bell maidens still change with `UNCHANGED BELL MAIDENS` on**; everywhere else
in the game, all bell maidens are frozen. §2's table and §8's assertions are
written to that behaviour.

*Reason:* `CLAUDE.md` §7 — fidelity to the reference wins, and the reference's
quirks are reproduced deliberately and documented rather than silently fixed.
The six being exactly the complete `NPCParamID 105810` group (§4) is evidence of
intent rather than accident.

*Consequence to carry into the plan:* the port does not implement this override
at all today, so it has to be built, not just gated. Because the block sits after
the zone-chance roll, building it changes behaviour **with the flag off as well**
— those six placements per m28 map stop obeying the Yahar'gul zone chance and
always randomize, which is what the reference does.

*How large that change is, measured — it is much smaller than the sentence
above sounds.* **fact:** Yahar'gul's zone chance is **100** for both m28 maps
(`EnemyRandomizer.cpp:114-115`, matching `FieldContainer.cs`'s
`YahargulChance = 100`), and the roll is `RandInt(0, 100)`
(`EnemyRandomizer.cpp:613`, `:230`), inclusive at both ends — 101 outcomes,
skipping only on `roll == 100`. So the zone chance skips a Yahar'gul placement
on exactly **1 roll in 101**, and the override only changes anything on a
placement the roll would have skipped. The reference has the same off-by-one:
`universalRand.Next(0, 101)` (`RandomizeFunctions.cs:190`) is also 0–100
inclusive against a chance of 100, so 1-in-101 is the reference's own quirk,
already faithfully ported — nothing here is asking for it to be fixed. Across the 12 forced placements (six in
each of `m28_00_00_00` and `m28_00_00_01`), P(at least one) =
1 − (100/101)¹² = **11.3%**. **On ≈89% of seeds the flag-off output is
byte-identical to today's.** **inference:** on the other ≈11% the divergence is
not confined to the placement that fired — the whole run draws from one
`std::mt19937` seeded once (`EnemyRandomizer.cpp:218`), so one extra draw shifts
every draw after it.

That is still a behaviour change to shipped code arriving as part of this
feature, and it should be called out in the plan's deviations section and
hardware-tested rather than assumed harmless. Note what the odds do to that
test, though: a single flag-off hardware run is ~89% likely to show no
difference at all, so seeing none proves nothing (§8).

**2. The setting is labelled `UNCHANGED BELL MAIDENS`.** *(2026-09-15)*

Matching the reference checkbox and the backlog row rather than the game's
internal *chime maiden* vocabulary. The `ENEMIES INCLUDED` picker keeps saying
`CHIME MAIDEN` / `CHIME MAIDEN (LIGHT)`, because that screen is generated from
`Characters.json`. The two screens therefore use different words for the same
creature **on purpose**; `docs/user-guide.md` should say so in one sentence at
documentation stage.

**3. The `ENEMIES INCLUDED` picker is left exactly as it is.** *(2026-09-15)*

Its two `CHIME MAIDEN` rows become inert while this setting is on, and that is
accepted. Row order is the meaning of every saved config (§7), so the table is
not regenerated, no row is hidden or greyed, and no new scroll state or
`ui_scroll_verify` case is introduced.

**4. The wrong comment at `EnemyRandomizer.cpp:20-23` is corrected separately,
not in this change.** *(2026-09-15)*

Row 16 stays strictly scoped to the feature, per the standing rule against
refactoring ahead of the ask.

> **This leaves a known-wrong comment in shipped code, and it needs its own
> backlog entry so it is not forgotten.** The comment claims the m28 override
> "only matters when `bellMaidenBool` has added ... to the exclusion list";
> verified against `RandomizeFunctions.cs`, the override runs at `:289`, after
> the exclusion test at `:26` and after the Yahar'gul zone-chance roll that sets
> `changeData = false` at `:193`, and is the last writer before the write gate at
> `:322`. It therefore applies with the flag off too. Decision 1 means the code
> the comment describes is being replaced anyway, so whoever plans this feature
> should expect the comment to need rewriting regardless — the decision here is
> only that *fixing the record* is not row 16's job.

## 11. Amendments after approval

What changed in this document after the developer approved it, and why the
approval survived it. An amendment that touched a §10 decision would not — it
would go back to the developer instead.

**2026-09-15 — three Corrections from stage 1, applied by `/refine-spec 16`.**

Raised by the `planner` subagent during the first `/plan 16` run and recorded in
`docs/features/016-unchanged-bell-maidens/log.md`; each was verified independently
before being applied here.

1. **§8 asserted something unprovable.** It said the six forced m28 placements
   must differ from vanilla *with the flag off as well*, and called that
   checkable from an output tree. It is not: with the flag off `c1050` is still
   in the pool, so a forced placement can be redrawn as a chime maiden by
   chance. §8 now asserts the flag-**on** case only — which is deterministic,
   because `c1050` is not in the flag-on pool — and says plainly why the
   flag-off case cannot be asserted, with the measured odds.
2. **§8 pointed the automated checks at the wrong tool.** It called
   `enemy_lookup.py` one of "two existing mirrors" that could carry the
   assertions. It cannot: it has no `selftest` and no assertions at all, and its
   four subcommands (`frozen`, `diff`, `compare`, `pool`) print a report and
   return 0 either way. §8 now places every automated assertion in
   `pool_verify.py selftest`, which has the harness and already imports
   `enemy_lookup`'s parsing, and describes `enemy_lookup.py` as the inspection
   tool it is. *(The log entry recording this says §8 "names
   `enemy_lookup.py`'s `selftest`"; the word `selftest` never appeared in §8 —
   the substance of the finding is right, the quotation was not.)*
3. **§10.1 read as a far larger change than it is.** It said the six placements
   "stop obeying the Yahar'gul zone chance" without giving the chance. Measured:
   the chance is 100, `RandInt` is inclusive at both ends, so the skip fires on
   1 roll in 101 and the override changes flag-off output on ≈11% of seeds.
   §10.1 now carries those figures.

Reconciled with them in the same pass, so the document does not argue with
itself: **§2**'s "Off = exactly today's behaviour" row (true on ≈89% of seeds,
not all), **§2**'s new note that 12 placements carry the override in the data
though only 6 are player-visible, **§3**'s "the deviation is small" claim (now
quantified), **§7**'s defaults-compatibility line (off is not quite today's
behaviour, but an old defaults file still means what it meant), and **§8**'s
hardware list (what a quiet flag-off run does and does not prove). This section
itself is new; `docs/features/_templates/spec.md` has no §11, so the template
needs one if amendment records are to be standard.

**All three are Corrections: none changes what the feature does.** No decision
in §10 was altered — decisions 1–4 stand exactly as the developer answered them
on 2026-09-15, including decision 1's commitment to build the m28 override.
Correction 3 makes the *size* of decision 1's consequence accurate; it does not
touch the decision. **The developer's approval therefore stands** and the status
line is unchanged.

---

<!--
Not in this document, on purpose:

  - implementation approach, file-by-file changes, code
  - the verification *mechanism* (§8 says what correct means, not how to check)

Those belong to stage 1.
-->
