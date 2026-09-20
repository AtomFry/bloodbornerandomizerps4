#!/usr/bin/env python3
"""Checking DO NOT RANDOMIZE CAGED DOGS - the ten identifiers, and a run.

The feature's whole data is ten hand-written (map prefix, entity ID) pairs in
src/Randomizer/CagedDogList.h. Nothing in the map data says "this dog is in a
cage": no field of the placement record isolates the set, the stat row is
exact for one area and not the other, and matching the placement names as
substrings would freeze 58 placements across nine map files. A list derived
once from the data and then checked by nobody is exactly the failure mode
spec 033 D3 exists to prevent, so this recomputes the set from the vanilla
tree three independent ways and asserts all three agree with the shipped
header - which is parsed, never restated.

  list       print the 26 placements the header matches, for eyeballing.
  protected  the feature's contract against an output tree: all 26 frozen,
             and the creature's other 70 placements still moving.
  selftest   the eight cases of plan 033 section 6, against the vanilla tree.

The three derivations, all measured in section 6 and re-measured here:

  A  cage proximity   - every enemy within 1.0 unit of a cage-model object
  B  nearest object   - every c1240 whose nearest object of ANY model is
                        within 1.0 unit
  C  the cage scripts - the event initialiser families that name these dogs
                        by entity ID, with the framing of both EMEVD files
                        validated two ways each first

A names the cage models and lets any creature match; B names the creature and
lets any object match; C does not use geometry at all. The radius is not a
magic number and is not asserted as one: the largest protected distance is
0.899 and the smallest unprotected one 2.671, so every radius in that range
gives the same answer, and those two bounds are what the selftest pins.

None of this proves the ENGINE consults the header correctly. Only hardware
does. See plan 033 section 6.

Usage:
    python caged_dogs_verify.py list      <vanilla_dvdroot>
    python caged_dogs_verify.py protected <vanilla_dvdroot> <output_dvdroot>
    python caged_dogs_verify.py selftest  <vanilla_dvdroot>
"""

import math
import os
import re
import struct
import sys
from collections import Counter, defaultdict

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from enemy_lookup import (BASE_MAPS, load_base_maps, load_map, enemies,  # noqa: E402
                          exclusion_reason, zone_scaled_npc)
from boss_verify import read_dcx, _i32, _i64, _utf16z  # noqa: E402
from pool_verify import _frozen  # noqa: E402
from names import model_name  # noqa: E402

SRC = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                   "..", "src", "Randomizer")

# The creature in every cage in the game. NOT c1230, the plain Hunting Dog,
# which has 14 placements and all 14 are in Hemwick Charnel Lane.
CAGED_MODEL = "c1240"

# Derivation A's input, and the only hard-coded model ids here: the two
# Central Yharnam cage models and the one the Forbidden Woods uses. Object
# model numbering is per-area in this game, so the shared trailing number is
# not evidence of a shared asset - the three were found from the geometry and
# confirmed against the scripts. None has an instance outside the five map
# files this feature touches.
CAGE_MODELS = ("o243010", "o243011", "o273011")

# Both areas hold more cage props than caged dogs, which matters for the
# hardware handoff: an empty cage is vanilla, not a miss.
CAGE_PROPS = {"m24_01_00_00": 13, "m27_00_00_00": 6}

RADIUS = 1.0            # anywhere in (0.899, 2.671) gives the same answer
MAX_PROTECTED_DIST = 0.899
MIN_UNPROTECTED_DIST = 2.671

PROTECTED_COUNT = 26
PER_MAP_COUNTS = {"m24_01_00_00": 6, "m24_01_00_01": 6, "m24_01_00_11": 6,
                  "m27_00_00_00": 4, "m27_00_00_01": 4}


# --- the shipped header ----------------------------------------------------

def parse_header():
    """The ten (map prefix, entity ID) pairs out of CagedDogList.h.

    Parsed rather than retyped for the same reason enemy_lookup parses the
    exclusion lists: a second copy in the test pins the test's copy and lets
    the shipped one drift, which is the one thing this tool exists to catch."""
    text = open(os.path.join(SRC, "CagedDogList.h"), encoding="utf-8").read()
    body = text.split("kList = {{", 1)[1].split("}};", 1)[0]
    body = re.sub(r"/\*.*?\*/", "", body, flags=re.S)
    body = re.sub(r"//[^\n]*", "", body)
    return [(p, int(e)) for p, e in re.findall(r'\{\s*"(\w+)"\s*,\s*(-?\d+)\s*\}', body)]


