#!/usr/bin/env python3
"""Generate the baked armour tables: ArmourTables.h (feature 040).

FOUR TABLES IN ONE HEADER, one per armour slot, because a player wears four
pieces and each slot is its own picker with its own config line. The four counts
sum to 124 and that total is asserted in the generated header itself.

THE SET IS DERIVED, NEVER LISTED. No armour id is hardcoded anywhere in this
file. A row belongs in slot s's picker when all three of these hold against a
vanilla dvdroot tree:

  1. It is an EquipParamProtector.param row whose protectorCategory (offset 210,
     u8) is s, for s in 0..3 - head, body, gloves, legwear, the game's own
     order. Category 4 is the 21 NOT NEEDED placeholders and is excluded by THIS
     test, not by a name filter: filtering on the name would also throw away any
     real piece the game ever left unnamed and would hide the placeholders
     behind a string comparison instead of the field that identifies them.
  2. The armour-name FMG holds a string for THAT EXACT ID which is non-empty,
     not the placeholder "*", does not match ^en\\d+$ (the untranslated debug
     rows), and is not "NOT NEEDED".
  3. equipModelGender (offset 209, u8) is 3 - wearable by both sexes. The two
     rows that are not are Crown of Illusions (250000) and Enlarged Head
     (430000), and they are the entire reason this test exists: both are named
     and both are obtainable, so nothing else excludes them.

That yields 33 / 34 / 26 / 31 = 124, and the generator asserts each count and
the total. A different number means the derivation is wrong, and no adjustment
to a threshold or a list fixes it.

OBTAINABILITY IS A TWO-SIDED CROSS-CHECK HERE, NOT A MEMBERSHIP GATE. On the
right hand (gen_weapon_table.py) the name test and the obtainability test agree
on every candidate and the generator treats a disagreement as proof it is wrong.
On the left hand (gen_left_hand_table.py) they disagree in one direction and
obtainability governs. On armour they disagree in BOTH directions, for two
reasons that are both understood, so neither shape fits and the check is stated
as an exact two-sided equality instead:

  included - obtainable == {230000, 231000, 232000, 233000}
      the four starting garments. A new character is already wearing them, so
      no shop sells them and no item lot drops them.
  obtainable - included == {250000, 430000}
      Crown of Illusions and Enlarged Head - obtainable, named, and wearable by
      neither sex, so test 3 keeps them out of every picker.

Either set coming out differently is a stop condition, which is why this is an
equality on two named sets rather than a count or a subset test.

LOT CATEGORY 1 IS ARMOUR. ItemLotParam's category 0 is weapons and 1 is armour -
measured, and the opposite way round from what a first reading of
gen_weapon_table.py's constant name suggests. The obtainability half of this
file therefore uses 1 where gen_weapon_table.py uses 0, deliberately, and the
two-sided equality above is what proves the choice: with category 0 the first
set holds 80 ids and the second holds {1500}.

NAMES COME FROM THE GAME, uppercased, read at each exact id, never composed. An
outfit prefix rule would be worse here than on the weapons: the game gives two
different body rows the name HUNTER GARB and two others STUDENT UNIFORM, and
those duplicates are shipped as duplicates (plan 040 D1) rather than being
disambiguated with an invented word. The generator asserts the duplicate set is
EXACTLY those two names, twice each, in the body slot - a third is a stop
condition.

THREE NAMES CONTAIN A SLASH - KNIGHT'S TROUSERS/DRESS, BLACK CHURCH
TROUSERS/DRESS and WHITE CHURCH TROUSERS/DRESS. '/' was added to Font8x8.cpp and
to gen_pool_table.py's RENDERABLE rather than the game's own strings being
sanitised, and the renderability check below is what would have caught it.

EACH ORDER IS LOAD-BEARING, AND THERE ARE FOUR OF THEM. defaults.cfg stores each
selection as one character per table row (see ModelPoolSelection.h), so
regenerating a table in a different order silently remaps a saved selection onto
a different piece - and on armour that is a piece of the same slot, which is
harder to notice than the wrong creature. Order per slot is FROZEN: uppercased
name, then ascending id. The id tie-break is what makes each duplicate-name pair
adjacent, so a player sees two identical rows next to each other rather than one
apparently hiding the other. If an order ever has to change, change that slot's
config key name at the same time.

Usage:
    python gen_armour_table.py <vanilla_dvdroot> [--check]

--check exits non-zero if the file on disk differs from what would be
generated, without writing anything - for starting_gear_verify.py.
"""

