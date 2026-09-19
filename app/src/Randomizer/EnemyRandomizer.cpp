// EnemyRandomizer.cpp - C++ port of RandomizeFunctions.cs/StartFunctions.cs/
// MainWindow.xaml.cs's "Randomize Enemies" feature. Ported once every
// dependency was read in full: the MSBB/DCX formats (see Msb/), the exact
// exclusion list and model-size table (EnemyExclusionList.h/ModelSizeTable.h,
// transcribed verbatim from the reference source/data files), and the exact
// mutation the reference tool performs (only Part.Enemy's NPCParamID/
// ThinkParamID/ModelName - nothing else).
//
// Deliberate differences from the reference tool, all scoped to what our
// own UI actually exposes (RANDOMIZE ENEMIES on, everything else off) or to
// fixing a real bug in the reference tool rather than faithfully
// reproducing it:
//
//  - Chalice dungeons (m29_*) are out of scope entirely: our UI has no
//    "chalice enemies" setting, and the reference tool's own chaliceEnemies
//    default is false. The 24 base overworld/DLC map files below are
//    exactly the reference tool's own hardcoded per-map call list
//    (StartFunctions.cs), cross-checked against the actual file listing in
//    the reference install's dvdroot_ps4/map/mapstudio/.
//  - The m28-specific "re-include these 6 c1050_* instances" override in
//    the reference Randomize() IS reproduced, and like the reference's it is
//    unconditional. Those six Yahar'gul placements bypass the fixed exclusion
//    test, the user's ENEMIES SKIPPED list and the zone-chance roll alike, so
//    ticking the chime maiden rows still leaves those twelve placements free
//    to change. See EnemyExclusionList.h's M28ForcedMaidenList() and
//    StepWriteMap below.
//  - ENEMIES SKIPPED (feature 032) is the port's own setting, not the
//    reference's: the reference has no per-creature "leave this one alone"
//    list. It replaces the reference's bellMaidenBool, which this port shipped
//    as UNCHANGED BELL MAIDENS and then retired - that checkbox's two live
//    patterns are now two of the 85 rows. See EnemySkipList.h.
//  - A selection that leaves nothing to draw no longer ends the run. When
//    every enemy the pool is allowed to contain is also one the run was told
//    to leave alone, the POOL half of that instruction yields for that run and
//    the pool is rebuilt without it; the PLACEMENT half still holds, so the
//    skipped placements stay frozen. That is feature 032 D4, and it exists
//    because the old Fail fired after StepMirror had already copied all six
//    folders - see StepBuildPool. An unusable VanillaSource still fails, with
//    its own message, which is the whole reason the two causes had to be
//    separated before this could change.
//  - The reference tool's per-map `cc` flag - which, once ANY enemy in a
//    map matches the exclusion list, forces every later enemy in that same
//    map to skip its zone-chance roll and randomize unconditionally for the
//    rest of the map - reads as leftover variable-scoping from the original
//    author (declared outside the per-enemy loop, never reset) rather than
//    intentional design, and isn't reproduced; the zone chance below is
//    applied as a plain per-enemy roll.
//  - `sizeOfEnemy` (the size-gate tolerance multiplier) has no field
//    initializer in the reference source (FieldContainer.cs) and is only
//    ever set from a slider's event handler - so on a fresh run where the
//    user never touches that slider, it's 0.0, which makes the retry
//    loop's `newSize > originalSize * 0` almost always true and the size
//    gate effectively never satisfied (every roll burns all 30000 tries).
//    kSizeToleranceMultiplier below is a real, working default instead.
//  - The reference tool re-reads and re-writes every map file several times
//    across separate passes (model-list merge, pool generation, mutation).
//    This port does one read and one write per map.
//
// After the main mutation pass, this also runs the reference tool's
// ParamScalingForBosses step (see BossParamScaling.h) - gated there by "No
// Scaling", which is unchecked by default, i.e. it runs unless a user
// explicitly disables it. Its data (NpcScalingTable.h) contains values that
// look like corruption (e.g. 900000006) but are real, intentional, hand-
// curated NPCParamID variants the reference tool ships - confirmed by
// checking the reference tool's own data files directly, not by inference.

#include "EnemyRandomizer.h"

#include "BossParamScaling.h"
#include "BossRandomizer.h"
#include "EnemyExclusionList.h"
#include "EnemySkipList.h"
#include "TreasureRandomizer.h"

#include "../Param/ParamBnd.h"
#include "DropRandomizer.h"
#include "StartingWeapons.h"
#include "FileIo.h"
#include "ModelSizeTable.h"
#include "PermaDarkness.h"

#include "../Msb/Dcx.h"
#include "../Msb/Msbb.h"
#include "../Platform/Log.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace bbr {