def is_protected(entries, mapname, entity):
    """Mirror of CagedDogList.h's IsProtectedCagedDog, `<= 0` test included."""
    if entity <= 0:
        return False
    return any(e == entity and prefix in mapname for prefix, e in entries)


# --- map data --------------------------------------------------------------

def objects(m):
    """Every OBJECT part, as (model, name, entity, pos).

    Objects live in the same PARTS_PARAM_ST section as enemies with a part
    type of 1 rather than 2, and carry name, model index, position and entity
    ID at the offsets enemy_lookup.enemies already uses."""
    for blob in m.parts.entries:
        if int.from_bytes(blob[0x14:0x18], "little") != 1:
            continue
        yield (m.model_name(_i32(blob, 0x1C)) or "",
               _utf16z(blob, _i64(blob, 0x08)),
               _i32(blob, _i64(blob, 0xB0)),
               struct.unpack_from("<fff", blob, 0x28))


def header_set(entries, maps):
    """{(mapname, part index)} the shipped header matches, and the rows."""
    keys, rows = set(), []
    for mapname in BASE_MAPS:
        for e in enemies(maps[mapname]):
            if is_protected(entries, mapname, e["entity"]):
                keys.add((mapname, e["idx"]))
                rows.append((mapname, e))
    return keys, rows


def derivation_a(maps):
    """Every enemy within RADIUS of a cage-model object.

    Also returns the two distance bounds the threshold-independence case
    needs: the largest distance among the placements the header protects, and
    the smallest among those it does not."""
    keys, prot, unprot = set(), [], []
    entries = parse_header()
    for mapname in BASE_MAPS:
        cages = [o[3] for o in objects(maps[mapname]) if o[0] in CAGE_MODELS]
        if not cages:
            continue
        for e in enemies(maps[mapname]):
            d = min(math.dist(e["pos"], c) for c in cages)
            if d <= RADIUS:
                keys.add((mapname, e["idx"]))
            (prot if is_protected(entries, mapname, e["entity"]) else unprot).append(d)
    return keys, (max(prot) if prot else None), (min(unprot) if unprot else None)


def derivation_b(maps):
    """Every c1240 whose nearest object of any model is within RADIUS."""
    keys = set()
    for mapname in BASE_MAPS:
        objs = [o[3] for o in objects(maps[mapname])]
        if not objs:
            continue
        for e in enemies(maps[mapname]):
            if e["model"] != CAGED_MODEL:
                continue
            if min(math.dist(e["pos"], o) for o in objs) <= RADIUS:
                keys.add((mapname, e["idx"]))
    return keys


# --- derivation C: the cage scripts ----------------------------------------

EVENT_SIZE = 48
INSTR_SIZE = 32
PARAM_SIZE = 32
LINKED_SIZE = 8
INIT_BANK, INIT_INDEX = 2000, 0   # event initialiser


class EmevdError(Exception):
    pass


