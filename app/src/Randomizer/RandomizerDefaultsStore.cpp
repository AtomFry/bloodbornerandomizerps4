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
            if (strcmp(key, "backup_existing_save") == 0) {
                defaults.backupExistingSaveData = (atoi(value) != 0);
            } else if (strcmp(key, "bloodborne_title_id") == 0) {
                defaults.bloodborneTitleId = value;
            } else if (strcmp(key, "replace_save_default_is_new") == 0) {
                defaults.replaceSaveDefaultIsNew = (atoi(value) != 0);
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
            } else if (strcmp(key, "enable_mergo_darkness") == 0) {
                defaults.enableMergoDarkness = (atoi(value) != 0);
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

    // 1024, not 512: the enemies_included row alone is ~99 bytes and the
    // feature spec has ~20 more settings queued. The clamp below is
    // still the real guard; this just keeps the margin comfortable.
    char buf[1024];
    int len = snprintf(buf, sizeof(buf),
                        "backup_existing_save=%d\nbloodborne_title_id=%s\nreplace_save_default_is_new=%d\n"
                        "randomize_enemies=%d\nrandomize_bosses=%d\nrandomize_treasure=%d\n"
                        "randomize_workshop_tools=%d\n"
                        "randomize_enemy_drops=%d\n"
                        "randomize_starting_weapons=%d\nrandomize_starting_guns=%d\n"
                        "randomize_shop_weapons=%d\nenable_mergo_darkness=%d\n"
                        "bosses_included=%s\n"
                        "enemies_included=%s\n"
                        "last_seed=%u\n",
                        defaults.backupExistingSaveData ? 1 : 0,
                        defaults.bloodborneTitleId.c_str(),
                        defaults.replaceSaveDefaultIsNew ? 1 : 0,
                        defaults.randomizeEnemies ? 1 : 0,
                        defaults.randomizeBosses ? 1 : 0,
                        defaults.randomizeTreasure ? 1 : 0,
                        defaults.randomizeWorkshopTools ? 1 : 0,
                        defaults.randomizeEnemyDrops ? 1 : 0,
                        defaults.randomizeStartingWeapons ? 1 : 0,
                        defaults.randomizeStartingGuns ? 1 : 0,
                        defaults.randomizeShopWeapons ? 1 : 0,
                        defaults.enableMergoDarkness ? 1 : 0,
                        defaults.bossesIncluded.Encode().c_str(),
                        defaults.enemiesIncluded.Encode().c_str(),
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
