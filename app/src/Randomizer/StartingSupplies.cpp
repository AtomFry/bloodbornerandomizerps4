// cstdlib before anything pulling <random>/<cmath>, or the toolchain's libc++
// fails on ::abs - same ordering constraint as BossRandomizer.cpp.
#include <cstdlib>

#include "StartingSupplies.h"

#include "CharaInitRows.h"

#include "../Msb/BinUtil.h"
#include "../Platform/Log.h"

namespace bbr {

namespace {

// The two goods rows, and the count. All three are pinned to the game's own data
// by starting_gear_verify.py A9 rather than taken from any document: goods 1000
// is named Blood Vial, 900 is named Quicksilver Bullets, and each carries
// maxNum == 20 - so 20 is the most of either the character can hold and is not a
// number anyone here chose.
const int32_t kBloodVial          = 1000;
const int32_t kQuicksilverBullets = 900;
const int32_t kSupplies[] = { kBloodVial, kQuicksilverBullets };

// A u8 in the row, which is why it is assigned as a byte below and not written
// with WriteI32LE. See CharaInitRows.h.
const uint8_t kSupplyCount = 20;

} // namespace

bool GrantStartingSupplies(std::vector<uint8_t>& plain,
                           const ParamMember& charaInitParam,
                           StartingSuppliesResult& result,
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
        for (int32_t item : kSupplies) {
            // What makes this pass idempotent, and what makes it safe to run
            // beside START WITH HUNTER TOOLS in either order: the question asked
            // is "is this item already here", never "which slot is mine".
            if (RowHasItem(plain, row->dataOffset, item)) continue;

            int slot = FirstEmptySlot(plain, row->dataOffset);
            if (slot < 0) {
                result.rowsFull++;
                break; // no room for this supply or the next one
            }

            WriteI32LE(plain, row->dataOffset + kItemIdBase + (size_t)slot * 4, item);
            // ONE BYTE. The counts are u8 and 80 bytes away from the ids.
            plain[row->dataOffset + kItemNumBase + (size_t)slot] = kSupplyCount;
            result.slotsWritten++;
            changed = true;
        }
        if (changed) result.rowsChanged++;
    }

    Log(("starting supplies: granted to " + std::to_string(result.rowsChanged) +
         " origin rows, " + std::to_string(result.slotsWritten) +
         " slots written, " + std::to_string(result.rowsMissing) +
         " rows missing, " + std::to_string(result.rowsFull) +
         " rows full").c_str());

    return true;
}

} // namespace bbr
