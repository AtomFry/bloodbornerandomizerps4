# Enemy Drops & Starting Weapons — Windows Trace and PS4 Implementation Plan

**Status: BUILT. Both features shipped — spec rows 4 and 5 are DONE.** The plan below was carried out; `src/Param/ParamBnd.*`, `src/Randomizer/DropRandomizer.*` and the starting-weapons work all exist, with `tools/drops_verify.py` and `tools/starting_weapons_verify.py` as their validators. Starting weapons grew its own document during implementation — [starting-weapons.md](starting-weapons.md) — which supersedes §2 here and records the three-toggle split that was chosen over the reference's single negative flag.

Read this document for the **trace and the measurements**: what the reference tool actually does to `NpcParam` and `ShopLineupParam`, and the file-format facts the implementation was built on (§3 onward). Those measurements are still accurate and still load-bearing. Do not read the cost estimates as outstanding work.

Both features are PARAM-based, which is why they were deferred in [boss-and-treasure-findings.md](boss-and-treasure-findings.md).

---

## 1. Randomize Enemy Drops — what Windows actually does

Setting `enemyDropBool`, gate at `StartFunctions.cs:1536-1539`, implementation `RandomizeFunctions.cs:3098-3171`. (Lines 2908-3096 are an entirely commented-out earlier implementation — ignore it.)

Touches exactly one param: **`NpcParam`**, cells **11** and **12** (the two item-lot columns).

**Pool:** for every row whose ID is not `252100` and not `6071`, collect cell 11 and cell 12 where the value is not `-1`.

**Assignment:** for every row passing the same two ID exclusions, if cell 11 is not `-1`, draw a random pool entry, reroll while it equals the current value, and write it to cell 11.

Four things worth flagging before anyone reproduces this:

| # | Behavior | Evidence |
|---|---|---|
| **D-1** | **Cell 12 is read into the pool but never written.** Drop slot 2 contributes items but never receives one | `:3134-3138` collect, no corresponding write in `:3143-3163` |
| **D-2** | **Sampling is WITH replacement** — no removal from the pool. Unlike treasure, this is not a permutation; the same drop can be handed to many enemies | no `RemoveAt` anywhere in the loop |
| **D-3** | **Off-by-one**: `universalRand.Next(0, itemLotList.Count - 1)` can never select the last pool entry | `:3151`, `:3156` |
| **D-4** | **Unbounded reroll**: `while (tempString == itemLotList[randomNumber])` has no iteration cap. If the pool degenerated to one distinct value this hangs | `:3154-3157` |

There is no category awareness and no protected-item list — the only exclusions are the two hardcoded row IDs.

## 2. Randomize Starting Weapons — what Windows actually does

Setting `startingWeaponsOnlyBool`, gate at `StartFunctions.cs:1541-1544` (`shopBool || startingWeaponsOnlyBool` — the two settings share one function), implementation inside `RandomizeShopItems`, `RandomizeFunctions.cs:609-1315`.

**The name is misleading, and this is the single most important finding for scoping.** Weapon rows in `ShopLineupParam` (those with `Cells[7] == "0"`) are randomized **only when `startingWeaponsOnlyBool` is set** — `shopBool` alone does *not* randomize shop weapons, only armor (`"1"`) and consumables (`"3"`) (`:890`, `:982`, `:1002`).

So "Randomize Starting Weapons" actually means: **randomize every weapon entry in the shop lineup, and additionally make whatever lands in the five starting slots usable at level 1.**

It works in two stages:

**Stage 1 — shop weapon rows** (`:876-980`). Skips rows whose `Cells[0]` value is in `vialAndBulletIDsList` (`"1000"`, `"900"`, `"240"`, plus the two gun-ammo entries when `keepGuns`). Draws from `shopWeaponList` **without replacement**. Two constrained rerolls: rows for `7000000`/`5000000`/`22000000` must land on an entry in the hardcoded ~89-item `rightHandList`; rows for `14000000`/`6000000` must land in the 16-item `leftHandList`. As it goes it records what landed in `ShopLineupParam` rows **2000/2001/2002** (and **2010/2011** unless `keepGuns`) into `new2000`…`new2011`.

**Stage 2 — stat requirement rewrite** (`:1043-1300`). For each captured starting weapon it computes ten upgrade variants by repeatedly adding **100** to the item ID (`:1074-1093`), then in `EquipParamWeapon` sets **cells 80/81/82/83** (the four stat requirements) on the base row and all ten variants to fixed low values matching the vanilla starter profile — e.g. `8,7,0,0` for the 7000000 slot, `9,8,0,0` for 5000000, `7,9,0,0` for 22000000, `7,9,5,0` for the two guns (`:1106-1176`).

