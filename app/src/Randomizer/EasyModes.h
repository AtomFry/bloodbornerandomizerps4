// EasyModes.h - EASY SHADOWS, EASY ROM, EASY FAILURES, EASY EMISSARY. Four
// independent on/off settings, one per multi-body boss arena, each turning
// that fight into a duel by replacing the DUPLICATE bodies with the tiny
// larva that stands in Iosefka's Clinic. The survivors - one Shadow, Rom
// herself, one Living Failure, one small emissary and the Celestial Emissary
// it grows into - are not touched by this feature at all.
//
// C++ port of the reference tool's EasyModes(string currentMap)
// (reference/Randomizer/MainWindowComponents/MainWindow.xaml.cs:934-1055),
// called from StartFunctions.cs:1239-1256 as four independent top-level ifs,
// one per flag, each naming its own map files.
//
// NOT A RANDOMIZER, and that is the point of it: the pass draws no
// randomness, so the same seed produces the same world with a setting on or
// off apart from the affected placements, and it applies whether or not
// enemy, boss or treasure randomization is on. Exactly like PermaDarkness.h
// and HunterTools.h in that respect.
//
// WHERE IT RUNS, and why the position is behaviour rather than taste. The
// call sits inside StepWriteMap, AFTER the enemy loop (and after boss
// assignment, which finishes before the first map is written) and BEFORE the
// BossScalingMaps() loop. That reproduces the reference's own ordering -
// enemies, bosses, AddTheRest, EasyModes, then ParamScalingForBosses - and it
// gives two things:
//   * the easy pass is the LAST writer of its placements, so an easy setting
//     wins over boss randomization, over ENEMIES SKIPPED and over anything
//     else on the enemy side: its targets are larvae regardless of what the
//     seed did to them first;
//   * a larva planted in a scaled zone is then scaled like any other
//     creature, so the NPCParamID actually written is that zone's variant
//     (900014609 in m27_00_00_01, 900014611 in m32_00_00_01, 900014614 in
//     m24_02_00_01, 900014627 in m35_00_00_00) rather than a flat 252100.
//     That is a consequence to expect, not to correct - all 31 scaled
//     variants are identical to 252100 in HP, echoes, team type, behaviour id
//     and item lot (plan-evidence 018 E4 M9).
//
// EXACTLY THREE FIELDS are written per replaced placement, matching the
// reference: NPCParamID, ThinkParamID and the model. EntityID, the part name,
// the position and every index-based cross-reference are left alone, because
// the fight scripts track each body by entity ID - around 30 references each
// for the Shadows and the Living Failures (E4 M10).
//
// WHAT NONE OF THIS ESTABLISHES: whether the replacement is harmless in
// practice, and whether the four fights still END. The larva's stat row says
// 2 HP and team type 26 (the Doll, Gehrman and the Messengers share it), but
// a byte is not observed behaviour - see CLAUDE.md §5 and the standing note
// on setting polarity. Only a hardware test decides both.
#pragma once

#include "../Msb/Msbb.h"

#include <array>
#include <cstdint>
#include <string>

