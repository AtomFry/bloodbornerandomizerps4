#!/usr/bin/env python3
"""Generate the baked trick-weapon table: TrickWeaponTable.h (feature 037).

THE SET IS DERIVED, NEVER LISTED. No weapon id is hardcoded anywhere in this
file. A row belongs in the picker when all four of these hold against a vanilla
dvdroot tree:

  1. It is an EquipParamWeapon.param row at upgrade tier 0, (id/100)%100 == 0.
  2. Byte 256 bit 0 of that row - rightHandEquipable - is set. The hand bits are
     PACKED BITS in one byte, not fields: reading byte 256 as a value is wrong,
     and writing it at all would corrupt three neighbours. weaponCategory looks
     like the melee/firearm field and is not (value 0 covers both the Hunter
     Blunderbuss and the Threaded Cane).
  3. The weapon-name FMG holds a string for THAT EXACT ID which is neither empty
     nor the placeholder "*".
  4. The id is obtainable in vanilla: sold in a ShopLineupParam.param row with
     equipType == 0, or present in an ItemLotParam.param slot whose matching
     lotItemCategory is 1.

Tests 3 and 4 are independent measurements of the same question - "is this a
weapon a player can actually get" - and on the shipped data they agree on every
one of the 85 candidates. The generator ASSERTS that agreement and refuses to
write if it ever breaks, so neither test is trusted on its own. That is what
makes the seven exclusions a finding rather than a guess; see
docs/features/037-start-with-trick-weapon/plan-evidence.md M3.

NAMES COME FROM THE GAME. Each row's display name is the uppercased FMG string
for that exact id. They are never composed from a prefix: the game spells four
of the 26 irregularly - "Ludwig's Uncanny Holy Blade", "Logarius' Uncanny
Wheel", "Uncanny Bowblade", "Uncanny Parasite" - so any prefix rule produces a
name the inventory disagrees with.

THE ORDER IS LOAD-BEARING. defaults.cfg stores the selection as one character
per table row (see ModelPoolSelection.h), so regenerating this table in a
different order silently remaps a saved selection onto the wrong VERSION of a
weapon - harder to notice than the wrong creature. Order is frozen: by the
weapon's base (version-0) name alphabetically, then version 0, 1, 2, so a
weapon's three rows are adjacent. If the order ever has to change, change the
config key name at the same time.

Usage:
    python gen_weapon_table.py <vanilla_dvdroot> [--check]

--check exits non-zero if the file on disk differs from what would be
generated, without writing anything - for trick_weapons_verify.py.
"""

import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from fmg import weapon_names  # noqa: E402
from gen_pool_table import RENDERABLE  # noqa: E402
from param_offsets import (BIT_LIMIT, BIT_TYPES, VALUE_SIZE,  # noqa: E402
                           load_defs, load_param, param_rows)

SRC = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "src", "Randomizer")
HEADER = "TrickWeaponTable.h"

WEAPON_PARAM = "EquipParamWeapon.param"
SHOP_PARAM = "ShopLineupParam.param"
LOT_PARAM = "ItemLotParam.param"

# ShopLineupParam: equipId at 0, equipType at 23. equipType 0 is "weapon".
SHOP_EQUIP_ID = 0
SHOP_EQUIP_TYPE = 23
SHOP_ROW_TYPE_WEAPON = 0

# ItemLotParam: eight (id, category) pairs. Category 1 is "weapon".
LOT_ITEM_ID = 0
LOT_ITEM_CATEGORY = 32
LOT_SLOTS = 8
LOT_CATEGORY_WEAPON = 1

PLACEHOLDER_NAME = "*"


# --- the weapon id decomposition ------------------------------------------
# Every EquipParamWeapon id is a multiple of 100 and decomposes exactly. `tier`
# must be % 100, not % 10: tier 10 is id + 1000, which %10 would read as version
# 0 tier 0 and collide with the base row. Getting the divisors wrong is how the
# backlog row arrived at 37 weapons instead of 47.

