// Renderer.h - the only place in this app that touches SDL_Renderer/SDL_Surface
// directly. Screens draw through this, never through SDL2 itself - that's the
// whole point of the Platform/UI split introduced in M2.
#pragma once

#include <SDL.h>

namespace bbr {

// The window size Platform creates (see Platform.cpp) - centralized here so
// every screen's centering math and Platform's SDL_CreateWindow call share
// one source of truth instead of four separate copy-pasted 1920/1080 pairs.
constexpr int kScreenWidth  = 1920;
constexpr int kScreenHeight = 1080;

class Renderer {
public:
    // windowSurface is used to create a software renderer, same as M1.
    bool Init(SDL_Surface* windowSurface);
    void Shutdown();

    void Clear(Uint8 r, Uint8 g, Uint8 b);
    void Present(SDL_Window* window);

    void DrawText(int x, int y, const char* text, int scale, Uint8 r, Uint8 g, Uint8 b);
    int  TextWidth(const char* text, int scale) const;

private:
    SDL_Renderer* renderer_ = nullptr;
};

} // namespace bbr