import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from fmg import armour_names  # noqa: E402
from gen_pool_table import RENDERABLE  # noqa: E402
from param_offsets import load_param, param_rows  # noqa: E402

SRC = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "src", "Randomizer")
HEADER = "ArmourTables.h"

PROTECTOR_PARAM = "EquipParamProtector.param"
SHOP_PARAM = "ShopLineupParam.param"
LOT_PARAM = "ItemLotParam.param"

# EQUIP_PARAM_PROTECTOR_ST, from tools/param_offsets.py against the real
# paramdef. Both are u8 and both are ordinary fields, not packed bits.
PROTECTOR_CATEGORY = 210
EQUIP_MODEL_GENDER = 209

# equipModelGender 3 is "both sexes". Only two rows in the four wearable
# categories are anything else, and they are the two the cross-check names.
GENDER_BOTH = 3

# protectorCategory 4 is the 21 NOT NEEDED placeholder rows. Named as a
# constant so the exclusion reads as what it is - a category - rather than as an
# off-by-one in the slot loop.
CATEGORY_PLACEHOLDER = 4

# ShopLineupParam: equipId at 0, equipType at 23. equipType 1 is "armour"
# (gen_weapon_table.py uses 0 for weapons in the same field).
SHOP_EQUIP_ID = 0
SHOP_EQUIP_TYPE = 23
SHOP_ROW_TYPE_ARMOUR = 1

# ItemLotParam: eight (id, category) pairs. Category 1 is ARMOUR - see the
# module docstring; this is the one constant most likely to be copied wrong.
LOT_ITEM_ID = 0
LOT_ITEM_CATEGORY = 32
LOT_SLOTS = 8
LOT_CATEGORY_ARMOUR = 1

PLACEHOLDER_NAME = "*"
NOT_NEEDED_NAME = "NOT NEEDED"
UNTRANSLATED = re.compile(r"^en\d+$")

# The four slots, in the game's own protectorCategory and field order: head,
# body, gloves, legwear. This order is used for the table declarations, the
# config keys, the UI rows and the four draws alike (plan 040 P5), and it is
# also the order of equip_Helm / equip_Armer / equip_Gaunt / equip_Leg in
# CharaInitParam - so the fifth column is the count each table must come out at.
#
# accessor, count constant, comment word, expected count
SLOTS = [
    (0, "HeadwearTable",   "kHeadwearCount",   "headwear", 33),
    (1, "BodyArmourTable", "kBodyArmourCount", "body armour", 34),
    (2, "GlovesTable",     "kGlovesCount",     "gloves", 26),
    (3, "LegwearTable",    "kLegwearCount",    "legwear", 31),
]
TOTAL = 124

# The duplicate display names this table ships, exactly (plan 040 D1). A third
# one, or one outside the body slot, is a stop condition.
EXPECTED_DUPLICATES = {"HUNTER GARB", "STUDENT UNIFORM"}
DUPLICATE_SLOT = 1

# The two sides of the obtainability cross-check, as exact sets.
STARTING_GARMENTS = {230000, 231000, 232000, 233000}
UNWEARABLE_BY_BOTH = {250000, 430000}


# --- the three membership tests ---------------------------------------------

def is_named(name):
    """Test 2. A row the game's own text does not label cannot be offered: the
    picker would have nothing to draw and the inventory nothing to agree with."""
    n = (name or "").strip()
    if not n or n == PLACEHOLDER_NAME or n == NOT_NEEDED_NAME:
        return False
    return not UNTRANSLATED.match(n)


