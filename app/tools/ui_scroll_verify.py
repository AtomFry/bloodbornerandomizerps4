"""Mirror of Controls.cpp's scroll math + a geometry check per screen.

No host C++ compiler exists in this project, so this is the standing substitute:
re-implement the arithmetic exactly as written and assert the properties that
matter. Pixel constants are read off the real draw calls.

Heights are NOT assumed. This used to model a line of text as 8 * scale, which
is Font8x8.cpp's fixed cell and has not been the live text path since the font
atlas landed. The real glyphs are EB Garamond at 33/44/55/66px, baked into
src/Platform/FontAtlasData.h, and they are both taller and vertically offset
from that cell - so the old model passed screens whose rows and "MORE ..."
hints actually overlap. The ink box below is derived from the baked metrics,
following font_atlas_verify.py's precedent of parsing the generated header
rather than retyping its numbers.

The model must not be weakened to make a screen pass. If a screen fails, the
screen's constants are wrong, not this file.
"""

import os
import re

SCREEN_H = 1080
HINT_SCALE = 3

# Where a screen draws two footer lines, they sit 50px apart. The
# categorised screens draw one - see foot_lines below.
FOOTER_LINE_GAP = 50

HEADER = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                      "..", "src", "Platform", "FontAtlasData.h")


# --- the real text metrics, parsed from the generated atlas -----------------

# The baked range: printable ASCII, then the Hunter's Mark one past its end
# (app/tools/mark_glyph.py).
FIRST_CODE = 32
MARK_CODE = 127


def load_ink_box(path):
    """Per-scale (inkTop, inkBottom, lineHeight), in pixels below the y passed
    to DrawText.

    FontAtlasDrawText places the TOP-LEFT of the line at y and rides the
    baseline at y + ascent, so a glyph's ink spans
        y + ascent - bearingY  ..  y + ascent - bearingY + height
    and no glyph in the face can draw above y + (ascent - max bearingY) or
    below y + (ascent + max(height - bearingY)). Those two bounds are the ink
    box: tight at the bottom, and 9-20px looser at the top than the line box,
    which is exactly where the near-collisions on these screens sit.

    Taken over all 95 glyphs with a non-zero box, not over the uppercase
    subset every shipped string happens to use: the help text this geometry is
    being corrected for is mixed case, and descenders are what bite.

    The Hunter's Mark at codepoint 127 is EXCLUDED. This box answers "how far
    can a line of TEXT reach", and every screen measured here draws text and
    nothing else; the mark is an emblem on one rail row of one screen, where
    settings_ui_verify.py checks it against that row's focus bar directly.
    Counting it here would let a change to the rune's height fail a scroll-hint
    clearance on the progress log, which never draws it - a verifier failure
    that tells you nothing true.
    """
    text = open(path, encoding="utf-8").read()

    sizes = re.search(r"const AtlasSize kSizes\[\d+\] = \{(.*?)\n\};",
                      text, re.S)
    if not sizes:
        raise SystemExit("could not find kSizes in %s" % path)

    meta = {}
    for row in re.findall(r"\{([^}]*)\}", sizes.group(1)):
        f = [x.strip() for x in row.split(",")]
        # uiScale, pixelSize, ascent, descent, atlasW, atlasH, glyphs, ...
        meta[int(f[0])] = dict(ascent=int(f[2]), descent=int(f[3]), table=f[6])

    box = {}
    for scale, m in meta.items():
        body = re.search(r"const AtlasGlyph %s\[\d+\] = \{(.*?)\n\};" % m["table"],
                         text, re.S)
        if not body:
            raise SystemExit("could not find %s in %s" % (m["table"], path))
        max_bearing_y = 0
        max_below = 0
        code = FIRST_CODE - 1
        for line in body.group(1).split("\n"):
            g = re.match(r"\s*\{([-0-9,]+)\},", line)
            if not g:
                continue
            code += 1
            if code == MARK_CODE:
                continue          # the emblem, not a letter - see the docstring
            # dataOffset, atlasX, atlasY, width, height, bearingX, bearingY, advance
            v = [int(x) for x in g.group(1).split(",")]
            w, h, bearing_y = v[3], v[4], v[6]
            if w == 0 or h == 0:
                continue          # space and friends have no ink at all
            max_bearing_y = max(max_bearing_y, bearing_y)
            max_below = max(max_below, h - bearing_y)
        box[scale] = (m["ascent"] - max_bearing_y,
                      m["ascent"] + max_below,
                      m["ascent"] + m["descent"])
    return box


