#!/usr/bin/env python3
"""Mirror of UI/SettingsModel.cpp's table and of the categorised screen's geometry.

Layer 2 of CLAUDE.md section 3. A clean cross-compile proves SettingsModel.cpp
links; it proves nothing about whether every setting has a category, whether the
categories are the ones the spec decided on, or whether a label plus its value
actually fits the pane it is drawn into. Those are the failures that compile
fine and only show up as a missing setting or a clipped word on a TV.

So this re-derives all of it from the sources: the table out of
SettingsModel.cpp, the fields out of RandomizerDefaults.h, the category mapping
out of the SPEC, and every text width out of the baked advances in
FontAtlasData.h - never from character counts, which stopped predicting width
when the proportional atlas replaced the fixed 8x8 cell.

The eight cases are the ones plan section 6 names:

  1  every bool settings field appears in exactly one entry, and every entry's
     flag names a field that exists
  2  every SettingId appears exactly once
  3  the six SETTINGS categories, their order, their membership and the
     within-category order match spec 7.1 - including the two placements it
     says must not be "corrected" - and SAVE, the worlds feature's seventh, is
     in the world editor's category list and not in the DEFAULTS tab's
  4  every entry, plus SEED, BLOODBORNE TITLE ID, NAME and HISTORY, has help
     text
  5  every rail row, pane row, heading and help string fits its column, measured
     from the atlas
  6  the vertical constants of plan 4.4 and 4.5 do not overlap, under the same
     ink box ui_scroll_verify.py uses
  7  no identifier from the save-data inventory (spec 4.8) survives in app/src
  8  a defaults.cfg carrying both retired keys loads with them ignored and
     everything else honoured

The worlds feature's milestone 4 adds a third screen to 5 and 6 - the WORLDS
tab - and a tab strip shared by it and the DEFAULTS tab. The worlds screen is
the same three-column split with a scrolling rail of worlds, a details pane of
readouts and wrapped notes, and the same help column, so its strings and its
vertical stack are measured here the same way. Its scroll bands are
ui_scroll_verify.py's; what is here is what fits, what overlaps, and that the
three screens have not drifted apart.

Milestone 4 extends 5 and 6 to the Enable wizard, which milestone 5 renames and
extends into the WORLD EDITOR: the same screen with three more rail rows (NAME
and SEED above the categories, HISTORY below them), a seventh category, a
header band and a Confirm list generated from the same table. The editor keeps
its OWN copy of the geometry constants - plan 5 gives the screens no shared
layout file - so this also parses both screens and fails if any SHARED constant
disagrees with its twin. That comparison is the thing standing between "one
geometry" and two that drift.

The startup-screen feature adds a tenth case to the list, and more work to 5.
The WORLDS tab's startup sequence now ends in an explicit outcome rather than
two booleans, and each non-None outcome owns a sentence and a prompt. That
mapping lives in WorldsScreen.cpp as a TABLE rather than a switch precisely so
it can be read out here and asserted TOTAL against the enum in WorldsScreen.h
(startup-screen plan P6): an outcome with no row would be an error screen with
a blank sentence and no way off it, which compiles perfectly. 10 is that
totality check; 5 grows the four sentences, the title, the scroll hint and the
two prompts, all measured against the full 1920 because the error state is
centred on the screen rather than drawn into a column.

The feature's other state, the loading one, adds to 5 and 6 instead: its two
words are measured against the 1000px block they are centred in, its four
elements are checked for order and clearance like any other stack, and the
bar's arithmetic is checked to divide into five EQUAL cells with no rounding
drift. What no tool here can see is the bar stepping or the frame it steps on;
that is the hardware test's, and the plan says so.

The editor's RAIL is deliberately no longer part of that comparison. Ten rows
do not fit Setup Defaults' 330/76 category band, so the editor runs its own
uniform 70px grid; the four constants that describe it are pinned against this
file's own copy instead, exactly as kHeaderY already is. Everything that
decides where the three COLUMNS are is still shared and still compared.

Usage:
    python settings_ui_verify.py
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "..", "src")
UI = os.path.join(SRC, "UI")
GAME = os.path.join(SRC, "Game")
RND = os.path.join(SRC, "Randomizer")
SPEC = os.path.join(HERE, "..", "..", "docs", "features", "randomizer-settings-ui",
                    "spec.md")
ATLAS = os.path.join(SRC, "Platform", "FontAtlasData.h")


def read(path):
    return open(path, encoding="utf-8").read()


def strip_comments(text):
    """Source with // and /* */ removed, so a case about IDENTIFIERS is not
    answered by prose. The store's tolerance comment names both retired keys on
    purpose - that is documentation, not a surviving key."""
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
    return re.sub(r"//[^\n]*", " ", text)


# ---------------------------------------------------------------------------
# the model, parsed out of SettingsModel.{h,cpp}
# ---------------------------------------------------------------------------

STRING_LITERAL = re.compile(r'"((?:[^"\\]|\\.)*)"')


def joined_literals(text):
    """Adjacent C string literals are one string to the compiler, so they are
    one string here too - the help text is written as a paragraph across
    several lines."""
    return "".join(STRING_LITERAL.findall(text))


def parse_model():
    src = read(os.path.join(UI, "SettingsModel.cpp"))
    hdr = read(os.path.join(UI, "SettingsModel.h"))

    block = re.search(r"const SettingDef kSettings\[\] = \{(.*?)\n\};", src, re.S)
    if not block:
        sys.exit("could not find kSettings in SettingsModel.cpp")

    entries = []
    for raw in re.split(r"\{\s*SettingId::", block.group(1))[1:]:
        ident = re.match(r"(\w+)", raw)
        cat = re.search(r"SettingCategory::(\w+)", raw)
        kind = re.search(r"SettingKind::(\w+)", raw)
        flag = re.search(r"&RandomizerDefaults::(\w+)", raw)
        literals = STRING_LITERAL.findall(raw)
        if not (ident and cat and kind and literals):
            sys.exit("unparsable kSettings entry: %s" % raw[:60])
        entries.append(dict(id=ident.group(1), category=cat.group(1),
                            kind=kind.group(1), label=literals[0],
                            help="".join(literals[1:]),
                            flag=flag.group(1) if flag else None))

    labels = re.search(r"const char\* const kCategoryLabels\[\] = \{(.*?)\n\};", src, re.S)
    category_labels = STRING_LITERAL.findall(labels.group(1)) if labels else []

    rail_help = {}
    for name in ("kSeedHelp", "kTitleIdHelp", "kNameHelp", "kHistoryHelp"):
        m = re.search(r"const char\* const %s =(.*?);" % name, src, re.S)
        rail_help[name] = joined_literals(m.group(1)) if m else ""

    order = re.search(r"enum class SettingCategory \{(.*?)\};", hdr, re.S)
    categories = [c.strip() for c in order.group(1).replace("\n", " ").split(",")
                  if c.strip() and not c.strip().startswith("//")]
    categories = [c for c in categories if c != "Count"]

    ids = re.search(r"enum class SettingId \{(.*?)\};", hdr, re.S)
    declared_ids = [c.strip() for c in ids.group(1).replace("\n", " ").split(",")
                    if c.strip()]

    return entries, category_labels, rail_help, categories, declared_ids


# ---------------------------------------------------------------------------
# the struct
# ---------------------------------------------------------------------------

def parse_defaults():
    text = read(os.path.join(RND, "RandomizerDefaults.h"))
    body = strip_comments(text)
    bools = re.findall(r"\bbool (\w+) = ", body)
    selections = dict(re.findall(r"\b(EnemyPoolSelection|BossPoolSelection|"
                                 r"EnemySkipSelection|TrickWeaponSelection|"
                                 r"LeftHandWeaponSelection) (\w+);",
                                 body))
    return bools, selections


# ---------------------------------------------------------------------------
# the spec's own category table
# ---------------------------------------------------------------------------

# Spec 7.1 names settings in prose; the rows keep the strings they shipped with
# (plan 9 D2), so the two vocabularies are mapped here rather than either being
# bent to the other. A setting missing from this map fails case 3 loudly, which
# is the point: a new setting must be placed in the spec table too.
PROSE_TO_LABEL = {
    "Randomize Enemies": "RANDOMIZE ENEMIES",
    "Enemies Included": "ENEMIES INCLUDED",
    "Enemies Skipped": "ENEMIES SKIPPED",
    "Protect Caged Dogs": "DO NOT RANDOMIZE CAGED DOGS",
    "Randomize Bosses": "RANDOMIZE BOSSES",
    "Bosses Included": "BOSSES INCLUDED",
    "Randomize Treasure": "RANDOMIZE TREASURE",
    "Randomize Workshop Tools": "RANDOMIZE WORKSHOP TOOLS",
    "Randomize Enemy Drops": "RANDOMIZE ENEMY DROPS",
    "Randomize Starting Weapons": "RANDOMIZE STARTING WEAPONS",
    "Randomize Starting Guns": "RANDOMIZE STARTING GUNS",
    "Randomize Shop Weapons": "RANDOMIZE SHOP WEAPONS",
    "Start With Hunter Tools": "START WITH HUNTER TOOLS",
    "Start With A Trick Weapon": "START WITH A TRICK WEAPON",
    "Start With A Left Weapon": "START WITH A LEFT WEAPON",
    "Easy Shadows": "EASY SHADOWS",
    "Easy Rom": "EASY ROM",
    "Easy Failures": "EASY FAILURES",
    "Easy Emissary": "EASY EMISSARY",
    "Enable Mergo Darkness": "ENABLE MERGO DARKNESS",
    "No Team Type": "ENEMIES ON SAME TEAM",
}

SPEC_CATEGORY_TO_ENUM = {
    "Enemies": "Enemies",
    "Bosses": "Bosses",
    "Items & Treasure": "ItemsTreasure",
    "Weapons & Starting Gear": "WeaponsGear",
    "Difficulty": "Difficulty",
    "World": "World",
}


def parse_spec_categories():
    """The six categories and their CURRENT settings, in the spec's own order."""
    text = read(SPEC)
    table = text.split("### 7.1 Category mapping", 1)[1].split("\n\n", 3)[1]
    rows = []
    for line in table.splitlines():
        m = re.match(r"\|\s*\*\*(.+?)\*\*\s*\|(.*?)\|", line)
        if not m:
            continue
        current = [s.strip() for s in m.group(2).split("·") if s.strip()]
        rows.append((m.group(1).strip(), current))
    return rows


# ---------------------------------------------------------------------------
# text metrics, from the baked atlas
# ---------------------------------------------------------------------------

# The Hunter's Mark's codepoint, one past printable ASCII - the glyph
# app/tools/mark_glyph.py bakes. Defined here rather than with the geometry
# requirements below because load_atlas, which runs on import, needs it.
MARK_CODE = 127

# U4 pass 4 baked the five button glyphs at 128..132. They are kept OUT of the
# TEXT ink box for exactly the reason the mark is: they are drawn in footers at
# a size chosen for them, not in running text, so letting them widen the box
# would move every row clearance in the app the next time one is resized.
#
# As it happens they currently sit inside it and change nothing - which is
# precisely why this is written down rather than left to luck.
BUTTON_CODES = range(128, 133)


def load_atlas():
    """(advances, text ink box, mark ink box) per scale, from the generated H.

    Same source and same arithmetic as ui_scroll_verify.py's load_ink_box - the
    parse is repeated rather than imported because that file is a script that
    runs its whole check suite on import.
    """
    text = read(ATLAS)
    sizes = re.search(r"const AtlasSize kSizes\[\d+\] = \{(.*?)\n\};", text, re.S)
    if not sizes:
        sys.exit("could not find kSizes in %s" % ATLAS)

    meta = {}
    for row in re.findall(r"\{([^}]*)\}", sizes.group(1)):
        f = [x.strip() for x in row.split(",")]
        meta[int(f[0])] = dict(ascent=int(f[2]), descent=int(f[3]), table=f[6])

    adv, ink, mark_ink = {}, {}, {}
    for scale, m in meta.items():
        body = re.search(r"const AtlasGlyph %s\[\d+\] = \{(.*?)\n\};" % m["table"],
                         text, re.S)
        glyphs, code = {}, 32
        max_bearing_y, max_below = 0, 0
        for line in body.group(1).split("\n"):
            g = re.match(r"\s*\{([-0-9,]+)\},", line)
            if not g:
                continue
            v = [int(x) for x in g.group(1).split(",")]
            glyphs[chr(code)] = v[7]
            if code == MARK_CODE:
                # The emblem, measured on its own. It is kept OUT of the text
                # ink box for the reason ui_scroll_verify.py's load_ink_box
                # spells out - it is drawn on one rail row, not in running text
                # - and the worlds rail checks it against that row's focus bar
                # explicitly instead.
                mark_ink[scale] = (m["ascent"] - v[6],
                                   m["ascent"] - v[6] + v[4])
            elif code in BUTTON_CODES:
                pass                      # see BUTTON_CODES
            elif v[3] and v[4]:
                max_bearing_y = max(max_bearing_y, v[6])
                max_below = max(max_below, v[4] - v[6])
            code += 1
        adv[scale] = glyphs
        ink[scale] = (m["ascent"] - max_bearing_y, m["ascent"] + max_below)
    return adv, ink, mark_ink


ADV, INK, MARK_INK = load_atlas()


def width(text, scale):
    return sum(ADV[scale][c] for c in text if c in ADV[scale])


def ink_top(y, scale):
    return y + INK[scale][0]


def ink_bottom(y, scale):
    return y + INK[scale][1]


def wrap(text, scale, max_width):
    """Mirror of Controls.cpp's WrapText: greedy on spaces, measured."""
    lines, line = [], ""
    for word in text.split():
        if not line:
            line = word
            continue
        if width(line + " " + word, scale) <= max_width:
            line += " " + word
        else:
            lines.append(line)
            line = word
    if line:
        lines.append(line)
    return lines


