// cstdlib before anything pulling <random>/<cmath>, or the toolchain's libc++
// fails on ::abs - same ordering constraint as BossRandomizer.cpp.
#include <cstdlib>

#include "TreasureRandomizer.h"

#include "../Platform/Log.h"

namespace bbr {

namespace {

// Spec §2.1. The trailing comments are the reference author's own annotations,
// several of which end in a question mark in the original source - they are
// reproduced verbatim as unverified labels, not as established fact.
const int32_t kBaseProtectedLots[] = {
    2600550, // Evil Eye Bridge key
    2400450, // key to the Old Town
    3500800, // key to the dungeon usually door
};

const int32_t kKeyItemLots[] = {
    2800290, // key to Cathedral Street C (UCW key)
    3200720, // key to nightmare classroom
    3200810, // Veranda of key (key to rom fight)
    2410990, // Invitation to the castle (cainhurst summons)
    3502000, // Parish length Startup Item (laurence skull)
    3401810, // Altar Elevator Startup Item (eye pendant)
};

// Unlike the rest of this file's labels, these two are CONFIRMED, not
// reference-author guesses (docs/plans/workshop-tools.md §2.2): each
// resolves through ItemLotParam to a category-4 goods item present in the
// English item-name table, and 2411000's MSB event name itself is
// "Item_..._1000_..." - blood gem attach/detach unlock.
const int32_t kWorkshopToolLots[] = {
    2411000, // Blood Gem Workshop Tool (chest after Gascoigne, Central Yharnam)
    2200360, // Rune Workshop Tool (chest after the Witch of Hemwick)
};

} // namespace

const std::vector<std::string>& TreasureMapOrder() {
    static const std::vector<std::string> kOrder = {
        // m21_00_00_00 (Hunter's Dream) is deliberately absent - commented out
        // in the reference. m21_01_00_00 is deliberately listed twice.
        "m21_01_00_00", "m21_01_00_00",
        "m22_00_00_00", "m23_00_00_00", "m23_00_00_01",
        "m24_00_00_00", "m24_00_00_01",
        "m24_01_00_00", "m24_01_00_01", "m24_01_00_11",
        "m24_02_00_00", "m24_02_00_01",
        "m25_00_00_00", "m26_00_00_00",
        "m27_00_00_00", "m27_00_00_01",
        "m28_00_00_00", "m28_00_00_01",
        "m32_00_00_00", "m32_00_00_01",
        "m33_00_00_00", "m34_00_00_00", "m35_00_00_00", "m36_00_00_00",
    };
    return kOrder;
}

bool IsProtectedItemLot(int32_t lot, bool randomizeWorkshopTools) {
    for (int32_t p : kBaseProtectedLots) {
        if (lot == p) return true;
    }
    for (int32_t p : kKeyItemLots) {
        if (lot == p) return true;
    }
    if (!randomizeWorkshopTools) {
        for (int32_t p : kWorkshopToolLots) {
            if (lot == p) return true;
        }
    }
    return false;
}

void CollectTreasureLots(const MsbbFile& msbb, bool randomizeWorkshopTools,
                         TreasurePool& pool) {
    for (const std::vector<uint8_t>& blob : msbb.events.entries) {
        if (event_fields::GetType(blob) != EventType::kTreasure) continue;

        int32_t lot = event_fields::GetTreasureItemLot1(blob);
        if (IsProtectedItemLot(lot, randomizeWorkshopTools)) continue;
        if (lot <= 1) continue; // spec §2.2: pool bound is > 1

        pool.lots.push_back(lot);
        pool.collected++;
    }
}

int AssignTreasureLots(const std::string& mapName, MsbbFile& msbb,
                       bool randomizeWorkshopTools, TreasurePool& pool,
                       std::mt19937& rng) {
    int changed = 0;

    for (std::vector<uint8_t>& blob : msbb.events.entries) {
        if (event_fields::GetType(blob) != EventType::kTreasure) continue;

        int32_t lot = event_fields::GetTreasureItemLot1(blob);
        if (IsProtectedItemLot(lot, randomizeWorkshopTools)) continue;
        if (lot <= 0) continue; // spec §2.2: location bound is > 0

        // Deviation T1: the reference checks the pool only once at function
        // entry and would index an emptied list mid-loop. Leaving the location
        // vanilla is always valid, and on measured data this never triggers -
        // pool size and destination count are equal by construction.
        if (pool.lots.empty()) {
            Log((mapName + ": treasure pool exhausted, remaining pickups left vanilla").c_str());
            break;
        }

        std::uniform_int_distribution<int> d(0, (int)pool.lots.size() - 1);
        int index = d(rng);

        event_fields::SetTreasureItemLot1(blob, pool.lots[(size_t)index]);
        pool.lots.erase(pool.lots.begin() + (long)index);
        changed++;
    }

    // Deviation T3: a summary per map rather than the reference's per-item log.
    if (changed > 0) {
        Log((mapName + ": " + std::to_string(changed) + " treasures randomized, " +
             std::to_string(pool.lots.size()) + " lots left in pool").c_str());
    }
    return changed;
}

} // namespace bbr
