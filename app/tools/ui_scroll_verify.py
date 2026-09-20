"""Mirror of Controls.cpp's scroll math + a geometry check per screen.

No host C++ compiler exists in this project, so this is the standing substitute:
re-implement the arithmetic exactly as written and assert the properties that
matter. Pixel constants are read off the real draw calls.
"""

SCREEN_H = 1080
HINT_GAP = 46
HINT_SCALE = 3


def glyph_h(scale):
    return 8 * scale


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

SCREENS = [
    # name, first_y, spacing, bottom_limit, item_scale, count,
    #   heading_bottom (lowest y+h of anything above the list),
    #   footer_top (highest y of anything below)
    # counts track SetupDefaultsScreen::kItemCount and kSaveDataRowCount, which
    # grow by one per settings row. They stayed at 15 through feature 032: D1
    # removed UNCHANGED BELL MAIDENS and ENEMIES SKIPPED took its place, so
    # that feature left these three entries alone. Feature 033 appends DO NOT
    # RANDOMIZE CAGED DOGS last on all three lists, 15 -> 16. START WITH
    # HUNTER TOOLS appends to the same three, 16 -> 17. Feature 018 appends
    # four more - EASY SHADOWS, EASY ROM, EASY FAILURES, EASY EMISSARY -
    # 17 -> 21. The progress log is NOT one of these: its count is a line
    # budget, not a settings-row count, and feature 018 adds up to four lines
    # to it, one per enabled setting, 16 -> 20.
    ("Setup Defaults",  300, 90, 870, 4, 21, 180 + glyph_h(5), SCREEN_H - 130),
    ("Wizard SaveData", 420, 90, 870, 4, 21, 260 + glyph_h(4), SCREEN_H - 130),
    ("Wizard Confirm",  420, 90, 870, 4, 21, 260 + glyph_h(4), SCREEN_H - 130),
    # Progress: worst case is the finished state (no live line, footer present).
    ("Progress log",    300, 70, 920, 3, 20, 200 + glyph_h(4), SCREEN_H - 130),
    # The enemy picker (UI/EnemyPicker.cpp): 82 rows is far too many at the
    # settings screens' scale 4 / 90px pitch, so it runs denser - scale 3 at
    # 52px, which fits 12 and turns 14 pages into 7. Heading at y=120 (scale
    # 5), count/page line at y=185 (scale 3).
    ("Enemy picker",    280, 52, 900, 3, 82, 185 + glyph_h(3), SCREEN_H - 130),
    # Same component, same band, shorter list - 17 bosses over two pages.
    ("Boss picker",     280, 52, 900, 3, 17, 185 + glyph_h(3), SCREEN_H - 130),
    # ENEMIES SKIPPED, same component again but with an instruction line, so
    # the list band starts at 332 instead of 280 and fits 11 rows instead of
    # 12 (spec 032 F13 option B). The thing above the list is no longer the
    # count line at 185 but the instruction at y=235, and the clearance this
    # checks - 332 - 46 = 286 against 259 - is the reason option B was chosen
    # over moving the heading. 85 rows is 8 pages at 11 and at 12 alike.
    ("Skipped picker",  332, 52, 900, 3, 85, 235 + glyph_h(3), SCREEN_H - 130),
]

failures = []

for name, first_y, spacing, bottom, scale, count, head_bot, foot_top in SCREENS:
    vis = visible_row_count(first_y, spacing, bottom)

    # G1: every visible row is fully on screen and clear of the footer.
    last_row_y = first_y + (vis - 1) * spacing
    if last_row_y + glyph_h(scale) > SCREEN_H:
        failures.append("%s: last row bottom %d exceeds screen %d"
                        % (name, last_row_y + glyph_h(scale), SCREEN_H))
    if last_row_y + glyph_h(scale) > foot_top:
        failures.append("%s: last row bottom %d collides with footer at %d"
                        % (name, last_row_y + glyph_h(scale), foot_top))

    # G2: the MORE BELOW hint clears the footer.
    hint_below = last_row_y + HINT_GAP
    if hint_below + glyph_h(HINT_SCALE) > foot_top:
        failures.append("%s: MORE BELOW bottom %d collides with footer at %d"
                        % (name, hint_below + glyph_h(HINT_SCALE), foot_top))

    # G3: the MORE ABOVE hint clears the heading above the list.
    hint_above = first_y - HINT_GAP
    if hint_above < head_bot:
        failures.append("%s: MORE ABOVE top %d overlaps heading ending at %d"
                        % (name, hint_above, head_bot))

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

    print("%-16s %2d rows visible of %2d   rows y=%d..%d   hints %d / %d"
          % (name, vis, count, first_y, last_row_y, hint_above, hint_below))

# Progress screen while RUNNING gives one slot to the live status line.
first_y, spacing, bottom, scale = 300, 70, 920, 3
slots = visible_row_count(first_y, spacing, bottom)
log_rows = max(1, slots - 1)
for line_count in range(0, 20):
    off = clamp_scroll(line_count - log_rows, line_count, log_rows)
    drawn = min(log_rows, max(0, line_count - off))
    live_y = first_y + drawn * spacing
    if live_y + glyph_h(scale) > SCREEN_H - 130 and line_count > 0:
        failures.append("running: live line at %d collides with footer" % live_y)
    # tail-follow must always show the newest line
    if line_count and off + drawn != line_count:
        failures.append("running: tail not followed at %d lines" % line_count)
print("%-16s %2d log rows + 1 live slot while running" % ("Progress (run)", log_rows))

print()
if failures:
    print("%d FAILURES:" % len(failures))
    for f in failures:
        print("  " + f)
    raise SystemExit(1)
print("all geometry and scroll properties PASSED")
