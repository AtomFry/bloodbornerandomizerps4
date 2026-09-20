#!/usr/bin/env python3
"""START WITH HUNTER TOOLS verification.

A PROPERTY VALIDATOR over the real item-data archive, in the same shape as
drops_verify.py: it never re-derives the C++'s answer, it checks invariants of
generated output against trusted vanilla input. Field offsets come from
tools/param_offsets.py against the real paramdef, not from the C++.

What the feature does: writes goods 4103 (Blood Gem Workshop Tool) and 4104
(Rune Workshop Tool) into the free starting-inventory slots of every
player-origin row in CharaInitParam, so a new character can fit gems and runes
from the first area. See app/src/Randomizer/HunterTools.h.

Invariants:
  H-I1  only CharaInitParam.param differs inside the archive
  H-I2  within CharaInitParam, ONLY the 22 origin rows differ
  H-I3  within an origin row, only item_* and itemNum_* differ - every other
        byte of the 300-byte row is intact
  H-I4  every origin row ends holding BOTH tools, exactly once each, count 1
  H-I5  the vanilla starting item (1x goods 100) is never displaced
  H-I6  the archive still holds the same members

This tool CANNOT prove the feature works in game. It proves the bytes land
where they were meant to. Whether Bloodborne unlocks gem fitting on possession
or on an event flag set by the vanilla chest is unknown and only a hardware
test can settle it - see HunterTools.h.

Usage:
    python hunter_tools_verify.py rows     <vanilla_dvdroot>
    python hunter_tools_verify.py verify   <vanilla_dvdroot> <output_dvdroot>
    python hunter_tools_verify.py selftest <vanilla_dvdroot>
"""

import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from boss_verify import read_dcx  # noqa: E402
from param_offsets import bnd4_members, param_rows  # noqa: E402

REL = os.path.join("param", "gameparam", "gameparam.parambnd.dcx")

CHARA_MEMBER = "CharaInitParam.param"
GOODS_MEMBER = "EquipParamGoods.param"
ITEMLOT_MEMBER = "ItemLotParam.param"

# CHARACTER_INIT_PARAM, 300-byte row. These MUST match HunterTools.cpp.
ITEM_ID_BASE = 124   # item_01,    s32, stride 4
ITEM_NUM_BASE = 204  # itemNum_01, u8,  stride 1
ITEM_SLOTS = 10
ROW_BYTES = 300
EMPTY = -1

BLOOD_GEM_TOOL = 4103
RUNE_TOOL = 4104
TOOLS = (BLOOD_GEM_TOOL, RUNE_TOOL)

# The lots those two goods reach the player through in vanilla, which is how
# the identification was made in the first place - checked here rather than
# trusted. Both are on TreasureRandomizer.cpp's protected list.
TOOL_LOTS = {2411000: BLOOD_GEM_TOOL, 2200360: RUNE_TOOL}

# ItemLotParam offsets, for the identification check only.
LOT_ID_BASE = 0
LOT_CAT_BASE = 32
LOT_CATEGORY_GOODS = 4

# The vanilla starting item every origin row carries, 1x.
VANILLA_START_ITEM = 100

# Every row HunterTools.cpp targets. Both ten-row blocks plus 3500/3501,
# because the data cannot say which block the game reads - see the comment on
# kOriginRows.
ORIGIN_ROWS = tuple(list(range(2000, 2010)) + list(range(3000, 3010)) + [3500, 3501])


def i32(buf, off):
    return struct.unpack_from("<i", buf, off)[0]


def set_i32(buf, off, v):
    buf[off:off + 4] = struct.pack("<i", v)


def load_archive(root):
    plain = bytearray(read_dcx(os.path.join(root, REL)))
    members = {name: (off, size) for name, off, size in bnd4_members(plain)}
    return plain, members


def member_rows(plain, members, name):
    """Yields (row_id, absolute_row_offset)."""
    off, size = members[name]
    member = plain[off:off + size]
    for rid, rel in param_rows(member):
        yield rid, off + rel


def row_map(plain, members, name):
    return {rid: at for rid, at in member_rows(plain, members, name)}


def slots(plain, at):
    """The row's ten (item_id, count) pairs."""
    return [(i32(plain, at + ITEM_ID_BASE + s * 4), plain[at + ITEM_NUM_BASE + s])
            for s in range(ITEM_SLOTS)]


def held(plain, at):
    """Item ids actually present in the row, in slot order."""
    return [v for v, _ in slots(plain, at) if v > 0]


