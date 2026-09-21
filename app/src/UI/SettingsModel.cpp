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
      "Replaces each enemy placement in the world with another creature drawn "
      "from the enemy pool. Does not affect bosses. Default: Off." },
    { SettingId::EnemiesIncluded, SettingCategory::Enemies, SettingKind::EnemyPool,
      "ENEMIES INCLUDED", nullptr,
      "Chooses which creatures may be used as replacements. Everything is "
      "included by default. Has no effect unless Randomize Enemies is on." },
    { SettingId::EnemiesSkipped, SettingCategory::Enemies, SettingKind::EnemySkip,
      "ENEMIES SKIPPED", nullptr,
      "Chooses which enemies are left exactly as the game placed them. Nothing "
      "is skipped by default. A skipped enemy can still appear elsewhere as a "
      "replacement. Has no effect unless Randomize Enemies is on." },
    { SettingId::DoNotRandomizeCagedDogs, SettingCategory::Enemies, SettingKind::Toggle,
      "DO NOT RANDOMIZE CAGED DOGS", &RandomizerDefaults::doNotRandomizeCagedDogs,
      "Leaves the caged dogs of Central Yharnam and the Forbidden Woods alone. "
      "This protects the ten cage placements, not the creature - Shaggy Hunting "
      "Dogs still appear elsewhere and still feed the pool. Replacements dropped "
      "into the Central Yharnam cages misbehave badly. Default: Off." },

    // --- Bosses ------------------------------------------------------------
    { SettingId::RandomizeBosses, SettingCategory::Bosses, SettingKind::Toggle,
      "RANDOMIZE BOSSES", &RandomizerDefaults::randomizeBosses,
      "Replaces each boss with another boss. Independent of Randomize Enemies - "
      "either, both or neither may be on. Default: Off." },
    { SettingId::BossesIncluded, SettingCategory::Bosses, SettingKind::BossPool,
      "BOSSES INCLUDED", nullptr,
      "Chooses which bosses may be used as replacements. Everything is included "
      "by default. Has no effect unless Randomize Bosses is on." },

    // --- Items and Treasure ------------------------------------------------
    { SettingId::RandomizeTreasure, SettingCategory::ItemsTreasure, SettingKind::Toggle,
      "RANDOMIZE TREASURE", &RandomizerDefaults::randomizeTreasure,
      "Shuffles the items found lying in the world. Default: Off." },
    { SettingId::RandomizeWorkshopTools, SettingCategory::ItemsTreasure, SettingKind::Toggle,
      "RANDOMIZE WORKSHOP TOOLS", &RandomizerDefaults::randomizeWorkshopTools,
      "Adds the two workshop tools - the Blood Gem and Rune workshop tools - to "
      "the treasure shuffle instead of leaving them where the game put them. "
      "Only meaningful when Randomize Treasure is also on. Different from Start "
      "With Hunter Tools, which grants them outright. Default: Off." },
    { SettingId::RandomizeEnemyDrops, SettingCategory::ItemsTreasure, SettingKind::Toggle,
      "RANDOMIZE ENEMY DROPS", &RandomizerDefaults::randomizeEnemyDrops,
      "Shuffles what enemies drop when killed. Default: Off." },

    // --- Weapons and Starting Gear -----------------------------------------
    { SettingId::RandomizeStartingWeapons, SettingCategory::WeaponsGear, SettingKind::Toggle,
      "RANDOMIZE STARTING WEAPONS", &RandomizerDefaults::randomizeStartingWeapons,
      "Randomizes the trick weapon choices offered in the Hunter's Dream. "
      "Default: Off." },
    { SettingId::RandomizeStartingGuns, SettingCategory::WeaponsGear, SettingKind::Toggle,
      "RANDOMIZE STARTING GUNS", &RandomizerDefaults::randomizeStartingGuns,
      "Randomizes the firearm choices offered in the Hunter's Dream. Independent "
      "of the trick weapon setting. Default: Off." },
    { SettingId::RandomizeShopWeapons, SettingCategory::WeaponsGear, SettingKind::Toggle,
      "RANDOMIZE SHOP WEAPONS", &RandomizerDefaults::randomizeShopWeapons,
      "Randomizes the weapons sold by the Bath Messengers. Default: Off." },
    { SettingId::StartWithHunterTools, SettingCategory::WeaponsGear, SettingKind::Toggle,
      "START WITH HUNTER TOOLS", &RandomizerDefaults::startWithHunterTools,
      "Grants the Blood Gem and Rune workshop tools at character creation, so "
      "gems and runes work from the first area instead of sitting unusable until "
      "their chests turn up. Different from Randomize Workshop Tools, which "
      "shuffles them into the treasure pool. Default: Off." },

    // --- Difficulty --------------------------------------------------------
    { SettingId::EasyShadows, SettingCategory::Difficulty, SettingKind::Toggle,
      "EASY SHADOWS", &RandomizerDefaults::easyShadows,
      "Replaces the duplicate bodies in the Shadows of Yharnam fight with "
      "harmless larvae, so it plays as a duel. Not a randomizer - the same seed "
      "gives the same world either way. Default: Off." },
    { SettingId::EasyRom, SettingCategory::Difficulty, SettingKind::Toggle,
      "EASY ROM", &RandomizerDefaults::easyRom,
      "The same, for Rom's attendant spiders. Default: Off." },
    { SettingId::EasyFailures, SettingCategory::Difficulty, SettingKind::Toggle,
      "EASY FAILURES", &RandomizerDefaults::easyFailures,
      "The same, for the Living Failures. Default: Off." },
    { SettingId::EasyEmissary, SettingCategory::Difficulty, SettingKind::Toggle,
      "EASY EMISSARY", &RandomizerDefaults::easyEmissary,
      "The same, for the Celestial Emissary's lesser emissaries. Default: Off." },

    // --- World -------------------------------------------------
    { SettingId::EnableMergoDarkness, SettingCategory::World, SettingKind::Toggle,
      "ENABLE MERGO DARKNESS", &RandomizerDefaults::enableMergoDarkness,
      "Cuts the scripted darkness in Mergo's Loft. Not a randomizer - it is a "
      "single fixed edit, and it applies whether or not anything else is on. Off "
      "leaves the area exactly as the game shipped it. Default: Off." },
};

