// cstdlib before anything pulling <random>/<cmath>, or the toolchain's libc++
// fails on ::abs - same ordering constraint as BossRandomizer.cpp.
#include <cstdlib>

#include "TeamType.h"

#include "../Platform/Log.h"

namespace bbr {

namespace {

// Byte offset within an NpcParam row, from tools/param_offsets.py against the
// real paramdef: cell 100 = teamType, u8, enum NPC_TEAM_TYPE, at byte 303 of a
// 388-byte row. The neighbours are npcType at 302 and moveType at 304, so an
// off-by-one here is a silent behavioural change to every creature in the game
// and still produces a plausible-looking tree - tools/team_type_verify.py case
// S5 exists to catch exactly that.
const size_t kTeamTypeOffset = 303;

// The reference writes this literal value into every row and nothing else:
//   byte eight = 25;
//   currentParam.Rows[i].Cells[100].Value = eight;
// (RandomizeFunctions.cs:3293-3298). What 25 MEANS is not established - see
// TeamType.h - and the value is ported verbatim rather than reasoned about.
const uint8_t kOneTeamTypeValue = (uint8_t)25;

} // namespace

bool ApplyOneTeamType(std::vector<uint8_t>& plain,
                      const ParamMember& npcParam,
                      TeamTypeResult& result,
                      std::string* error) {
    std::vector<ParamRow> rows;
    if (!ParseParamRows(plain, npcParam, rows, error)) return false;

    for (const ParamRow& row : rows) {
        // ParseParamRows only proves the row STARTS inside the member, so check
        // the field itself is in the buffer. Structural, not a filter: a row
        // that fails this means the archive is not the one this was measured
        // against, and the run ends rather than the row being skipped.
        const size_t at = row.dataOffset + kTeamTypeOffset;
        if (at >= plain.size()) {
            if (error) {
                *error = "NpcParam row " + std::to_string(row.id) +
                         " teamType runs past the end of the archive";
            }
            return false;
        }

        if (plain[at] != kOneTeamTypeValue) result.rowsChanged++;
        plain[at] = kOneTeamTypeValue;
        result.rowsWritten++;
    }

    Log(("team type: set " + std::to_string(result.rowsWritten) +
         " NpcParam rows to team type " + std::to_string((int)kOneTeamTypeValue) +
         " (" + std::to_string(result.rowsChanged) + " were not already on it)").c_str());
    return true;
}

} // namespace bbr
