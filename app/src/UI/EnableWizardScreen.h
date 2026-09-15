// EnableWizardScreen.h - the ENABLE RANDOMIZER wizard. This slice: Screen 1
// (Save Data, including a first real randomizer setting - RANDOMIZE ENEMIES -
// tacked on ahead of the full Screen 2/2A/2B seed-and-settings hierarchy the
// blueprint describes, a deliberate shortcut to get one real setting through
// the whole pipeline), Screen 1A (Select Replace Save), a Confirm screen,
// and a Progress screen.
//
// Progress now does one real step - RANDOMIZE ENEMIES calls into
// Randomizer/EnemyRandomizer.cpp for real, reading vanilla map files from
// kVanillaSourceDir and writing the randomized ones into
// /data/GoldHEN/AFR/<titleId>/dvdroot_ps4/map/mapstudio/ - and everything
// else (Save Data backup/restore) is still simulated: Log()'d and shown on
// screen, nothing touches real Save Data yet. That's deliberate, matching
// the user's own call to prioritize the AFR/randomizer path first: replace
// each simulated step with a real one, one at a time.
//
// Screen 1 shows one summary row for the replace choice ("REPLACE SAVE:
// <current choice>"); Screen 1A is where it's actually picked, from a
// single unified list: NEW SAVE DATA, LEAVE EXISTING SAVE DATA, and then
// whatever randomizer save backups exist. Those backups are stub entries
// today - Save Data backup isn't a real capability yet - but they
// represent real future data: the app's own randomizer-save backups,
// created when BACKUP EXISTING SAVE is YES at commit time. This proves the
// navigation shape, not real backups.
//
// One Screen, not a screen-per-step - same reasoning as every other
// internal-step screen in this app (see SetupDefaultsScreen.h): a real
// screen stack isn't justified yet, and this way Screen 1's in-progress
// choices survive every trip into Screen 1A/Confirm/Progress for free.
#pragma once

#include "ModelPicker.h"
#include "Screen.h"

#include "../Randomizer/EnemyRandomizer.h"
#include "../Randomizer/RandomizerDefaults.h"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace bbr {

class EnableWizardScreen : public Screen {
public:
    // Non-const: this screen persists the seed it used back to defaults.cfg
    // at commit time (only lastSeed - its own toggle states stay per-run).
    explicit EnableWizardScreen(RandomizerDefaults& defaults);

    void Update(const ButtonEdges& input) override;
    void Draw(Renderer& renderer) override;
    ScreenId RequestedScreen() const override { return requestedScreen_; }

private:
    enum class Step { SaveData, SelectReplace, EditSeed, EnemyPicker, BossPicker,
                      Confirm, Progress };
    enum class ReplaceChoice { NewSaveData, LeaveExisting, SelectFromBackup };

    void UpdateSaveData(const ButtonEdges& input);
    void UpdateSelectReplace(const ButtonEdges& input);
    void UpdateEditSeed(const ButtonEdges& input);
    void UpdateEnemyPicker(const ButtonEdges& input);
    void UpdateBossPicker(const ButtonEdges& input);
    void ReturnFromPicker(int row);
    void UpdateConfirm(const ButtonEdges& input);
    void UpdateProgress(const ButtonEdges& input);

    void DrawSaveData(Renderer& renderer);
    void DrawSelectReplace(Renderer& renderer);
    void DrawEditSeed(Renderer& renderer);
    void DrawEnemyPicker(Renderer& renderer);
    void DrawBossPicker(Renderer& renderer);
    void DrawConfirm(Renderer& renderer);
    void DrawProgress(Renderer& renderer);

    void GoToStep(Step step);
    std::string ReplaceDisplayText() const;
    std::string SeedDisplayText() const; // zero-padded to kSeedDigits
    void RollNewSeed();

    // The commit is split in two because the randomizer is stepped across
    // frames rather than run in one call (see EnemyRandomizer.h): StartCommit
    // emits the up-front lines and creates the job, UpdateProgress advances
    // it one step per frame, and FinishCommit emits the result lines once
    // it's done. Each line is Log()'d and stored for on-screen display too.
    // RANDOMIZE ENEMIES is real (see header comment above); Save Data
    // backup/replace is still simulated.
    void StartCommit();
    void FinishCommit();
    void AddProgressLine(const std::string& line);

    Step step_ = Step::SaveData;
    int  selected_ = 0;
    int  scrollOffset_ = 0; // top visible row of the settings/confirm lists

    static const int kSeedDigits = 10; // matches the reference tool's seed box

    RandomizerDefaults& defaults_; // written back at commit, seed only

    std::string   titleId_; // which AFR/<titleId>/dvdroot_ps4 the commit writes into
    bool          backupExistingSave_;
    ReplaceChoice replaceChoice_ = ReplaceChoice::NewSaveData;
    std::string   selectedBackupLabel_; // only meaningful once ReplaceChoice::SelectFromBackup is chosen
    bool          randomizeEnemies_;
    bool          randomizeBosses_;
    bool          randomizeTreasure_;
    bool          randomizeWorkshopTools_;
    bool          randomizeEnemyDrops_;
    bool          randomizeStartingWeapons_;
    bool          randomizeStartingGuns_;
    bool          randomizeShopWeapons_;
    bool          enableMergoDarkness_;

    // Per-run like every other toggle here: seeded from defaults, edited
    // freely, and NOT written back (only lastSeed is - see StartCommit).
    EnemyPoolSelection enemiesIncluded_;
    BossPoolSelection  bossesIncluded_;
    // One instance drives both lists - only one can be open at a time.
    ModelPicker        picker_;

    // Always a concrete value - there is no "random" mode. Left/right on the
    // seed row rolls a new one, X edits it digit by digit.
    uint32_t      seed_;
    char          seedBuf_[kSeedDigits + 1] = "0000000000"; // editor working buffer
    int           seedCursor_ = 0;

    std::vector<std::string> progressLines_;

    // Top visible line of the progress log. While the run is going this
    // tracks the tail automatically (you want to see what's happening now);
    // once it finishes, up/down scroll back through what scrolled past.
    int  progressScroll_ = 0;
    bool progressFollowTail_ = true;

    // Non-null only while the randomizer is actually running - StartCommit
    // creates it, UpdateProgress steps it, FinishCommit clears it.
    std::unique_ptr<EnemyRandomizerJob> job_;
    bool commitFinished_ = false;

    // Index of the first "the run is over" line in progressLines_, which
    // DrawProgress colors differently. Nothing is a completion line until
    // FinishCommit says so, hence the out-of-range default.
    std::size_t completionLineStart_ = (std::size_t)-1;

    ScreenId requestedScreen_ = ScreenId::None;
};

} // namespace bbr
