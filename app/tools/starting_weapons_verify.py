#!/usr/bin/env python3
"""Starting weapon / shop weapon randomization verification.

A PROPERTY VALIDATOR over the real item-data archive. It never re-derives which
weapon should land in which slot - it checks invariants of generated output
against trusted vanilla input.

The melee and firearm candidate lists are parsed straight out of the reference
C# (RandomizeFunctions.cs rightHandList/leftHandList), NOT out of our
StartingWeaponLists.h. That keeps the validator independent of the code it is
validating: if the transcription into C++ were wrong, this would catch it.

Invariants (docs/plans/starting-weapons.md):
  SW-I1  only ShopLineupParam.param and EquipParamWeapon.param may differ
  SW-I2  within ShopLineupParam only equipId may differ; row ids and equipType intact
  SW-I3  melee slots (2000-2002) get melee-list ids; gun slots (2010-2011) get firearms
  SW-I4  every assigned equipId exists as a row in EquipParamWeapon
  SW-I5  the three melee slots are distinct, and so are the two gun slots
  SW-I6  within EquipParamWeapon only the four stat-requirement bytes may differ,
         and only on an assigned weapon or one of its +100n upgrade tiers
  SW-I7  each assigned slot's weapon carries that slot's stat profile on every
         tier that exists
  SW-I8  shop rows outside the five slots are a permutation of vanilla stock
  SW-I9  the archive member list is unchanged

Usage:
    python starting_weapons_verify.py pool     <vanilla_dvdroot>
    python starting_weapons_verify.py verify   <vanilla_dvdroot> <output_dvdroot>
    python starting_weapons_verify.py selftest <vanilla_dvdroot>
"""

import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from boss_verify import read_dcx  # noqa: E402
from param_offsets import bnd4_members, param_rows  # noqa: E402

REL = os.path.join("param", "gameparam", "gameparam.parambnd.dcx")

SHOP_MEMBER = "ShopLineupParam.param"
WEAPON_MEMBER = "EquipParamWeapon.param"

SHOP_EQUIP_ID = 0     # s32
SHOP_EQUIP_TYPE = 23  # u8, 0 = weapon
EQUIP_TYPE_WEAPON = 0

# EquipParamWeapon requirements are single bytes, not int32.
PROPER_STRENGTH = 237
PROPER_AGILITY = 238
PROPER_MAGIC = 239
PROPER_FAITH = 240
STAT_OFFSETS = (PROPER_STRENGTH, PROPER_AGILITY, PROPER_MAGIC, PROPER_FAITH)

MELEE_SLOTS = (2000, 2001, 2002)
GUN_SLOTS = (2010, 2011)
NEVER_TOUCHED = {1000, 900, 240}

# Slot -> (strength, agility, magic, faith), the vanilla starter profiles.
STAT_PROFILES = {
    2000: (8, 7, 0, 0),
    2001: (9, 8, 0, 0),
    2002: (7, 9, 0, 0),
    2010: (7, 9, 5, 0),
    2011: (7, 9, 5, 0),
}

UPGRADE_TIERS = 10
UPGRADE_STRIDE = 100

CS_REL = os.path.join("reference", "Randomizer", "MainWindowComponents",
                      "RandomizeFunctions.cs")


def i32(buf, off):
    return struct.unpack_from("<i", buf, off)[0]


def set_i32(buf, off, v):
    buf[off:off + 4] = struct.pack("<i", v)


def find_reference_cs():
    """Walks up from tools/ looking for the reference C# file."""
    here = os.path.dirname(os.path.abspath(__file__))
    for _ in range(6):
        cand = os.path.join(here, CS_REL)
        if os.path.exists(cand):
            return cand
        here = os.path.dirname(here)
    return None


def reference_lists():
    """melee, firearm id sets, parsed from the reference C#.

    The stray slash on "/29000000" is stripped here to match deviation SW-4 -
    the validator has to agree that the typo is a typo, or it would reject a
    perfectly good assignment.
    """
    path = find_reference_cs()
    if path is None:
        raise SystemExit("could not locate %s to parse the reference lists" % CS_REL)
    text = open(path, encoding="utf-8", errors="replace").read()

    def grab(var):
        pat = re.compile(re.escape(var) + r'\.Add\("(/?\d+)"\)')
        return [int(m.group(1).lstrip("/")) for m in pat.finditer(text)]

    return set(grab("rightHandList")), set(grab("leftHandList"))


