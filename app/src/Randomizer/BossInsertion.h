// BossInsertion.h - BOSSES CAN REPLACE ENEMIES and its seven modifiers
// (feature 013). Writes boss identities over ordinary enemy placements in the
// fourteen explorable areas, so a boss can stand loose in the world with no fog
// gate, no health bar and no music.
//
// C++ port of the reference tool's InsertBossesVoid(currentMap, nonoList)
// (reference/Randomizer/MainWindowComponents/RandomizeFunctions.cs:1460-1552),
// driven by StartFunctions.cs:1013-1133's fourteen per-area flags, and fed by
// the insertBossesEnemy harvest inside GenerateBossList (:1556-1846).
//
// WHERE IT RUNS, and why the position is behaviour rather than taste. The call
// sits inside StepWriteMap, AFTER the enemy loop, AFTER boss assignment (which
// finishes before the first map is written) and AFTER ApplyEasyModes, and
// BEFORE the BossScalingMaps() loop. That is a SECOND PASS over each map, which
// is both the reference's own ordering - enemies, bosses, AddTheRest,
// EasyModes, then ParamScalingForBosses - and what makes SPREAD BOSSES OUT
// possible at all: the whole map's eligible set has to be in hand before any
// placement is chosen, and a per-placement decision taken during the enemy loop
// could never supply that (spec 013 D-N).
//
// Running before the scaling loop is the other half: the scaling pass rewrites
// whatever NPCParamID it finds, so an inserted boss in a scaled area picks up
// that area's variant rather than keeping its arena stats. Seven of the 32
// identities are not in the scaling table and therefore DO arrive at arena
// strength; that is a consequence to expect, not to correct.
//
// NOTHING IS DRAWN WHEN THE FEATURE IS OFF. Every gate - the master toggle, the
// empty picker, the area tick, the zero count - is tested before the first
// RandInt, so a run with BOSSES CAN REPLACE ENEMIES off is byte-identical to
// the run the app made before this existed, roll for roll. A single stray draw
// would reshuffle every map after it on every seed, and up to 178 draws per map
// share the run's one mt19937, so this is the invariant to check first after
// touching anything here.
//
// EXACTLY THREE FIELDS are written per replaced placement, the same three the
// enemy and boss passes write: NPCParamID, ThinkParamID and the model index.
// Position, rotation, scale, EntityID and the part name are read-only - the
// position is read, because spacing needs it, and never written. Nothing moves.
//
// WHAT AN ELIGIBLE TARGET IS (spec 013 D-C). A Part.Enemy that:
//   * the 103-pattern fixed exclusion list does not name,
//   * ENEMIES SKIPPED does not tick,
//   * DO NOT RANDOMIZE CAGED DOGS does not protect while it is on,
//   * and whose model name resolves.
// The first of those is what keeps every boss placement out: all 62 boss-named
// placements across the 24 base maps are on the fixed list, so zero of them are
// eligible, measured rather than assumed (plan-evidence 013 §E4 M4). That is
// why there is no boss-name test in the collector - the rule is already there,
// once, and a second copy of it could only disagree with the first. The three
// protections hold under REPLACE ALL ENEMIES too.
//
// THE LIST IS NOT BossPoolTable.h's. The reference harvests insertBossesEnemy
// at :1657, which sits before three of the de-eligibility tests the boss pool
// applies, and it applies neither the 15-pattern insert-reject list nor the
// per-map npc dedupe. Measured: 32 identities across 22 models against the boss
// pool's 18 across 17. So this has its own harvest, its own table
// (InsertedBossTable.h), its own picker and its own config key, and
// BossRandomizer.cpp is not edited (plan 013 P5).
//
// WHAT NONE OF THIS ESTABLISHES: whether a loose boss aggros, fights, dies or
// crashes the game; whether a save made in an affected area still loads;
// whether REPLACED ENEMY'S AI works at all. A think id refers to behaviours
// that belong to a particular body, and reading a byte has never told this
// project what Bloodborne does with it (CLAUDE.md §5). Only the console can say
// - see spec 013 §8's hardware plan.
#pragma once

#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include "../Msb/Msbb.h"
#include "BossRandomizer.h"      // BossIdentity, reused as a plain value type
#include "EnemyPoolSelection.h"

namespace bbr {

// The eight settings, plus the two protections this pass has to honour that
// belong to other settings.
struct InsertionOptions {
    // BOSSES CAN REPLACE ENEMIES. The master switch; with it false the other
    // seven do nothing and the pass returns before its first draw.
    bool enabled = false;

    // BOSS REPLACEMENTS: false is UP TO, true is EXACTLY.
    bool exactCount = false;

