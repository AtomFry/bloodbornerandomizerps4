#!/usr/bin/env python3
"""START WITH A TRICK WEAPON verification (feature 037).

A PROPERTY VALIDATOR, in the same shape as hunter_tools_verify.py: it never
re-derives the C++'s answer, it checks invariants of generated output against
trusted vanilla input. Every field offset comes from tools/param_offsets.py
against the real paramdef, and the C++'s own offsets are PARSED OUT OF THE
SOURCE rather than retyped here - a retyped constant pins this tool's copy and
lets the shipped one drift.

The membership rules are imported from gen_weapon_table.py, so there is one
implementation of them; but membership is then RE-DERIVED a second, independent
way (by obtainability alone, and by name alone) and the two are required to
agree. A mirror that compared the table against a copy of itself would prove
nothing.

WHAT THIS CANNOT DO. It pins the rules, not the C++ implementation of them. It
cannot show the engine consults the table correctly, and it deliberately does
not predict WHICH weapon a seed draws - that is a hardware check. That
Bloodborne reads equip_Wep_Right on a character-creation row at all was settled
on the console on 2026-09-27 and not here: the milestone-3 probe build wrote two
candidate routes and the character spawned HOLDING the drawn weapon, so the
inventory route was deleted and this tool now pins the one surviving write.

Usage:
    python trick_weapons_verify.py list     <vanilla_dvdroot>
    python trick_weapons_verify.py selftest <vanilla_dvdroot>
    python trick_weapons_verify.py verify   <vanilla_dvdroot> <output_dvdroot>
                                            [--ticked <78-character line>]
"""

import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from fmg import WEAPON_NAME_FMG, load_fmg, parse_fmg  # noqa: E402
from gen_pool_table import RENDERABLE  # noqa: E402
from gen_weapon_table import (HEADER, PLACEHOLDER_NAME, WEAPON_PARAM,  # noqa: E402
                              base_id, decompose, derive, obtainable_ids,
                              right_hand_tier0)
from boss_verify import read_dcx  # noqa: E402
from param_offsets import (bnd4_members, field_offsets, load_defs,  # noqa: E402
                           load_param, param_rows)

SRC = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "src", "Randomizer")

CHARA_MEMBER = "CharaInitParam.param"

# EQUIP_PARAM_WEAPON_ST, 316-byte row. SINGLE BYTES, all four of them: a 4-byte
# write at 237 corrupts the three neighbours. StartingWeapons.cpp's header
# comment is the standing note on this.
PROPER_STRENGTH = 237
PROPER_AGILITY = 238
PROPER_MAGIC = 239   # bloodtinge
PROPER_FAITH = 240   # arcane
REQ_OFFSETS = (PROPER_STRENGTH, PROPER_AGILITY, PROPER_MAGIC, PROPER_FAITH)

# CHARACTER_INIT_PARAM, 300-byte row.
BASE_STR = 197
BASE_DEX = 198
BASE_MAG = 199
BASE_FAI = 200

# The ten player origins. 3000-3009 repeat them field for field and 3500/3501
# repeat 2000, so the stat minimum is measured once over the block that has all
# ten distinct backgrounds.
ORIGIN_STAT_ROWS = tuple(range(2000, 2010))

# The upgrade stride, confirmed by param_offsets.py's `upgrade` command: tier n
# is base + 100n, ten tiers.
TIER_STRIDE = 100
TIER_COUNT = 10

# D3's profile: the component-wise minimum of the ten origins' four stats.
# DERIVED here from the param, not asserted - T8 is what checks it lands on
# these numbers, and it could not under any other reading of properMagic /
# properFaith.
GRANT_PROFILE = (9, 9, 5, 6)

# Font8x8.cpp fits 71 characters on a line at scale 3; the picker draws the name
# and a flag word and, for this list, no id column.
LINE_CHARS = 71
FLAG_WORD = "YES"

EXPECTED_FMG_IDS = 1408
EXPECTED_ROWS = 78
EXPECTED_CANDIDATES = 85

# --- the archive, and the fields the grant writes ---------------------------

REL = os.path.join("param", "gameparam", "gameparam.parambnd.dcx")
WEAPON_ROW_BYTES = 316
CHARA_ROW_BYTES = 300

