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
  3  the six categories, their order, their membership and the within-category
     order match spec 7.1 - including the two placements it says must not be
     "corrected"
  4  every entry, plus SEED, BLOODBORNE TITLE ID and FINISH, has help text
  5  every rail row, pane row, heading and help string fits its column, measured
     from the atlas
  6  the vertical constants of plan 4.4 and 4.5 do not overlap, under the same
     ink box ui_scroll_verify.py uses
  7  no identifier from the save-data inventory (spec 4.8) survives in app/src
  8  a defaults.cfg carrying both retired keys loads with them ignored and
     everything else honoured

Milestone 4 extends 5 and 6 to the Enable wizard, which is the same screen with
two more rail rows (SEED above the categories, FINISH below them), a two-sided
header band, and a Confirm list generated from the same table. The wizard keeps
its OWN copy of the geometry constants - plan 5 gives the two screens no shared
layout file - so this also parses both screens and fails if any shared constant
disagrees with its twin. That comparison is the thing standing between "one
geometry" and two that drift.

Usage:
    python settings_ui_verify.py
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "..", "src")
UI = os.path.join(SRC, "UI")
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
    for name in ("kSeedHelp", "kTitleIdHelp", "kFinishHelp"):
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
                                 r"EnemySkipSelection) (\w+);", body))
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
    "Easy Shadows": "EASY SHADOWS",
    "Easy Rom": "EASY ROM",
    "Easy Failures": "EASY FAILURES",
    "Easy Emissary": "EASY EMISSARY",
    "Enable Mergo Darkness": "ENABLE MERGO DARKNESS",
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

def load_atlas():
    """(advances, ink box) per scale, parsed from the generated header.

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

    adv, ink = {}, {}
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
            code += 1
            if v[3] and v[4]:
                max_bearing_y = max(max_bearing_y, v[6])
                max_below = max(max_below, v[4] - v[6])
        adv[scale] = glyphs
        ink[scale] = (m["ascent"] - max_bearing_y, m["ascent"] + max_below)
    return adv, ink


ADV, INK = load_atlas()


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
RAIL_RULE1_Y = 296
RAIL_FIRST_Y, RAIL_PITCH = 330, 76
RAIL_RULE2_Y = 778            # wizard only - below the six categories
FINISH_Y = 806                # wizard only
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

RAIL_INK_END = 850            # the lowest ink in the rail column (FINISH)

FOOTER_LINE = "UP DOWN MOVE   LEFT RIGHT CHANGE   X SELECT   O BACK   OPTIONS SAVE"
# The wizard's Settings step drops OPTIONS SAVE: OPTIONS commits, and it
# commits on Confirm only (spec 10, 9.2).
WIZARD_FOOTER_LINE = "UP DOWN MOVE   LEFT RIGHT CHANGE   X SELECT   O BACK"

# The wizard's header band: the seed readout runs right from RAIL_X at scale 4,
# the target readout is right-aligned to this edge at scale 3.
HEADER_TARGET_RIGHT = 1860
SEED_DIGITS = 10

# Confirm, which is still a flat review list - the seed row plus all 18
# settings, centred on the screen at scale 4 (kSettingsLayout of plan 4.5).
CONFIRM_SCALE = 4
CONFIRM_GAP = "   "           # what ConfirmItems puts between label and value
SCREEN_W = 1920

# The help pane's readiness summary, shown while the rail cursor is on FINISH.
SUMMARY_BLANK_LINES = 1       # one blank line between the summary and the prose

# The picker lists draw at scale 3 (ModelPicker's kItemScale). PICKER_MIN_GAP is
# the clear space demanded between the longest label and the flag column - not a
# measurement, a requirement: below this the two columns read as one.
PICKER_ROW_SCALE = 3
PICKER_MIN_GAP = 60

# The pool sizes the "N OF M" values are drawn from.
POOL_COUNTS = {}
for table, const in (("EnemyPoolTable.h", "kEnemyPoolModelCount"),
                     ("BossPoolTable.h", "kBossPoolModelCount"),
                     ("EnemySkipTable.h", "kEnemySkipModelCount")):
    POOL_COUNTS[const] = int(re.search(r"const int %s = (\d+);" % const,
                                       read(os.path.join(RND, table))).group(1))

KIND_TO_COUNT = {
    "EnemyPool": POOL_COUNTS["kEnemyPoolModelCount"],
    "EnemySkip": POOL_COUNTS["kEnemySkipModelCount"],
    "BossPool": POOL_COUNTS["kBossPoolModelCount"],
}


def widest_value(entry):
    """The widest string SettingValueText can return for this setting."""
    if entry["kind"] == "Toggle":
        return max(("YES", "NO"), key=lambda s: width(s, ROW_SCALE))
    count = KIND_TO_COUNT[entry["kind"]]
    return max(("%d OF %d" % (n, count) for n in range(count + 1)),
               key=lambda s: width(s, ROW_SCALE))


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
SHARED_GEOMETRY = [
    "kRailX", "kRailW", "kPaneX", "kPaneW", "kPaneValueRight", "kHelpX", "kHelpW",
    "kTitleY", "kHeaderRuleY", "kColumnRuleY", "kColumnRuleH",
    "kRailRow0Y", "kRailRuleY", "kRailFirstY", "kRailPitch",
    "kPaneHeadingY", "kHelpTitleY", "kHelpPitch", "kHelpRuleY", "kHelpBodyY",
    "kHelpTitleMaxLines", "kHelpBodyMaxLines", "kFooterY", "kRuleThickness",
    "kBarOffsetY", "kBarHeight",
    "kTitleScale", "kHeadingScale", "kRowScale", "kFooterScale",
]


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


def parse_wizard_rail():
    """kFinishRow and kRailItemCount, as the wizard's header declares them."""
    body = strip_comments(read(os.path.join(UI, "EnableWizardScreen.h")))
    out = {}
    for name in ("kFinishRow", "kRailItemCount"):
        m = re.search(r"%s\s*=\s*kCategoryCount \+ (\d+);" % name, body)
        out[name] = 6 + int(m.group(1)) if m else None
    return out


