#!/usr/bin/env python3
"""Boss randomization verification tooling.

Two deliberately different mechanisms, per docs/plans/boss-randomization.md §7:

  pool    - an ORACLE derived from the reference C# source (GenerateBossList).
            Used for data-assumption checks (V0) and to prove the pool rules
            produce something sane against real vanilla maps. It does NOT try
            to predict which boss lands where in the C++ build - cross-language
            std::mt19937/uniform_int_distribution matching would be testing the
            PRNG, not the rules.

  verify  - a PROPERTY VALIDATOR (V2/V3/V4). It never re-derives expected
            assignments. It checks invariants of real generated output against
            the real vanilla input, so a misreading of the rules shared with
            the C++ still cannot make a fabricated NPCParamID exist in vanilla
            data or an out-of-range modelIndex valid.

Usage:
    python boss_verify.py pool   <vanilla_dvdroot>
    python boss_verify.py verify <vanilla_dvdroot> <output_dvdroot> [--enemies-also] [--easy]
    python boss_verify.py selftest <vanilla_dvdroot>
"""

import os
import sys
import zlib

# --- DCX container (DCX_DFLT_10000_44_9, per src/Msb/Dcx.cpp) ---------------

DCX_DATA_START = 76


def read_dcx(path):
    with open(path, "rb") as f:
        b = f.read()
    if b[:4] != b"DCX\x00":
        raise ValueError("%s: bad DCX magic" % path)
    compressed_size = int.from_bytes(b[32:36], "big")
    return zlib.decompress(b[DCX_DATA_START:DCX_DATA_START + compressed_size])


# --- MSBB parsing ----------------------------------------------------------
# Offsets mirror src/Msb/Msbb.cpp, which in turn mirrors SoulsFormats'
# MSBB (MSBBB.cs / PartsParam.cs / ModelParam.cs). Kept as named constants so
# a layout mistake shows up in one place rather than as a magic number.

PART_NAME_OFFSET      = 0x08
PART_TYPE             = 0x14
PART_MODEL_INDEX      = 0x1C
PART_BASE_DATA_OFFSET = 0xB0
PART_TYPE_DATA_OFFSET = 0xB8
ENEMY_THINK_IN_TYPEDATA = 8
ENEMY_NPC_IN_TYPEDATA   = 12

MODEL_NAME_OFFSET = 0x00
MODEL_TYPE        = 0x08

PARTS_TYPE_ENEMY = 2
MODEL_TYPE_ENEMY = 2


def _i32(b, off):
    return int.from_bytes(b[off:off + 4], "little", signed=True)


def _i64(b, off):
    return int.from_bytes(b[off:off + 8], "little", signed=True)


def _utf16z(b, off):
    end = off
    while end + 1 < len(b) and b[end:end + 2] != b"\x00\x00":
        end += 2
    return b[off:end].decode("utf-16-le", errors="replace")


class Section:
    def __init__(self, entries):
        self.entries = entries


def _read_section(b, pos, expected_type):
    offset_count = _i32(b, pos + 4)
    pos += 8
    name_offset = _i64(b, pos)
    pos += 8
    entry_offsets = []
    for _ in range(offset_count - 1):
        entry_offsets.append(_i64(b, pos))
        pos += 8
    next_param_offset = _i64(b, pos)
    pos += 8

    type_name = _utf16z(b, name_offset)
    if type_name != expected_type:
        raise ValueError("expected %s, got %s" % (expected_type, type_name))

    end_sentinel = next_param_offset if next_param_offset != 0 else len(b)
    boundaries = sorted(entry_offsets + [end_sentinel])
    entries = []
    for off in entry_offsets:
        end = next((x for x in boundaries if x > off), len(b))
        entries.append(bytearray(b[off:end]))  # mutable, so selftest can corrupt
    return Section(entries), next_param_offset