# CHARACTER_INIT_PARAM. Named rather than numbered here because the numbers are
# COMPUTED from the real paramdef below and then checked against the C++; a
# retyped offset would pin this tool's copy and let the shipped one drift.
EQUIP_WEP_RIGHT = "equip_Wep_Right"
ITEM_FIELDS = ["item_%02d" % (n + 1) for n in range(10)]
ITEM_NUM_FIELDS = ["itemNum_%02d" % (n + 1) for n in range(10)]
# The four clothing entries every origin ships with - Black Hood, Foreign Garb,
# Sullied Bandage, Foreign Trousers. They are the neighbours of the field route A
# writes, so "they survived" is the check that matters most in that row.
CLOTHING_FIELDS = ["equip_Helm", "equip_Armer", "equip_Gaunt", "equip_Leg"]

ORIGIN_ROWS = tuple(list(range(2000, 2010)) + list(range(3000, 3010)) + [3500, 3501])
VANILLA_START_ITEM = 100  # 1x Hunter's Mark, slot 0 of every origin row
EMPTY_SLOT = -1

# The two workshop tools, so T12 can show that START WITH HUNTER TOOLS' write
# survives a grant untouched. The ids are hunter_tools_verify.py's; they are here
# only to prove the two features do not interfere. Since the inventory route was
# deleted the grant touches no slot at all, which is why T12 can now demand the
# item arrays be EXACTLY the tools' own.
WORKSHOP_TOOLS = (4103, 4104)

GRANT = "Grant"
COFFIN = "CoffinSlot"


def chara_offsets(root):
    """{field name: byte offset} for CHARACTER_INIT_PARAM, from the real
    paramdef. The row size is checked too: a different computed size means every
    offset below is describing a different layout."""
    offs, size = field_offsets(load_defs(root)["CHARACTER_INIT_PARAM"])
    return {f["name"]: start for f, start, _w in offs}, size


def cpp_constants():
    """The numbers TrickWeaponGrant.cpp actually compiles, parsed out of it."""
    src = open(os.path.join(SRC, "TrickWeaponGrant.cpp"), encoding="utf-8").read()
    rows = open(os.path.join(SRC, "CharaInitRows.h"), encoding="utf-8").read()
    reqs = open(os.path.join(SRC, "WeaponRequirements.h"), encoding="utf-8").read()
    out = {}
    for name, text in (("kEquipWepRight", src),
                       ("kItemIdBase", rows), ("kItemNumBase", rows),
                       ("kItemSlots", rows), ("kRowBytes", rows),
                       ("kProperStrength", reqs), ("kProperAgility", reqs),
                       ("kProperMagic", reqs), ("kProperFaith", reqs),
                       ("kUpgradeTiers", reqs), ("kUpgradeStride", reqs)):
        m = re.search(r"\b%s\s*=\s*(-?\d+)" % name, text)
        out[name] = int(m.group(1)) if m else None
    m = re.search(r"kGrantProfile\s*=\s*\{\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+)\s*\}", src)
    out["kGrantProfile"] = tuple(int(g) for g in m.groups()) if m else None
    out["_src"] = src
    out["_rows"] = rows
    out["_reqs"] = reqs
    return out


# --- the archive ------------------------------------------------------------

def load_archive(root):
    plain = bytearray(read_dcx(os.path.join(root, REL)))
    members = {name: (off, size) for name, off, size in bnd4_members(plain)}
    return plain, members


def member_row_map(plain, members, name):
    off, size = members[name]
    member = plain[off:off + size]
    return {rid: off + rel for rid, rel in param_rows(member)}


def i32(buf, off):
    return struct.unpack_from("<i", buf, off)[0]


def set_i32(buf, off, v):
    buf[off:off + 4] = struct.pack("<i", v)


# --- the mirror of the grant pass -------------------------------------------
#
# This writes what TrickWeaponGrant.cpp writes, from the rules rather than from
# the C++. It deliberately does NOT model the draw: which of several ticked
# weapons a seed picks is not something a mirror can or should predict (see the
# module docstring), so the chosen weapon is an argument.

def apply_grant(plain, members, offs, weapon, profile):
    chara = member_row_map(plain, members, CHARA_MEMBER)
    weapons = member_row_map(plain, members, WEAPON_PARAM)

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
        # One field, and nothing else in the 300-byte row. The item slots are
        # deliberately NOT touched: the inventory route was deleted once the
        # probe showed equip_Wep_Right is read.
        set_i32(plain, chara[rid] + offs[EQUIP_WEP_RIGHT], weapon)


