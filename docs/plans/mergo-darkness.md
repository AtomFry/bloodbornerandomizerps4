# Feature 8 — Enable Mergo Darkness

**Spec reference:** `docs/randomization-feature-spec.md` §2, row 8 (`permaDarknessBool`).
**Status: implemented and hardware-tested. Renamed and inverted after testing —
see §2.7, which is the most important section in this document.**

**Row label: `ENABLE MERGO DARKNESS`. YES makes the world permanently dark. NO
is the normal game. Default NO.**

> ## ⚠ This plan's original analysis was wrong
>
> The first version of this document concluded that the vanilla bytes meant
> vanilla behaviour, so it named the setting `DISABLE MERGO DARKNESS` and made
> "leave the file alone" the default. **On console that default produces a
> permanently dark world from the first spawn.** The setting has been renamed
> and its polarity inverted to match what the game actually does.
>
> §1 and §2.7 describe the tested behaviour. §2.1–§2.6 are kept because the
> byte-level decoding in them is still correct and still useful — but read them
> as *"which bytes are which"*, never as *"what the game does with them"*.
> That inference is exactly what turned out to be false.

---

## 1. What this setting does, in plain terms

**Mergo's Wet Nurse**, the boss at the top of Mergo's Loft, fights you in
darkness. This setting turns that darkness on for the **whole game**.

| `ENABLE MERGO DARKNESS` | What the app writes | What you get |
|---|---|---|
| **NO** (default) | four bytes into `event/common.emevd.dcx` | The normal game, normally lit |
| **YES** | nothing — the mirrored vanilla file is left alone | The world is dark from the moment you spawn, and stays that way |

With it on, loading a brand-new game looks like standing in the middle of the
Wet Nurse fight after she darkens the arena — and it does not wear off.

It is a novelty, not a difficulty option in any measured sense: nothing about
enemy placement, damage or drops changes. It just makes the game much harder to
see.

### What it does not do

- It does not affect the randomizers. The same seed produces the same world
  either way, and it draws no randomness.
- It cannot soft-lock a run.
- It is unrelated to the Blood Moon and to Insight-driven world changes.

### Note the polarity, and do not "correct" it

The app writes the file either way; the setting only chooses **which** of two
byte patterns goes into it. The pattern that produces darkness is the one the
game itself ships with, and the four-byte poke is what produces the normal game.
That is backwards from intuition and it is what the console actually does — §2.7
has the evidence and the unresolved question of why.

---

## 2. What the real data says

### 2.1 The event chain, end to end

Four pieces, all read out of the vanilla tree:

**`event/common.emevd.dcx`, event `6548972`** — initialised from event `0` (the
common constructor) and from event `50` (the save / NG+ init event), so it is
running from the moment you load in:

```
[0] 2004[08]  (10000, 5630, 1)    SetSpEffect: character 10000 (the player) gets SpEffect 5630
[1] 1000[04]  (1)                 end with restart  -> re-runs, so the effect never lapses
```

This single instruction is what the feature edits.

The terminator argument is meaningful, not boilerplate: across `common.emevd.dcx`
the 48 `1000[04]` terminators split exactly 24 `0` / 24 `1`, consistent with the
usual `0 = End` / `1 = Restart` meaning. *(Inference — the split is measured, the
naming is from the wider EMEVD convention.)*

**`event/m26_00_00_00.emevd.dcx` (Nightmare of Mensis), event `12604810`** — hands
the Wet Nurse her own state while the player is marked:

```
[0] 4[05]     (0, 10000, 5630, 1) wait until the player HAS SpEffect 5630
[1] 2004[08]  (2600800, 5631, 0)  SetSpEffect: entity 2600800 gets SpEffect 5631
[2] 4[05]     (0, 10000, 5630, 0) wait until the player does NOT have it
[3] 1000[04]  (1)                 end with restart
```

Event `12604840` does the same for entity `2600801` as part of a longer sequence.

**`m26_00_00_00.emevd.dcx`, event `12604807`** — the area lighting, which is the
part a player actually sees:

