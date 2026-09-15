// Platform.h - owns SDL2 init/shutdown, the window, and process lifecycle.
// This is the only layer allowed to know about SDL2 or OpenOrbis headers;
// Application/UI/Screen code talks to Renderer/Input/Log only.
#pragma once

#include "Input.h"
#include "Log.h"
#include "Renderer.h"

#include <string>

namespace bbr {

class Platform {
public:
    // Resets the log, brings up SDL2 + window + renderer + input. On
    // failure, fills *err and the caller should not enter the frame loop.
    bool Init(std::string* err);
    void Shutdown();

    // Drains pending SDL events into `input`. Returns false if the OS asked
    // the app to quit (SDL_QUIT / window close).
    bool PumpEvents(Input& input);

    // Pushes whatever Renderer has drawn this frame to the screen. Kept on
    // Platform (not Renderer) because presenting needs the window handle,
    // which nothing above this layer is allowed to hold.
    void Present();

    Renderer& GetRenderer() { return renderer_; }
    Input&    GetInput()    { return input_; }

    // Ends the app and hands control back to the XMB via
    // sceSystemServiceLoadExec("exit", NULL). Does not return on success.
    // See Platform.cpp for why this exists instead of `return` from main():
    // every official OpenOrbis sample avoids returning from main() entirely,
    // and M1 confirmed on hardware that doing so crashes with SIGSYS.
    [[noreturn]] void Quit();

private:
    struct SDL_Window* window_ = nullptr;
    Input    input_;
    Renderer renderer_;
};

} // namespace bbr
