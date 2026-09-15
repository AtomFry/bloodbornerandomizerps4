#!/usr/bin/env python3
"""Answering "is THIS enemy actually being randomized, and if not why".

Enemies are identified in the game data by a placement name (c<model>_<nnnn>),
an EntityID, and an XYZ position - never by anything a player would recognise.
So there are three practical ways to pin one down, in increasing certainty:

  frozen  - list every placement that CANNOT change, with the reason. If an
            enemy looks identical run after run, it is almost certainly here.
  diff    - compare vanilla against one randomized tree, per placement.
  compare - compare TWO randomized trees made with DIFFERENT seeds. Anything
            identical in both is frozen, whatever the cause. This is the
            decisive test and needs no knowledge of what the models look like.

  pool    - how the candidate pool is weighted by model, which explains which
            replacements show up disproportionately often.

Exclusion lists are parsed straight out of the C++ headers so this tool and the
randomizer cannot drift apart.

Usage:
    python enemy_lookup.py frozen  <vanilla_dvdroot> [map]
    python enemy_lookup.py diff    <vanilla_dvdroot> <output_dvdroot> [map]
    python enemy_lookup.py compare <output_a> <output_b> [map]
    python enemy_lookup.py pool    <vanilla_dvdroot>
"""

import os
import re
import struct
import sys
from collections import Counter

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from boss_verify import read_dcx, Msbb, _i32, _i64, _utf16z  # noqa: E402
from names import model_name, map_name, map_is_unused, available as names_available  # noqa: E402

SRC = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                   "..", "src", "Randomizer")

BASE_MAPS = [
    "m21_00_00_00", "m21_01_00_00", "m22_00_00_00", "m23_00_00_00", "m23_00_00_01",
    "m24_00_00_00", "m24_00_00_01", "m24_01_00_00", "m24_01_00_01", "m24_01_00_11",
    "m24_02_00_00", "m24_02_00_01", "m25_00_00_00", "m26_00_00_00", "m27_00_00_00",
    "m27_00_00_01", "m28_00_00_00", "m28_00_00_01", "m32_00_00_00", "m32_00_00_01",
    "m33_00_00_00", "m34_00_00_00", "m35_00_00_00", "m36_00_00_00",
]

def zone_of(mapname):
    """Full map name from the Smithbox alias data, e.g.
    'Central Yharnam (pre-DLC)' - which also tells us which variants the
    retail game never loads."""
    return map_name(mapname)


def _load_list(header, func):
    """Parse a string array out of the C++ header. Returns [] if the function
    isn't there - EnemyExclusionListExtra was removed on 2026-09-13, and this
    tool should keep working against both old and new trees."""
    text = open(os.path.join(SRC, header), encoding="utf-8").read()
    marker = "inline const std::array<const char*, "
    idx = text.find(func)
    if idx == -1 or text.rfind(marker, 0, idx) == -1:
        return []
    body = text[idx:].split("kList = {", 1)
    if len(body) < 2:
        return []
    body = body[1].split("};", 1)[0]
    body = re.sub(r"/\*.*?\*/", "", body, flags=re.S)
    body = re.sub(r"//[^\n]*", "", body)
    return re.findall(r'"([^"]+)"', body)


EXCL = _load_list("EnemyExclusionList.h", "EnemyExclusionList()")
EXTRA = _load_list("EnemyExclusionList.h", "EnemyExclusionListExtra()")


def exclusion_reason(name):
    """Returns (kind, pattern) or None. EXTRA is checked first because it is
    ours rather than the reference tool's, and that distinction matters."""
    for p in EXTRA:
        if p in name:
            return ("ours", p)
    for p in EXCL:
        if p in name:
            return ("reference", p)
    return None


def enemies(m):
    for i, blob in enumerate(m.parts.entries):
        if int.from_bytes(blob[0x14:0x18], "little") != 2:
            continue
        td = _i64(blob, 0xB8)
        bd = _i64(blob, 0xB0)
        yield {
            "idx": i,
            "name": _utf16z(blob, _i64(blob, 0x08)),
            "npc": _i32(blob, td + 12),
            "think": _i32(blob, td + 8),
            "talk": _i32(blob, td + 16),
            "entity": _i32(blob, bd),
            "model": m.model_name(_i32(blob, 0x1C)) or "",
            "pos": struct.unpack_from("<fff", blob, 0x28),
        }