class Msbb:
    def __init__(self, data):
        if data[:4] != b"MSB ":
            raise ValueError("bad MSB magic")
        pos = 16
        self.models, pos = _read_section(data, pos, "MODEL_PARAM_ST")
        self.events, pos = _read_section(data, pos, "EVENT_PARAM_ST")
        self.regions, pos = _read_section(data, pos, "POINT_PARAM_ST")
        self.parts, pos = _read_section(data, pos, "PARTS_PARAM_ST")

    def model_name(self, index):
        if index < 0 or index >= len(self.models.entries):
            return None
        blob = self.models.entries[index]
        return _utf16z(blob, _i64(blob, MODEL_NAME_OFFSET))

    def enemies(self):
        """Yields (index_in_parts, name, npc, think, entity_id, model_index)."""
        for i, blob in enumerate(self.parts.entries):
            if int.from_bytes(blob[PART_TYPE:PART_TYPE + 4], "little") != PARTS_TYPE_ENEMY:
                continue
            td = _i64(blob, PART_TYPE_DATA_OFFSET)
            bd = _i64(blob, PART_BASE_DATA_OFFSET)
            yield (
                i,
                _utf16z(blob, _i64(blob, PART_NAME_OFFSET)),
                _i32(blob, td + ENEMY_NPC_IN_TYPEDATA),
                _i32(blob, td + ENEMY_THINK_IN_TYPEDATA),
                _i32(blob, bd),
                _i32(blob, PART_MODEL_INDEX),
            )


# --- Reference rules (docs/plans/boss-randomization.md §4) ------------------

BOSS_NAMES = [
    "5090", "c2090_0003", "c2100_0000", "c2100_0001", "c2120_0000",
    "c2120_0001", "c2120_0002", "c2320_0000", "c2500_0000", "c2570_0001",
    "c2510_0000", "c2710_0000", "c2720_0000", "c4520_0002", "c4030_0000",
    "c4030_0001", "c4030_0002", "c4030_0003", "c4030_0004", "c4500_0000",
    "c4510_0000", "c4540_0000", "c4541_0000", "c5000_0000", "c5020_0000",
    "c5070_0000", "c5080_0000", "c5100_0000", "c5120_0001", "c5400_0000",
    "c5510_0000", "c8050_0000", "c3130_0000", "c5010_0000", "c4510_0002",
    "c3050_0000", "c3060_0000", "c5110",
]

REJECT_NPC = {250060, 250070, 250090, 212600, 212610, 212620}

POOL_INSERT_REJECT = [
    "c2500_0000", "c5071_0000", "c4030_0004", "c2100_0000", "c2100_0001",
    "c4540_0000", "c4030_0001", "c4030_0002", "c4030_0003", "c2120_0001",
    "c2120_0000", "c2120_0002", "c2570", "c2571", "c4030_0000",
]

BOSS_MAP_ORDER = [
    "m24_00_00_01", "m24_02_00_01", "m21_00_00_00", "m21_01_00_00",
    "m22_00_00_00", "m23_00_00_00", "m23_00_00_01", "m24_01_00_01",
    "m25_00_00_00", "m26_00_00_00", "m27_00_00_01", "m28_00_00_01",
    "m32_00_00_01", "m33_00_00_00", "m34_00_00_00", "m35_00_00_00",
    "m36_00_00_00",
]

FIXUP_GROUPS = {
    "c2500_0000": ["c2570_0001"],
    "c5510_0000": ["c5510_0001", "c5510_0002"],
    "c4520_0002": ["c4520_0000"],
    "c4030_0004": ["c4030_0000"],
}


def is_boss_name(name):
    return any(p in name for p in BOSS_NAMES)


def pool_eligible(map_name, name, npc, think, entity_id):
    """Spec §4.2, lesserBosses = false."""
    if npc in REJECT_NPC:
        return False
    if entity_id == -1:
        return False
    if npc in (0, -1):
        return False
    if think == -1:
        return False
    if "c5070" in name:
        return False
    if "c3060" in name and npc != 210306016:
        return False
    if "34" in map_name and npc == 210030:
        return False
    if "m28" in map_name and "c2100" in name:
        return False
    if "m26" not in map_name and "c0000_0005" in name:
        return False
    return True


