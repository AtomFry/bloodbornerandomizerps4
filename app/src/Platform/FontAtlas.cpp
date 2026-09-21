#include "FontAtlas.h"

#include "FontAtlasData.h"
#include "Log.h"

#include <cstdlib>
#include <cstring>

namespace bbr {

namespace {

using fontdata::AtlasGlyph;
using fontdata::AtlasSize;

// One texture per baked size, built once in Init.
SDL_Texture* g_textures[fontdata::kSizeCount] = { nullptr };
bool g_ready = false;

// Maps the scale a screen passes to a baked size. Exact match when the scale
// is one of the baked ones (3/4/5/6, which is every scale the UI currently
// uses); otherwise the closest, so an unexpected value degrades in size rather
// than vanishing.
int SizeIndexForScale(int scale) {
    int best = 0;
    int bestDelta = -1;
    for (int i = 0; i < fontdata::kSizeCount; i++) {
        int delta = fontdata::kSizes[i].uiScale - scale;
        if (delta < 0) delta = -delta;
        if (bestDelta < 0 || delta < bestDelta) {
            bestDelta = delta;
            best = i;
        }
    }
    return best;
}

const AtlasGlyph* FindGlyph(const AtlasSize& size, unsigned char ch) {
    if (ch < fontdata::kFirstChar || ch > fontdata::kLastChar) return nullptr;
    return &size.glyphs[ch - fontdata::kFirstChar];
}

// Expands one size's tightly-packed coverage runs into a full RGBA atlas and
// uploads it. The baked data stores only each glyph's w*h ink box (162 KB for
// all four sizes) rather than the padded atlas rectangle (286 KB); this is
// where those runs get placed at their baked atlasX/atlasY.
SDL_Texture* BuildTexture(SDL_Renderer* renderer, const AtlasSize& size) {
    const int w = size.atlasWidth;
    const int h = size.atlasHeight;

    // White everywhere, alpha carrying the coverage - so one atlas can be
    // tinted to any Palette colour with SDL_SetTextureColorMod.
    Uint32* pixels = static_cast<Uint32*>(std::calloc(
        static_cast<size_t>(w) * static_cast<size_t>(h), sizeof(Uint32)));
    if (!pixels) return nullptr;

    for (int i = 0; i < fontdata::kGlyphCount; i++) {
        const AtlasGlyph& g = size.glyphs[i];
        if (g.width == 0 || g.height == 0) continue;   // space, and friends
        for (int row = 0; row < g.height; row++) {
            const unsigned char* src =
                size.coverage + g.dataOffset + static_cast<size_t>(row) * g.width;
            Uint32* dst = pixels + static_cast<size_t>(g.atlasY + row) * w + g.atlasX;
            for (int col = 0; col < g.width; col++) {
                // RGBA32 is byte-order aware, so build it through a surface
                // rather than hand-packing the channels here.
                dst[col] = 0x00FFFFFFu | (static_cast<Uint32>(src[col]) << 24);
            }
        }
    }

    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom(
        pixels, w, h, 32, w * static_cast<int>(sizeof(Uint32)),
        SDL_PIXELFORMAT_RGBA32);
    if (!surface) {
        std::free(pixels);
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    std::free(pixels);   // the texture owns its own copy now

    if (texture) SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    return texture;
}

} // namespace

bool FontAtlasInit(SDL_Renderer* renderer) {
    if (g_ready) return true;
    if (!renderer) return false;

    for (int i = 0; i < fontdata::kSizeCount; i++) {
        g_textures[i] = BuildTexture(renderer, fontdata::kSizes[i]);
        if (!g_textures[i]) {
            Log("FontAtlasInit: texture build failed, falling back to 8x8");
            FontAtlasShutdown();
            return false;
        }
    }

    g_ready = true;
    Log("FontAtlasInit ok");
    return true;
}

void FontAtlasShutdown() {
    for (int i = 0; i < fontdata::kSizeCount; i++) {
        if (g_textures[i]) SDL_DestroyTexture(g_textures[i]);
        g_textures[i] = nullptr;
    }
    g_ready = false;
}

bool FontAtlasReady() { return g_ready; }

void FontAtlasDrawText(SDL_Renderer* renderer, int x, int y, const char* text,
                       int scale, Uint8 r, Uint8 g, Uint8 b) {
    if (!g_ready || !text) return;

    const int index = SizeIndexForScale(scale);
    const AtlasSize& size = fontdata::kSizes[index];
    SDL_Texture* texture = g_textures[index];

    SDL_SetTextureColorMod(texture, r, g, b);

    // Callers pass the top-left, as DrawText8x8 did; the pen rides the baseline.
    const int baseline = y + size.ascent;
    int pen = x;

    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(text);
         *p; p++) {
        const AtlasGlyph* glyph = FindGlyph(size, *p);
        if (!glyph) continue;              // outside printable ASCII: skip
        if (glyph->width && glyph->height) {
            SDL_Rect src = { glyph->atlasX, glyph->atlasY,
                             glyph->width, glyph->height };
            SDL_Rect dst = { pen + glyph->bearingX, baseline - glyph->bearingY,
                             glyph->width, glyph->height };
            SDL_RenderCopy(renderer, texture, &src, &dst);
        }
        pen += glyph->advance;
    }
}

int FontAtlasTextWidth(const char* text, int scale) {
    if (!text) return 0;
    const AtlasSize& size = fontdata::kSizes[SizeIndexForScale(scale)];
    int width = 0;
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(text);
         *p; p++) {
        const AtlasGlyph* glyph = FindGlyph(size, *p);
        if (glyph) width += glyph->advance;
    }
    return width;
}

int FontAtlasLineHeight(int scale) {
    const AtlasSize& size = fontdata::kSizes[SizeIndexForScale(scale)];
    return size.ascent + size.descent;
}

} // namespace bbr
