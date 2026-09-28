#!/usr/bin/env python3
"""START WITH A LEFT WEAPON verification (feature 038).

A PROPERTY VALIDATOR, in the same shape as trick_weapons_verify.py and
hunter_tools_verify.py: it never re-derives the C++'s answer, it checks
invariants of generated output against trusted vanilla input. Every field offset
comes from tools/param_offsets.py against the real paramdef, and the C++'s own
offsets are PARSED OUT OF THE SOURCE rather than retyped here.

WHAT IS IMPORTED AND WHY. Feature 038 is feature 037 for the other hand, so
everything hand-agnostic is imported from trick_weapons_verify.py rather than
copied: the archive reader, the paramdef offset lookup, the 22 origin rows, the
requirement offsets, the origin-stat reader, the owner-rank mirror and the report
printer. Duplicating those would mean two tools that can disagree about what the
shipped data is. What is NOT imported is anything about WHICH field is written or
WHICH weapons are offered - this file owns that, and the membership rules come
from gen_left_hand_table.py, which owns them once.

THE ONE RULE THAT IS GENUINELY DIFFERENT. On the right hand the name test and the
obtainability test agree on every candidate, and trick_weapons_verify.py asserts
that agreement. On the left hand they do NOT: 16 of the 18 candidates are named
and only 14 are obtainable, because the game names a Wooden Shield and a Torch it
never gives a player. So L4 below asserts the disagreement in its own direction -
obtainable implies named, never the reverse - and the picker is the intersection.
Asserting agreement here would be asserting something false about correct data.

WHAT THIS CANNOT DO. It pins the rules, not the C++ implementation of them. It
cannot show the engine consults the table correctly, and it deliberately does not
predict WHICH weapon a seed draws - that is a hardware check. Nor can it show
that Bloodborne honours equip_Wep_Left. Feature 037's hardware test on 2026-09-27
settled that the game reads equip_Wep_Right on these rows and equips what it
finds; that its left-hand sibling eight bytes along behaves the same way is an
inference until the console says so.

Usage:
    python left_hand_weapons_verify.py list     <vanilla_dvdroot>
    python left_hand_weapons_verify.py selftest <vanilla_dvdroot>
    python left_hand_weapons_verify.py verify   <vanilla_dvdroot> <output_dvdroot>
                                               [--ticked <14-character line>]
                                               [--granted-right <weapon id>]
"""

import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gen_pool_table import RENDERABLE  # noqa: E402
from gen_left_hand_table import (HEADER, derive, left_hand_tier0)  # noqa: E402
from gen_weapon_table import (PLACEHOLDER_NAME, WEAPON_PARAM,  # noqa: E402
                              decompose, named_ids, obtainable_ids,
                              right_hand_tier0)
# Everything hand-agnostic, from feature 037's tool. See the module docstring.
from trick_weapons_verify import (CHARA_MEMBER, CHARA_ROW_BYTES,  # noqa: E402
                                 CLOTHING_FIELDS, COFFIN, EMPTY_SLOT, GRANT,
                                 GRANT_PROFILE, ITEM_FIELDS, ITEM_NUM_FIELDS,
                                 LINE_CHARS, FLAG_WORD, ORIGIN_ROWS,
                                 PROPER_STRENGTH, REL, REQ_OFFSETS, TIER_COUNT,
                                 TIER_STRIDE, VANILLA_START_ITEM,
                                 WEAPON_ROW_BYTES, WORKSHOP_TOOLS,
                                 apply_hunter_tools, chara_offsets, i32,
                                 load_archive, member_row_map, origin_stats,
                                 report, req_writer_sequence, set_i32,
                                 weapon_rows, wieldable)

SRC = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "src", "Randomizer")

# CHARACTER_INIT_PARAM. Named rather than numbered: the number is COMPUTED from
# the real paramdef and then checked against the C++'s constant.
EQUIP_WEP_LEFT = "equip_Wep_Left"
# Feature 037's field, eight bytes earlier. It is BOTH a thing this feature must
# never write on its own (L13) and a thing `verify` can be told to tolerate when
# the run had both features on (--granted-right).
EQUIP_WEP_RIGHT = "equip_Wep_Right"

EXPECTED_CANDIDATES = 18   # tier-0 rows with leftHandEquipable set
EXPECTED_NAMED = 16        # ...of which the game's own text names
EXPECTED_ROWS = 14         # ...of which a shop or an item lot can give a player

# How many of the 14 no origin can wield as the game ships them. Reported as an
# assertion because it is the measurement that justifies the profile existing at
# all: five of fourteen, and they are the heavy end - the Cannon, the Church
# Cannon, the Gatling Gun, Evelyn and the Repeating Pistol.
EXPECTED_UNWIELDABLE = 5

# The one row in either hand's table with no upgrade tiers. Asserted as a COUNT
# and reported as an id, never asserted as an id: "exactly one row is a base row
# and nothing else" is the property, and which row it is is data.
EXPECTED_TIERLESS = 1


