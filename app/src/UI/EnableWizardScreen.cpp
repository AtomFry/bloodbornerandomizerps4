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
// These indices are also the POSITION of each entry in the DrawSaveData and
// DrawConfirm `items` vectors, which Controls.cpp's DrawMenuList pairs with
// `selected == i` - an entry added to either list out of order compiles,
// passes ui_scroll_verify.py and mislabels every row below it.
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
// Drill-ins, not toggles: 82 and 85 rows cannot live inline.
// ENEMIES SKIPPED sits directly after ENEMIES INCLUDED (feature 032 P16) -
// the two enemy lists differ by one word and must be read side by side.
// UNCHANGED BELL MAIDENS used to be row 4; D1 retired it, every row below it
// moved up one, and the new row lands here. Still 15 rows in total, so no
// geometry moves and ui_scroll_verify.py's 15-row entries stand.
const int kEnemiesIncludedRow  = 12;
const int kEnemiesSkippedRow   = 13;
const int kBossesIncludedRow   = 14;
// Appended last, and deliberately not moved up next to RANDOMIZE ENEMIES, the
// toggle it modifies: going last is what guarantees no existing row index
// moves and so no existing row can be mislabelled (feature 033 P5, P13).
const int kDoNotRandomizeCagedDogsRow = 15;
// Appended last for the same reason, and deliberately NOT placed next to
// RANDOMIZE WORKSHOP TOOLS: they are different features on the same two items
// (that one shuffles them into the treasure pool, this one grants them at
// character creation) and inserting there would shift every row below it.
// kSaveDataRowCount goes with it, 16 -> 17, along with ui_scroll_verify.py.
const int kStartWithHunterToolsRow = 16;
// The four easy-mode settings (feature 018), appended last for the same
// reason as the two rows above: an index added at the END cannot mislabel an
// existing row, and both list vectors below must gain their entries in the
// same order and in the same place. Backlog order - rows 18, 19, 20, 21 of
// docs/randomization-feature-spec.md §6 (plan 018 P1). kSaveDataRowCount goes
// with them, 17 -> 21, along with ui_scroll_verify.py.
const int kEasyShadowsRow      = 17;
const int kEasyRomRow          = 18;
const int kEasyFailuresRow     = 19;
const int kEasyEmissaryRow     = 20;
const int kSaveDataRowCount    = 21;

