// Must precede everything: BossRandomizer.h pulls in <random>, which drags in
// the toolchain's libc++ <cmath>, which fails unless ::abs is already declared.
// EnemyRandomizer.cpp gets away without this only because its own include list
// happens to reach <cstdlib> first.
#include <cstdlib>

#include "BossRandomizer.h"

#include "BossList.h"

#include "../Platform/Log.h"

// Deliberately no <algorithm>: pulling it in here drags the toolchain's libc++
// <cmath> into a state where it can't find ::abs. The only uses were two
// std::find calls, which are plain loops below instead.

namespace bbr {

namespace {

// Spec §4.4 D2/D3: the reference's reroll loops are unbounded and can also
// index an emptied pool. Both are hangs or crashes on a console, so every
// reroll here is capped; on exhaustion the caller leaves the placement
// untouched, which always yields a valid (vanilla) boss.
const int kRerollCap = 200;

bool Contains(const std::string& s, const char* needle) {
    return s.find(needle) != std::string::npos;
}

bool Contains(const std::string& s, const std::string& needle) {
    return !needle.empty() && s.find(needle) != std::string::npos;
}

bool ContainsAny(const std::string& s, std::initializer_list<const char*> needles) {
    for (const char* n : needles) {
        if (Contains(s, n)) return true;
    }
    return false;
}

bool IsRejectedNpc(int32_t npc) {
    return npc == 250060 || npc == 250070 || npc == 250090 ||
           npc == 212600 || npc == 212610 || npc == 212620;
}

// Spec §4.2 - GenerateBossList's eligibility, with lesserBosses fixed false.
bool PoolEligible(const std::string& mapName, const std::string& name,
                  int32_t npc, int32_t think, int32_t entityID) {
    if (IsRejectedNpc(npc))                            return false;
    if (entityID == -1)                                return false;
    if (npc == 0 || npc == -1)                         return false;
    if (think == -1)                                   return false;
    if (Contains(name, "c5070"))                       return false;
    if (Contains(name, "c3060") && npc != 210306016)   return false;
    if (Contains(mapName, "34") && npc == 210030)      return false;
    if (Contains(mapName, "m28") && Contains(name, "c2100")) return false;
    if (!Contains(mapName, "m26") && Contains(name, "c0000_0005")) return false;
    return true;
}

// Spec §4.3 - RandomizeBosses' eligibility. Deliberately a DIFFERENT list from
// PoolEligible; see deviation D5 - the two have drifted apart in the reference
// and both are reproduced as-is rather than reconciled on a guess.
bool AssignEligible(const std::string& mapName, const std::string& name,
                    int32_t npc, int32_t think, int32_t entityID) {
    if (IsRejectedNpc(npc))                            return false;
    if (entityID == -1)                                return false;
    if (npc == 0 || npc == -1)                         return false;
    if (Contains(name, "c3060"))                       return false;
    if (!Contains(mapName, "m36") && think == 454000)  return false;
    if (Contains(mapName, "m22") && Contains(name, "c2100_0001")) return false;
    if (Contains(mapName, "m27") && ContainsAny(name, {"c2120_0001", "c2120_0002"})) return false;
    if (Contains(mapName, "m35") &&
        ContainsAny(name, {"c4030_0001", "c4030_0002", "c4030_0003", "c4030_0004"})) return false;
    if (!Contains(mapName, "m26") && Contains(name, "c0000_0005")) return false;
    if (Contains(mapName, "34") && npc == 210030)      return false;
    if (Contains(mapName, "m28") && Contains(name, "c2100")) return false;
    return true;
}

// Spec §4.4 step 1 - per-map model blacklists.
bool ModelBlockedInMap(const std::string& mapName, const std::string& model) {
    if (Contains(mapName, "m24_00")) return ContainsAny(model, {"4500", "4520"});
    if (Contains(mapName, "m34_00")) return Contains(model, "3060");
    if (Contains(mapName, "m24_02")) {
        return ContainsAny(model, {"5100", "8050", "4520", "4500", "3060"});
    }
    return false;
}

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

int RandIndex(std::mt19937& rng, size_t count) {
    std::uniform_int_distribution<int> d(0, (int)count - 1);
    return d(rng);
}

// Spec §4.4 step 5. The "*4510" exception is reproduced literally from
// RandomizeFunctions.cs:2457 - given pool strings are npc*think*model, that
// guard actually matches a ThinkParamID beginning 4510, not a model. Its intent
// is unrecoverable from the source, so it is copied rather than "fixed".
void DrainPool(BossPool& pool, size_t chosenIndex, const BossIdentity& chosen) {
    pool.entries.erase(pool.entries.begin() + (long)chosenIndex);

    for (size_t i = pool.entries.size(); i-- > 0;) {
        const BossIdentity& e = pool.entries[i];
        if (e.model != chosen.model) continue;
        if (std::to_string(e.think).rfind("4510", 0) == 0) continue; // "*4510"
        pool.entries.erase(pool.entries.begin() + (long)i);
    }

    if (pool.entries.empty()) pool.entries = pool.refill; // §4.4 step 6
}

void ApplyIdentity(std::vector<uint8_t>& blob, const BossIdentity& id, int32_t modelIndex) {
    part_fields::SetEnemyNPCParamID(blob, id.npc);
    // Spec §4.4 step 4: this one NPCParamID forces its own think ID.
    part_fields::SetEnemyThinkParamID(blob, id.npc == 507200 ? 507200 : id.think);
    part_fields::SetModelIndex(blob, modelIndex);
}

struct FixupGroup {
    const char* leader;
    const char* companions[3];
};

// Spec §4.5. The reference's own comments name these: Emissary, wet nurse,
// maria, living failures.
const FixupGroup kFixups[] = {
    { "c2500_0000", { "c2570_0001", nullptr,      nullptr } },
    { "c5510_0000", { "c5510_0001", "c5510_0002", nullptr } },
    { "c4520_0002", { "c4520_0000", nullptr,      nullptr } },
    { "c4030_0004", { "c4030_0000", nullptr,      nullptr } },
};

} // namespace

const std::vector<std::string>& BossMapOrder() {
    static const std::vector<std::string> kOrder = {
        "m24_00_00_01", "m24_02_00_01", "m21_00_00_00", "m21_01_00_00",
        "m22_00_00_00", "m23_00_00_00", "m23_00_00_01", "m24_01_00_01",
        "m25_00_00_00", "m26_00_00_00", "m27_00_00_01", "m28_00_00_01",
        "m32_00_00_01", "m33_00_00_00", "m34_00_00_00", "m35_00_00_00",
        "m36_00_00_00",
    };
    return kOrder;
}

void CollectBossCandidates(const std::string& mapName, const MsbbFile& msbb,
                           const BossPoolSelection& included, BossPool& pool) {
    // Spec §4.2's second, narrower reject list, applied at insertion time.
    static const char* kInsertReject[] = {
        "c2500_0000", "c5071_0000", "c4030_0004", "c2100_0000", "c2100_0001",
        "c4540_0000", "c4030_0001", "c4030_0002", "c4030_0003", "c2120_0001",
        "c2120_0000", "c2120_0002", "c2570", "c2571", "c4030_0000",
    };

    std::vector<int32_t> npcSeenInThisMap; // dedupe is per-map in the reference

    for (const std::vector<uint8_t>& blob : msbb.parts.entries) {
        if (part_fields::GetType(blob) != PartsType::kEnemy) continue;

        std::string name = part_fields::GetName(blob);
        int32_t npc = part_fields::GetEnemyNPCParamID(blob);
        int32_t think = part_fields::GetEnemyThinkParamID(blob);
        int32_t entityID = part_fields::GetEntityID(blob);
        std::string model = ModelNameAt(msbb, part_fields::GetModelIndex(blob));

        // Tracked regardless of every filter below - the reference captures it
        // before any eligibility test (RandomizeFunctions.cs:1571-1574).
        if (Contains(name, "c4540_0000")) {
            pool.orphan = BossIdentity{ npc, think, model };
            pool.orphanFound = true;
        }

        if (!IsBossName(name)) continue;
        if (!PoolEligible(mapName, name, npc, think, entityID)) continue;

        bool rejected = false;
        for (const char* r : kInsertReject) {
            if (Contains(name, r)) { rejected = true; break; }
        }
        if (rejected) continue;

        bool npcAlreadySeen = false;
        for (int32_t seen : npcSeenInThisMap) {
            if (seen == npc) { npcAlreadySeen = true; break; }
        }
        if (npcAlreadySeen) continue;
        npcSeenInThisMap.push_back(npc);

        if (think == 1) continue; // reference drops these when serializing
        if (model.empty()) continue;

        // The boss picker. Filtered here, at the single point identities
        // enter the pool, so every downstream rule - the dedupe, the refill
        // list, DrainPool, AddTheRest - operates on the filtered set without
        // knowing this exists.
        if (!included.IsModelEnabled(model, BossPoolTable().data())) continue;

        pool.entries.push_back(BossIdentity{ npc, think, model });
    }
}

void FinalizeBossPool(BossPool& pool) {
    std::vector<BossIdentity> deduped;
    for (const BossIdentity& e : pool.entries) {
        bool already = false;
        for (const BossIdentity& d : deduped) {
            if (d == e) { already = true; break; }
        }
        if (!already) deduped.push_back(e);
    }
    pool.entries = deduped;

    pool.refill.clear();
    for (const BossIdentity& e : pool.entries) {
        bool sameModel = false;
        for (const BossIdentity& r : pool.refill) {
            if (r.model == e.model) { sameModel = true; break; }
        }
        if (!sameModel) pool.refill.push_back(e);
    }

    Log(("boss randomizer: pool built - " + std::to_string(pool.entries.size()) +
         " identities, " + std::to_string(pool.refill.size()) + " model-distinct").c_str());
}

std::string ChooseOrphanPhaseOneMap(std::mt19937& rng) {
    // One draw, matching the reference's universalRand.Next(0, 3).
    static const char* kMaps[3] = { "m24_02_00_01", "m24_01_00_01", "m34_00_00_00" };
    return kMaps[RandIndex(rng, 3)];
}

bool ApplyOrphanPhaseOne(const std::string& mapName, MsbbFile& msbb, const BossPool& pool) {
    if (!pool.orphanFound) return false;

    const char* target = nullptr;
    if (Contains(mapName, "m24_01"))      target = "c2710_0000";
    else if (Contains(mapName, "m24_02")) target = "c2500_0000";
    else if (Contains(mapName, "m34_00")) target = "c4510_0000";
    if (target == nullptr) return false;

    int32_t modelIndex = FindEnemyModelIndex(msbb, pool.orphan.model);
    if (modelIndex < 0) {
        Log("boss randomizer: orphan phase one - model missing from map, skipped");
        return false;
    }

    bool changed = false;
    for (std::vector<uint8_t>& blob : msbb.parts.entries) {
        if (part_fields::GetType(blob) != PartsType::kEnemy) continue;
        if (!Contains(part_fields::GetName(blob), target)) continue;
        ApplyIdentity(blob, pool.orphan, modelIndex);
        changed = true;
    }

    if (changed) {
        Log((mapName + ": orphan phase one applied to " + target).c_str());
    }
    return changed;
}

int AssignBossesInMap(const std::string& mapName, MsbbFile& msbb, BossPool& pool,
                      std::mt19937& rng) {
    if (pool.entries.empty() && pool.refill.empty()) return 0;

    int changed = 0;
    std::vector<std::string> assignedNames; // drives the fixup post-pass below

    for (size_t p = 0; p < msbb.parts.entries.size(); p++) {
        std::vector<uint8_t>& blob = msbb.parts.entries[p];
        if (part_fields::GetType(blob) != PartsType::kEnemy) continue;

        std::string name = part_fields::GetName(blob);
        if (!IsBossName(name)) continue;

        int32_t npc = part_fields::GetEnemyNPCParamID(blob);
        int32_t think = part_fields::GetEnemyThinkParamID(blob);
        int32_t entityID = part_fields::GetEntityID(blob);
        if (!AssignEligible(mapName, name, npc, think, entityID)) continue;

        if (pool.entries.empty()) pool.entries = pool.refill;
        if (pool.entries.empty()) break;

        // §4.4 steps 1-2, with the reroll bounded per D2.
        size_t chosenIndex = 0;
        BossIdentity chosen;
        bool ok = false;
        for (int attempt = 0; attempt < kRerollCap; attempt++) {
            chosenIndex = (size_t)RandIndex(rng, pool.entries.size());
            chosen = pool.entries[chosenIndex];
            if (ModelBlockedInMap(mapName, chosen.model)) continue;
            if (Contains(name, chosen.model)) continue; // never replace a boss with itself
            ok = true;
            break;
        }
        if (!ok) {
            // D2: leave the placement vanilla rather than looping forever.
            Log((mapName + "/" + name + ": no compatible boss after " +
                 std::to_string(kRerollCap) + " attempts, left unchanged").c_str());
            continue;
        }

        int32_t modelIndex = FindEnemyModelIndex(msbb, chosen.model);
        if (modelIndex < 0) {
            Log((mapName + "/" + name + ": model " + chosen.model +
                 " missing from map, left unchanged").c_str());
            continue;
        }

        Log((mapName + "/" + name + ": " + std::to_string(npc) + "*" + std::to_string(think) +
             " -> " + std::to_string(chosen.npc) + "*" + std::to_string(chosen.think) + "*" +
             chosen.model).c_str());

        ApplyIdentity(blob, chosen, modelIndex);
        assignedNames.push_back(name);
        changed++;

        DrainPool(pool, chosenIndex, chosen);
    }

    // §4.5 as a post-pass (deviation D7): applying fixups inline lets a
    // companion that is itself an eligible target and sits later in the parts
    // list overwrite the sync - measured to genuinely happen for the Emissary
    // pair. Running them after every assignment makes the leader's identity win.
    for (const FixupGroup& group : kFixups) {
        // Only when the leader was actually assigned this run. The reference
        // fires its fixups from inside the assignment block, so a leader that
        // assignment skipped never propagates - and propagating a skipped
        // leader's vanilla identity would silently undo a companion's own
        // legitimate randomization (m35's c4030_0004 / c4030_0000 pair does
        // exactly this: the leader is excluded there, the companion is not).
        bool leaderAssigned = false;
        for (const std::string& n : assignedNames) {
            if (n == group.leader) { leaderAssigned = true; break; }
        }
        if (!leaderAssigned) continue;

        int32_t leaderNpc = 0, leaderThink = 0, leaderModelIndex = -1;
        bool leaderFound = false;

        for (const std::vector<uint8_t>& blob : msbb.parts.entries) {
            if (part_fields::GetType(blob) != PartsType::kEnemy) continue;
            if (part_fields::GetName(blob) != group.leader) continue;
            leaderNpc = part_fields::GetEnemyNPCParamID(blob);
            leaderThink = part_fields::GetEnemyThinkParamID(blob);
            leaderModelIndex = part_fields::GetModelIndex(blob);
            leaderFound = true;
            break;
        }
        if (!leaderFound) continue;

        for (const char* companion : group.companions) {
            if (companion == nullptr) break;
            for (std::vector<uint8_t>& blob : msbb.parts.entries) {
                if (part_fields::GetType(blob) != PartsType::kEnemy) continue;
                if (part_fields::GetName(blob) != companion) continue;
                part_fields::SetEnemyNPCParamID(blob, leaderNpc);
                part_fields::SetEnemyThinkParamID(blob, leaderThink);
                part_fields::SetModelIndex(blob, leaderModelIndex);
                Log((mapName + ": fixup " + group.leader + " -> " + companion).c_str());
            }
        }
    }

    return changed;
}

int AddTheRestInMap(const std::string& mapName, MsbbFile& msbb, BossPool& pool,
                    std::mt19937& rng) {
    std::vector<const char*> targets;
    if (Contains(mapName, "m22")) {
        targets = { "c2100_0001" };
    } else if (Contains(mapName, "m27")) {
        targets = { "c2120_0001", "c2120_0002" };
    } else if (Contains(mapName, "m35")) {
        targets = { "c4030_0001", "c4030_0002", "c4030_0003" };
    } else {
        return 0;
    }

    int changed = 0;

    for (std::vector<uint8_t>& blob : msbb.parts.entries) {
        if (part_fields::GetType(blob) != PartsType::kEnemy) continue;

        std::string name = part_fields::GetName(blob);
        bool isTarget = false;
        for (const char* t : targets) {
            if (Contains(name, t)) { isTarget = true; break; }
        }
        if (!isTarget) continue;
        if (pool.refill.empty()) break;

        size_t chosenIndex = 0;
        BossIdentity chosen;
        bool ok = false;
        for (int attempt = 0; attempt < kRerollCap; attempt++) {
            chosenIndex = (size_t)RandIndex(rng, pool.refill.size());
            chosen = pool.refill[chosenIndex];
            if (Contains(name, chosen.model)) continue;
            ok = true;
            break;
        }
        if (!ok) continue;

        int32_t modelIndex = FindEnemyModelIndex(msbb, chosen.model);
        if (modelIndex < 0) continue;

        ApplyIdentity(blob, chosen, modelIndex);
        // The reference draws AddTheRest from combinedBossList2 with removal.
        pool.refill.erase(pool.refill.begin() + (long)chosenIndex);
        changed++;

        Log((mapName + "/" + name + ": add-the-rest -> " + std::to_string(chosen.npc) + "*" +
             std::to_string(chosen.think) + "*" + chosen.model).c_str());
    }

    return changed;
}

} // namespace bbr
