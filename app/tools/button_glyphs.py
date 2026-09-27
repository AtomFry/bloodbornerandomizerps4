#!/usr/bin/env python3
"""The four PlayStation face buttons, as coverage masks the font atlas bakes.

Why glyphs, and why here
------------------------
Same argument mark_glyph.py makes, for the same reason. Renderer exposes
FillRect, FillRectBlend and DrawText and nothing in this app loads an image at
runtime, so a circle built from FillRects would be a staircase - and a circle
is the one shape a stack of axis-aligned bars cannot be. Baking each button at
an unused codepoint costs no new runtime code: they ride the same coverage
blob, the same atlas texture and the same SDL_SetTextureColorMod tint as every
letter, so they scale with the UI's scales and take whatever Palette colour the
caller asks for.

That tint is also what makes them COLOURED. The four buttons are colour-coded
on the console itself and in the game's own menus, and a prompt row that says
which button in its own colour is most of what makes a footer read as a
console footer. One DrawText call carries one colour, so Controls.cpp draws
each glyph separately from its label - see DrawPromptRow.

Why outlines and not filled discs
---------------------------------
The console draws the face buttons as an outlined ring with the symbol inside
it, and at 23px - scale 3, the footer size - a filled disc with a knocked-out
symbol turns into a blob. An outline survives the downsample because every
stroke is the same width, which is also why these are drawn per size rather
than downsampled from one large master.

The shapes
----------
Cross, circle, triangle and square, each inside a ring, in a unit square.
Strokes are in units of the square's height so they scale with everything
else. The ring is common to all four; only the symbol inside differs.
"""

try:
    from PIL import Image, ImageDraw
except ImportError:                                    # pragma: no cover
    raise SystemExit("PIL/Pillow is required: python -m pip install Pillow")

# The codepoints the buttons are baked at. They follow the Hunter's Mark at
# 127, so the baked range stays CONTIGUOUS - kFirstChar..kLastChar with no
# holes, which is what FontAtlas.cpp's FindGlyph assumes and the only property
# of this range anything depends on.
#
# FindGlyph already takes an unsigned char and DrawText already reinterprets
# its text as unsigned, so nothing in C++ changes to reach past 127.
CROSS_CODE    = 128
CIRCLE_CODE   = 129
TRIANGLE_CODE = 130
SQUARE_CODE   = 131

# The d-pad, which is not a face button but belongs with them: almost every
# footer in this app begins "UP DOWN ...", and that is the glyph the console
# puts there. It is drawn as an outlined plus rather than as four arrows - at
# 17px the arrowheads fill in and the whole thing reads as a blob.
DPAD_CODE     = 132

FIRST_CODE = CROSS_CODE
LAST_CODE  = DPAD_CODE
CODES = (CROSS_CODE, CIRCLE_CODE, TRIANGLE_CODE, SQUARE_CODE, DPAD_CODE)

NAMES = {
    CROSS_CODE:    "BUTTON CROSS",
    CIRCLE_CODE:   "BUTTON CIRCLE",
    TRIANGLE_CODE: "BUTTON TRIANGLE",
    SQUARE_CODE:   "BUTTON SQUARE",
    DPAD_CODE:     "DPAD",
}

# Ink box as a fraction of the em, and where it sits relative to the baseline.
#
# Sized against the CAPITAL rather than the line: a prompt reads as "<button>
# SELECT", so the ring should look like it belongs beside the word, not tower
# over it. EB Garamond's caps are ~0.71 em; 0.72 puts the ring a hair proud of
# the capital, which is how the console's own prompts sit.
#
# The ring is a circle, so the ink box is square and BUTTON_ASPECT is 1.0. It
# is written out rather than assumed because the metrics helper below is
# otherwise identical to the mark's, and the mark's is not square.
BUTTON_HEIGHT = 0.72      # em
BUTTON_ASPECT = 1.0       # width / height
BUTTON_DESCEND = 0.02     # em below the baseline - the ring is optically
                          # centred on the x-height, not sat on the baseline
BUTTON_SIDE = 0.09        # em of side bearing on each side

SS = 8                    # supersampling factor, as mark_glyph.py

# Stroke widths, in units of the square's height.
RING_W = 0.085
SYM_W  = 0.080

# How far in from the ring the symbol sits. The console's symbols are small
# inside a generous ring; crowding them makes the glyph read as a solid dot.
INSET = 0.30


def _ring(draw, size):
    r = RING_W * size
    draw.ellipse([r / 2, r / 2, size - r / 2, size - r / 2],
                 outline=255, width=max(1, int(round(r))))


