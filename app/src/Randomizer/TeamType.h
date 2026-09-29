// TeamType.h - NO TEAM TYPE, the C++ port of the reference WPF tool's
// TeamTypeRando() (RandomizeFunctions.cs:3287-3296). Shipped under the label
// ENEMIES ON SAME TEAM. See docs/features/027-no-team-type/.
//
// What it does: writes the fixed value 25 into NpcParam.teamType in EVERY row,
// with no exclusion of any kind - no protection list, no NPC or boss exemption,
// no map scoping, no row-id skip. Every creature in the game therefore ends on
// one shared allegiance, including the Hunter's Dream residents and chalice
// creatures (the table is global). That is exactly what the reference does, and
// matching it is the rule in CLAUDE.md §7.
//
// NOT a randomizer: it draws no randomness at all, so the same seed produces
// the same world with it on or off - the same property ENABLE MERGO DARKNESS
// and START WITH HUNTER TOOLS have. This is enforced by the signature below,
// which is given no std::mt19937 to draw from, rather than by a comment.
//
// It is independent of every other setting. Because every row ends holding the
// same value, it does not matter which creature the enemy, boss or easy-mode
// pass put in a spot, nor whether the drop pass ran. The other NpcParam writer
// (DropRandomizer.cpp) touches bytes 44 and 48 of the same 388-byte row, so the
// two are provably disjoint and no ordering constraint exists between them.
//
// What it edits: NpcParam.param, in place inside the decompressed archive -
// one 1-byte store over an existing field, nothing resized and nothing rebuilt.
// See Param/ParamBnd.h for why that avoids needing a param writer. The
// reference re-serialises every member through PARAM.Write(); this port pokes
// bytes instead (plan-evidence.md §E1).
//
// UNVERIFIED, and it is the whole question this feature asks: we do not know
// what Bloodborne does with the value. The label asserts an effect this project
// has never observed, 25 is not known to be a "hostile to everything" marker,
// and the 378 rows that already hold 25 in vanilla are not known to be hostile
// to anything. docs/known-traps.md: byte decoding does not prove game
// behaviour. Only the hardware test settles it, and a null result there is a
// finding about the label, not a failure of this pass.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../Param/ParamBnd.h"

namespace bbr {

struct TeamTypeResult {
    int rowsWritten = 0;  // rows the pass stored into - every row, so 31398
    int rowsChanged = 0;  // of those, how many did not already hold 25
};

// Puts every NpcParam row onto the one team type, in place inside `plain` (the
// decompressed archive). Deterministic and exhaustive: no row is skipped, and
// no byte outside teamType is touched.
//
// Returns false only on a structural problem, with `error` set; partial edits
// are possible in that case, so the caller should treat a failure as fatal to
// the run rather than continuing.
bool ApplyOneTeamType(std::vector<uint8_t>& plain,
                      const ParamMember& npcParam,
                      TeamTypeResult& result,
                      std::string* error);

} // namespace bbr
