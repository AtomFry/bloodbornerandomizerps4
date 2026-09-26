// SetupDefaultsScreen.h - the categorised Settings screen for the values a new
// run starts from: a decorative header, a left rail of BLOODBORNE TITLE ID
// plus the six categories, the selected category's settings in the middle, and
// contextual help on the right.
//
// Per docs/plans/ui-blueprint-wizards.md this is a plain editor (not a wizard,
// and the wizards are gone): edits are local
// until OPTIONS writes them into Application's canonical RandomizerDefaults
// and persists them to disk; O discards them.
//
// THE ROW CONSTANTS ARE GONE. Every setting used to be addressed by a
// hardcoded index that had to agree with a parallel `items` vector and with a
// branch in ToggleRow, which is why three separate features appended their row
// last rather than put it where it belonged. Settings now come from
// SettingsModel.h, whose declaration order IS the display order, and
// AdjustSetting is the only thing that writes one.
//
// CONTROLS. Up/Down move within whichever region has focus; Left/Right change
// the selected setting's value; X advances (rail -> pane, or open a picker or
// the title-ID editor); O returns (pane -> rail, rail -> menu). X NO LONGER
// TOGGLES - it used to both toggle and drill in depending on the row, and one
// button now has one meaning (spec section 10, 9.2).
//
// The title-ID editor and the three pickers are internal modes of this screen,
// not separate Screens - same reasoning as WorldEditorScreen's internal Step
// enum: Application.cpp rebuilds screens on every switch, so a real drill-in
// would destroy this screen's in-progress edits on the way back.
#pragma once

#include "ModelPicker.h"
#include "Screen.h"
#include "SettingsModel.h"

#include "../Randomizer/RandomizerDefaults.h"

#include <string>

namespace bbr {

class SetupDefaultsScreen : public Screen {
public:
    explicit SetupDefaultsScreen(RandomizerDefaults& defaults)
        : defaults_(defaults), working_(defaults) {}

    void Update(const ButtonEdges& input) override;
    void Draw(Renderer& renderer) override;
    ScreenId RequestedScreen() const override { return requestedScreen_; }

private:
    // One picker mode rather than three: which list is open is the SettingDef
    // the pane was sitting on, and the four Selection* functions turn that
    // into the table, the count, the flags and the vocabulary. Naming the
    // three lists here would put a settings identity back in the screen.
    enum class Mode { Settings, EditTitleId, Picker };

    // The two focus targets. The header band draws and never takes focus
    // (plan 9 D1).
    enum class Focus { Rail, List };

    // The categories THIS screen shows, in rail order - a LIST, not a range
    // over SettingCategory. Milestone 5 of the worlds feature adds
    // SettingCategory::Save for the world editor, and a per-world save policy
    // has no business on the screen that says what a NEW world starts from
    // (worlds plan section 3.3). Iterating an own list is what stops an
    // editor-only category turning up here by accident.
    static constexpr SettingCategory kCategories[] = {
        SettingCategory::Enemies,
        SettingCategory::Bosses,
        SettingCategory::ItemsTreasure,
        SettingCategory::WeaponsGear,
        SettingCategory::Difficulty,
        SettingCategory::World,
    };
    static const int kCategoryCount =
        (int)(sizeof(kCategories) / sizeof(kCategories[0]));
    // Rail rows: 0 is BLOODBORNE TITLE ID, 1..6 are the categories. No FINISH
    // - this screen saves with OPTIONS and has no commit path, which is why it
    // is the first of the two screens to be built on the model.
    static const int kRailItemCount = kCategoryCount + 1;
    static const int kTitleIdLen = 9; // 4 letters + 5 digits, the PS4's own title ID shape

    void UpdateSettings(const ButtonEdges& input);
    void UpdateRail(const ButtonEdges& input);
    void UpdatePane(const ButtonEdges& input);
    void UpdateEditTitleId(const ButtonEdges& input);

    void DrawSettings(Renderer& renderer);
    void DrawRail(Renderer& renderer);
    void DrawRailRow(Renderer& renderer, int y, const char* text, bool focused,
                     bool current);
    void DrawPane(Renderer& renderer);
    void DrawHelp(Renderer& renderer);
    void DrawEditTitleId(Renderer& renderer);

    void OpenTitleIdEditor();
    std::string TitleIdDisplay() const;

    SettingCategory Category() const { return kCategories[lastCategory_]; }
    const SettingDef& SelectedSetting() const;

    RandomizerDefaults& defaults_; // Application's canonical copy - only touched on SAVE
    RandomizerDefaults  working_;  // this screen's own edits, discarded on BACK

    Mode  mode_  = Mode::Settings;
    Focus focus_ = Focus::Rail;

    // One cursor over the whole rail column, and one cursor plus one scroll
    // offset per category, so leaving a category and coming back lands on the
    // row that was left. Nothing clears these - that is what makes "returning
    // restores position" true everywhere instead of true wherever someone
    // remembered to restore it.
    int railCursor_   = 0;
    int lastCategory_ = 0; // an index into kCategories; the category the pane
                           // keeps showing, even while the rail cursor sits on
                           // row 0 (plan P13)
    int listCursor_[kCategoryCount] = { 0 };
    int listScroll_[kCategoryCount] = { 0 };

    // One picker instance drives all three lists - only one can be open at a
    // time - and this is which setting it was opened for.
    ModelPicker picker_;
    const SettingDef* pickerSetting_ = nullptr;

    char editBuf_[kTitleIdLen + 1] = "CUSA00000"; // working buffer while in EditTitleId
    int  cursor_ = 0;                             // 0..8, which character is being edited

    ScreenId requestedScreen_ = ScreenId::None;
};

} // namespace bbr