def assign_eligible(map_name, name, npc, think, entity_id):
    """Spec §4.3 - deliberately a different list from pool_eligible (D5)."""
    if npc in REJECT_NPC:
        return False
    if entity_id == -1:
        return False
    if npc in (0, -1):
        return False
    if "c3060" in name:
        return False
    if "m36" not in map_name and think == 454000:
        return False
    if "m22" in map_name and "c2100_0001" in name:
        return False
    if "m27" in map_name and ("c2120_0001" in name or "c2120_0002" in name):
        return False
    if "m35" in map_name and any(
        s in name for s in ("c4030_0001", "c4030_0002", "c4030_0003", "c4030_0004")
    ):
        return False
    if "m26" not in map_name and "c0000_0005" in name:
        return False
    if "34" in map_name and npc == 210030:
        return False
    if "m28" in map_name and "c2100" in name:
        return False
    return True


def map_path(root, map_name):
    return os.path.join(root, "map", "mapstudio", map_name + ".msb.dcx")


def load_maps(root):
    maps = {}
    for name in BOSS_MAP_ORDER:
        p = map_path(root, name)
        if os.path.exists(p):
            maps[name] = Msbb(read_dcx(p))
    return maps


def build_pool(maps):
    """Oracle for spec §4.2. Returns (pool, refill_pool, ook_identity)."""
    pool = []
    ook = None
    for map_name in BOSS_MAP_ORDER:
        msbb = maps.get(map_name)
        if msbb is None:
            continue
        per_map = []
        seen_npc = set()
        for _, name, npc, think, entity_id, model_index in msbb.enemies():
            model = msbb.model_name(model_index) or ""
            if "c4540_0000" in name:
                ook = (npc, think, model)
            if not is_boss_name(name):
                continue
            if not pool_eligible(map_name, name, npc, think, entity_id):
                continue
            if any(s in name for s in POOL_INSERT_REJECT):
                continue
            if npc in seen_npc:
                continue
            seen_npc.add(npc)
            if think == 1:
                continue
            per_map.append((npc, think, model))
        pool.extend(per_map)

    deduped = []
    for e in pool:
        if e not in deduped:
            deduped.append(e)

    refill = []
    seen_models = set()
    for e in deduped:
        if e[2] in seen_models:
            continue
        seen_models.add(e[2])
        refill.append(e)

    return deduped, refill, ook


def boss_pool_models(vanilla_root):
    """The distinct models of the boss candidate pool, in the frozen order the
    picker's baked table and its config string use: display name, then model id
    to break ties. Mirrors enemy_lookup.engine_pool_models.

    build_pool() above is the oracle for WHICH identities are eligible; this
    only projects it down to models and orders them for the UI.
    """
    from names import model_name
    pool, _refill, _ook = build_pool(load_maps(vanilla_root))
    models = sorted({m for _npc, _think, m in pool if m})
    return sorted(models, key=lambda mdl: ((model_name(mdl) or mdl).upper(), mdl))


# --- Commands --------------------------------------------------------------

def cmd_pool(vanilla_root):
    maps = load_maps(vanilla_root)
    print("loaded %d/%d boss maps from %s\n" % (len(maps), len(BOSS_MAP_ORDER), vanilla_root))
    missing = [m for m in BOSS_MAP_ORDER if m not in maps]
    if missing:
        print("  MISSING: %s\n" % ", ".join(missing))

    pool, refill, ook = build_pool(maps)

    print("=== V0: data assumptions ===")
    ok = True

    if ook is None:
        print("  FAIL  Orphan source placement c4540_0000 not found in any boss map")
        ok = False
    else:
        print("  Orphan (c4540_0000) identity: npc=%d think=%d model=%s" % ook)
        if ook[1] == 454000:
            print("  PASS  Orphan ThinkParamID == 454000 - spec §5 holds: forcing this")
            print("        identity makes §4.3 skip the placement, so AddOrphanPhaseOne")
            print("        survives rather than being clobbered.")
        else:
            print("  FAIL  Orphan ThinkParamID is %d, not 454000 - spec §5's premise is")
            print("        WRONG. AddOrphanPhaseOne's effect would be overwritten by")
            print("        assignment. Revisit before relying on it." % ook[1])
            ok = False

    if len(pool) == 0:
        print("  FAIL  boss pool is empty")
        ok = False
    else:
        print("  PASS  boss pool has %d entries (refill pool: %d)" % (len(pool), len(refill)))

    dupes = len(pool) - len(set(pool))
    print("  %s  pool identity-dedupe: %d duplicates remain" %
          ("PASS" if dupes == 0 else "FAIL", dupes))
    ok = ok and dupes == 0

    print("\n=== boss pool (npc*think*model) ===")
    for npc, think, model in pool:
        print("  %d*%d*%s" % (npc, think, model))

    print("\n=== assignment targets per map ===")
    total = 0
    for map_name in BOSS_MAP_ORDER:
        msbb = maps.get(map_name)
        if msbb is None:
            continue
        targets = [
            name for _, name, npc, think, eid, _ in msbb.enemies()
            if is_boss_name(name) and assign_eligible(map_name, name, npc, think, eid)
        ]
        total += len(targets)
        if targets:
            print("  %-14s %2d  %s" % (map_name, len(targets), ", ".join(sorted(targets))))
    print("  %-14s %2d total" % ("", total))

    if total == 0:
        print("\n  FAIL  no assignment targets - boss randomization would be a no-op")
        ok = False

    print("\nV0 %s" % ("PASSED" if ok else "FAILED"))
    return 0 if ok else 1


