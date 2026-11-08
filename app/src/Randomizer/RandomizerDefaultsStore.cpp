#include "RandomizerDefaultsStore.h"

#include <orbis/libkernel.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace bbr {

namespace {
const char* kPath = "/data/bbrandomizer/defaults.cfg";

// Same BSD open(2) flag values proven correct for sceKernelOpen elsewhere
// in this project (Platform.cpp, Game/AfrManager.cpp) - musl's <fcntl.h>
// values are Linux's and don't match this FreeBSD-derived kernel.
const int kBsdRdonly = 0x0000;
const int kBsdWronly = 0x0001;
const int kBsdCreat  = 0x0200;
const int kBsdTrunc  = 0x0400;
} // namespace

// ---------------------------------------------------------------------------
// The settings block, shared by defaults.cfg and a world's revision file
// ---------------------------------------------------------------------------

std::string FormatSettings(const RandomizerDefaults& defaults) {
    // 2048, not 1024, AND THAT GROWTH IS A PREREQUISITE OF FEATURE 013 RATHER
    // THAN A TIDY-UP (spec 013 D-I). The block below is 1,279 bytes with every
    // selection at full length, and a whole defaults.cfg is 1,330 - both pinned
    // as exact equalities by a pool_verify selftest case. At char buf[1024] the
    // block overflowed, and snprintf would have dropped the tail of the last key
    // IN SILENCE, producing a config that loads and is wrong.
    //
    // The arithmetic, so the next feature can rebase it rather than re-measure:
    // the block was 790 bytes before feature 013 (with rows 27 and 11 landed),
    // 013 adds 248 across its eight keys, and the two lines defaults.cfg wraps
    // the block in - bloodborne_title_id and last_seed - cost 51 more, giving
    // 1,038 / 1,089. Then 1,040 / 1,091, because correcting
    // gen_weapon_table.py's LOT_CATEGORY_WEAPON took LeftHandWeaponTable.h from
    // 14 rows to 16 and every picker line is one character per table row. Then
    // 1,279 / 1,330 once feature 040 added its five keys, which are 239 bytes
    // together: 55 + 56 + 50 + 56 for the four armour pickers (each the key, the
    // '=', its 33 / 34 / 26 / 31 character selection line and a newline) and 22
    // for start_with_supplies=1. The read buffer in LoadRandomizerDefaults is
    // already char buf[4096], so the 1,330-byte worst case loads. At 2,048 the
    // block has 769 bytes spare.
    //
    // EVERY KEY NAME'S LENGTH IS LOAD-BEARING because of that exact equality.
    // Renaming one means redoing this arithmetic and the verifier's figure.
    // The clamp below is still the real guard; this only keeps the margin.
    char buf[2048];
    int len = snprintf(buf, sizeof(buf),
                        "randomize_enemies=%d\nrandomize_bosses=%d\nrandomize_treasure=%d\n"
                        "randomize_workshop_tools=%d\n"
                        "randomize_enemy_drops=%d\n"
                        "randomize_starting_weapons=%d\nrandomize_starting_guns=%d\n"
                        "randomize_shop_weapons=%d\n"
                        "randomize_shop_armour=%d\nrandomize_shop_items=%d\n"
                        "enable_mergo_darkness=%d\n"
                        "no_team_type=%d\n"
                        "do_not_randomize_caged_dogs=%d\n"
                        "start_with_hunter_tools=%d\n"
                        "start_with_supplies=%d\n"
                        "easy_shadows=%d\neasy_rom=%d\n"
                        "easy_failures=%d\neasy_emissary=%d\n"
                        "start_fresh_save=%d\n"
                        "bosses_can_replace_enemies=%d\n"
                        "boss_replacements_exact=%d\n"
                        "boss_replacements_per_map=%d\n"
                        "replace_all_enemies=%d\n"
                        "space_out_inserted_bosses=%d\n"
                        "inserted_boss_keeps_enemy_ai=%d\n"
                        "bosses_included=%s\n"
                        "enemies_included=%s\n"
                        "enemies_skipped=%s\n"
                        "trick_weapons_included=%s\n"
                        "left_hand_weapons_included=%s\n"
                        "armour_head_included=%s\n"
                        "armour_body_included=%s\n"
                        "armour_gloves_included=%s\n"
                        "armour_legwear_included=%s\n"
                        "inserted_bosses_included=%s\n"
                        "inserted_boss_areas=%s\n",
                        defaults.randomizeEnemies ? 1 : 0,
                        defaults.randomizeBosses ? 1 : 0,
                        defaults.randomizeTreasure ? 1 : 0,
                        defaults.randomizeWorkshopTools ? 1 : 0,
                        defaults.randomizeEnemyDrops ? 1 : 0,
                        defaults.randomizeStartingWeapons ? 1 : 0,
                        defaults.randomizeStartingGuns ? 1 : 0,
                        defaults.randomizeShopWeapons ? 1 : 0,
                        defaults.randomizeShopArmour ? 1 : 0,
                        defaults.randomizeShopItems ? 1 : 0,
                        defaults.enableMergoDarkness ? 1 : 0,
                        defaults.noTeamType ? 1 : 0,
                        defaults.doNotRandomizeCagedDogs ? 1 : 0,
                        defaults.startWithHunterTools ? 1 : 0,
                        defaults.startWithSupplies ? 1 : 0,
                        defaults.easyShadows ? 1 : 0,
                        defaults.easyRom ? 1 : 0,
                        defaults.easyFailures ? 1 : 0,
                        defaults.easyEmissary ? 1 : 0,
                        defaults.startFreshSave ? 1 : 0,
                        defaults.bossesCanReplaceEnemies ? 1 : 0,
                        defaults.bossReplacementsExact ? 1 : 0,
                        defaults.bossReplacementsPerMap,
                        defaults.replaceAllEnemies ? 1 : 0,
                        defaults.spaceOutInsertedBosses ? 1 : 0,
                        defaults.insertedBossKeepsEnemyAi ? 1 : 0,
                        defaults.bossesIncluded.Encode().c_str(),
                        defaults.enemiesIncluded.Encode().c_str(),
                        defaults.enemiesSkipped.Encode().c_str(),
                        defaults.trickWeapons.Encode().c_str(),
                        defaults.leftHandWeapons.Encode().c_str(),
                        defaults.headwear.Encode().c_str(),
                        defaults.bodyArmour.Encode().c_str(),
                        defaults.gloves.Encode().c_str(),
                        defaults.legwear.Encode().c_str(),
                        defaults.insertedBosses.Encode().c_str(),
                        defaults.bossAreas.Encode().c_str());
    // snprintf returns the length it WOULD have written, which can exceed
    // sizeof(buf) if the format ever outgrows it - clamp so a future setting
    // can't turn this into an out-of-bounds sceKernelWrite (see
    // docs/plans/workshop-tools.md §4.4).
    if (len < 0) return std::string();
    if ((size_t)len > sizeof(buf)) len = (int)sizeof(buf);
    return std::string(buf, (size_t)len);
}

