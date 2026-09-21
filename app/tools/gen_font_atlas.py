#!/usr/bin/env python3
"""Bake a real typeface into src/Platform/FontAtlasData.h.

Why this exists
---------------
The app used to draw text with Font8x8.cpp: 8x8 one-bit glyphs, uppercase and
digits only, scaled up by integer nearest-neighbour and drawn one
SDL_RenderFillRect per lit pixel. That is why the UI reads as blocky and why
every user-facing string had to be checked against a 42-entry glyph table
(docs/known-traps.md).

This script moves the expensive part - rasterising a real typeface with
FreeType - onto the PC, at author time. The PS4 only ever blits pixels, so the
app links no new library and takes no new load-time NID risk
(docs/ps4-homebrew-findings.md section 7). It is the same "generate on PC, bake
constants into a header" idiom already used by ModelSizeTable.h and
NpcScalingTable.h.

What it emits
-------------
For each text scale the UI actually uses (3, 4, 5, 6 - see the kScale*
constants under src/UI/), one baked size containing:

  * per-glyph metrics: position in the atlas, ink size, bearings, advance
  * the glyph coverage bytes, TIGHTLY packed (sum of w*h), not the padded
    atlas rectangle - that is 162 KB instead of 286 KB for the same pixels

The runtime (FontAtlas.cpp) expands those tight runs into one SDL_Texture per
size at startup, positioning each glyph at the atlasX/atlasY baked here.

Coverage, not colour: each byte is how much ink covers that pixel (0..255).
The runtime turns it into white-with-alpha and tints it per draw with
SDL_SetTextureColorMod, so one atlas serves every Palette colour.

Usage
-----
    python gen_font_atlas.py [--font fonts/EBGaramond-Variable.ttf]
                             [--out ../src/Platform/FontAtlasData.h]

Re-run it whenever the typeface or the set of scales changes, then rebuild.
"""

import argparse
import os
import sys

try:
    from PIL import ImageFont
except ImportError:
    sys.exit("PIL/Pillow is required: python -m pip install Pillow")

HERE = os.path.dirname(os.path.abspath(__file__))

# ASCII printable. The whole point of the change is that this is a contiguous
# range with no holes, so no caller has to check a glyph table any more.
FIRST_CH, LAST_CH = 32, 126

# UI text scale -> em size in pixels.
#
# NOT 8*scale, which is the obvious mapping and the wrong one. Font8x8's glyphs
# filled their 8-pixel box, so a capital stood 8*scale pixels tall. In a real
# typeface the em is the design size, not the cap height: EB Garamond's capitals
# are about 0.71 em. Feeding it 8*scale therefore renders capitals at ~71% of
# the height they used to be - a legibility regression on a TV, and one nobody
# asked for.
#
# 11*scale puts cap height back within a pixel or two of the old font at every
# scale (23 vs 24, 30 vs 32, 39 vs 40, 47 vs 48) while keeping the resulting
# line height inside the tightest row spacing the UI uses (52 px in the enemy
# picker at scale 3, against a 44 px line).
#
# Every scale the UI passes must appear here; FontAtlas.cpp clamps anything
# else to the nearest. If text reads too small or too large on hardware, change
# these numbers and re-run - no C++ change is needed.
SCALES = ((3, 33), (4, 44), (5, 55), (6, 66))

ATLAS_WIDTH = 512   # shelf-packing width; height falls out of the packing
PAD = 1             # transparent gutter so neighbours cannot bleed in


def rasterize(font_path, pixel_size):
    """One baked size: metrics, shelf-packed layout, tight coverage bytes."""
    font = ImageFont.truetype(font_path, pixel_size)
    ascent, descent = font.getmetrics()

    glyphs, blob = [], bytearray()
    x = y = row_height = 0

    for code in range(FIRST_CH, LAST_CH + 1):
        ch = chr(code)
        mask = font.getmask(ch, mode="L")
        w, h = mask.size
        bbox = font.getbbox(ch)
        advance = int(round(font.getlength(ch)))

        if w == 0 or h == 0:
            # Space and anything else with no ink: advance only, no pixels.
            glyphs.append(dict(x=0, y=0, w=0, h=0, bx=0, by=0,
                               adv=advance, off=len(blob)))
            continue

        if x + w + PAD > ATLAS_WIDTH:      # start a new shelf
            x = 0
            y += row_height + PAD
            row_height = 0

        glyphs.append(dict(
            x=x, y=y, w=w, h=h,
            bx=bbox[0],                    # pen -> left edge of ink
            by=ascent - bbox[1],           # baseline -> top edge of ink
            adv=advance,
            off=len(blob),
        ))
        blob += bytes(mask)                # tight w*h coverage, row-major

        x += w + PAD
        row_height = max(row_height, h)

    atlas_height = y + row_height + PAD
    return dict(pixel=pixel_size, ascent=ascent, descent=descent,
                width=ATLAS_WIDTH, height=atlas_height,
                glyphs=glyphs, blob=bytes(blob))


