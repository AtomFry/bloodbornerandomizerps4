#!/usr/bin/env python3
"""NO TEAM TYPE verification (feature 027, shipped as ENEMIES HOSTILE TO EACH
OTHER).

A PROPERTY VALIDATOR over the real item-data archive, in the same shape as
drops_verify.py and hunter_tools_verify.py: it never re-derives the C++'s
answer, it checks invariants of generated output against trusted vanilla input.
The field offset comes from tools/param_offsets.py against the real paramdef
(NPC_PARAM_ST cell 100 = teamType, u8, at byte 303 of a 388-byte row), not from
the C++.

What the feature does: writes the fixed value 25 into NpcParam.teamType in
EVERY one of the 31398 rows, with no exclusion of any kind - no protection list,
no NPC or boss exemption, no map scoping, no row-id skip. That is exactly what
the reference tool's TeamTypeRando() does (RandomizeFunctions.cs:3287-3296). See
app/src/Randomizer/TeamType.h.

Invariants:
  T-I1  only NpcParam.param differs inside the archive
  T-I2  within NpcParam, only byte 303 of a row differs - bytes 0-302 and
        304-387 are intact (itemLotId_1 tolerated only under --drops)
  T-I3  EVERY row carries 25, no row carries any other value, and the count
        written is 31398
  T-I4  the row id set, the row count and the 388-byte stride are unchanged
  T-I5  the 378 rows that already held 25 in vanilla are byte-identical
  T-I6  the archive holds the same members, each the same size
  T-I7  under --off, every row's teamType equals vanilla's

This tool CANNOT prove the feature works in game, and that matters more here
than for any other param feature: it shows an output tree carries the byte the
reference writes, everywhere and only there, and it says NOTHING WHATEVER about
what Bloodborne does with the value. 25 is not known to mean "hostile to
everything"; the 378 rows already holding it in vanilla are not known to be
hostile to anything. docs/known-traps.md - byte decoding does not prove game
behaviour. Only the hardware test settles the label.

--drops says the run also had RANDOMIZE ENEMY DROPS on, which writes
itemLotId_1 of the same rows. It relaxes those four bytes per row and nothing
else; drops_verify.py owns whether the values it put there are legal.

Usage:
    python team_type_verify.py census   <vanilla_dvdroot>
    python team_type_verify.py verify   <vanilla_dvdroot> <output_dvdroot>
                                        [--off] [--drops]
    python team_type_verify.py selftest <vanilla_dvdroot>
"""

import collections
import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from boss_verify import read_dcx  # noqa: E402
from param_offsets import bnd4_members, param_rows  # noqa: E402

REL = os.path.join("param", "gameparam", "gameparam.parambnd.dcx")

NPC_MEMBER = "NpcParam.param"

# MUST match app/src/Randomizer/TeamType.cpp. The neighbours are npcType at 302
# and moveType at 304 - selftest case S5 is the off-by-one that would otherwise
# ship, because writing 25 into either one still produces a plausible tree.
TEAM_TYPE_OFFSET = 303
TEAM_TYPE_VALUE = 25
ROW_BYTES = 388

# RANDOMIZE ENEMY DROPS' field, tolerated only under --drops.
ITEMLOT1_OFFSET = 44

# The two rows the drop pass skips (RandomizeFunctions.cs:3125-3127). This
# feature takes NO exclusions, so both must be WRITTEN - case S7.
DROP_EXCLUDED_ROWS = (252100, 6071)

# Measured against data/vanilla/dvdroot_ps4 on 2026-09-28. Case S0 pins them so
# that a different game tree is reported rather than silently changing what
# every other case means.
EXPECTED_ROWS = 31398
EXPECTED_DISTINCT = 13
EXPECTED_ALREADY_25 = 378
EXPECTED_TO_CHANGE = EXPECTED_ROWS - EXPECTED_ALREADY_25  # 31020


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