def load_archive(root):
    plain = bytearray(read_dcx(os.path.join(root, REL)))
    members = {name: (off, size) for name, off, size in bnd4_members(plain)}
    return plain, members


def rows_of(plain, members, member_name):
    """{row_id: absolute_row_offset}"""
    off, size = members[member_name]
    member = plain[off:off + size]
    return {rid: off + rel for rid, rel in param_rows(member)}


def shop_weapon_rows(plain, members):
    """{row_id: offset} for shop rows selling a weapon we are allowed to touch."""
    out = {}
    for rid, at in rows_of(plain, members, SHOP_MEMBER).items():
        if plain[at + SHOP_EQUIP_TYPE] != EQUIP_TYPE_WEAPON:
            continue
        if i32(plain, at + SHOP_EQUIP_ID) in NEVER_TOUCHED:
            continue
        out[rid] = at
    return out


def cmd_pool(vanilla_root):
    plain, members = load_archive(vanilla_root)
    melee_ids, gun_ids = reference_lists()
    shop = shop_weapon_rows(plain, members)
    weapons = rows_of(plain, members, WEAPON_MEMBER)

    sold = set()
    for at in shop.values():
        sold.add(i32(plain, at + SHOP_EQUIP_ID))

    melee_pool = sorted(sold & melee_ids)
    gun_pool = sorted(sold & gun_ids)

    print("reference lists: %d melee, %d firearms" % (len(melee_ids), len(gun_ids)))
    print("shop weapon rows: %d  (distinct weapons sold: %d)" % (len(shop), len(sold)))
    print("candidate pools:  %d melee, %d firearms" % (len(melee_pool), len(gun_pool)))

    missing = sorted((melee_ids | gun_ids) - set(weapons))
    if missing:
        print("WARNING %d list ids have no EquipParamWeapon row: %s" % (len(missing), missing))
    not_sold = sorted((melee_ids | gun_ids) - sold)
    if not_sold:
        print("NOTE %d list ids are not sold anywhere, so unreachable: %s"
              % (len(not_sold), not_sold))

    print("\nvanilla starting slots:")
    all_rows = rows_of(plain, members, SHOP_MEMBER)
    for rid in MELEE_SLOTS + GUN_SLOTS:
        if rid in all_rows:
            print("  %d -> %d" % (rid, i32(plain, all_rows[rid] + SHOP_EQUIP_ID)))
    return 0


