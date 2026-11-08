#!/usr/bin/env python3
"""Shop listing randomization verification - weapons, armour and consumables.

A PROPERTY VALIDATOR over the real item-data archive. It never re-derives which
item should land in which listing - it checks invariants of generated output
against trusted vanilla input.

It owns ShopLineupParam, which is why TWO features are verified here rather than
by two tools (spec 011 §6): backlog row 5 (RANDOMIZE STARTING WEAPONS / GUNS /
SHOP WEAPONS) permutes the 644 equipType 0 listings and the five Hunter's Dream
coffin rows, and backlog row 11 (RANDOMIZE SHOP ARMOUR / SHOP ITEMS) permutes the
220 equipType 1 and 397 shufflable equipType 3 listings. Those three buckets are
disjoint, and EVERY per-listing check below dispatches on the listing's VANILLA
equipType byte - never on the numeric shape of an id, because weapon, armour and
goods ids overlap and five goods ids also exist as EquipParamProtector row ids.

The melee and firearm candidate lists are parsed straight out of the reference
C# (RandomizeFunctions.cs rightHandList/leftHandList), NOT out of our
StartingWeaponLists.h. That keeps the validator independent of the code it is
validating: if the transcription into C++ were wrong, this would catch it.

Invariants (docs/plans/starting-weapons.md, docs/features/011-randomize-shop-items/):
  SW-I1  only ShopLineupParam.param and EquipParamWeapon.param may differ.
         EquipParamProtector.param and EquipParamGoods.param are read for the
         existence checks and stay UNTOLERATED - feature 011 reads them and must
         never write them
  SW-I2  within ShopLineupParam only equipId may differ; row ids and equipType intact
  SW-I3  melee slots (2000-2002) get melee-list ids; gun slots (2010-2011) get
         firearms - equipType 0 listings only
  SW-I4  every assigned equipId on an equipType 0 listing exists as a row in
         EquipParamWeapon
  SW-I5  the three melee slots are distinct, and so are the two gun slots
  SW-I6  within EquipParamWeapon only the four stat-requirement bytes may differ,
         and only on an assigned weapon or one of its +100n upgrade tiers
  SW-I7  each assigned slot's weapon carries that slot's stat profile on every
         tier that exists
  SW-I8  shop rows outside the five slots are a permutation of vanilla stock -
         equipType 0 listings only
  SW-I9  the archive member list is unchanged
  SW-I10 every assigned equipId on an equipType 1 listing exists as a row in
         EquipParamProtector, and on an equipType 3 listing in EquipParamGoods
  SW-I11 each bucket's output ids are a permutation of that bucket's vanilla ids
  SW-I12 the buckets never cross: an id written into a listing was one that
         listing's OWN bucket held in vanilla
  SW-I13 the three never-touched ids (1000 Blood Vials, 900 Quicksilver Bullets,
         240) are neither sources nor destinations - a listing selling one is
         unchanged, and no listing is ever assigned one
  SW-I14 a bucket that was NOT declared is byte-identical to vanilla, and a
         declared one is either untouched or wholesale-changed - never partly
         permuted

Usage:
    python starting_weapons_verify.py pool     <vanilla_dvdroot>
    python starting_weapons_verify.py verify   <vanilla_dvdroot> <output_dvdroot>
                                               [--shop-armour] [--shop-items]
                                               [--granted <weapon id>]...
    python starting_weapons_verify.py selftest <vanilla_dvdroot>

--granted <id> says the run also had a character-creation grant on and that it
granted that weapon. It may be given MORE THAN ONCE: feature 037's START WITH A
TRICK WEAPON and feature 038's START WITH A LEFT WEAPON can both be on in one
run, and this tool does not care which hand a granted weapon went into - only
which weapons were allowed to be restatted. Without the flag a correct combined
run reports FAILED on three invariants that are working exactly as intended -
see GRANT_PROFILE.

--shop-armour and --shop-items say the run had feature 011's two settings on.
They are DECLARATIONS, not tolerances: an undeclared bucket must come out
byte-identical to vanilla, so passing neither is how an all-off run is checked,
and passing one is how "the other setting stayed off" is checked. This tool
cannot and does not predict which item a seed puts in a given listing -
determinism is a hardware check.
"""

import os
import random
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from boss_verify import read_dcx  # noqa: E402
from param_offsets import bnd4_members, param_rows  # noqa: E402

REL = os.path.join("param", "gameparam", "gameparam.parambnd.dcx")