namespace {

struct MapEntry {
    const char* name;
    int zoneChance; // 0-100, mirrors the reference tool's per-zone Chance defaults
};

// Exactly the reference tool's hardcoded per-map call list (StartFunctions.cs),
// cross-checked against the real file listing under the reference install's
// map/mapstudio/ directory. Zone chance values are the reference tool's own
// FieldContainer.cs field-initializer defaults (HemwickChance,
// OldYharnamChance, ... - all default 100 except UpperCathedralWard/Cainhurst
// at 60, HuntersNightmare at 90, ResearchHall at 30). m21_* maps aren't
// matched by any of the reference tool's `currentMap.Contains(...)` zone
// checks at all, so they get no gate (100, same practical effect).
const std::array<MapEntry, 24> kBaseMaps = {{
    {"m21_00_00_00", 100},
    {"m21_01_00_00", 100},
    {"m22_00_00_00", 100},  // Hemwick Charnel Lane
    {"m23_00_00_00", 100},  // Old Yharnam
    {"m23_00_00_01", 100},
    {"m24_00_00_00", 100},  // Cathedral Ward
    {"m24_00_00_01", 100},
    {"m24_01_00_00", 100},  // Central Yharnam
    {"m24_01_00_01", 100},
    {"m24_01_00_11", 100},
    {"m24_02_00_00", 60},   // Upper Cathedral Ward
    {"m24_02_00_01", 60},
    {"m25_00_00_00", 60},   // Cainhurst Castle
    {"m26_00_00_00", 100},  // Nightmare of Mensis
    {"m27_00_00_00", 100},  // Forbidden Woods
    {"m27_00_00_01", 100},
    {"m28_00_00_00", 100},  // Yahar'gul, Unseen Village
    {"m28_00_00_01", 100},
    {"m32_00_00_00", 100},  // Byrgenwerth
    {"m32_00_00_01", 100},
    {"m33_00_00_00", 100},  // Lecture Building / Yahar'gul frontier
    {"m34_00_00_00", 90},   // Hunter's Nightmare
    {"m35_00_00_00", 30},   // Research Hall
    {"m36_00_00_00", 100},  // Fishing Hamlet
}};

// See file header comment: the reference tool's own default for this
// tolerance is an unset-slider 0.0, which effectively disables the size
// gate. This is a real, working default instead.
const double kSizeToleranceMultiplier = 2.0;

// Defensive cap on the reference tool's unbounded "reroll until the model
// name isn't one of these banned substrings" loop for m24_02/m35 (see
// below) - the reference C# has no cap at all here and would hang forever
// if the pool ever had zero valid candidates. With a pool of hundreds of
// entries and a handful of banned substrings this is never expected to
// come remotely close to triggering.
const int kBannedModelRerollCap = 100000;

// The substrings those two maps reroll away from. Named rather than written
// inline twice because the enemy picker now has to ask "does the pool contain
// anything that ISN'T one of these" before entering the reroll loop.
const std::array<const char*, 5> kBannedM2402 = {{
    "c1000", "c5040", "2630", "1051", "c1180"
}};
const std::array<const char*, 10> kBannedM35 = {{
    "c1000", "c5040", "2630", "1051", "c1180",
    "c1111", "c2620", "2090", "1250", "1260"
}};

struct PoolEntry {
    int32_t npc = 0;
    int32_t think = 0;
    std::string model;
};

bool ParsePoolString(const std::string& s, PoolEntry& out) {
    size_t firstStar = s.find('*');
    size_t lastStar = s.rfind('*');
    if (firstStar == std::string::npos || lastStar == std::string::npos || firstStar == lastStar) {
        return false;
    }
    out.npc = atoi(s.substr(0, firstStar).c_str());
    out.think = atoi(s.substr(firstStar + 1, lastStar - firstStar - 1).c_str());
    out.model = s.substr(lastStar + 1, 5);
    return true;
}

std::string ModelNameAtIndex(const MsbbSection& models, int32_t index) {
    if (index < 0 || (size_t)index >= models.entries.size()) return std::string();
    return model_fields::GetName(models.entries[(size_t)index]);
}

// Templated on the container so the same check serves both an inline
// initializer_list and the named kBannedM2402/kBannedM35 arrays.
template <typename Needles>
bool ContainsAny(const std::string& s, const Needles& needles) {
    for (const char* n : needles) {
        if (s.find(n) != std::string::npos) return true;
    }
    return false;
}

// AFR needs all six of these present, not just the files that actually
// change - the reference tool's own README is explicit about it, confirmed
// by direct observation rather than derived from "AFR is a file-level
// redirect" reasoning. They're mirrored verbatim from the vanilla source
// first; the write phase then overwrites only the map files actually
// randomized, and the emevd phase only common.emevd.dcx - everything else
// stays an exact vanilla copy. One mirror step per folder, which is also
// why this is an array rather than an inline initializer list.
const std::array<const char*, 6> kMirrorFolders = {{
    "chr", "event", "map", "param", "script", "sfx"
}};

// The two messages StepBuildPool can fail with. Both reach the TV verbatim,
// behind EnableWizardScreen's "ENEMY RANDOMIZATION FAILED - " prefix, and
// Font8x8.cpp's glyph table has no lowercase and no ':' - an unrenderable
// character draws as a full-width BLANK column rather than as nothing, so a
// lowercase message satisfies every width budget and still shows the player an
// empty line. Both are therefore written inside the font's 42-character set,
// and inside the 42 characters the 29-character prefix leaves on a
// 71-character line. pool_verify.py selftest case 6 parses these two
// definitions and asserts both properties; do not inline the strings.
//
// The cost is that the text log reads "enemy randomizer: VANILLA SOURCE ..."
// rather than all-lowercase. See plan 032 section 3.4 (P18) for why the case
// is fixed here rather than at the UI.
const char* const kFailNoMapsRead = "VANILLA SOURCE UNREADABLE - NO MAPS FOUND";
const char* const kFailNoCandidates = "NO ENEMIES FOUND IN THE VANILLA MAPS";

std::string Upper(const std::string& s) {
    std::string out = s;
    for (char& c : out) c = (char)toupper((unsigned char)c);
    return out;
}

} // namespace

struct EnemyRandomizerJob::State {
    // MergeModels is split out of WriteMaps because boss assignment also needs
    // to resolve a model name to an index, and it runs before the write loop.
    // BossCollect/BossAssign are skipped entirely when bosses are off.
    enum class Phase {
        Mirror, ReadMaps, BuildPool, MergeModels, BossCollect, BossAssign,
        TreasureCollect, TreasureAssign, WriteMaps, Emevd, ItemData, Finished
    };

    struct LoadedMap {
        std::string name;
        int zoneChance;
        MsbbFile msbb;
    };

    State(const std::string& vanillaDir, const std::string& outputDir, uint32_t seed,
          const EnemyRandomizerOptions& opts)
        : vanillaDvdrootDir(vanillaDir), outputDvdrootDir(outputDir), rng(seed), options(opts) {
        maps.reserve(kBaseMaps.size());
        // Once per run rather than once per placement: the ticked rows never
        // change mid-run, and the write loop asks this question ~2,900 times.
        skipPatterns = BuildSkipPatterns(options.enemiesSkipped);
    }

    // Boss phases address maps by name; the write loop addresses them by index.
    LoadedMap* FindMap(const std::string& name) {
        for (LoadedMap& lm : maps) {
            if (lm.name == name) return &lm;
        }
        return nullptr;
    }

