# Boss Randomization — Behavior Specification & Implementation Boundary

**Increment scope: boss randomization only.** No treasure randomization, no PARAM stack, no enemy-drop or shop work. Written before coding, from the investigation in [boss-and-treasure-findings.md](boss-and-treasure-findings.md), so that the C++ implementation and the Python verification tooling are both written against *this document* rather than against each other.

---

## 1. Intended behavior

When **RANDOMIZE BOSSES** is enabled, each qualifying boss placement across the 17 base maps is reassigned the identity (`NPCParamID`, `ThinkParamID`, model) of a different boss drawn from a pool built out of the vanilla boss placements in those same maps. Multi-part boss fights keep their companion placements in sync. When the setting is disabled, boss placements are left byte-identical to vanilla.

Boss randomization is independent of enemy randomization: either, both, or neither may be enabled. They operate on disjoint placements — every boss name is already in `EnemyExclusionList.h`, so the enemy pass never touches a boss placement.

## 2. In scope

- The 17 base maps the reference tool processes for bosses, **in its order** (`StartFunctions.cs:952-976`), which is deliberately not the enemy pass's 24-map order and starts with the two most constrained maps while the pool is still full:
  `m24_00_00_01, m24_02_00_01, m21_00_00_00, m21_01_00_00, m22_00_00_00, m23_00_00_00, m23_00_00_01, m24_01_00_01, m25_00_00_00, m26_00_00_00, m27_00_00_01, m28_00_00_01, m32_00_00_01, m33_00_00_00, m34_00_00_00, m35_00_00_00, m36_00_00_00`
- Pool construction per `GenerateBossList` (`RandomizeFunctions.cs:1554-1847`).
- Assignment per `RandomizeBosses`' non-chalice branch (`:2012-2475`).
- The four multi-entity fixups (`:2374-2431`).
- `AddTheRest` (`MainWindow.xaml.cs:1057+`) for the placements assignment deliberately skips in `m22`, `m27_00_00_01`, `m35`.
- `AddOrphanPhaseOne` (`:1849-2011`) — see §5.
- A `randomizeBosses` setting following the existing `randomizeEnemies` pattern through `RandomizerDefaults` → store → Setup Defaults → Enable wizard.

## 3. Out of scope (deliberate, consistent with the port's existing simplifications)

| Excluded | Reason |
|---|---|
| Chalice dungeons (`m29*`) | Already out of scope port-wide; `combinedBossList` therefore reduces to base-map bosses, since `chaliceBossString` would be empty |
| `oopsAllBosses` mode | No UI for it; ~200 lines of the reference's boss code is this branch |
| `lesserBossesBool` | Fixed `false`, matching the reference's unchecked-by-default UI |
| `InsertBossesVoid` ("bosses can replace regular enemies") | A separate reference feature with its own setting; not this increment |
| Per-run text logging of boss assignments | The port logs via `Log()`; it does not reproduce the reference's log files |

## 4. Rules (the contract both implementations are written against)

### 4.1 Boss identification
A placement is a boss if `Part.Enemy.Name` contains any of the 38 `bossList` substrings (`MainWindow.xaml.cs:161-198`). Plain substring match. **Not** `EnemyExclusionListExtra()` — those 45 entries are this port's own heuristic protections for enemy randomization and have no reference-tool equivalent.

### 4.2 Pool eligibility (`GenerateBossList`, lesserBosses = false)
Reject the placement if any holds:
- `NPCParamID` ∈ {250060, 250070, 250090, 212600, 212610, 212620}
- `EntityID == -1`
- `NPCParamID` ∈ {0, -1}
- `ThinkParamID == -1`
- name contains `c5070`
- name contains `c3060` and `NPCParamID != 210306016`
- map name contains `"34"` and `NPCParamID == 210030`
- map name contains `m28` and name contains `c2100`
- name contains `c0000_0005` (all maps except `m26`)

