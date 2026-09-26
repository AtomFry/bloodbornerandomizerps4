#!/usr/bin/env python3
"""The Hunter's Mark rune, as a coverage mask the font atlas can bake.

Why a font glyph and not a sprite
---------------------------------
Renderer exposes exactly three primitives - FillRect, FillRectBlend and
DrawText - and nothing in this app loads an image at runtime. A mark drawn
from FillRects would be a stack of axis-aligned bars, which is the one thing
this shape is not. Baking it into FontAtlasData.h at an unused codepoint costs
no new runtime code at all: it rides the same tight-coverage blob, the same
atlas texture and the same SDL_SetTextureColorMod tint as every letter, so it
scales with the UI's scales and takes whatever Palette colour the caller asks
for (see FontAtlas.h).

Why vector and not a PNG
------------------------
The mark is drawn here as strokes in a unit square and rasterised per baked
size, 8x supersampled and box-filtered down. A single PNG would have to be
downsampled to 23 px for scale 3 and would silt up; drawing it at each size
keeps the strokes even. It also means no binary asset enters the repo for a
shape that is nine line segments.

The shape: a vertical stem with a hooked head, two arms sweeping down and out
below the head, a kite closing beneath them, and the detached fleck on the
left that the in-game rune has. It is a deliberate simplification - at 23 px
the fine breaks in the original painted strokes disappear into grey, so the
strokes that survive are drawn cleanly rather than suggested.
"""

try:
    from PIL import Image, ImageDraw
except ImportError:                                    # pragma: no cover
    raise SystemExit("PIL/Pillow is required: python -m pip install Pillow")

# The codepoint the mark is baked at. 127 (DEL) is the first value past
# printable ASCII, so the baked range stays contiguous - kFirstChar..kLastChar
# with no holes, which is what FontAtlas.cpp's FindGlyph assumes.
MARK_CODE = 127
MARK_CHAR = chr(MARK_CODE)

# Ink box as a fraction of the em, and where it sits relative to the baseline.
# The mark stands well clear of a capital (EB Garamond's caps are ~0.71 em) so
# it reads as an emblem beside a word rather than as a letter, and it sits on
# the baseline with a little of its tail below, like a comma. It is sized
# against the LINE, not the capital: at 1.02 em its ink spans 2..36 of scale
# 3's 44-pixel line box, which font_atlas_verify.py asserts at every size -
# push it much past this and the rune starts colliding with the row above.
MARK_HEIGHT = 1.02      # em
MARK_ASPECT = 0.55      # width / height - the rune is tall and narrow
MARK_DESCEND = 0.07     # em below the baseline
MARK_SIDE = 0.10        # em of side bearing on each side

SS = 8                  # supersampling factor

# Strokes in a unit square, y down: (points, width). Widths are in units of
# the square's height, so they scale with everything else.
STROKES = [
    # The stem, drawn in two segments so it kinks very slightly at the arms -
    # a straight line here looks machined next to the sweeping arms.
    ([(0.50, 0.03), (0.505, 0.38)], 0.068),
    ([(0.505, 0.38), (0.475, 0.88)], 0.068),
    # The hooked head: a short stroke set off to the right of the stem top.
    ([(0.66, 0.06), (0.645, 0.20)], 0.048),
    # The arms, sweeping down and out from just below the head.
    ([(0.505, 0.30), (0.17, 0.50)], 0.060),
    ([(0.505, 0.30), (0.86, 0.46)], 0.060),
    # The kite closing under the arms and meeting the stem again.
    ([(0.86, 0.46), (0.49, 0.79)], 0.055),
    ([(0.28, 0.60), (0.49, 0.79)], 0.055),
    # The detached fleck on the left, where the kite's left side breaks.
    ([(0.17, 0.50), (0.10, 0.58)], 0.048),
]

# The tail: the stem thickens into a drop rather than stopping square.
TAIL = [(0.475, 0.80), (0.515, 0.88), (0.475, 0.97), (0.435, 0.88)]


def mark_mask(pixel_size):
    """(width, height, coverage bytes) for the mark at this em size.

    Coverage is row-major, one byte per pixel, 0..255 - the same shape as
    PIL's font masks, so the generator can treat it exactly like a glyph.
    """
    h = max(1, int(round(pixel_size * MARK_HEIGHT)))
    w = max(1, int(round(h * MARK_ASPECT)))

    big = Image.new("L", (w * SS, h * SS), 0)
    draw = ImageDraw.Draw(big)

    def px(p):
        return (p[0] * w * SS, p[1] * h * SS)

    for points, width in STROKES:
        draw.line([px(p) for p in points], fill=255,
                  width=max(1, int(round(width * h * SS))), joint="curve")
        # Round the ends: PIL's line caps are square, and a square cap on a
        # diagonal stroke reads as a cut-off pixel staircase once downsampled.
        r = max(1, int(round(width * h * SS / 2.0)))
        for p in (points[0], points[-1]):
            cx, cy = px(p)
            draw.ellipse([cx - r, cy - r, cx + r, cy + r], fill=255)

    draw.polygon([px(p) for p in TAIL], fill=255)

    small = big.resize((w, h), Image.BOX)
    return w, h, bytes(small.tobytes())


def mark_metrics(pixel_size, ascent):
    """The baked metrics for the mark, in the same terms as a real glyph.

    bearingX is the pen-to-ink gap, bearingY the baseline-to-ink-top distance
    (positive up, as FontAtlas.cpp's `baseline - bearingY` expects), and the
    advance clears the ink plus a side bearing each side.
    """
    w, h, _ = mark_mask(pixel_size)
    side = max(1, int(round(pixel_size * MARK_SIDE)))
    descend = int(round(pixel_size * MARK_DESCEND))
    return dict(bx=side, by=h - descend, adv=w + 2 * side)


if __name__ == "__main__":                             # pragma: no cover
    import sys
    size = int(sys.argv[1]) if len(sys.argv) > 1 else 33
    w, h, cov = mark_mask(size)
    print("%dpx em -> %dx%d ink, metrics %s" % (size, w, h, mark_metrics(size, 0)))
    img = Image.frombytes("L", (w, h), cov)
    out = sys.argv[2] if len(sys.argv) > 2 else "mark_preview.png"
    img.resize((w * 6, h * 6), Image.NEAREST).save(out)
    print("wrote %s" % out)
