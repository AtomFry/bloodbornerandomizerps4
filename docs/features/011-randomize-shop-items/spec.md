# Feature 011 — Randomize Shop Items

**Status: APPROVED** — the developer read and approved this spec on 2026-09-28.
Human gate 0h is passed; it is ready for `/plan 11`. §9 is deleted; its four
questions were answered the same day and are recorded as D1–D4 in §10.

**Backlog row:** `docs/randomization-feature-spec.md` §4, row **11** ("Randomize Shop Items", `shopBool`). Row 5 ("Randomize Starting Weapons") is already shipped and covers the weapon half of the same reference function; this spec covers only the other two halves.

**Reference:** `reference/Randomizer/MainWindowComponents/RandomizeFunctions.cs:609-1315` — `RandomizeShopItems`, the armour branch at `:982-1001` and the consumable branch at `:1002-1021`. Gated at `StartFunctions.cs:1541`.

**Plan:** `docs/features/011-randomize-shop-items/plan.md` — added after the spec is approved

---

## 1. What this feature does

The shops in Bloodborne are fixed lists. The Messengers in the Hunter's Dream bath sell a known set of things for Blood Echoes, and a second known set for Insight, and both lists grow as you hand over badges and push the story on. Everything in them — which garb is for sale, what it costs, which currency buys it, whether you can buy it more than once — is the same in every playthrough.

This feature shuffles those lists. It takes every armour listing in the game's shops and deals the armour out again at random among them, and does the same, separately, for every consumable and material listing. **Blood Vials and Quicksilver Bullets are left exactly where they are** — the reference protects them by name everywhere they are sold, and so does this.

Nothing is added and nothing is removed. Each shuffle deals out exactly the items that were already on sale, the same number of times each, so every piece of armour and every consumable that a vanilla playthrough can buy is still buyable somewhere. What changes is *where*: which shop a thing is in, what it costs, whether it costs Blood Echoes or Insight, and whether you can buy it once or forever. A hunter's garb that cost sixty thousand echoes can turn up as a one- or two-Insight purchase; Blood Rock, normally an Insight item, can turn up priced in echoes.

It exists because the shops are the one part of the game's economy that the randomizer otherwise leaves completely intact. With it on, working out what the Messengers are selling this run becomes part of the run.

It is a straight port of an existing reference setting, not a new idea. It is the sibling of the shipped **RANDOMIZE SHOP WEAPONS**, which already does exactly this to the weapon listings.

---

## 2. What does the player experience?

| Setting | Off | On |
| ------- | --- | -- |
| **RANDOMIZE SHOP ARMOUR** | The shops sell 44 pieces of armour, listed 220 times between them: 100 listings priced in Blood Echoes from 500 to 60,000, and 120 listings priced in Insight from 1 to 5. Every listing is a once-only purchase. Each piece appears in exactly one of the two shops. | The same 44 pieces fill the same 220 listings, dealt out at random. Every piece is still sold, still five times over, still once-only. What moves is the shop, the currency and the price: most pieces end up sold in both shops, and a piece's price bears no relation to what it was. About 5 of the 220 listings happen to keep their vanilla armour. |
| **RANDOMIZE SHOP ITEMS** (consumables and materials) | The shops sell 60 consumables and materials across 424 listings — Blood Vials and Quicksilver Bullets, throwables and papers, Blood Stone Shards through Blood Rock, Ritual Blood, Tomb Molds, chalices and ritual materials. Prices run from 10 to 48,000 echoes, or 1 to 60 Insight. Most listings are unlimited; 40 are once-only. | Blood Vials keep all 13 of their listings and Quicksilver Bullets keep all 13 of theirs, unchanged, at their vanilla prices in every shop. One further listing, which in vanilla points at an item that does not exist, is also left alone. The remaining 57 items and 397 listings are dealt out at random. Every one of the 57 is still sold, the same number of times as before, but at a different price, sometimes in the other currency — and the purchase limit belongs to the listing, not the item, so things that were once-only (the Hunter Chief Emblem, the Resonant Bells, the root chalices) usually become unlimited, and about 36 normally unlimited things become once-only. About 9 of the 397 listings happen to keep their vanilla item. |