```
[1] 3[00]     (257, 12604802)     group 1: event flag 12604802 is set
[2] 4[05]     (1, 10000, 5630, 0) ...AND the player does NOT have 5630
[4] 2013[01]  (122)               -> lighting preset 122
[5] 3[00]     (258, 12604802)     group 2: same flag
[6] 4[05]     (2, 10000, 5630, 1) ...AND the player DOES have 5630
[8] 2013[01]  (156)               -> lighting preset 156
[9] 1000[04]  (1)                 end with restart
```

**`m26_00_00_00.emevd.dcx`, event `12601800`** — the Wet Nurse's own boss script. It
waits on entity `2600802` reaching a state, then on `2600800` dying, sets the
boss-defeated flag, stops the music, and then:

```
[22] 2004[21] (10000, 5630)       CLEAR SpEffect 5630 from the player
```

**That instruction is why the darkness is not permanent in the normal game.** It is
removed the moment her boss script reaches line 22, and everything after that runs
lit.

**The entities, resolved against `m26_00_00_00.msb.dcx`:**

| Entity | Placement | Model | Name |
|---|---|---|---|
| 2600800 | `c5510_0000` | `c5510` | Mergo's Wet Nurse |
| 2600801 | `c5510_0001` | `c5510` | Mergo's Wet Nurse |
| 2600802 | `c5510_0002` | `c5510` | Mergo's Wet Nurse |

These nine instructions, across five events, are **every** reference to either
SpEffect anywhere in the event tree — confirmed by decoding all 30 `.emevd.dcx`
files and matching 5630 / 5631 against every integer argument of every instruction,
not just `SetSpEffect` calls. Eight are in `m26`; the ninth is the global applier.
Nothing outside Nightmare of Mensis reads this marker.

**On `2013[01]` being a lighting preset** *(inference, well supported)*: it takes a
single small even integer, appears 67 times, never in `common.emevd.dcx`, and each
map file uses its own handful of values (`m26` uses 40, 88, 122, 156). That is the
signature of a per-map environment/lighting slot selector, and the behaviour it
produces here — one value while the boss lives, another once the marker is gone —
matches what Mergo's Loft visibly does.

### 2.2 Both SpEffects are pure markers

Dumped from `SpEffectParam.param` (7856 rows, 528-byte stride). 5630 and 5631 differ
from each other in exactly two fields:

| Field | 5630 (player) | 5631 (Wet Nurse) |
|---|---|---|
| `effectEndurance` | 50.0 | 50.0 |
| `stateInfo` | 255 | 210 |

Every damage, defence, attack, weight, recovery and resistance rate is `1.0`; every
point change is `0`. Neither effect does anything mechanically on its own — they are
tags, and the visible consequence comes from the scripts that read them (§2.1).

Note the 50-second `effectEndurance` on both: neither would persist unaided. The
global event restarts forever to keep the player's marker topped up, which is why it
survives until something explicitly clears it.

### 2.3 What a randomized run does to the clearing step

This is what turns a temporary darkness into a permanent one, and it is the least
certain link in the chain — a hypothesis with its evidence, not a finding.

The Wet Nurse occupies three placements in `m26`, and her boss script depends on
more than one of them: it waits on `2600802` reaching a state, *then* on `2600800`
dying, before reaching the line that clears the marker. Of those three, the boss
randomizer's eligibility list contains only `c5510_0000` (entity `2600800`) —
`c5510_0001` and `c5510_0002` are not boss placements and are left alone.

So a randomized Mergo's Loft holds a replacement boss at `2600800` alongside two
genuine Wet Nurses. Whether the scripted sequence still runs to completion in that
arrangement is exactly the question, and it is not answerable by reading event bytes:
it depends on how the replacement's death interacts with a script written around a
three-body fight. If it stalls anywhere before instruction 22, the marker is never
cleared and the area never brightens.

**This is the one claim in the document that wants a hardware test rather than more
decoding** — see §5.3 Check 2.

### 2.4 The byte-level change, and why the reference has two branches

The vanilla instruction's argument bytes are:

```
10 27 00 00 | fe 15 00 00 | 01 00 00 00      -> (10000, 5630, 1)
              ^^ ^^ ^^      ^^
           index 4  5  6   index 8
```

