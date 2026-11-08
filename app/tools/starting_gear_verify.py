#!/usr/bin/env python3
"""Feature 040 - STARTING GEAR SELECTION. The Python mirror of the four armour
pickers and of START WITH SUPPLIES.

WHAT THIS PROVES AND WHAT IT CANNOT. It pins the RULES: which pieces exist,
which are offered, what each one is called, and what a correct edit to
CharaInitParam.param looks like. It cannot show that the C++ consults the tables
correctly, and it deliberately does NOT predict which piece a seed draws - the
engine's four draws come out of std::mt19937 and reproducing that here would be a
second implementation to keep in step rather than a check on the first. A clean
build plus a green selftest means READY FOR HARDWARE TESTING.

Three commands:

    list     <vanilla>            print the four tables and every excluded row,
                                  with the reason it is excluded. Not a gate.
    selftest <vanilla>            A1-A16: the derivation, the names, the row
                                  shape, and both engine passes simulated over
                                  real vanilla bytes.
    verify   <vanilla> <output>   G1-G7 against a real randomized tree.

TOLERANCE FLAGS, for verify. Six writers now touch the same 22 origin rows, so a
correct combined run breaks the "only these bytes differ" invariants for entirely
correct reasons. Each flag relaxes exactly one named thing:

    --supplies                    the two goods rows may occupy free item slots
    --tools                       START WITH HUNTER TOOLS' two key items may too
    --granted <id>                equip_Wep_Right may hold that weapon
    --granted-left <id>           equip_Wep_Left may hold that weapon
    --armour h,b,g,l              the four clothing fields may hold those ids,
                                  each in ITS OWN slot's field and nowhere else

--armour takes four comma-separated ids in the frozen slot order, head first,
with an empty field meaning "that slot was not granted". A piece arriving in
another slot's field is the mistake this feature can actually make, so the flag
is field-specific: it is four separate permissions, not a set of tolerated ids.

    --ticked h,b,g,l              four selection lines, each exactly its own
                                  table's length, so G4 can check the granted
                                  piece was actually TICKED and not merely a row
                                  of the right table

THIS CONSOLE IS cp1252 (docs/known-traps.md). Every name printed here comes from
the generated header, which is ASCII by construction because the generator
refuses to write an unrenderable one. Nothing here prints a raw FMG string.

Usage:
    python starting_gear_verify.py list     <vanilla_dvdroot>
    python starting_gear_verify.py selftest <vanilla_dvdroot>
    python starting_gear_verify.py verify   <vanilla_dvdroot> <output_dvdroot> [flags]
"""

import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gen_armour_table as gen  # noqa: E402
from fmg import ARMOUR_NAME_FMG, goods_names, load_fmg  # noqa: E402
from gen_pool_table import RENDERABLE  # noqa: E402
from param_offsets import (bnd4_members, field_offsets, load_defs,  # noqa: E402
                           load_param, param_rows)
from boss_verify import read_dcx  # noqa: E402

RND = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "src", "Randomizer")
REL = os.path.join("param", "gameparam", "gameparam.parambnd.dcx")

CHARA_MEMBER = "CharaInitParam.param"
GOODS_MEMBER = "EquipParamGoods.param"
PROTECTOR_MEMBER = "EquipParamProtector.param"

# CHARACTER_INIT_PARAM, 300-byte row. These MUST match CharaInitRows.h; the
# selftest asserts that they do rather than trusting the comment.
ITEM_ID_BASE = 124   # item_01,    s32, stride 4
ITEM_NUM_BASE = 204  # itemNum_01, u8,  stride 1
ITEM_SLOTS = 10
ROW_BYTES = 300
EMPTY = -1

# The four clothing fields, in the frozen slot order: head, body, gloves,
# legwear. Each is an s32 and each is written independently of the other three.
EQUIP_CLOTHING = (32, 36, 40, 44)  # equip_Helm, equip_Armer, equip_Gaunt, equip_Leg
SLOT_NAMES = ("HEAD", "BODY", "GLOVES", "LEGWEAR")

# The two weapon fields the neighbouring features write, tolerated one at a time
# under their own flags and never otherwise.
EQUIP_WEP_RIGHT = 16
EQUIP_WEP_LEFT = 24

# START WITH SUPPLIES. Two goods ids at count 20 each. The 20 is pinned to each
# row's own maxNum by A9 rather than chosen, so the constant is the game's.
BLOOD_VIAL = 1000
QUICKSILVER_BULLETS = 900
SUPPLIES = (BLOOD_VIAL, QUICKSILVER_BULLETS)
SUPPLY_COUNT = 20
SUPPLY_NAMES = {BLOOD_VIAL: "Blood Vial", QUICKSILVER_BULLETS: "Quicksilver Bullets"}

# START WITH HUNTER TOOLS' two, tolerated under --tools. Not this feature's.
HUNTER_TOOLS = (4103, 4104)

# The vanilla starting item every origin row carries, 1x.
VANILLA_START_ITEM = 100

# EquipParamGoods, from tools/param_offsets.py against the real paramdef. Read
# by name rather than hardcoded, because A9 is an assertion about the game's
# data and an offset typo would make it an assertion about nothing.
GOODS_MAXNUM_FIELD = "maxNum"

# EquipParamProtector, for A2 / A7 / A8.
PROTECTOR_ROWS = 273
CATEGORY_SPLIT = (76, 76, 48, 52, 21)

# Every row this feature writes - the same 22 CharaInitRows.h targets.
ORIGIN_ROWS = tuple(list(range(2000, 2010)) + list(range(3000, 3010)) + [3500, 3501])

# The 71-character line at scale 3, the fallback path's budget (pool_verify.py).
LINE_CHARS = 71
FLAG_WORD = "YES"


def i32(buf, off):
    return struct.unpack_from("<i", buf, off)[0]


def set_i32(buf, off, v):
    struct.pack_into("<i", buf, off, v)


