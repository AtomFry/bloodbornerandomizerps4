// EnableWizardScreen.h - the ENABLE RANDOMIZER wizard. The flow is Settings ->
// Confirm -> Progress, with the seed editor and the three list pickers as
// drill-ins off the Settings step.
//
// Progress does the real work: it calls into Randomizer/EnemyRandomizer.cpp,
// reading vanilla map files from kVanillaSourceDir and writing the randomized
// ones into /data/GoldHEN/AFR/<titleId>/dvdroot_ps4/.
//
// The Settings step is the categorised screen: a decorative header, a left
// rail of SEED, the six categories and FINISH, the selected category's
// settings in the middle, and contextual help on the right. It used to be a
// flat 19-row list called Step::SaveData, addressed by a block of hardcoded
// row constants that had to agree with a parallel `items` vector and with a
// 190-line if/else chain - see SettingsModel.h for why that design had to go.
// THE ROW CONSTANTS ARE GONE, and so is the second copy of the settings list
// that DrawConfirm used to carry: Confirm is generated from the same table.
//
// The wizard also used to open on two save-data rows - BACKUP EXISTING SAVE
// and REPLACE SAVE - with Screen 1A (Select Replace Save) behind the second
// and simulated backup/replace/restore lines in the progress log. None of it
// was ever implemented, and the randomizer-settings-ui spec §4.8 removed all
// of it: the app does not touch save data at all, and save-data handling is to
// be redesigned as separate work rather than carried forward as a stub.
//
// CONTROLS. Up/Down move within whichever region has focus; Left/Right change
// the selected setting's value (and roll a new seed on the SEED row); X
// advances (rail -> pane, open a picker, open the seed editor, FINISH ->
// Confirm); O returns (pane -> rail, rail -> menu). X NO LONGER TOGGLES - it
// used to both toggle and drill in depending on the row, and one button now
// has one meaning (spec section 10, 9.2). OPTIONS commits, on Confirm only.
//
// One Screen, not a screen-per-step - same reasoning as every other
// internal-step screen in this app (see SetupDefaultsScreen.h): a real
// screen stack isn't justified yet, and this way Screen 1's in-progress
// choices survive every trip into the seed editor, a picker, Confirm and
// Progress for free.
#pragma once

#include "ModelPicker.h"
#include "Screen.h"
#include "SettingsModel.h"

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
    // One Picker step rather than three: which list is open is the SettingDef
    // the pane was sitting on, and the four Selection* functions turn that
    // into the table, the count, the flags and the vocabulary. Naming the
    // three lists here would put a settings identity back in the screen.
    enum class Step { Settings, EditSeed, Picker, Confirm, Progress };

    // The two focus targets on the Settings step. The header band draws and
    // never takes focus (plan 9 D1).
    enum class Focus { Rail, List };

    static const int kCategoryCount = (int)SettingCategory::Count;
    // Rail rows: 0 is SEED, 1..6 are the categories, 7 is FINISH. Neither end
    // row is a category - each is separated from the block by its own rule.
    static const int kFinishRow     = kCategoryCount + 1;
    static const int kRailItemCount = kCategoryCount + 2;

    void UpdateSettings(const ButtonEdges& input);
    void UpdateRail(const ButtonEdges& input);
    void UpdatePane(const ButtonEdges& input);
    void UpdateEditSeed(const ButtonEdges& input);
    void UpdatePicker(const ButtonEdges& input);
    void UpdateConfirm(const ButtonEdges& input);
    void UpdateProgress(const ButtonEdges& input);

    void DrawSettings(Renderer& renderer);
    void DrawRail(Renderer& renderer);
    void DrawRailRow(Renderer& renderer, int y, const char* text, bool focused,
                     bool current);
    void DrawPane(Renderer& renderer);
    void DrawHelp(Renderer& renderer);
    void DrawEditSeed(Renderer& renderer);
    void DrawConfirm(Renderer& renderer);
    void DrawProgress(Renderer& renderer);

    void GoToStep(Step step);
    std::string SeedDisplayText() const; // zero-padded to kSeedDigits
    void RollNewSeed();
    void OpenSeedEditor();

    SettingCategory Category() const { return (SettingCategory)lastCategory_; }
    const SettingDef& SelectedSetting() const;
    // The seed row, then all 18 settings in category order - one list, built
    // from the model, shown on Confirm (plan 9 D3).
    std::vector<std::string> ConfirmItems() const;

    // The commit is split in two because the randomizer is stepped across
    // frames rather than run in one call (see EnemyRandomizer.h): StartCommit
    // emits the up-front lines and creates the job, UpdateProgress advances
    // it one step per frame, and FinishCommit emits the result lines once
    // it's done. Each line is Log()'d and stored for on-screen display too.
    void StartCommit();
    void FinishCommit();
    void AddProgressLine(const std::string& line);

    Step  step_  = Step::Settings;
    Focus focus_ = Focus::Rail;

    // One cursor over the whole rail column, and one cursor plus one scroll
    // offset per category, so leaving a category and coming back lands on the
    // row that was left. Nothing clears these - that is what makes "returning
    // restores position" true everywhere instead of true wherever someone
    // remembered to restore it (spec section 2).
    int railCursor_   = 0;
    int lastCategory_ = 0; // 0..5; the category the pane keeps showing, even
                           // while the rail cursor sits on SEED or FINISH (P13)
    int listCursor_[kCategoryCount] = { 0 };
    int listScroll_[kCategoryCount] = { 0 };

    // Confirm is a review list with no cursor, so it owns a scroll offset of
    // its own - zeroed when Confirm is entered from FINISH, and never by
    // anything else.
    int confirmScroll_ = 0;

    static const int kSeedDigits = 10; // matches the reference tool's seed box

    RandomizerDefaults& defaults_; // written back at commit, seed only

    // THIS RUN'S settings - the 15 loose bools and 3 selection members this
    // screen used to carry, now one whole RandomizerDefaults so the same
    // SettingsModel table serves this screen and Setup Defaults alike.
    // Copy-constructed from defaults_ and edited freely; NOT written back
    // (only lastSeed is - see StartCommit). bloodborneTitleId travels with it,
    // which is where the commit's output path comes from.
    RandomizerDefaults run_;

    // One instance drives all three lists - only one can be open at a time -
    // and this is which setting it was opened for.
    ModelPicker        picker_;
    const SettingDef*  pickerSetting_ = nullptr;

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
