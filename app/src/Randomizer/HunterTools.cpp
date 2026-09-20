// cstdlib before anything pulling <random>/<cmath>, or the toolchain's libc++
// fails on ::abs - same ordering constraint as BossRandomizer.cpp.
#include <cstdlib>

#include "HunterTools.h"

#include "../Msb/BinUtil.h"
#include "../Platform/Log.h"

namespace bbr {

namespace {

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

// The two goods rows, confirmed through ItemLotParam against the real vanilla
// tree rather than taken from any document: lot 2411000 yields id 4103 at
// category 4 (goods), and lot 2200360 yields id 4104. Those are the two lots
// TreasureRandomizer.cpp already protects as the workshop tools.
const int32_t kBloodGemWorkshopTool = 4103;
const int32_t kRuneWorkshopTool     = 4104;
const int32_t kTools[] = { kBloodGemWorkshopTool, kRuneWorkshopTool };

// Every CharaInitParam row carrying the player-origin signature - exactly one
// starting item, 1x goods 100 - which on the real vanilla file is these 22 and
// nothing else. The 1658 rows that hold 5x1000 and 900 are NPC templates and
// are deliberately not here.
//
// Both ten-row blocks are written because the data cannot say which one the
// game actually reads: 2000-2009 and 3000-3009 are identical in every field
// this feature touches, differing only in equip_Helm (230000 vs -1). 3500 and
// 3501 carry the same signature and are included on the same reasoning. This
// is a hedge, and a cheap one - writing a starting item into a row the game
// never instantiates does nothing at all, whereas guessing wrong and writing
// only one block would look exactly like "the feature does not work".
//
// If the hardware test shows the tools arrive, this list can be narrowed to
// whichever block is live; it cannot be narrowed before then.
const int32_t kOriginRows[] = {
    2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009,
    3000, 3001, 3002, 3003, 3004, 3005, 3006, 3007, 3008, 3009,
    3500, 3501,
};

const ParamRow* FindRow(const std::vector<ParamRow>& rows, int32_t id) {
    for (const ParamRow& r : rows) {
        if (r.id == id) return &r;
    }
    return nullptr;
}

// True if this row already lists `itemId` in any slot. Guards against granting
// a duplicate if a row ever ships with a tool already in it, and makes the
// whole pass safely re-runnable over an already-edited buffer.
bool RowHasItem(const std::vector<uint8_t>& plain, size_t rowOffset, int32_t itemId) {
    for (int s = 0; s < kItemSlots; s++) {
        if (ReadI32LE(plain, rowOffset + kItemIdBase + (size_t)s * 4) == itemId) return true;
    }
    return false;
}

// Index of the first empty slot, or -1 when the row is full. Vanilla origin
// rows use slot 0 and leave 1-9 at -1, so this always finds room; it is
// written as a search rather than a hardcoded "slots 1 and 2" so that turning
// this on alongside a future feature that also grants a starting item cannot
// have the two silently overwrite each other.
int FirstEmptySlot(const std::vector<uint8_t>& plain, size_t rowOffset) {
    for (int s = 0; s < kItemSlots; s++) {
        if (ReadI32LE(plain, rowOffset + kItemIdBase + (size_t)s * 4) == kEmptySlot) return s;
    }
    return -1;
}

} // namespace

bool GrantHunterTools(std::vector<uint8_t>& plain,
                      const ParamMember& charaInitParam,
                      HunterToolsResult& result,
                      std::string* error) {
    std::vector<ParamRow> rows;
    if (!ParseParamRows(plain, charaInitParam, rows, error)) return false;

    for (int32_t id : kOriginRows) {
        const ParamRow* row = FindRow(rows, id);
        if (row == nullptr) {
            // Not fatal on its own: a row missing from one block still leaves
            // the other block written. The count is reported so a wholesale
            // mismatch (a different game version, say) is visible in the log
            // rather than looking like a silent success.
            result.rowsMissing++;
            continue;
        }

        // ParseParamRows only proves the row STARTS inside the member; the
        // fields written below sit up to 213 bytes in, so check the whole row
        // fits before touching any of it.
        if (row->dataOffset + kRowBytes > plain.size()) {
            if (error) {
                *error = "CharaInitParam row " + std::to_string(id) +
                         " runs past the end of the archive";
            }
            return false;
        }

        bool changed = false;
        for (int32_t tool : kTools) {
            if (RowHasItem(plain, row->dataOffset, tool)) continue;

            int slot = FirstEmptySlot(plain, row->dataOffset);
            if (slot < 0) {
                result.rowsFull++;
                break; // no room for this tool or the next one
            }

            WriteI32LE(plain, row->dataOffset + kItemIdBase + (size_t)slot * 4, tool);
            plain[row->dataOffset + kItemNumBase + (size_t)slot] = 1;
            result.slotsWritten++;
            changed = true;
        }
        if (changed) result.rowsChanged++;
    }

    Log(("hunter tools: granted to " + std::to_string(result.rowsChanged) + " origin rows, " +
         std::to_string(result.slotsWritten) + " slots written, " +
         std::to_string(result.rowsMissing) + " rows missing").c_str());

    return true;
}

} // namespace bbr