def all_vanilla_triples(vanilla_root):
    """Every (npc, think, model) present anywhere in the vanilla map set.

    Deliberately wider than the boss maps: the enemy pool is drawn from all 24
    base maps, so a legitimate enemy assignment can carry an identity that
    appears in none of the 17 boss maps.
    """
    triples = set()
    mapdir = os.path.join(vanilla_root, "map", "mapstudio")
    for dirpath, _, filenames in os.walk(mapdir):
        for fn in filenames:
            if not fn.endswith(".msb.dcx"):
                continue
            try:
                msbb = Msbb(read_dcx(os.path.join(dirpath, fn)))
            except Exception:
                continue
            for _, _, npc, think, _, mi in msbb.enemies():
                triples.add((npc, think, msbb.model_name(mi) or ""))
    return triples


def load_scaling_ids(repo_root):
    """Every NPCParamID that BossParamScaling may legitimately write.

    These live in NpcScalingTable.h, not in any map: the scaling pass swaps a
    placement's NPCParamID for a pre-tuned per-zone variant of the SAME
    creature. Such a value is absent from vanilla map placements by design, so
    "not in the maps" is NOT evidence of corruption - checking only placements
    is the incomplete check that has burned this project before.
    """
    path = os.path.join(repo_root, "PS4", "bbrandomizer", "src", "Randomizer",
                        "NpcScalingTable.h")
    ids = set()
    try:
        with open(path, encoding="utf-8") as f:
            for line in f:
                stripped = line.lstrip()
                if stripped.startswith("//"):
                    continue
                for tok in stripped.replace("{", " ").replace("}", " ").split(","):
                    tok = tok.strip()
                    if tok.isdigit():
                        ids.add(int(tok))
    except OSError:
        pass
    return ids