Then, at insertion, additionally reject if the name contains any of:
`c2500_0000, c5071_0000, c4030_0004, c2100_0000, c2100_0001, c4540_0000, c4030_0001, c4030_0002, c4030_0003, c2120_0001, c2120_0000, c2120_0002, c2570, c2571, c4030_0000`

Dedupe by `NPCParamID` **within each map**. Drop entries whose `ThinkParamID == 1` when serializing. The pool is then globally deduped by exact identity (`StartFunctions.cs:827`).

A second pool, the **refill pool**, is the same list additionally deduped so that no two entries share a model (`StartFunctions.cs:868-879`).

### 4.3 Assignment eligibility (`RandomizeBosses`)
This is a *different* list from 4.2 and the difference is preserved deliberately (see D5). Reject if:
- `NPCParamID` ∈ {250060, 250070, 250090, 212600, 212610, 212620}
- `EntityID == -1`
- `NPCParamID` ∈ {0, -1}
- name contains `c3060`
- `ThinkParamID == 454000` and map is not `m36`
- `m22` + name contains `c2100_0001`
- `m27` + name contains `c2120_0001` or `c2120_0002`
- `m35` + name contains `c4030_0001`, `c4030_0002`, `c4030_0003`, or `c4030_0004`
- name contains `c0000_0005` and map is not `m26`
- map name contains `"34"` and `NPCParamID == 210030`
- map name contains `m28` and name contains `c2100`

### 4.4 Selection
Draw uniformly from the pool. Then apply, in order:
1. **Map model blacklists** — `m24_00`: reject models containing `4500`/`4520`. `m34_00`: reject `3060`. `m24_02`: reject `5100`/`8050`/`4520`/`4500`/`3060`.
2. **Self-match reroll** — reject if the placement's own `Name` contains the candidate's model (a boss must not be replaced by itself).
3. Assign `NPCParamID`, `ThinkParamID`, model.
4. If the assigned `NPCParamID == 507200`, force `ThinkParamID = 507200`.
5. **Drain**: remove the chosen entry, then remove every remaining entry whose serialized form contains `*<model>` **except** entries containing the literal `*4510`.
6. If the pool is empty, refill from the refill pool (§4.2).

### 4.5 Multi-entity fixups
After assigning a placement whose `Name` is exactly one of these, copy the same identity onto its companions in the same map:

| Assigned | Companions |
|---|---|
| `c2500_0000` | `c2570_0001` |
| `c5510_0000` | `c5510_0001`, `c5510_0002` |
| `c4520_0002` | `c4520_0000` |
| `c4030_0004` | `c4030_0000` |

### 4.6 AddTheRest
For `m22` (`c2100_0001`), `m27_00_00_01` (`c2120_0001`, `c2120_0002`), and `m35` (`c4030_0001`, `c4030_0002`, `c4030_0003`) — the placements §4.3 skips — draw from the **refill pool** with removal, applying the same self-match reroll. Runs after all per-map assignment.

## 5. AddOrphanPhaseOne, and why it is not dead code

Before assignment, one of three maps is chosen at random (`m24_02_00_01`, `m24_01_00_01`, `m34_00_00_00`) and a specific placement in it is forced to the Orphan of Kos identity captured from `m36` (`c4540_0000`): `m24_01`→`c2710_0000`, `m24_02`→`c2500_0000`, `m34_00`→`c4510_0000`.

At first reading this looks pointless, because assignment runs afterwards and those placements are `bossList` members. **It is not pointless**: §4.3 rejects any placement with `ThinkParamID == 454000` outside `m36`. If the Orphan's think ID is `454000`, then forcing it onto the placement makes assignment skip that placement, so the Orphan identity survives. The effect is conditional on a value in the game data, which the Python tooling verifies against real files rather than assuming (see §7, check V0).

## 6. Deliberate deviations from the reference implementation

Each is a conscious decision, not an oversight.

