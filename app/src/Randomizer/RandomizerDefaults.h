// RandomizerDefaults.h - the baseline a NEW randomizer profile starts from
// (see UI_BLUEPRINT.md: "Defaults != Profile"). Grows one field per Setup
// Defaults row as that screen grows.
#pragma once

#include <cstdint>
#include <string>

#include "EnemyPoolSelection.h"

namespace bbr {

struct RandomizerDefaults {
    // Manually entered by the user (Setup Defaults screen) - always
    // "AAAA00000" shaped (4 letters, 5 digits), same format the PS4 itself
    // enforces for every title ID, official or a user's own repackaged one.
    //
    // CUSA03173 (Europe / GOTY) is the struct's own default - what a fresh
    // install with no defaults.cfg starts from. It is one of the six real
    // Bloodborne title IDs listed in Game/GameInfo.cpp, and it is what the
    // reference tool's own Nexus release targets.
    //
    // This was "CUSA03175" until 2026-09-15, which is NOT a real Bloodborne
    // title ID - a fresh install would have written its output for a title
    // that is not installed, AFR would have had nothing to overlay, and the
    // game would have launched unmodified with no error anywhere. Silent, and
    // indistinguishable from "the randomizer did nothing". If a title ID is
    // ever changed here again, check it against GameInfo.cpp's list first.
    //
    // Auto-detection/selection from installed titles is a later milestone.
    std::string bloodborneTitleId = "CUSA03173";

    // "Yes" is the struct's own default, not just the store's fallback -
    // this is what a brand-new install (no defaults.cfg on disk yet) uses.
    bool backupExistingSaveData = true;

    // Pre-fills the Enable wizard's Screen 1 "Replace Save" choice. Only
    // two options, deliberately: "select from backup" isn't a sensible
    // *default* the way it's a sensible one-off choice - a default is what
    // a brand-new Enable run starts from before any backups necessarily
    // exist yet. true = NEW SAVE DATA (the struct's own default), false =
    // LEAVE EXISTING SAVE DATA.
    bool replaceSaveDefaultIsNew = true;

    // First real randomizer setting - deliberately minimal (see the
    // conversation this was scoped down from): the goal is one working
    // setting through the whole pipeline before fleshing out the rest.
    // "No" is the struct's own default (fresh install, or the key missing
    // from an older defaults.cfg written before this field existed).
    bool randomizeEnemies = false;

    // Second real randomizer setting. Independent of randomizeEnemies - either,
    // both, or neither may be on. "No" is the struct's own default, so an
    // existing defaults.cfg written before this field existed keeps boss
    // randomization off rather than silently enabling it.
    bool randomizeBosses = false;

    // Third real randomizer setting - world pickups (treasure). Independent of
    // the other two. "No" is the struct's own default, so an existing
    // defaults.cfg written before this field existed keeps it off.
    bool randomizeTreasure = false;

    // Puts the two workshop tool key items (blood gem, runes) into the
    // treasure shuffle instead of leaving them on the protected list. Only
    // meaningful when randomizeTreasure is also on - see
    // docs/plans/workshop-tools.md D3. "No" is the struct's own
    // default, matching the reference and every other field's "an older
    // defaults.cfg must not silently turn this on" rule.
    bool randomizeWorkshopTools = false;

    // Fourth randomizer setting - enemy drops. The first one that requires
    // rewriting the game's item-data archive rather than map files.
    bool randomizeEnemyDrops = false;

    // The Hunter's Dream weapon choices, split into three independent
    // toggles - see docs/plans/starting-weapons.md for why this differs from
    // the reference tool's single confusingly-named setting.
    bool randomizeStartingWeapons = false;
    bool randomizeStartingGuns = false;
    bool randomizeShopWeapons = false;

    // Which enemies may be used as replacements by RANDOMIZE ENEMIES. All
    // enabled is the default and is exactly the behaviour that existed before
    // the picker, so an older defaults.cfg is unaffected. See
    // docs/plans/pickers.md.
    EnemyPoolSelection enemiesIncluded;

