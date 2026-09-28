// cstdlib before anything pulling <random>/<cmath>, or the toolchain's libc++
// fails on ::abs - same ordering constraint as BossRandomizer.cpp.
#include <cstdlib>

#include "HunterTools.h"

#include "CharaInitRows.h"

#include "../Msb/BinUtil.h"
#include "../Platform/Log.h"

namespace bbr {

namespace {

// The two goods rows, confirmed through ItemLotParam against the real vanilla
// tree rather than taken from any document: lot 2411000 yields id 4103 at
// category 4 (goods), and lot 2200360 yields id 4104. Those are the two lots
// TreasureRandomizer.cpp already protects as the workshop tools.
const int32_t kBloodGemWorkshopTool = 4103;
const int32_t kRuneWorkshopTool     = 4104;
const int32_t kTools[] = { kBloodGemWorkshopTool, kRuneWorkshopTool };

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
