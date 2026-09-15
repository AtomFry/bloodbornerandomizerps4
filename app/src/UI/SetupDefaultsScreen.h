// SetupDefaultsScreen.h - BACKUP EXISTING SAVE (toggle) and BLOODBORNE
// TITLE ID (drill-in, per-character editor). Per UI_BLUEPRINT.md, this is a
// plain editor (not a wizard): edits are local until Options writes them
// into Application's canonical RandomizerDefaults and persists them to
// disk; O (BACK) discards them. X always means "activate whatever's
// highlighted" (toggle the boolean row, open the editor on the title ID
// row) - it deliberately does NOT save, so its meaning doesn't change
// depending on which row is selected the way an earlier pass had it.
//
// The title ID editor is an internal mode of this screen, not a separate
// Screen/ScreenId - same reasoning as EnableWizardScreen's internal Step
// enum: a real screen stack isn't justified for one drill-in, and this way
// the boolean row's in-progress edit survives the trip into and out of the
// editor for free (nothing gets destroyed/recreated).
#pragma once

#include "ModelPicker.h"
#include "Screen.h"

#include "../Randomizer/RandomizerDefaults.h"

namespace bbr {

class SetupDefaultsScreen : public Screen {
public:
    explicit SetupDefaultsScreen(RandomizerDefaults& defaults)
        : defaults_(defaults), working_(defaults) {}

    void Update(const ButtonEdges& input) override;
    void Draw(Renderer& renderer) override;
    ScreenId RequestedScreen() const override { return requestedScreen_; }

private:
    enum class Mode { List, EditTitleId, EnemyPicker, BossPicker };

    static const int kItemCount = 14;
    static const int kTitleIdRow = 0;
    static const int kBackupRow = 1;
    static const int kReplaceDefaultRow = 2;
    static const int kRandomizeEnemiesRow = 3;
    static const int kRandomizeBossesRow = 4;
    static const int kRandomizeTreasureRow = 5;
    static const int kRandomizeWorkshopToolsRow = 6;
    static const int kRandomizeDropsRow = 7;
    static const int kStartingWeaponsRow = 8;
    static const int kStartingGunsRow = 9;
    static const int kShopWeaponsRow = 10;
    // Appended rather than grouped with the RANDOMIZE rows: it is not a
    // randomizer, and going last means no existing row index has to move.
    static const int kDisableMergoDarknessRow = 11;
    // Drill-in, not a toggle: 82 rows cannot live inline (see EnemyPicker.h).
    static const int kEnemiesIncludedRow = 12;
    static const int kBossesIncludedRow = 13;
    static const int kTitleIdLen = 9; // 4 letters + 5 digits, the PS4's own title ID shape

    void UpdateList(const ButtonEdges& input);
    void UpdateEditTitleId(const ButtonEdges& input);
    void ToggleRow(int row); // toggles whichever boolean-shaped row this is; no-op for kTitleIdRow

    void DrawList(Renderer& renderer);
    std::string EnemiesIncludedText() const;
    std::string BossesIncludedText() const;
    void DrawEditTitleId(Renderer& renderer);

    RandomizerDefaults& defaults_; // Application's canonical copy - only touched on SAVE
    RandomizerDefaults  working_;  // this screen's own edits, discarded on BACK

    Mode mode_ = Mode::List;
    int  selected_ = 0;
    int  scrollOffset_ = 0; // index of the top visible row; more rows than fit

    // One picker instance drives both lists - only one can be open at a
    // time, and Reset() clears its cursor on entry either way.
    ModelPicker picker_;

    char editBuf_[kTitleIdLen + 1] = "CUSA00000"; // working buffer while in EditTitleId
    int  cursor_ = 0;                             // 0..8, which character is being edited

    ScreenId requestedScreen_ = ScreenId::None;
};

} // namespace bbr
