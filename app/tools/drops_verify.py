#!/usr/bin/env python3
"""Enemy drop randomization verification.

A PROPERTY VALIDATOR over the real item-data archive: it never re-derives which
drop should land on which enemy, it checks invariants of generated output
against trusted vanilla input. Field offsets come from tools/param_offsets.py,
not from the C++.

Invariants (docs/plans/param-features.md):
  D-I1  only NpcParam.param differs inside the archive
  D-I2  within NpcParam, only itemLotId_1 differs - row ids and everything else intact
  D-I3  every assigned drop exists in the vanilla pool (no fabricated item lots)
  D-I4  the two excluded rows (252100, 6071) are untouched
  D-I5  rows whose vanilla drop was -1 stay -1
  D-I6  itemLotId_2 is never written (reference reads it but never assigns it)
  D-I7  archive still holds the same members

Usage:
    python drops_verify.py pool     <vanilla_dvdroot>
    python drops_verify.py verify   <vanilla_dvdroot> <output_dvdroot>
    python drops_verify.py selftest <vanilla_dvdroot>
"""

import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from boss_verify import read_dcx  # noqa: E402
from param_offsets import bnd4_members, param_rows  # noqa: E402

REL = os.path.join("param", "gameparam", "gameparam.parambnd.dcx")

ITEMLOT1_OFFSET = 44   # NpcParam.itemLotId_1, s32
ITEMLOT2_OFFSET = 48   # NpcParam.itemLotId_2, s32
EXCLUDED_ROWS = {252100, 6071}
NPC_MEMBER = "NpcParam.param"


def i32(buf, off):
    return struct.unpack_from("<i", buf, off)[0]


def set_i32(buf, off, v):
    buf[off:off + 4] = struct.pack("<i", v)


def load_archive(root):
    plain = bytearray(read_dcx(os.path.join(root, REL)))
    members = {name: (off, size) for name, off, size in bnd4_members(plain)}
    return plain, members


def npc_rows(plain, members):
    """Yields (row_id, absolute_row_offset)."""
    off, size = members[NPC_MEMBER]
    member = plain[off:off + size]
    for rid, rel in param_rows(member):
        yield rid, off + rel


def build_pool(plain, members):
    """Reference pool rule: both lot columns, skipping -1 and the excluded rows."""
    pool = []
    for rid, at in npc_rows(plain, members):
        if rid in EXCLUDED_ROWS:
            continue
        l1 = i32(plain, at + ITEMLOT1_OFFSET)
        l2 = i32(plain, at + ITEMLOT2_OFFSET)
        if l1 != -1:
            pool.append(l1)
        if l2 != -1:
            pool.append(l2)
    return pool


def eligible_rows(plain, members):
    """Rows the reference would reassign: not excluded, lot1 != -1."""
    out = {}
    for rid, at in npc_rows(plain, members):
        if rid in EXCLUDED_ROWS:
            continue
        l1 = i32(plain, at + ITEMLOT1_OFFSET)
        if l1 != -1:
            out[rid] = l1
    return out


def cmd_pool(vanilla_root):
    plain, members = load_archive(vanilla_root)
    rows = list(npc_rows(plain, members))
    pool = build_pool(plain, members)
    elig = eligible_rows(plain, members)

    print("item-data archive: %d members, %d bytes" % (len(members), len(plain)))
    print("NpcParam rows          : %d" % len(rows))
    print("pool entries           : %d  (%d distinct)" % (len(pool), len(set(pool))))
    print("rows eligible to change: %d" % len(elig))
    print()
    missing = [m for m in (NPC_MEMBER, "EquipParamWeapon.param", "ShopLineupParam.param")
               if m not in members]
    print("excluded rows present  : %s"
          % sorted(r for r in EXCLUDED_ROWS if any(rid == r for rid, _ in rows)))
    print("expected members missing: %s" % (missing or "none"))
    print()
    print("EXPECTED ON HARDWARE: pool %d, up to %d rows reassigned" % (len(pool), len(elig)))
    return 0