Both shuffles are decided once, when the run's files are generated, from the run's seed. A shop's contents never change again during the run.

The two shuffles never interfere with each other, and neither interferes with **RANDOMIZE SHOP WEAPONS**: armour listings, consumable listings and weapon listings are three separate sets of listings, and each shuffle only ever deals within its own set. With all three on, a shop's whole list is scrambled but a weapon slot still holds a weapon, an armour slot still holds armour, and a consumable slot still holds a consumable. The three choices in the coffins at the start of the game are weapon listings and are untouched by this feature.

These are **two** independent settings (D1), so a player may take the armour shuffle without the consumable one.

---

## 3. What does the existing randomizer do?

The reference's *Randomize shop items* checkbox sets `shopBool`, which — together with the starting-weapons checkbox — calls one function, `RandomizeShopItems` (`RandomizeFunctions.cs:609-1315`). That single function contains three independent features; `shopBool` owns two of them and row 5 owns the third.

It works in two passes over the shop lineup table.

**Pass one, `:848-875` — build the pools.** It walks every listing in row order. Any listing whose item id is in a three-entry skip list (`:635-638`: Blood Vial, Quicksilver Bullets, and one unnamed id) is ignored entirely. Every other listing contributes its item id to one of three pools according to the listing's equipment type: weapons, armour, consumables. The pools are lists of ids, not of distinct ids — an item sold in five places is in the pool five times.

**Pass two, `:877-1022` — deal them back out.** It walks the listings again in the same order, applying the same three-entry skip list, and for each listing takes a uniformly random id out of the matching pool and writes it into that listing, *removing* it from the pool as it goes (`:986-999` armour, `:1006-1019` consumables). Because pass one built each pool from precisely the listings pass two will write, the pools run out exactly as the last listing is filled: the result is a uniform random permutation of each type's existing stock. The armour and consumable branches are each gated on `shopBool` (`:984`, `:1004`); the weapon branch is gated on the starting-weapons flag instead (`:892`), which is why the reference checkbox called "Randomize shop items" does not touch weapons.

Three things about the reference are worth stating plainly, because they are the shape of the feature rather than incidental detail:

* **Only the item id is written.** Price, currency, stock limit, unlock flag and shop are all other fields of the listing and are never touched. The listing keeps its terms; only the goods move.
* **The pools are bucketed by equipment type and by nothing else.** Shop, currency and price tier are not consulted, so an Insight listing and an echo listing draw from the same pool.
* **The skip list is matched on the item id, not the listing.** That is what makes the Blood Vial and Quicksilver Bullet protection complete: all 13 listings of each are skipped as sources *and* as destinations, in every shop, so neither can be displaced and neither can be duplicated somewhere else.

**Bugs and quirks.** The armour and consumable branches are clean — no off-by-one in the pool index, no unbounded reroll loop, no dead write. That is unlike the weapon branch of the same function (`:896`, `:794`) and unlike the reference's item-drop code, both of which carry defects the port had to decide about. There is nothing here to decide about.

Two reference behaviours do look odd and are not defects:

* The third skipped id, `240`, has no name in the game's text and no row in the item table at all (§4 F9). Skipping it is load-bearing, not cosmetic: if it entered the pool, some real shop listing would end up selling an item that does not exist. The port already reproduces this skip list for the weapon feature (`app/src/Randomizer/StartingWeapons.cpp:22`).
* A listing can be dealt the item it already had. There is no reroll against that, and with heavily duplicated stock it is expected to happen a handful of times (§4 F7).

