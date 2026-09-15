// MenuScreen.h - the main menu, rebuilt to match UI_BLUEPRINT.md: a status
// banner plus exactly three actions (no "Manage Saves"/"Randomizer
// Profiles" as standalone items anymore - those only exist as sub-screens
// reached from inside the Enable/Disable wizards). ENABLE RANDOMIZER /
// DISABLE RANDOMIZER currently drill into an empty PlaceholderScreen; per
// the blueprint, selecting Enable while already ON (or Disable while OFF)
// is not special-cased - the wizard is always reachable regardless of
// current status.
//
// The banner is derived fresh from disk on every construction - GameInfo
// finds every real Bloodborne install (by content fingerprint, not a
// title-ID whitelist - see GameInfo.h) and AfrManager checks each one's AFR
// content for the randomizer marker. There is deliberately no persisted
// on/off flag anywhere: that was tried and reverted (see git history/
// conversation) because a self-reported flag can drift from what's
// actually on disk. This will read OFF until something real writes that
// marker, which nothing does yet.
#pragma once

#include "Screen.h"

namespace bbr {

class MenuScreen : public Screen {
public:
    static const int kItemCount = 4;

    MenuScreen();

    void Update(const ButtonEdges& input) override;
    void Draw(Renderer& renderer) override;
    bool WantsExit() const override { return wantsExit_; }
    ScreenId RequestedScreen() const override { return requestedScreen_; }

private:
    bool randomizerOn_ = false;

    int  selected_ = 0;
    bool wantsExit_ = false;
    ScreenId requestedScreen_ = ScreenId::None;
};

} // namespace bbr