def apply_hunter_tools(plain, members, offs):
    """hunter_tools_verify.py's own edit, reproduced so T12 can show the two
    features coexist rather than arguing that they do."""
    chara = member_row_map(plain, members, CHARA_MEMBER)
    for rid in ORIGIN_ROWS:
        if rid not in chara:
            continue
        at = chara[rid]
        for tool in WORKSHOP_TOOLS:
            ids = [i32(plain, at + offs[ITEM_FIELDS[k]]) for k in range(10)]
            if tool in ids:
                continue
            free = [k for k in range(10) if ids[k] == EMPTY_SLOT]
            if not free:
                break
            set_i32(plain, at + offs[ITEM_FIELDS[free[0]]], tool)
            plain[at + offs[ITEM_NUM_FIELDS[free[0]]]] = 1


# --- G1-G6, over a real or simulated output tree ----------------------------

def compare(van, van_m, out, out_m, offs, ticked=None):
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
    # The ONE field the shipped route writes, and no other byte of the row. The
    # item slots are NOT allowed here: allowing them is what the deleted
    # inventory route needed, and a tree still carrying that write must fail.
    #
    # The consequence, deliberately accepted: an output tree that also had START
    # WITH HUNTER TOOLS on carries tool ids in those slots and is reported as a
    # G3 failure by this tool. That direction is hunter_tools_verify.py's job,
    # with its --granted flag; a grant-only tree is what `verify` here describes.
    allowed = [offs[EQUIP_WEP_RIGHT] + k for k in range(4)]
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

        # G3: only equip_Wep_Right may differ, byte for byte.
        diff = [k for k in range(CHARA_ROW_BYTES) if vrow[k] != orow[k]]
        outside = sorted(set(diff) - set(allowed))
        if outside:
            failures.append("G3 origin row %d changed outside equip_Wep_Right "
                            "(offsets %s)" % (rid, outside[:8]))

        granted_ids.add(i32(out, oa + offs[EQUIP_WEP_RIGHT]))

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
    if weapon is not None:
        expected = {weapon + t * TIER_STRIDE for t in range(TIER_COUNT + 1)}
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


# --- the owner-ranked writer, as a rule rather than as the C++ --------------

REQ_RANK = {COFFIN: 0, GRANT: 1}


def req_writer_sequence(writes):
    """Mirror of WeaponRequirementWriter: a list of (owner, profile) applied to
    one weapon, returning the profile left behind. Higher rank wins, and an
    equal rank still lands."""
    claimed = None
    value = None
    for owner, profile in writes:
        if claimed is not None and REQ_RANK[owner] < REQ_RANK[claimed]:
            continue
        claimed, value = owner, profile
    return value


# --- reading the two params -------------------------------------------------

def weapon_rows(root):
    """{weapon id: (str, skl, blt, arc)} for every row in the weapon param."""
    p = load_param(root, WEAPON_PARAM)
    out = {}
    for wid, off in param_rows(p):
        out[wid] = tuple(p[off + o] for o in REQ_OFFSETS)
    return out


def origin_stats(root):
    """{origin row id: (str, skl, blt, arc)} for the ten backgrounds."""
    p = load_param(root, CHARA_MEMBER)
    out = {}
    for rid, off in param_rows(p):
        if rid in ORIGIN_STAT_ROWS:
            out[rid] = (p[off + BASE_STR], p[off + BASE_DEX],
                        p[off + BASE_MAG], p[off + BASE_FAI])
    return out


def wieldable(req, stats):
    return all(r <= s for r, s in zip(req, stats))


# --- reading the committed header -------------------------------------------

def baked_table():
    """The (id, displayName) rows of TrickWeaponTable.h, in file order."""
    text = open(os.path.join(SRC, HEADER), encoding="utf-8").read()
    body = text.split("kTable = {{", 1)[1].split("}};", 1)[0]
    rows = re.findall(r'\{\s*"(\d+)",\s*"([^"]*)",\s*(\d+)\s*\}', body)
    return [(int(w), n, int(p)) for w, n, p in rows]


def baked_count():
    text = open(os.path.join(SRC, HEADER), encoding="utf-8").read()
    m = re.search(r"kTrickWeaponCount\s*=\s*(\d+)", text)
    return int(m.group(1)) if m else -1


# --- list -------------------------------------------------------------------