namespace bbr {

// The replacement creature, baked as named constants rather than captured at
// runtime from map data (plan 018 P3). The reference captures it while
// building the enemy pool - a placement whose model name contains c2521 has
// its three values copied into stoneGuyParam/stoneGuyThink/stoneGuyModelName
// (MainWindow.xaml.cs:744-762) - and then reuses whatever it caught. That is
// safe there only by accident: all three c2521 placements in the tree are
// identical, and on the reference's own picker path the capture never runs at
// all, so EasyModes writes zeroed params and a null model name.
//
// Provenance of the values below (plan-evidence 018 E4 M7): model c2521 has
// exactly three placements tree-wide, all named c2521_0000, all carrying
// NpcParam 252100 / ThinkParam 252100 / entity ID 2410771, in m24_01_00_00,
// _01 and _11. easy_modes_verify.py selftest case 5 re-asserts the triple
// against the real vanilla tree on every run, so this cannot quietly rot.
//
// The model name is `const char* const`, not the plan's `const char*`: a
// non-const pointer at namespace scope has EXTERNAL linkage, so the plain
// form would be a duplicate symbol the moment a second translation unit
// included this header (EnemyRandomizer.cpp does). Same spelling the rest of
// the port already uses for a named string constant - see
// WorldEditorScreen.cpp's kEnemyFailPrefix. Value and name unchanged.
const int32_t     kEasyModeNpcParamId   = 252100;
const int32_t     kEasyModeThinkParamId = 252100;
const char* const kEasyModeModelName    = "c2521";

// Which of the four settings a table row belongs to. One row can only ever
// serve one setting - the four are independent, and turning one on must
// change only its own maps.
enum class EasyModeFlag {
    kShadows,
    kRom,
    kFailures,
    kEmissary,
};

// One (setting, exact map name, placement name patterns) rule. The pattern
// array is nullptr-terminated, the shape BossRandomizer.cpp's FixupGroup
// already uses; ten is the longest list (the emissary's c2500_0001 ...
// c2500_0010), so eleven slots covers every row with its terminator.
struct EasyModeEntry {
    EasyModeFlag flag;
    const char*  map;
    const char*  patterns[11];
};

// The feature's whole data. Six rows, 79 placements tree-wide, 42 in the map
// variants the retail game actually loads.
//
// m27_00_00_00 IS ABSENT ON PURPOSE. It holds the same three c2120
// placements as m27_00_00_01, with the same values, and the reference still
// does not patch it: its Easy Shadows call site passes m27_00_00_01 alone,
// while Easy Rom and Easy Emissary each pass both variants of their area.
// The asymmetry is in the reference source as written and spec 018 §10 D3
// fixes it as agreed behaviour, so ADDING THE MAP WOULD BE A BEHAVIOUR
// CHANGE, not a tidy-up. easy_modes_verify.py selftest case 2 asserts the
// omission rather than assuming it.
//
// MATCHING IS SUBSTRING, NOT EQUALITY - name.find(pattern) != npos, mirroring
// the reference's Name.Contains. Easy Rom depends on it: the single pattern
// c1400 is what catches all thirty of Rom's children, c1400_0000 through
// c1400_0029. Keying by EXACT map name is what keeps substring matching from
// reaching where it should not: c1400 also matches 78 placements in two
// chalice maps, c2500_0001 matches in nine maps including Central Yharnam and
// the Forbidden Woods, and c2120_0002 matches in m26_00_00_00 as well
// (plan-evidence 018 E4 M5). None of those is a target of this feature.
inline const std::array<EasyModeEntry, 6>& EasyModeTable() {
    static const std::array<EasyModeEntry, 6> kTable = {{
        // Shadows of Yharnam - the two duplicate hunters. c2120_0000, the
        // strongest of the three at 1425 HP, is the survivor.
        { EasyModeFlag::kShadows,  "m27_00_00_01",
          { "c2120_0001", "c2120_0002", nullptr } },
        // Rom's children - thirty per map file, both variants patched. Rom
        // herself is a different model and is untouched.
        { EasyModeFlag::kRom,      "m32_00_00_00", { "c1400", nullptr } },
        { EasyModeFlag::kRom,      "m32_00_00_01", { "c1400", nullptr } },
        // Living Failures - three of the duplicates. c4030_0000 and
        // c4030_0004 are left alone.
        { EasyModeFlag::kFailures, "m35_00_00_00",
          { "c4030_0001", "c4030_0002", "c4030_0003", nullptr } },
        // The Celestial Emissary crowd. Ten names are listed because the
        // reference lists ten; only seven of them exist in either map file -
        // _0004, _0005 and _0008 are absent - so each row matches 7. The
        // small emissary c2500_0000, the Celestial Emissary c2570_0001 it
        // grows into, and c2500_0011/_0012/_0013 elsewhere in Upper Cathedral
        // Ward are all survivors.
        { EasyModeFlag::kEmissary, "m24_02_00_00",
          { "c2500_0001", "c2500_0002", "c2500_0003", "c2500_0004", "c2500_0005",
            "c2500_0006", "c2500_0007", "c2500_0008", "c2500_0009", "c2500_0010",
            nullptr } },
        { EasyModeFlag::kEmissary, "m24_02_00_01",
          { "c2500_0001", "c2500_0002", "c2500_0003", "c2500_0004", "c2500_0005",
            "c2500_0006", "c2500_0007", "c2500_0008", "c2500_0009", "c2500_0010",
            nullptr } },
    }};
    return kTable;
}

// Which of the four settings are on for this run. All false is "do nothing",
// and is the state in which the pass touches no map at all.
struct EasyModeOptions {
    bool shadows  = false;
    bool rom      = false;
    bool failures = false;
    bool emissary = false;

    bool Any() const { return shadows || rom || failures || emissary; }
};

// How many placements each setting replaced across the whole run. Fixed
// numbers when the map files are all present - 2 / 60 / 3 / 14, the Rom and
// emissary figures doubled because this port writes both variants of those
// two areas - so a 0 or a wrong number on the console means a pattern list or
// a map name is wrong.
struct EasyModeCounts {
    int shadows  = 0;
    int rom      = 0;
    int failures = 0;
    int emissary = 0;
};

// Applies the enabled easy settings to one already-randomized map, in place.
// Returns how many placements it replaced in THIS map; `counts` accumulates
// per setting across the run.
//
// Draws no randomness. Writes nothing when no flag is set, when the map is in
// no enabled row, or when the map's Models section does not declare c2521 -
// see the .cpp for that last case, which StepMergeModels makes impossible.
int ApplyEasyModes(const std::string& mapName, MsbbFile& msbb,
                   const EasyModeOptions& options, EasyModeCounts& counts);

} // namespace bbr