SHOP_MEMBER = "ShopLineupParam.param"
WEAPON_MEMBER = "EquipParamWeapon.param"
# Read-only, for SW-I10's existence check. Deliberately NOT added to the
# tolerated set in compare(): feature 011 reads these two tables to know which
# ids exist and writes neither, so a single changed byte in either is SW-I1.
PROTECTOR_MEMBER = "EquipParamProtector.param"
GOODS_MEMBER = "EquipParamGoods.param"

SHOP_EQUIP_ID = 0     # s32
SHOP_EQUIP_TYPE = 23  # u8: which table equipId indexes
EQUIP_TYPE_WEAPON = 0
EQUIP_TYPE_ARMOUR = 1
EQUIP_TYPE_GOODS = 3

# equipType -> (bucket name, the member its ids must exist in). The listing's
# own vanilla byte picks the row; nothing here is inferred from an id's value.
BUCKETS = {
    EQUIP_TYPE_WEAPON: ("weapon", WEAPON_MEMBER),
    EQUIP_TYPE_ARMOUR: ("armour", PROTECTOR_MEMBER),
    EQUIP_TYPE_GOODS: ("items", GOODS_MEMBER),
}

# SW-I14's churn bound. A selection-sampling permutation over a whole bucket
# either reshuffles it wholesale - about 5 of the 220 armour listings and 9 of
# the 397 item listings are expected to draw the item they already had, since 44
# distinct ids over 220 listings and 57 over 397 means each listing has about a
# 2% chance of drawing its own - or, astronomically unlikely, comes out as the
# identity. What it CANNOT do is leave most of the bucket holding its vanilla
# item: that is a half-applied pass, and this is the bound that says so.
DECLARED_CHURN_FRACTION = 0.5

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

# Feature 037's START WITH A TRICK WEAPON also lowers requirements, and spec 037
# D4 says the GRANT wins where the two features land on the same weapon. Feature
# 038's START WITH A LEFT WEAPON does the same thing with the same profile for
# the other hand, and inherits the same rule. When --granted <id> is passed -
# once per grant the run made - three invariants relax by exactly that much and
# no more: CharaInitParam.param may differ (SW-I1), each granted weapon and its
# ten tiers may be restatted (SW-I6), and if a granted weapon is ALSO a coffin
# pick then this profile is what that slot's weapon must carry (SW-I7).
#
# Without the flag nothing changes, which is the point: a run with neither grant
# on is still held to the invariants exactly as it was before.
#
# The flag is not per-hand, because to this tool the hand is invisible: it reads
# EquipParamWeapon, where a weapon is a weapon. Which field of CharaInitParam
# each grant wrote is trick_weapons_verify.py's and
# left_hand_weapons_verify.py's to police.
#
# GRANT_PROFILE is retyped here on purpose, the same way STAT_PROFILES is: it is
# the thing being checked, so parsing it out of the C++ would let both move
# together. That it is the real origin minimum is trick_weapons_verify.py T8's
# job, measured from CharaInitParam.
CHARA_MEMBER = "CharaInitParam.param"
GRANT_PROFILE = (9, 9, 5, 6)

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

    # Feature 011's two buckets beside the weapon pool. Ids and counts only,
    # never names: printing an item name on this console raises
    # UnicodeEncodeError that reads like a parse failure (docs/known-traps.md),
    # and this tool needs no FMG access at all.
    all_shop = rows_of(plain, members, SHOP_MEMBER)
    print()
    for etype in (EQUIP_TYPE_ARMOUR, EQUIP_TYPE_GOODS):
        name, member = BUCKETS[etype]
        listings = [i32(plain, at + SHOP_EQUIP_ID) for at in all_shop.values()
                    if plain[at + SHOP_EQUIP_TYPE] == etype]
        shufflable = [e for e in listings if e not in NEVER_TOUCHED]
        member_rows = set(rows_of(plain, members, member))
        missing_ids = sorted(set(shufflable) - member_rows)
        print("%s bucket: %d listings, %d shufflable, %d distinct   "
              "(%d skipped by the never-touch list)"
              % (name, len(listings), len(shufflable), len(set(shufflable)),
                 len(listings) - len(shufflable)))
        if missing_ids:
            print("  WARNING %d sold ids have no %s row: %s"
                  % (len(missing_ids), member, missing_ids))

    print("\nvanilla starting slots:")
    all_rows = rows_of(plain, members, SHOP_MEMBER)
    for rid in MELEE_SLOTS + GUN_SLOTS:
        if rid in all_rows:
            print("  %d -> %d" % (rid, i32(plain, all_rows[rid] + SHOP_EQUIP_ID)))
    return 0


