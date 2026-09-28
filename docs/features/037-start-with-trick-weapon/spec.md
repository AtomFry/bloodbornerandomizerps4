# Feature 037 — Start With A Trick Weapon

**Status: APPROVED** — the developer read and approved this spec on 2026-09-27.
Human gate 0h is passed; it is ready for `/plan 37`.

**Reference:** **New feature.** The reference tool has no equivalent — it never
touches the character-creation template at all (§3).

**Plan:** `docs/features/037-start-with-trick-weapon/plan.md` — added after the spec is approved

---

## 1. What does this feature do?

In vanilla Bloodborne a new character wakes in Iosefka's Clinic with nothing in
either hand. The first weapon comes later, from the three-way choice of coffins
in the Hunter's Dream, and until then the clinic and the first stretch of Central
Yharnam are played bare-fisted.

This feature lets the player start the run already holding a trick weapon.

It is a **pool picker**, not a single choice, with the same none/one/many
semantics `ENEMIES INCLUDED` already has:

* tick **nothing** and nothing is granted — the run is vanilla in this respect
* tick **one** weapon and every new character starts with that weapon
* tick **several** and one of them is drawn at random for the run

Each of a weapon's three versions is its own row in the picker, so
`SAW CLEAVER`, `UNCANNY SAW CLEAVER` and `LOST SAW CLEAVER` are three separate
choices and the player may tick any combination of them (D2).

It changes what a new character *begins with*. It does not change what the
Hunter's Dream offers, and it does not change anything for a character that
already exists.

**This feature is not yet known to be possible.** The game's character-creation
template has room to name a starting weapon, and the room is unused, but nothing
observed so far shows that Bloodborne reads it. §4.5 sets out exactly how far the
evidence goes, and §8 defines the console probe that settles it — the first
milestone of the plan (D6). The honest outcome if the probe fails is that the
feature is deleted rather than grown; that is the rule row 34 set for itself and
this row inherits it.

---

## 2. What does the player experience?

| Setting | Off | On |
| ------- | --- | -- |
| **START WITH A TRICK WEAPON** | A new character wakes in Iosefka's Clinic empty-handed. The first weapon is the Hunter's Dream coffin choice | A new character wakes in Iosefka's Clinic **with a trick weapon**, and can use it against the clinic beast and everything before the Dream. Which weapon depends on what is ticked: nothing ticked grants nothing, one ticked always grants that one, several ticked draws one of them for the run |

**What the picker offers.** **78 rows** — all 26 of Bloodborne's right-hand trick
weapons, base game and The Old Hunters together, each listed three times: the
ordinary weapon, its Uncanny version and its Lost version. So the Saw Cleaver
appears as three rows and the Kos Parasite as three rows. §4.1 names them and
§4.2 is the evidence that 78 is the whole set.

**What the three versions mean to the player.** Nothing except which blood gems
will fit later: the three versions of a weapon have identical strength, skill,
bloodtinge and arcane requirements and identical everything else that matters at
the clinic — they differ only in the shapes of their gem slots (§4.3). The
inventory shows the name the game shows, so a ticked Lost Saw Cleaver arrives
called "Lost Saw Cleaver".

**What the picker leaves alone.** The game's other **16** player weapons — the
12 firearms, both shields and both torches, all of them left-hand items, none of
which has a Lost or Uncanny version — are **not** in the picker and can never be
granted (D1). Nor can bare fists. Nor can six further right-hand entries that
exist in the game's data but have no name anywhere in the game's own text and
cannot be obtained in vanilla by any means (§4.2). That is the whole of the rest.

**Everything else about the starting character is unchanged.** Same origin, same
starting stats and level, same Hunter's Mark, same Black Hood, Foreign Garb,
Sullied Bandage and Foreign Trousers. Nothing is taken away to make room.

**When it takes effect.** At character creation only. Ticking the setting and
loading an existing save grants nothing — a run has to start a new character for
it to be visible.

**The weapon is usable immediately.** A level-4 or level-10 starting character
cannot meet most trick weapons' requirements as the game ships them — 39 of the
78 rows cannot be wielded by *any* origin (§4.3). The granted weapon's
requirements are lowered to strength 9, skill 9, bloodtinge 5 and arcane 6, which
every origin meets, so whatever is granted can be swung at once (D3). That
reduction stays in place for that weapon for the whole run, including its
upgraded forms; the side effect is accepted deliberately.

**In hand, or in the inventory.** The intent is a character who wakes up holding
the weapon. If the game turns out to accept the grant only as an inventory item,
that is an acceptable outcome rather than a failure, and the player equips it from
the menu on the first screen (D5). §8's probe is what decides which.