INK = load_ink_box(HEADER)


def ink_top(y, scale):
    """Topmost pixel a line of text drawn at `y` can occupy."""
    return y + INK[scale][0]


def ink_bottom(y, scale):
    """Bottommost pixel, exclusive - one past the last row of ink."""
    return y + INK[scale][1]


# --- mirror of Controls.cpp -------------------------------------------------

def visible_row_count(first_y, spacing, bottom_limit):
    if spacing <= 0:
        return 1
    rows = (bottom_limit - first_y) // spacing + 1
    return max(rows, 1)


def clamp_scroll(offset, count, visible):
    return max(0, min(offset, max(0, count - visible)))


def scroll_to_show(offset, selected, count, visible):
    if selected < offset:
        offset = selected
    if selected >= offset + visible:
        offset = selected - visible + 1
    return clamp_scroll(offset, count, visible)


def navigate(selected, count, direction):
    return (selected + count - 1) % count if direction < 0 else (selected + 1) % count


# --- screens, as actually coded --------------------------------------------

# name, first_y, spacing, bottom_limit, item_scale, hint_gap, count,
#   head_y, head_scale   (the lowest thing drawn ABOVE the list)
#   footer_y             (the FIRST footer line below it)
#   foot_lines           (how many footer lines that screen draws)
#
# foot_lines exists because the categorised screens draw ONE footer line, at
# y=1000, where every flat list draws two 50px apart. Checking a second line
# that is not there would fail G5 on a screen whose footer is fine.
#
# hint_gap mirrors ListLayout::hintGap, which replaced the single file-scope
# kHintGap in Controls.cpp. It had to become per-layout: sweeping one global
# value over 46-64 satisfies no screen set, because the feasible band depends
# on the item scale (4 on the settings screens, 3 on the pickers and the log)
# and on what sits above and below.
#
# counts track kSaveDataRowCount, which grows by one per settings row - and,
# until the randomizer-settings-ui feature, SetupDefaultsScreen::kItemCount
# with it. BOTH settings screens are now the categorised screen: each pane is
# Controls.h's kPaneLayout and shows ONE CATEGORY at a time, so its count is
# the largest category (4 today; 7 once the whole backlog lands, per plan M5),
# not the 18-setting total. Both are under the 8 rows the pane fits, so it
# never scrolls yet - the window is checked anyway, because the pane that
# silently truncates is the failure mode being removed.
#
# The flat-list history below therefore applies to Wizard Confirm, which is
# still a review list of every setting at once, and to the two settings screens
# up to the point they stopped being flat lists. The counts
# stayed at 15 through feature 032: D1
# removed UNCHANGED BELL MAIDENS and ENEMIES SKIPPED took its place, so
# that feature left these three entries alone. Feature 033 appends DO NOT
# RANDOMIZE CAGED DOGS last on all three lists, 15 -> 16. START WITH
# HUNTER TOOLS appends to the same three, 16 -> 17. Feature 018 appends
# four more - EASY SHADOWS, EASY ROM, EASY FAILURES, EASY EMISSARY -
# 17 -> 21. The randomizer-settings-ui spec §4.8 then removes save-data
# handling in full, taking BACKUP EXISTING SAVE and REPLACE SAVE off all
# three lists, 21 -> 19.
#
# The progress log is NOT one of these: its count is a line budget, not a
# settings-row count. Feature 018 added up to four lines to it, one per
# enabled setting, 16 -> 20; §4.8 removes the two simulated save-data
# lines StartCommit always emitted, 20 -> 18.
SCREENS = [
    # The settings pane of the categorised Setup Defaults screen: Controls.h's
    # kPaneLayout {330, 76, 880, 52} at item scale 3, under the category
    # heading at y=220 (scale 4) and above the single footer line at y=1000.
    ("Setup Defaults",  330, 76, 880, 3, 52, 4, 220, 4, 1000, 1),
    # The world editor's Settings step - the Enable wizard's, renamed with the
    # screen in worlds milestone 5. It is the same categorised screen as Setup
    # Defaults, drawing the same kPaneLayout with the same single footer line -
    # identical numbers on purpose, which settings_ui_verify.py asserts by
    # parsing both screens' constants and comparing them. Only the RAIL beside
    # it differs, and a rail is not a scrolling list.
    ("Editor Settings", 330, 76, 880, 3, 52, 4, 220, 4, 1000, 1),
    # Confirm is still a flat review list, and in worlds milestone 6 it became
    # the ACTIVATION confirmation (B10): EIGHT head rows now, not three - NAME,
    # SEED, TARGET, then what is deactivated, where its save goes, what is
    # activated, what happens to the incoming save and roughly how long it
    # takes - followed by all 19 settings. Its two footers sit 30px lower than
    # every other screen's. Under the ink box, kSettingsLayout's last row at
    # y=870 and a footer at SCREEN_H - 130 leave an EMPTY feasible band for the
    # hint between them: it needs >= 57 to clear the row and <= 45 to clear the
    # footer. Moving the footer is what keeps six visible rows instead of five.
    #
    # What sits above the list is no longer the state line at y=260: it is the
    # second line of the wrapped sentence under it, at kConfirmSentenceY +
    # kConfirmSentencePitch = 366, at the row scale.
    ("Editor Confirm",  470, 80, 870, 4, 60, 27, 352, 3, SCREEN_H - 100, 2),
    # The editor's revision list (kHistoryLayout). It HAS a cursor, unlike
    # Confirm, because selecting a revision is what makes it current again -
    # so it runs at the row scale and the scroll properties below are the ones
    # that matter. What sits above it is the HISTORY sub-heading at y=240.
    #
    # THE COUNT IS A STAND-IN, not an inventory: a world's history is
    # append-only and unbounded (spec worlds D2). 24 is comfortably past the
    # rows the band fits, so the window is exercised rather than trivially
    # satisfied.
    ("Editor history",  340, 60, 820, 3, 50, 24, 240, 4, SCREEN_H - 130, 2),
    # Progress: worst case is the finished state (no live line, footer present).
    ("Progress log",    300, 70, 920, 3, 50, 18, 200, 4, SCREEN_H - 130, 2),
    # The enemy picker (UI/ModelPicker.cpp): 82 rows is far too many at the
    # settings screens' scale 4 / 90px pitch, so it runs denser - scale 3 at
    # 52px, which fits 12 and turns 14 pages into 7. Heading at y=120 (scale
    # 5), count/page line at y=185 (scale 3).
    ("Enemy picker",    280, 52, 900, 3, 52, 82, 185, 3, SCREEN_H - 130, 2),
    # Same component, same band, shorter list - 17 bosses over two pages.
    ("Boss picker",     280, 52, 900, 3, 52, 17, 185, 3, SCREEN_H - 130, 2),
    # The WORLDS tab's rail (UI/WorldsScreen.cpp kRailLayout). Same band and
    # same pitch as the settings pane opposite it, so switching tabs moves the
    # content and not the furniture. What sits above it in that column is the
    # active tab's name at y=118 (scale 5, Controls.h kTabHeadingY) and the
    # header rule below that.
    #
    # THE COUNT IS A STAND-IN, not an inventory: the rail is "+ NEW WORLD",
    # "VANILLA" and however many worlds the player made, which is unbounded.
    # 24 is picked to be comfortably past the 8 rows the band fits, so the
    # scroll properties below are exercised rather than trivially satisfied.
    ("Worlds rail",     330, 76, 880, 3, 52, 24, 118, 5, 1000, 1),
    # The WORLDS tab's details pane (kDetailLayout). Denser than a settings
    # pane - these are readouts and wrapped sentences, not editable rows - so
    # it fits eleven lines where the settings pane fits eight. It has NO
    # CURSOR, so the count here is the worst case the screen can build (seven
    # label/value rows for a world plus a three-line note) rather than a list
    # the player can scroll: settings_ui_verify.py is what pins that worst
    # case to the strings, and this is what pins the band it has to fit.
    ("Worlds details",  330, 52, 880, 3, 44, 11, 220, 4, 1000, 1),
    # The WORLDS tab's startup log - reconciliation and first-run capture -
    # drawn with the wizard's progress-log geometry, listed separately because
    # a shared set of numbers that nothing checks twice is a shared set of
    # numbers one of the two screens can walk away from.
    ("Worlds startup",  300, 70, 920, 3, 50, 18, 200, 4, SCREEN_H - 130, 2),
    # ENEMIES SKIPPED, same component again but with an instruction line, so
    # the list band starts at 332 instead of 280 and fits 11 rows instead of
    # 12 (spec 032 F13 option B). The thing above the list is no longer the
    # count line at 185 but the instruction at y=235. 85 rows is 8 pages at
    # 11 and at 12 alike.
    ("Skipped picker",  332, 52, 900, 3, 52, 85, 235, 3, SCREEN_H - 130, 2),
]

