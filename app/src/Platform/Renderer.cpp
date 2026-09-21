#include "Renderer.h"

#include "Font8x8.h"
#include "FontAtlas.h"

namespace bbr {

bool Renderer::Init(SDL_Surface* windowSurface) {
    renderer_ = SDL_CreateSoftwareRenderer(windowSurface);
    if (!renderer_) return false;

    // The atlas is the normal path; Font8x8 stays as the fallback so a failure
    // here costs legibility, not the whole app. Both honour the same
    // "(x, y) is the text's top-left" contract, so no screen has to care which
    // one answered.
    FontAtlasInit(renderer_);
    return true;
}

void Renderer::Shutdown() {
    FontAtlasShutdown();
    if (renderer_) SDL_DestroyRenderer(renderer_);
    renderer_ = nullptr;
}

void Renderer::Clear(Uint8 r, Uint8 g, Uint8 b) {
    SDL_SetRenderDrawColor(renderer_, r, g, b, 255);
    SDL_RenderClear(renderer_);
}

void Renderer::Present(SDL_Window* window) {
    SDL_UpdateWindowSurface(window);
}

void Renderer::DrawText(int x, int y, const char* text, int scale, Uint8 r, Uint8 g, Uint8 b) {
    if (FontAtlasReady()) {
        FontAtlasDrawText(renderer_, x, y, text, scale, r, g, b);
        return;
    }
    DrawText8x8(renderer_, x, y, text, scale, r, g, b);
}

int Renderer::TextWidth(const char* text, int scale) const {
    if (FontAtlasReady()) return FontAtlasTextWidth(text, scale);
    return TextWidth8x8(text, scale);
}

void Renderer::FillRect(int x, int y, int w, int h, Uint8 r, Uint8 g, Uint8 b) {
    SDL_Rect rect = { x, y, w, h };
    SDL_SetRenderDrawColor(renderer_, r, g, b, 255);
    SDL_RenderFillRect(renderer_, &rect);
}

void Renderer::FillRectBlend(int x, int y, int w, int h, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    SDL_Rect rect = { x, y, w, h };
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer_, r, g, b, a);
    SDL_RenderFillRect(renderer_, &rect);
    // Back to NONE unconditionally: every other draw in the app assumes the
    // opaque mode SDL starts in, and a leaked BLEND would make Clear and
    // FillRect behave differently depending on what drew last frame.
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_NONE);
}

int Renderer::LineHeight(int scale) const {
    if (FontAtlasReady()) return FontAtlasLineHeight(scale);
    return 8 * scale;
}

} // namespace bbr