**Combined with `RANDOMIZE STARTING WEAPONS`.** Independent, and either, both or
neither may be on. With both on the player starts with the granted weapon *and*
finds three randomized coffins in the Hunter's Dream. Where the same weapon is
both the grant and a coffin pick, the grant's requirement reduction is the one
that applies (D4) — a difference of a point or two that no player would see, made
an explicit rule rather than an accident of ordering.

---

## 3. What does the existing randomizer do?

**Nothing. There is no reference implementation of this feature.**

Established rather than assumed: the string `CharaInitParam` does not occur
anywhere in `reference/Randomizer/`. The only matches in the whole `reference/`
tree are in `reference/SoulsFormats/`, in the MSB part-record definitions of four
*other* FromSoftware games, where an unrelated field happens to point at that
param. The reference tool has no code path that edits character creation.

The nearest reference behaviour is the one this port already ships as row 5's
three toggles: it randomizes the **weapons on sale** — the Hunter's Dream coffin
choices and the Bath Messengers' stock. That changes what the player can *pick
up in the Dream*, never what the character starts holding.

So there is no reference behaviour to match and no reference quirk to preserve.
The standing rule in `CLAUDE.md` §7 — match the reference before improving it —
is satisfied trivially: this is an addition, not a change to anything the
reference does.

---

## 4. What do we know?

All measurements are against `data/vanilla/dvdroot_ps4`, taken with
`app/tools/param_offsets.py`'s existing paramdef and param readers, not from any
document. Every claim below is labelled **fact**, **inference** or
**assumption**.

### 4.1 The 78 picker rows, and where the names come from

**Fact.** The game's own English text carries every weapon name, in
`msg/engus/item.msgbnd.dcx`, and it is present in the local vanilla tree. The 26
right-hand trick weapons are:

Amygdalan Arm, Beast Claw, Beast Cutter, Beasthunter Saif, Blade of Mercy,
Bloodletter, Boom Hammer, Burial Blade, Chikage, Church Pick, Holy Moonlight
Sword, Hunter Axe, Kirkhammer, Kos Parasite, Logarius' Wheel, Ludwig's Holy
Blade, Rakuyo, Reiterpallasch, Rifle Spear, Saw Cleaver, Saw Spear, Simon's
Bowblade, Stake Driver, Threaded Cane, Tonitrus, Whirligig Saw.

**Fact.** Each of the 26 has exactly three named versions in the game's text —
the plain weapon, an Uncanny one and a Lost one — giving **78 named right-hand
rows**, which is the picker. All 78 names are distinct, so no row is ambiguous
without an id beside it.

**Fact.** The game's text does not name them uniformly: it is "Uncanny Saw
Cleaver" for most, but "Ludwig's Uncanny Holy Blade", "Logarius' Uncanny Wheel",
"Uncanny Bowblade" and "Uncanny Parasite". The picker must show the name the game
shows rather than a name assembled from a prefix and a base, or the row will not
match what the inventory says.

**Fact.** All 78 names are renderable by the app's 8x8 fallback font — they use
only letters, spaces and the apostrophe, all of which are in the glyph table
(`docs/known-traps.md`, "8x8 font limitations"). The longest is
`UNCANNY HOLY MOONLIGHT SWORD` at 28 characters; with an id column and a flag
column the widest row comes to 43 characters, inside the 71-character budget
`pool_verify.py` pins and narrower than the 49-character widest row
`ENEMIES SKIPPED` already draws.

**Inference.** The names therefore do **not** need hand-authoring, and the
picker's table does not need a curated name file the way the enemy pickers need
`tools/data/Characters.json`. The backlog row's "37 names is a hand-authored
table" is superseded: the list can be generated from the vanilla tree the same
way `gen_pool_table.py` generates the three creature tables, with the same
`--check` pin and the same frozen order. Reasoning: the generator already takes a
vanilla dvdroot path, already bakes a committed header, and the name source here
is the shipped game rather than community data — strictly better provenance than
the enemy tables have. It also removes the per-version naming trap above, because
a generated name cannot disagree with the inventory.

### 4.2 78 is the whole set, and what it spares

**Fact.** The weapon param holds 1090 rows. A weapon id decomposes exactly into
family, weapon-within-family, version and upgrade tier, and unpacking it gives
**47 distinct player weapons**, each with a base row plus ten upgrade tiers and,
for the trick weapons, an Uncanny and a Lost variant of every tier.

