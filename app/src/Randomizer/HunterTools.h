// HunterTools.h - START WITH HUNTER TOOLS. Puts the two workshop key items
// into the player's starting inventory so blood gems and Caryll runes can be
// used from the first minute of a run.
//
// Why this exists: the randomizer hands out gems and runes from the very first
// area, but vanilla gates the ability to FIT either one behind two key items
// that sit most of the way through the early game - the Blood Gem Workshop
// Tool (a chest in Central Yharnam) and the Rune Workshop Tool (a chest in
// Hemwick Charnel Lane). Until both are found the drops are dead weight.
//
// This has NO reference-tool counterpart. The Windows randomizer's workshop
// setting (docs/plans/workshop-tools.md) is a different feature entirely: it
// puts those two items into the treasure SHUFFLE. This one grants them
// outright and leaves the world alone. The two are independent and may both
// be on - see §7 of CLAUDE.md on matching the reference before extending it;
// this is an addition, not a change to anything the reference does.
//
// What it edits: CharaInitParam.param, the per-origin character template that
// says what a new character starts with. Like every other param feature here
// the archive is never rebuilt - each write overwrites one s32 or one u8
// inside an existing row. See Param/ParamBnd.h.
//
// UNVERIFIED, and the whole feature rides on it: we do not know whether
// Bloodborne unlocks gem fitting because the player HOLDS the item or because
// an event flag was set when the vanilla chest was opened. Nothing in the
// param data settles it. The evidence leans towards possession - goods 4103
// and 4104 carry refId -1 (no special effect), and their item lots carry only
// the generic "5"+lotId collected-marker that every ordinary pickup has, so
// there is no bespoke unlock flag attached to either. But that is inference,
// not proof, and only a hardware test can decide it. If the unlock turns out
// to be flag-based this feature grants two inert key items and the honest fix
// is to delete it, not to grow it.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../Param/ParamBnd.h"

namespace bbr {

struct HunterToolsResult {
    int rowsChanged = 0;   // origin rows that received at least one tool
    int slotsWritten = 0;  // item slots filled across all of those rows
    int rowsMissing = 0;   // targets absent from the real param - see kOriginRows
    int rowsFull = 0;      // targets with no free item slot (never seen on vanilla)
};

// Writes both tools into every origin row's starting inventory, in place
// inside `plain` (the decompressed archive). Deterministic - it draws no
// randomness, so the same seed produces the same world with it on or off.
//
// Returns false only on a structural problem, with `error` set; partial edits
// are possible in that case, so the caller should treat a failure as fatal to
// the run rather than continuing.
bool GrantHunterTools(std::vector<uint8_t>& plain,
                      const ParamMember& charaInitParam,
                      HunterToolsResult& result,
                      std::string* error);

} // namespace bbr
