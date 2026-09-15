// BossList.h - the reference WPF tool's bossList (MainWindow.xaml.cs:161-198,
// 38 entries, read verbatim from the C# source). A Part.Enemy is a boss
// placement if its Name contains any of these as a substring - the same plain
// `Name.Contains(...)` test the reference uses. See
// docs/plans/boss-randomization.md §4.1.
//
// These same 38 strings are ALSO the tail of EnemyExclusionList.h, and that is
// not duplication to factor away: they serve opposite purposes that happen to
// share a source list. As exclusions they keep boss placements out of *enemy*
// randomization; here they identify what *boss* randomization targets. The
// reference has the same double use (unusedPlusBossList is the enemy nono-list,
// while bossList alone drives GenerateBossList/RandomizeBosses), so the two are
// kept separate here for the same reason - either can change without the other.
//
// Note: this port briefly carried an extra 45-entry protective list of its own
// (EnemyExclusionListExtra), which was never part of boss identification and
// was removed on 2026-09-13 - see docs/enemy-exclusion-history.md.
#pragma once

#include <array>
#include <string>

namespace bbr {

inline const std::array<const char*, 38>& BossNameList() {
    static const std::array<const char*, 38> kList = {
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

inline bool IsBossName(const std::string& name) {
    for (const char* pattern : BossNameList()) {
        if (name.find(pattern) != std::string::npos) return true;
    }
    return false;
}

} // namespace bbr