# --- the identification: prove 4103/4104 really are the workshop tools -------

def check_identification(plain, members):
    """The tool ids are not asserted here, they are resolved from the two item
    lots the vanilla chests use. Returns (failures, notes)."""
    failures, notes = [], []

    lots = row_map(plain, members, ITEMLOT_MEMBER)
    for lot, expect in sorted(TOOL_LOTS.items()):
        if lot not in lots:
            failures.append("lot %d missing from %s" % (lot, ITEMLOT_MEMBER))
            continue
        at = lots[lot]
        got = i32(plain, at + LOT_ID_BASE)
        cat = i32(plain, at + LOT_CAT_BASE)
        if got != expect or cat != LOT_CATEGORY_GOODS:
            failures.append("lot %d yields id %d cat %d, expected id %d cat %d"
                            % (lot, got, cat, expect, LOT_CATEGORY_GOODS))
        else:
            notes.append("lot %d -> goods %d, confirmed" % (lot, got))

    goods = row_map(plain, members, GOODS_MEMBER)
    for t in TOOLS:
        if t not in goods:
            failures.append("goods row %d does not exist" % t)
    return failures, notes


# --- vanilla-side preconditions ---------------------------------------------

def check_vanilla(plain, members):
    """The feature assumes every origin row has room and a known shape. If that
    stops being true the C++ silently does less, so it is checked here."""
    failures = []
    rows = row_map(plain, members, CHARA_MEMBER)

    for rid in ORIGIN_ROWS:
        if rid not in rows:
            failures.append("origin row %d missing from vanilla %s" % (rid, CHARA_MEMBER))
            continue
        at = rows[rid]
        if at + ROW_BYTES > len(plain):
            failures.append("origin row %d runs past the archive" % rid)
            continue
        s = slots(plain, at)
        if s[0] != (VANILLA_START_ITEM, 1):
            failures.append("origin row %d slot 0 is %r, expected (%d, 1)"
                            % (rid, s[0], VANILLA_START_ITEM))
        free = [k for k in range(ITEM_SLOTS) if s[k][0] == EMPTY]
        if len(free) < len(TOOLS):
            failures.append("origin row %d has %d free slots, needs %d"
                            % (rid, len(free), len(TOOLS)))
        for t in TOOLS:
            if t in [v for v, _ in s]:
                failures.append("origin row %d already holds tool %d in vanilla" % (rid, t))
    return failures


# --- output comparison ------------------------------------------------------

def compare(van, van_m, out, out_m):
    failures, notes = [], []

    # H-I6 / H-I1
    if set(van_m) != set(out_m):
        failures.append("H-I6 archive membership changed")
        return failures, notes, 0
    for name in sorted(van_m):
        (vo, vs), (oo, osz) = van_m[name], out_m[name]
        if vs != osz:
            failures.append("H-I6 member %s changed size %d -> %d" % (name, vs, osz))
            continue
        if name == CHARA_MEMBER:
            continue
        if van[vo:vo + vs] != out[oo:oo + osz]:
            failures.append("H-I1 member %s differs but should not" % name)

    van_rows = row_map(van, van_m, CHARA_MEMBER)
    out_rows = row_map(out, out_m, CHARA_MEMBER)
    if set(van_rows) != set(out_rows):
        failures.append("H-I2 CharaInitParam row ids changed")
        return failures, notes, 0

    targets = set(ORIGIN_ROWS)
    changed = 0

    for rid in sorted(van_rows):
        va, oa = van_rows[rid], out_rows[rid]
        same = van[va:va + ROW_BYTES] == out[oa:oa + ROW_BYTES]

        if rid not in targets:
            if not same:
                failures.append("H-I2 non-origin row %d changed" % rid)
            continue

        if not same:
            changed += 1

        # H-I3: only the item arrays may differ. Rebuild the vanilla row with
        # the output's item fields patched in; anything left over is a stray
        # write somewhere else in the row.
        patched = bytearray(van[va:va + ROW_BYTES])
        for s in range(ITEM_SLOTS):
            set_i32(patched, ITEM_ID_BASE + s * 4, i32(out, oa + ITEM_ID_BASE + s * 4))
            patched[ITEM_NUM_BASE + s] = out[oa + ITEM_NUM_BASE + s]
        if patched != out[oa:oa + ROW_BYTES]:
            failures.append("H-I3 row %d changed outside item_* / itemNum_*" % rid)

        out_slots = slots(out, oa)
        ids = [v for v, _ in out_slots]

        # H-I4
        for t in TOOLS:
            n = ids.count(t)
            if n != 1:
                failures.append("H-I4 row %d holds tool %d %d times, expected 1" % (rid, t, n))
                continue
            k = ids.index(t)
            if out_slots[k][1] != 1:
                failures.append("H-I4 row %d tool %d has count %d, expected 1"
                                % (rid, t, out_slots[k][1]))

        # H-I5
        if out_slots[0] != (VANILLA_START_ITEM, 1):
            failures.append("H-I5 row %d displaced the vanilla starting item: slot 0 is %r"
                            % (rid, out_slots[0]))

    if changed != len(targets):
        failures.append("H-I4 %d of %d origin rows changed" % (changed, len(targets)))

    notes.append("%d origin rows received both tools" % changed)
    return failures, notes, changed


