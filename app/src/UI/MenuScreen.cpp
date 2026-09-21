#include "MenuScreen.h"

#include "Controls.h"

#include "../Game/AfrManager.h"
#include "../Game/GameInfo.h"
#include "../Platform/Log.h"

#include <string>
#include <vector>

namespace bbr {

namespace {
const char* kItems[MenuScreen::kItemCount] = {
    "ENABLE RANDOMIZER",
    "DISABLE RANDOMIZER",
    "SETUP DEFAULTS",
    "SAVE DATA PROBE (TEST)",
    "EXIT",
};
const int kEnableIndex  = 0;
const int kDisableIndex = 1;
const int kDefaultsIndex = 2;
const int kProbeIndex    = 3;   // TEMPORARY - see UI/SaveProbeScreen.h
const int kExitIndex    = MenuScreen::kItemCount - 1;

const int kTitleScale  = 5;
const int kBannerScale = 3;
const int kItemScale   = 4;
const int kFooterScale = 3;
} // namespace

MenuScreen::MenuScreen() {
    // "On" if ANY detected Bloodborne install's AFR content carries the
    // randomizer marker (see AfrStatus::randomized). Nothing produces that
    // marker yet, so this is always false today - honestly, because
    // nothing was found, not because a flag says so.
    std::vector<TitleInfo> titles = GameInfo::DetectAll();
    Log((std::string("menu: GameInfo::DetectAll found ") + std::to_string(titles.size())
         + " title(s)").c_str());
    for (const TitleInfo& title : titles) {
        AfrStatus afr = AfrManager::Check(title.titleId);
        Log((std::string("menu:   ") + title.titleId
             + " writable=" + (afr.writable ? "Y" : "N")
             + " seeded=" + (afr.seeded ? "Y" : "N")
             + " randomized=" + (afr.randomized ? "Y" : "N")).c_str());
        if (afr.randomized) randomizerOn_ = true;
    }
    Log(randomizerOn_ ? "menu: status derived as ON" : "menu: status derived as OFF");
}

void MenuScreen::Update(const ButtonEdges& input) {
    selected_ = NavigateVertical(selected_, kItemCount, input);

    if (input.cross) {
        if (selected_ == kExitIndex) {
            Log("menu: EXIT selected");
            wantsExit_ = true;
        } else if (selected_ == kEnableIndex) {
            Log("menu: ENABLE RANDOMIZER selected - switching screen");
            requestedScreen_ = ScreenId::EnableWizard;
        } else if (selected_ == kDisableIndex) {
            Log("menu: DISABLE RANDOMIZER selected - switching screen");
            requestedScreen_ = ScreenId::DisableWizard;
        } else if (selected_ == kDefaultsIndex) {
            Log("menu: SETUP DEFAULTS selected - switching screen");
            requestedScreen_ = ScreenId::SetupDefaults;
        } else if (selected_ == kProbeIndex) {
            requestedScreen_ = ScreenId::SaveDataProbe;
        }
    }

    if (input.circle) {
        Log("O pressed at main menu - exiting");
        wantsExit_ = true;
    }
}

void MenuScreen::Draw(Renderer& renderer) {
    renderer.Clear(20, 24, 28);

    DrawCenteredLabel(renderer, 140, "BLOODBORNE RANDOMIZER", kTitleScale, Palette::Heading);

    const char* banner = randomizerOn_ ? "RANDOMIZER STATUS ON" : "RANDOMIZER STATUS OFF";
    DrawCenteredLabel(renderer, 260, banner, kBannerScale, Palette::Text);

    std::vector<std::string> items(kItems, kItems + kItemCount);
    DrawMenuList(renderer, 460, 100, items, selected_, kItemScale,
                 Palette::Text, Palette::Selected);

    DrawCenteredLabel(renderer, kScreenHeight - 80, "X SELECT   O EXIT", kFooterScale, Palette::Dim);
}

} // namespace bbr