def compare(van_plain, van_members, out_plain, out_members):
    failures = []
    notes = []

    # SW-I9 / SW-I1
    if sorted(van_members) != sorted(out_members):
        failures.append("SW-I9 archive member list changed")
        return failures, notes, {}

    for name, (voff, vsize) in van_members.items():
        ooff, osize = out_members[name]
        if vsize != osize:
            failures.append("SW-I9 member %s size changed %d -> %d" % (name, vsize, osize))
            continue
        if name in (SHOP_MEMBER, WEAPON_MEMBER):
            continue
        if van_plain[voff:voff + vsize] != out_plain[ooff:ooff + osize]:
            failures.append("SW-I1 member %s modified (only shop/weapon params may change)"
                            % name)

    melee_ids, gun_ids = reference_lists()
    van_shop = rows_of(van_plain, van_members, SHOP_MEMBER)
    out_shop = rows_of(out_plain, out_members, SHOP_MEMBER)
    if set(van_shop) != set(out_shop):
        failures.append("SW-I2 ShopLineupParam row id set changed")
        return failures, notes, {}

    van_weapons = rows_of(van_plain, van_members, WEAPON_MEMBER)
    out_weapons = rows_of(out_plain, out_members, WEAPON_MEMBER)
    if set(van_weapons) != set(out_weapons):
        failures.append("SW-I1 EquipParamWeapon row id set changed")
        return failures, notes, {}

    # --- ShopLineupParam: only equipId may move -----------------------------
    assigned = {}
    slot_changed = 0
    shop_changed = 0
    for rid, vat in van_shop.items():
        oat = out_shop[rid]
        v_id = i32(van_plain, vat + SHOP_EQUIP_ID)
        o_id = i32(out_plain, oat + SHOP_EQUIP_ID)

        if van_plain[vat + 4:vat + 32] != out_plain[oat + 4:oat + 32]:
            failures.append("SW-I2 shop row %d changed outside equipId" % rid)

        if v_id == o_id:
            continue

        if rid in STAT_PROFILES:
            assigned[rid] = o_id
            slot_changed += 1
        else:
            shop_changed += 1

        if rid in MELEE_SLOTS and o_id not in melee_ids:
            failures.append("SW-I3 melee slot %d assigned %d, not on the melee list"
                            % (rid, o_id))
        if rid in GUN_SLOTS and o_id not in gun_ids:
            failures.append("SW-I3 gun slot %d assigned %d, not on the firearm list"
                            % (rid, o_id))
        if o_id not in van_weapons:
            failures.append("SW-I4 row %d assigned %d, which has no EquipParamWeapon row"
                            % (rid, o_id))
        if o_id in NEVER_TOUCHED:
            failures.append("SW-I2 row %d assigned a never-touch id %d" % (rid, o_id))

    # SW-I5: distinctness within each group of slots.
    for label, group in (("melee", MELEE_SLOTS), ("gun", GUN_SLOTS)):
        final = [i32(out_plain, out_shop[r] + SHOP_EQUIP_ID) for r in group if r in out_shop]
        if len(final) != len(set(final)):
            failures.append("SW-I5 %s slots offer a duplicate weapon: %s" % (label, final))

    # SW-I8: non-slot weapon stock is a permutation of itself.
    def stock(plain, shop):
        out = []
        for rid, at in sorted(shop.items()):
            if rid in STAT_PROFILES:
                continue
            if plain[at + SHOP_EQUIP_TYPE] != EQUIP_TYPE_WEAPON:
                continue
            eid = i32(plain, at + SHOP_EQUIP_ID)
            if eid in NEVER_TOUCHED:
                continue
            out.append(eid)
        return out

    v_stock, o_stock = stock(van_plain, van_shop), stock(out_plain, out_shop)
    if sorted(v_stock) != sorted(o_stock):
        failures.append("SW-I8 shop weapon stock is not a permutation of vanilla "
                        "(%d vanilla vs %d output entries)" % (len(v_stock), len(o_stock)))

    # --- EquipParamWeapon: only stat bytes, only on assigned families -------
    allowed = set()
    for rid, weapon in assigned.items():
        for tier in range(UPGRADE_TIERS + 1):
            allowed.add(weapon + tier * UPGRADE_STRIDE)

    stat_rows = 0
    for wid, vat in van_weapons.items():
        oat = out_weapons[wid]
        vrow = van_plain[vat:vat + 316]
        orow = out_plain[oat:oat + 316]
        if vrow == orow:
            continue

        diff_offsets = {i for i in range(len(vrow)) if vrow[i] != orow[i]}
        if not diff_offsets <= set(STAT_OFFSETS):
            failures.append("SW-I6 weapon row %d changed outside the stat fields "
                            "(offsets %s)" % (wid, sorted(diff_offsets - set(STAT_OFFSETS))[:8]))
        if wid not in allowed:
            failures.append("SW-I6 weapon row %d had stats rewritten but is not an "
                            "assigned weapon or one of its upgrade tiers" % wid)
        stat_rows += 1

    # SW-I7: the profile actually landed, on every tier that exists.
    for rid, weapon in assigned.items():
        want = STAT_PROFILES[rid]
        for tier in range(UPGRADE_TIERS + 1):
            wid = weapon + tier * UPGRADE_STRIDE
            if wid not in out_weapons:
                continue
            at = out_weapons[wid]
            got = tuple(out_plain[at + o] for o in STAT_OFFSETS)
            if got != want:
                failures.append("SW-I7 slot %d weapon %d tier %d has stats %s, expected %s"
                                % (rid, weapon, tier, got, want))

    if assigned:
        notes.append("slot assignments: " +
                     ", ".join("%d->%d" % (r, w) for r, w in sorted(assigned.items())))

    counts = {"slots": slot_changed, "shop": shop_changed, "stat_rows": stat_rows}
    return failures, notes, counts


def cmd_verify(vanilla_root, output_root):
    opath = os.path.join(output_root, REL)
    if not os.path.exists(opath):
        print("no item-data archive in output tree - expected when every param toggle was off")
        return 0
    van_plain, van_members = load_archive(vanilla_root)
    out_plain, out_members = load_archive(output_root)

    print("vanilla decompressed=%d  output decompressed=%d" % (len(van_plain), len(out_plain)))
    failures, notes, counts = compare(van_plain, van_members, out_plain, out_members)
    if counts:
        print("starting slots changed: %d   other shop weapon rows changed: %d   "
              "weapon rows restatted: %d"
              % (counts["slots"], counts["shop"], counts["stat_rows"]))
    for n in notes:
        print("  NOTE: " + n)
    if failures:
        print("\n%d FAILURES:" % len(failures))
        for f in failures[:50]:
            print("  " + f)
        if len(failures) > 50:
            print("  ... and %d more" % (len(failures) - 50))
        return 1
    print("\nAll starting-weapon property checks PASSED")
    return 0