So it touches two params: **`ShopLineupParam`** (cell 0, the item ID) and **`EquipParamWeapon`** (cells 80-83).

Quirks to decide on:

| # | Behavior | Evidence |
|---|---|---|
| **S-1** | Three `while` loops with **no cap** (`while (!rightHand)`, `while (!leftHand)`, and the self-match guard). Hangs if no pool entry satisfies the hand constraint | `:896-899`, `:907-917`, `:926-936` |
| **S-2** | The self-match guard compares a pool string against `Cells[0].ToString()` — the **cell's display string** (`"name = value"`), not `.Value.ToString()`. It almost certainly never matches, making the guard a no-op | `:896` vs `:882` |
| **S-3** | `rightHandList` contains `"/29000000"` — a stray leading slash, so that weapon can never satisfy the right-hand constraint | `:794` |
| **S-4** | The `+100` upgrade-tier assumption is hardcoded and unverified against real data | `:1074-1093` |

## 3. The infrastructure question, re-measured

The previous finding was "these need a BND4 + PARAMDEF + PARAM stack, a milestone in its own right." That was right about *what the Windows code uses* and wrong about *what the PS4 port needs*. Measured against the real files:

### 3.1 DCX — already solved

Both archives decompress with the port's **existing** `DcxDecompress`:

| File | On disk | Decompressed | Inner |
|---|---|---|---|
| `gameparam.parambnd.dcx` | 1,819,017 | 28,309,992 | `BND4` |
| `paramdef.paramdefbnd.dcx` | 82,877 | 640,040 | `BND4` |

### 3.2 BND4 — a flat container, and simpler than feared

Parsed from the real file: **65 member files**, header size 64, file-header stride 36, `format=0x74`, `unicode=1`. Crucially **all 65 members are individually uncompressed** (compressed size == uncompressed size for every one), so there is no nested DCX to handle.

Locations of the three params we need:

| Member | Offset | Size |
|---|---|---|
| `EquipParamWeapon.param` | 4,246,096 | 386,558 |
| `NpcParam.param` | 8,982,464 | 13,227,416 |
| `ShopLineupParam.param` | 22,749,184 | 96,278 |

### 3.3 PARAM — fixed-size rows, so no PARAMDEF is needed at runtime

`NpcParam.param` parsed directly: `paramType = NPC_PARAM_ST`, **31,398 rows**, row descriptors at `0x40` with a 24-byte stride (`id` int32, pad, `dataOffset` int64, `nameOffset` int64), and row data offsets that advance uniformly — **388 bytes per row**.

That is the whole unlock. The Windows code needs `PARAMDEF` only to turn a *cell index* (11, 12, 80-83) into a typed accessor. We can do that conversion **once, offline, in Python**, using the real `paramdef.paramdefbnd.dcx`, and bake the resulting **byte offsets** in as constants — exactly the pattern already used by `ModelSizeTable.h` and `NpcScalingTable.h`.

### 3.4 Nothing changes size, so nothing needs re-serializing

Every edit is an `int32` written over an existing `int32`. No rows are added or removed, no strings change, no offsets move. So the modified param bytes can be written **in place** into the decompressed BND4 buffer, and that buffer recompressed — no BND4 writer, no PARAM writer, no string-table rebuild.

This is the same insight that made the MSBB port tractable (opaque blobs + fixed-offset pokes), applied one level up.

