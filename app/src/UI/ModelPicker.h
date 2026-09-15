// ModelPicker.h - the "which creatures may be used as replacements" list.
// Drives both the enemy picker (82 rows) and the boss picker (17), which
// behave identically; the host supplies the table, the count and the flags.
//
// NOT a Screen. Application.cpp constructs screens fresh on every switch, so
// a real drill-in would destroy the Enable wizard's in-progress toggles on the
// way back - the same reason SetupDefaultsScreen's title-ID editor is an
// internal mode rather than its own ScreenId. This is a component both hosting
// screens own and delegate to while their own mode says so.
//
// The host owns the selection; this only edits what it is handed.
//
// Layout is denser than the settings screens (item scale 3, 52px spacing, 12
// visible rows instead of 6): 82 rows at the settings screens' size would be
// fourteen pages. Scale 3 is already proven here - it is what the progress log
// uses. Geometry is asserted by tools/ui_scroll_verify.py.
#pragma once

#include "Controls.h"

#include "../Randomizer/EnemyPoolSelection.h"

namespace bbr {

class Renderer;

class ModelPicker {
public:
    // Call when entering the mode, so the cursor starts at the top rather than
    // wherever it was left last time.
    void Reset();

    // Returns true once the user is finished and the host should leave the
    // mode. `enabled` is the selection's flag array, edited in place.
    bool Update(const ButtonEdges& input, const ModelPoolEntry* table, int count,
                bool* enabled);

    void Draw(Renderer& renderer, const char* heading, const ModelPoolEntry* table,
              int count, const bool* enabled);

private:
    // Select-all / select-none wipe a hand-built list with one press and there
    // is no undo, so both confirm first. A third mode rather than a separate
    // screen, for the same reason the picker itself is not one.
    enum class Pending { None, EnableAll, DisableAll };

    void MovePage(int direction, int count);
    void DrawConfirm(Renderer& renderer, int count, int enabledCount);

    int     cursor_ = 0;
    int     scroll_ = 0;
    Pending pending_ = Pending::None;
};

} // namespace bbr