    int RandInt(int loInclusive, int hiInclusive) {
        std::uniform_int_distribution<int> d(loInclusive, hiInclusive);
        return d(rng);
    }

    // `pool` is guaranteed non-empty here, by StepBuildPool rather than by a
    // guard on this line: it either failed the run, or chose a vector it had
    // already checked was non-empty. The one path that reaches StepWriteMap
    // with an empty pool is "enemies are off", and that loop's whole body is
    // gated on options.randomizeEnemies, so it never gets this far. That
    // matters because RandInt(0, -1) is uniform_int_distribution(0, -1), whose
    // precondition is a <= b: it is undefined behaviour, and the garbage it
    // returns then indexes an empty vector. Do not make an empty pool
    // reachable here.
    PoolEntry DrawCandidate() {
        PoolEntry e;
        ParsePoolString(pool[(size_t)RandInt(0, (int)pool.size() - 1)], e);
        return e;
    }

    // Ends the run unsuccessfully. Mirrors the early `return result` paths
    // the single-shot version used before it was made resumable.
    void Fail(const std::string& message) {
        result.success = false;
        result.error = message;
        Log(("enemy randomizer: " + message).c_str());
        phase = Phase::Finished;
    }

    void StepMirror();
    void StepReadMap();
    void StepBuildPool();
    void StepMergeModels();
    void StepBossCollect();
    void StepBossAssign();
    void StepTreasureCollect();
    void StepTreasureAssign();
    Phase AfterBossPhase() const; // treasure work, or straight to the write loop
    void StepWriteMap();
    void StepEmevd();
    void StepItemData();

    std::string vanillaDvdrootDir;
    std::string outputDvdrootDir;
    std::mt19937 rng;
    EnemyRandomizerOptions options;

    Phase phase = Phase::Mirror;
    size_t mirrorIndex = 0;
    size_t readIndex = 0;
    size_t mergeIndex = 0;
    size_t bossCollectIndex = 0;
    size_t bossAssignIndex = 0;
    size_t treasureCollectIndex = 0;
    size_t treasureAssignIndex = 0;
    size_t writeIndex = 0;

    std::vector<LoadedMap> maps;
    std::vector<std::string> pool;

    // The same pool built WITHOUT the user's own skip choice, i.e. ignoring
    // ENEMIES SKIPPED. The reference tool's fixed
    // exclusion list and ENEMIES INCLUDED still apply to it; only the "leave
    // this creature alone" instruction does not. StepBuildPool draws from it
    // when `pool` comes out empty, which is feature 032 D4, and frees it
    // either way. Built during the read phase because contribution-time
    // filtering and after-the-fact filtering are different rules once the
    // per-map NPC-id dedupe is in play - see StepReadMap.
    std::vector<std::string> poolIgnoringSkips;

    // ENEMIES SKIPPED, flattened to the ticked rows' model ids. Empty when
    // nothing is ticked, which is the default.
    std::vector<std::string> skipPatterns;

    // Does the pool contain anything m24_02 / m35 are allowed to use? Computed
    // once in StepBuildPool; see the reroll loops in StepWriteMaps for why.
    bool poolHasUnbannedM2402 = false;
    bool poolHasUnbannedM35 = false;

    std::vector<std::string> allEnemyModelNames;
    std::set<std::string> seenModelNames;
    std::string outMapDir;

    BossPool bossPool;
    std::string orphanMap;
    TreasurePool treasurePool;
    std::vector<uint8_t> itemDataPlain; // decompressed archive, held between steps
    int itemDataStep = 0;

    EnemyRandomizerResult result;

    int stepsDone = 0;
    // Refined in StepBuildPool once the real map count is known.
    int stepsTotal = (int)kMirrorFolders.size() + (int)kBaseMaps.size() + 1 +
                     (int)kBaseMaps.size() + 1;
};

void EnemyRandomizerJob::State::StepMirror() {
    const char* folder = kMirrorFolders[mirrorIndex];
    Log((std::string("enemy randomizer: mirroring ") + folder + " from vanilla source").c_str());

    std::string src = vanillaDvdrootDir + "/" + folder;
    std::string dst = outputDvdrootDir + "/" + folder;
    if (!CopyDirRecursive(src, dst)) {
        Log((std::string("enemy randomizer: warning - incomplete mirror of ") + folder).c_str());
    }

    if (++mirrorIndex >= kMirrorFolders.size()) {
        Log("enemy randomizer: pass 1 - reading maps and building enemy pool");
        phase = Phase::ReadMaps;
    }
}