# ---------------------------------------------------------------------------
# geometry, as coded in SetupDefaultsScreen.cpp and Controls.h
# ---------------------------------------------------------------------------

SCREEN_H = 1080

RAIL_X, RAIL_W = 60, 580
PANE_X, PANE_W = 680, 700
HELP_X, HELP_W = 1420, 440
PANE_VALUE_RIGHT = 1380
VALUE_GAP = 40                 # the gap a label must leave before its value

TITLE_Y, TITLE_SCALE = 36, 5
HEADER_Y, HEADER_SCALE = 118, 4
HEADER_RULE_Y = 196
COLUMN_RULE_Y, COLUMN_RULE_H = 210, 750
RAIL_ROW0_Y = 230
# Setup Defaults' rail: row 0, a rule, then six categories at the pane's pitch.
DEFAULTS_RULE1_Y = 296
DEFAULTS_FIRST_Y, DEFAULTS_PITCH = 330, 76
# The world editor's rail: NAME, SEED, a rule, SEVEN categories, a rule,
# HISTORY - ten rows on one uniform 70px grid from RAIL_ROW0_Y.
EDITOR_PITCH = 70
EDITOR_ROW1_Y = 300
EDITOR_RULE1_Y = 358
EDITOR_FIRST_Y = 370
EDITOR_RULE2_Y = 848
EDITOR_HISTORY_Y = 860
PANE_HEADING_Y, PANE_HEADING_SCALE = 220, 4
PANE_FIRST_Y, PANE_PITCH, PANE_BOTTOM, PANE_HINT_GAP = 330, 76, 880, 52
HELP_TITLE_Y, HELP_PITCH = 220, 52
HELP_RULE_Y = 330
HELP_BODY_Y = 356
HELP_TITLE_MAX_LINES = 2
HELP_BODY_MAX_LINES = 11
FOOTER_Y = 1000
ROW_SCALE = 3
BAR_OFFSET_Y, BAR_HEIGHT = -10, 64

RAIL_INK_END = 910            # the lowest ink either rail column reaches
                              # (the editor's HISTORY row)

FOOTER_LINE = "UP DOWN MOVE   LEFT RIGHT CHANGE   X SELECT   O BACK   OPTIONS SAVE"
# The editor's footer is NOT the same string and must be read from the editor
# itself. It was aliased to FOOTER_LINE until 2026-09-25, which meant the
# "editor footer fits" case below measured the DEFAULTS footer and would have
# passed however wide the editor's grew. Assigned lazily in the editor case,
# where parse_named_strings has run.

# The editor's header band: one readout, the target, right-aligned to this
# edge at scale 3. The seed readout that used to run right from RAIL_X moved
# into the pane.
HEADER_TARGET_RIGHT = 1860
SEED_DIGITS = 10
NAME_CAP_ROWS = 16            # the name cap, as a sanity bound on the parse

# Confirm, which is still a flat review list - the head rows plus every
# setting, centred on the screen at scale 4 (kSettingsLayout of the
# randomizer-settings-ui plan's 4.5). Three of the head rows replaced the
# readiness summary that used to sit in FINISH's help pane; worlds milestone 6
# added five more, because Confirm is now the ACTIVATION confirmation and B10
# says what it has to state: which world is deactivated and where its save
# goes, which is activated, what happens to its save, and roughly how long it
# takes.
CONFIRM_SCALE = 4

# U2 replaced that flat list with two fixed tiers of label-left/value-right rows,
# so there is no CONFIRM_GAP string between them any more and no head-row count:
# the label sits at the block's left edge and the value at its right. What has to
# be checked instead is that the two never meet in the middle, which is this - the
# smallest gap a row is allowed to leave between label and value. 40px is about a
# space and a half at scale 4, and a row tighter than that reads as one word.
CONFIRM_MIN_GAP = 40

# U4 pass 4: DrawPromptRow's two gaps, mirrored from Controls.cpp. Parsed
# rather than repeated would be better, but they are file-local constants in an
# anonymous namespace; the case below fails loudly if they drift.
PROMPT_GLYPH_GAP = 10
PROMPT_GAP = 46

# Spelled as a name so the joiner below reads without an escape inside it.
NEWLINE = chr(10)

# What a refusal sentence's runtime half is measured as: a title id is 9
# characters, a block count up to 10 digits, and a stored-save error is
# whatever the walk said. 24 characters is longer than any of the first two and
# is the budget the third is held to - a longer one is truncated on screen,
# which is stated in the milestone 6 implementation report.
RUNTIME_STANDIN = "X" * 24
SCREEN_W = 1920

# The picker lists draw at scale 3 (ModelPicker's kItemScale). PICKER_MIN_GAP is
# the clear space demanded between the longest label and the flag column - not a
# measurement, a requirement: below this the two columns read as one.
PICKER_ROW_SCALE = 3
PICKER_MIN_GAP = 60

# The pool sizes the "N OF M" values are drawn from.
POOL_COUNTS = {}
for table, const in (("EnemyPoolTable.h", "kEnemyPoolModelCount"),
                     ("BossPoolTable.h", "kBossPoolModelCount"),
                     ("EnemySkipTable.h", "kEnemySkipModelCount"),
                     ("TrickWeaponTable.h", "kTrickWeaponCount"),
                     ("LeftHandWeaponTable.h", "kLeftHandWeaponCount")):
    POOL_COUNTS[const] = int(re.search(r"const int %s = (\d+);" % const,
                                       read(os.path.join(RND, table))).group(1))

# SettingKind::SaveChoice's two named states, as SettingValueText returns them.
# Retyped here rather than parsed because they are the thing being checked: the
# case below fails if the C++ stops saying exactly these.
SAVE_CHOICE_VALUES = ["KEEP EXISTING", "START FRESH"]

KIND_TO_COUNT = {
    "EnemyPool": POOL_COUNTS["kEnemyPoolModelCount"],
    "EnemySkip": POOL_COUNTS["kEnemySkipModelCount"],
    "BossPool": POOL_COUNTS["kBossPoolModelCount"],
    # The fourth pool kind is not creatures - it is feature 037's 78 trick
    # weapon versions - but its row draws "N OF M" exactly like the other three,
    # so it is measured exactly like them.
    "TrickWeaponPool": POOL_COUNTS["kTrickWeaponCount"],
    # And the fifth, feature 038's 14 left-hand weapons. Same treatment again.
    "LeftHandWeaponPool": POOL_COUNTS["kLeftHandWeaponCount"],
}


def widest_value(entry):
    """The widest string SettingValueText can return for this setting."""
    if entry["kind"] == "Toggle":
        return max(("YES", "NO"), key=lambda s: width(s, ROW_SCALE))
    if entry["kind"] == "SaveChoice":
        return max(SAVE_CHOICE_VALUES, key=lambda s: width(s, ROW_SCALE))
    count = KIND_TO_COUNT[entry["kind"]]
    return max(("%d OF %d" % (n, count) for n in range(count + 1)),
               key=lambda s: width(s, ROW_SCALE))


def widest_name(cap):
    """The widest world name a player can type: `cap` characters of the widest
    glyph NormalizeWorldName lets through. Not the name that happens to be on
    this console."""
    letter = max("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ",
                 key=lambda c: ADV[ROW_SCALE][c])
    return letter * cap


def widest_title_id():
    """A title ID is 4 uppercase letters and 5 digits - the widest one the
    editor can produce, not the one that happens to be the default."""
    letter = max("ABCDEFGHIJKLMNOPQRSTUVWXYZ", key=lambda c: ADV[ROW_SCALE][c])
    digit = max("0123456789", key=lambda c: ADV[ROW_SCALE][c])
    return letter * 4 + digit * 5


# ---------------------------------------------------------------------------
# the two screens' geometry constants, parsed from the screens themselves
# ---------------------------------------------------------------------------

# Every constant BOTH categorised screens must agree on. Named explicitly
# rather than "whatever appears in both files", so renaming one on one side
# shrinks the comparison loudly instead of silently.
# Deliberately NOT here:
#
#   kHeaderY     - wizard-only. Both screens' row-0 values (SEED, BLOODBORNE
#                  TITLE ID) moved into the pane, so Setup Defaults' header band
#                  is now empty and has no item to place. The wizard still draws
#                  TARGET there. The mirror below still checks it against the
#                  wizard, so the constant is not unpinned, only unshared.
#   kHeaderScale - gone from both screens with those readouts. The one thing
#                  left in a header, TARGET, is drawn at kRowScale.
#   kTitleY      - the DEFAULTS tab has no screen title any more. Its top row
#   kTitleScale    is the tab strip, whose geometry is Controls.h's and is
#                  checked there; the editor, which is not tabbed, keeps both
#                  and they stay pinned by the `mirror` dict below.
#   kRailRuleY   - editor-only from worlds milestone 5. The editor's rail is
#   kRailFirstY    ten rows deep where Setup Defaults' is seven, and ten rows
#   kRailPitch     at 76px put the last one's ink two pixels inside the column
#                  rule and under its own focus bar. kRailRow0Y is still shared
#                  - both rails still start in the same place - and the three
#                  that differ are pinned by the `mirror` dict, so they are
#                  unshared and not unpinned.
SHARED_GEOMETRY = [
    # U4 pass 1: the column frames' padding. A fourth copy of a shared number,
    # in the same three files, compared for the same reason as the rest.
    "kPanelPad",
    "kRailX", "kRailW", "kPaneX", "kPaneW", "kPaneValueRight", "kHelpX", "kHelpW",
    "kHeaderRuleY", "kColumnRuleY", "kColumnRuleH",
    "kRailRow0Y",
    "kPaneHeadingY", "kHelpTitleY", "kHelpPitch", "kHelpRuleY", "kHelpBodyY",
    "kHelpTitleMaxLines", "kHelpBodyMaxLines", "kFooterY", "kRuleThickness",
    "kBarOffsetY", "kBarHeight",
    "kHeadingScale", "kRowScale", "kFooterScale",
]

# The subset ALL THREE screens carry - the two categorised ones and the WORLDS
# tab. The worlds rail is a ListLayout rather than a row-0-plus-block rail, so
# the four kRail*Y constants are not in it; everything that decides where the
# three columns are, is.
THREE_COLUMN_GEOMETRY = [
    "kRailX", "kRailW", "kPaneX", "kPaneW", "kPaneValueRight", "kHelpX", "kHelpW",
    "kHeaderRuleY", "kColumnRuleY", "kColumnRuleH", "kPaneHeadingY",
    "kHelpTitleY", "kHelpPitch", "kHelpRuleY", "kHelpBodyY",
    "kHelpTitleMaxLines", "kHelpBodyMaxLines", "kFooterY", "kRuleThickness",
    "kBarOffsetY", "kBarHeight",
    "kHeadingScale", "kRowScale", "kFooterScale",
]

# Clear space demanded inside the rail's left gutter, between the Hunter's
# Mark and the name beside it. Not a measurement, a requirement - but a much
# smaller one than the 24 px the mark needed when it sat at the row's RIGHT
# edge. There, the requirement was that the two NOT read as one thing; here,
# marking the row is the whole point, and the gap only has to keep the rune
# from touching the first letter. Two spaces at scale 3.
MARK_MIN_GAP = 14

# The mark is a baked glyph at codepoint 127 (app/tools/mark_glyph.py), so the
# rail is measured against its real advance out of the atlas, exactly as it is
# measured against the advance of every letter. Nothing in WorldsScreen.cpp
# carries a width for it any more, which is the point: there is no hand-kept
# constant here that can drift away from what the atlas actually holds.
MARK_CHAR = chr(MARK_CODE)

# The worst case the details pane can build, as WorldsScreen::Details builds
# it: seven label/value rows for a world, four for Vanilla plus its own note,
# one for + NEW WORLD plus its own note - and then AT MOST ONE further note,
# either why this world cannot be activated or why no row is ACTIVE. The pane
# has no cursor, so "worst case" has to fit the band rather than scroll.
DETAIL_ROWS_WORLD = 7
DETAIL_ROWS_VANILLA = 4
DETAIL_ROWS_NEW = 1

# Every label and every widest value the details pane can show, so the column
# is measured against what it can be asked to draw rather than against today's
# save.
DETAIL_LABELS = ["STATUS", "SEED", "SETTINGS", "REVISION", "SAVE DATA",
                 "SAVED ON", "LAST PLAYED", "CREATED"]
DETAIL_VALUES = ["ACTIVE", "NOT ACTIVE", "9999999999", "99 OF 99 ON",
                 "99 OF 99", "9999 MB IN 9999 FILES", "NONE", "REVISION 9999",
                 "2026-09-22 12:00:00", "NEVER", "-"]


def parse_prompt_rows(filename):
    """Every `const ButtonPrompt kFoo[] = {...}` in a screen, as {name: rows}.

    A row is a list of (glyph_or_None, label). U4 pass 4 replaced the footer
    STRINGS with these, so what used to be a string-width check is now a check
    against the same arithmetic DrawPromptRow does.
    """
    body = strip_comments(read(os.path.join(UI, filename)))
    out = {}
    for name, inner in re.findall(
            r"const ButtonPrompt (\w+)\[\]\s*=\s*\{(.*?)\};", body, re.S):
        rows = []
        for glyph, label in re.findall(r"\{\s*([A-Za-z_]\w*)\s*,\s*\"([^\"]*)\"\s*\}",
                                       inner):
            rows.append((None if glyph == "nullptr" else glyph, label))
        if rows:
            out[name] = rows
    # The singletons the problem table carries are declared one per line rather
    # than as arrays, so they are picked up separately.
    for name, glyph, label in re.findall(
            r"const ButtonPrompt (\w+)\s*=\s*\{\s*([A-Za-z_]\w*)\s*,\s*\"([^\"]*)\"\s*\}",
            body):
        out[name] = [(None if glyph == "nullptr" else glyph, label)]
    return out