`docs/windows-randomizer-technical-review.md` §5.2 covers this function and is **still accurate but too coarse to plan from**: it describes the bucketing and the draw-without-replacement correctly, and correctly notes that the whole archive is rewritten as a unit, but it treats the function as one feature ("randomizes the Hunter's Dream shop's weapon/armor/consumable lineup") and so does not record that the three buckets are gated by two different checkboxes. `docs/plans/param-features.md` §2 already corrects that and is the better starting point.

---

## 4. What do we know?

All measurements are against `data/vanilla/dvdroot_ps4` using `app/tools/param_offsets.py` (field offsets and rows) and `app/tools/fmg.py` (the names the game itself displays, from `msg/engus/item.msgbnd.dcx`).

**F1 — the shop table's size and its three parts. Fact, measured.** The shop lineup table has **1,288 listings**: **644** weapon, **220** armour, **424** consumable. Nothing else — there is no listing of any other equipment type. The three counts sum to 1,288, so the three sets are a partition, which is the basis of the disjointness claim in §2 and in F11.

**F2 — what the armour listings are. Fact, measured, named from the game's own armour text.** 44 distinct pieces, each listed exactly 5 times. They are complete or partial sets: Bone Ash, Henryk's, Yharnam Hunter, Cainhurst, Gascoigne's, Tomb Prospector, Crowfeather, Gehrman's Hunter, Madman, Ashen Hunter, Maria Hunter, plus the Gray Wolf Cap, Gold Ardeo and Beak Mask. 100 listings are priced in Blood Echoes (500–60,000), 120 in Insight (1–5). **Every one of the 220 listings is a single purchase.** No piece is listed in both shops.

**F3 — what the consumable listings are. Fact, measured, named from the game's own item text.** 60 distinct items across 424 listings. In groups: the two the economy runs on (Blood Vial ×13, Quicksilver Bullets ×13); status cures (Antidote, Sedative); throwables and buffs (Molotov and Rope Molotov and their Delayed variants, Poison Knife, Oil Urn, Pebble, Throwing Knife, Pungent Blood Cocktail, Fire Paper, Bolt Paper, Bone Marrow Ash, Numbing Mist, Beast Blood Pellet, Blue Elixir, Lead Elixir, Shaman Bone Blade, Hand Lantern); utility (Bold Hunter's Mark, Shining Coins, the two Resonant Bells); upgrade materials (Blood Stone Shard, Twin Blood Stone Shards, Blood Stone Chunk, Blood Rock); the Hunter Chief Emblem; and **24 chalice-dungeon items** — Short Ritual Root Chalice, five Sinister root chalices, Ritual Blood (1)–(5), Tomb Mold (1)–(5), the three Coldblood Flower items, Pearl Slug, Red Jelly, Sage's Wrist, Sage's Hair, Inflicted Organ, Yellow Backbone, Bastard of Loran, Bloodshot Eyeball, Living String.

**F4 — the protection is real and complete. Fact, from the reference source and the data together.** The skip list at `RandomizeFunctions.cs:635-638` is item ids `1000`, `900` and `240`. Measured against the data, `1000` is Blood Vial and `900` is Quicksilver Bullets, and they hold **13 listings each** — one in each of the thirteen echo-priced shop blocks, at every price tier. Because the skip is matched on item id in both passes, those 26 listings are untouched and neither item can appear anywhere else. **This is the single most important finding for the backlog's cost estimate: the risk the estimate hides is already handled by the behaviour being ported, and needs no invention.**

**F5 — what the shuffle actually touches. Fact, derived from F1 and F4 by counting.** 220 armour listings and **397** of the 424 consumable listings — 424 less Blood Vial's 13, Quicksilver Bullets' 13 and the one `240` listing. **617 of the 1,288 listings**, leaving 671 alone: the 644 weapon listings, which belong to row 5, and the 27 skipped ones.

**F6 — nothing can be lost, globally. Fact, from the algorithm's structure (§3) plus F1.** Each pool is built from exactly the listings that will be written, and each draw removes what it takes, so the multiset of ids is preserved: after the shuffle each of the 44 armour pieces still occupies exactly 5 listings and each of the 57 shuffled consumables still occupies exactly as many as before. The feature cannot delete an item from the game's shops. What it can do is move an item into a shop or a price tier a given playthrough does not reach — see F10.