failures = []

for (name, first_y, spacing, bottom, scale, hint_gap, count,
     head_y, head_scale, foot_y, foot_lines) in SCREENS:
    vis = visible_row_count(first_y, spacing, bottom)
    last_row_y = first_y + (vis - 1) * spacing
    hint_above_y = first_y - hint_gap
    hint_below_y = last_row_y + hint_gap

    head_bot  = ink_bottom(head_y, head_scale)
    foot_top  = ink_top(foot_y, HINT_SCALE)
    foot_bot  = ink_bottom(foot_y + (foot_lines - 1) * FOOTER_LINE_GAP, HINT_SCALE)

    # G1: every visible row is fully on screen and clear of the footer.
    if ink_bottom(last_row_y, scale) > SCREEN_H:
        failures.append("%s: last row ink bottom %d exceeds screen %d"
                        % (name, ink_bottom(last_row_y, scale), SCREEN_H))
    if ink_bottom(last_row_y, scale) > foot_top:
        failures.append("%s: last row ink bottom %d collides with footer ink at %d"
                        % (name, ink_bottom(last_row_y, scale), foot_top))

    # G2: the first row clears whatever heading sits above it.
    if ink_top(first_y, scale) < head_bot:
        failures.append("%s: first row ink top %d overlaps heading ending at %d"
                        % (name, ink_top(first_y, scale), head_bot))

    # G3: MORE BELOW clears the last ROW above it and the footer below it.
    #     The row half is the check the old 8 * scale model never made - it
    #     compared the hints only against the furniture - and it is why a
    #     single global gap looked like a fix and was not.
    if ink_top(hint_below_y, HINT_SCALE) < ink_bottom(last_row_y, scale):
        failures.append("%s: MORE BELOW ink top %d overlaps last row ending at %d"
                        % (name, ink_top(hint_below_y, HINT_SCALE),
                           ink_bottom(last_row_y, scale)))
    if ink_bottom(hint_below_y, HINT_SCALE) > foot_top:
        failures.append("%s: MORE BELOW ink bottom %d collides with footer ink at %d"
                        % (name, ink_bottom(hint_below_y, HINT_SCALE), foot_top))

    # G4: MORE ABOVE clears the heading above it and the first ROW below it.
    if ink_top(hint_above_y, HINT_SCALE) < head_bot:
        failures.append("%s: MORE ABOVE ink top %d overlaps heading ending at %d"
                        % (name, ink_top(hint_above_y, HINT_SCALE), head_bot))
    if ink_bottom(hint_above_y, HINT_SCALE) > ink_top(first_y, scale):
        failures.append("%s: MORE ABOVE ink bottom %d overlaps first row at %d"
                        % (name, ink_bottom(hint_above_y, HINT_SCALE),
                           ink_top(first_y, scale)))

    # G5: the LAST footer line is itself on screen. This is what the wizard's
    #     30px move spends, so it is checked rather than assumed.
    if foot_bot > SCREEN_H:
        failures.append("%s: last footer line ink bottom %d exceeds screen %d"
                        % (name, foot_bot, SCREEN_H))

    # S1: walking the cursor all the way round (twice) always keeps it visible,
    #     and the offset never goes out of range.
    off, sel = 0, 0
    for direction in (1, -1):
        for _ in range(count * 2):
            sel = navigate(sel, count, direction)
            off = scroll_to_show(off, sel, count, vis)
            if not (off <= sel < off + vis):
                failures.append("%s: selection %d not visible in window [%d,%d)"
                                % (name, sel, off, off + vis))
            if not (0 <= off <= max(0, count - vis)):
                failures.append("%s: offset %d out of range" % (name, off))

    # S2: free scrolling (Confirm / finished progress log) can't leave the list.
    off = 0
    for step in [1] * (count + 5) + [-1] * (count + 10):
        off = clamp_scroll(off + step, count, vis)
        if not (0 <= off <= max(0, count - vis)):
            failures.append("%s: free-scroll offset %d out of range" % (name, off))

    # S3: the last item must be reachable.
    if clamp_scroll(count, count, vis) + vis < count:
        failures.append("%s: last item unreachable" % name)

    print("%-16s %2d rows visible of %2d   rows y=%d..%d   hints %d / %d   "
          "gap %d   clearances above %d / below %d"
          % (name, vis, count, first_y, last_row_y, hint_above_y, hint_below_y,
             hint_gap,
             ink_top(first_y, scale) - ink_bottom(hint_above_y, HINT_SCALE),
             ink_top(hint_below_y, HINT_SCALE) - ink_bottom(last_row_y, scale)))

