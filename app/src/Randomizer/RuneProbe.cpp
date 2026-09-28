// cstdlib before anything pulling <random>/<cmath>, or the toolchain's libc++
// fails on ::abs - same ordering constraint as BossRandomizer.cpp.
#include <cstdlib>

#include "RuneProbe.h"

#include "CharaInitRows.h"
#include "../Msb/BinUtil.h"
#include "../Platform/Log.h"

namespace bbr {

namespace {

// equip_Accessory01..05, s32, stride 4. Not in CharaInitRows.h because no
// shipped feature writes them - if this probe becomes a feature, they move.
const size_t kAccessoryBase  = 64;
const int    kAccessorySlots = 5;

// Five EquipParamAccessory ids, one per slot, spread across sortId order so a
// positive result samples the table rather than one corner of it. See
// RuneProbe.h for why these five.
const int32_t kProbeAccessories[kAccessorySlots] = { 111, 100, 113, 149, 150 };

} // namespace

bool RunRuneProbe(std::vector<uint8_t>& plain,
                  const ParamMember& charaInitParam,
                  RuneProbeResult& result,
                  std::string* error) {
    std::vector<ParamRow> rows;
    if (!ParseParamRows(plain, charaInitParam, rows, error)) return false;

    for (int32_t id : kOriginRows) {
        const ParamRow* row = FindRow(rows, id);
        if (row == nullptr) {
            result.rowsMissing++;
            continue;
        }

        if (row->dataOffset + kRowBytes > plain.size()) {
            if (error) {
                *error = "CharaInitParam row " + std::to_string(id) +
                         " runs past the end of the archive";
            }
            return false;
        }

        // Written unconditionally rather than into the first empty slot. Every
        // origin row has all five accessory slots at -1 on vanilla - measured -
        // and an empty-slot search was what silently skipped 12 rows in the
        // first probe, where "empty" turned out to be 0 in some rows and -1 in
        // others. A probe that quietly writes fewer rows than it claims is the
        // one failure mode that would make this result unreadable.
        for (int s = 0; s < kAccessorySlots; s++) {
            WriteI32LE(plain, row->dataOffset + kAccessoryBase + (size_t)s * 4,
                       kProbeAccessories[s]);
            result.accessoryWrites++;
        }
        result.rowsChanged++;
    }

    // Logged in full: the log is the only record that survives a hardware run,
    // and without the counts a blank Memory Altar cannot be told apart from a
    // probe that never wrote.
    std::string ids;
    for (int s = 0; s < kAccessorySlots; s++) {
        if (s) ids += ", ";
        ids += std::to_string(kProbeAccessories[s]);
    }
    Log(("rune probe 2: " + std::to_string(result.rowsChanged) + " rows written, " +
         std::to_string(result.accessoryWrites) + " accessory slots; ids " + ids +
         "; rows missing " + std::to_string(result.rowsMissing)).c_str());
    return true;
}

} // namespace bbr
