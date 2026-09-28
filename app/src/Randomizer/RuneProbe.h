// RuneProbe.h - A TEMPORARY HARDWARE PROBE. Delete this file once it has
// answered its question; it is not a feature and must never ship enabled.
//
// PROBE 2. The first probe wrote a 魔石 "magic stone" id - 200050 Milkweed and
// friends, the ids a third-party mod awards through ItemLotParam with
// lotItemCategory 8 - into three CharaInitParam fields. The bytes reached the
// shipped tree on all 22 origin rows and the game ignored every one of them.
//
// WHY THAT FAILED, which is the useful half of the result: each slot resolves
// its id against a particular param. item_01..10 looks its id up in
// EquipParamGoods, which stops at 12140; equip_Accessory01..05 looks its id up
// in EquipParamAccessory, whose 41 rows are 100..150. A category-8 id is in
// neither table, so the game looked for a row that does not exist and granted
// nothing. The probe was well formed and pointed at the wrong namespace.
//
// THE QUESTION NOW. EquipParamAccessory's 41 rows have sortId 1..41 with no
// gaps - a complete, ordered set of the right size to be Bloodborne's runes -
// and CharaInitParam row 9001 already sets equip_Accessory01 = 100, so the
// field is used by the shipped game with a real id from that table. Nothing
// establishes that those 41 rows ARE runes, because the accessory name FMG is
// an empty 128-byte stub and no link from them to the 31 rune names in
// 魔石名.fmg could be found. Only the console can say.
//
// THE DESIGN. Five different accessory ids into the five accessory slots, so
// one boot tests five rows at once and names which ones arrived rather than
// only proving a negative. They are spread across the sort order, and include
// the one id the shipped game itself uses:
//
//   slot 1  <- 111  (sortId 1,  first in the game's own display order)
//   slot 2  <- 100  (sortId 3,  the id row 9001 sets - the precedent)
//   slot 3  <- 113  (sortId 11)
//   slot 4  <- 149  (sortId 21)
//   slot 5  <- 150  (sortId 41, last in display order)
//
// Whatever appears in the Memory Altar both answers the question and starts
// the id-to-name map that could not be built from the data.
//
// The first probe's other two candidates are gone. item_01..10 and
// secondaryItem_01..06 were disproven for a category-8 id, and leaving them in
// would only make this observation ambiguous.
//
// WHY IT RIDES ON START WITH HUNTER TOOLS. Reaching the Memory Altar to
// observe the result needs the Rune Workshop Tool, and that setting already
// grants it. No new UI, no new config key, and the probe cannot fire in a run
// that could not see its own result.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../Param/ParamBnd.h"

namespace bbr {

struct RuneProbeResult {
    int rowsChanged = 0;
    int accessoryWrites = 0;  // accessory slots filled across all origin rows
    int rowsMissing = 0;
};

// Writes the five probe accessories into every origin row's five accessory
// slots, in place. Deterministic - it draws no randomness.
bool RunRuneProbe(std::vector<uint8_t>& plain,
                  const ParamMember& charaInitParam,
                  RuneProbeResult& result,
                  std::string* error);

} // namespace bbr