    // The same, for RANDOMIZE BOSSES. Independent of the enemy list.
    BossPoolSelection bossesIncluded;

    // Which enemies RANDOMIZE ENEMIES must leave alone - the opposite of
    // enemiesIncluded in both directions, and nothing ticked by default. An
    // absent or wrong-length enemies_skipped line therefore leaves NOTHING
    // skipped, which is the same run the app made before this existed.
    //
    // This replaced UNCHANGED BELL MAIDENS, and the old saved value is
    // deliberately NOT migrated (feature 032 D1): a defaults.cfg still
    // carrying unchanged_bell_maidens=1 loads fine with that key ignored, and
    // the maidens randomize again until the two chime maiden rows are ticked
    // here. That loss is intended and documented.
    EnemySkipSelection enemiesSkipped;

    // Cuts the scripted darkness in Mergo's Loft - see
    // docs/plans/mergo-darkness.md. Unlike every other field here this is
    // not a randomizer: it is a single fixed edit to event/common.emevd.dcx,
    // and it applies whether or not anything else is on.
    //
    // "No" is the struct's own default, and here that genuinely means "do
    // nothing" rather than "do the conservative thing" - false is the state in
    // which the app leaves the file exactly as the game shipped it. That is
    // what makes the usual absent-key rule safe: a defaults.cfg written before
    // this key existed reads as false and the app touches nothing.
    //
    // Note this reverses what the port did before the setting existed, which
    // cut the darkness unconditionally (D3 in the plan).
    bool enableMergoDarkness = false;

    // DO NOT RANDOMIZE CAGED DOGS - a modifier on randomizeEnemies that pins
    // the ten caged-dog spots of the Central Yharnam kennel yard and the
    // Forbidden Woods cluster, so whatever else the run does those cages keep
    // what the game put in them. It is a PLACEMENT protection, not a creature
    // one: the creature still arrives elsewhere and still feeds the pool
    // (spec 033 D6), which is what makes it a different thing from
    // enemiesSkipped. See Randomizer/CagedDogList.h.
    //
    // "No" is the struct's own default (spec 033 D2), so a defaults.cfg
    // written before this field existed reads as off and the run is the one
    // the app made before this existed, roll for roll.
    bool doNotRandomizeCagedDogs = false;

    // START WITH HUNTER TOOLS - grants the Blood Gem and Rune Workshop Tools
    // at character creation so gems and runes are usable from the first area
    // rather than sitting unusable in the inventory until their chests turn
    // up. See Randomizer/HunterTools.h.
    //
    // Distinct from randomizeWorkshopTools above, which shuffles those same
    // two items into the treasure pool. They are independent and either,
    // both, or neither may be on.
    //
    // "No" is the struct's own default, so a defaults.cfg written before this
    // key existed reads as off and nothing about the run changes.
    bool startWithHunterTools = false;

    // EASY SHADOWS, EASY ROM, EASY FAILURES, EASY EMISSARY - four
    // independent settings, one per multi-body boss arena, each replacing
    // that fight's duplicate bodies with the tiny Iosefka's Clinic larva so
    // the fight plays as a duel. Any combination is valid, including all
    // four with every randomizer off. See Randomizer/EasyModes.h.
    //
    // Not randomizers: they draw no randomness and the same seed gives the
    // same world with them on or off, apart from the affected placements.
    //
    // "No" is each one's own default, so a defaults.cfg written before these
    // keys existed reads as all four off and nothing about the run changes.
    bool easyShadows = false;
    bool easyRom = false;
    bool easyFailures = false;
    bool easyEmissary = false;

    // The seed the last commit actually used, so the wizard can open showing
    // it and "run that again" is two button presses. Unlike everything else
    // here this is a remembered run artifact rather than a preference, which
    // is why the Enable wizard writes it and Setup Defaults doesn't show it.
    // 0 means "never set" - the wizard rolls a value on sight of it, so the
    // field is never blank.
    uint32_t lastSeed = 0;
};

} // namespace bbr