def decompose(wid):
    return {
        "family": wid // 1000000,
        "weapon": (wid // 100000) % 10,
        "version": (wid // 10000) % 10,
        "tier": (wid // 100) % 100,
    }


def base_id(wid):
    """The version-0, tier-0 row of the same weapon."""
    d = decompose(wid)
    return d["family"] * 1000000 + d["weapon"] * 100000


# --- packed bit fields -----------------------------------------------------

def bit_position(defn, field_name):
    """(byte offset, bit shift) of a packed bit field.

    param_offsets.field_offsets() reports width 0 for a bit field and does not
    give the shift, so this replays its own bit_offset accumulator. Kept here
    rather than in param_offsets.py because it is this feature that needed it.
    """
    pos = 0
    bit_offset = -1
    bit_type = "u8"
    for f in defn["fields"]:
        t = f["type"]
        is_bit = t in BIT_TYPES and f["bit_size"] != -1
        if not is_bit:
            bit_offset = -1
            if t == "fixstr":
                pos += f["array_len"]
            elif t == "fixstrW":
                pos += f["array_len"] * 2
            elif t == "dummy8":
                pos += f["array_len"]
            else:
                pos += VALUE_SIZE.get(t, 4)
            continue
        new_bit_type = "u8" if t == "dummy8" else t
        limit = BIT_LIMIT[new_bit_type]
        if (bit_offset == -1 or new_bit_type != bit_type
                or bit_offset + f["bit_size"] > limit):
            bit_offset = 0
            bit_type = new_bit_type
            start = pos
            pos += VALUE_SIZE[new_bit_type]
        else:
            start = pos - VALUE_SIZE[bit_type]
        if f["name"] == field_name:
            return start, bit_offset
        bit_offset += f["bit_size"]
    raise SystemExit("no packed bit field named %r" % field_name)


# --- the four tests --------------------------------------------------------

def right_hand_tier0(root):
    """Test 1 and test 2: every tier-0 row whose rightHandEquipable bit is set.
    Returns a sorted list of ids - the candidate set, 85 rows as shipped."""
    defs = load_defs(root)
    byte_off, shift = bit_position(defs["EQUIP_PARAM_WEAPON_ST"], "rightHandEquipable")
    p = load_param(root, WEAPON_PARAM)
    out = []
    for wid, off in param_rows(p):
        if decompose(wid)["tier"] != 0:
            continue
        if p[off + byte_off] & (1 << shift):
            out.append(wid)
    return sorted(out), (byte_off, shift)


def named_ids(root, candidates):
    """Test 3: candidates the game's own text names, excluding the "*"
    placeholder and the ids with no string at all."""
    names = weapon_names(root)
    out = set()
    for wid in candidates:
        n = names.get(wid, "").strip()
        if n and n != PLACEHOLDER_NAME:
            out.add(wid)
    return out, names


def obtainable_ids(root, candidates):
    """Test 4: candidates sold in a shop as a weapon, or dropped by an item lot
    in a slot whose category says weapon."""
    wanted = set(candidates)
    out = set()

    shop = load_param(root, SHOP_PARAM)
    for _rid, off in param_rows(shop):
        if shop[off + SHOP_EQUIP_TYPE] != SHOP_ROW_TYPE_WEAPON:
            continue
        equip_id = struct.unpack_from("<i", shop, off + SHOP_EQUIP_ID)[0]
        if equip_id in wanted:
            out.add(equip_id)

    lot = load_param(root, LOT_PARAM)
    for _rid, off in param_rows(lot):
        for s in range(LOT_SLOTS):
            cat = struct.unpack_from("<i", lot, off + LOT_ITEM_CATEGORY + 4 * s)[0]
            if cat != LOT_CATEGORY_WEAPON:
                continue
            item = struct.unpack_from("<i", lot, off + LOT_ITEM_ID + 4 * s)[0]
            if item in wanted:
                out.add(item)
    return out


def derive(root):
    """The whole derivation. Returns a dict the generator and the verifier both
    read, so there is exactly one implementation of the membership rules."""
    candidates, hand_bit = right_hand_tier0(root)
    named, names = named_ids(root, candidates)
    obtainable = obtainable_ids(root, candidates)

    named_only = sorted(named - obtainable)
    obtainable_only = sorted(obtainable - named)
    if named_only or obtainable_only:
        raise SystemExit(
            "the two membership tests disagree, so the derivation is wrong:\n"
            "  named but not obtainable: %s\n"
            "  obtainable but not named: %s\n"
            "No adjustment to a threshold or a list fixes this - re-measure."
            % (named_only, obtainable_only))

    included = sorted(named)
    excluded = [w for w in candidates if w not in named]

    # Order: base-weapon name alphabetically, then version 0, 1, 2. FROZEN.
    def base_name(wid):
        return names.get(base_id(wid), "").upper()

    rows = []
    for wid in sorted(included, key=lambda w: (base_name(w), decompose(w)["version"])):
        rows.append((wid, names[wid].upper()))

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
    L.append("// %s - GENERATED by tools/gen_weapon_table.py." % HEADER)
    L.append("// Do not edit by hand; regenerate and re-run trick_weapons_verify.py.")
    L.append("//")
    L.append("// Every right-hand trick-weapon VERSION a player can be granted at")
    L.append("// character creation - feature 037, START WITH A TRICK WEAPON. The 26")
    L.append("// trick weapons, each three times: the plain weapon, its Uncanny version")
    L.append("// (version 1) and its Lost version (version 2).")
    L.append("//")
    L.append("// The set is DERIVED, not listed: a tier-0 EquipParamWeapon row with the")
    L.append("// rightHandEquipable bit set (byte %d bit %d), named by the game's own"
             % (byte_off, shift))
    L.append("// text, and obtainable in vanilla from a shop or an item lot. Firearms,")
    L.append("// shields and torches are left-hand items and are absent; so are Bare")
    L.append("// Fists and the seven unnamed, unobtainable right-hand rows. See the")
    L.append("// generator's header comment for why two independent tests, not one.")
    L.append("//")
    L.append("// ORDER IS LOAD-BEARING: RandomizerDefaults stores the picker's selection")
    L.append("// as one character per row, positionally. Sorted by the weapon's base")
    L.append("// (version-0) name, then version 0/1/2, so a weapon's three rows are")
    L.append("// adjacent. A regenerated table in a different order silently remaps a")
    L.append("// saved selection onto the wrong VERSION of a weapon.")
    L.append("//")
    L.append("// `model` holds the weapon id in DECIMAL, as a string, because the picker")
    L.append("// keys and logs on that field; the engine converts it with strtol. There")
    L.append("// is deliberately no second array of ids to fall out of step with this.")
    L.append("// `poolEntries` is 1 for every row: each row is exactly one weapon.")
    L.append("//")
    L.append("// Names are FROM THE GAME'S OWN TEXT, uppercased, read at each exact id -")
    L.append("// never composed from a prefix. Four of the 26 are spelled irregularly.")
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
    L.append("inline const std::array<ModelPoolEntry, %d>& TrickWeaponTable() {" % len(rows))
    L.append("    static const std::array<ModelPoolEntry, %d> kTable = {{" % len(rows))
    for wid, name in rows:
        L.append('        { %-*s %-*s %d },'
                 % (id_width + 4, '"%d",' % wid, width + 3, '"%s",' % name, 1))
    L.append("    }};")
    L.append("    return kTable;")
    L.append("}")
    L.append("")
    L.append("const int kTrickWeaponCount = %d;" % len(rows))
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