# --- commands ---------------------------------------------------------------

def cmd_rows(vanilla_root):
    plain, members = load_archive(vanilla_root)

    id_fail, id_notes = check_identification(plain, members)
    for n in id_notes:
        print("  " + n)
    for f in id_fail:
        print("  FAIL " + f)

    rows = row_map(plain, members, CHARA_MEMBER)
    print("")
    print("%s: %d rows, %d of them targeted" % (CHARA_MEMBER, len(rows), len(ORIGIN_ROWS)))
    for rid in ORIGIN_ROWS:
        if rid not in rows:
            print("  %-6d MISSING" % rid)
            continue
        at = rows[rid]
        s = slots(plain, at)
        free = sum(1 for v, _ in s if v == EMPTY)
        print("  %-6d holds=%-12s free slots=%d" % (rid, held(plain, at), free))

    pre = check_vanilla(plain, members)
    print("")
    if id_fail or pre:
        for f in (id_fail + pre)[:30]:
            print("  FAIL " + f)
        print("")
        print("%d PRECONDITION FAILURES" % len(id_fail + pre))
        return 1
    print("all preconditions hold - every origin row has room for both tools")
    return 0


def cmd_verify(vanilla_root, output_root):
    opath = os.path.join(output_root, REL)
    if not os.path.exists(opath):
        print("no item-data archive in output tree - expected when no param feature was on")
        return 0
    van, van_m = load_archive(vanilla_root)
    out, out_m = load_archive(output_root)

    print("vanilla decompressed=%d  output decompressed=%d" % (len(van), len(out)))
    failures, notes, changed = compare(van, van_m, out, out_m)
    for n in notes:
        print("  NOTE: " + n)
    if failures:
        print("")
        print("%d FAILURES:" % len(failures))
        for f in failures[:50]:
            print("  " + f)
        if len(failures) > 50:
            print("  ... and %d more" % (len(failures) - 50))
        return 1
    print("")
    print("all invariants hold")
    return 0