**Fact.** Of those 47, the split by which hand the game allows is clean and comes
from the data itself:

| Group | Distinct weapons | Named rows at tier 0 |
|---|---|---|
| Right-hand only | 26 | **78 — the whole picker** |
| Left-hand only | 16 | 16 — 12 firearms, 2 shields, 2 torches, none with a Lost or Uncanny version |
| Neither hand | 5 | Bare Fists, and four rows the game's text does not name |

**Fact.** Counting every right-hand row at tier 0 regardless of version gives
**84**, six more than the picker. All six are excluded, and two independent tests
agree on all six:

| Row | Why it is excluded |
|---|---|
| version 8 under Logarius' Wheel | the game's text gives it the placeholder name `*`, and it appears in no shop and no item lot |
| version 9 under the Kos Parasite | no name at all in the game's text; in no shop and no item lot |
| four unnamed weapons (families 37, 39, 40, 41) | no name at all; in no shop and no item lot |

**Fact.** By contrast, **all 78** picker rows are obtainable in vanilla — every
one of them is sold in a shop or appears in an item lot. So the picker is exactly
"the right-hand weapons a player can actually get", and the six exclusions are
exactly the ones a player can never see.

**Fact — what versions 8 and 9 are.** Version **9** exists for 44 of the 47
weapons, one row each: no upgrade tiers, no hand bits set, price 0, sell value
-1, sort id -1, and a name of `*` or nothing. Version **8** exists exactly twice
in the whole param — one under Ludwig's Rifle, one under Logarius' Wheel — and
those two are full upgradeable rows with real prices but no player-facing name or
description.

**Inference.** Version 9 is the param's per-weapon placeholder row, not content:
a row with no hand bit cannot be held in either hand, and one with no upgrade
tiers, no price and a `*` name is not a weapon the game ever hands out. The two
version-8 rows are cut or internal duplicates: fully statted and priced, but
nameless and unobtainable. Neither belongs in a picker of things a player can be
given. Reasoning: the "unobtainable anywhere" measurement is independent of the
"unnamed" measurement and they agree on every one of the six rows, which is what
makes this a finding rather than a guess.

**Fact, and a correction to the backlog row.** Row 37 says the list is 37 and
that no field distinguishing melee from firearm was findable. Both are wrong:

* 37 is the count of top-level id families, which groups *different weapons*
  together — Saw Cleaver with Saw Spear, Hunter Axe with Burial Blade, Kirkhammer
  with Ludwig's Holy Blade, Hunter Pistol with Evelyn and the Repeating Pistol.
* The weapon param carries `rightHandEquipable` and `leftHandEquipable` bits, and
  they partition all 47 without a single ambiguous case. `weaponCategory` also
  exists but is **not** the field to use: its value 0 covers both the Hunter
  Blunderbuss and the Threaded Cane.

**Fact.** The port already carries a hand classification, transcribed from the
reference tool for row 5: `StartingWeaponLists.h`'s 76-entry "melee" list and
14-entry "firearm" list. Measured against the hand bits, those two lists are hand
lists, not weapon-type lists, and they are incomplete: the firearm list contains
the Loch Shield and the Hunter's Torch, and the melee list omits the base Beast
Claw and the Uncanny Saw Spear — so it is 76 where this feature's picker is 78.
They are the reference's membership tests and they are correct for what row 5 uses
them for; they are **not** a usable definition of "the trick weapons".

**Fact.** 33 of the 78 rows are The Old Hunters weapons. This is not a new
exposure: the reference's own starting-weapon list, which this port reproduces for
row 5, already puts DLC weapons into the Hunter's Dream coffins, so a run can
already offer one today.

### 4.3 Why the requirements have to come down

**Fact.** The ten player origins start at level 10, except Waste of Skin at level
4. Across all ten, the lowest value any origin has is strength 9, skill 9,
bloodtinge 5, arcane 6.

**Fact.** A weapon's Uncanny and Lost versions carry requirements identical to
the plain weapon — checked across all 52 variant rows, zero differences. So the
78 picker rows present 26 distinct requirement sets, and the wieldability picture
below is the same whichever version is ticked.

**Fact.** Measured against those origins, of the 26 trick weapons as the game
ships them — and so of the 78 rows, three per weapon:

| | Weapons | Picker rows |
|---|---|---|
| wieldable by **every** origin | 4 — Saw Cleaver, Saw Spear, Hunter Axe, Threaded Cane | 12 |
| wieldable by **at least one** origin | 13 | 39 |
| wieldable by **no** origin at all | 13 | 39 |

