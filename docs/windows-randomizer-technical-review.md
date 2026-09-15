# Windows Bloodborne Enemy Randomizer — Technical Review

**Subject repository**: `Alo81/Bloodborne-Enemy-Randomizer` (local clone at repo root; `origin` confirmed via `git remote -v`).
**Purpose of this document**: an accurate, citation-backed account of how the existing Windows/WPF randomizer actually works, produced to inform later decisions about reproducing its behavior in the native C++ PS4 homebrew application. **This document is research only — no porting decisions are made here, and neither the Windows randomizer nor the PS4 project were modified to produce it.**

Every claim below is one of three kinds, and they are kept visually distinct throughout:

- **Fact** — directly established by reading the source, with a `file:line` citation.
- **Inference** — a reasonable interpretation of intent/behavior that is *not* literally stated in the code (e.g., "this is probably the Orphan of Kos encounter"), always marked as such.
- **Porting note** — a forward-looking observation about what this implies for a PS4/C++ reimplementation. These are informational, not a design proposal.

Where the code itself is ambiguous, contradictory, or incomplete, this document says so explicitly rather than guessing — see the **Open Questions** section (§13) and the inline "Open question" callouts.

---

## 1. Executive Summary

The Windows randomizer is a single-purpose WPF desktop app (~13,100 lines of C# across the app project, plus a large vendored third-party library). It has **no separation between UI and domain logic at the type level** — the entire application, including all randomization algorithms, is one `partial class MainWindow` split across eight files for physical organization only. There is no settings object, no dependency injection, no test suite, and no persistence of user preferences between runs.

Functionally, the tool:

1. Expects to be launched from inside a folder whose **parent** directory contains a `dvdroot_ps4` folder — the conventional root of an extracted/staged Bloodborne PS4 disc image (`StartFunctions.cs:52-54`). There is no folder-picker UI despite a file-dialog library being referenced in the project.
2. Reads Bloodborne's map files (`MSBB`, a Bloodborne-specific variant of FromSoftware's MSB format) and, for several features, its param tables (`gameparam.parambnd.dcx`, via `PARAM`/`PARAMDEF`/`BND4`) using the vendored **SoulsFormats** library.
3. Applies one or more of ~17 independent randomization/normalization routines — enemy placement, boss placement, NPC appearance, shop contents, starting weapons, item drops, treasure/item lots, melee/gun movesets, decals, dialogue params, VFX params, gem/rune generation, and a few dead or stub features — each gated by its own checkbox.
4. Writes every modification **back to the exact same file path it was read from**, restoring from a `.bak` sibling file first so each run starts from a guaranteed-vanilla baseline (`StartFunctions.cs:206-286`). There is no separate output directory; the `dvdroot_ps4` tree is mutated in place.
5. Terminates the entire process (`Environment.Exit(0)`, `StartFunctions.cs:2605`) at the end of a successful run — the app is architected for exactly one randomization per process launch.

The single most important structural finding for the PS4 porting effort: **the tool's "no vanilla data" problem does not exist for the Windows app** — it manufactures its own vanilla baseline via the `.bak` restore-then-rewrite cycle, using files that are assumed to already be sitting in `dvdroot_ps4`. The existing project memory note that "AFR is overlay / no vanilla data" means this exact mechanism **cannot be ported as-is**: an AFR-style overlay on PS4 homebrew has no on-disk vanilla copy to restore from, so the "restore-from-.bak, then always randomize from a known-vanilla state" model that gives the Windows tool its determinism guarantee has no direct PS4 equivalent and needs a substitute design (see §12).

A second major finding: a substantial fraction of the visible feature set is either **dead code**, a **non-functional stub**, or **mislabeled** (a "randomizer" that contains no randomness). This matters enormously for scoping a reimplementation — faithfully "porting everything" would mean porting several no-ops. These are catalogued precisely in §5 and §13 so they can be triaged deliberately rather than rediscovered by surprise.

---

## 2. Repository Map

```
Bloodborne-Enemy-Randomizer/               (git root; remote: Alo81/Bloodborne-Enemy-Randomizer)
├── README.md                              minimal — links to a Nexus Mods page, one design note
├── reference/                             the reviewed application (renamed from "MSB Test" 2026-09-14)
│   ├── BloodborneRandomizer.sln           solution file (2 projects)
│   ├── Randomizer/                        the WPF application (all Bloodborne-specific logic lives here)
│   │   ├── Randomizer.csproj              WinExe, net core 3.1, WPF, refs SoulsFormats + 2 NuGet pkgs
│   │   ├── app.manifest                   default VS template, no customization
│   │   ├── App.xaml / App.xaml.cs         entry point — StartupUri only, no code
│   │   ├── Window1.xaml / .xaml.cs        secondary "Scaling Window" (per-area scale picker; non-functional, see §5.9)
│   │   ├── menu_knowledge_04215.png       app icon (repurposed Bloodborne "Insight" menu graphic)
│   │   └── MainWindowComponents/          all of MainWindow, split into 6 partial-class files
│   │       ├── MainWindow.xaml / .xaml.cs main window layout + constructor + several randomization helper methods
│   │       ├── UIComponents.cs            every XAML-wired event handler (checkboxes, buttons, sliders, textboxes)
│   │       ├── FieldContainer.cs          flat declaration of ~150-180 shared mutable fields ("the settings model")
│   │       ├── BooleanHandler.cs          SetBooleans() — bulk snapshot of checkbox state into fields, run-start UI lock
│   │       ├── StartFunctions.cs          DoSomething()/DoSomething2() — the entire run orchestration (2,574-line method)
│   │       └── RandomizeFunctions.cs      the 17 randomization/normalization algorithms themselves
│   └── SoulsFormats/                      vendored third-party library (JKAnderson/SoulsFormats)
│       ├── SoulsFormats.sln
│       └── SoulsFormats/                  168 source files covering DCX, BND3/BND4/BXF3/BXF4, MSB1/2/3/B/D/N/S,
│                                          PARAM/PARAMDEF, FMG, EMEVD, FLVER, TAE, HKX — only a small slice is used
├── app/                                   (untracked; the PS4 homebrew port — not in scope, not touched)
├── docs/                                  (untracked; this document and the port's specs and plans)
└── data/                                  (gitignored; vanilla baseline, prior run output, the extracted Nexus release)
```

**Two projects, one solution.** `Randomizer.csproj` is the entire application; `SoulsFormats.csproj` is a general-purpose, multi-game FromSoftware file-format library (`netstandard2.1`) referenced as a `ProjectReference` (`Randomizer.csproj:21`). SoulsFormats is not Bloodborne-specific — it also supports Dark Souls 1/2/3, Demon's Souls, Sekiro, and Elden Ring-era formats to varying degrees; the randomizer uses only a narrow slice of it (§8).

The `app/` directory is untracked and `/data/` is gitignored; both are local working files, not part of the reviewed application. The `data/runs/*/dvdroot_ps4/` trees are useful corroborating evidence of the real on-disk file layout the app expects (e.g. `dvdroot_ps4/paramdef/paramdef.paramdefbnd.dcx` is present exactly where the code expects it), but they were not analyzed as part of this review beyond that spot-check.

---

## 3. Architecture Overview

The requested "layered" diagram does not match this codebase — there is no enforced layering. The actual shape is:

```
App.xaml (StartupUri) ──▶ MainWindow (WPF Window, shown automatically)
                              │
                              ├─ MainWindow.xaml.cs   constructor: static exclusion-list setup, one blocking
                              │                        startup MessageBox, some randomization helper methods
                              ├─ UIComponents.cs       every control's event handler; TEST_Click is the "Randomize" button
                              ├─ FieldContainer.cs     ~150-180 shared mutable fields — the de facto "settings object"
                              ├─ BooleanHandler.cs     SetBooleans(): bulk-reads checkboxes into fields, locks UI
                              │
                              │   TEST_Click spawns two raw Threads:
                              ▼
                        StartFunctions.cs: DoSomething()      StartFunctions.cs: DoSomething2()
                        (2,574-line orchestration method)     (1-second UI stopwatch ticker; no game-data role)
                              │
                              │  seeds universalRand/keyitemRand, resolves dvdroot_ps4 path,
                              │  restores .bak files, then calls into:
                              ▼
                        RandomizeFunctions.cs
                        (17 independent randomization/normalization routines,
                         each: MSBB.Read or PARAM/BND4 read → mutate in memory → write back to the same path)
                              │
                              ▼
                        SoulsFormats (vendored library)
                        MSBB / BND4 / PARAM / PARAMDEF / DCX (transparent, via SoulsFile<T> base class)
                              │
                              ▼
                        dvdroot_ps4/ on disk — modified in place, .bak siblings kept alongside
```

Every box in the middle three rows is **the same C# object instance** (`this`, the `MainWindow`), because every file is `partial class MainWindow`. There is no method-call boundary carrying a settings/config parameter object between "UI" and "orchestration" and "randomization" — they all read and write the same instance fields directly. **Fact**, established independently by all three files' opening `partial class MainWindow` declarations (`UIComponents.cs:11`, `FieldContainer.cs:7`, `BooleanHandler.cs:3`, `StartFunctions.cs:11`, `RandomizeFunctions.cs:9`, `MainWindow.xaml.cs`).