Line that up with the two branches of `PermaDarknessFunction`
(`reference/Randomizer/MainWindowComponents/MainWindow.xaml.cs:1453`), which our
`ApplyPermaDarkness()` ports byte for byte:

| Branch | writes to [4][5][6][8] | resulting args | effect |
|---|---|---|---|
| `permaDarknessBool == true` | 254, 21, 0, 1 | `(10000, 5630, 1)` | **identical to vanilla — a no-op** |
| `permaDarknessBool == false` | 159, 134, 1, 0 | `(10000, 99999, 0)` | the player is given SpEffect **99999** |

**SpEffect 99999 does not exist** — confirmed absent from `SpEffectParam.param`. So
the `false` branch is "apply a nonexistent effect", i.e. apply nothing. The player
never gets marked, Mergo's Loft never enters its dark state, and the Wet Nurse never
gets 5631.

**Why a no-op branch exists at all:** the reference tool edits the game's files in
place (`tempEMEVD.Write(currentEmevd)`, same path it read from) and keeps no pristine
copy. Once it has cut the wire, the only way back is to write the original values in
again — that is the `true` branch. It is a *restore*, not a feature.

**Our port does not need it.** Every run reads from a clean vanilla source and writes
into a separate AFR output tree, so not writing at all *is* the restore. That is what
makes D6 possible and what removes the polarity confusion entirely: our setting has
one action ("cut the wire") and one idle state ("write nothing").

| Our setting | Reference equivalent | What we do |
|---|---|---|
| `DISABLE MERGO DARKNESS = YES` | box **unchecked** | call `ApplyPermaDarkness(..., false)`, write the file |
| `DISABLE MERGO DARKNESS = NO` | box **checked** | skip the step; the mirrored vanilla file stands |

### 2.5 What our port does today

`EnemyRandomizer.cpp:671` calls `ApplyPermaDarkness(emevdPlain, /*permaDarknessOn=*/false)`
with the flag hardcoded, on every run, and writes the result over the mirrored
`event/common.emevd.dcx`.

**Every randomized tree this app has ever produced has the Wet Nurse's darkness cut.**
It was ported as a faithful copy of the reference tool's default behaviour —
`PermaDarkness.h`'s header comment says as much ("Runs unconditionally on every
reference-tool run, regardless of any checkbox") — and that is accurate as far as it
goes. What the comment does not say, because it was written without decoding the
event, is that the branch being hardcoded is a modification rather than a
pass-through.

One inconsistency this feature also fixes: the poke only happens if a run happens,
and a run only happens if at least one randomizer toggle is on. So today, committing
with everything off leaves vanilla darkness intact, while committing with anything on
removes it — the same settings producing different darkness depending on unrelated
toggles. Under D4 the setting decides it in both cases.

### 2.6 This also closes an open question in the technical review

`docs/windows-randomizer-technical-review.md` §13, open question 9, asks whether
`permaDarknessBool` actually gates the unconditional `PermaDarknessFunction` call at
`StartFunctions.cs:1352`. **It does** — the flag is read inside the function body, and
both branches write, which is why the call site needs no `if`. Worth marking resolved.

---

### 2.7 The hardware test, and what it falsified

**Tested on console, both directions, by the user.**

| Setting (old names) | File state | Observed |
|---|---|---|
| `DISABLE MERGO DARKNESS = NO` | vanilla bytes, untouched | **World dark from the first spawn, and stays dark** |
| `DISABLE MERGO DARKNESS = YES` | poked to SpEffect 99999 | **Normal, lit game** |

Described as "spawning into a new game is as if we're in the middle of the
Mergo's Wet Nurse fight when the world goes dark, and it stays that way the
entire game."

**Three claims in this document are therefore false**, and they are all
inferences from §2.1–§2.6 rather than measurements:

1. *"The vanilla bytes mean vanilla behaviour, so the `true` branch is a no-op
   restore."* It is byte-true and behaviourally false. This was the load-bearing
   error — D1, D3 and D6 were all built on it.
