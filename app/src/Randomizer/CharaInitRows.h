// CharaInitRows.h - the player-origin rows of CharaInitParam.param, the row
// size, the starting-inventory slot offsets, and the three helpers every
// feature that writes a starting item needs.
//
// WHY THIS IS ITS OWN HEADER. Two features now write these rows: START WITH
// HUNTER TOOLS (HunterTools.cpp) and START WITH A TRICK WEAPON
// (TrickWeaponGrant.cpp). Everything here was HunterTools.cpp's, moved out
// VERBATIM - no value changed - so row 34's output is byte-identical before and
// after the move, and so the eventual narrowing of the row list (row 34's open
// item O2) stays a one-place edit instead of two that can disagree.
//
// It carries no feature's own constants: no item id, no weapon field, nothing
// about what is being granted. Only the shape of the row and how to find room
// in it.
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