const int kSettingCount = (int)(sizeof(kSettings) / sizeof(kSettings[0]));

// Rail labels for the six categories, in enum order. The ampersand rather than
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
};

const char* const kSeedHelp =
    "The number that determines this run's randomization. The same seed with "
    "the same settings always produces the same world. Left/Right rolls a new "
    "one; X edits it digit by digit.";

const char* const kTitleIdHelp =
    "The PS4 title ID of the Bloodborne installation the randomizer writes for. "
    "Four letters and five digits, the shape the console itself uses. X edits it "
    "one character at a time. Default: CUSA03173, the Europe and Game of the "
    "Year release.";

const char* const kFinishHelp =
    "Review the run and start randomizing. Shows the seed, the target title ID, "
    "and how many settings are enabled.";

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

// Linear rather than an index built at startup: 18 entries scanned once per
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
    (void)direction; // a toggle has two states: left and right both flip it
    if (def.kind != SettingKind::Toggle || def.flag == nullptr) return;
    values.*(def.flag) = !(values.*(def.flag));
}

bool IsDrillIn(const SettingDef& def) { return def.kind != SettingKind::Toggle; }

// The four Selection* functions are the one place the three picker types are
// told apart. They need a switch rather than a pointer-to-member because
// EnemyPoolSelection, EnemySkipSelection and BossPoolSelection are three
// distinct types, and the polarity of the skip list lives in its type.
bool* SelectionFlags(const SettingDef& def, RandomizerDefaults& values) {
    switch (def.kind) {
        case SettingKind::EnemyPool: return values.enemiesIncluded.enabled;
        case SettingKind::EnemySkip: return values.enemiesSkipped.enabled;
        case SettingKind::BossPool:  return values.bossesIncluded.enabled;
        case SettingKind::Toggle:    break;
    }
    return nullptr;
}

const ModelPoolEntry* SelectionTable(const SettingDef& def) {
    switch (def.kind) {
        case SettingKind::EnemyPool: return EnemyPoolTable().data();
        case SettingKind::EnemySkip: return EnemySkipTable().data();
        case SettingKind::BossPool:  return BossPoolTable().data();
        case SettingKind::Toggle:    break;
    }
    return nullptr;
}

int SelectionCount(const SettingDef& def) {
    switch (def.kind) {
        case SettingKind::EnemyPool: return kEnemyPoolModelCount;
        case SettingKind::EnemySkip: return kEnemySkipModelCount;
        case SettingKind::BossPool:  return kBossPoolModelCount;
        case SettingKind::Toggle:    break;
    }
    return 0;
}

const PickerStrings& SelectionStrings(const SettingDef& def) {
    switch (def.kind) {
        case SettingKind::EnemySkip: return kEnemiesSkippedStrings;
        case SettingKind::BossPool:  return kBossesIncludedStrings;
        case SettingKind::EnemyPool: break;
        case SettingKind::Toggle:    break;
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
const char* FinishHelp()  { return kFinishHelp; }

} // namespace bbr