**Inference.** Without a requirement change, exactly half the picker grants a
weapon the character is holding and cannot swing. That is why §2 states the
weapon is usable immediately, and it is the evidence behind D3. Reasoning: the
numbers above are direct comparisons of each weapon's four requirement values
against each origin's four stat values.

**Fact.** The port already does exactly this rewrite for row 5:
`StartingWeapons.cpp`'s `ApplyStatProfile()` lowers the four requirements on a
chosen weapon and on each of its ten upgrade tiers. What it does not have is a
profile for a *granted* weapon — its five profiles are keyed by Hunter's Dream
coffin slot. D3 supplies the one profile it needs and leaves the function
unchanged.

**Fact, and a second correction to the backlog row.** The row states that
`7010000` is the Lost Saw Cleaver and `7020000` the Uncanny. The game's own text
says the opposite: version 1 is **Uncanny** and version 2 is **Lost**, for all 26
weapons. With D2 listing every version separately, getting this backwards would
mislabel 52 of the 78 picker rows.

### 4.4 The rest of the starting character, as it ships

**Fact.** All 22 player-origin rows of the character-creation param hold exactly
one starting item — a single Hunter's Mark — with the other nine item slots empty,
and a full set of starting clothes: Black Hood, Foreign Garb, Sullied Bandage,
Foreign Trousers. Nothing has to be displaced to grant a weapon; there is room
for one and eight spare.

**Fact.** Those are the same 22 rows `START WITH HUNTER TOOLS` (row 34) already
writes, and the same param member the same pass already has decompressed and
open. The two origin blocks are identical in every field either feature touches
and the data cannot say which one the game instantiates, which is why row 34
writes all 22 as a deliberate hedge.

### 4.5 The central unknown, stated as far as the evidence actually goes

This is the whole risk of the feature, and the byte-level evidence does **not**
resolve it. `docs/known-traps.md`, "Byte decoding does not prove game behavior",
and `docs/plans/mergo-darkness.md` §2.7 are the standing reason to treat it this
way: on row 8 the byte analysis was right in every particular and the conclusion
about what the game did with the bytes was backwards.

**Fact.** The character-creation template has four weapon fields per row — two
per hand — and on **all 1700 row descriptors in the entire param**, player origins
and NPC templates alike, every one of those four fields is empty. Bloodborne's
shipped data never uses them.

**Fact.** The neighbouring clothing fields of the same family *are* used heavily:
between 1557 and 1641 rows set each of the four. So the engine demonstrably reads
equipment out of these rows — the starting Black Hood comes from here.

**Fact.** The template's ordinary inventory slots, the ones row 34 writes goods
into, **do** carry weapon ids in the shipped data: three rows list a Hunter Axe or
Saw Spear, a Kirkhammer and a Hunter Pistol as inventory items, and 120 further
rows list weapons — including upgraded ones — in the secondary-item slots.

**Inference, two-sided, and this is the honest position.** That the engine reads
*equipment* from these rows is established by the clothing. That it reads the
*weapon* fields specifically is not: they are unexercised everywhere in the game's
own data, which is weaker ground than "other FromSoftware titles use them", not
stronger. Meanwhile the *inventory* route is demonstrably exercised with weapon
ids. So there are two candidate routes, and the one the backlog row assumed is the
unused one. D5 accepts either outcome; §8's probe decides which the feature ships
on.

**Assumption, with its verification.** A weapon granted this way behaves like any
other weapon once held — it can be equipped, upgraded and gem-fitted normally.
Verified by the hardware test in §8; nothing offline can establish it.

### 4.6 What the feature costs in the shipped UI and config

**Fact.** The picker component is already generic over three lists of 82, 17 and
85 rows. A 78-row list is smaller than two of the three and four rows smaller
than `ENEMIES INCLUDED`, which is shipped and hardware-proven; at 12 visible rows
it is 7 pages, the same as 82. The settings table is now identity-keyed rather
than index-keyed, so the setting itself is one entry in one list.

**Fact.** The settings block in the config file is 584 bytes today inside a
1024-byte buffer, and a whole `defaults.cfg` worst case is 635 bytes — both pinned
by exact-equality `pool_verify.py` selftest cases. A 78-character selection line
under a key named like `trick_weapons_included=` costs 102 bytes including its
newline, taking the settings block to **686** and the worst-case file to **737**.
That leaves 338 bytes of the buffer unused. There is ample room; the exact
arithmetic and the pinned cases are stage C's to update.

