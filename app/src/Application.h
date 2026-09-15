// Application.h - owns a Platform, a ScreenManager, and app-wide state
// (RandomizerSettings so far) that needs to survive a screen switch, and
// runs the frame loop. This is what main() calls into; everything else
// lives underneath it.
#pragma once

namespace bbr {

class Application {
public:
    // Brings up the platform, sets the initial screen, runs until a screen
    // requests exit, then hands off to Platform::Quit(). Never returns.
    [[noreturn]] void Run();
};

} // namespace bbr
