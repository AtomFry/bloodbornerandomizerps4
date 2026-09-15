#include "Renderer.h"

#include "Font8x8.h"

namespace bbr {

bool Renderer::Init(SDL_Surface* windowSurface) {
    renderer_ = SDL_CreateSoftwareRenderer(windowSurface);
    return renderer_ != nullptr;
}

void Renderer::Shutdown() {
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
    DrawText8x8(renderer_, x, y, text, scale, r, g, b);
}

int Renderer::TextWidth(const char* text, int scale) const {
    return TextWidth8x8(text, scale);
}

} // namespace bbr