**Fact.** Two shapes of the existing machinery do not fit a weapon list without a
decision from stage C, and they are named here only because they bear on what the
player sees: a picker table row is keyed by a **string** model id while weapon ids
are integers, and the picker draws every row as that id followed by the display
name — so a weapon row would read `7010000 UNCANNY SAW CLEAVER` unless the table
is given something else to draw. All 78 names are distinct, so the id column is
not needed to disambiguate them the way it is for the enemy lists.

**Fact.** A picker-only setting is not a toggle. The shipped readiness summary
counts toggles ("9 OF 15 SETTINGS ENABLED") and the commit gate asks which
toggles are on; a setting whose only state is a pool selection is in neither
today. Whether ticking one weapon and nothing else is enough to build a tree is a
consequence of that, and §7 records it as a constraint.

---

## 5. Terminology

**Trick weapon.** A right-hand weapon with a transformed second form — what
Bloodborne means by the term, and in this feature exactly the 26 weapons of
§4.1. Distinguished here from **firearm**, **shield** and **torch**, which are
left-hand items and are not in the picker.

**Version.** A trick weapon's plain, Uncanny or Lost form. Same weapon, same
requirements, different gem-slot shapes, different name in the inventory. All
three are separate picker rows (D2), which is why the picker is 78 rows and not
26. Not to be confused with **upgrade tier**, the +1…+10 states, which the picker
never offers — a granted weapon is always at +0.

**Origin.** The character-creation background the player chooses — Milquetoast,
Waste of Skin, and the rest. Each one is a separate template row, which is why
the feature has to write more than one.

**Grant.** Put the weapon into the new character's possession. Deliberately
distinct from **equip**, which is to have it in hand at spawn. §8's probe decides
whether those two are separable here; D5 accepts either.

**Pool picker.** The shipped drill-in list pattern — `ENEMIES INCLUDED`,
`BOSSES INCLUDED`, `ENEMIES SKIPPED`. Ticked rows are the candidates.

---

## 6. Scope

Covers backlog row **37** only, "Start with a trick weapon", in
`docs/randomization-feature-spec.md` §4.

### In scope

* One new setting granting a trick weapon at character creation, with pool-picker
  none/one/many semantics
* The 78-row weapon list — 26 trick weapons × three versions each — generated from
  the vanilla tree and pinned like the three existing creature tables
* Lowering the granted weapon's requirements to the measured origin minimum so it
  is usable at the starting level (D3)
* The console probe of §8, as milestone 1 of the plan (D6), and the explicit
  contract that a negative result deletes the feature rather than growing it
* The stated precedence rule against `RANDOMIZE STARTING WEAPONS` (D4)

### Out of scope

* **Firearms, shields and torches** (D1). A firearm equivalent is later work and
  its own backlog row; because the hand is derivable from the data rather than
  hand-classified, adding it later is mechanical
* **The six unnamed, unobtainable right-hand rows** of §4.2
* **Upgrade tiers.** A granted weapon is at +0; the picker offers versions, not
  upgrades
* **Starting armour, runes, gems, blood vials, bullets or any other starting
  item.** Row 34 covers the two workshop tools; nothing else is on the list
* **Changing what the Hunter's Dream offers.** That is row 5, already shipped
* **Any fallback route if the probe fails.** If the character-creation template
  turns out not to grant weapons at all, finding another way in is new work and a
  new backlog row. Note that the inventory route is *not* a fallback in this
  sense — it is an accepted outcome under D5
* **Narrowing the 22 origin rows to the live block.** That is row 34's open item
  O2 and depends on the same hardware test
* **Chalice dungeons**, per `CLAUDE.md` §7

---

## 7. Constraints and decisions

The six decisions themselves are in §10. The constraints they sit inside:

* **The probe governs the feature.** The behavioural question in §4.5 is not
  answerable offline. No amount of further param work moves it, and the feature
  must not be described anywhere as working until the console says it does — the
  `BUILT` versus `DONE` distinction in the backlog's status legend, and
  `CLAUDE.md` §3.
* **A negative probe result deletes the feature.** Granting an invisible weapon
  is worse than not offering the setting. This is row 34's own rule, stated in
  `HunterTools.h`, and it applies here unchanged. It does not apply to the
  inventory outcome, which D5 accepts.
* **Default is nothing ticked.** A `defaults.cfg` written before this setting
  existed, or carrying a stale-length selection line, must read as *nothing
  granted*, so an existing config produces the run it produced before, roll for
  roll. This is the same fail-safe polarity `ENEMIES SKIPPED` uses.
