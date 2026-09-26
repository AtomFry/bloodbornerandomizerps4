#!/usr/bin/env python3
"""Verify FontAtlasData.h against the typeface it was baked from.

Layer 2 of the three verification layers in CLAUDE.md section 3. A clean
cross-compile proves FontAtlas.cpp links; it proves nothing about whether the
baked metrics place glyphs correctly. Baseline and bearing mistakes are exactly
the class of bug that compiles fine and looks wrong only on a TV.

So this re-derives every number independently from the TTF and compares it to
what the generator wrote, then replays the C++ draw loop's arithmetic in Python
and checks the composed result against PIL's own rendering of the same string.

Usage:
    python font_atlas_verify.py [--font fonts/EBGaramond-Variable.ttf]
                                [--header ../src/Platform/FontAtlasData.h]
"""

import argparse
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import mark_glyph  # noqa: E402  - the source of truth for codepoint 127

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError:
    sys.exit("PIL/Pillow is required: python -m pip install Pillow")

HERE = os.path.dirname(os.path.abspath(__file__))

SAMPLES = [
    "Randomizer Seed",
    "Waste of Skin",
    "ENEMIES SKIPPED",
    "gypsy jamboree quaff 0123456789",
    "Wg. il, MW (1,024) - typography!",
]

# Deliberately loose: PIL lays a whole string out in one pass and may hint or
# round a shared origin differently from our per-glyph placement. One pixel of
# disagreement is rounding; more than that is a real metrics bug.
TOLERANCE = 1


def parse_header(path):
    """Pulls kSizes and the per-size glyph tables back out of the generated H."""
    text = open(path, "r").read()

    sizes = []
    block = re.search(r"const AtlasSize kSizes\[\d+\] = \{(.*?)\};", text, re.S)
    if not block:
        sys.exit("could not find kSizes in %s" % path)
    for line in block.group(1).strip().splitlines():
        m = re.match(r"\s*\{(\d+), (\d+), (\d+), (\d+), (\d+), (\d+), "
                     r"kGlyphs(\d+), kCoverage(\d+), (\d+)\},", line)
        if m:
            g = [int(v) for v in m.groups()]
            sizes.append(dict(uiScale=g[0], pixel=g[1], ascent=g[2],
                              descent=g[3], width=g[4], height=g[5],
                              coverageSize=g[8]))

    glyphs = {}
    for m in re.finditer(r"const AtlasGlyph kGlyphs(\d+)\[\d+\] = \{(.*?)\};",
                         text, re.S):
        scale = int(m.group(1))
        rows = []
        for line in m.group(2).strip().splitlines():
            g = re.match(r"\s*\{(\d+),(\d+),(\d+),(\d+),(\d+),(-?\d+),(-?\d+),(\d+)\},",
                         line)
            if g:
                v = [int(x) for x in g.groups()]
                rows.append(dict(off=v[0], x=v[1], y=v[2], w=v[3], h=v[4],
                                 bx=v[5], by=v[6], adv=v[7]))
        glyphs[scale] = rows
    return sizes, glyphs


def check_metrics(font_path, sizes, glyphs):
    """Every baked number re-derived straight from the TTF."""
    failures = []
    for size in sizes:
        font = ImageFont.truetype(font_path, size["pixel"])
        ascent, descent = font.getmetrics()
        if (ascent, descent) != (size["ascent"], size["descent"]):
            failures.append("scale %d: ascent/descent baked %s, font says %s"
                            % (size["uiScale"], (size["ascent"], size["descent"]),
                               (ascent, descent)))
        rows = glyphs[size["uiScale"]]
        # Stops one short of the mark: codepoint 127 is not in the typeface,
        # so check_mark speaks for it instead.
        for code in range(32, mark_glyph.MARK_CODE):
            g = rows[code - 32]
            ch = chr(code)
            mask = font.getmask(ch, mode="L")
            w, h = mask.size
            adv = int(round(font.getlength(ch)))
            # A glyph with either dimension zero has no pixels at all - PIL
            # reports space as (5, 0), width but no height. The generator
            # stores those uniformly as (0, 0) plus an advance, and
            # FontAtlas.cpp skips the blit either way, so normalise before
            # comparing rather than flagging a difference that cannot render.
            if w == 0 or h == 0:
                w = h = 0
            if (g["w"], g["h"]) != (w, h):
                failures.append("scale %d %r: size baked %s, font says %s"
                                % (size["uiScale"], ch, (g["w"], g["h"]), (w, h)))
            if g["adv"] != adv:
                failures.append("scale %d %r: advance baked %d, font says %d"
                                % (size["uiScale"], ch, g["adv"], adv))
            if w and h:
                bbox = font.getbbox(ch)
                if g["bx"] != bbox[0] or g["by"] != ascent - bbox[1]:
                    failures.append(
                        "scale %d %r: bearings baked %s, font says %s"
                        % (size["uiScale"], ch, (g["bx"], g["by"]),
                           (bbox[0], ascent - bbox[1])))
    return failures