def compare(van_plain, van_members, out_plain, out_members, granted=None,
            shop_armour=False, shop_items=False):
    failures = []
    notes = []
    # Which of feature 011's two buckets the run declared. equipType 0 is not in
    # here on purpose: row 5's buckets have no declaration flag, and this
    # feature must never write an equipType 0 listing at all.
    declared = {EQUIP_TYPE_ARMOUR: shop_armour, EQUIP_TYPE_GOODS: shop_items}
    # `granted` is one id, several ids, or None. Normalised once here so every
    # use below reads the same whether the run had one grant or two.
    if granted is None:
        grants = ()
    elif isinstance(granted, int):
        grants = (granted,)
    else:
        grants = tuple(granted)
    tolerated = {SHOP_MEMBER, WEAPON_MEMBER}
    if grants:
        tolerated.add(CHARA_MEMBER)

    # SW-I9 / SW-I1
    if sorted(van_members) != sorted(out_members):
        failures.append("SW-I9 archive member list changed")
        return failures, notes, {}

    for name, (voff, vsize) in van_members.items():
        ooff, osize = out_members[name]
        if vsize != osize:
            failures.append("SW-I9 member %s size changed %d -> %d" % (name, vsize, osize))
            continue
        if name in tolerated:
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

    # The id sets the three buckets' existence checks are made against, read from
    # the VANILLA tree and read-only. SW-I4 has always used the weapon one; the
    # other two are what makes an armour or consumable listing judgeable at all
    # (F13). Before this the weapon check ran on every listing and an armour id
    # failed it, so the tool was wrong in feature 011's presence.
    bucket_row_ids = {
        EQUIP_TYPE_WEAPON: set(van_weapons),
        EQUIP_TYPE_ARMOUR: set(rows_of(van_plain, van_members, PROTECTOR_MEMBER)),
        EQUIP_TYPE_GOODS: set(rows_of(van_plain, van_members, GOODS_MEMBER)),
    }

    # What each bucket held in vanilla: the multiset SW-I11 compares against and
    # the set SW-I12 tests membership in. Keyed by the VANILLA equipType byte,
    # and the three never-touched ids are excluded, because a listing selling one
    # is not part of any permutation - SW-I13 requires it to be untouched.
    van_bucket_ids = {t: [] for t in BUCKETS}
    for rid, vat in van_shop.items():
        vtype = van_plain[vat + SHOP_EQUIP_TYPE]
        v_id = i32(van_plain, vat + SHOP_EQUIP_ID)
        if vtype in van_bucket_ids and v_id not in NEVER_TOUCHED:
            van_bucket_ids[vtype].append(v_id)

    # --- ShopLineupParam: only equipId may move -----------------------------
    assigned = {}
    slot_changed = 0
    shop_changed = 0
    bucket_changed = {t: 0 for t in BUCKETS}
    for rid, vat in van_shop.items():
        oat = out_shop[rid]
        v_id = i32(van_plain, vat + SHOP_EQUIP_ID)
        o_id = i32(out_plain, oat + SHOP_EQUIP_ID)
        # The bucket comes from the VANILLA byte, so a listing whose equipType
        # was tampered with is still judged as what it was - and SW-I2 below
        # catches the tampering itself.
        vtype = van_plain[vat + SHOP_EQUIP_TYPE]

        if van_plain[vat + 4:vat + 32] != out_plain[oat + 4:oat + 32]:
            failures.append("SW-I2 shop row %d changed outside equipId" % rid)

        if v_id == o_id:
            continue

        # SW-I13, both halves: a protected listing is not a destination, and a
        # protected id is not a source. Applies to every bucket - the list is
        # one list, shared by both features through Randomizer/ShopLineup.h.
        if v_id in NEVER_TOUCHED:
            failures.append("SW-I13 row %d sold never-touched id %d and was changed to %d"
                            % (rid, v_id, o_id))
        if o_id in NEVER_TOUCHED:
            failures.append("SW-I13 row %d assigned a never-touch id %d" % (rid, o_id))

        if vtype in bucket_changed:
            bucket_changed[vtype] += 1

        # SW-I10 / SW-I4 - ONE existence check, dispatched on the bucket. Every
        # id lands in the table its own listing points at, and nothing is
        # inferred from how big the number is.
        if vtype in BUCKETS:
            _name, member = BUCKETS[vtype]
            if o_id not in bucket_row_ids[vtype]:
                code = "SW-I4" if vtype == EQUIP_TYPE_WEAPON else "SW-I10"
                failures.append("%s row %d assigned %d, which has no %s row"
                                % (code, rid, o_id, member))
        else:
            failures.append("SW-I12 row %d has equipType %d, which no feature writes, "
                            "but changed %d -> %d" % (rid, vtype, v_id, o_id))

        if vtype == EQUIP_TYPE_WEAPON:
            # Row 5's territory, and these three checks are scoped to it: an
            # armour or consumable id is not on either weapon list and does not
            # belong to a coffin slot.
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
        elif vtype in BUCKETS:
            # SW-I12: feature 011's two buckets never cross. The existence check
            # above cannot say this on its own - five goods ids also exist as
            # EquipParamProtector row ids - so membership of the listing's own
            # vanilla bucket is asserted separately.
            name, _member = BUCKETS[vtype]
            if o_id not in set(van_bucket_ids[vtype]):
                failures.append("SW-I12 row %d is a %s listing but was assigned %d, "
                                "which no %s listing sold in vanilla"
                                % (rid, name, o_id, name))

    # --- SW-I11 / SW-I14: feature 011's two buckets -------------------------
    for vtype in (EQUIP_TYPE_ARMOUR, EQUIP_TYPE_GOODS):
        name, _member = BUCKETS[vtype]
        want = sorted(van_bucket_ids[vtype])
        got = []
        for rid, vat in van_shop.items():
            if van_plain[vat + SHOP_EQUIP_TYPE] != vtype:
                continue
            if i32(van_plain, vat + SHOP_EQUIP_ID) in NEVER_TOUCHED:
                continue
            got.append(i32(out_plain, out_shop[rid] + SHOP_EQUIP_ID))
        if want != sorted(got):
            failures.append("SW-I11 the %s bucket is not a permutation of vanilla "
                            "(%d listings, %d distinct in vanilla, %d in output)"
                            % (name, len(want), len(set(want)), len(set(got))))

        changed = bucket_changed[vtype]
        if not declared[vtype]:
            if changed:
                failures.append("SW-I14 %d of the %d %s listings changed, but the run "
                                "did not declare that bucket" % (changed, len(want), name))
        elif changed and changed < DECLARED_CHURN_FRACTION * len(want):
            failures.append("SW-I14 only %d of the %d declared %s listings changed - a "
                            "whole-bucket permutation cannot leave that many holding "
                            "their vanilla item" % (changed, len(want), name))
        elif declared[vtype]:
            notes.append("%s bucket declared: %d of %d listings changed"
                         % (name, changed, len(want)))

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
    for g in grants:
        for tier in range(UPGRADE_TIERS + 1):
            allowed.add(g + tier * UPGRADE_STRIDE)

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

    # SW-I7: the profile actually landed, on every tier that exists. A weapon
    # that is both a coffin pick and the grant carries the GRANT's profile - D4,
    # and it holds whichever of the two passes ran first.
    for rid, weapon in assigned.items():
        want = GRANT_PROFILE if weapon in grants else STAT_PROFILES[rid]
        for tier in range(UPGRADE_TIERS + 1):
            wid = weapon + tier * UPGRADE_STRIDE
            if wid not in out_weapons:
                continue
            at = out_weapons[wid]
            got = tuple(out_plain[at + o] for o in STAT_OFFSETS)
            if got != want:
                failures.append("SW-I7 slot %d weapon %d tier %d has stats %s, expected %s"
                                % (rid, weapon, tier, got, want))

    for g in grants:
        wrong_rows = []
        for tier in range(UPGRADE_TIERS + 1):
            wid = g + tier * UPGRADE_STRIDE
            if wid not in out_weapons:
                continue
            at = out_weapons[wid]
            if tuple(out_plain[at + o] for o in STAT_OFFSETS) != GRANT_PROFILE:
                wrong_rows.append(wid)
        if wrong_rows:
            failures.append("SW-I7 granted weapon %d does not carry %s on rows %s"
                            % (g, GRANT_PROFILE, wrong_rows[:6]))
        notes.append("tolerating a character-creation grant of weapon %d" % g)

    if assigned:
        notes.append("slot assignments: " +
                     ", ".join("%d->%d" % (r, w) for r, w in sorted(assigned.items())))

    # "shop" is row 5's churn figure and counts equipType 0 listings ONLY - it
    # was every changed non-slot listing before feature 011 existed, which would
    # have folded the two features' counts into one number.
    counts = {"slots": slot_changed, "shop": shop_changed, "stat_rows": stat_rows,
              "armour": bucket_changed[EQUIP_TYPE_ARMOUR],
              "items": bucket_changed[EQUIP_TYPE_GOODS]}
    return failures, notes, counts