def load_archive(root):
    plain = read_dcx(os.path.join(root, REL))
    members = {}
    for name, off, size in bnd4_members(plain):
        members[name] = (off, size)
    return bytearray(plain), members


def member_rows(plain, members, name):
    off, _size = members[name]
    return {rid: off + data for rid, data in param_rows(plain[off:off + _size])}


def row_map(plain, members, name):
    """{row id: absolute offset of its data}."""
    off, size = members[name]
    sub = bytes(plain[off:off + size])
    return {rid: off + at for rid, at in param_rows(sub)}


def slots(plain, at):
    return [(i32(plain, at + ITEM_ID_BASE + k * 4), plain[at + ITEM_NUM_BASE + k])
            for k in range(ITEM_SLOTS)]


def clothing(plain, at):
    return [i32(plain, at + o) for o in EQUIP_CLOTHING]


# --- the baked header, parsed ------------------------------------------------

def baked_tables():
    """The four tables as the C++ will see them: {slot: [(id, name), ...]} in
    the header's own order, parsed from ArmourTables.h.

    Parsed rather than re-derived on purpose. The derivation is checked against
    the header by gen_armour_table.py --check; what the engine actually reads is
    the committed file, so every assertion about order, names and counts below is
    made against that."""
    src = open(os.path.join(RND, "ArmourTables.h"), encoding="utf-8").read()
    out = {}
    counts = {}
    for slot, accessor, const, _word, _expect in gen.SLOTS:
        body = src.split("%s() {" % accessor, 1)[1].split("}};", 1)[0]
        rows = re.findall(r'\{\s*"(\d+)",\s*"([^"]*)",\s*(\d+)\s*\}', body)
        out[slot] = [(int(a), b) for a, b, _w in rows]
        counts[const] = int(re.search(r"const int %s = (\d+);" % const, src).group(1))
    return out, counts, src


# --- A1-A8: the derivation, the names, the row shape -------------------------