This is the most consequential architectural fact for a reimplementation: **there is no existing settings/config boundary to reuse or mirror structurally.** A PS4 port will need to design that boundary from scratch (see §12).

---

## 4. End-to-End Randomization Flow

### Narrative

1. **App launch.** `App.xaml:5`'s `StartupUri="MainWindowComponents/MainWindow.xaml"` is the entirety of the startup mechanism — `App.xaml.cs:14-16` is an empty `partial class App : Application { }`. No command-line arguments, no splash screen, no pre-flight validation of the game folder.

2. **`MainWindow` constructor** (`MainWindow.xaml.cs:14-16` calls `InitializeComponent()`, then continuation through roughly line 208):
   - Syncs 4 of the 14 per-area percentage `Label`s from their field defaults (`:22-25`); the other 10 rely on XAML literal text happening to match the field default (a latent trap — see §13).
   - Shows a **blocking** `MessageBox` explaining the enemy-size slider mechanic (`:26-34`, quoted in full in §6.1).
   - Force-disables `RandomizeKeyItemsBox` (`:35`) — a shipped-but-turned-off feature.
   - Populates three static hardcoded lookup lists — `chaliceBossParams`, `unusedList`, `bossList` — merged into `unusedPlusBossList` (`:54-208`). These are exclusion/classification tables, not user settings.

3. **User configures checkboxes/sliders/textboxes.** Each control has its own one-line `Checked`/`ValueChanged`/`TextChanged` handler in `UIComponents.cs` that copies the control's value into one field in `FieldContainer.cs` — e.g. `RandomizeEnemiesCheck_Checked` sets `randomizeEnemiesBool` (`UIComponents.cs:113-116`). There is no shared/generic control-to-field mapping mechanism; every one is hand-written individually.

4. **User clicks "RANDOMIZE" (`TEST` button).** `TEST_Click` (`UIComponents.cs:147-158`):
   - Parses the seed textbox (truncated to 10 chars) as an `int`; on failure, generates `new Random().Next()` and writes it back into the textbox so the user can see/reuse it.
   - Starts two `System.Threading.Thread`s: `DoSomething` (the real work) and `DoSomething2` (a UI stopwatch).

5. **`DoSomething()` — the entire pipeline** (`StartFunctions.cs:32-2606`, one method, run in order):
   - Seeds `universalRand = new Random(seed)` and `keyitemRand = new Random(seed)` (`:34,38`) — see §7 for why the second is dead.
   - Marshals `SetBooleans()` onto the UI thread (`:40-43`) — the authoritative, run-start snapshot of every checkbox into its field, and simultaneously disables ~90 controls so the user can't change settings mid-run (no cancel button exists anywhere).
   - Resolves `filePath = <exe's parent dir>\dvdroot_ps4` (`:52-54`) — pure path convention, no folder browser, no existence check.
   - Loads plain-text auxiliary data the *tool* ships (not game data): `NPCScalingFile.txt`, `NPCParamsFile.txt`, `Names.txt`, `Sizes.txt` from `dvdroot_ps4\Mod Files\NPC Scaling File\` (`:56-89`).
   - Builds `mapList` (24 base maps + ~19 chalice-dungeon map paths) and `eventFileList` (16 emevd paths) from **literal hardcoded strings**, not filesystem enumeration (`:100-164`), with one conditional extra map gated on `File.Exists` (`GOBAdded`, `:143-147`).
   - Creates (if `logging`, hardcoded `true`) several empty timestamped log files (`:166-202`).
   - **Backup/restore cycle** for `gameparam.parambnd.dcx`, all 16 event files, and all ~43 map files: if a `.bak` sibling exists, delete the live file and copy `.bak` back over it (restoring vanilla state), then re-create `.bak` from that now-vanilla file (`:234-307`). This is the mechanism that makes re-running the tool against the same folder idempotent/deterministic.
   - **Cross-map model sync pass** (`:310-383`) — the first real *write*: reads every map's `Models.Enemies`, unions them into a master list, then re-reads and rewrites every map so each map's `Models` section lists every enemy model that could possibly be randomized into it (otherwise a placed enemy could reference a model entry the map doesn't declare).
   - Dispatches, in strict sequential order, to the individual randomization routines in `RandomizeFunctions.cs`/`MainWindow.xaml.cs`, each gated by its own boolean and each updating a progress bar / task label via `Dispatcher.Invoke`: enemy list generation → enemy randomization → boss list generation → boss randomization → boss insertion into enemy slots → NPC list/randomization → "easy mode" fight patches → item-lot randomization → perma-darkness event patch (unconditional — see §13) → melee/gun moveset randomization → item drops → shop items → AI sound normalization → team-type normalization → face/talk/blood/gem param randomizers → enemy/boss stat scaling.
   - **Final size recompute** (dead computation, discarded) and **completion**: progress → 100%, `MessageBox.Show($"Finished. Seed: {seed}.")` (`:2575`), UI reset, then **`Environment.Exit(0)`** (`:2605`) — the process terminates unconditionally right after re-enabling controls the user will never get to interact with.

6. **Individual randomization routines** (`RandomizeFunctions.cs`) each independently: `MSBB.Read`/`PARAM`+`BND4` read the relevant file(s), mutate objects in memory, then `Write()` back to the identical path. There is no batching, no shared transaction, no in-memory-only "preview" — every category writes to disk as it completes.

7. **Output.** The user is told to find logs in `dvdroot_ps4\Mod Files\Logs. Don't Delete\` (static XAML label, `MainWindow.xaml:43`). There is no separate "output" folder to copy anywhere — the same `dvdroot_ps4` tree the tool was pointed at **is** the final AFR-ready output, since it was modified in place.

### Execution-flow diagram

```
User clicks TEST ("RANDOMIZE")
   │
   ▼
TEST_Click (UIComponents.cs:147)  — resolve/generate seed
   │
   ├──▶ Thread: DoSomething2()  (UI stopwatch, no game-data role)
   │
   └──▶ Thread: DoSomething()  (StartFunctions.cs:32)
           │
           ├─ seed universalRand, keyitemRand (dead)
           ├─ SetBooleans() [Dispatcher.Invoke]         — authoritative settings snapshot
           ├─ resolve dvdroot_ps4 path (convention only)
           ├─ load tool's own NPC-scaling text files
           ├─ build mapList/eventFileList (hardcoded literals)
           ├─ restore-then-rebackup: params, events, maps  (.bak cycle ⇒ guaranteed vanilla baseline)
           ├─ cross-map model-sync write                    (first real MSBB.Write)
           │
           ├─ [if oopsAll]      OopsAll / OopsAllBoss        special single-species mode
           ├─ [else]            GenerateEnemyList → Randomize (per map)         ─┐
           │                    GenerateBossList → RandomizeBosses (per map)     │  each: MSBB.Read →
           │                    InsertBossesVoid (per eligible map)              │  mutate → MSBB.Write
           │                    GenerateNPCList → RandomizeNPCs (per map)        │
           │                    EasyModes (per gated fight)                     ─┘
           ├─ RandomizeItemLots (per map, MSBB Treasure events)
           ├─ PermaDarknessFunction (common.emevd.dcx) — unconditional, see §13
           ├─ RandomizeMeleeMoveset / RandomizeGunMoveset         ─┐
           ├─ RandomizeItemDrops / RandomizeShopItems              │ each: BND4 read →
           ├─ AiSoundParamRandomizer / TeamTypeRando                │ PARAM/PARAMDEF →
           ├─ VfxRandomizer(Face) / DecalRandomizer / TalkRandomizer│ mutate rows/cells →
           ├─ GemGenParamRandomizer                                │ parambnd.Write
           ├─ ParamScalingForBosses (per map)                     ─┘
           │
           ▼
   dvdroot_ps4/ tree — modified in place, .bak siblings retained
           │
           ▼
   MessageBox "Finished. Seed: N."  →  Environment.Exit(0)
```

---

## 5. Randomization Category Analysis

All line numbers below are in `reference/Randomizer/MainWindowComponents/RandomizeFunctions.cs` unless otherwise noted. "Live" means the code path actually executes in a normal run; several categories below are **not** live and are labeled accordingly — this is a load-bearing distinction for scoping a port.

### 5.1 Enemy Placement — `Randomize` (`:11-602`) — **live**

