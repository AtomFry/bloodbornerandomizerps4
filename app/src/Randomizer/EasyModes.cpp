#include "EasyModes.h"

#include "../Platform/Log.h"

namespace bbr {

namespace {

bool FlagEnabled(EasyModeFlag flag, const EasyModeOptions& options) {
    switch (flag) {
        case EasyModeFlag::kShadows:  return options.shadows;
        case EasyModeFlag::kRom:      return options.rom;
        case EasyModeFlag::kFailures: return options.failures;
        case EasyModeFlag::kEmissary: return options.emissary;
    }
    return false;
}

int& CounterFor(EasyModeFlag flag, EasyModeCounts& counts) {
    switch (flag) {
        case EasyModeFlag::kShadows:  return counts.shadows;
        case EasyModeFlag::kRom:      return counts.rom;
        case EasyModeFlag::kFailures: return counts.failures;
        case EasyModeFlag::kEmissary: return counts.emissary;
    }
    return counts.shadows;
}

// Mirrors the reference's Name.Contains - see the header on why matching is
// substring, and why keying the table by exact map name is what contains it.
bool MatchesAnyPattern(const std::string& name, const char* const* patterns) {
    for (const char* const* p = patterns; *p != nullptr; p++) {
        if (name.find(*p) != std::string::npos) return true;
    }
    return false;
}

// Same guard as EnemyRandomizer.cpp's helper of the same name: an index out
// of range yields an empty string rather than reading past the section.
std::string ModelNameAtIndex(const MsbbSection& models, int32_t index) {
    if (index < 0 || (size_t)index >= models.entries.size()) return std::string();
    return model_fields::GetName(models.entries[(size_t)index]);
}

} // namespace

int ApplyEasyModes(const std::string& mapName, MsbbFile& msbb,
                   const EasyModeOptions& options, EasyModeCounts& counts) {
    // Tested before anything is read, so a run with all four off costs one
    // boolean compare per map and touches nothing.
    if (!options.Any()) return 0;

    // Does any ENABLED row name this map? Exact map name, never a prefix:
    // the patterns are substrings, and several of them match placements in
    // maps this feature must not touch (header, plan-evidence 018 E4 M5).
    bool applies = false;
    for (const EasyModeEntry& entry : EasyModeTable()) {
        if (mapName == entry.map && FlagEnabled(entry.flag, options)) {
            applies = true;
            break;
        }
    }
    if (!applies) return 0;

    // A placement cannot reference a model name the map's Models section does
    // not declare - the reference's own MSBB.Write throws for exactly this.
    // StepMergeModels runs unconditionally and guarantees c2521 is present in
    // all 24 maps, so this lookup cannot fail today. It is checked anyway
    // because the failure it guards against - a future change making the
    // merge conditional - would otherwise surface as a map that will not load
    // on hardware. Log once and leave every placement in this map alone; a -1
    // model index is never written. Deliberate divergence from the reference,
    // which throws (plan-evidence 018 E5.6).
    int32_t modelIndex = -1;
    for (size_t i = 0; i < msbb.models.entries.size(); i++) {
        if (model_fields::GetType(msbb.models.entries[i]) != ModelType::kEnemy) continue;
        if (model_fields::GetName(msbb.models.entries[i]) == kEasyModeModelName) {
            modelIndex = (int32_t)i;
            break;
        }
    }
    if (modelIndex < 0) {
        Log((mapName + ": easy modes skipped - model " + kEasyModeModelName +
             " is not declared in this map")
                .c_str());
        return 0;
    }

    int replaced = 0;

    // One pass per enabled row naming this map, which is the reference's own
    // shape. Every map appears in exactly one row today, so this is one pass.
    for (const EasyModeEntry& entry : EasyModeTable()) {
        if (mapName != entry.map) continue;
        if (!FlagEnabled(entry.flag, options)) continue;

        for (auto& partBlob : msbb.parts.entries) {
            if (part_fields::GetType(partBlob) != PartsType::kEnemy) continue;

            std::string name = part_fields::GetName(partBlob);
            if (!MatchesAnyPattern(name, entry.patterns)) continue;

            int32_t oldNpc = part_fields::GetEnemyNPCParamID(partBlob);
            int32_t oldThink = part_fields::GetEnemyThinkParamID(partBlob);
            std::string oldModel =
                ModelNameAtIndex(msbb.models, part_fields::GetModelIndex(partBlob));

            // Exactly three fields, matching the reference. EntityID, the
            // part name and the position are deliberately left alone - the
            // fight scripts track each body by entity ID.
            part_fields::SetEnemyNPCParamID(partBlob, kEasyModeNpcParamId);
            part_fields::SetEnemyThinkParamID(partBlob, kEasyModeThinkParamId);
            part_fields::SetModelIndex(partBlob, modelIndex);

            CounterFor(entry.flag, counts)++;
            replaced++;

            // The same map/old/new shape the enemy loop's own per-placement
            // line uses, so an easy replacement reads out of the log right
            // next to whatever roll it has just overwritten.
            Log((mapName + " " + name + ": " + std::to_string(oldNpc) + "*" +
                 std::to_string(oldThink) + "*" + oldModel + " -> " +
                 std::to_string(kEasyModeNpcParamId) + "*" +
                 std::to_string(kEasyModeThinkParamId) + "*" + kEasyModeModelName)
                    .c_str());
        }
    }

    return replaced;
}

} // namespace bbr
