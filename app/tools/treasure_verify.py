#!/usr/bin/env python3
"""Treasure randomization verification tooling.

A PROPERTY VALIDATOR, per docs/plans/treasure-randomization.md §3. It never
re-derives which item should land where - it checks invariants of real
generated output against real vanilla input, so a rules misreading shared with
the C++ still cannot make a fabricated item lot exist in vanilla data or make
the pool accounting balance.

MSBB parsing is imported from boss_verify.py rather than duplicated.

Usage:
    python treasure_verify.py pool     <vanilla_dvdroot> [--workshop]
    python treasure_verify.py verify   <vanilla_dvdroot> <output_dvdroot> [--workshop]
    python treasure_verify.py selftest <vanilla_dvdroot>

--workshop means the run being checked had RANDOMIZE WORKSHOP TOOLS on, i.e.
the two workshop-tool item lots (docs/plans/workshop-tools.md) were NOT
protected. Omitting it (the default) checks a run with the setting off.
"""

import copy
import os
import sys
from collections import Counter

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from boss_verify import read_dcx, Msbb, _i32, _i64, _utf16z  # noqa: E402

# --- Event / Treasure layout (spec §4; mirrors src/Msb/Msbb.cpp) -----------

EV_NAME_OFFSET      = 0x00
EV_ENTITY_ID        = 0x08
EV_TYPE             = 0x0C
EV_TYPE_DATA_OFFSET = 0x20

EVENT_TYPE_TREASURE = 0x4
TREASURE_ITEMLOT1   = 0x10
TREASURE_ITEMLOT2   = 0x14
TREASURE_ITEMLOT3   = 0x18

# --- Reference rules (spec §2.1, §2.4) -------------------------------------

BASE_PROTECTED = [2600550, 2400450, 3500800]
KEY_ITEM_LOTS  = [2800290, 3200720, 3200810, 2410990, 3502000, 3401810]
WORKSHOP_LOTS  = [2411000, 2200360]

# m21_00_00_00 absent (commented out in the reference); m21_01_00_00 twice.
TREASURE_MAP_ORDER = [
    "m21_01_00_00", "m21_01_00_00",
    "m22_00_00_00", "m23_00_00_00", "m23_00_00_01",
    "m24_00_00_00", "m24_00_00_01",
    "m24_01_00_00", "m24_01_00_01", "m24_01_00_11",
    "m24_02_00_00", "m24_02_00_01",
    "m25_00_00_00", "m26_00_00_00",
    "m27_00_00_00", "m27_00_00_01",
    "m28_00_00_00", "m28_00_00_01",
    "m32_00_00_00", "m32_00_00_01",
    "m33_00_00_00", "m34_00_00_00", "m35_00_00_00", "m36_00_00_00",
]
DISTINCT_MAPS = sorted(set(TREASURE_MAP_ORDER))
CONTROL_MAP = "m21_00_00_00"  # never touched by the reference - spec §2.4

DUPLICATED_MAP = "m21_01_00_00"
# spec I11, measured against real vanilla data (workshop-tools.md §2.3):
# turning workshop tools on adds the 4 placements (§2.1) to the eligible set.
EXPECTED_ELIGIBLE = {False: 1041, True: 1045}


def protected(lot, randomize_workshop_tools=False):
    if lot in BASE_PROTECTED or lot in KEY_ITEM_LOTS:
        return True
    return (not randomize_workshop_tools) and lot in WORKSHOP_LOTS


def treasures(msbb):
    """Yields (index, entity_id, name, lot1, lot2, lot3, type)."""
    for i, blob in enumerate(msbb.events.entries):
        etype = int.from_bytes(blob[EV_TYPE:EV_TYPE + 4], "little")
        if etype != EVENT_TYPE_TREASURE:
            continue
        td = _i64(blob, EV_TYPE_DATA_OFFSET)
        yield (i,
               _i32(blob, EV_ENTITY_ID),
               _utf16z(blob, _i64(blob, EV_NAME_OFFSET)),
               _i32(blob, td + TREASURE_ITEMLOT1),
               _i32(blob, td + TREASURE_ITEMLOT2),
               _i32(blob, td + TREASURE_ITEMLOT3),
               etype)


def load(root, names):
    maps = {}
    for n in names:
        p = os.path.join(root, "map", "mapstudio", n + ".msb.dcx")
        if os.path.exists(p):
            maps[n] = Msbb(read_dcx(p))
    return maps


