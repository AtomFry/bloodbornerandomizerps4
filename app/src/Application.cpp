#include "Application.h"

#include "Platform/Platform.h"
#include "Randomizer/RandomizerDefaults.h"
#include "Randomizer/RandomizerDefaultsStore.h"
#include "UI/ScreenManager.h"
#include "UI/SetupDefaultsScreen.h"
#include "UI/WorldEditorScreen.h"
#include "UI/WorldsScreen.h"

#include <memory>
#include <string>

namespace bbr {

namespace {
// The three screens the app has (Screen.h). Both tabs and the editor need
// Application's canonical RandomizerDefaults: DEFAULTS edits it and writes it
// back on save, WORLDS pre-fills a new world from it and takes the AFR title
// out of it, and the editor writes back only the seed it used, at activation
// time, so the next new world opens showing it.
//
// `worldId` is what the screen that asked for WorldEditor named (Screen.h):
// empty for a new world, a "w-NNNN" for an existing one, and the literal
// "vanilla" for the row that activates Vanilla (B8). `session` is the worlds
// screen's cross-switch state - the startup jobs run once per launch and the
// save container is read once, not once per tab switch (see WorldsScreen.h).
std::unique_ptr<Screen> MakeScreen(ScreenId id, RandomizerDefaults& defaults,
                                   const std::string& worldId,
                                   WorldsSession& session) {
    switch (id) {
        case ScreenId::Worlds:   return std::make_unique<WorldsScreen>(defaults, session);
        case ScreenId::Defaults: return std::make_unique<SetupDefaultsScreen>(defaults);
        // An empty world id is a NEW world, pre-filled from the DEFAULTS tab
        // (B6); a "w-NNNN" opens that world on its current revision (B7); and
        // "vanilla" opens straight on the activation confirmation, because
        // Vanilla has nothing to edit (B20).
        case ScreenId::WorldEditor:
            Log(("worlds: opening the editor for " +
                 (worldId.empty() ? std::string("a new world") : worldId)).c_str());
            return std::make_unique<WorldEditorScreen>(defaults, worldId);
        default: return nullptr;
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

    // The worlds screen's state across screen switches. Owned here because
    // Application is what outlives a screen (Application.h), and because the
    // startup reconciliation belongs to the launch rather than to any one
    // visit to the WORLDS tab.
    WorldsSession worldsSession;

    ScreenManager screens;
    // The app opens on WORLDS, whose first two modes are the startup sweep
    // and reconciliation (worlds plan section 4.3) and first-run capture
    // (B26). There is no main menu behind it any more: WORLDS is the main
    // screen, and O on its rail is what ends the app.
    screens.SetScreen(std::make_unique<WorldsScreen>(defaults, worldsSession));

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
            // Both answers are taken BEFORE SetScreen destroys the screen that
            // gave them; current is a raw pointer into it and is not touched
            // again afterwards, so that's safe.
            std::string worldId = current->RequestedWorldId();
            screens.SetScreen(MakeScreen(requested, defaults, worldId, worldsSession));
        }
    }

    platform.Shutdown();
    platform.Quit();
}

} // namespace bbr