| # | Reference behavior | This port | Rationale |
|---|---|---|---|
| **D1** | Pool drain (§4.4 step 5) sits inside `if (logging)` (`:2435-2464`), so the algorithm depends on a diagnostics flag | Always drain | `logging` is hardcoded `true` in the reference, so this reproduces the *effective* behavior. Reproducing the structural coupling would be copying a defect |
| **D2** | Reroll loops (`:2297`, `:2310`, `:2323`, `:2341`) are unbounded; `addCounter` looks like a cap but is never checked | Bounded at 200 attempts; on exhaustion **leave the placement unmodified** | Unbounded loops on a console hang the app with no recovery. Skipping keeps the vanilla boss, which is always safe |
| **D3** | Reroll loops can index an emptied pool out of range | Refill first; if still empty, skip the placement | Same reasoning as D2 |
| **D4** | Reads and rewrites each map from disk several times across passes | One read, one write per map; boss assignment mutates in memory | Pre-existing port convention, already documented in `EnemyRandomizer.cpp` |
| **D5** | §4.2 and §4.3 exclusion lists have drifted apart | Both reproduced exactly as-is | Whether the drift is intentional is undeterminable from the source; changing either would alter behavior on unknown-to-us grounds |
| **D6** | `paramNumber--` / `newNPCParam` bookkeeping during assignment | Not reproduced | Write-only in the reference for this path; feeds no output |
| **D7** | Fixups (§4.5) are applied inline at assignment time, so a companion that is itself an eligible target and sits **later** in the parts list gets independently reassigned afterwards, breaking the sync | Fixups applied as a **post-pass** after all assignments in the map, so the leader's identity always wins | Measured against real vanilla data (§8): the Emissary pair genuinely desyncs in the reference. The fixups exist precisely to stop a fight being half-randomized, so inline ordering defeats their own purpose. The post-pass preserves the evident intent, is deterministic, and touches no eligibility logic |

## 8. V0 results — measured against real vanilla data

Run: `python app/tools/boss_verify.py pool "data/vanilla/dvdroot_ps4"` (all 17 boss maps loaded).

- **Orphan premise confirmed.** `c4540_0000` in `m36` is `npc=454000 think=454000 model=c4540`. `ThinkParamID == 454000` holds, so §5 is correct: `AddOrphanPhaseOne` is not dead code.
- **Pool: 18 distinct identities**; refill pool (model-deduped): 17.
- **27 assignment targets** across 16 of the 17 maps.
- **Fixup reality check** (parts-list index order, and whether the companion is itself a `bossList` name):

## 9. Verification results — COMPLETE (2026-09-12)

All six layers closed. Hardware run confirmed by the user: no regression with bosses off, bosses randomized when on.

| Layer | Result |
|---|---|
| V0 data assumptions | **PASS** — Orphan premise confirmed (`c4540_0000` = `npc 454000, think 454000, model c4540`); pool 18 identities / 17 model-distinct; 27 assignment targets |
| V1 determinism | **PASS** — user confirmed same seed reproduces the same bosses |
| V2 valid assignments | **PASS** — property validator, 0 failures against real hardware output |
| V3 game-data validity | **PASS** — all output parses; every `modelIndex` in range; part names and EntityIDs unchanged |
| V4 vanilla preserved | **PASS** — user confirmed enemies-only run shows no boss changes |
| V5 build/package | **PASS** — clean cross-build, no warnings, `.pkg` regenerated |
| V6 in-game | **PASS** — user confirmed bosses spawn, fight, and are killable |

**Validator selftest: 5/5.** It catches a non-boss mutation, a fabricated `NPCParamID`, an out-of-range `modelIndex`; it stays quiet on an unmodified tree and on a scaling-only rewrite.

**33 boss placements reassigned** in the verified run (27 direct targets + fixup companions + `AddTheRest`).

Two validator corrections were needed, both because the first version applied an invariant that was too strict rather than because the C++ was wrong:

