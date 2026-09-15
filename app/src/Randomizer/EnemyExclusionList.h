// EnemyExclusionList.h - the reference WPF tool's "unused + boss" enemy
// name-substring exclusion list (MainWindow.xaml.cs's unusedList/bossList,
// read verbatim from the reference C# source, MainWindow.xaml.cs lines
// 94-198: unusedList is 66 entries at lines 94-159, bossList is 38 entries
// at lines 161-198). An enemy Part is excluded from both the randomization
// pool and from being retargeted if its Name contains any of these as a
// substring.
//
// Matches unusedPlusBossList AFTER the reference tool's
// `unusedPlusBossList.RemoveAt(3)` (StartFunctions.cs:580): index 3 of the
// concatenated unusedList+bossList is unusedList[3] = "c2560", already
// removed below (commented out in place, so the original 66-entry order is
// still visible) rather than reproduced then stripped at runtime. The
// reference source has no comment explaining this removal - it's applied
// here purely to match the original tool's effective behavior. 66 - 1 + 38
// = 103 entries below.
#pragma once

#include <array>
#include <string>

namespace bbr {

inline const std::array<const char*, 103>& EnemyExclusionList() {
    static const std::array<const char*, 103> kList = {
        // unusedList (66 in the reference source, "c2560" removed here)
        "c2120_9999", "c2120_9998", "c0000",
        /* "c2560" removed - see file header comment */
        "c0", "c1020", "c1030", "c1080", "c2030", "c2300", "c2310", "c2800",
        "c2810", "c2910", "c3110", "c4150", "c5030", "c5110", "c5140", "c5150",
        "c5400", "c5420", "c5500", "c5501", "c5502", "c5520", "c5521", "c5522",
        "c7000", "c7010", "c7100", "c8010", "c8020", "c2501", "c90", "c9030",
        "c2501", "c2571", "c8030_0000", "c8040_0000", "c9020_0000",
        "c9020_0001", "c9020_0002", "c9020_0003", "c8030_0000", "c5072_0000",
        "c4023_0000", "c4160_0000", "c4160_0001", "c4160_0002", "c4160_0003",
        "c4550_0000", "c7110_0000", "c5071_0000", "c4511_0000", "c5510_0001",
        "c5510_0002", "c4540_0000", "c4543_0000", "c8070_0000", "c5130_0000",
        "c4031_0000", "c4520_0000", "c1190", "c1180", "c1060_0002",
        // bossList (38 entries)
        "5090", "c2090_0003", "c2100_0000", "c2100_0001", "c2120_0000",
        "c2120_0001", "c2120_0002", "c2320_0000", "c2500_0000", "c2570_0001",
        "c2510_0000", "c2710_0000", "c2720_0000", "c4520_0002", "c4030_0000",
        "c4030_0001", "c4030_0002", "c4030_0003", "c4030_0004", "c4500_0000",
        "c4510_0000", "c4540_0000", "c4541_0000", "c5000_0000", "c5020_0000",
        "c5070_0000", "c5080_0000", "c5100_0000", "c5120_0001", "c5400_0000",
        "c5510_0000", "c8050_0000", "c3130_0000", "c5010_0000", "c4510_0002",
        "c3050_0000", "c3060_0000", "c5110",
    };
    return kList;
}

// There used to be a second, larger list here - 45 extra name patterns this
// port added on top of the reference tool's own. It was REMOVED on 2026-09-13
// and this port now matches the reference tool exactly.
//
// Short version of why: it was added on 2026-09-09 from a single observation
// (c1130_0000 being overwritten), generalised by a rarity heuristic, and the
// symptom that prompted it was later traced to a completely different cause -
// a contaminated VanillaSource, not missing exclusions. Because the patterns
// match as substrings with no map scoping, those 45 entries froze 119
// placements, 18 of them protecting common trash mobs (worst case: Huntsman
// (Transformed), 283 placements game-wide). That is what made specific enemies
// near the start of the game identical in every run.
//
// The full list, what each entry actually was, and the handful worth
// reconsidering if hardware testing turns up a real problem, are preserved in
// docs/enemy-exclusion-history.md. Re-add individual entries there with a
// stated reason rather than restoring the list wholesale.

// Enemy Part.Name substring match, mirroring the reference tool's
// `Name.Contains(nonoList[j])` check exactly (plain substring test, not
// exact/prefix equality).
inline bool IsExcludedEnemyName(const std::string& name) {
    for (const char* pattern : EnemyExclusionList()) {
        if (name.find(pattern) != std::string::npos) return true;
    }
    return false;
}

} // namespace bbr
