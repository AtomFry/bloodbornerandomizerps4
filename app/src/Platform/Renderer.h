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

    // A solid rectangle. Separators, rules and the focus-highlight bar are all
    // 2px-or-thicker fills rather than SDL_RenderDrawLine, so this is the only
    // new shape primitive the categorised screens need. The SDL calls behind
    // it are the same two Font8x8 used for every glyph pixel, so nothing here
    // is unproven on the console.
    void FillRect(int x, int y, int w, int h, Uint8 r, Uint8 g, Uint8 b);

    // The same rectangle blended over what is already there, for the dimmed
    // parent behind an overlay. SDL_SetRenderDrawBlendMode is the one entry
    // point in this feature that has never run on this hardware, which is why
    // the picker's select-all prompt exercises it before any new screen
    // depends on it. The blend mode is restored to NONE before returning so a
    // later Clear or FillRect is unaffected either way.
    void FillRectBlend(int x, int y, int w, int h, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

    // Height of one line of text at `scale`, from the atlas's own metrics -
    // so screens that space rows by hand stop assuming 8 * scale, which is
    // true only of the Font8x8 fallback.
    int LineHeight(int scale) const;

private:
    SDL_Renderer* renderer_ = nullptr;
};

} // namespace bbr