# Progress screen while RUNNING gives one slot to the live status line.
first_y, spacing, bottom, scale, foot_y = 300, 70, 920, 3, SCREEN_H - 130
slots = visible_row_count(first_y, spacing, bottom)
log_rows = max(1, slots - 1)
for line_count in range(0, 18):
    off = clamp_scroll(line_count - log_rows, line_count, log_rows)
    drawn = min(log_rows, max(0, line_count - off))
    live_y = first_y + drawn * spacing
    if ink_bottom(live_y, scale) > ink_top(foot_y, HINT_SCALE) and line_count > 0:
        failures.append("running: live line ink bottom %d collides with footer"
                        % ink_bottom(live_y, scale))
    # tail-follow must always show the newest line
    if line_count and off + drawn != line_count:
        failures.append("running: tail not followed at %d lines" % line_count)
print("%-16s %2d log rows + 1 live slot while running" % ("Progress (run)", log_rows))

print()
print("ink box, parsed from FontAtlasData.h:")
for scale in sorted(INK):
    top, bot, line = INK[scale]
    print("  scale %d: ink %d..%d below the draw y, line box %d"
          % (scale, top, bot, line))

print()
if failures:
    print("%d FAILURES:" % len(failures))
    for f in failures:
        print("  " + f)
    raise SystemExit(1)
print("all geometry and scroll properties PASSED")