def prompt_row_width(rows, scale):
    """The width DrawPromptRow will lay out, by its own arithmetic.

    Controls.cpp: glyph + kPromptGlyphGap + label per prompt, kPromptGap
    between prompts. The glyph advance comes from button_glyphs, which is what
    the atlas baked, so this is measured rather than assumed.
    """
    total = 0
    for i, (glyph, label) in enumerate(rows):
        if glyph:
            total += button_advance(scale) + PROMPT_GLYPH_GAP
        total += width(label, scale)
        if i + 1 < len(rows):
            total += PROMPT_GAP
    return total


def button_advance(scale):
    """One baked button glyph's advance at this UI scale.

    Read out of the generated header, not recomputed from button_glyphs.py:
    what matters is what the atlas actually carries, and the two would agree
    only for as long as nobody forgets to re-run the generator.
    """
    return ADV[scale][chr(128)]


def parse_geometry(filename):
    """The file-local `const int kFoo = N;` block, plus kRuleColor's triple.

    Plan 4.4 calls this "one geometry, shared by the wizard's Settings step and
    SetupDefaultsScreen", but plan 5 assigns neither milestone a shared layout
    file, so each screen carries its own copy. Two copies of a number is two
    things to drift - this is what stops them.
    """
    body = strip_comments(read(os.path.join(UI, filename)))
    values = {name: int(value) for name, value
              in re.findall(r"const int (\w+)\s*=\s*(-?\d+);", body)}
    rule = re.search(r"const Color kRuleColor = \{([^}]*)\}", body)
    if rule:
        values["kRuleColor"] = tuple(int(x) for x in rule.group(1).split(","))
    return values


def parse_controls_tabs():
    """The tab strip's geometry and its two labels, from Controls.{h,cpp}.

    The strip lives in Controls rather than in either screen precisely so the
    two tabbed screens cannot disagree about it, which is why this is parsed
    once and not compared between screens like everything else here.
    """
    hdr = strip_comments(read(os.path.join(UI, "Controls.h")))
    values = {name: int(value) for name, value
              in re.findall(r"constexpr int (\w+)\s*=\s*(-?\d+);", hdr)}
    src = strip_comments(read(os.path.join(UI, "Controls.cpp")))
    block = re.search(r"kTabLabels\[kTabCount\] = \{(.*?)\};", src, re.S)
    labels = STRING_LITERAL.findall(block.group(1)) if block else []
    return values, labels


def parse_list_layout(filename, name):
    """A `const ListLayout kFoo = { firstY, spacing, bottomLimit, hintGap };`."""
    body = strip_comments(read(os.path.join(UI, filename)))
    m = re.search(r"ListLayout %s\s*=\s*\{([^}]*)\}" % name, body)
    if not m:
        sys.exit("could not find %s in %s" % (name, filename))
    return [int(x) for x in m.group(1).split(",")]


def parse_named_strings(filename):
    """Every `const char* const kFoo = "...";` in a screen, by name.

    The worlds screen's labels, notes and help are named constants for exactly
    this reason: a string the app draws and nothing measures is a string that
    fits until someone edits it.
    """
    body = strip_comments(read(os.path.join(UI, filename)))
    out = {}
    for m in re.finditer(r"const char\* const (\w+)\s*=(.*?);", body, re.S):
        out[m.group(1)] = joined_literals(m.group(2))
    return out


def parse_startup_problems():
    """The StartupProblem enum, and the sentence/prompt table that must cover it.

    Two sources on purpose: the enum is the set of outcomes the code can
    produce, the table is the set the screen can DRAW, and the case below is
    that they are the same set. Reading the table alone would only prove the
    rows it happens to have are well formed.
    """
    hdr = strip_comments(read(os.path.join(UI, "WorldsScreen.h")))
    m = re.search(r"enum class StartupProblem\s*\{([^}]*)\}", hdr)
    if not m:
        sys.exit("could not find enum class StartupProblem in WorldsScreen.h")
    enumerators = [n.strip() for n in m.group(1).split(",") if n.strip()]

    src = strip_comments(read(os.path.join(UI, "WorldsScreen.cpp")))
    t = re.search(r"kProblemTable\[\]\s*=\s*\{(.*?)\};", src, re.S)
    if not t:
        sys.exit("could not find kProblemTable in WorldsScreen.cpp")
    rows = re.findall(r"\{\s*StartupProblem::(\w+)\s*,\s*(\w+)\s*,\s*(\w+)\s*\}",
                      t.group(1))
    return enumerators, rows


def parse_activation_strings():
    """SaveActionName, SaveActionSentence and every refusal sentence.

    Confirm draws all three and this file measures none of them unless it
    reads them out of the source: they live in Game/WorldActivation.cpp,
    because they are the transaction's words rather than a screen's, and a
    screen that draws a string it did not define still has to fit it.

    A refusal sentence can carry runtime text - a title id, a block count, a
    manifest error. Those are replaced with a worst-case stand-in rather than
    dropped, because a sentence that fits only when the substitution is empty
    is a sentence that does not fit.
    """
    body = strip_comments(read(os.path.join(GAME, "WorldActivation.cpp")))

    def switch_table(func):
        chunk = body[body.index(func):]
        chunk = chunk[:chunk.index("\n}")]
        return [m for m in STRING_LITERAL.findall(chunk) if m != "?"]

    names = switch_table("const char* SaveActionName(")
    sentences = switch_table("const char* SaveActionSentence(")

    # Every Refuse(RefusalReason::X, ...) call in CheckActivation, as one
    # string each: the literals joined, with RUNTIME_STANDIN wherever an
    # expression was concatenated in.
    check = body[body.index("Refusal CheckActivation("):]
    check = check[:check.index("const char* SaveActionName(")]
    refusals = []
    for m in re.finditer(r"Refuse\(RefusalReason::(\w+),(.*?)\);", check, re.S):
        arg = m.group(2)
        parts, pos = [], 0
        for lit in STRING_LITERAL.finditer(arg):
            gap = arg[pos:lit.start()]
            if "+" in gap and pos != 0:
                parts.append(RUNTIME_STANDIN)
            parts.append(lit.group(1))
            pos = lit.end()
        if "+" in arg[pos:]:
            parts.append(RUNTIME_STANDIN)
        refusals.append((m.group(1), "".join(parts)))
    return names, sentences, refusals


def parse_screen_categories(filename):
    """The category list a screen iterates, from its own header.

    BOTH tabbed-or-not settings screens carry one, and neither ranges over
    SettingCategory, precisely so the editor-only Save category cannot appear
    on the screen that says what a NEW world starts from (worlds plan 3.3).
    The DIFFERENCE between the two lists is the rule, so both are parsed.
    """
    body = strip_comments(read(os.path.join(UI, filename)))
    block = re.search(r"kCategories\[\] = \{(.*?)\};", body, re.S)
    if not block:
        sys.exit("could not find kCategories in %s" % filename)
    return re.findall(r"SettingCategory::(\w+)", block.group(1))


def parse_world_name_cap():
    """The name cap NormalizeWorldName actually enforces, not a retyped 16."""
    body = strip_comments(read(os.path.join(RND, "WorldStore.cpp")))
    fn = body.split("std::string NormalizeWorldName", 1)[1]
    return int(re.search(r"out\.size\(\) < (\d+)", fn).group(1))


def parse_editor_rail(categories):
    """kHistoryRow and kRailItemCount, as the editor's header declares them.

    Both are written as arithmetic on kCategoryCount, so the offsets are what
    is parsed and the count comes from the editor's own category list - not
    from a 6 or a 7 retyped here.
    """
    body = strip_comments(read(os.path.join(UI, "WorldEditorScreen.h")))
    out = {}
    for name in ("kHistoryRow", "kRailItemCount"):
        m = re.search(r"%s\s*=\s*kCategoryCount \+ (\d+);" % name, body)
        out[name] = len(categories) + int(m.group(1)) if m else None
    return out


def ui_scroll_count(entry):
    """A screen's row count as the OTHER mirror carries it, so the two cannot
    disagree about how many rows a list has. None means that file has no entry
    for the screen, which since U2 is itself the assertion for Confirm.

    COMMENTED-OUT LINES ARE SKIPPED. ui_scroll_verify.py keeps U2's removed
    Confirm entry in a comment as the record of what it used to be, and a
    literal-text search finds that tuple and reports a list where there is none.
    """
    body = NEWLINE.join(line for line in
                     read(os.path.join(HERE, "ui_scroll_verify.py")).splitlines()
                     if not line.lstrip().startswith("#"))
    m = re.search(r'\("%s",\s*[-0-9]+,\s*[-0-9]+,\s*[-0-9]+,\s*[-0-9]+,\s*'
                  r'[-0-9]+,\s*(\d+),' % re.escape(entry), body)
    return int(m.group(1)) if m else None


# ---------------------------------------------------------------------------
# the save-data inventory (spec 4.8)
# ---------------------------------------------------------------------------

RETIRED_IDENTIFIERS = [
    "backupExistingSaveData", "replaceSaveDefaultIsNew",
    "backup_existing_save", "replace_save_default_is_new",
    "SelectReplace", "UpdateSelectReplace", "DrawSelectReplace",
    "ReplaceChoice", "ReplaceOption", "kReplaceOptions", "kReplaceOptionCount",
    "kNewSaveIndex", "kLeaveExistingIndex", "ReplaceDisplayText",
    "selectedBackupLabel_", "backupExistingSave_", "replaceChoice_",
    "kBackupRow", "kReplaceDefaultRow",
]

RETIRED_STRINGS = [
    "BACKUP EXISTING SAVE", "REPLACE SAVE", "DEFAULT REPLACE SAVE",
    "BACKING UP EXISTING SAVE", "SKIPPING BACKUP PROCESS",
    "REMOVING EXISTING SAVE DATA", "LEAVING EXISTING SAVE DATA",
    "RESTORING SAVE DATA",
]


def walk_sources():
    for root, _dirs, files in os.walk(SRC):
        for name in files:
            if name.endswith((".h", ".cpp")):
                yield os.path.join(root, name)


# ---------------------------------------------------------------------------
# a mirror of the store's key chain
# ---------------------------------------------------------------------------

def store_keys():
    text = read(os.path.join(RND, "RandomizerDefaultsStore.cpp"))
    body = strip_comments(text)
    load = re.findall(r'strcmp\(key, "(\w+)"\)', body)
    # The save format string separates its keys with an ESCAPED newline, which
    # is two source characters - without splitting on it, "...=%d\nrandomize_"
    # reads as a key called "nrandomize_...".
    save = re.findall(r'(\w+)=%[dsu]', body.replace("\\n", " "))
    return load, save


def mirror_load(config, known):
    """What LoadRandomizerDefaults does: split on the first '=', ignore any key
    it does not recognise, and leave a wrong-length selection alone."""
    out = {}
    for line in config.split("\n"):
        if "=" not in line:
            continue
        key, value = line.split("=", 1)
        if key in known:
            out[key] = value
    return out


# ---------------------------------------------------------------------------
# the cases
# ---------------------------------------------------------------------------

