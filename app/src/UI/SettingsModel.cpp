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
    // Feature 013 - eight rows, in the order the spec's section 2 table puts
    // them: the master switch, then how many and how, then where, then which,
    // then the two that shape what arrives. Declared after BOSSES INCLUDED, so
    // the Bosses pane goes from 2 rows to 10 and becomes the first settings
    // pane in the app that has to scroll.
    //
    // Three rows carry an explicit warning in their help text (spec 013 D-O).
    // The reference tool interrupts the user with a modal on its one checkbox;
    // this app has no modal for any setting and is not growing one, so the
    // warning lives in the help text of each row that carries a distinct risk -
    // instability, a test control expected to break the game, and a state that
    // may not work at all. Do not soften these into labels.
    { SettingId::BossesCanReplaceEnemies, SettingCategory::Bosses, SettingKind::Toggle,
      "BOSSES CAN REPLACE ENEMIES", &RandomizerDefaults::bossesCanReplaceEnemies,
      "Lets bosses stand loose in the world in place of ordinary creatures, "
      "with no fog gate, no health bar and no music. The seven settings below "
      "do nothing while this is off. This may make the game unstable or cause "
      "crashes." },
    { SettingId::BossReplacements, SettingCategory::Bosses, SettingKind::NamedChoice,
      "BOSS REPLACEMENTS", &RandomizerDefaults::bossReplacementsExact,
      "Up To treats the number below as a ceiling each map approaches, so some "
      "areas get their full share and others get one or none. Exactly gives "
      "every area that number. How many bosses you tick changes which ones "
      "arrive, never how many.",
      nullptr, 0, 0, "UP TO", "EXACTLY" },
    { SettingId::BossReplacementsPerMap, SettingCategory::Bosses, SettingKind::Number,
      "BOSS REPLACEMENTS PER MAP", nullptr,
      "How many bosses one area can hold. At the default of 1 that is 14 in a "
      "full run; at 15 it is up to 210. An area with fewer creatures available "
      "than asked for gets as many as it can and the run says so.",
      &RandomizerDefaults::bossReplacementsPerMap, 0, 15 },
    { SettingId::ReplaceAllEnemies, SettingCategory::Bosses, SettingKind::Toggle,
      "REPLACE ALL ENEMIES", &RandomizerDefaults::replaceAllEnemies,
      "Turns every eligible creature in every ticked area into a boss at once, "
      "ignoring the two settings above. This is a test control and not a way "
      "to play: it is expected to break the game." },
    { SettingId::BossAreas, SettingCategory::Bosses, SettingKind::InsertAreaPool,
      "AREAS INCLUDED", nullptr,
      "Choose which of the fourteen explorable areas bosses can appear in. "
      "Every creature in an unticked area is left alone." },
    { SettingId::InsertedBosses, SettingCategory::Bosses, SettingKind::InsertedBossPool,
      "INSERTED BOSSES", nullptr,
      "Choose which bosses can appear in the world. This is not the same list "
      "as Bosses Included, which decides who holds which arena. With nothing "
      "ticked here, nothing is inserted." },
    { SettingId::SpreadBossesOut, SettingCategory::Bosses, SettingKind::Toggle,
      "SPREAD BOSSES OUT", &RandomizerDefaults::spaceOutInsertedBosses,
      "Pushes an area's bosses apart, so each tends to hold its own part of "
      "it. Off, two can end up in the same room. At one boss per area it "
      "changes nothing." },
    { SettingId::BossBehaviour, SettingCategory::Bosses, SettingKind::NamedChoice,
      "BOSS BEHAVIOUR", &RandomizerDefaults::insertedBossKeepsEnemyAi,
      "Own AI makes an arriving boss fight the way it fights in its arena. "
      "Replaced Enemy's AI drives it with the script of the creature it "
      "replaced instead, which may simply not work - it may stand inert, "
      "flail, or crash the game.",
      nullptr, 0, 0, "OWN AI", "REPLACED ENEMY'S AI" },

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
    // Feature 011 - the other two thirds of the same shop table, declared
    // immediately after the weapon row because that is where the
    // randomizer-settings-ui spec 7.1 table puts them and because the three
    // read as a set. Each shuffles within its own kind and the three buckets
    // never cross.
    { SettingId::RandomizeShopArmour, SettingCategory::WeaponsGear, SettingKind::Toggle,
      "RANDOMIZE SHOP ARMOUR", &RandomizerDefaults::randomizeShopArmour,
      "Randomizes the armour sold by the Bath Messengers. Prices and purchase "
      "limits stay with the listing, not the item." },
    { SettingId::RandomizeShopItems, SettingCategory::WeaponsGear, SettingKind::Toggle,
      "RANDOMIZE SHOP ITEMS", &RandomizerDefaults::randomizeShopItems,
      "Randomizes the items and materials sold by the Bath Messengers. Blood "
      "Vials and Quicksilver Bullets are left alone." },
    { SettingId::StartWithHunterTools, SettingCategory::WeaponsGear, SettingKind::Toggle,
      "START WITH HUNTER TOOLS", &RandomizerDefaults::startWithHunterTools,
      "Start with the Blood Gem and Rune Workshop Tools. Gems and runes can be "
      "fitted from the first area." },
    // Declared after START WITH HUNTER TOOLS, which is where the
    // randomizer-settings-ui spec 7.1 table now puts it. It is a pool and not
    // a toggle: its whole state is what is ticked, so there is no YES/NO row
    // above it and nothing to keep in step with the selection.
    { SettingId::StartWithTrickWeapon, SettingCategory::WeaponsGear,
      SettingKind::TrickWeaponPool,
      "START WITH A TRICK WEAPON", nullptr,
      "Start holding one of the trick weapons you tick here. Tick several and "
      "one is drawn for the run. Its requirements are lowered so you can use "
      "it at once." },
    // Feature 038 - the same setting for the other hand, declared immediately
    // after it because that is where the settings-UI spec 7.1 table puts it and
    // because the two read as a pair. Also a pool and not a toggle.
    //
    // LEFT, not LEFT-HAND: "START WITH A LEFT-HAND WEAPON" measures 573 px and
    // the row's label + gap + widest value budget is 700 px against a 118 px
    // "10 OF 14", so the longer wording overflows the settings pane by 31 px
    // (settings_ui_verify.py case 5). The help text below carries what "left"
    // means. The picker's own heading is the same string for the same reason.
    { SettingId::StartWithLeftHandWeapon, SettingCategory::WeaponsGear,
      SettingKind::LeftHandWeaponPool,
      "START WITH A LEFT WEAPON", nullptr,
      "Start holding one of the firearms, shields or torches you tick here in "
      "your left hand. Tick several and one is drawn for the run. Its "
      "requirements are lowered so you can use it at once." },
    // Feature 040 - four pickers, one per armour slot, declared immediately
    // after the two weapon pickers because that is where the settings-UI spec
    // 7.1 table puts them and because the six read as one group: what a new
    // character starts holding, then what it starts wearing. In the game's own
    // slot order, head first (plan 040 P5).
    //
    // Four separate settings and not one outfit picker: there is no outfit
    // concept anywhere in this feature, and each slot is drawn independently of
    // the other three. Pools and not toggles, like the two above - each one's
    // whole state is what is ticked.
    //
    // No armour requirement is rewritten by any of them: the armour row has no
    // requirement fields at all (starting_gear_verify.py A7), which is why these
    // four help texts say nothing about requirements where the two above do.
    { SettingId::StartWithHeadwear, SettingCategory::WeaponsGear,
      SettingKind::HeadwearPool,
      "START WITH HEADWEAR", nullptr,
      "Start wearing one of the hats, masks and helmets you tick here. Tick "
      "several and one is drawn for the run. Tick nothing and you wear what your "
      "origin gives you." },
    { SettingId::StartWithBodyArmour, SettingCategory::WeaponsGear,
      SettingKind::BodyArmourPool,
      "START WITH BODY ARMOUR", nullptr,
      "Start wearing one of the coats, garbs and robes you tick here. Tick "
      "several and one is drawn for the run. Tick nothing and you wear what your "
      "origin gives you." },
    { SettingId::StartWithGloves, SettingCategory::WeaponsGear,
      SettingKind::GlovesPool,
      "START WITH GLOVES", nullptr,
      "Start wearing one of the gloves and gauntlets you tick here. Tick several "
      "and one is drawn for the run. Tick nothing and you wear what your origin "
      "gives you." },
    { SettingId::StartWithLegwear, SettingCategory::WeaponsGear,
      SettingKind::LegwearPool,
      "START WITH LEGWEAR", nullptr,
      "Start wearing one of the trousers, greaves and hakama you tick here. Tick "
      "several and one is drawn for the run. Tick nothing and you wear what your "
      "origin gives you." },
    // A toggle and the last of feature 040's five, so the group ends on the one
    // row of it that has a YES/NO. It is a toggle and therefore DOES join the
    // readiness summary's count, which the four pickers above deliberately do
    // not (spec 040 B15).
    { SettingId::StartWithSupplies, SettingCategory::WeaponsGear,
      SettingKind::Toggle,
      "START WITH SUPPLIES", &RandomizerDefaults::startWithSupplies,
      "Start with 20 Blood Vials and 20 Quicksilver Bullets, on top of whatever "
      "else you are carrying. Nothing you already start with is taken away." },

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
    { SettingId::NoTeamType, SettingCategory::World, SettingKind::Toggle,
      "ENEMIES ON SAME TEAM", &RandomizerDefaults::noTeamType,
      "Puts every creature in the game onto one shared team - enemies, bosses "
      "and the Hunter's Dream residents alike. Off, each keeps the team the "
      "game shipped, and shuffling can stand rival teams side by side, so "
      "enemies sometimes fight each other. On, that infighting tends to "
      "stop." },

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

