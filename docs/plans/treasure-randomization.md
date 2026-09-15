# Treasure Randomization — Behavior Specification & Implementation Boundary

**Increment scope: treasure/pickup randomization only.** No enemy drops, no shop contents, no PARAM/BND/PARAMDEF work. Written before coding, from measurements against real vanilla data, so the C++ and the Python validator are both implemented against *this document* rather than against each other.

Companion docs: [boss-randomization.md](boss-randomization.md) (the pattern this follows), [boss-and-treasure-findings.md](boss-and-treasure-findings.md) (why drops/shops are deferred).

---

## 1. What "treasure randomization" means here

A **treasure** is a world pickup — an item lying on the ground, in a chest, or on a corpse. In the map data it is an `Event` of type `Treasure`, and the item it grants is an **item-lot ID** in that event's `ItemLot1` field. Randomizing treasure means permuting which item-lot ID sits at which pickup location.

It does **not** mean touching what enemies drop, what shops sell, or the contents of the item lots themselves. Those live in `gameparam.parambnd.dcx` and are out of scope.

## 2. What the Windows implementation actually does

Three pieces, all driven by the `randomizeItemLots` setting:

| Piece | Location | Role |
|---|---|---|
| exclusion setup | `StartFunctions.cs:1260-1283` | Builds `nonoItemLots` |
| `GenerateItemLotList` | `MainWindow.xaml.cs:1499-1520` | Builds the item pool |
| `RandomizeItemLots` | `RandomizeFunctions.cs:3306-3358` | Assigns pool entries to locations |

### 2.1 Exclusions

`nonoItemLots` starts with three entries, then absorbs all six `keyItemLots`, then the two `workshopItemsList` entries **unless** the "Randomize Workshop Tools" setting is on:

| Lot ID | Author's comment | Category |
|---|---|---|
| 2600550 | Evil Eye Bridge key | base |
| 2400450 | key to the Old Town | base |
| 3500800 | key to the dungeon usually door | base |
| 2800290 | key to Cathedral Street C (UCW key) | key item |
| 3200720 | key to nightmare classroom | key item |
| 3200810 | Veranda of key (key to rom fight) | key item |
| 2410990 | Invitation to the castle (cainhurst summons) | key item |
| 3502000 | Parish length Ω Startup Item (laurence skull) | key item |
| 3401810 | Altar Elevator Startup Item (eye pendant) | key item |
| 2411000 | Blood gem workshop tool | workshop |
| 2200360 | Rune tool | workshop |

Those comments are the original author's own, several with question marks; they are carried across verbatim and treated as **unverified annotations**, not facts.

Progression/key items therefore get special treatment only via this hand-maintained blacklist. There is no logic-aware placement, no reachability analysis, and no notion of item category.

### 2.2 Pool eligibility vs location eligibility — they are NOT the same

This is the one genuinely subtle thing in the algorithm:

| | Condition |
|---|---|
| **Pool** (`GenerateItemLotList:1515`) | `ItemLot1 > 1` and not in `nonoItemLots` |
| **Location** (`RandomizeItemLots:3328`) | `ItemLot1 > 0` and not in `nonoItemLots` |

`> 1` versus `> 0`. A treasure whose `ItemLot1` is exactly `1` would be a valid *destination* but would never enter the *pool*.

**Measured against real vanilla data: no treasure has `ItemLot1 == 1`, so the asymmetry has no effect.** Both sets contain exactly the same 1041 treasures. It is reproduced anyway (free, and avoids a silent divergence if the assumption ever breaks) and documented here so nobody "simplifies" the two conditions into one on the assumption they're equivalent.

