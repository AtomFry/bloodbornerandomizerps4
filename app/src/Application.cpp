#include "Application.h"

#include "Platform/Platform.h"
#include "Randomizer/RandomizerDefaults.h"
#include "Randomizer/RandomizerDefaultsStore.h"
#include "UI/EnableWizardScreen.h"
#include "UI/MenuScreen.h"
#include "UI/PlaceholderScreen.h"
#include "UI/ScreenManager.h"
#include "UI/SaveProbeScreen.h"
#include "UI/SetupDefaultsScreen.h"

#include <memory>
#include <string>

namespace bbr {

namespace {
// Disable is still PlaceholderScreen - see UI_BLUEPRINT.md for what it
// becomes. EnableWizard/SetupDefaults are real now and need Application's
// canonical RandomizerDefaults. SetupDefaults writes it back on save;
// EnableWizard writes back only the seed it used, at commit time, so the
// next run opens showing it - see each screen's header. MenuScreen
// needs nothing passed in - it derives its own status fresh from disk
// every time (see MenuScreen.cpp).
std::unique_ptr<Screen> MakeScreen(ScreenId id, RandomizerDefaults& defaults) {
    switch (id) {
        case ScreenId::Menu:          return std::make_unique<MenuScreen>();
        case ScreenId::EnableWizard:  return std::make_unique<EnableWizardScreen>(defaults);
        case ScreenId::DisableWizard: return std::make_unique<PlaceholderScreen>("DISABLE RANDOMIZER");
        case ScreenId::SetupDefaults: return std::make_unique<SetupDefaultsScreen>(defaults);
        // TEMPORARY - see UI/SaveProbeScreen.h
        case ScreenId::SaveDataProbe: return std::make_unique<SaveProbeScreen>(defaults.bloodborneTitleId);
        default:                      return nullptr;
    }
}
} // namespace

void Application::Run() {
    Platform platform;
    std::string err;
    if (!platform.Init(&err)) {
        Log(("Platform::Init FAILED: " + err).c_str());
        platform.Quit();
    }

    // Loaded once for the life of the process; SetupDefaultsScreen edits a
    // working copy and only writes back here (and to disk) on SAVE.
    RandomizerDefaults defaults = LoadRandomizerDefaults();
    Log(("defaults: loaded (bloodborne title id = " + defaults.bloodborneTitleId + ")").c_str());

    ScreenManager screens;
    screens.SetScreen(std::make_unique<MenuScreen>());

    Log("entering frame loop");
    bool running = true;
    int frame = 0;

    while (running) {
        if (!platform.PumpEvents(platform.GetInput())) {
            Log("SDL_QUIT received");
            break;
        }

        Screen* current = screens.Current();
        current->Update(platform.GetInput().Current());
        current->Draw(platform.GetRenderer()); // each Screen clears its own background
        platform.Present();

        if (frame == 0) Log("first frame presented");
        frame++;

        if (current->WantsExit()) {
            running = false;
        } else if (ScreenId requested = current->RequestedScreen(); requested != ScreenId::None) {
            // current is a raw pointer into the screen SetScreen() is about
            // to destroy - it's not touched again after this, so that's safe.
            screens.SetScreen(MakeScreen(requested, defaults));
        }
    }

    platform.Shutdown();
    platform.Quit();
}

} // namespace bbr