def _stroke(draw, size, points, closed=False):
    w = max(1, int(round(SYM_W * size)))
    pts = [(x * size, y * size) for x, y in points]
    if closed:
        pts = pts + [pts[0]]
    draw.line(pts, fill=255, width=w, joint="curve")
    # Round the ends, as mark_glyph.py does: PIL's caps are square, and a
    # square cap on a diagonal reads as a cut-off staircase once downsampled.
    r = max(1, w // 2)
    for p in (pts[0], pts[-1]):
        draw.ellipse([p[0] - r, p[1] - r, p[0] + r, p[1] + r], fill=255)


def _symbol(draw, size, code):
    a, b = INSET, 1.0 - INSET

    if code == CROSS_CODE:
        _stroke(draw, size, [(a, a), (b, b)])
        _stroke(draw, size, [(b, a), (a, b)])

    elif code == CIRCLE_CODE:
        w = max(1, int(round(SYM_W * size)))
        draw.ellipse([a * size, a * size, b * size, b * size],
                     outline=255, width=w)

    elif code == TRIANGLE_CODE:
        # Nudged down a little: an equilateral triangle centred on the box
        # looks high, because its mass is in the lower half.
        top, bot = a - 0.02, b - 0.03
        _stroke(draw, size, [(0.5, top), (b, bot), (a, bot)], closed=True)

    elif code == SQUARE_CODE:
        # Pulled in further than the other three. A square's CORNERS are its
        # extreme points, so at the shared inset it crowds the ring in four
        # places where the circle and triangle crowd it in none.
        s, t = a + 0.03, b - 0.03
        _stroke(draw, size, [(s, s), (t, s), (t, t), (s, t)], closed=True)

    elif code == DPAD_CODE:
        # A plus, inscribed rather than ringed - the d-pad is not a round
        # button and giving it the ring would say it was one.
        _stroke(draw, size, [(0.5, 0.14), (0.5, 0.86)])
        _stroke(draw, size, [(0.14, 0.5), (0.86, 0.5)])

    else:                                              # pragma: no cover
        raise ValueError("not a button codepoint: %r" % (code,))


def button_mask(code, pixel_size):
    """(width, height, coverage bytes) for one button at this em size.

    Coverage is row-major, one byte per pixel, 0..255 - the same shape as
    PIL's font masks, so the generator treats it exactly like a glyph.
    """
    h = max(1, int(round(pixel_size * BUTTON_HEIGHT)))
    w = max(1, int(round(h * BUTTON_ASPECT)))

    big = Image.new("L", (w * SS, h * SS), 0)
    draw = ImageDraw.Draw(big)

    # The d-pad has no ring; every face button does.
    if code != DPAD_CODE:
        _ring(draw, h * SS)
    _symbol(draw, h * SS, code)

    small = big.resize((w, h), Image.BOX)
    return w, h, bytes(small.tobytes())


def button_metrics(code, pixel_size, ascent):
    """The baked metrics, in the same terms as a real glyph.

    bearingX is the pen-to-ink gap, bearingY the baseline-to-ink-top distance
    (positive up, as FontAtlas.cpp's `baseline - bearingY` expects), and the
    advance clears the ink plus a side bearing each side.
    """
    w, h, _ = button_mask(code, pixel_size)
    side = max(1, int(round(pixel_size * BUTTON_SIDE)))
    descend = int(round(pixel_size * BUTTON_DESCEND))
    return dict(bx=side, by=h - descend, adv=w + 2 * side)


if __name__ == "__main__":                             # pragma: no cover
    import sys
    size = int(sys.argv[1]) if len(sys.argv) > 1 else 33
    out = sys.argv[2] if len(sys.argv) > 2 else "button_preview.png"

    tiles = []
    for code in CODES:
        w, h, cov = button_mask(code, size)
        print("%-16s %dpx em -> %dx%d ink, metrics %s"
              % (NAMES[code], size, w, h, button_metrics(code, size, 0)))
        tiles.append(Image.frombytes("L", (w, h), cov))

    gap = 6
    sheet = Image.new("L", (sum(t.width for t in tiles) + gap * (len(tiles) + 1),
                            max(t.height for t in tiles) + gap * 2), 0)
    x = gap
    for t in tiles:
        sheet.paste(t, (x, gap))
        x += t.width + gap
    sheet.resize((sheet.width * 6, sheet.height * 6), Image.NEAREST).save(out)
    print("wrote %s" % out)