def candidates(root):
    """Test 1 alone: every protector row in a wearable slot, as
    {slot: [(id, offset), ...]}. Category 4's placeholders are absent."""
    p = load_param(root, PROTECTOR_PARAM)
    if p is None:
        raise SystemExit("%s not present in the vanilla tree" % PROTECTOR_PARAM)
    out = {s: [] for s, _a, _c, _w, _n in SLOTS}
    placeholders = 0
    for rid, off in param_rows(p):
        cat = p[off + PROTECTOR_CATEGORY]
        if cat == CATEGORY_PLACEHOLDER:
            placeholders += 1
            continue
        if cat not in out:
            continue
        out[cat].append((rid, off))
    return p, out, placeholders


def obtainable_ids(root, wanted):
    """The cross-check's own measurement: ids sold in a shop as ARMOUR, or
    present in an item-lot slot whose matching category says armour.

    Deliberately NOT imported from gen_weapon_table.py. That module's
    obtainable_ids() is bound to lotItemCategory 0 and shop equipType 0, which
    are the weapon values; parameterising it would put a second meaning on a
    function two shipped generators depend on for the first one."""
    wanted = set(wanted)
    out = set()

    shop = load_param(root, SHOP_PARAM)
    for _rid, off in param_rows(shop):
        if shop[off + SHOP_EQUIP_TYPE] != SHOP_ROW_TYPE_ARMOUR:
            continue
        equip_id = struct.unpack_from("<i", shop, off + SHOP_EQUIP_ID)[0]
        if equip_id in wanted:
            out.add(equip_id)

    lot = load_param(root, LOT_PARAM)
    for _rid, off in param_rows(lot):
        for s in range(LOT_SLOTS):
            cat = struct.unpack_from("<i", lot, off + LOT_ITEM_CATEGORY + 4 * s)[0]
            if cat != LOT_CATEGORY_ARMOUR:
                continue
            item = struct.unpack_from("<i", lot, off + LOT_ITEM_ID + 4 * s)[0]
            if item in wanted:
                out.add(item)
    return out