def check_mark(sizes, glyphs):
    """The Hunter's Mark at codepoint 127, re-derived from mark_glyph.py.

    The mark is not in the typeface, so check_metrics cannot speak for it -
    that loop stops one short. It gets the same treatment here: every baked
    number re-drawn independently and compared, plus the two things that are
    true of the mark and of nothing else. It must HAVE ink - a mark that baked
    empty would simply not draw, and the rail would look exactly as it did the
    day before this existed - and it must sit inside the typeface's own
    vertical band, because WorldsScreen draws it on a text row whose focus bar
    is sized from the atlas's ink box.
    """
    failures = []
    for size in sizes:
        scale = size["uiScale"]
        g = glyphs[scale][mark_glyph.MARK_CODE - 32]
        w, h, ink = mark_glyph.mark_mask(size["pixel"])
        m = mark_glyph.mark_metrics(size["pixel"], size["ascent"])

        if (g["w"], g["h"]) != (w, h):
            failures.append("scale %d mark: size baked %s, drawn %s"
                            % (scale, (g["w"], g["h"]), (w, h)))
        if (g["bx"], g["by"], g["adv"]) != (m["bx"], m["by"], m["adv"]):
            failures.append("scale %d mark: metrics baked %s, drawn %s"
                            % (scale, (g["bx"], g["by"], g["adv"]),
                               (m["bx"], m["by"], m["adv"])))
        if not any(ink):
            failures.append("scale %d mark: drawn glyph is blank" % scale)
        if g["adv"] <= 0:
            failures.append("scale %d mark: advance %d" % (scale, g["adv"]))

        # Drawn at a row's y, the ink spans [ascent - by, ascent - by + h)
        # below that y. Both edges must stay inside the line the atlas reports.
        top = size["ascent"] - g["by"]
        bottom = top + g["h"]
        if top < 0 or bottom > size["ascent"] + size["descent"]:
            failures.append("scale %d mark: ink %d..%d escapes the line 0..%d"
                            % (scale, top, bottom,
                               size["ascent"] + size["descent"]))
    return failures


def check_layout(font_path, sizes, glyphs):
    """Replays FontAtlasDrawText's arithmetic and compares against PIL."""
    failures = []
    for size in sizes:
        font = ImageFont.truetype(font_path, size["pixel"])
        rows = glyphs[size["uiScale"]]
        for sample in SAMPLES:
            w = max(1, sum(rows[ord(c) - 32]["adv"] for c in sample) + 64)
            h = size["ascent"] + size["descent"] + 32

            # Ours: pen on the baseline, dst = (pen + bearingX,
            # baseline - bearingY) - exactly what FontAtlas.cpp does.
            ours = Image.new("L", (w, h), 0)
            baseline = size["ascent"]
            pen = 0
            for c in sample:
                g = rows[ord(c) - 32]
                if g["w"] and g["h"]:
                    mask = font.getmask(c, mode="L")
                    glyph = Image.new("L", mask.size)
                    glyph.putdata(list(mask))
                    ours.paste(glyph, (pen + g["bx"], baseline - g["by"]))
                pen += g["adv"]

            # Theirs: PIL renders the whole run itself.
            theirs = Image.new("L", (w, h), 0)
            ImageDraw.Draw(theirs).text((0, 0), sample, fill=255, font=font)

            ob, tb = ours.getbbox(), theirs.getbbox()
            if ob is None or tb is None:
                failures.append("scale %d %r: one render was empty"
                                % (size["uiScale"], sample))
                continue
            for i, axis in enumerate(("left", "top", "right", "bottom")):
                if abs(ob[i] - tb[i]) > TOLERANCE:
                    failures.append(
                        "scale %d %r: %s edge ours=%d PIL=%d"
                        % (size["uiScale"], sample, axis, ob[i], tb[i]))
    return failures


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--font", default=os.path.join(HERE, "fonts",
                                                   "EBGaramond-Variable.ttf"))
    ap.add_argument("--header", default=os.path.join(
        HERE, "..", "src", "Platform", "FontAtlasData.h"))
    args = ap.parse_args()

    sizes, glyphs = parse_header(args.header)
    print("parsed %d baked sizes: %s"
          % (len(sizes), ", ".join("scale %d = %dpx" % (s["uiScale"], s["pixel"])
                                   for s in sizes)))

    print("\nchecking baked metrics against the typeface...")
    metric_failures = check_metrics(args.font, sizes, glyphs)
    print("  %d glyphs x %d sizes: %s"
          % (mark_glyph.MARK_CODE - 32, len(sizes),
             "OK" if not metric_failures
             else "%d MISMATCHES" % len(metric_failures)))

    print("\nchecking the Hunter's Mark against mark_glyph.py...")
    mark_failures = check_mark(sizes, glyphs)
    print("  1 glyph x %d sizes: %s"
          % (len(sizes), "OK" if not mark_failures
             else "%d MISMATCHES" % len(mark_failures)))

    print("\nreplaying the C++ draw loop against PIL's own layout...")
    layout_failures = check_layout(args.font, sizes, glyphs)
    print("  %d samples x %d sizes: %s"
          % (len(SAMPLES), len(sizes), "OK" if not layout_failures
             else "%d MISMATCHES" % len(layout_failures)))

    failures = metric_failures + mark_failures + layout_failures
    if failures:
        print("\nFAILURES:")
        for f in failures[:40]:
            print("  " + f)
        if len(failures) > 40:
            print("  ... and %d more" % (len(failures) - 40))
        return 1

    print("\nPASS - baked metrics match the typeface, the mark matches "
          "its drawing, and the draw maths lands where PIL puts it.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