def engine_pool(root):
    """The randomizer's ACTUAL candidate pool, as a list of (npc, think, model).

    This is a deliberate line-for-line replay of EnemyRandomizer.cpp's
    StepReadMaps contribution loop plus StepBuildPool's dedupe, and it is the
    single source of truth for every tool that needs to know what can be
    drawn. Do not reimplement it.

    Three rules here are easy to miss, and cmd_pool got all three wrong until
    2026-09-13 (it reported 442 entries across 83 models; the truth is 333
    across 82):

      * placements whose name contains "c2561" never contribute (:358);
      * "c1110_0000" contributes only when its TalkID is exactly 111010 (:357);
      * duplicate suppression is by NPC id and is reset PER MAP
        (contributedNpcIds is declared inside the map loop), not by
        (npc, think, model) globally.

    See docs/plans/pickers.md section 2.2.
    """
    pool, seen = [], set()
    for mapname in BASE_MAPS:
        m = load_map(root, mapname)
        if m is None:
            continue
        contributed = set()
        for e in enemies(m):
            if exclusion_reason(e["name"]):
                continue
            if e["think"] <= 1 or e["npc"] <= 1:
                continue
            if e["npc"] in contributed:
                continue
            if "c1110_0000" in e["name"] and e["talk"] != 111010:
                continue
            if "c2561" in e["name"]:
                continue
            if not e["model"]:
                continue
            contributed.add(e["npc"])
            key = (e["npc"], e["think"], e["model"])
            if key in seen:      # StepBuildPool's exact-duplicate dedupe
                continue
            seen.add(key)
            pool.append(key)
    return pool


def engine_pool_models(root):
    """The distinct models of engine_pool(), sorted by display name then id -
    the frozen order the picker's baked table and its config string use."""
    models = sorted({mdl for _, _, mdl in engine_pool(root)})
    return sorted(models, key=lambda mdl: (model_name(mdl).upper(), mdl))


def load_map(root, name):
    p = os.path.join(root, "map", "mapstudio", name + ".msb.dcx")
    if not os.path.exists(p):
        return None
    return Msbb(read_dcx(p))


def maps_for(arg):
    return [arg] if arg else BASE_MAPS


def cmd_frozen(root, only=None):
    print("Placements that can NEVER be randomized (identical in every run).")
    print('"ours" = this port\'s own heuristic list; "reference" = the Windows tool\'s.')
    print()
    totals = Counter()
    for mn in maps_for(only):
        m = load_map(root, mn)
        if m is None:
            continue
        rows = []
        for e in enemies(m):
            r = exclusion_reason(e["name"])
            if r:
                rows.append((e, r))
                totals[r[0]] += 1
        if not rows:
            continue
        ours = [x for x in rows if x[1][0] == "ours"]
        print("%s  (%s)%s: %d frozen of %d placements, %d by OUR list"
              % (mn, zone_of(mn), "  [UNUSED MAP]" if map_is_unused(mn) else "",
                 len(rows), len(list(enemies(m))), len(ours)))
        for e, (kind, pat) in rows:
            if kind != "ours":
                continue
            print("    %-14s %-26s entity=%-9d pos=(%6.0f,%6.0f,%6.0f)  ours: %s"
                  % (e["name"], model_name(e["model"]), e["entity"],
                     e["pos"][0], e["pos"][1], e["pos"][2], pat))
    print()
    print("total frozen placements: %d by the reference's list, %d by ours"
          % (totals["reference"], totals["ours"]))
    return 0


def _model_map(root, mn):
    m = load_map(root, mn)
    if m is None:
        return None
    return {e["idx"]: e for e in enemies(m)}


