// SettingsModel.h - one description of every randomizer setting, replacing
// the hardcoded row indices both settings screens used to address them by.
//
// WHY THIS EXISTS. Until now a setting WAS its position: kEnemiesIncludedRow
// = 12 had to agree with items[12] in a parallel vector and with a branch in
// a 190-line if/else chain, and inserting a row renumbered everything below
// it. Three separate features accepted a worse row placement rather than
// renumber (see the tombstone comments this replaces). Categories are a
// REGROUPING of rows, which is exactly the operation that design forbade -
// so the indices go and every setting gets a stable identity instead.
//
// Declaration order in kSettings IS display order: the table is ordered by
// category and then by the within-category order of the randomizer-settings-ui
// spec §7.1. There is no second list to keep in step, and no row constant
// anywhere. Adding a setting is one entry in one place, wherever it belongs.
//
// Both screens can share this because both hold a whole RandomizerDefaults -
// the Enable wizard a per-run copy, Setup Defaults the persisted values it is
// editing - so a toggle is a pointer-to-member into that struct and works
// against either object.
//
// This is UI-layer only. Nothing under Randomizer/ knows that categories,
// help text or panes exist (CLAUDE.md §6).
#pragma once

#include "ModelPicker.h"

#include "../Randomizer/RandomizerDefaults.h"

#include <string>

namespace bbr {

// The six groups of spec §7.1, in rail order, plus SAVE. Count is the rail's
// category count, not a category - it is what sizes the per-category cursor
// arrays.
//
// SAVE IS EDITOR-ONLY (worlds plan §3.3). It holds the one per-world save
// policy, which is not something a NEW world starts from, so the DEFAULTS tab
// must not show it. Neither screen iterates this enum: each carries its own
// kCategories list, and the difference between the two lists IS the rule -
// settings_ui_verify.py asserts SAVE is in the editor's and not in Defaults'.
enum class SettingCategory {
    Enemies,
    Bosses,
    ItemsTreasure,
    WeaponsGear,
    Difficulty,
    World,
    Save,
    Count
};

// What a setting IS, which decides how it is drawn and what Left/Right and X
// do to it. The three pool kinds differ only in which baked table and which
// selection type they belong to - EnemyPoolSelection, EnemySkipSelection and
// BossPoolSelection are three distinct types, so they cannot share a
// pointer-to-member the way the toggles do.
//
// SaveChoice is a second bool-backed kind rather than a Toggle: it carries the
// same pointer-to-member and Left/Right flips it the same way, but its two
// states are named - KEEP EXISTING and START FRESH - and it is deliberately
// NOT counted by ToggleCount()/EnabledToggleCount(), because "9 OF 15
// SETTINGS ENABLED" is a statement about what the run randomizes and a save
// policy is not one of those.
enum class SettingKind { Toggle, SaveChoice, EnemyPool, EnemySkip, BossPool };

// A stable name per setting. NEVER reordered and never reused: a screen asks
// for a setting by identity, and nothing outside this file may assume an id's
// numeric value. Adding one goes wherever it reads best.
enum class SettingId {
    RandomizeEnemies,
    EnemiesIncluded,
    EnemiesSkipped,
    DoNotRandomizeCagedDogs,
    RandomizeBosses,
    BossesIncluded,
    RandomizeTreasure,
    RandomizeWorkshopTools,
    RandomizeEnemyDrops,
    RandomizeStartingWeapons,
    RandomizeStartingGuns,
    RandomizeShopWeapons,
    StartWithHunterTools,
    EasyShadows,
    EasyRom,
    EasyFailures,
    EasyEmissary,
    EnableMergoDarkness,
    SaveData,
};

struct SettingDef {
    SettingId       id;
    SettingCategory category;
    SettingKind     kind;
    const char*     label;           // the on-screen row label - today's shipped
                                     // wording, not the spec's prose name (§9 D2)
    bool RandomizerDefaults::* flag; // Toggle and SaveChoice; nullptr for the
                                     // three pool kinds
    const char*     help;            // spec Appendix A, never empty
};

// The table, read only through the functions below so no screen indexes it.
int               SettingCount();
const SettingDef& SettingAt(int index);

// Rail-side view of the same table.
const char*       CategoryLabel(SettingCategory category);
int               CategorySize(SettingCategory category);
const SettingDef& SettingInCategory(SettingCategory category, int index);

// "YES"/"NO" for a toggle, "KEEP EXISTING"/"START FRESH" for SaveChoice,
// "N OF M" for the three pool kinds.
std::string SettingValueText(const SettingDef& def, const RandomizerDefaults& values);

// THE ONLY WRITER of a setting. Flips a toggle or a SaveChoice whichever way
// `direction` points - both have two states, so left and right do the same
// thing to them - and is a no-op for the pool kinds, which are edited in their
// picker. X never comes here: X advances, it does not change a value (spec §10
// 9.2).
void AdjustSetting(const SettingDef& def, RandomizerDefaults& values, int direction);

// True for the three settings X opens a picker for.
bool IsDrillIn(const SettingDef& def);

// The picker's four host-supplied pieces, so a host does not repeat the
// switch on kind. SelectionFlags returns nullptr for a toggle.
bool*                 SelectionFlags(const SettingDef& def, RandomizerDefaults& values);
const ModelPoolEntry* SelectionTable(const SettingDef& def);
int                   SelectionCount(const SettingDef& def);
const PickerStrings&  SelectionStrings(const SettingDef& def);

// The readiness summary's two numbers: how many settings are toggles, and how
// many of those are on. SaveChoice is not a toggle and is in neither.
int ToggleCount();
int EnabledToggleCount(const RandomizerDefaults& values);

// The rail rows that are not settings. Their help lives beside kSettings
// rather than inside either screen, so one verifier case covers every help
// string the feature draws. SEED is spec Appendix A; BLOODBORNE TITLE ID has
// no Appendix A entry, so the randomizer-settings-ui plan's §4.1 supplies one
// drawn from the field comment in RandomizerDefaults.h (P14). NAME and
// HISTORY are the world editor's two new rows (worlds plan §4.5), and they
// replace FINISH, which went with the Enable wizard's rail.
const char* SeedHelp();
const char* TitleIdHelp();
const char* NameHelp();
const char* HistoryHelp();

} // namespace bbr