2. *"The darkness is confined to Mergo's Loft, because nothing outside `m26`
   reads the marker."* The scan behind that was thorough and is still correct
   about the **event files** — but the effect is plainly global, so something
   outside EMEVD reads it. `stateInfo 255` on a marker SpEffect is read by the
   behaviour/engine layer, which no amount of event decoding would have shown.
3. *"The reference tool's checkbox is named backwards."* It is not. "Nurse
   Perma-Darkness ✓" gives you perma-darkness, exactly as it says. Our rename to
   `DISABLE …` was the thing that inverted it, and it has been undone.

**What is still solid:** everything in §2.1–§2.6 about *which bytes are which* —
the event, the offsets, the two states, SpEffect 99999 not existing, the m26
chain. Those were measured. It is only the step from "which bytes" to "what the
game does" that failed.

#### Why the shipped bytes behave differently under AFR — unresolved

The uncomfortable part: an unmodified console runs these exact bytes and is not
dark. Ours mirrors them verbatim and is. Hypotheses, none tested:

- **The installed game's file differs from our `VanillaSource` dump.** If the
  retail/patched `common.emevd.dcx` is not the one we mirror, then overlaying
  ours *replaces* a patched file with an older one, and the difference has
  nothing to do with our poke. **This is the most plausible explanation and the
  cheapest to check** — compare our `VanillaSource/event/common.emevd.dcx`
  against the file the installed game actually ships.
- **The marker is inert in vanilla because something else clears it**, and that
  something is disturbed by the randomized tree.
- **`2004[08]`'s third argument is not what the decode assumes.** Vanilla passes
  `1` here and every other call in the tree passes `0`, which was noted in §2.1
  and never explained.

Until one of those is confirmed, the setting is correct by measurement and
unexplained by theory. That is an acceptable place to ship from, and a bad place
to refactor from — hence the warnings in `PermaDarkness.h` and `StepEmevd`.

---

## 3. Design decisions

> **Superseded by §2.7 where they conflict.** D1, D3 and D6 below were all
> reasoned from "vanilla bytes = vanilla behaviour", which the hardware test
> disproved. They are kept because the *other* reasons in them still hold (font
> glyphs, row placement, the absent-key rule), and because the record of how a
> plausible chain of inference produced a wrong default is worth more than a
> tidy document. **What actually shipped is this:**
>
> | | |
> |---|---|
> | **Label** | `ENABLE MERGO DARKNESS` |
> | **YES** | write `(10000, 5630, 1)` → world is dark |
> | **NO** (default) | write `(10000, 99999, 0)` → normal lit game |
> | **Config key** | `enable_mergo_darkness` |
> | **Run gate** | YES starts a run on its own; either state is written whenever a run happens for any reason |
>
> **D6 is superseded as well.** It argued for skipping the write in one state so
> the mirrored vanilla file would stand bit-exactly. Both states are written now
> — see §3.1, which replaced it.
>
> The config key was renamed rather than reused, so an existing `defaults.cfg`
> carrying `disable_mergo_darkness` is ignored and falls back to the struct
> default — which is now the lit game. That is the safe direction, and it means
> anyone who saved settings under the broken build is repaired by upgrading
> rather than left dark.

All settled.

**D1 — Name: `DISABLE MERGO DARKNESS`.** Named for the action, so YES means "do
it" and NO means "leave the game alone" — the thing the reference's name inverted.

- **"Mergo" is how players refer to this fight.** The boss is formally Mergo's Wet
  Nurse and Mergo is the infant she attends, so the shorthand is not literally
  accurate — but it is what everyone says, and a row label should use the name the
  audience already has. Explicitly chosen over the literal name.
- Punctuation has to go regardless: the 8x8 font is letters, digits and space only
  (`Font8x8.cpp` has no punctuation glyphs, and anything else renders as a blank
  gap), so a possessive apostrophe was never available.
- At 22 characters it is comfortably shorter than the longest existing row
  (`RANDOMIZE STARTING WEAPONS`, 26). Rendered width with its value is 1008px of
  1920, centred.

*Considered and dropped:* `DISABLE WET NURSE DARKNESS` (26 chars) and
`DISABLE MERGOS WET NURSE DARKNESS` (33 chars, 1404px — fits, but bare `MERGOS`
reads badly without its apostrophe).

