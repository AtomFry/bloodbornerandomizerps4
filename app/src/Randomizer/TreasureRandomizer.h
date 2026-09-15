// TreasureRandomizer.h - the C++ port of the reference WPF tool's treasure /
// world-pickup randomization (GenerateItemLotList + RandomizeItemLots, plus the
// nonoItemLots setup in StartFunctions.cs). Written against
// docs/plans/treasure-randomization.md, which is the contract this and
// tools/treasure_verify.py are both implemented from - read that first; it
// records the rules, the measurements taken against real vanilla data, and the
// deliberate deviations (T1-T3).
//
// A treasure is an Event of type 0x4 whose ItemLot1 field names the item lot it
// grants. Randomizing treasure permutes which item lot sits at which pickup.
// It does NOT touch enemy drops or shop contents - those live in
// gameparam.parambnd.dcx and need the deferred PARAM stack.
//
// Everything here mutates already-parsed MsbbFile objects in memory. No file
// I/O: EnemyRandomizer owns reading and writing.
#pragma once

#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include "../Msb/Msbb.h"

namespace bbr {

struct TreasurePool {
    // Item lots harvested from eligible treasures, drained as they are placed
    // (spec §2.3 - without replacement, so the whole thing is a permutation).
    std::vector<int32_t> lots;
    int collected = 0;
};

// The maps treasure randomization runs over, in the reference tool's own order
// (StartFunctions.cs:1293-1316 / 1326-1349). Two things about this list are
// deliberate reproductions of reference quirks, both documented in spec §2.4:
// m21_00_00_00 (Hunter's Dream) is absent, and m21_01_00_00 (Abandoned Old
// Workshop) appears TWICE. Both the collect pass and the assign pass walk this
// same list, which is what keeps pool size and destination count balanced.
const std::vector<std::string>& TreasureMapOrder();

// True for the item lots the reference never moves or overwrites (spec §2.1):
// three base entries, six key items, and - unless randomizeWorkshopTools - the
// two workshop tools.
bool IsProtectedItemLot(int32_t lot, bool randomizeWorkshopTools);

// Spec §2.2 pool eligibility: ItemLot1 > 1 and not protected. Call once per
// entry in TreasureMapOrder().
void CollectTreasureLots(const MsbbFile& msbb, bool randomizeWorkshopTools,
                         TreasurePool& pool);

// Spec §2.2 location eligibility: ItemLot1 > 0 and not protected. Note the
// bound differs from the collect pass (> 0 here, > 1 there) - that asymmetry is
// real in the reference and is preserved; see the spec for why it is currently
// inert. Returns the number of treasures changed.
int AssignTreasureLots(const std::string& mapName, MsbbFile& msbb,
                       bool randomizeWorkshopTools, TreasurePool& pool,
                       std::mt19937& rng);

} // namespace bbr
