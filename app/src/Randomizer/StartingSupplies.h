// StartingSupplies.h - START WITH SUPPLIES. Puts 20 Blood Vials and 20
// Quicksilver Bullets into a new character's starting inventory.
//
// Why this exists: a randomized run can put something lethal in the first room,
// and vanilla starts a character with no vials and no bullets at all - the first
// few of each are a pickup or a purchase away. A run that ends before the player
// has either is not a harder run, it is a shorter one.
//
// WHY 20 AND WHY THESE TWO. The count is not a chosen number: 20 is each of
// those two goods rows' own maxNum in EquipParamGoods, which is the most of
// either a character can hold, and starting_gear_verify.py's A9 asserts that
// rather than trusting this comment. Two consumables and no more, no count
// setting and no consumables picker (spec 040 D2): the setting is one toggle
// because there is nothing here a player would want to choose.
//
// THE SAME SHAPE AS HunterTools.cpp, and copied in shape only - that file is not
// extended and not called. Per origin row, per item: skip the item if the row
// already lists it, otherwise take the first EMPTY slot and write the id and the
// count. Both features on fill four of the ten slots and neither can take the
// other's, because neither ever names a slot index; each searches.
//
// THE COUNT IS A SINGLE BYTE. itemNum_01..itemNum_10 are u8 and sit 80 bytes
// past the end of the s32 item ids, so a slot is two writes at two offsets and
// the second is one byte wide. A four-byte write there would silently corrupt
// three neighbours - the same trap CharaInitRows.h and StartingWeapons.cpp both
// document.
//
// IT DRAWS NOTHING, so it commutes with every pass in the run: the same seed
// produces the same world with this on or off, exactly like START WITH HUNTER
// TOOLS, and it can be called before or after any other pass with identical
// bytes either way. It is also idempotent over an already-edited buffer, because
// RowHasItem is what decides whether to write at all.
//
// UNVERIFIED, and unlike the hunter tools there is not much to doubt: the game
// is being asked to start the character with a consumable in an inventory slot,
// which is what slot 0's Hunter's Mark already is. What the console settles is
// whether the vials arrive at all and whether they arrive WITHOUT displacing the
// Mark or a hunter tool - that second half is the one that fails silently, and it
// is why every write here goes through FirstEmptySlot.
//
// What it edits: the ten item_NN / itemNum_NN pairs of the 22 player-origin rows
// of CharaInitParam.param, and nothing else. The archive is never rebuilt: each
// write is one value over another inside an existing row. The six
// secondaryItem_NN slots are deliberately untouched (spec 040 §3.2) - the first
// rune probe found "empty" meaning 0 rather than -1 in those, so FirstEmptySlot's
// test is right for item_NN and must not be extended to them.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../Param/ParamBnd.h"

namespace bbr {

struct StartingSuppliesResult {
    int rowsChanged = 0;   // origin rows that received at least one supply
    int slotsWritten = 0;  // item slots filled across all of those rows
    int rowsMissing = 0;   // targets absent from the real param - see kOriginRows
    int rowsFull = 0;      // targets with no free item slot (never seen on vanilla)
};

// Writes both supplies into every origin row's starting inventory, in place
// inside `plain` (the decompressed archive). Deterministic - it draws no
// randomness, so the same seed produces the same world with it on or off.
//
// Returns false only on a structural problem, with `error` set; partial edits
// are possible in that case, so the caller should treat a failure as fatal to
// the run rather than continuing.
bool GrantStartingSupplies(std::vector<uint8_t>& plain,
                           const ParamMember& charaInitParam,
                           StartingSuppliesResult& result,
                           std::string* error);

} // namespace bbr