- **Player-facing effect (inference)**: regular (non-boss) enemy spawns in each map are replaced with a different enemy's identity, with per-region "what % of enemies in this zone get touched" sliders and an optional file-size-based compatibility filter.
- **Reads**: `MSBB.Read(currentMap).Parts.Enemies` — `.Name`, `.NPCParamID`, `.ThinkParamID`, `.ModelName`.
- **Writes**: `.NPCParamID`, `.ThinkParamID`, `.ModelName` mutated in place; `tempGUY.Write(currentMap)` (`:600`) — one write per map.
- **Selection**: raw `universalRand.Next(0, pool.Count)` against `enemyDataRandomized` (normal) or `chaliceEnemiesString` (chalice-chance roll) or `oopsAllEnemyString` (single-species mode). Enemy identity travels as one atomic `"NPCParamID*ThinkParamID*ModelName"` string, parsed by index/substring (`:355-358`).
- **Constrained by**: a size-compatibility reroll loop (bounded to 30 tries for two specific maps, 30,000 for everything else — an intentional-looking but unexplained asymmetry, `:379-466`); per-map hardcoded model-substring exclusions for `m24_02` and `m35`, both **unbounded** while-loops (`:468-496` — open question, possible infinite loop if the pool can't satisfy the exclusion); per-region percentage-chance gates for 13 named zones (`:51-287`); a chalice-dungeon (`m29`) branch that excludes `chaliceBossParams` and any `"1050"`-containing model; and a hardcoded 6-entry force-include list specific to map `m28` (`:289-320`).
- **Dependency**: entirely dependent on pools (`enemyDataRandomized`, `chaliceEnemiesString`, `nameList`/`sizeList`) built elsewhere in `StartFunctions.cs`/`MainWindow.xaml.cs` — cannot be understood or exercised in isolation.
- **Known bug**: `:538-544` (the `oopsAll` branch) loops `unusedList.Count` times but indexes `nonoList[j]` — a list/index mismatch that either throws or silently truncates, depending on relative list sizes.

### 5.2 Shop / Starting Weapons — `RandomizeShopItems` (`:609-1315`) — **live**

- **Player-facing effect (inference)**: randomizes the Hunter's Dream shop's weapon/armor/consumable lineup; optionally forces the starting-class weapon/gun choices to be scrambled while patching their stat requirements so the randomized weapon stays usable at level 1 (and across its first ~10 upgrade tiers).
- **Reads/writes**: `ShopLineupParam` and `EquipParamWeapon`, via `BND4.Read` of `paramdef.paramdefbnd.dcx` and `gameparam.parambnd.dcx` (`:611-628`).
- **Selection**: shop slots are bucketed by category (weapon/armor/consumable via `Cells[7].Value`, `:847-874`), then drawn **without replacement** (`RemoveAt` after each pick) — no in-category duplicates unless the pool is exhausted.
- **Constrained**: starting right-hand weapon slots reroll until the pick is itself in a hardcoded ~89-entry `rightHandList`; starting gun slots reroll against a 16-entry `leftHandList`. `startingWeaponsOnlyBool` gates a follow-up pass that overwrites stat-requirement cells with fixed low values matching vanilla-starter difficulty, applied to the base weapon and its first ~10 "+N" variants (ID + 100 per tier, a Bloodborne-specific convention).
- **Known bug**: `rightHandList.Add("/29000000")` (`:794`) — a stray leading `/` means this weapon ID can never actually match, silently shrinking the intended pool by one.

### 5.3 NPC Randomization — `GenerateNPCList` (`:1316-1327`) + `RandomizeNPCs` (`:1329-1458`) — **live, but likely broken pipeline (see below)**

- **Player-facing effect (inference)**: swaps the outfit/appearance variant of human NPCs (all share model `c0000`) for a different variant, with hostile NPCs (invaders) drawn from a separate pool.
- **Data quirk, not a bug**: outfit variant lives in `.UnkT07` (an unlabeled SoulsFormats field), parsed as a 4-character token — distinct from the 5-character `cXXXX` convention used everywhere else in the file.
- **Open question / likely gap**: `GenerateNPCList` only populates `npcEnemyList` (a list of `MSBB.Part.Enemy` objects); `RandomizeNPCs` reads entirely different fields, `npcList`/`hostileNpcList` (string-encoded pools). No code in this file converts one into the other — either that conversion happens elsewhere (unverified) or `GenerateNPCList`'s output is currently unused. Flagged in §13, not resolved here.
- **Known bug**: the hostile-NPC reroll loop (`:1356-1360`) overwrites `thisentityID` — a field otherwise used elsewhere for entity-ID bookkeeping — with an enemy-identity string, looking like a copy-paste artifact.

### 5.4 Boss Insertion into Regular Slots — `InsertBossesVoid` (`:1460-1552`) — **live**

- **Player-facing effect (inference)**: gives ordinary enemy spawns a percentage chance (global `bossPercentage` slider, rolled via `universalRand.Next(0,101) &lt;= bossPercentage`) to instead spawn as a boss-tier enemy — extra, unscripted boss encounters layered on top of normal exploration.
- **Known bug**: for `NPCParamID == 551000`, the code computes `tempThinkIdInt = 551111` but never assigns it — the enemy's `.ThinkParamID` is left unchanged from the original enemy it replaced (`:1538-1545`), an apparently-incomplete fix for one specific NPC.
- No pool-removal — the same boss can be inserted repeatedly across a map/run.

### 5.5 Boss Pool Construction — `GenerateBossList` (`:1554-1847`) — **live (data-gathering, not itself random)**

Scans every map for boss candidates (matched against a `bossList` name-substring table) and applies an extensive set of hardcoded exclusions to filter out placeholder rows, decoy/cut content, and secondary hit-boxes belonging to multi-part bosses (full ID lists in the source report; representative: `NPCParamID ∈ {250060,250070,250090,212600,212610,212620}`, and name-substring exclusions for `c2500_0000, c5071_0000, c4030_0001..0004, c2120_0000..0002, c2570, c2571`, etc.). Builds the serialized pools (`enemyData`, `insertBossesString`) and the parallel chalice-dungeon boss pools (`chaliceBossString`, `newChaliceBossString`/`newChaliceBossString2` — the latter pair used later as a "refill" source once the primary pool is exhausted). Tracks one specific entity (`c4540_0000`) as `OoKEnemy` — **inference**: very likely the Orphan of Kos, given later cross-map synchronization logic (§5.6) — but this identification is inferred from map/ID correlation, not stated in the code.

### 5.6 Orphan-Phase Sync — `AddOrphanPhaseOne` (`:1849-2011`) — **live (consistency patch, not randomization)**

For three hardcoded map/entity pairs (`m24_01`→`c2710_0000`, `m24_02`→`c2500_0000`, `m34_00`→`c4510_0000`), forces that entity's identity to match whatever was randomly assigned to the tracked `OoKEnemy` (§5.5) — keeping a "first phase" placeholder consistent with the actual randomized boss across multiple maps. Writes the map twice in a row (redundant but harmless).

### 5.7 Boss Randomization — `RandomizeBosses` (`:2012-2692`) — **live**

- **Player-facing effect (inference)**: swaps each qualifying boss for a different boss, with extensive special-casing to keep multi-entity fights internally consistent.
- **Selection**: chalice-dungeon branch removes all pool entries sharing a model name after a pick (avoiding duplicate multi-variant selection) and refills from a secondary pool when exhausted; normal-map branch has additional map-specific exclusion blacklists (e.g. `m24_00` excludes Vicar-Amelia-class models) and self-match reroll loops that are **not actually bounded** despite an `addCounter` that looks like a cap but is never checked for termination (`:2354-2360`).
- **Multi-entity sync fix-ups**, hardcoded by exact name and given inline labels by the original author: "Emissary fix," "wet nurse fix," "maria fix," "living failures fix" (`:2374-2431`, duplicated for the `oopsAllBosses` branch at `:2601-2658`) — each propagates one boss's newly-picked identity to its 1-2 companion MSB entities (extra limbs/adds/clones) so a multi-part fight doesn't end up half-vanilla.
- **Known drift**: the exclusion conditions here are a near-duplicate of, but not identical to, `GenerateBossList`'s (§5.5) — the two lists should logically stay in sync but currently don't, a maintenance hazard called out explicitly for anyone re-deriving these rules.
- **Known bug**: `NPCParamID == 507200` forces `.ThinkParamID = 507200` regardless of the drawn boss's own think ID — deliberate-looking but undocumented.

### 5.8 Melee/Gun Moveset — `RandomizeMeleeMoveset` (`:2694-2798`) / `RandomizeGunMoveset` (`:2800-2904`) — **live**

Structurally identical (per-PARAM-column, without-replacement Fisher-Yates shuffle across rows), but with **inverted semantics** for their shared `rowsToReplace` parameter: melee treats it as a blacklist (randomize everything except these rows), gun treats it as a whitelist (randomize only these rows). Nothing in the code documents this inversion — a port that assumes symmetry between the two would silently produce wrong output for one of them.

### 5.9 Item Drops — `RandomizeItemDrops` (`:2906-3172`) — **live implementation present, but a full alternate implementation is dead/commented out above it**

Excludes two hardcoded `NpcParam` IDs entirely (`252100`, `6071`); builds a pool from both `ItemLot1` and `ItemLot2` columns but **writes back only to column 1** — column 2 is read into the shared pool but never reassigned, an inconsistency. Selection uses `universalRand.Next(0, itemLotList.Count - 1)`, which **excludes the last pool element from ever being selected** — an off-by-one bug that recurs six more times in §5.14 (Gem/Rune). Self-match reroll has no pool removal and no iteration cap (possible infinite loop if the pool degenerates to one distinct value).

### 5.10 Key Item Randomization — `RandomizeKeys` (`:3174-3262`) — **not functional (stub)**

Builds a real 14-entry map list with correct real-world zone names in comments, picks a random map, then `switch`es on it with **every case body empty**. This function currently does nothing observable. Confirms the git-history commit "added item randomization" / the `RandomizeKeyItemsBox` UI checkbox refers to a feature that is present in scaffolding only and force-disabled in the UI (`MainWindow.xaml.cs:35`).

### 5.11 Team-Type Normalization — `TeamTypeRando` (`:3264-3304`) — **live, but not random**

Despite its name and placement among the randomizers, this contains **no RNG call at all** — it unconditionally sets every `NpcParam` row's team/faction cell to a fixed value (`25`). **Inference**: this exists to force every NPC onto one hostile-to-player faction so that randomized enemy placements don't end up passive/neutral toward the player due to faction mismatches. This should be understood by a porting team as a deterministic data-normalization step, not a source of run-to-run variety.

### 5.12 Treasure/Item Lot Randomization — `RandomizeItemLots` (`:3306-3358`) — **live**

Distinct from §5.9: operates on map-embedded `Events.Treasures` (open-world/corpse pickups) rather than enemy drop tables. Correctly implemented without-replacement sampling (no off-by-one here), with a caller-supplied exclusion list (`nonoItemLots`).

### 5.13 AI Sound Normalization — `AiSoundParamRandomizer` (`:3430-4550`) — **live, but not random**

A 62-case `switch` **on row position** (not row ID) that resets each `AiSoundParam` row to a fixed, hand-tuned 7-value set. This is fragile by construction (row-order-dependent) and, like §5.11, is a deterministic rebalance pass mislabeled as a randomizer. Why this normalization is needed alongside enemy-identity randomization is not evident from this file (open question, §13).

### 5.14 Decal / Talk / VFX Param Randomizers — `DecalRandomizer` (`:4552-4631`), `TalkRandomizer` (`:4633-4712`), `VfxRandomizer` (`:5129-5251`, parameterized by param name, called for `FaceGenParam`/`FaceParam`) — **live**

All three share one algorithm, structurally different from every identity-swap category above: for each **column** (field) of the param, gather that field's value from every row and shuffle it independently, then reassemble. The result is a chimera — a given output row's fields are drawn from many different original rows, not a pairwise "this enemy's whole record swapped with that one's." This is the file's second fundamentally distinct randomization *shape* (column-independent shuffle vs. atomic identity swap) and must be preserved as a distinct concept in any reimplementation, not folded into a single generic "randomize this param" routine.

### 5.15 Bullet Randomization — `BulletRandomizer` (`:4714-5127`) — **mostly dead**

A ~300-line candidate ID list and a full shuffle implementation are both commented out. The live code path does a small fixed substitution (three specific cannon-bullet rows replaced with one specific bullet definition) — not randomization in the shipped build.

### 5.16 Gem/Rune Generation — `GemGenParamRandomizer` (`:5253-5509`) — **live**

Randomizes both **how many** gem/rune slots an enemy's drop has (probability-banded roll, with `sixGemBool`/`threeGemBool` UI overrides) and **which** gem definitions fill each slot. **Known bugs**: the natural roll's `== 101` six-slot branch is unreachable given `Random.Next(0,101)`'s exclusive upper bound (only the explicit `sixGemBool` override can produce 6 slots); the same off-by-one pool-exclusion pattern as §5.9 recurs six times; and a concrete double-write bug (`:5477-5478`) writes `Cells[26]` twice (once from `genThree`, immediately clobbered by `genFour`) while `Cells[27]` is never written — an exact carry-forward of a typo present in the dead, commented-out predecessor code, meaning it went unnoticed through at least one prior revision.

### 5.17 Enemy/Boss Stat Scaling — `ParamScalingForBosses` (`MainWindow.xaml.cs:224-732`, called from `StartFunctions.cs`, not `RandomizeFunctions.cs`)

Not analyzed in the same depth as the above (owned by the UI-layer research pass), but two facts are established: it is driven by 15 per-area "scale index" fields (`dreamScale`, `hemwickScale`, etc.), and the UI meant to let the user customize those indices (`Window1`, the "Scaling Window") is **non-functional** — its `CloseScalingWindowButton_Click` handler has its entire propagation-to-`MainWindow` logic commented out (`Window1.xaml.cs:65-99`), so those 15 fields always take their hardcoded default values (`StartFunctions.cs:1605-1619`) regardless of what the user selects in that window. **Open question**: whether `ParamScalingForBosses` is skipped entirely when `noScalingBool` is set is established (`StartFunctions.cs:1601`), but whether the *values* it applies ever meaningfully vary at all in practice (given the dead customization UI) was not independently re-verified against `MainWindow.xaml.cs:224-732`'s internals in this pass.

### Category interdependency summary

- **Enemy → Boss**: `GenerateBossList` and boss randomization must run after (or produce lists independent of) enemy list generation; `InsertBossesVoid` explicitly depends on the boss pool existing.
- **Boss ↔ Orphan-phase sync**: `AddOrphanPhaseOne` depends on `GenerateBossList` having already tagged `OoKEnemy`, and must run before/interleaved with `RandomizeBosses` for the sync to be meaningful.
- **Shop → Starting weapons**: the stat-requirement rewrite pass inside `RandomizeShopItems` strictly depends on that same function's earlier shop-slot assignment having already run in the same invocation (shared local/field state, not re-enterable safely).
- **Cross-map model sync** (`StartFunctions.cs:310-383`) must run *before* enemy randomization, since it's what guarantees a map's `Models` section can represent whatever enemy `Randomize` later places there.
- Most other categories (movesets, item drops, decals, talk, VFX, AI sound, team-type, gems) are **independent** of each other and of the enemy/boss/NPC pipeline — they touch disjoint PARAM tables.

---

## 6. Bloodborne Data / File Format Analysis

### Formats actually used by the application

| Format | Role | Reader/Writer | Confirmed usage |
|---|---|---|---|
| **MSBB** (Bloodborne MSB variant) | Map layout: enemy/boss/NPC placements, treasure events, model lists | `SoulsFormats.MSBB` (`reference/SoulsFormats/Formats/MSB\MSBB\*.cs`) | `MSBB.Read(path)` / `.Write(path)` throughout `StartFunctions.cs` and `RandomizeFunctions.cs`, e.g. `RandomizeFunctions.cs:15`, `StartFunctions.cs:208,320,353` |
| **DCX** (compression) | Wraps almost every file the tool touches (`.msb.dcx`, `.parambnd.dcx`, `.paramdefbnd.dcx`, `.emevd.dcx`) | `SoulsFormats.DCX`, invoked **transparently** inside `SoulsFile<T>.Read`/`.Write` — no direct `DCX.Decompress`/`Compress` call appears anywhere in the app | Implicit in every `MSBB.Read`/`BND4.Read` call on a `.dcx` path |
| **BND4** (archive) | Container for `gameparam.parambnd.dcx` and `paramdef.paramdefbnd.dcx` | `SoulsFormats.BND4` | e.g. `RandomizeFunctions.cs:612,620` (and ~10 more near-identical call sites) |
| **PARAM / PARAMDEF** | Tabular game-balance/config data (weapon stats, shop lineups, NPC AI params, decals, VFX, gems, etc.) | `SoulsFormats.PARAM`, `SoulsFormats.PARAMDEF` | Loaded from the `BND4` files above, e.g. `RandomizeFunctions.cs:611-631` (`ShopLineupParam`, `EquipParamWeapon`) |

### Formats present in the vendored library but confirmed **unused** by the app

FMG (text/message files), EMEVD's own parsed object model (event files are backed up/restored as raw bytes via `File.Copy`, never opened through `SoulsFormats.EMEVD`), BND3/BXF3/BXF4, FLVER (3D models), TAE (animation timing), HKX (havok physics/collision), GPARAM, and everything else in `reference/SoulsFormats/Formats/` outside the four rows above. This was confirmed by repository-wide grep for each type name within `reference/Randomizer/` returning no construction/`.Read` call sites. **Porting note**: a PS4 native reimplementation only needs MSBB + DCX + a BND4-family reader + PARAM/PARAMDEF to match this app's actual file-format footprint — the rest of SoulsFormats' surface area is not a requirement unless new features are planned.

### Concrete traced example

A particular map's enemy randomization begins at `StartFunctions.cs:320` (`MSBB.Read(mapList[i])`, part of the cross-map model-sync pass) and, later in the pipeline, `RandomizeFunctions.cs:15` (`MSBB.Read(currentMap)` inside `Randomize`) reads the same map again fresh. The function iterates `tempGUY.Parts.Enemies`, and for each record not excluded, replaces `.NPCParamID`, `.ThinkParamID`, and `.ModelName` together (parsed from a `"*"`-joined string drawn from a pre-built pool), then calls `tempGUY.Write(currentMap)` (`RandomizeFunctions.cs:600`) — writing the same `.msb.dcx` path it opened, with DCX recompression handled transparently by the `SoulsFile<T>` base class. A concrete hardcoded path from the source confirms the real directory convention: `MSBB.Read(filePath + "\map\mapstudio\" + "\m29_52_01_00\m29_52_01_91.msb.dcx")` (`StartFunctions.cs:353`).

For the param side: `RandomizeShopItems` (`RandomizeFunctions.cs:609-1315`) opens `paramdef.paramdefbnd.dcx` via `BND4.Read(paramDefPath)` (`:612`) and `gameparam.parambnd.dcx` via `BND4.Read(paramPath)` (`:620`), applies the paramdefs to get typed `PARAM` objects, mutates `ShopLineupParam`/`EquipParamWeapon` rows, then serializes each modified `PARAM` back into its `BinderFile.Bytes` and calls `parambnd.Write(paramPath)` (`:1313`) — the whole `BND4` archive is rewritten as a unit even though only two of its member params changed.

**Note on data-model completeness**: no Bloodborne-specific data (enemy names, item names, param field meanings) is bundled as external resource files in the `reference/Randomizer` project — the only non-code asset is one PNG icon. Every piece of Bloodborne domain knowledge (enemy IDs, boss names, exclusion lists, weapon-ID-to-name comments) is a **hardcoded literal inside the C# source**, not loaded from a data file the tool ships. This is a significant scoping fact for a port: there is no "data file" to just copy over — the actual knowledge must be extracted from the C# source itself (§5's hardcoded-list citations are the closest thing to that extraction currently available).

---

## 7. Seed and Determinism Analysis

**Fact — seed acquisition** (`UIComponents.cs:150-155`, `TEST_Click`): the seed textbox's text (truncated to its first 10 characters) is parsed as `int`; if parsing fails (empty, non-numeric, or a value outside `int` range), `seed = new Random().Next()` (system-time-seeded) is used instead and written back into the textbox so the user can see and later re-enter the seed that was actually used.

**Fact — RNG construction** (`StartFunctions.cs:34,38`):
```csharp
universalRand = new Random(seed);
keyitemRand = new Random(seed);
```
Both instances are constructed from the **same** integer seed. An explanatory comment (`StartFunctions.cs:36-37`) states the original intent: `keyitemRand` was meant to let key-item placement stay stable even if other settings changed between runs, "in case one \[randomizer\] crashes." **Fact**: `keyitemRand.Next()` is never called anywhere in the project (confirmed by project-wide search) — this second RNG stream is entirely vestigial in the current codebase.

**Fact — single shared RNG in practice**: every live randomization category draws exclusively from `universalRand` (`FieldContainer.cs:34`), a single `System.Random` instance shared across the whole run. All consumption is strictly sequential on one thread (`DoSomething`'s own thread) — no `Task`/`Parallel`/async code touches randomization state, so there is no thread-interleaving nondeterminism.

**Determinism, with three caveats:**

1. **List-construction order is hardcoded, not filesystem-derived**, for the overwhelming majority of pools (`mapList`, `eventFileList`, exclusion tables, weapon-ID lists) — they're built from literal `.Add()` calls in a fixed source-code order, not `Directory.GetFiles()` enumeration (the one live `Directory.GetFiles` call sits inside dead/commented code, `StartFunctions.cs:331` region). This means, for a fixed version of the tool and a fixed settings combination, the *sequence* of `universalRand.Next()` calls is fully determined by the seed alone.
2. **One filesystem-dependent exception**: `GOBAdded` (`StartFunctions.cs:143-147`) is set by `File.Exists` on one specific optional chalice map file. If present, it changes list sizes (and therefore the total number and sequence of RNG draws) for that run. Two installs of "the same game" that differ only in whether this one file is present will diverge in output for the *same* seed — an environment-dependent, not seed-dependent, determinism wrinkle.
3. **`System.Random` is not a specified, versioned algorithm** — .NET does not guarantee `System.Random`'s output sequence is stable across all .NET versions/platforms (behavior has in fact changed between .NET Framework and modern .NET for a given seed, historically). The Windows app runs on `netcoreapp3.1`. **Porting note**: a native C++ PS4 implementation cannot rely on matching `System.Random`'s internal algorithm implicitly — if bit-for-bit seed compatibility with the Windows tool's output is ever a goal, the specific PRNG algorithm and exact call sequence would both need to be reimplemented deliberately; if it is not a goal, the PS4 version is free to choose any well-specified PRNG, but should still pin down its own algorithm explicitly (e.g., a documented PCG/xorshift/splitmix) so that *its own* seed-reproducibility guarantee is well-defined and testable.

**Fact — "same seed, same settings ⇒ same output" holds within a single install/version**, modulo caveat 2. This is directly useful as a black-box acceptance test as the memory/task notes suggest, *provided* the test harness controls for the `GOBAdded`-triggering file's presence and pins the exact tool build.

**Execution order affecting results**: yes — because every category draws from the same shared `universalRand` sequentially, the *order in which randomization categories run* (hardcoded in `DoSomething()`, §4) directly determines which random values each category consumes. A reimplementation that runs categories in a different order, or that changes how many `.Next()` calls a given category makes internally (e.g., fixing the off-by-one bugs in §5.9/§5.16, which don't change call count, versus adding new reroll bounds, which could), will diverge from the existing tool's output for the same seed even if every individual algorithm is otherwise faithfully reproduced. This should be treated as a first-class design constraint if seed-parity with the Windows tool is ever desired, and as an explicit non-goal (with that stated plainly) if it is not.

---

## 8. Settings and Configuration Inventory

There is no settings *object* — every row below is one independent field in `FieldContainer.cs`, populated via one hand-written UI handler in `UIComponents.cs` and/or the bulk snapshot `SetBooleans()` (`BooleanHandler.cs:5-204`). "Implementation location" cites where the setting is *read* by the randomization pipeline; most fields are declared in `FieldContainer.cs` and that is omitted below for brevity except where notable.

| Setting (UI control) | User-facing purpose | Type | Default | Read at | Notes |
|---|---|---|---|---|---|
| `RandomizeEnemiesCheck` | Randomize enemies (excl. chalice) | bool | unchecked | `StartFunctions.cs:582` (gates shuffle), `Randomize` | |
| `ChaliceEnemies` | Include chalice enemies in pool | bool | unchecked | `StartFunctions.cs:501,610,663` | |
| `BossCheckBox` | Randomize bosses (excl. chalice) | bool | unchecked | also sets `chaliceBosses` (`BooleanHandler.cs:9`) | see `ChaliceBoss` note below |
| `ChaliceBoss` | (label) Include chalice bosses | bool | unchecked | **no-op** — `Checked` handler body commented out (`UIComponents.cs:25-28`) | dead control; `chaliceBosses` is actually driven by `BossCheckBox` |
| `InsertBosses` | Bosses can replace regular enemies | bool | unchecked | `StartFunctions.cs:1013` | shows a crash-risk warning MessageBox when checked |
| `ChaliceBossBox` | Bosses can replace enemies (chalice) | bool | unchecked | per-area boss-insertion gating | |
| `OopsAllCheck` + `OopsAllStringName` | Force every enemy to one user-typed species | bool + string | unchecked / empty | `StartFunctions.cs:385-466`, `Randomize` oopsAll branch | mutually exclusive with `ExEnemyBox` in the UI |
| `OopsAllBossesCheck` + `OopsBoss` | Force every boss to one user-typed species | bool + string | unchecked / empty | `RandomizeBosses` oopsAllBosses branch | mutually exclusive with `ExBossBox` |
| `ExEnemyBox` / `ExBossBox` | Exclude typed enemies/bosses from the pool instead | bool | unchecked | `excludeEnemiesBool`/`excludeBossesBool`, `StartFunctions.cs:536-577,714-823` | |
| `AddNPCS` | Randomize NPCs | bool | unchecked | `StartFunctions.cs:1132` | pipeline gap noted in §5.3 |
| `BellMaidenBox` | Keep Bell Maidens unchanged | bool | unchecked | `StartFunctions.cs:45-50` | adds 3 IDs to exclusion list |
| `LesserBossesBox` | Include group-fight members as single bosses | bool | unchecked | `GenerateBossList:1631-1743` | |
| `ArmorRandomizerCheckBox` | Randomize non-key overworld items | bool | unchecked | feeds `RandomizeItemLots` gating | |
| `RandomizeKeyItemsBox` | Randomize key items (with logic) | bool | unchecked | **force-disabled** (`MainWindow.xaml.cs:35`); backing feature (`RandomizeKeys`) is a non-functional stub | |
| `RandomizeShopBox` | Randomize shop items | bool | unchecked | `StartFunctions.cs:1541-1544` (`shopBool`) | |
| `EnemyDropBox` | Randomize enemy item drops | bool | unchecked | `StartFunctions.cs:1536-1539` | |
| `WorkshopBox` | Randomize workshop tools | bool | unchecked | `StartFunctions.cs:1277-1283` | |
| `KeepGunsBox` | Keep vanilla starting guns | bool | unchecked | shop/starting-weapon logic | |
| `StartingWeaponsRandomizeBox` | Randomize starting weapons | bool | unchecked | `startingWeaponsOnlyBool`, gates stat-rewrite pass | |
| `EasyMultiBossesBox` / `EasyFailuresBox` / `EasyRomBox` / `EasyWitchesBox` | Simplify specific multi-phase/group fights | bool | unchecked | `StartFunctions.cs:1239-1256` | `EasyRomBox`/`EasyWitchesBox` have **no live `Checked` handler** — only captured once via `SetBooleans()` at run start |
| `VFXChange` ("Super Aggro") | unclear exact effect from UI layer alone | bool | unchecked | `AiSoundParamRandomizer` gate (inferred name mismatch — needs confirmation) | label doesn't self-document; no tooltip |
| `FaceBox` | Randomize face data | bool | unchecked | `StartFunctions.cs:1562-1566` (`VfxRandomizer("FaceGenParam"/"FaceParam")`) | |
| `TalkBox` | Randomize talk/dialogue param | bool | unchecked | **force-disabled** at run start (`BooleanHandler.cs:74`) | shipped but turned off |
| `BloodBox` | Randomize blood decals | bool | unchecked | `StartFunctions.cs:1568-1571` (`DecalRandomizer`) | |
| `AllDmgAll` ("No Team Type") | Normalize NPC faction/team | bool | unchecked | `TeamTypeRando` (not actually random, §5.11) | |
| `GemsAndRunesBox` | Randomize gem/rune generation | bool | unchecked | `GemGenParamRandomizer` gate | |
| `SixGemBox` / `ThreeGemBox` | Force 6 / 3 gem slots | bool | unchecked, mutually exclusive | `GemGenParamRandomizer:5431-5439` | UI enforces mutual exclusion |
| `NoScaleBox` | Disable enemy/boss stat scaling entirely | bool | unchecked | `StartFunctions.cs:1601` | |
| `CustomScalingBox` | Use per-area custom scaling profile | bool | unchecked | `StartFunctions.cs:1603` | drives a **non-functional** UI (`Window1`, §5.17) — always resolves to hardcoded defaults in practice |
| `EnemySizeSlider` | File-size compatibility limit multiplier for enemy replacement | double (1-10, default 1) | 1 | `Randomize`'s size-reroll loop (`RandomizeFunctions.cs:379-466`) | the only data-bound (vs. manually-synced) control in the app |
| Per-area boss-chance sliders (14 areas × 6 stepper buttons) | % chance an enemy becomes a boss, per zone | double, 0-100 | 100 (most areas) / 30 (Research Hall) | **not read anywhere in `StartFunctions.cs`** (open question — likely consumed inside `RandomizeFunctions.cs`/`MainWindow.xaml.cs`, not independently confirmed in this pass) | 84 near-identical hand-written button handlers, no shared helper |
| `bossPercentage` (global boss-replace chance) | % chance an eligible enemy becomes a boss via insertion | double, 0-100 | unconfirmed default | `InsertBossesVoid:1522-1523` | |
| `chaliceChanceFloat` | % chance an enemy is drawn from the chalice pool | double, 0-100 | unconfirmed default | `Randomize` chalice branch | |
| `SeedTextbox` | Numeric seed | int (≤10 digits) | empty (auto-random) | `TEST_Click`, `StartFunctions.cs:34,38` | |
| `logging` | Write diagnostic logs | bool | `true` (compile-time constant, `FieldContainer.cs:10`) | gates ~20 log-write blocks throughout | **not exposed in the UI at all** despite the git-history commit "Add a logging boolean" |

### Groupings

- **Core placement randomization**: enemy, boss, NPC toggles and their chalice/insertion/exclusion modifiers.
- **Item/economy randomization**: shop, drops, workshop, starting weapons, treasure/item lots, (non-functional) key items.
- **Cosmetic/param randomization**: face, blood decals, talk (disabled), team-type (not random), gems/runes, movesets, AI sound (not random).
- **Difficulty/safety knobs**: easy-mode fight patches, enemy-size slider, no-scaling toggle, boss-percentage sliders.
- **Run mechanics**: seed, logging (hidden), the "Oops All X" override mode and its exclusion-mode counterpart.

### Settings interactions / conditional behavior / apparent gaps

- `OopsAllCheck`/`OopsAllBossesCheck` are mutually exclusive with `ExEnemyBox`/`ExBossBox` at the UI level (checking one force-unchecks the other pair).
- `SixGemBox`/`ThreeGemBox` are mutually exclusive with each other.
- `CustomScalingBox` is fully wired to gate a code path, but that code path's actual per-area customization values can never differ from hardcoded defaults because `Window1`'s save logic is dead (§5.17) — a setting that "does something" structurally but produces the same numeric effect as if it were always off, for the scaling *values* (whether it changes whether the code path *runs at all* is confirmed; whether the *values* used differ from the no-scaling defaults is not).
- `ChaliceBoss` is a vestigial, fully dead checkbox (§ table above).
- `RandomizeKeyItemsBox` and `TalkBox` are both wired to real gating logic but are force-disabled/force-set before the user can interact with them.
- `EasyRomBox`/`EasyWitchesBox` behave identically to every other checkbox in end effect (since all settings are captured once via `SetBooleans()` at run start regardless), but are structurally inconsistent with the rest of the codebase's redundant live-handler pattern — worth normalizing rather than porting the inconsistency.

---

## 9. Input / Output Filesystem Model

**Required input**: a folder named `dvdroot_ps4` must exist as a sibling of the randomizer executable's own parent directory (`StartFunctions.cs:52-54`) — i.e., the exe is expected to live one level *inside* a folder structure that also contains `dvdroot_ps4`. There is no UI to point the tool elsewhere; this is a hard convention with **no existence check** (an early `File.ReadAllLines` on a tool-provided text file, not a game file, will throw first if the convention isn't met — see below).

**No folder-picker is used** despite `Ookii.Dialogs` being referenced in the `.csproj` — confirmed unused by repository-wide search.

**Modification model**: strictly **in-place, with a `.bak`-mediated vanilla-restore cycle**, not a separate output tree:
- Before touching `gameparam.parambnd.dcx`, every `eventFileList` entry, and every map in `mapList`, the tool checks for a `.bak` sibling; if present, it deletes the live file and copies `.bak` back over it (restoring the pristine pre-randomization state), then immediately re-creates `.bak` from that now-vanilla file (`StartFunctions.cs:234-307`).
- All subsequent writes target the **exact same path** the file was restored to/read from.
- This means the `.bak` files function as the tool's own "vanilla data source" — there is no separate pristine archive anywhere else on disk. **This directly informs the PS4/AFR porting concern**: the Windows tool's determinism/idempotency guarantee ("run it again, get a fresh randomization from vanilla, not a randomization-of-a-randomization") depends entirely on these `.bak` files being present in the exact same folder as the live files, which is not how an AFR overlay (no on-disk vanilla baseline) is structured.

**Files copied unchanged**: none identified — every file the tool opens for a randomization category it also writes back (the two exceptions being the tool's own bundled text-based scaling/name/size data files under `Mod Files\NPC Scaling File\`, which are read-only inputs the tool ships with, not game data).

**Files generated from scratch**: the timestamped log files under `Mod Files\Logs. Don't Delete\` (mostly created empty; only a few categories — boss log, inserted-boss log header, randomized-NPC log header — actually receive live `WriteLine` content, since most `WriteLine` calls in the codebase are commented out); the `.bak` files themselves, on first run.

### Example output tree (based on actual path-construction code, not guessed)

```
dvdroot_ps4/
├── map/
│   └── mapstudio/
│       ├── m21_00_00_00.msb.dcx            (modified in place)
│       ├── m21_00_00_00.msb.dcx.bak        (vanilla snapshot, recreated each run)
│       ├── m24_02_00_01.msb.dcx            (Upper Cathedral Ward — stricter size-reroll cap)
│       ├── ... (≈24 base maps + ≈19 chalice-dungeon maps, all hardcoded literal paths)
│       └── m29_52_01_00/
│           └── m29_52_01_91.msb.dcx        (read for model-harvesting only, StartFunctions.cs:353)
├── event/
│   ├── common.emevd.dcx                    (perma-darkness patch target)
│   ├── common.emevd.dcx.bak
│   └── ... (16 total event files, backed up/restored as raw bytes only)
├── param/
│   └── gameparam/
│       ├── gameparam.parambnd.dcx          (modified in place — ShopLineupParam, EquipParamWeapon,
│       │                                     NpcParam, TalkParam, DecalParam, GemGenParam, etc.)
│       └── gameparam.parambnd.dcx.bak
├── paramdef/
│   └── paramdef.paramdefbnd.dcx            (read-only — provides row/column layout, never modified)
└── Mod Files/
    ├── NPC Scaling File/
    │   ├── NPCScalingFile.txt              (tool-provided input data, read-only)
    │   ├── NPCParamsFile.txt
    │   ├── Names.txt
    │   └── Sizes.txt
    └── Logs. Don't Delete/
        └── <h-mm-ss-tt>-<Category>Log.txt  (EnemyLog, BossLog, DummyLog, RandomizedNPCLog,
                                              RandomizedItemLog, EnemySizes, InsertedBossesLog, ScaleLog)
```

Since the output *is* the same `dvdroot_ps4` tree, "copying the result into AFR" (per the task's framing) in the Windows workflow means: point the tool at a folder structure whose `dvdroot_ps4` **is already** the AFR staging folder, run it, and the AFR folder now contains the randomized game data directly — there is no separate build/export/copy step inside the tool itself.

---

## 10. Dependency Analysis

| Dependency | Purpose | Used by | Essential to core behavior? | PS4 equivalent needed? |
|---|---|---|---|---|
| **SoulsFormats** (vendored `ProjectReference`) | Generic FromSoftware file-format read/write (MSBB, BND4, PARAM/PARAMDEF, DCX, and much more unused) | `StartFunctions.cs`, `RandomizeFunctions.cs` | **Yes** — this is the only way the app touches game data | Yes, but only a narrow slice: MSBB, DCX, a BND4-family reader, PARAM/PARAMDEF (§6) |
| **.NET Core 3.1 / WPF** (`Microsoft.NET.Sdk.WindowsDesktop`) | UI framework, app hosting | entire `reference/Randomizer` project | Yes, for the *Windows app itself* | No — PS4 needs an entirely different, platform-appropriate UI approach; nothing here is reusable directly |
| **MahApps.Metro** v2.2.0 (NuGet) | Referenced for themed WPF chrome | **Confirmed unused** — no `mah:` namespace or Metro control anywhere in the XAML | No | No |
| **Ookii.Dialogs** v1.0.0 (NuGet) | Referenced for Vista-style folder/file dialogs | **Confirmed unused** — no dialog API call anywhere in the code | No | No |
| **System.Random** (BCL) | RNG | `RandomizeFunctions.cs`, `StartFunctions.cs` | Yes, functionally — but see §7's caveat that its algorithm isn't a portable/versioned guarantee | Yes — needs its own explicitly-chosen, documented PRNG on PS4 |
| **System.Threading.Thread** (BCL, raw, no `async`/`await`/`Task`) | Background execution + UI polling loop | `TEST_Click`, `DoSomething`/`DoSomething2` | Yes, for the Windows app's threading model specifically | No — PS4 will have its own concurrency/threading model; nothing here generalizes beyond "do the work off the main/UI thread and report progress back" |
| **Microsoft.VisualStudio.Windows.Forms** (local HintPath reference to a Visual Studio IDE assembly) | Unclear — a hard-coded, machine-specific path into a VS install (`Randomizer.csproj:25-27`) | Unconfirmed usage; likely a design-time-only artifact from a WinForms-interop control accidentally left in the project | Unclear — **open question**, flagged below | No |

**Core functionality dependencies** (must have an equivalent for a PS4 native reimplementation to touch the same game data): SoulsFormats' MSBB/DCX/BND4/PARAM support, and *a* PRNG with an explicit, documented algorithm.

**Windows/application-infrastructure dependencies** (do not need a PS4 equivalent, or need a completely different platform-appropriate substitute rather than a port): WPF, MahApps.Metro (unused anyway), Ookii.Dialogs (unused anyway), raw `Thread`-based concurrency, `Dispatcher.Invoke` UI marshaling, `MessageBox.Show`, and the stray VS-IDE assembly reference.

**Open question**: the `Microsoft.VisualStudio.Windows.Forms` reference (`Randomizer.csproj:25-27`) points to a hardcoded path six directories up from the project (`..\..\..\..\..\..\Program Files (x86)\Microsoft Visual Studio\2019\Community\...`) — this only resolves on a machine with that exact VS edition/version installed at the default path, and no code path referencing WinForms interop was identified during this review. Whether this reference is load-bearing (some control was dragged from a WinForms toolbox onto the WPF designer) or entirely vestigial was not resolved in this pass.

---

## 11. Conceptual Architecture Layers

Forcing a clean N-layer model onto this codebase would misrepresent it — as established in §3, there is no enforced boundary between any of these "layers"; they are all methods and fields on one `partial class MainWindow`. With that caveat stated plainly, the *conceptual* responsibilities do separate along these lines, file-by-file:

1. **Presentation** — `MainWindow.xaml`, `Window1.xaml`, and the event-handler bodies in `UIComponents.cs`. Purely WPF control definitions and one-line handlers copying control state into fields.
2. **Application/workflow orchestration** — `StartFunctions.cs` (`DoSomething`/`DoSomething2`), `BooleanHandler.cs` (`SetBooleans`). Decides *what* runs, *in what order*, gated by *which* settings; owns the backup/restore cycle and progress reporting.
3. **Randomization engine (generic shape)** — the without-replacement draw idiom, the per-column shuffle idiom, and the repeated PARAM/BND4 load-mutate-save boilerplate, all embedded inside `RandomizeFunctions.cs` rather than factored into shared utilities.
4. **Bloodborne domain knowledge** — every hardcoded ID list, exclusion table, map-name comment, and special-case branch cataloged in §5/§6, spread across `RandomizeFunctions.cs`, `MainWindow.xaml.cs` (constructor's exclusion lists), and `StartFunctions.cs` (map/event path literals, item-lot ID comments).
5. **Game data / file format access** — `SoulsFormats.MSBB`/`BND4`/`PARAM`/`PARAMDEF`, consumed via their static `Read`/instance `Write` API.
6. **Compression/archive infrastructure** — `SoulsFormats.DCX`, invoked transparently by layer 5; never touched directly by the app.
7. **Filesystem/platform services** — raw `System.IO.File`/`Directory` calls for path resolution, `.bak` management, and log writing, scattered directly inside `StartFunctions.cs` rather than isolated behind an abstraction.

### Classification for a PS4 reimplementation

| Layer / area | Classification | Rationale |
|---|---|---|
| WPF UI (layer 1) | **Replace with platform-specific implementation** | PS4 homebrew needs a controller-driven, non-WPF UI entirely; nothing here (control choice, layout, MahApps) transfers |
| Tooltip/label copy, the startup explanation MessageBox text (§6.1 quote in the UI-agent findings) | **Reproduce behavior directly** (as content, not code) | Player-facing explanations of mechanics (e.g. the enemy-size slider) are valuable, reusable *documentation*, independent of the UI toolkit |
| Orchestration sequence/gating logic (layer 2) | **Reimplement conceptually** | The *order* and *conditions* under which categories run is meaningful domain logic (see §7's determinism analysis) worth preserving deliberately, but the raw-`Thread` + `Dispatcher.Invoke` mechanics are Windows/WPF-specific and should not be ported literally |
| Generic randomization idioms (layer 3: without-replacement draw, per-column shuffle, param-load boilerplate) | **Reimplement conceptually** | The *shapes* of these algorithms are worth keeping (and improving — e.g. fixing the recurring off-by-one), but the C#-specific implementation (list `RemoveAt`, `Cell.ToString()` field-name parsing) should not be copied as-is |
| Bloodborne domain knowledge — hardcoded ID/exclusion lists (layer 4) | **Reproduce behavior directly, after independent verification** | This is the actual "what makes Bloodborne randomization work" content; it should be carried forward as data, but every list should be re-verified against the game's real param/MSB data rather than trusted blindly, since several were shown to contain bugs, drift between near-duplicate lists, and at least one unreachable branch |
| MSBB/BND4/PARAM/DCX format handling (layers 5-6) | **Reproduce behavior directly (format semantics), reimplement mechanically (in C++)** | The *file formats themselves* are fixed external facts (FromSoftware's binary layouts) that must be matched byte-for-byte; the *code* that reads/writes them must be freshly written in C++ (there is no portable SoulsFormats-for-C++ implied by this review — that's a separate research question) |
| Filesystem/backup conventions (layer 7) | **Probably unnecessary as designed / needs a substitute** | The `.bak`-restore-then-rewrite model assumes a vanilla file sits right next to the live one, which doesn't match the AFR-overlay reality on PS4 (per existing project memory) — this specific mechanism should not be ported; an equivalent "how do we guarantee determinism/idempotency without an on-disk vanilla copy" design is needed instead |
| Dead/stub features (Key Item randomization, `ChaliceBoss` checkbox, `Window1` scaling customization, `BulletRandomizer`'s bulk of logic, `CutEnemyScan`, the RemoveAt(3) mystery) | **Probably unnecessary for PS4** | These do not currently do anything in the Windows app; porting them would mean porting no-ops. Worth a deliberate decision (confirm with original intent/author if possible) rather than silent omission, but they are not blocking artifacts of "how the randomizer works" |

---

## 12. Windows → PS4 Porting Considerations (Observations Only — No Design Proposed)

This section stays within the task's boundary — no port is being proposed — but records the specific, concrete tensions this review surfaced that will need a decision later:

- **No settings boundary exists to mirror.** A PS4 implementation must design its own configuration representation from scratch; there is nothing here to structurally copy (§3, §12 is not inventing one, just noting the gap).
- **The `.bak` vanilla-restore mechanism has no direct AFR equivalent.** This is the single most important open design question flagged by this review, and it directly matches previously-recorded project context that AFR is an overlay with no vanilla data present. Any equivalent "make re-randomization safe/idempotent" behavior on PS4 will need a different mechanism.
- **Determinism is real but fragile and undocumented as a contract.** §7 establishes that same-seed/same-settings/same-install reproducibility holds today, but nothing in the codebase treats this as a guaranteed, tested contract — no test suite exists, and the exact RNG call sequence per category is an emergent property of hardcoded list-construction order, not a documented invariant. If bit-for-bit parity with the Windows tool is ever a goal for the PS4 version, that would need to be a deliberate, explicit design constraint, verified with real regression tests, not assumed to fall out of "porting the algorithms."
- **A meaningful fraction of the "feature list" is not real.** Any scoping conversation about "what do we need to reproduce" must start from the *live* feature set in §5, not from the UI checkbox list in §8, since several checkboxes are wired to non-functional or already-disabled code.
- **The two distinct randomization "shapes" (atomic identity-swap vs. per-column independent shuffle) are semantically different and should be modeled as different operations**, not unified into one generic "randomize this data" utility — doing so would produce different (and likely worse, or at least different) results than the existing tool for the column-shuffle categories.
- **Several concrete bugs exist in the reference implementation** (§5.1, §5.2, §5.4, §5.8's semantic inversion, §5.9, §5.16). Each should be triaged individually — reproduced deliberately (if seed/behavior parity matters) or fixed deliberately (if correctness matters more) — rather than either blindly copied or blindly "corrected" without a decision being made.

---

## 13. Open Questions and Evidence Gaps

These are explicitly unresolved by this review and are listed with pointers to what further investigation would need to check:

1. **Where/how the enemy/boss/NPC data pools (`enemyDataRandomized`, `chaliceEnemiesString`, `nameList`/`sizeList`, `combinedBossList2`, `npcList`/`hostileNpcList`, `bossList`, `oopsAllEnemyString`, `BossListString`, `unusedList`, `nonoItemLots`) are actually populated.** `RandomizeFunctions.cs` only *consumes* these; they are built somewhere in `StartFunctions.cs`/`MainWindow.xaml.cs`, but the exact construction logic (especially for `bossList`/`unusedList`, referenced from the `MainWindow` constructor) was not traced end-to-end in this pass with full line-level rigor for every list. **Needed**: a follow-up read of `MainWindow.xaml.cs:54-208` and the `GenerateEnemyList`/`OopsAll`/`OopsAllBoss` helper bodies (`MainWindow.xaml.cs:734-933`) specifically for pool-construction logic.
2. **`GenerateNPCList`/`RandomizeNPCs` pipeline gap** (§5.3): does `npcEnemyList` (built by `GenerateNPCList`) ever get converted into the `npcList`/`hostileNpcList` string pools `RandomizeNPCs` actually reads? Not resolved — needs a targeted search across `MainWindow.xaml.cs` for where those two string-pool fields are populated.
3. **Whether `ParamScalingForBosses`'s applied values ever meaningfully vary**, given that its customization UI (`Window1`) is confirmed dead (§5.17) — established that the 15 scale-index fields always take hardcoded defaults, not established whether those defaults themselves produce a no-op scaling pass or a real (if fixed) one. **Needed**: full read of `MainWindow.xaml.cs:224-732`.
4. **Why `AiSoundParamRandomizer` (§5.13) and `TeamTypeRando` (§5.11) — both deterministic, non-random normalization passes — are necessary at all in the context of identity randomization.** No code comment explains the causal link between "we randomized enemy identities" and "therefore AI hearing ranges/faction IDs need resetting." **Needed**: either the original author's rationale, or independent research into how `AiSoundParam`/team-type fields are actually keyed to enemy behavior in Bloodborne's engine.
5. **The exact real-world identity of several inferred entities**: `OoKEnemy`/`c4540_0000` (inferred: Orphan of Kos), `GOBAdded`'s target map (inferred: some "Great One Beast"-related optional content, per git history "Added Great One Beast Mod Support" — but the mapping from that commit to this specific field was not independently re-confirmed against the commit's actual diff in this pass), and the precise meaning of UI labels without tooltips (`VFXChange` "Super Aggro", `GemsAndRunesBox`, `PermaDarknessBox`, `AllDmgAll` "No Team Type"). **Needed**: either developer/community documentation (e.g. the linked Nexus Mods page) or direct correlation against known Bloodborne param/ID references.
6. **Whether the per-area boss-chance slider fields (`HemwickChance`, `CathedralWardChance`, etc.) are consumed anywhere** — confirmed *not* read in `StartFunctions.cs`; not confirmed whether they're read inside `RandomizeFunctions.cs`/`MainWindow.xaml.cs` (the researcher covering `RandomizeFunctions.cs` did not report consuming these specific fields either, which raises the possibility they are altogether unused, but this is not a confirmed negative).
7. **The `Microsoft.VisualStudio.Windows.Forms` project reference's purpose** (§10) — vestigial design-time artifact vs. load-bearing, not resolved.
8. **The `unusedPlusBossList.RemoveAt(3)` line** (`StartFunctions.cs:580`) — removes a specific hardcoded-position entry (`"c2560"`) from the exclusion list at a specific pipeline moment, with no comment explaining intent. Flagged as fragile, unexplained logic rather than resolved.
9. ~~**Whether `permaDarknessBool` actually gates `PermaDarknessFunction`'s unconditional call site** (`StartFunctions.cs:1352`)~~ — **RESOLVED: it does.** The flag is read inside the function body (`MainWindow.xaml.cs:1461`), and *both* branches write, which is why the call site needs no `if`. There is no early return. The twist the review could not see without the game data: the `true` branch writes the values vanilla already has, so it is a no-op restore — needed only because that tool edits game files in place — and the **unchecked default is the branch that modifies the game**. Full decode in `docs/plans/mergo-darkness.md` §2; shipped in this port as `DISABLE MERGO DARKNESS`, with the polarity corrected.
10. **Real-world verification of every hardcoded ID list against the actual game data** was explicitly out of scope for this code-reading exercise (no game files were extracted/cross-referenced) — every ID-to-meaning mapping in this document is either a direct code comment (treated as author's own claim, not independently verified) or this reviewer's inference from naming/context, and is flagged as such throughout.

---

## 14. Recommended Code Reading Path

For an engineer picking this repository up cold, wanting to answer "how does a Bloodborne enemy actually get randomized":

1. **Start at `reference/Randomizer/App.xaml`** — confirms the entire startup mechanism is `StartupUri`; nothing else to see here, move on quickly.
2. **`reference/Randomizer/MainWindowComponents/MainWindow.xaml`** — skim the layout to build a mental map of every setting the user can touch; note the tooltips (they're the best player-facing documentation that exists) and the static instructional labels.
3. **`reference/Randomizer/MainWindowComponents/MainWindow.xaml.cs` (constructor, lines ~14-208)** — see how the static exclusion lists (`unusedList`, `bossList`, `chaliceBossParams`) get built, and read the startup MessageBox text in full (§6.1-equivalent quote) for the clearest plain-English explanation of the size-based enemy-swap constraint anywhere in the codebase.
4. **`reference/Randomizer/MainWindowComponents/FieldContainer.cs`** — read top to bottom once; this *is* the settings model (a flat field list), and having it in mind makes every later file legible.
5. **`reference/Randomizer/MainWindowComponents/UIComponents.cs:147-158` (`TEST_Click`)** — the actual "go" button handler; note how thin it is (seed handling + spawning two threads) because everything else was already captured live by per-control handlers elsewhere in this same file.
6. **`reference/Randomizer/MainWindowComponents/BooleanHandler.cs` (`SetBooleans`)** — the authoritative run-start settings snapshot; read once to see the full list of boolean settings in one place.
7. **`reference/Randomizer/MainWindowComponents/StartFunctions.cs:32` onward (`DoSomething`)** — the heart of the orchestration. Read it as a linear script (it is one); pay special attention to the `.bak` backup/restore block (~lines 200-310) since that's the file-safety/determinism foundation everything else depends on, and to the long sequence of `if (someBool) { ...; SomeRandomizeFunction(...); }` blocks that make up the rest of the method — this sequence *is* the feature list, gating included.
8. **`reference/Randomizer/MainWindowComponents/RandomizeFunctions.cs:11` (`Randomize`)** — the actual enemy-placement algorithm; this is probably the single method a reader most wants, and it's a good representative example of the file's overall style (identity-swap via `"*"`-joined strings, region-chance rolls, hardcoded per-map exclusions, a reroll loop).
9. From there, follow `DoSomething()`'s call order (§4's flow diagram) into `GenerateBossList` → `RandomizeBosses` → `InsertBossesVoid` for the boss-side mirror of the same pattern, then into `RandomizeShopItems`/`RandomizeMeleeMoveset`/`GemGenParamRandomizer` to see the two other randomization *shapes* (atomic swap vs. per-column shuffle) described in §5.
10. **`SoulsFormats\reference/SoulsFormats/Formats/MSB\MSBB\MSBBB.cs`** and **`SoulsFormats\reference/SoulsFormats/Formats/PARAM\PARAM.cs`** — only once the above is clear, dip into the vendored library to see what `MSBB.Part.Enemy`/`PARAM.Row`/`PARAM.Cell` actually expose; this library is generic infrastructure and should be treated as a reference, not something to read cover-to-cover.

---

## Appendix: Sources

This document synthesizes: direct inspection of the repository structure, `.csproj`/`.sln` files, and `SoulsFormats` type usage performed in this session; a full line-by-line read of `reference/Randomizer/MainWindowComponents/RandomizeFunctions.cs` (5,511 lines); a full line-by-line read of `reference/Randomizer/MainWindowComponents/StartFunctions.cs` (2,608 lines) plus `FieldContainer.cs` and `BooleanHandler.cs`; and a full read of the UI layer (`App.xaml(.cs)`, `MainWindow.xaml(.cs)`, `Window1.xaml(.cs)`, `UIComponents.cs`). Every specific claim in this document traces to one of those passes and carries its own `file:line` citation; no game files were extracted or cross-referenced against a live Bloodborne install as part of this review.