// Linear rather than an index built at startup: twenty-one entries scanned once per
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
        case SettingKind::TrickWeaponPool:
            return std::to_string(values.trickWeapons.CountEnabled()) + " OF " +
                   std::to_string(kTrickWeaponCount);
        case SettingKind::LeftHandWeaponPool:
            return std::to_string(values.leftHandWeapons.CountEnabled()) + " OF " +
                   std::to_string(kLeftHandWeaponCount);
        // Feature 040's four. Four cases and not one, because each reads its own
        // selection and its own count constant - the four tables are different
        // sizes and a shared case would have to be told which.
        case SettingKind::HeadwearPool:
            return std::to_string(values.headwear.CountEnabled()) + " OF " +
                   std::to_string(kHeadwearCount);
        case SettingKind::BodyArmourPool:
            return std::to_string(values.bodyArmour.CountEnabled()) + " OF " +
                   std::to_string(kBodyArmourCount);
        case SettingKind::GlovesPool:
            return std::to_string(values.gloves.CountEnabled()) + " OF " +
                   std::to_string(kGlovesCount);
        case SettingKind::LegwearPool:
            return std::to_string(values.legwear.CountEnabled()) + " OF " +
                   std::to_string(kLegwearCount);
        // The row carries its own two words, unlike SaveChoice above, which is
        // the whole difference between the two kinds.
        case SettingKind::NamedChoice:
            return (values.*(def.flag)) ? def.choiceOn : def.choiceOff;
        case SettingKind::Number:
            return std::to_string(values.*(def.number));
        case SettingKind::InsertedBossPool:
            return std::to_string(values.insertedBosses.CountEnabled()) + " OF " +
                   std::to_string(kInsertedBossModelCount);
        case SettingKind::InsertAreaPool:
            return std::to_string(values.bossAreas.CountEnabled()) + " OF " +
                   std::to_string(kInsertAreaCount);
    }
    return "";
}

