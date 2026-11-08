// CharaInitRows.h - the player-origin rows of CharaInitParam.param, the row
// size, the starting-inventory slot offsets, and the three helpers every
// feature that writes a starting item needs.
//
// WHY THIS IS ITS OWN HEADER. Six writers now touch these rows: START WITH
// HUNTER TOOLS (HunterTools.cpp), START WITH A TRICK WEAPON
// (TrickWeaponGrant.cpp), START WITH A LEFT WEAPON (LeftHandWeaponGrant.cpp),
// START WITH SUPPLIES (StartingSupplies.cpp), the four armour pickers
// (ArmourGrant.cpp) and the temporary rune probe. Everything here was
// HunterTools.cpp's, moved out VERBATIM - no value changed - so row 34's output
// is byte-identical before and after the move, and so the eventual narrowing of
// the row list (row 34's open item O2) stays a one-place edit instead of six
// that can disagree.
//
// It carries no feature's own constants: no item id, no goods id, no count,
// nothing about what is being granted. Only the shape of the row - which fields
// live where - and how to find room in it. The four clothing offsets below are
// here on exactly that ground and the two weapon offsets are not: a clothing
// field is part of the row's shape in the same way the item arrays are, whereas
// which weapon field a feature writes is that feature's own choice between two
// siblings.
#pragma once

#include <cstdint>
#include <vector>

#include "../Msb/BinUtil.h"
#include "../Param/ParamBnd.h"

namespace bbr {

// Field offsets within a 300-byte CHARACTER_INIT_PARAM row, from
// tools/param_offsets.py against the real paramdef.
//
// item_01..item_10 are s32 item ids (-1 = empty slot); itemNum_01..itemNum_10
// are the matching u8 counts. The two arrays are NOT adjacent - 80 bytes of
// other fields sit between them - so a slot needs two writes at two offsets,
// and the count is a SINGLE BYTE. Writing four bytes there would silently
// corrupt equip_Wep_Right_GenId's neighbours, the same trap
// StartingWeapons.cpp documents for the u8 stat requirements.
const size_t kItemIdBase  = 124; // item_01,    s32, stride 4
const size_t kItemNumBase = 204; // itemNum_01, u8,  stride 1
const int    kItemSlots   = 10;
const size_t kRowBytes    = 300; // computed row size; guards the bounds check

const int32_t kEmptySlot = -1;

// The four clothing fields, as an array in the game's own slot order - head,
// body, gloves, legwear - which is also protectorCategory 0/1/2/3 and the order
// of the fields themselves. equip_Helm 32, equip_Armer 36, equip_Gaunt 40,
// equip_Leg 44, each an s32 armour id.
//
// AN ARRAY AND NOT FOUR CONSTANTS, because the pass that writes them walks the
// four slots in step with four selections and four draws, and an array is what
// makes "slot i" one index rather than a switch that can be got wrong in one
// arm. Indexed by protectorCategory, which is what lets an id be checked against
// the field it is going into: starting_gear_verify.py A8 establishes that
// id/1000 % 10 equals that category on every wearable row.
//
// These are UNLIKE vanilla's two weapon fields in one way worth knowing: all
// four are POPULATED on the shipped rows - the 2000 block wears 230000 / 231000 /
// 232000 / 233000 and the 3000 block and 3500/3501 wear the same three with a
// bare head - so a write here replaces a real value rather than filling in a -1.
// That is why a picker with nothing ticked must leave its slot alone instead of
// writing anything at all.
const size_t kEquipClothing[] = {
    32, // equip_Helm,  s32 - protectorCategory 0
    36, // equip_Armer, s32 - protectorCategory 1
    40, // equip_Gaunt, s32 - protectorCategory 2
    44, // equip_Leg,   s32 - protectorCategory 3
};
const int kClothingSlots = 4;

// Every CharaInitParam row carrying the player-origin signature - exactly one
// starting item, 1x goods 100 - which on the real vanilla file is these 22 and
// nothing else. The 1658 rows that hold 5x1000 and 900 are NPC templates and
// are deliberately not here.
//
// Both ten-row blocks are written because the data cannot say which one the
// game actually reads: 2000-2009 and 3000-3009 are identical in every field
// either feature touches, differing only in equip_Helm (230000 vs -1). 3500 and
// 3501 carry the same signature and are included on the same reasoning. This
// is a hedge, and a cheap one - writing a starting item into a row the game
// never instantiates does nothing at all, whereas guessing wrong and writing
// only one block would look exactly like "the feature does not work".
//
// If the hardware test shows the grants arrive, this list can be narrowed to
// whichever block is live; it cannot be narrowed before then.
const int32_t kOriginRows[] = {
    2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009,
    3000, 3001, 3002, 3003, 3004, 3005, 3006, 3007, 3008, 3009,
    3500, 3501,
};

inline const ParamRow* FindRow(const std::vector<ParamRow>& rows, int32_t id) {
    for (const ParamRow& r : rows) {
        if (r.id == id) return &r;
    }
    return nullptr;
}

// True if this row already lists `itemId` in any slot. Guards against granting
// a duplicate if a row ever ships with the item already in it, and makes a pass
// safely re-runnable over an already-edited buffer.
inline bool RowHasItem(const std::vector<uint8_t>& plain, size_t rowOffset, int32_t itemId) {
    for (int s = 0; s < kItemSlots; s++) {
        if (ReadI32LE(plain, rowOffset + kItemIdBase + (size_t)s * 4) == itemId) return true;
    }
    return false;
}

// Index of the first empty slot, or -1 when the row is full. Vanilla origin
// rows use slot 0 and leave 1-9 at -1, so this always finds room; it is
// written as a search rather than a hardcoded slot index so that two features
// both granting a starting item cannot silently overwrite each other.
inline int FirstEmptySlot(const std::vector<uint8_t>& plain, size_t rowOffset) {
    for (int s = 0; s < kItemSlots; s++) {
        if (ReadI32LE(plain, rowOffset + kItemIdBase + (size_t)s * 4) == kEmptySlot) return s;
    }
    return -1;
}

} // namespace bbr
