// ShopLineup.h - the ShopLineupParam field offsets, the three equipType values
// and the one skip list, shared by every feature that writes that table.
//
// TWO FEATURES WRITE ShopLineupParam AND THEY MUST NOT DRIFT APART. Feature 5
// (RANDOMIZE STARTING WEAPONS / GUNS / SHOP WEAPONS) permutes the equipType 0
// rows; feature 11 (RANDOMIZE SHOP ARMOUR / SHOP ITEMS) permutes the equipType 1
// and equipType 3 rows. They share the table, not a feature - so the constants
// live here and the passes live in StartingWeapons.cpp and ShopStock.cpp.
//
// WHY THE SKIP LIST IS HERE AND NOWHERE ELSE. The reference tool refuses to
// touch any listing selling item id 1000, 900 or 240 - Blood Vials, Quicksilver
// Bullets and one id with no row anywhere (RandomizeFunctions.cs:635-638). Both
// features must protect exactly the same three ids: a second copy would let the
// two protections diverge, and a shop that stops selling Blood Vials is the one
// failure that ends a playthrough (spec 011 §7.2).
//
// THE BUCKET COMES FROM equipType, NEVER FROM THE SHAPE OF AN ID. Weapon,
// armour and goods ids are small integers in overlapping numeric ranges, and
// five goods ids also exist as EquipParamProtector row ids. The only reliable
// statement about which table an equipId points into is the listing's own
// equipType byte.
#pragma once

#include <cstddef>
#include <cstdint>

namespace bbr {

// Field offsets, from tools/param_offsets.py against the real paramdef.
const size_t kShopEquipId   = 0;   // ShopLineupParam.equipId,   s32, 32-byte row
const size_t kShopEquipType = 23;  // ShopLineupParam.equipType, u8

// Which table equipId indexes. 1288 listings in the real archive split
// 644 / 220 / 424 across these three, and that partition is what makes the two
// features non-interfering (spec 011 §4 F11).
const uint8_t kEquipTypeWeapon = 0;  // EquipParamWeapon
const uint8_t kEquipTypeArmour = 1;  // EquipParamProtector
const uint8_t kEquipTypeGoods  = 3;  // EquipParamGoods

// The reference skips any shop row selling one of these regardless of type
// (RandomizeFunctions.cs:635-638). On real data all 27 such listings are
// equipType 3, so it changes nothing for weapons and everything for items.
const int32_t kNeverTouchEquipIds[] = { 1000, 900, 240 };

inline bool IsNeverTouchedEquipId(int32_t equipId) {
    for (int32_t v : kNeverTouchEquipIds) {
        if (equipId == v) return true;
    }
    return false;
}

} // namespace bbr
