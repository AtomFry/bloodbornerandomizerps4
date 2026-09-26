// WorldEditorScreen.h - the screen that creates and edits ONE WORLD: its name,
// its seed, its randomizer settings, its save policy, and the append-only
// history of every recipe it has ever had (worlds B6, B7, B12, B14, B16).
//
// It is EnableWizardScreen renamed and extended (worlds plan P8), not a new
// screen: the rail, the three columns, the pickers, the seed editor, the
// confirm list and the progress log all carry over unchanged, because a world
// IS a recipe and the wizard was already the thing that edited one. What is
// new is a NAME row, a SAVE category, a HISTORY row, and the fact that
// pressing OPTIONS writes the result into Randomizer/WorldStore instead of
// only into memory.
//
// Progress runs Game/WorldActivation - the whole seven-phase transaction, not
// just the randomizer: the outgoing world's save is backed up and filed, the
// incoming world's tree is generated into a staging directory and swapped in,
// and its save becomes live. This screen no longer names an output path, an
// EnemyRandomizerOptions field or the vanilla source at all; it starts the
// job, drains its lines into the progress log, and reports what the
// generation half of it did once it is over.
//
// VANILLA OPENS STRAIGHT ON CONFIRM. X on the worlds rail's VANILLA row routes
// here with the id "vanilla" (B8). Vanilla has no editable settings and no
// revisions (B20), so there is nothing for the Settings step to show: the
// screen starts in Confirm, states the same B10 activation statement every
// other world gets, and O returns to the WORLDS tab rather than to a rail
// that would have nothing on it.
//
// THE RAIL (worlds plan §4.5): NAME, SEED, a rule, the six settings categories
// plus SAVE, a rule, HISTORY. There is no FINISH row any more - OPTIONS
// activates from anywhere on the screen, which is the same button Setup
// Defaults already saves with. The rail therefore runs ten rows deep and no
// longer shares the settings pane's 76px pitch; it has its own, and
// settings_ui_verify.py pins it separately rather than against Setup Defaults'
// seven-row rail.
//
// WHICH WORLD. An empty world id means a NEW world, pre-filled from the
// DEFAULTS tab (B6); a "w-NNNN" means that world, opened on its current
// revision (B7). The id is allocated by WorldStore on the first OPTIONS press
// and never changes afterwards, so editing and re-editing in one visit appends
// at most one revision per distinct recipe.
//
// A REVISION IS APPENDED ONLY WHEN THE RECIPE DIFFERS (worlds plan P4). A
// rename rewrites world.cfg and appends nothing; making an older revision
// current again is itself an append, so the history grows and never rewinds
// (spec worlds D2). None of that logic lives here - WorldStore::AppendRevision
// owns it, and this screen reports which way it went.
//
// CONTROLS. Up/Down move within whichever region has focus; Left/Right change
// the selected setting's value (and do nothing at all on the rail); X advances
// (rail -> pane, open a picker, open the name or seed editor, open HISTORY); O
// returns (pane -> rail, rail -> the WORLDS tab). X NO LONGER TOGGLES - it
// used to both toggle and drill in depending on the row, and one button now
// has one meaning (spec section 10, 9.2). OPTIONS writes the world and opens
// Confirm.
//
// One Screen, not a screen-per-step - same reasoning as every other
// internal-step screen in this app (see SetupDefaultsScreen.h): a real
// screen stack isn't justified yet, and this way the in-progress choices
// survive every trip into the name editor, the seed editor, a picker, the
// history, Confirm and Progress for free.
#pragma once

#include "ModelPicker.h"
#include "Screen.h"
#include "SettingsModel.h"

#include "../Game/WorldActivation.h"
#include "../Platform/SaveData.h"
#include "../Randomizer/EnemyRandomizer.h"
#include "../Randomizer/RandomizerDefaults.h"
#include "../Randomizer/WorldStore.h"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace bbr {

class WorldEditorScreen : public Screen {
public:
    // Non-const: this screen persists the seed it used back to defaults.cfg
    // at commit time (only lastSeed - its own toggle states stay per-run).
    //
    // `worldId` is empty for a new world and a "w-NNNN" for an existing one,
    // exactly as Screen::RequestedWorldId() reported it.
    WorldEditorScreen(RandomizerDefaults& defaults, const std::string& worldId);
    ~WorldEditorScreen();