def strip_line_comments(text):
    """The source with its // comments removed.

    Every shape assertion below runs against THIS, not the raw file. The pass's
    header comment necessarily names equip_Wep_Right, equip_Subwep_Left and the
    item slots in order to say that it deliberately leaves them alone, so a
    check for "that identifier does not appear" run over the raw text would be
    checking the prose rather than the code."""
    return "\n".join(line.split("//", 1)[0] for line in text.split("\n"))


def cpp_constants():
    """The numbers LeftHandWeaponGrant.cpp actually compiles, parsed out of it."""
    raw = open(os.path.join(SRC, "LeftHandWeaponGrant.cpp"), encoding="utf-8").read()
    src = strip_line_comments(raw)
    out = {}
    m = re.search(r"\bkEquipWepLeft\s*=\s*(-?\d+)", src)
    out["kEquipWepLeft"] = int(m.group(1)) if m else None
    m = re.search(r"kGrantProfile\s*=\s*\{\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+)\s*\}", src)
    out["kGrantProfile"] = tuple(int(g) for g in m.groups()) if m else None
    out["_src"] = src
    return out


# --- the mirror of the grant pass -------------------------------------------
#
# This writes what LeftHandWeaponGrant.cpp writes, from the rules rather than
# from the C++. It deliberately does NOT model the draw: which of several ticked
# weapons a seed picks is not something a mirror can or should predict.

def apply_grant(plain, members, offs, weapon, profile):
    chara = member_row_map(plain, members, CHARA_MEMBER)
    weapons = member_row_map(plain, members, WEAPON_PARAM)

    # Thirteen of the 14 have eleven rows; the Loch Shield has one. A missing
    # tier is skipped, exactly as WeaponRequirementWriter skips it.
    for tier in range(TIER_COUNT + 1):
        wid = weapon + tier * TIER_STRIDE
        if wid not in weapons:
            continue
        at = weapons[wid]
        for off, val in zip(REQ_OFFSETS, profile):
            plain[at + off] = val

    for rid in ORIGIN_ROWS:
        if rid not in chara:
            continue
        # One field, and nothing else in the 300-byte row. Not the item slots -
        # feature 037's probe route was deleted and this feature never had one -
        # and NOT equip_Wep_Right, which is the other hand's and which a
        # left-hand grant writing by mistake is the failure L13 exists to catch.
        set_i32(plain, chara[rid] + offs[EQUIP_WEP_LEFT], weapon)


def apply_right_hand_grant(plain, members, offs, weapon):
    """Feature 037's edit, reproduced so L14 can show the two hands coexist
    rather than arguing that they do. Its own tool owns the assertions about it;
    this is only enough of it to occupy the bytes."""
    chara = member_row_map(plain, members, CHARA_MEMBER)
    weapons = member_row_map(plain, members, WEAPON_PARAM)
    for tier in range(TIER_COUNT + 1):
        wid = weapon + tier * TIER_STRIDE
        if wid in weapons:
            at = weapons[wid]
            for off, val in zip(REQ_OFFSETS, GRANT_PROFILE):
                plain[at + off] = val
    for rid in ORIGIN_ROWS:
        if rid in chara:
            set_i32(plain, chara[rid] + offs[EQUIP_WEP_RIGHT], weapon)


# --- G1-G6, over a real or simulated output tree ----------------------------