bool ApplySettingKey(const char* key, const char* value, RandomizerDefaults& out) {
    if (strcmp(key, "bloodborne_title_id") == 0) {
        // Defaults only - a revision file never carries this key, because the
        // AFR title is not a per-world value (spec worlds D25).
        out.bloodborneTitleId = value;
    } else if (strcmp(key, "randomize_enemies") == 0) {
        out.randomizeEnemies = (atoi(value) != 0);
    } else if (strcmp(key, "randomize_bosses") == 0) {
        out.randomizeBosses = (atoi(value) != 0);
    } else if (strcmp(key, "randomize_treasure") == 0) {
        out.randomizeTreasure = (atoi(value) != 0);
    } else if (strcmp(key, "randomize_workshop_tools") == 0) {
        out.randomizeWorkshopTools = (atoi(value) != 0);
    } else if (strcmp(key, "randomize_enemy_drops") == 0) {
        out.randomizeEnemyDrops = (atoi(value) != 0);
    } else if (strcmp(key, "randomize_starting_weapons") == 0) {
        out.randomizeStartingWeapons = (atoi(value) != 0);
    } else if (strcmp(key, "randomize_starting_guns") == 0) {
        out.randomizeStartingGuns = (atoi(value) != 0);
    } else if (strcmp(key, "randomize_shop_weapons") == 0) {
        out.randomizeShopWeapons = (atoi(value) != 0);
    } else if (strcmp(key, "randomize_shop_armour") == 0) {
        // Feature 011. Absent -> the struct's own false, which is the run the
        // app made before these two keys existed: the pass never runs and the
        // shop table is byte-identical to vanilla. Both key LENGTHS are
        // load-bearing - pool_verify.py pins the settings block at an exact
        // byte count - so renaming either means redoing that arithmetic.
        out.randomizeShopArmour = (atoi(value) != 0);
    } else if (strcmp(key, "randomize_shop_items") == 0) {
        out.randomizeShopItems = (atoi(value) != 0);
    } else if (strcmp(key, "bosses_included") == 0) {
        out.bossesIncluded.Decode(value);
    } else if (strcmp(key, "enemies_included") == 0) {
        // Wrong length -> left at the all-enabled default. See
        // EnemyPoolSelection::Decode.
        out.enemiesIncluded.Decode(value);
    } else if (strcmp(key, "enemies_skipped") == 0) {
        // Same guard, opposite fail-safe: a wrong-length or
        // absent value leaves NOTHING skipped, because that is
        // this type's constructed state - see
        // ModelPoolSelection's DefaultSelected.
        out.enemiesSkipped.Decode(value);
    } else if (strcmp(key, "trick_weapons_included") == 0) {
        // Same guard and the same fail-safe as enemies_skipped: a
        // wrong-length or absent value leaves NOTHING ticked, which
        // grants nothing. The key name's LENGTH is load-bearing -
        // pool_verify.py pins the settings block at an exact byte
        // count - so renaming it means redoing that arithmetic.
        out.trickWeapons.Decode(value);
    } else if (strcmp(key, "left_hand_weapons_included") == 0) {
        // Feature 038, and the same guard and fail-safe again: a
        // wrong-length or absent value leaves NOTHING ticked, which
        // grants nothing. This key's LENGTH is load-bearing too -
        // pool_verify.py pins the settings block at an exact byte
        // count - so renaming it means redoing that arithmetic.
        out.leftHandWeapons.Decode(value);
    } else if (strcmp(key, "armour_head_included") == 0) {
        // Feature 040's four, one per armour slot, in the game's own slot order.
        // The same guard and the same fail-safe as the two weapon pickers above:
        // a wrong-length or absent value leaves NOTHING ticked, which leaves that
        // slot's vanilla garment alone.
        //
        // FOUR keys and not one, because the four slots are drawn independently
        // and each selection is bound to one table, so a line pasted into the
        // wrong key is rejected by the length guard rather than remapped onto
        // another slot's pieces. All four key LENGTHS are load-bearing -
        // pool_verify.py pins the settings block at an exact byte count - so
        // renaming any of them means redoing that arithmetic.
        out.headwear.Decode(value);
    } else if (strcmp(key, "armour_body_included") == 0) {
        out.bodyArmour.Decode(value);
    } else if (strcmp(key, "armour_gloves_included") == 0) {
        out.gloves.Decode(value);
    } else if (strcmp(key, "armour_legwear_included") == 0) {
        out.legwear.Decode(value);
    } else if (strcmp(key, "enable_mergo_darkness") == 0) {
        out.enableMergoDarkness = (atoi(value) != 0);
    } else if (strcmp(key, "no_team_type") == 0) {
        // Feature 027, NO TEAM TYPE. Absent -> the struct's own false,
        // which is the run the app made before this key existed: the pass
        // never runs and the item-data archive is untouched by it. This
        // key's LENGTH is load-bearing - pool_verify.py pins the settings
        // block at an exact byte count - so renaming it means redoing that
        // arithmetic.
        out.noTeamType = (atoi(value) != 0);
    } else if (strcmp(key, "do_not_randomize_caged_dogs") == 0) {
        // Absent -> the struct's own false, which is the run the app
        // made before this key existed (spec 033 D2).
        out.doNotRandomizeCagedDogs = (atoi(value) != 0);
    } else if (strcmp(key, "start_with_hunter_tools") == 0) {
        // Absent -> the struct's own false, which is the run the app
        // made before this key existed.
        out.startWithHunterTools = (atoi(value) != 0);
    } else if (strcmp(key, "start_with_supplies") == 0) {
        // Feature 040. Absent -> the struct's own false, which is the run the
        // app made before this key existed: the pass never runs and no origin row
        // gains an item. This key's LENGTH is load-bearing too.
        out.startWithSupplies = (atoi(value) != 0);
    } else if (strcmp(key, "easy_shadows") == 0) {
        // Same rule for all four: absent -> the struct's own false,
        // which is the run the app made before these keys existed.
        out.easyShadows = (atoi(value) != 0);
    } else if (strcmp(key, "easy_rom") == 0) {
        out.easyRom = (atoi(value) != 0);
    } else if (strcmp(key, "easy_failures") == 0) {
        out.easyFailures = (atoi(value) != 0);
    } else if (strcmp(key, "easy_emissary") == 0) {
        out.easyEmissary = (atoi(value) != 0);
    } else if (strcmp(key, "start_fresh_save") == 0) {
        // Absent -> the struct's own false, which is KEEP EXISTING - always
        // the default, and what every activation reverts to (spec worlds
        // D17). A defaults.cfg written before this key existed therefore
        // reads as KEEP EXISTING, which is the only safe reading of silence.
        out.startFreshSave = (atoi(value) != 0);
    } else if (strcmp(key, "bosses_can_replace_enemies") == 0) {
        // Feature 013's master switch. Absent -> the struct's own false, which
        // is not an approximation of the run the app made before this key
        // existed but that run exactly: the pass is tested before its first
        // draw, so an off run consumes no randomness and every seed still
        // produces the world it used to.
        out.bossesCanReplaceEnemies = (atoi(value) != 0);
    } else if (strcmp(key, "boss_replacements_exact") == 0) {
        // false is UP TO, true is EXACTLY.
        out.bossReplacementsExact = (atoi(value) != 0);
    } else if (strcmp(key, "boss_replacements_per_map") == 0) {
        // The one integer setting in the config. Clamped on read as well as in
        // AdjustSetting, because a hand-edited file is the one path that can
        // present a value the UI cannot: 0 to 15 inclusive (spec 013 D-K), and
        // atoi returns 0 for anything unparsable, which is the harmless end.
        int n = atoi(value);
        if (n < 0) n = 0;
        if (n > 15) n = 15;
        out.bossReplacementsPerMap = n;
    } else if (strcmp(key, "replace_all_enemies") == 0) {
        // The temporary test control (spec 013 §6). When it is deleted this
        // branch goes with it and a defaults.cfg still carrying the line loads
        // with it ignored, exactly as the retired bell-maidens key does. (Not
        // spelled with its underscores here: pool_verify.py asserts that the
        // only mention of that key name left in this file is the one comment
        // below, which is how it tells documentation from a surviving branch.)
        out.replaceAllEnemies = (atoi(value) != 0);
    } else if (strcmp(key, "space_out_inserted_bosses") == 0) {
        out.spaceOutInsertedBosses = (atoi(value) != 0);
    } else if (strcmp(key, "inserted_boss_keeps_enemy_ai") == 0) {
        // false is OWN AI, true is REPLACED ENEMY'S AI.
        out.insertedBossKeepsEnemyAi = (atoi(value) != 0);
    } else if (strcmp(key, "inserted_bosses_included") == 0) {
        // 22 rows, and the same length guard as every other selection: a
        // wrong-length or absent value leaves all 22 ticked, which is this
        // type's constructed state. That reads as every boss enabled with the
        // master switch off, which is today's run.
        out.insertedBosses.Decode(value);
    } else if (strcmp(key, "inserted_boss_areas") == 0) {
        // 14 rows, same guard, same fail-safe: every area enabled with the
        // master switch off.
        out.bossAreas.Decode(value);
    } else if (strcmp(key, "last_seed") == 0) {
        // strtoul, not atoi: a seed can exceed INT_MAX.
        out.lastSeed = (uint32_t)strtoul(value, nullptr, 10);
    } else {
        return false;
    }
    return true;
}