def build_pool(van_maps, randomize_workshop_tools=False):
    """Spec §2.2 pool rule (lot > 1), walked in TREASURE_MAP_ORDER so the
    duplicated map contributes twice - exactly as the reference does."""
    pool = []
    for name in TREASURE_MAP_ORDER:
        m = van_maps.get(name)
        if m is None:
            continue
        for _, _, _, lot1, _, _, _ in treasures(m):
            if protected(lot1, randomize_workshop_tools):
                continue
            if lot1 > 1:
                pool.append(lot1)
    return pool


def eligible_locations(van_maps, randomize_workshop_tools=False):
    """Spec §2.2 location rule (lot > 0), one entry per distinct slot."""
    out = {}
    for name in DISTINCT_MAPS:
        m = van_maps.get(name)
        if m is None:
            continue
        for idx, ent, _, lot1, _, _, _ in treasures(m):
            if protected(lot1, randomize_workshop_tools):
                continue
            if lot1 > 0:
                out[(name, idx)] = (ent, lot1)
    return out


# --- Commands ---------------------------------------------------------------

def cmd_pool(vanilla_root, randomize_workshop_tools=False):
    van = load(vanilla_root, DISTINCT_MAPS + [CONTROL_MAP])
    print("loaded %d/%d treasure maps (+control %s: %s)" %
          (len([m for m in DISTINCT_MAPS if m in van]), len(DISTINCT_MAPS),
           CONTROL_MAP, "yes" if CONTROL_MAP in van else "MISSING"))

    pool = build_pool(van, randomize_workshop_tools)
    locs = eligible_locations(van, randomize_workshop_tools)
    dup = van.get(DUPLICATED_MAP)
    dup_eligible = 0
    if dup:
        dup_eligible = sum(1 for _, _, _, l, _, _, _ in treasures(dup)
                           if l > 1 and not protected(l, randomize_workshop_tools))

    ok = True
    print("\n=== V0: data assumptions (workshop tools %s) ===" %
          ("ON" if randomize_workshop_tools else "OFF"))
    total_tr = sum(sum(1 for _ in treasures(m)) for n, m in van.items() if n in DISTINCT_MAPS)
    print("  total Treasure events (type 0x4) : %d" % total_tr)
    print("  eligible locations (lot > 0)     : %d" % len(locs))
    print("  pool entries (lot > 1, dup pass) : %d" % len(pool))
    print("  %s contributes twice        : %d entries" % (DUPLICATED_MAP, dup_eligible))

    expected = EXPECTED_ELIGIBLE[randomize_workshop_tools]
    if len(locs) != expected:
        print("  FAIL  expected %d eligible locations, got %d" % (expected, len(locs)))
        ok = False
    else:
        print("  PASS  eligible-location count matches the documented %d" % expected)

    if len(pool) != len(locs) + dup_eligible:
        print("  FAIL  pool (%d) != locations (%d) + duplicate pass (%d)"
              % (len(pool), len(locs), dup_eligible))
        ok = False
    else:
        print("  PASS  pool balances locations + the duplicated map pass")

    ones = [v for v in locs.values() if v[1] == 1]
    print("  %s  lot == 1 treasures: %d (the >1 / >0 asymmetry is %s)"
          % ("PASS" if not ones else "NOTE", len(ones),
             "inert" if not ones else "LIVE - revisit spec 2.2"))

    if CONTROL_MAP in van:
        ctrl = list(treasures(van[CONTROL_MAP]))
        print("  control map %s: %d treasures, lots %s"
              % (CONTROL_MAP, len(ctrl), [t[3] for t in ctrl]))

    print("\n  distinct pool values: %d" % len(set(pool)))
    print("\nV0 %s" % ("PASSED" if ok else "FAILED"))
    return 0 if ok else 1