**F7 — how many listings keep their vanilla item. Fact, computed from the measured stock.** Under a uniform permutation of a multiset, the expected number of positions holding their original value is Σnᵢ²/N. For armour that is 44×5²/220 = **exactly 5.0**; for the shuffled consumables, **8.7**. So a correct run leaves roughly 5 of 220 armour listings and 9 of 397 consumable listings apparently unchanged. A verifier must not require every listing to change.

**F8 — purchase limits stay with the listing. Fact, measured.** All 220 armour listings are single purchases. Of the 397 shuffled consumable listings, **40 are single purchases and 357 unlimited**. The 40 are exactly the listings of the nine normally-once-only items: the two Resonant Bells, the Hunter Chief Emblem, the Short Ritual Root Chalice and the five Sinister root chalices. Since only the item id is written, after the shuffle each of those nine lands on an unlimited listing with probability 357/397 ≈ 90%, and about 36 of the 40 once-only listings end up holding a normally unlimited item. This is the most surprising player-visible consequence of the reference's design and it is stated in §2 for that reason.

**F9 — every shuffled id names a real item. Fact, measured.** All 44 armour ids have a name in the game's armour text and a row in the armour table. All 57 shuffled consumable ids have a name in the item text and a row in the goods table. The one exception in the whole consumable set is id `240`, which has *no* name (it is not even inside the name table's id ranges) and *no* goods row — and it is the third entry of the reference's skip list, so it never enters a pool. **The feature therefore cannot put a blank or broken entry into a shop.** For contrast, the three ids that do lack names sit in the weapon set (`90100000`, `90110000`, `90120000`, in a three-listing block of their own) and are row 5's business, not this feature's.

**F10 — the shops, and the one thing that could not be established. Fact, measured, with an assumption on top.** The 1,288 listings fall into 20 blocks of listing ids. Sixteen of them are four families of shop, each family repeated as several price tiers holding an identical item list at escalating prices:

| Family | Blocks | Listings each | Sells | Priced in |
| --- | --- | --- | --- | --- |
| A | 5 | 144 | weapons, armour, consumables | Blood Echoes, 10–60,000 |
| B | 5 | 51 | armour, consumables | Insight, 1–60 |
| C | 4 | 38 | weapons, consumables incl. the five Sinister root chalices | Blood Echoes, 2–20,000 |
| D | 4 | 38 | weapons, consumables | Blood Echoes, 20–27,000 |

Plus the six starting listings (the three coffin melee choices, the two firearm choices, and the `240` listing) and a three-listing block of unnamed weapons. Families A and B are **confidently** the Bath Messengers' two stocks — B's entire stock is priced in single-digit-to-60 Insight and includes unlimited Blood Rock, which is the Insight shop's signature. Families C and D are **not identified**; their item lists are close to A's consumables with much higher prices, which is consistent with a later-cycle or otherwise-gated stock, but nothing offline settles it. **Assumption, to verify on hardware:** every family is stock a player can actually see. If C and D are not reachable, then 112 of the 397 consumable listings are a sink, and the permutation — which is global, not per-shop — will move items into them and out of the reachable shops. **D4 includes them anyway** and leaves this to the console, because restricting the pool later is a one-line change and excluding 28% of the bucket on a suspicion is not.

**F11 — this feature cannot collide with RANDOMIZE SHOP WEAPONS, and needs no arbitration. Fact, from F1 and the port's code.** Both features write the same field of the same table (`app/src/Randomizer/StartingWeapons.cpp:18-19`: item id at byte 0 of a 32-byte listing, equipment type at byte 23). But the shipped feature filters on equipment type 0 in every loop it has — pool build and write alike — and this feature is defined on types 1 and 3, and F1 shows the three sets partition the table exactly. So no listing is a candidate for both, neither feature can draw an id the other could write, and running one after the other in either order gives the same result. **Feature 037's problem does not recur here**: spec 037 D4 needed an owner-ranked writer because two features genuinely wanted the same bytes of the same weapon row. Nothing here does. The one thing that *is* shared is the run's random number stream, so the order the passes run in has to be fixed for a seed to keep meaning the same thing — that is a plan concern, not a correctness one.