void ApplySettingsText(const std::string& text, RandomizerDefaults& out) {
    // Plain "key=value" lines. Values are either an int or a short string -
    // handled by splitting on the first '=' ourselves rather than sscanf,
    // since one config holds both kinds. Unknown keys are ignored.
    size_t pos = 0;
    while (pos <= text.size()) {
        size_t nl = text.find('\n', pos);
        std::string line = text.substr(pos, nl == std::string::npos
                                                ? std::string::npos : nl - pos);
        if (!line.empty() && line[line.size() - 1] == '\r') line.resize(line.size() - 1);

        size_t eq = line.find('=');
        if (eq != std::string::npos) {
            ApplySettingKey(line.substr(0, eq).c_str(), line.substr(eq + 1).c_str(), out);
        }

        if (nl == std::string::npos) break;
        pos = nl + 1;
    }
}

// ---------------------------------------------------------------------------
// defaults.cfg
// ---------------------------------------------------------------------------

RandomizerDefaults LoadRandomizerDefaults() {
    RandomizerDefaults defaults; // struct's own defaults stand if anything below fails

    int fd = sceKernelOpen(kPath, kBsdRdonly, 0777);
    if (fd < 0) return defaults; // no file yet - e.g. first run

    char buf[4096];
    int n = sceKernelRead(fd, buf, sizeof(buf) - 1);
    sceKernelClose(fd);
    if (n <= 0) return defaults;
    buf[n] = '\0';

    ApplySettingsText(std::string(buf, (size_t)n), defaults);
    return defaults;
}

