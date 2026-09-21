// FontAtlas.h - text drawn from a pre-rasterised typeface instead of the
// 8x8 bitmap table in Font8x8.cpp.
//
// The typeface (EB Garamond, SIL OFL 1.1) is rasterised on the PC by
// app/tools/gen_font_atlas.py and baked into FontAtlasData.h. Nothing new is
// linked on the PS4 side: FreeType runs at author time, and the app only ever
// blits pixels through the SDL2 calls it already uses. That keeps the
// "link fewer libraries" rule in docs/ps4-homebrew-findings.md section 7
// intact - no new .sprx, so no new load-time bad-NID risk.
//
// Unlike Font8x8 this covers all of printable ASCII (32..126), including
// lowercase, so the "unknown characters render blank, silently" trap in
// docs/known-traps.md no longer applies to new user-facing strings.
#pragma once

#include <SDL.h>

namespace bbr {

// Builds one texture per baked size. Call once, after the renderer exists and
// before any DrawText. Returns false if a texture could not be created, in
// which case the caller should fall back to Font8x8.
bool FontAtlasInit(SDL_Renderer* renderer);

// Releases the textures. Safe to call without a successful Init.
void FontAtlasShutdown();

// True once Init has succeeded - lets Renderer pick the atlas path or the
// 8x8 fallback without duplicating the decision.
bool FontAtlasReady();

// Draws `text` with its TOP-LEFT at (x, y), matching DrawText8x8's contract so
// existing screens keep their layout maths. Internally the pen sits on the
// baseline at y + ascent. `scale` is the same 3/4/5/6 the UI already passes;
// anything else clamps to the nearest baked size.
void FontAtlasDrawText(SDL_Renderer* renderer, int x, int y, const char* text,
                       int scale, Uint8 r, Uint8 g, Uint8 b);

// Width in pixels of `text` at `scale` - the sum of per-glyph advances, so
// unlike the 8x8 font this is NOT characterCount * fixedAdvance.
int FontAtlasTextWidth(const char* text, int scale);

// Height of one line at `scale` (ascent + descent). Screens that space rows by
// hand can use this instead of assuming 8 * scale.
int FontAtlasLineHeight(int scale);

} // namespace bbr