def row_map(plain, members):
    return {rid: at for rid, at in npc_rows(plain, members)}


def strides(rows):
    """The distinct gaps between consecutive row data offsets."""
    offs = sorted(rows.values())
    return sorted({offs[i + 1] - offs[i] for i in range(len(offs) - 1)})


def census(plain, members):
    rows = row_map(plain, members)
    dist = collections.Counter(plain[at + TEAM_TYPE_OFFSET] for at in rows.values())
    return rows, dist


# --- commands --------------------------------------------------------------

def cmd_census(vanilla_root):
    plain, members = load_archive(vanilla_root)
    rows, dist = census(plain, members)
    print("item-data archive: %d members, %d bytes decompressed"
          % (len(members), len(plain)))
    print("%s rows        : %d" % (NPC_MEMBER, len(rows)))
    print("row stride(s) seen  : %s" % strides(rows))
    print("teamType offset     : %d (u8, cell 100)" % TEAM_TYPE_OFFSET)
    print()
    print("  teamType  rows")
    for value in sorted(dist):
        print("  %8d  %6d%s" % (value, dist[value],
                                "   <- the value this feature writes"
                                if value == TEAM_TYPE_VALUE else ""))
    print()
    print("distinct values     : %d" % len(dist))
    print("already at %d       : %d" % (TEAM_TYPE_VALUE, dist[TEAM_TYPE_VALUE]))
    print("would change        : %d" % (len(rows) - dist[TEAM_TYPE_VALUE]))
    print()
    print("EXPECTED ON HARDWARE: all %d rows written, %d of them changed"
          % (len(rows), len(rows) - dist[TEAM_TYPE_VALUE]))
    ok = (len(rows) == EXPECTED_ROWS and strides(rows) == [ROW_BYTES]
          and len(dist) == EXPECTED_DISTINCT
          and dist[TEAM_TYPE_VALUE] == EXPECTED_ALREADY_25)
    print()
    print("matches what plan 027 was measured against: %s" % ("yes" if ok else "NO"))
    return 0


