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
    python enemy_lookup.py frozen  <vanilla_dvdroot> [map] [--skip=c1050,c1051]
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
# The six m28 placements the reference forces back into randomization
# regardless of any exclusion or skip.
M28_FORCED = _load_list("EnemyExclusionList.h", "M28ForcedMaidenList()")

# UNCHANGED BELL MAIDENS' three patterns used to be parsed out of the same
# header as BELL. Feature 032 D1 retired that setting; "skipped" below is the
# user's own list and is passed in by the caller rather than parsed, because
# it is a runtime choice and not a constant. The two rows that reproduce the
# old setting are c1050 and c1051 - c1055 never matched anything.
SKIP_MAIDENS = ("c1050", "c1051")


def exclusion_reason(name, skipped=()):
    """Returns (kind, pattern) or None. EXTRA is checked first because it is
    ours rather than the reference tool's, and that distinction matters.

    `skipped` mirrors EnemyRandomizerOptions::enemiesSkipped - the model ids
    the player ticked in ENEMIES SKIPPED. It is reported as its own kind
    because the engine keeps the two tests separate too (EnemySkipList.h):
    the reference's list never yields and the player's can."""
    for p in EXTRA:
        if p in name:
            return ("ours", p)
    for p in EXCL:
        if p in name:
            return ("reference", p)
    for p in skipped:
        if p in name:
            return ("skipped", p)
    return None


def is_m28_forced(mapname, name):
    """Mirror of `isM28 && IsM28ForcedMaidenName(name)` in StepWriteMap. The
    map test is part of the rule: the reference's override lives inside an
    `if (currentMap.Contains("m28"))` block."""
    if "m28" not in mapname:
        return False
    return any(p in name for p in M28_FORCED)


def randomizes(mapname, name, roll, zone_chance, skipped=()):
    """Mirror of StepWriteMap's placement decision - the four lines that decide
    whether a placement is retargeted:

        bool forced = isM28 && IsM28ForcedMaidenName(name);
        if (!forced && IsExcludedEnemyName(name)) continue;
        if (!forced && IsSkippedName(name, skipPatterns)) continue;
        int roll = RandInt(0, 100);
        if (!forced && roll >= lm.zoneChance) continue;

    `roll` is the value RandInt(0, 100) returned. Kept here rather than in
    pool_verify so there is one mirror of the rule, next to the pool mirror."""
    forced = is_m28_forced(mapname, name)
    if not forced and exclusion_reason(name, skipped):
        return False
    if not forced and roll >= zone_chance:
        return False
    return True


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


def _parse_int_array(header, func):
    """Parse a std::array<int64_t, N> of plain integers out of a C++ header."""
    text = open(os.path.join(SRC, header), encoding="utf-8").read()
    idx = text.find(func)
    body = text[idx:].split("= {", 1)[1].split("};", 1)[0]
    body = re.sub(r"/\*.*?\*/", "", body, flags=re.S)
    body = re.sub(r"//[^\n]*", "", body)
    return [int(x) for x in re.findall(r"-?\d+", body)]


def _scaling_maps():
    """{mapname: zoneScale} from BossParamScaling.h's own array. Scale 0 is an
    explicit no-op in the reference (m21_01), kept so the count stays 16."""
    text = open(os.path.join(SRC, "BossParamScaling.h"), encoding="utf-8").read()
    body = text.split("kMaps = {{", 1)[1].split("}};", 1)[0]
    return {m: int(v) for m, v in re.findall(r'\{"(\w+)",\s*(-?\d+)\}', body)}


_SCALE_CACHE = {}