**F12 — the port already has the whole mechanism. Fact.** The archive is opened, edited and written once per run by a single item-data phase (`app/src/Randomizer/EnemyRandomizer.cpp:1030-1062`), which the shipped shop-weapon pass already hangs off. `app/tools/starting_weapons_verify.py` already parses this table out of both vanilla and output archives and already asserts that only the item id may move within it.

**F13 — that verifier will report false failures on a combined run. Fact, read from the code.** `starting_weapons_verify.py`'s shop loop iterates *every* listing, not only weapon listings, and for any listing whose id changed it asserts the new id exists as a weapon row (`SW-I4`) and counts it as shop-weapon churn. An armour or consumable id will fail that assertion. So the existing verifier is not merely incomplete for this feature — it is wrong in its presence, and must learn about the equipment-type field before a run with both features on can be checked at all.

**F14 — the estimate in the backlog row is right about the mechanism and wrong about the risk. Inference, from F4, F8 and F10.** "Same table, same field, same loop, two more type buckets" is accurate (F11, F12). But the row's *Low* reads as low-consequence as well as low-effort, and the consequences measured here are not small: purchase limits and currencies detach from items (F8), and roughly 29% of the consumable listings are in shops nobody has yet confirmed a player can reach (F10). None of it makes a run unplayable, which is F15, but it is more than "two more buckets".

**F15 — a run stays playable. Inference, with its reasoning.** The items whose loss would matter are Blood Vials and Quicksilver Bullets, and those are protected outright (F4). Of the rest: Bold Hunter's Mark duplicates a permanent starting item; the upgrade materials (Blood Stone Shard through Blood Rock) all drop and are found in the world, so the shop is a convenience; Antidote and Sedative are conveniences; the throwables are optional; the 24 chalice items are chalice content (§6). The only shop item that opens a door is the Hunter Chief Emblem, and the door it opens is the Cathedral Ward shortcut into Old Yharnam, which is reachable on foot through Central Yharnam regardless — and by F6 the Emblem is still on sale somewhere in any case. **To verify:** confirm on hardware that a run with this feature on can be completed, and in particular that the Messengers still sell vials and bullets at vanilla prices from the first visit.

---

## 5. Terminology

**Listing** — one row of the game's shop table: one item offered by one shop at one price, with its own currency, stock limit and unlock condition. The same item is usually several listings. This spec counts listings, not items, because that is what the shuffle permutes.

**Bucket** — the set of listings of one equipment type. This feature has two buckets (armour, consumables); the shipped weapon feature has the third.

**Family / price tier** — several blocks of listings with an identical item list at different prices appear to be the same shop at different stages of a playthrough. "Family" means the shop; "tier" means one priced copy of its list. See F10.

**Once-only listing** — a listing the game allows to be bought a single time. Distinct from an item being rare: after the shuffle the limit belongs to the listing, not the item (F8).

---

## 6. Scope

### In scope

* Backlog row 11 only.
* Shuffling the **armour** listings among themselves: 220 listings, 44 pieces (F2).
* Shuffling the **consumable and material** listings among themselves: 397 listings, 57 items, with Blood Vials, Quicksilver Bullets and the dangling `240` listing excluded exactly as the reference excludes them (F4, F9).
* **Two** new player settings (D1), wired the way every shipped setting is: defaults, world store, both settings screens, the run's options, the engine.
* Extending `app/tools/starting_weapons_verify.py` — which already owns this table — rather than adding a second verifier that parses it again, including the correction F13 requires.

