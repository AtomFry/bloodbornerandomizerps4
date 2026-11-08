// cstdlib before anything pulling <random>/<cmath>, or the toolchain's libc++
// fails on ::abs - same ordering constraint as StartingWeapons.cpp.
#include <cstdlib>

#include "ShopStock.h"

#include "ShopLineup.h"

#include "../Msb/BinUtil.h"
#include "../Platform/Log.h"

namespace bbr {

namespace {

// One bucket's permutation, and the ONE filter both of its lists come from.
//
// WHY THE TWO LISTS ARE BUILT IN ONE PASS. `pool` is the multiset of ids the
// listings in `targets` currently hold, and the exactness of the permutation
// rests on it being precisely that: draw-and-erase empties the pool on the last
// target, so every id is dealt exactly once and nothing is invented. Build the
// two lists from filters that differ by one condition and the permutation stops
// being a permutation - either the pool runs dry early or ids are left over.
//
// Reproduces the reference's Next + RemoveAt selection sampling
// (RandomizeFunctions.cs:609-700) rather than the Fisher-Yates feature 5 uses.
// The two are distributionally identical; this one traces line for line.
int PermuteBucket(std::vector<uint8_t>& plain,
                  const std::vector<ParamRow>& shopRows,
                  uint8_t equipType,
                  std::mt19937& rng) {
    std::vector<const ParamRow*> targets;
    std::vector<int32_t> pool;
    for (const ParamRow& row : shopRows) {
        if (plain[row.dataOffset + kShopEquipType] != equipType) continue;
        int32_t equipId = ReadI32LE(plain, row.dataOffset + kShopEquipId);
        if (IsNeverTouchedEquipId(equipId)) continue;
        targets.push_back(&row);
        pool.push_back(equipId);
    }

    int changed = 0;
    for (size_t i = 0; i < targets.size(); i++) {
        // An empty pool would make the distribution below (0, -1), which is
        // undefined behaviour. It cannot happen on real data - the pool starts
        // the same size as targets - and this is the guard that keeps that
        // true, the way the reference's own Count > 0 test does.
        if (pool.empty()) break;
        std::uniform_int_distribution<int> pick(0, (int)pool.size() - 1);
        int index = pick(rng);
        int32_t chosen = pool[(size_t)index];
        pool.erase(pool.begin() + (long)index);

        int32_t before = ReadI32LE(plain, targets[i]->dataOffset + kShopEquipId);
        WriteI32LE(plain, targets[i]->dataOffset + kShopEquipId, chosen);
        if (before != chosen) changed++;
    }

    Log(("shop stock: equipType " + std::to_string((int)equipType) + " - " +
         std::to_string(targets.size()) + " listings permuted, " +
         std::to_string(changed) + " actually changed").c_str());
    return changed;
}

} // namespace

bool RandomizeShopStock(std::vector<uint8_t>& plain,
                        const ParamMember& shopParam,
                        const ShopStockOptions& options,
                        std::mt19937& rng,
                        ShopStockResult& result,
                        std::string* error) {
    if (!options.Any()) return true;

    std::vector<ParamRow> shopRows;
    if (!ParseParamRows(plain, shopParam, shopRows, error)) return false;

    // Armour first, then items, and a bucket that is off is not walked at all.
    // The order is fixed rather than incidental: it is what makes a seed plus a
    // SET of settings reproduce a run's shops (spec 011 §7.10).
    if (options.randomizeShopArmour) {
        result.armourRowsChanged = PermuteBucket(plain, shopRows, kEquipTypeArmour, rng);
    }
    if (options.randomizeShopItems) {
        result.itemRowsChanged = PermuteBucket(plain, shopRows, kEquipTypeGoods, rng);
    }

    return true;
}

} // namespace bbr
