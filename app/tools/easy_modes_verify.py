#!/usr/bin/env python3
"""Checking EASY SHADOWS / EASY ROM / EASY FAILURES / EASY EMISSARY.

The feature's whole data is six rows in src/Randomizer/EasyModes.h - a flag, an
exact map name and a list of placement-name patterns - plus three baked
constants naming the replacement creature. Both are PARSED out of the header
here, never restated, for the reason enemy_lookup parses the exclusion lists:
a second copy in the test pins the test's copy and lets the shipped one drift,
which is the one thing this tool exists to catch.

  show      print what the table resolves to against a vanilla tree.
  verify    the output-tree contract: the targets replaced, the survivors and
            the settings-off maps left alone, the two param rows untouched.
  selftest  the twelve cases of plan 018 section 6, against the vanilla tree.

WHAT THIS PINS, AND WHAT IT CANNOT. It pins the RULES and the DATA: that the
table selects the placements the reference selects, that the replacement
identity is real, that the replacement is 2 HP and 18 echoes rather than
something dangerous, and that a generated tree has the properties the contract
demands. It does NOT pin the C++: there is no host C++ compiler in this
project, so ApplyEasyModes and this mirror can drift and only hardware would
notice. And nothing here - no field of any param, no count of anything - can
establish that the replacement is HARMLESS or that the four fights still END.
Those two questions are hardware's alone (plan 018 section 6, Hardware).

"Unchanged" never means byte-identical to vanilla anywhere in this tool. The
stat pass runs on every map on every run and rewrites the NPCParamID of Rom,
the surviving Shadow, the surviving Living Failure, the three Upper Cathedral
emissaries and the Celestial Emissary, and every .msb.dcx is re-serialised and
re-compressed regardless. Frozen means pool_verify._frozen's definition: same
model, same think, and an npc that is either the vanilla value or that map's
zone-scaled variant of it.

Usage:
    python easy_modes_verify.py show     <vanilla_dvdroot>
    python easy_modes_verify.py verify   <vanilla_dvdroot> <output_dvdroot>
                                         [shadows] [rom] [failures] [emissary]
                                         [--no-randomizers]
    python easy_modes_verify.py selftest <vanilla_dvdroot>

`verify` takes the settings the run actually had ON. --no-randomizers says the
run had every randomizer off as well, which is what lets the survivor and
settings-off checks be ASSERTED rather than merely reported: with the enemy or
boss pass on, a changed survivor is that pass doing its job and boss_verify.py
/ pool_verify.py are the tools that judge it, not this one.
"""

import copy
import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from boss_verify import (read_dcx, _i32, _i64, _utf16z, _set_i32,  # noqa: E402
                         PART_MODEL_INDEX, PART_BASE_DATA_OFFSET,
                         PART_TYPE_DATA_OFFSET, ENEMY_NPC_IN_TYPEDATA,
                         ENEMY_THINK_IN_TYPEDATA)
from enemy_lookup import (BASE_MAPS, load_base_maps, load_map, enemies,  # noqa: E402
                          zone_scaled_npc)
from pool_verify import _frozen  # noqa: E402
from param_offsets import load_param, param_rows  # noqa: E402

SRC = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                   "..", "src", "Randomizer")
REPO = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..")
REF_XAML = os.path.join(REPO, "reference", "Randomizer", "MainWindowComponents",
                        "MainWindow.xaml.cs")

FLAGS = ("shadows", "rom", "failures", "emissary")