def compare_trees(van_maps, out_maps, enemies_also=False, vanilla_triples=None,
                  scaling_ids=None, vanilla_think_model=None, easy_suppress=None):
    """V2/V3/V4 property checks. Returns (failures, changed_count).

    Never re-derives expected assignments - only checks invariants that hold
    regardless of which boss landed where.

    enemies_also: the run under test had enemy randomization on too, so changes
    to non-boss placements are expected rather than a violation.

    easy_suppress: {(map, placement name)} the easy-mode settings (feature 018)
    deliberately overwrote. Those placements are Rom's children and the small
    emissaries - not boss names - so each would otherwise raise a V2 non-boss
    objection on a perfectly correct tree. Only that one question is
    suppressed; every other property below still applies to them. Nothing
    POSITIVE about the easy settings is asserted here - easy_modes_verify.py
    owns that, and V2's semantics are load-bearing for boss randomization and
    should not grow a second meaning.
    """
    if vanilla_triples is None:
        vanilla_triples = set()
    if scaling_ids is None:
        scaling_ids = set()
    if vanilla_think_model is None:
        vanilla_think_model = set()
    if easy_suppress is None:
        easy_suppress = set()

    def identity_ok(npc, think, model):
        # Either the exact vanilla identity, or a scaling variant of a real
        # creature: the scaling pass rewrites only NPCParamID, so (think, model)
        # must still be a pair that exists in vanilla.
        if (npc, think, model) in vanilla_triples:
            return True
        return npc in scaling_ids and (think, model) in vanilla_think_model
    for msbb in van_maps.values():
        for _, _, npc, think, _, mi in msbb.enemies():
            vanilla_triples.add((npc, think, msbb.model_name(mi) or ""))

    failures = []
    changed_total = 0
    nonboss_changed = 0

    for map_name in BOSS_MAP_ORDER:
        van, out = van_maps.get(map_name), out_maps.get(map_name)
        if van is None or out is None:
            continue

        van_e = list(van.enemies())
        out_e = list(out.enemies())
        if len(van_e) != len(out_e):
            failures.append("%s: enemy count changed %d -> %d" %
                            (map_name, len(van_e), len(out_e)))
            continue

        assigned = {}
        for (vi, vname, vnpc, vthink, veid, vmi), (oi, oname, onpc, othink, oeid, omi) in zip(van_e, out_e):
            vmodel = van.model_name(vmi) or ""
            omodel = out.model_name(omi) or ""

            if vname != oname:
                failures.append("%s: part name changed %s -> %s" % (map_name, vname, oname))
            if veid != oeid:
                failures.append("%s/%s: EntityID changed %d -> %d" % (map_name, vname, veid, oeid))

            if omi < 0 or omi >= len(out.models.entries):
                failures.append("%s/%s: V3 modelIndex %d out of range (models=%d)" %
                                (map_name, vname, omi, len(out.models.entries)))

            unchanged = (vnpc, vthink, vmodel) == (onpc, othink, omodel)
            if unchanged:
                continue

            # V2: only boss placements the rules allow may change - unless the
            # run also had enemy randomization on, in which case non-boss
            # changes are the enemy pass doing its job.
            if not is_boss_name(vname):
                nonboss_changed += 1
                if (map_name, vname) in easy_suppress:
                    pass  # an easy-mode target - see the docstring
                elif not enemies_also:
                    failures.append("%s/%s: V2 non-boss placement changed" % (map_name, vname))
                elif not identity_ok(onpc, othink, omodel):
                    failures.append(
                        "%s/%s: V2 enemy assignment %d*%d*%s is in neither vanilla data "
                        "nor the scaling table" % (map_name, vname, onpc, othink, omodel))
                continue

            # A boss REASSIGNMENT swaps the whole identity. The scaling pass
            # rewrites NPCParamID alone and leaves think+model untouched, so a
            # change of npc only - to a known scaling id - is scaling doing its
            # job on a placement boss randomization never selected. Treating
            # those as assignments produced false "ineligible placement
            # changed" and "self-assignment" reports on real hardware output.
            scaling_only = (vthink == othink and vmodel == omodel and
                            onpc in scaling_ids)
            if scaling_only:
                continue

            changed_total += 1
            assigned[vname] = (onpc, othink, omodel)

            if not assign_eligible(map_name, vname, vnpc, vthink, veid):
                # AddTheRest legitimately changes some §4.3-ineligible placements.
                addtherest = (
                    ("m22" in map_name and "c2100_0001" in vname) or
                    ("m27" in map_name and ("c2120_0001" in vname or "c2120_0002" in vname)) or
                    ("m35" in map_name and any(s in vname for s in
                                               ("c4030_0001", "c4030_0002", "c4030_0003")))
                )
                fixup_target = any(vname in comps for comps in FIXUP_GROUPS.values())
                orphan_target = vname in ("c2710_0000", "c2500_0000", "c4510_0000")
                if not (addtherest or fixup_target or orphan_target):
                    failures.append("%s/%s: V2 ineligible placement changed" % (map_name, vname))

            # V2: assigned identity must exist in vanilla data somewhere.
            if not identity_ok(onpc, othink, omodel):
                failures.append("%s/%s: V2 assigned identity %d*%d*%s is in neither vanilla "
                                "data nor the scaling table"
                                % (map_name, vname, onpc, othink, omodel))

            # V2: a boss must not be replaced by its own model.
            if omodel and omodel in vname:
                failures.append("%s/%s: V2 self-assignment (model %s)" % (map_name, vname, omodel))

        # V2: multi-entity fixup groups must share one identity - but only when
        # the leader was actually reassigned. Spec D7 makes the fixup a
        # post-pass, and it is skipped entirely for a leader that assignment
        # excluded (m35's c4030_0004), so a companion there is free to carry its
        # own independent assignment.
        out_by_name = {n: (npc, th, out.model_name(mi) or "")
                       for _, n, npc, th, _, mi in out_e}
        for leader, companions in FIXUP_GROUPS.items():
            if leader not in assigned:
                continue
            for comp in companions:
                if comp not in out_by_name:
                    continue  # companion doesn't exist in this map
                # Compare think+model only: the scaling pass rewrites
                # NPCParamID per zone and can legitimately differ between two
                # placements that the fixup did synchronise.
                if out_by_name[comp][1:] != assigned[leader][1:]:
                    failures.append("%s: V2 fixup group %s/%s out of sync (leader %s, companion %s)"
                                    % (map_name, leader, comp, assigned[leader], out_by_name[comp]))

    return failures, changed_total, nonboss_changed