def _scale_map():
    """{(tracked value): {zoneScale: scaled value}} - an exact mirror of
    ApplyBossParamScaling, precomputed.

    Two measured facts make a plain dictionary exact rather than approximate,
    and both are pinned by pool_verify selftest cases:

      * every tracked value appears EXACTLY ONCE in the scaling table, so
        "find the position" is single-valued and the reference's nested-loop
        positions list has one entry per tracked value;
      * the reference rewrites NPCParamIDs in place, in position order, so a
        value scaled early could in principle be re-scaled later by another
        position. On the values a randomized map can actually contain that
        never happens - but it is a property of the DATA, not of the table,
        which is why the selftest checks it rather than assuming it.
    """
    if not _SCALE_CACHE:
        params = _parse_int_array("NpcScalingTable.h", "NpcParamsTable()")
        scaling = _parse_int_array("NpcScalingTable.h", "NpcScalingTable()")
        pos = {}
        for v in params:
            for j, x in enumerate(scaling):
                if x == v:
                    pos.setdefault(v, []).append(j)
        for v, js in pos.items():
            per_zone = {}
            for j in js:
                for z in set(_scaling_maps().values()):
                    if z and j + z < len(scaling):
                        per_zone[z] = scaling[j + z]
            _SCALE_CACHE[v] = per_zone
    return _SCALE_CACHE


def zone_scaled_npc(mapname, npc):
    """What ApplyBossParamScaling turns `npc` into in `mapname`, or `npc`
    itself when that map is not scaled or the value is not tracked.

    Needed because "this placement is frozen" cannot be plain equality with
    vanilla: the stat pass runs AFTER the enemy pass and rewrites NPCParamID
    on 1,369 of the 2,269 overwritable placements, a frozen one included. A
    checker that demanded equality would fail on a correct implementation -
    which is exactly the bug pool_verify's retired `maidens` command had."""
    zone = _scaling_maps().get(mapname, 0)
    if not zone:
        return npc
    return _scale_map().get(npc, {}).get(zone, npc)


def load_base_maps(root):
    """Parse the 24 base maps once, for callers that need many pool variants.

    engine_pool() re-reads every map on each call, which is fine for one
    answer and far too slow for the 80-odd selections pool_verify's selftest
    samples. Pass the result back in as engine_pool(maps=...)."""
    return {mn: load_map(root, mn) for mn in BASE_MAPS}