**D2 — Placement: last row, after `RANDOMIZE SHOP WEAPONS`.** Every other row is a
`RANDOMIZE …` row and they are grouped; this one is not a randomizer at all. Putting
it at the end also means **no row renumbering** in either screen — the single riskiest
part of the workshop-tools change (its §5.2 regression guard) does not arise here.

**D3 — Default is NO, meaning "leave the game alone".**
- An absent key in an older `defaults.cfg` reads as `false`, which is the
  do-nothing state. The struct's usual rule holds.
- The app should not silently delete a vanilla effect nobody asked it to delete.

⚠ **This changes existing behaviour.** Today every run cuts the darkness; after this,
runs at the default will not. Mergo's Loft will be dark again, the way the normal game
plays it. That is intended, and §5.2 accounts for it so it is not mistaken for a
regression.

**D4 — It DOES gate a run on its own.** Add `disableMergoDarkness_` to the big `||`
in `StartCommit`. Turning it on with everything else off must produce a tree — the
commit's job is to build something that matches the settings. This is the opposite of
the workshop-tools plan's D3, and for a good reason: workshop tools is meaningless
without treasure randomization, while this setting is a complete change on its own.

Because D1 put the action on YES, there is no asymmetry to worry about: YES is the
only state that needs a file written, so `|| disableMergoDarkness_` covers it
exactly. NO with everything else off correctly does nothing, because nothing is what
NO asks for.

**D5 — Report it in the progress log.** Only when it does something, matching the
"no noise for features that were never going to run" rule:

```
DISABLED MERGO DARKNESS     (setting YES)
                                (setting NO: no line)
```

**D6 — When the setting is NO, skip the step entirely.** No write, no re-compression.
The `event/` folder is already mirrored verbatim from vanilla (`kMirrorFolders`,
`EnemyRandomizer.cpp:175`), so skipping leaves a genuine, bit-exact vanilla file in
place. Writing the reference's restore bytes instead would be semantically identical
but is not guaranteed to reproduce the original DCX byte for byte, since our
`DcxCompress` need not match From Software's zlib settings. Skipping is provably
correct rather than argued-correct.

No stale-file risk: the mirror re-copies `event/` from vanilla at the start of every
run, so a previous run's modified file is always overwritten before this step decides
anything.

---

### 3.1 Write both states — and why not to pre-poke VanillaSource

Replaces D6.

Once the polarity was known, the setting still read oddly: the default wrote to
the file and the non-default wrote nothing. Adam's suggestion was to poke
`VanillaSource` once so the source tree already held the lit bytes, leaving the
setting to poke darkness back in only when asked.

**Right instinct, wrong tree to edit.** The same result comes from writing the
chosen pattern in both cases, which is what shipped:

| | |
|---|---|
| NO | write `(10000, 99999, 0)` |
| YES | write `(10000, 5630, 1)` |

The asymmetry disappears — the setting selects a value rather than selecting
whether anything happens — and `ApplyPermaDarkness`'s `true` branch, dead until
now, becomes the live path for YES.

**Why editing `VanillaSource` was rejected:**

- **It breaks silently on re-dump.** Re-extracting `VanillaSource` from the disc
  is an ordinary maintenance action, and it would quietly revert the poke and
  flip the default back to a dark world with nothing to indicate why.
- **It breaks seed sharing.** One person's source tree would be pre-poked and
  another's would not, so identical settings and seed would produce different
  games.
- **It makes "vanilla" a lie.** Every verifier and diff in this project treats
  that tree as pristine reference data;
  `mergo_darkness_verify show VanillaSource-Clean` would report the poked state.
- **It hides §2.7's anomaly** instead of leaving it visible for whoever
  eventually explains it.

**And writing both states turns out to be a hedge against §2.7's leading
hypothesis.** If a user's `VanillaSource` is not the same revision as the file
their installed game ships, "leave the mirrored copy alone" means *"ship
whatever that dump happened to contain"* — unknown by construction. Writing both
states pins the output to one of two known patterns no matter what the source
holds. The uncertainty stops mattering.

