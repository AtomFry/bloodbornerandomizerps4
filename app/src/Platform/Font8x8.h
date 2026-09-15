// Font8x8.h - tiny hand-authored bitmap font, drawn through SDL_Renderer.
//
// Deliberately not FreeType: FreeType text rendering was only ever proven on
// the separate Scene2D raw-framebuffer path, never combined with the SDL2
// window-surface path this app uses. Staying on the exact SDL2 path already
// proven on hardware adds zero new libraries, and every extra library is a
// chance at a load-time bad-NID crash (docs/ps4-homebrew-findings.md §7).
//
// NOTE: uppercase A-Z and digits only. FindGlyph renders anything else BLANK,
// silently - check every user-facing string against that before adding it.
// Add glyphs here as later screens need them.
#pragma once

#include <SDL.h>

namespace bbr {

// Draws text using 8x8 glyphs, each pixel scaled up to `scale` x `scale`.
// Unknown characters (not in the table below) are drawn as blank space.
void DrawText8x8(SDL_Renderer* renderer, int x, int y, const char* text,
                  int scale, Uint8 r, Uint8 g, Uint8 b);

// Width in pixels of `text` if drawn with DrawText8x8 at the given scale.
int TextWidth8x8(const char* text, int scale);

} // namespace bbr
