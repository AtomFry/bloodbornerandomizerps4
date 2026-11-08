// RandomizerDefaults.h - the baseline a NEW randomizer profile starts from
// (see docs/plans/ui-blueprint-wizards.md: "Defaults != Profile" - frozen, but
// that distinction still holds). Grows one field per Setup
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
    // Bloodborne title IDs listed in docs/ps4-homebrew-findings.md section 6,
    // and it is what the reference tool's own Nexus release targets.
    //
    // This was "CUSA03175" until 2026-09-15, which is NOT a real Bloodborne
    // title ID - a fresh install would have written its output for a title
    // that is not installed, AFR would have had nothing to overlay, and the
    // game would have launched unmodified with no error anywhere. Silent, and
    // indistinguishable from "the randomizer did nothing". If a title ID is
    // ever changed here again, check it against that list first.
    //
    // Auto-detection/selection from installed titles is a later milestone.
    std::string bloodborneTitleId = "CUSA03173";

    // The two save-data fields that used to sit here are gone. Nothing behind
    // them was ever implemented: every save-data operation was a simulated
    // progress line, so the fields only ever decided which line got printed.
    // Save-data handling is to be redesigned as separate work, and this starts
    // it from a clean slate rather than a stub (spec randomizer-settings-ui
    // 9.1b/9.3, §4.8). An older defaults.cfg still carrying their keys loads
    // fine with those lines ignored - see RandomizerDefaultsStore.cpp.

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

    // RANDOMIZE SHOP ARMOUR and RANDOMIZE SHOP ITEMS (feature 011) - the other
    // two thirds of the same shop table, each shuffled within its own kind. See
    // Randomizer/ShopStock.h.
    //
    // Independent of each other and of the three toggles above: the shop's
    // weapon, armour and consumable listings are three disjoint sets, so any
    // combination of the five is valid.
    //
    // "No" is each one's own default, so a defaults.cfg written before these
    // keys existed reads as off and the shops are the ones the game shipped -
    // the standing rule that an older defaults.cfg must not silently turn a
    // setting on.
    bool randomizeShopArmour = false;
    bool randomizeShopItems = false;

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

    // NO TEAM TYPE, shipped as ENEMIES ON SAME TEAM (feature 027).
    // Puts every NpcParam row onto the one team type (25), exactly as the
    // reference tool's TeamTypeRando() does. See TeamType.h.
    //
    // Like enableMergoDarkness above this is not a randomizer: it draws no
    // randomness and writes the same fixed byte every run, so the same seed
    // produces the same world with it on or off. false is the state in which
    // this feature leaves the item-data archive untouched, which is what makes
    // the absent-key rule safe here too - a defaults.cfg written before this
    // key existed reads as off and the pass never runs.
    //
    // The LABEL asserts an effect this project has never observed: what
    // Bloodborne does with team type 25 is unknown, and the hardware test of
    // feature 027 is what settles whether the label is true. The field and the
    // config key are therefore named for the mechanism and the backlog row
    // rather than for the label, so revising the label costs no saved file.
    bool noTeamType = false;

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

    // START WITH A TRICK WEAPON - which of the 78 right-hand trick weapon
    // versions may be granted to a new character. See
    // Randomizer/TrickWeaponGrant.h.
    //
    // There is deliberately NO accompanying boolean: the setting's whole state
    // is this pool. Nothing ticked is off, one ticked always grants that one,
    // several ticked draws one for the run - the same none/one/many semantics
    // ENEMIES INCLUDED has, so a separate toggle would be a second way to say
    // the same thing and a second thing to disagree with itself.
    //
    // Nothing ticked is the type's own default (see TrickWeaponSelection's
    // false), so a defaults.cfg written before this key existed, or one
    // carrying a stale-length line, reads as nothing granted and produces the
    // run it produced before, roll for roll.
    TrickWeaponSelection trickWeapons;

    // START WITH A LEFT-HAND WEAPON - which of the 14 left-hand weapons may be
    // granted to a new character: 11 firearms plus the Loch Shield, the Hunter's
    // Torch and the Fist of Gratia. See Randomizer/LeftHandWeaponGrant.h.
    //
    // Feature 038 is feature 037 for the other hand and shares its shape
    // exactly: no accompanying boolean, the same none/one/many semantics, and
    // nothing ticked as the type's own default, so a defaults.cfg written before
    // this key existed - or one carrying a stale-length line - reads as nothing
    // granted and produces the run it produced before, roll for roll.
    //
    // Independent of trickWeapons above. Both may be on and each draws its own
    // weapon; a character can start with one in each hand.
    LeftHandWeaponSelection leftHandWeapons;

    // START WITH HEADWEAR / BODY ARMOUR / GLOVES / LEGWEAR - which of the 124
    // wearable armour pieces may be granted to a new character, one picker per
    // slot. See Randomizer/ArmourGrant.h and Randomizer/ArmourTables.h.
    //
    // FOUR SELECTIONS AND NOT ONE, in the game's own slot order (head, body,
    // gloves, legwear), because a player wears four pieces and each slot is
    // drawn independently of the other three: changing what is ticked in one
    // must not change which piece a fixed seed draws in another (spec 040 B6).
    //
    // The same shape as the two weapon pickers above and independent of them and
    // of each other: no accompanying boolean, the same none/one/many semantics,
    // and nothing ticked as each type's own default - so a defaults.cfg written
    // before these keys existed, or one carrying a stale-length line, reads as
    // that slot's vanilla garment left alone, and the run is the one the app made
    // before this existed, roll for roll.
    HeadwearSelection   headwear;
    BodyArmourSelection bodyArmour;
    GlovesSelection     gloves;
    LegwearSelection    legwear;

    // START WITH SUPPLIES - grants 20 Blood Vials and 20 Quicksilver Bullets at
    // character creation, into free inventory slots. See
    // Randomizer/StartingSupplies.h.
    //
    // A toggle and not a picker, unlike the four above: there is nothing to
    // choose. Two consumables, one count each, and the 20 is each goods row's
    // own maxNum rather than a number anyone picked (spec 040 D2).
    //
    // "No" is the struct's own default, so a defaults.cfg written before this
    // key existed reads as off and nothing about the run changes.
    bool startWithSupplies = false;

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

    // SAVE DATA - the one setting in the world editor's SAVE category
    // (spec worlds D6). false is KEEP EXISTING and true is START FRESH.
    //
    // Unlike every other field here this one is not a preference and not a
    // randomizer: it decides what happens to SAVE DATA when the world it
    // belongs to is activated. KEEP EXISTING restores that world's own save,
    // or adopts the live one if it has none; START FRESH empties the
    // container so the game starts a new playthrough.
    //
    // "KEEP EXISTING" is the struct's own default and is ALWAYS the default
    // (spec worlds D17). A world left on START FRESH would decline to load
    // its own save on every future activation, so activation reverts it to
    // KEEP EXISTING as part of the same transaction - it applies to one
    // activation and the player chooses it deliberately every time.
    //
    // It is stored in the recipe but regeneration ignores it, so it cannot
    // affect output parity. It is editor-only: the Defaults tab does not show
    // it, because a per-world save policy is not something a new world starts
    // from.
    bool startFreshSave = false;

    // BOSSES CAN REPLACE ENEMIES and its seven modifiers (feature 013) - the
    // insertion pass that writes boss identities over ordinary enemy placements
    // in the fourteen explorable areas, as a second pass after the enemy and
    // boss passes. See Randomizer/BossInsertion.h.
    //
    // THE MASTER SWITCH IS TESTED BEFORE ANY DRAW. With it false the pass
    // returns before it reaches a RandInt, so a run is byte-identical to the one
    // the app made before this existed, roll for roll - the same discipline
    // doNotRandomizeCagedDogs keeps. That is what makes "absent -> false" safe
    // here: a defaults.cfg written before these keys existed reads as off, and
    // off is not a conservative approximation of today's run, it IS today's run.
    //
    // The other seven do nothing at all while this is false, and none of them
    // is a second way of switching the feature on.
    bool bossesCanReplaceEnemies = false;

    // BOSS REPLACEMENTS - false is UP TO, true is EXACTLY (spec 013 D-L).
    // UP TO makes the count below a ceiling a map approaches; EXACTLY makes it
    // min(count, what the map can supply) precisely. Named for the state `true`
    // means, so the field reads the way the config key does.
    bool bossReplacementsExact = false;

    // BOSS REPLACEMENTS PER MAP - how many insertions one map file gets, 0 to
    // 15 inclusive (spec 013 D-K: 15 is the last count at which spreading still
    // keeps every map's closest pair above 20 paces). 0 means none anywhere and
    // the run still succeeds.
    //
    // ONE, not zero, is the struct's own default, and unlike every bool here
    // that is not "the run the app made before this existed" on its own - the
    // master switch above is what supplies that. A count with no master switch
    // on inserts nothing.
    int bossReplacementsPerMap = 1;

    // REPLACE ALL ENEMIES - a deliberately TEMPORARY test control (spec 013 §6).
    // Replaces every eligible placement in every ticked area at once, ignoring
    // the two settings above. It exists to answer one question on hardware -
    // what 1,381 loose bosses do to the game - and is expected to crash or to
    // make the game unplayable. It is deleted once one hardware run has
    // answered, which is feature 013's milestone 5; the key is then simply no
    // longer read, exactly as unchanged_bell_maidens is not.
    bool replaceAllEnemies = false;

    // SPREAD BOSSES OUT - chooses the placements by greedy farthest-point over
    // their positions rather than by an unweighted draw, so each inserted boss
    // tends to hold its own part of a map. At a count of 1 it changes nothing
    // and consumes the same draws (spec 013 D-F, D-N).
    bool spaceOutInsertedBosses = false;

    // BOSS BEHAVIOUR - false is OWN AI, true is REPLACED ENEMY'S AI. True
    // leaves the placement's existing ThinkParamID alone, which is the only way
    // the reference tool ever expressed this (its 551000 branch assigns a local
    // and never the field). It MAY SIMPLY NOT WORK - a think id refers to
    // behaviours that belong to a particular body - and only the console can
    // say. False, the default, writes the arriving boss's own think id.
    bool insertedBossKeepsEnemyAi = false;

    // INSERTED BOSSES - which of the 22 boss models may arrive in the world.
    // NOT bossesIncluded: that list is 17 rows and governs which bosses hold
    // which arena, this one is 22 and governs which arrive as replacements, and
    // the two are separate choices about separate passes (spec 013 D-B).
    //
    // All ticked is the default, and with nothing ticked the whole pass is
    // skipped silently - no error, no warning, nothing refused at commit
    // (spec 013 D-G), which is the reference's own behaviour when its list
    // comes up empty.
    InsertedBossSelection insertedBosses;

    // AREAS INCLUDED - which of the fourteen explorable areas the pass visits.
    // Each row is a map-name prefix covering one to three map files. All ticked
    // is the default; with nothing ticked nothing happens and the run succeeds.
    // Neither m21 file is a row, so the Hunter's Dream and the Abandoned Old
    // Workshop can never take part.
    InsertAreaSelection bossAreas;

    // The seed the last commit actually used, so the wizard can open showing
    // it and "run that again" is two button presses. Unlike everything else
    // here this is a remembered run artifact rather than a preference, which
    // is why the Enable wizard writes it and Setup Defaults doesn't show it.
    // 0 means "never set" - the wizard rolls a value on sight of it, so the
    // field is never blank.
    uint32_t lastSeed = 0;
};

} // namespace bbr
