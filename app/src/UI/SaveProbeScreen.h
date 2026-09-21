// SaveProbeScreen.h - TEMPORARY DIAGNOSTIC, delete with SaveDataProbe.{h,cpp},
// ScreenId::SaveDataProbe, the menu row and -lSceSaveData.
//
// One screen, one button: X runs the save-data probe and shows what it found.
// The output also goes to live.log, so a probe that kills the app still leaves
// its trail - which is the likeliest interesting failure here.
//
// Deliberately NOT a setting. The categorised screens are driven by
// SettingsModel's table, whose 18 entries are asserted against the spec by
// settings_ui_verify.py; adding a nineteenth fake one would fail that check and
// put a diagnostic into a model that is meant to describe the randomizer. A
// main-menu row costs nothing and is trivially removed.
#pragma once

#include "Screen.h"

#include <string>
#include <vector>

namespace bbr {

class SaveProbeScreen : public Screen {
public:
    explicit SaveProbeScreen(const std::string& titleId);

    void Update(const ButtonEdges& input) override;
    void Draw(Renderer& renderer) override;
    ScreenId RequestedScreen() const override { return requestedScreen_; }

private:
    std::string              titleId_;
    bool                     didBackup_ = false;
    std::vector<std::string> lines_;
    bool                     hasRun_ = false;
    int                      scroll_ = 0;

    ScreenId requestedScreen_ = ScreenId::None;
};

} // namespace bbr