// The four strings feature 032 puts on the commit screen, named rather than
// inlined so pool_verify.py selftest case 6 can parse them out of this file
// and assert what they have to satisfy.
//
// Font8x8.cpp's glyph table is 42 characters - A-Z, 0-9, space and ' ( ) - ,
// and nothing else. There is no lowercase and no ':'. DrawText8x8 advances
// the cursor for a character it cannot render, so an unrenderable one is a
// full-width BLANK column, not a missing one: a line can be well inside every
// width budget and still show the player nothing. Hence the prefix's ' - ',
// which is already this screen's separator elsewhere.
//
// Budget: 71 characters fit a line at scale 3. The prefix takes 29, leaving
// 42 for whatever EnemyRandomizer's Fail() put in result.error - which is why
// those two messages are named constants over there as well.
const char* const kEnemyFailPrefix = "ENEMY RANDOMIZATION FAILED - ";
const char* const kPoolFellBackLine1 = "ALL SELECTED ENEMIES WERE ALSO SKIPPED";
const char* const kPoolFellBackLine2 = "SKIPPED ENEMIES WERE USED AS REPLACEMENTS FOR THIS RUN";
const char* const kNothingRandomizedLine = "NO ENEMIES WERE RANDOMIZED - EVERY ENEMY WAS SKIPPED";

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
      doNotRandomizeCagedDogs_(defaults.doNotRandomizeCagedDogs),
      startWithHunterTools_(defaults.startWithHunterTools),
      easyShadows_(defaults.easyShadows),
      easyRom_(defaults.easyRom),
      easyFailures_(defaults.easyFailures),
      easyEmissary_(defaults.easyEmissary),
      enemiesIncluded_(defaults.enemiesIncluded),
      bossesIncluded_(defaults.bossesIncluded),
      enemiesSkipped_(defaults.enemiesSkipped),
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
        case Step::SkipPicker:    UpdateSkipPicker(input); break;
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
    // No left/right branch for the three drill-in rows - they are not
    // toggles, so there is nothing to cycle through.
    if ((input.left || input.right) && selected_ == kDisableMergoDarknessRow) {
        enableMergoDarkness_ = !enableMergoDarkness_;
        Log(enableMergoDarkness_ ? "enable wizard: enable mergo darkness = YES"
                                   : "enable wizard: enable mergo darkness = NO");
    }
    if ((input.left || input.right) && selected_ == kDoNotRandomizeCagedDogsRow) {
        doNotRandomizeCagedDogs_ = !doNotRandomizeCagedDogs_;
        Log(doNotRandomizeCagedDogs_ ? "enable wizard: do not randomize caged dogs = YES"
                                      : "enable wizard: do not randomize caged dogs = NO");
    }
    if ((input.left || input.right) && selected_ == kStartWithHunterToolsRow) {
        startWithHunterTools_ = !startWithHunterTools_;
        Log(startWithHunterTools_ ? "enable wizard: start with hunter tools = YES"
                                   : "enable wizard: start with hunter tools = NO");
    }
    if ((input.left || input.right) && selected_ == kEasyShadowsRow) {
        easyShadows_ = !easyShadows_;
        Log(easyShadows_ ? "enable wizard: easy shadows = YES"
                          : "enable wizard: easy shadows = NO");
    }
    if ((input.left || input.right) && selected_ == kEasyRomRow) {
        easyRom_ = !easyRom_;
        Log(easyRom_ ? "enable wizard: easy rom = YES"
                      : "enable wizard: easy rom = NO");
    }
    if ((input.left || input.right) && selected_ == kEasyFailuresRow) {
        easyFailures_ = !easyFailures_;
        Log(easyFailures_ ? "enable wizard: easy failures = YES"
                           : "enable wizard: easy failures = NO");
    }
    if ((input.left || input.right) && selected_ == kEasyEmissaryRow) {
        easyEmissary_ = !easyEmissary_;
        Log(easyEmissary_ ? "enable wizard: easy emissary = YES"
                           : "enable wizard: easy emissary = NO");
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
        } else if (selected_ == kEnemiesSkippedRow) {
            picker_.Reset();
            GoToStep(Step::SkipPicker);
        } else if (selected_ == kBossesIncludedRow) {
            picker_.Reset();
            Log("enable wizard: opening boss picker");
            GoToStep(Step::BossPicker);
        } else if (selected_ == kDoNotRandomizeCagedDogsRow) {
            doNotRandomizeCagedDogs_ = !doNotRandomizeCagedDogs_;
            Log(doNotRandomizeCagedDogs_ ? "enable wizard: do not randomize caged dogs = YES"
                                          : "enable wizard: do not randomize caged dogs = NO");
        } else if (selected_ == kStartWithHunterToolsRow) {
            startWithHunterTools_ = !startWithHunterTools_;
            Log(startWithHunterTools_ ? "enable wizard: start with hunter tools = YES"
                                       : "enable wizard: start with hunter tools = NO");
        } else if (selected_ == kEasyShadowsRow) {
            easyShadows_ = !easyShadows_;
            Log(easyShadows_ ? "enable wizard: easy shadows = YES"
                              : "enable wizard: easy shadows = NO");
        } else if (selected_ == kEasyRomRow) {
            easyRom_ = !easyRom_;
            Log(easyRom_ ? "enable wizard: easy rom = YES"
                          : "enable wizard: easy rom = NO");
        } else if (selected_ == kEasyFailuresRow) {
            easyFailures_ = !easyFailures_;
            Log(easyFailures_ ? "enable wizard: easy failures = YES"
                               : "enable wizard: easy failures = NO");
        } else if (selected_ == kEasyEmissaryRow) {
            easyEmissary_ = !easyEmissary_;
            Log(easyEmissary_ ? "enable wizard: easy emissary = YES"
                               : "enable wizard: easy emissary = NO");
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
    if (picker_.Update(input, kEnemiesIncludedStrings, EnemyPoolTable().data(),
                       kEnemyPoolModelCount, enemiesIncluded_.enabled)) {
        ReturnFromPicker(kEnemiesIncludedRow);
    }
}