The one argument D6 made for skipping the write — that re-compressing might not
reproduce byte-identical DCX — is empirically dead: the lit path already
re-compresses and runs correctly on hardware.

---

## 4. Implementation

Ten files, all small. No new engine logic — `ApplyPermaDarkness()` already does the
work; it has simply never been told when not to.

### 4.1 Settings chain (follows `randomizeWorkshopTools` exactly)

| File | Change |
|---|---|
| `src/Randomizer/RandomizerDefaults.h` | Add `bool disableMergoDarkness = false;` after `randomizeShopWeapons`, with a comment recording that `false` is the do-nothing state (D3) |
| `src/Randomizer/RandomizerDefaultsStore.cpp` | Read `disable_mergo_darkness` in the key loop; add it to the `snprintf` format and argument list. Current worst case is ~295 bytes of `char buf[512]`; this key adds ~30, for ~325. The clamp added by the workshop-tools change stays as the guard |

### 4.2 Setup Defaults screen

| File | Change |
|---|---|
| `src/UI/SetupDefaultsScreen.h` | `kItemCount` 11 → 12; add `kDisableMergoDarknessRow = 11`. **No renumbering** (D2) |
| `src/UI/SetupDefaultsScreen.cpp` | One `else if` branch in `ToggleRow`; one entry appended to the `DrawList` items vector |

### 4.3 Enable wizard

| File | Change |
|---|---|
| `src/UI/EnableWizardScreen.h` | Add `bool disableMergoDarkness_;` after `randomizeShopWeapons_` |
| `src/UI/EnableWizardScreen.cpp` | Add `kDisableMergoDarknessRow = 11`; `kSaveDataRowCount` 11 → 12; initialise from `defaults.disableMergoDarkness` in the ctor init list; add the left/right branch and the `input.cross` branch in `UpdateSaveData`; **add `disableMergoDarkness_` to the `||` that decides whether to create the job** (D4) and set `options.disableMergoDarkness`; add the D5 line in `FinishCommit`; append the row to both the `DrawSaveData` and `DrawConfirm` item vectors |

### 4.4 Engine

| File | Change |
|---|---|
| `src/Randomizer/EnemyRandomizer.h` | Add `bool disableMergoDarkness = false;` to `EnemyRandomizerOptions`. Not a param feature, so it stays out of `AnyParamFeature()` |
| `src/Randomizer/EnemyRandomizer.cpp` | In `StepEmevd`, wrap the existing block in `if (options.disableMergoDarkness) { … }`, still calling `ApplyPermaDarkness(..., /*permaDarknessOn=*/false)` — that is the branch that cuts the wire (§2.4). Add an `else` logging that the file was left as vanilla. The best-effort, `Fail`-free structure around it is unchanged |
| `src/Randomizer/PermaDarkness.h` | Rewrite the header comment. It currently says the poke runs "regardless of any checkbox" and treats both states as opaque byte patterns. Replace with §2's findings: which event, which SpEffects, which entities, that the `true` branch is the reference's in-place *restore* and is therefore unused here, and that this port achieves the same thing by not writing at all |

Filenames and the `ApplyPermaDarkness` signature stay as they are. The `true` branch
becomes unreachable in this port, but it is correct, tested, and documents how the
reference restores — deleting it would mean touching the build file for no behavioural
gain. Note it in the comment rather than removing it.

Progress step counting is unaffected: `StepEmevd` is one step whether or not it writes.

### 4.5 Docs

| File | Change |
|---|---|
| `docs/randomization-feature-spec.md` | Row 8: replace "the Blood Moon / darkness state is forced" with the Wet Nurse description, record the rename from the reference's inverted name, update Status. Decrement the §10 tally |
| `docs/windows-randomizer-technical-review.md` | Mark §13 open question 9 resolved (§2.6) |

---

## 5. Verification

### 5.1 A new verifier — `tools/mergo_darkness_verify.py`

None of the existing verifiers touch `.emevd` files, so there is nothing to extend.
This one is small — the decode in §2.1 is about 40 lines of Python on top of
`boss_verify.read_dcx`.