def cmd_selftest(vanilla_root):
    """Builds synthetic output from real vanilla bytes and checks that compare()
    accepts a correct edit and rejects each way of getting it wrong."""
    van, van_m = load_archive(vanilla_root)
    rows = row_map(van, van_m, CHARA_MEMBER)
    results = []

    def apply_good(buf):
        for rid in ORIGIN_ROWS:
            at = rows[rid]
            s = [i32(buf, at + ITEM_ID_BASE + k * 4) for k in range(ITEM_SLOTS)]
            free = [k for k in range(ITEM_SLOTS) if s[k] == EMPTY]
            for t, k in zip(TOOLS, free):
                set_i32(buf, at + ITEM_ID_BASE + k * 4, t)
                buf[at + ITEM_NUM_BASE + k] = 1

    def run(mutate):
        out = bytearray(van)
        mutate(out)
        f, _, _ = compare(van, van_m, out, van_m)
        return f

    # The identification and the vanilla preconditions, against real data.
    id_fail, _ = check_identification(van, van_m)
    results.append(("4103/4104 resolve from the two vanilla chest lots", not id_fail, id_fail))
    pre = check_vanilla(van, van_m)
    results.append(("every origin row has room for both tools", not pre, pre))

    results.append(("a correct edit passes", not run(apply_good), run(apply_good)))

    def unedited(buf):
        pass
    results.append(("doing nothing is REJECTED", bool(run(unedited)), []))

    def only_one_tool(buf):
        for rid in ORIGIN_ROWS:
            at = rows[rid]
            s = [i32(buf, at + ITEM_ID_BASE + k * 4) for k in range(ITEM_SLOTS)]
            k = [j for j in range(ITEM_SLOTS) if s[j] == EMPTY][0]
            set_i32(buf, at + ITEM_ID_BASE + k * 4, BLOOD_GEM_TOOL)
            buf[at + ITEM_NUM_BASE + k] = 1
    results.append(("granting only the gem tool is REJECTED", bool(run(only_one_tool)), []))

    def one_block_only(buf):
        for rid in ORIGIN_ROWS:
            if not (2000 <= rid <= 2009):
                continue
            at = rows[rid]
            s = [i32(buf, at + ITEM_ID_BASE + k * 4) for k in range(ITEM_SLOTS)]
            free = [k for k in range(ITEM_SLOTS) if s[k] == EMPTY]
            for t, k in zip(TOOLS, free):
                set_i32(buf, at + ITEM_ID_BASE + k * 4, t)
                buf[at + ITEM_NUM_BASE + k] = 1
    results.append(("writing only the 2000 block is REJECTED", bool(run(one_block_only)), []))

    def zero_count(buf):
        apply_good(buf)
        at = rows[ORIGIN_ROWS[0]]
        s = [i32(buf, at + ITEM_ID_BASE + k * 4) for k in range(ITEM_SLOTS)]
        buf[at + ITEM_NUM_BASE + s.index(BLOOD_GEM_TOOL)] = 0
    results.append(("a tool with count 0 is REJECTED", bool(run(zero_count)), []))

    def displace_start_item(buf):
        apply_good(buf)
        at = rows[ORIGIN_ROWS[0]]
        set_i32(buf, at + ITEM_ID_BASE, BLOOD_GEM_TOOL)
    results.append(("displacing the vanilla starting item is REJECTED",
                    bool(run(displace_start_item)), []))

    def stray_write(buf):
        apply_good(buf)
        at = rows[ORIGIN_ROWS[0]]
        set_i32(buf, at + 12, 9999)  # soul, nowhere near the item arrays
    results.append(("a stray write elsewhere in the row is REJECTED",
                    bool(run(stray_write)), []))

    def touch_npc_row(buf):
        apply_good(buf)
        victim = sorted(r for r in rows if r not in set(ORIGIN_ROWS))[0]
        set_i32(buf, rows[victim] + ITEM_ID_BASE, BLOOD_GEM_TOOL)
    results.append(("changing a non-origin row is REJECTED", bool(run(touch_npc_row)), []))

    def touch_other_member(buf):
        apply_good(buf)
        off, _ = van_m[GOODS_MEMBER]
        set_i32(buf, off + 0x40, 12345)
    results.append(("changing another param member is REJECTED",
                    bool(run(touch_other_member)), []))

    # The C++ constants must agree with the paramdef, or every offset above is
    # checking the wrong bytes in both implementations at once.
    src = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                       "..", "src", "Randomizer", "HunterTools.cpp")
    cpp = open(src, encoding="utf-8").read()
    agreed = ("kItemIdBase  = %d" % ITEM_ID_BASE in cpp and
              "kItemNumBase = %d" % ITEM_NUM_BASE in cpp and
              "kItemSlots   = %d" % ITEM_SLOTS in cpp and
              "kRowBytes    = %d" % ROW_BYTES in cpp and
              "kBloodGemWorkshopTool = %d" % BLOOD_GEM_TOOL in cpp and
              "kRuneWorkshopTool     = %d" % RUNE_TOOL in cpp)
    results.append(("HunterTools.cpp offsets match this tool", agreed, []))

    cpp_rows = all(str(r) in cpp for r in ORIGIN_ROWS)
    results.append(("HunterTools.cpp targets all %d origin rows" % len(ORIGIN_ROWS),
                    cpp_rows, []))

    passed = 0
    for label, ok, detail in results:
        print("  %-52s %s" % (label, "ok" if ok else "MISSED"))
        if not ok and detail:
            print("     failures were: %s" % (detail[:3],))
        if ok:
            passed += 1
    print("")
    print("selftest %d/%d" % (passed, len(results)))
    return 0 if passed == len(results) else 1


def main():
    if len(sys.argv) >= 3 and sys.argv[1] == "rows":
        return cmd_rows(sys.argv[2])
    if len(sys.argv) >= 4 and sys.argv[1] == "verify":
        return cmd_verify(sys.argv[2], sys.argv[3])
    if len(sys.argv) >= 3 and sys.argv[1] == "selftest":
        return cmd_selftest(sys.argv[2])
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main())