def derive(root):
    """The whole derivation. Returns a dict the generator and the verifier both
    read, so there is exactly one implementation of the membership rules."""
    p, cands, placeholders = candidates(root)
    names = armour_names(root)

    rows = {}
    excluded = {}
    for slot, _accessor, _const, _word, expect in SLOTS:
        keep, drop = [], []
        for rid, off in cands[slot]:
            raw = names.get(rid, "")
            gender = p[off + EQUIP_MODEL_GENDER]
            if not is_named(raw):
                drop.append((rid, "unnamed in the game's own text"))
                continue
            if gender != GENDER_BOTH:
                drop.append((rid, "equipModelGender %d, not %d" % (gender, GENDER_BOTH)))
                continue
            keep.append((rid, raw.upper()))
        # FROZEN ORDER: uppercased name, then ascending id. The id tie-break is
        # what puts each duplicate-name pair adjacent.
        keep.sort(key=lambda r: (r[1], r[0]))
        if len(keep) != expect:
            raise SystemExit(
                "slot %d derived %d rows, expected %d.\n"
                "The three membership tests are the derivation - a different "
                "count means one of them is wrong, and no adjustment to a "
                "threshold or a list fixes it." % (slot, len(keep), expect))
        rows[slot] = keep
        excluded[slot] = drop

    total = sum(len(rows[s]) for s, _a, _c, _w, _n in SLOTS)
    if total != TOTAL:
        raise SystemExit("the four slots sum to %d, not %d" % (total, TOTAL))

    # --- the two-sided cross-check ----------------------------------------
    included = {rid for slot in rows for rid, _n in rows[slot]}
    all_candidates = {rid for slot in cands for rid, _o in cands[slot]}
    obtainable = obtainable_ids(root, all_candidates)

    only_included = included - obtainable
    only_obtainable = obtainable - included
    if only_included != STARTING_GARMENTS or only_obtainable != UNWEARABLE_BY_BOTH:
        raise SystemExit(
            "the obtainability cross-check no longer holds:\n"
            "  included but not obtainable: %s (expected %s)\n"
            "  obtainable but not included: %s (expected %s)\n"
            "Both are exact sets with known reasons - the four starting "
            "garments need no shop, and the two head pieces are wearable by "
            "neither sex. A difference means the data or a rule changed; "
            "re-measure rather than widening a set."
            % (sorted(only_included), sorted(STARTING_GARMENTS),
               sorted(only_obtainable), sorted(UNWEARABLE_BY_BOTH)))

    # --- the duplicate display names, asserted exactly --------------------
    dups = {}
    for slot, _a, _c, _w, _n in SLOTS:
        seen = {}
        for _rid, name in rows[slot]:
            seen[name] = seen.get(name, 0) + 1
        for name, n in seen.items():
            if n > 1:
                dups[(slot, name)] = n
    expected = {(DUPLICATE_SLOT, name): 2 for name in EXPECTED_DUPLICATES}
    if dups != expected:
        raise SystemExit(
            "the duplicate display names are %s, expected %s.\n"
            "Plan 040 D1 ships exactly two duplicate pairs, both in the body "
            "slot, under the names the game gives them. A third pair, or one "
            "in another slot, is a stop condition - it is not to be resolved "
            "by inventing a label." % (sorted(dups), sorted(expected)))

    return {
        "rows": rows,
        "excluded": excluded,
        "candidates": cands,
        "included": included,
        "obtainable": obtainable,
        "names": names,
        "protector": p,
        "placeholders": placeholders,
    }


def check_renderable(rows):
    for slot in sorted(rows):
        for rid, name in rows[slot]:
            bad = sorted({c for c in name if c not in RENDERABLE})
            if bad:
                raise SystemExit(
                    "%d: name %r contains characters the 8x8 font cannot "
                    "render: %r\nAdd the glyphs to Font8x8.cpp rather than "
                    "sanitizing the game's own text - a renamed row stops "
                    "matching the inventory." % (rid, name, bad))


# --- emit ------------------------------------------------------------------