void EnemyRandomizerJob::State::StepReadMap() {
    const MapEntry& mapEntry = kBaseMaps[readIndex];

    // Every exit path from this function has to advance readIndex, so the
    // phase transition is done once here rather than at each return.
    struct AdvanceOnExit {
        State* s;
        ~AdvanceOnExit() {
            if (s->phase != Phase::ReadMaps) return; // Fail() already moved us on
            if (++s->readIndex >= kBaseMaps.size()) s->phase = Phase::BuildPool;
        }
    } advance{this};

    std::string path = vanillaDvdrootDir + "/map/mapstudio/" + mapEntry.name + ".msb.dcx";
    std::vector<uint8_t> raw;
    if (!ReadWholeFile(path, raw)) {
        Log(("enemy randomizer: skipping missing map " + std::string(mapEntry.name)).c_str());
        return;
    }

    {
        std::vector<uint8_t> plain;
        std::string dcxError;
        if (!DcxDecompress(raw, plain, &dcxError)) {
            Fail(std::string("failed to decompress ") + mapEntry.name + " (read " +
                 std::to_string(raw.size()) + " bytes): " + dcxError);
            return;
        }

        LoadedMap lm;
        lm.name = mapEntry.name;
        lm.zoneChance = mapEntry.zoneChance;
        if (!lm.msbb.Parse(plain)) {
            Fail(std::string("failed to parse MSBB for ") + mapEntry.name);
            return;
        }

        for (auto& modelBlob : lm.msbb.models.entries) {
            if (model_fields::GetType(modelBlob) == ModelType::kEnemy) {
                std::string name = model_fields::GetName(modelBlob);
                if (seenModelNames.insert(name).second) allEnemyModelNames.push_back(name);
            }
        }

        // Two pools, contributed to in one pass. They differ in exactly one
        // rule - whether the user's own skip choice applies - and each needs
        // its OWN dedupe set: contributedNpcIds is what makes a placement's
        // eligibility depend on which earlier placements contributed, so
        // sharing one set would make poolIgnoringSkips something other than
        // "the pool this run would have had without the skip". Building it
        // consumes no randomness, so a run that skips nothing is byte-for-byte
        // the run it was before this existed.
        std::set<int32_t> contributedNpcIds;
        std::set<int32_t> contributedNpcIdsIgnoringSkips;
        for (auto& partBlob : lm.msbb.parts.entries) {
            if (part_fields::GetType(partBlob) != PartsType::kEnemy) continue;

            std::string name = part_fields::GetName(partBlob);
            int32_t think = part_fields::GetEnemyThinkParamID(partBlob);
            int32_t npc = part_fields::GetEnemyNPCParamID(partBlob);
            int32_t talk = part_fields::GetEnemyTalkID(partBlob);

            // The reference tool's fixed list is not the user's choice, so
            // neither pool ignores it. ENEMIES SKIPPED is, and is what
            // separates the two.
            if (IsExcludedEnemyName(name)) continue;
            const bool skipped = IsSkippedName(name, skipPatterns);

            if (think <= 1) continue;
            if (npc <= 1) continue;
            if (name.find("c1110_0000") != std::string::npos && talk != 111010) continue;
            if (name.find("c2561") != std::string::npos) continue;

            std::string modelName = ModelNameAtIndex(lm.msbb.models, part_fields::GetModelIndex(partBlob));
            if (modelName.empty()) continue;

            // The enemy picker. Filtering here rather than at draw time is
            // what makes the feature one line: everything downstream - the
            // shuffle, the dedupe, the size gate, the per-zone chance - already
            // works against whatever the pool happens to contain. It is not a
            // skip, so it applies to both pools.
            if (!options.enemiesIncluded.IsModelEnabled(modelName, EnemyPoolTable().data())) continue;

            // The dedupe test used to sit above these rules. Moving it below
            // them changes nothing - every rule between is a pure predicate on
            // the placement, and a rejected placement never reached the insert
            // either way - and it is what lets one pass feed two pools.
            const std::string entry =
                std::to_string(npc) + "*" + std::to_string(think) + "*" + modelName;
            if (!skipped && !contributedNpcIds.count(npc)) {
                contributedNpcIds.insert(npc);
                pool.push_back(entry);
            }
            if (!contributedNpcIdsIgnoringSkips.count(npc)) {
                contributedNpcIdsIgnoringSkips.insert(npc);
                poolIgnoringSkips.push_back(entry);
            }
        }

        maps.push_back(std::move(lm));
    }
}

void EnemyRandomizerJob::State::StepBuildPool() {
    // An empty pool used to have one message and one outcome: end the run.
    // Three different things could cause it, only one of which is a failure,
    // and the one that is not is the one a player can actually reach. Feature
    // 032 D4 separates them. In order:
    //
    //   1. no map could be read at all       -> FAIL, naming the source
    //   2. maps read, nothing eligible at all -> FAIL, the residual bad tree
    //   3. everything selected was skipped    -> fall back, D4
    //   4. pool empty and enemies are off     -> the log line this always had
    //   5. otherwise                          -> today's pool, untouched
    //
    // Case 3 is the only one that changes a run's outcome rather than its
    // error message. Before this it ended the run in the single Fail below -
    // AFTER StepMirror had copied all six folders, so the player was left with
    // a half-built output tree and an error naming a cause they could not act
    // on. Confirmed on hardware:
    // data/runs/Error Log - Chime Maidens/live.log:26394.
    //
    // Cases 1 and 2 are why D4 could not simply delete the Fail, and why
    // feature 016 deferred the same change on 2026-09-16: this was also the
    // only hard error the enemy path had for an unusable VanillaSource, so
    // removing it unseparated would report a broken installation as a
    // successful run that randomized nothing. The two messages are
    // deliberately different strings.
    //
    // The UB hazard the old Fail guarded is closed by construction, not
    // relaxed - see DrawCandidate(). Cases 1 and 2 return before any draw;
    // cases 3 and 5 both select a vector already known to be non-empty; case 4
    // is the only path that reaches StepWriteMap with an empty pool, and that
    // loop's whole body is gated on options.randomizeEnemies.
    //
    // Every case tests options.randomizeEnemies explicitly, exactly as the
    // single Fail it replaces did. That gate is load-bearing rather than tidy:
    // StartCommit's empty-selection refusal only fires when enemies are on, so
    // a bosses-only run committed with an empty saved ENEMIES INCLUDED reaches
    // here with BOTH vectors empty, and without the gate it would report a
    // fallback on a run that never intended to randomize an enemy.
    if (options.randomizeEnemies) {
        if (maps.empty()) {
            Fail(kFailNoMapsRead);
            return;
        }
        if (poolIgnoringSkips.empty()) {
            Fail(kFailNoCandidates);
            return;
        }
        if (pool.empty()) {
            // D4. The POOL half of the skip instruction yields for this run;
            // the PLACEMENT half does not, so the skipped placements still
            // stay frozen in StepWriteMap. This is the one configuration in
            // which a skipped creature appears somewhere new, which is why
            // the UI is told about it.
            pool = poolIgnoringSkips;
            result.poolFellBack = true;
            Log("enemy randomizer: every selected enemy was also skipped - "
                "falling back to the selection for this run");
        }
    }
    if (pool.empty() && !options.randomizeEnemies) {
        Log("enemy randomizer: enemy pool empty and enemies disabled - skipping enemy shuffle");
    }

    // Nothing below reads it, and it is the larger of the two whenever they
    // differ.
    std::vector<std::string>().swap(poolIgnoringSkips);

    // Fisher-Yates shuffle, then dedupe exact-string duplicates preserving
    // first-occurrence order - mirrors the reference tool's "pick random
    // remaining element, remove it, repeat" shuffle followed by
    // enemyDataRandomized.Distinct().
    for (size_t i = pool.size(); i > 1; i--) {
        size_t j = (size_t)RandInt(0, (int)i - 1);
        std::swap(pool[i - 1], pool[j]);
    }
    {
        std::vector<std::string> deduped;
        std::set<std::string> seen;
        deduped.reserve(pool.size());
        for (auto& s : pool) {
            if (seen.insert(s).second) deduped.push_back(s);
        }
        pool = std::move(deduped);
    }

    // Once, rather than up to kBannedModelRerollCap times per placement.
    for (const std::string& entry : pool) {
        PoolEntry e;
        if (!ParsePoolString(entry, e)) continue;
        if (!ContainsAny(e.model, kBannedM2402)) poolHasUnbannedM2402 = true;
        if (!ContainsAny(e.model, kBannedM35)) poolHasUnbannedM35 = true;
    }

    result.poolSize = (int)pool.size();
    Log(("enemy randomizer: pool built - " + std::to_string(pool.size()) +
         " distinct candidates from " + std::to_string(maps.size()) + " maps").c_str());
    if (!poolHasUnbannedM2402 || !poolHasUnbannedM35) {
        Log("enemy randomizer: warning - selection leaves no allowed model for "
            "upper cathedral ward and/or research hall; their reroll filter is bypassed");
    }

    Log("enemy randomizer: pass 2 - merging models and mutating enemies");

    outMapDir = outputDvdrootDir + "/map/mapstudio/";
    if (!MakeDirsRecursive(outMapDir)) {
        Fail("failed to create output map directory");
        return;
    }

    // Now that the maps which actually exist have been counted, the progress
    // denominator can stop guessing.
    const int bossSteps = options.randomizeBosses
                              ? (int)BossMapOrder().size() +      // BossCollect
                                (int)BossMapOrder().size() + 2    // BossAssign: orphan + maps + rest
                              : 0;
    const int treasureSteps = options.randomizeTreasure
                                  ? 2 * (int)TreasureMapOrder().size()
                                  : 0;
    const int itemDataSteps = options.AnyParamFeature() ? 4 : 0;
    stepsTotal = (int)kMirrorFolders.size() + (int)kBaseMaps.size() + 1 +
                 (int)maps.size() + bossSteps + treasureSteps + (int)maps.size() + 1 +
                 itemDataSteps;

    if (maps.empty()) {
        phase = Phase::Emevd;
    } else {
        phase = Phase::MergeModels;
    }
}