`ItemLot1 == -1` does occur (Hunter's Dream entity 64), and both conditions correctly reject it.

### 2.3 Algorithm

Pool is built across **all** maps first, then consumed across all maps:

```csharp
randomNumber = universalRand.Next(0, itemLotList.Count);
tempGuy.Events.Treasures[i].ItemLot1 = itemLotList[randomNumber];
itemLotList.RemoveAt(randomNumber);
```

Draw-uniformly-then-remove = sampling **without replacement**, i.e. a Fisher-Yates shuffle dealt out in map-then-treasure iteration order. Since the pool and the destination set are the same 1041 treasures, this is an exact **permutation**: every eligible vanilla item lot appears exactly once somewhere, and every eligible location receives exactly one.

Duplicates are therefore not introduced by the algorithm — but 1041 treasures carry only **645 distinct** lot IDs, so values that repeat in vanilla still repeat after shuffling.

Only `ItemLot1` is touched. `ItemLot2` and `ItemLot3` are never read or written.

### 2.4 Map list, and two surprises in it

`StartFunctions.cs:1293-1316` and `1326-1349` list the maps. Both lists are identical, and both contain:

- **`m21_00_00_00` (Hunter's Dream) commented out.** Its 2 treasures (lot `2100000`, and one with lot `-1`) are never pooled and never randomized. Whether this is deliberate protection of the starting area or an accident is not determinable from the source, but the effect is clear and useful: **Hunter's Dream is an untouched control group.**
- **`m21_01_00_00` (Abandoned Old Workshop) listed twice**, in both the generate and the randomize loop. It has 3 treasures, all eligible.

The doubling is almost certainly a copy-paste slip, but it is self-consistent: the pool gains those 3 entries twice *and* the map is randomized twice (the second pass re-randomizing what the first wrote), so pool and destination counts stay balanced at 1044 each. Its real consequence is that those 3 Old Workshop item lots each appear **twice** in the world while every other item appears once.

**Decision: reproduced as-is**, because the alternative is inventing behavior, and both variants are equally safe with respect to pool exhaustion (1041/1041 without the duplicate, 1044/1044 with it). Flagged here as a candidate for a future deliberate deviation if you'd rather not have three duplicated items.

### 2.5 Pool exhaustion

`RandomizeItemLots` guards `if (itemLotList.Count > 0)` once, at function entry, but never re-checks inside the loop. If the pool emptied mid-map the reference would index an empty list and throw. On the measured data pool size exactly equals destination count, so the last draw consumes the last entry and it never trips — but there is zero margin.

### 2.6 Write-back

`tempGuy.Write(currentMap)` unconditionally at the end of every call, whether or not anything changed — the reference's in-place model.

## 3. Behavioral contract

### Inputs
Clean vanilla tree; a seed; `randomizeTreasure` on/off.

### Output
A tree in which every eligible treasure's `ItemLot1` has been permuted among the eligible pool, and nothing else has changed.

### Invariants (testable without running Bloodborne)

| # | Invariant | Rationale |
|---|---|---|
| **I1** | Only `Treasure` (type `0x4`) events change | Nothing else is a target |
| **I2** | Within a changed treasure, only `ItemLot1` differs — `ItemLot2`, `ItemLot3`, `EventID`, name and type are byte-identical | The algorithm writes one field |
| **I3** | Every resulting `ItemLot1` value exists as a vanilla `ItemLot1` somewhere in the pooled maps | The randomizer must not fabricate item IDs |
| **I4** | The multiset of `ItemLot1` values over all eligible locations is **exactly** the vanilla multiset | Permutation, not resampling. Strictly stronger than I3 and catches duplication/loss |
| **I5** | No `nonoItemLots` value is ever moved, and no protected location's value changes | Key/progression items stay put |
| **I6** | Treasures in `m21_00_00_00` (Hunter's Dream) are unchanged | The reference excludes that map |
| **I7** | Non-Treasure events and all Parts/Models/Regions data are unchanged by this feature | Scope containment |
| **I8** | Output parses as structurally valid MSBB with intact section framing | No malformed data |
| **I9** | Same seed + same input ⇒ byte-identical output | Determinism |
| **I10** | Feature disabled ⇒ treasure data byte-identical to vanilla | Vanilla preservation |
| **I11** | Eligible-location count is 1041 (1044 counting the duplicated map pass) | Guards against a silent eligibility-rule regression |

I4 is the strongest and cheapest real check: a permutation invariant needs no knowledge of *which* item went *where*, so the validator never has to reimplement the algorithm.

## 4. PS4 implementation boundary

**No new file-format capability is required.** Confirmed against the current code:

- `Msbb.cpp:114/130` already parses and re-serializes `EVENT_PARAM_ST`, keeping every event as an opaque blob that round-trips verbatim.
- Event header layout (from `EventParam.cs`): `nameOffset` @ `0x00`, `EventID` @ `0x08`, **`Type` @ `0x0C`**, `baseDataOffset` @ `0x18`, `typeDataOffset` @ `0x20`.
- Treasure type data: `partIndex2` @ +`0x08`, **`ItemLot1` @ +`0x10`**, `ItemLot2` @ +`0x14`, `ItemLot3` @ +`0x18`.
- Verified empirically: across the 23 pooled maps there are 6229 events, of which **1056 are type `0x4`**, and their `ItemLot1` values are sane (42000–3600700).

So the change is: one new `event_fields` accessor group next to the existing `part_fields`/`model_fields`, one new `TreasureRandomizer` translation unit, two new job phases, and the settings/UI chain — mirroring the boss increment exactly.

`ItemLot1` is a fixed-width `int32` poked in place, so no blob changes length and the existing "entry blobs are position-independent" guarantee is untouched.

## 5. Deliberate deviations

| # | Reference | This port | Why |
|---|---|---|---|
| **T1** | Pool guard checked once at function entry; can index an empty pool mid-loop | Re-checked before every draw; if empty, the location is left vanilla | Zero-margin crash on a console. Cannot trigger on measured data |
| **T2** | Writes every map unconditionally | Writes through the existing one-read-one-write pipeline | Pre-existing port convention |
| **T3** | Per-item text log file | `Log()` summary per map plus a run total | Excessive logging for 1041 items; the validator provides the detail |

Everything else — the exclusion list, both eligibility conditions including the inert `>1`/`>0` asymmetry, the map list with Hunter's Dream excluded and Abandoned Old Workshop doubled, without-replacement selection, and `ItemLot1`-only mutation — is reproduced as-is.

## 6. PS4 implementation

New: `Randomizer/TreasureRandomizer.h/.cpp` (map order, protected-lot test, collect, assign) and an `event_fields` accessor group in `Msb/Msbb.h/.cpp` (`GetType`, `Get/SetTreasureItemLot1`). Modified: `EnemyRandomizer.h/.cpp` gains `randomizeTreasure`/`randomizeWorkshopTools` options, a `treasuresRandomized` result counter, and two phases; plus the settings chain (`RandomizerDefaults`, store, Setup Defaults, Enable wizard) following the `randomizeEnemies`/`randomizeBosses` pattern exactly.

Pipeline position — treasure work runs after boss work and before the write loop, so all three passes mutate the same in-memory maps and each map is still read once and written once:

```
Mirror → ReadMaps → BuildPool → MergeModels → [BossCollect → BossAssign]
       → [TreasureCollect ×24 → TreasureAssign ×24] → WriteMaps → Emevd
```

Ordering is safe because the passes touch disjoint sections: enemies and bosses write `Parts`, treasure writes `Events`. The treasure pool is therefore built from vanilla `ItemLot1` values regardless of what the other passes did.

Both treasure phases walk `TreasureMapOrder()` including its duplicate entry, and the duplicate resolves to the *same* in-memory map, so the second assign pass sees the first pass's writes — the same semantics the reference gets from its disk round-trip.

## 7. Verification results (pre-hardware)

| Layer | Result |
|---|---|
| **V0** data assumptions | **PASS** — 1056 Treasure events; 1041 eligible locations (matches I11); pool 1044 = 1041 + the duplicated map's 3; zero `lot == 1` treasures, so the `>1`/`>0` asymmetry is confirmed inert; control map has 2 treasures (`2100000`, `-1`) |
| **V1** determinism | Deferred to hardware (V7) — the RNG is the same seeded `std::mt19937` the other passes use |
| **V2** output invariants | **PASS** on a simulated full run: 1044 assignments, pool exhausted to 0, 1039 slots visibly changed (2 landed on their own value by chance, expected in a permutation), 0 failures |
| **V3** vanilla preserved | **PASS** — validator silent on an unmodified tree; phases skipped entirely when the setting is off |
| **V4** build/package | **PASS** — clean cross-build, no warnings, `.pkg` regenerated |
| Validator selftest | **6/6** — catches a fabricated lot (I3), a moved protected lot (I5), a touched control map (I6), an `ItemLot2` write (I2), a non-Treasure event edit (I7), and stays silent on a clean tree |
| Boss validator regression | **5/5** — unchanged |

**Precise prediction for the hardware run:** the summary line should read `RANDOMIZED 1044 TREASURE PICKUPS`, and the log should end with `0 lots unused`. Any other number means the eligibility rules or the map list diverged, and is a fail signal before the game is even launched.

## 8. Out of scope / deferred

Enemy drops (`RandomizeItemDrops`, `NpcParam`), shop contents and starting weapons (`RandomizeShopItems`, `ShopLineupParam`/`EquipParamWeapon`), and everything else requiring BND4/PARAM/PARAMDEF. Unchanged from the findings document: that stack is a milestone of its own.

Also out of scope: logic-aware or reachability-aware placement. The reference has none, and neither does this.