    void Update(const ButtonEdges& input) override;
    void Draw(Renderer& renderer) override;
    ScreenId RequestedScreen() const override { return requestedScreen_; }

private:
    // One Picker step rather than three: which list is open is the SettingDef
    // the pane was sitting on, and the four Selection* functions turn that
    // into the table, the count, the flags and the vocabulary. Naming the
    // three lists here would put a settings identity back in the screen.
    enum class Step { Settings, EditName, EditSeed, Picker, History, Confirm,
                      Progress };

    // The two focus targets on the Settings step. The header band draws and
    // never takes focus (plan 9 D1).
    enum class Focus { Rail, List };

    // The categories THIS screen shows, in rail order - a LIST, not a range
    // over SettingCategory, for the same reason SetupDefaultsScreen carries
    // one: the difference between the two lists is what keeps SAVE off the
    // DEFAULTS tab (worlds plan §3.3). SAVE is last, below WORLD, because it
    // is the only one of them that is not about what the randomizer writes.
    static constexpr SettingCategory kCategories[] = {
        SettingCategory::Enemies,
        SettingCategory::Bosses,
        SettingCategory::ItemsTreasure,
        SettingCategory::WeaponsGear,
        SettingCategory::Difficulty,
        SettingCategory::World,
        SettingCategory::Save,
    };
    static const int kCategoryCount =
        (int)(sizeof(kCategories) / sizeof(kCategories[0]));

    // Rail rows: 0 is NAME, 1 is SEED, 2..2+kCategoryCount-1 are the
    // categories, and the last is HISTORY. The three non-category rows are
    // named here and nowhere else.
    static const int kNameRow       = 0;
    static const int kSeedRow       = 1;
    static const int kFirstCatRow   = 2;
    static const int kHistoryRow    = kCategoryCount + 2;
    static const int kRailItemCount = kCategoryCount + 3;

    void UpdateSettings(const ButtonEdges& input);
    void UpdateRail(const ButtonEdges& input);
    void UpdatePane(const ButtonEdges& input);
    void UpdateEditName(const ButtonEdges& input);
    void UpdateEditSeed(const ButtonEdges& input);
    void UpdatePicker(const ButtonEdges& input);
    void UpdateHistory(const ButtonEdges& input);
    void UpdateConfirm(const ButtonEdges& input);
    void UpdateProgress(const ButtonEdges& input);

    void DrawSettings(Renderer& renderer);
    void DrawRail(Renderer& renderer);
    void DrawRailRow(Renderer& renderer, int y, const char* text, bool focused,
                     bool current);
    void DrawPane(Renderer& renderer);
    void DrawHelp(Renderer& renderer);
    void DrawEditName(Renderer& renderer);
    void DrawEditSeed(Renderer& renderer);
    void DrawHistory(Renderer& renderer);
    void DrawConfirm(Renderer& renderer);
    void DrawProgress(Renderer& renderer);

    void GoToStep(Step step);
    // OPTIONS's destination: writes the world, then opens Confirm with the
    // activation plan not yet built - UpdateConfirm builds it on its first
    // frame, so the screen can say CHECKING while phase 1 reads the container.
    void OpenConfirm();
    std::string SeedDisplayText() const; // zero-padded to kSeedDigits
    std::string NameDisplayText() const; // the name, or NOT SET
    void RollNewSeed();
    void OpenNameEditor();
    void OpenSeedEditor();

    // The world half. LoadWorld fills name_, seed_ and run_ from the store on
    // the way in; SaveWorld writes them back on OPTIONS, creating the world if
    // this is the first press. Both are no-ops without a signed-in user, which
    // is the one state in which the editor still has to draw.
    void LoadWorld();
    void SaveWorld();
    void LoadHistory();

    // The recipe as the store stores it: the seed plus the whole settings
    // struct. Built in one place so the comparison AppendRevision makes is
    // against exactly what would be written.
    WorldRecipe CurrentRecipe() const;

    SettingCategory Category() const { return kCategories[lastCategory_]; }
    const SettingDef& SelectedSetting() const;
    // The name row, the seed row, then all settings in category order - one
    // list, built from the model, shown on Confirm (plan 9 D3).
    std::vector<std::string> ConfirmItems() const;
    // The B10 statement, as label/value rows: which world is deactivated and
    // where its save goes, which is activated, and what happens to the save.
    // Always the same number of rows, refused or not, so one geometry covers
    // both (settings_ui_verify.py measures it).
    std::vector<std::string> ConfirmHeadRows() const;
    // Roughly how long this activation takes (B10), coarse on purpose.
    const char* DurationText() const;
    // One line per revision, newest first: which revision, its seed, and how
    // many settings it changed from the revision before it (worlds plan §7
    // milestone 5 step 6).
    std::vector<std::string> HistoryItems() const;