### Out of scope

* **Weapon listings.** Backlog row 5, shipped as **RANDOMIZE STARTING WEAPONS**, **RANDOMIZE STARTING GUNS** and **RANDOMIZE SHOP WEAPONS**. This feature must not touch equipment type 0, and the coffin choices at the start of the game are weapon listings.
* **Prices, currencies, stock limits, unlock flags and which shop a listing belongs to.** The reference writes the item id and nothing else (§3), and the consequences of that are the feature, not a defect to correct.
* **Chalice dungeons**, per CLAUDE.md §7 and `docs/design-decisions.md`. Note precisely what that does and does not mean here: 24 of the shuffled consumables are chalice materials and chalices (F3), and they are shuffled, because they are stock of ordinary Hunter's Dream shops and excluding them would be a deviation from the reference for no stated reason. What stays out of scope is chalice-dungeon *content* — no chalice shop is added, no chalice behaviour is checked, and "a chalice ritual material moved shop" is not a defect of this feature.
* Everything else param-based still on the backlog — movesets, gems, decals, talk, VFX, AI sound.
* Any protected-item list beyond the reference's three ids. D3 keeps the once-only listings in the shuffle, so nothing is protected on that account either.

---

## 7. Constraints and decisions

1. **Match the reference** (CLAUDE.md §7). The algorithm is a uniform permutation of each bucket's existing stock, drawn without replacement, writing only the item id, with the reference's three-id skip list matched on item id. Every deviation from that is recorded as a decision in §10, not assumed.
2. **Preserve the reference's skip list as-is**, including `240`. It is not a quirk to tolerate — it keeps a nonexistent item out of the pool (F9). The port already reproduces the same list for the weapon feature; the two must not drift apart.
3. **Do not touch equipment type 0.** Row 5 owns it. The two features' correctness depends on that partition holding (F11).
4. **Only the item id may change, in either bucket.** Every other field of a listing, and every other member of the archive, must come out byte-identical.
5. **The item-data archive is edited in place and written once.** This feature is another pass inside the existing phase (F12); it must not decompress or rewrite the archive a second time.
6. **The pass order within that phase must be fixed and documented**, because all passes share one random stream and the order decides what each draws (F11).
7. **Bounded work only.** No reroll loop of any kind is needed here — the reference has none in these two branches — and none may be introduced. An unbounded loop hangs a console (deviation SW-5, `docs/plans/starting-weapons.md` §6).
8. **Settings naming follows the shipped rows.** The neighbour is `RANDOMIZE SHOP WEAPONS`, "Randomizes the weapons sold by the Bath Messengers." D1 settles that there are two rows and that both sit in WEAPONS AND STARTING GEAR beside `RANDOMIZE SHOP WEAPONS`; their exact label and help wording is stage C's to write. Per the standing note on setting naming, nothing about a label or a polarity is confirmed until it has been seen on hardware.
9. **Save safety.** This changes shop contents only. It has no effect on save data and does not depend on the per-world save policy.
10. **No seed parity with the Windows tool** is claimed or attempted; the port's own seed reproducibility is what matters (`docs/windows-randomizer-technical-review.md` §7).

---

## 8. How will we know it works?

### Automated testing

Extending `app/tools/starting_weapons_verify.py`, which already reads this table out of vanilla and output archives (F12), and which must in any case be corrected per F13 before a combined run can be judged.

What must be shown true of a generated output archive:

