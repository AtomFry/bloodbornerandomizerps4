// CagedDogList.h - DO NOT RANDOMIZE CAGED DOGS, the ten caged Shaggy Hunting
// Dog placements the enemy pass leaves alone when the setting is on
// (feature 033). Six in the Central Yharnam kennel yard, four in the
// Forbidden Woods cage cluster.
//
// WHY THIS IS A SEPARATE HEADER, and not a few more rows in EnemySkipList.h
// or EnemyExclusionList.h. Those two match a placement NAME as a substring,
// game-wide, and are keyed on the creature. This one is keyed on the SPOT:
// it matches an entity ID inside a named area, and it is deliberately
// unreachable by name. All eight of the placement names below are reused by
// unrelated dogs elsewhere - matching them as substrings catches 58
// placements across nine map files, 32 of which must keep randomizing (spec
// 033 §4 F7, plan §3.3). The name column here is documentation only.
//
// WHY ENTITY IDS. No field of the dog placement records isolates the set:
// the stat row is exact for the Forbidden Woods four and misses two of the
// Central Yharnam six, the kennel-yard collision surface carries a seventh
// dog that spec D5 leaves unprotected, and the Forbidden Woods cages share a
// surface with three other creatures. A hand-written list of ten identifiers
// is the only scheme that catches exactly the intended placements, and
// app/tools/caged_dogs_verify.py exists because a list nobody rechecks is
// the failure mode that buys (spec D3): it recomputes the set from the
// vanilla tree three independent ways and parses the entries below rather
// than restating them.
//
// WHAT TICKING THE SETTING MEANS, and what it deliberately does NOT mean:
//   * these placements are never re-targeted - the test sits in
//     StepWriteMap, after the ENEMIES SKIPPED test and before the zone roll,
//     inside the same `!forced &&` so the m28 Yahar'gul override still wins;
//   * they still CONTRIBUTE to the replacement pool. StepReadMap's loop is
//     untouched, so the pool stays at 333 entries across 82 models and the
//     caged dogs' own stat variants 124401 and 124501 - which no other
//     placement in the game supplies - stay in circulation. That is the
//     deliberate difference from ENEMIES SKIPPED, which couples the two
//     halves on purpose (spec 033 D6);
//   * their stat rows are still re-tuned for the zone by BossParamScaling,
//     exactly as a skipped creature's are today. "Unchanged" here means "not
//     re-targeted", never "byte-identical to vanilla" (spec 033 D8).
// Like the skip test, this one is applied BEFORE the zone roll is drawn, so
// a protected placement consumes no randomness and the seed's meaning
// shifts. That is intended: the same seed with this setting on is a
// different world everywhere.
#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace bbr {

// One protected spot, as (the area's map-name prefix, the placement's entity
// ID). The prefix is what scopes the entity ID to its area: each entry
// matches one placement in every map file whose name contains the prefix -
// Central Yharnam ships as three files and the Forbidden Woods as two, all
// five holding the same placements, and which of them the game loads depends
// on the player's install. Ten entries, 26 placements.
struct CagedDogEntry {
    const char* mapPrefix;
    int32_t     entityID;
};

inline const std::array<CagedDogEntry, 10>& CagedDogList() {
    static const std::array<CagedDogEntry, 10> kList = {{
        // Central Yharnam kennel yard - m24_01_00_00, _01 and _11.
        { "m24_01", 2410271 }, // c1240_0004, one of the two that come out
        { "m24_01", 2410272 }, // c1240_0005, the other
        { "m24_01", 2410275 }, // c1240_0008, penned
        { "m24_01", 2410277 }, // c1240_0010, penned
        { "m24_01", 2410278 }, // c1240_0011, penned
        { "m24_01", 2410279 }, // c1240_0012, penned
        // Forbidden Woods cage cluster - m27_00_00_00 and _01. The cluster
        // holds six cages; two are empty in the unmodified game.
        { "m27",    2700301 }, // c1240_0001
        { "m27",    2700302 }, // c1240_0002
        { "m27",    2700308 }, // c1240_0004
        { "m27",    2700309 }, // c1240_0005
    }};
    return kList;
}

// The only way the rest of the engine asks "is this spot protected". Both
// halves are required: an entity ID alone is not unique across the game, and
// a map prefix alone is a whole area.
//
// The `<= 0` test is not defensive tidiness - it is load-bearing. Two of the
// twelve Central Yharnam dogs carry entity ID -1, as do hundreds of
// placements across the maps, so an unguarded comparison against a table
// that ever grew a -1 entry would freeze them all. Rejecting it before
// anything is compared keeps that impossible rather than merely absent.
inline bool IsProtectedCagedDog(const std::string& mapName, int32_t entityID) {
    if (entityID <= 0) return false;
    for (const CagedDogEntry& entry : CagedDogList()) {
        if (entry.entityID == entityID &&
            mapName.find(entry.mapPrefix) != std::string::npos) {
            return true;
        }
    }
    return false;
}

} // namespace bbr
