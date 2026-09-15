// StartingWeaponLists.h - the reference WPF tool's rightHandList / leftHandList
// (RandomizeFunctions.cs), transcribed mechanically from the C# source rather
// than by hand.
//
// The reference uses both purely as membership tests
// (`rightHandList.Contains(candidate)`), so duplicate entries in the original
// carry no meaning and are collapsed here: rightHandList had 79 entries /
// 76 unique, leftHandList 16 / 14.
//
// One deliberate fix (deviation SW-4, see docs/plans/starting-weapons.md): the
// reference's melee list contains "/29000000" with a stray leading slash, so
// that weapon can never match and is silently unusable as a starting weapon.
// The slash is dropped here. Evidence it is a typo rather than intent: the
// weapon is real and sold in the shop, its sibling variant 29020000 is in the
// list unslashed, and the three genuine duplicates in the original list are
// not slashed - so "/" was never a disable convention.
//
// Every id below is a real EquipParamWeapon row, and all of them are sold
// somewhere in ShopLineupParam - verified against the vanilla data.
#pragma once

#include <array>
#include <cstdint>

namespace bbr {

// Weapons valid for the three starting melee slots (ShopLineupParam rows
// 2000/2001/2002).
inline const std::array<int32_t, 76>& StartingMeleeWeaponIds() {
    static const std::array<int32_t, 76> kList = {
        2000000, 2010000, 2020000, 4000000, 4010000, 4020000,
        5000000, 5010000, 5020000, 5100000, 5110000, 5120000,
        7000000, 7010000, 7020000, 7100000, 7120000, 8000000,
        8010000, 8020000, 8100000, 8110000, 8120000, 9010000,
        9020000, 10000000, 10010000, 10020000, 10100000, 10110000,
        10120000, 11000000, 11010000, 11020000, 12000000, 12010000,
        12020000, 13000000, 13010000, 13020000, 22000000, 22010000,
        22020000, 23000000, 23010000, 23020000, 24000000, 24010000,
        24020000, 25000000, 25010000, 25020000, 26000000, 26010000,
        26020000, 27000000, 27010000, 27020000, 28000000, 28010000,
        28020000, 29000000, 29010000, 29020000, 30000000, 30010000,
        30020000, 31000000, 31010000, 31020000, 32000000, 32010000,
        32020000, 38000000, 38010000, 38020000,
    };
    return kList;
}

// Firearms valid for the two starting gun slots (rows 2010/2011).
inline const std::array<int32_t, 14>& StartingFirearmIds() {
    static const std::array<int32_t, 14> kList = {
        6000000, 6100000, 14000000, 14100000, 14200000, 15000000,
        18000000, 18100000, 19100000, 20000000, 33000000, 34000000,
        35000000, 36000000,
    };
    return kList;
}

inline bool IsMeleeWeaponId(int32_t id) {
    for (int32_t v : StartingMeleeWeaponIds()) {
        if (v == id) return true;
    }
    return false;
}

inline bool IsFirearmId(int32_t id) {
    for (int32_t v : StartingFirearmIds()) {
        if (v == id) return true;
    }
    return false;
}

} // namespace bbr
