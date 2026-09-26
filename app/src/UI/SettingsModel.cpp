#include "SettingsModel.h"

namespace bbr {

namespace {

// Every setting, in spec 7.1's category order and, within a category, in that
// table's order. THIS ORDER IS THE DISPLAY ORDER - there is no second list and
// no row index, so an entry moved here moves on screen and nothing else has to
// be told about it.
//
// Two placements are deliberate and must not be "corrected" (spec 7.1): the
// four easy modes go in DIFFICULTY rather than BOSSES, because they are fixed
// edits rather than randomizers; and START WITH HUNTER TOOLS is kept in a
// different category from RANDOMIZE WORKSHOP TOOLS, because the two act on the
// same two items, are independent, and are constantly confused.
//
// Labels are the strings these rows already shipped with (plan 9 D2). The
// spec's prose uses shorter names - "Protect Caged Dogs" for the row that says
// DO NOT RANDOMIZE CAGED DOGS - and those are prose, not label changes.
//
// Help is spec Appendix A verbatim, with "-" for the dash and no italic
// markers: the atlas covers printable ASCII (32..126), so an em dash would
// draw as nothing at all.
const SettingDef kSettings[] = {
    // --- Enemies -----------------------------------------------------------
    { SettingId::RandomizeEnemies, SettingCategory::Enemies, SettingKind::Toggle,
      "RANDOMIZE ENEMIES", &RandomizerDefaults::randomizeEnemies,
      "Randomizes enemy placements throughout the world. Bosses are "
      "unaffected." },
    { SettingId::EnemiesIncluded, SettingCategory::Enemies, SettingKind::EnemyPool,
      "ENEMIES INCLUDED", nullptr,
      "Choose which enemies can appear as replacements." },
    { SettingId::EnemiesSkipped, SettingCategory::Enemies, SettingKind::EnemySkip,
      "ENEMIES SKIPPED", nullptr,
      "Choose enemies that keep their original placements. They may still "
      "appear as replacements elsewhere." },
    { SettingId::DoNotRandomizeCagedDogs, SettingCategory::Enemies, SettingKind::Toggle,
      "DO NOT RANDOMIZE CAGED DOGS", &RandomizerDefaults::doNotRandomizeCagedDogs,
      "Keeps the caged dogs of Central Yharnam and the Forbidden Woods in "
      "their original placements. Other enemies behave badly in these cages." },

    // --- Bosses ------------------------------------------------------------
    { SettingId::RandomizeBosses, SettingCategory::Bosses, SettingKind::Toggle,
      "RANDOMIZE BOSSES", &RandomizerDefaults::randomizeBosses,
      "Randomizes boss placements throughout the world." },
    { SettingId::BossesIncluded, SettingCategory::Bosses, SettingKind::BossPool,
      "BOSSES INCLUDED", nullptr,
      "Choose which bosses can appear as replacements." },

    // --- Items and Treasure ------------------------------------------------
    { SettingId::RandomizeTreasure, SettingCategory::ItemsTreasure, SettingKind::Toggle,
      "RANDOMIZE TREASURE", &RandomizerDefaults::randomizeTreasure,
      "Randomizes items found throughout the world." },
    { SettingId::RandomizeWorkshopTools, SettingCategory::ItemsTreasure, SettingKind::Toggle,
      "RANDOMIZE WORKSHOP TOOLS", &RandomizerDefaults::randomizeWorkshopTools,
      "Adds the Blood Gem and Rune Workshop Tools to the treasure pool. "
      "Requires Randomize Treasure." },
    { SettingId::RandomizeEnemyDrops, SettingCategory::ItemsTreasure, SettingKind::Toggle,
      "RANDOMIZE ENEMY DROPS", &RandomizerDefaults::randomizeEnemyDrops,
      "Randomizes what enemies drop when killed." },

    // --- Weapons and Starting Gear -----------------------------------------
    { SettingId::RandomizeStartingWeapons, SettingCategory::WeaponsGear, SettingKind::Toggle,
      "RANDOMIZE STARTING WEAPONS", &RandomizerDefaults::randomizeStartingWeapons,
      "Randomizes the trick weapons offered in the Hunter's Dream." },
    { SettingId::RandomizeStartingGuns, SettingCategory::WeaponsGear, SettingKind::Toggle,
      "RANDOMIZE STARTING GUNS", &RandomizerDefaults::randomizeStartingGuns,
      "Randomizes the firearms offered in the Hunter's Dream." },
    { SettingId::RandomizeShopWeapons, SettingCategory::WeaponsGear, SettingKind::Toggle,
      "RANDOMIZE SHOP WEAPONS", &RandomizerDefaults::randomizeShopWeapons,
      "Randomizes the weapons sold by the Bath Messengers." },
    { SettingId::StartWithHunterTools, SettingCategory::WeaponsGear, SettingKind::Toggle,
      "START WITH HUNTER TOOLS", &RandomizerDefaults::startWithHunterTools,
      "Start with the Blood Gem and Rune Workshop Tools. Gems and runes can be "
      "fitted from the first area." },

    // --- Difficulty --------------------------------------------------------
    { SettingId::EasyShadows, SettingCategory::Difficulty, SettingKind::Toggle,
      "EASY SHADOWS", &RandomizerDefaults::easyShadows,
      "Replaces the duplicate Shadows of Yharnam with harmless larvae, so the "
      "fight is a duel." },
    { SettingId::EasyRom, SettingCategory::Difficulty, SettingKind::Toggle,
      "EASY ROM", &RandomizerDefaults::easyRom,
      "Replaces Rom's attendant spiders with harmless larvae." },
    { SettingId::EasyFailures, SettingCategory::Difficulty, SettingKind::Toggle,
      "EASY FAILURES", &RandomizerDefaults::easyFailures,
      "Replaces the duplicate Living Failures with harmless larvae." },
    { SettingId::EasyEmissary, SettingCategory::Difficulty, SettingKind::Toggle,
      "EASY EMISSARY", &RandomizerDefaults::easyEmissary,
      "Replaces the Celestial Emissary's lesser emissaries with harmless "
      "larvae." },

    // --- World -------------------------------------------------
    { SettingId::EnableMergoDarkness, SettingCategory::World, SettingKind::Toggle,
      "ENABLE MERGO DARKNESS", &RandomizerDefaults::enableMergoDarkness,
      "Turns the Wet Nurse's arena darkness on for the whole game, from your "
      "first spawn." },

    // --- Save --------------------------------------------------------------
    //
    // The one entry the DEFAULTS tab must never show (worlds plan 3.3). It is
    // not a randomizer and not a preference: it decides what happens to the
    // save container when THIS world is activated, which is a per-world
    // question. The Defaults screen iterates a list of six categories that
    // does not contain this one.
    { SettingId::SaveData, SettingCategory::Save, SettingKind::SaveChoice,
      "SAVE DATA", &RandomizerDefaults::startFreshSave,
      "Keep Existing restores this world's save, or uses your current save if "
      "it has none. Start Fresh begins a new playthrough, and applies to this "
      "activation only. Your save is always backed up first." },
};

const int kSettingCount = (int)(sizeof(kSettings) / sizeof(kSettings[0]));

// Rail labels for the categories, in enum order. SAVE is last and is drawn
// only by the world editor - see the enum's own note. The ampersand rather than
// "AND" (plan P8): the atlas draws printable ASCII, so the glyph exists. The
// Font8x8 fallback has no such glyph, which is why pool_verify.py's renderable()
// budget covers the strings that path still has to draw and not these.
const char* const kCategoryLabels[] = {
    "ENEMIES",
    "BOSSES",
    "ITEMS & TREASURE",
    "WEAPONS & STARTING GEAR",
    "DIFFICULTY",
    "WORLD",
    "SAVE",
};

const char* const kSeedHelp =
    "Determines this world's randomization. The same seed and settings always "
    "produce the same world.";

const char* const kTitleIdHelp =
    "The PS4 title ID of the Bloodborne installation the randomizer writes to. "
    "CUSA03173 is the Europe and Game of the Year release.";

// FINISH is gone with the Enable wizard's rail: the world editor activates on
// OPTIONS from anywhere on the screen, so there is no row to describe. NAME
// and HISTORY are its two replacements (worlds plan 4.5).
const char* const kNameHelp =
    "What this world is called. Up to 16 characters. Renaming does not create "
    "a new revision.";

const char* const kHistoryHelp =
    "Every set of settings this world has had, newest first. Choosing an older "
    "one makes it current, so the list only grows.";

} // namespace

int SettingCount() { return kSettingCount; }

const SettingDef& SettingAt(int index) {
    if (index < 0) index = 0;
    if (index >= kSettingCount) index = kSettingCount - 1;
    return kSettings[index];
}

const char* CategoryLabel(SettingCategory category) {
    int i = (int)category;
    if (i < 0 || i >= (int)SettingCategory::Count) return "";
    return kCategoryLabels[i];
}

int CategorySize(SettingCategory category) {
    int n = 0;
    for (int i = 0; i < kSettingCount; i++) {
        if (kSettings[i].category == category) n++;
    }
    return n;
}

// Linear rather than an index built at startup: nineteen entries scanned once per
// draw is nothing beside the glyph blits the same frame costs, and a prebuilt
// index would be a second structure to keep in step - the exact thing this
// file exists to remove.
const SettingDef& SettingInCategory(SettingCategory category, int index) {
    int n = 0;
    for (int i = 0; i < kSettingCount; i++) {
        if (kSettings[i].category != category) continue;
        if (n == index) return kSettings[i];
        n++;
    }
    return kSettings[0]; // out of range: the caller clamped its cursor wrong
}

std::string SettingValueText(const SettingDef& def, const RandomizerDefaults& values) {
    switch (def.kind) {
        case SettingKind::Toggle:
            return (values.*(def.flag)) ? "YES" : "NO";
        // Named states rather than YES/NO. "SAVE DATA   YES" would not say
        // which of the two things it means, and the one it would be read as -
        // "yes, keep my save" - is the opposite of what true stores.
        case SettingKind::SaveChoice:
            return (values.*(def.flag)) ? "START FRESH" : "KEEP EXISTING";
        case SettingKind::EnemyPool:
            return std::to_string(values.enemiesIncluded.CountEnabled()) + " OF " +
                   std::to_string(kEnemyPoolModelCount);
        case SettingKind::EnemySkip:
            return std::to_string(values.enemiesSkipped.CountEnabled()) + " OF " +
                   std::to_string(kEnemySkipModelCount);
        case SettingKind::BossPool:
            return std::to_string(values.bossesIncluded.CountEnabled()) + " OF " +
                   std::to_string(kBossPoolModelCount);
    }
    return "";
}

// THE ONLY WRITER of a setting. Everything that used to flip a boolean - the
// left/right chain and the X chain, once about 190 lines between them - comes
// through here now, which is what makes "X never toggles" a property of one
// function rather than a rule every branch had to remember.
void AdjustSetting(const SettingDef& def, RandomizerDefaults& values, int direction) {
    (void)direction; // two states: left and right both flip them
    bool twoState = (def.kind == SettingKind::Toggle ||
                     def.kind == SettingKind::SaveChoice);
    if (!twoState || def.flag == nullptr) return;
    values.*(def.flag) = !(values.*(def.flag));
}

// The three picker kinds, and only those. SaveChoice is edited in place by
// Left/Right exactly as a toggle is, so X must do nothing on it either.
bool IsDrillIn(const SettingDef& def) {
    return def.kind != SettingKind::Toggle && def.kind != SettingKind::SaveChoice;
}

// The four Selection* functions are the one place the three picker types are
// told apart. They need a switch rather than a pointer-to-member because
// EnemyPoolSelection, EnemySkipSelection and BossPoolSelection are three
// distinct types, and the polarity of the skip list lives in its type.
bool* SelectionFlags(const SettingDef& def, RandomizerDefaults& values) {
    switch (def.kind) {
        case SettingKind::EnemyPool: return values.enemiesIncluded.enabled;
        case SettingKind::EnemySkip: return values.enemiesSkipped.enabled;
        case SettingKind::BossPool:  return values.bossesIncluded.enabled;
        case SettingKind::Toggle:     break;
        case SettingKind::SaveChoice: break;
    }
    return nullptr;
}

const ModelPoolEntry* SelectionTable(const SettingDef& def) {
    switch (def.kind) {
        case SettingKind::EnemyPool: return EnemyPoolTable().data();
        case SettingKind::EnemySkip: return EnemySkipTable().data();
        case SettingKind::BossPool:  return BossPoolTable().data();
        case SettingKind::Toggle:     break;
        case SettingKind::SaveChoice: break;
    }
    return nullptr;
}

int SelectionCount(const SettingDef& def) {
    switch (def.kind) {
        case SettingKind::EnemyPool: return kEnemyPoolModelCount;
        case SettingKind::EnemySkip: return kEnemySkipModelCount;
        case SettingKind::BossPool:  return kBossPoolModelCount;
        case SettingKind::Toggle:     break;
        case SettingKind::SaveChoice: break;
    }
    return 0;
}

const PickerStrings& SelectionStrings(const SettingDef& def) {
    switch (def.kind) {
        case SettingKind::EnemySkip: return kEnemiesSkippedStrings;
        case SettingKind::BossPool:  return kBossesIncludedStrings;
        case SettingKind::EnemyPool:  break;
        case SettingKind::Toggle:     break;
        case SettingKind::SaveChoice: break;
    }
    return kEnemiesIncludedStrings;
}

int ToggleCount() {
    int n = 0;
    for (int i = 0; i < kSettingCount; i++) {
        if (kSettings[i].kind == SettingKind::Toggle) n++;
    }
    return n;
}

int EnabledToggleCount(const RandomizerDefaults& values) {
    int n = 0;
    for (int i = 0; i < kSettingCount; i++) {
        const SettingDef& def = kSettings[i];
        if (def.kind == SettingKind::Toggle && def.flag && (values.*(def.flag))) n++;
    }
    return n;
}

const char* SeedHelp()    { return kSeedHelp; }
const char* TitleIdHelp() { return kTitleIdHelp; }
const char* NameHelp()    { return kNameHelp; }
const char* HistoryHelp() { return kHistoryHelp; }

} // namespace bbr