def easy_mode_targets(van_maps):
    """{(map, name)} the feature-018 table selects, within the boss maps.

    Imported lazily and from easy_modes_verify.py rather than parsed again
    here: EasyModes.h must have exactly one reader on this side, or the two
    copies drift and the suppression stops matching what the port writes.
    The import is inside the function because easy_modes_verify imports this
    module at load time."""
    from easy_modes_verify import parse_table, matched_in
    out = set()
    for _flag, map_name, patterns in parse_table():
        m = van_maps.get(map_name)
        if m is None:
            continue  # not a boss map, so compare_trees never walks it anyway
        for name in matched_in(m, patterns):
            out.add((map_name, name))
    return out


def cmd_verify(vanilla_root, output_root, enemies_also=False, easy=False):
    van_maps = load_maps(vanilla_root)
    out_maps = load_maps(output_root)
    triples = all_vanilla_triples(vanilla_root)
    think_model = {(t, m) for _, t, m in triples}
    repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
    scaling = load_scaling_ids(repo_root)
    print("vanilla identity set: %d triples, %d (think, model) pairs; scaling table: %d ids"
          % (len(triples), len(think_model), len(scaling)))
    suppress = easy_mode_targets(van_maps) if easy else set()
    if easy:
        by_map = {}
        for map_name, name in suppress:
            by_map[map_name] = by_map.get(map_name, 0) + 1
        print("--easy: not asking whether %d placement(s) may change - %s"
              % (len(suppress),
                 ", ".join("%s x%d" % (mn, n) for mn, n in sorted(by_map.items()))))
        print("        they are EasyModes.h's own targets. What they were replaced")
        print("        WITH is easy_modes_verify.py's question, not this tool's.")
    failures, changed_total, nonboss_changed = compare_trees(
        van_maps, out_maps, enemies_also=enemies_also, vanilla_triples=triples,
        scaling_ids=scaling, vanilla_think_model=think_model, easy_suppress=suppress)

    print("compared %d maps; %d boss placements changed, %d non-boss placements changed" %
          (len(out_maps), changed_total, nonboss_changed))
    if changed_total == 0:
        print("NOTE: nothing changed - this is V4 (vanilla preserved) passing, or boss")
        print("      randomization did not run.")
    if failures:
        print("\n%d FAILURES:" % len(failures))
        for f in failures[:60]:
            print("  " + f)
        if len(failures) > 60:
            print("  ... and %d more" % (len(failures) - 60))
        return 1
    print("\nAll property checks PASSED")
    return 0


# --- selftest: prove the validator actually catches things -----------------

def _set_i32(blob, off, value):
    blob[off:off + 4] = int(value).to_bytes(4, "little", signed=True)


def _set_enemy_npc(blob, value):
    _set_i32(blob, _i64(blob, PART_TYPE_DATA_OFFSET) + ENEMY_NPC_IN_TYPEDATA, value)


def _find_part(msbb, predicate):
    for i, blob in enumerate(msbb.parts.entries):
        if int.from_bytes(blob[PART_TYPE:PART_TYPE + 4], "little") != PARTS_TYPE_ENEMY:
            continue
        name = _utf16z(blob, _i64(blob, PART_NAME_OFFSET))
        if predicate(name):
            return i, name
    return None, None