* **The weapon list's order is load-bearing.** A selection is stored positionally,
  one character per row, so regenerating the table in a different order silently
  remaps a saved selection onto the wrong weapons — and with 78 rows that means
  onto the wrong *version* of a weapon, which is harder to notice than the wrong
  creature. Frozen order, and a changed count must be rejected rather than
  misapplied.
* **Names come from the game, not from a hand-written table**, and not assembled
  from a prefix — §4.1's four irregular names are the reason. A generated table
  with a `--check` pin, matching `gen_pool_table.py`.
* **Only renderable names ship.** The 8x8 fallback font draws unsupported
  characters as blank rather than failing (`docs/known-traps.md`). All 78 names
  pass today; the generator must fail loudly rather than quietly if that ever
  changes.
* **Row 5 must not change.** `RANDOMIZE STARTING WEAPONS`,
  `RANDOMIZE STARTING GUNS` and `RANDOMIZE SHOP WEAPONS` keep their present
  behaviour exactly, byte for byte, and `ApplyStatProfile` is reused rather than
  altered. D4's precedence must be an explicit rule in the code, not a consequence
  of which of two calls happens to run second.
* **Row 34 must not change.** Both features write the same origin rows of the
  same param member, and both must be able to be on at once without either losing
  its write.
* **Determinism.** Drawing the weapon consumes randomness. Turning the setting on
  must not silently reshuffle the rest of a run's world for a given seed, or two
  runs on the same seed stop being comparable.
* **A picker-only setting must still gate a run.** Ticking one weapon and nothing
  else is a complete request and must build a tree, the way row 34's D3 settled
  for a standalone toggle. §4.6 records why that does not fall out of the existing
  machinery for free.
* **The archive is edited in place.** Every write is one value over another inside
  an existing row; nothing is resized and the archive is never rebuilt.

---

## 8. How will we know it works?

### Automated testing

Nothing offline can prove the feature works — §4.5 is the reason. What offline
verification must establish is that the edit is the intended edit and nothing else
changed:

* The 78-row list is exactly the set of named, obtainable right-hand player
  weapon versions in the vanilla tree, re-derived from the data rather than
  compared against a copy of itself, and the committed table matches what the
  generator produces now.
* The six excluded right-hand rows of §4.2 are absent, and the exclusion is
  re-derived rather than hardcoded as six ids.
* Every name in the table is the name the game's own text gives that exact row,
  and every one is drawable by the shipped font.
* With nothing ticked, the character-creation param is byte-identical to vanilla
  and no row changes.
* With one row ticked, every player-origin row names that weapon *version* — not
  its base version — and no other field of those rows changes; in particular the
  starting Hunter's Mark and the four clothing entries survive untouched.
* With several ticked, the weapon granted is one of the ticked ones, the same seed
  gives the same weapon, and a different seed can give a different one.
* No non-origin row is touched, and no other member of the archive is touched.
* The granted weapon's requirements come out at strength 9, skill 9, bloodtinge 5
  and arcane 6 — met by every origin — at the base row and at all ten upgrade
  tiers, and no other weapon's requirements change.
* With `RANDOMIZE STARTING WEAPONS` also on, both features' writes are present and
  the grant is what set the shared weapon's requirements (D4).
* The config selection line round-trips at 78 characters, a wrong-length line is
  rejected and leaves nothing ticked, and the config file still fits its buffer.

The existing tools already reach all of this data: `param_offsets.py` resolves the
fields, `hunter_tools_verify.py` is the closest existing shape (same param, same
origin rows, same "prove each way of getting it wrong is rejected" discipline),
and `starting_weapons_verify.py` already asserts on the weapon requirement bytes.

### Hardware testing

**The probe is milestone 1 and it is the whole decision** (D6). Everything else in
this section is contingent on it, and the plan does not wait on row 34 — though
note that row 34's own outstanding hardware test would split this unknown in two
if it happens to run first, because tools arriving in a new character's inventory
would prove the game reads these origin rows and leave only the narrower question
about the weapon fields. That is useful context for reading a negative result, not
a dependency.

1. Tick exactly one row — plain `SAW CLEAVER` is the right choice, because it is
   the weapon a player recognises instantly and is one of the four already
   wieldable by every origin, so a failure cannot be a requirements failure in
   disguise. Build, install, **start a new character**.
2. Look at the character in Iosefka's Clinic.