# What the six rows must resolve to. Stated here so case 1 is an assertion
# rather than a printout - the header says which patterns, this says which
# placements they have to reach in the real tree.
EXPECTED = {
    "m27_00_00_01": {"c2120_0001", "c2120_0002"},
    "m32_00_00_00": {"c1400_%04d" % i for i in range(30)},
    "m32_00_00_01": {"c1400_%04d" % i for i in range(30)},
    "m35_00_00_00": {"c4030_0001", "c4030_0002", "c4030_0003"},
    "m24_02_00_00": {"c2500_0001", "c2500_0002", "c2500_0003", "c2500_0006",
                     "c2500_0007", "c2500_0009", "c2500_0010"},
    "m24_02_00_01": {"c2500_0001", "c2500_0002", "c2500_0003", "c2500_0006",
                     "c2500_0007", "c2500_0009", "c2500_0010"},
}
TREE_TOTAL = 79    # across the 24 loaded maps
RETAIL_TOTAL = 42  # the same, minus the two variants the retail game never loads

# Spec 018 B9's survivors, per map. Everything the four fights keep.
SURVIVORS = {
    "m27_00_00_01": ["c2120_0000"],
    "m32_00_00_00": ["c5100_0000"],
    "m32_00_00_01": ["c5100_0000"],
    "m35_00_00_00": ["c4030_0000", "c4030_0004"],
    "m24_02_00_00": ["c2500_0000", "c2570_0001",
                     "c2500_0011", "c2500_0012", "c2500_0013"],
    "m24_02_00_01": ["c2500_0000", "c2570_0001",
                     "c2500_0011", "c2500_0012", "c2500_0013"],
}

# The four arena leaders no pattern may ever reach, as bare names.
ARENA_LEADERS = ("c2120_0000", "c2500_0000", "c2570_0001", "c4030_0000")

# Spec 018 D3: the Forbidden Woods variant the reference does NOT patch, even
# though it holds the same three Shadow placements. Its omission is agreed
# behaviour, so it is asserted rather than assumed.
UNPATCHED_TWIN = "m27_00_00_00"
UNPATCHED_TWIN_SHADOWS = {"c2120_0000": 212700, "c2120_0001": 212710,
                          "c2120_0002": 212720}

# NpcParam field offsets, from param_offsets.py against the real PARAMDEF.
NPC_BEHAVIOR_VARIATION = 0    # s32
NPC_HIT_HEIGHT = 16           # f32
NPC_HIT_RADIUS = 20           # f32
NPC_HP = 32                   # u32
NPC_GET_SOUL = 40             # u32
NPC_ITEM_LOT_1 = 44           # s32
NPC_ITEM_LOT_2 = 48           # s32
NPC_TEAM_TYPE = 303           # u8

# ItemLotParam field offsets, same source.
LOT_ITEM_IDS = 0              # lotItemId01..08, 8 x s32
LOT_BASE_POINT_01 = 64        # u16
LOT_GET_ITEM_FLAG = 128       # s32

# The larva's own stat row and its 31 zone-tuned variants.
LARVA_SCALED_ROWS = list(range(900014601, 900014632))
CORD_ITEM = 4321              # One Third of Umbilical Cord
CORD_LOT = 28040
CORD_FLAG = 50001205

PARAM_REL = os.path.join("param", "gameparam", "gameparam.parambnd.dcx")


# --- the shipped header ----------------------------------------------------

def _strip_comments(text):
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def header_text():
    return open(os.path.join(SRC, "EasyModes.h"), encoding="utf-8").read()


def parse_constants():
    """(npc, think, model) out of EasyModes.h's three baked constants."""
    text = _strip_comments(header_text())
    npc = int(re.search(r"kEasyModeNpcParamId\s*=\s*(-?\d+)", text).group(1))
    think = int(re.search(r"kEasyModeThinkParamId\s*=\s*(-?\d+)", text).group(1))
    model = re.search(r'kEasyModeModelName\s*=\s*"(\w+)"', text).group(1)
    return npc, think, model


def parse_table():
    """[(flag, map name, [patterns])] out of EasyModes.h's six rows."""
    body = _strip_comments(header_text()).split("kTable = {{", 1)[1].split("}};", 1)[0]
    rows = []
    for flag, mapname, pats in re.findall(
            r'\{\s*EasyModeFlag::k(\w+)\s*,\s*"(\w+)"\s*,\s*\{(.*?)\}\s*\}', body, re.S):
        rows.append((flag.lower(), mapname, re.findall(r'"([^"]+)"', pats)))
    return rows


