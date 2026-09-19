// EnemySkipList.h - ENEMIES SKIPPED, the user's own "leave this creature
// alone" list (feature 032). The counterpart to EnemyExclusionList.h, and
// deliberately not part of it.
//
// WHY THIS IS A SEPARATE HEADER. EnemyExclusionList.h is what the REFERENCE
// tool always excludes: 103 patterns transcribed verbatim, frozen, the same
// for every run. This is what the PLAYER chose this run. They are applied at
// the same two places and look alike at the call site, but they differ in
// every way that matters - provenance, lifetime, and whether they are allowed
// to yield. The fixed list never yields; this one does, once, when honouring
// it would leave the pool empty (spec 032 D4, see StepBuildPool). Feature 016
// kept its three patterns separate for the same reason.
//
// WHAT TICKING A ROW MEANS, in both directions:
//   * the creature's own placements are never overwritten - the test sits
//     beside the exclusion test in StepWriteMap, inside the same `!forced &&`
//     so the m28 Yahar'gul override still wins;
//   * it never arrives as a replacement - the test also sits in StepReadMap's
//     contribution loop, so it never enters the pool.
// Both tests are applied BEFORE the zone roll is drawn, so a skipped
// placement consumes no randomness and the seed's meaning shifts. That is
// intended: the same seed with a different skip list is a different world.
#pragma once

#include <string>
#include <vector>

#include "EnemySkipTable.h"
#include "ModelPoolSelection.h"

namespace bbr {

// The ticked rows' model ids, built once per run rather than re-walking an
// 85-row table per placement. An empty result means "skip nothing", which is
// the default and the overwhelmingly common case - IsSkippedName then costs
// one empty-vector test per placement.
inline std::vector<std::string> BuildSkipPatterns(
    const ModelPoolSelection<kEnemySkipModelCount, false>& selection) {
    std::vector<std::string> patterns;
    for (int i = 0; i < kEnemySkipModelCount; i++) {
        if (selection.enabled[i]) patterns.push_back(EnemySkipTable()[(size_t)i].model);
    }
    return patterns;
}

// Substring match on the placement name, the same shape as
// IsExcludedEnemyName. Substring rather than equality because a placement is
// named "<model>_<nnnn>" - "c1050" has to match "c1050_0117".
//
// Matching on the NAME rather than on the resolved model name is not an
// oversight: across all 2,877 placements in the 24 base maps, name[:5] equals
// the model without exception and no model id occurs inside another
// creature's placement name, so the two agree - and the name is what the
// exclusion test beside this one already uses.
inline bool IsSkippedName(const std::string& name,
                          const std::vector<std::string>& patterns) {
    for (const std::string& pattern : patterns) {
        if (name.find(pattern) != std::string::npos) return true;
    }
    return false;
}

} // namespace bbr