def cmd_selftest(vanilla_root):
    """Corrupt a known-good tree in specific ways and require each to be caught.

    A property validator that has only ever returned PASS proves nothing, so
    each case below asserts BOTH that a failure is reported and that it is the
    expected one.
    """
    import copy

    base = load_maps(vanilla_root)
    if not base:
        print("no maps loaded from %s" % vanilla_root)
        return 2
    all_triples = all_vanilla_triples(vanilla_root)

    cases = []

    # 1. A non-boss placement changed -> V2 must object.
    m = copy.deepcopy(base)
    target = m["m21_00_00_00"]
    idx, name = _find_part(target, lambda n: not is_boss_name(n))
    _set_enemy_npc(target.parts.entries[idx], 999999)
    cases.append(("non-boss placement changed", m, "V2 non-boss placement changed"))

    # 2. A boss assigned an identity that exists nowhere in vanilla -> V2.
    m = copy.deepcopy(base)
    target = m["m21_00_00_00"]
    idx, name = _find_part(target, is_boss_name)
    _set_enemy_npc(target.parts.entries[idx], 123456789)
    cases.append(("fabricated NPCParamID", m, "neither vanilla data nor the scaling table"))

    # 3. modelIndex pointing past the model table -> V3.
    m = copy.deepcopy(base)
    target = m["m21_00_00_00"]
    idx, name = _find_part(target, is_boss_name)
    _set_i32(target.parts.entries[idx], PART_MODEL_INDEX, 99999)
    cases.append(("out-of-range modelIndex", m, "modelIndex"))

    # 4. A scaling-only NPCParamID rewrite must NOT be reported (this exact
    #    false positive showed up against real hardware output).
    scaling_ids_local = load_scaling_ids(
        os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", "..")))
    m = copy.deepcopy(base)
    target = m["m26_00_00_00"]
    idx, name = _find_part(target, is_boss_name)
    if scaling_ids_local:
        _set_enemy_npc(target.parts.entries[idx], sorted(scaling_ids_local)[-1])
        failures_s, _, _ = compare_trees(
            base, m, vanilla_triples=all_triples, scaling_ids=scaling_ids_local,
            vanilla_think_model={(t, mo) for _, t, mo in all_triples})
        quiet = not failures_s
        print("  %-28s %s" % ("scaling-only npc rewrite", "IGNORED" if quiet else "FALSE POSITIVE"))
        if not quiet:
            print("     unexpected: %s" % failures_s[:3])
        extra_passed = 1 if quiet else 0
    else:
        print("  %-28s %s" % ("scaling-only npc rewrite", "SKIPPED (no scaling table)"))
        extra_passed = 1

    passed = 0
    for label, corrupted, expected_fragment in cases:
        failures, _, _ = compare_trees(base, corrupted)
        hit = any(expected_fragment in f for f in failures)
        print("  %-28s %s" % (label, "CAUGHT" if hit else "MISSED"))
        if not hit:
            print("     expected a failure containing %r; got: %s" %
                  (expected_fragment, failures[:3] or "no failures at all"))
        else:
            passed += 1

    # 4. And the null case must still pass, or the validator is just noisy.
    failures, _, _ = compare_trees(base, copy.deepcopy(base))
    clean = not failures
    print("  %-28s %s" % ("unmodified tree", "CLEAN" if clean else "FALSE POSITIVE"))
    if not clean:
        print("     unexpected: %s" % failures[:3])
    else:
        passed += 1

    passed += extra_passed
    total = len(cases) + 2  # + unmodified-tree + scaling-only cases
    print("\nselftest %d/%d" % (passed, total))
    return 0 if passed == total else 1


def main():
    if len(sys.argv) >= 3 and sys.argv[1] == "pool":
        return cmd_pool(sys.argv[2])
    if len(sys.argv) >= 4 and sys.argv[1] == "verify":
        enemies_also = "--enemies-also" in sys.argv[4:]
        easy = "--easy" in sys.argv[4:]
        return cmd_verify(sys.argv[2], sys.argv[3], enemies_also, easy)
    if len(sys.argv) >= 3 and sys.argv[1] == "selftest":
        return cmd_selftest(sys.argv[2])
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main())