def build(root):
    d = derive(root)
    rows = d["rows"]
    check_renderable(rows)

    L = []
    L.append("// %s - GENERATED by tools/gen_armour_table.py." % HEADER)
    L.append("// Do not edit by hand; regenerate and re-run starting_gear_verify.py.")
    L.append("//")
    L.append("// Every armour piece a player can be granted at character creation -")
    L.append("// feature 040, the four START WITH ... settings. FOUR TABLES, one per")
    L.append("// slot, in the game's own protectorCategory order: head, body, gloves,")
    L.append("// legwear, %d / %d / %d / %d rows summing to %d."
             % (len(rows[0]), len(rows[1]), len(rows[2]), len(rows[3]), TOTAL))
    L.append("//")
    L.append("// The set is DERIVED, not listed: an EquipParamProtector row in that")
    L.append("// slot's category (offset %d), named by the game's own text, and"
             % PROTECTOR_CATEGORY)
    L.append("// wearable by both sexes (equipModelGender %d at offset %d). The %d"
             % (GENDER_BOTH, EQUIP_MODEL_GENDER, d["placeholders"]))
    L.append("// NOT NEEDED placeholders of category %d are absent, so are the unnamed"
             % CATEGORY_PLACEHOLDER)
    L.append("// rows, and so are the Crown of Illusions and the Enlarged Head, which")
    L.append("// the game marks wearable by neither sex.")
    L.append("//")
    L.append("// EACH ORDER IS LOAD-BEARING, AND THERE ARE FOUR OF THEM:")
    L.append("// RandomizerDefaults stores each picker's selection as one character per")
    L.append("// row, positionally. Sorted by uppercased name, then ascending id - the")
    L.append("// id tie-break is what keeps each duplicate-name pair adjacent. A")
    L.append("// regenerated table in a different order silently remaps a saved")
    L.append("// selection onto a different piece of the same slot.")
    L.append("//")
    L.append("// TWO NAMES APPEAR TWICE EACH in the body table - HUNTER GARB and")
    L.append("// STUDENT UNIFORM - because the game gives two different rows each of")
    L.append("// those names. They are shipped as the game spells them rather than")
    L.append("// disambiguated with an invented word (plan 040 D1), and the generator")
    L.append("// asserts the duplicate set is exactly those two.")
    L.append("//")
    L.append("// `model` holds the armour id in DECIMAL, as a string, because the picker")
    L.append("// keys and logs on that field; the engine converts it with strtol. There")
    L.append("// is deliberately no second array of ids to fall out of step with this.")
    L.append("// `poolEntries` is 1 for every row: each row is exactly one piece.")
    L.append("//")
    L.append("// Names are FROM THE GAME'S OWN TEXT, uppercased, read at each exact id -")
    L.append("// never composed from an outfit prefix.")
    L.append("#pragma once")
    L.append("")
    L.append("#include <array>")
    L.append("")
    L.append('#include "ModelPoolSelection.h"')
    L.append("")
    L.append("namespace bbr {")

    for slot, accessor, const, word, _expect in SLOTS:
        table = rows[slot]
        width = max(len(n) for _r, n in table)
        id_width = max(len(str(r)) for r, _n in table)
        L.append("")
        L.append("// The %d %s pieces - protectorCategory %d, equip_%s's field."
                 % (len(table), word, slot,
                    ("Helm", "Armer", "Gaunt", "Leg")[slot]))
        L.append("// Kept as a function-local static for the same reason as the pool")
        L.append("// tables: header-only, no translation-unit duplication.")
        L.append("inline const std::array<ModelPoolEntry, %d>& %s() {"
                 % (len(table), accessor))
        L.append("    static const std::array<ModelPoolEntry, %d> kTable = {{"
                 % len(table))
        for rid, name in table:
            L.append('        { %-*s %-*s %d },'
                     % (id_width + 4, '"%d",' % rid, width + 3, '"%s",' % name, 1))
        L.append("    }};")
        L.append("    return kTable;")
        L.append("}")
        L.append("")
        L.append("const int %s = %d;" % (const, len(table)))

    L.append("")
    L.append("// The wearable set is %d pieces and the four tables ARE that set - no" % TOTAL)
    L.append("// piece is in two slots and none is missing. Spelled as a compile-time")
    L.append("// assertion rather than left to the generator, so a hand-edited header")
    L.append("// cannot quietly disagree with the derivation.")
    L.append("static_assert(%s == %d,"
             % (" + ".join(c for _s, _a, c, _w, _n in SLOTS), TOTAL))
    L.append('              "the four armour tables must cover all %d wearable pieces");'
             % TOTAL)
    L.append("")
    L.append("} // namespace bbr")
    L.append("")
    return "\n".join(L)


def run(root, check):
    text = build(root)
    out = os.path.join(SRC, HEADER)
    if check:
        if not os.path.exists(out):
            print("FAIL: %s does not exist" % out)
            return 1
        with open(out, encoding="utf-8", newline="") as f:
            on_disk = f.read()
        if on_disk.replace("\r\n", "\n") != text:
            print("FAIL: %s is stale - regenerate it" % HEADER)
            return 1
        print("PASS: %s matches the vanilla tree" % HEADER)
        return 0
    with open(out, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
    # Deliberately does not print any armour name: this console is cp1252.
    print("wrote %s (%d rows)" % (HEADER, text.count("        { ")))
    return 0


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    check = "--check" in sys.argv[1:]
    if len(args) < 1:
        print(__doc__)
        return 2
    return run(args[0], check)


if __name__ == "__main__":
    sys.exit(main())