// Every map's Models.Enemies section gets an entry for every enemy model that
// exists in any map, so a placement can later reference any of them by index.
// This used to live inline in StepWriteMap; boss assignment runs before the
// write loop and needs the same guarantee, so it moved to its own phase.
void EnemyRandomizerJob::State::StepMergeModels() {
    LoadedMap& lm = maps[mergeIndex];

    std::set<std::string> present;
    for (auto& modelBlob : lm.msbb.models.entries) {
        if (model_fields::GetType(modelBlob) == ModelType::kEnemy) {
            present.insert(model_fields::GetName(modelBlob));
        }
    }
    for (const std::string& name : allEnemyModelNames) {
        if (present.insert(name).second) {
            lm.msbb.models.entries.push_back(model_fields::BuildEnemyModelEntry(name));
        }
    }

    if (++mergeIndex >= maps.size()) {
        phase = options.randomizeBosses ? Phase::BossCollect : AfterBossPhase();
    }
}

EnemyRandomizerJob::State::Phase EnemyRandomizerJob::State::AfterBossPhase() const {
    return options.randomizeTreasure ? Phase::TreasureCollect : Phase::WriteMaps;
}

void EnemyRandomizerJob::State::StepTreasureCollect() {
    const std::string& mapName = TreasureMapOrder()[treasureCollectIndex];
    if (LoadedMap* lm = FindMap(mapName)) {
        CollectTreasureLots(lm->msbb, options.randomizeWorkshopTools, treasurePool);
    } else {
        Log(("treasure randomizer: map not loaded, skipped for pool: " + mapName).c_str());
    }

    if (++treasureCollectIndex >= TreasureMapOrder().size()) {
        Log(("treasure randomizer: pool built - " + std::to_string(treasurePool.lots.size()) +
             " item lots").c_str());
        phase = Phase::TreasureAssign;
    }
}

void EnemyRandomizerJob::State::StepTreasureAssign() {
    const std::string& mapName = TreasureMapOrder()[treasureAssignIndex];
    if (LoadedMap* lm = FindMap(mapName)) {
        result.treasuresRandomized += AssignTreasureLots(
            mapName, lm->msbb, options.randomizeWorkshopTools, treasurePool, rng);
    }

    if (++treasureAssignIndex >= TreasureMapOrder().size()) {
        Log(("treasure randomizer: " + std::to_string(result.treasuresRandomized) +
             " treasures randomized, " + std::to_string(treasurePool.lots.size()) +
             " lots unused").c_str());
        phase = Phase::WriteMaps;
    }
}

void EnemyRandomizerJob::State::StepBossCollect() {
    const std::string& mapName = BossMapOrder()[bossCollectIndex];
    if (LoadedMap* lm = FindMap(mapName)) {
        CollectBossCandidates(mapName, lm->msbb, options.bossesIncluded, bossPool);
    } else {
        Log(("boss randomizer: map not loaded, skipped for pool: " + mapName).c_str());
    }

    if (++bossCollectIndex >= BossMapOrder().size()) {
        FinalizeBossPool(bossPool);
        if (bossPool.entries.empty()) {
            Log("boss randomizer: pool empty - boss randomization will do nothing");
        }
        orphanMap = ChooseOrphanPhaseOneMap(rng);
        phase = Phase::BossAssign;
    }
}

