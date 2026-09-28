#!/usr/bin/env python3
"""Generate the baked left-hand-weapon table: LeftHandWeaponTable.h (feature 038).

FEATURE 038 IS FEATURE 037 FOR THE OTHER HAND, and this file is deliberately
BESIDE gen_weapon_table.py rather than inside it. What is shared is imported from
there and used unchanged: the id decomposition, the packed-bit locator, the
tier-0 hand-bit filter, the name test, the obtainability test and the font
renderability set. What is NOT shared is the one rule that differs, and it is the
reason for the separate file:

  ON THE RIGHT HAND THE TWO MEMBERSHIP TESTS AGREE. ON THE LEFT THEY DO NOT.

gen_weapon_table.py's derive() treats a disagreement between "the game's text
names it" and "a shop or an item lot can give it to you" as proof that the
derivation itself is wrong, and refuses to write. That is correct for the right
hand, where all 78 included rows pass both tests and all seven excluded rows fail
both. On the left hand, measured against the shipped data:

  18 tier-0 rows have leftHandEquipable set
  16 of them are named by the game's own text
  14 of them are obtainable from a shop or an item lot
  2 - Wooden Shield (19000000) and Torch (20100000) - are NAMED BUT UNOBTAINABLE

So here the two tests are not two measurements of one question; they are a
conjunction. OBTAINABILITY IS THE GOVERNING RULE and the name test is the second
condition, not a cross-check: a row belongs in the picker only when it passes
both. Parameterising derive() on the hand would have meant making its
disagreement assertion conditional - weakening the guard that feature 037's stop
conditions are built on - so the left-hand rule lives here instead, stated once
and asserted in its own direction.

A row belongs in the picker when all four of these hold against a vanilla
dvdroot tree:

  1. It is an EquipParamWeapon.param row at upgrade tier 0, (id/100)%100 == 0.
  2. Byte 256 bit 1 of that row - leftHandEquipable - is set. The hand bits are
     PACKED BITS in one byte, not fields: reading byte 256 as a value is wrong,
     and writing it at all would corrupt three neighbours. bit 0 is
     rightHandEquipable, which is feature 037's; bit 2 is bothHandEquipable and
     is not consulted by either feature.
  3. The weapon-name FMG holds a string for THAT EXACT ID which is neither empty
     nor the placeholder "*".
  4. The id is obtainable in vanilla: sold in a ShopLineupParam.param row with
     equipType == 0, or present in an ItemLotParam.param slot whose matching
     lotItemCategory is 1.

What the 14 are: 11 firearms (Hunter Blunderbuss, Ludwig's Rifle, Hunter Pistol,
Evelyn, Repeating Pistol, Cannon, Rosmarinus, Flamesprayer, Gatling Gun, Church
Cannon, Piercing Rifle) plus the Loch Shield, the Hunter's Torch and the Fist of
Gratia. All fourteen are in scope by the developer's decision; none is
hand-excluded, and the membership rule is what decides, never a list.

NO VERSION DIMENSION. Unlike the right hand, no left-hand weapon has an Uncanny
or a Lost form: every one of the 14 is version 0, and the only other versions any
left-hand family carries are the param's placeholder version 9 and the two
nameless version-8 rows. So this is a flat 14-row table and the order rule has
nothing to break a tie on. The generator ASSERTS the flatness rather than
assuming it - a version-1 firearm appearing would change the row count, which is
a stop condition, not something to absorb.

NAMES COME FROM THE GAME, uppercased, read at each exact id. Nothing is composed.

THE ORDER IS LOAD-BEARING. defaults.cfg stores the selection as one character per
table row (see ModelPoolSelection.h), so regenerating this table in a different
order silently remaps a saved selection onto the wrong weapon. Order is frozen,
and it is the SAME RULE feature 037 uses: by the weapon's base (version-0) name
alphabetically, then version. With every row at version 0 that is simply
alphabetical by name, but the rule is written the same way so the two tables
cannot drift apart. If the order ever has to change, change the config key name
at the same time.

Usage:
    python gen_left_hand_table.py <vanilla_dvdroot> [--check]

--check exits non-zero if the file on disk differs from what would be
generated, without writing anything - for left_hand_weapons_verify.py.
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gen_pool_table import RENDERABLE  # noqa: E402
from gen_weapon_table import (base_id, decompose, named_ids,  # noqa: E402
                              obtainable_ids, tier0_with_hand_bit)

SRC = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "src", "Randomizer")
HEADER = "LeftHandWeaponTable.h"

HAND_BIT_FIELD = "leftHandEquipable"


def left_hand_tier0(root):
    """Tests 1 and 2: every tier-0 row whose leftHandEquipable bit is set.
    18 rows as shipped. The filter itself is feature 037's, imported."""
    return tier0_with_hand_bit(root, HAND_BIT_FIELD)