def matched_in(m, patterns):
    """The placement names `patterns` reaches in one parsed map.

    Substring matching, mirroring the header's own rule and the reference's
    Name.Contains. Safe only because the caller has already keyed on the exact
    map name - c1400 matches in two chalice maps and c2500_0001 in nine maps
    (plan-evidence 018 E4 M5)."""
    return {e["name"] for e in enemies(m)
            if any(p in e["name"] for p in patterns)}


def selected(maps, flags=FLAGS, rows=None):
    """{(map name, placement name)} the ENABLED settings select."""
    out = set()
    for flag, mapname, patterns in (rows if rows is not None else parse_table()):
        if flag not in flags:
            continue
        m = maps.get(mapname)
        if m is None:
            continue
        for name in matched_in(m, patterns):
            out.add((mapname, name))
    return out


# --- params ----------------------------------------------------------------

def npc_row_offsets(root):
    p = load_param(root, "NpcParam.param")
    return p, dict(param_rows(p))


def npc_signature(p, off):
    """The seven fields case 7 pins, as one comparable tuple."""
    return (struct.unpack_from("<I", p, off + NPC_HP)[0],
            struct.unpack_from("<I", p, off + NPC_GET_SOUL)[0],
            p[off + NPC_TEAM_TYPE],
            struct.unpack_from("<f", p, off + NPC_HIT_HEIGHT)[0],
            struct.unpack_from("<f", p, off + NPC_HIT_RADIUS)[0],
            struct.unpack_from("<i", p, off + NPC_BEHAVIOR_VARIATION)[0],
            struct.unpack_from("<i", p, off + NPC_ITEM_LOT_1)[0])


def param_row_bytes(root, member, row_id, size):
    """One param row's raw bytes, or None if the archive or row is absent."""
    if not os.path.exists(os.path.join(root, PARAM_REL)):
        return None
    p = load_param(root, member)
    if p is None:
        return None
    for rid, off in param_rows(p):
        if rid == row_id:
            return bytes(p[off:off + size])
    return None


def parse_drop_exclusions():
    """The row ids IsExcludedNpcRow holds, parsed out of DropRandomizer.cpp."""
    text = open(os.path.join(SRC, "DropRandomizer.cpp"), encoding="utf-8").read()
    body = text.split("bool IsExcludedNpcRow", 1)[1].split("}", 1)[0]
    return {int(x) for x in re.findall(r"id\s*==\s*(\d+)", body)}


# --- the output-tree contract ----------------------------------------------

def part_position(blob):
    return struct.unpack_from("<fff", blob, 0x28)