// Step 0 applies the Orphan phase-one sync, steps 1..N assign one map each in
// the reference tool's boss-map order, and the final step runs AddTheRest -
// which the reference also does only after every map has been assigned.
void EnemyRandomizerJob::State::StepBossAssign() {
    const size_t mapCount = BossMapOrder().size();

    if (bossAssignIndex == 0) {
        if (LoadedMap* lm = FindMap(orphanMap)) {
            ApplyOrphanPhaseOne(orphanMap, lm->msbb, bossPool);
        }
        bossAssignIndex++;
        return;
    }

    if (bossAssignIndex <= mapCount) {
        const std::string& mapName = BossMapOrder()[bossAssignIndex - 1];
        if (LoadedMap* lm = FindMap(mapName)) {
            result.bossesRandomized += AssignBossesInMap(mapName, lm->msbb, bossPool, rng);
        }
        bossAssignIndex++;
        return;
    }

    for (const char* mapName : { "m22_00_00_00", "m27_00_00_01", "m35_00_00_00" }) {
        if (LoadedMap* lm = FindMap(mapName)) {
            result.bossesRandomized += AddTheRestInMap(mapName, lm->msbb, bossPool, rng);
        }
    }
    Log(("boss randomizer: " + std::to_string(result.bossesRandomized) +
         " boss placements randomized").c_str());
    phase = AfterBossPhase();
}

void EnemyRandomizerJob::State::StepWriteMap() {
    LoadedMap& lm = maps[writeIndex];

    {
        // The model merge itself moved to StepMergeModels (boss assignment runs
        // earlier and needs it too); this is only the name -> index lookup.
        std::unordered_map<std::string, int32_t> enemyModelIndex;
        for (size_t i = 0; i < lm.msbb.models.entries.size(); i++) {
            if (model_fields::GetType(lm.msbb.models.entries[i]) == ModelType::kEnemy) {
                enemyModelIndex[model_fields::GetName(lm.msbb.models.entries[i])] = (int32_t)i;
            }
        }

        bool isM2402 = lm.name.find("m24_02") != std::string::npos;
        bool isM35   = lm.name.find("m35") != std::string::npos;
        bool isM28   = lm.name.find("m28") != std::string::npos;

        // Enemy randomization is independent of the boss pass - with it off the
        // map still gets its model merge, boss assignment, scaling and write.
        // The loop body below is left at its original indentation so this stays
        // a one-line diff rather than a re-indent of 80 lines.
        if (options.randomizeEnemies) {
        for (auto& partBlob : lm.msbb.parts.entries) {
            if (part_fields::GetType(partBlob) != PartsType::kEnemy) continue;

            std::string name = part_fields::GetName(partBlob);

            // The reference's m28 override: these six placements are forced
            // back into randomization past every gate below - the fixed
            // exclusion list, the user's ENEMIES SKIPPED list, and the roll.
            bool forced = isM28 && IsM28ForcedMaidenName(name);

            if (!forced && IsExcludedEnemyName(name)) continue;
            // Inside the same !forced, so the m28 Yahar'gul override still
            // wins - the reference applies it unconditionally and feature 032
            // F5 requires that to keep holding. Before the roll, so a skipped
            // placement draws no randomness.
            if (!forced && IsSkippedName(name, skipPatterns)) continue;

            // The roll is drawn for every non-excluded placement, forced ones
            // included, and only then ignored. Skipping the draw for forced
            // placements would remove 12 RandInt calls from the stream and
            // reshuffle m28 and every map after it on every seed - see
            // docs/plans/016-unchanged-bell-maidens/plan.md A3.
            int roll = RandInt(0, 100);
            if (!forced && roll >= lm.zoneChance) continue;

            std::string originalModelName =
                ModelNameAtIndex(lm.msbb.models, part_fields::GetModelIndex(partBlob));
            int64_t originalSize = LookupModelSize(originalModelName);

            PoolEntry candidate = DrawCandidate();
            int tries = 0;
            if (isM2402 || isM35) {
                while (LookupModelSize(candidate.model) > originalSize && tries < 30) {
                    candidate = DrawCandidate();
                    tries++;
                }
            } else {
                while ((double)LookupModelSize(candidate.model) >
                           (double)originalSize * kSizeToleranceMultiplier &&
                       tries < 30000) {
                    candidate = DrawCandidate();
                    tries++;
                }
            }

            // The `poolHas...` guards exist because of the enemy picker: with
            // every enabled model on the banned list - ticking only c2630 is
            // enough, it is banned in both maps - these loops would spin the
            // full cap on every placement and then place a banned model
            // anyway. Checking once beats rerolling 100000 times to reach the
            // same answer. With the full pool both flags are true and this
            // behaves exactly as before.
            if (isM2402 && poolHasUnbannedM2402) {
                int guard = 0;
                while (guard++ < kBannedModelRerollCap &&
                       ContainsAny(candidate.model, kBannedM2402)) {
                    candidate = DrawCandidate();
                }
            }
            if (isM35 && poolHasUnbannedM35) {
                int guard = 0;
                while (guard++ < kBannedModelRerollCap &&
                       ContainsAny(candidate.model, kBannedM35)) {
                    candidate = DrawCandidate();
                }
            }

            auto it = enemyModelIndex.find(candidate.model);
            if (it == enemyModelIndex.end()) continue; // see header comment: not expected to happen

            int32_t oldThink = part_fields::GetEnemyThinkParamID(partBlob);
            int32_t oldNpc = part_fields::GetEnemyNPCParamID(partBlob);

            part_fields::SetModelIndex(partBlob, it->second);
            part_fields::SetEnemyThinkParamID(partBlob, candidate.think);
            part_fields::SetEnemyNPCParamID(partBlob, candidate.npc);
            result.enemiesRandomized++;

            // Per-enemy detail, not just the run summary - lets this be
            // diffed against the reference tool's own enemyLogFilePath
            // output (same shape: map, old npc/think/model, new
            // npc/think/model) when tracking down a behavior mismatch.
            Log((lm.name + " " + name + ": " + std::to_string(oldNpc) + "*" +
                 std::to_string(oldThink) + "*" + originalModelName + " -> " +
                 std::to_string(candidate.npc) + "*" + std::to_string(candidate.think) + "*" +
                 candidate.model)
                    .c_str());
        }

        } // if (options.randomizeEnemies)

        for (const BossScalingMapEntry& scalingEntry : BossScalingMaps()) {
            if (lm.name == scalingEntry.name) {
                int scaledCount = ApplyBossParamScaling(lm.msbb, scalingEntry.zoneScale);
                if (scaledCount > 0) {
                    Log((lm.name + ": param-scaled " + std::to_string(scaledCount) +
                         " tracked enemy NPCParamID(s) for this zone")
                            .c_str());
                    result.npcParamsScaled += scaledCount;
                }
                break;
            }
        }

        std::vector<uint8_t> newPlain = lm.msbb.Serialize();
        std::vector<uint8_t> newDcx = DcxCompress(newPlain);

        std::string outPath = outMapDir + lm.name + ".msb.dcx";
        if (!WriteWholeFile(outPath, newDcx)) {
            Fail("failed to write " + outPath);
            return;
        }
        result.mapsProcessed++;
    }

    if (++writeIndex >= maps.size()) phase = Phase::Emevd;
}

