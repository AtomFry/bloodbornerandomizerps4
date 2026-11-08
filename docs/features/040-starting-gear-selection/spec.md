# Feature 040 — Starting Gear Selection

**Status: APPROVED** — 2026-09-29. All four decisions are recorded in §10. **D1 was taken against this spec's recommendation.** Three of
them are scope decisions and all three are blocking: they change what §1 and §2
describe, not only how it is built.

**Reference:** **New feature.** The reference tool has no equivalent — it never
touches the character-creation template and never touches the armour table at all
(§3).

**Plan:** `docs/features/040-starting-gear-selection/plan.md` — added after the
spec is approved

---

## 1. What does this feature do?

A run can already decide what a new character *holds*. `START WITH A TRICK
WEAPON` and `START WITH A LEFT WEAPON` put a weapon in each hand, and
`START WITH HUNTER TOOLS` puts the two workshop key items in the pack. Nothing
decides what the character *wears*, nothing decides what supplies are in the
pack, and nothing lets the player fix what the Hunter's Dream coffins offer
instead of randomizing them.

This feature closes that gap. Under the scope this spec recommends it adds two
settings:

* **an outfit picker** — the same none/one/many pool picker the weapon settings
  use, over the game's wearable outfits. Tick nothing and the character wears
  what it always wore; tick one and every new character wears that outfit; tick
  several and one is drawn for the run.
* **a supplies toggle** — a new character wakes with a full pouch of Blood Vials
  and Quicksilver Bullets instead of an empty one.

It changes what a new character *begins with*. It does not change what the
Hunter's Dream offers, it does not change what the shops sell, and it does
nothing at all for a character that already exists.

