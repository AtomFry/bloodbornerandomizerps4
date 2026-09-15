// StartingWeapons.h - randomizes the weapons offered in Hunter's Dream, and
// optionally the Messengers' shop stock. Ported from the reference WPF tool's
// RandomizeShopItems (RandomizeFunctions.cs:609-1315) but deliberately
// restructured - see docs/plans/starting-weapons.md for the trace and the
// reasoning behind each difference.
//
// What the player sees: on the first visit to Hunter's Dream there is a choice
// of three melee weapons and a choice of two firearms. Those are five rows in
// ShopLineupParam (2000/2001/2002 and 2010/2011).
//
// The reference exposes this as ONE toggle that randomizes every weapon sold
// anywhere in the game (644 rows), plus a second negative toggle to opt the
// guns back out. This port splits it into three independent positive toggles
// so each does what its name says:
//
//   randomizeStartingWeapons - the three melee choices only
//   randomizeStartingGuns    - the two firearm choices only
//   randomizeShopWeapons     - the Messengers' stock, excluding those five
//
// Like enemy drops, this edits the item-data archive in place: every write is
// one integer over another inside an existing row, nothing is resized, so the
// archive is never rebuilt. See Param/ParamBnd.h.
#pragma once

#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include "../Param/ParamBnd.h"

namespace bbr {

struct StartingWeaponOptions {
    bool randomizeStartingWeapons = false;
    bool randomizeStartingGuns = false;
    bool randomizeShopWeapons = false;

    bool Any() const {
        return randomizeStartingWeapons || randomizeStartingGuns || randomizeShopWeapons;
    }
};

struct StartingWeaponsResult {
    int meleeSlotsChanged = 0;
    int gunSlotsChanged = 0;
    int shopRowsChanged = 0;
    int statRowsRewritten = 0;  // base weapons + their upgrade tiers
};

// Applies whichever of the three features are enabled, in place inside `plain`
// (the decompressed archive). Returns false only on a structural problem, with
// `error` set; partial edits are possible in that case, so the caller should
// treat a failure as fatal to the run rather than continuing.
bool RandomizeStartingWeapons(std::vector<uint8_t>& plain,
                              const ParamMember& shopParam,
                              const ParamMember& weaponParam,
                              const StartingWeaponOptions& options,
                              std::mt19937& rng,
                              StartingWeaponsResult& result,
                              std::string* error);

} // namespace bbr
