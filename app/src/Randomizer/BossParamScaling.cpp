#include "BossParamScaling.h"

#include "NpcScalingTable.h"

#include <vector>

namespace bbr {

namespace {

// Mirrors the reference tool's own npcParamPositions build exactly (nested
// loop, no early-exit): for every tracked base ID, record every position in
// the scaling table where that exact value appears. Computed once and
// cached, rather than recomputed per map call as the reference source does -
// same result, since neither table changes during a run.
const std::vector<int>& NpcParamPositions() {
    static const std::vector<int> kPositions = [] {
        std::vector<int> positions;
        const auto& params = NpcParamsTable();
        const auto& scaling = NpcScalingTable();
        for (size_t i = 0; i < params.size(); i++) {
            for (size_t j = 0; j < scaling.size(); j++) {
                if (params[i] == scaling[j]) positions.push_back((int)j);
            }
        }
        return positions;
    }();
    return kPositions;
}

} // namespace

int ApplyBossParamScaling(MsbbFile& msbb, int zoneScale) {
    if (zoneScale == 0) return 0; // mirrors the reference source's empty case (m21_01)

    const auto& scaling = NpcScalingTable();
    const std::vector<int>& positions = NpcParamPositions();
    int changed = 0;

    for (int p : positions) {
        size_t scaledIndex = (size_t)p + (size_t)zoneScale;
        if (scaledIndex >= scaling.size()) continue; // defensive - see header note
        int32_t scaledValue = (int32_t)scaling[scaledIndex];
        int64_t trackedValue = scaling[(size_t)p];

        for (auto& partBlob : msbb.parts.entries) {
            if (part_fields::GetType(partBlob) != PartsType::kEnemy) continue;
            if (part_fields::GetEnemyNPCParamID(partBlob) == trackedValue) {
                part_fields::SetEnemyNPCParamID(partBlob, scaledValue);
                changed++;
            }
        }
    }

    return changed;
}

} // namespace bbr