1. **Scaling variants are legitimate.** `BossParamScaling` writes `900xxxxxx`-range `NPCParamID`s that exist in `NpcScalingTable.h` but in no map placement. The first run reported 1,480 false failures on this. The invariant is now "present in vanilla placements **or** a scaling-table id whose `(think, model)` pair exists in vanilla" — this is the same incomplete-check trap the project hit once before: absence from map placements is not evidence of corruption.
2. **A scaling-only rewrite is not a reassignment.** Five placements changed `NPCParamID` while keeping `think` and `model` identical — the scaling pass acting on placements boss randomization correctly skipped. Classifying those as assignments produced false "ineligible placement changed" and "self-assignment" reports. Both cases are now regression-guarded in the selftest.

**D7 confirmed working on real output** — every fixup group is synchronised, including the Emissary pair that desyncs in the reference:

| Map | Leader → identity | Companions |
|---|---|---|
| `m24_02_00_01` | `c2500_0000` → `500241*c5000` | `c2570_0001` matches |
| `m26_00_00_00` | `c5510_0000` → `452000*c4520` | `c5510_0001`, `c5510_0002` match |
| `m35_00_00_00` | `c4520_0002` → `805000*c8050` | `c4520_0000` matches |

**§5 confirmed end-to-end**: `m34_00_00_00/c4510_0000` came out as `think 454000, model c4540` — the Orphan identity, applied by `AddOrphanPhaseOne` and then *not* overwritten by assignment, exactly because §4.3 skips `ThinkParamID == 454000` outside `m36`.

---

| Fixup | Leader idx | Companion idx | Companion independently targeted? | Reference outcome |
|---|---|---|---|---|
| Emissary `c2500_0000`→`c2570_0001` | 20 | 57 (after) | **Yes** | **Desyncs** — motivates D7 |
| Wet Nurse `c5510_0000`→`c5510_0001/0002` | 117 | 118, 119 (after) | No | Survives |
| Maria `c4520_0002`→`c4520_0000` | 116 | 115 (before) | No | Survives |
| Living Failures `c4030_0004`→`c4030_0000` | 100 | 96 (before) | Yes | Never fires — the leader is excluded from assignment in `m35` (§4.3), so `c4030_0000` is simply randomized on its own. Reproduced as-is |

## 7. Verification layers

| Layer | What it proves | How | Runs where |
|---|---|---|---|
| **V0** | The data assumptions in this spec hold | Python reads real vanilla maps: confirm the Orphan think-ID hypothesis (§5), and that the pool is non-empty and plausible | Here, now |
| **V1** | Deterministic randomization | Same seed twice ⇒ byte-identical output; different seeds ⇒ different output | Python oracle here; output comparison after hardware run |
| **V2** | Valid assignments per §4 | Every assigned triple exists in vanilla data; no placement gets its own model; fixup groups share one identity; no ineligible placement changed | Python property validator |
| **V3** | Correct game-data modification | Output parses as valid MSBB; section framing intact; every `modelIndex` in range; only `Part.Enemy` bytes differ | Python property validator |
| **V4** | Vanilla preserved when disabled | With the setting off, output maps byte-identical to the enemies-only baseline | Python diff |
| **V5** | Package builds and deploys | Cross-toolchain clean build + `.pkg` generation | Here |
| **V6** | In-game correctness | Bosses actually spawn, fight, and can be killed | **PS4 hardware — you** |

### Why the Python tooling is not "a second implementation of the same bug"

Two separate mechanisms, only one of which re-derives the algorithm:

- The **property validator** (V2, V3, V4) never re-derives expected assignments. It checks *invariants* of the real generated output against the real vanilla input — values exist in source data, indices in range, groups consistent, non-targets unchanged. A shared misreading of the rules cannot make a fabricated `NPCParamID` exist in vanilla data or an out-of-range `modelIndex` valid.
- The **oracle** (V0, V1) is derived from the C# source directly and is used for data-assumption checks and determinism, not for asserting specific expected assignments. It deliberately does **not** attempt to bit-match C++ `std::mt19937`/`uniform_int_distribution` draw-for-draw; cross-language PRNG matching is fragile and would be testing the PRNG, not the rules.

The residual risk this leaves — both implementations sharing a misreading of §4 that still yields structurally valid output — is real and is what V6 exists to catch.
