# Boss & Item Randomization — Implementation Findings

**Status: superseded — both features are built and shipped.** This document is kept as the investigation record, not as a plan. Boss randomization (spec #2) and treasure randomization (spec #3) are both DONE and hardware-verified; they were built against [boss-randomization.md](boss-randomization.md) and [treasure-randomization.md](treasure-randomization.md), which are the contracts the code and validators actually follow. Read this one for *why* the two were sequenced the way they were and what the reference tool's undocumented behaviour turned out to be — not for current status.

The PARAM-based features this document defers (enemy drops, starting weapons) were also subsequently built; see [param-features.md](param-features.md).

This document exists because the task explicitly required it: *"If the Windows implementation contains undocumented behavior, special cases, exclusions, or assumptions, document those findings before reproducing them."* The investigation surfaced three things that materially change how these two features should be sequenced, so this is written as an implementation-ready plan rather than a post-hoc summary.

All line references are to the reference Windows tool unless marked `[PS4]`.

---

## Part A — Boss Randomization

### A1. What the Windows implementation actually does

Boss randomization is four cooperating pieces, not one:

| Piece | Location | Role |
|---|---|---|
| `GenerateBossList` | `RandomizeFunctions.cs:1554-1847` | Scans each map, builds the boss candidate pool |
| pool assembly | `StartFunctions.cs:826-890` | Dedupes, builds `combinedBossList` / `combinedBossList2`, shuffles |
| `AddOrphanPhaseOne` | `RandomizeFunctions.cs:1849-2011` | Cross-map Orphan "phase one" sync |
| `RandomizeBosses` | `RandomizeFunctions.cs:2012-2692` | Per-map assignment + multi-entity fixups |
| `AddTheRest` | `MainWindow.xaml.cs:1057+` | Randomizes the placements `RandomizeBosses` deliberately skipped |

**Boss identification** is `Name.Contains(bossList[j])` against the 38-entry `bossList` (`MainWindow.xaml.cs:161-198`) — plain substring matching, same style as the enemy exclusion list. Those 38 strings are already present in `[PS4] EnemyExclusionList.h` as the tail of the 103-entry array, where they serve the opposite purpose (keeping bosses *out* of enemy randomization).

**Pool construction** (`GenerateBossList`), with `lesserBossesBool == false` (the UI default):

A placement becomes a candidate only if it passes all of: `NPCParamID` not in `{250060, 250070, 250090, 212600, 212610, 212620}`; `EntityID != -1`; `NPCParamID` not in `{0, -1}`; `ThinkParamID != -1`; name does not contain `c5070`; if name contains `c3060` then `NPCParamID` must be exactly `210306016`. Then map-scoped rejections: map contains `"34"` and `NPCParamID == 210030`; map contains `m28` and name contains `c2100`; and (for any map that isn't `m26`) name contains `c0000_0005`.

Surviving candidates are then filtered *again* at insertion time against a second, different 15-entry name list (`:1695-1702`) — `c2500_0000, c5071_0000, c4030_0004, c2100_0000, c2100_0001, c4540_0000, c4030_0001, c4030_0002, c4030_0003, c2120_0001, c2120_0000, c2120_0002, c2570, c2571, c4030_0000` — and deduped by `NPCParamID` within the map. Entries with `ThinkParamID == 1` are dropped when serializing to the pool (`:1791`).

**Assignment** (`RandomizeBosses`) has its *own* exclusion list (`:2042-2146`) that overlaps but does not match the pool list. It adds: name contains `c3060` (outside `m29`); `ThinkParamID == 454000` outside `m36`; `m22` + `c2100_0001`; `m27` + `c2120_0001`/`c2120_0002`; `m35` + `c4030_0001`/`0002`/`0003`; `m35` + `c4030_0004`. Then a draw from `combinedBossList`, map-specific model rerolls (`m24_00` rejects models containing `4500`/`4520`; `m34_00` rejects `3060`; `m24_02` rejects `5100`/`8050`/`4520`/`4500`/`3060`), and a self-match reroll so a boss is never replaced by its own model.

**Multi-entity fixups** (`:2374-2431`) propagate the chosen identity to companion placements so multi-part fights don't end up half-randomized. The code's own labels: "Emissary fix" (`c2500_0000` → `c2570_0001`), "wet nurse fix" (`c5510_0000` → `c5510_0001`, `c5510_0002`), "maria fix" (`c4520_0002` → `c4520_0000`), "living failures fix" (`c4030_0004` → `c4030_0000`).

**Map order matters and is not the enemy order.** Bosses are processed over 17 base maps in a deliberate sequence starting `m24_00_00_01`, `m24_02_00_01` (`StartFunctions.cs:952-976`) — the two maps with the most restrictive reroll blacklists go first, while the pool is still full. The enemy pass uses a different 24-map list.

### A2. Undocumented behavior and defects found — these must be decided on, not silently copied

1. **The pool drain lives inside `if (logging)`.** `combinedBossList.RemoveAt(random)` and the follow-up "remove same-model entries" loop are both inside the logging block at `:2435-2464`. The randomization *algorithm* is therefore coupled to a diagnostics flag. Because `logging` is hardcoded `true` (`FieldContainer.cs:10`) the drain always happens in practice, so the *effective* behavior is "always drain" — but any port must reproduce the effective behavior deliberately rather than mirroring the structure.

2. **The same-model purge has an unexplained exception.** The purge skips entries containing the literal `"*4510"` (`:2457`). Given pool strings are `npc*think*model`, that guard matches on a *ThinkParamID* beginning `4510`, not on the model. Its intent is unknown and it should be reproduced literally and flagged, not "cleaned up."

3. **Unbounded reroll loops.** The self-match loop (`:2341-2360`) increments `addCounter` and resets it at the pool size but never uses it to terminate — it is a counter that does nothing. The map-specific model rerolls (`:2297`, `:2310`, `:2323`) have no cap at all. On a console these are hangs, not exceptions. **A port must bound them**, which is a deliberate behavioral deviation to document.

4. **Pool exhaustion can index out of range.** The entry guard checks `combinedBossList.Count > 0`, but the reroll loops re-draw without re-checking after the drain has emptied the pool.

5. **The two exclusion lists have drifted.** Pool eligibility and assignment eligibility are near-duplicates that are not identical. Whether that is intentional ("eligible to be replaced" vs "eligible as a replacement") or accidental is not determinable from the code.

6. **`AddOrphanPhaseOne`'s work is likely clobbered.** It runs *before* `RandomizeBosses` (`StartFunctions.cs:921-938`), picks one of three maps via `universalRand.Next(0, 3)`, and forces one placement to the Orphan's identity — but the placements it targets (`c2710_0000`, `c2500_0000`, `c4510_0000`) are all `bossList` members that `RandomizeBosses` will then re-target. The sync only survives where that placement happens to be excluded.

### A3. How it maps onto the PS4 architecture

The good news: **no new file-format capability is required.** Boss randomization touches exactly the same fields the enemy randomizer already mutates — `NPCParamID`, `ThinkParamID`, and the model reference — on `Part.Enemy` blobs in the same 17 map files that are already read and written.

One accessor is missing. The pool filter needs `EntityID`, which `[PS4] Msbb.cpp` does not expose. It is cheap and I verified the layout: `Part`'s common header stores `baseDataOffset` as the i64 at entry+`0xB0`, and `EntityID` is the first i32 at that offset (`PartsParam.cs`, `Part(BinaryReaderEx)`). So `GetEntityID(blob) = ReadI32LE(blob, ReadI64LE(blob, 0xB0))`, read-only.

**Proposed phase integration.** The job is already a resumable phase machine (`EnemyRandomizerJob`), and boss work fits it without redesign — but with one required restructure. Boss assignment must run in the Windows boss-map order, which is not the write-loop order, and it must be able to resolve a boss model name to a `modelIndex`. Today the model merge happens inside `StepWriteMap`. So:

```
Mirror → ReadMaps → BuildPool → MergeModels → AssignBosses → WriteMaps → Emevd
                                 ^^^^^^^^^^^   ^^^^^^^^^^^^
                                 extracted     new, 17 steps,
                                 from          in reference
                                 StepWriteMap  boss-map order
```

Extracting the model merge into its own phase is a move, not a rewrite, and it makes every map able to reference any enemy model before either assignment pass runs. Boss assignment then mutates in-memory blobs only; `WriteMaps` continues to do enemy mutation, `BossParamScaling`, serialize, and write exactly as today. Ordering is safe in both directions because boss placements are already excluded from enemy randomization by `EnemyExclusionList.h`, so the two passes touch disjoint placements — and scaling still runs last, seeing boss-randomized IDs, matching the reference.

**Deliberate scope reductions**, consistent with the port's existing documented simplifications: no chalice dungeons (so `combinedBossList` is base-map bosses only, since `chaliceBossString` would be empty), no `oopsAllBosses` mode, and `lesserBossesBool` fixed to `false`.

### A4. Estimated shape

New: `Randomizer/BossList.h` (38 names + `IsBossName`), `Randomizer/BossRandomizer.h/.cpp` (pool build, assignment, fixups, `AddTheRest`, Orphan sync). Modified: `Msb/Msbb.h/.cpp` (one accessor), `Randomizer/EnemyRandomizer.h/.cpp` (two phases + a settings flag), plus the settings/UI chain — `RandomizerDefaults.h`, `RandomizerDefaultsStore.cpp`, `SetupDefaultsScreen`, `EnableWizardScreen` — following the existing `randomizeEnemies` pattern exactly.

---

## Part B — Item Randomization

### B1. It is three separate features, not one

The Windows tool randomizes items in three unrelated places, and they have very different costs on PS4:

| Feature | Windows location | Data touched | PS4 feasibility |
|---|---|---|---|
| **Treasure / world pickups** | `RandomizeItemLots`, `RandomizeFunctions.cs:3306-3358` | MSB `Events.Treasures[].ItemLot1` | **Feasible now** — same map files already parsed |
| **Enemy drops** | `RandomizeItemDrops`, `:2906-3172` | `NpcParam` rows in `gameparam.parambnd.dcx` | **Blocked** — needs a PARAM stack |
| **Shop contents / starting weapons** | `RandomizeShopItems`, `:609-1315` | `ShopLineupParam`, `EquipParamWeapon` | **Blocked** — needs a PARAM stack |

### B2. The blocker: the PS4 port has no PARAM capability at all

The port's entire file-format surface is DCX + MSBB + one targeted EMEVD byte-poke. It has **no BND4 reader, no PARAMDEF support, no PARAM row/cell model, and no BND repacking**. Two of the three item features are pure param editing — the Windows code opens `paramdef.paramdefbnd.dcx` and `gameparam.parambnd.dcx` via `BND4.Read`, applies paramdefs, mutates rows, re-serializes each `PARAM` into its `BinderFile.Bytes`, and rewrites the whole archive.

Building that stack is a substantial standalone milestone comparable to the original MSBB work — a BND4 container parser/writer, PARAMDEF parsing, typed cell access, and byte-exact repacking, all needing the same independent round-trip validation the MSBB layer got. It is emphatically not part of an incremental item-randomization change, and attempting it inside this task would be exactly the kind of broad architectural expansion the task's own constraints rule out.

**This also means the current enemies-only output is correct in a way worth preserving:** the port never writes `gameparam.parambnd.dcx` at all, it only mirrors it verbatim — which is precisely why the contaminated-vanilla bug was visible as "items already randomized" rather than something the port itself did.

### B3. Treasure randomization is genuinely tractable — format confirmed

I verified the layout from `SoulsFormats` rather than assuming it:

- Treasure is `EventType 0x4` (`EventParam.cs`, `enum EventType`).
- Event common header: `nameOffset` @ `0x00`, `EventID` @ `0x08`, `Type` @ `0x0C`, `baseDataOffset` @ `0x18`, `typeDataOffset` @ `0x20`.
- Treasure type data: `partIndex2` @ +`0x08`, then **`ItemLot1` @ +`0x10`**, `ItemLot2` @ +`0x14`, `ItemLot3` @ +`0x18` (confirmed by the `UnkT1C` field landing at +`0x1C`).

`[PS4] MsbbFile` already parses the Events section into blobs and round-trips them verbatim, so this needs only a small `event_fields` accessor pair (`GetType`, `Get/SetTreasureItemLot1`) alongside the existing `part_fields`/`model_fields`, plus one more job phase. No new container or archive work.

The Windows algorithm itself is simple and well-behaved — unusually so for this codebase: collect every `ItemLot1 > 0` not in a caller-supplied exclusion list, then assign by drawing **without replacement** (`:3352`). Notably it has none of the off-by-one or unbounded-loop defects that affect the drop and gem randomizers.

**Exclusions to preserve**, built in `StartFunctions.cs:1260-1283`: `nonoItemLots` (never randomized) and `keyItemLots`, with the workshop-tool lots conditionally excluded on `workshopBool`. The inline comments there are the original author's own uncertain annotations (one literally reads `//Evil Eye Bridge key (key in nightmare of mensis?)`) — they should be carried across as-is and treated as unverified.

---

## Part C — Verification strategy, and why it gates everything

There is **no host C++ compiler in this environment** and no test harness in the repo. The cross-toolchain build catches type and link errors only; it cannot execute anything. The established project pattern for logic correctness is an independent Python mirror of the byte-level algorithm run against real map files — that is how the MSBB round-trip and the `BossParamScaling` tables were validated.

So "add appropriate tests for the deterministic/randomization logic" concretely means: write a Python mirror of the boss pool build and assignment, run it against the newly cleaned vanilla tree at `data/vanilla/dvdroot_ps4`, and assert that (a) the same seed produces identical assignments across runs, (b) different seeds diverge, (c) every assigned `NPCParamID`/`ThinkParamID` pair exists in the vanilla data, and (d) no non-boss placement changes. That is a real piece of work in its own right, and without it "it compiles and produces files" is not evidence the feature is correct — which the task rightly calls out.

Everything past the generated files — that a randomized boss actually spawns, fights correctly, and doesn't crash — is only verifiable on hardware, by you.

---

## Part D — Recommended sequencing

The three findings that changed the plan: the boss pool drain is coupled to a logging flag and the reroll loops are unbounded (so a faithful port needs deliberate, documented deviations); two of the three item features are blocked behind a PARAM stack that doesn't exist; and neither feature can be verified here without first building a Python mirror.

Given that, and that your own task text gates item randomization on boss randomization being *verified* — which requires a hardware test only you can run — I stopped before implementing rather than landing two large, unverifiable features into a codebase that just reached a known-good state.

Proposed increments, each independently reviewable and testable:

1. **Boss randomization** — `EntityID` accessor, `BossList.h`, `BossRandomizer`, the `MergeModels`/`AssignBosses` phase split, and the settings/UI chain. Ships with a Python mirror validating determinism against the clean vanilla tree. Then a hardware test.
2. **Treasure item randomization** — `event_fields` accessors, one phase, the `nonoItemLots`/`keyItemLots` exclusions. Small, self-contained, and independent of increment 1.
3. **PARAM stack** — BND4 + PARAMDEF + PARAM, validated by byte-exact round-trip against a real `gameparam.parambnd.dcx` before anything writes through it. A milestone in its own right.
4. **Enemy drops and shop randomization** — built on increment 3.

Increments 1 and 2 are independent; 2 is the smaller and lower-risk of the two if you would rather bank a quick win first.
