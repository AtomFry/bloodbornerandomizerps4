// DropRandomizer.h - the C++ port of the reference WPF tool's "Randomize Enemy
// Drops" feature (RandomizeFunctions.cs:3098-3171). Written against
// docs/plans/param-features.md; read that first for the trace and the decisions.
//
// What it randomizes: which item lot an enemy drops when killed. That lives in
// NpcParam, not in any map file - which is why this feature needs the item-data
// archive (gameparam.parambnd.dcx) while enemies/bosses/treasure did not.
//
// The archive is never rebuilt. Each edit overwrites one 4-byte integer inside
// an existing row with another, so the decompressed buffer is poked in place
// and re-emitted. See Param/ParamBnd.h for why that avoids needing a PARAMDEF
// parser or a param writer.
//
// Field mapping, resolved from the real paramdef by tools/param_offsets.py
// (the reference source only ever says "Cells[11]" / "Cells[12]"):
//   cell 11 = itemLotId_1, s32, at byte 44 of a 388-byte NpcParam row
//   cell 12 = itemLotId_2, s32, at byte 48
#pragma once

#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include "../Param/ParamBnd.h"

namespace bbr {

struct DropRandomizerResult {
    int poolSize = 0;        // item lots collected
    int distinctPool = 0;    // how many of those are distinct values
    int rowsChanged = 0;     // NpcParam rows whose drop was reassigned
    int distinctAssigned = 0;// distinct values actually placed
};

// Randomizes enemy drops in place inside `plain` (the decompressed archive).
// `npcParam` must be the located NpcParam.param member. Returns false only on a
// structural problem, in which case `error` is set and nothing was modified.
bool RandomizeEnemyDrops(std::vector<uint8_t>& plain,
                         const ParamMember& npcParam,
                         std::mt19937& rng,
                         DropRandomizerResult& result,
                         std::string* error);

} // namespace bbr