**Revised infrastructure estimate:** a BND4 *locator* (~60 lines: walk 65 headers, return each member's byte range), a PARAM *row index* (~40 lines: walk row descriptors, map row ID → data offset), and a baked offset table. No PARAMDEF parser, no PARAM serializer, no BND4 serializer.

## 4. The real risk: output file size, not parsing

The port's `DcxCompress` emits **stored (uncompressed) DEFLATE blocks** because the PS4 toolchain's zlib is decompress-only. Measured on your actual hardware output:

| Map | Vanilla | PS4 output | Ratio |
|---|---|---|---|
| `m24_01_00_01` | 252,144 | 2,989,512 | 11.9× |
| `m22_00_00_00` | 73,374 | 829,483 | 11.3× |
| `m26_00_00_00` | 145,254 | 1,911,200 | 13.2× |

That has been fine for maps. Applied to `gameparam.parambnd.dcx` it means writing roughly **28 MB in place of 1.8 MB** — a ~15× increase on a file the game loads at startup. This is the genuine unknown, and it is a *runtime* risk, not a parsing one:

- It may simply work, as the maps did.
- It may slow boot noticeably.
- It may hit a size assumption in the game's own loader — the failure mode would be a boot hang or crash, not a graceful error.

Mitigations, in increasing cost: test it and accept the size; implement a real DEFLATE encoder (fixed-Huffman literals only is modest, LZ77 + fixed Huffman is a few hundred lines and would also shrink every map file ~12×); or ship params only when a param-touching setting is enabled, so the default enemies/bosses/treasure run keeps the vanilla 1.8 MB file.

The third option is cheap and should be the default regardless: **only write `gameparam.parambnd.dcx` when a param feature is actually on.**

## 5. Proposed increments

Each is independently reviewable and testable, and each ends at a state where the previous features still work.

1. **Param access layer + a read-only proof.** DCX → BND4 locator → PARAM row index, plus an offline Python tool that computes and bakes the cell→byte offsets. Prove it by round-tripping `gameparam.parambnd.dcx` **unmodified** and confirming the re-emitted file still parses and the game still boots. This is the checkpoint that retires the size risk before any randomization depends on it.
2. **Enemy drops.** One param (`NpcParam`), one field, the simplest algorithm of the two. Reproduce D-1/D-2 as-is; bound D-4 and decide explicitly on D-3.
3. **Starting weapons.** Two params, the hand-constraint rerolls, the captured slots and the stat rewrite. Larger, and it needs a decision on the misleading setting name (§2) since it randomizes the whole shop weapon list.

Doing (1) first is the point of the split: it converts the one genuine unknown into a measured yes/no before we build features on top of it.

## 5a. Decisions taken (2026-09-13)

| Question | Decision |
|---|---|
| Starting Weapons randomizes the whole shop weapon list | **Preserve Windows behavior.** The misleading name is documented in §2; revisit naming later |
| Enemy drops read cell 12 but never write it (D-1) | **Preserve Windows behavior.** Do not "fix" what merely looks suspicious; documented for later investigation |
| Off-by-one makes the last pool entry unreachable (D-3) | **Fix it** — deliberate deviation from Windows, to be covered by a test |

## 5b. Questions resolved from the game's own data

Answered offline with `tools/param_offsets.py`, which reads the real `paramdef.paramdefbnd.dcx`. Cell index maps one-to-one onto paramdef field index (confirmed from SoulsFormats `PARAM.Row.ReadCells`, which allocates one cell per field including padding and bit fields).

**The magic cell indices, named at last:**

| Windows reference | Field | Type | Byte offset in row |
|---|---|---|---|
| `NpcParam` cell 11 | `itemLotId_1` | s32 | 44 |
| `NpcParam` cell 12 | `itemLotId_2` | s32 | 48 |
| `EquipParamWeapon` cell 80 | `properStrength` | u8 | 237 |
| `EquipParamWeapon` cell 81 | `properAgility` | u8 | 238 |
| `EquipParamWeapon` cell 82 | `properMagic` | u8 | 239 |
| `EquipParamWeapon` cell 83 | `properFaith` | u8 | 240 |
| `ShopLineupParam` cell 0 | `equipId` | s32 | 0 |
| `ShopLineupParam` cell 7 | `equipType` | u8 | 23 |

Note the stat requirements are **u8, not s32** — a single-byte poke, not a 4-byte one.

**The offset computation is self-validating.** Walking the paramdef yields a row size that must match the stride measured independently from the row table, and it does for all three:

| Param | Computed from paramdef | Measured from row table | Rows |
|---|---|---|---|
| `NPC_PARAM_ST` | 388 | 388 | 31,398 |
| `EQUIP_PARAM_WEAPON_ST` | 316 | 316 | 1,090 |
| `SHOP_LINEUP_PARAM` | 32 | 32 | 1,288 |

**The `+100` upgrade stride is real** (question 4): for all five starting weapons (`7000000`, `5000000`, `22000000`, `14000000`, `6000000`) the base row and all ten `base + 100n` variants exist in `EquipParamWeapon`. The reference's assumption holds.

Also confirming the trace: `ShopLineupParam`'s first row IDs are `1, 2000, 2001, 2002, 2010, 2011, …` — exactly the starting-weapon slots the Windows code captures.

## 5c. Increment 1 — implemented, awaiting hardware

New `Param/ParamBnd.h/.cpp`: a **locator**, not a parser — it lists the archive's members and their byte ranges, nothing more. No PARAMDEF support, no writer.

A new `ItemData` phase runs after the event patch, gated on a diagnostic `REWRITE ITEM DATA (TEST)` toggle in the Enable wizard (deliberately not persisted to `defaults.cfg`; it is an experiment, not a feature). In four steps it frees the parsed maps, decompresses the archive, lists its contents, and re-emits it **unchanged**.

Offline validation (`tools/itemdata_verify.py roundtrip`):

- vanilla archive: 1,819,017 bytes on disk → 28,309,992 decompressed, **65 entries**
- all three target params located correctly
- emulating the exact PS4 write path produces **28,312,234 bytes (27.0 MB, 15.6×)**
- that stream inflates back **byte-identical**, and still lists the same 65 entries
- 432 stored deflate blocks — the same multi-block path the map files already exercise on hardware, just at scale

So the bytes we intend to write are provably valid. What remains unproven is only whether the game accepts a 27 MB file where it expects 1.8 MB.

## 5d. Increment 2 — enemy drops, implemented, awaiting hardware

**Scope decision (2026-09-13):** the standalone read-only round trip was skipped at the user's direction. Enemy drops now carries both risks — the archive rewrite *and* the randomization — into one hardware test. The log distinguishes them (see §5e).

New `Randomizer/DropRandomizer.h/.cpp`; `Param/ParamBnd` gained a row-table reader. The former `REWRITE ITEM DATA (TEST)` toggle became the real `RANDOMIZE ENEMY DROPS` setting, persisted like the others.

**What it does:** collects every non-`-1` value of `itemLotId_1` and `itemLotId_2` across `NpcParam` (skipping rows `252100` and `6071`), then reassigns `itemLotId_1` on every eligible row by drawing from that pool **with replacement** — so, unlike treasure, this is not a permutation and the same drop can land on many enemies. Only `itemLotId_1` is ever written.

**Measured against the real archive:**

| | |
|---|---|
| NpcParam rows | 31,398 |
| pool entries | 25,197 (618 distinct) |
| rows eligible to change | 25,196 |

Note the pool is 25,196 + 1: **exactly one row in the game has a second drop slot set.** So preserved quirk D-1 (cell 12 feeds the pool but never receives) has almost no practical consequence — worth recording, since it looked significant on paper.

**Deviations from Windows in this feature:**

| # | Reference | This port | Why |
|---|---|---|---|
| D-1 | `itemLotId_2` pooled but never written | **Preserved** | User decision: don't "fix" what merely looks suspicious. Affects one row |
| D-3 | `Next(0, count - 1)` — last pool entry unreachable | **Fixed** | User decision. C++ `uniform_int_distribution(0, size-1)` is inclusive of both bounds, so this is the full range — note the C# `Next` upper bound is *exclusive*, which is precisely where the original bug came from |
| D-4 | Uncapped reroll while candidate == current | Capped at 64; on exhaustion the row keeps its vanilla drop | Uncapped loops hang a console |

**Validation** (`tools/drops_verify.py`, selftest **6/6**): catches a fabricated item lot, a write to `itemLotId_2`, a change to an excluded row, a drop given to a `-1` row, and modification of any other param; silent on an unmodified archive. Against a simulated full run it reported 25,196 changed rows and 0 failures.

## 5e. Telling the two risks apart if hardware fails

Because the round trip and the randomization ship together, the log is the discriminator:

- Progress reaches `RANDOMIZING ENEMY DROPS` and the summary shows `65 ENTRIES` and a plausible reassignment count → the read/parse/write path worked. A boot failure then points at **file size**.
- The run fails or reports an unexpected entry count → the **archive handling** is wrong, and randomization never mattered.
- The game boots but drops are wrong/absent → the **randomization logic**, with the archive path proven fine.

Recovery in all cases: delete `param/gameparam/gameparam.parambnd.dcx` from the AFR folder.

## 6. Open questions to settle before coding

1. **Does the game accept a ~28 MB `gameparam.parambnd.dcx`?** The single blocking unknown. Increment 1 answers it.
2. **Is the `+100` upgrade-tier stride real?** Verifiable offline against `EquipParamWeapon` row IDs before relying on it (S-4).
3. **Should "Randomize Starting Weapons" keep its Windows meaning** (randomize the entire shop weapon list) or be narrowed to the five starting slots? The reference behavior is surprising enough that reproducing it silently seems wrong.
4. **D-1: leave drop slot 2 write-only-never-written, or fix it?** Reproducing is faithful; fixing is arguably what the author meant.
5. **D-3: preserve the off-by-one** (last pool entry unreachable) or correct it?
6. **What are cells 11/12 and 80-83 actually named?** The offline paramdef pass will tell us, and the names should go in the docs so these stop being magic indices.

## 7. Out of scope for this plan

Shop armor/consumable randomization (`shopBool`), movesets, gems, decals, talk, VFX and AI-sound params. All are param-based and become cheap once increment 1 exists, but none are in this plan.
