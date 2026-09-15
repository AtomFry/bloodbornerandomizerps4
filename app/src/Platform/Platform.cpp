#include "Platform.h"

#include "Log.h"

#include <SDL.h>

#include <stdio.h>
#include <string.h>

#include <orbis/libkernel.h>
#include <orbis/Sysmodule.h>
#include <orbis/SystemService.h>

namespace bbr {

void Log(const char* text) {
    const char* dirs[] = { "/data/bbrandomizer", "/data" };
    for (const char* dir : dirs) {
        if (dir == dirs[0]) sceKernelMkdir(dir, 0777);

        char path[256];
        snprintf(path, sizeof(path), "%s/live.log", dir);

        int fd = sceKernelOpen(path, 0x0001 /*O_WRONLY*/ | 0x0200 /*O_CREAT*/ | 0x0008 /*O_APPEND*/, 0777);
        if (fd < 0) continue;

        sceKernelWrite(fd, text, strlen(text));
        sceKernelWrite(fd, "\n", 1);
        sceKernelFsync(fd);
        sceKernelClose(fd);
    }
}

bool Platform::Init(std::string* err) {
    sceKernelMkdir("/data/bbrandomizer", 0777);
    sceKernelUnlink("/data/bbrandomizer/live.log");
    Log("bbrandomizer starting");

    sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_SYSTEM_SERVICE);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) != 0) {
        if (err) *err = "SDL_Init failed";
        return false;
    }
    Log("SDL_Init ok");

    window_ = reinterpret_cast<struct SDL_Window*>(SDL_CreateWindow(
        "main", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        kScreenWidth, kScreenHeight, 0));
    if (!window_) {
        if (err) *err = "SDL_CreateWindow failed";
        SDL_Quit();
        return false;
    }
    Log("SDL_CreateWindow ok");

    SDL_Surface* windowSurface = SDL_GetWindowSurface(reinterpret_cast<SDL_Window*>(window_));
    if (!windowSurface) {
        if (err) *err = "SDL_GetWindowSurface failed";
        SDL_Quit();
        return false;
    }
    Log("SDL_GetWindowSurface ok");

    if (!renderer_.Init(windowSurface)) {
        if (err) *err = "Renderer::Init failed";
        SDL_Quit();
        return false;
    }
    Log("Renderer::Init ok");

    input_.Open();
    Log("Input::Open done");

    return true;
}

void Platform::Shutdown() {
    Log("shutting down");
    input_.Close();
    renderer_.Shutdown();
    if (window_) SDL_DestroyWindow(reinterpret_cast<SDL_Window*>(window_));
    window_ = nullptr;
    SDL_Quit();
}

void Platform::Present() {
    renderer_.Present(reinterpret_cast<SDL_Window*>(window_));
}

bool Platform::PumpEvents(Input& input) {
    input.BeginFrame();

    SDL_Event ev;
    while (SDL_PollEvent(&ev) != 0) {
        if (ev.type == SDL_QUIT) return false;
        input.HandleEvent(ev);
    }
    return true;
}

void Platform::Quit() {
    Log("calling sceSystemServiceLoadExec(\"exit\")");
    sceSystemServiceLoadExec("exit", NULL);

    // Should not be reached - if it is, avoid the known-bad `return` path
    // (see header comment) and idle instead so the log line above survives.
    Log("sceSystemServiceLoadExec returned - idling instead of exiting main()");
    for (;;) sceKernelUsleep(1 * 1000 * 1000);
}

} // namespace bbr