def engine_pool(root, skipped=(), included=None, maps=None):
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

    `included` mirrors EnemyRandomizerOptions::enemiesIncluded - the set of
    model ids ENEMIES INCLUDED has ticked. None means every model, which is
    both the default and the behaviour before the picker existed. Note this
    models the picker as a plain set, not as ModelPoolSelection: the engine
    reads a model that is ABSENT from the baked table as enabled, and this
    would read it as disabled. That divergence is unreachable here, because
    every model a placement can contribute is in the table by construction -
    the table IS this function's output. Feature 032 needs this to express
    "the selection starves the pool", which is the one thing the pool-empty
    decision turns on.

    `maps` is an optional preloaded {name: Msbb} from load_base_maps(), for
    callers asking many variants of the same question.

    See docs/plans/pickers.md section 2.2.
    """
    pool, seen = [], set()
    for mapname in BASE_MAPS:
        m = maps.get(mapname) if maps is not None else load_map(root, mapname)
        if m is None:
            continue
        contributed = set()
        for e in enemies(m):
            if exclusion_reason(e["name"], skipped):
                continue
            if included is not None and e["model"] not in included:
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


def overwritable_placements(root, maps=None):
    """Every placement the ENEMY randomizer is allowed to overwrite, as
    (mapname, enemy dict).

    This is a different set from engine_pool()'s. The pool asks "what can be
    used as a REPLACEMENT" and applies think/npc/dedupe/c1110/c2561 on top;
    this asks "what can be REPLACED", which is the fixed exclusion list and
    nothing else - exactly the test at EnemyRandomizer.cpp's StepWriteMap.
    Feature 032's ENEMIES SKIPPED is a list of the models on THIS side, which
    is why it has 85 rows where the pool table has 82."""
    out = []
    for mapname in BASE_MAPS:
        m = maps.get(mapname) if maps is not None else load_map(root, mapname)
        if m is None:
            continue
        for e in enemies(m):
            if exclusion_reason(e["name"]):
                continue
            if not e["model"]:
                continue
            out.append((mapname, e))
    return out


def overwritable_counts(root, maps=None):
    """{model id: how many overwritable placements it owns}. This is the skip
    table's weight column - the number of placements a ticked row protects,
    NOT a draw weight, because nothing ever draws from that table."""
    counts = {}
    for _mn, e in overwritable_placements(root, maps):
        counts[e["model"]] = counts.get(e["model"], 0) + 1
    return counts


def overwritable_models(root, maps=None):
    """The distinct models of overwritable_placements(), in the skip table's
    frozen order: display name upper-cased, then model id to break ties - the
    same rule the other two baked tables use."""
    models = sorted(overwritable_counts(root, maps).keys())
    return sorted(models, key=lambda mdl: (model_name(mdl).upper(), mdl))


def engine_pool_models(root, skipped=()):
    """The distinct models of engine_pool(), sorted by display name then id -
    the frozen order the picker's baked table and its config string use.

    NOTE: the baked EnemyPoolTable.h is the skipped=() order and must stay
    that way. Regenerating it against a skipped pool would silently remap
    every saved ENEMIES INCLUDED selection - see plan 032 section 5.1."""
    models = sorted({mdl for _, _, mdl in engine_pool(root, skipped)})
    return sorted(models, key=lambda mdl: (model_name(mdl).upper(), mdl))


def load_map(root, name):
    p = os.path.join(root, "map", "mapstudio", name + ".msb.dcx")
    if not os.path.exists(p):
        return None
    return Msbb(read_dcx(p))


def maps_for(arg):
    return [arg] if arg else BASE_MAPS


def cmd_frozen(root, only=None, skipped=()):
    print("Placements that can NEVER be randomized (identical in every run).")
    print('"ours" = this port\'s own heuristic list; "reference" = the Windows tool\'s.')
    print('"skipped" = models passed with --skip=, i.e. the player\'s ENEMIES SKIPPED list.')
    if skipped:
        print()
        print("NOTE: the six m28 forced placements are matched by the skip list here but")
        print("are still randomized by the engine - the override bypasses the exclusion.")
    print()
    totals = Counter()
    for mn in maps_for(only):
        m = load_map(root, mn)
        if m is None:
            continue
        rows = []
        for e in enemies(m):
            r = exclusion_reason(e["name"], skipped)
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
    # totals["skipped"] is counted above but was dropped from this line until
    # 2026-09-16, so the flagged form printed the same number as the plain one.
    print("total frozen placements: %d by the reference's list, %d by ours, %d by the skip list"
          % (totals["reference"], totals["ours"], totals["skipped"]))
    print("total: %d" % sum(totals.values()))
    return 0


def _model_map(root, mn):
    m = load_map(root, mn)
    if m is None:
        return None
    return {e["idx"]: e for e in enemies(m)}


def cmd_diff(vanilla, output, only=None, skipped=()):
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
                r = exclusion_reason(e["name"], skipped)
                note = ("frozen (%s: %s)" % r) if r else "same model by chance"
                if is_m28_forced(mn, e["name"]):
                    note = "m28 forced - randomized, redrew the same model"
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
    # --skip c1050,c1051 replaces the old --bell, which was the same two
    # patterns under a setting that no longer exists (feature 032 D1).
    a = [x for x in sys.argv if not x.startswith("--skip")]
    skipped = ()
    for x in sys.argv:
        if x.startswith("--skip="):
            skipped = tuple(m for m in x[len("--skip="):].split(",") if m)
    if len(a) >= 3 and a[1] == "frozen":
        return cmd_frozen(a[2], a[3] if len(a) > 3 else None, skipped)
    if len(a) >= 4 and a[1] == "diff":
        return cmd_diff(a[2], a[3], a[4] if len(a) > 4 else None, skipped)
    if len(a) >= 4 and a[1] == "compare":
        return cmd_compare(a[2], a[3], a[4] if len(a) > 4 else None)
    if len(a) >= 3 and a[1] == "pool":
        return cmd_pool(a[2])
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main())