// THE ONLY WRITER of a setting. Everything that used to flip a boolean - the
// left/right chain and the X chain, once about 190 lines between them - comes
// through here now, which is what makes "X never toggles" a property of one
// function rather than a rule every branch had to remember.
void AdjustSetting(const SettingDef& def, RandomizerDefaults& values, int direction) {
    // A Number is the one kind where `direction` means something: it steps by
    // one and clamps at both ends rather than wrapping, so holding Left parks
    // the count at numberMin instead of jumping back to numberMax. Clamping
    // rather than wrapping because 0 and the maximum are both meaningful values
    // a player will want to stop on.
    if (def.kind == SettingKind::Number) {
        if (def.number == nullptr) return;
        int value = values.*(def.number) + (direction >= 0 ? 1 : -1);
        if (value < def.numberMin) value = def.numberMin;
        if (value > def.numberMax) value = def.numberMax;
        values.*(def.number) = value;
        return;
    }
    (void)direction; // two states: left and right both flip them
    bool twoState = (def.kind == SettingKind::Toggle ||
                     def.kind == SettingKind::SaveChoice ||
                     def.kind == SettingKind::NamedChoice);
    if (!twoState || def.flag == nullptr) return;
    values.*(def.flag) = !(values.*(def.flag));
}

// The seven picker kinds, and only those. SaveChoice, NamedChoice and Number
// are all edited in place by Left/Right exactly as a toggle is, so X must do
// nothing on any of them either.
bool IsDrillIn(const SettingDef& def) {
    return def.kind != SettingKind::Toggle &&
           def.kind != SettingKind::SaveChoice &&
           def.kind != SettingKind::NamedChoice &&
           def.kind != SettingKind::Number;
}