def compare(van_plain, van_members, out_plain, out_members):
    failures = []
    notes = []

    # D-I7 / D-I1: same members, and only NpcParam may differ.
    if sorted(van_members) != sorted(out_members):
        failures.append("D-I7 archive member list changed")
        return failures, notes, 0

    for name, (voff, vsize) in van_members.items():
        ooff, osize = out_members[name]
        if vsize != osize:
            failures.append("D-I7 member %s size changed %d -> %d" % (name, vsize, osize))
            continue
        if name == NPC_MEMBER:
            continue
        if van_plain[voff:voff + vsize] != out_plain[ooff:ooff + osize]:
            failures.append("D-I1 member %s modified (only %s may change)" % (name, NPC_MEMBER))

    pool = set(build_pool(van_plain, van_members))
    van_rows = dict(npc_rows(van_plain, van_members))
    out_rows = dict(npc_rows(out_plain, out_members))
    if set(van_rows) != set(out_rows):
        failures.append("D-I2 NpcParam row id set changed")
        return failures, notes, 0

    changed = 0
    for rid, vat in van_rows.items():
        oat = out_rows[rid]
        v1 = i32(van_plain, vat + ITEMLOT1_OFFSET)
        o1 = i32(out_plain, oat + ITEMLOT1_OFFSET)
        v2 = i32(van_plain, vat + ITEMLOT2_OFFSET)
        o2 = i32(out_plain, oat + ITEMLOT2_OFFSET)

        # D-I6: the second lot column must never be written.
        if v2 != o2:
            failures.append("D-I6 row %d itemLotId_2 changed %d -> %d" % (rid, v2, o2))

        # D-I2: nothing else in the row may move.
        voff = van_plain[vat:vat + ITEMLOT1_OFFSET]
        ooff = out_plain[oat:oat + ITEMLOT1_OFFSET]
        if voff != ooff:
            failures.append("D-I2 row %d bytes before itemLotId_1 changed" % rid)
        vtail = van_plain[vat + ITEMLOT1_OFFSET + 4:vat + ITEMLOT2_OFFSET]
        otail = out_plain[oat + ITEMLOT1_OFFSET + 4:oat + ITEMLOT2_OFFSET]
        if vtail != otail:
            failures.append("D-I2 row %d bytes between the two lot fields changed" % rid)

        if v1 == o1:
            continue
        changed += 1

        if rid in EXCLUDED_ROWS:
            failures.append("D-I4 excluded row %d changed %d -> %d" % (rid, v1, o1))
        if v1 == -1:
            failures.append("D-I5 row %d had no drop (-1) but was assigned %d" % (rid, o1))
        if o1 not in pool:
            failures.append("D-I3 row %d assigned %d, which is not a vanilla pool value"
                            % (rid, o1))

    return failures, notes, changed


def cmd_verify(vanilla_root, output_root):
    opath = os.path.join(output_root, REL)
    if not os.path.exists(opath):
        print("no item-data archive in output tree - expected when enemy drops were off")
        return 0
    van_plain, van_members = load_archive(vanilla_root)
    out_plain, out_members = load_archive(output_root)

    print("vanilla decompressed=%d  output decompressed=%d" % (len(van_plain), len(out_plain)))
    failures, notes, changed = compare(van_plain, van_members, out_plain, out_members)
    print("NpcParam rows with a changed drop: %d" % changed)
    for n in notes:
        print("  NOTE: " + n)
    if failures:
        print("\n%d FAILURES:" % len(failures))
        for f in failures[:50]:
            print("  " + f)
        if len(failures) > 50:
            print("  ... and %d more" % (len(failures) - 50))
        return 1
    print("\nAll enemy-drop property checks PASSED")
    return 0


def cmd_selftest(vanilla_root):
    """Corrupt a good archive in specific ways; each must be caught."""
    van_plain, van_members = load_archive(vanilla_root)
    results = []

    def run(label, mutate, expect):
        out = bytearray(van_plain)
        mutate(out)
        f, _, _ = compare(van_plain, van_members, out, van_members)
        hit = any(expect in x for x in f)
        results.append((label, hit, f))

    rows = dict(npc_rows(van_plain, van_members))
    elig = eligible_rows(van_plain, van_members)
    pool = build_pool(van_plain, van_members)
    a_row = next(iter(elig))

    results.append(("unmodified archive",
                    not compare(van_plain, van_members, bytearray(van_plain), van_members)[0],
                    []))

    run("fabricated item lot",
        lambda b: set_i32(b, rows[a_row] + ITEMLOT1_OFFSET, 987654321), "D-I3")

    run("itemLotId_2 written",
        lambda b: set_i32(b, rows[a_row] + ITEMLOT2_OFFSET, pool[0]), "D-I6")

    excluded_present = [r for r in EXCLUDED_ROWS if r in rows]
    if excluded_present:
        r = excluded_present[0]
        run("excluded row changed",
            lambda b, r=r: set_i32(b, rows[r] + ITEMLOT1_OFFSET, pool[0]), "D-I4")

    no_drop = [rid for rid, at in rows.items()
               if rid not in EXCLUDED_ROWS and i32(van_plain, at + ITEMLOT1_OFFSET) == -1]
    if no_drop:
        r = no_drop[0]
        run("drop given to a -1 row",
            lambda b, r=r: set_i32(b, rows[r] + ITEMLOT1_OFFSET, pool[0]), "D-I5")

    # A different member modified.
    other = next(n for n in van_members if n != NPC_MEMBER)
    off, _ = van_members[other]
    run("another param modified",
        lambda b: b.__setitem__(slice(off, off + 4), b"\xDE\xAD\xBE\xEF"), "D-I1")

    passed = 0
    for label, ok, detail in results:
        print("  %-28s %s" % (label, "OK" if ok else "MISSED"))
        if not ok:
            print("     failures were: %s" % (detail[:3] or "none at all"))
        else:
            passed += 1
    print("\nselftest %d/%d" % (passed, len(results)))
    return 0 if passed == len(results) else 1


def main():
    if len(sys.argv) >= 3 and sys.argv[1] == "pool":
        return cmd_pool(sys.argv[2])
    if len(sys.argv) >= 4 and sys.argv[1] == "verify":
        return cmd_verify(sys.argv[2], sys.argv[3])
    if len(sys.argv) >= 3 and sys.argv[1] == "selftest":
        return cmd_selftest(sys.argv[2])
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main())