def cmd_list(root):
    d = derive(root)
    names = d["names"]
    print("candidates (tier-0 rows with rightHandEquipable set): %d"
          % len(d["candidates"]))
    print("included: %d    excluded: %d" % (len(d["included"]), len(d["excluded"])))
    print("")
    print("  %-4s %-10s %-30s %s" % ("#", "id", "name", "fam/wep/ver"))
    for i, (wid, name) in enumerate(d["rows"]):
        k = decompose(wid)
        # ASCII-uppercased only: this console is cp1252 and printing the FMG's
        # own mixed-case text, or its Japanese member name, raises
        # UnicodeEncodeError that reads like a parse failure.
        print("  %-4d %-10d %-30s %d/%d/%d"
              % (i, wid, name, k["family"], k["weapon"], k["version"]))
    print("")
    print("excluded right-hand tier-0 rows, and why:")
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

    # T1 - the FMG frames validly by both checks, and covers 1408 ids.
    fmg_ok, fmg_detail = True, ""
    try:
        entries, ids = load_fmg(root, WEAPON_NAME_FMG)
    except Exception as exc:  # noqa: BLE001 - the message is the finding
        fmg_ok, fmg_detail, entries, ids = False, str(exc), {}, 0
    cases.append(("T1 name FMG frames validly and covers %d ids" % EXPECTED_FMG_IDS,
                  fmg_ok and ids == EXPECTED_FMG_IDS and
                  entries.get(7000000) == "Saw Cleaver", fmg_detail))

    # A corrupt header must be REJECTED, or the two framing equalities are
    # decoration. Flip the group count and the offset table stops matching it.
    rejected = False
    try:
        raw = _raw_name_fmg(root)
        bad = bytearray(raw)
        struct.pack_into("<i", bad, 0x0C, EXPECTED_FMG_IDS + 1)
        parse_fmg(bytes(bad), member_size=len(raw))
    except Exception:  # noqa: BLE001
        rejected = True
    cases.append(("T1 a wrong group count in the FMG header is REJECTED", rejected, ""))

    d = derive(root)
    candidates, _bit = right_hand_tier0(root)
    names = d["names"]

    cases.append(("T1 candidate set is %d right-hand tier-0 rows" % EXPECTED_CANDIDATES,
                  len(candidates) == EXPECTED_CANDIDATES, ""))

    # T2 - derivation by name alone.
    by_name = {w for w in candidates
               if names.get(w, "").strip() not in ("", PLACEHOLDER_NAME)}
    cases.append(("T2 derivation by NAME alone gives exactly %d rows" % EXPECTED_ROWS,
                  len(by_name) == EXPECTED_ROWS, sorted(by_name)[:3]))

    # T3 - derivation by obtainability alone, a genuinely separate measurement:
    # ShopLineupParam and ItemLotParam, nothing to do with the text.
    by_obtainable = obtainable_ids(root, candidates)
    cases.append(("T3 derivation by OBTAINABILITY alone gives exactly %d rows"
                  % EXPECTED_ROWS, len(by_obtainable) == EXPECTED_ROWS, ""))

    # T4 - the two agree, and every other candidate fails BOTH.
    excluded = [w for w in candidates if w not in by_name]
    fails_both = [w for w in excluded if w not in by_obtainable]
    cases.append(("T4 the two derivations name the same rows",
                  by_name == by_obtainable,
                  sorted(by_name ^ by_obtainable)))
    cases.append(("T4 every excluded candidate fails BOTH tests",
                  len(fails_both) == len(excluded) and len(excluded) > 0, ""))
    notes.append("excluded ids (reported, never asserted as a list): %s" % (excluded,))

    # T5 - the committed table.
    baked = baked_table()
    expected_rows = [(w, n) for w, n in d["rows"]]
    cases.append(("T5 the committed table is %d rows" % EXPECTED_ROWS,
                  len(baked) == EXPECTED_ROWS and baked_count() == EXPECTED_ROWS,
                  len(baked)))
    cases.append(("T5 the committed table is in the frozen order",
                  [(w, n) for w, n, _p in baked] == expected_rows, ""))
    baked_names = [n for _w, n, _p in baked]
    cases.append(("T5 all %d names are distinct" % EXPECTED_ROWS,
                  len(set(baked_names)) == len(baked_names), ""))
    unrenderable = sorted({c for n in baked_names for c in n if c not in RENDERABLE})
    cases.append(("T5 every name is renderable by Font8x8.cpp",
                  not unrenderable, unrenderable))
    widest = max(len(n) for n in baked_names)
    cases.append(("T5 widest name + flag word is inside %d characters" % LINE_CHARS,
                  widest + 1 + len(FLAG_WORD) <= LINE_CHARS,
                  widest + 1 + len(FLAG_WORD)))
    cases.append(("T5 every row's poolEntries is 1",
                  all(p == 1 for _w, _n, p in baked), ""))

    # T6 - every name is the FMG string for THAT EXACT id, never composed. The
    # composed form is built here and required to DISAGREE for four of the 26,
    # so the check cannot pass by accident on a prefix rule.
    read_ok = all(n == names.get(w, "").upper() for w, n, _p in baked)
    cases.append(("T6 every name is the FMG string for that exact id", read_ok, ""))
    composed_wrong = []
    for w, n, _p in baked:
        ver = decompose(w)["version"]
        if ver == 0:
            continue
        prefix = "UNCANNY " if ver == 1 else "LOST "
        if prefix + names.get(base_id(w), "").upper() != n:
            composed_wrong.append(n)
    cases.append(("T6 a composed prefix name would be wrong for 8 rows (4 weapons)",
                  len(composed_wrong) == 8, composed_wrong))

    # T7 - all ten upgrade tiers exist for every row, so the profile covers 11.
    reqs = weapon_rows(root)
    missing_tiers = [w for w, _n, _p in baked
                     if any(w + TIER_STRIDE * n not in reqs
                            for n in range(1, TIER_COUNT + 1))]
    cases.append(("T7 all %d rows have all ten +100n tiers" % EXPECTED_ROWS,
                  not missing_tiers, missing_tiers))

    # T8 - the origin minimum, and wieldability before and after.
    stats = origin_stats(root)
    minimum = tuple(min(s[k] for s in stats.values()) for k in range(4))
    cases.append(("T8 the ten origins' component-wise minimum is %s"
                  % (GRANT_PROFILE,),
                  len(stats) == len(ORIGIN_STAT_ROWS) and minimum == GRANT_PROFILE,
                  minimum))
    # A weapon carrying the profile is wieldable by an origin exactly when that
    # origin's four stats all meet it - which every origin does, by construction
    # of the minimum. Checked rather than argued, and checked for all ten so a
    # single outlier origin cannot hide.
    cases.append(("T8 every origin meets the profile, so all %d rows become wieldable"
                  % EXPECTED_ROWS,
                  all(wieldable(GRANT_PROFILE, s) for s in stats.values()), ""))
    unwieldable = [w for w, _n, _p in baked
                   if not any(wieldable(reqs[w], s) for s in stats.values())]
    cases.append(("T8 39 of %d rows are wieldable by NO origin as shipped"
                  % EXPECTED_ROWS, len(unwieldable) == 39, len(unwieldable)))

    # T9 - the three versions of each weapon carry identical requirements.
    by_weapon = {}
    for w, _n, _p in baked:
        by_weapon.setdefault(base_id(w), []).append(w)
    differing = [b for b, ws in by_weapon.items() if len({reqs[w] for w in ws}) != 1]
    cases.append(("T9 all three versions of each of the 26 weapons share requirements",
                  len(by_weapon) == 26 and not differing, differing))

    # --- M3: the C++'s own numbers, and the archive ------------------------
    offs, chara_size = chara_offsets(root)
    cpp = cpp_constants()
    cases.append(("M3 the paramdef's computed row size is %d" % CHARA_ROW_BYTES,
                  chara_size == CHARA_ROW_BYTES, chara_size))
    cases.append(("M3 TrickWeaponGrant.cpp's equip_Wep_Right offset is the paramdef's",
                  cpp["kEquipWepRight"] == offs[EQUIP_WEP_RIGHT],
                  (cpp["kEquipWepRight"], offs[EQUIP_WEP_RIGHT])))
    cases.append(("M3 CharaInitRows.h's row shape is the paramdef's",
                  cpp["kItemIdBase"] == offs[ITEM_FIELDS[0]] and
                  cpp["kItemNumBase"] == offs[ITEM_NUM_FIELDS[0]] and
                  cpp["kItemSlots"] == 10 and
                  cpp["kRowBytes"] == CHARA_ROW_BYTES, ""))
    cases.append(("M3 WeaponRequirements.h's four offsets are the paramdef's",
                  (cpp["kProperStrength"], cpp["kProperAgility"],
                   cpp["kProperMagic"], cpp["kProperFaith"]) == REQ_OFFSETS and
                  cpp["kUpgradeTiers"] == TIER_COUNT and
                  cpp["kUpgradeStride"] == TIER_STRIDE, ""))
    cases.append(("M3 the C++'s grant profile is the measured origin minimum",
                  cpp["kGrantProfile"] == GRANT_PROFILE, cpp["kGrantProfile"]))
    # The shipped shape, replacing the probe build's two-route cases. The probe
    # answered on 2026-09-27 - the character spawned HOLDING the drawn weapon -
    # so the inventory route and its fixed Threaded Cane came out, and what has
    # to be pinned now is that they STAYED out. One write per origin row, of the
    # DRAWN weapon, and no fixed weapon id anywhere in the pass.
    cases.append(("M4 the pass writes equip_Wep_Right with the DRAWN weapon",
                  "kEquipWepRight, weapon)" in cpp["_src"], ""))
    cases.append(("M4 exactly ONE write per origin row",
                  cpp["_src"].count("WriteI32LE") == 1, cpp["_src"].count("WriteI32LE")))
    cases.append(("M4 the deleted inventory route left nothing behind",
                  not any(n in cpp["_src"] for n in ("kProbeInventoryWeapon",
                                                     "kItemIdBase", "kItemNumBase",
                                                     "RowHasItem", "FirstEmptySlot")), ""))
    # A weapon id is seven or eight decimal digits. None may be spelled in the
    # pass: the weapon it writes is the one it drew, never a constant.
    fixed_ids = re.findall(r"\b\d{7,8}\b", cpp["_src"])
    cases.append(("M4 no fixed weapon id is spelled in the pass",
                  not fixed_ids, fixed_ids))

    van, van_m = load_archive(root)
    saw_cleaver = 7000000

    # T10 - nothing ticked writes nothing at all, and draws nothing.
    #
    # There is no mirror to run here: "nothing ticked" means the pass returns
    # before it reaches the archive OR the RNG, and what has to be checked is
    # that the C++ really is shaped that way. The empty-list return must come
    # BEFORE the uniform_int_distribution, or turning the setting on with
    # nothing ticked would consume a roll and move every later one.
    src = cpp["_src"]
    empty_at = src.find("if (candidates.empty()) return true;")
    draw_at = src.find("uniform_int_distribution")
    write_at = src.find("WriteI32LE")
    cases.append(("T10 nothing ticked returns before the draw and before any write",
                  empty_at > 0 and empty_at < draw_at < write_at, ""))
    cases.append(("T10 the pass draws exactly once",
                  src.count("uniform_int_distribution") == 1 and
                  src.count("(rng)") == 1, src.count("(rng)")))
    f_empty, _ = compare(van, van_m, bytearray(van), van_m, offs)
    cases.append(("T10 an unchanged archive is REJECTED as a granted run",
                  bool(f_empty), ""))

    # T11 - one ticked, the one shipped route.
    one = bytearray(van)
    apply_grant(one, van_m, offs, saw_cleaver, GRANT_PROFILE)
    f_one, _ = compare(van, van_m, one, van_m, offs, ticked={saw_cleaver})
    cases.append(("T11 a correct one-weapon grant passes G1-G6", not f_one, f_one))

    # ...and each way of getting it wrong is rejected.
    def rejected(mutate, tag, ticked=None):
        bad = bytearray(van)
        apply_grant(bad, van_m, offs, saw_cleaver, GRANT_PROFILE)
        mutate(bad)
        f, _ = compare(van, van_m, bad, van_m, offs, ticked=ticked)
        return any(tag in x for x in f), f

    chara = member_row_map(van, van_m, CHARA_MEMBER)
    weapons_map = member_row_map(van, van_m, WEAPON_PARAM)

    ok, det = rejected(lambda b: set_i32(b, chara[2000] + offs[EQUIP_WEP_RIGHT],
                                         saw_cleaver + 10000), "G4")
    cases.append(("T11 one row naming a DIFFERENT version is REJECTED", ok, det[:2]))

    # The DELETED inventory route. A tree that still carried it would put a
    # weapon id in a free item slot, which G3 allowed during the probe build and
    # must not allow now.
    def add_inventory_write(b):
        at = chara[2000]
        free = next(k for k in range(10)
                    if i32(b, at + offs[ITEM_FIELDS[k]]) == EMPTY_SLOT)
        set_i32(b, at + offs[ITEM_FIELDS[free]], saw_cleaver)
        b[at + offs[ITEM_NUM_FIELDS[free]]] = 1
    ok, det = rejected(add_inventory_write, "G3")
    cases.append(("T11 a tree still carrying the DELETED inventory write is REJECTED",
                  ok, det[:2]))

    ok, det = rejected(lambda b: set_i32(b, chara[2000] + offs[ITEM_FIELDS[0]],
                                         saw_cleaver), "G3")
    cases.append(("T11 displacing the starting Hunter's Mark is REJECTED", ok, det[:2]))

    ok, det = rejected(lambda b: set_i32(b, chara[2000] + offs["equip_Helm"], -1), "G3")
    cases.append(("T11 losing a clothing entry is REJECTED", ok, det[:2]))

    ok, det = rejected(lambda b: set_i32(b, chara[2000] + offs["equip_Wep_Left"],
                                         saw_cleaver), "G3")
    cases.append(("T11 a stray write elsewhere in the 300-byte row is REJECTED",
                  ok, det[:2]))

    non_origin = sorted(r for r in chara if r not in set(ORIGIN_ROWS))[0]
    ok, det = rejected(lambda b: set_i32(b, chara[non_origin] + offs[EQUIP_WEP_RIGHT],
                                         saw_cleaver), "G2")
    cases.append(("T11 writing a non-origin row is REJECTED", ok, det[:2]))

    other_member = next(n for n in van_m if n not in (CHARA_MEMBER, WEAPON_PARAM))
    ok, det = rejected(lambda b: b.__setitem__(
        slice(van_m[other_member][0], van_m[other_member][0] + 4), b"\xDE\xAD\xBE\xEF"),
        "G1")
    cases.append(("T11 changing another param member is REJECTED", ok, det[:2]))

    unrelated = next(w for w in sorted(weapons_map)
                     if w < saw_cleaver or w > saw_cleaver + TIER_COUNT * TIER_STRIDE)
    ok, det = rejected(lambda b: b.__setitem__(weapons_map[unrelated] + PROPER_STRENGTH, 99),
                       "G5")
    cases.append(("T11 restatting an unrelated weapon is REJECTED", ok, det[:2]))

    ok, det = rejected(lambda b: b.__setitem__(weapons_map[saw_cleaver] + PROPER_STRENGTH, 99),
                       "G5")
    cases.append(("T11 the granted weapon missing the profile is REJECTED", ok, det[:2]))

    ok, det = rejected(lambda b: None, "G4", ticked={saw_cleaver + 10000})
    cases.append(("T11 granting a weapon that was not ticked is REJECTED", ok, det[:2]))

    # T12 - both features on, in both orders, and neither takes the other's slot.
    t12 = []
    for label, order in (("tools then grant", ("tools", "grant")),
                         ("grant then tools", ("grant", "tools"))):
        both = bytearray(van)
        for step in order:
            if step == "tools":
                apply_hunter_tools(both, van_m, offs)
            else:
                apply_grant(both, van_m, offs, saw_cleaver, GRANT_PROFILE)
        good = True
        for rid in ORIGIN_ROWS:
            at = member_row_map(both, van_m, CHARA_MEMBER)[rid]
            slots = [(i32(both, at + offs[ITEM_FIELDS[k]]),
                      both[at + offs[ITEM_NUM_FIELDS[k]]]) for k in range(10)]
            ids = [v for v, _c in slots]
            if ids[0] != VANILLA_START_ITEM:
                good = False
            for want in WORKSHOP_TOOLS:
                if ids.count(want) != 1 or slots[ids.index(want)][1] != 1:
                    good = False
            # The grant takes no slot at all now, so the item array is exactly
            # the Hunter's Mark plus the two tools and nothing else.
            if [v for v in ids if v != EMPTY_SLOT] != ([VANILLA_START_ITEM] +
                                                       list(WORKSHOP_TOOLS)):
                good = False
            if i32(both, at + offs[EQUIP_WEP_RIGHT]) != saw_cleaver:
                good = False
        t12.append((label, good))
    cases.append(("T12 both features on: every write survives, in either order",
                  all(g for _l, g in t12), t12))

    # T13 - B6, both ways round, against the writer's RULE.
    coffin = (8, 7, 0, 0)
    cases.append(("T13 coffin then grant leaves the GRANT profile",
                  req_writer_sequence([(COFFIN, coffin),
                                       (GRANT, GRANT_PROFILE)]) == GRANT_PROFILE, ""))
    cases.append(("T13 grant then coffin ALSO leaves the GRANT profile",
                  req_writer_sequence([(GRANT, GRANT_PROFILE),
                                       (COFFIN, coffin)]) == GRANT_PROFILE, ""))
    cases.append(("T13 two coffin slots on one weapon: the second still lands",
                  req_writer_sequence([(COFFIN, coffin),
                                       (COFFIN, (7, 9, 0, 0))]) == (7, 9, 0, 0), ""))
    # ...and the rule the mirror encodes is the one the C++ compiles.
    reqs_src = cpp["_reqs"]
    cases.append(("T13 the C++ ranks Grant above CoffinSlot and compares with <",
                  "CoffinSlot = 0" in reqs_src and "Grant = 1" in reqs_src and
                  "(int)owner < (int)existing->owner) return 0" in reqs_src, ""))
    sw_src = open(os.path.join(SRC, "StartingWeapons.cpp"), encoding="utf-8").read()
    cases.append(("T13 row 5 writes as CoffinSlot and the grant as Grant",
                  "ReqOwner::CoffinSlot" in sw_src and
                  "ReqOwner::Grant" in cpp["_src"] and
                  "ReqOwner::Grant" not in sw_src, ""))

    # T14 - the config line.
    cases.append(("T14 a %d-character line round-trips" % EXPECTED_ROWS,
                  decode_selection("1" + "0" * (EXPECTED_ROWS - 1)) == [0], ""))
    cases.append(("T14 a 77-character line leaves NOTHING ticked",
                  decode_selection("1" * (EXPECTED_ROWS - 1)) is None, ""))
    cases.append(("T14 a 79-character line leaves NOTHING ticked",
                  decode_selection("1" * (EXPECTED_ROWS + 1)) is None, ""))
    store = open(os.path.join(SRC, "RandomizerDefaultsStore.cpp"), encoding="utf-8").read()
    cases.append(("T14 the key is trick_weapons_included in both load and save",
                  store.count('"trick_weapons_included"') == 1 and
                  "trick_weapons_included=%s" in store, ""))

    return report(cases, notes)