| Observed | What it means |
|---|---|
| **Holding a Saw Cleaver** | The feature works as §2's first reading describes. Proceed to the checks below |
| **Saw Cleaver in the inventory, not in hand** | The grant works and the equip does not. This is an accepted outcome under D5: the feature ships, §2's "in hand" wording becomes "in the inventory", and the checks below still apply |
| **Nothing at all** | Either the game does not read the weapon fields, or it does not read these origin rows. The feature cannot be built this way and §7's deletion rule applies |

Once the probe is positive, the remaining checks:

* The weapon can be swung, transformed, and used to kill the clinic beast — not
  merely present in a menu.
* Every origin gets it, including Waste of Skin, and the weapon is wieldable
  without levelling up. A "requirements not met" message here means the
  requirement rewrite did not land.
* A weapon needing high strength or arcane — Logarius' Wheel and the Kos Parasite
  are the two extremes — is equally usable, which is what proves the rewrite
  covers the whole list and not just the easy end of it.
* A ticked **Lost** or **Uncanny** row grants that version and the inventory shows
  that version's name — the check that the 78-row table is not quietly collapsing
  to 26.
* With several ticked, two runs on the same seed grant the same weapon and the
  rest of the world is identical; runs on different seeds vary.
* With nothing ticked, the clinic spawn is empty-handed exactly as vanilla.
* With `RANDOMIZE STARTING WEAPONS` also on, the granted weapon is present and the
  Hunter's Dream still offers three randomized coffins, and neither has broken the
  other.
* An existing save loaded with the setting on gains nothing and is undamaged.

**The most important failure cases**, in the order they are likely: nothing is
granted at all (§4.5); the weapon is granted but cannot be equipped because the
requirement rewrite missed; the wrong *version* arrives, which looks like success
and is a table or ordering fault; and the weapon arrives but the starting Hunter's
Mark or clothing has been displaced.

---

## 10. Decisions

All six were put to the developer on 2026-09-27 and answered the same day. They
are part of the agreed behaviour of the feature.

| Date       | Decision |
| ---------- | -------- |
| 2026-09-27 | **D1 — Trick weapons only.** The pool is the named, right-hand-equipable weapons and nothing else. Firearms, shields and torches are out of scope; a firearm setting may come later as its own backlog row. The hand classification is measured from the data, not chosen — see §4.2 |
| 2026-09-27 | **D2 — Every version is its own picker row.** `SAW CLEAVER`, `UNCANNY SAW CLEAVER` and `LOST SAW CLEAVER` are three independent choices, so the player picks the exact weapon they want. This makes the picker **78 rows**, not 26. *Chosen against the spec's own recommendation of base-only* — the developer's reason was that the player should be able to see and pick each version |
| 2026-09-27 | **D3 — One requirement profile, at the measured origin minimum: strength 9, skill 9, bloodtinge 5, arcane 6**, applied through the existing `ApplyStatProfile` unchanged. Every origin meets it, so every one of the 78 rows is wieldable at once. The reduction persists on that weapon's upgrade tiers for the whole run; that side effect is accepted deliberately |
| 2026-09-27 | **D4 — The grant wins, as an explicit rule.** Where the granted weapon is also a `RANDOMIZE STARTING WEAPONS` coffin pick, the grant's requirement profile is the one that applies, and the code must say so rather than inheriting it from the order of two calls in the item-data pass |
| 2026-09-27 | **D5 — In hand preferred, in the inventory accepted.** The feature ships on whichever route the probe supports. A weapon that arrives in the inventory rather than in hand is an acceptable outcome, not a failure |
| 2026-09-27 | **D6 — Plan the whole feature now**, with the probe as milestone 1 of the plan rather than a separate planning pass, and without waiting for row 34's hardware test. *Chosen against the spec's own recommendation of testing row 34 first* — that reasoning is retained in §8 as context for reading a negative result |
| 2026-09-27 | **Version 8 and 9 rows are excluded, settled by investigation rather than referred to the developer.** Version 9 is the param's per-weapon placeholder — no hand bit, no upgrade tiers, no price, name `*` or absent. The two version-8 rows are nameless, unobtainable internal duplicates. All six excluded right-hand rows are both unnamed in the game's own text and absent from every shop and every item lot; all 78 included rows are obtainable. See §4.2 |

---

## Appendix — research notes for stage C

*Not part of the specification, and not covered by the developer's approval. These are pointers from the spec investigation to save stage C a search. Verify anything here before relying on it.*

**Where the behaviour lives**

* `app/src/Randomizer/HunterTools.cpp` — `kOriginRows`, `FirstEmptySlot`, and the
  per-row bounds check. This feature's closest sibling: same param member, same 22
  rows, same re-runnable-pass discipline. What is genuinely shared is the row list
  and the guard pattern; the field offsets and the grant itself are not.