```
python mergo_darkness_verify.py show   <dvdroot>            # print event 6548972's args
python mergo_darkness_verify.py verify <dvdroot>            # expect vanilla  (10000, 5630, 1)
python mergo_darkness_verify.py verify <dvdroot> --disabled # expect disabled (10000, 99999, 0)
```

Worth building rather than eyeballing, for one specific reason: **the two states differ
by four bytes inside a zlib-compressed file.** There is no way to tell a correct run
from a broken one by looking at the output tree, and "the area looked dark" is a slow
and unreliable way to find out.

Have it assert the surrounding structure too — event found, two instructions, first is
`2004[08]`, 12 argument bytes — so a future change to `DcxCompress` or to the EMEVD walk
fails loudly here rather than silently poking the wrong offset.

### 5.2 Test matrix

| Run | Expect |
|---|---|
| Any feature on, setting **NO** | `event/common.emevd.dcx` **byte-identical to vanilla** (D6 — skipped, not rewritten); `verify` passes. ⚠ This differs from every run made before this change, which is the intended behaviour change in D3 — `common.emevd.dcx` should be the *only* file that differs from a same-seed pre-change run |
| Any feature on, setting **YES** | The file differs from vanilla; `verify --disabled` passes; args are `(10000, 99999, 0)`. Byte-identical to today's output for the same seed — this is the regression guard on the engine path |
| Setting **YES**, everything else off | A run happens and a tree is built (D4). Every `map/mapstudio/*.msb.dcx` is an exact vanilla copy; only `common.emevd.dcx` is poked |
| Everything off including the setting | No run, no tree. Nothing is asked for, so nothing is built |
| Setting toggled, same seed | Every `map/mapstudio/*.msb.dcx` and the item-data archive byte-identical across the two runs. This setting must not perturb the RNG stream — it never draws from it |

### 5.3 Hardware check

Two things need eyes on a TV rather than more byte-reading. Mergo's Loft is deep into
the game, so a save parked near the Wet Nurse is worth keeping for this.

**Check 1 — does the setting do what §1 says?** Boss randomization **off**, so the Wet
Nurse is still in her own arena:

- Setting **NO** → Mergo's Loft is dark, and brightens after she dies.
- Setting **YES** → Mergo's Loft is lit throughout.

If both look the same, the marker chain means something other than area lighting and §1
needs rewriting. The findings in §2.1 and §2.4 stand regardless.

**Check 2 — is the "perma" failure mode real?** Setting **NO**, boss randomization
**on**, so something other than the Wet Nurse holds entity `2600800`: kill whatever is
in that arena and see whether the area brightens afterwards.

- Brightens → the boss script still completes, the permanent-darkness failure mode is
  not real, and §1's "why you might want to turn it on" loses its main argument.
- Stays dark → §2.3 is confirmed, and the reference tool's default is explained.

This is the more interesting of the two: it is the only test of *why the setting
exists* rather than of the mechanism it uses. Either result is worth recording in
§2.3 — neither changes the implementation.

---

## 6. Resolved decisions

Kept as a record of what was decided and why, since several reversed during design.

**Default = NO, meaning do nothing (D3).** Chosen over "NO = today's behaviour of
cutting the darkness". The app should not silently delete a vanilla effect, and an
absent config key must mean the inert state.

**The setting starts a run on its own (D4).** Chosen over the workshop-tools
precedent of staying out of the run gate. A commit should build a tree that matches
the settings, and this setting is a complete change by itself rather than a modifier
on another feature.

**Named for the action, not the end state (D1).** The reference's `permaDarknessBool`
is named so that its *unchecked* position is the one that edits the game, which reads
backwards to everyone who meets it. Naming the row `DISABLE MERGO DARKNESS` makes
YES mean "do it" and NO mean "don't", and as a side effect removes the run-gate
asymmetry that the inverted name created.

**Open, and deliberately not blocking:** whether the permanent-darkness failure mode
in §2.3 is real. §5.3 Check 2 settles it. The setting is worth having either way, NO
is the right default either way, and no code changes on the answer — only §1's and
§2.3's wording.