def decode_selection(line):
    """Mirror of ModelPoolSelection<78, false>::Decode. Returns the ticked
    indices, or None when the length guard rejects the line - which leaves the
    selection at its constructed state, and that state is NOTHING ticked."""
    if len(line) != EXPECTED_ROWS:
        return None
    return [i for i, c in enumerate(line) if c != "0"]


def cmd_verify(vanilla_root, output_root, ticked=None):
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

    failures, notes = compare(van, van_m, out, out_m, offs, ticked=picked)
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


def _raw_name_fmg(root):
    """The undecoded weapon-name FMG member, for the negative framing case."""
    from boss_verify import read_dcx
    from fmg import ITEM_MSGBND
    from param_offsets import bnd4_members
    buf = read_dcx(os.path.join(root, ITEM_MSGBND))
    for name, off, size in bnd4_members(buf):
        if name == WEAPON_NAME_FMG:
            return bytes(buf[off:off + size])
    raise SystemExit("weapon-name FMG member not found")


def report(cases, notes=()):
    passed = 0
    for label, ok, detail in cases:
        print("  %-64s %s" % (label, "ok" if ok else "FAILED"))
        if not ok and detail:
            print("     was: %s" % (detail,))
        if ok:
            passed += 1
    for n in notes:
        print("  NOTE: " + n)
    print("")
    print("selftest %d/%d" % (passed, len(cases)))
    return 0 if passed == len(cases) else 1


def main():
    args = list(sys.argv[1:])
    ticked = None
    if "--ticked" in args:
        i = args.index("--ticked")
        ticked = args[i + 1]
        del args[i:i + 2]
    if len(args) >= 2 and args[0] == "list":
        return cmd_list(args[1])
    if len(args) >= 2 and args[0] == "selftest":
        return cmd_selftest(args[1])
    if len(args) >= 3 and args[0] == "verify":
        return cmd_verify(args[1], args[2], ticked=ticked)
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main())