def ui_scroll_count(entry):
    """A screen's row count as the OTHER mirror carries it, so the two cannot
    disagree about how many rows Confirm has."""
    body = read(os.path.join(HERE, "ui_scroll_verify.py"))
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
    cases.append(("1: the three selection fields have exactly one entry each",
                  len(selections) == 3 and
                  all(kinds.count(k) == 1 for k in ("EnemyPool", "EnemySkip", "BossPool"))))
    cases.append(("1: every entry is a toggle with a flag or a pool without one",
                  all((e["flag"] is not None) == (e["kind"] == "Toggle") for e in entries)))

    # --- 2: ids are unique and declared -------------------------------------
    used = [e["id"] for e in entries]
    cases.append(("2: every SettingId appears exactly once, none is unused",
                  sorted(used) == sorted(set(used)) and
                  sorted(used) == sorted(declared_ids)))

    # --- 3: the spec's categories, order and membership ----------------------
    spec_rows = parse_spec_categories()
    ok = len(spec_rows) == 6 and len(categories) == 6
    if ok:
        for i, (spec_name, spec_settings) in enumerate(spec_rows):
            if SPEC_CATEGORY_TO_ENUM.get(spec_name) != categories[i]:
                ok = False
                detail.append("   category %d: spec says %s, model says %s"
                              % (i, spec_name, categories[i]))
                continue
            want = [PROSE_TO_LABEL.get(s, "?" + s) for s in spec_settings]
            got = [e["label"] for e in entries if e["category"] == categories[i]]
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
    cases.append(("3: the six rail labels are declared, one per category",
                  len(category_labels) == 6))

    # --- 4: help coverage ----------------------------------------------------
    cases.append(("4: all %d settings have non-empty help" % len(entries),
                  all(e["help"].strip() for e in entries)))
    cases.append(("4: SEED, BLOODBORNE TITLE ID and FINISH have non-empty help",
                  all(rail_help[k].strip() for k in
                      ("kSeedHelp", "kTitleIdHelp", "kFinishHelp"))))

    # --- 5: everything fits, measured from the atlas -------------------------
    rail_rows = list(category_labels) + ["FINISH"]
    # Row 0 at its widest REAL value. Every Bloodborne title ID the console can
    # have is CUSA-prefixed (Game/GameInfo.cpp lists all six), and CUSA03173 is
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

    titles = [e["label"] for e in entries] + ["BLOODBORNE TITLE ID", "SEED", "FINISH"]
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
    cases.append(("5: the footer line fits the screen (%d px of 1920)"
                  % width(FOOTER_LINE, ROW_SCALE),
                  width(FOOTER_LINE, ROW_SCALE) <= 1920))

    # --- 6: the vertical constants do not overlap ----------------------------
    rail_stack = [
        ("screen title", ink_top(TITLE_Y, TITLE_SCALE), ink_bottom(TITLE_Y, TITLE_SCALE)),
        ("header readout", ink_top(HEADER_Y, HEADER_SCALE),
         ink_bottom(HEADER_Y, HEADER_SCALE)),
        ("header rule", HEADER_RULE_Y, HEADER_RULE_Y + 2),
        ("rail row 0", ink_top(RAIL_ROW0_Y, ROW_SCALE), ink_bottom(RAIL_ROW0_Y, ROW_SCALE)),
        ("rail rule 1", RAIL_RULE1_Y, RAIL_RULE1_Y + 2),
    ]
    for i in range(6):
        y = RAIL_FIRST_Y + i * RAIL_PITCH
        rail_stack.append(("category %d" % i, ink_top(y, ROW_SCALE),
                           ink_bottom(y, ROW_SCALE)))
    rail_stack.append(("rail rule 2", RAIL_RULE2_Y, RAIL_RULE2_Y + 2))
    rail_stack.append(("FINISH", ink_top(FINISH_Y, ROW_SCALE),
                       ink_bottom(FINISH_Y, ROW_SCALE)))

    overlaps = [(a[0], b[0]) for a, b in zip(rail_stack, rail_stack[1:]) if b[1] < a[2]]
    cases.append(("6: the rail column's %d elements are in order and clear of each other"
                  % len(rail_stack), not overlaps))
    if overlaps:
        detail.append("   rail overlaps: %s" % overlaps)
    cases.append(("6: the rail's ink ends at %d, inside its column rule (%d..%d)"
                  % (rail_stack[-1][2], COLUMN_RULE_Y, COLUMN_RULE_Y + COLUMN_RULE_H),
                  rail_stack[-1][2] <= RAIL_INK_END and
                  rail_stack[-1][2] < COLUMN_RULE_Y + COLUMN_RULE_H and
                  rail_stack[-1][2] < ink_top(FOOTER_Y, ROW_SCALE)))

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

    # --- 5/6 (wizard): the same screen, plus SEED, FINISH and Confirm --------
    #
    # The wizard's rail is the Setup Defaults rail with two more rows, so
    # everything above already covers the six categories, FINISH and the SEED
    # row. What is left is what only the wizard has: a second copy of the
    # geometry, a two-sided header band, the readiness summary, and Confirm.
    wiz = parse_geometry("EnableWizardScreen.cpp")
    setup = parse_geometry("SetupDefaultsScreen.cpp")
    absent = [k for k in SHARED_GEOMETRY if k not in wiz or k not in setup]
    differ = [k for k in SHARED_GEOMETRY
              if k in wiz and k in setup and wiz[k] != setup[k]]
    cases.append(("6: both screens' %d shared geometry constants are identical"
                  % len(SHARED_GEOMETRY), not absent and not differ))
    if absent or differ:
        detail.append("   geometry absent=%s differ=%s"
                      % (absent, [(k, wiz.get(k), setup.get(k)) for k in differ]))
    cases.append(("6: both screens draw their rules in the same colour",
                  wiz.get("kRuleColor") is not None and
                  wiz.get("kRuleColor") == setup.get("kRuleColor")))

    # This file's own constants are a THIRD copy. Checking cases 5 and 6
    # against numbers that no longer match the screens would pass a screen that
    # is wrong, which is the one failure a geometry mirror must not have.
    mirror = {
        "kRailX": RAIL_X, "kRailW": RAIL_W, "kPaneX": PANE_X, "kPaneW": PANE_W,
        "kPaneValueRight": PANE_VALUE_RIGHT, "kHelpX": HELP_X, "kHelpW": HELP_W,
        "kTitleY": TITLE_Y, "kHeaderY": HEADER_Y, "kHeaderRuleY": HEADER_RULE_Y,
        "kColumnRuleY": COLUMN_RULE_Y, "kColumnRuleH": COLUMN_RULE_H,
        "kRailRow0Y": RAIL_ROW0_Y, "kRailRuleY": RAIL_RULE1_Y,
        "kRailFirstY": RAIL_FIRST_Y, "kRailPitch": RAIL_PITCH,
        "kPaneHeadingY": PANE_HEADING_Y, "kHelpTitleY": HELP_TITLE_Y,
        "kHelpPitch": HELP_PITCH, "kHelpRuleY": HELP_RULE_Y,
        "kHelpBodyY": HELP_BODY_Y, "kHelpTitleMaxLines": HELP_TITLE_MAX_LINES,
        "kHelpBodyMaxLines": HELP_BODY_MAX_LINES, "kFooterY": FOOTER_Y,
        "kTitleScale": TITLE_SCALE,
        "kHeadingScale": PANE_HEADING_SCALE, "kRowScale": ROW_SCALE,
        "kBarOffsetY": BAR_OFFSET_Y, "kBarHeight": BAR_HEIGHT,
        "kRailRule2Y": RAIL_RULE2_Y, "kFinishY": FINISH_Y,
        "kHeaderTargetRight": HEADER_TARGET_RIGHT,
    }
    stale = [(k, v, wiz.get(k)) for k, v in mirror.items() if wiz.get(k) != v]
    cases.append(("6: this file's %d geometry constants match the screens'"
                  % len(mirror), not stale))
    if stale:
        detail.append("   stale mirror constants: %s" % stale)

    rail = parse_wizard_rail()
    cases.append(("6: the wizard's rail is 8 rows - SEED, six categories, FINISH",
                  rail["kRailItemCount"] == len(category_labels) + 2 and
                  rail["kFinishRow"] == len(category_labels) + 1))

    # The header band. Two readouts, one growing right from RAIL_X and one
    # right-aligned to HEADER_TARGET_RIGHT, at their widest possible values.
    hdr_seed = "SEED  " + max("0123456789", key=lambda c: ADV[ROW_SCALE][c]) * SEED_DIGITS
    hdr_target = "TARGET  " + widest_title_id()
    seed_end = RAIL_X + width(hdr_seed, HEADER_SCALE)
    target_start = HEADER_TARGET_RIGHT - width(hdr_target, ROW_SCALE)
    cases.append(("5: the header's two readouts clear each other (%d px apart)"
                  % (target_start - seed_end), seed_end < target_start and
                  HEADER_TARGET_RIGHT <= SCREEN_W))
    cases.append(("5: the wizard footer line fits the screen (%d px of %d)"
                  % (width(WIZARD_FOOTER_LINE, ROW_SCALE), SCREEN_W),
                  width(WIZARD_FOOTER_LINE, ROW_SCALE) <= SCREEN_W))

    # The readiness summary FINISH shows in the help pane: three lines, a
    # blank, then FINISH's own help. Each summary line is wrapped like any
    # other, so the case is that none of them NEEDS wrapping and the block as a
    # whole still fits the 11 body lines the pane draws.
    digit = max("0123456789", key=lambda c: ADV[ROW_SCALE][c])
    toggles = sum(1 for e in entries if e["kind"] == "Toggle")
    summary = ["SEED  " + digit * SEED_DIGITS,
               "TARGET  " + widest_title_id(),
               "%s OF %d SETTINGS ENABLED" % (digit * len(str(toggles)), toggles)]
    over = [s for s in summary if len(wrap(s, ROW_SCALE, HELP_W)) > 1]
    summary_total = (len(summary) + SUMMARY_BLANK_LINES +
                     len(wrap(rail_help["kFinishHelp"], ROW_SCALE, HELP_W)))
    cases.append(("5: each readiness line fits %d px on one line (widest %d px)"
                  % (HELP_W, max(width(s, ROW_SCALE) for s in summary)), not over))
    cases.append(("5: the readiness block is %d of the %d body lines drawn"
                  % (summary_total, HELP_BODY_MAX_LINES),
                  summary_total <= HELP_BODY_MAX_LINES))

    # Confirm: the seed row, then every setting in category order, at its
    # widest value. Centred, so the budget is the whole screen.
    confirm = ["SEED   " + digit * SEED_DIGITS]
    confirm += [e["label"] + CONFIRM_GAP + widest_value(e) for e in entries]
    widest_confirm = max(confirm, key=lambda s: width(s, CONFIRM_SCALE))
    cases.append(("6: Confirm lists %d rows - the seed row and all %d settings"
                  % (len(confirm), len(entries)),
                  len(confirm) == len(entries) + 1 and
                  ui_scroll_count("Wizard Confirm") == len(confirm)))
    cases.append(("5: every Confirm row fits the screen at scale 4 (widest %d px, %s)"
                  % (width(widest_confirm, CONFIRM_SCALE), widest_confirm),
                  width(widest_confirm, CONFIRM_SCALE) <= SCREEN_W))

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