def main():
    entries, category_labels, rail_help, categories, declared_ids = parse_model()
    bools, selections = parse_defaults()
    cases = []
    detail = []

    # --- 1: every bool field is a setting, every flag is a field -------------
    flagged = [e["flag"] for e in entries if e["flag"]]
    missing = [b for b in bools if b not in flagged]
    twice = [b for b in set(flagged) if flagged.count(b) > 1]
    unknown = [f for f in flagged if f not in bools]
    cases.append(("1: all %d bool fields appear in exactly one entry" % len(bools),
                  not missing and not twice and not unknown))
    if missing or twice or unknown:
        detail.append("   missing=%s twice=%s unknown=%s" % (missing, twice, unknown))
    kinds = [e["kind"] for e in entries]
    cases.append(("1: the five selection fields have exactly one entry each",
                  len(selections) == 5 and
                  all(kinds.count(k) == 1 for k in ("EnemyPool", "EnemySkip",
                                                    "BossPool", "TrickWeaponPool",
                                                    "LeftHandWeaponPool"))))
    # SaveChoice is the second bool-backed kind: it carries a flag exactly as a
    # Toggle does and differs only in how it is named and counted. The four
    # pool kinds carry none - START WITH A TRICK WEAPON and START WITH A
    # LEFT-HAND WEAPON deliberately have no boolean beside their selection
    # (spec 037, plan 4.2; feature 038 inherits it).
    two_state = ("Toggle", "SaveChoice")
    cases.append(("1: every entry is a toggle or a save choice with a flag, or a "
                  "pool without one",
                  all((e["flag"] is not None) == (e["kind"] in two_state)
                      for e in entries)))
    cases.append(("1: exactly one SaveChoice entry, and it is SAVE DATA",
                  [e["label"] for e in entries if e["kind"] == "SaveChoice"] ==
                  ["SAVE DATA"]))

    # --- 2: ids are unique and declared -------------------------------------
    used = [e["id"] for e in entries]
    cases.append(("2: every SettingId appears exactly once, none is unused",
                  sorted(used) == sorted(set(used)) and
                  sorted(used) == sorted(declared_ids)))

    # --- 3: the spec's categories, order and membership ----------------------
    #
    # The spec's 7.1 table is the six SETTINGS categories. SAVE is the worlds
    # feature's seventh and has no row there, because it is not a randomizer
    # setting - it is checked below, as an editor-only category.
    spec_rows = parse_spec_categories()
    settings_categories = [c for c in categories if c != "Save"]
    ok = len(spec_rows) == 6 and settings_categories == categories[:6]
    if ok:
        for i, (spec_name, spec_settings) in enumerate(spec_rows):
            if SPEC_CATEGORY_TO_ENUM.get(spec_name) != settings_categories[i]:
                ok = False
                detail.append("   category %d: spec says %s, model says %s"
                              % (i, spec_name, settings_categories[i]))
                continue
            want = [PROSE_TO_LABEL.get(s, "?" + s) for s in spec_settings]
            got = [e["label"] for e in entries
                   if e["category"] == settings_categories[i]]
            if want != got:
                ok = False
                detail.append("   %s: spec %s, model %s" % (spec_name, want, got))
    cases.append(("3: six categories, their order and their membership match spec 7.1",
                  ok))
    by_label = {e["label"]: e for e in entries}
    cases.append(("3: the four easy modes are in Difficulty, not Bosses (spec 7.1)",
                  all(by_label[l]["category"] == "Difficulty" for l in
                      ("EASY SHADOWS", "EASY ROM", "EASY FAILURES", "EASY EMISSARY"))))
    cases.append(("3: START WITH HUNTER TOOLS is not beside RANDOMIZE WORKSHOP TOOLS",
                  by_label["START WITH HUNTER TOOLS"]["category"] == "WeaponsGear" and
                  by_label["RANDOMIZE WORKSHOP TOOLS"]["category"] == "ItemsTreasure"))
    cases.append(("3: the %d rail labels are declared, one per category"
                  % len(categories), len(category_labels) == len(categories)))
    # Each screen iterates a list of its own rather than the whole enum, so
    # that an editor-only category cannot turn up on the screen that says what
    # a NEW world starts from (worlds plan 3.3, hazard row 6). The two lists
    # differ by exactly Save, and that difference IS the rule.
    defaults_categories = parse_screen_categories("SetupDefaultsScreen.h")
    editor_categories = parse_screen_categories("WorldEditorScreen.h")
    cases.append(("3: the DEFAULTS tab iterates the %d settings categories and "
                  "not SAVE" % len(settings_categories),
                  defaults_categories == settings_categories))
    cases.append(("3: the world editor iterates all %d categories, SAVE last"
                  % len(categories),
                  editor_categories == categories and categories[-1] == "Save"))
    cases.append(("3: SAVE holds exactly the one setting the worlds spec gives it",
                  [e["label"] for e in entries if e["category"] == "Save"] ==
                  ["SAVE DATA"]))
    if defaults_categories != settings_categories or editor_categories != categories:
        detail.append("   DEFAULTS list %s, editor list %s, model %s"
                      % (defaults_categories, editor_categories, categories))

    # --- 4: help coverage ----------------------------------------------------
    cases.append(("4: all %d settings have non-empty help" % len(entries),
                  all(e["help"].strip() for e in entries)))
    cases.append(("4: SEED, BLOODBORNE TITLE ID, NAME and HISTORY have non-empty "
                  "help",
                  all(rail_help[k].strip() for k in
                      ("kSeedHelp", "kTitleIdHelp", "kNameHelp", "kHistoryHelp"))))

    # --- 5: everything fits, measured from the atlas -------------------------
    rail_rows = list(category_labels) + ["NAME", "HISTORY"]
    # Row 0 at its widest REAL value. Every Bloodborne title ID the console can
    # have is CUSA-prefixed (ps4-homebrew-findings.md section 6 lists all six), and CUSA03173 is
    # the struct's own default - this is the 552px row plan M3 sized the column
    # on.
    rail_rows.append("BLOODBORNE TITLE ID   CUSA03173")
    rail_rows.append("BLOODBORNE TITLE ID   NOT SET")
    rail_rows.append("SEED   " + max("0123456789", key=lambda c: ADV[ROW_SCALE][c]) * 10)
    too_wide = [s for s in rail_rows if width(s, ROW_SCALE) > RAIL_W]
    cases.append(("5: every rail row fits %d px at scale 3 (widest %d px)"
                  % (RAIL_W, max(width(s, ROW_SCALE) for s in rail_rows)),
                  not too_wide))
    if too_wide:
        detail.append("   over the rail budget: %s" % too_wide)
    # The title-ID editor cycles all 26 letters and all 10 digits, so a player
    # CAN type a wider ID than any real one. That row is checked against the
    # thing it could actually collide with - the column rule at x 660, which
    # the 580px budget leaves a 20px gutter before - rather than against the
    # budget, which was set from the widest real value.
    worst_row0 = "BLOODBORNE TITLE ID   " + widest_title_id()
    cases.append(("5: the widest TYPEABLE title-ID row (%d px) still clears the "
                  "column rule at x %d" % (width(worst_row0, ROW_SCALE), PANE_X - 20),
                  RAIL_X + width(worst_row0, ROW_SCALE) < PANE_X - 20))

    widest_row = max(entries, key=lambda e: width(e["label"], ROW_SCALE) + VALUE_GAP +
                     width(widest_value(e), ROW_SCALE))
    widest_row_px = (width(widest_row["label"], ROW_SCALE) + VALUE_GAP +
                     width(widest_value(widest_row), ROW_SCALE))
    cases.append(("5: every label + %d + widest value fits %d px (widest %d px, %s)"
                  % (VALUE_GAP, PANE_W, widest_row_px, widest_row["label"]),
                  widest_row_px <= PANE_W))

    headings = [(c, width(c, PANE_HEADING_SCALE)) for c in category_labels]
    cases.append(("5: every category heading fits %d px at scale 4 (widest %d px)"
                  % (PANE_W, max(w for _c, w in headings)),
                  all(w <= PANE_W for _c, w in headings)))

    titles = ([e["label"] for e in entries] +
              ["BLOODBORNE TITLE ID", "SEED", "NAME", "HISTORY"])
    title_lines = {t: len(wrap(t, ROW_SCALE, HELP_W)) for t in titles}
    cases.append(("5: every help title wraps into at most %d lines at %d px (worst %d)"
                  % (HELP_TITLE_MAX_LINES, HELP_W, max(title_lines.values())),
                  max(title_lines.values()) <= HELP_TITLE_MAX_LINES))

    bodies = [e["help"] for e in entries] + list(rail_help.values())
    body_lines = {b[:24]: len(wrap(b, ROW_SCALE, HELP_W)) for b in bodies}
    longest_word = max((w for b in bodies for w in b.split()),
                       key=lambda w: width(w, ROW_SCALE))
    cases.append(("5: every help body wraps into at most %d lines at %d px (worst %d)"
                  % (HELP_BODY_MAX_LINES, HELP_W, max(body_lines.values())),
                  max(body_lines.values()) <= HELP_BODY_MAX_LINES))
    cases.append(("5: no single help word exceeds %d px (widest '%s', %d px)"
                  % (HELP_W, longest_word, width(longest_word, ROW_SCALE)),
                  width(longest_word, ROW_SCALE) <= HELP_W))
    # Not in the plan's list, but this milestone writes the footer and nothing
    # else measures it: it is centred, so its budget is the whole screen.
    # (The Setup Defaults footer is a prompt row since U4 pass 4 and is
    # measured with every other row further down, against DrawPromptRow's own
    # arithmetic rather than as a string.)

    # --- 6: the vertical constants do not overlap ----------------------------
    # Two rails now, not one: the DEFAULTS tab's seven rows and the world
    # editor's ten. They no longer share a pitch (see SHARED_GEOMETRY), so they
    # no longer share a stack either - and the editor's, being the deeper of
    # the two, is the one that has to be shown to fit.
    def rail_stack_for(name, row0_y, extra_top, rule1_y, first_y, pitch,
                       category_count, rule2_y, last_y):
        stack = []
        if name == "editor":
            stack += [
                ("screen title", ink_top(TITLE_Y, TITLE_SCALE),
                 ink_bottom(TITLE_Y, TITLE_SCALE)),
                ("header readout", ink_top(HEADER_Y, HEADER_SCALE),
                 ink_bottom(HEADER_Y, HEADER_SCALE)),
            ]
        stack.append(("header rule", HEADER_RULE_Y, HEADER_RULE_Y + 2))
        stack.append(("rail row 0", ink_top(row0_y, ROW_SCALE),
                      ink_bottom(row0_y, ROW_SCALE)))
        if extra_top is not None:
            stack.append(("rail row 1", ink_top(extra_top, ROW_SCALE),
                          ink_bottom(extra_top, ROW_SCALE)))
        stack.append(("rail rule 1", rule1_y, rule1_y + 2))
        for i in range(category_count):
            y = first_y + i * pitch
            stack.append(("category %d" % i, ink_top(y, ROW_SCALE),
                          ink_bottom(y, ROW_SCALE)))
        if rule2_y is not None:
            stack.append(("rail rule 2", rule2_y, rule2_y + 2))
            stack.append(("last row", ink_top(last_y, ROW_SCALE),
                          ink_bottom(last_y, ROW_SCALE)))
        return stack

    rails = {
        "DEFAULTS": rail_stack_for("defaults", RAIL_ROW0_Y, None, DEFAULTS_RULE1_Y,
                                   DEFAULTS_FIRST_Y, DEFAULTS_PITCH,
                                   len(settings_categories), None, None),
        "editor": rail_stack_for("editor", RAIL_ROW0_Y, EDITOR_ROW1_Y,
                                 EDITOR_RULE1_Y, EDITOR_FIRST_Y, EDITOR_PITCH,
                                 len(categories), EDITOR_RULE2_Y, EDITOR_HISTORY_Y),
    }
    for who, rail_stack in rails.items():
        overlaps = [(a[0], b[0]) for a, b in zip(rail_stack, rail_stack[1:])
                    if b[1] < a[2]]
        cases.append(("6: the %s rail's %d elements are in order and clear of "
                      "each other" % (who, len(rail_stack)), not overlaps))
        if overlaps:
            detail.append("   %s rail overlaps: %s" % (who, overlaps))
        cases.append(("6: the %s rail's ink ends at %d, inside its column rule "
                      "(%d..%d)" % (who, rail_stack[-1][2], COLUMN_RULE_Y,
                                    COLUMN_RULE_Y + COLUMN_RULE_H),
                      rail_stack[-1][2] <= RAIL_INK_END and
                      rail_stack[-1][2] < COLUMN_RULE_Y + COLUMN_RULE_H and
                      rail_stack[-1][2] < ink_top(FOOTER_Y, ROW_SCALE)))

    # The editor's rail runs at its own pitch, so its focus bar has to be shown
    # to still contain a row and still miss the next one at that pitch.
    ed_bar_bottom = EDITOR_FIRST_Y + BAR_OFFSET_Y + BAR_HEIGHT
    cases.append(("6: the editor rail's focus bar contains its row and misses the "
                  "next at pitch %d" % EDITOR_PITCH,
                  EDITOR_FIRST_Y + BAR_OFFSET_Y <= ink_top(EDITOR_FIRST_Y, ROW_SCALE) and
                  ed_bar_bottom >= ink_bottom(EDITOR_FIRST_Y, ROW_SCALE) and
                  ed_bar_bottom < EDITOR_FIRST_Y + EDITOR_PITCH + BAR_OFFSET_Y))

    rail_stack = rails["editor"]
    visible = (PANE_BOTTOM - PANE_FIRST_Y) // PANE_PITCH + 1
    last_row_y = PANE_FIRST_Y + (visible - 1) * PANE_PITCH
    pane_stack = [
        ("pane heading", ink_top(PANE_HEADING_Y, PANE_HEADING_SCALE),
         ink_bottom(PANE_HEADING_Y, PANE_HEADING_SCALE)),
        ("MORE ABOVE", ink_top(PANE_FIRST_Y - PANE_HINT_GAP, ROW_SCALE),
         ink_bottom(PANE_FIRST_Y - PANE_HINT_GAP, ROW_SCALE)),
        ("first row", ink_top(PANE_FIRST_Y, ROW_SCALE),
         ink_bottom(PANE_FIRST_Y, ROW_SCALE)),
        ("last row", ink_top(last_row_y, ROW_SCALE), ink_bottom(last_row_y, ROW_SCALE)),
        ("MORE BELOW", ink_top(last_row_y + PANE_HINT_GAP, ROW_SCALE),
         ink_bottom(last_row_y + PANE_HINT_GAP, ROW_SCALE)),
        ("footer", ink_top(FOOTER_Y, ROW_SCALE), ink_bottom(FOOTER_Y, ROW_SCALE)),
    ]
    overlaps = [(a[0], b[0]) for a, b in zip(pane_stack, pane_stack[1:]) if b[1] < a[2]]
    cases.append(("6: pane heading, hints, %d rows and footer clear each other" % visible,
                  not overlaps and ink_bottom(FOOTER_Y, ROW_SCALE) <= SCREEN_H))
    if overlaps:
        detail.append("   pane overlaps: %s" % overlaps)

    bar_top, bar_bottom = PANE_FIRST_Y + BAR_OFFSET_Y, PANE_FIRST_Y + BAR_OFFSET_Y + BAR_HEIGHT
    cases.append(("6: the focus bar contains its row's ink and misses the next bar",
                  bar_top <= ink_top(PANE_FIRST_Y, ROW_SCALE) and
                  bar_bottom >= ink_bottom(PANE_FIRST_Y, ROW_SCALE) and
                  bar_bottom < PANE_FIRST_Y + PANE_PITCH + BAR_OFFSET_Y))

    help_stack = [("help title", ink_top(HELP_TITLE_Y, ROW_SCALE),
                   ink_bottom(HELP_TITLE_Y + (HELP_TITLE_MAX_LINES - 1) * HELP_PITCH,
                              ROW_SCALE)),
                  ("help rule", HELP_RULE_Y, HELP_RULE_Y + 2),
                  ("help body", ink_top(HELP_BODY_Y, ROW_SCALE),
                   ink_bottom(HELP_BODY_Y + (HELP_BODY_MAX_LINES - 1) * HELP_PITCH,
                              ROW_SCALE))]
    overlaps = [(a[0], b[0]) for a, b in zip(help_stack, help_stack[1:]) if b[1] < a[2]]
    cases.append(("6: help title, rule and %d body lines clear each other and the footer"
                  % HELP_BODY_MAX_LINES,
                  not overlaps and help_stack[-1][2] < ink_top(FOOTER_Y, ROW_SCALE)))
    if overlaps:
        detail.append("   help overlaps: %s" % overlaps)

    # --- 5/6 (editor): the same screen, plus NAME, SEED, HISTORY and Confirm --
    #
    # The editor's rail is the Setup Defaults rail with three more rows and a
    # seventh category, so everything above already covers the labels. What is
    # left is what only the editor has: a second copy of the geometry, a header
    # band, the name editor, the revision list and Confirm.
    wiz = parse_geometry("WorldEditorScreen.cpp")
    setup = parse_geometry("SetupDefaultsScreen.cpp")
    absent = [k for k in SHARED_GEOMETRY if k not in wiz or k not in setup]
    differ = [k for k in SHARED_GEOMETRY
              if k in wiz and k in setup and wiz[k] != setup[k]]
    cases.append(("6: both screens' %d shared geometry constants are identical"
                  % len(SHARED_GEOMETRY), not absent and not differ))
    if absent or differ:
        detail.append("   geometry absent=%s differ=%s"
                      % (absent, [(k, wiz.get(k), setup.get(k)) for k in differ]))
    # U4 PASS 1 TURNED THIS INTO A STRONGER CLAIM. The three screens each used
    # to declare `const Color kRuleColor = { 64, 72, 82 }` - a fourth copy of a
    # constant, compared here so the copies could not drift. They now all read
    # `= Palette::Rule`, so there is nothing left to drift: one definition, three
    # aliases. What is checked is that none of them has gone back to a literal.
    rule_src = {name: strip_comments(read(os.path.join(UI, name + ".cpp")))
                for name in ("WorldEditorScreen", "SetupDefaultsScreen", "WorldsScreen")}
    aliased = {n: "const Color kRuleColor = Palette::Rule;" in s
               for n, s in rule_src.items()}
    cases.append(("U4: all %d screens take their rule colour from the palette"
                  % len(aliased), all(aliased.values())))
    if not all(aliased.values()):
        detail.append("   rule colour still a literal in: %s"
                      % [n for n, ok in aliased.items() if not ok])

    # ...and nothing clears the screen for itself any more. DrawGround() is the
    # single caller, which is what makes the ground - and, in pass 3, the
    # vignette - one change rather than thirteen.
    clears = {n: "renderer.Clear(" in s for n, s in rule_src.items()}
    cases.append(("U4: no screen calls renderer.Clear directly - DrawGround does",
                  not any(clears.values())))
    if any(clears.values()):
        detail.append("   still clears for itself: %s"
                      % [n for n, bad in clears.items() if bad])

    # THE THREE COLUMN FRAMES MUST NOT TOUCH. Each is its column padded by
    # kPanelPad, and the padding is a fourth copy of a shared number, so this
    # checks the gap the padding actually leaves rather than the number itself.
    # At 14px of padding the 40px gutters leave 12px between frames - enough
    # that two frames read as two panels rather than as one box with a line
    # down it. A padding that closed the gap would merge them.
    pad = wiz.get("kPanelPad")
    gaps = []
    if pad is not None:
        gaps = [(PANE_X - pad) - (RAIL_X + RAIL_W + pad),
                (HELP_X - pad) - (PANE_X + PANE_W + pad)]
    cases.append(("U4: the three column frames leave a gap between them (%s px)"
                  % gaps, pad is not None and all(g > 0 for g in gaps)))

    # --- U1: the activation loading screen IS the startup loading screen ------
    #
    # Two files draw the same four elements at two moments of one session - the
    # startup screen while the app comes up, the editor while a world activates.
    # A player who can tell that two files drew them is looking at a bug, so the
    # geometry is compared rather than merely copied, exactly as the rail's is
    # above. The editor's names are kAct*, the startup screen's kLoad*.
    #
    # THE BAR IS NOW IN THIS LIST TOO. It was excluded while startup drew five
    # discrete cells and the activation one continuous fill; the developer asked
    # for startup to match the activation, so both are now a track with a fill
    # over it and the minimum-fill width is twinned along with the band. The one
    # remaining difference is what DRIVES the fill - five coarse stages there, a
    # real 0..1 here - which is behaviour rather than geometry.
    worlds_geo = parse_geometry("WorldsScreen.cpp")
    LOADING_TWINS = [
        ("kActHeadlineY", "kLoadWordmarkY"),   # the headline
        ("kActRuleY",     "kLoadRuleY"),       # the rule under it
        ("kActBarY",      "kLoadBarY"),        # the bar's band
        ("kActBarH",      "kLoadBarH"),
        ("kActBarMinFillW", "kLoadBarMinFillW"),
        ("kActNameY",     "kLoadWordY"),       # the line under the bar
        ("kActBlockX",    "kLoadBlockX"),      # the block both are centred on
        ("kActBlockW",    "kLoadBlockW"),
    ]
    missing = [(a, b) for a, b in LOADING_TWINS
               if a not in wiz or b not in worlds_geo]
    moved = [(a, wiz.get(a), b, worlds_geo.get(b)) for a, b in LOADING_TWINS
             if a in wiz and b in worlds_geo and wiz[a] != worlds_geo[b]]
    cases.append(("U1: the activation loading screen's %d geometry constants "
                  "equal the startup screen's" % len(LOADING_TWINS),
                  not missing and not moved))
    if missing or moved:
        detail.append("   loading geometry missing=%s moved=%s" % (missing, moved))

    # The block has to be centred on the screen, which is the one property the
    # twin comparison above cannot catch: both files could be wrong together.
    cases.append(("U1: the loading block is centred on the screen",
                  wiz.get("kActBlockX") is not None and
                  wiz.get("kActBlockW") is not None and
                  2 * wiz["kActBlockX"] + wiz["kActBlockW"] == SCREEN_W))

    # This file's own constants are a THIRD copy. Checking cases 5 and 6
    # against numbers that no longer match the screens would pass a screen that
    # is wrong, which is the one failure a geometry mirror must not have.
    mirror = {
        "kRailX": RAIL_X, "kRailW": RAIL_W, "kPaneX": PANE_X, "kPaneW": PANE_W,
        "kPaneValueRight": PANE_VALUE_RIGHT, "kHelpX": HELP_X, "kHelpW": HELP_W,
        "kTitleY": TITLE_Y, "kHeaderY": HEADER_Y, "kHeaderRuleY": HEADER_RULE_Y,
        "kColumnRuleY": COLUMN_RULE_Y, "kColumnRuleH": COLUMN_RULE_H,
        "kRailRow0Y": RAIL_ROW0_Y, "kRailRow1Y": EDITOR_ROW1_Y,
        "kRailRuleY": EDITOR_RULE1_Y,
        "kRailFirstY": EDITOR_FIRST_Y, "kRailPitch": EDITOR_PITCH,
        "kPaneHeadingY": PANE_HEADING_Y, "kHelpTitleY": HELP_TITLE_Y,
        "kHelpPitch": HELP_PITCH, "kHelpRuleY": HELP_RULE_Y,
        "kHelpBodyY": HELP_BODY_Y, "kHelpTitleMaxLines": HELP_TITLE_MAX_LINES,
        "kHelpBodyMaxLines": HELP_BODY_MAX_LINES, "kFooterY": FOOTER_Y,
        "kTitleScale": TITLE_SCALE,
        "kHeadingScale": PANE_HEADING_SCALE, "kRowScale": ROW_SCALE,
        "kBarOffsetY": BAR_OFFSET_Y, "kBarHeight": BAR_HEIGHT,
        "kRailRule2Y": EDITOR_RULE2_Y, "kHistoryY": EDITOR_HISTORY_Y,
        "kHeaderTargetRight": HEADER_TARGET_RIGHT,
        "kCategoryRows": len(categories),
    }
    stale = [(k, v, wiz.get(k)) for k, v in mirror.items() if wiz.get(k) != v]
    cases.append(("6: this file's %d geometry constants match the screens'"
                  % len(mirror), not stale))
    if stale:
        detail.append("   stale mirror constants: %s" % stale)
    # The DEFAULTS rail keeps its own three, which nothing else pins now that
    # they are out of SHARED_GEOMETRY.
    setup_rail = {"kRailRuleY": DEFAULTS_RULE1_Y, "kRailFirstY": DEFAULTS_FIRST_Y,
                  "kRailPitch": DEFAULTS_PITCH}
    setup_stale = [(k, v, setup.get(k)) for k, v in setup_rail.items()
                   if setup.get(k) != v]
    cases.append(("6: the DEFAULTS rail's own %d constants match this file's"
                  % len(setup_rail), not setup_stale))
    if setup_stale:
        detail.append("   stale DEFAULTS rail constants: %s" % setup_stale)

    rail = parse_editor_rail(categories)
    cases.append(("6: the editor's rail is %d rows - NAME, SEED, %d categories, "
                  "HISTORY" % (len(categories) + 3, len(categories)),
                  rail["kRailItemCount"] == len(categories) + 3 and
                  rail["kHistoryRow"] == len(categories) + 2))

    # The header band. One readout now, right-aligned to HEADER_TARGET_RIGHT at
    # its widest possible value, and it has to clear the centred screen title
    # above it rather than a second readout beside it.
    editor_strings = parse_named_strings("WorldEditorScreen.cpp")
    hdr_target = "TARGET  " + widest_title_id()
    target_start = HEADER_TARGET_RIGHT - width(hdr_target, ROW_SCALE)
    title_end = (SCREEN_W + width(editor_strings["kScreenTitle"], TITLE_SCALE)) // 2
    cases.append(("5: the header readout clears the screen title (%d px apart, "
                  "starts at %d)" % (target_start - title_end, target_start),
                  target_start > title_end and HEADER_TARGET_RIGHT <= SCREEN_W and
                  target_start > RAIL_X))
    # --- U4 pass 4: every footer is a PROMPT ROW now -------------------------
    #
    # What used to be one centred string per screen is a row of glyph/label
    # pairs, so the width check has to reproduce DrawPromptRow's arithmetic
    # rather than measure a string. Every row in every screen is checked, not
    # just the three that used to have named constants - the conversion turned
    # a handful of strings into a dozen rows and they all have to fit.
    all_rows = {}
    for src in ("WorldEditorScreen.cpp", "WorldsScreen.cpp",
                "SetupDefaultsScreen.cpp", "ModelPicker.cpp"):
        for name, rows in parse_prompt_rows(src).items():
            all_rows[src.replace(".cpp", "") + "." + name] = rows
    widest_prompt_name = max(all_rows,
                             key=lambda k: prompt_row_width(all_rows[k], ROW_SCALE))
    widest_prompt_px = prompt_row_width(all_rows[widest_prompt_name], ROW_SCALE)
    cases.append(("U4: all %d prompt rows fit the screen (widest %s, %d px of %d)"
                  % (len(all_rows), widest_prompt_name, widest_prompt_px, SCREEN_W),
                  all(prompt_row_width(r, ROW_SCALE) <= SCREEN_W
                      for r in all_rows.values())))

    # A prompt whose glyph is not one of the five baked ones would draw a blank
    # gap - DrawText advances for a glyph it cannot find.
    known = {"kBtnCross", "kBtnCircle", "kBtnTriangle", "kBtnSquare",
             "kBtnDpad", "kBtnDpadVert", "kBtnDpadHorz"}
    unknown = sorted({g for r in all_rows.values() for g, _ in r if g and g not in known})
    cases.append(("U4: every prompt glyph is one of the %d baked buttons" % len(known),
                  not unknown))
    if unknown:
        detail.append("   unknown prompt glyphs: %s" % unknown)

    editor_footer = " ".join(l for _, l in all_rows["WorldEditorScreen.kFooterPrompts"])
    # OPTIONS saves the world AND opens the activation confirmation, and the
    # footer has to say both - it said only SAVE until 2026-09-25.
    cases.append(("5: the editor footer says OPTIONS both saves and activates",
                  "OPTIONS SAVE AND ACTIVATE" in editor_footer))

    digit = max("0123456789", key=lambda c: ADV[ROW_SCALE][c])
    name_cap = parse_world_name_cap()

    # The name editor: sixteen characters of the widest glyph the alphabet
    # allows, at the editor scale, centred - so the budget is the screen.
    name_scale = 6
    widest_edit_name = widest_name(name_cap)
    cases.append(("5: the name editor's %d characters fit the screen at scale %d "
                  "(%d px of %d)"
                  % (name_cap, name_scale, width(widest_edit_name, name_scale),
                     SCREEN_W),
                  width(widest_edit_name, name_scale) <= SCREEN_W and
                  name_cap == NAME_CAP_ROWS))

    # Confirm, which is the ACTIVATION confirmation from worlds milestone 6 on.
    # Eight head rows at their widest possible values, then every setting in
    # category order at its widest value. Centred, so the budget is the whole
    # screen.
    action_names, action_sentences, refusals = parse_activation_strings()
    widest_action = max(action_names, key=lambda t: width(t, CONFIRM_SCALE))
    widest_outgoing = max([editor_strings[k] for k in
                           ("kOutgoingNoContainer", "kOutgoingEmpty",
                            "kOutgoingNowhere", "kOutgoingFiled")],
                          key=lambda t: width(t, CONFIRM_SCALE))
    widest_duration = max([editor_strings[k] for k in
                           ("kDurationUnknown", "kDurationShort",
                            "kDurationMedium", "kDurationLong")],
                          key=lambda t: width(t, CONFIRM_SCALE))
    # A world name is capped at 16 characters; the "no world is active" line is
    # longer than that, so the DEACTIVATING row is measured against whichever
    # of the two is wider.
    widest_world = max([widest_name(name_cap), editor_strings["kNoOutgoingWorld"]],
                       key=lambda t: width(t, CONFIRM_SCALE))
    # --- U2: two fixed tiers, label left and value right, and NO list --------
    #
    # Each row is a label at the block's left edge and a value at its right, so
    # what has to fit is label + value + a gap that keeps them apart - measured
    # against the BLOCK (1200px), not the screen. A row that fits 1920 but not
    # the block would collide with itself in the middle.
    block_w = wiz["kConfirmBlockRight"] - wiz["kConfirmBlockX"]
    tier1 = [(editor_strings["kRowDeactivating"], widest_world),
             (editor_strings["kRowOutgoingSave"], widest_outgoing),
             (editor_strings["kRowActivating"],   widest_name(name_cap)),
             (editor_strings["kRowIncomingSave"], widest_action),
             (editor_strings["kRowHowLong"],      widest_duration)]
    # Tier 2's settings value is a COUNT, and the widest it can be is every
    # toggle on at two digits each - "99 OF 99 ON" is past what the model can
    # reach and is therefore a safe bound.
    tier2 = [(editor_strings["kRowSeed"],     digit * SEED_DIGITS),
             (editor_strings["kRowTarget"],   widest_title_id()),
             (editor_strings["kRowSettings"],
              max("99 OF 99 ON", editor_strings["kConfirmVanillaSettings"],
                  key=lambda t: width(t, ROW_SCALE)))]

    def tier_fits(rows, scale):
        """The widest (label, value) pair and whether it clears the block."""
        worst = max(rows, key=lambda r: width(r[0], scale) + width(r[1], scale))
        used = width(worst[0], scale) + width(worst[1], scale)
        return worst, used

    worst1, used1 = tier_fits(tier1, CONFIRM_SCALE)
    worst2, used2 = tier_fits(tier2, ROW_SCALE)
    cases.append(("U2: Confirm is %d fixed rows - %d plan and %d world, no list"
                  % (wiz["kConfirmPlanRows"] + wiz["kConfirmWorldRows"],
                     wiz["kConfirmPlanRows"], wiz["kConfirmWorldRows"]),
                  wiz["kConfirmPlanRows"] == len(tier1) and
                  wiz["kConfirmWorldRows"] == len(tier2) and
                  ui_scroll_count("Editor Confirm") is None))
    cases.append(("U2: every tier 1 row fits the block at scale %d "
                  "(worst %d of %d px, %s / %s)"
                  % (CONFIRM_SCALE, used1, block_w, worst1[0], worst1[1]),
                  used1 + CONFIRM_MIN_GAP <= block_w))
    cases.append(("U2: every tier 2 row fits the block at scale %d "
                  "(worst %d of %d px, %s / %s)"
                  % (ROW_SCALE, used2, block_w, worst2[0], worst2[1]),
                  used2 + CONFIRM_MIN_GAP <= block_w))
    cases.append(("U2: the Confirm block is centred on the screen",
                  wiz["kConfirmBlockX"] + wiz["kConfirmBlockRight"] == SCREEN_W))

    # THE SETTINGS LIST MUST NOT COME BACK without the layout changing with it.
    # U2 removed it deliberately - no drill-in, no expansion - and the failure
    # this guards against is a well-meaning edit that reinstates a per-setting
    # loop into a screen whose geometry has no room to scroll it.
    editor_src = strip_comments(read(os.path.join(UI, "WorldEditorScreen.cpp")))
    confirm_body = editor_src.split("ConfirmWorldRows")[-1]
    cases.append(("U2: Confirm builds no per-setting rows and cannot scroll",
                  "SettingValueText" not in confirm_body and
                  "confirmScroll_" not in editor_src and
                  "kConfirmGap" not in editor_src))
    # The count it shows instead has to be the SAME sentence the WORLDS rail
    # shows against the row, or two screens state one fact two ways.
    cases.append(("U2: the settings row is EnabledToggleCount over ToggleCount",
                  "EnabledToggleCount" in editor_src and
                  "ToggleCount()" in editor_src))

    # B10 says the confirmation states which world is deactivated and where its
    # save goes, which is activated, what happens to its save, and roughly how
    # long it takes. Five rows, and this is what stops one of them quietly
    # going away - U2 moved them into tier 1 and dropped NAME, which ACTIVATING
    # already said, but B10 itself is unchanged.
    cases.append(("6: Confirm states all five B10 activation facts",
                  all(editor_strings[k] in "".join(r[0] for r in tier1) for k in
                      ("kRowDeactivating", "kRowOutgoingSave", "kRowActivating",
                       "kRowIncomingSave", "kRowHowLong"))))

    # The state line, and the sentence band under it. The sentence is the phase
    # 6 row in the player's words, or the refusal's own sentence - both far too
    # long for scale 4, so both are wrapped at the row scale into a band of
    # kConfirmSentenceMax lines. A sentence that needs one more line than the
    # band has is a sentence the player only reads half of.
    band_w = wiz["kConfirmSentenceW"]
    band_max = wiz["kConfirmSentenceMax"]
    state_line = max([editor_strings["kConfirmHeading"],
                      editor_strings["kConfirmChecking"],
                      editor_strings["kCannotActivate"] + " - SAVE DIRECTORY"],
                     key=lambda t: width(t, CONFIRM_SCALE))
    cases.append(("5: the Confirm state line fits the screen at scale 4 "
                  "(widest %d px, %s)" % (width(state_line, CONFIRM_SCALE), state_line),
                  width(state_line, CONFIRM_SCALE) <= SCREEN_W))

    banded = action_sentences + [text for _, text in refusals]
    worst = max(banded, key=lambda t: len(wrap(t, ROW_SCALE, band_w)))
    cases.append(("6: all %d Confirm sentences wrap into %d lines of %d px "
                  "(worst is %d lines: %s)"
                  % (len(banded), band_max, band_w, len(wrap(worst, ROW_SCALE, band_w)),
                     worst),
                  all(len(wrap(t, ROW_SCALE, band_w)) <= band_max for t in banded)))

    # ...and the whole screen stacks without collision, top to bottom. There is
    # no list any more, so this replaces the old "clears the MORE ABOVE hint"
    # check with the run of five gaps that now decide the screen's shape:
    # state -> sentence -> tier 1 -> rule -> tier 2 -> footer.
    band_last_y = wiz["kConfirmSentenceY"] + (band_max - 1) * wiz["kConfirmSentencePitch"]
    tier1_last_y = wiz["kConfirmPlanY"] + (wiz["kConfirmPlanRows"] - 1) * wiz["kConfirmPlanPitch"]
    tier2_last_y = wiz["kConfirmWorldY"] + (wiz["kConfirmWorldRows"] - 1) * wiz["kConfirmWorldPitch"]
    footer_y = SCREEN_H - 80
    stack = [
        ("the sentence band clears the state line",
         ink_top(wiz["kConfirmSentenceY"], ROW_SCALE),
         ink_bottom(wiz["kConfirmStateY"], CONFIRM_SCALE)),
        ("tier 1 clears the sentence band",
         ink_top(wiz["kConfirmPlanY"], CONFIRM_SCALE),
         ink_bottom(band_last_y, ROW_SCALE)),
        ("the tier rule clears tier 1",
         wiz["kConfirmTierRuleY"],
         ink_bottom(tier1_last_y, CONFIRM_SCALE)),
        ("tier 2 clears the tier rule",
         ink_top(wiz["kConfirmWorldY"], ROW_SCALE),
         wiz["kConfirmTierRuleY"] + wiz["kRuleThickness"]),
        ("the footer clears tier 2",
         ink_top(footer_y, ROW_SCALE),
         ink_bottom(tier2_last_y, ROW_SCALE)),
    ]
    for label, top, bottom in stack:
        cases.append(("U2: %s (%d px)" % (label, top - bottom), top > bottom))

    # Every block's own pitch has to clear its scale's line box, or a block
    # overlaps itself however well it clears its neighbours.
    for label, pitch, scale in (("the sentence band", wiz["kConfirmSentencePitch"], ROW_SCALE),
                                ("tier 1", wiz["kConfirmPlanPitch"], CONFIRM_SCALE),
                                ("tier 2", wiz["kConfirmWorldPitch"], ROW_SCALE)):
        box = INK[scale][1] - INK[scale][0]
        cases.append(("U2: %s does not overlap itself (pitch %d, line box %d)"
                      % (label, pitch, box), pitch >= box))

    # Confirm's ONE footer line, which says ACTIVATE rather than COMMIT and drops
    # the OPTIONS half entirely when there is nothing to activate. U2 removed the
    # UP DOWN SCROLL line above it along with the list it described.
    go_labels = " ".join(l for _, l in all_rows["WorldEditorScreen.kConfirmGo"])
    back_labels = " ".join(l for _, l in all_rows["WorldEditorScreen.kConfirmBack"])
    cases.append(("6: the Confirm footer offers OPTIONS only when it activates",
                  "OPTIONS" in go_labels and "OPTIONS" not in back_labels))

    # HISTORY: one line per revision, "REVISION n   <seed>   n CHANGED", with a
    # cursor, at the row scale. The revision count is unbounded, so the widest
    # line is measured at four-digit numbers rather than at today's.
    history_rows = ["REVISION 9999   " + digit * SEED_DIGITS + "   9999 CHANGED",
                    "REVISION 9999   " + digit * SEED_DIGITS + "   CREATED",
                    "THIS WORLD HAS NOT BEEN SAVED YET"]
    widest_history = max(history_rows, key=lambda t: width(t, ROW_SCALE))
    cases.append(("5: every HISTORY row fits the screen at scale %d (widest %d px)"
                  % (ROW_SCALE, width(widest_history, ROW_SCALE)),
                  width(widest_history, ROW_SCALE) <= SCREEN_W))
    cases.append(("6: ui_scroll_verify.py checks the revision list",
                  ui_scroll_count("Editor history") is not None))

    # --- 5/6 (worlds): the tab strip and the WORLDS tab ---------------------
    #
    # A third screen on the same three-column split, plus the strip that sits
    # above two of them. Everything here is measured the same way the two
    # categorised screens are; what is NOT here is the scroll band, which is
    # ui_scroll_verify.py's "Worlds rail" and "Worlds details" entries.
    tabs, tab_labels = parse_controls_tabs()
    worlds = parse_geometry("WorldsScreen.cpp")
    worlds_strings = parse_named_strings("WorldsScreen.cpp")
    name_cap = parse_world_name_cap()

    cases.append(("6: the tab strip declares %d tabs and names both of them"
                  % tabs.get("kTabCount", 0),
                  tabs.get("kTabCount") == 2 and
                  tab_labels == ["WORLDS", "DEFAULTS"]))

    # Both labels, boxed and spaced as DrawTabs lays them out. The strip is
    # left-aligned on the rail's own edge, so its budget is the screen.
    strip_x = tabs["kTabX"]
    tab_boxes = []
    for label in tab_labels:
        box_w = width(label, tabs["kTabScale"]) + tabs["kTabPadX"] * 2
        tab_boxes.append((strip_x, strip_x + box_w, label))
        strip_x += box_w + tabs["kTabGap"]
    strip_end = tab_boxes[-1][1] if tab_boxes else tabs["kTabX"]
    cases.append(("5: both tab labels fit their boxes and the strip ends at %d of %d"
                  % (strip_end, SCREEN_W), strip_end <= SCREEN_W))
    gaps = [b[0] - a[1] for a, b in zip(tab_boxes, tab_boxes[1:])]
    cases.append(("6: the tab boxes clear each other (%s px apart)" % gaps,
                  all(g == tabs["kTabGap"] for g in gaps)))
    cases.append(("6: the active tab's box contains its own label's ink",
                  tabs["kTabY"] + tabs["kTabBoxOffsetY"] <=
                  ink_top(tabs["kTabY"], tabs["kTabScale"]) and
                  tabs["kTabY"] + tabs["kTabBoxOffsetY"] + tabs["kTabBoxHeight"] >=
                  ink_bottom(tabs["kTabY"], tabs["kTabScale"])))

    # All three screens' copies of the three-column split.
    setup_absent = [k for k in THREE_COLUMN_GEOMETRY if k not in worlds or k not in setup]
    setup_differ = [k for k in THREE_COLUMN_GEOMETRY
                    if k in worlds and k in setup and worlds[k] != setup[k]]
    cases.append(("6: the worlds screen's %d three-column constants match the "
                  "settings screens'" % len(THREE_COLUMN_GEOMETRY),
                  not setup_absent and not setup_differ))
    if setup_absent or setup_differ:
        detail.append("   worlds geometry absent=%s differ=%s"
                      % (setup_absent, [(k, worlds.get(k), setup.get(k))
                                        for k in setup_differ]))
    cases.append(("6: all three screens draw their rules in the same colour",
                  worlds.get("kRuleColor") == setup.get("kRuleColor")))

    # The header band: the tab strip, the active tab's name, and the readout
    # that says which world is active - which is the only place UNMANAGED and
    # FIRST RUN are stated, because in both of them no rail row is marked
    # (worlds B32, B4).
    heading_scale = tabs["kTabHeadingScale"]
    worlds_stack = [
        ("tab box", tabs["kTabY"] + tabs["kTabBoxOffsetY"],
         tabs["kTabY"] + tabs["kTabBoxOffsetY"] + tabs["kTabBoxHeight"]),
        ("heading band",
         min(ink_top(tabs["kTabHeadingY"], heading_scale),
             ink_top(worlds["kStateY"], ROW_SCALE)),
         max(ink_bottom(tabs["kTabHeadingY"], heading_scale),
             ink_bottom(worlds["kStateY"], ROW_SCALE))),
        ("header rule", worlds["kHeaderRuleY"],
         worlds["kHeaderRuleY"] + worlds["kRuleThickness"]),
        ("column rule top", worlds["kColumnRuleY"], worlds["kColumnRuleY"]),
        ("pane heading", ink_top(worlds["kPaneHeadingY"], worlds["kHeadingScale"]),
         ink_bottom(worlds["kPaneHeadingY"], worlds["kHeadingScale"])),
    ]
    overlaps = [(a[0], b[0]) for a, b in zip(worlds_stack, worlds_stack[1:])
                if b[1] < a[2]]
    cases.append(("6: the worlds screen's %d header elements are in order and clear"
                  % len(worlds_stack), not overlaps))
    if overlaps:
        detail.append("   worlds header overlaps: %s" % overlaps)

    # The heading and the readout share a band, so they have to clear each
    # other across the screen instead of down it.
    heading_end = tabs["kTabX"] + width("DEFAULTS", heading_scale)
    readout = "ACTIVE  " + widest_name(name_cap)
    readout_start = worlds["kStateRight"] - width(readout, ROW_SCALE)
    cases.append(("5: the heading and the ACTIVE readout clear each other "
                  "(%d px apart)" % (readout_start - heading_end),
                  heading_end < readout_start and
                  worlds["kStateRight"] <= SCREEN_W))

    # The rail. A world name is at most `name_cap` characters of A-Z, 0-9 and
    # space (worlds plan P10), and the Hunter's Mark sits in a gutter at the
    # row's LEFT, which every row is indented past - nothing clips the label,
    # so both the gutter's own clear space and what is left for a name are
    # checked here.
    mark_w = width(MARK_CHAR, ROW_SCALE)
    if not mark_w:
        sys.exit("the mark glyph (codepoint 127) is not in the baked atlas - "
                 "re-run gen_font_atlas.py")
    gutter = worlds["kMarkGutter"]
    cases.append(("5: the rail's %d px gutter holds the %d px mark and %d px of "
                  "clear space before the name"
                  % (gutter, mark_w, gutter - mark_w),
                  gutter - mark_w >= MARK_MIN_GAP))
    rail_budget = worlds["kRailW"] - gutter
    worlds_rail_rows = [worlds_strings["kRowNewWorld"], widest_name(name_cap),
                        "VANILLA"]
    widest_rail = max(worlds_rail_rows, key=lambda t: width(t, ROW_SCALE))
    cases.append(("5: every worlds rail row fits the %d px left after the "
                  "gutter (widest %d px, %s)"
                  % (rail_budget, width(widest_rail, ROW_SCALE), widest_rail),
                  width(widest_rail, ROW_SCALE) <= rail_budget))
    rail_layout = parse_list_layout("WorldsScreen.cpp", "kRailLayout")
    # The mark is drawn at the row's own y and scale, so it and the label are
    # both bounded relative to that y - but by different boxes, because the
    # mark is deliberately outside the text ink box (see load_atlas). It stands
    # taller than a capital, so the focus bar has to contain it too. This is
    # the check that fails if the rune is ever redrawn larger.
    mark_top, mark_bottom = MARK_INK[ROW_SCALE]
    bar_top = worlds["kBarOffsetY"]
    bar_bottom = bar_top + worlds["kBarHeight"]
    # The gutter is inside the rail, so the mark is inside the focus bar
    # horizontally as well - the bar starts at kRailX and the mark is drawn
    # there. A negative gutter, or one wider than the rail, would put it
    # outside; both are caught by the gutter cases above.

    cases.append(("6: the worlds rail's focus bar contains its row's ink and "
                  "the mark's (mark ink %d..%d, bar %d..%d)"
                  % (mark_top, mark_bottom, bar_top, bar_bottom),
                  bar_top <= ink_top(0, ROW_SCALE) and
                  bar_bottom >= ink_bottom(0, ROW_SCALE) and
                  bar_top <= mark_top and bar_bottom >= mark_bottom and
                  bar_bottom < rail_layout[1] + bar_top))

    # The details pane. Its heading is the row's own name, so it is measured
    # at the widest one a player can type.
    cases.append(("5: the details heading fits %d px at scale %d (widest %d px)"
                  % (worlds["kPaneW"], worlds["kHeadingScale"],
                     width(widest_name(name_cap), worlds["kHeadingScale"])),
                  width(widest_name(name_cap), worlds["kHeadingScale"]) <=
                  worlds["kPaneW"] and
                  width(worlds_strings["kRowNewWorld"], worlds["kHeadingScale"]) <=
                  worlds["kPaneW"]))
    widest_detail = max(((width(l, ROW_SCALE) + VALUE_GAP + width(v, ROW_SCALE), l, v)
                         for l in DETAIL_LABELS for v in DETAIL_VALUES))
    cases.append(("5: every detail label + %d + widest value fits %d px "
                  "(widest %d px, %s / %s)"
                  % (VALUE_GAP, worlds["kPaneW"], widest_detail[0],
                     widest_detail[1], widest_detail[2]),
                  widest_detail[0] <= worlds["kPaneW"]))

    # The notes, and the worst case the pane can be asked to draw. It has no
    # cursor: a note that pushed a row past the band would be a row the player
    # cannot reach.
    detail_layout = parse_list_layout("WorldsScreen.cpp", "kDetailLayout")
    detail_visible = ((detail_layout[2] - detail_layout[0]) // detail_layout[1]) + 1
    notes = {k: v for k, v in worlds_strings.items() if k.startswith("kNote")}
    note_lines = {k: len(wrap(v, ROW_SCALE, worlds["kPaneW"])) for k, v in notes.items()}
    worst_note = max(note_lines.values())
    own_note = max(note_lines["kNoteNewWorld"], note_lines["kNoteVanilla"])
    worst_pane = max(DETAIL_ROWS_WORLD + worst_note,
                     DETAIL_ROWS_VANILLA + note_lines["kNoteVanilla"] + worst_note,
                     DETAIL_ROWS_NEW + note_lines["kNoteNewWorld"] + worst_note)
    cases.append(("5: the worst details pane is %d rows of the %d the band draws"
                  % (worst_pane, detail_visible), worst_pane <= detail_visible))
    detail.append("   details pane: %d rows visible, worst %d "
                  "(notes %s, own-note worst %d)"
                  % (detail_visible, worst_pane, note_lines, own_note))
    longest_note_word = max((w for v in notes.values() for w in v.split()),
                            key=lambda w: width(w, ROW_SCALE))
    cases.append(("5: no note word exceeds the pane (%d px, '%s')"
                  % (width(longest_note_word, ROW_SCALE), longest_note_word),
                  width(longest_note_word, ROW_SCALE) <= worlds["kPaneW"]))
    cases.append(("5: ui_scroll_verify.py checks the details pane at %d rows"
                  % detail_visible,
                  ui_scroll_count("Worlds details") == detail_visible))

    # The help column, measured exactly as the settings screens' is.
    world_help = {k: v for k, v in worlds_strings.items() if k.startswith("kHelp")}
    help_titles = [v for k, v in world_help.items() if k.endswith("Title")]
    help_bodies = [v for k, v in world_help.items() if k.endswith("Body")]
    worst_title = max(len(wrap(t, ROW_SCALE, worlds["kHelpW"])) for t in help_titles)
    worst_body = max(len(wrap(b, ROW_SCALE, worlds["kHelpW"])) for b in help_bodies)
    longest_help_word = max((w for b in help_bodies + help_titles for w in b.split()),
                            key=lambda w: width(w, ROW_SCALE))
    cases.append(("5: the %d worlds help titles wrap into at most %d lines (worst %d)"
                  % (len(help_titles), worlds["kHelpTitleMaxLines"], worst_title),
                  worst_title <= worlds["kHelpTitleMaxLines"]))
    cases.append(("5: the %d worlds help bodies wrap into at most %d lines (worst %d)"
                  % (len(help_bodies), worlds["kHelpBodyMaxLines"], worst_body),
                  worst_body <= worlds["kHelpBodyMaxLines"] and len(help_bodies) == 3))
    cases.append(("5: no worlds help word exceeds %d px (widest '%s', %d px)"
                  % (worlds["kHelpW"], longest_help_word,
                     width(longest_help_word, ROW_SCALE)),
                  width(longest_help_word, ROW_SCALE) <= worlds["kHelpW"]))

    # The two footers this milestone writes. The DEFAULTS tab now has two -
    # Left/Right change a value in the pane and switch tabs on the rail, and
    # the footer says whichever is true right now (B31).
    # Both are prompt rows since U4 pass 4, and their WIDTHS are checked with
    # every other row above. What is checked here is that each still SAYS what
    # this milestone put in it - the two facts that made them two footers.
    worlds_row = all_rows["WorldsScreen.kFooterPrompts"]
    rail_row = all_rows["SetupDefaultsScreen.kRailFooterPrompts"]
    # The words LEFT RIGHT are gone from both: the axis d-pad glyph says which
    # half of the pad, so the label is just the verb. What is asserted is that
    # TABS is still offered and still on the horizontal d-pad.
    def has(row, glyph, label):
        return any(g == glyph and l == label for g, l in row)

    cases.append(("5: the worlds footer puts TABS on the horizontal d-pad and "
                  "DELETE on TRIANGLE",
                  has(worlds_row, "kBtnDpadHorz", "TABS") and
                  has(worlds_row, "kBtnTriangle", "DELETE")))
    cases.append(("5: the DEFAULTS rail footer puts TABS on the horizontal d-pad",
                  has(rail_row, "kBtnDpadHorz", "TABS")))

    # An AXIS d-pad and a plain one mean different things, so a prompt that
    # means one direction must not carry the all-arms glyph - that is the pad
    # saying "any direction" over a label that means one axis.
    axis_labels = {"TABS", "CHANGE", "SELECT"}
    miscast = [(k, l) for k, r in all_rows.items() for g, l in r
               if g == "kBtnDpad" and l in axis_labels]
    cases.append(("U4: no single-axis prompt carries the all-arms d-pad", not miscast))
    if miscast:
        detail.append("   all-arms d-pad on an axis prompt: %s" % miscast)

    # The delete confirmation (B17). Centred on the whole screen rather than
    # drawn into the details pane, so these are measured against 1920 - which
    # is why they carry their own kDelete/kRefuse prefixes and are not swept up
    # by the kNote pass above.
    delete_strings = {k: v for k, v in worlds_strings.items()
                      if k.startswith("kDelete") or k.startswith("kRefuse")}
    widest_delete = max(delete_strings.values(),
                        key=lambda t: width(t, ROW_SCALE))
    cases.append(("5: the %d delete-confirmation strings fit the screen "
                  "(widest %d px of %d)"
                  % (len(delete_strings), width(widest_delete, ROW_SCALE), SCREEN_W),
                  len(delete_strings) >= 6 and
                  all(width(v, ROW_SCALE) <= SCREEN_W
                      for v in delete_strings.values())))
    # B17 and B20 in the strings themselves: the confirmation has to say the
    # save is kept, and Vanilla and the active world each have to be refused
    # rather than silently ignored.
    cases.append(("5: the confirmation says the save is kept, and both refusals "
                  "exist",
                  "KEPT" in delete_strings.get("kDeleteLine2", "") and
                  "CANNOT BE DELETED" in delete_strings.get("kRefuseVanilla", "") and
                  "ACTIVE" in delete_strings.get("kRefuseActive", "")))

    # --- 5/10 (startup screen): the error state ------------------------------
    #
    # The startup sequence's four failures, each with a sentence the SCREEN
    # owns and a prompt of its own. Centred on the whole screen, so measured
    # against 1920 and not against a column - and the mapping is asserted
    # total against the enum, because the failure a missing row produces is a
    # blank error screen rather than a build error.
    problem_enum, problem_rows = parse_startup_problems()
    expected_outcomes = [e for e in problem_enum if e != "None"]
    row_outcomes = [r[0] for r in problem_rows]
    cases.append(("10: the outcome table has one row for each of the %d non-None "
                  "StartupProblem outcomes" % len(expected_outcomes),
                  row_outcomes == expected_outcomes and
                  len(expected_outcomes) == 4))
    if row_outcomes != expected_outcomes:
        detail.append("   outcome table %s vs enum %s" % (row_outcomes, expected_outcomes))

    # The SENTENCE is still a named string. The PROMPT became a ButtonPrompt in
    # U4 pass 4, so it is checked against the parsed prompt rows instead - it is
    # no longer a string and cannot be measured as one.
    missing = [r[1] for r in problem_rows if r[1] not in worlds_strings]
    missing += [r[2] for r in problem_rows
                if ("WorldsScreen." + r[2]) not in all_rows]
    cases.append(("10: every sentence and prompt the table names resolves",
                  not missing))
    if missing:
        detail.append("   outcome table names unmeasurable: %s" % missing)

    sentences = [worlds_strings[r[1]] for r in problem_rows if r[1] in worlds_strings]
    cases.append(("10: all %d outcome sentences are distinct and non-empty"
                  % len(sentences),
                  len(set(sentences)) == len(sentences) and all(sentences)))

    # B10: X for the three that leave a usable app behind, O for the one that
    # does not. NoSignedInPlayer is the only row that may offer the exit.
    prompts = {r[0]: r[2] for r in problem_rows}
    cases.append(("10: every prompt is kPromptContinue or kPromptExit, and only "
                  "NoSignedInPlayer exits",
                  all(v in ("kPromptContinue", "kPromptExit") for v in prompts.values()) and
                  [k for k, v in prompts.items() if v == "kPromptExit"] ==
                  ["NoSignedInPlayer"]))

    sub_scale = worlds["kStartupSubScale"]
    widest_sentence = max(sentences, key=lambda t: width(t, sub_scale))
    cases.append(("5: the %d startup problem sentences fit the screen at scale %d "
                  "(widest %d px of %d)"
                  % (len(sentences), sub_scale, width(widest_sentence, sub_scale),
                     SCREEN_W),
                  all(width(t, sub_scale) <= SCREEN_W for t in sentences)))
    detail.append("   widest startup problem sentence %d px: %s"
                  % (width(widest_sentence, sub_scale), widest_sentence))
    cases.append(("5: the startup problem title fits the screen at scale %d (%d px)"
                  % (worlds["kStartupTitleScale"],
                     width(worlds_strings["kProblemTitle"],
                           worlds["kStartupTitleScale"])),
                  width(worlds_strings["kProblemTitle"],
                        worlds["kStartupTitleScale"]) <= SCREEN_W))

    # The two footer lines the error state draws, in the band ui_scroll_verify
    # already checks: the scroll hint above, the outcome's prompt below.
    # Both are single-prompt rows since U4 pass 4, and their widths ride the
    # all_rows check above. What is asserted here is that the problem table
    # still offers a way OFF the screen for each outcome - an error state with
    # no prompt is the one thing an error state cannot be.
    problem_prompts = [all_rows["WorldsScreen.kPromptContinue"],
                       all_rows["WorldsScreen.kPromptExit"]]
    cases.append(("5: both startup problem prompts name a button and a verb",
                  all(len(r) == 1 and r[0][0] and r[0][1] for r in problem_prompts)))

    # --- 5/6 (startup screen): the loading state ----------------------------
    #
    # Four elements and nothing else: the wordmark, a rule, a five-cell bar and
    # the word LOADING, all inside one centred block. Nothing available here
    # can observe the bar stepping or the frame ordering - what IS checkable is
    # that the block is centred, that five equal cells divide it exactly, that
    # both words fit inside it, and that the stack does not collide with
    # itself. A cell width that rounded would put the fifth cell off the
    # block's right edge on a TV and nowhere else.
    title_scale = worlds["kStartupTitleScale"]
    block_w = worlds["kLoadBlockW"]
    cases.append(("5: the wordmark fits the loading block at scale %d (%d px of %d)"
                  % (title_scale, width(worlds_strings["kWordmark"], title_scale),
                     block_w),
                  width(worlds_strings["kWordmark"], title_scale) <= block_w))
    cases.append(("5: LOADING fits the loading block at scale %d (%d px of %d)"
                  % (sub_scale, width(worlds_strings["kLoadingWord"], sub_scale),
                     block_w),
                  worlds_strings["kLoadingWord"] == "LOADING" and
                  width(worlds_strings["kLoadingWord"], sub_scale) <= block_w))

    cases.append(("6: the loading block is centred (%d + %d + %d == %d)"
                  % (worlds["kLoadBlockX"], block_w, worlds["kLoadBlockX"], SCREEN_W),
                  worlds["kLoadBlockX"] * 2 + block_w == SCREEN_W))

    # B3: FIVE EQUAL steps, now as one continuous fill rather than five cells.
    # kLoadCellW / kLoadCellGap are gone, so what used to be an "the cells divide
    # the block exactly" check is an "every step lands on a whole pixel and the
    # last one fills the block" check. A denominator that did not divide 1000
    # would leave the finished bar short of its own right edge.
    stages = worlds["kLoadStageCount"]
    steps = [block_w * n // stages for n in range(stages + 1)]
    cases.append(("6: the loading bar's %d steps divide the %d px block exactly "
                  "(%s)" % (stages, block_w, steps),
                  stages == 5 and block_w % stages == 0 and
                  steps[-1] == block_w and steps[0] == 0))
    cases.append(("6: the loading bar and the activation bar are both one fill",
                  "kLoadCellW" not in worlds and "kLoadCellGap" not in worlds and
                  "kLoadBarMinFillW" in worlds and "kActBarMinFillW" in wiz))

    load_stack = [
        ("wordmark", ink_top(worlds["kLoadWordmarkY"], title_scale),
         ink_bottom(worlds["kLoadWordmarkY"], title_scale)),
        ("rule", worlds["kLoadRuleY"], worlds["kLoadRuleY"] + worlds["kRuleThickness"]),
        ("bar", worlds["kLoadBarY"], worlds["kLoadBarY"] + worlds["kLoadBarH"]),
        ("LOADING", ink_top(worlds["kLoadWordY"], sub_scale),
         ink_bottom(worlds["kLoadWordY"], sub_scale)),
    ]
    overlaps = [(a[0], b[0]) for a, b in zip(load_stack, load_stack[1:]) if b[1] < a[2]]
    cases.append(("6: the loading state's %d elements are in order, clear of each "
                  "other and inside the screen" % len(load_stack),
                  not overlaps and load_stack[0][1] >= 0 and
                  load_stack[-1][2] <= SCREEN_H))
    if overlaps:
        detail.append("   loading stack overlaps: %s" % overlaps)

    # --- 7: nothing from the save-data inventory survives --------------------
    survivors = []
    for path in walk_sources():
        body = strip_comments(read(path))
        for ident in RETIRED_IDENTIFIERS:
            if re.search(r"\b%s\b" % re.escape(ident), body):
                survivors.append("%s: %s" % (os.path.basename(path), ident))
        for s in RETIRED_STRINGS:
            if s in body:
                survivors.append("%s: %r" % (os.path.basename(path), s))
    cases.append(("7: no save-data identifier from spec 4.8 survives under app/src",
                  not survivors))
    if survivors:
        detail.append("   survivors: %s" % survivors)

    # --- 8: an old defaults.cfg still loads ----------------------------------
    load, save = store_keys()
    retired = ("backup_existing_save", "replace_save_default_is_new")
    values = {}
    for key in load:
        if key == "bloodborne_title_id":
            values[key] = "CUSA03173"
        elif key == "bosses_included":
            values[key] = "1" * POOL_COUNTS["kBossPoolModelCount"]
        elif key == "enemies_included":
            values[key] = "1" * POOL_COUNTS["kEnemyPoolModelCount"]
        elif key == "enemies_skipped":
            values[key] = "0" * POOL_COUNTS["kEnemySkipModelCount"]
        elif key == "trick_weapons_included":
            # Nothing ticked, which is this key's own fail-safe: the 78-character
            # line has to survive the round trip as written, and a line of the
            # wrong length must leave the selection alone (checked in
            # trick_weapons_verify.py T14).
            values[key] = "0" * POOL_COUNTS["kTrickWeaponCount"]
        elif key == "left_hand_weapons_included":
            # The same fail-safe again, at 14 characters rather than 78. The two
            # lines are DIFFERENT lengths on purpose: each is bound to one table,
            # so a line pasted into the wrong key is rejected by the length guard
            # instead of being remapped onto another feature's weapons.
            values[key] = "0" * POOL_COUNTS["kLeftHandWeaponCount"]
        elif key == "last_seed":
            values[key] = "4294967295"
        else:
            values[key] = "1"
    lines = ["%s=%s" % (k, v) for k, v in values.items()]
    lines.insert(1, "backup_existing_save=1")
    lines.append("replace_save_default_is_new=0")
    lines.append("some_key_from_a_later_build=7")
    parsed = mirror_load("\n".join(lines) + "\n", set(load))

    cases.append(("8: the store neither reads nor writes either retired key",
                  not any(k in load or k in save for k in retired)))
    cases.append(("8: a defaults.cfg carrying both retired keys ignores them",
                  not any(k in parsed for k in retired) and
                  "some_key_from_a_later_build" not in parsed))
    cases.append(("8: every one of the %d live keys round-trips unchanged" % len(load),
                  parsed == values))
    cases.append(("8: every key the store reads it also writes, and vice versa",
                  sorted(load) == sorted(save)))

    # --- 9: the picker's alignment band holds every row it can be given ------
    #
    # ModelPicker draws label-left / flag-right inside a fixed band instead of
    # space-padding a centred string, which only ever lined up in a monospace
    # font. A regenerated pool table with a longer display name would overlap
    # the flag column silently, so the band is measured against the real tables
    # here rather than trusted.
    picker_src = read(os.path.join(UI, "ModelPicker.cpp"))
    row_x = int(re.search(r"const int kRowX\s*=\s*(\d+);", picker_src).group(1))
    flag_right = int(re.search(r"const int kRowFlagRight\s*=\s*(\d+);", picker_src).group(1))
    band = flag_right - row_x

    widest_label, widest_row = 0, ""
    for table in ("EnemyPoolTable.h", "EnemySkipTable.h", "BossPoolTable.h"):
        src = read(os.path.join(RND, table))
        for model, name in re.findall(r'"(c\d+)"\s*,\s*"([^"]+)"', src):
            label = "%s %s" % (model.upper(), name)
            w = width(label, PICKER_ROW_SCALE)
            if w > widest_label:
                widest_label, widest_row = w, label

    # The flag column is as wide as the longest word any host passes.
    flag_words = re.findall(r'"([A-Z-]+)",\s*"([A-Z-]+)",', read(os.path.join(UI, "ModelPicker.h")))
    widest_flag = max((width(w, PICKER_ROW_SCALE)
                       for pair in flag_words for w in pair), default=0)

    gap = band - widest_label - widest_flag
    cases.append(("9: the picker band fits the widest row of all three tables",
                  gap >= PICKER_MIN_GAP))
    detail.append("  picker band %d px: widest label %d (%s), widest flag %d, gap %d"
                  % (band, widest_label, widest_row, widest_flag, gap))
    cases.append(("9: the picker band is centred on the 1920 surface",
                  row_x + band // 2 == 960))

    # --- U4 pass 2: the picker's row box and its panel ----------------------
    #
    # Both of these are tight, and both were arithmetic before they were code.
    pick = parse_geometry("ModelPicker.cpp")
    pitch = PICKER_PITCH if "PICKER_PITCH" in globals() else 52
    box_top = pick["kPickerBarOffsetY"]
    box_bot = box_top + pick["kPickerBarHeight"]

    # The band must contain the row's whole ink box - scale 3 puts ink 9..44
    # below the draw y - and must not reach into the next row's band.
    ink_a, ink_b = INK[PICKER_ROW_SCALE][0], INK[PICKER_ROW_SCALE][1]
    cases.append(("U4: the picker band holds the row's ink (%d..%d inside %d..%d)"
                  % (ink_a, ink_b, box_top, box_bot),
                  box_top <= ink_a and box_bot >= ink_b))
    # One band ends at box_bot; the next begins a whole pitch after box_top, so
    # what separates them is pitch - height and nothing to do with the offset.
    cases.append(("U4: picker bands do not touch (%d px between them)"
                  % (pitch - pick["kPickerBarHeight"]),
                  box_bot < pitch + box_top))

    # --- U4 pass 3: the graded ground ---------------------------------------
    #
    # The vignette is drawn as opaque concentric rings rather than blended over
    # the frame, and the ENTIRE justification for that is that the rings tile the
    # screen exactly once - same pixel count as the clear they replace, on a
    # software renderer where every pixel is CPU work. If they ever overlap or
    # leave a gap, that argument is void and the screen is visibly wrong, so the
    # tiling is re-derived here rather than trusted.
    ctrl = parse_geometry("Controls.cpp")
    vm, vs = ctrl["kVignetteMargin"], ctrl["kVignetteSteps"]
    vstep = vm // vs
    ring_px = 0
    for i in range(vs):
        inset = i * vstep
        w, h = SCREEN_W - 2 * inset, SCREEN_H - 2 * inset
        ring_px += 2 * (w * vstep) + 2 * (vstep * (h - 2 * vstep))
    centre_px = (SCREEN_W - 2 * vm) * (SCREEN_H - 2 * vm)
    cases.append(("U4: the vignette's %d rings tile the screen exactly once "
                  "(%d + %d == %d)" % (vs, ring_px, centre_px, SCREEN_W * SCREEN_H),
                  ring_px + centre_px == SCREEN_W * SCREEN_H))
    cases.append(("U4: the vignette margin divides into whole rings (%d / %d = %d)"
                  % (vm, vs, vstep), vm % vs == 0 and 2 * vm < SCREEN_H))

    # The ground must stay darkest at the edge and reach full Ground at the
    # centre; a floor at or above 100 would be no vignette at all.
    cases.append(("U4: the vignette darkens toward the edge (floor %d%%)"
                  % ctrl["kVignetteFloor"],
                  0 < ctrl["kVignetteFloor"] < 100))

    # THE PICKER HAS NO PANEL FRAME, and this is the measurement that says why -
    # kept as a case so the conclusion is re-derived on every run rather than
    # remembered. A frame enclosing the list must enclose its scroll hints, so
    # its top edge belongs between the count line's ink and MORE ABOVE's. That
    # window is eight pixels wide, for a 2px line needing clearance either side.
    #
    # If this case ever FAILS, the window has opened - somebody moved the count
    # line or the list - and framing the picker is worth revisiting.
    hint_above = parse_list_layout("ModelPicker.cpp", "kPickerLayout")[0] -                  parse_list_layout("ModelPicker.cpp", "kPickerLayout")[3]
    window = ink_top(hint_above, PICKER_ROW_SCALE) - ink_bottom(185, PICKER_ROW_SCALE)
    cases.append(("U4: the picker still has no room for a panel frame "
                  "(%d px between the count line and MORE ABOVE)" % window,
                  window < 16 and "DrawPanelFrame" not in picker_src))

    failures = 0
    for name, ok in cases:
        print("  %-72s %s" % (name, "ok" if ok else "FAILED"))
        if not ok:
            failures += 1
    for line in detail:
        print(line)
    print("%d/%d passing" % (len(cases) - failures, len(cases)))

    print()
    print("model: %d settings, %d toggles, %d drill-ins, %d categories"
          % (len(entries), kinds.count("Toggle"),
             len(entries) - kinds.count("Toggle"), len(categories)))
    for i, name in enumerate(categories):
        members = [e["label"] for e in entries if e["category"] == name]
        print("  %-14s %d  %s" % (name, len(members), ", ".join(members)))
    print("widest rail row %d/%d px, widest pane row %d/%d px, "
          "worst help body %d/%d lines"
          % (max(width(s, ROW_SCALE) for s in rail_rows), RAIL_W,
             widest_row_px, PANE_W, max(body_lines.values()), HELP_BODY_MAX_LINES))

    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