1. **Nothing else changed.** No member of the archive other than the shop table differs, unless another enabled feature owns it. The archive's member list and every member's size are unchanged.
2. **Only item ids moved.** The set of listing ids is unchanged, and within every listing every byte except the item id is unchanged — price, currency, stock limit, unlock flag and equipment type included.
3. **Each bucket is a permutation of its own vanilla stock.** The multiset of armour ids across the 220 armour listings is identical to vanilla's, and likewise for the 397 shuffled consumable listings (F6).
4. **Type never crosses.** No armour listing holds a consumable or weapon id, and no consumable listing holds an armour or weapon id.
5. **The protection held.** All 13 Blood Vial listings and all 13 Quicksilver Bullet listings hold their vanilla item at their vanilla price, and the `240` listing is untouched (F4). No listing anywhere holds one of those three ids as a new value.
6. **Every assigned id is a real item** — present in the armour table or the goods table as appropriate (F9).
7. **Off means off.** With the setting or settings off, the shop table is byte-identical to vanilla.
8. **Independence from row 5.** With this feature and **RANDOMIZE SHOP WEAPONS** both on, criteria 2–6 hold *and* row 5's own criteria hold, with no listing claimed by both (F11). Achieving this is what F13's correction is for.
9. **The shuffle is a shuffle.** It is not required that every listing change — about 5 armour and 9 consumable listings are expected not to (F7) — but the overall number of changed listings must be consistent with a uniform permutation rather than with a partial or aborted pass.
10. **The same seed gives the same shops**, and different seeds give different ones.

### Hardware testing

11. **The vials are there.** On the first visit to the Hunter's Dream, the Messengers still sell Blood Vials and Quicksilver Bullets, at vanilla prices, in unlimited quantity. This is the criterion that decides whether the run is playable, and it is the first thing to look at.
12. **The lists are scrambled and coherent.** Both Messenger stocks show armour and consumables that are not vanilla, every entry shows a real name and icon, and nothing shows as blank or unpurchasable-for-no-reason. Buying works and delivers the item shown.
13. **The currencies crossed.** Armour or items that are normally Blood Echo purchases appear in the Insight stock and vice versa, at the listing's own price — confirming §2's central claim rather than assuming it.
14. **Which shops exist.** Record what the player can actually reach, to settle F10's assumption, which D4 deliberately left to the console: the two Bath Messenger stocks, and whether anything corresponding to families C or D is ever visible.
15. **A run completes.** The most important failure cases, in order: vials or bullets missing or mispriced; a shop entry that cannot be bought or crashes the menu; the game failing to load the item archive at all; and progression blocked by a shop item that turned out to matter more than F15 expects.
16. **Off is off.** With the settings off, the shops are vanilla.

---

## 10. Decisions

All four questions were put to the developer on 2026-09-28 and answered the same
day, each taking the spec's own recommendation. They are part of the agreed
behaviour of the feature.

| Date | Decision |
| ---- | -------- |
| 2026-09-28 | **D1 — Two settings, not one, and both in WEAPONS AND STARTING GEAR.** `RANDOMIZE SHOP ARMOUR` and `RANDOMIZE SHOP ITEMS` are independent toggles beside the shipped `RANDOMIZE SHOP WEAPONS`. This splits the reference's single `shopBool`, exactly as row 5 split its single checkbox into three positive toggles. The buckets differ in risk — armour is 220 cosmetic listings whose worst case is a price change, consumables are 397 holding the upgrade materials and every purchase-limit flip — and coupling them would make the harmless change unavailable without the risky one. All three shop rows sit in one category so they read together |
| 2026-09-28 | **D2 — The shuffle crosses shops, matching the reference.** The pool is keyed by `equipType` alone; shop, currency and price tier are not consulted. The consequence is accepted deliberately: most of the 44 armour pieces end up on sale in both the Blood Echo and Insight shops at unrelated prices, and Blood Rock can become an echo purchase. The shipped `RANDOMIZE SHOP WEAPONS` already permutes across all 644 weapon listings in every shop, so a per-shop rule here would make the three shop settings behave inconsistently with each other |
| 2026-09-28 | **D3 — Once-only listings stay in the shuffle.** Only `equipId` is written, so a listing's purchase limit stays with the listing rather than following the item. About 36 normally unlimited items become buy-once, and the nine normally once-only items — two Resonant Bells, the Hunter Chief Emblem, and six chalices — become unlimited with high probability. Nothing measured is harmful, it is what the reference does, and §2 states it plainly rather than leaving it to be discovered |
| 2026-09-28 | **D4 — The two unidentified shop families are included, and hardware settles them.** Families C and D are 112 of the 397 consumable listings, 28% of the bucket, and their identity resisted the param data, the item-name FMGs and the whole reference source. If they are unreachable they act as a sink: items permute into them and off sale. Including them is the reversible choice — restricting the pool later is a one-line change — and it is recorded as assumption F10 with its own acceptance criterion rather than as a settled fact |