void EnemyRandomizerJob::State::StepEmevd() {
    // ENABLE MERGO DARKNESS. Both states are WRITTEN - the setting chooses
    // which of the two known byte patterns event/common.emevd.dcx ends up
    // holding, rather than one state meaning "write" and the other "leave the
    // mirrored copy alone".
    //
    // That matters for more than tidiness. The alternative made the ON state
    // mean "whatever bytes StepMirror happened to copy", which is unknown by
    // construction: if a user's VanillaSource dump is not the same revision as
    // the file their installed game ships, the result would differ between
    // users for the same seed and settings. Writing both states pins the
    // output to a known value either way.
    //
    // Which pattern gives which outcome is established by hardware test, not
    // by reading the event - the vanilla bytes produce a permanently dark
    // world and the poke produces the normal game, which is the opposite of
    // what the decode suggests. See docs/plans/mergo-darkness.md §2.7 and
    // the polarity note in PermaDarkness.h before changing this.
    //
    // Best-effort - a missing/unexpected file here shouldn't fail the whole
    // run, since map randomization is the actual feature.
    {
        std::string emevdInPath = vanillaDvdrootDir + "/event/common.emevd.dcx";
        std::vector<uint8_t> emevdRaw;
        if (ReadWholeFile(emevdInPath, emevdRaw)) {
            std::vector<uint8_t> emevdPlain;
            std::string emevdErr;
            if (DcxDecompress(emevdRaw, emevdPlain, &emevdErr)) {
                if (ApplyPermaDarkness(emevdPlain, options.enableMergoDarkness)) {
                    std::string emevdOutDir = outputDvdrootDir + "/event/";
                    if (MakeDirsRecursive(emevdOutDir) &&
                        WriteWholeFile(emevdOutDir + "common.emevd.dcx", DcxCompress(emevdPlain))) {
                        Log(options.enableMergoDarkness
                                ? "enemy randomizer: mergo darkness ON - wrote event/common.emevd.dcx"
                                : "enemy randomizer: mergo darkness OFF - wrote event/common.emevd.dcx");
                    } else {
                        Log("enemy randomizer: failed to write event/common.emevd.dcx");
                    }
                } else {
                    Log("enemy randomizer: event/common.emevd.dcx - target event not found, skipped");
                }
            } else {
                Log(("enemy randomizer: failed to decompress event/common.emevd.dcx: " + emevdErr).c_str());
            }
        } else {
            Log("enemy randomizer: event/common.emevd.dcx not found in vanilla source, skipped");
        }
    }

    Log(("enemy randomizer: done - " + std::to_string(result.mapsProcessed) + " maps, " +
         std::to_string(result.enemiesRandomized) + " enemies randomized, " +
         std::to_string(result.npcParamsScaled) + " param-scaled").c_str());

    if (options.AnyParamFeature()) {
        phase = Phase::ItemData;
        return;
    }

    result.success = true;
    phase = Phase::Finished;
}

// Enemy drops live in the game's item-data archive rather than in any map, so
// this phase decompresses that archive, rewrites NpcParam's drop field in
// place, and re-emits it. Nothing is rebuilt - see Param/ParamBnd.h.
//
// Two independent risks ride on this phase, and the log distinguishes them: the
// randomization itself, and the fact that this port's compressor writes stored
// (uncompressed) blocks, so the archive lands ~15x larger than the original.
void EnemyRandomizerJob::State::StepItemData() {
    const std::string relPath = "/param/gameparam/gameparam.parambnd.dcx";

    if (itemDataStep == 0) {
        // The parsed maps are no longer needed and the archive is large; free
        // them first so the two buffers never coexist with 24 maps in memory.
        maps.clear();
        maps.shrink_to_fit();

        std::vector<uint8_t> raw;
        if (!ReadWholeFile(vanillaDvdrootDir + relPath, raw)) {
            Log("item data: gameparam.parambnd.dcx not found in vanilla source, skipped");
            itemDataStep = 3;
            return;
        }
        std::string err;
        if (!DcxDecompress(raw, itemDataPlain, &err)) {
            Fail("failed to decompress gameparam.parambnd.dcx: " + err);
            return;
        }
        result.itemDataPlainBytes = itemDataPlain.size();
        Log(("item data: read " + std::to_string(raw.size()) + " bytes, decompressed to " +
             std::to_string(itemDataPlain.size())).c_str());
        itemDataStep = 1;
        return;
    }

    if (itemDataStep == 1) {
        std::vector<ParamMember> members;
        std::string err;
        if (!ParseParamBnd(itemDataPlain, members, &err)) {
            Fail("failed to parse gameparam.parambnd.dcx: " + err);
            return;
        }
        result.itemDataMembers = (int)members.size();
        Log(("item data: archive holds " + std::to_string(members.size()) + " entries").c_str());

        // Each param feature locates only what it needs, so a missing member
        // is only fatal to the feature that wants it.
        if (options.randomizeEnemyDrops) {
            const ParamMember* npc = FindParamMember(members, "NpcParam.param");
            if (npc == nullptr) {
                Fail("NpcParam.param not present in gameparam.parambnd.dcx");
                return;
            }
            DropRandomizerResult drops;
            if (!RandomizeEnemyDrops(itemDataPlain, *npc, rng, drops, &err)) {
                Fail("enemy drop randomization failed: " + err);
                return;
            }
            result.dropsRandomized = drops.rowsChanged;
            result.dropPoolSize = drops.poolSize;
        }

        StartingWeaponOptions weaponOpts;
        weaponOpts.randomizeStartingWeapons = options.randomizeStartingWeapons;
        weaponOpts.randomizeStartingGuns = options.randomizeStartingGuns;
        weaponOpts.randomizeShopWeapons = options.randomizeShopWeapons;

        if (weaponOpts.Any()) {
            const ParamMember* shop = FindParamMember(members, "ShopLineupParam.param");
            const ParamMember* weapon = FindParamMember(members, "EquipParamWeapon.param");
            if (shop == nullptr || weapon == nullptr) {
                Fail("ShopLineupParam.param or EquipParamWeapon.param missing from "
                     "gameparam.parambnd.dcx");
                return;
            }
            StartingWeaponsResult weapons;
            if (!RandomizeStartingWeapons(itemDataPlain, *shop, *weapon, weaponOpts, rng,
                                          weapons, &err)) {
                Fail("starting weapon randomization failed: " + err);
                return;
            }
            result.startingMeleeChanged = weapons.meleeSlotsChanged;
            result.startingGunsChanged = weapons.gunSlotsChanged;
            result.shopWeaponsChanged = weapons.shopRowsChanged;
        }

        itemDataStep = 2;
        return;
    }

    if (itemDataStep == 2) {
        std::string outDir = outputDvdrootDir + "/param/gameparam/";
        std::vector<uint8_t> outDcx = DcxCompress(itemDataPlain);
        result.itemDataWrittenBytes = outDcx.size();

        if (!MakeDirsRecursive(outDir) ||
            !WriteWholeFile(outDir + "gameparam.parambnd.dcx", outDcx)) {
            Fail("failed to write gameparam.parambnd.dcx");
            return;
        }
        Log(("item data: wrote " + std::to_string(outDcx.size()) + " bytes").c_str());

        itemDataPlain.clear();
        itemDataPlain.shrink_to_fit();
        itemDataStep = 3;
        return;
    }

    result.success = true;
    phase = Phase::Finished;
}