def derive(root):
    """The whole derivation. Returns a dict the generator and the verifier both
    read, so there is exactly one implementation of the membership rules."""
    candidates, hand_bit = left_hand_tier0(root)
    named, names = named_ids(root, candidates)
    obtainable = obtainable_ids(root, candidates)

    # The one direction that MUST hold: nothing may be obtainable in vanilla and
    # yet have no name in the game's own text. A row a player can be handed but
    # which the inventory cannot label would mean the name source and the
    # obtainability source are describing different data, and no adjustment to a
    # list fixes that. The other direction is expected and is the finding this
    # file exists for - see the module docstring.
    obtainable_only = sorted(obtainable - named)
    if obtainable_only:
        raise SystemExit(
            "obtainable but unnamed left-hand rows: %s\n"
            "A row a player can get but the game's text cannot name means the two "
            "sources disagree about what the data IS - re-measure rather than "
            "adjusting a rule." % obtainable_only)

    included = sorted(named & obtainable)
    excluded = [w for w in candidates if w not in set(included)]

    versions = sorted({decompose(w)["version"] for w in included})
    if versions != [0]:
        raise SystemExit(
            "left-hand rows are no longer all version 0: %s\n"
            "This table is flat on purpose - no left-hand weapon has an Uncanny "
            "or Lost form. A version dimension appearing changes the row count, "
            "and therefore the config line's length, which is a stop condition."
            % versions)

    # Order: base-weapon name alphabetically, then version. FROZEN, and the same
    # rule gen_weapon_table.py freezes so the two tables cannot drift apart.
    def base_name(wid):
        return names.get(base_id(wid), "").upper()

    rows = [(wid, names[wid].upper())
            for wid in sorted(included,
                              key=lambda w: (base_name(w), decompose(w)["version"]))]

    return {
        "rows": rows,
        "candidates": candidates,
        "included": included,
        "excluded": excluded,
        "named": named,
        "obtainable": obtainable,
        "names": names,
        "hand_bit": hand_bit,
    }


def check_renderable(rows):
    for wid, name in rows:
        bad = sorted({c for c in name if c not in RENDERABLE})
        if bad:
            raise SystemExit(
                "%d: name %r contains characters the 8x8 font cannot render: %r\n"
                "Add the glyphs to Font8x8.cpp rather than sanitizing the game's "
                "own text - a renamed row stops matching the inventory."
                % (wid, name, bad))


# --- emit ------------------------------------------------------------------

def build(root):
    d = derive(root)
    rows = d["rows"]
    check_renderable(rows)
    if len({n for _w, n in rows}) != len(rows):
        raise SystemExit("two rows share a display name; the picker would be ambiguous")
    width = max(len(n) for _w, n in rows)
    id_width = max(len(str(w)) for w, _n in rows)
    byte_off, shift = d["hand_bit"]

    L = []
    L.append("// %s - GENERATED by tools/gen_left_hand_table.py." % HEADER)
    L.append("// Do not edit by hand; regenerate and re-run left_hand_weapons_verify.py.")
    L.append("//")
    L.append("// Every left-hand weapon a player can be granted at character")
    L.append("// creation - feature 038, START WITH A LEFT-HAND WEAPON. Feature 037's")
    L.append("// table for the other hand: 11 firearms plus the Loch Shield, the")
    L.append("// Hunter's Torch and the Fist of Gratia.")
    L.append("//")
    L.append("// The set is DERIVED, not listed: a tier-0 EquipParamWeapon row with the")
    L.append("// leftHandEquipable bit set (byte %d bit %d), named by the game's own"
             % (byte_off, shift))
    L.append("// text, AND obtainable in vanilla from a shop or an item lot. Trick")
    L.append("// weapons are right-hand items and are absent; so are Bare Fists, the")
    L.append("// two unnamed rows, and the Wooden Shield and the Torch, which the")
    L.append("// game names but no shop and no item lot can give a player.")
    L.append("//")
    L.append("// UNLIKE FEATURE 037'S TABLE THIS ONE IS FLAT. No left-hand weapon has")
    L.append("// an Uncanny or a Lost form, so there is no version dimension and each")
    L.append("// weapon is exactly one row. The generator asserts that rather than")
    L.append("// assuming it.")
    L.append("//")
    L.append("// ORDER IS LOAD-BEARING: RandomizerDefaults stores the picker's selection")
    L.append("// as one character per row, positionally. Sorted by name, by the same")
    L.append("// frozen rule TrickWeaponTable.h uses. A regenerated table in a")
    L.append("// different order silently remaps a saved selection onto the wrong")
    L.append("// weapon.")
    L.append("//")
    L.append("// `model` holds the weapon id in DECIMAL, as a string, because the picker")
    L.append("// keys and logs on that field; the engine converts it with strtol. There")
    L.append("// is deliberately no second array of ids to fall out of step with this.")
    L.append("// `poolEntries` is 1 for every row: each row is exactly one weapon.")
    L.append("//")
    L.append("// Names are FROM THE GAME'S OWN TEXT, uppercased, read at each exact id -")
    L.append("// never composed.")
    L.append("#pragma once")
    L.append("")
    L.append("#include <array>")
    L.append("")
    L.append('#include "ModelPoolSelection.h"')
    L.append("")
    L.append("namespace bbr {")
    L.append("")
    L.append("// Kept as a function-local static for the same reason as the pool tables:")
    L.append("// header-only, no translation-unit duplication.")
    L.append("inline const std::array<ModelPoolEntry, %d>& LeftHandWeaponTable() {"
             % len(rows))
    L.append("    static const std::array<ModelPoolEntry, %d> kTable = {{" % len(rows))
    for wid, name in rows:
        L.append('        { %-*s %-*s %d },'
                 % (id_width + 4, '"%d",' % wid, width + 3, '"%s",' % name, 1))
    L.append("    }};")
    L.append("    return kTable;")
    L.append("}")
    L.append("")
    L.append("const int kLeftHandWeaponCount = %d;" % len(rows))
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
    # Deliberately does not print any weapon name: this console is cp1252.
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