    // BOSS REPLACEMENTS PER MAP, 0-15 inclusive. 0 inserts nothing anywhere and
    // the run still succeeds.
    int countPerMap = 1;

    // REPLACE ALL ENEMIES - TEMPORARY, AND BUILT TO BE DELETED (spec 013 §6).
    //
    // THE QUESTION IT EXISTS TO ANSWER: what does Bloodborne do when every one
    // of the 1,381 ordinary creatures in the fourteen areas is a boss? Nothing
    // static can answer it - not a build, not a verifier, not this comment. The
    // honest answers are "it crashes on load", "it loads and is unplayable" and
    // "it is fine", and which one it is decides how much of a warning the other
    // seven settings need.
    //
    // THE DESIGN: one boolean, tested inside this pass only, that replaces
    // every eligible target in every ticked area and makes exactCount and
    // countPerMap irrelevant. It deliberately does NOT bypass the three
    // protections above - a creature the player told the run to leave alone is
    // not replaced by this either - because a test control that disagrees with
    // the settings around it tests something nobody asked about.
    //
    // WHAT RETIRES IT: one hardware run with it on, its outcome recorded in
    // docs/features/013-bosses-replace-enemies/log.md. That is the required gate
    // before milestone 5, and milestone 5's entire content is deleting this
    // field, its config key, its settings row and the branch below. The key is
    // then simply no longer read, exactly as unchanged_bell_maidens is not.
    bool replaceAll = false;

    // SPREAD BOSSES OUT. Chooses the placements by greedy farthest-point over
    // their positions instead of by an unweighted draw. At a count of 1 it
    // produces the same placement as off and consumes the same draws.
    bool spaceOut = false;

    // BOSS BEHAVIOUR: false is OWN AI, true is REPLACED ENEMY'S AI. True is
    // implemented as NOT WRITING ThinkParamID, which is the only way the
    // reference ever expressed it (its 551000 branch assigns a local and never
    // the field - RandomizeFunctions.cs:1536-1547).
    bool keepEnemyAi = false;

    // INSERTED BOSSES, 22 rows. Filters the harvest, so an unticked model never
    // enters the pool and never arrives. Nothing ticked skips the whole pass
    // SILENTLY - no error, no warning, nothing refused at commit (spec 013
    // D-G), which is the reference's own behaviour when its list comes up empty.
    InsertedBossSelection includedBosses;

    // AREAS INCLUDED, 14 rows of map-name prefixes. An unticked area is not
    // visited at all. Nothing ticked means nothing happens and the run succeeds.
    InsertAreaSelection areas;

    // NOT SETTINGS OF THIS FEATURE, and not read from any of its config keys:
    // EnemyRandomizer fills these two from the enemy options before the pass
    // runs. They are here rather than being two more parameters so that the
    // three protections of an eligible target are stated in ONE place - see the
    // header comment - and so nobody can call the pass having honoured two of
    // the three.
    std::vector<std::string> skipPatterns;      // ENEMIES SKIPPED, flattened
    bool doNotRandomizeCagedDogs = false;
};

// The run's supply of identities, drained BY IDENTITY across the whole run
// rather than per map.
struct InsertionPool {
    // Drained as bosses are inserted; refilled from `refill` when it empties.
    std::vector<BossIdentity> entries;

    // The FULL enabled identity list, not a model-deduped one. That is what
    // makes "any two enabled identities' use counts differ by at most one" true
    // (spec 013 §8 criterion 10), and it is the one place this deliberately
    // differs from BossPool::refill, which is model-deduped. Do not reuse
    // DrainPool here: it also erases same-model entries, which breaks the same
    // criterion from the other direction.
    std::vector<BossIdentity> refill;
};

struct InsertionCounts {
    int inserted = 0;       // placements actually rewritten
    int shortfallMaps = 0;  // map files that could not supply the count asked for
};

// Harvests one map's contribution to the insertion list. Call once per map, in
// any order - the harvest is measured set-identical over the enemy pass's
// 24-file order, the reference's 16 and BossMapOrder()'s 17, which is what lets
// it ride along inside StepReadMap.
void CollectInsertionCandidates(const std::string& mapName, const MsbbFile& msbb,
                                const InsertionOptions& options, InsertionPool& pool);

// Builds `refill` from what was collected. Call once, after every map.
void FinalizeInsertionPool(InsertionPool& pool);

// The pass. Returns the number of placements rewritten in this map file, and
// adds it to `counts`. Draws nothing at all when any gate above is shut.
int ApplyBossInsertion(const std::string& mapName, MsbbFile& msbb,
                       const InsertionOptions& options, InsertionPool& pool,
                       std::mt19937& rng, InsertionCounts& counts);

} // namespace bbr
