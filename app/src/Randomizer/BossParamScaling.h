// BossParamScaling.h - C++ port of the reference tool's ParamScalingForBosses
// (MainWindow.xaml.cs). Runs after enemy randomization: for any enemy whose
// (post-randomization) NPCParamID happens to be one of ~1170 "tracked" base
// identities (NpcScalingTable.h), replaces it with a pre-tuned variant of
// that SAME creature appropriate for the zone the map belongs to. Gated in
// the reference tool by "No Scaling" (unchecked by default, i.e. this runs
// unless the user explicitly disables it) - matched here by simply always
// running it, since our UI has no equivalent toggle and the reference
// default is "on".
//
// Only NPCParamID changes here - never ThinkParamID or ModelName. That's
// deliberate (matches the reference): these tracked identities are variant
// stat blocks of the same creature/model/behavior, not a different enemy
// entirely, so only the stats need to shift.
#pragma once

#include "../Msb/Msbb.h"

#include <array>
#include <string>

namespace bbr {

// The reference tool calls ParamScalingForBosses on exactly one representative
// map file per zone (not all 24), each with that zone's own scale constant.
// m21_01_00_00 is deliberately included with no scale applied (0) - the
// reference source's own switch has an empty case for it, i.e. the function
// runs but changes nothing there.
struct BossScalingMapEntry {
    const char* name;
    int zoneScale; // 0 means "no scaling for this map" (mirrors an empty case)
};

inline const std::array<BossScalingMapEntry, 16>& BossScalingMaps() {
    static const std::array<BossScalingMapEntry, 16> kMaps = {{
        {"m21_00_00_00", 20}, // dreamScale - Hunter's Dream
        {"m21_01_00_00", 0},  // no case in the reference source - no-op
        {"m22_00_00_00", 7},  // hemwickScale
        {"m23_00_00_01", 4},  // oldScale
        {"m24_00_00_01", 21}, // cathedralScale
        {"m24_01_00_01", 1},  // centralScale
        {"m24_02_00_01", 14}, // upperScale
        {"m25_00_00_00", 12}, // cainhurstScale
        {"m26_00_00_00", 17}, // mensisScale
        {"m27_00_00_01", 9},  // woodsScale
        {"m28_00_00_01", 13}, // yahargulScale
        {"m32_00_00_01", 11}, // byrgenwerthScale
        {"m33_00_00_00", 10}, // frontierScale
        {"m34_00_00_00", 24}, // nightmareScale
        {"m35_00_00_00", 27}, // researchScale
        {"m36_00_00_00", 29}, // hamletScale
    }};
    return kMaps;
}

// Applies boss/tracked-enemy param scaling to one already-randomized map.
// Returns the number of NPCParamID values changed (for logging).
int ApplyBossParamScaling(MsbbFile& msbb, int zoneScale);

} // namespace bbr