* `app/src/Randomizer/EnemyRandomizer.cpp` — `StepItemData`, step 1. Where the
  param member is located and where the hunter-tools pass already sits relative to
  the starting-weapons pass; the ordering D4 forbids relying on.
* `app/src/Randomizer/StartingWeapons.cpp` — `ApplyStatProfile` and
  `kStatProfiles`. The requirement rewrite D3 reuses, and the reason it needs a
  profile that is not keyed by shop row.
* `app/src/Randomizer/StartingWeaponLists.h` — the reference's two hand lists.
  Read the header comment before reusing either; §4.2 records what they actually
  are and why 76 is not 78.
* `app/src/Randomizer/ModelPoolSelection.h` — the `DefaultSelected` template
  parameter is the fail-safe polarity this setting needs, and `Encode`/`Decode`
  are the config format.
* `app/src/UI/SettingsModel.cpp` — `kSettings`; one entry adds a row, and
  `SettingKind` is where a fourth pool kind would go.
* `app/tools/gen_pool_table.py` — the generator pattern, its `--check` mode, and
  its frozen-order warning; the model for a weapon table.
* `app/tools/param_offsets.py` — `load_defs`, `field_offsets`, `load_param`,
  `param_rows`. Everything in §4 was measured with these four functions; do not
  write a second param reader.

**Worth checking early**

* `data/vanilla/dvdroot_ps4/msg/engus/item.msgbnd.dcx` holds every weapon name, in
  a BND4 member whose name is Japanese. It is an FMG, wide 64-bit variant: group
  table at 0x28 with 16-byte entries, string-offset-table address at 0x18, offsets
  are 8 bytes, strings are UTF-16LE. `param_offsets.bnd4_members` reads the
  container already; the FMG reader is the only new parsing needed, and it is about
  twenty lines. Nothing in `app/tools/` parses FMG yet.
* The same archive's weapon-description FMG is a second, independent signal on
  whether a row is real content — the placeholder rows carry `*` or nothing there
  too.
* `ShopLineupParam` and `ItemLotParam` together are what make the obtainability
  test cheap; the lot param has eight item-id fields per row.
* The weapon id decomposes as family / weapon-within-family / version / upgrade
  tier. Getting this wrong is how the backlog arrived at 37 weapons instead of 47;
  derive it rather than assuming any single divisor. Note the Uncanny/Lost order
  is the opposite of what the backlog row states.
* `rightHandEquipable` and `leftHandEquipable` are packed **bits** in one byte,
  not whole fields — the same single-byte trap `StartingWeapons.cpp` documents for
  the requirement values, one step worse.
* The console is cp1252 here. Printing a weapon name or a Japanese member name
  without `PYTHONIOENCODING=utf-8` raises `UnicodeEncodeError`, which looks like a
  parsing failure and is not. `text_inventory.py` writes a file for exactly this
  reason.
* `pool_verify.py`'s config-size cases are exact equalities on purpose and will
  fail the moment a key is added. That is intended; update the arithmetic and the
  comment that narrates its history. §4.6 has the new numbers.
* Row 34's open item O2 — narrowing the 22 origin rows — is blocked on the same
  hardware test as this feature and should be resolved once for both.

**Dead ends already walked**

* `reference/` has no equivalent. `CharaInitParam` appears only in
  `reference/SoulsFormats/`, in four other games' MSB definitions. There is no
  reference behaviour to trace and no quirk to preserve.
* `weaponCategory` looks like the melee/firearm field and is not: value 0 covers
  both the Hunter Blunderbuss and the Threaded Cane. `wepmotionCategory`,
  `bulletConsumeNum` and `repositoryCategory` were each checked and each fail to
  separate the two groups. The hand bits are the only clean split.
* `StartingWeaponLists.h` looks like a ready-made classification and is not — it
  is the reference's hand lists, it puts a shield and a torch among the firearms,
  and it omits two real trick-weapon rows.
* Building the version list by appending "UNCANNY " and "LOST " to a base name
  gives the wrong string for four of the 26 weapons. Read the names, do not
  compose them.
* Looking for an existing example of a weapon in the character-creation weapon
  fields: there is none, anywhere in the param. Do not spend time searching for
  one; §4.5 is that search's result.
* `equip_Wep_Right_GenId` and its three siblings sit just past the requirement
  bytes and are empty on every row. They were checked in case they were a required
  companion to the weapon fields; nothing in the data says either way.
