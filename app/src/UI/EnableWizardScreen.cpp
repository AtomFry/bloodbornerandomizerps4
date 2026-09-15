// cstdlib before anything pulling <random>/<cmath>, or the toolchain's libc++
// fails on ::abs - same ordering constraint as BossRandomizer.cpp.
#include <cstdlib>

#include "EnableWizardScreen.h"

#include "Controls.h"

#include "../Platform/Log.h"
#include "../Randomizer/EnemyRandomizer.h"
#include "../Randomizer/RandomizerDefaultsStore.h"

#include <cstring>
#include <ctime>
#include <random>
#include <string>
#include <vector>

namespace bbr {

namespace {
const int kBackupToggleRow     = 0;
const int kReplaceRow          = 1;
const int kSeedRow             = 2;
const int kRandomizeEnemiesRow = 3;
const int kRandomizeBossesRow  = 4;
const int kRandomizeTreasureRow = 5;
const int kRandomizeWorkshopToolsRow = 6;
const int kRandomizeDropsRow   = 7;
const int kStartingWeaponsRow  = 8;
const int kStartingGunsRow     = 9;
const int kShopWeaponsRow      = 10;
// Appended rather than grouped with the RANDOMIZE rows: it is not a
// randomizer, and going last means no existing row index has to move.
const int kDisableMergoDarknessRow = 11;
// Drill-in, not a toggle: 82 rows cannot live inline (see EnemyPicker.h).
const int kEnemiesIncludedRow  = 12;
const int kBossesIncludedRow   = 13;
const int kSaveDataRowCount    = 14;

// One unified list: the two fixed actions, then whatever randomizer save
// backups exist. The backups are stub entries - Save Data backup isn't a
// real capability yet - but they represent real future data: this app's
// own randomizer-save backups, created at commit time when BACKUP EXISTING
// SAVE is YES (see EnableWizardScreen.h).
struct ReplaceOption { const char* label; const char* detail; };
const ReplaceOption kReplaceOptions[] = {
    { "NEW SAVE DATA",                "" },
    { "LEAVE EXISTING SAVE DATA",     "" },
    { "RANDOMIZER SAVE LUDWIG RUN",    "CREATED SEP 06 2026 STUB" },
    { "RANDOMIZER SAVE NEW GAME PLUS", "CREATED SEP 07 2026 STUB" },
    { "VANILLA SAVE SEP 01 2026",      "CREATED SEP 01 2026 STUB" },
};
const int kReplaceOptionCount = sizeof(kReplaceOptions) / sizeof(kReplaceOptions[0]);
const int kNewSaveIndex       = 0;
const int kLeaveExistingIndex = 1;

const int kTitleScale  = 5;
const int kItemScale   = 4;
const int kDetailScale = 3;
const int kFooterScale = 3;
const int kProgressScale = 3;

// Both settings lists share a band: from under the sub-heading down to just
// above where the "MORE BELOW" hint has to clear the footer.
const ListLayout kSettingsLayout = { 420, 90, 870 };

// The progress log is denser and starts higher. One slot is given up to the
// live status line while the run is going, so the log itself shows one fewer
// line then than it does once the run has finished.
const ListLayout kProgressLayout = { 300, 70, 920 };
} // namespace

EnableWizardScreen::EnableWizardScreen(RandomizerDefaults& defaults)
    : defaults_(defaults),
      titleId_(defaults.bloodborneTitleId),
      backupExistingSave_(defaults.backupExistingSaveData),
      replaceChoice_(defaults.replaceSaveDefaultIsNew ? ReplaceChoice::NewSaveData
                                                       : ReplaceChoice::LeaveExisting),
      randomizeEnemies_(defaults.randomizeEnemies),
      randomizeBosses_(defaults.randomizeBosses),
      randomizeTreasure_(defaults.randomizeTreasure),
      randomizeWorkshopTools_(defaults.randomizeWorkshopTools),
      randomizeEnemyDrops_(defaults.randomizeEnemyDrops),
      randomizeStartingWeapons_(defaults.randomizeStartingWeapons),
      randomizeStartingGuns_(defaults.randomizeStartingGuns),
      randomizeShopWeapons_(defaults.randomizeShopWeapons),
      enableMergoDarkness_(defaults.enableMergoDarkness),
      enemiesIncluded_(defaults.enemiesIncluded),
      bossesIncluded_(defaults.bossesIncluded),
      seed_(defaults.lastSeed) {
    // 0 means defaults.cfg has never carried a seed, so roll one immediately -
    // the row should never come up blank or showing a meaningless zero.
    if (seed_ == 0) RollNewSeed();
}

void EnableWizardScreen::GoToStep(Step step) {
    step_ = step;
    selected_ = 0;
    scrollOffset_ = 0; // a fresh step always starts at the top of its list
}

std::string EnableWizardScreen::ReplaceDisplayText() const {
    switch (replaceChoice_) {
        case ReplaceChoice::NewSaveData:      return "NEW SAVE DATA";
        case ReplaceChoice::LeaveExisting:    return "LEAVE EXISTING SAVE DATA";
        case ReplaceChoice::SelectFromBackup: return selectedBackupLabel_;
    }
    return "";
}

std::string EnableWizardScreen::SeedDisplayText() const {
    std::string digits = std::to_string(seed_);
    // A uint32 is at most 10 digits so this can't currently underflow, but the
    // subtraction is unsigned - guard it rather than depend on the type.
    if (digits.size() >= (size_t)kSeedDigits) return digits;
    // Zero-padded so the row and the digit editor show the same thing.
    return std::string((size_t)kSeedDigits - digits.size(), '0') + digits;
}

void EnableWizardScreen::RollNewSeed() {
    // NOT time(nullptr) directly: two presses inside the same second would
    // hand back the same value and look broken. One generator seeded from the
    // clock, drawn from repeatedly, gives a different value every press.
    static std::mt19937 roller((uint32_t)time(nullptr));
    seed_ = (uint32_t)roller();
    Log(("enable wizard: rolled new seed " + std::to_string(seed_)).c_str());
}

void EnableWizardScreen::Update(const ButtonEdges& input) {
    switch (step_) {
        case Step::SaveData:      UpdateSaveData(input); break;
        case Step::SelectReplace: UpdateSelectReplace(input); break;
        case Step::EditSeed:      UpdateEditSeed(input); break;
        case Step::EnemyPicker:   UpdateEnemyPicker(input); break;
        case Step::BossPicker:    UpdateBossPicker(input); break;
        case Step::Confirm:       UpdateConfirm(input); break;
        case Step::Progress:      UpdateProgress(input); break;
    }
}

void EnableWizardScreen::UpdateSaveData(const ButtonEdges& input) {
    selected_ = NavigateVertical(selected_, kSaveDataRowCount, input);
    scrollOffset_ = ScrollToShow(scrollOffset_, selected_, kSaveDataRowCount,
                                 VisibleRowCount(kSettingsLayout));

    if ((input.left || input.right) && selected_ == kBackupToggleRow) {
        backupExistingSave_ = !backupExistingSave_;
        Log(backupExistingSave_ ? "enable wizard: backup existing save = YES"
                                 : "enable wizard: backup existing save = NO");
    }
    if ((input.left || input.right) && selected_ == kSeedRow) {
        RollNewSeed();
    }
    if ((input.left || input.right) && selected_ == kRandomizeEnemiesRow) {
        randomizeEnemies_ = !randomizeEnemies_;
        Log(randomizeEnemies_ ? "enable wizard: randomize enemies = YES"
                               : "enable wizard: randomize enemies = NO");
    }
    if ((input.left || input.right) && selected_ == kRandomizeBossesRow) {
        randomizeBosses_ = !randomizeBosses_;
        Log(randomizeBosses_ ? "enable wizard: randomize bosses = YES"
                              : "enable wizard: randomize bosses = NO");
    }
    if ((input.left || input.right) && selected_ == kRandomizeTreasureRow) {
        randomizeTreasure_ = !randomizeTreasure_;
        Log(randomizeTreasure_ ? "enable wizard: randomize treasure = YES"
                                : "enable wizard: randomize treasure = NO");
    }
    if ((input.left || input.right) && selected_ == kRandomizeWorkshopToolsRow) {
        randomizeWorkshopTools_ = !randomizeWorkshopTools_;
        Log(randomizeWorkshopTools_ ? "enable wizard: randomize workshop tools = YES"
                                     : "enable wizard: randomize workshop tools = NO");
    }
    if ((input.left || input.right) && selected_ == kRandomizeDropsRow) {
        randomizeEnemyDrops_ = !randomizeEnemyDrops_;
        Log(randomizeEnemyDrops_ ? "enable wizard: randomize enemy drops = YES"
                                  : "enable wizard: randomize enemy drops = NO");
    }
    if ((input.left || input.right) && selected_ == kStartingWeaponsRow) {
        randomizeStartingWeapons_ = !randomizeStartingWeapons_;
        Log(randomizeStartingWeapons_ ? "enable wizard: randomize starting weapons = YES"
                                       : "enable wizard: randomize starting weapons = NO");
    }
    if ((input.left || input.right) && selected_ == kStartingGunsRow) {
        randomizeStartingGuns_ = !randomizeStartingGuns_;
        Log(randomizeStartingGuns_ ? "enable wizard: randomize starting guns = YES"
                                    : "enable wizard: randomize starting guns = NO");
    }
    if ((input.left || input.right) && selected_ == kShopWeaponsRow) {
        randomizeShopWeapons_ = !randomizeShopWeapons_;
        Log(randomizeShopWeapons_ ? "enable wizard: randomize shop weapons = YES"
                                   : "enable wizard: randomize shop weapons = NO");
    }
    // No left/right branch for kEnemiesIncludedRow - it is a drill-in, not a
    // toggle, so there is nothing to cycle through.
    if ((input.left || input.right) && selected_ == kDisableMergoDarknessRow) {
        enableMergoDarkness_ = !enableMergoDarkness_;
        Log(enableMergoDarkness_ ? "enable wizard: enable mergo darkness = YES"
                                   : "enable wizard: enable mergo darkness = NO");
    }

    if (input.cross) {
        if (selected_ == kBackupToggleRow) {
            backupExistingSave_ = !backupExistingSave_;
            Log(backupExistingSave_ ? "enable wizard: backup existing save = YES"
                                     : "enable wizard: backup existing save = NO");
        } else if (selected_ == kReplaceRow) {
            Log("enable wizard: opening select-replace screen");
            GoToStep(Step::SelectReplace);
        } else if (selected_ == kSeedRow) {
            std::string padded = SeedDisplayText();
            memcpy(seedBuf_, padded.c_str(), kSeedDigits);
            seedBuf_[kSeedDigits] = '\0';
            seedCursor_ = 0;
            Log("enable wizard: opening seed editor");
            GoToStep(Step::EditSeed);
        } else if (selected_ == kRandomizeEnemiesRow) {
            randomizeEnemies_ = !randomizeEnemies_;
            Log(randomizeEnemies_ ? "enable wizard: randomize enemies = YES"
                                   : "enable wizard: randomize enemies = NO");
        } else if (selected_ == kRandomizeBossesRow) {
            randomizeBosses_ = !randomizeBosses_;
            Log(randomizeBosses_ ? "enable wizard: randomize bosses = YES"
                                  : "enable wizard: randomize bosses = NO");
        } else if (selected_ == kRandomizeTreasureRow) {
            randomizeTreasure_ = !randomizeTreasure_;
            Log(randomizeTreasure_ ? "enable wizard: randomize treasure = YES"
                                    : "enable wizard: randomize treasure = NO");
        } else if (selected_ == kRandomizeWorkshopToolsRow) {
            randomizeWorkshopTools_ = !randomizeWorkshopTools_;
            Log(randomizeWorkshopTools_ ? "enable wizard: randomize workshop tools = YES"
                                         : "enable wizard: randomize workshop tools = NO");
        } else if (selected_ == kRandomizeDropsRow) {
            randomizeEnemyDrops_ = !randomizeEnemyDrops_;
            Log(randomizeEnemyDrops_ ? "enable wizard: randomize enemy drops = YES"
                                      : "enable wizard: randomize enemy drops = NO");
        } else if (selected_ == kStartingWeaponsRow) {
            randomizeStartingWeapons_ = !randomizeStartingWeapons_;
            Log(randomizeStartingWeapons_ ? "enable wizard: randomize starting weapons = YES"
                                           : "enable wizard: randomize starting weapons = NO");
        } else if (selected_ == kStartingGunsRow) {
            randomizeStartingGuns_ = !randomizeStartingGuns_;
            Log(randomizeStartingGuns_ ? "enable wizard: randomize starting guns = YES"
                                        : "enable wizard: randomize starting guns = NO");
        } else if (selected_ == kShopWeaponsRow) {
            randomizeShopWeapons_ = !randomizeShopWeapons_;
            Log(randomizeShopWeapons_ ? "enable wizard: randomize shop weapons = YES"
                                       : "enable wizard: randomize shop weapons = NO");
        } else if (selected_ == kDisableMergoDarknessRow) {
            enableMergoDarkness_ = !enableMergoDarkness_;
            Log(enableMergoDarkness_ ? "enable wizard: enable mergo darkness = YES"
                                       : "enable wizard: enable mergo darkness = NO");
        } else if (selected_ == kEnemiesIncludedRow) {
            picker_.Reset();
            Log("enable wizard: opening enemy picker");
            GoToStep(Step::EnemyPicker);
        } else if (selected_ == kBossesIncludedRow) {
            picker_.Reset();
            Log("enable wizard: opening boss picker");
            GoToStep(Step::BossPicker);
        }
    }

    if (input.options) {
        Log("enable wizard: NEXT from save data step - opening confirm screen");
        GoToStep(Step::Confirm);
    }

    if (input.circle) {
        Log("enable wizard: cancelled at first step - no changes made");
        requestedScreen_ = ScreenId::Menu;
    }
}

void EnableWizardScreen::UpdateSelectReplace(const ButtonEdges& input) {
    selected_ = NavigateVertical(selected_, kReplaceOptionCount, input);

    if (input.cross) {
        if (selected_ == kNewSaveIndex) {
            replaceChoice_ = ReplaceChoice::NewSaveData;
            selectedBackupLabel_.clear();
            Log("enable wizard: replace save data = NEW SAVE DATA");
        } else if (selected_ == kLeaveExistingIndex) {
            replaceChoice_ = ReplaceChoice::LeaveExisting;
            selectedBackupLabel_.clear();
            Log("enable wizard: replace save data = LEAVE EXISTING SAVE DATA");
        } else {
            replaceChoice_ = ReplaceChoice::SelectFromBackup;
            selectedBackupLabel_ = kReplaceOptions[selected_].label;
            Log((std::string("enable wizard: replace save data = ") + kReplaceOptions[selected_].label).c_str());
        }
        GoToStep(Step::SaveData);
    }

    if (input.circle) {
        Log("enable wizard: replace-save selection cancelled - no change");
        GoToStep(Step::SaveData);
    }
}

void EnableWizardScreen::UpdateEditSeed(const ButtonEdges& input) {
    // Same control scheme as the title-ID editor: left/right walks the cursor,
    // up/down cycles the digit under it.
    if (input.left)  seedCursor_ = (seedCursor_ + kSeedDigits - 1) % kSeedDigits;
    if (input.right) seedCursor_ = (seedCursor_ + 1) % kSeedDigits;

    if (input.up || input.down) {
        int dir = input.up ? 1 : -1;
        int digit = ((seedBuf_[seedCursor_] - '0') + dir + 10) % 10;
        seedBuf_[seedCursor_] = (char)('0' + digit);
    }

    if (input.cross) {
        // Ten digits can express more than a uint32 holds, so clamp rather
        // than wrap - a silently wrapped seed would be a seed you can't retype.
        unsigned long long value = strtoull(seedBuf_, nullptr, 10);
        if (value > 0xFFFFFFFFull) {
            value = 0xFFFFFFFFull;
            Log("enable wizard: seed clamped to 4294967295");
        }
        seed_ = (uint32_t)value;
        Log(("enable wizard: seed set to " + std::to_string(seed_)).c_str());
        GoToStep(Step::SaveData);
        selected_ = kSeedRow;
    }

    if (input.circle) {
        Log("enable wizard: seed edit cancelled - unchanged");
        GoToStep(Step::SaveData);
        selected_ = kSeedRow;
    }
}

// Both pickers return to the row they were opened from, the same way the seed
// editor does. GoToStep zeroes the scroll and these rows are past the visible
// window, so the offset is restored too rather than letting the list jump for a
// frame before ScrollToShow catches up.
void EnableWizardScreen::ReturnFromPicker(int row) {
    GoToStep(Step::SaveData);
    selected_ = row;
    scrollOffset_ = ScrollToShow(scrollOffset_, selected_, kSaveDataRowCount,
                                 VisibleRowCount(kSettingsLayout));
}

void EnableWizardScreen::UpdateEnemyPicker(const ButtonEdges& input) {
    if (picker_.Update(input, EnemyPoolTable().data(), kEnemyPoolModelCount,
                       enemiesIncluded_.enabled)) {
        ReturnFromPicker(kEnemiesIncludedRow);
    }
}

void EnableWizardScreen::UpdateBossPicker(const ButtonEdges& input) {
    if (picker_.Update(input, BossPoolTable().data(), kBossPoolModelCount,
                       bossesIncluded_.enabled)) {
        ReturnFromPicker(kBossesIncludedRow);
    }
}

void EnableWizardScreen::UpdateConfirm(const ButtonEdges& input) {
    // No cursor here - this is a review list - so up/down move the window
    // itself, which is the only way to read the settings that don't fit.
    int visible = VisibleRowCount(kSettingsLayout);
    if (input.up)   scrollOffset_--;
    if (input.down) scrollOffset_++;
    scrollOffset_ = ClampScroll(scrollOffset_, kSaveDataRowCount, visible);

    if (input.options) {
        Log("enable wizard: commit confirmed - running commit");
        // Switch screens BEFORE any work happens: the randomizer is stepped
        // from UpdateProgress across many frames, so the progress screen has
        // to already be the thing being drawn.
        StartCommit();
        GoToStep(Step::Progress);
    }

    if (input.circle) {
        Log("enable wizard: confirm cancelled - back to save data step");
        GoToStep(Step::SaveData);
    }
}

void EnableWizardScreen::UpdateProgress(const ButtonEdges& input) {
    // One coarse unit of randomizer work per frame. The frame loop is
    // serial (Update -> Draw -> Present), so doing the whole 10-20s run in
    // one call would leave nothing on screen until it finished - stepping
    // it here is what makes the progress display possible at all.
    if (job_) {
        job_->Step();
        if (job_->Done()) FinishCommit();
        return; // input is ignored while the commit is still running
    }

    // The run is over, so the log is static and can be read back: up/down
    // scroll it, which also unpins it from the tail.
    if (input.up) {
        progressFollowTail_ = false;
        progressScroll_--;
    }
    if (input.down) {
        progressFollowTail_ = false;
        progressScroll_++;
    }

    // O is the only other thing that does anything here - this is a result
    // screen, not a step with its own choices.
    if (input.circle) {
        Log("enable wizard: returning to menu");
        requestedScreen_ = ScreenId::Menu;
    }
}

namespace {
// A one-time manual FTP drop point: nothing on the PS4 ships a vanilla
// dvdroot_ps4 copy (AFR is a redirect/overlay over the real installed game,
// not a full tree - see UI_BLUEPRINT.md/project memory), so the randomizer
// needs its own read-only vanilla source to randomize FROM. The user seeds
// this folder by hand, once, over FTP.
const char* kVanillaSourceDir = "/data/bbrandomizer/VanillaSource/dvdroot_ps4";
} // namespace

void EnableWizardScreen::AddProgressLine(const std::string& line) {
    progressLines_.push_back(line);
    Log(("enable wizard: " + line).c_str());
}

void EnableWizardScreen::StartCommit() {
    progressLines_.clear();
    completionLineStart_ = (std::size_t)-1;
    commitFinished_ = false;
    job_.reset();

    AddProgressLine(backupExistingSave_ ? "BACKING UP EXISTING SAVE (SIMULATED)"
                                        : "SKIPPING BACKUP PROCESS");

    switch (replaceChoice_) {
        case ReplaceChoice::NewSaveData:
            AddProgressLine("REMOVING EXISTING SAVE DATA (SIMULATED)");
            break;
        case ReplaceChoice::LeaveExisting:
            AddProgressLine("LEAVING EXISTING SAVE DATA");
            break;
        case ReplaceChoice::SelectFromBackup:
            AddProgressLine("RESTORING SAVE DATA " + selectedBackupLabel_ + " (SIMULATED)");
            break;
    }

    // The seed is now always chosen on the settings screen - rolled or typed -
    // so this reports it rather than generating one.
    uint32_t seed = seed_;
    AddProgressLine("USING SEED " + std::to_string(seed));

    // Remembered so the next run opens showing it: "do that again" becomes
    // open-the-wizard-and-commit. Only lastSeed is written back; the wizard's
    // own toggle states stay per-run, as they always have.
    defaults_.lastSeed = seed;
    SaveRandomizerDefaults(defaults_);

    // D12/D8: an empty pool would be caught by StepBuildPool, but only after
    // the mirror phase has already copied most of the game into the AFR folder
    // - so the user would watch it fail and be left with a half-built tree.
    // Refuse up front instead, and say what to do about it.
    if (randomizeEnemies_ && enemiesIncluded_.NoneEnabled()) {
        AddProgressLine("NO ENEMIES SELECTED");
        AddProgressLine("SELECT AT LEAST ONE ENEMY OR TURN OFF RANDOMIZE ENEMIES");
        completionLineStart_ = progressLines_.size();
        AddProgressLine("COMMIT CANCELLED - NOTHING WAS WRITTEN");
        commitFinished_ = true;
        return;
    }

    // The boss equivalent, and it guards a nastier failure: the boss pool is
    // drained as arenas are assigned and refilled from a model-distinct copy
    // when it empties. With no bosses selected BOTH are empty, so the refill
    // never helps and the next draw calls RandIndex(rng, 0) -> a
    // uniform_int_distribution(0, -1), which is undefined behaviour rather
    // than a clean failure. BossRandomizer.cpp's own header warns about this.
    if (randomizeBosses_ && bossesIncluded_.NoneEnabled()) {
        AddProgressLine("NO BOSSES SELECTED");
        AddProgressLine("SELECT AT LEAST ONE BOSS OR TURN OFF RANDOMIZE BOSSES");
        completionLineStart_ = progressLines_.size();
        AddProgressLine("COMMIT CANCELLED - NOTHING WAS WRITTEN");
        commitFinished_ = true;
        return;
    }

    if (!randomizeEnemies_) AddProgressLine("SKIPPING ENEMY RANDOMIZATION");
    if (!randomizeBosses_)  AddProgressLine("SKIPPING BOSS RANDOMIZATION");
    if (!randomizeTreasure_) AddProgressLine("SKIPPING TREASURE RANDOMIZATION");
    if (!randomizeEnemyDrops_) AddProgressLine("SKIPPING ENEMY DROP RANDOMIZATION");
    if (!randomizeStartingWeapons_) AddProgressLine("SKIPPING STARTING WEAPON RANDOMIZATION");
    if (!randomizeStartingGuns_) AddProgressLine("SKIPPING STARTING GUN RANDOMIZATION");
    if (!randomizeShopWeapons_) AddProgressLine("SKIPPING SHOP WEAPON RANDOMIZATION");

    // enableMergoDarkness_ IS in this list, unlike randomizeWorkshopTools_
    // below: it is a complete change on its own rather than a modifier on
    // another feature, so turning it on alone must still build a tree (D4 in
    // docs/plans/mergo-darkness.md). There is no asymmetry to worry
    // about because the setting is named for its action - YES is the only
    // state that writes anything, and NO asks for nothing to be done.
    if (randomizeEnemies_ || randomizeBosses_ || randomizeTreasure_ ||
        randomizeEnemyDrops_ || randomizeStartingWeapons_ ||
        randomizeStartingGuns_ || randomizeShopWeapons_ ||
        enableMergoDarkness_) {
        std::string outputDir = "/data/GoldHEN/AFR/" + titleId_ + "/dvdroot_ps4";
        EnemyRandomizerOptions options;
        options.randomizeEnemies = randomizeEnemies_;
        options.randomizeBosses = randomizeBosses_;
        options.randomizeTreasure = randomizeTreasure_;
        // Meaningless without randomizeTreasure (D3), so it deliberately does
        // NOT appear in the big || above - ticking it alone must not start a
        // run that does nothing.
        options.randomizeWorkshopTools = randomizeWorkshopTools_;
        options.randomizeEnemyDrops = randomizeEnemyDrops_;
        options.randomizeStartingWeapons = randomizeStartingWeapons_;
        options.randomizeStartingGuns = randomizeStartingGuns_;
        options.randomizeShopWeapons = randomizeShopWeapons_;
        options.enableMergoDarkness = enableMergoDarkness_;
        options.enemiesIncluded = enemiesIncluded_;
        options.bossesIncluded = bossesIncluded_;
        job_.reset(new EnemyRandomizerJob(kVanillaSourceDir, outputDir, seed, options));
    } else {
        FinishCommit();
    }
}

void EnableWizardScreen::FinishCommit() {
    bool failed = false;

    if (job_) {
        const EnemyRandomizerResult& result = job_->Result();
        if (result.success) {
            if (randomizeEnemies_) {
                AddProgressLine("RANDOMIZED " + std::to_string(result.enemiesRandomized) +
                                " ENEMIES ACROSS " + std::to_string(result.mapsProcessed) + " MAPS");
            }
            if (randomizeBosses_) {
                AddProgressLine("RANDOMIZED " + std::to_string(result.bossesRandomized) +
                                " BOSS PLACEMENTS");
            }
            if (randomizeTreasure_) {
                // D5: report workshop-tools state by extending this line rather
                // than a separate SKIPPING line, which would fire even when
                // treasure randomization was never going to run. No parentheses
                // - the 8x8 font has no punctuation glyphs (Font8x8.cpp), so
                // they'd render as blank gaps.
                std::string line = "RANDOMIZED " + std::to_string(result.treasuresRandomized) +
                                    " TREASURE PICKUPS";
                if (randomizeWorkshopTools_) line += " WORKSHOP TOOLS INCLUDED";
                AddProgressLine(line);
            }
            if (randomizeStartingWeapons_ || randomizeStartingGuns_) {
                AddProgressLine("STARTING CHOICES: " +
                                std::to_string(result.startingMeleeChanged) + " WEAPONS, " +
                                std::to_string(result.startingGunsChanged) + " GUNS");
            }
            if (randomizeShopWeapons_) {
                AddProgressLine("RANDOMIZED " + std::to_string(result.shopWeaponsChanged) +
                                " SHOP WEAPONS");
            }
            if (randomizeEnemyDrops_) {
                AddProgressLine("RANDOMIZED " + std::to_string(result.dropsRandomized) +
                                " ENEMY DROPS");
            }
            // Reported only when it did something. There is no SKIPPING
            // counterpart: NO means the app deliberately left the file alone,
            // which is not a skipped step worth a line (D5).
            // Reported only when it is ON, because ON is the unusual
            // outcome: the whole game runs dark. OFF is the normal game.
            if (enableMergoDarkness_) {
                AddProgressLine("MERGO DARKNESS ENABLED - THE WORLD WILL BE DARK");
            }
            // Only when it did something - an all-enabled pool is the default
            // and saying so every run is noise.
            if (randomizeEnemies_ && !enemiesIncluded_.AllEnabled()) {
                AddProgressLine("ENEMY POOL LIMITED TO " +
                                std::to_string(enemiesIncluded_.CountEnabled()) + " OF " +
                                std::to_string(kEnemyPoolModelCount) + " ENEMIES");
            }
            if (randomizeBosses_ && !bossesIncluded_.AllEnabled()) {
                AddProgressLine("BOSS POOL LIMITED TO " +
                                std::to_string(bossesIncluded_.CountEnabled()) + " OF " +
                                std::to_string(kBossPoolModelCount) + " BOSSES");
            }
            // The item-data archive is rewritten once for whichever param
            // features ran, so report it once rather than per feature.
            if (randomizeEnemyDrops_ || randomizeStartingWeapons_ ||
                randomizeStartingGuns_ || randomizeShopWeapons_) {
                AddProgressLine("ITEM DATA " + std::to_string(result.itemDataMembers) +
                                " ENTRIES, WROTE " +
                                std::to_string(result.itemDataWrittenBytes / 1048576) + " MB");
            }
        } else {
            AddProgressLine("ENEMY RANDOMIZATION FAILED: " + result.error);
            failed = true;
        }
        job_.reset();
    }

    // The closing flourish reads as "it worked, go play" - showing it after
    // a failure would be actively misleading, so it's gated on nothing
    // having failed above.
    completionLineStart_ = progressLines_.size();
    if (!failed) {
        AddProgressLine("WHAT ARE YOU STILL DOING HERE");
        AddProgressLine("ENOUGH TREMBLING IN YOUR BOOTS");
        AddProgressLine("A HUNTER MUST HUNT");
    } else {
        AddProgressLine("COMMIT FAILED - CHECK THE LOG FOR DETAILS");
    }

    commitFinished_ = true;
}

void EnableWizardScreen::Draw(Renderer& renderer) {
    switch (step_) {
        case Step::SaveData:      DrawSaveData(renderer); break;
        case Step::SelectReplace: DrawSelectReplace(renderer); break;
        case Step::EditSeed:      DrawEditSeed(renderer); break;
        case Step::EnemyPicker:   DrawEnemyPicker(renderer); break;
        case Step::BossPicker:    DrawBossPicker(renderer); break;
        case Step::Confirm:       DrawConfirm(renderer); break;
        case Step::Progress:      DrawProgress(renderer); break;
    }
}

void EnableWizardScreen::DrawSaveData(Renderer& renderer) {
    renderer.Clear(20, 24, 28);

    DrawCenteredLabel(renderer, 140, "ENABLE RANDOMIZER", kTitleScale, Palette::Heading);
    DrawCenteredLabel(renderer, 260, "SAVE DATA", kItemScale, Palette::Text);

    std::vector<std::string> items = {
        std::string("BACKUP EXISTING SAVE   ") + (backupExistingSave_ ? "YES" : "NO"),
        std::string("REPLACE SAVE   ") + ReplaceDisplayText(),
        std::string("SEED   ") + SeedDisplayText(),
        std::string("RANDOMIZE ENEMIES   ") + (randomizeEnemies_ ? "YES" : "NO"),
        std::string("RANDOMIZE BOSSES   ") + (randomizeBosses_ ? "YES" : "NO"),
        std::string("RANDOMIZE TREASURE   ") + (randomizeTreasure_ ? "YES" : "NO"),
        std::string("RANDOMIZE WORKSHOP TOOLS   ") + (randomizeWorkshopTools_ ? "YES" : "NO"),
        std::string("RANDOMIZE ENEMY DROPS   ") + (randomizeEnemyDrops_ ? "YES" : "NO"),
        std::string("RANDOMIZE STARTING WEAPONS   ") + (randomizeStartingWeapons_ ? "YES" : "NO"),
        std::string("RANDOMIZE STARTING GUNS   ") + (randomizeStartingGuns_ ? "YES" : "NO"),
        std::string("RANDOMIZE SHOP WEAPONS   ") + (randomizeShopWeapons_ ? "YES" : "NO"),
        std::string("ENABLE MERGO DARKNESS   ") + (enableMergoDarkness_ ? "YES" : "NO"),
        std::string("ENEMIES INCLUDED   ") +
            std::to_string(enemiesIncluded_.CountEnabled()) + " OF " +
            std::to_string(kEnemyPoolModelCount),
        std::string("BOSSES INCLUDED   ") +
            std::to_string(bossesIncluded_.CountEnabled()) + " OF " +
            std::to_string(kBossPoolModelCount),
    };
    DrawScrollableList(renderer, kSettingsLayout, items, selected_, scrollOffset_, kItemScale,
                       Palette::Text, Palette::Selected);

    DrawCenteredLabel(renderer, kScreenHeight - 130, "LEFT RIGHT CHANGE   X EDIT SEED",
                      kFooterScale, Palette::Dim);
    DrawCenteredLabel(renderer, kScreenHeight - 80, "OPTIONS NEXT   O BACK", kFooterScale, Palette::Dim);
}

void EnableWizardScreen::DrawSelectReplace(Renderer& renderer) {
    renderer.Clear(20, 24, 28);

    DrawCenteredLabel(renderer, 140, "REPLACE SAVE", kTitleScale, Palette::Heading);

    std::vector<std::string> items;
    for (int i = 0; i < kReplaceOptionCount; i++) items.push_back(kReplaceOptions[i].label);
    DrawMenuList(renderer, 380, 90, items, selected_, kItemScale, Palette::Text, Palette::Selected);

    DrawCenteredLabel(renderer, 380 + kReplaceOptionCount * 90 + 60, kReplaceOptions[selected_].detail,
                       kDetailScale, Palette::Dim);

    DrawCenteredLabel(renderer, kScreenHeight - 80, "X SELECT   O CANCEL", kFooterScale, Palette::Dim);
}

void EnableWizardScreen::DrawEditSeed(Renderer& renderer) {
    renderer.Clear(20, 24, 28);

    DrawCenteredLabel(renderer, 300, "RANDOMIZER SEED", kTitleScale, Palette::Heading);

    // Drawn a character at a time so the one under the cursor can be
    // highlighted - same approach as DrawEditTitleId.
    const int kEditScale = 6;
    int totalWidth = renderer.TextWidth(seedBuf_, kEditScale);
    int perChar    = renderer.TextWidth("A", kEditScale);
    int startX     = (kScreenWidth - totalWidth) / 2;
    int y          = 500;

    for (int i = 0; i < kSeedDigits; i++) {
        char single[2] = { seedBuf_[i], '\0' };
        Color c = (i == seedCursor_) ? Palette::Selected : Palette::Text;
        renderer.DrawText(startX + i * perChar, y, single, kEditScale, c.r, c.g, c.b);
    }

    DrawCenteredLabel(renderer, kScreenHeight - 130, "LEFT RIGHT SELECT   UP DOWN CHANGE",
                      kFooterScale, Palette::Dim);
    DrawCenteredLabel(renderer, kScreenHeight - 80, "X ACCEPT   O CANCEL",
                      kFooterScale, Palette::Dim);
}

void EnableWizardScreen::DrawEnemyPicker(Renderer& renderer) {
    picker_.Draw(renderer, "ENEMIES INCLUDED", EnemyPoolTable().data(),
                 kEnemyPoolModelCount, enemiesIncluded_.enabled);
}

void EnableWizardScreen::DrawBossPicker(Renderer& renderer) {
    picker_.Draw(renderer, "BOSSES INCLUDED", BossPoolTable().data(),
                 kBossPoolModelCount, bossesIncluded_.enabled);
}

void EnableWizardScreen::DrawConfirm(Renderer& renderer) {
    renderer.Clear(20, 24, 28);

    DrawCenteredLabel(renderer, 140, "ENABLE RANDOMIZER", kTitleScale, Palette::Heading);
    DrawCenteredLabel(renderer, 260, "CONFIRM", kItemScale, Palette::Text);

    std::vector<std::string> items = {
        std::string("BACKUP EXISTING SAVE   ") + (backupExistingSave_ ? "YES" : "NO"),
        std::string("REPLACE SAVE   ") + ReplaceDisplayText(),
        std::string("SEED   ") + SeedDisplayText(),
        std::string("RANDOMIZE ENEMIES   ") + (randomizeEnemies_ ? "YES" : "NO"),
        std::string("RANDOMIZE BOSSES   ") + (randomizeBosses_ ? "YES" : "NO"),
        std::string("RANDOMIZE TREASURE   ") + (randomizeTreasure_ ? "YES" : "NO"),
        std::string("RANDOMIZE WORKSHOP TOOLS   ") + (randomizeWorkshopTools_ ? "YES" : "NO"),
        std::string("RANDOMIZE ENEMY DROPS   ") + (randomizeEnemyDrops_ ? "YES" : "NO"),
        std::string("RANDOMIZE STARTING WEAPONS   ") + (randomizeStartingWeapons_ ? "YES" : "NO"),
        std::string("RANDOMIZE STARTING GUNS   ") + (randomizeStartingGuns_ ? "YES" : "NO"),
        std::string("RANDOMIZE SHOP WEAPONS   ") + (randomizeShopWeapons_ ? "YES" : "NO"),
        std::string("ENABLE MERGO DARKNESS   ") + (enableMergoDarkness_ ? "YES" : "NO"),
        std::string("ENEMIES INCLUDED   ") +
            std::to_string(enemiesIncluded_.CountEnabled()) + " OF " +
            std::to_string(kEnemyPoolModelCount),
        std::string("BOSSES INCLUDED   ") +
            std::to_string(bossesIncluded_.CountEnabled()) + " OF " +
            std::to_string(kBossPoolModelCount),
    };
    DrawScrollableList(renderer, kSettingsLayout, items, -1, scrollOffset_, kItemScale,
                       Palette::Text, Palette::Selected);

    DrawCenteredLabel(renderer, kScreenHeight - 130, "UP DOWN SCROLL", kFooterScale, Palette::Dim);
    DrawCenteredLabel(renderer, kScreenHeight - 80, "OPTIONS COMMIT   O BACK", kFooterScale, Palette::Dim);
}

void EnableWizardScreen::DrawProgress(Renderer& renderer) {
    renderer.Clear(20, 24, 28);

    DrawCenteredLabel(renderer, 120, "ENABLE RANDOMIZER", kTitleScale, Palette::Heading);
    DrawCenteredLabel(renderer, 200, "PROGRESS", kItemScale, Palette::Text);

    // The log scrolls, so it draws a window rather than everything. While the
    // run is going the bottom slot belongs to the live status line, which is
    // why the log gets one fewer row then than it does afterwards.
    int slots     = VisibleRowCount(kProgressLayout);
    int logRows   = job_ ? slots - 1 : slots;
    if (logRows < 1) logRows = 1;
    int lineCount = (int)progressLines_.size();

    // Pinned to the tail until the user scrolls away from it. Writing the
    // resolved offset back means a later up/down starts from what's actually
    // on screen rather than from a stale value.
    int offset = progressFollowTail_ ? lineCount - logRows : progressScroll_;
    offset = ClampScroll(offset, lineCount, logRows);
    progressScroll_ = offset;

    // The trailing completion message is colored distinctly to read as
    // "done", not as another status line.
    int drawn = 0;
    for (int row = 0; row < logRows; row++) {
        int index = offset + row;
        if (index >= lineCount) break;
        Color c = ((std::size_t)index >= completionLineStart_) ? Palette::Good : Palette::Text;
        DrawCenteredLabel(renderer, kProgressLayout.firstY + row * kProgressLayout.spacing,
                          progressLines_[(std::size_t)index].c_str(), kProgressScale, c);
        drawn++;
    }

    // While the randomizer is running this is the only line that changes,
    // and it's the whole point of the screen - what's happening right now.
    // It sits directly under the last log line, inside the reserved slot.
    if (job_) {
        std::string live = job_->StatusText() + "  " +
                           std::to_string((int)(job_->Progress() * 100.0f + 0.5f)) + "%";
        DrawCenteredLabel(renderer, kProgressLayout.firstY + drawn * kProgressLayout.spacing,
                          live.c_str(), kProgressScale, Palette::Heading);
    }

    DrawScrollHints(renderer, kProgressLayout, lineCount, offset, logRows);

    if (commitFinished_) {
        DrawCenteredLabel(renderer, kScreenHeight - 130, "UP DOWN SCROLL", kFooterScale,
                          Palette::Dim);
        DrawCenteredLabel(renderer, kScreenHeight - 80, "O RETURN TO MENU", kFooterScale,
                          Palette::Dim);
    }
}

} // namespace bbr