// The four Selection* functions are the one place the seven picker types are
// told apart. They need a switch rather than a pointer-to-member because
// EnemyPoolSelection, EnemySkipSelection, BossPoolSelection,
// TrickWeaponSelection, LeftHandWeaponSelection, InsertedBossSelection and
// InsertAreaSelection are seven distinct types, and the polarity of the skip
// list and of the two weapon lists lives in each one's type.
//
// Every switch below is exhaustive and none has a default label, so a twelfth
// kind is a compiler warning in four places rather than a picker that silently
// draws the wrong table.
bool* SelectionFlags(const SettingDef& def, RandomizerDefaults& values) {
    switch (def.kind) {
        case SettingKind::EnemyPool: return values.enemiesIncluded.enabled;
        case SettingKind::EnemySkip: return values.enemiesSkipped.enabled;
        case SettingKind::BossPool:  return values.bossesIncluded.enabled;
        case SettingKind::TrickWeaponPool: return values.trickWeapons.enabled;
        case SettingKind::LeftHandWeaponPool: return values.leftHandWeapons.enabled;
        case SettingKind::HeadwearPool:   return values.headwear.enabled;
        case SettingKind::BodyArmourPool: return values.bodyArmour.enabled;
        case SettingKind::GlovesPool:     return values.gloves.enabled;
        case SettingKind::LegwearPool:    return values.legwear.enabled;
        case SettingKind::InsertedBossPool: return values.insertedBosses.enabled;
        case SettingKind::InsertAreaPool: return values.bossAreas.enabled;
        case SettingKind::Toggle:      break;
        case SettingKind::SaveChoice:  break;
        case SettingKind::NamedChoice: break;
        case SettingKind::Number:      break;
    }
    return nullptr;
}

const ModelPoolEntry* SelectionTable(const SettingDef& def) {
    switch (def.kind) {
        case SettingKind::EnemyPool: return EnemyPoolTable().data();
        case SettingKind::EnemySkip: return EnemySkipTable().data();
        case SettingKind::BossPool:  return BossPoolTable().data();
        case SettingKind::TrickWeaponPool: return TrickWeaponTable().data();
        case SettingKind::LeftHandWeaponPool: return LeftHandWeaponTable().data();
        case SettingKind::HeadwearPool:   return HeadwearTable().data();
        case SettingKind::BodyArmourPool: return BodyArmourTable().data();
        case SettingKind::GlovesPool:     return GlovesTable().data();
        case SettingKind::LegwearPool:    return LegwearTable().data();
        case SettingKind::InsertedBossPool: return InsertedBossTable().data();
        case SettingKind::InsertAreaPool: return InsertAreaTable().data();
        case SettingKind::Toggle:      break;
        case SettingKind::SaveChoice:  break;
        case SettingKind::NamedChoice: break;
        case SettingKind::Number:      break;
    }
    return nullptr;
}

int SelectionCount(const SettingDef& def) {
    switch (def.kind) {
        case SettingKind::EnemyPool: return kEnemyPoolModelCount;
        case SettingKind::EnemySkip: return kEnemySkipModelCount;
        case SettingKind::BossPool:  return kBossPoolModelCount;
        case SettingKind::TrickWeaponPool: return kTrickWeaponCount;
        case SettingKind::LeftHandWeaponPool: return kLeftHandWeaponCount;
        case SettingKind::HeadwearPool:   return kHeadwearCount;
        case SettingKind::BodyArmourPool: return kBodyArmourCount;
        case SettingKind::GlovesPool:     return kGlovesCount;
        case SettingKind::LegwearPool:    return kLegwearCount;
        case SettingKind::InsertedBossPool: return kInsertedBossModelCount;
        case SettingKind::InsertAreaPool: return kInsertAreaCount;
        case SettingKind::Toggle:      break;
        case SettingKind::SaveChoice:  break;
        case SettingKind::NamedChoice: break;
        case SettingKind::Number:      break;
    }
    return 0;
}

const PickerStrings& SelectionStrings(const SettingDef& def) {
    switch (def.kind) {
        case SettingKind::EnemySkip: return kEnemiesSkippedStrings;
        case SettingKind::BossPool:  return kBossesIncludedStrings;
        case SettingKind::TrickWeaponPool: return kTrickWeaponsStrings;
        case SettingKind::LeftHandWeaponPool: return kLeftHandWeaponsStrings;
        case SettingKind::HeadwearPool:   return kHeadwearStrings;
        case SettingKind::BodyArmourPool: return kBodyArmourStrings;
        case SettingKind::GlovesPool:     return kGlovesStrings;
        case SettingKind::LegwearPool:    return kLegwearStrings;
        case SettingKind::InsertedBossPool: return kInsertedBossesStrings;
        case SettingKind::InsertAreaPool: return kBossAreasStrings;
        case SettingKind::EnemyPool:   break;
        case SettingKind::Toggle:      break;
        case SettingKind::SaveChoice:  break;
        case SettingKind::NamedChoice: break;
        case SettingKind::Number:      break;
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