def compare(van_plain, van_members, out_plain, out_members, off=False, drops=False):
    """Returns (failures, rows_at_value, rows_changed)."""
    failures = []

    # T-I6 / T-I1: same members, and only NpcParam may differ.
    if sorted(van_members) != sorted(out_members):
        failures.append("T-I6 archive member list changed")
        return failures, 0, 0

    for name, (voff, vsize) in van_members.items():
        ooff, osize = out_members[name]
        if vsize != osize:
            failures.append("T-I6 member %s size changed %d -> %d" % (name, vsize, osize))
            continue
        if name == NPC_MEMBER:
            continue
        if van_plain[voff:voff + vsize] != out_plain[ooff:ooff + osize]:
            failures.append("T-I1 member %s modified (only %s may change)"
                            % (name, NPC_MEMBER))

    van_rows = row_map(van_plain, van_members)
    out_rows = row_map(out_plain, out_members)

    # T-I4: the row table itself.
    if set(van_rows) != set(out_rows):
        failures.append("T-I4 NpcParam row id set changed (%d vanilla rows, %d output rows)"
                        % (len(van_rows), len(out_rows)))
        return failures, 0, 0
    if strides(out_rows) != strides(van_rows):
        failures.append("T-I4 row stride changed %s -> %s"
                        % (strides(van_rows), strides(out_rows)))

    at_value = 0
    changed = 0
    for rid, vat in van_rows.items():
        oat = out_rows[rid]
        vval = van_plain[vat + TEAM_TYPE_OFFSET]
        oval = out_plain[oat + TEAM_TYPE_OFFSET]
        if oval == TEAM_TYPE_VALUE:
            at_value += 1
        if oval != vval:
            changed += 1

        if off:
            # T-I7: the feature was off, so the field must not have moved.
            if oval != vval:
                failures.append("T-I7 row %d teamType changed %d -> %d with the "
                                "setting off" % (rid, vval, oval))
        else:
            # T-I3: every row, no exceptions. One miss is a failure.
            if oval != TEAM_TYPE_VALUE:
                failures.append("T-I3 row %d teamType is %d, not %d"
                                % (rid, oval, TEAM_TYPE_VALUE))

        # T-I2: nothing else in the 388-byte row may move. T-I5 is the same
        # check for the rows that already held 25 - they are covered by it
        # because their byte 303 is excluded from neither side's slice when it
        # did not change.
        vrow = bytearray(van_plain[vat:vat + ROW_BYTES])
        orow = bytearray(out_plain[oat:oat + ROW_BYTES])
        vrow[TEAM_TYPE_OFFSET] = orow[TEAM_TYPE_OFFSET]  # judged above
        if drops:
            # RANDOMIZE ENEMY DROPS owns these four bytes; whether the value it
            # wrote is legal is drops_verify.py's question, not this tool's.
            vrow[ITEMLOT1_OFFSET:ITEMLOT1_OFFSET + 4] = \
                orow[ITEMLOT1_OFFSET:ITEMLOT1_OFFSET + 4]
        if vrow[:TEAM_TYPE_OFFSET] != orow[:TEAM_TYPE_OFFSET]:
            failures.append("T-I2 row %d bytes before teamType changed" % rid)
        if vrow[TEAM_TYPE_OFFSET + 1:] != orow[TEAM_TYPE_OFFSET + 1:]:
            failures.append("T-I2 row %d bytes after teamType changed" % rid)

        # T-I5, stated separately from T-I2 because it is the claim worth making
        # on its own: a row that already held 25 must come out untouched.
        if vval == TEAM_TYPE_VALUE and vrow != orow:
            failures.append("T-I5 row %d already held %d in vanilla but its bytes "
                            "changed" % (rid, TEAM_TYPE_VALUE))

    if not off and at_value != len(van_rows):
        failures.append("T-I3 %d of %d rows carry %d"
                        % (at_value, len(van_rows), TEAM_TYPE_VALUE))
    if not off and len(van_rows) != EXPECTED_ROWS:
        failures.append("T-I3 the tree holds %d rows, not the %d this feature was "
                        "measured against" % (len(van_rows), EXPECTED_ROWS))

    return failures, at_value, changed


def cmd_verify(vanilla_root, output_root, off=False, drops=False):
    opath = os.path.join(output_root, REL)
    if not os.path.exists(opath):
        if off:
            print("no item-data archive in output tree - expected when no param "
                  "feature was on, and consistent with the setting being off")
            return 0
        print("FAILED: no item-data archive in the output tree, but the setting "
              "was on")
        return 1
    van_plain, van_members = load_archive(vanilla_root)
    out_plain, out_members = load_archive(output_root)

    print("vanilla decompressed=%d  output decompressed=%d"
          % (len(van_plain), len(out_plain)))
    print("mode: %s%s" % ("OFF (T-I7)" if off else "ON (T-I1-T-I6)",
                          ", --drops tolerated" if drops else ""))
    failures, at_value, changed = compare(van_plain, van_members, out_plain,
                                          out_members, off=off, drops=drops)
    print("rows holding %d: %d" % (TEAM_TYPE_VALUE, at_value))
    print("rows whose teamType differs from vanilla: %d" % changed)
    if failures:
        print("\n%d FAILURES:" % len(failures))
        for f in failures[:50]:
            print("  " + f)
        if len(failures) > 50:
            print("  ... and %d more" % (len(failures) - 50))
        return 1
    print("\nAll team-type property checks PASSED")
    print("This says nothing about what the game does with the value - see the "
          "module docstring.")
    return 0


# --- selftest --------------------------------------------------------------

