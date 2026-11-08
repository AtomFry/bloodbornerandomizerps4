// Must precede everything, for the same reason BossRandomizer.cpp says so:
// BossInsertion.h pulls in <random>, which drags in the toolchain's libc++
// <cmath>, which fails unless ::abs is already declared.
#include <cstdlib>

#include "BossInsertion.h"

#include "BossList.h"
#include "CagedDogList.h"
#include "EnemyExclusionList.h"
#include "EnemySkipList.h"
#include "InsertAreaTable.h"
#include "InsertedBossTable.h"

#include "../Platform/Log.h"

namespace bbr {

namespace {

bool Contains(const std::string& s, const char* needle) {
    return s.find(needle) != std::string::npos;
}

bool IsRejectedNpc(int32_t npc) {
    return npc == 250060 || npc == 250070 || npc == 250090 ||
           npc == 212600 || npc == 212610 || npc == 212620;
}

// The SIX tests of RandomizeFunctions.cs:1587-1638 that precede the harvest
// line at :1657, and no more than six. PoolEligible() in BossRandomizer.cpp
// applies three further ones - !m26 && c0000_0005, m34 && npc == 210030, and
// m28 && c2100 - and those run AFTER the add, setting an addEnemy flag that
// gates a different list further down. Reproducing them here would drop three
// Witch of Hemwick identities the reference genuinely harvests (plan-evidence
// 013 §E1.3, §E4 M2), so this is a separate function and not a flag on that one.
//
// The reference's seventh test, tempList.Contains(enemy), is vacuous:
// MSBB.Part.Enemy has no value equality and tempList only ever holds objects
// from the same map, each added once. It is omitted rather than reproduced.
bool InsertionEligible(const std::string& name, int32_t npc, int32_t think,
                       int32_t entityID) {
    if (IsRejectedNpc(npc))                          return false;
    if (entityID == -1)                              return false;
    if (npc == 0 || npc == -1)                       return false;
    if (think == -1)                                 return false;
    if (Contains(name, "c5070"))                     return false;
    if (Contains(name, "c3060") && npc != 210306016) return false;
    return true;
}

// Moon Presence. The reference strips it with a loop that removes ONE entry and
// then leaves; the list is Distinct()-ed before the pass and holds exactly one
// c5400 entry, so a plain filter is equivalent - measured, not assumed.
const char* const kExcludedModel = "c5400";

// :1841, written in the C# as ThinkParamID.ToString() != "1" - a string
// comparison, which is why a numeric search of the reference finds nothing.
const int32_t kExcludedThink = 1;

int32_t FindEnemyModelIndex(const MsbbFile& msbb, const std::string& modelName) {
    for (size_t i = 0; i < msbb.models.entries.size(); i++) {
        if (model_fields::GetType(msbb.models.entries[i]) != ModelType::kEnemy) continue;
        if (model_fields::GetName(msbb.models.entries[i]) == modelName) return (int32_t)i;
    }
    return -1;
}

std::string ModelNameAt(const MsbbFile& msbb, int32_t index) {
    if (index < 0 || (size_t)index >= msbb.models.entries.size()) return std::string();
    return model_fields::GetName(msbb.models.entries[(size_t)index]);
}

int RandInt(std::mt19937& rng, int loInclusive, int hiInclusive) {
    std::uniform_int_distribution<int> d(loInclusive, hiInclusive);
    return d(rng);
}

// Is this map file part of a ticked area? Returns false for a map that belongs
// to no area at all, which is both m21 files - so the Hunter's Dream and the
// Abandoned Old Workshop are never visited whatever is ticked.
//
// The fourteen rows are map-name PREFIXES, each covering one to three files, so
// the unused variants of a doubled area are visited exactly as the reference
// visits them. The prefixes are mutually exclusive, so the first match is the
// only match.
bool AreaTicked(const std::string& mapName, const InsertAreaSelection& areas) {
    const std::array<ModelPoolEntry, kInsertAreaCount>& table = InsertAreaTable();
    for (int i = 0; i < kInsertAreaCount; i++) {
        const std::string prefix = table[(size_t)i].model;
        if (mapName.compare(0, prefix.size(), prefix) == 0) return areas.enabled[i];
    }
    return false;
}

struct Target {
    size_t partIndex = 0;
    part_fields::Position pos;
};

// Squared distance in double, never a square root and never a division: two
// placements can share one coordinate exactly (1,367 distinct positions among
// the 1,381 live-area targets), so a zero distance is reachable and must be
// tolerated rather than guarded against.
double Dist2(const part_fields::Position& a, const part_fields::Position& b) {
    const double dx = (double)a.x - (double)b.x;
    const double dy = (double)a.y - (double)b.y;
    const double dz = (double)a.z - (double)b.z;
    return dx * dx + dy * dy + dz * dz;
}

// Draw one identity, erase it, and refill from the FULL enabled list when the
// supply empties. Across the whole run, not per map: that is what makes every
// enabled identity get used once before any is used twice (spec 013 D-G).
BossIdentity DrawIdentity(InsertionPool& pool, std::mt19937& rng) {
    const size_t index = (size_t)RandInt(rng, 0, (int)pool.entries.size() - 1);
    BossIdentity chosen = pool.entries[index];
    pool.entries.erase(pool.entries.begin() + (long)index);
    if (pool.entries.empty()) pool.entries = pool.refill;
    return chosen;
}

} // namespace

void CollectInsertionCandidates(const std::string& mapName, const MsbbFile& msbb,
                                const InsertionOptions& options, InsertionPool& pool) {
    (void)mapName; // the harvest's rules are per-placement; see InsertionEligible
    if (!options.enabled) return;

    for (const std::vector<uint8_t>& blob : msbb.parts.entries) {
        if (part_fields::GetType(blob) != PartsType::kEnemy) continue;

        const std::string name = part_fields::GetName(blob);
        if (!IsBossName(name)) continue;

        const int32_t npc = part_fields::GetEnemyNPCParamID(blob);
        const int32_t think = part_fields::GetEnemyThinkParamID(blob);
        if (!InsertionEligible(name, npc, think, part_fields::GetEntityID(blob))) continue;
        if (think == kExcludedThink) continue;

        const std::string model = ModelNameAt(msbb, part_fields::GetModelIndex(blob));
        if (model.empty()) continue;
        if (model == kExcludedModel) continue;

        // The picker. Filtering at harvest time rather than at draw time is what
        // keeps the drain, the refill and the "used once before any twice"
        // property working against whatever the player ticked, with no second
        // rule anywhere downstream.
        if (!options.includedBosses.IsModelEnabled(model, InsertedBossTable().data())) continue;

        const BossIdentity identity{ npc, think, model };
        bool already = false;
        for (const BossIdentity& e : pool.entries) {
            if (e == identity) { already = true; break; }
        }
        if (!already) pool.entries.push_back(identity);
    }
}

void FinalizeInsertionPool(InsertionPool& pool) {
    // The entries are already distinct by (npc, think, model) - the collector
    // dedupes as it appends, which is the reference's own Distinct() at
    // StartFunctions.cs:1015-1021. So the refill list is a straight copy, and
    // that is the whole point of it: NOT model-deduped.
    pool.refill = pool.entries;
}

int ApplyBossInsertion(const std::string& mapName, MsbbFile& msbb,
                       const InsertionOptions& options, InsertionPool& pool,
                       std::mt19937& rng, InsertionCounts& counts) {
    // EVERY GATE BEFORE THE FIRST DRAW. See the header: one stray RandInt on
    // this path reshuffles every map after it on every seed.
    if (!options.enabled) return 0;
    if (pool.entries.empty()) return 0;          // nothing ticked: a SILENT skip
    if (!AreaTicked(mapName, options.areas)) return 0;
    if (options.countPerMap <= 0 && !options.replaceAll) return 0;

    // Step 2: the map's eligible targets, in parts order, with their positions.
    std::vector<Target> eligible;
    for (size_t i = 0; i < msbb.parts.entries.size(); i++) {
        const std::vector<uint8_t>& blob = msbb.parts.entries[i];
        if (part_fields::GetType(blob) != PartsType::kEnemy) continue;

        const std::string name = part_fields::GetName(blob);
        if (IsExcludedEnemyName(name)) continue;
        if (IsSkippedName(name, options.skipPatterns)) continue;
        if (options.doNotRandomizeCagedDogs &&
            IsProtectedCagedDog(mapName, part_fields::GetEntityID(blob))) continue;
        if (ModelNameAt(msbb, part_fields::GetModelIndex(blob)).empty()) continue;

        Target t;
        t.partIndex = i;
        t.pos = part_fields::GetPosition(blob);
        eligible.push_back(t);
    }
    if (eligible.empty()) return 0;

    const int supply = (int)eligible.size();

    // Step 3: HOW MANY. The count and the supply decide this, and nothing else -
    // never the composition of the pool. That is the failure spec 013 D-M
    // forecloses: one pool entry per ticked boss would make the boss rate a
    // function of how many bosses are ticked (6.2% at 22, 0.30% at one), so
    // "up to 5 Amygdalas" would mean 0.16 expected in the smallest map. Ticking
    // fewer bosses changes WHICH arrive and never HOW MANY.
    const int cap = options.countPerMap < supply ? options.countPerMap : supply;
    int k = 0;
    if (options.replaceAll) {
        k = supply;
    } else if (options.exactCount) {
        // An equality on min(N, supply), and no draws at all: EXACTLY means
        // exactly.
        k = cap;
    } else {
        // UP TO: one draw per eligible placement, in order, converting while
        // there is room. The per-placement probability is therefore cap / supply
        // exactly. All `supply` draws are taken whether or not the ceiling has
        // been reached, so the number of draws depends on the map and not on how
        // early the ceiling filled.
        for (int i = 0; i < supply; i++) {
            const int draw = RandInt(rng, 0, supply - 1);
            if (k < cap && draw < cap) k++;
        }
    }
    if (k <= 0) return 0;

    // Step 4: WHICH k, in the order they will be written.
    std::vector<size_t> chosen;   // indices into `eligible`
    chosen.reserve((size_t)k);
    if (options.replaceAll) {
        for (size_t i = 0; i < eligible.size(); i++) chosen.push_back(i);
    } else if (!options.spaceOut) {
        // k draws without replacement: swap-and-shrink over a working index
        // list, which is the same shape as the enemy pass's shuffle.
        std::vector<size_t> remaining;
        remaining.reserve(eligible.size());
        for (size_t i = 0; i < eligible.size(); i++) remaining.push_back(i);
        for (int n = 0; n < k; n++) {
            const size_t pick = (size_t)RandInt(rng, 0, (int)remaining.size() - 1);
            chosen.push_back(remaining[pick]);
            remaining[pick] = remaining.back();
            remaining.pop_back();
        }
    } else {
        // Greedy farthest-point. One draw for the first pick - the same single
        // draw the branch above takes for it, which is why a count of 1 gives
        // the same placement and the same RNG consumption in both states - then
        // k - 1 deterministic picks, each maximising the minimum squared
        // distance to everything already chosen. Lowest index wins a tie, so
        // two placements at identical coordinates resolve stably.
        std::vector<bool> taken(eligible.size(), false);
        const size_t first = (size_t)RandInt(rng, 0, supply - 1);
        chosen.push_back(first);
        taken[first] = true;
        for (int n = 1; n < k; n++) {
            size_t best = eligible.size();
            double bestDist = -1.0;
            for (size_t i = 0; i < eligible.size(); i++) {
                if (taken[i]) continue;
                double nearest = -1.0;
                for (size_t c : chosen) {
                    const double d = Dist2(eligible[i].pos, eligible[c].pos);
                    if (nearest < 0.0 || d < nearest) nearest = d;
                }
                if (nearest > bestDist) {   // strict: lowest index keeps a tie
                    bestDist = nearest;
                    best = i;
                }
            }
            if (best >= eligible.size()) break;  // nothing left, k > supply
            chosen.push_back(best);
            taken[best] = true;
        }
    }

    // Step 5: write them, in the order step 4 produced.
    int written = 0;
    for (size_t index : chosen) {
        std::vector<uint8_t>& blob = msbb.parts.entries[eligible[index].partIndex];
        const std::string name = part_fields::GetName(blob);
        const int32_t oldNpc = part_fields::GetEnemyNPCParamID(blob);
        const int32_t oldThink = part_fields::GetEnemyThinkParamID(blob);
        const std::string oldModel = ModelNameAt(msbb, part_fields::GetModelIndex(blob));

        const BossIdentity identity = DrawIdentity(pool, rng);

        const int32_t modelIndex = FindEnemyModelIndex(msbb, identity.model);
        if (modelIndex < 0) {
            // StepMergeModels merges the union of all 24 base maps' enemy models
            // into every map unconditionally, and all 22 insertion models are in
            // that union, so this is not expected. If it happens the placement
            // is left exactly as it was - a valid creature - and the run
            // continues.
            Log((mapName + " " + name + ": boss insertion skipped - model " +
                 identity.model + " not in this map").c_str());
            continue;
        }

        part_fields::SetEnemyNPCParamID(blob, identity.npc);
        // REPLACED ENEMY'S AI is NOT WRITING the field. See the header: that is
        // the only way the reference expresses it, and it is why the placement
        // keeps whatever think id the enemy pass left it with rather than a
        // value this pass chose.
        if (!options.keepEnemyAi) {
            part_fields::SetEnemyThinkParamID(blob, identity.think);
        }
        part_fields::SetModelIndex(blob, modelIndex);
        written++;

        // The same shape as the enemy loop's per-placement line, so the two can
        // be read together and diffed against the reference's own log.
        Log((mapName + " " + name + ": inserted boss " + std::to_string(oldNpc) + "*" +
             std::to_string(oldThink) + "*" + oldModel + " -> " +
             std::to_string(identity.npc) + "*" +
             std::to_string(options.keepEnemyAi ? oldThink : identity.think) + "*" +
             identity.model)
                .c_str());
    }

    // B20: a map that cannot supply the count asked for inserts as many as it
    // can and says so. Never a failure - the smallest of the fourteen areas
    // holds 54 targets against a maximum count of 15, so this is unreachable
    // through the UI and reachable only through a hand-edited config or a
    // narrow ENEMIES SKIPPED selection.
    if (!options.replaceAll && options.countPerMap > supply) {
        counts.shortfallMaps++;
        Log((mapName + ": boss insertion asked for " +
             std::to_string(options.countPerMap) + " and the map offers " +
             std::to_string(supply)).c_str());
    }

    counts.inserted += written;
    return written;
}

} // namespace bbr