void EnableWizardScreen::UpdateSkipPicker(const ButtonEdges& input) {
    if (picker_.Update(input, kEnemiesSkippedStrings, EnemySkipTable().data(),
                       kEnemySkipModelCount, enemiesSkipped_.enabled)) {
        ReturnFromPicker(kEnemiesSkippedRow);
    }
}

void EnableWizardScreen::UpdateBossPicker(const ButtonEdges& input) {
    if (picker_.Update(input, kBossesIncludedStrings, BossPoolTable().data(),
                       kBossPoolModelCount, bossesIncluded_.enabled)) {
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
    // The four easy-mode settings are in this list for the same reason
    // enableMergoDarkness_ is: each one writes real map files on its own and
    // needs no other feature to mean anything, so ticking one alone must
    // start a run. They are deliberately NOT gated on randomizeEnemies_.
    if (randomizeEnemies_ || randomizeBosses_ || randomizeTreasure_ ||
        randomizeEnemyDrops_ || randomizeStartingWeapons_ ||
        randomizeStartingGuns_ || randomizeShopWeapons_ ||
        enableMergoDarkness_ || startWithHunterTools_ ||
        easyShadows_ || easyRom_ || easyFailures_ || easyEmissary_) {
        std::string outputDir = "/data/GoldHEN/AFR/" + titleId_ + "/dvdroot_ps4";
        EnemyRandomizerOptions options;
        options.randomizeEnemies = randomizeEnemies_;
        options.randomizeBosses = randomizeBosses_;
        options.randomizeTreasure = randomizeTreasure_;
        // Meaningless without randomizeTreasure (D3), so it deliberately does
        // NOT appear in the big || above - ticking it alone must not start a
        // run that does nothing.
        // Meaningless without randomizeEnemies for the same reason (D3), so it
        // is likewise absent from the big || above.
        options.randomizeWorkshopTools = randomizeWorkshopTools_;
        options.randomizeEnemyDrops = randomizeEnemyDrops_;
        options.randomizeStartingWeapons = randomizeStartingWeapons_;
        options.randomizeStartingGuns = randomizeStartingGuns_;
        options.randomizeShopWeapons = randomizeShopWeapons_;
        options.enableMergoDarkness = enableMergoDarkness_;
        // Meaningless without randomizeEnemies (spec 033 B10), exactly like
        // randomizeWorkshopTools above, so it too is absent from the big ||:
        // ticking it alone must not start a run that does nothing. It also
        // gets no SKIPPING line - NO is simply the run this app already made.
        options.doNotRandomizeCagedDogs = doNotRandomizeCagedDogs_;
        // IS in the big || above, unlike the two modifiers either side of it:
        // this one changes the game on its own - it needs no other feature to
        // mean anything - so ticking it alone must build a tree, exactly like
        // enableMergoDarkness_.
        options.startWithHunterTools = startWithHunterTools_;
        // Four independent settings on one struct, none of them a
        // randomizer and none of them a modifier on another feature - they
        // apply whether or not anything else above is on, and they are the
        // last writer of the placements they touch. See EasyModes.h.
        options.easyModes.shadows = easyShadows_;
        options.easyModes.rom = easyRom_;
        options.easyModes.failures = easyFailures_;
        options.easyModes.emissary = easyEmissary_;
        options.enemiesIncluded = enemiesIncluded_;
        options.bossesIncluded = bossesIncluded_;
        options.enemiesSkipped = enemiesSkipped_;
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
                // D5 again: extend this line rather than add a SKIPPING one.
                // No parentheses - the 8x8 font has no punctuation glyphs.
                std::string line = "RANDOMIZED " + std::to_string(result.enemiesRandomized) +
                                   " ENEMIES ACROSS " + std::to_string(result.mapsProcessed) +
                                   " MAPS";
                AddProgressLine(line);

                // Feature 032 D4. Everything the run was allowed to draw was
                // also something it was told to leave alone, so the pool half
                // of that instruction yielded. Said in two short lines rather
                // than one long one, matching the shape StartCommit already
                // uses for NO ENEMIES SELECTED. Without it the run silently
                // contradicts the setting.
                if (result.poolFellBack) {
                    AddProgressLine(kPoolFellBackLine1);
                    AddProgressLine(kPoolFellBackLine2);
                }
                // The every-row-skipped case: the run succeeded and genuinely
                // changed nothing, which a bare success line would not say.
                if (result.enemiesRandomized == 0) {
                    AddProgressLine(kNothingRandomizedLine);
                }
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
            // Reported as a state rather than a count, deliberately: the
            // honest count covers both blocks of origin rows (HunterTools.cpp)
            // and a player who picks one origin would read 22 as a defect. No
            // SKIPPING counterpart, for the same reason enableMergoDarkness
            // has none - NO means the app left the file alone.
            if (startWithHunterTools_) {
                AddProgressLine("STARTING WITH BOTH HUNTER WORKSHOP TOOLS");
            }
            // One line per ENABLED easy setting, each carrying its count
            // (plan 018 P2). No SKIPPING counterparts, for the same reason
            // the two features above have none. The counts are fixed - 2 /
            // 60 / 3 / 14 - so a 0 or a wrong number here means a pattern
            // list or a map name is wrong. EASY ROM reads 60 and EASY
            // EMISSARY 14 because this port writes both map variants of
            // those two areas.
            if (easyShadows_) {
                AddProgressLine("EASY SHADOWS REPLACED " +
                                std::to_string(result.easyCounts.shadows) + " PLACEMENTS");
            }
            if (easyRom_) {
                AddProgressLine("EASY ROM REPLACED " +
                                std::to_string(result.easyCounts.rom) + " PLACEMENTS");
            }
            if (easyFailures_) {
                AddProgressLine("EASY FAILURES REPLACED " +
                                std::to_string(result.easyCounts.failures) + " PLACEMENTS");
            }
            if (easyEmissary_) {
                AddProgressLine("EASY EMISSARY REPLACED " +
                                std::to_string(result.easyCounts.emissary) + " PLACEMENTS");
            }
            // Only when it did something - an all-enabled pool is the default
            // and saying so every run is noise.
            if (randomizeEnemies_ && !enemiesIncluded_.AllEnabled()) {
                AddProgressLine("ENEMY POOL LIMITED TO " +
                                std::to_string(enemiesIncluded_.CountEnabled()) + " OF " +
                                std::to_string(kEnemyPoolModelCount) + " ENEMIES");
            }
            // Only when it did something: nothing skipped is the default and
            // saying so every run is noise, exactly like the pool line above.
            if (randomizeEnemies_ && enemiesSkipped_.CountEnabled() > 0) {
                AddProgressLine(std::to_string(enemiesSkipped_.CountEnabled()) + " OF " +
                                std::to_string(kEnemySkipModelCount) +
                                " ENEMIES SKIPPED");
            }
            if (randomizeBosses_ && !bossesIncluded_.AllEnabled()) {
                AddProgressLine("BOSS POOL LIMITED TO " +
                                std::to_string(bossesIncluded_.CountEnabled()) + " OF " +
                                std::to_string(kBossPoolModelCount) + " BOSSES");
            }
            // The item-data archive is rewritten once for whichever param
            // features ran, so report it once rather than per feature.
            if (randomizeEnemyDrops_ || randomizeStartingWeapons_ ||
                randomizeStartingGuns_ || randomizeShopWeapons_ ||
                startWithHunterTools_) {
                AddProgressLine("ITEM DATA " + std::to_string(result.itemDataMembers) +
                                " ENTRIES, WROTE " +
                                std::to_string(result.itemDataWrittenBytes / 1048576) + " MB");
            }
        } else {
            AddProgressLine(kEnemyFailPrefix + result.error);
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
        case Step::SkipPicker:    DrawSkipPicker(renderer); break;
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
        // Position 13, matching kEnemiesSkippedRow. DrawSaveData's and
        // DrawConfirm's lists must stay identical in shape.
        std::string("ENEMIES SKIPPED   ") +
            std::to_string(enemiesSkipped_.CountEnabled()) + " OF " +
            std::to_string(kEnemySkipModelCount),
        std::string("BOSSES INCLUDED   ") +
            std::to_string(bossesIncluded_.CountEnabled()) + " OF " +
            std::to_string(kBossPoolModelCount),
        // Position 15, matching kDoNotRandomizeCagedDogsRow - appended last,
        // see the constant. DrawSaveData's and DrawConfirm's lists must stay
        // identical in shape.
        std::string("DO NOT RANDOMIZE CAGED DOGS   ")
            + (doNotRandomizeCagedDogs_ ? "YES" : "NO"),
        // Position 16, matching kStartWithHunterToolsRow - appended last, see
        // the constant. DrawSaveData's and DrawConfirm's lists must stay
        // identical in shape.
        std::string("START WITH HUNTER TOOLS   ")
            + (startWithHunterTools_ ? "YES" : "NO"),
        // Positions 17-20, matching kEasyShadowsRow, kEasyRomRow,
        // kEasyFailuresRow and kEasyEmissaryRow - appended last, in the same
        // order as the constants. DrawSaveData's and DrawConfirm's lists must
        // stay identical in shape.
        std::string("EASY SHADOWS   ") + (easyShadows_ ? "YES" : "NO"),
        std::string("EASY ROM   ") + (easyRom_ ? "YES" : "NO"),
        std::string("EASY FAILURES   ") + (easyFailures_ ? "YES" : "NO"),
        std::string("EASY EMISSARY   ") + (easyEmissary_ ? "YES" : "NO"),
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
    picker_.Draw(renderer, kEnemiesIncludedStrings, EnemyPoolTable().data(),
                 kEnemyPoolModelCount, enemiesIncluded_.enabled);
}

void EnableWizardScreen::DrawSkipPicker(Renderer& renderer) {
    picker_.Draw(renderer, kEnemiesSkippedStrings, EnemySkipTable().data(),
                 kEnemySkipModelCount, enemiesSkipped_.enabled);
}

void EnableWizardScreen::DrawBossPicker(Renderer& renderer) {
    picker_.Draw(renderer, kBossesIncludedStrings, BossPoolTable().data(),
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
        // Position 13, matching kEnemiesSkippedRow. DrawSaveData's and
        // DrawConfirm's lists must stay identical in shape.
        std::string("ENEMIES SKIPPED   ") +
            std::to_string(enemiesSkipped_.CountEnabled()) + " OF " +
            std::to_string(kEnemySkipModelCount),
        std::string("BOSSES INCLUDED   ") +
            std::to_string(bossesIncluded_.CountEnabled()) + " OF " +
            std::to_string(kBossPoolModelCount),
        // Position 15, matching kDoNotRandomizeCagedDogsRow - appended last,
        // see the constant. DrawSaveData's and DrawConfirm's lists must stay
        // identical in shape.
        std::string("DO NOT RANDOMIZE CAGED DOGS   ")
            + (doNotRandomizeCagedDogs_ ? "YES" : "NO"),
        // Position 16, matching kStartWithHunterToolsRow - appended last, see
        // the constant. DrawSaveData's and DrawConfirm's lists must stay
        // identical in shape.
        std::string("START WITH HUNTER TOOLS   ")
            + (startWithHunterTools_ ? "YES" : "NO"),
        // Positions 17-20, matching kEasyShadowsRow, kEasyRomRow,
        // kEasyFailuresRow and kEasyEmissaryRow - appended last, in the same
        // order as the constants. DrawSaveData's and DrawConfirm's lists must
        // stay identical in shape.
        std::string("EASY SHADOWS   ") + (easyShadows_ ? "YES" : "NO"),
        std::string("EASY ROM   ") + (easyRom_ ? "YES" : "NO"),
        std::string("EASY FAILURES   ") + (easyFailures_ ? "YES" : "NO"),
        std::string("EASY EMISSARY   ") + (easyEmissary_ ? "YES" : "NO"),
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