def compare(van, van_m, out, out_m, offs, ticked=None, granted_right=None):
    failures, notes = [], []

    # G6 / G1
    if sorted(van_m) != sorted(out_m):
        failures.append("G6 archive member list changed")
        return failures, notes
    changed_members = []
    for name in sorted(van_m):
        (vo, vs), (oo, osz) = van_m[name], out_m[name]
        if vs != osz:
            failures.append("G6 member %s changed size %d -> %d" % (name, vs, osz))
            continue
        if van[vo:vo + vs] != out[oo:oo + osz]:
            changed_members.append(name)
    stray = [n for n in changed_members if n not in (CHARA_MEMBER, WEAPON_PARAM)]
    if stray:
        failures.append("G1 members changed that this feature never writes: %s" % stray)

    van_rows = member_row_map(van, van_m, CHARA_MEMBER)
    out_rows = member_row_map(out, out_m, CHARA_MEMBER)
    if set(van_rows) != set(out_rows):
        failures.append("G2 CharaInitParam row ids changed")
        return failures, notes

    targets = set(ORIGIN_ROWS)
    # The ONE field this feature writes, and no other byte of the row. The item
    # slots are not allowed - START WITH HUNTER TOOLS' direction is that tool's
    # job, with its own --granted-left flag - and neither is equip_Wep_Right
    # unless --granted-right says the run had feature 037 on too. Keeping the
    # default narrow is what makes "a left-hand grant only ever writes the
    # left-hand field" a checkable claim rather than a comment.
    allowed = [offs[EQUIP_WEP_LEFT] + k for k in range(4)]
    if granted_right is not None:
        allowed += [offs[EQUIP_WEP_RIGHT] + k for k in range(4)]
        notes.append("tolerating feature 037's grant of weapon %d" % granted_right)
    granted_ids = set()
    rows_written = 0

    for rid in sorted(van_rows):
        va, oa = van_rows[rid], out_rows[rid]
        vrow = van[va:va + CHARA_ROW_BYTES]
        orow = out[oa:oa + CHARA_ROW_BYTES]
        if rid not in targets:
            if vrow != orow:
                failures.append("G2 non-origin row %d changed" % rid)
            continue
        if vrow == orow:
            continue
        rows_written += 1

        # G3: only equip_Wep_Left may differ, byte for byte.
        diff = [k for k in range(CHARA_ROW_BYTES) if vrow[k] != orow[k]]
        outside = sorted(set(diff) - set(allowed))
        if outside:
            failures.append("G3 origin row %d changed outside equip_Wep_Left "
                            "(offsets %s)" % (rid, outside[:8]))

        granted_ids.add(i32(out, oa + offs[EQUIP_WEP_LEFT]))
        if granted_right is not None:
            got = i32(out, oa + offs[EQUIP_WEP_RIGHT])
            if got != granted_right:
                failures.append("G3 origin row %d has %d in equip_Wep_Right, not the "
                                "tolerated %d" % (rid, got, granted_right))

        # The starting Hunter's Mark and the four clothing entries survive.
        if (i32(out, oa + offs[ITEM_FIELDS[0]]) != VANILLA_START_ITEM or
                out[oa + offs[ITEM_NUM_FIELDS[0]]] != 1):
            failures.append("G3 origin row %d displaced the starting Hunter's Mark" % rid)
        for f in CLOTHING_FIELDS:
            if i32(van, va + offs[f]) != i32(out, oa + offs[f]):
                failures.append("G3 origin row %d changed %s" % (rid, f))

    # G4
    if rows_written != len(targets):
        failures.append("G4 %d of %d origin rows were written"
                        % (rows_written, len(targets)))
    if len(granted_ids) > 1:
        failures.append("G4 origin rows name different weapons: %s" % sorted(granted_ids))
    table_ids = {w for w, _n, _p in baked_table()}
    weapon = sorted(granted_ids)[0] if len(granted_ids) == 1 else None
    if weapon is not None:
        if weapon not in table_ids:
            failures.append("G4 granted weapon %d is not one of the %d picker rows"
                            % (weapon, EXPECTED_ROWS))
        if ticked is not None and weapon not in ticked:
            failures.append("G4 granted weapon %d was not ticked" % weapon)
        notes.append("granted weapon %d to %d origin rows" % (weapon, rows_written))

    # G5
    van_w = member_row_map(van, van_m, WEAPON_PARAM)
    out_w = member_row_map(out, out_m, WEAPON_PARAM)
    if set(van_w) != set(out_w):
        failures.append("G5 EquipParamWeapon row ids changed")
        return failures, notes
    expected = set()
    for base in (weapon, granted_right):
        if base is not None:
            expected |= {base + t * TIER_STRIDE for t in range(TIER_COUNT + 1)}
    for wid in sorted(van_w):
        va, oa = van_w[wid], out_w[wid]
        vrow = van[va:va + WEAPON_ROW_BYTES]
        orow = out[oa:oa + WEAPON_ROW_BYTES]
        if vrow == orow:
            if wid in expected and tuple(out[oa + o] for o in REQ_OFFSETS) != GRANT_PROFILE:
                failures.append("G5 weapon row %d does not carry the profile" % wid)
            continue
        diff = {k for k in range(WEAPON_ROW_BYTES) if vrow[k] != orow[k]}
        if not diff <= set(REQ_OFFSETS):
            failures.append("G5 weapon row %d changed outside the requirement bytes "
                            "(offsets %s)" % (wid, sorted(diff - set(REQ_OFFSETS))[:8]))
        if wid not in expected:
            failures.append("G5 weapon row %d had its requirements changed but is not "
                            "the granted weapon or one of its tiers" % wid)
        elif tuple(out[oa + o] for o in REQ_OFFSETS) != GRANT_PROFILE:
            failures.append("G5 weapon row %d does not carry the profile" % wid)

    return failures, notes


# --- reading the committed header -------------------------------------------

def baked_table():
    """The (id, displayName, poolEntries) rows of LeftHandWeaponTable.h, in file
    order."""
    text = open(os.path.join(SRC, HEADER), encoding="utf-8").read()
    body = text.split("kTable = {{", 1)[1].split("}};", 1)[0]
    rows = re.findall(r'\{\s*"(\d+)",\s*"([^"]*)",\s*(\d+)\s*\}', body)
    return [(int(w), n, int(p)) for w, n, p in rows]


def baked_count():
    text = open(os.path.join(SRC, HEADER), encoding="utf-8").read()
    m = re.search(r"kLeftHandWeaponCount\s*=\s*(\d+)", text)
    return int(m.group(1)) if m else -1