def cmd_verify(vanilla_root, output_root, granted=None, shop_armour=False,
               shop_items=False):
    opath = os.path.join(output_root, REL)
    if not os.path.exists(opath):
        print("no item-data archive in output tree - expected when every param toggle was off")
        return 0
    van_plain, van_members = load_archive(vanilla_root)
    out_plain, out_members = load_archive(output_root)

    print("vanilla decompressed=%d  output decompressed=%d" % (len(van_plain), len(out_plain)))
    print("declared: %s" % (", ".join(
        [n for n, on in (("--shop-armour", shop_armour), ("--shop-items", shop_items))
         if on] + ["--granted %d" % g for g in (granted or ())]) or "nothing"))
    failures, notes, counts = compare(van_plain, van_members, out_plain, out_members,
                                      granted=granted, shop_armour=shop_armour,
                                      shop_items=shop_items)
    if counts:
        print("starting slots changed: %d   other shop weapon rows changed: %d   "
              "weapon rows restatted: %d"
              % (counts["slots"], counts["shop"], counts["stat_rows"]))
        print("shop armour listings changed: %d   shop item listings changed: %d"
              % (counts["armour"], counts["items"]))
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

    # --- feature 037's --granted tolerance -----------------------------------
    #
    # Every case above ran WITHOUT the flag and still holds, which is one half of
    # "passes with and without it". These are the other half: with the flag a
    # combined run passes, without it the same bytes are rejected, and the flag
    # is not a blanket amnesty.
    chara_off, _ = van_members[CHARA_MEMBER]
    grant_weapon = next(w for w in sorted(weapons)
                        if all(w < pk or w > pk + UPGRADE_TIERS * UPGRADE_STRIDE
                               for pk in picks.values()))

    def apply_grant(buf, weapon):
        for tier in range(UPGRADE_TIERS + 1):
            wid = weapon + tier * UPGRADE_STRIDE
            if wid not in weapons:
                continue
            for off, val in zip(STAT_OFFSETS, GRANT_PROFILE):
                buf[weapons[wid] + off] = val

    combined = bytearray(good)
    apply_grant(combined, grant_weapon)
    # Stands in for the grant's writes into CharaInitParam. This tool models
    # those no further than "that member is allowed to differ";
    # trick_weapons_verify.py owns the other half.
    combined[chara_off + 0x100] ^= 0xFF

    fc, _, _ = compare(van_plain, van_members, combined, van_members,
                       granted=grant_weapon)
    results.append(("037: a combined run passes WITH --granted", not fc, fc))

    fn, _, _ = compare(van_plain, van_members, combined, van_members)
    results.append(("037: the same bytes are REJECTED without --granted",
                    any("SW-I1" in x for x in fn) and any("SW-I6" in x for x in fn), []))

    wrong = bytearray(combined)
    other_weapon = next(w for w in sorted(weapons)
                        if all(w < pk or w > pk + UPGRADE_TIERS * UPGRADE_STRIDE
                               for pk in list(picks.values()) + [grant_weapon]))
    wrong[weapons[other_weapon] + PROPER_STRENGTH] = 99
    fw, _, _ = compare(van_plain, van_members, wrong, van_members, granted=grant_weapon)
    results.append(("037: --granted still rejects an unrelated restatted weapon",
                    any("SW-I6" in x for x in fw), []))

    # Where the grant and a coffin slot share a weapon, SW-I7 wants the GRANT's
    # profile - D4 stated as a test rather than as a comment.
    shared = bytearray(van_plain)
    for rid, w in picks.items():
        assign(shared, rid, w)
    apply_grant(shared, picks[2000])
    fs, _, _ = compare(van_plain, van_members, shared, van_members, granted=picks[2000])
    results.append(("037: D4 - a shared weapon carrying the GRANT profile passes",
                    not fs, fs))

    slot_wins = bytearray(van_plain)
    for rid, w in picks.items():
        assign(slot_wins, rid, w)
    fsl, _, _ = compare(van_plain, van_members, slot_wins, van_members,
                        granted=picks[2000])
    results.append(("037: D4 - the coffin profile winning instead is REJECTED",
                    any("SW-I7" in x for x in fsl), []))

    # --- feature 038: two grants in one run ----------------------------------
    #
    # Both character-creation grants can be on at once, so --granted has to be
    # repeatable. These four say that it is, and that repeating it is not a
    # blanket amnesty either.
    left_weapon = next(w for w in sorted(weapons)
                       if all(w < pk or w > pk + UPGRADE_TIERS * UPGRADE_STRIDE
                              for pk in list(picks.values()) + [grant_weapon]))
    two = bytearray(good)
    apply_grant(two, grant_weapon)
    apply_grant(two, left_weapon)
    two[chara_off + 0x100] ^= 0xFF

    f2, _, _ = compare(van_plain, van_members, two, van_members,
                       granted=[grant_weapon, left_weapon])
    results.append(("038: two grants in one run pass with --granted given twice",
                    not f2, f2))

    f2one, _, _ = compare(van_plain, van_members, two, van_members,
                          granted=[grant_weapon])
    results.append(("038: the same bytes are REJECTED with only one id named",
                    any("SW-I6" in x for x in f2one), []))

    # A single int must keep behaving exactly as it did before the flag became
    # repeatable - that is what makes every case above still meaningful.
    f1, _, _ = compare(van_plain, van_members, combined, van_members,
                       granted=grant_weapon)
    f1list, _, _ = compare(van_plain, van_members, combined, van_members,
                           granted=[grant_weapon])
    results.append(("038: one id as an int and as a list are the same check",
                    f1 == f1list and not f1, (f1, f1list)))

    wrong2 = bytearray(two)
    unrelated2 = next(w for w in sorted(weapons)
                      if all(w < pk or w > pk + UPGRADE_TIERS * UPGRADE_STRIDE
                             for pk in list(picks.values()) + [grant_weapon,
                                                              left_weapon]))
    wrong2[weapons[unrelated2] + PROPER_STRENGTH] = 99
    fw2, _, _ = compare(van_plain, van_members, wrong2, van_members,
                        granted=[grant_weapon, left_weapon])
    results.append(("038: two ids still reject a THIRD restatted weapon",
                    any("SW-I6" in x for x in fw2), []))

    # --- feature 011: the two shop-stock declarations ------------------------
    #
    # S1-S11 of plan 011 §6. Every case above ran with neither flag and still
    # holds, which is half of "an unmodified archive passes with no flags". These
    # simulate output in Python - a seeded selection-sampling permutation over
    # each bucket, the same shape ShopStock.cpp performs - and then break it one
    # way at a time. Nothing here predicts WHICH item a seed puts in a listing;
    # that is a hardware check.
    def bucket_targets(etype):
        """[(row id, absolute offset)] for one bucket's shufflable listings."""
        out = []
        for rid, at in sorted(shop.items()):
            if van_plain[at + SHOP_EQUIP_TYPE] != etype:
                continue
            if i32(van_plain, at + SHOP_EQUIP_ID) in NEVER_TOUCHED:
                continue
            out.append((rid, at))
        return out

    armour_targets = bucket_targets(EQUIP_TYPE_ARMOUR)
    item_targets = bucket_targets(EQUIP_TYPE_GOODS)

    def permute(buf, etype, seed):
        """Selection sampling, exactly ShopStock.cpp's loop, over one bucket."""
        rng = random.Random(seed)
        targets = bucket_targets(etype)
        pool = [i32(van_plain, at + SHOP_EQUIP_ID) for _rid, at in targets]
        for _rid, at in targets:
            set_i32(buf, at + SHOP_EQUIP_ID, pool.pop(rng.randrange(len(pool))))

    def ids_at(buf, targets):
        return sorted(i32(buf, at + SHOP_EQUIP_ID) for _rid, at in targets)

    def check(label, buf, expect, armour=False, items=False, granted=None):
        """expect None -> must pass; a code string -> must be rejected for it."""
        f, _, _ = compare(van_plain, van_members, buf, van_members, granted=granted,
                          shop_armour=armour, shop_items=items)
        ok = (not f) if expect is None else any(expect in x for x in f)
        results.append((label, ok, f))

    # S1 - an identity permutation is a permutation.
    check("011 S1: an unmodified archive passes with BOTH flags",
          bytearray(van_plain), None, armour=True, items=True)

    # S2 / S3 - one bucket at a time, declared and undeclared.
    armour_only = bytearray(van_plain)
    permute(armour_only, EQUIP_TYPE_ARMOUR, 11)
    check("011 S2: an armour-only permutation passes with --shop-armour",
          armour_only, None, armour=True)
    check("011 S2: the same bytes are REJECTED without it",
          armour_only, "SW-I14")
    check("011 S2: and declaring only --shop-items does not cover it",
          armour_only, "SW-I14", items=True)

    items_only = bytearray(van_plain)
    permute(items_only, EQUIP_TYPE_GOODS, 12)
    check("011 S3: an items-only permutation passes with --shop-items",
          items_only, None, items=True)
    check("011 S3: the same bytes are REJECTED without it",
          items_only, "SW-I14")

    # S4 - both at once, and the multisets checked here as well as by SW-I11.
    both = bytearray(van_plain)
    permute(both, EQUIP_TYPE_ARMOUR, 11)
    permute(both, EQUIP_TYPE_GOODS, 12)
    check("011 S4: both buckets permuted pass with both flags",
          both, None, armour=True, items=True)
    results.append(("011 S4: each bucket's %d and %d output ids equal vanilla's"
                    % (len(armour_targets), len(item_targets)),
                    ids_at(both, armour_targets) == ids_at(van_plain, armour_targets) and
                    ids_at(both, item_targets) == ids_at(van_plain, item_targets), []))

    # S5 - the buckets crossing. Five goods ids also exist as
    # EquipParamProtector row ids, so the existence check alone cannot say this.
    crossed = bytearray(both)
    set_i32(crossed, armour_targets[0][1] + SHOP_EQUIP_ID,
            i32(both, item_targets[0][1] + SHOP_EQUIP_ID))
    set_i32(crossed, item_targets[0][1] + SHOP_EQUIP_ID,
            i32(both, armour_targets[0][1] + SHOP_EQUIP_ID))
    check("011 S5: an armour and a consumable listing trading ids is REJECTED",
          crossed, "SW-I12", armour=True, items=True)

    # S6 - an id with no row in the bucket's own table.
    fabricated_armour = bytearray(both)
    set_i32(fabricated_armour, armour_targets[1][1] + SHOP_EQUIP_ID, 987654321)
    check("011 S6: an armour listing given an id with no EquipParamProtector row",
          fabricated_armour, "SW-I10", armour=True, items=True)
    fabricated_item = bytearray(both)
    set_i32(fabricated_item, item_targets[1][1] + SHOP_EQUIP_ID, 987654321)
    check("011 S6: a consumable listing given an id with no EquipParamGoods row",
          fabricated_item, "SW-I10", armour=True, items=True)

    # S7 - the three never-touched ids, in both directions. These 27 listings are
    # the ones whose loss would end a playthrough.
    vial_rows = [(rid, at) for rid, at in sorted(shop.items())
                 if i32(van_plain, at + SHOP_EQUIP_ID) == 1000]
    results.append(("011 S7: the vanilla tree really has Blood Vial listings to protect",
                    len(vial_rows) >= 2, []))
    swapped_vials = bytearray(both)
    for (_vrid, vat), (_grid, gat) in zip(vial_rows[:2], item_targets[:2]):
        keep = i32(both, gat + SHOP_EQUIP_ID)
        set_i32(swapped_vials, gat + SHOP_EQUIP_ID, 1000)
        set_i32(swapped_vials, vat + SHOP_EQUIP_ID, keep)
    check("011 S7: trading two Blood Vial listings' ids away is REJECTED",
          swapped_vials, "SW-I13", armour=True, items=True)
    for protected in sorted(NEVER_TOUCHED):
        planted = bytearray(both)
        set_i32(planted, item_targets[2][1] + SHOP_EQUIP_ID, protected)
        check("011 S7: writing never-touched id %d into a listing is REJECTED" % protected,
              planted, "SW-I13", armour=True, items=True)

    # S8 - anything but equipId, in a bucket the run DID declare. The equipType
    # case above covers the byte the dispatch reads; this is one of the others.
    price_touched = bytearray(both)
    price_touched[item_targets[3][1] + 4] ^= 0xFF
    check("011 S8: a byte of a listing outside equipId is REJECTED even when declared",
          price_touched, "SW-I2", armour=True, items=True)
    type_touched = bytearray(both)
    type_touched[armour_targets[2][1] + SHOP_EQUIP_TYPE] = 9
    check("011 S8: so is equipType, which is what the bucket dispatch reads",
          type_touched, "SW-I2", armour=True, items=True)

    # S9 - the two new members are READ, never written.
    for member in (PROTECTOR_MEMBER, GOODS_MEMBER):
        moff, _msize = van_members[member]
        touched = bytearray(both)
        touched[moff:moff + 4] = b"\xDE\xAD\xBE\xEF"
        check("011 S9: one changed byte of %s is REJECTED" % member,
              touched, "SW-I1", armour=True, items=True)

    # S10 - a declared bucket is untouched or wholesale-changed, never partly
    # permuted. About 5 of 220 and 9 of 397 listings are EXPECTED to hold their
    # vanilla item, so reverting one more must not fail.
    # Reverted by SWAPPING rather than by overwriting: a listing that holds its
    # vanilla item inside a permutation is one extra fixed point, not a missing
    # id, and SW-I11 would rightly reject the latter.
    one_fixed = bytearray(both)
    for targets in (armour_targets, item_targets):
        _rid, at = targets[0]
        want = i32(van_plain, at + SHOP_EQUIP_ID)
        if i32(one_fixed, at + SHOP_EQUIP_ID) == want:
            continue
        other = next(o for _r, o in targets
                     if i32(one_fixed, o + SHOP_EQUIP_ID) == want)
        set_i32(one_fixed, other + SHOP_EQUIP_ID, i32(one_fixed, at + SHOP_EQUIP_ID))
        set_i32(one_fixed, at + SHOP_EQUIP_ID, want)
    check("011 S10: one listing per bucket holding its vanilla item still passes",
          one_fixed, None, armour=True, items=True)
    # A genuine permutation of the armour bucket that only moves the last 20
    # listings - so SW-I11 holds and SW-I14 is the only thing left to catch it.
    # That is what a half-applied pass looks like from the outside.
    partial = bytearray(van_plain)
    tail = armour_targets[200:]
    tail_pool = [i32(van_plain, at + SHOP_EQUIP_ID) for _rid, at in tail]
    tail_rng = random.Random(13)
    for _rid, at in tail:
        set_i32(partial, at + SHOP_EQUIP_ID, tail_pool.pop(tail_rng.randrange(len(tail_pool))))
    check("011 S10: only 20 of the 220 armour listings moving is REJECTED",
          partial, "SW-I14", armour=True, items=True)

    # S11 - row 5, row 11 and a grant in one archive, which is the shape of the
    # single hardware run plan 011 D2 takes. Then the same archive with one of
    # row 5's own rules broken, to show the three declarations are not an
    # amnesty for each other.
    all_on = bytearray(good)
    apply_grant(all_on, grant_weapon)
    all_on[chara_off + 0x100] ^= 0xFF
    permute(all_on, EQUIP_TYPE_ARMOUR, 11)
    permute(all_on, EQUIP_TYPE_GOODS, 12)
    check("011 S11: row 5 + row 11 + a grant passes with all three declarations",
          all_on, None, armour=True, items=True, granted=grant_weapon)
    all_on_broken = bytearray(all_on)
    set_i32(all_on_broken, shop[2000] + SHOP_EQUIP_ID, gun_pool[0])
    check("011 S11: row 5's melee rule still fails on that same archive",
          all_on_broken, "SW-I3", armour=True, items=True, granted=grant_weapon)

    passed = 0
    for label, ok, detail in results:
        print("  %-58s %s" % (label, "OK" if ok else "MISSED"))
        if not ok:
            print("     failures were: %s" % (detail[:3] or "none at all"))
        else:
            passed += 1
    print("\nselftest %d/%d" % (passed, len(results)))
    return 0 if passed == len(results) else 1


def main():
    args = list(sys.argv[1:])
    # Repeatable: one occurrence per grant the run made. A single occurrence
    # still behaves exactly as it did before feature 038 existed.
    granted = []
    while "--granted" in args:
        i = args.index("--granted")
        granted.append(int(args[i + 1]))
        del args[i:i + 2]
    # Feature 011's two declarations. Order-independent and combinable with
    # --granted in any arrangement, because each is removed before the
    # positional arguments are read.
    shop_armour = "--shop-armour" in args
    if shop_armour:
        args.remove("--shop-armour")
    shop_items = "--shop-items" in args
    if shop_items:
        args.remove("--shop-items")
    if len(args) >= 2 and args[0] == "pool":
        return cmd_pool(args[1])
    if len(args) >= 3 and args[0] == "verify":
        return cmd_verify(args[1], args[2], granted=granted,
                          shop_armour=shop_armour, shop_items=shop_items)
    if len(args) >= 2 and args[0] == "selftest":
        return cmd_selftest(args[1])
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main())