def cmd_selftest(vanilla_root):
    """Build a plausible good output, then break it in specific ways."""
    van_plain, van_members = load_archive(vanilla_root)
    melee_ids, gun_ids = reference_lists()
    shop = rows_of(van_plain, van_members, SHOP_MEMBER)
    weapons = rows_of(van_plain, van_members, WEAPON_MEMBER)

    sold = {i32(van_plain, at + SHOP_EQUIP_ID) for at in shop_weapon_rows(van_plain, van_members).values()}
    melee_pool = sorted(sold & melee_ids & set(weapons))
    gun_pool = sorted(sold & gun_ids & set(weapons))

    def assign(buf, rid, weapon):
        set_i32(buf, shop[rid] + SHOP_EQUIP_ID, weapon)
        for tier in range(UPGRADE_TIERS + 1):
            wid = weapon + tier * UPGRADE_STRIDE
            if wid not in weapons:
                continue
            for off, val in zip(STAT_OFFSETS, STAT_PROFILES[rid]):
                buf[weapons[wid] + off] = val

    # A well-formed randomized archive: three distinct melee, two distinct guns.
    good = bytearray(van_plain)
    picks = {2000: melee_pool[0], 2001: melee_pool[1], 2002: melee_pool[2],
             2010: gun_pool[0], 2011: gun_pool[1]}
    for rid, w in picks.items():
        assign(good, rid, w)

    results = []

    def run(label, mutate, expect):
        out = bytearray(good)
        mutate(out)
        f, _, _ = compare(van_plain, van_members, out, van_members)
        results.append((label, any(expect in x for x in f), f))

    f0, _, _ = compare(van_plain, van_members, bytearray(van_plain), van_members)
    results.append(("unmodified archive", not f0, f0))

    fg, _, _ = compare(van_plain, van_members, good, van_members)
    results.append(("well-formed randomization", not fg, fg))

    run("melee slot given a firearm",
        lambda b: set_i32(b, shop[2000] + SHOP_EQUIP_ID, gun_pool[0]), "SW-I3")

    run("gun slot given a melee weapon",
        lambda b: set_i32(b, shop[2010] + SHOP_EQUIP_ID, melee_pool[0]), "SW-I3")

    run("fabricated weapon id",
        lambda b: set_i32(b, shop[2001] + SHOP_EQUIP_ID, 987654321), "SW-I4")

    run("duplicate melee slots",
        lambda b: set_i32(b, shop[2002] + SHOP_EQUIP_ID, picks[2000]), "SW-I5")

    run("stat profile not applied",
        lambda b: b.__setitem__(weapons[picks[2000]] + PROPER_STRENGTH, 99), "SW-I7")

    # An unrelated weapon restatted - allowed offsets, wrong row.
    unrelated = next(w for w in sorted(weapons)
                     if all(w < p or w > p + UPGRADE_TIERS * UPGRADE_STRIDE
                            for p in picks.values()))
    run("unrelated weapon restatted",
        lambda b: b.__setitem__(weapons[unrelated] + PROPER_STRENGTH, 99), "SW-I6")

    # A non-stat byte inside an assigned weapon row.
    run("weapon row changed outside stat fields",
        lambda b: b.__setitem__(weapons[picks[2000]] + 100, 0x5A), "SW-I6")

    # Shop stock invented rather than permuted.
    non_slot = next(r for r in sorted(shop_weapon_rows(van_plain, van_members))
                    if r not in STAT_PROFILES)
    run("shop stock not a permutation",
        lambda b: set_i32(b, shop[non_slot] + SHOP_EQUIP_ID, melee_pool[0]), "SW-I8")

    run("equipType changed",
        lambda b: b.__setitem__(shop[non_slot] + SHOP_EQUIP_TYPE, 9), "SW-I2")

    other = next(n for n in van_members if n not in (SHOP_MEMBER, WEAPON_MEMBER))
    ooff, _ = van_members[other]
    run("another param modified",
        lambda b: b.__setitem__(slice(ooff, ooff + 4), b"\xDE\xAD\xBE\xEF"), "SW-I1")

    passed = 0
    for label, ok, detail in results:
        print("  %-38s %s" % (label, "OK" if ok else "MISSED"))
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