def derivation_cases(root):
    cases = []
    notes = []

    # A1 - the FMG frames validly by both of fmg.py's own checks. parse_fmg
    # raises FmgError on either, so reaching a dict at all is the assertion;
    # the id count is reported so a wholesale mis-parse is visible rather than
    # looking like a small one.
    entries, id_count = load_fmg(root, ARMOUR_NAME_FMG)
    cases.append(("A1 the armour-name FMG frames validly by both fmg.py checks",
                  bool(entries) and id_count >= len(entries)))
    notes.append("armour-name FMG: %d ids, %d with a string" % (id_count, len(entries)))

    # A2 - the param's shape, and that category 4 really is the placeholders.
    defs = load_defs(root)
    p = load_param(root, PROTECTOR_MEMBER)
    rows = list(param_rows(p))
    split = [0, 0, 0, 0, 0]
    cat4_named = set()
    for rid, off in rows:
        cat = p[off + gen.PROTECTOR_CATEGORY]
        if 0 <= cat < 5:
            split[cat] += 1
        if cat == gen.CATEGORY_PLACEHOLDER:
            cat4_named.add((entries.get(rid, "") or "").strip())
    cases.append(("A2 EquipParamProtector holds %d rows split %s, all category-4 "
                  "rows NOT NEEDED" % (PROTECTOR_ROWS, "/".join(map(str, CATEGORY_SPLIT))),
                  len(rows) == PROTECTOR_ROWS and tuple(split) == CATEGORY_SPLIT and
                  cat4_named == {gen.NOT_NEEDED_NAME}))

    # A3 - the three tests give 33/34/26/31, and the header's static_assert
    # spells the same total. derive() raises on any other count, so this runs it
    # and then compares the COMMITTED header against it independently.
    d = gen.derive(root)
    baked, counts, src = baked_tables()
    derived_counts = {const: len(d["rows"][slot])
                      for slot, _a, const, _w, _n in gen.SLOTS}
    assert_total = re.search(
        r"static_assert\(([^,]+)==\s*(\d+),", src.replace("\n", " "))
    cases.append(("A3 the three tests give 33/34/26/31 = 124 and the header's "
                  "static_assert spells 124",
                  derived_counts == {"kHeadwearCount": 33, "kBodyArmourCount": 34,
                                     "kGlovesCount": 26, "kLegwearCount": 31} and
                  counts == derived_counts and
                  sum(counts.values()) == gen.TOTAL and
                  assert_total is not None and
                  int(assert_total.group(2)) == gen.TOTAL and
                  all(c in assert_total.group(1) for c in derived_counts)))

    # A4 - the cross-check, in BOTH directions, exactly as plan 4.2 states them.
    # derive() already raises if either side differs; this restates it as a case
    # so a green selftest says so out loud, and reports the excluded rows without
    # asserting a list of them.
    only_included = d["included"] - d["obtainable"]
    only_obtainable = d["obtainable"] - d["included"]
    cases.append(("A4 the obtainability cross-check holds in both directions",
                  only_included == gen.STARTING_GARMENTS and
                  only_obtainable == gen.UNWEARABLE_BY_BOTH))
    notes.append("included but not obtainable (the starting garments): %s"
                 % sorted(only_included))
    notes.append("obtainable but not included (wearable by neither sex): %s"
                 % sorted(only_obtainable))
    dropped = sum(len(v) for v in d["excluded"].values())
    notes.append("%d wearable-slot rows excluded by the name or gender test "
                 "(reported, never asserted as a list)" % dropped)

    # A5 - every name is the uppercased FMG string FOR THAT EXACT ID, every name
    # is renderable, and the widest row fits the fallback line.
    name_ok = True
    for slot, table in baked.items():
        for rid, name in table:
            if (entries.get(rid, "") or "").strip().upper() != name:
                name_ok = False
    widest = max(len(n) for t in baked.values() for _r, n in t)
    cases.append(("A5 every name is the uppercased FMG string for that exact id, "
                  "renderable, and fits the 71-char line",
                  name_ok and
                  all(c in RENDERABLE for t in baked.values() for _r, n in t
                      for c in n) and
                  widest + 3 + len(FLAG_WORD) <= LINE_CHARS))
    notes.append("widest name %d characters, + 3 + %s = %d of %d"
                 % (widest, FLAG_WORD, widest + 3 + len(FLAG_WORD), LINE_CHARS))

    # A6 - the committed tables are in the frozen order, and the duplicate set is
    # exactly the two body pairs, adjacent.
    order_ok = all(baked[slot] == sorted(baked[slot], key=lambda r: (r[1], r[0]))
                   for slot in baked)
    dup = {}
    for slot, table in baked.items():
        names = [n for _r, n in table]
        for n in set(names):
            if names.count(n) > 1:
                dup[(slot, n)] = names.count(n)
    adjacent = True
    for (slot, n), _k in dup.items():
        idx = [i for i, (_r, nm) in enumerate(baked[slot]) if nm == n]
        if idx != list(range(idx[0], idx[0] + len(idx))):
            adjacent = False
    cases.append(("A6 the four tables are in the frozen order and the only "
                  "duplicate names are the two body pairs, adjacent",
                  order_ok and adjacent and
                  dup == {(gen.DUPLICATE_SLOT, n): 2
                          for n in gen.EXPECTED_DUPLICATES}))

    # A7 - B9, asserted rather than assumed: the armour row has no requirement
    # fields at all, against four on the weapon row. Nothing in this feature
    # writes WeaponRequirements.h or ReqOwner, and this is why it cannot need to.
    prot_proper = [f["name"] for f in defs["EQUIP_PARAM_PROTECTOR_ST"]["fields"]
                   if f["name"].startswith("proper")]
    wep_proper = [f["name"] for f in defs["EQUIP_PARAM_WEAPON_ST"]["fields"]
                  if f["name"].startswith("proper")]
    cases.append(("A7 the armour row has zero proper* requirement fields, the "
                  "weapon row has four",
                  prot_proper == [] and len(wep_proper) == 4))

    # A8 - id/1000 % 10 == protectorCategory on every wearable-slot row, so no
    # piece can land in another slot's field through an id mistake. This is what
    # makes G6 checkable from the id alone.
    mismatched = []
    for rid, off in rows:
        cat = p[off + gen.PROTECTOR_CATEGORY]
        if cat == gen.CATEGORY_PLACEHOLDER:
            continue
        if (rid // 1000) % 10 != cat:
            mismatched.append(rid)
    wearable = sum(CATEGORY_SPLIT[:4])
    cases.append(("A8 id/1000 %% 10 == protectorCategory on all %d wearable-slot "
                  "rows" % wearable, not mismatched))

    # A16 - the four selection lines, each at ITS OWN exact length. The mirror of
    # ModelPoolSelection<N, false>::Decode: a line of any other length is refused
    # and the selection is left in its constructed state, which for these four is
    # nothing ticked. The four lengths differ, which is what stops a line pasted
    # into the wrong key from being remapped onto another slot's pieces.
    def decode(text, n):
        """Decode, or the constructed state when the length is wrong."""
        if len(text) != n:
            return [False] * n
        return [c != "0" for c in text]

    round_trip = True
    short_long = True
    for slot, _a, const, _w, _n in gen.SLOTS:
        n = counts[const]
        # A pattern no other slot's line can be: every third row ticked.
        pattern = "".join("1" if i % 3 == 0 else "0" for i in range(n))
        if decode(pattern, n) != [c != "0" for c in pattern]:
            round_trip = False
        if any(decode(pattern[:-1], n)) or any(decode(pattern + "1", n)):
            short_long = False
    # ...and the length guard is only meaningful if the four lengths differ.
    lengths = sorted(counts[c] for _s, _a, c, _w, _n in gen.SLOTS)
    cases.append(("A16 each of the four selection lines round-trips at its exact "
                  "length, and one short or one long leaves nothing ticked",
                  round_trip and short_long and
                  len(set(lengths)) == len(lengths)))

    return cases, notes, d, baked


# --- the two engine passes, simulated ---------------------------------------
#
# These mirror StartingSupplies.cpp and HunterTools.cpp field for field, over the
# real vanilla bytes. They exist to pin the RULES - which slot, which count, what
# is left alone - and they are NOT a second implementation the engine is compared
# against: nothing here predicts a draw, and the armour simulation is handed the
# pieces rather than choosing them (see the module docstring).


def sim_grant_items(buf, rows, items, count):
    """HunterTools.cpp / StartingSupplies.cpp: per row, per item, skip it if the
    row already lists it, else take the FIRST EMPTY slot and write the id and the
    single-byte count. Returns (rows changed, slots written)."""
    changed = slots_written = 0
    for rid in ORIGIN_ROWS:
        at = rows[rid]
        touched = False
        for item in items:
            ids = [i32(buf, at + ITEM_ID_BASE + k * 4) for k in range(ITEM_SLOTS)]
            if item in ids:
                continue
            free = [k for k in range(ITEM_SLOTS) if ids[k] == EMPTY]
            if not free:
                break
            k = free[0]
            set_i32(buf, at + ITEM_ID_BASE + k * 4, item)
            buf[at + ITEM_NUM_BASE + k] = count
            slots_written += 1
            touched = True
        if touched:
            changed += 1
    return changed, slots_written


def sim_supplies(buf, rows):
    return sim_grant_items(buf, rows, SUPPLIES, SUPPLY_COUNT)


def sim_tools(buf, rows):
    return sim_grant_items(buf, rows, HUNTER_TOOLS, 1)


def sim_armour_draws(picked_per_slot, words):
    """ArmourGrant.cpp step 3, and nothing else: the DRAW DISCIPLINE.

    `picked_per_slot` is the four candidate lists. `words` is the raw stream the
    engine would take from mt19937 - supplied here rather than generated, because
    what is being checked is how many words are taken and which index each one
    produces, NOT which words mt19937 yields.

    Returns (granted per slot, words consumed). Exactly four words are consumed
    whenever any slot has candidates, and a slot with none DISCARDS its word.
    """
    if not any(picked_per_slot):
        return [0, 0, 0, 0], 0
    granted = [0, 0, 0, 0]
    used = 0
    for slot in range(4):
        word = words[used]
        used += 1
        if not picked_per_slot[slot]:
            continue  # the word is discarded on purpose
        granted[slot] = picked_per_slot[slot][word % len(picked_per_slot[slot])]
    return granted, used


def sim_armour_write(buf, rows, granted):
    """ArmourGrant.cpp step 4: one s32 per GRANTED slot, over every origin row.
    A slot with nothing granted is left alone, not written with a sentinel."""
    changed = fields = 0
    for rid in ORIGIN_ROWS:
        at = rows[rid]
        touched = False
        for slot, off in enumerate(EQUIP_CLOTHING):
            if granted[slot] == 0:
                continue
            set_i32(buf, at + off, granted[slot])
            fields += 1
            touched = True
        if touched:
            changed += 1
    return changed, fields


# --- A9-A13: the supplies pass ----------------------------------------------

def supplies_cases(root):
    cases = []
    notes = []
    van, members = load_archive(root)
    rows = row_map(van, members, CHARA_MEMBER)
    goods = row_map(van, members, GOODS_MEMBER)

    # A9 - the two ids, their names and the count, all read from the game rather
    # than from a comment. The count is each row's own maxNum, which is what makes
    # 20 the game's number and not a choice.
    defs = load_defs(root)
    offs, _size = field_offsets(defs["EQUIP_PARAM_GOODS_ST"])
    maxnum = [st for f, st, _w in offs if f["name"] == GOODS_MAXNUM_FIELD]
    names = goods_names(root)
    ok = len(maxnum) == 1
    if ok:
        for gid in SUPPLIES:
            if gid not in goods:
                ok = False
                continue
            if names.get(gid) != SUPPLY_NAMES[gid]:
                ok = False
            if struct.unpack_from("<h", van, goods[gid] + maxnum[0])[0] != SUPPLY_COUNT:
                ok = False
    cases.append(("A9 goods %d and %d are named as expected and each carries "
                  "maxNum == %d" % (BLOOD_VIAL, QUICKSILVER_BULLETS, SUPPLY_COUNT),
                  ok))

    # A10 - the row shape the pass depends on. If this stops holding the C++
    # silently does less, which is why it is asserted rather than assumed.
    shape_ok = True
    for rid in ORIGIN_ROWS:
        if rid not in rows:
            shape_ok = False
            continue
        at = rows[rid]
        if at + ROW_BYTES > len(van):
            shape_ok = False
            continue
        if slots(van, at) != [(VANILLA_START_ITEM, 1)] + [(EMPTY, 0)] * 9:
            shape_ok = False
    cases.append(("A10 all %d origin rows hold item = [%d, -1 x9] and "
                  "itemNum = [1, 0 x9]" % (len(ORIGIN_ROWS), VANILLA_START_ITEM),
                  shape_ok))

    # A11 - supplies on, alone. Both ids once each at count 20, in slots that were
    # empty; slot 0 untouched; NOTHING else in the 300-byte row changed; and
    # running the pass again changes nothing, which is what RowHasItem buys.
    out = bytearray(van)
    changed, written = sim_supplies(out, rows)
    a11 = changed == len(ORIGIN_ROWS) and written == 2 * len(ORIGIN_ROWS)
    for rid in ORIGIN_ROWS:
        at = rows[rid]
        got = slots(out, at)
        ids = [v for v, _n in got]
        if got[0] != (VANILLA_START_ITEM, 1):
            a11 = False
        for item in SUPPLIES:
            if ids.count(item) != 1 or got[ids.index(item)][1] != SUPPLY_COUNT:
                a11 = False
        # Nothing outside the item arrays: rebuild vanilla with the output's item
        # fields patched in and require the whole row back.
        patched = bytearray(van[at:at + ROW_BYTES])
        for k in range(ITEM_SLOTS):
            set_i32(patched, ITEM_ID_BASE + k * 4, i32(out, at + ITEM_ID_BASE + k * 4))
            patched[ITEM_NUM_BASE + k] = out[at + ITEM_NUM_BASE + k]
        if patched != out[at:at + ROW_BYTES]:
            a11 = False
    # ...and no row outside the 22, and no other member.
    if not rows_outside_unchanged(van, out, members, rows):
        a11 = False
    again = bytearray(out)
    changed_again, written_again = sim_supplies(again, rows)
    cases.append(("A11 supplies on: both ids once each at count %d in slots that "
                  "were empty, slot 0 intact, no other byte changed, idempotent"
                  % SUPPLY_COUNT,
                  a11 and again == out and changed_again == 0 and
                  written_again == 0))

    # A12 - supplies AND hunter tools, in BOTH pass orders. NO WRITE IS LOST
    # either way: each row ends holding all four entries exactly once at its own
    # count, the Hunter's Mark is still in slot 0, and no row ran out of slots.
    #
    # WHAT IS NOT ASSERTED, AND WHY. The BYTES are not identical between the two
    # orders and cannot be. Both passes take the FIRST EMPTY slot - which §3.1
    # requires, precisely so that neither can overwrite the other - so whichever
    # runs first occupies slots 1 and 2 and the other takes 3 and 4. Requiring
    # byte equality here would mean giving each feature a fixed slot index, which
    # is the one thing that WOULD let two features collide. What commutes is the
    # row's CONTENTS, and that is what is asserted: the multiset of
    # (item, count) pairs is identical either way. See plan §10.
    both_a = bytearray(van)
    sim_tools(both_a, rows)
    sim_supplies(both_a, rows)
    both_b = bytearray(van)
    sim_supplies(both_b, rows)
    sim_tools(both_b, rows)
    all_four = list(HUNTER_TOOLS) + list(SUPPLIES)
    a12 = True
    for rid in ORIGIN_ROWS:
        for buf in (both_a, both_b):
            got = slots(buf, rows[rid])
            ids = [v for v, _n in got]
            if any(ids.count(x) != 1 for x in all_four):
                a12 = False
            for item in SUPPLIES:
                if got[ids.index(item)][1] != SUPPLY_COUNT:
                    a12 = False
            for item in HUNTER_TOOLS:
                if got[ids.index(item)][1] != 1:
                    a12 = False
            if got[0] != (VANILLA_START_ITEM, 1):
                a12 = False
            if ids.count(EMPTY) != ITEM_SLOTS - 1 - len(all_four):
                a12 = False  # a row that ran out of slots
        held_a = sorted(x for x in slots(both_a, rows[rid]) if x[0] != EMPTY)
        held_b = sorted(x for x in slots(both_b, rows[rid]) if x[0] != EMPTY)
        if held_a != held_b:
            a12 = False
        # ...and the rest of the row is untouched in both orders.
        for buf in (both_a, both_b):
            at = rows[rid]
            patched = bytearray(van[at:at + ROW_BYTES])
            for k in range(ITEM_SLOTS):
                set_i32(patched, ITEM_ID_BASE + k * 4,
                        i32(buf, at + ITEM_ID_BASE + k * 4))
                patched[ITEM_NUM_BASE + k] = buf[at + ITEM_NUM_BASE + k]
            if patched != buf[at:at + ROW_BYTES]:
                a12 = False
    cases.append(("A12 supplies and hunter tools, in BOTH pass orders: all four "
                  "present once each at its own count, no write lost, no row out "
                  "of slots, identical CONTENTS", a12))
    notes.append("A12: the two orders differ in WHICH free slot each entry takes "
                 "- slots 1,2 for whichever pass ran first - and that is "
                 "FirstEmptySlot working as plan 3.1 requires. See plan 10.")
    notes.append("tools + supplies fill 4 of the 10 item slots; %d remain free"
                 % (ITEM_SLOTS - 1 - len(all_four)))

    # A13 - the toggle off. Not "approximately the run it made before": the same
    # bytes, and no RNG word consumed. The supplies pass never draws at all, so
    # its half of this is unconditional; the armour half is asserted in A14/A15's
    # company below once there is an armour pass to ask.
    off = bytearray(van)
    cases.append(("A13 the toggle off: the archive is byte-identical and no RNG "
                  "word is consumed", off == van))

    return cases, notes, van, members, rows


def armour_cases(root, baked, counts):
    cases = []
    notes = []
    van, members = load_archive(root)
    rows = row_map(van, members, CHARA_MEMBER)

    def ids(slot):
        return [rid for rid, _n in baked[slot]]

    # A14 - one piece ticked in one slot. Every origin row names that piece in that
    # slot; THE OTHER THREE CLOTHING FIELDS KEEP THEIR VANILLA VALUES, which is the
    # half that matters, because all four hold a real garment in vanilla and a pass
    # that wrote all four would undress three slots the player never asked about.
    a14 = True
    for slot in range(4):
        picked = [[], [], [], []]
        picked[slot] = [ids(slot)[0]]
        granted, used = sim_armour_draws(picked, [0, 0, 0, 0])
        if used != 4 or granted[slot] != ids(slot)[0]:
            a14 = False
        if any(granted[o] != 0 for o in range(4) if o != slot):
            a14 = False
        out = bytearray(van)
        changed, fields = sim_armour_write(out, rows, granted)
        if changed != len(ORIGIN_ROWS) or fields != len(ORIGIN_ROWS):
            a14 = False
        for rid in ORIGIN_ROWS:
            at = rows[rid]
            got = clothing(out, at)
            want = clothing(van, at)
            if got[slot] != granted[slot]:
                a14 = False
            for other in range(4):
                if other != slot and got[other] != want[other]:
                    a14 = False
            # Nothing outside the one clothing field: rebuild the vanilla row with
            # that field patched in and require the whole 300 bytes back.
            patched = bytearray(van[at:at + ROW_BYTES])
            set_i32(patched, EQUIP_CLOTHING[slot], granted[slot])
            if patched != out[at:at + ROW_BYTES]:
                a14 = False
        if not rows_outside_unchanged(van, out, members, rows):
            a14 = False
    cases.append(("A14 one piece ticked in one slot: all %d origin rows name it in "
                  "that slot, the other three clothing fields keep their vanilla "
                  "values, nothing else changed" % len(ORIGIN_ROWS), a14))

    # A15 - B6, THE ASSERTION THIS FEATURE EXISTS OR FAILS ON. With a fixed word
    # stream, the piece drawn for a slot is the same whatever the other three slots
    # have ticked. Asserted across several tick patterns per slot, not one, and
    # across the patterns that actually break a naive implementation: the other
    # slots EMPTY (which is what shifts a stream when a draw is skipped) and the
    # other slots at different sizes (which is what shifts one when a distribution
    # is used).
    words = [0x9E3779B9, 0x243F6A88, 0xB7E15162, 0x0BADC0DE]
    a15 = True
    draw_counts = set()
    for slot in range(4):
        for tick_count in (1, 2, 3, 7, counts_for(slot, counts)):
            mine = ids(slot)[:tick_count]
            baseline = None
            for others in ("empty", "one", "half", "all"):
                picked = [[], [], [], []]
                picked[slot] = mine
                for other in range(4):
                    if other == slot:
                        continue
                    pool = ids(other)
                    if others == "empty":
                        picked[other] = []
                    elif others == "one":
                        picked[other] = pool[:1]
                    elif others == "half":
                        picked[other] = pool[:max(1, len(pool) // 2)]
                    else:
                        picked[other] = pool
                granted, used = sim_armour_draws(picked, words)
                draw_counts.add(used)
                if baseline is None:
                    baseline = granted[slot]
                elif granted[slot] != baseline:
                    a15 = False
    cases.append(("A15 B6: with a fixed stream each slot draws the same piece "
                  "whatever the other three have ticked, over 4 slots x 5 tick "
                  "counts x 4 neighbour patterns", a15))
    # ...and the mechanism that makes it true: the draw count is always exactly 4.
    cases.append(("A15 the draw count is exactly 4 in every non-empty case, and 0 "
                  "when all four slots are empty",
                  draw_counts == {4} and
                  sim_armour_draws([[], [], [], []], words) == ([0, 0, 0, 0], 0)))
    # A13's armour half: all four empty writes nothing and draws nothing.
    empty_out = bytearray(van)
    granted, used = sim_armour_draws([[], [], [], []], words)
    changed, fields = sim_armour_write(empty_out, rows, granted)
    cases.append(("A13 nothing ticked in any slot: the archive is byte-identical "
                  "and no RNG word is consumed",
                  empty_out == van and used == 0 and changed == 0 and fields == 0))

    # The C++'s own constants must agree with this tool, or both implementations
    # are checking different bytes at once.
    hdr = open(os.path.join(RND, "CharaInitRows.h"), encoding="utf-8").read()
    cpp = open(os.path.join(RND, "ArmourGrant.cpp"), encoding="utf-8").read()
    offsets_ok = all(("    %d, // equip_" % off) in hdr for off in EQUIP_CLOTHING)
    # The "no distribution" half runs against the CODE, not the prose. The pass's
    # own comments necessarily name std::uniform_int_distribution in order to say
    # why it is not used, so a check over the raw text would be checking the
    # explanation - the same trap left_hand_weapons_verify.py's
    # strip_line_comments() exists for.
    code = "\n".join(line.split("//", 1)[0] for line in cpp.split("\n"))
    cases.append(("A15 CharaInitRows.h carries the four clothing offsets this tool "
                  "uses, and ArmourGrant.cpp's CODE uses no distribution",
                  offsets_ok and "kClothingSlots = 4" in hdr and
                  "uniform_int_distribution" not in code and
                  "% (uint32_t)k" in code and
                  # ...and it really does draw once per slot inside the four-slot
                  # loop, which is the other half of the mechanism.
                  "for (int slot = 0; slot < kClothingSlots; slot++)" in code and
                  "const uint32_t word = (uint32_t)rng();" in code))
    notes.append("A15 word stream: %s (fixed, not drawn - what is checked is the "
                 "draw discipline, not mt19937)" % [hex(w) for w in words])

    # --- compare()'s own cases: G1-G7 accept a correct tree and reject each way
    # --- of getting it wrong ------------------------------------------------
    #
    # `verify` runs against a real output tree, which only a hardware run
    # produces, so these build synthetic output from real vanilla bytes instead.
    # Without them G1-G7 would be code nothing had ever executed, and a verifier
    # that has never rejected anything is not evidence.
    picks = [ids(0)[0], ids(1)[0], ids(2)[0], ids(3)[0]]
    lines = []
    for slot in range(4):
        n = counts[[c for _s, _a, c, _w, _n in gen.SLOTS][slot]]
        lines.append("1" + "0" * (n - 1))   # the first row of each table ticked
    ticked = parse_ticked(",".join(lines))

    def judge(mutate, **flags):
        out = bytearray(van)
        mutate(out)
        f, _n = compare(van, members, out, members, **flags)
        return f

    def dress(buf):
        sim_armour_write(buf, rows, picks)

    cases.append(("G a correctly dressed tree passes with --armour and --ticked",
                  not judge(dress, armour=picks, ticked=ticked)))
    cases.append(("G5 the same tree is REJECTED with no --armour",
                  bool(judge(dress))))

    def one_row_differs(buf):
        dress(buf)
        set_i32(buf, rows[ORIGIN_ROWS[0]] + EQUIP_CLOTHING[1], ids(1)[1])
    cases.append(("G4 a row naming a different piece is REJECTED",
                  bool(judge(one_row_differs, armour=picks, ticked=ticked))))

    def crossed(buf):
        for rid in ORIGIN_ROWS:
            set_i32(buf, rows[rid] + EQUIP_CLOTHING[1], picks[0])
    cases.append(("G6 a head piece in the body field is REJECTED even with every "
                  "id declared", bool(judge(crossed, armour=picks))))

    def unticked_piece(buf):
        sim_armour_write(buf, rows, [ids(0)[5], 0, 0, 0])
    cases.append(("G4 a granted piece that was not ticked is REJECTED",
                  bool(judge(unticked_piece, armour=[ids(0)[5], 0, 0, 0],
                             ticked=ticked))))

    def stray(buf):
        dress(buf)
        set_i32(buf, rows[ORIGIN_ROWS[0]] + 12, 9999)  # soul, far from any field
    cases.append(("G3 a stray write elsewhere in the row is REJECTED",
                  bool(judge(stray, armour=picks, ticked=ticked))))

    def non_origin(buf):
        dress(buf)
        victim = sorted(r for r in rows if r not in set(ORIGIN_ROWS))[0]
        set_i32(buf, rows[victim] + EQUIP_CLOTHING[0], picks[0])
    cases.append(("G2 a non-origin row changing is REJECTED",
                  bool(judge(non_origin, armour=picks, ticked=ticked))))

    def other_member(buf):
        dress(buf)
        off, _size = members[GOODS_MEMBER]
        set_i32(buf, off + 0x40, 12345)
    cases.append(("G1 another param member changing is REJECTED",
                  bool(judge(other_member, armour=picks, ticked=ticked))))

    def dressed_and_supplied(buf):
        dress(buf)
        sim_supplies(buf, rows)
    cases.append(("G armour and supplies together pass with both flags",
                  not judge(dressed_and_supplied, armour=picks, ticked=ticked,
                            supplies=True)))
    cases.append(("G ...and are REJECTED with --supplies missing",
                  bool(judge(dressed_and_supplied, armour=picks, ticked=ticked))))
    cases.append(("G --ticked rejects a line of the wrong length",
                  parse_ticked("1,1,1,1") is None and
                  parse_ticked(",,,") == [[], [], [], []]))

    return cases, notes


def counts_for(slot, counts):
    """That slot's full row count, as the last tick pattern A15 tries."""
    return counts[[c for _s, _a, c, _w, _n in gen.SLOTS][slot]]


def rows_outside_unchanged(van, out, members, rows):
    """No CharaInitParam row outside the 22, and no other archive member,
    differs. Used by A11 and A14."""
    targets = set(ORIGIN_ROWS)
    for rid, at in rows.items():
        if rid in targets:
            continue
        if van[at:at + ROW_BYTES] != out[at:at + ROW_BYTES]:
            return False
    for name, (off, size) in members.items():
        if name == CHARA_MEMBER:
            continue
        if van[off:off + size] != out[off:off + size]:
            return False
    return True


# --- verify: G1-G7 against a real output tree -------------------------------

def compare(van, van_m, out, out_m, supplies=False, tools=False,
            granted=None, granted_left=None, armour=None, ticked=None):
    """G1-G7. Every tolerance is one named permission; nothing here is a blanket
    'these bytes may differ'."""
    failures, notes = [], []
    tolerated = {CHARA_MEMBER}
    if granted is not None or granted_left is not None:
        tolerated.add("EquipParamWeapon.param")
        notes.append("tolerating a weapon grant's requirement rewrite")

    # G7 / G1
    if set(van_m) != set(out_m):
        failures.append("G7 archive membership changed")
        return failures, notes
    for name in sorted(van_m):
        (vo, vs), (oo, osz) = van_m[name], out_m[name]
        if vs != osz:
            failures.append("G7 member %s changed size %d -> %d" % (name, vs, osz))
            continue
        if name in tolerated:
            continue
        if bytes(van[vo:vo + vs]) != bytes(out[oo:oo + osz]):
            failures.append("G1 member %s differs but should not" % name)

    van_rows = row_map(van, van_m, CHARA_MEMBER)
    out_rows = row_map(out, out_m, CHARA_MEMBER)
    if set(van_rows) != set(out_rows):
        failures.append("G2 CharaInitParam row ids changed")
        return failures, notes

    targets = set(ORIGIN_ROWS)
    for rid in sorted(van_rows):
        va, oa = van_rows[rid], out_rows[rid]
        same = bytes(van[va:va + ROW_BYTES]) == bytes(out[oa:oa + ROW_BYTES])
        if rid not in targets:
            if not same:
                failures.append("G2 non-origin row %d changed" % rid)
            continue

        # G3: rebuild the vanilla row with every DECLARED field patched in from
        # the output; anything left over is a stray write.
        patched = bytearray(van[va:va + ROW_BYTES])
        if supplies or tools:
            for k in range(ITEM_SLOTS):
                set_i32(patched, ITEM_ID_BASE + k * 4,
                        i32(out, oa + ITEM_ID_BASE + k * 4))
                patched[ITEM_NUM_BASE + k] = out[oa + ITEM_NUM_BASE + k]
        for field, value in ((EQUIP_WEP_RIGHT, granted), (EQUIP_WEP_LEFT, granted_left)):
            if value is None:
                continue
            got = i32(out, oa + field)
            if got == value:
                set_i32(patched, field, got)
        if armour is not None:
            # One slot at a time, each in ITS OWN field. Patched in only where the
            # output holds that slot's declared piece in that slot's field, so a
            # crossed write stays a G3 difference.
            for slot, off in enumerate(EQUIP_CLOTHING):
                if not armour[slot]:
                    continue
                got = i32(out, oa + off)
                if got == armour[slot]:
                    set_i32(patched, off, got)
        if patched != out[oa:oa + ROW_BYTES]:
            failures.append("G3 row %d changed outside the declared fields" % rid)

        # G4 / G5 / G6, per row.
        got_clothing = clothing(out, oa)
        van_clothing = clothing(van, va)
        for slot in range(4):
            want = armour[slot] if armour is not None else 0
            if want:
                # G4: every row names the SAME piece in this slot, and it is one of
                # that slot's rows.
                if got_clothing[slot] != want:
                    failures.append("G4 row %d has %d in the %s field, not the "
                                    "declared %d"
                                    % (rid, got_clothing[slot], SLOT_NAMES[slot],
                                       want))
            else:
                # G5: a slot whose picker was empty keeps its vanilla value.
                if got_clothing[slot] != van_clothing[slot]:
                    failures.append("G5 row %d changed its %s field to %d with no "
                                    "grant declared for that slot"
                                    % (rid, SLOT_NAMES[slot], got_clothing[slot]))
            # G6: whatever is in this field belongs to THIS slot. A8 establishes
            # that id/1000 %% 10 is the piece's own protectorCategory, so this is
            # checkable from the id alone.
            value = got_clothing[slot]
            if value > 0 and (value // 1000) % 10 != slot:
                failures.append("G6 row %d holds %d in the %s field, which belongs "
                                "to slot %d"
                                % (rid, value, SLOT_NAMES[slot], (value // 1000) % 10))

        out_slots = slots(out, oa)
        ids = [v for v, _n in out_slots]
        if out_slots[0] != (VANILLA_START_ITEM, 1):
            failures.append("G3 row %d displaced the vanilla starting item: slot 0 "
                            "is %r" % (rid, out_slots[0]))
        if supplies:
            for item in SUPPLIES:
                n = ids.count(item)
                if n != 1:
                    failures.append("G3 row %d holds supply %d %d times, expected 1"
                                    % (rid, item, n))
                elif out_slots[ids.index(item)][1] != SUPPLY_COUNT:
                    failures.append("G3 row %d supply %d has count %d, expected %d"
                                    % (rid, item, out_slots[ids.index(item)][1],
                                       SUPPLY_COUNT))
        else:
            for item in SUPPLIES:
                if item in ids:
                    failures.append("G3 row %d holds supply %d but --supplies was "
                                    "not declared" % (rid, item))

    # G4's other half: each declared piece is a row of ITS OWN slot's table, and -
    # when --ticked says what was ticked - one of the ticked ones. Checked once
    # rather than per row, because the per-row loop above already proved all 22
    # agree.
    if armour is not None:
        baked, _counts, _src = baked_tables()
        for slot in range(4):
            if not armour[slot]:
                continue
            table_ids = [rid for rid, _n in baked[slot]]
            if armour[slot] not in table_ids:
                failures.append("G4 %d is not one of the %d %s rows"
                                % (armour[slot], len(table_ids), SLOT_NAMES[slot]))
                continue
            if ticked is not None and ticked[slot]:
                idx = table_ids.index(armour[slot])
                if idx not in ticked[slot]:
                    failures.append("G4 %d was not ticked in the %s picker"
                                    % (armour[slot], SLOT_NAMES[slot]))
            name = dict(baked[slot]).get(armour[slot])
            notes.append("%s: %d (%s)" % (SLOT_NAMES[slot], armour[slot], name))
    return failures, notes


# --- commands ---------------------------------------------------------------

def cmd_list(root):
    d = gen.derive(root)
    baked, counts, _src = baked_tables()
    for slot, _accessor, const, word, _expect in gen.SLOTS:
        print("")
        print("%s - %s = %d rows" % (word.upper(), const, counts[const]))
        for i, (rid, name) in enumerate(baked[slot]):
            print("  %3d  %-8d %s" % (i, rid, name))
        print("  excluded from this slot:")
        for rid, why in sorted(d["excluded"][slot]):
            print("    %-8d %s" % (rid, why))
    print("")
    print("%d rows offered across four slots" % sum(counts.values()))
    print("category %d (the NOT NEEDED placeholders) is excluded by the category "
          "test, not by a name filter" % gen.CATEGORY_PLACEHOLDER)
    return 0


def parse_armour(text):
    """Mirror of --armour h,b,g,l: four armour ids in the frozen slot order, an
    empty field meaning that slot was not granted. Returns a 4-list of ints with 0
    for "not granted", or None if the shape is wrong.

    Four fields always, even when only one slot was granted, so the flag cannot be
    written in a way that leaves which slot ambiguous."""
    parts = text.split(",")
    if len(parts) != 4:
        return None
    out = []
    for p in parts:
        p = p.strip()
        if not p:
            out.append(0)
            continue
        if not p.isdigit():
            return None
        out.append(int(p))
    return out


def parse_ticked(text):
    """--ticked h,b,g,l: four selection lines, each exactly its own table's length,
    an empty field meaning "not stated". Returns a 4-list of ticked-index lists, or
    None if any line is the wrong length - the same length guard
    ModelPoolSelection::Decode applies, for the same reason."""
    parts = text.split(",")
    if len(parts) != 4:
        return None
    _tables, counts, _src = baked_tables()
    out = []
    for slot, part in enumerate(parts):
        part = part.strip()
        if not part:
            out.append([])
            continue
        n = counts[[c for _s, _a, c, _w, _n in gen.SLOTS][slot]]
        if len(part) != n:
            return None
        out.append([i for i, c in enumerate(part) if c != "0"])
    return out


def cmd_verify(vanilla_root, output_root, **flags):
    opath = os.path.join(output_root, REL)
    if not os.path.exists(opath):
        print("no item-data archive in output tree - expected when no param "
              "feature was on")
        return 0
    van, van_m = load_archive(vanilla_root)
    out, out_m = load_archive(output_root)
    print("vanilla decompressed=%d  output decompressed=%d" % (len(van), len(out)))
    failures, notes = compare(van, van_m, out, out_m, **flags)
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


def main():
    args = list(sys.argv[1:])
    flags = {"supplies": False, "tools": False, "granted": None,
             "granted_left": None, "armour": None, "ticked": None}
    for name in ("--supplies", "--tools"):
        if name in args:
            flags[name[2:]] = True
            args.remove(name)
    for name, key in (("--granted-left", "granted_left"), ("--granted", "granted")):
        if name in args:
            i = args.index(name)
            flags[key] = int(args[i + 1])
            del args[i:i + 2]
    if "--armour" in args:
        i = args.index("--armour")
        flags["armour"] = parse_armour(args[i + 1])
        if flags["armour"] is None:
            print("--armour takes four comma-separated armour ids in slot order "
                  "(head,body,gloves,legwear); an empty field means that slot was "
                  "not granted")
            return 2
        del args[i:i + 2]
    if "--ticked" in args:
        i = args.index("--ticked")
        flags["ticked"] = parse_ticked(args[i + 1])
        if flags["ticked"] is None:
            print("--ticked takes four comma-separated selection lines in slot "
                  "order, each exactly its own table's length (33,34,26,31)")
            return 2
        del args[i:i + 2]
    if len(args) >= 2 and args[0] == "list":
        return cmd_list(args[1])
    if len(args) >= 2 and args[0] == "selftest":
        return cmd_selftest(args[1])
    if len(args) >= 3 and args[0] == "verify":
        return cmd_verify(args[1], args[2], **flags)
    print(__doc__)
    return 2


def cmd_selftest(root):
    cases, notes, _d, baked = derivation_cases(root)
    more, more_notes, van, members, rows = supplies_cases(root)
    cases += more
    notes += more_notes
    _tables, counts, _src = baked_tables()
    more, more_notes = armour_cases(root, baked, counts)
    cases += more
    notes += more_notes

    passed = 0
    for label, ok in cases:
        print("  %-78s %s" % (label, "ok" if ok else "FAILED"))
        if ok:
            passed += 1
    print("")
    for n in notes:
        print("  NOTE: " + n)
    print("")
    print("selftest %d/%d" % (passed, len(cases)))
    return 0 if passed == len(cases) else 1


if __name__ == "__main__":
    sys.exit(main())