EnemyRandomizerJob::EnemyRandomizerJob(const std::string& vanillaDvdrootDir,
                                       const std::string& outputDvdrootDir,
                                       uint32_t seed,
                                       const EnemyRandomizerOptions& options)
    : s_(new State(vanillaDvdrootDir, outputDvdrootDir, seed, options)) {}

EnemyRandomizerJob::~EnemyRandomizerJob() = default;

bool EnemyRandomizerJob::Done() const {
    return s_->phase == State::Phase::Finished;
}

void EnemyRandomizerJob::Step() {
    if (Done()) return;

    switch (s_->phase) {
        case State::Phase::Mirror:      s_->StepMirror();      break;
        case State::Phase::ReadMaps:    s_->StepReadMap();     break;
        case State::Phase::BuildPool:   s_->StepBuildPool();   break;
        case State::Phase::MergeModels: s_->StepMergeModels(); break;
        case State::Phase::BossCollect: s_->StepBossCollect(); break;
        case State::Phase::BossAssign:  s_->StepBossAssign();  break;
        case State::Phase::TreasureCollect: s_->StepTreasureCollect(); break;
        case State::Phase::TreasureAssign:  s_->StepTreasureAssign();  break;
        case State::Phase::WriteMaps:   s_->StepWriteMap();    break;
        case State::Phase::Emevd:       s_->StepEmevd();       break;
        case State::Phase::ItemData:    s_->StepItemData();    break;
        case State::Phase::Finished:    break;
    }

    s_->stepsDone++;
}

// Describes the work the NEXT Step() will do, not the work just finished.
// That's deliberate: the caller draws a frame between steps, so a status
// set here is on screen for exactly the duration of the step it names -
// naming the completed step instead would show every label one step late.
std::string EnemyRandomizerJob::StatusText() const {
    switch (s_->phase) {
        case State::Phase::Mirror:
            return "COPYING " + Upper(kMirrorFolders[s_->mirrorIndex]);
        case State::Phase::ReadMaps:
            return "READING MAP " + std::to_string(s_->readIndex + 1) + " OF " +
                   std::to_string(kBaseMaps.size());
        case State::Phase::BuildPool:
            return "BUILDING ENEMY POOL";
        case State::Phase::MergeModels:
            return "MERGING MODELS " + std::to_string(s_->mergeIndex + 1) + " OF " +
                   std::to_string(s_->maps.size());
        case State::Phase::BossCollect:
            return "BUILDING BOSS POOL " + std::to_string(s_->bossCollectIndex + 1) + " OF " +
                   std::to_string(BossMapOrder().size());
        case State::Phase::BossAssign:
            return "RANDOMIZING BOSSES " + std::to_string(s_->bossAssignIndex) + " OF " +
                   std::to_string(BossMapOrder().size());
        case State::Phase::TreasureCollect:
            return "BUILDING ITEM POOL " + std::to_string(s_->treasureCollectIndex + 1) +
                   " OF " + std::to_string(TreasureMapOrder().size());
        case State::Phase::TreasureAssign:
            return "RANDOMIZING TREASURE " + std::to_string(s_->treasureAssignIndex + 1) +
                   " OF " + std::to_string(TreasureMapOrder().size());
        case State::Phase::WriteMaps:
            return "RANDOMIZING MAP " + std::to_string(s_->writeIndex + 1) + " OF " +
                   std::to_string(s_->maps.size());
        case State::Phase::Emevd:
            return "PATCHING EVENT SCRIPTS";
        case State::Phase::ItemData:
            return "RANDOMIZING ENEMY DROPS";
        case State::Phase::Finished:
            return s_->result.success ? "DONE" : "FAILED";
    }
    return "WORKING";
}

float EnemyRandomizerJob::Progress() const {
    if (Done()) return 1.0f;
    if (s_->stepsTotal <= 0) return 0.0f;

    float p = (float)s_->stepsDone / (float)s_->stepsTotal;
    if (p < 0.0f) return 0.0f;
    if (p > 1.0f) return 1.0f;
    return p;
}

const EnemyRandomizerResult& EnemyRandomizerJob::Result() const {
    return s_->result;
}

} // namespace bbr
