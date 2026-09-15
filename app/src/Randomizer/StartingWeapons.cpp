// cstdlib before anything pulling <random>/<cmath>, or the toolchain's libc++
// fails on ::abs - same ordering constraint as BossRandomizer.cpp.
#include <cstdlib>

#include "StartingWeapons.h"

#include "StartingWeaponLists.h"

#include "../Msb/BinUtil.h"
#include "../Platform/Log.h"

namespace bbr {

namespace {

// Field offsets, from tools/param_offsets.py against the real paramdef.
const size_t kShopEquipId   = 0;   // ShopLineupParam.equipId,   s32, 32-byte row
const size_t kShopEquipType = 23;  // ShopLineupParam.equipType, u8
const uint8_t kEquipTypeWeapon = 0;

// EquipParamWeapon stat requirements are u8 - single bytes, NOT int32 like the
// drop field. Writing four bytes here would silently corrupt neighbours.
const size_t kProperStrength = 237;
const size_t kProperAgility  = 238;
const size_t kProperMagic    = 239;
const size_t kProperFaith    = 240;

// The reference skips any shop row selling one of these regardless of type
// (RandomizeFunctions.cs:635-638). Reproduced for fidelity; on real data none
// of them are weapon rows, so it changes nothing today.
const int32_t kNeverTouchEquipIds[] = { 1000, 900, 240 };

// The five Hunter's Dream choices.
const int32_t kMeleeSlotRows[] = { 2000, 2001, 2002 };
const int32_t kGunSlotRows[]   = { 2010, 2011 };

// Stat profile applied to whatever lands in each slot, so a randomized
// starting weapon stays wieldable at level 4. Values are the reference's
// (:1106-1176), keyed by slot rather than by the vanilla weapon that was there.
struct StatProfile {
    int32_t row;
    uint8_t strength, agility, magic, faith;
};
const StatProfile kStatProfiles[] = {
    { 2000, 8, 7, 0, 0 },
    { 2001, 9, 8, 0, 0 },
    { 2002, 7, 9, 0, 0 },
    { 2010, 7, 9, 5, 0 },
    { 2011, 7, 9, 5, 0 },
};

// A weapon's ten upgrade variants are its id + 100n. Verified against the real
// EquipParamWeapon for all five vanilla starters: every tier exists.
const int kUpgradeTiers = 10;
const int32_t kUpgradeStride = 100;

bool IsNeverTouched(int32_t equipId) {
    for (int32_t v : kNeverTouchEquipIds) {
        if (equipId == v) return true;
    }
    return false;
}

bool IsStartingSlotRow(int32_t rowId) {
    for (int32_t r : kMeleeSlotRows) {
        if (rowId == r) return true;
    }
    for (int32_t r : kGunSlotRows) {
        if (rowId == r) return true;
    }
    return false;
}

const ParamRow* FindRow(const std::vector<ParamRow>& rows, int32_t id) {
    for (const ParamRow& r : rows) {
        if (r.id == id) return &r;
    }
    return nullptr;
}

// Draws distinct ids, removing as it goes so the player is never offered the
// same weapon twice in one choice.
int32_t DrawDistinct(std::vector<int32_t>& pool, std::mt19937& rng) {
    std::uniform_int_distribution<int> pick(0, (int)pool.size() - 1);
    int index = pick(rng);
    int32_t chosen = pool[(size_t)index];
    pool.erase(pool.begin() + (long)index);
    return chosen;
}

const StatProfile* ProfileForRow(int32_t rowId) {
    for (const StatProfile& p : kStatProfiles) {
        if (p.row == rowId) return &p;
    }
    return nullptr;
}

// Rewrites the four stat requirements on a weapon and each of its upgrade
// tiers. Missing tiers are skipped rather than treated as an error.
int ApplyStatProfile(std::vector<uint8_t>& plain, const std::vector<ParamRow>& weaponRows,
                     int32_t weaponId, const StatProfile& p) {
    int written = 0;
    for (int tier = 0; tier <= kUpgradeTiers; tier++) {
        const ParamRow* row = FindRow(weaponRows, weaponId + tier * kUpgradeStride);
        if (row == nullptr) continue;
        plain[row->dataOffset + kProperStrength] = p.strength;
        plain[row->dataOffset + kProperAgility]  = p.agility;
        plain[row->dataOffset + kProperMagic]    = p.magic;
        plain[row->dataOffset + kProperFaith]    = p.faith;
        written++;
    }
    return written;
}

} // namespace

bool RandomizeStartingWeapons(std::vector<uint8_t>& plain,
                              const ParamMember& shopParam,
                              const ParamMember& weaponParam,
                              const StartingWeaponOptions& options,
                              std::mt19937& rng,
                              StartingWeaponsResult& result,
                              std::string* error) {
    if (!options.Any()) return true;

    std::vector<ParamRow> shopRows;
    if (!ParseParamRows(plain, shopParam, shopRows, error)) return false;
    std::vector<ParamRow> weaponRows;
    if (!ParseParamRows(plain, weaponParam, weaponRows, error)) return false;

    // Candidate pools: every distinct weapon sold anywhere, split by hand.
    std::vector<int32_t> meleePool, gunPool;
    for (const ParamRow& row : shopRows) {
        if (plain[row.dataOffset + kShopEquipType] != kEquipTypeWeapon) continue;
        int32_t equipId = ReadI32LE(plain, row.dataOffset + kShopEquipId);
        if (IsNeverTouched(equipId)) continue;

        std::vector<int32_t>* target = nullptr;
        if (IsMeleeWeaponId(equipId)) target = &meleePool;
        else if (IsFirearmId(equipId)) target = &gunPool;
        if (target == nullptr) continue;

        bool seen = false;
        for (int32_t v : *target) {
            if (v == equipId) { seen = true; break; }
        }
        if (!seen) target->push_back(equipId);
    }

    Log(("starting weapons: pools - " + std::to_string(meleePool.size()) + " melee, " +
         std::to_string(gunPool.size()) + " firearms").c_str());

    // Slots are assigned first so the shop pass below can skip them.
    struct Assignment { int32_t row; int32_t weapon; };
    std::vector<Assignment> assigned;

    if (options.randomizeStartingWeapons) {
        for (int32_t rowId : kMeleeSlotRows) {
            const ParamRow* row = FindRow(shopRows, rowId);
            if (row == nullptr || meleePool.empty()) continue;
            int32_t chosen = DrawDistinct(meleePool, rng);
            WriteI32LE(plain, row->dataOffset + kShopEquipId, chosen);
            assigned.push_back({ rowId, chosen });
            result.meleeSlotsChanged++;
        }
    }

    if (options.randomizeStartingGuns) {
        for (int32_t rowId : kGunSlotRows) {
            const ParamRow* row = FindRow(shopRows, rowId);
            if (row == nullptr || gunPool.empty()) continue;
            int32_t chosen = DrawDistinct(gunPool, rng);
            WriteI32LE(plain, row->dataOffset + kShopEquipId, chosen);
            assigned.push_back({ rowId, chosen });
            result.gunSlotsChanged++;
        }
    }

    // Whatever landed in a starting slot has to be usable at level 4.
    for (const Assignment& a : assigned) {
        const StatProfile* p = ProfileForRow(a.row);
        if (p == nullptr) continue;
        result.statRowsRewritten += ApplyStatProfile(plain, weaponRows, a.weapon, *p);
        Log(("starting weapons: row " + std::to_string(a.row) + " -> weapon " +
             std::to_string(a.weapon)).c_str());
    }

    // The Messengers' stock, as a permutation of what it already sells. The
    // five starting rows are deliberately excluded - they belong to the two
    // toggles above, and letting both passes write the same row would make the
    // toggles interfere.
    if (options.randomizeShopWeapons) {
        std::vector<const ParamRow*> targets;
        std::vector<int32_t> stock;
        for (const ParamRow& row : shopRows) {
            if (plain[row.dataOffset + kShopEquipType] != kEquipTypeWeapon) continue;
            if (IsStartingSlotRow(row.id)) continue;
            int32_t equipId = ReadI32LE(plain, row.dataOffset + kShopEquipId);
            if (IsNeverTouched(equipId)) continue;
            targets.push_back(&row);
            stock.push_back(equipId);
        }

        // Fisher-Yates over the stock, then deal it back out in row order.
        for (size_t i = stock.size(); i > 1; i--) {
            std::uniform_int_distribution<int> pick(0, (int)i - 1);
            std::swap(stock[i - 1], stock[(size_t)pick(rng)]);
        }
        for (size_t i = 0; i < targets.size(); i++) {
            int32_t before = ReadI32LE(plain, targets[i]->dataOffset + kShopEquipId);
            WriteI32LE(plain, targets[i]->dataOffset + kShopEquipId, stock[i]);
            if (before != stock[i]) result.shopRowsChanged++;
        }
        Log(("starting weapons: shop stock permuted across " +
             std::to_string(targets.size()) + " rows, " +
             std::to_string(result.shopRowsChanged) + " actually changed").c_str());
    }

    return true;
}

} // namespace bbr