    Step  step_  = Step::Settings;
    Focus focus_ = Focus::Rail;

    // One cursor over the whole rail column, and one cursor plus one scroll
    // offset per category, so leaving a category and coming back lands on the
    // row that was left. Nothing clears these - that is what makes "returning
    // restores position" true everywhere instead of true wherever someone
    // remembered to restore it (spec section 2).
    int railCursor_   = 0;
    int lastCategory_ = 0; // an index into kCategories; the category the pane
                           // keeps showing, even while the rail cursor sits on
                           // NAME, SEED or HISTORY (P13)
    int listCursor_[kCategoryCount] = { 0 };
    int listScroll_[kCategoryCount] = { 0 };

    // Confirm is a review list with no cursor, so it owns a scroll offset of
    // its own - zeroed when Confirm is entered from OPTIONS, and never by
    // anything else.
    int confirmScroll_ = 0;

    // Phase 1's answer, which is also everything the B10 statement is built
    // from. Costly - PlanActivation reads the whole save container - so it is
    // built once, on Confirm's first frame, and never in the draw path.
    ActivationPlan plan_;
    bool           planReady_   = false;
    bool           planPending_ = false;

    static const int kSeedDigits = 10; // matches the reference tool's seed box
    static const int kNameLen    = 16; // worlds plan P10, and the cap
                                       // NormalizeWorldName enforces

    // Vanilla is a world (B20) but not an editable one: the screen opens on
    // Confirm for it and never shows the rail.
    bool isVanilla_ = false;

    RandomizerDefaults& defaults_; // written back at commit, seed only

    // THIS WORLD'S settings - one whole RandomizerDefaults so the same
    // SettingsModel table serves this screen and Setup Defaults alike.
    // Copy-constructed from defaults_ for a new world and overwritten from the
    // world's current revision for an existing one; NOT written back into
    // defaults_ (only lastSeed is - see StartCommit). bloodborneTitleId
    // travels with it, which is where the commit's output path comes from.
    RandomizerDefaults run_;

    // The world being edited. `worldId_` is empty until the first OPTIONS
    // press allocates one, and `storedName_` is what world.cfg last held, so
    // a rename can be told from no change at all.
    savedata::User              user_;
    std::unique_ptr<WorldStore> store_;
    std::string                 worldId_;
    std::string                 name_;
    std::string                 storedName_;
    bool                        isNewWorld_ = true;
    std::string                 storeError_;    // shown on Confirm when set
    std::string                 saveNote_;      // what the last OPTIONS did

    // The revision list, re-read from disk whenever HISTORY is opened, newest
    // first (spec worlds D14).
    std::vector<WorldRevision> revisions_;
    int historyCursor_ = 0;
    int historyScroll_ = 0;

    // One instance drives all three lists - only one can be open at a time -
    // and this is which setting it was opened for.
    ModelPicker        picker_;
    const SettingDef*  pickerSetting_ = nullptr;

    // Always a concrete value - there is no "random" mode. X on the seed row
    // edits it digit by digit and SQUARE rolls a new one in the editor.
    uint32_t      seed_;
    char          seedBuf_[kSeedDigits + 1] = "0000000000"; // editor working buffer
    int           seedCursor_ = 0;

    char          nameBuf_[kNameLen + 1] = "                "; // 16 spaces
    int           nameCursor_ = 0;

    std::vector<std::string> progressLines_;

    // Top visible line of the progress log. While the run is going this
    // tracks the tail automatically (you want to see what's happening now);
    // once it finishes, up/down scroll back through what scrolled past.
    int  progressScroll_ = 0;
    bool progressFollowTail_ = true;

    // Non-null only while the activation is actually running - StartCommit
    // creates it, UpdateProgress steps it, FinishCommit clears it.
    std::unique_ptr<WorldActivationJob> job_;
    bool commitFinished_ = false;

    // Index of the first "the run is over" line in progressLines_, which
    // DrawProgress colors differently. Nothing is a completion line until
    // FinishCommit says so, hence the out-of-range default.
    std::size_t completionLineStart_ = (std::size_t)-1;

    ScreenId requestedScreen_ = ScreenId::None;

    // The commit is split in two because the randomizer is stepped across
    // frames rather than run in one call (see EnemyRandomizer.h): StartCommit
    // emits the up-front lines and creates the job, UpdateProgress advances
    // it one step per frame, and FinishCommit emits the result lines once
    // it's done. Each line is Log()'d and stored for on-screen display too.
    void StartCommit();
    void FinishCommit();
    void AddProgressLine(const std::string& line);
};

} // namespace bbr