def compare(van_maps, out_maps, randomize_workshop_tools=False):
    """Returns (failures, notes, changed_count)."""
    failures = []
    notes = []
    changed = 0

    pool = Counter(build_pool(van_maps, randomize_workshop_tools))
    locs = eligible_locations(van_maps, randomize_workshop_tools)
    result_multiset = Counter()

    all_names = DISTINCT_MAPS + [CONTROL_MAP]
    for name in all_names:
        van, out = van_maps.get(name), out_maps.get(name)
        if van is None or out is None:
            continue

        v_events = van.events.entries
        o_events = out.events.entries
        if len(v_events) != len(o_events):
            failures.append("%s: I8 event count changed %d -> %d"
                            % (name, len(v_events), len(o_events)))
            continue

        # I1/I7: no non-Treasure event may differ at all.
        for i, (vb, ob) in enumerate(zip(v_events, o_events)):
            vtype = int.from_bytes(vb[EV_TYPE:EV_TYPE + 4], "little")
            otype = int.from_bytes(ob[EV_TYPE:EV_TYPE + 4], "little")
            if vtype != otype:
                failures.append("%s event %d: I1 event type changed %d -> %d"
                                % (name, i, vtype, otype))
            elif vtype != EVENT_TYPE_TREASURE and bytes(vb) != bytes(ob):
                failures.append("%s event %d: I7 non-Treasure event modified (type %d)"
                                % (name, i, vtype))

        v_tr = {t[0]: t for t in treasures(van)}
        o_tr = {t[0]: t for t in treasures(out)}

        for idx, vt in v_tr.items():
            ot = o_tr.get(idx)
            if ot is None:
                failures.append("%s: I8 treasure at event %d disappeared" % (name, idx))
                continue
            _, vent, vname, vlot1, vlot2, vlot3, _ = vt
            _, oent, oname, olot1, olot2, olot3, _ = ot

            # I2: identity fields and the untouched lots must not move.
            if (vent, vname, vlot2, vlot3) != (oent, oname, olot2, olot3):
                failures.append("%s/%s: I2 field other than ItemLot1 changed "
                                "(entity %d->%d, lot2 %d->%d, lot3 %d->%d)"
                                % (name, vname, vent, oent, vlot2, olot2, vlot3, olot3))

            is_eligible = (name, idx) in locs

            if name == CONTROL_MAP:
                if vlot1 != olot1:
                    failures.append("%s/%s: I6 control map treasure changed %d -> %d"
                                    % (name, vname, vlot1, olot1))
                continue

            if not is_eligible:
                if vlot1 != olot1:
                    failures.append("%s/%s: I5 protected/ineligible treasure changed %d -> %d"
                                    % (name, vname, vlot1, olot1))
                continue

            result_multiset[olot1] += 1
            if vlot1 != olot1:
                changed += 1

            # I5: a protected value must never be placed anywhere either.
            if protected(olot1, randomize_workshop_tools):
                failures.append("%s/%s: I5 protected lot %d placed into a randomized slot"
                                % (name, vname, olot1))

            # I3: never fabricate an item lot.
            if pool[olot1] == 0:
                failures.append("%s/%s: I3 resulting lot %d is not a vanilla pool value"
                                % (name, vname, olot1))

    if changed == 0:
        notes.append("no treasure changed - this is I10 (disabled) passing, "
                     "or treasure randomization did not run")
        return failures, notes, changed

    # I4: the result must be the pool minus exactly the entries overwritten by
    # the duplicated map's second pass. Spec 2.4: the reference randomizes
    # m21_01_00_00 twice, so that many draws are consumed and discarded.
    dup_slots = sum(1 for (n, _) in locs if n == DUPLICATED_MAP)
    surplus = pool - result_multiset
    deficit = result_multiset - pool

    if deficit:
        failures.append("I4 output contains lots beyond the pool's supply: %s"
                        % dict(list(deficit.items())[:6]))
    surplus_total = sum(surplus.values())
    if surplus_total != dup_slots:
        failures.append("I4 pool accounting off: %d pool entries unplaced, expected exactly "
                        "%d (the %s second-pass discards)"
                        % (surplus_total, dup_slots, DUPLICATED_MAP))
    else:
        notes.append("I4 pool accounting exact: %d unplaced entries == %s's %d slots "
                     "randomized twice" % (surplus_total, DUPLICATED_MAP, dup_slots))

    if sum(result_multiset.values()) != len(locs):
        failures.append("I4 placed %d values across %d eligible slots"
                        % (sum(result_multiset.values()), len(locs)))

    return failures, notes, changed


def cmd_verify(vanilla_root, output_root, randomize_workshop_tools=False):
    names = DISTINCT_MAPS + [CONTROL_MAP]
    van = load(vanilla_root, names)
    out = load(output_root, names)
    failures, notes, changed = compare(van, out, randomize_workshop_tools)

    print("compared %d maps (workshop tools %s); %d treasure pickups changed" %
          (len(out), "ON" if randomize_workshop_tools else "OFF", changed))
    for n in notes:
        print("  NOTE: " + n)
    if failures:
        print("\n%d FAILURES:" % len(failures))
        for f in failures[:60]:
            print("  " + f)
        if len(failures) > 60:
            print("  ... and %d more" % (len(failures) - 60))
        return 1
    print("\nAll treasure property checks PASSED")
    return 0


# --- selftest ---------------------------------------------------------------

def _set_lot1(blob, value):
    td = _i64(blob, EV_TYPE_DATA_OFFSET)
    blob[td + TREASURE_ITEMLOT1: td + TREASURE_ITEMLOT1 + 4] = \
        int(value).to_bytes(4, "little", signed=True)