def check_trees(van_maps, out_maps, flags, strict, npc, think, model, rows=None):
    """The contract of plan 018 section 6, over two already-parsed trees.

    Returns (failures, notes). `strict` means the run had every randomizer off
    as well, so a survivor or a settings-off placement that moved is this
    feature's fault and nothing else's. Without it those two checks are
    reported and not asserted - see the module docstring.

    Split out from cmd_verify so selftest case 12 can feed it a tree it
    corrupted itself and require each corruption to be caught."""
    failures, notes = [], []
    rows = rows if rows is not None else parse_table()
    targets = selected(van_maps, flags, rows)
    table_maps = sorted({mapname for _f, mapname, _p in rows})

    replaced = 0
    for mapname in table_maps + [UNPATCHED_TWIN]:
        van, out = van_maps.get(mapname), out_maps.get(mapname)
        if van is None or out is None:
            notes.append("%s: absent from one of the two trees, skipped" % mapname)
            continue
        v, o = list(enemies(van)), list(enemies(out))
        if len(v) != len(o):
            failures.append("%s: %d placements in vanilla, %d in output"
                            % (mapname, len(v), len(o)))
            continue

        expected_npc = zone_scaled_npc(mapname, npc)
        survivors = SURVIVORS.get(mapname, [])
        moved_off = 0

        for a, b in zip(v, o):
            if (mapname, a["name"]) in targets:
                replaced += 1
                if b["model"] != model:
                    failures.append("%s/%s: model is %s, expected %s"
                                    % (mapname, a["name"], b["model"], model))
                if b["think"] != think:
                    failures.append("%s/%s: ThinkParamID is %d, expected %d"
                                    % (mapname, a["name"], b["think"], think))
                if b["npc"] != expected_npc:
                    failures.append("%s/%s: NPCParamID is %d, expected %d"
                                    % (mapname, a["name"], b["npc"], expected_npc))
                if b["entity"] != a["entity"]:
                    failures.append("%s/%s: EntityID changed %d -> %d"
                                    % (mapname, a["name"], a["entity"], b["entity"]))
                if b["pos"] != a["pos"]:
                    failures.append("%s/%s: position changed %s -> %s"
                                    % (mapname, a["name"], a["pos"], b["pos"]))
                continue

            # Not a target. Survivors are named individually because they are
            # the point of the feature; everything else in these maps is the
            # settings-off case.
            if _frozen(mapname, a, b):
                continue
            if a["name"] in survivors:
                if strict:
                    failures.append("%s/%s: survivor not frozen, %s/%s/%s -> %s/%s/%s"
                                    % (mapname, a["name"], a["model"], a["think"],
                                       a["npc"], b["model"], b["think"], b["npc"]))
                else:
                    notes.append("%s/%s: survivor changed - expected if the enemy or "
                                 "boss pass was on, see boss_verify.py"
                                 % (mapname, a["name"]))
            elif strict:
                failures.append("%s/%s: placement changed with its setting off, "
                                "%s/%s/%s -> %s/%s/%s"
                                % (mapname, a["name"], a["model"], a["think"], a["npc"],
                                   b["model"], b["think"], b["npc"]))
            else:
                moved_off += 1

        if moved_off:
            notes.append("%s: %d non-target placement(s) changed - the enemy and boss "
                         "passes are judged by boss_verify.py, not here"
                         % (mapname, moved_off))

    if len(targets) != replaced:
        failures.append("selected %d placements but only compared %d"
                        % (len(targets), replaced))
    return failures, notes


def check_params(vanilla_root, output_root, npc):
    """B12: the larva's stat row and the cord's item lot are never written."""
    failures, notes = [], []
    if not os.path.exists(os.path.join(output_root, PARAM_REL)):
        notes.append("output tree has no item-data archive - no param feature ran, "
                     "so B12 holds trivially")
        return failures, notes
    for member, row_id, size, label in (
            ("NpcParam.param", npc, 388, "the larva's stat row"),
            ("ItemLotParam.param", CORD_LOT, 148, "the umbilical cord's item lot")):
        a = param_row_bytes(vanilla_root, member, row_id, size)
        b = param_row_bytes(output_root, member, row_id, size)
        if a is None or b is None:
            failures.append("%s row %d missing from one of the two trees"
                            % (member, row_id))
        elif a != b:
            failures.append("%s row %d (%s) differs between the trees"
                            % (member, row_id, label))
        else:
            notes.append("%s row %d (%s) byte-identical" % (member, row_id, label))
    return failures, notes


# --- commands --------------------------------------------------------------