def cmd_diff(vanilla, output, only=None):
    print("%-14s %-14s %-8s %-8s %s" % ("map", "placement", "vanilla", "now", "note"))
    same_total = changed_total = 0
    for mn in maps_for(only):
        a, b = _model_map(vanilla, mn), _model_map(output, mn)
        if not a or not b:
            continue
        for idx, e in a.items():
            f = b.get(idx)
            if not f:
                continue
            if e["model"] == f["model"]:
                same_total += 1
                r = exclusion_reason(e["name"])
                note = ("frozen (%s: %s)" % r) if r else "same model by chance"
                if only:
                    print("  %-12s %-14s %-24s %-24s %s"
                          % (mn, e["name"], model_name(e["model"]),
                             model_name(f["model"]), note))
            else:
                changed_total += 1
    print()
    print("%d placements changed model, %d kept it" % (changed_total, same_total))
    return 0


def cmd_compare(a_root, b_root, only=None):
    """The decisive test: two DIFFERENT seeds. Identical => frozen."""
    print("Comparing two randomized trees. Anything identical in both is frozen,")
    print("regardless of cause - no knowledge of what the models look like needed.")
    print()
    identical = []
    differing = 0
    for mn in maps_for(only):
        a, b = _model_map(a_root, mn), _model_map(b_root, mn)
        if not a or not b:
            continue
        for idx, e in a.items():
            f = b.get(idx)
            if not f:
                continue
            if e["model"] == f["model"]:
                identical.append((mn, e))
            else:
                differing += 1

    explained = [x for x in identical if exclusion_reason(x[1]["name"])]
    mystery = [x for x in identical if not exclusion_reason(x[1]["name"])]
    print("%d placements differ between the two seeds (working normally)" % differing)
    print("%d placements are identical in both:" % len(identical))
    print("    %d explained by an exclusion list" % len(explained))
    print("    %d NOT explained - these are the interesting ones" % len(mystery))
    if mystery:
        print()
        print("  unexplained identical placements:")
        for mn, e in mystery[:40]:
            print("    %-12s %-14s %-26s entity=%-9d pos=(%6.0f,%6.0f,%6.0f)"
                  % (mn, e["name"], model_name(e["model"]), e["entity"],
                     e["pos"][0], e["pos"][1], e["pos"][2]))
        if len(mystery) > 40:
            print("    ... and %d more" % (len(mystery) - 40))
        print()
        print("  (a handful is expected - with ~425 pool entries, some draws")
        print("   land on the original model by chance)")
    return 0


def cmd_pool(root):
    pool = engine_pool(root)
    placements = Counter()
    for mapname in BASE_MAPS:
        m = load_map(root, mapname)
        if m is None:
            continue
        for e in enemies(m):
            placements[e["model"]] += 1

    by_model = Counter(mdl for _, _, mdl in pool)
    total = len(pool)
    print("enemy pool: %d distinct identities across %d models" % (total, len(by_model)))
    print()
    print("Each pool entry is one equally-likely draw, so a model with many")
    print("stat/AI variants gets picked far more often than a rare one.")
    print("See docs/deferred-ideas.md §2.")
    print()
    print("  %-8s %-34s %-9s %-8s %s"
          % ("model", "name", "in pool", "share", "placements"))
    for model, n in by_model.most_common(15):
        print("  %-8s %-34s %-9d %-8s %d"
              % (model, model_name(model), n,
                 "%.1f%%" % (100.0 * n / total), placements[model]))
    top5 = sum(n for _, n in by_model.most_common(5))
    singles = sum(1 for n in by_model.values() if n == 1)
    print()
    print("  top 5 models account for %.0f%% of every replacement rolled"
          % (100.0 * top5 / total))
    print("  %d of the %d models have a single entry, worth %.2f%% each"
          % (singles, len(by_model), 100.0 / total))
    return 0


def main():
    a = sys.argv
    if len(a) >= 3 and a[1] == "frozen":
        return cmd_frozen(a[2], a[3] if len(a) > 3 else None)
    if len(a) >= 4 and a[1] == "diff":
        return cmd_diff(a[2], a[3], a[4] if len(a) > 4 else None)
    if len(a) >= 4 and a[1] == "compare":
        return cmd_compare(a[2], a[3], a[4] if len(a) > 4 else None)
    if len(a) >= 3 and a[1] == "pool":
        return cmd_pool(a[2])
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main())
