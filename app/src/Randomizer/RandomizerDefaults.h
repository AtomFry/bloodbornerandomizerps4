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
    // enforces for every title ID, official or a user's own repackaged
    // one. CUSA03175 is the struct's own default (what a fresh install with
    // no defaults.cfg starts from) because it's the version the randomizer
    // mod is actually built around, not a placeholder - most users won't
    // need to touch this row at all. Auto-detection/selection from
    // installed titles is a later milestone - see the conversation this
    // was scoped down from.
    std::string bloodborneTitleId = "CUSA03175";

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

    // The seed the last commit actually used, so the wizard can open showing
    // it and "run that again" is two button presses. Unlike everything else
    // here this is a remembered run artifact rather than a preference, which
    // is why the Enable wizard writes it and Setup Defaults doesn't show it.
    // 0 means "never set" - the wizard rolls a value on sight of it, so the
    // field is never blank.
    uint32_t lastSeed = 0;
};

} // namespace bbr
