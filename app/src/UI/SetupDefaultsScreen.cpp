#include "SetupDefaultsScreen.h"

#include "Controls.h"

#include "../Platform/Log.h"
#include "../Randomizer/RandomizerDefaultsStore.h"

#include <string.h>

#include <string>
#include <vector>

namespace bbr {

namespace {
const int kTitleScale  = 5;
const int kItemScale   = 4;
const int kFooterScale = 3;

// The list runs from under the heading down to just above the "MORE BELOW"
// hint, which itself has to clear the footer at kScreenHeight - 130. Starting
// at 300 rather than 340 uses dead space the heading wasn't occupying and is
// worth a whole extra visible row (7 of 10 instead of 6).
const ListLayout kListLayout = { 300, 90, 870 };
const int kEditScale   = 6;

char CycleLetter(char c, int dir) {
    int idx = ((c - 'A') + dir + 26) % 26;
    return (char)('A' + idx);
}

char CycleDigit(char c, int dir) {
    int idx = ((c - '0') + dir + 10) % 10;
    return (char)('0' + idx);
}
} // namespace

void SetupDefaultsScreen::Update(const ButtonEdges& input) {
    if (mode_ == Mode::EnemyPicker) {
        if (picker_.Update(input, kEnemiesIncludedStrings, EnemyPoolTable().data(),
                           kEnemyPoolModelCount,
                           working_.enemiesIncluded.enabled)) mode_ = Mode::List;
        return;
    }
    if (mode_ == Mode::SkipPicker) {
        if (picker_.Update(input, kEnemiesSkippedStrings, EnemySkipTable().data(),
                           kEnemySkipModelCount,
                           working_.enemiesSkipped.enabled)) mode_ = Mode::List;
        return;
    }
    if (mode_ == Mode::BossPicker) {
        if (picker_.Update(input, kBossesIncludedStrings, BossPoolTable().data(),
                           kBossPoolModelCount,
                           working_.bossesIncluded.enabled)) mode_ = Mode::List;
        return;
    }
    if (mode_ == Mode::List) UpdateList(input);
    else                     UpdateEditTitleId(input);
}

std::string SetupDefaultsScreen::EnemiesIncludedText() const {
    return std::to_string(working_.enemiesIncluded.CountEnabled()) + " OF " +
           std::to_string(kEnemyPoolModelCount);
}

std::string SetupDefaultsScreen::EnemiesSkippedText() const {
    return std::to_string(working_.enemiesSkipped.CountEnabled()) + " OF " +
           std::to_string(kEnemySkipModelCount);
}

std::string SetupDefaultsScreen::BossesIncludedText() const {
    return std::to_string(working_.bossesIncluded.CountEnabled()) + " OF " +
           std::to_string(kBossPoolModelCount);
}

void SetupDefaultsScreen::ToggleRow(int row) {
    if (row == kBackupRow) {
        working_.backupExistingSaveData = !working_.backupExistingSaveData;
        Log(working_.backupExistingSaveData ? "defaults: backup existing save = YES"
                                             : "defaults: backup existing save = NO");
    } else if (row == kReplaceDefaultRow) {
        working_.replaceSaveDefaultIsNew = !working_.replaceSaveDefaultIsNew;
        Log(working_.replaceSaveDefaultIsNew ? "defaults: replace save default = NEW SAVE DATA"
                                              : "defaults: replace save default = LEAVE EXISTING SAVE DATA");
    } else if (row == kRandomizeEnemiesRow) {
        working_.randomizeEnemies = !working_.randomizeEnemies;
        Log(working_.randomizeEnemies ? "defaults: randomize enemies = YES"
                                       : "defaults: randomize enemies = NO");
    } else if (row == kRandomizeBossesRow) {
        working_.randomizeBosses = !working_.randomizeBosses;
        Log(working_.randomizeBosses ? "defaults: randomize bosses = YES"
                                      : "defaults: randomize bosses = NO");
    } else if (row == kRandomizeTreasureRow) {
        working_.randomizeTreasure = !working_.randomizeTreasure;
        Log(working_.randomizeTreasure ? "defaults: randomize treasure = YES"
                                        : "defaults: randomize treasure = NO");
    } else if (row == kRandomizeWorkshopToolsRow) {
        working_.randomizeWorkshopTools = !working_.randomizeWorkshopTools;
        Log(working_.randomizeWorkshopTools ? "defaults: randomize workshop tools = YES"
                                             : "defaults: randomize workshop tools = NO");
    } else if (row == kRandomizeDropsRow) {
        working_.randomizeEnemyDrops = !working_.randomizeEnemyDrops;
        Log(working_.randomizeEnemyDrops ? "defaults: randomize enemy drops = YES"
                                          : "defaults: randomize enemy drops = NO");
    } else if (row == kStartingWeaponsRow) {
        working_.randomizeStartingWeapons = !working_.randomizeStartingWeapons;
        Log(working_.randomizeStartingWeapons ? "defaults: randomize starting weapons = YES"
                                               : "defaults: randomize starting weapons = NO");
    } else if (row == kStartingGunsRow) {
        working_.randomizeStartingGuns = !working_.randomizeStartingGuns;
        Log(working_.randomizeStartingGuns ? "defaults: randomize starting guns = YES"
                                            : "defaults: randomize starting guns = NO");
    } else if (row == kShopWeaponsRow) {
        working_.randomizeShopWeapons = !working_.randomizeShopWeapons;
        Log(working_.randomizeShopWeapons ? "defaults: randomize shop weapons = YES"
                                           : "defaults: randomize shop weapons = NO");
    } else if (row == kDisableMergoDarknessRow) {
        working_.enableMergoDarkness = !working_.enableMergoDarkness;
        Log(working_.enableMergoDarkness ? "defaults: enable mergo darkness = YES"
                                           : "defaults: enable mergo darkness = NO");
    } else if (row == kDoNotRandomizeCagedDogsRow) {
        working_.doNotRandomizeCagedDogs = !working_.doNotRandomizeCagedDogs;
        Log(working_.doNotRandomizeCagedDogs ? "defaults: do not randomize caged dogs = YES"
                                              : "defaults: do not randomize caged dogs = NO");
    } else if (row == kStartWithHunterToolsRow) {
        working_.startWithHunterTools = !working_.startWithHunterTools;
        Log(working_.startWithHunterTools ? "defaults: start with hunter tools = YES"
                                           : "defaults: start with hunter tools = NO");
    } else if (row == kEasyShadowsRow) {
        working_.easyShadows = !working_.easyShadows;
        Log(working_.easyShadows ? "defaults: easy shadows = YES"
                                  : "defaults: easy shadows = NO");
    } else if (row == kEasyRomRow) {
        working_.easyRom = !working_.easyRom;
        Log(working_.easyRom ? "defaults: easy rom = YES"
                              : "defaults: easy rom = NO");
    } else if (row == kEasyFailuresRow) {
        working_.easyFailures = !working_.easyFailures;
        Log(working_.easyFailures ? "defaults: easy failures = YES"
                                   : "defaults: easy failures = NO");
    } else if (row == kEasyEmissaryRow) {
        working_.easyEmissary = !working_.easyEmissary;
        Log(working_.easyEmissary ? "defaults: easy emissary = YES"
                                   : "defaults: easy emissary = NO");
    }
}

void SetupDefaultsScreen::UpdateList(const ButtonEdges& input) {
    selected_ = NavigateVertical(selected_, kItemCount, input);
    scrollOffset_ = ScrollToShow(scrollOffset_, selected_, kItemCount,
                                 VisibleRowCount(kListLayout));

    // X always means "activate whatever's highlighted" - toggle for the
    // boolean-shaped rows, drill in for the title ID and enemy picker rows.
    // Left/right also toggles a boolean row directly, same as every other
    // boolean row in this app, but does nothing on the two drill-in rows:
    // there is nothing there to cycle through. Saving is Options, not X -
    // see ButtonEdges::options.
    if ((input.left || input.right) &&
        selected_ != kTitleIdRow && selected_ != kEnemiesIncludedRow &&
        selected_ != kEnemiesSkippedRow && selected_ != kBossesIncludedRow) {
        ToggleRow(selected_);
    }

    if (input.cross) {
        if (selected_ == kTitleIdRow) {
            // Seed the editor from whatever's already set, if it's the
            // right shape; otherwise leave editBuf_ at its current/default
            // value untouched.
            const std::string& src = working_.bloodborneTitleId;
            if ((int)src.size() == kTitleIdLen) {
                memcpy(editBuf_, src.c_str(), kTitleIdLen);
                editBuf_[kTitleIdLen] = '\0';
            }
            cursor_ = 0;
            mode_ = Mode::EditTitleId;
        } else if (selected_ == kEnemiesIncludedRow) {
            picker_.Reset();
            mode_ = Mode::EnemyPicker;
        } else if (selected_ == kEnemiesSkippedRow) {
            picker_.Reset();
            mode_ = Mode::SkipPicker;
        } else if (selected_ == kBossesIncludedRow) {
            picker_.Reset();
            mode_ = Mode::BossPicker;
        } else {
            ToggleRow(selected_);
        }
    }

    if (input.options) {
        defaults_ = working_;
        SaveRandomizerDefaults(defaults_);
        Log("defaults: saved");
        requestedScreen_ = ScreenId::Menu;
    }

    if (input.circle) {
        Log("defaults: cancelled - no changes saved");
        requestedScreen_ = ScreenId::Menu;
    }
}

void SetupDefaultsScreen::UpdateEditTitleId(const ButtonEdges& input) {
    // Left/right moves which character is being edited (that's the actual
    // left-to-right motion); up/down changes the highlighted character -
    // swapped from the first pass per testing feedback.
    if (input.left)  cursor_ = (cursor_ + kTitleIdLen - 1) % kTitleIdLen;
    if (input.right) cursor_ = (cursor_ + 1) % kTitleIdLen;

    if (input.up || input.down) {
        int dir = input.up ? 1 : -1;
        if (cursor_ < 4) editBuf_[cursor_] = CycleLetter(editBuf_[cursor_], dir);
        else             editBuf_[cursor_] = CycleDigit(editBuf_[cursor_], dir);
    }

    if (input.cross) {
        working_.bloodborneTitleId = editBuf_;
        Log((std::string("defaults: title id set to ") + editBuf_).c_str());
        mode_ = Mode::List;
    }

    if (input.circle) {
        Log("defaults: title id edit cancelled");
        mode_ = Mode::List;
    }
}

void SetupDefaultsScreen::Draw(Renderer& renderer) {
    if (mode_ == Mode::EnemyPicker) {
        picker_.Draw(renderer, kEnemiesIncludedStrings, EnemyPoolTable().data(),
                     kEnemyPoolModelCount, working_.enemiesIncluded.enabled);
    } else if (mode_ == Mode::SkipPicker) {
        picker_.Draw(renderer, kEnemiesSkippedStrings, EnemySkipTable().data(),
                     kEnemySkipModelCount, working_.enemiesSkipped.enabled);
    } else if (mode_ == Mode::BossPicker) {
        picker_.Draw(renderer, kBossesIncludedStrings, BossPoolTable().data(),
                     kBossPoolModelCount, working_.bossesIncluded.enabled);
    } else if (mode_ == Mode::List) {
        DrawList(renderer);
    } else {
        DrawEditTitleId(renderer);
    }
}

void SetupDefaultsScreen::DrawList(Renderer& renderer) {
    renderer.Clear(20, 24, 28);

    DrawCenteredLabel(renderer, 180, "SETUP DEFAULTS", kTitleScale, Palette::Heading);

    std::string titleIdDisplay = working_.bloodborneTitleId.empty() ? "NOT SET" : working_.bloodborneTitleId;
    std::vector<std::string> items = {
        std::string("BLOODBORNE TITLE ID   ") + titleIdDisplay,
        std::string("BACKUP EXISTING SAVE   ") + (working_.backupExistingSaveData ? "YES" : "NO"),
        std::string("DEFAULT REPLACE SAVE   ")
            + (working_.replaceSaveDefaultIsNew ? "NEW SAVE DATA" : "LEAVE EXISTING SAVE DATA"),
        std::string("RANDOMIZE ENEMIES   ") + (working_.randomizeEnemies ? "YES" : "NO"),
        std::string("RANDOMIZE BOSSES   ") + (working_.randomizeBosses ? "YES" : "NO"),
        std::string("RANDOMIZE TREASURE   ") + (working_.randomizeTreasure ? "YES" : "NO"),
        std::string("RANDOMIZE WORKSHOP TOOLS   ") + (working_.randomizeWorkshopTools ? "YES" : "NO"),
        std::string("RANDOMIZE ENEMY DROPS   ") + (working_.randomizeEnemyDrops ? "YES" : "NO"),
        std::string("RANDOMIZE STARTING WEAPONS   ") + (working_.randomizeStartingWeapons ? "YES" : "NO"),
        std::string("RANDOMIZE STARTING GUNS   ") + (working_.randomizeStartingGuns ? "YES" : "NO"),
        std::string("RANDOMIZE SHOP WEAPONS   ") + (working_.randomizeShopWeapons ? "YES" : "NO"),
        std::string("ENABLE MERGO DARKNESS   ") + (working_.enableMergoDarkness ? "YES" : "NO"),
        std::string("ENEMIES INCLUDED   ") + EnemiesIncludedText(),
        // Position 13, matching kEnemiesSkippedRow - see the header. Directly
        // after ENEMIES INCLUDED, which it is the opposite of.
        std::string("ENEMIES SKIPPED   ") + EnemiesSkippedText(),
        std::string("BOSSES INCLUDED   ") + BossesIncludedText(),
        // Position 15, matching kDoNotRandomizeCagedDogsRow - appended last,
        // see the header.
        std::string("DO NOT RANDOMIZE CAGED DOGS   ")
            + (working_.doNotRandomizeCagedDogs ? "YES" : "NO"),
        // Position 16, matching kStartWithHunterToolsRow - appended last,
        // see the header.
        std::string("START WITH HUNTER TOOLS   ")
            + (working_.startWithHunterTools ? "YES" : "NO"),
        // Positions 17-20, matching kEasyShadowsRow, kEasyRomRow,
        // kEasyFailuresRow and kEasyEmissaryRow - appended last, in the same
        // order as the constants, see the header.
        std::string("EASY SHADOWS   ") + (working_.easyShadows ? "YES" : "NO"),
        std::string("EASY ROM   ") + (working_.easyRom ? "YES" : "NO"),
        std::string("EASY FAILURES   ") + (working_.easyFailures ? "YES" : "NO"),
        std::string("EASY EMISSARY   ") + (working_.easyEmissary ? "YES" : "NO"),
    };
    DrawScrollableList(renderer, kListLayout, items, selected_, scrollOffset_, kItemScale,
                       Palette::Text, Palette::Selected);

    DrawCenteredLabel(renderer, kScreenHeight - 130, "LEFT RIGHT X TOGGLE EDIT", kFooterScale, Palette::Dim);
    DrawCenteredLabel(renderer, kScreenHeight - 80, "OPTIONS SAVE   O BACK", kFooterScale, Palette::Dim);
}

void SetupDefaultsScreen::DrawEditTitleId(Renderer& renderer) {
    renderer.Clear(20, 24, 28);

    DrawCenteredLabel(renderer, 300, "BLOODBORNE TITLE ID", kTitleScale, Palette::Heading);

    int totalWidth = renderer.TextWidth(editBuf_, kEditScale);
    int perChar    = renderer.TextWidth("A", kEditScale);
    int startX     = (kScreenWidth - totalWidth) / 2;
    int y          = 460;

    char single[2] = { 0, 0 };
    for (int i = 0; i < kTitleIdLen; i++) {
        single[0] = editBuf_[i];
        Color c = (i == cursor_) ? Palette::Selected : Palette::Text;
        renderer.DrawText(startX + i * perChar, y, single, kEditScale, c.r, c.g, c.b);
    }

    DrawCenteredLabel(renderer, kScreenHeight - 130, "LEFT RIGHT SELECT   UP DOWN CHANGE", kFooterScale, Palette::Dim);
    DrawCenteredLabel(renderer, kScreenHeight - 80, "X SAVE   O CANCEL", kFooterScale, Palette::Dim);
}

} // namespace bbr