**The scope is the substance of this row, and it is not settled.** The backlog
row names three separate things — armour, starting supplies, and fixing the
Dream's coffin picks — and an unbounded reading of all three is larger than rows
37 and 38 put together. The three were put to the developer as concrete
alternatives with measured costs and are settled in §10: **armour ships as four
per-slot pickers** (D1, against this spec's recommendation), **supplies as one
fixed toggle** (D2), and **the coffins are out of scope** (D3). §1, §2, §4.7, §6
and §8 have been reconciled to those answers.

**Unlike rows 37 and 38, this does not rest on an unproven route.** The game
demonstrably dresses a new character from the same template rows those two
features write, and demonstrably fills the pack from them — §4.1 and §4.4 are the
evidence. There is no probe milestone here and no "delete the feature if it does
not work" clause.

---

## 2. What does the player experience?

| Setting | Off | On |
| ------- | --- | -- |
| **START WITH HEADWEAR** (33 rows) | Nothing on the head, as the vanilla start | One of the ticked head pieces, drawn for the run |
| **START WITH BODY ARMOUR** (34 rows) | The starting sleeveless garb | One of the ticked body pieces |
| **START WITH GLOVES** (26 rows) | The starting bandaged forearms | One of the ticked glove pieces |
| **START WITH LEGWEAR** (31 rows) | The starting plain trousers | One of the ticked legwear pieces |
| **START WITH SUPPLIES** | A new character wakes with an empty pouch — no Blood Vials, no Quicksilver Bullets. The first vials come from the clinic and Central Yharnam | A new character wakes carrying **20 Blood Vials and 20 Quicksilver Bullets**, a full pouch of each, usable from the first fight |

**Four pickers, one per slot, offering all 124 wearable pieces between them**
(§10 D1). The counts — 33 head, 34 body, 26 gloves, 31 legwear — sum to exactly
the 124 the game lets a player wear, so nothing wearable is hidden. §4.2 is the
measurement and the two independent tests behind it.

**Each slot is drawn independently, so a loadout can be mixed.** Tick one piece in
a slot and every character wears it; tick several and one is drawn; tick none and
that slot keeps its vanilla garment. That is the point of D1's per-slot shape over
a single outfit picker: a player can ask for the Crowfeather hood over Gehrman's
coat, which no outfit picker can express.

**A consequence worth stating plainly: matched outfits are no longer a thing the
UI knows about.** The game groups its 124 pieces into 36 outfits, 25 of them
complete, but that grouping exists nowhere in the settings — a player who wants
the whole Charred Hunter set ticks its four pieces in four pickers. Nothing stops
them, and nothing helps them either. The upside is that **every row label comes
straight from the game's own text**, with no outfit name invented, because
Bloodborne names every piece and names no outfit (§4.3). Under the rejected
single-picker option that invention was unavoidable.

**What it takes away.** Unlike a weapon, armour replaces something: the character
is already dressed. Each picker that has anything ticked displaces that slot's
vanilla garment; each picker left empty leaves it alone. So a run with only
**START WITH HEADWEAR** ticked produces a character in the starting garb and
trousers with a hunter's hat on — and, because the vanilla head slot is empty on
the 3000 block, that is the one slot where the feature adds rather than replaces.
In every case the four starting garments remain ordinary items the character owns;
nothing is destroyed.

**What the picker leaves out.** Nothing a player can wear. The armour table also
holds **126 further entries in those same four slots that the game gives no
player-facing name at all** and that appear in no shop and no pickup anywhere in
the game — costume rows worn by particular non-player characters, and placeholder
rows — plus two named oddities, the Crown of Illusions and the Enlarged Head,
that the game marks as wearable by neither sex. All 128 are excluded. §4.2 is the
measurement, and the two independent tests behind it.

**Every piece is wearable at once.** Bloodborne's armour carries no strength,
skill, bloodtinge or arcane requirement and no equip-weight limit, so nothing has
to be lowered the way a granted weapon's requirements are, and a level-4 Waste of
Skin can wear the heaviest set in the game. §4.2.

**The supplies are a full pouch, not an unlimited one.** 20 of each is what
Bloodborne lets a hunter carry; buying more puts them in storage, and this
setting does not put anything in storage.

**When it takes effect.** At character creation only. Turning either setting on
and loading an existing save does nothing — a run has to start a new character
for it to be visible.

**Combined with the settings already shipped.** All independent; any combination
may be on.

* With `START WITH A TRICK WEAPON` or `START WITH A LEFT WEAPON`, the character
  wakes dressed *and* armed. Neither displaces the other.
* With `START WITH HUNTER TOOLS`, the pack holds the two workshop tools *and* the
  supplies. There is room for both and room to spare (§4.4).
* With `RANDOMIZE STARTING WEAPONS` / `GUNS`, the Hunter's Dream still offers its
  randomized coffins. This feature does not touch them.
* With `RANDOMIZE SHOP ARMOUR`, the Bath Messengers still sell a scrambled
  wardrobe. What the character starts wearing is decided here and is not affected
  by what the shop happens to stock.

---

## 3. What does the existing randomizer do?

**Nothing. There is no reference implementation of any part of this feature.**

Established rather than assumed, by searching `reference/Randomizer/`:

* `CharaInitParam` does not occur anywhere in the reference randomizer's own
  source. The only matches in the whole `reference/` tree are inside
  `reference/SoulsFormats/`, in the map-part definitions of four *other*
  FromSoftware games. The reference tool has no code path that edits character
  creation — the same finding spec 037 §3 recorded, re-checked here.
* `EquipParamProtector` likewise does not occur anywhere in the reference
  randomizer's own source. The reference never reads or writes the armour table.

The nearest reference behaviour is the shop shuffle, which this port already
ships as rows 5 and 11: it changes what the Bath Messengers and the Hunter's
Dream coffins *sell*. It never changes what a character starts with.

So there is no reference behaviour to match and no reference quirk to preserve.
`CLAUDE.md` §7's rule — match the reference before improving it — is satisfied
trivially: every part of this is an addition.

---

## 4. What do we know?

All measurements are against `data/vanilla/dvdroot_ps4`, taken with
`app/tools/param_offsets.py`'s paramdef and param readers and `app/tools/fmg.py`'s
message reader — not from any document. Every claim is labelled **fact**,
**inference** or **assumption**.

### 4.1 The character already arrives dressed, and the clothes come from the rows this feature writes

**Fact.** The character-creation param holds 1700 rows. The 22 player-origin rows
— the same 22 `START WITH HUNTER TOOLS`, `START WITH A TRICK WEAPON` and
`START WITH A LEFT WEAPON` already write, listed once in `CharaInitRows.h` — each
name four garments in four dedicated clothing fields, and they are the four the
player can see: the game's own English text calls them **Foreign Garb**,
**Sullied Bandage** and **Foreign Trousers**, plus a **Black Hood** on one of the
two ten-row blocks.

**Fact.** Those fields are exercised heavily across the whole param: 1557 rows
name a head piece, 1641 a body piece, 1615 gloves and 1632 legwear, drawn from 48,
51, 40 and 43 distinct values. This is the opposite situation from row 37, whose
weapon field was empty on all 1700 rows and had to be settled by a console probe.

**Inference — the game reads these fields, and this is as close to proof as
offline work gets.** The four garments the shipped rows name are the four a new
Bloodborne character is visibly wearing. If the engine ignored the fields the
character would arrive naked, which it does not. Reasoning: the field values and
the observed starting outfit are the same four items, and no other param in the
tree names a starting outfit. `docs/known-traps.md`'s standing warning — byte
decoding does not prove game behaviour — still applies to *what* a write does,
which is why §8 keeps a hardware check; it does not leave the existence of the
route in doubt the way row 37's did.

**Fact, and a small correction to `CharaInitRows.h`.** The two ten-row blocks,
2000–2009 and 3000–3009, differ in **two** fields, not the one the header names:
the head-piece field (a Black Hood on the 2000 block, empty on the 3000 block) and
the six secondary-item slots (`-1` on the 2000 block, `0` on the 3000 block). The
header's own statement is still true as written — it says the blocks are identical
"in every field either feature touches" — but a reader looking for the whole
difference will not find it there.

**Inference — the 3000 block is the live one, and a new character's bare head is
the evidence.** A vanilla character starts with nothing on its head, and the
3000 block is the one whose head field is empty. Reasoning: the two blocks are
otherwise interchangeable, so the observable difference picks the block. This
changes nothing about how the feature is built — all 22 rows are written, as the
three shipped features already do — but it is the cheapest answer yet to row 34's
open item O2, and it is checkable on hardware without building anything (§8).

**Fact.** The eight companion `*_GenId` fields that sit past the stat block are
`-1` on all 1700 rows, all four clothing slots included — 6800 values, no
exceptions. So the shipped data never pairs a clothing id with one, which is the
same ground on which `TrickWeaponGrant.cpp` deliberately leaves them alone.

### 4.2 What the armour table actually offers

**Fact.** The armour param holds **273 rows**. 21 of them are not armour at all:
they sit in a fifth slot category and the game's text names every one of them
`NOT NEEDED` — the placeholder for "this character wears nothing here". That
leaves **252 rows in the four wearable slots**: 76 head, 76 body, 48 gloves, 52
legwear.

**Fact.** Of those 252, **126 carry a player-facing name** in the game's own
English text; the other **126 have no string at all**, or the placeholder `*`, or
an internal label of the form `en5304`. Two of the 126 named rows are then
excluded on the game's own say-so: the **Crown of Illusions** and the **Enlarged
Head** are the only two rows in the table the game marks as wearable by neither
sex, and both are cut content. That leaves **124 pieces** — 33 head, 34 body, 26
gloves, 31 legwear — every one of them marked wearable by both sexes.

**Fact — the obtainability test agrees, and that is what makes 124 a finding
rather than a name filter.** Independently of naming, **120 of the 124 are
obtainable in vanilla** — sold in a shop listing or present in a world pickup.
The four that are not are exactly **Black Hood, Foreign Garb, Sullied Bandage and
Foreign Trousers**: the clothes the character is already wearing, which is why
they need no shop and no pickup. In the other direction, **not one of the 126
unnamed rows** is in any shop or any pickup anywhere.

*Corrected 2026-09-29 during stage C, and the correction matters because §8 had
been written to the wrong version.* The **two cut rows are not like the unnamed
ones**: Crown of Illusions (250000) and Enlarged Head (430000) are **named and
both obtainable** — verified, each present in an `ItemLotParam` slot with
`lotItemCategory 1`. They are excluded on the **sex test alone**
(`equipModelGender == 0`). So the two tests do not agree in either direction, and
the honest statement of the relationship is a two-sided equality with two named
exceptions each way:

* `included − obtainable == {230000, 231000, 232000, 233000}` — the starting clothes
* `obtainable − included == {250000, 430000}` — the two cut rows

**The 124 do not change.** What changes is that a generator must not treat
agreement as a gate.

**Fact — 36 outfits, 25 of them complete.** An armour id decomposes exactly into
an outfit number and a slot, and the slot it decodes to equals the row's own slot
field on all 252 rows without a single mismatch. Grouping the 124 that way gives
**36 outfits**: 25 with all four pieces, 5 with three, 3 with two, 3 with one. The
gaps are in the game's data, not introduced here — ten outfits have no gloves,
five no legwear, three no head piece, two no body piece. Eleven of the 36 are
available in the shops in their entirety; the rest are found in the world.

**Fact.** The armour param has **no** strength, skill, bloodtinge or arcane
requirement fields — the four `proper*` requirement fields that exist on weapons
and on some consumables are simply absent from the armour row layout. Armour also
carries a weight value, but Bloodborne imposes no equip-load limit on it.

**Inference.** Nothing in §2's "every outfit is wearable at once" needs a
requirement rewrite, and the `WeaponRequirementWriter` this feature sits beside is
not involved at all. Reasoning: a requirement that does not exist as a field
cannot fail a check. This is the single largest difference in cost between this
feature and rows 37 and 38, whose requirement rewrite was most of their risk.

**Fact, restated 2026-09-29 during stage C — the original premise was false.** The
claim had been that exactly one named row carries a permanent effect and it is the
Enlarged Head. It is not: `residentSpEffectId2` is **uniformly 6300** across the
included pieces, and the one row carrying a value at `residentSpEffectId` within
the wearable, both-sexes set is not the Enlarged Head (which the sex test excludes
anyway). **The conclusion survives and is what the feature relies on**: the effect
fields are uniform across every piece a player can be granted, so no piece brings
a hidden buff or debuff another does not. A verifier written to the original
premise would have failed.

### 4.3 The names exist, and three of them cannot currently be drawn

**Fact.** Every armour name is in the game's own English text, in the same
container `app/tools/fmg.py` already reads for weapon names
(`msg/engus/item.msgbnd.dcx`), in a sibling member. So the picker's table can be
generated and pinned exactly the way `gen_weapon_table.py` generates the 78-row
trick-weapon table, and no name has to be hand-authored.

**Fact.** The longest of the 124 is `DECORATIVE OLD HUNTER TROUSERS` at 30
characters — comfortably inside the 71-character line the pool tables are pinned
to, even with an id column.

**Fact — a blocker the generator will hit on its first run.** Three of the 124
names contain a forward slash: `Knight's Trousers/Dress`,
`Black Church Trousers/Dress` and `White Church Trousers/Dress`. The slash is not
in the 8x8 fallback font's glyph table, so `gen_pool_table.py`'s renderability
check refuses it. Either the glyph is added or those three names are sanitised;
the font draws an unsupported character as blank rather than failing, so doing
neither would ship three rows with a hole in them (`docs/known-traps.md`, "8x8
font limitations").

**Fact — the game has no name for an outfit, only for its pieces.** There is no
set-name text anywhere in the message containers and no set-name field in the
armour row; the only grouping is the id arithmetic of §4.2. So a picker whose rows
are *outfits* must invent each row's label — most naturally from the body piece,
which is what makes the Charred Hunter outfit's head piece read as "Gray Wolf
Cap" and the Executioner outfit's as "Gold Ardeo". A picker whose rows are
*pieces* takes every label straight from the game. This was a real cost on the
option this spec recommended, and **it is one of the reasons §10 D1 went the other
way**: per-slot pickers are piece pickers, so no outfit label is invented and this
whole difficulty does not arise.

### 4.4 The pack, and the hard bound on how much can go in it

**Fact.** Each origin row has **ten inventory slots**. Vanilla uses one — a single
Hunter's Mark — and leaves nine empty. `START WITH HUNTER TOOLS` fills two of the
nine when it is on. So the supplies this feature can grant are bounded at **nine
kinds of item, or seven with the hunter tools also on**, and the two Blood
Vial/Quicksilver Bullet entries §2 describes fit with five to spare.

**Fact.** The slot's count is a **single byte**, so 1–255 per slot is
representable. Bloodborne's own carry cap is 20 for both Blood Vials and
Quicksilver Bullets.

**Fact.** The shipped param routinely exceeds that cap on its own rows: 4505
slot-and-row combinations across the 1700 rows name a count above the item's carry
cap, including 30 Blood Vials on rows 15–19. Counts of 1, 2, 3, 5, 10, 20, 30, 50
and 99 all occur.

**Inference.** A count above 20 is not malformed and will not be ignored — the
game's own data does it — but what the engine does with the overflow (clamp, or
spill into storage) is not decidable from the data. Reasoning: the values exist in
shipped rows, so the field accepts them; nothing in the param says where the
excess lands. §2's 20 is the value that cannot be wrong either way, and §10 D2
records the choice to grant exactly that and add no count setting.

**Fact.** These same ten slots are the route the game's own data uses most
heavily for exactly the items this feature wants to grant: Blood Vials appear on
1617 rows, Quicksilver Bullets on 1498, Fire Paper on 1190, Molotov Cocktails on
1073. The supplies half of this feature is therefore the *best*-evidenced part of
it — better evidenced than the outfit half, and far better than row 37's weapon
field was.

**Fact.** Six further secondary-item slots exist per row and are used on 1376 of
the 1700 rows, but are empty on all 22 origin rows. They are not needed and are
not used here.

### 4.5 Which table each slot resolves against, and why that matters

**Fact.** The inventory slots resolve their ids against the goods param, which
holds **295 rows with ids from 1 to 12140**. **251 carry a player-facing name.**
**178 of the 251 are obtainable in vanilla** — in a shop or a world pickup.

**Fact.** That table is a mixed bag, not a consumables list. Alongside Blood
Vials, bullets, throwables and papers it holds gestures, insight items, Coldblood
echoes, upgrade materials, chalices, every progression key and badge, and the two
workshop tools row 34 grants. Filtering to *named, obtainable, stackable and
carrying an effect* gives **40 rows** — a usable definition of "consumable" — but
**Quicksilver Bullets is not one of them**, because ammunition carries no effect
of its own. Any rule has to name the bullets explicitly.

**Fact — the namespace trap, already paid for once on this project.** Caryll runes
and blood gems are not in the goods table; they are a separate category of id, and
the first rune probe wrote one of those ids into these slots, reached the shipped
tree on all 22 origin rows, and was ignored completely. A well-formed write into
the wrong table looks exactly like a field the game does not read
(`RuneProbe.h`). The armour fields of §4.2 resolve against the armour param and
the inventory slots against the goods param; neither accepts the other's ids.

### 4.6 The Dream's five coffins, as the game ships them and as row 5 leaves them

**Fact.** The Hunter's Dream offers five weapons across two coffin choices, and
they are five rows of the shop table, not part of the character template: **Saw
Cleaver, Hunter Axe and Threaded Cane** in the trick-weapon coffin, **Hunter
Pistol and Hunter Blunderbuss** in the firearm coffin.

**Fact.** `RANDOMIZE STARTING WEAPONS` overwrites the three trick-weapon rows and
`RANDOMIZE STARTING GUNS` the two firearm rows, drawing without repetition from
every distinct weapon sold anywhere in the game — a pool of **76 trick weapons and
14 firearms** as the port measures it today, and it then lowers the drawn weapon's
requirements so a level-4 character can use it.

**Fact.** Those two pools are not the same sets as the shipped pickers. The
`START WITH A TRICK WEAPON` picker is **78** rows and the row-5 melee pool is 76;
the difference is the base Beast Claw and the Uncanny Saw Spear, both sold in
shops and both missing from the reference tool's hand list the port transcribed.
The firearm figures agree at 14.

**Inference.** A setting that *fixes* the coffins rather than randomizing them
would be writing the same five shop rows two shipped toggles already write, which
makes precedence a rule that has to be stated rather than inherited — the same
situation spec 037 D4 settled for the requirement bytes. It would also have to
choose between the 78/14 picker tables and row 5's 76/14 pools, or accept that
two settings in the same category offer visibly different lists of the same
weapons. Reasoning: both act on the same five rows of the same table, and the
existing tables already disagree by two rows. §10 D3 settles it: out of scope here, filed as its own row.

### 4.7 What the shipped UI and config cost

**Fact.** The picker component is already generic over lists of 82, 85, 17, 78, 22,
14 and 14 rows. **§10 D1's four per-slot lists are 33, 34, 26 and 31 rows** — every
one of them smaller than five of the seven already shipped, and none of them
reaching three pages of 12 visible rows. The 124-row single list that would have
been the largest yet is not built.

**Fact.** The settings block is 1038 bytes today inside a 2048-byte buffer, and a
whole worst-case `defaults.cfg` is 1089 bytes — both pinned by exact-equality
cases in `pool_verify.py`. **Under §10 D1 and D2** the four per-slot selection
lines cost **216 bytes** and the supplies toggle **31**, taking the block to
**1285** and the file to **1336**, leaving **712 bytes** of the buffer unused.
(Measured by the orchestrating session against the post-row-13 figures rather than
carried from the draft, because the baseline moved three times during this
feature's specification.) The exact arithmetic and the pinned cases are stage C's
to update, and — given that rows 11, 13 and 27 each rebased the previous feature's
figures — stage C should expect to re-measure rather than trust this number.

**Fact.** The `WEAPONS AND GEAR` category holds eight settings today. §10 D1 and
D2 make it **thirteen** — four armour pickers and one supplies toggle. Measured by
the orchestrating session, all five labels fit: the widest is
`START WITH BODY ARMOUR` at **609 px** of the 700-pixel budget with a `12 OF 34`
value, which is under the app's current worst row (`BOSS BEHAVIOUR`, 663 px), so
this feature does not become the pane's constraint. The
details pane holds each row's label, a gap and its widest value inside a 700-pixel
budget, which `settings_ui_verify.py` case 5 asserts, and it is what forced
`START WITH A LEFT WEAPON` to drop the word "hand" — so a setting's name here is
constrained by pixels, not by characters.

**Fact.** A picker-only setting is not a toggle, so it does not appear in the
readiness summary's toggle count and does not appear in the commit gate's list of
toggles that are on. Plan 037 D2 decided deliberately to leave that alone; the
same applies to an outfit picker, and §7 records it.

---

## 5. Terminology

**Outfit.** The four pieces the game groups together by id — a head piece, a body
piece, gloves and legwear. What a player calls a "set". Bloodborne's own text has
no name for one (§4.3), only for each piece. 36 of them exist (§4.2), and 11 are
incomplete in the game itself.

**Piece.** One wearable item in one slot. 124 of them exist across the 36
outfits.

**Slot.** Head, body, gloves, legwear — the four places a piece can go. Unrelated
to an **inventory slot**, of which each origin row has ten (§4.4).

**Supplies.** The consumables a character starts carrying. Here, Blood Vials and
Quicksilver Bullets, and §10 D2 settles that those two are all this feature grants.

**Origin.** The character-creation background the player chooses — Milquetoast,
Waste of Skin, and the rest. Each is a separate template row, which is why every
feature here writes 22 of them.

**Grant.** Put the item into a new character's possession. Distinct from
**equip**, which is to have it worn or in hand at spawn. For armour the two
coincide: the shipped rows already dress the character, so a granted outfit is a
worn outfit.

**Coffin pick.** One of the five weapons the Hunter's Dream offers. A shop
listing, not part of the character template (§4.6).

**Pool picker.** The shipped drill-in list pattern — `ENEMIES INCLUDED`,
`START WITH A TRICK WEAPON`, and the rest. Ticked rows are the candidates; none,
one and many are all meaningful.

---

## 6. Scope

Covers backlog row **40** only, "Starting gear selection", in
`docs/randomization-feature-spec.md` §4.

### In scope

Under the decisions recorded in §10:

* **Four armour settings, one per slot** — `START WITH HEADWEAR` (33 rows),
  `START WITH BODY ARMOUR` (34), `START WITH GLOVES` (26), `START WITH LEGWEAR`
  (31) — each with pool-picker none/one/many semantics, each drawn independently,
  the four lists together covering all 124 wearable pieces of §4.2, generated from
  the vanilla tree and pinned like the existing tables (D1)
* **One supplies toggle**, granting 20 Blood Vials and 20 Quicksilver Bullets into
  free inventory slots of the same origin rows (D2)
* Leaving a slot's starting garment in place whenever that slot's picker has
  nothing ticked (§2)
* The glyph or sanitisation decision the three slashed names force (§4.3)
* **Offering every wearable piece, with no curation** (D4) — which under D1's
  per-slot shape is satisfied by construction, since 33 + 34 + 26 + 31 = 124

### Out of scope

* **Fixing the Hunter's Dream coffin picks** — **D3**, and to be filed as its own
  backlog row. It writes a different param, collides with two shipped toggles, and
  inherits a pool-definition disagreement this feature has no reason to resolve
  (§4.6). Keeping it out is what leaves this row additive throughout
* **Matched-outfit grouping in the UI.** D1 chose per-slot pickers, so the game's
  36 outfits are not a concept the settings express. A player assembles a set by
  ticking four pieces. Offering both shapes at once was not on the table and is not
  contemplated here
* **Runes and other accessories.** Row 39 is BLOCKED and the accessory probe has
  **not** answered. Nothing here assumes an accessory slot is read, and nothing
  here depends on row 39. If row 39 ever unblocks, a rune setting is its own row;
  should it instead turn out that runes can be granted as ordinary inventory items,
  that would be a route into row 39 and not a change to this one
* **Blood gems.** Same reason, same separate category of id (§4.5)
* **Starting Insight, echoes, stats, level or origin.** None of them is this row
* **Upgrade materials, chalices, key items and badges**, all of which sit in the
  same table as the supplies and none of which is a starting supply. Granting a
  progression key at character creation is a sequence-breaking change and is not
  contemplated here
* **Changing what the shops sell.** Rows 5 and 11, both shipped
* **Narrowing the 22 origin rows to the live block.** §4.1 finds evidence for
  which block is live, but that is row 34's open item O2 and should be resolved
  once for all four features that write these rows, not inside this one
* **Chalice dungeons**, per `CLAUDE.md` §7

---

## 7. Constraints and decisions

All four decisions are recorded in §10. The constraints they sit inside:

* **Default is nothing granted.** A `defaults.cfg` written before these settings
  existed, or carrying a stale-length selection line, must read as *nothing
  ticked and the toggle off*, so an existing config produces the run it produced
  before, roll for roll. Same fail-safe polarity as `START WITH A TRICK WEAPON`.
* **Each of the four lists' order is load-bearing, independently.** A selection is
  stored positionally, one character per row, so regenerating any table in a
  different order silently remaps a saved selection onto a different piece. Four
  frozen orders, four counts, and a changed count in any one of them must be
  rejected rather than misapplied — four times the surface the rejected
  single-picker option would have had (§10 D1).
* **Names come from the game, not from a hand-written table.** The generated-and-
  pinned pattern of `gen_weapon_table.py` — and under §10 D1 with **no exception**,
  because every row of all four pickers is a piece the game names. The
  invented-outfit-label problem §4.3 describes belongs to the rejected option.
* **Only renderable names ship.** The generator must fail loudly on the three
  slashed names rather than letting the font draw a blank (§4.3).
* **The four shipped features writing these rows must not change.**
  `START WITH HUNTER TOOLS`, `START WITH A TRICK WEAPON`,
  `START WITH A LEFT WEAPON` and the temporary rune probe — which is **still in the
  build and still unanswered** — all write the same 22 rows of the same param
  member. D1's four armour writes and D2's inventory writes join them, taking the
  count of features writing these rows to six. Every combination must be able to be on at once
  with no write lost, whichever order the passes run in — which is what
  `CharaInitRows.h`'s slot search already exists to guarantee for the inventory
  half.
* **Rows 5 and 11 must not change.** Neither the coffin rows nor the shop stock is
  touched at all, §10 D3 having put the coffins out of scope — so no precedence
  rule against row 5 is needed, and this feature stays additive throughout.
* **The starting Hunter's Mark survives.** It is the one inventory slot vanilla
  uses, and nothing may displace it.
* **Determinism, and D1 makes it four draws rather than one.** Each slot with
  something ticked consumes randomness. Turning any of the four on must not
  reshuffle the rest of a run's world for a given seed, or two runs on the same
  seed stop being comparable — which is why the weapon grants are the last RNG
  consumers of a run and these draws belong in the same place. **The four draws
  must also be taken in a fixed slot order**, so that ticking headwear does not
  move which gloves a seed produces.
* **A picker-only setting must still gate a run.** Ticking one piece in one slot
  and nothing else is a complete request and must build a tree, the way plan 037 D2
  settled for the readiness summary.
* **No id may be written into the wrong table's namespace** (§4.5). An armour id
  goes only in a clothing field and a goods id only in an inventory slot.
* **The archive is edited in place.** Every write is one value over another inside
  an existing row; nothing is resized and the archive is never rebuilt.

---

## 8. How will we know it works?

### Automated testing

What offline verification must establish is that the edit is the intended edit and
nothing else changed:

* **Each of the four tables is exactly the named, sex-wearable pieces of its own
  slot**, re-derived from the data rather than compared against a copy of itself,
  and each committed table matches what the generator produces now. **Their four
  row counts sum to 124**, which is the assertion that says no wearable piece was
  dropped between them and none counted twice.
* **The two-sided obtainability equality of §4.2 holds exactly**:
  `included − obtainable == {230000, 231000, 232000, 233000}` and
  `obtainable − included == {250000, 430000}`. Asserted as an equality with both
  exception sets named, **not** as "the two tests agree" — which they do not, and
  which as an acceptance criterion was unsatisfiable, so a generator built on it
  would have refused to write. Corrected 2026-09-29 during stage C.
* Every name in the table is the name the game's own text gives that exact row,
  and every one is drawable by the shipped font.
* With nothing ticked and the toggle off, the character-creation param is
  byte-identical to vanilla and no row changes.
* With one piece ticked in one slot, every player-origin row names that piece in
  that slot, **the other three slots keep their vanilla values**, and no other
  field of those rows changes — in particular the starting Hunter's Mark survives.
* With several ticked in a slot, the piece granted is one of the ticked ones, the
  same seed gives the same piece, and a different seed can give a different one.
* **With pieces ticked in more than one slot, each slot is satisfied from its own
  list** — no head piece lands in a glove slot — and the four writes are
  independent: changing what is ticked in one slot must not change which piece a
  fixed seed draws in another.
* With the supplies toggle on, both items are present exactly once per origin row
  at the intended count, in slots that were empty, and the pass is safely
  re-runnable over an already-edited buffer.
* With the supplies toggle and `START WITH HUNTER TOOLS` both on, all four entries
  are present in every origin row in either pass order, and no row runs out of
  slots.
* No non-origin row is touched, and no other member of the archive is touched.
* No id is written outside its own table's row range.
* Each config selection line round-trips at its exact length, a wrong-length line
  is rejected and leaves nothing ticked, and the config file still fits its
  buffer.

The existing tools reach all of this data: `param_offsets.py` resolves the
fields, `fmg.py` reads the names, `hunter_tools_verify.py` is the closest existing
shape for the inventory half and `trick_weapons_verify.py` for the picker half.

### Hardware testing

**A free observation first, before anything is built.** Start a vanilla new
character and look at its head. A bare head confirms §4.1's inference that the
3000 block is the live one and that the head field is read as "wear nothing";
a Black Hood says the 2000 block is live. Either answer is consistent with this
feature, and both are worth more than the same observation costs, because it is
also row 34's open item O2.

Then, with one complete outfit ticked and the supplies toggle on, build, install
and **start a new character**:

| Observed | What it means |
|---|---|
| Wearing all four pieces of the ticked outfit, 20 vials and 20 bullets in the pouch | Both settings work as §2 describes |
| Dressed but the pouch is empty, or the reverse | One half works and the other does not; the two halves write different fields and can fail independently |
| Neither | The origin rows are not being reached at all, which would contradict three shipped and hardware-tested features and points at the build rather than the feature |

Once that is positive, the remaining checks:

* The granted pieces can be **unequipped and re-equipped** from the menu, and the
  four vanilla starting garments are still owned. A worn piece the character does
  not own would be a different outcome from the one §2 describes.
* An incomplete outfit leaves the starting garment in the slots it does not cover,
  visible on the character — the check that the grant is per-slot and not
  wholesale.
* Every origin gets it, **including Waste of Skin at level 4**, with no
  "requirements not met" and no weight penalty. §4.2 predicts there can be none;
  this is where that prediction is tested.
* The heaviest outfit in the game is as wearable as the lightest.
* With several ticked, two runs on the same seed grant the same outfit and the
  rest of the world is identical; different seeds vary.
* With nothing ticked and the toggle off, the clinic spawn is exactly vanilla.
* With `START WITH A TRICK WEAPON`, `START WITH A LEFT WEAPON` and
  `START WITH HUNTER TOOLS` also on, all five grants arrive together and none has
  displaced another.
* An existing save loaded with both settings on gains nothing and is undamaged.

**The most important failure cases**, in the order they are likely: a piece
granted into the wrong slot, which looks like a partial success and is an id-
decomposition fault; the supplies overwriting the Hunter's Mark or a hunter tool
instead of taking a free slot; a count above the carry cap silently discarding the
whole entry rather than clamping it (§4.4); and three picker rows with a blank
where a slash should be (§4.3).

---

## 10. Decisions

| Date | Decision |
| ---------- | -------- |
| 2026-09-29 | **D1 (was Q1) — armour ships as FOUR per-slot pickers, not one outfit picker.** *Taken against this spec's recommendation.* `START WITH HEADWEAR` (33 rows), `START WITH BODY ARMOUR` (34), `START WITH GLOVES` (26), `START WITH LEGWEAR` (31) — **33 + 34 + 26 + 31 = 124**, exactly the wearable set, so nothing is hidden. The developer chose mix-and-match loadouts over a single set choice: a player can ask for the Crowfeather hood over Gehrman's coat, which no outfit picker can express. **The cost accepted:** four tables, four config keys, four UI rows, four draws and four frozen orders instead of one of each — 216 bytes of config against 62, and `WEAPONS AND GEAR` going from eight settings to thirteen. **The cost avoided, which this spec had understated:** no outfit label has to be invented, because the game names every piece and names no outfit (§4.3), so every row of all four pickers comes straight from the game's own text. Measured by the orchestrating session: all five new labels fit, the widest being `START WITH BODY ARMOUR` at **609 px** of the 700-pixel budget, under the app's current worst row at 663 px. |
| 2026-09-29 | **D2 (was Q2) — supplies are one toggle granting 20 Blood Vials and 20 Quicksilver Bullets.** Taken as recommended. No count setting and no consumables picker. 20 is the value that is correct whether the engine clamps an over-cap count or spills it into storage — which §4.4 establishes cannot be decided offline, the shipped param itself exceeding the cap 4,505 times. These are also exactly the two items row 11's 27-entry skip list protects as the ones a playthrough cannot do without, and the two most common entries in the game's own template rows (Blood Vials on 1617 rows, bullets on 1498). Two writes, no draw, no table, no failure mode. |
| 2026-09-29 | **D3 (was Q3) — fixing the Hunter's Dream coffin picks is OUT of this row**, to be filed as its own backlog row. Taken as recommended. It writes a different param — five `ShopLineupParam` rows, not the character template — it collides with two shipped, hardware-proven toggles that already overwrite exactly those five rows, and it inherits a real pool-definition disagreement (row 5 measures 76 melee / 14 firearms against the shipped picker's 78 / 14, the gap being the base Beast Claw and the Uncanny Saw Spear). Rows 37 and 38 have already taken most of its value by letting a player start holding any weapon. **Keeping it out is what leaves this feature additive throughout**, and therefore what keeps §8 short. |
| 2026-09-29 | **D4 (was Q4) — offer every wearable piece; do not curate.** Taken as recommended, and **under D1 it is satisfied by construction**: the question as asked was whether a 36-row *outfit* picker should include the 11 outfits the game ships incomplete, and per-slot pickers have no notion of an outfit, so every one of the 124 pieces is independently offerable and the question dissolves. The developer's intent — show the player everything real rather than curating on their behalf, as this project's pickers have consistently done — is recorded because it is the standing principle, not because it requires an instruction here. |

| Date       | Decision |
| ---------- | -------- |
| | |

---

## Appendix — research notes for stage C

*Not part of the specification, and not covered by the developer's approval. These are pointers from the spec investigation to save stage C a search. Verify anything here before relying on it.*

**Where the behaviour lives**

* `app/src/Randomizer/CharaInitRows.h` — `kOriginRows`, `kRowBytes`,
  `FirstEmptySlot`, `RowHasItem`. The inventory half needs nothing new from it;
  the clothing fields are four more offsets in the same row and belong beside the
  ones already there.
* `app/src/Randomizer/HunterTools.cpp` — the closest sibling for the supplies
  half: same rows, same slot search, same re-runnable-pass discipline, and it
  already sets the count byte as a single byte.
* `app/src/Randomizer/TrickWeaponGrant.cpp` — the closest sibling for the picker
  half: one draw, last RNG consumer of the run, and the per-row bounds check.
* `app/src/Randomizer/EnemyRandomizer.cpp` — `StepItemData`. Where the param
  member is located and where the three existing character-creation passes sit
  relative to each other.
* `app/src/Randomizer/ModelPoolSelection.h` — the `DefaultSelected` template
  parameter is the fail-safe polarity these settings need.
* `app/src/Randomizer/ShopLineup.h` — read the header before touching the shop
  table at all, and note the warning that a bucket comes from the listing's own
  type byte and never from the shape of an id. Relevant only under Q3's option B.
* `app/tools/gen_weapon_table.py` — the generator pattern, its `--check` mode,
  its frozen-order warning and its two-independent-tests assertion. The model for
  an armour table. **But see the defect below before copying its lot test.**
* `app/tools/fmg.py` — already reads the container the armour and goods names are
  in; it needs one more member name each, spelled as an escape so the file stays
  ASCII.
* `app/tools/param_offsets.py` — `load_defs`, `field_offsets`, `load_param`,
  `param_rows`. Everything in §4 was measured with these four. Do not write a
  second param reader.

**Worth checking early**

* **`gen_weapon_table.py` has its item-lot category wrong, and an armour
  generator must not copy it.** It uses `lotItemCategory == 1` as "weapon". On
  the real data category **1 is armour** (98 protector ids against 3 weapon ids)
  and category **0 is weapon** (63 weapon ids against 3 protector ids, all three
  on the dummy lot row 0). Row 037's 78-row table is unaffected — all 78 are in a
  shop listing, so the lot half of its obtainability test contributed nothing —
  but the generator's header claims two independent tests where it currently has
  one and a half. Worth fixing while the same code is open.
* An armour id decomposes as outfit × 10000 + slot × 1000. Verified against the
  row's own slot field on all 252 wearable rows with no mismatch, so the
  decomposition can be asserted rather than assumed.
* The 21 fifth-category rows the game names `NOT NEEDED` are the "wears nothing
  here" placeholders. Filter on the slot category, not on the name.
* `gen_pool_table.py`'s `RENDERABLE` set has no `/`. Three armour names need it or
  need sanitising; decide which before the table is generated, not after.
* `pool_verify.py`'s config-size cases are exact equalities on purpose and will
  fail the moment a key is added. Update the arithmetic and the comment that
  narrates its history; §4.7 has the new numbers for the recommended scope.
* `settings_ui_verify.py` case 5 is a pixel budget, not a character budget. It is
  what forced `START WITH A LEFT WEAPON` to drop "hand"; measure any new label
  through it before settling on the wording.
* The console here is cp1252. Printing an armour or goods name, or a Japanese
  member name, without `PYTHONIOENCODING=utf-8` raises `UnicodeEncodeError` that
  reads like a parse failure and is not.
* **A conditional pointer for row 39, not for this row.** The goods table holds
  thirteen rows the game's own text names `Caryll Rune: "…"`, ids 1600–1612, each
  carrying a carry cap of one. They are in no shop and no pickup. Nothing here
  establishes what they are or whether granting one does anything, and row 39
  remains blocked on the accessory probe — but if that probe comes back negative,
  these rows are the next thing to look at, and they sit in the one namespace the
  inventory slots are already known to read. Do not act on this inside row 40.

**Dead ends already walked**

* `reference/` has no equivalent for any part of this. Neither `CharaInitParam`
  nor `EquipParamProtector` appears anywhere in the reference randomizer's own
  source. There is no behaviour to trace and no quirk to preserve.
* Looking for a **set name** in the game's text or a set field in the armour row:
  there is neither. The only grouping is the id arithmetic. Do not spend time
  searching; §4.3 is that search's result.
* Looking for **armour stat requirements**: the four `proper*` fields that exist
  on weapons and on twelve goods rows are simply absent from the armour row
  layout. `WeaponRequirements.h` and the shared requirement writer are not
  involved in this feature at all.
* Looking for armour in **item lots under category 2**: there is no category 2
  anywhere in the item-lot param. That search is what uncovered the category
  mislabel above.
* The **secondary-item slots** were checked as extra room for supplies. They are
  used on 1376 of 1700 rows but empty on all 22 origin rows, and the ten ordinary
  slots already leave seven spare in the worst case, so there is no reason to
  touch them.
* `equip_Arrow`, `equip_Bolt` and their sub-variants sit between the clothing
  fields and the accessory fields and look like the natural home for bullets.
  They are `-1` on all 22 origin rows and Bloodborne has no arrows or bolts;
  Quicksilver Bullets are an ordinary goods id and belong in an inventory slot.