def decode_selection(line):
    """Mirror of ModelPoolSelection<14, false>::Decode. Returns the ticked
    indices, or None when the length guard rejects the line - which leaves the
    selection at its constructed state, and that state is NOTHING ticked."""
    if len(line) != EXPECTED_ROWS:
        return None
    return [i for i, c in enumerate(line) if c != "0"]


# --- list -------------------------------------------------------------------

def cmd_list(root):
    d = derive(root)
    names = d["names"]
    print("candidates (tier-0 rows with leftHandEquipable set): %d" % len(d["candidates"]))
    print("named by the game's text: %d    obtainable in vanilla: %d"
          % (len(d["named"]), len(d["obtainable"])))
    print("included: %d    excluded: %d" % (len(d["included"]), len(d["excluded"])))
    print("")
    print("  %-4s %-10s %-22s %s" % ("#", "id", "name", "fam/wep/ver"))
    for i, (wid, name) in enumerate(d["rows"]):
        k = decompose(wid)
        # ASCII-uppercased only: this console is cp1252 and printing the FMG's
        # own mixed-case text raises UnicodeEncodeError that reads like a parse
        # failure.
        print("  %-4d %-10d %-22s %d/%d/%d"
              % (i, wid, name, k["family"], k["weapon"], k["version"]))
    print("")
    print("excluded left-hand tier-0 rows, and why:")
    for wid in d["excluded"]:
        k = decompose(wid)
        raw = names.get(wid)
        if raw is None:
            why = "no name in the game's text"
        elif raw.strip() == PLACEHOLDER_NAME:
            why = "name is the placeholder %r" % PLACEHOLDER_NAME
        elif not raw.strip():
            why = "name is empty"
        else:
            why = "named but not obtainable"
        obt = "obtainable" if wid in d["obtainable"] else "in no shop and no item lot"
        print("  %-10d ver %d   %-34s %s" % (wid, k["version"], why, obt))
    return 0


# --- selftest ---------------------------------------------------------------

