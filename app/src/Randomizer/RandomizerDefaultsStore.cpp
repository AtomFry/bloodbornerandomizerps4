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
    // 1024, not 512: enemies_included is ~99 bytes, enemies_skipped ~102, and
    // the feature spec has ~20 more settings queued. Worst case today is 636
    // bytes for a whole defaults.cfg, pinned by a pool_verify selftest case.
    // The clamp below is still the real guard; this just keeps the margin
    // comfortable.
    char buf[1024];
    int len = snprintf(buf, sizeof(buf),
                        "randomize_enemies=%d\nrandomize_bosses=%d\nrandomize_treasure=%d\n"
                        "randomize_workshop_tools=%d\n"
                        "randomize_enemy_drops=%d\n"
                        "randomize_starting_weapons=%d\nrandomize_starting_guns=%d\n"
                        "randomize_shop_weapons=%d\nenable_mergo_darkness=%d\n"
                        "do_not_randomize_caged_dogs=%d\n"
                        "start_with_hunter_tools=%d\n"
                        "easy_shadows=%d\neasy_rom=%d\n"
                        "easy_failures=%d\neasy_emissary=%d\n"
                        "start_fresh_save=%d\n"
                        "bosses_included=%s\n"
                        "enemies_included=%s\n"
                        "enemies_skipped=%s\n",
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
                        defaults.startFreshSave ? 1 : 0,
                        defaults.bossesIncluded.Encode().c_str(),
                        defaults.enemiesIncluded.Encode().c_str(),
                        defaults.enemiesSkipped.Encode().c_str());
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
    } else if (strcmp(key, "enable_mergo_darkness") == 0) {
        out.enableMergoDarkness = (atoi(value) != 0);
    } else if (strcmp(key, "do_not_randomize_caged_dogs") == 0) {
        // Absent -> the struct's own false, which is the run the app
        // made before this key existed (spec 033 D2).
        out.doNotRandomizeCagedDogs = (atoi(value) != 0);
    } else if (strcmp(key, "start_with_hunter_tools") == 0) {
        // Absent -> the struct's own false, which is the run the app
        // made before this key existed.
        out.startWithHunterTools = (atoi(value) != 0);
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
