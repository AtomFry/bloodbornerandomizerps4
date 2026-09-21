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

RandomizerDefaults LoadRandomizerDefaults() {
    RandomizerDefaults defaults; // struct's own defaults stand if anything below fails

    int fd = sceKernelOpen(kPath, kBsdRdonly, 0777);
    if (fd < 0) return defaults; // no file yet - e.g. first run

    char buf[4096];
    int n = sceKernelRead(fd, buf, sizeof(buf) - 1);
    sceKernelClose(fd);
    if (n <= 0) return defaults;
    buf[n] = '\0';

    // Plain "key=value" lines. Values are either an int or a short string -
    // handled by splitting on the first '=' ourselves rather than sscanf,
    // since one config now holds both kinds.
    char* line = strtok(buf, "\n");
    while (line) {
        char* eq = strchr(line, '=');
        if (eq) {
            *eq = '\0';
            const char* key = line;
            const char* value = eq + 1;
            if (strcmp(key, "bloodborne_title_id") == 0) {
                defaults.bloodborneTitleId = value;
            } else if (strcmp(key, "randomize_enemies") == 0) {
                defaults.randomizeEnemies = (atoi(value) != 0);
            } else if (strcmp(key, "randomize_bosses") == 0) {
                defaults.randomizeBosses = (atoi(value) != 0);
            } else if (strcmp(key, "randomize_treasure") == 0) {
                defaults.randomizeTreasure = (atoi(value) != 0);
            } else if (strcmp(key, "randomize_workshop_tools") == 0) {
                defaults.randomizeWorkshopTools = (atoi(value) != 0);
            } else if (strcmp(key, "randomize_enemy_drops") == 0) {
                defaults.randomizeEnemyDrops = (atoi(value) != 0);
            } else if (strcmp(key, "randomize_starting_weapons") == 0) {
                defaults.randomizeStartingWeapons = (atoi(value) != 0);
            } else if (strcmp(key, "randomize_starting_guns") == 0) {
                defaults.randomizeStartingGuns = (atoi(value) != 0);
            } else if (strcmp(key, "randomize_shop_weapons") == 0) {
                defaults.randomizeShopWeapons = (atoi(value) != 0);
            } else if (strcmp(key, "bosses_included") == 0) {
                defaults.bossesIncluded.Decode(value);
            } else if (strcmp(key, "enemies_included") == 0) {
                // Wrong length -> left at the all-enabled default. See
                // EnemyPoolSelection::Decode.
                defaults.enemiesIncluded.Decode(value);
            } else if (strcmp(key, "enemies_skipped") == 0) {
                // Same guard, opposite fail-safe: a wrong-length or
                // absent value leaves NOTHING skipped, because that is
                // this type's constructed state - see
                // ModelPoolSelection's DefaultSelected.
                defaults.enemiesSkipped.Decode(value);
            } else if (strcmp(key, "enable_mergo_darkness") == 0) {
                defaults.enableMergoDarkness = (atoi(value) != 0);
            } else if (strcmp(key, "do_not_randomize_caged_dogs") == 0) {
                // Absent -> the struct's own false, which is the run the app
                // made before this key existed (spec 033 D2).
                defaults.doNotRandomizeCagedDogs = (atoi(value) != 0);
            } else if (strcmp(key, "start_with_hunter_tools") == 0) {
                // Absent -> the struct's own false, which is the run the app
                // made before this key existed.
                defaults.startWithHunterTools = (atoi(value) != 0);
            } else if (strcmp(key, "easy_shadows") == 0) {
                // Same rule for all four: absent -> the struct's own false,
                // which is the run the app made before these keys existed.
                defaults.easyShadows = (atoi(value) != 0);
            } else if (strcmp(key, "easy_rom") == 0) {
                defaults.easyRom = (atoi(value) != 0);
            } else if (strcmp(key, "easy_failures") == 0) {
                defaults.easyFailures = (atoi(value) != 0);
            } else if (strcmp(key, "easy_emissary") == 0) {
                defaults.easyEmissary = (atoi(value) != 0);
            } else if (strcmp(key, "last_seed") == 0) {
                // strtoul, not atoi: a seed can exceed INT_MAX.
                defaults.lastSeed = (uint32_t)strtoul(value, nullptr, 10);
            }
        }
        line = strtok(nullptr, "\n");
    }
    return defaults;
}

void SaveRandomizerDefaults(const RandomizerDefaults& defaults) {
    sceKernelMkdir("/data/bbrandomizer", 0777);

    // 1024, not 512: enemies_included is ~99 bytes, enemies_skipped ~102, and
    // the feature spec has ~20 more settings queued. Worst case today is 616
    // bytes, pinned by a pool_verify selftest case. The clamp below is still
    // the real guard; this just keeps the margin comfortable.
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
    char buf[1024];
    int len = snprintf(buf, sizeof(buf),
                        "bloodborne_title_id=%s\n"
                        "randomize_enemies=%d\nrandomize_bosses=%d\nrandomize_treasure=%d\n"
                        "randomize_workshop_tools=%d\n"
                        "randomize_enemy_drops=%d\n"
                        "randomize_starting_weapons=%d\nrandomize_starting_guns=%d\n"
                        "randomize_shop_weapons=%d\nenable_mergo_darkness=%d\n"
                        "do_not_randomize_caged_dogs=%d\n"
                        "start_with_hunter_tools=%d\n"
                        "easy_shadows=%d\neasy_rom=%d\n"
                        "easy_failures=%d\neasy_emissary=%d\n"
                        "bosses_included=%s\n"
                        "enemies_included=%s\n"
                        "enemies_skipped=%s\n"
                        "last_seed=%u\n",
                        defaults.bloodborneTitleId.c_str(),
                        defaults.randomizeEnemies ? 1 : 0,
                        defaults.randomizeBosses ? 1 : 0,
                        defaults.randomizeTreasure ? 1 : 0,
                        defaults.randomizeWorkshopTools ? 1 : 0,
                        defaults.randomizeEnemyDrops ? 1 : 0,
                        defaults.randomizeStartingWeapons ? 1 : 0,
                        defaults.randomizeStartingGuns ? 1 : 0,
                        defaults.randomizeShopWeapons ? 1 : 0,
                        defaults.enableMergoDarkness ? 1 : 0,
                        defaults.doNotRandomizeCagedDogs ? 1 : 0,
                        defaults.startWithHunterTools ? 1 : 0,
                        defaults.easyShadows ? 1 : 0,
                        defaults.easyRom ? 1 : 0,
                        defaults.easyFailures ? 1 : 0,
                        defaults.easyEmissary ? 1 : 0,
                        defaults.bossesIncluded.Encode().c_str(),
                        defaults.enemiesIncluded.Encode().c_str(),
                        defaults.enemiesSkipped.Encode().c_str(),
                        (unsigned)defaults.lastSeed);
    // snprintf returns the length it WOULD have written, which can exceed
    // sizeof(buf) if the format ever outgrows it - clamp so a future setting
    // can't turn this into an out-of-bounds sceKernelWrite (see
    // docs/plans/workshop-tools.md §4.4).
    if (len < 0) return;
    if ((size_t)len > sizeof(buf)) len = (int)sizeof(buf);

    int fd = sceKernelOpen(kPath, kBsdWronly | kBsdCreat | kBsdTrunc, 0777);
    if (fd < 0) return;
    sceKernelWrite(fd, buf, (size_t)len);
    sceKernelFsync(fd);
    sceKernelClose(fd);
}

} // namespace bbr