def read_emevd(root, mapname):
    """Parse one event/<map>.emevd.dcx into {target event: [params, ...]}.

    A reader that frames an event's instruction range wrongly can still
    produce plausible output - bank 2000 instructions are common enough that
    a wrong window finds some - so the framing is validated two independent
    ways before anything is decoded, and a failure raises rather than
    degrades. Plan 033 says halt rather than adjust offsets until output
    appears, because "adjust until it looks right" is how a mis-framed reader
    gets shipped.

    Returns (initialiser families, the two framing check descriptions)."""
    path = os.path.join(root, "event", mapname + ".emevd.dcx")
    b = read_dcx(path)
    if b[:4] != b"EVD\x00":
        raise EmevdError("%s: bad EMEVD magic %r" % (mapname, b[:4]))
    if b[5] != 0xFF:
        raise EmevdError("%s: not the 64-bit (Bloodborne) EMEVD format" % mapname)

    file_size = _i32(b, 0x0C)
    (event_count, event_off, instr_count, instr_off,
     unk_count, unk_off, layer_count, layer_off,
     param_count, param_off, linked_count, linked_off) = struct.unpack_from("<12q", b, 0x10)
    arg_len, arg_off = struct.unpack_from("<2q", b, 0x70)

    # Framing check 1: every populated section's offset plus its computed
    # size lands exactly on the next section's offset, and the header's own
    # file-size field matches the file. A window computed from the wrong
    # stride does not survive this.
    if file_size != len(b):
        raise EmevdError("%s: header file size %d, actual %d" % (mapname, file_size, len(b)))
    sections = [("events", event_off, event_count * EVENT_SIZE),
                ("instructions", instr_off, instr_count * INSTR_SIZE),
                ("unknowns", unk_off, unk_count * INSTR_SIZE),
                ("layers", layer_off, layer_count * INSTR_SIZE),
                ("parameters", param_off, param_count * PARAM_SIZE),
                ("linked files", linked_off, linked_count * LINKED_SIZE),
                ("argument data", arg_off, arg_len)]
    sections = sorted((o, n, sz) for n, o, sz in sections if sz)
    for i, (off, name, size) in enumerate(sections):
        end = off + size
        limit = sections[i + 1][0] if i + 1 < len(sections) else len(b)
        if end > limit:
            raise EmevdError("%s: %s section ends at %d, past %d"
                             % (mapname, name, end, limit))
        if i + 1 < len(sections) and end != limit:
            raise EmevdError("%s: %s section ends at %d, next section starts at %d"
                             % (mapname, name, end, limit))
    check1 = "%d sections reconcile, %d bytes" % (len(sections), file_size)

    # Framing check 2: the per-event instruction counts sum to the file's own
    # instruction total. Independent of check 1 - it reads the event table
    # rather than the header - and it is the one that catches an event stride
    # that happens to keep the sections contiguous.
    events, total = [], 0
    for i in range(event_count):
        base = event_off + i * EVENT_SIZE
        event_id, count, off = struct.unpack_from("<3q", b, base)[:3]
        events.append((event_id, count, off))
        total += count
    if total != instr_count:
        raise EmevdError("%s: per-event instruction counts sum to %d, header says %d"
                         % (mapname, total, instr_count))
    check2 = "%d events' instruction counts sum to %d" % (event_count, instr_count)

    families = defaultdict(list)
    for _event_id, count, off in events:
        for j in range(count):
            ib = instr_off + off + j * INSTR_SIZE
            if ib + INSTR_SIZE > len(b):
                raise EmevdError("%s: instruction runs past end of file" % mapname)
            bank, index = struct.unpack_from("<ii", b, ib)
            if (bank, index) != (INIT_BANK, INIT_INDEX):
                continue
            size = _i64(b, ib + 8)
            aoff = _i32(b, ib + 16)
            args = b[arg_off + aoff:arg_off + aoff + size]
            # args[0] is the slot and args[1] the target event; the rest are
            # the parameters the initialised event receives.
            vals = list(struct.unpack_from("<%di" % (len(args) // 4), args, 0))
            if len(vals) >= 2:
                families[vals[1]].append(vals[2:])
    return families, (check1, check2)


def nearest_cage_entity(maps, mapname):
    """{dog entity ID: the entity ID of the cage object it stands on}.

    Derivation A's pairing, which derivation C cross-checks against the
    script's own. Only six of the ten dogs can be checked this way: the four
    Central Yharnam penned dogs stand on cage props carrying entity ID -1."""
    m = maps[mapname]
    cages = [o for o in objects(m) if o[0] in CAGE_MODELS]
    out = {}
    entries = parse_header()
    for e in enemies(m):
        if not is_protected(entries, mapname, e["entity"]):
            continue
        _d, o = min((math.dist(e["pos"], c[3]), c) for c in cages)
        out[e["entity"]] = o[2]
    return out


# --- commands --------------------------------------------------------------

def cmd_list(root):
    entries = parse_header()
    maps = load_base_maps(root)
    _keys, rows = header_set(entries, maps)
    print("CagedDogList.h holds %d identifiers, matching %d placements."
          % (len(entries), len(rows)))
    print("%-14s %-12s %9s %8s %8s  %s"
          % ("map", "placement", "entity", "stat", "think", "creature"))
    for mapname, e in rows:
        print("%-14s %-12s %9d %8d %8d  %s"
              % (mapname, e["name"], e["entity"], e["npc"], e["think"],
                 model_name(e["model"]) or e["model"]))
    print()
    print("Not a pass/fail gate - run selftest for that.")
    return 0


def cmd_protected(vanilla, output):
    """The contract, from an output tree: all 26 frozen, the other 70 moving.

    "Frozen" is pool_verify's definition, not byte equality with vanilla: the
    stat pass runs after the enemy pass and rewrites a protected placement's
    stat row exactly as it does a skipped creature's (spec 033 D8), so same
    model, same behaviour row, and a stat row that is either the vanilla
    value or the zone-scaled variant of it.

    The other 70 c1240 placements are reported as a RATE, not asserted:
    turning the setting on removes 26 placements from the roll stream, so a
    protected run and an unprotected run are different worlds and only the
    proportion is comparable."""
    entries = parse_header()
    checked, not_frozen = 0, []
    others, others_changed = 0, 0

    for mapname in BASE_MAPS:
        van, out = load_map(vanilla, mapname), load_map(output, mapname)
        if van is None or out is None:
            print("FAIL: %s missing from one of the two trees" % mapname)
            return 1
        v, o = list(enemies(van)), list(enemies(out))
        if len(v) != len(o):
            print("FAIL: %s has %d placements in vanilla, %d in output"
                  % (mapname, len(v), len(o)))
            return 1
        for a, b in zip(v, o):
            if is_protected(entries, mapname, a["entity"]):
                checked += 1
                if not _frozen(mapname, a, b):
                    not_frozen.append("%s %s: %s/%s/%s -> %s/%s/%s"
                                      % (mapname, a["name"], a["model"], a["think"],
                                         a["npc"], b["model"], b["think"], b["npc"]))
            elif a["model"] == CAGED_MODEL and not exclusion_reason(a["name"]):
                others += 1
                if (a["npc"], a["think"], a["model"]) != (b["npc"], b["think"], b["model"]):
                    others_changed += 1

    rate = (100.0 * others_changed / others) if others else 0.0
    print("protected placements  : %d of %d expected" % (checked, PROTECTED_COUNT))
    print("                        %d frozen, %d NOT frozen"
          % (checked - len(not_frozen), len(not_frozen)))
    print("other %s placements: %d, of which %d changed (%.1f%%)"
          % (CAGED_MODEL, others, others_changed, rate))
    print("change rate           : %.1f%% - reported, not asserted. Compare it"
          " against the same seed" % rate)
    print("                        with the setting off, not against a fixed number.")
    for x in not_frozen[:20]:
        print("  NOT FROZEN: " + x)
    if len(not_frozen) > 20:
        print("  ... and %d more" % (len(not_frozen) - 20))

    if checked != PROTECTED_COUNT:
        print("FAIL: matched %d protected placements, expected %d"
              % (checked, PROTECTED_COUNT))
        return 1
    if not_frozen:
        print("FAIL: %d of %d protected placement(s) were not frozen"
              % (len(not_frozen), checked))
        return 1
    print("PASS: all %d protected placements kept their creature and behaviour row"
          % checked)
    return 0


def cmd_selftest(root):
    """The eight cases of plan 033 section 6, against the vanilla tree."""
    cases = []
    entries = parse_header()
    maps = load_base_maps(root)

    # --- 1. the header itself ---------------------------------------------
    cases.append(("033: CagedDogList.h parses to exactly ten entries",
                  len(entries) == 10))
    cases.append(("033: the ten entries are all distinct",
                  len(set(entries)) == 10))
    cases.append(("033: every map prefix is m24_01 or m27",
                  {p for p, _e in entries} == {"m24_01", "m27"}))
    cases.append(("033: the predicate rejects entity id <= 0",
                  not is_protected([("m24_01", -1), ("m24_01", 0)], "m24_01_00_00", -1)
                  and not is_protected([("m24_01", 0)], "m24_01_00_00", 0)))

    # --- 2. what they match ------------------------------------------------
    keys, rows = header_set(entries, maps)
    per_map = Counter(mapname for mapname, _e in rows)
    cases.append(("033: the ten match exactly 26 placements",
                  len(rows) == PROTECTED_COUNT))
    cases.append(("033: 6 per m24_01 file, 4 per m27 file, none elsewhere",
                  dict(per_map) == PER_MAP_COUNTS))

    # --- 3, 4, 5. the two geometric derivations ----------------------------
    a_keys, max_prot, min_unprot = derivation_a(maps)
    cases.append(("033: derivation A (cage proximity) gives the same 26",
                  a_keys == keys))
    b_keys = derivation_b(maps)
    cases.append(("033: derivation B (nearest object) gives the same 26",
                  b_keys == keys))
    cases.append(("033: largest protected distance is 0.899",
                  round(max_prot, 3) == MAX_PROTECTED_DIST))
    cases.append(("033: smallest unprotected distance is 2.671",
                  round(min_unprot, 3) == MIN_UNPROTECTED_DIST))
    cases.append(("033: the radius is anywhere in that range, not a magic number",
                  max_prot < RADIUS < min_unprot))

    # --- 6. derivation C, the cage scripts ---------------------------------
    # Both files' framing is validated inside read_emevd, which raises rather
    # than returning something plausible. A raise here is a stop condition,
    # not a failed case, so it is deliberately not caught.
    cy_ids = {e for p, e in entries if p == "m24_01"}
    fw_ids = {e for p, e in entries if p == "m27"}

    cy_fams, cy_checks = read_emevd(root, "m24_01_00_00")
    fw_fams, fw_checks = read_emevd(root, "m27_00_00_00")
    for label, checks in (("m24_01_00_00", cy_checks), ("m27_00_00_00", fw_checks)):
        cases.append(("033: %s framing - %s" % (label, checks[0]), True))
        cases.append(("033: %s framing - %s" % (label, checks[1]), True))

    # Central Yharnam: exactly one initialiser family is initialised six
    # times and names exactly the six entity IDs, one per instance.
    cy_match = [ev for ev, insts in cy_fams.items()
                if len(insts) == 6
                and {i for params in insts for i in params if i in cy_ids} == cy_ids
                and all(len([i for i in params if i in cy_ids]) == 1 for params in insts)]
    cases.append(("033: one m24_01 event family names exactly the six, x6",
                  len(cy_match) == 1))

    # Forbidden Woods: exactly one family is initialised four times, each
    # instance naming one of the four and the entity ID of the very cage
    # object derivation A put it next to. Two other families name the same
    # four; the cage pairing is what singles this one out, and it is
    # independent confirmation that derivation A found cages and not scenery.
    pairing = nearest_cage_entity(maps, "m27_00_00_00")
    fw_match = [ev for ev, insts in fw_fams.items()
                if len(insts) == 4
                and {i for params in insts for i in params if i in fw_ids} == fw_ids
                and all(len([i for i in params if i in fw_ids]) == 1 and
                        pairing[[i for i in params if i in fw_ids][0]] in params
                        for params in insts)]
    cases.append(("033: one m27 event family pairs each of the four with its cage",
                  len(fw_match) == 1))

    # --- 7. what the protected placements are ------------------------------
    cases.append(("033: every protected placement is a %s" % CAGED_MODEL,
                  all(e["model"] == CAGED_MODEL for _mn, e in rows)))
    cases.append(("033: their stat rows are 124400, 124401, 124501 only",
                  {e["npc"] for _mn, e in rows} == {124400, 124401, 124501}))
    cases.append(("033: none is matched by the fixed exclusion list",
                  not any(exclusion_reason(e["name"]) for _mn, e in rows)))

    # --- 8. the name-substring trap, pinned in the failing direction -------
    trap_names = {e["name"] for _mn, e in rows}
    trap = [(mapname, e["name"]) for mapname in BASE_MAPS
            for e in enemies(maps[mapname])
            if any(n in e["name"] for n in trap_names)]
    cases.append(("033: the eight names as substrings catch 58 across 9 files",
                  len(trap_names) == 8 and len(trap) == 58
                  and len({mn for mn, _n in trap}) == 9))

    # Recorded rather than asserted, because the hardware handoff needs it:
    # both areas hold more cage props than caged dogs.
    for mapname, expected in sorted(CAGE_PROPS.items()):
        props = sum(1 for o in objects(maps[mapname]) if o[0] in CAGE_MODELS)
        cases.append(("033: %s has %d cage props for %d caged dogs"
                      % (mapname, expected, PER_MAP_COUNTS[mapname]),
                      props == expected))

    failures = 0
    for name, ok in cases:
        print("  %-62s %s" % (name, "ok" if ok else "FAILED"))
        if not ok:
            failures += 1
    print("%d/%d passing" % (len(cases) - failures, len(cases)))
    return 1 if failures else 0


def main():
    a = sys.argv[1:]
    if len(a) == 2 and a[0] == "list":
        return cmd_list(a[1])
    if len(a) == 3 and a[0] == "protected":
        return cmd_protected(a[1], a[2])
    if len(a) == 2 and a[0] == "selftest":
        return cmd_selftest(a[1])
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main())