def cmd_selftest(vanilla_root):
    """Simulate output in Python; every wrong tree below must be caught."""
    van, van_m = load_archive(vanilla_root)
    rows, dist = census(van, van_m)
    results = []

    def fails(mutate, off=False, drops=False, members=None):
        out = bytearray(van)
        mutate(out)
        f, _, _ = compare(van, van_m, out, members or van_m, off=off, drops=drops)
        return f

    def case(label, ok, detail=()):
        results.append((label, ok, list(detail)))

    # S0 - the data tree is the one this plan was measured against.
    s0 = (len(rows) == EXPECTED_ROWS and strides(rows) == [ROW_BYTES]
          and len(dist) == EXPECTED_DISTINCT
          and dist[TEAM_TYPE_VALUE] == EXPECTED_ALREADY_25
          and len(rows) - dist[TEAM_TYPE_VALUE] == EXPECTED_TO_CHANGE)
    case("S0  vanilla census is %d rows / %d values / %d at %d"
         % (EXPECTED_ROWS, EXPECTED_DISTINCT, EXPECTED_ALREADY_25, TEAM_TYPE_VALUE),
         s0, ["%d rows, %d values, %d at %d, stride %s"
              % (len(rows), len(dist), dist[TEAM_TYPE_VALUE], TEAM_TYPE_VALUE,
                 strides(rows))])

    def untouched(buf):
        pass

    def write_all(buf, value=TEAM_TYPE_VALUE, offset=TEAM_TYPE_OFFSET, skip=()):
        for rid, at in rows.items():
            if rid in skip:
                continue
            buf[at + offset] = value

    # S1 - an unmodified archive is a correct OFF run and a failed ON run.
    case("S1  unmodified archive passes --off", not fails(untouched, off=True),
         fails(untouched, off=True))
    case("S1  unmodified archive is REJECTED in on-mode (T-I3 can fail)",
         any("T-I3" in x for x in fails(untouched)))

    # S2 / S3 - a fully simulated pass.
    case("S2  25 in every row passes on-mode", not fails(write_all), fails(write_all))
    case("S3  the same tree is REJECTED under --off (T-I7)",
         any("T-I7" in x for x in fails(write_all, off=True)))

    # S4 - one row missed. 31398 of 31398 is the whole claim.
    a_changing_row = next(rid for rid, at in rows.items()
                          if van[at + TEAM_TYPE_OFFSET] != TEAM_TYPE_VALUE)

    def miss_one(buf):
        write_all(buf, skip=(a_changing_row,))
    case("S4  one row left at its vanilla value is REJECTED",
         any("T-I3" in x for x in fails(miss_one)))

    # S5 - the off-by-one that would otherwise ship.
    for neighbour, field in ((TEAM_TYPE_OFFSET - 1, "npcType"),
                             (TEAM_TYPE_OFFSET + 1, "moveType")):
        def wrong_offset(buf, o=neighbour):
            write_all(buf, offset=o)
        f = fails(wrong_offset)
        case("S5  writing %d at offset %d (%s) is REJECTED"
             % (TEAM_TYPE_VALUE, neighbour, field),
             any("T-I2" in x for x in f) and any("T-I3" in x for x in f), f)

    # S6 - the right place, the wrong value.
    def wrong_value(buf):
        write_all(buf, value=24)
    case("S6  a uniform value that is not %d is REJECTED" % TEAM_TYPE_VALUE,
         any("T-I3" in x for x in fails(wrong_value)))

    # S7 - the drop pass's two exclusions are NOT exclusions here.
    present = [r for r in DROP_EXCLUDED_ROWS if r in rows]

    def skip_drop_exclusions(buf):
        write_all(buf, skip=tuple(present))
    case("S7  skipping the drop pass's rows %s is REJECTED" % (present,),
         bool(present) and any("T-I3" in x for x in fails(skip_drop_exclusions)))

    # S8 - the --drops tolerance is one field wide, and T-I3 survives it.
    lot_row = next(iter(rows))

    def one_drop_changed(buf):
        write_all(buf)
        buf[rows[lot_row] + ITEMLOT1_OFFSET:rows[lot_row] + ITEMLOT1_OFFSET + 4] = \
            struct.pack("<i", 123456)
    case("S8  a changed itemLotId_1 is REJECTED without --drops",
         any("T-I2" in x for x in fails(one_drop_changed)))
    case("S8  ...and tolerated with --drops",
         not fails(one_drop_changed, drops=True), fails(one_drop_changed, drops=True))

    def drop_changed_partial_team(buf):
        one_drop_changed(buf)
        buf[rows[a_changing_row] + TEAM_TYPE_OFFSET] = van[rows[a_changing_row]
                                                           + TEAM_TYPE_OFFSET]
    case("S8  ...and T-I3 is still asserted under --drops",
         any("T-I3" in x for x in fails(drop_changed_partial_team, drops=True)))

    # S9 - the rest of the archive.
    other = next(n for n in van_m if n != NPC_MEMBER)
    other_off, other_size = van_m[other]

    def touch_other_member(buf):
        write_all(buf)
        buf[other_off + 0x40] ^= 0xFF
    case("S9  changing a byte of another member is REJECTED (T-I1)",
         any("T-I1" in x for x in fails(touch_other_member)))

    short = dict(van_m)
    short[other] = (other_off, other_size - 4)
    case("S9  a changed member size is REJECTED (T-I6)",
         any("T-I6" in x for x in fails(write_all, members=short)))

    dropped = {k: v for k, v in van_m.items() if k != other}
    case("S9  a changed member list is REJECTED (T-I6)",
         any("T-I6" in x for x in fails(write_all, members=dropped)))

    # S10 - the combination a hardware run will actually produce.
    shuffle_rows = [rid for rid, at in list(rows.items())[:500]
                    if struct.unpack_from("<i", van, at + ITEMLOT1_OFFSET)[0] != -1]

    def combined(buf):
        write_all(buf)
        for rid in shuffle_rows:
            at = rows[rid] + ITEMLOT1_OFFSET
            cur = struct.unpack_from("<i", van, at)[0]
            buf[at:at + 4] = struct.pack("<i", cur + 10)
    case("S10 a combined team-type + drop-shuffle tree passes with --drops",
         not fails(combined, drops=True), fails(combined, drops=True))
    case("S10 ...and fails without it", bool(fails(combined)))

    # The C++ constants must agree with this tool, or both are checking the
    # wrong byte at once - and the loop must still have no filter in it, which
    # is the one thing about this pass a reader could "helpfully" break.
    src = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                       "..", "src", "Randomizer", "TeamType.cpp")
    cpp = open(src, encoding="utf-8").read()
    case("TeamType.cpp offsets and value match this tool",
         ("kTeamTypeOffset = %d" % TEAM_TYPE_OFFSET) in cpp
         and ("kOneTeamTypeValue = (uint8_t)%d" % TEAM_TYPE_VALUE) in cpp)
    body = cpp[cpp.index("for (const ParamRow& row : rows)"):]
    case("TeamType.cpp's row loop contains no continue and no row-id test",
         "continue" not in body and not re.search(r"row\.id\s*==", body))

    passed = 0
    for label, ok, detail in results:
        print("  %-62s %s" % (label, "ok" if ok else "MISSED"))
        if not ok and detail:
            print("     failures were: %s" % (detail[:3],))
        if ok:
            passed += 1
    print("")
    print("selftest %d/%d" % (passed, len(results)))
    return 0 if passed == len(results) else 1


def main():
    args = list(sys.argv[1:])
    off = "--off" in args
    drops = "--drops" in args
    for flag in ("--off", "--drops"):
        while flag in args:
            args.remove(flag)
    if len(args) >= 2 and args[0] == "census":
        return cmd_census(args[1])
    if len(args) >= 3 and args[0] == "verify":
        return cmd_verify(args[1], args[2], off=off, drops=drops)
    if len(args) >= 2 and args[0] == "selftest":
        return cmd_selftest(args[1])
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main())