def cmd_selftest(root):
    cases = []
    notes = []

    d = derive(root)
    candidates, hand_bit = left_hand_tier0(root)
    names = d["names"]

    # L1 - the candidate set, and that the hand bit really is the sibling bit.
    right_candidates, right_bit = right_hand_tier0(root)
    cases.append(("L1 candidate set is %d left-hand tier-0 rows" % EXPECTED_CANDIDATES,
                  len(candidates) == EXPECTED_CANDIDATES, len(candidates)))
    cases.append(("L1 leftHandEquipable is bit 1 of the SAME byte as bit 0's "
                  "rightHandEquipable",
                  hand_bit[0] == right_bit[0] and right_bit[1] == 0 and
                  hand_bit[1] == 1, (hand_bit, right_bit)))
    # The two hands must not overlap at tier 0: a row in both would make the two
    # pickers able to grant the same weapon into both hands, and neither
    # feature's "only this field" claim would mean anything.
    both = sorted(set(candidates) & set(right_candidates))
    cases.append(("L1 no tier-0 row is equipable in BOTH hands", not both, both))

    # L2 - derivation by name alone.
    by_name, _ = named_ids(root, candidates)
    cases.append(("L2 derivation by NAME alone gives %d rows" % EXPECTED_NAMED,
                  len(by_name) == EXPECTED_NAMED, len(by_name)))

    # L3 - derivation by obtainability alone: ShopLineupParam and ItemLotParam,
    # nothing to do with the text. This is the GOVERNING test here.
    by_obtainable = obtainable_ids(root, candidates)
    cases.append(("L3 derivation by OBTAINABILITY alone gives %d rows" % EXPECTED_ROWS,
                  len(by_obtainable) == EXPECTED_ROWS, len(by_obtainable)))

    # L4 - the direction that must hold, and the direction that must not be
    # asserted. Feature 037's tool requires the two tests to AGREE; here that
    # would be false about correct data, so what is asserted is the implication.
    cases.append(("L4 every obtainable candidate is also NAMED",
                  by_obtainable <= by_name, sorted(by_obtainable - by_name)))
    cases.append(("L4 the picker is the intersection, %d rows" % EXPECTED_ROWS,
                  set(d["included"]) == (by_name & by_obtainable) and
                  len(d["included"]) == EXPECTED_ROWS, len(d["included"])))
    named_only = sorted(by_name - by_obtainable)
    cases.append(("L4 the two tests disagree, and every row they disagree on is "
                  "EXCLUDED",
                  bool(named_only) and all(w not in set(d["included"])
                                           for w in named_only), named_only))
    excluded = d["excluded"]
    cases.append(("L4 no excluded candidate is obtainable",
                  not [w for w in excluded if w in by_obtainable], ""))
    notes.append("excluded ids (reported, never asserted as a list): %s" % (excluded,))
    notes.append("named but unobtainable, so excluded: %s" % (named_only,))
    # The generator must REFUSE the impossible direction rather than absorbing
    # it, or L4's implication is decoration.
    rejected = False
    try:
        import gen_left_hand_table as gen
        saved = gen.named_ids
        gen.named_ids = lambda root_, cands: (set(), {})
        try:
            gen.derive(root)
        finally:
            gen.named_ids = saved
    except SystemExit:
        rejected = True
    cases.append(("L4 an obtainable-but-unnamed row is REJECTED by the generator",
                  rejected, ""))

    # L5 - the committed table.
    baked = baked_table()
    expected_rows = [(w, n) for w, n in d["rows"]]
    cases.append(("L5 the committed table is %d rows" % EXPECTED_ROWS,
                  len(baked) == EXPECTED_ROWS and baked_count() == EXPECTED_ROWS,
                  (len(baked), baked_count())))
    cases.append(("L5 the committed table is in the frozen order",
                  [(w, n) for w, n, _p in baked] == expected_rows, ""))
    baked_names = [n for _w, n, _p in baked]
    cases.append(("L5 all %d names are distinct" % EXPECTED_ROWS,
                  len(set(baked_names)) == len(baked_names), ""))
    unrenderable = sorted({c for n in baked_names for c in n if c not in RENDERABLE})
    cases.append(("L5 every name is renderable by the font atlas",
                  not unrenderable, unrenderable))
    widest = max(len(n) for n in baked_names)
    cases.append(("L5 widest name + flag word is inside %d characters" % LINE_CHARS,
                  widest + 1 + len(FLAG_WORD) <= LINE_CHARS,
                  widest + 1 + len(FLAG_WORD)))
    cases.append(("L5 every row's poolEntries is 1",
                  all(p == 1 for _w, _n, p in baked), ""))

    # L6 - every name is the FMG string for THAT EXACT id, never composed.
    cases.append(("L6 every name is the FMG string for that exact id",
                  all(n == names.get(w, "").upper() for w, n, _p in baked), ""))

    # L7 - FLATNESS. No Uncanny, no Lost, one row per weapon. This is the claim
    # that makes a 14-character config line right, so it is measured against the
    # whole param rather than against the table: if a version-1 firearm existed
    # anywhere, the table would be understating the set.
    versions = sorted({decompose(w)["version"] for w, _n, _p in baked})
    cases.append(("L7 every row is version 0", versions == [0], versions))
    families = {(decompose(w)["family"], decompose(w)["weapon"]) for w, _n, _p in baked}
    cases.append(("L7 %d rows are %d distinct weapons - one row each" % (EXPECTED_ROWS,
                                                                        EXPECTED_ROWS),
                  len(families) == EXPECTED_ROWS, len(families)))
    reqs = weapon_rows(root)
    stray_versions = sorted(w for w in reqs
                            if (decompose(w)["family"], decompose(w)["weapon"]) in families
                            and decompose(w)["version"] in (1, 2))
    cases.append(("L7 no Uncanny or Lost row exists for ANY of the 14 weapons",
                  not stray_versions, stray_versions[:6]))

    # L8 - the upgrade tiers, and the one row that has none.
    tierless = [w for w, _n, _p in baked
                if all(w + TIER_STRIDE * n not in reqs
                       for n in range(1, TIER_COUNT + 1))]
    partial = [w for w, _n, _p in baked
               if w not in tierless and
               any(w + TIER_STRIDE * n not in reqs for n in range(1, TIER_COUNT + 1))]
    cases.append(("L8 exactly %d row has NO upgrade tiers and none has only some"
                  % EXPECTED_TIERLESS,
                  len(tierless) == EXPECTED_TIERLESS and not partial,
                  (tierless, partial)))
    notes.append("the tierless row (profile covers 1 row, not 11): %s" % (tierless,))

    # L9 - the origin minimum, and wieldability before and after. The profile is
    # DERIVED from CharaInitParam here, not asserted, exactly as feature 037's
    # T8 does it - and it has to land on the same numbers, because it is the
    # same measurement of the same ten origins.
    stats = origin_stats(root)
    minimum = tuple(min(s[k] for s in stats.values()) for k in range(4))
    cases.append(("L9 the ten origins' component-wise minimum is %s" % (GRANT_PROFILE,),
                  minimum == GRANT_PROFILE, minimum))
    cases.append(("L9 every origin meets the profile, so all %d rows become wieldable"
                  % EXPECTED_ROWS,
                  all(wieldable(GRANT_PROFILE, s) for s in stats.values()), ""))
    unwieldable = [w for w, _n, _p in baked
                   if not any(wieldable(reqs[w], s) for s in stats.values())]
    cases.append(("L9 %d of %d rows are wieldable by NO origin as shipped"
                  % (EXPECTED_UNWIELDABLE, EXPECTED_ROWS),
                  len(unwieldable) == EXPECTED_UNWIELDABLE, len(unwieldable)))

    # --- the C++'s own numbers, and the archive -----------------------------
    offs, chara_size = chara_offsets(root)
    cpp = cpp_constants()
    cases.append(("C1 the paramdef's computed row size is %d" % CHARA_ROW_BYTES,
                  chara_size == CHARA_ROW_BYTES, chara_size))
    cases.append(("C1 the pass's equip_Wep_Left offset is the paramdef's",
                  cpp["kEquipWepLeft"] == offs[EQUIP_WEP_LEFT],
                  (cpp["kEquipWepLeft"], offs[EQUIP_WEP_LEFT])))
    cases.append(("C1 it is EIGHT bytes past feature 037's equip_Wep_Right",
                  cpp["kEquipWepLeft"] == offs[EQUIP_WEP_RIGHT] + 8, ""))
    cases.append(("C1 the C++'s grant profile is the measured origin minimum",
                  cpp["kGrantProfile"] == GRANT_PROFILE, cpp["kGrantProfile"]))
    src = cpp["_src"]
    cases.append(("C2 the pass writes equip_Wep_Left with the DRAWN weapon",
                  "kEquipWepLeft, weapon)" in src, ""))
    cases.append(("C2 exactly ONE write per origin row",
                  src.count("WriteI32LE") == 1, src.count("WriteI32LE")))
    cases.append(("C2 it touches no item slot and no other equip field",
                  not any(n in src for n in ("kItemIdBase", "kItemNumBase",
                                             "RowHasItem", "FirstEmptySlot",
                                             "kEquipWepRight", "equip_Subwep")), ""))
    # A weapon id is seven or eight decimal digits. None may be spelled in the
    # pass: the weapon it writes is the one it drew, never a constant.
    fixed_ids = re.findall(r"\b\d{7,8}\b", src)
    cases.append(("C2 no fixed weapon id is spelled in the pass", not fixed_ids,
                  fixed_ids))
    # It must go through the shared writer, or spec 037 D4's precedence rule -
    # which this feature inherits - is not being applied to it at all.
    cases.append(("C2 the profile goes through the shared writer as ReqOwner::Grant",
                  "reqWriter.Apply(" in src and "ReqOwner::Grant" in src, ""))

    van, van_m = load_archive(root)
    evelyn = next(w for w, n, _p in baked if n == "EVELYN")

    # L10 - nothing ticked writes nothing at all, and draws nothing.
    #
    # There is no mirror to run here: "nothing ticked" means the pass returns
    # before it reaches the archive OR the RNG, and what has to be checked is
    # that the C++ really is shaped that way. The empty-list return must come
    # BEFORE the uniform_int_distribution, or turning the setting on with
    # nothing ticked would consume a roll and move every later one.
    empty_at = src.find("if (candidates.empty()) return true;")
    draw_at = src.find("uniform_int_distribution")
    write_at = src.find("WriteI32LE")
    cases.append(("L10 nothing ticked returns before the draw and before any write",
                  empty_at > 0 and empty_at < draw_at < write_at, ""))
    cases.append(("L10 the pass draws exactly once",
                  src.count("uniform_int_distribution") == 1 and
                  src.count("(rng)") == 1, src.count("(rng)")))
    f_empty, _ = compare(van, van_m, bytearray(van), van_m, offs)
    cases.append(("L10 an unchanged archive is REJECTED as a granted run",
                  bool(f_empty), ""))

    # L11 - one ticked.
    one = bytearray(van)
    apply_grant(one, van_m, offs, evelyn, GRANT_PROFILE)
    f_one, det_one = compare(van, van_m, one, van_m, offs, ticked={evelyn})
    cases.append(("L11 a correct one-weapon grant passes G1-G6", not f_one, f_one))

    # The tierless row has to pass too: it is the only weapon in either hand's
    # table whose profile covers one row rather than eleven, and G5's "and its
    # ten tiers carry the profile" must not turn that into a failure.
    if tierless:
        shield = bytearray(van)
        apply_grant(shield, van_m, offs, tierless[0], GRANT_PROFILE)
        f_shield, _ = compare(van, van_m, shield, van_m, offs, ticked={tierless[0]})
        cases.append(("L11 granting the row with NO upgrade tiers also passes",
                      not f_shield, f_shield))

    # ...and each way of getting it wrong is rejected.
    def rejected_case(mutate, tag, ticked=None, granted_right=None):
        bad = bytearray(van)
        apply_grant(bad, van_m, offs, evelyn, GRANT_PROFILE)
        mutate(bad)
        f, _ = compare(van, van_m, bad, van_m, offs, ticked=ticked,
                       granted_right=granted_right)
        return any(tag in x for x in f), f

    chara = member_row_map(van, van_m, CHARA_MEMBER)
    weapons_map = member_row_map(van, van_m, WEAPON_PARAM)

    ok, det = rejected_case(lambda b: set_i32(b, chara[2000] + offs[EQUIP_WEP_LEFT],
                                              evelyn + 100), "G4")
    cases.append(("L11 one row naming a DIFFERENT weapon is REJECTED", ok, det[:2]))

    # THE CASE THAT MATTERS MOST HERE: writing the other hand's field. The two
    # offsets are eight bytes apart and the two features are otherwise identical,
    # so a copy-paste that grants into equip_Wep_Right would look like success.
    ok, det = rejected_case(lambda b: set_i32(b, chara[2000] + offs[EQUIP_WEP_RIGHT],
                                              evelyn), "G3")
    cases.append(("L11 writing the RIGHT hand's field is REJECTED", ok, det[:2]))

    ok, det = rejected_case(lambda b: set_i32(b, chara[2000] + offs[ITEM_FIELDS[0]],
                                              evelyn), "G3")
    cases.append(("L11 displacing the starting Hunter's Mark is REJECTED", ok, det[:2]))

    ok, det = rejected_case(lambda b: set_i32(b, chara[2000] + offs["equip_Helm"], -1),
                            "G3")
    cases.append(("L11 losing a clothing entry is REJECTED", ok, det[:2]))

    ok, det = rejected_case(lambda b: set_i32(b, chara[2000] + offs["equip_Subwep_Left"],
                                              evelyn), "G3")
    cases.append(("L11 a stray write elsewhere in the 300-byte row is REJECTED",
                  ok, det[:2]))

    non_origin = sorted(r for r in chara if r not in set(ORIGIN_ROWS))[0]
    ok, det = rejected_case(lambda b: set_i32(b, chara[non_origin] + offs[EQUIP_WEP_LEFT],
                                              evelyn), "G2")
    cases.append(("L11 writing a non-origin row is REJECTED", ok, det[:2]))

    other_member = next(n for n in van_m if n not in (CHARA_MEMBER, WEAPON_PARAM))
    ok, det = rejected_case(lambda b: b.__setitem__(
        slice(van_m[other_member][0], van_m[other_member][0] + 4), b"\xDE\xAD\xBE\xEF"),
        "G1")
    cases.append(("L11 changing another param member is REJECTED", ok, det[:2]))

    unrelated = next(w for w in sorted(weapons_map)
                     if w < evelyn or w > evelyn + TIER_COUNT * TIER_STRIDE)
    ok, det = rejected_case(
        lambda b: b.__setitem__(weapons_map[unrelated] + PROPER_STRENGTH, 99), "G5")
    cases.append(("L11 restatting an unrelated weapon is REJECTED", ok, det[:2]))

    ok, det = rejected_case(
        lambda b: b.__setitem__(weapons_map[evelyn] + PROPER_STRENGTH, 99), "G5")
    cases.append(("L11 the granted weapon missing the profile is REJECTED", ok, det[:2]))

    ok, det = rejected_case(lambda b: None, "G4", ticked={evelyn + 100})
    cases.append(("L11 granting a weapon that was not ticked is REJECTED", ok, det[:2]))

    # L12 - both hands on, in both orders. The two features write fields eight
    # bytes apart in the same row, which is exactly the shape in which one write
    # eats the other.
    saw_cleaver = 7000000
    l12 = []
    for label, order in (("right then left", ("right", "left")),
                         ("left then right", ("left", "right"))):
        both_b = bytearray(van)
        for step in order:
            if step == "right":
                apply_right_hand_grant(both_b, van_m, offs, saw_cleaver)
            else:
                apply_grant(both_b, van_m, offs, evelyn, GRANT_PROFILE)
        rows = member_row_map(both_b, van_m, CHARA_MEMBER)
        good = all(i32(both_b, rows[rid] + offs[EQUIP_WEP_LEFT]) == evelyn and
                   i32(both_b, rows[rid] + offs[EQUIP_WEP_RIGHT]) == saw_cleaver
                   for rid in ORIGIN_ROWS)
        f_both, _ = compare(van, van_m, both_b, van_m, offs, ticked={evelyn},
                            granted_right=saw_cleaver)
        l12.append((label, good, f_both))
    cases.append(("L12 both hands granted: each keeps its own field, in either order",
                  all(g for _l, g, _f in l12), [(l, g) for l, g, _f in l12]))
    cases.append(("L12 --granted-right accepts the combined tree",
                  not any(f for _l, _g, f in l12),
                  [f[:2] for _l, _g, f in l12 if f]))
    # ...and the tolerance stays narrow: a DIFFERENT weapon in the right-hand
    # field is still a failure.
    ok, det = rejected_case(lambda b: set_i32(b, chara[2000] + offs[EQUIP_WEP_RIGHT],
                                              saw_cleaver + 100), "G3",
                            ticked={evelyn}, granted_right=saw_cleaver)
    cases.append(("L12 --granted-right still rejects a DIFFERENT right-hand weapon",
                  ok, det[:2]))

    # L13 - all three features on. The grants use two equip fields and START
    # WITH HUNTER TOOLS uses the item slots, so no two of the three ever want the
    # same bytes - asserted as an exact item array rather than as presence checks.
    three = bytearray(van)
    apply_hunter_tools(three, van_m, offs)
    apply_right_hand_grant(three, van_m, offs, saw_cleaver)
    apply_grant(three, van_m, offs, evelyn, GRANT_PROFILE)
    rows = member_row_map(three, van_m, CHARA_MEMBER)
    good3 = True
    for rid in ORIGIN_ROWS:
        at = rows[rid]
        ids = [i32(three, at + offs[ITEM_FIELDS[k]]) for k in range(10)]
        if [v for v in ids if v != EMPTY_SLOT] != ([VANILLA_START_ITEM] +
                                                   list(WORKSHOP_TOOLS)):
            good3 = False
        if i32(three, at + offs[EQUIP_WEP_RIGHT]) != saw_cleaver:
            good3 = False
        if i32(three, at + offs[EQUIP_WEP_LEFT]) != evelyn:
            good3 = False
    cases.append(("L13 all three character-creation features on: every write survives",
                  good3, ""))

    # L14 - spec 037 D4's precedence, inherited. The rule lives in
    # WeaponRequirements.h and both grants and the coffin pass go through it, so
    # what is checked here is that THIS pass is ranked Grant and that the rule
    # still resolves the way D4 requires.
    coffin = (8, 7, 0, 0)
    cases.append(("L14 coffin then grant leaves the GRANT profile",
                  req_writer_sequence([(COFFIN, coffin),
                                       (GRANT, GRANT_PROFILE)]) == GRANT_PROFILE, ""))
    cases.append(("L14 grant then coffin ALSO leaves the GRANT profile",
                  req_writer_sequence([(GRANT, GRANT_PROFILE),
                                       (COFFIN, coffin)]) == GRANT_PROFILE, ""))
    # Both grants rank Grant, so where both hands drew the same weapon - which
    # cannot happen, since L1 proved no row is equipable in both hands - the
    # equal-rank rule would still let the second land. Checked because the rule
    # is shared and a future third feature could make it reachable.
    cases.append(("L14 two Grant-ranked writes on one weapon: the second still lands",
                  req_writer_sequence([(GRANT, GRANT_PROFILE),
                                       (GRANT, (1, 1, 1, 1))]) == (1, 1, 1, 1), ""))

    # L15 - the config line.
    cases.append(("L15 a %d-character line round-trips" % EXPECTED_ROWS,
                  decode_selection("1" + "0" * (EXPECTED_ROWS - 1)) == [0], ""))
    cases.append(("L15 a %d-character line leaves NOTHING ticked" % (EXPECTED_ROWS - 1),
                  decode_selection("1" * (EXPECTED_ROWS - 1)) is None, ""))
    cases.append(("L15 a %d-character line leaves NOTHING ticked" % (EXPECTED_ROWS + 1),
                  decode_selection("1" * (EXPECTED_ROWS + 1)) is None, ""))
    # Feature 037's 78-character line pasted into this key is rejected by the
    # length guard rather than remapped. That the two tables are different sizes
    # is what makes that automatic, and it is worth pinning.
    cases.append(("L15 feature 037's 78-character line is REJECTED here",
                  decode_selection("1" * 78) is None, ""))
    store = open(os.path.join(SRC, "RandomizerDefaultsStore.cpp"), encoding="utf-8").read()
    cases.append(("L15 the key is left_hand_weapons_included in both load and save",
                  store.count('"left_hand_weapons_included"') == 1 and
                  "left_hand_weapons_included=%s" in store, ""))

    return report(cases, notes)


