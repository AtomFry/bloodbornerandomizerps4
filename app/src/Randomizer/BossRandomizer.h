// BossRandomizer.h - the C++ port of the reference WPF tool's "Randomize
// Bosses" feature (GenerateBossList / RandomizeBosses / AddOrphanPhaseOne /
// AddTheRest). Written against docs/plans/boss-randomization.md, which is the
// contract this and tools/boss_verify.py are both implemented from - read that
// first; it records the rules, the deliberate deviations (D1-D7), and the V0
// measurements taken against real vanilla data.
//
// Scope, matching the rest of this port: base maps only (no chalice dungeons),
// no oopsAllBosses mode, lesserBosses fixed false. Boss randomization touches
// exactly the fields enemy randomization already does - NPCParamID,
// ThinkParamID and the model reference on Part.Enemy blobs - so it needs no new
// file-format capability, only Msbb's EntityID accessor.
//
// Everything here mutates already-parsed MsbbFile objects in memory. No file
// I/O: EnemyRandomizer owns reading and writing, and its one-read-one-write-per-
// map convention is preserved.
#pragma once

#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include "../Msb/Msbb.h"
#include "EnemyPoolSelection.h"

namespace bbr {

struct BossIdentity {
    int32_t npc = 0;
    int32_t think = 0;
    std::string model;

    bool operator==(const BossIdentity& o) const {
        return npc == o.npc && think == o.think && model == o.model;
    }
};

struct BossPool {
    // Drained as bosses are assigned (spec §4.4 step 5); refilled from
    // `refill` when it empties.
    std::vector<BossIdentity> entries;
    // The same pool additionally deduped so no two entries share a model
    // (StartFunctions.cs:868-879). Also the pool AddTheRest draws from.
    std::vector<BossIdentity> refill;

    // The Orphan of Kos identity harvested from c4540_0000 (spec §5).
    BossIdentity orphan;
    bool orphanFound = false;
};

// The 17 base maps boss randomization runs over, in the reference tool's own
// call order (StartFunctions.cs:952-976). Deliberately not the enemy pass's
// 24-map order: the two most constrained maps go first, while the pool is full.
const std::vector<std::string>& BossMapOrder();

// Spec §4.2. Call once per map, in BossMapOrder order.
// `included` is the boss picker: an identity whose model is unticked never
// enters the pool, so it is never used as a replacement. It does NOT protect
// that boss's own arena, which is still reassigned like any other.
void CollectBossCandidates(const std::string& mapName, const MsbbFile& msbb,
                           const BossPoolSelection& included, BossPool& pool);

// Dedupes and builds the refill list. Call once, after every map is collected.
void FinalizeBossPool(BossPool& pool);

// Spec §5. Picks one of three maps; returns its name, or "" if the Orphan
// identity was never found. Consumes one RNG draw, as the reference does.
std::string ChooseOrphanPhaseOneMap(std::mt19937& rng);

// Spec §5. No-op unless mapName is the chosen map and holds the expected
// placement. Returns true if a placement was changed.
bool ApplyOrphanPhaseOne(const std::string& mapName, MsbbFile& msbb, const BossPool& pool);

// Spec §4.3-4.5. Assigns bosses in one map and applies the multi-entity fixups
// as a post-pass (deviation D7). Returns the number of placements changed.
int AssignBossesInMap(const std::string& mapName, MsbbFile& msbb, BossPool& pool,
                      std::mt19937& rng);

// Spec §4.6. Handles the placements §4.3 deliberately skips, in m22 / m27 / m35.
// Returns the number of placements changed.
int AddTheRestInMap(const std::string& mapName, MsbbFile& msbb, BossPool& pool,
                    std::mt19937& rng);

} // namespace bbr