def _set_lot2(blob, value):
    td = _i64(blob, EV_TYPE_DATA_OFFSET)
    blob[td + TREASURE_ITEMLOT2: td + TREASURE_ITEMLOT2 + 4] = \
        int(value).to_bytes(4, "little", signed=True)


def _first_treasure_idx(msbb, predicate):
    for idx, ent, name, l1, l2, l3, _ in treasures(msbb):
        if predicate(l1):
            return idx
    return None


def cmd_selftest(vanilla_root):
    """Corrupt a known-good tree in specific ways; each must be caught."""
    names = DISTINCT_MAPS + [CONTROL_MAP]
    base = load(vanilla_root, names)
    if not base:
        print("no maps loaded from %s" % vanilla_root)
        return 2

    results = []

    # A clean tree must be silent.
    f, _, _ = compare(base, copy.deepcopy(base))
    results.append(("unmodified tree", not f, f))

    # I3: fabricate an item lot.
    m = copy.deepcopy(base)
    tgt = m["m24_01_00_01"]
    idx = _first_treasure_idx(tgt, lambda l: l > 1 and not protected(l))
    _set_lot1(tgt.events.entries[idx], 777777777)
    f, _, _ = compare(base, m)
    results.append(("fabricated item lot", any("I3" in x for x in f), f))

    # I5: move a protected key item.
    m = copy.deepcopy(base)
    tgt = m["m24_01_00_01"]
    idx = _first_treasure_idx(tgt, lambda l: protected(l))
    if idx is not None:
        _set_lot1(tgt.events.entries[idx], 2410100)
        f, _, _ = compare(base, m)
        results.append(("protected lot moved", any("I5" in x for x in f), f))

    # Workshop-tools-plan.md §5.1: pin the --workshop flag's actual meaning in
    # both directions. Swap (not overwrite) a workshop lot with an ordinary
    # eligible lot so the pool/location multiset stays balanced either way -
    # an overwrite would trip unrelated I3/I4 accounting failures and make
    # the "no failure" side of this check meaningless.
    m = copy.deepcopy(base)
    tgt = m["m24_01_00_01"]
    ws_idx = ws_lot = other_idx = other_lot = None
    for idx, ent, name, l1, l2, l3, _ in treasures(tgt):
        if l1 in WORKSHOP_LOTS and ws_idx is None:
            ws_idx, ws_lot = idx, l1
        elif l1 > 1 and l1 not in WORKSHOP_LOTS and not protected(l1) and other_idx is None:
            other_idx, other_lot = idx, l1
    if ws_idx is not None and other_idx is not None:
        _set_lot1(tgt.events.entries[ws_idx], other_lot)
        _set_lot1(tgt.events.entries[other_idx], ws_lot)

        f, _, _ = compare(base, m, False)
        results.append(("workshop lot swapped, --workshop OFF -> I5", any("I5" in x for x in f), f))

        f, _, _ = compare(base, m, True)
        results.append(("workshop lot swapped, --workshop ON -> no failure", not f, f))

    # I6: touch the control map.
    m = copy.deepcopy(base)
    tgt = m[CONTROL_MAP]
    idx = _first_treasure_idx(tgt, lambda l: l > 0)
    _set_lot1(tgt.events.entries[idx], 2410100)
    f, _, _ = compare(base, m)
    results.append(("control map touched", any("I6" in x for x in f), f))

    # I2: change ItemLot2, which the randomizer must never write.
    m = copy.deepcopy(base)
    tgt = m["m24_01_00_01"]
    idx = _first_treasure_idx(tgt, lambda l: l > 1 and not protected(l))
    _set_lot2(tgt.events.entries[idx], 12345)
    f, _, _ = compare(base, m)
    results.append(("ItemLot2 modified", any("I2" in x for x in f), f))

    # I7: modify a non-Treasure event.
    m = copy.deepcopy(base)
    tgt = m["m24_01_00_01"]
    for i, blob in enumerate(tgt.events.entries):
        if int.from_bytes(blob[EV_TYPE:EV_TYPE + 4], "little") != EVENT_TYPE_TREASURE:
            blob[EV_ENTITY_ID:EV_ENTITY_ID + 4] = (999999).to_bytes(4, "little")
            break
    f, _, _ = compare(base, m)
    results.append(("non-Treasure event edited", any("I7" in x for x in f), f))

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
    args = sys.argv[1:]
    workshop = "--workshop" in args
    if workshop:
        args = [a for a in args if a != "--workshop"]

    if len(args) >= 2 and args[0] == "pool":
        return cmd_pool(args[1], workshop)
    if len(args) >= 3 and args[0] == "verify":
        return cmd_verify(args[1], args[2], workshop)
    if len(args) >= 2 and args[0] == "selftest":
        return cmd_selftest(args[1])
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main())