def emit_blob(out, name, blob):
    out.append("const unsigned char %s[%d] = {" % (name, len(blob)))
    for i in range(0, len(blob), 24):
        out.append("    " + "".join("%d," % b for b in blob[i:i + 24]))
    out.append("};")
    out.append("")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--font", default=os.path.join(HERE, "fonts",
                                                   "EBGaramond-Variable.ttf"))
    ap.add_argument("--out", default=os.path.join(
        HERE, "..", "src", "Platform", "FontAtlasData.h"))
    args = ap.parse_args()

    if not os.path.isfile(args.font):
        sys.exit("font not found: %s" % args.font)

    baked = [rasterize(args.font, px) for _, px in SCALES]

    out = []
    out.append("// FontAtlasData.h - GENERATED by app/tools/gen_font_atlas.py.")
    out.append("// Do not hand-edit; re-run the generator instead.")
    out.append("//")
    out.append("// Typeface: EB Garamond (SIL Open Font License 1.1).")
    out.append("// The license text ships beside the source font at")
    out.append("// app/tools/fonts/OFL.txt and must stay with any redistribution.")
    out.append("//")
    out.append("// Each glyph's coverage is stored tightly (w*h bytes, row-major,")
    out.append("// 0 = no ink, 255 = solid) at `dataOffset` into the size's blob.")
    out.append("// FontAtlas.cpp expands these into one texture per size at startup.")
    out.append("#pragma once")
    out.append("")
    out.append("namespace bbr {")
    out.append("namespace fontdata {")
    out.append("")
    out.append("struct AtlasGlyph {")
    out.append("    unsigned int  dataOffset;   // into the size's coverage blob")
    out.append("    unsigned short atlasX, atlasY;")
    out.append("    unsigned char width, height;")
    out.append("    short bearingX;             // pen -> left edge of ink")
    out.append("    short bearingY;             // baseline -> top edge of ink")
    out.append("    unsigned short advance;     // pen movement after this glyph")
    out.append("};")
    out.append("")
    out.append("struct AtlasSize {")
    out.append("    int uiScale;                // the scale value screens pass")
    out.append("    int pixelSize, ascent, descent;")
    out.append("    int atlasWidth, atlasHeight;")
    out.append("    const AtlasGlyph* glyphs;   // FirstChar..LastChar inclusive")
    out.append("    const unsigned char* coverage;")
    out.append("    unsigned int coverageSize;")
    out.append("};")
    out.append("")
    out.append("constexpr int kFirstChar = %d;" % FIRST_CH)
    out.append("constexpr int kLastChar  = %d;" % LAST_CH)
    out.append("constexpr int kGlyphCount = %d;" % (LAST_CH - FIRST_CH + 1))
    out.append("constexpr int kSizeCount  = %d;" % len(SCALES))
    out.append("")

    for (scale, _), b in zip(SCALES, baked):
        emit_blob(out, "kCoverage%d" % scale, b["blob"])
        out.append("const AtlasGlyph kGlyphs%d[%d] = {" % (scale, len(b["glyphs"])))
        for code, g in zip(range(FIRST_CH, LAST_CH + 1), b["glyphs"]):
            ch = chr(code)
            label = "space" if ch == " " else ch
            out.append("    {%d,%d,%d,%d,%d,%d,%d,%d}, // '%s'"
                       % (g["off"], g["x"], g["y"], g["w"], g["h"],
                          g["bx"], g["by"], g["adv"], label))
        out.append("};")
        out.append("")

    out.append("const AtlasSize kSizes[%d] = {" % len(SCALES))
    for (scale, _), b in zip(SCALES, baked):
        out.append("    {%d, %d, %d, %d, %d, %d, kGlyphs%d, kCoverage%d, %d},"
                   % (scale, b["pixel"], b["ascent"], b["descent"],
                      b["width"], b["height"], scale, scale, len(b["blob"])))
    out.append("};")
    out.append("")
    out.append("} // namespace fontdata")
    out.append("} // namespace bbr")
    out.append("")

    dest = os.path.abspath(args.out)
    with open(dest, "w", newline="\n") as fh:
        fh.write("\n".join(out))

    total = sum(len(b["blob"]) for b in baked)
    print("wrote %s" % dest)
    for (scale, px), b in zip(SCALES, baked):
        print("  scale %d (%2dpx): atlas %dx%-4d ascent %2d  coverage %6d bytes"
              % (scale, px, b["width"], b["height"], b["ascent"], len(b["blob"])))
    print("  total coverage %d bytes (%.0f KB), header %.0f KB"
          % (total, total / 1024.0, os.path.getsize(dest) / 1024.0))
    return 0


if __name__ == "__main__":
    sys.exit(main())
