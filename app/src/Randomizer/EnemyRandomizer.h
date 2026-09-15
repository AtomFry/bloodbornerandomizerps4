// EnemyRandomizer.h - the C++ port of the reference WPF tool's "Randomize
// Enemies" feature (RandomizeFunctions.cs/StartFunctions.cs/
// MainWindow.xaml.cs, all read in full before this was written). Scoped to
// exactly what our own UI exposes - chalice dungeons and the reference's
// per-run text-file audit logging remain absent; oopsAll/excludeEnemies is
// now exposed, as the enemy picker (see EnemyPoolSelection.h) -
// see the .cpp file's header comment for the full list of deliberate
// simplifications versus the reference tool.
#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "EnemyPoolSelection.h"

namespace bbr {

struct EnemyRandomizerResult {
    bool success = false;
    std::string error;       // set when success is false
    int mapsProcessed = 0;
    int enemiesRandomized = 0;
    int npcParamsScaled = 0; // see BossParamScaling.h
    int bossesRandomized = 0;
    int treasuresRandomized = 0;

    // Item-data archive diagnostics, populated whenever the archive is
    // rewritten. Useful for telling a size problem apart from a logic problem
    // if the game misbehaves.
    int itemDataMembers = 0;
    size_t itemDataPlainBytes = 0;
    size_t itemDataWrittenBytes = 0;
    int dropsRandomized = 0;
    int dropPoolSize = 0;
    int startingMeleeChanged = 0;
    int startingGunsChanged = 0;
    int shopWeaponsChanged = 0;
};

// What a run should actually do. Both may be off (the job then just mirrors
// the vanilla tree and applies the perma-darkness poke, which is exactly the
// verified-good behavior this port already had).
struct EnemyRandomizerOptions {
    bool randomizeEnemies = false;
    bool randomizeBosses  = false;
    bool randomizeTreasure = false;

    // These four live in the game's item-data archive (gameparam.parambnd.dcx)
    // rather than in map files, so enabling any of them makes the app rewrite
    // that archive - see docs/plans/param-features.md. The archive is loaded and
    // written once no matter how many are on.
    bool randomizeEnemyDrops = false;

    // The three Hunter's Dream melee choices, the two firearm choices, and the
    // Messengers' weapon stock - independent by design, unlike the reference
    // tool's single toggle. See docs/plans/starting-weapons.md.
    bool randomizeStartingWeapons = false;
    bool randomizeStartingGuns = false;
    bool randomizeShopWeapons = false;

    bool AnyParamFeature() const {
        return randomizeEnemyDrops || randomizeStartingWeapons ||
               randomizeStartingGuns || randomizeShopWeapons;
    }

    // The reference's "Randomize Workshop Tools" setting: when false, the two
    // workshop tool item lots are protected from treasure randomization. No UI
    // for it yet, so it matches the reference's unchecked-by-default state.
    bool randomizeWorkshopTools = false;

    // Which enemy models RANDOMIZE ENEMIES may draw as replacements. All
    // enabled by default, which is exactly the behaviour that existed before
    // the picker - so every existing call site is unaffected by adding this.
    //
    // Only filters the POOL. Placements of a disabled enemy are still
    // randomized like any other; disabling one does not pin it in place.
    // See docs/plans/pickers.md.
    EnemyPoolSelection enemiesIncluded;

    // The same, for the boss pool. Note that pool is drained and refilled
    // as arenas are assigned, so a one-model selection means every boss
    // arena gets that model - see BossRandomizer.cpp's DrainPool.
    BossPoolSelection bossesIncluded;

    // Cuts the scripted darkness in Mergo's Loft by disarming one instruction
    // in event/common.emevd.dcx - see PermaDarkness.h and
    // docs/plans/mergo-darkness.md. The only option here that isn't a
    // randomizer: it draws no randomness and the same seed produces the same
    // world with it on or off.
    //
    // false leaves the mirrored vanilla file untouched, which is why it is
    // safe as the default. Note this is the opposite polarity to the
    // reference tool's checkbox, which is named for the resulting state
    // rather than the action.
    bool enableMergoDarkness = false;
};

// A full run takes 10-20 seconds, which is far too long to spend inside one
// Screen::Update() call: Application.cpp's frame loop is strictly serial
// (PumpEvents -> Update -> Draw -> Present), so nothing can be drawn while
// Update is running and the UI simply freezes until the work finishes.
//
// So the run is exposed as a resumable job instead. Step() performs one
// coarse unit of work and returns, letting the caller present a frame
// between units. Steps are deliberately coarse - one mirrored folder, one
// map - because Present() typically blocks on vsync, so every step costs a
// frame; ~56 steps adds under a second of frame time, while per-enemy
// stepping would add real seconds for progress nobody can read anyway.
//
// Reads vanilla .msb.dcx map files from `vanillaDvdrootDir` (expects the
// map/mapstudio/*.msb.dcx layout, i.e. pass the dvdroot_ps4 root), applies
// enemy randomization with the given seed, and writes the resulting
// .msb.dcx files under `outputDvdrootDir`/map/mapstudio/, having first
// mirrored the six folders AFR needs. The vanilla tree is only ever read.
// Named for what it started as; it now drives boss randomization too (see
// BossRandomizer.h). Left renamed-not-at-all deliberately - a rename would
// touch every call site for no behavioral gain.
class EnemyRandomizerJob {
public:
    EnemyRandomizerJob(const std::string& vanillaDvdrootDir,
                       const std::string& outputDvdrootDir,
                       uint32_t seed,
                       const EnemyRandomizerOptions& options);
    ~EnemyRandomizerJob();

    EnemyRandomizerJob(const EnemyRandomizerJob&) = delete;
    EnemyRandomizerJob& operator=(const EnemyRandomizerJob&) = delete;

    // Performs one unit of work. Does nothing once Done().
    void Step();
    bool Done() const;

    // What the job is working on right now, for on-screen display - e.g.
    // "COPYING SFX", "RANDOMIZING MAP 7/24". Never empty.
    std::string StatusText() const;

    // 0.0-1.0, for a progress bar. The denominator is refined once the real
    // map count is known, so it can shift slightly partway through a run.
    float Progress() const;

    // Only meaningful once Done().
    const EnemyRandomizerResult& Result() const;

private:
    struct State;
    std::unique_ptr<State> s_; // destructor defined in the .cpp - State is incomplete here
};

} // namespace bbr
