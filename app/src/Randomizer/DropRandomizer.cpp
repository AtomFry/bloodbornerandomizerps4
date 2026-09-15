// cstdlib before anything pulling <random>/<cmath>, or the toolchain's libc++
// fails on ::abs - same ordering constraint as BossRandomizer.cpp.
#include <cstdlib>

#include "DropRandomizer.h"

#include "../Msb/BinUtil.h"
#include "../Platform/Log.h"

#include <set>

namespace bbr {

namespace {

// Byte offsets within an NpcParam row, from tools/param_offsets.py.
const size_t kItemLot1Offset = 44; // itemLotId_1, s32  (reference "Cells[11]")
const size_t kItemLot2Offset = 48; // itemLotId_2, s32  (reference "Cells[12]")

// The reference's only exclusions - two hardcoded row IDs, skipped both when
// building the pool and when assigning (RandomizeFunctions.cs:3125-3127,
// 3145-3147). There is no protected-item list for drops.
bool IsExcludedNpcRow(int32_t id) {
    return id == 252100 || id == 6071;
}

// Deviation: the reference rerolls with `while (current == candidate)` and no
// iteration cap, which hangs if the pool degenerates. Bounded here; on
// exhaustion the row keeps its vanilla drop, which is always valid.
const int kRerollCap = 64;

} // namespace

bool RandomizeEnemyDrops(std::vector<uint8_t>& plain,
                         const ParamMember& npcParam,
                         std::mt19937& rng,
                         DropRandomizerResult& result,
                         std::string* error) {
    std::vector<ParamRow> rows;
    if (!ParseParamRows(plain, npcParam, rows, error)) return false;

    // Pass 1 - build the pool. The reference collects BOTH itemLotId_1 and
    // itemLotId_2 (skipping -1), even though only slot 1 is ever written back.
    // That asymmetry is preserved deliberately: see docs/plans/param-features.md
    // D-1. Slot 2 therefore contributes drops to the pool but never receives
    // one, which is what the Windows tool does.
    std::vector<int32_t> pool;
    pool.reserve(rows.size() * 2);
    for (const ParamRow& row : rows) {
        if (IsExcludedNpcRow(row.id)) continue;

        int32_t lot1 = ReadI32LE(plain, row.dataOffset + kItemLot1Offset);
        int32_t lot2 = ReadI32LE(plain, row.dataOffset + kItemLot2Offset);
        if (lot1 != -1) pool.push_back(lot1);
        if (lot2 != -1) pool.push_back(lot2);
    }

    if (pool.empty()) {
        if (error) *error = "enemy drop pool is empty";
        return false;
    }

    result.poolSize = (int)pool.size();
    {
        std::set<int32_t> distinct(pool.begin(), pool.end());
        result.distinctPool = (int)distinct.size();
    }

    // Pass 2 - assign. Sampling is WITH replacement (the reference never removes
    // from the pool), so unlike treasure this is not a permutation: one drop can
    // be handed to many enemies.
    //
    // Deviation: the reference draws with Next(0, count - 1), which can never
    // select the last pool entry. That off-by-one is FIXED here - the full range
    // is used - per the decision recorded in the plan.
    std::uniform_int_distribution<int> pick(0, (int)pool.size() - 1);
    std::set<int32_t> assigned;

    for (const ParamRow& row : rows) {
        if (IsExcludedNpcRow(row.id)) continue;

        const size_t at = row.dataOffset + kItemLot1Offset;
        int32_t current = ReadI32LE(plain, at);
        if (current == -1) continue;

        int32_t chosen = current;
        for (int attempt = 0; attempt < kRerollCap; attempt++) {
            int32_t candidate = pool[(size_t)pick(rng)];
            if (candidate != current) {
                chosen = candidate;
                break;
            }
        }
        if (chosen == current) continue; // reroll exhausted - leave it vanilla

        WriteI32LE(plain, at, chosen);
        assigned.insert(chosen);
        result.rowsChanged++;
    }

    result.distinctAssigned = (int)assigned.size();

    Log(("enemy drops: pool " + std::to_string(result.poolSize) + " (" +
         std::to_string(result.distinctPool) + " distinct), reassigned " +
         std::to_string(result.rowsChanged) + " rows using " +
         std::to_string(result.distinctAssigned) + " distinct lots").c_str());
    return true;
}

} // namespace bbr