def cmd_show(root):
    npc, think, model = parse_constants()
    rows = parse_table()
    maps = {mapname: load_map(root, mapname) for _f, mapname, _p in rows}
    print("EasyModes.h: %d rows, replacement %d*%d*%s" % (len(rows), npc, think, model))
    print("%-10s %-14s %6s  %s" % ("setting", "map", "hits", "placements"))
    total, retail = 0, 0
    for flag, mapname, patterns in rows:
        m = maps[mapname]
        if m is None:
            print("%-10s %-14s %6s  MAP NOT FOUND" % (flag, mapname, "-"))
            continue
        names = sorted(matched_in(m, patterns))
        total += len(names)
        if zone_scaled_npc(mapname, npc) != npc:
            retail += len(names)
        shown = ", ".join(names) if len(names) <= 6 else \
            ", ".join(names[:3]) + ", ... , " + names[-1]
        print("%-10s %-14s %6d  %s" % (flag, mapname, len(names), shown))
    print()
    print("%d placements tree-wide, %d in the map variants the retail game loads."
          % (total, retail))
    print("Not a pass/fail gate - run selftest for that.")
    return 0


def cmd_verify(vanilla_root, output_root, flags, strict):
    npc, think, model = parse_constants()
    rows = parse_table()
    names = sorted({mapname for _f, mapname, _p in rows}) + [UNPATCHED_TWIN]
    van_maps = {mn: load_map(vanilla_root, mn) for mn in names}
    out_maps = {mn: load_map(output_root, mn) for mn in names}

    print("settings ON: %s" % (", ".join(sorted(flags)) if flags else "none"))
    print("replacement : %d*%d*%s, from EasyModes.h" % (npc, think, model))
    print("survivor and settings-off checks are %s"
          % ("ASSERTED - the run had every randomizer off" if strict
             else "REPORTED ONLY - pass --no-randomizers to assert them"))

    failures, notes = check_trees(van_maps, out_maps, flags, strict, npc, think, model,
                                  rows=rows)
    pf, pn = check_params(vanilla_root, output_root, npc)
    failures += pf
    notes += pn

    print("selected     : %d placements" % len(selected(van_maps, flags, rows)))
    for n in notes[:30]:
        print("  note: " + n)
    if len(notes) > 30:
        print("  ... and %d more notes" % (len(notes) - 30))
    if failures:
        print("\n%d FAILURES:" % len(failures))
        for f in failures[:40]:
            print("  " + f)
        if len(failures) > 40:
            print("  ... and %d more" % (len(failures) - 40))
        return 1
    print("\nPASS - the rules hold on this tree. Ready for hardware testing, which is"
          "\n       the only thing that can say whether the fights still end.")
    return 0


# --- selftest --------------------------------------------------------------

def _fake_model_entry(name):
    """A minimal Models-section blob the checker's model_name() can read.

    Only for selftest case 12's synthetic output tree: c2521 is declared in
    five maps and in none of the six this feature writes, so the port's
    StepMergeModels is what puts it there on a real run. Reproducing that
    merge is not this tool's job - the checker needs a name at an index, and
    that is all this provides."""
    blob = bytearray(16)
    struct.pack_into("<q", blob, 0x00, 16)  # name offset
    struct.pack_into("<i", blob, 0x08, 2)   # ModelType::kEnemy
    blob += name.encode("utf-16-le") + b"\x00\x00"
    return blob


def _apply_easy(m, mapname, patterns, npc, think, model):
    """Mirror of ApplyEasyModes over one parsed map, for the synthetic tree."""
    index = next((i for i in range(len(m.models.entries))
                  if m.model_name(i) == model), None)
    if index is None:
        m.models.entries.append(_fake_model_entry(model))
        index = len(m.models.entries) - 1
    written = zone_scaled_npc(mapname, npc)
    hit = 0
    for blob in m.parts.entries:
        if int.from_bytes(blob[0x14:0x18], "little") != 2:
            continue
        if not any(p in _utf16z(blob, _i64(blob, 0x08)) for p in patterns):
            continue
        td = _i64(blob, PART_TYPE_DATA_OFFSET)
        _set_i32(blob, td + ENEMY_NPC_IN_TYPEDATA, written)
        _set_i32(blob, td + ENEMY_THINK_IN_TYPEDATA, think)
        _set_i32(blob, PART_MODEL_INDEX, index)
        hit += 1
    return hit