def cmd_verify(vanilla_root, output_root, ticked=None, granted_right=None):
    opath = os.path.join(output_root, REL)
    if not os.path.exists(opath):
        print("no item-data archive in output tree - expected when no param feature was on")
        return 0
    offs, _size = chara_offsets(vanilla_root)
    van, van_m = load_archive(vanilla_root)
    out, out_m = load_archive(output_root)
    print("vanilla decompressed=%d  output decompressed=%d" % (len(van), len(out)))

    picked = None
    if ticked is not None:
        idx = decode_selection(ticked)
        if idx is None:
            print("--ticked must be exactly %d characters" % EXPECTED_ROWS)
            return 2
        rows = baked_table()
        picked = {rows[i][0] for i in idx}

    failures, notes = compare(van, van_m, out, out_m, offs, ticked=picked,
                              granted_right=granted_right)
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
    print("all invariants hold (G1-G6)")
    return 0


def main():
    args = list(sys.argv[1:])
    ticked = None
    granted_right = None
    if "--ticked" in args:
        i = args.index("--ticked")
        ticked = args[i + 1]
        del args[i:i + 2]
    if "--granted-right" in args:
        i = args.index("--granted-right")
        granted_right = int(args[i + 1])
        del args[i:i + 2]
    if len(args) >= 2 and args[0] == "list":
        return cmd_list(args[1])
    if len(args) >= 2 and args[0] == "selftest":
        return cmd_selftest(args[1])
    if len(args) >= 3 and args[0] == "verify":
        return cmd_verify(args[1], args[2], ticked=ticked, granted_right=granted_right)
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main())