void SaveRandomizerDefaults(const RandomizerDefaults& defaults) {
    sceKernelMkdir("/data/bbrandomizer", 0777);

    // The two keys defaults.cfg wraps the shared settings block in. Both stay
    // snprintf format strings so the whole key set is still readable out of
    // this file by settings_ui_verify.py, which pins that every key the store
    // reads it also writes.
    //
    // Dropping unchanged_bell_maidens here is genuinely free: the loader above
    // is key=value with unknown keys ignored, so an existing file that still
    // carries that line loads fine and every other setting keeps its meaning.
    // Only the SELECTION VALUES are positional, not the lines themselves.
    //
    // backup_existing_save and replace_save_default_is_new were dropped the
    // same way, and for the same reason it is safe: an existing defaults.cfg
    // still carrying either line loads with that line ignored and every other
    // setting honoured. They are simply no longer read and no longer written.
    char head[64];
    int hn = snprintf(head, sizeof(head), "bloodborne_title_id=%s\n",
                      defaults.bloodborneTitleId.c_str());
    char tail[32];
    int tn = snprintf(tail, sizeof(tail), "last_seed=%u\n", (unsigned)defaults.lastSeed);
    if (hn < 0 || tn < 0) return;
    if ((size_t)hn > sizeof(head)) hn = (int)sizeof(head);
    if ((size_t)tn > sizeof(tail)) tn = (int)sizeof(tail);

    std::string text = std::string(head, (size_t)hn) +
                       FormatSettings(defaults) +
                       std::string(tail, (size_t)tn);

    int fd = sceKernelOpen(kPath, kBsdWronly | kBsdCreat | kBsdTrunc, 0777);
    if (fd < 0) return;
    sceKernelWrite(fd, text.data(), text.size());
    sceKernelFsync(fd);
    sceKernelClose(fd);
}

} // namespace bbr