def _find(m, name):
    for blob in m.parts.entries:
        if int.from_bytes(blob[0x14:0x18], "little") != 2:
            continue
        if _utf16z(blob, _i64(blob, 0x08)) == name:
            return blob
    return None


def cmd_selftest(root):
    """The twelve cases of plan 018 section 6, against the vanilla tree."""
    cases = []
    npc, think, model = parse_constants()
    rows = parse_table()
    maps = load_base_maps(root)
    by_map = {mapname: (flag, patterns) for flag, mapname, patterns in rows}

    # --- 1. the table resolves to the expected placement sets --------------
    resolved = {mapname: matched_in(maps[mapname], patterns)
                for _f, mapname, patterns in rows}
    cases.append(("018: the six rows resolve to the expected placement sets",
                  resolved == EXPECTED))
    total = sum(len(s) for s in resolved.values())
    # "Retail-loaded" is derived, not listed: the four variants the game loads
    # are exactly the ones BossParamScaling names for their zone, which is the
    # same fact case 8 depends on.
    retail = sum(len(s) for mn, s in resolved.items()
                 if zone_scaled_npc(mn, npc) != npc)
    cases.append(("018: %d placements tree-wide, %d retail-loaded"
                  % (TREE_TOTAL, RETAIL_TOTAL),
                  total == TREE_TOTAL and retail == RETAIL_TOTAL))

    # --- 2. m27_00_00_00 is out of the table, and holds the same Shadows ----
    twin = {e["name"]: e["npc"] for e in enemies(maps[UNPATCHED_TWIN])
            if "c2120" in e["name"]}
    cases.append(("018: %s is not in the table (D3)" % UNPATCHED_TWIN,
                  UNPATCHED_TWIN not in {mn for _f, mn, _p in rows}))
    cases.append(("018: ...and it holds the same three Shadows, 212700/10/20",
                  twin == UNPATCHED_TWIN_SHADOWS))

    # --- 3. the survivors ---------------------------------------------------
    cases.append(("018: no survivor is matched by a pattern in its own map",
                  all(not (set(survivors) & resolved[mn])
                      for mn, survivors in SURVIVORS.items())))
    p, offs = npc_row_offsets(root)
    shadow_hp = {}
    for e in enemies(maps["m27_00_00_01"]):
        if e["name"] in UNPATCHED_TWIN_SHADOWS:
            shadow_hp[e["name"]] = struct.unpack_from("<I", p, offs[e["npc"]] + NPC_HP)[0]
    cases.append(("018: the surviving Shadow is the strongest, 1425 v 900 and 800",
                  shadow_hp == {"c2120_0000": 1425, "c2120_0001": 900,
                                "c2120_0002": 800}))

    # --- 4. no pattern over-reaches INSIDE its own map ----------------------
    # Asserted per (map, row), never tree-wide: the same patterns DO match in
    # other maps, which is why the table is keyed by exact map name.
    over = []
    for _f, mapname, patterns in rows:
        for pat in patterns:
            extra = {e["name"] for e in enemies(maps[mapname]) if pat in e["name"]} \
                - EXPECTED[mapname]
            if extra:
                over.append((mapname, pat, sorted(extra)))
    cases.append(("018: no pattern reaches a non-target inside its own map",
                  not over))
    cases.append(("018: ...in particular c2500_0001 does not take c2500_0011",
                  "c2500_0011" not in resolved["m24_02_00_01"] and
                  "c2500_0011" in {e["name"] for e in enemies(maps["m24_02_00_01"])}))

    # --- 5. the baked triple is real ---------------------------------------
    larvae = [(mn, e) for mn in BASE_MAPS for e in enemies(maps[mn])
              if e["model"] == model]
    cases.append(("018: %s has three placements, all %d*%d, entity 2410771"
                  % (model, npc, think),
                  len(larvae) == 3 and
                  all(e["name"] == "c2521_0000" and e["npc"] == npc and
                      e["think"] == think and e["entity"] == 2410771
                      for _mn, e in larvae)))

    # --- 6. the model is declared somewhere --------------------------------
    declared = [mn for mn in BASE_MAPS
                if any(int.from_bytes(b[0x08:0x0C], "little") == 2 and
                       _utf16z(b, _i64(b, 0x00)) == model
                       for b in maps[mn].models.entries)]
    cases.append(("018: %s is declared as an enemy model in 5 base maps" % model,
                  len(declared) == 5))

    # --- 7. the replacement cannot be dangerous ----------------------------
    base_sig = npc_signature(p, offs[npc])
    cases.append(("018: NpcParam %d is 2 HP, 18 echoes, team 26, lot %d"
                  % (npc, CORD_LOT),
                  base_sig == (2, 18, 26, 1.0, 0.20000000298023224, 25210, CORD_LOT)))
    cases.append(("018: all 31 scaled variants are identical in every one of those",
                  all(r in offs and npc_signature(p, offs[r]) == base_sig
                      for r in LARVA_SCALED_ROWS)))

    # --- 8. the value actually written, per map ----------------------------
    cases.append(("018: the written NPCParamID is the zone variant where there is one",
                  {mn: zone_scaled_npc(mn, npc) for mn in EXPECTED} ==
                  {"m27_00_00_01": 900014609, "m32_00_00_01": 900014611,
                   "m24_02_00_01": 900014614, "m35_00_00_00": 900014627,
                   "m32_00_00_00": npc, "m24_02_00_00": npc}))

    # --- 9. the drop situation, asserted as it is --------------------------
    lot = load_param(root, "ItemLotParam.param")
    lot_offs = dict(param_rows(lot))
    cord_lots = [rid for rid, off in lot_offs.items()
                 if any(struct.unpack_from("<i", lot, off + LOT_ITEM_IDS + 4 * k)[0]
                        == CORD_ITEM for k in range(8))]
    cases.append(("018: lot %d is the only source of item %d" % (CORD_LOT, CORD_ITEM),
                  cord_lots == [CORD_LOT]))
    cases.append(("018: ...with a single getItemFlagId %d" % CORD_FLAG,
                  struct.unpack_from("<i", lot, lot_offs[CORD_LOT] + LOT_GET_ITEM_FLAG)[0]
                  == CORD_FLAG))
    referencing = {rid for rid, off in offs.items()
                   if struct.unpack_from("<i", p, off + NPC_ITEM_LOT_1)[0] == CORD_LOT
                   or struct.unpack_from("<i", p, off + NPC_ITEM_LOT_2)[0] == CORD_LOT}
    cases.append(("018: exactly 32 NpcParam rows carry it - %d plus the 31 variants" % npc,
                  referencing == {npc} | set(LARVA_SCALED_ROWS)))
    # The 31 variants are deliberately UNPROTECTED from drop randomization, in
    # the port and in the reference alike (plan 018 section 3.2). A change on
    # either side of this is a decision, and this is what catches it.
    cases.append(("018: IsExcludedNpcRow holds %d and 6071 and nothing else" % npc,
                  parse_drop_exclusions() == {npc, 6071}))

    # --- 10. no pattern matches an arena leader ----------------------------
    cases.append(("018: no pattern in any row matches an arena leader",
                  not [(mn, pat, leader)
                       for _f, mn, pats in rows for pat in pats
                       for leader in ARENA_LEADERS if pat in leader]))

    # --- 11. the reference's own name lists, verbatim ----------------------
    # Couples this test to reference/Randomizer/MainWindowComponents/
    # MainWindow.xaml.cs, which CLAUDE.md section 1 already fixes in place for
    # starting_weapons_verify.py.
    ref = open(REF_XAML, encoding="utf-8").read()
    body = ref.split("private void EasyModes(", 1)[1].split("tempGUY.Write", 1)[0]
    ref_names, ref_keys = {}, {}
    for chunk in re.split(r"//\s*(shadows|rom|emissary|failures)\b", body)[1:]:
        if chunk in FLAGS:
            current = chunk
            continue
        ref_names[current] = set(re.findall(r'Name\.Contains\("([^"]+)"\)', chunk))
        key = re.search(r'currentMap\.Contains\("([^"]+)"\)', chunk)
        ref_keys[current] = key.group(1) if key else None
    ours = {}
    for flag, _mn, patterns in rows:
        ours.setdefault(flag, set()).update(patterns)
    cases.append(("018: the four reference name lists match the table verbatim",
                  ref_names == ours))
    cases.append(("018: every table map matches its reference branch key",
                  all(ref_keys[flag] in mapname for flag, mapname, _p in rows)))

    # --- 12. corrupt and catch ---------------------------------------------
    # Built from the vanilla tree: apply the mirror of the pass, then break it
    # one way at a time and require each break to be reported.
    van = {mn: maps[mn] for mn in list(EXPECTED) + [UNPATCHED_TWIN]}
    good = copy.deepcopy(van)
    for mapname, (flag, patterns) in by_map.items():
        _apply_easy(good[mapname], mapname, patterns, npc, think, model)

    def run(out):
        f, _n = check_trees(van, out, FLAGS, True, npc, think, model, rows=rows)
        return f

    cases.append(("018: a correct easy tree is clean", not run(good)))

    bad = copy.deepcopy(good)
    blob = _find(bad["m27_00_00_01"], "c2120_0000")  # the surviving Shadow
    _set_i32(blob, _i64(blob, PART_TYPE_DATA_OFFSET) + ENEMY_NPC_IN_TYPEDATA, 999999)
    cases.append(("018: a changed survivor is caught",
                  any("survivor not frozen" in f for f in run(bad))))

    bad = copy.deepcopy(good)
    blob = _find(bad["m35_00_00_00"], "c4030_0001")
    _set_i32(blob, _i64(blob, PART_TYPE_DATA_OFFSET) + ENEMY_NPC_IN_TYPEDATA, 403010)
    cases.append(("018: a target left at its vanilla identity is caught",
                  any("NPCParamID is 403010" in f for f in run(bad))))

    bad = copy.deepcopy(good)
    blob = _find(bad["m24_02_00_01"], "c2500_0002")
    _set_i32(blob, _i64(blob, PART_BASE_DATA_OFFSET), 12345)
    cases.append(("018: an EntityID changed on a replaced placement is caught",
                  any("EntityID changed" in f for f in run(bad))))

    untouched = copy.deepcopy(van)
    f_off, _n = check_trees(van, untouched, (), True, npc, think, model, rows=rows)
    cases.append(("018: an unmodified tree with every setting off is clean", not f_off))

    failures = 0
    for name, ok in cases:
        print("  %-66s %s" % (name, "ok" if ok else "FAILED"))
        if not ok:
            failures += 1
    print("%d/%d passing" % (len(cases) - failures, len(cases)))
    return 1 if failures else 0


def main():
    a = sys.argv[1:]
    if len(a) == 2 and a[0] == "show":
        return cmd_show(a[1])
    if len(a) >= 3 and a[0] == "verify":
        rest = a[3:]
        strict = "--no-randomizers" in rest
        flags = tuple(f for f in rest if f in FLAGS)
        unknown = [x for x in rest if x not in FLAGS and x != "--no-randomizers"]
        if unknown:
            print("unknown argument(s): %s" % ", ".join(unknown))
            return 2
        return cmd_verify(a[1], a[2], flags, strict)
    if len(a) == 2 and a[0] == "selftest":
        return cmd_selftest(a[1])
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main())