---

## Appendix — research notes for stage C

*Not part of the specification, and not covered by the developer's approval. These are pointers from the spec investigation to save stage C a search. Verify anything here before relying on it.*

**Where the behaviour lives**

* `reference/Randomizer/MainWindowComponents/RandomizeFunctions.cs:848-875` — the pool-building loop; read it before the write loop, because the exactness of the permutation is a property of the two loops agreeing on their filter.
* `reference/Randomizer/MainWindowComponents/RandomizeFunctions.cs:982-1021` — the two branches this feature is. Everything between `:609` and `:980`, and everything after `:1022`, is row 5.
* `reference/Randomizer/MainWindowComponents/StartFunctions.cs:1541` — the single gate for both settings, and the reason `keepGuns` appears to interact with this feature. It does not: `keepGuns` only extends the skip list with two weapon ids.
* `app/src/Randomizer/StartingWeapons.cpp` — the pattern to follow and the code most at risk of being disturbed. `kNeverTouchEquipIds`, `kShopEquipId`, `kShopEquipType` are already there and are the same constants this feature needs.
* `app/src/Randomizer/EnemyRandomizer.cpp:1030-1062` — the item-data phase, where a new pass is added and where the pass order is decided.
* `app/tools/starting_weapons_verify.py` — invariants SW-I1/I2/I8 are the model for this feature's, and its shop loop is the thing F13 says is wrong in this feature's presence. Worth reading its `stock()` helper: it is already type-filtered and the surrounding loop is not.
* `app/src/UI/SettingsModel.cpp` — the settings table; the three shop rows and their help wording are the immediate neighbours.

**Worth checking early**

* `app/tools/param_offsets.py load_param` wants the archive member's full filename, `"ShopLineupParam.param"`, not the param type — passing `"ShopLineupParam"` returns `None` and fails inside `param_rows` with a confusing `TypeError`.
* Field offsets come from `param_offsets.py fields <root> SHOP_LINEUP_PARAM`; the param *type* string there is not the member filename above.
* `app/tools/fmg.py` reads item, weapon and armour names from one container; the members are Japanese-named and the module is deliberately silent about printing them. Any new tool code that prints a name needs `PYTHONIOENCODING=utf-8` or it raises what looks like a parse error on this console (`docs/known-traps.md`).
* Listing ids encode a shop block in their high digits; grouping by `id // 10000` is what makes F10's family table fall out, and it is also the cheapest way to restrict the pool later, should the hardware check behind D4 show families C and D are unreachable.
* The stock limit field and the two price fields are worth reading even though nothing writes them — they are what makes F8's and D2's consequences measurable, and a verifier asserting "unchanged outside the item id" covers them for free.

**Dead ends already walked**

* Looking for a shop-specific protected-item list in the reference: there is none. The three ids at `:635-638` are the whole of it, and they are shared with the weapon path.
* Looking for a bug to decide about in the armour or consumable branch: there is none. The defects catalogued in `docs/plans/param-features.md` §2 (`S-1`–`S-4`) and §1 (`D-1`–`D-4`) all belong to the weapon and item-drop paths.
* Expecting an owner-ranked writer like feature 037's: the equipment-type partition makes it unnecessary, and F11 records why rather than leaving it to be re-derived.
* Trying to identify shop families C and D from the param data, the item names and the reference source: none of the three settles it. The next thing to try is the event data or the shop-opening scripts, which this investigation did not open.
