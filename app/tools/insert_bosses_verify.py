#!/usr/bin/env python3
"""BOSSES CAN REPLACE ENEMIES verification tooling (feature 013).

Two deliberately different mechanisms, the same split boss_verify.py uses:

  selftest - ORACLES and a MODEL. The oracles are measurements against the real
             vanilla tree and against the port's own generated headers: how many
             eligible targets each area holds, what the 32-identity insertion
             list is, which map files each of the fourteen area prefixes covers,
             and which identities the stat-scaling table does not track. The
             model is a Python replay of the five steps of plan 013 section 4,
             used to check the PROPERTIES those steps are supposed to have -
             that EXACTLY is an equality, that UP TO's mean does not depend on
             how many bosses are ticked, that the pool deals every identity
             once before any twice, that spacing beats a uniform draw, and that
             every off path consumes no randomness.

             It does NOT try to predict which boss lands where. Matching
             std::mt19937 across two languages would test the PRNG, not the
             rules - the same reason boss_verify.py does not.

  verify   - a PROPERTY VALIDATOR over a real generated tree against the real
             vanilla input. Spec 013 section 8 criteria 2-18, each reported
             separately and each naming the map and placement that failed it.

WHAT MAKES AN INSERTED BOSS IDENTIFIABLE IN OUTPUT. The 22 insertion models and
the 82 models of the enemy pool are disjoint sets, by construction rather than
by luck: every boss-named placement is on the fixed exclusion list, so no boss
model can ever contribute to the enemy pool. So a placement whose OUTPUT model
is one of the 22 was written by this pass and by nothing else - which is what
lets verify separate this feature's work from the enemy pass, the boss pass and
the easy modes without being told what else the run had switched on.

Usage:
    python insert_bosses_verify.py selftest <vanilla_dvdroot>
    python insert_bosses_verify.py verify   <vanilla_dvdroot> <output_dvdroot>
                                            [--count=N] [--exactly] [--replace-all]
                                            [--spread] [--keep-ai]
                                            [--areas=m22,m27] [--bosses=c2100,c4030]
                                            [--skipped=c1050,c1051] [--caged-dogs]

Every flag names the configuration the OUTPUT TREE was generated with. They
default to the app's own defaults - count 1, UP TO, every area, every boss, OWN
AI, spacing off - so a default run needs no flags.
"""

import os
import random
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import boss_verify as BV          # noqa: E402
import enemy_lookup as EL         # noqa: E402
import caged_dogs_verify as CD    # noqa: E402

RND = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "src", "Randomizer")

# Measured, and asserted rather than assumed - see selftest.
EXPECTED_IDENTITIES = 32
EXPECTED_MODELS = 22
EXPECTED_LIVE_TARGETS = 1381
EXPECTED_ALL_TARGETS = 2269
EXPECTED_AREA_FILES = 22
MAX_COUNT = 15


# ---------------------------------------------------------------------------
# the port's own headers, parsed rather than restated
# ---------------------------------------------------------------------------

def parse_table(header):
    """[(model, display name, weight)] out of a generated ModelPoolEntry table.

    Parsed, not retyped: a second copy of a generated table in the checker pins
    the checker's copy and lets the shipped one drift, which is the failure the
    table checks exist to catch.
    """
    text = open(os.path.join(RND, header), encoding="utf-8").read()
    body = text.split("kTable = {{", 1)[1].split("}};", 1)[0]
    body = re.sub(r"//[^\n]*", "", body)
    return [(m, n, int(w)) for m, n, w in
            re.findall(r'\{\s*"(\w+)"\s*,\s*"([^"]*)"\s*,\s*(-?\d+)\s*\}', body)]


def table_count(header, const):
    text = open(os.path.join(RND, header), encoding="utf-8").read()
    return int(re.search(r"const int %s = (\d+);" % const, text).group(1))


# ---------------------------------------------------------------------------
# oracles
# ---------------------------------------------------------------------------

def eligible_targets(root, maps, skipped=(), caged_dogs=False):
    """{map file: [enemy dict]} - the placements the insertion pass may write
    over, per plan 013 B5.

    Built from enemy_lookup.overwritable_placements, which is the port's own
    "what can be REPLACED" oracle and already applies the 103-pattern fixed
    exclusion list and the model-resolves test. The two optional protections are
    layered on top in the same order the pass applies them.
    """
    cages = CD.parse_header() if caged_dogs else []
    out = {}
    for mapname, e in EL.overwritable_placements(root, maps):
        if skipped and any(p in e["name"] for p in skipped):
            continue
        if caged_dogs and CD.is_protected(cages, mapname, e["entity"]):
            continue
        out.setdefault(mapname, []).append(e)
    return out


def area_of(mapname, prefixes=None):
    """Which area prefix covers this map file, or None. The prefixes are
    mutually exclusive, which selftest checks rather than assumes."""
    for prefix in (prefixes or BV.INSERT_AREA_PREFIXES):
        if mapname.startswith(prefix):
            return prefix
    return None


def scaling_tracked(root, maps):
    """(tracked, untracked) insertion identities, by whether the stat-scaling
    table rewrites their NPCParamID in at least one live area file.

    zone_scaled_npc returns its input unchanged for an untracked value, which
    reads as "everything is scaled" if you look at the return value alone - so
    this tests the INEQUALITY, which is the only form of the question that
    distinguishes the two.
    """
    live = [lf for _p, lf, _n, _c in BV.insertion_areas(root, maps)]
    tracked, untracked = [], []
    for npc, think, model in BV.insertion_list(root, maps):
        if any(EL.zone_scaled_npc(lf, npc) != npc for lf in live):
            tracked.append((npc, think, model))
        else:
            untracked.append((npc, think, model))
    return tracked, untracked


# ---------------------------------------------------------------------------
# the model of the pass - plan 013 section 4, steps 1 to 5
# ---------------------------------------------------------------------------

class Draws:
    """A counting RNG. Every draw in the model goes through here, because "an
    off run consumes NO randomness" is the one invariant whose breakage stays
    invisible until a player reuses a seed - so it is checked by counting, not
    by reading the code.

    randrange(n) is C++'s RandInt(0, n - 1): same inclusive-exclusive shape,
    deliberately not the same generator.
    """

    def __init__(self, seed):
        self.rnd = random.Random(seed)
        self.n = 0

    def below(self, n):
        self.n += 1
        return self.rnd.randrange(n)


class Pool:
    """The run pool: drawn by identity, erased, refilled from the FULL enabled
    list when it empties, across the whole run rather than per map."""

    def __init__(self, identities):
        self.refill = list(identities)
        self.entries = list(identities)
        self.used = {i: 0 for i in identities}

    def draw(self, draws):
        i = draws.below(len(self.entries))
        chosen = self.entries.pop(i)
        self.used[chosen] = self.used.get(chosen, 0) + 1
        if not self.entries:
            self.entries = list(self.refill)
        return chosen


def choose(positions, count, draws, exact=False, replace_all=False, spread=False):
    """Steps 3 and 4: how many, then which, in write order.

    A line-by-line replay of plan 013 section 4. The two steps are separate on
    purpose (plan P3): that is what makes a count of 1 consume the same draws
    with spacing on and off, and it is what makes spacing possible at all.
    """
    supply = len(positions)
    if supply == 0:
        return []
    if count <= 0 and not replace_all:
        return []          # the pass's fourth gate, before any draw
    cap = min(count, supply)

    # --- step 3: how many -------------------------------------------------
    if replace_all:
        k = supply
    elif exact:
        k = cap                      # an equality, and no draws
    else:
        k = 0
        for _ in range(supply):      # one draw per eligible placement, always
            if draws.below(supply) < cap and k < cap:
                k += 1
    if k <= 0:
        return []

    # --- step 4: which k --------------------------------------------------
    if replace_all:
        return list(range(supply))
    if not spread:
        remaining = list(range(supply))
        chosen = []
        for _ in range(k):
            pick = draws.below(len(remaining))
            chosen.append(remaining[pick])
            remaining[pick] = remaining[-1]
            remaining.pop()
        return chosen
    chosen = [draws.below(supply)]
    taken = set(chosen)
    while len(chosen) < k:
        best, best_dist = None, -1.0
        for i in range(supply):
            if i in taken:
                continue
            nearest = min(dist2(positions[i], positions[c]) for c in chosen)
            if nearest > best_dist:   # strict, so the lowest index keeps a tie
                best, best_dist = i, nearest
        if best is None:
            break
        chosen.append(best)
        taken.add(best)
    return chosen


def capped_binomial_mean(supply, cap):
    """E[min(X, cap)] for X ~ Binomial(supply, cap / supply).

    THE ANALYTIC MEAN OF THE RULE AS WRITTEN, and the reason the check below is
    an equality against this rather than against N. UP TO takes one draw per
    eligible placement at probability cap / supply, which has mean exactly cap,
    and then refuses to go past cap - and truncating the upper tail of a
    distribution lowers its mean. The cost is small at a high count and large at
    a low one: at the smallest area's 54 targets the mean is 13.9 of 15, 4.3 of
    5 and 0.63 of 1.

    That is a property of the design in plan 013 section 4 step 3, not of this
    implementation of it, and it is worth knowing before the hardware test: at
    the default count of 1, UP TO puts about nine bosses in a full run and not
    fourteen. EXACTLY 1 is the setting that gives fourteen.

    Asserting against this instead of against N is also the STRONGER check. A
    probability taken from pool composition - the D-M failure - would miss this
    number by a factor of 20 at one boss ticked, and any other reading of the
    ceiling would miss it too.
    """
    p = float(cap) / float(supply)
    total = 0.0
    prob = (1.0 - p) ** supply
    for j in range(supply + 1):
        total += prob * min(j, cap)
        if j < supply and p > 0.0:
            prob *= (supply - j) / float(j + 1) * p / (1.0 - p)
    return total


def dist2(a, b):
    return (a[0] - b[0]) ** 2 + (a[1] - b[1]) ** 2 + (a[2] - b[2]) ** 2


def min_pairwise(positions, indices):
    if len(indices) < 2:
        return None
    return min(dist2(positions[a], positions[b])
               for i, a in enumerate(indices) for b in indices[i + 1:]) ** 0.5


# ---------------------------------------------------------------------------
# the static read of the C++, for the one invariant nothing else can see
# ---------------------------------------------------------------------------

def gates_precede_draws():
    """True if every gate in ApplyBossInsertion is tested before its first draw.

    Criterion 1 - "an off run is byte-identical to today's" - can only be proved
    by a tree diff on hardware. What CAN be proved here is the property the diff
    would be testing: that no path with the feature off reaches a RandInt. The
    four early returns are the whole of that argument, so their position
    relative to the first draw is read out of the source rather than trusted.
    """
    src = open(os.path.join(RND, "BossInsertion.cpp"), encoding="utf-8").read()
    body = src.split("int ApplyBossInsertion", 1)[1]
    first_draw = body.find("RandInt(")
    if first_draw < 0:
        return False, "ApplyBossInsertion never draws at all"
    gates = ["if (!options.enabled) return 0;",
             "if (pool.entries.empty()) return 0;",
             "if (!AreaTicked(mapName, options.areas)) return 0;",
             "if (options.countPerMap <= 0 && !options.replaceAll) return 0;"]
    for gate in gates:
        at = body.find(gate)
        if at < 0:
            return False, "gate missing: %s" % gate
        if at > first_draw:
            return False, "gate after the first draw: %s" % gate
    return True, ""


# ---------------------------------------------------------------------------
# selftest
# ---------------------------------------------------------------------------

def cmd_selftest(root):
    cases = []
    detail = []

    def case(name, ok, note=None):
        cases.append((name, bool(ok)))
        if note and not ok:
            detail.append("   " + note)

    maps = EL.load_base_maps(root)

    # --- the eligible-target set -------------------------------------------
    targets = eligible_targets(root, maps)
    areas = BV.insertion_areas(root, maps)
    live_files = [lf for _p, lf, _n, _c in areas]
    live_total = sum(len(targets.get(lf, [])) for lf in live_files)
    all_total = sum(len(v) for v in targets.values())
    case("eligible targets over the fourteen live area files are %d" % EXPECTED_LIVE_TARGETS,
         live_total == EXPECTED_LIVE_TARGETS, "measured %d" % live_total)
    case("eligible targets over all 24 base map files are %d" % EXPECTED_ALL_TARGETS,
         all_total == EXPECTED_ALL_TARGETS, "measured %d" % all_total)

    boss_named = [(mn, e["name"]) for mn, es in targets.items() for e in es
                  if BV.is_boss_name(e["name"])]
    case("zero boss-named placements are eligible targets",
         not boss_named, "%d of them: %s" % (len(boss_named), boss_named[:4]))

    case("neither m21 file contributes a single target",
         not targets.get("m21_00_00_00") and not targets.get("m21_01_00_00"))

    # The two protections, each measured as a SET DIFFERENCE rather than as a
    # count, so a protection that removed the wrong placements would still fail.
    cages = CD.parse_header()
    caged = {(mn, e["name"]) for mn, es in targets.items() for e in es
             if CD.is_protected(cages, mn, e["entity"])}
    guarded = eligible_targets(root, maps, caged_dogs=True)
    guarded_set = {(mn, e["name"]) for mn, es in guarded.items() for e in es}
    case("all 26 caged-dog placements are eligible, and all 26 fall outside "
         "the set when DO NOT RANDOMIZE CAGED DOGS is on",
         len(caged) == 26 and not (caged & guarded_set),
         "%d caged dogs eligible, %d still inside" % (len(caged), len(caged & guarded_set)))

    skip_rows = ("c1000", "c1051", "c1090", "c1170", "c1290")  # Old Yharnam's five kinds
    skipped = eligible_targets(root, maps, skipped=skip_rows)
    reachable = {(mn, e["name"]) for mn, es in targets.items() for e in es
                 if any(p in e["name"] for p in skip_rows)}
    skipped_set = {(mn, e["name"]) for mn, es in skipped.items() for e in es}
    case("every ENEMIES SKIPPED-reachable placement falls outside the set when "
         "its rows are ticked",
         reachable and not (reachable & skipped_set),
         "%d reachable, %d still inside" % (len(reachable), len(reachable & skipped_set)))
    case("ticking Old Yharnam's five kinds starves it below the maximum count "
         "- the shortfall case of criterion 18",
         len(skipped.get("m23_00_00_01", [])) < MAX_COUNT,
         "m23_00_00_01 still offers %d" % len(skipped.get("m23_00_00_01", [])))

    # --- the insertion list ------------------------------------------------
    identities = BV.insertion_list(root, maps)
    models = BV.insertion_models(root, maps)
    case("the insertion list is %d identities" % EXPECTED_IDENTITIES,
         len(identities) == EXPECTED_IDENTITIES, "measured %d" % len(identities))
    case("...across %d models" % EXPECTED_MODELS,
         len(models) == EXPECTED_MODELS, "measured %d" % len(models))
    case("Moon Presence (c5400) is absent",
         all(m != "c5400" for _n, _t, m in identities))
    case("every identity is distinct by (npc, think, model)",
         len(set(identities)) == len(identities))

    ref16 = [mn for mn in EL.BASE_MAPS if mn.endswith("_00_00")]
    case("the harvest is set-identical over kBaseMaps' 24 files, the "
         "reference's 16 and BossMapOrder()'s 17",
         set(identities) == set(BV.insertion_list(root, maps, order=ref16)) ==
         set(BV.insertion_list(root, maps, order=BV.BOSS_MAP_ORDER)))

    # --- both generated tables --------------------------------------------
    import gen_pool_table as GEN
    for key, header, const in (("insertboss", "InsertedBossTable.h", "kInsertedBossModelCount"),
                               ("area", "InsertAreaTable.h", "kInsertAreaCount")):
        on_disk = open(os.path.join(RND, header), encoding="utf-8", newline="").read()
        want = GEN.build(GEN.KINDS[key], root)
        case("%s is what the generator would write, row for row" % header,
             on_disk.replace("\r\n", "\n") == want)
        rows = parse_table(header)
        case("%s declares %d rows and its count constant agrees" % (header, len(rows)),
             len(rows) == table_count(header, const))

    boss_rows = parse_table("InsertedBossTable.h")
    case("the inserted-boss table's weights sum to the %d identities" % EXPECTED_IDENTITIES,
         sum(w for _m, _n, w in boss_rows) == EXPECTED_IDENTITIES)
    case("the inserted-boss table's models ARE the harvest's models, in order",
         [m for m, _n, _w in boss_rows] == models)
    case("it is NOT BossPoolTable.h - 22 rows against 17, and Moon Presence "
         "is in that one and not this",
         len(boss_rows) == 22 and
         len(parse_table("BossPoolTable.h")) == 17 and
         any(m == "c5400" for m, _n, _w in parse_table("BossPoolTable.h")))

    area_rows = parse_table("InsertAreaTable.h")
    case("the area table's weights sum to the %d live-area targets" % EXPECTED_LIVE_TARGETS,
         sum(w for _m, _n, w in area_rows) == EXPECTED_LIVE_TARGETS)

    # --- the area partition ------------------------------------------------
    prefixes = [m for m, _n, _w in area_rows]
    case("the table's fourteen prefixes are the fourteen of the reference's "
         "per-area flags", sorted(prefixes) == sorted(BV.INSERT_AREA_PREFIXES))
    matched = {}
    for mn in EL.BASE_MAPS:
        hits = [p for p in prefixes if mn.startswith(p)]
        matched[mn] = hits
    case("no map file matches two prefixes - so the first match is the only match",
         all(len(h) <= 1 for h in matched.values()),
         str({k: v for k, v in matched.items() if len(v) > 1}))
    covered = [mn for mn, h in matched.items() if h]
    case("the fourteen prefixes cover exactly the %d area map files" % EXPECTED_AREA_FILES,
         len(covered) == EXPECTED_AREA_FILES, "covered %d" % len(covered))
    case("and match neither m21 file",
         not matched["m21_00_00_00"] and not matched["m21_01_00_00"])
    case("every prefix covers at least one file", all(
        any(mn.startswith(p) for mn in EL.BASE_MAPS) for p in prefixes))

    # --- the off paths draw nothing ---------------------------------------
    ok, why = gates_precede_draws()
    case("every gate in ApplyBossInsertion is tested before its first draw", ok, why)

    positions = [tuple(e["pos"]) for e in targets["m24_01_00_01"]]
    for label, kwargs, count in (("count 0", {}, 0),
                                 ("count 0, EXACTLY", {"exact": True}, 0)):
        d = Draws(1)
        chosen = choose(positions, count, d, **kwargs)
        case("%s inserts nothing and consumes no draw" % label,
             chosen == [] and d.n == 0, "%d chosen, %d draws" % (len(chosen), d.n))
    d = Draws(1)
    chosen = choose([], 5, d)
    case("no eligible target inserts nothing and consumes no draw",
         chosen == [] and d.n == 0)

    # --- EXACTLY is an equality -------------------------------------------
    exact_ok, exact_note = True, ""
    for lf in live_files:
        pos = [tuple(e["pos"]) for e in targets[lf]]
        for n in (1, 5, 15):
            d = Draws(hash((lf, n)) & 0xFFFF)
            chosen = choose(pos, n, d, exact=True)
            if len(chosen) != min(n, len(pos)) or d.n != len(chosen):
                exact_ok = False
                exact_note = "%s n=%d gave %d of %d, %d draws" % (
                    lf, n, len(chosen), min(n, len(pos)), d.n)
    case("EXACTLY is an equality on min(N, eligible targets) in all fourteen "
         "areas, and takes no draw to decide the count", exact_ok, exact_note)

    # --- REPLACE ALL ENEMIES ----------------------------------------------
    all_ok = True
    for lf in live_files:
        pos = [tuple(e["pos"]) for e in targets[lf]]
        d = Draws(7)
        chosen = choose(pos, 0, d, replace_all=True)
        if sorted(chosen) != list(range(len(pos))) or d.n != 0:
            all_ok = False
    case("the override replaces every eligible target, at count 0, with no "
         "selection draw at all", all_ok)

    # --- UP TO: a ceiling, and a mean that does not depend on the pool -----
    #
    # This is the criterion that fails if the per-placement probability is taken
    # from pool composition instead of from the count (spec 013 D-M). The two
    # means below are measured with 32 identities and with 1, and they must be
    # the same number - the selection never sees the pool at all, which is what
    # the model makes checkable.
    smallest = "m24_02_00_01"
    pos = [tuple(e["pos"]) for e in targets[smallest]]
    trials = 200
    for n in (1, 5, 15):
        counts = []
        over = 0
        for seed in range(trials):
            d = Draws(seed)
            chosen = choose(pos, n, d)
            counts.append(len(chosen))
            if len(chosen) > min(n, len(pos)):
                over += 1
        mean = sum(counts) / float(trials)
        want = capped_binomial_mean(len(pos), min(n, len(pos)))
        case("UP TO N=%d never exceeds min(N, supply) over %d seeds" % (n, trials),
             over == 0)
        # Half a boss of slack at 200 seeds. min(X, cap) has a standard
        # deviation of about 2 at the top of the range, so the standard error of
        # a 200-trial mean is around 0.15 and half a boss is three of those -
        # wide enough not to fail on noise, and twenty times tighter than the
        # gap a probability taken from pool composition would open.
        case("UP TO N=%d has the mean the rule predicts, %.2f measured against "
             "%.2f analytic" % (n, mean, want), abs(mean - want) <= 0.5,
             "mean %.3f against the rule's own %.3f" % (mean, want))
        case("UP TO N=%d rises with N and stays under it (%.2f of %d)"
             % (n, mean, n), n * 0.6 <= mean <= n)
    case("UP TO does not put every map at N - it would be EXACTLY wearing a "
         "different label", len(set(
             len(choose([tuple(e["pos"]) for e in targets[lf]], 5, Draws(3)))
             for lf in live_files)) > 1)

    # The single-boss case, stated the way spec 013 section 8 criterion 4 states
    # it: the mean is a property of the count and the supply, so narrowing the
    # picker cannot move it. The model reaches the pool only in step 5, which is
    # why this is an exact equality and not a tolerance.
    def mean_count(identities_subset, n):
        total = 0
        pool = Pool(identities_subset)
        for seed in range(trials):
            d = Draws(seed)
            chosen = choose(pos, n, d)
            for _ in chosen:
                pool.draw(d)
            total += len(chosen)
        return total / float(trials)

    many = mean_count(identities, 5)
    one = mean_count([identities[0]], 5)
    case("UP TO's mean is IDENTICAL with 32 identities ticked and with one "
         "(%.3f vs %.3f) - the D-M failure" % (many, one), many == one)

    # --- the pool property -------------------------------------------------
    pool = Pool(identities)
    d = Draws(99)
    for lf in live_files:
        p = [tuple(e["pos"]) for e in targets[lf]]
        for _ in choose(p, 15, d, exact=True):
            pool.draw(d)
    spread_use = max(pool.used.values()) - min(pool.used.values())
    case("across a whole run at the maximum count, any two identities' use "
         "counts differ by at most one (max %d, min %d)"
         % (max(pool.used.values()), min(pool.used.values())), spread_use <= 1)
    case("...and every enabled identity was used at least once",
         min(pool.used.values()) >= 1)

    # --- spacing -----------------------------------------------------------
    #
    # Measured against the uniform draw's own mean for the same file and count,
    # not against a reproduction of the selection - the same reason boss_verify
    # does not predict placements.
    worse = []
    equal_at_one = True
    for lf in live_files:
        p = [tuple(e["pos"]) for e in targets[lf]]
        uni = []
        spr = []
        for seed in range(12):
            uni.append(min_pairwise(p, choose(p, 10, Draws(seed), exact=True)))
            spr.append(min_pairwise(p, choose(p, 10, Draws(seed), exact=True, spread=True)))
        if sum(spr) / len(spr) <= sum(uni) / len(uni):
            worse.append(lf)
        for seed in range(5):
            a = choose(p, 1, Draws(seed), exact=True)
            b = Draws(seed)
            c = choose(p, 1, b, exact=True, spread=True)
            d2 = Draws(seed)
            choose(p, 1, d2, exact=True)
            if a != c or b.n != d2.n:
                equal_at_one = False
    case("spacing beats the uniform draw's measured mean minimum distance in "
         "all fourteen areas at a count of 10", not worse, "worse in %s" % worse)
    case("at a count of 1 spacing produces the same placement and consumes the "
         "same number of draws", equal_at_one)

    # --- the seven unscaled identities ------------------------------------
    tracked, untracked = scaling_tracked(root, maps)
    case("25 of the 32 identities are tracked by the stat-scaling table, seven "
         "are not", len(tracked) == 25 and len(untracked) == 7,
         "%d tracked, %d untracked" % (len(tracked), len(untracked)))
    case("the seven are three Witch of Hemwick, three Living Failure and the "
         "Small Celestial Emissary",
         sorted(m for _n, _t, m in untracked) ==
         sorted(["c2100"] * 3 + ["c4030"] * 3 + ["c2500"]),
         str(sorted(untracked)))

    # --- verify's own failure paths ---------------------------------------
    #
    # Every case below corrupts a known-good tree in one specific way and
    # requires verify to report the criterion that covers it. Without these,
    # "verify passed" would only mean the tool ran.
    import copy
    default_cfg = parse_flags([])

    clean_fails, clean_inserted = compare_trees(root, maps, copy.deepcopy(maps),
                                                default_cfg)
    case("verify reports nothing on an unmodified tree",
         not clean_fails and not clean_inserted, str(clean_fails)[:200])

    # An honest simulated insertion: copy a real boss placement's own identity -
    # npc, think and model index, all three from the same map - onto an eligible
    # ordinary placement of that map. m22 holds Witch of Hemwick, which is in the
    # insertion list, so this is exactly what the pass would write.
    def inject(tree, mapfile, donor_name, target_name):
        m = tree[mapfile]
        donor = next(e for e in EL.enemies(m) if donor_name in e["name"])
        target = next(e for e in EL.enemies(m) if e["name"] == target_name)
        blob = m.parts.entries[target["idx"]]
        BV._set_enemy_npc(blob, donor["npc"])
        BV._set_i32(blob, BV._i64(blob, BV.PART_TYPE_DATA_OFFSET) +
                    BV.ENEMY_THINK_IN_TYPEDATA, donor["think"])
        BV._set_i32(blob, BV.PART_MODEL_INDEX, _i32_model_index(m, donor))
        return target["name"]

    def _i32_model_index(m, donor):
        for i, blob in enumerate(m.models.entries):
            if m.model_name(i) == donor["model"]:
                return i
        raise AssertionError("donor model missing")

    good = copy.deepcopy(maps)
    victim = targets["m22_00_00_00"][0]["name"]
    inject(good, "m22_00_00_00", "c2100_0000", victim)
    f, ins = compare_trees(root, maps, good, default_cfg)
    case("verify SEES a simulated insertion on an eligible placement, and "
         "objects to nothing", sum(len(v) for v in ins.values()) == 1 and not f,
         str(f)[:200])

    # A boss placement replaced -> criteria 13 and 15.
    bad = copy.deepcopy(maps)
    boss_target = next(e["name"] for e in EL.enemies(maps["m22_00_00_00"])
                       if BV.is_boss_name(e["name"]) and "c2100_0000" not in e["name"])
    inject(bad, "m22_00_00_00", "c2100_0000", boss_target)
    f, _ins = compare_trees(root, maps, bad, default_cfg)
    case("verify catches a boss placement being replaced (13 and 15)",
         13 in f and 15 in f, str(sorted(f))[:120])

    # The Hunter's Dream -> criterion 14.
    bad = copy.deepcopy(maps)
    dream = list(EL.enemies(maps["m21_00_00_00"]))
    donor21 = next(e for e in EL.enemies(maps["m22_00_00_00"])
                   if "c2100_0000" in e["name"])
    blob = bad["m21_00_00_00"].parts.entries[dream[0]["idx"]]
    BV._set_enemy_npc(blob, donor21["npc"])
    BV._set_i32(blob, BV._i64(blob, BV.PART_TYPE_DATA_OFFSET) +
                BV.ENEMY_THINK_IN_TYPEDATA, donor21["think"])
    m21models = bad["m21_00_00_00"]
    idx = next((i for i in range(len(m21models.models.entries))
                if m21models.model_name(i) == donor21["model"]), None)
    if idx is None:
        # The Hunter's Dream does not ship the Witch's model at all, which is
        # itself the reason no insertion can land there - nothing to inject.
        case("verify catches an insertion in the Hunter's Dream (14)", True)
    else:
        BV._set_i32(blob, BV.PART_MODEL_INDEX, idx)
        f, _ins = compare_trees(root, maps, bad, default_cfg)
        case("verify catches an insertion in the Hunter's Dream (14)", 14 in f,
             str(sorted(f))[:120])

    # A fabricated NPCParamID behind a real insertion model -> criterion 9.
    bad = copy.deepcopy(maps)
    inject(bad, "m22_00_00_00", "c2100_0000", victim)
    t = next(e for e in EL.enemies(bad["m22_00_00_00"]) if e["name"] == victim)
    BV._set_enemy_npc(bad["m22_00_00_00"].parts.entries[t["idx"]], 123456789)
    f, _ins = compare_trees(root, maps, bad, default_cfg)
    case("verify catches a fabricated NPCParamID (9)", 9 in f, str(sorted(f))[:120])

    # A moved placement -> criterion 17. One byte of the position is enough.
    bad = copy.deepcopy(maps)
    bad["m22_00_00_00"].parts.entries[targets["m22_00_00_00"][0]["idx"]][0x28] ^= 0xFF
    f, _ins = compare_trees(root, maps, bad, default_cfg)
    case("verify catches a placement that moved (17)", 17 in f, str(sorted(f))[:120])

    # EXACTLY 1 against a tree with no insertions at all -> criterion 3. This is
    # the case a pass that did nothing would otherwise satisfy.
    exact_cfg = parse_flags(["--exactly", "--count=1"])
    f, _ins = compare_trees(root, maps, copy.deepcopy(maps), exact_cfg)
    case("verify catches EXACTLY N not being met - the equality that a pass "
         "doing nothing would otherwise satisfy (3)", 3 in f, str(sorted(f))[:120])

    # The override against the same empty tree -> criterion 5.
    f, _ins = compare_trees(root, maps, copy.deepcopy(maps),
                            parse_flags(["--replace-all"]))
    case("verify catches the override leaving eligible targets unreplaced (5)",
         5 in f, str(sorted(f))[:120])

    failures = 0
    for name, ok in cases:
        print("  %-74s %s" % (name, "ok" if ok else "FAILED"))
        if not ok:
            failures += 1
    for line in detail:
        print(line)
    print("%d/%d passing" % (len(cases) - failures, len(cases)))
    return 1 if failures else 0


# ---------------------------------------------------------------------------
# verify
# ---------------------------------------------------------------------------

POS_OFF = 0x28
POS_END = 0x4C   # Position, Rotation and Scale: three Vector3 from 0x28


def part_transform_bytes(m, idx):
    return bytes(m.parts.entries[idx][POS_OFF:POS_END])


def compare_trees(vanilla, van, out, cfg):
    """(fails, inserted) - the whole of criteria 2 to 18 as property checks.

    Takes preloaded trees rather than two paths, so selftest can hand it a
    corrupted in-memory copy. A property validator that has only ever returned
    PASS proves nothing.
    """
    identities = BV.insertion_list(vanilla, van)
    models = set(BV.insertion_models(vanilla, van))
    ticked_models = set(cfg["bosses"]) if cfg["bosses"] else set(models)
    ticked_areas = set(cfg["areas"]) if cfg["areas"] else set(BV.INSERT_AREA_PREFIXES)
    targets = eligible_targets(vanilla, van, skipped=cfg["skipped"],
                              caged_dogs=cfg["caged_dogs"])
    all_triples = BV.all_vanilla_triples(vanilla)
    tracked, _untracked = scaling_tracked(vanilla, van)
    tracked_npcs = {n for n, _t, _m in tracked}

    fails = {}

    def fail(criterion, message):
        fails.setdefault(criterion, []).append(message)

    inserted = {}   # map file -> [(name, vanilla e, out identity)]
    for mn in EL.BASE_MAPS:
        vm, om = van.get(mn), out.get(mn)
        if vm is None or om is None:
            continue
        van_e = list(EL.enemies(vm))
        out_e = list(EL.enemies(om))
        if len(van_e) != len(out_e):
            fail(2, "%s: enemy count changed %d -> %d" % (mn, len(van_e), len(out_e)))
            continue
        eligible_names = {e["name"] for e in targets.get(mn, [])}
        for ve, oe in zip(van_e, out_e):
            # 17: the transform is never written, for ANY placement.
            if part_transform_bytes(vm, ve["idx"]) != part_transform_bytes(om, oe["idx"]):
                fail(17, "%s/%s: position, rotation or scale changed" % (mn, ve["name"]))
            if oe["model"] not in models:
                continue   # not this pass's work - see the module docstring
            if ve["model"] in models and (ve["npc"], ve["think"], ve["model"]) == \
                    (oe["npc"], oe["think"], oe["model"]):
                continue   # a vanilla boss placement, untouched
            inserted.setdefault(mn, []).append((ve, oe))

            # 2 / 15: only an eligible target, and only in a ticked area.
            if ve["name"] not in eligible_names:
                fail(15, "%s/%s: a protected or ineligible placement carries an "
                         "inserted boss" % (mn, ve["name"]))
            prefix = area_of(mn)
            if prefix is None:
                fail(14, "%s/%s: an inserted boss outside the fourteen areas"
                         % (mn, ve["name"]))
            elif prefix not in ticked_areas:
                fail(2, "%s/%s: an inserted boss in an unticked area (%s)"
                        % (mn, ve["name"], prefix))
            # 13: and never over a boss placement.
            if BV.is_boss_name(ve["name"]):
                fail(13, "%s/%s: a boss placement was replaced" % (mn, ve["name"]))

            # 9: a real identity from the list, and a ticked model.
            base = [(n, t, m) for n, t, m in identities
                    if m == oe["model"] and t == (ve["think"] if cfg["keep_ai"] else oe["think"])]
            scaled_ok = any(EL.zone_scaled_npc(mn, n) == oe["npc"] or n == oe["npc"]
                            for n, _t, _m in base)
            if not base or not scaled_ok:
                fail(9, "%s/%s: %d*%d*%s is not an insertion identity (scaled or not)"
                        % (mn, ve["name"], oe["npc"], oe["think"], oe["model"]))
            if oe["model"] not in ticked_models:
                fail(9, "%s/%s: model %s is not ticked" % (mn, ve["name"], oe["model"]))
            if (oe["npc"], oe["think"], oe["model"]) not in all_triples and \
                    oe["npc"] not in tracked_npcs:
                fail(9, "%s/%s: %d*%d*%s exists in no vanilla map and is not a "
                        "scaling variant" % (mn, ve["name"], oe["npc"], oe["think"],
                                             oe["model"]))

            # 11: the AI choice, as an exact equality in both states.
            if cfg["keep_ai"]:
                if oe["think"] != ve["think"]:
                    fail(11, "%s/%s: REPLACED ENEMY'S AI but think %d -> %d"
                             % (mn, ve["name"], ve["think"], oe["think"]))
            else:
                if oe["think"] == ve["think"] and base and \
                        all(t != ve["think"] for _n, t, _m in base):
                    fail(11, "%s/%s: OWN AI but think is still the vanilla %d"
                             % (mn, ve["name"], ve["think"]))

            # 16: a scaled area rewrites a tracked identity's NPCParamID.
            if any(n in tracked_npcs for n, _t, _m in base):
                want = {EL.zone_scaled_npc(mn, n) for n, _t, _m in base}
                if oe["npc"] not in want:
                    fail(16, "%s/%s: npc %d is not this area's variant of a "
                             "tracked identity %s" % (mn, ve["name"], oe["npc"],
                                                      sorted(want)))

    # --- the per-map counts, one criterion per configuration --------------
    n = cfg["count"]
    for mn in EL.BASE_MAPS:
        supply = len(targets.get(mn, []))
        got = len(inserted.get(mn, []))
        prefix = area_of(mn)
        participating = prefix is not None and prefix in ticked_areas and ticked_models
        if not participating:
            if got:
                fail(8, "%s: %d insertions in a map that does not participate" % (mn, got))
            continue
        if cfg["replace_all"]:
            if got != supply:
                fail(5, "%s: the override changed %d of %d eligible targets" % (mn, got, supply))
        elif not ticked_models:
            if got:
                fail(7, "%s: %d insertions with an empty INSERTED BOSSES" % (mn, got))
        elif n <= 0:
            if got:
                fail(6, "%s: %d insertions at count 0" % (mn, got))
        elif cfg["exact"]:
            if got != min(n, supply):
                fail(3, "%s: EXACTLY %d gave %d, expected %d" % (mn, n, got, min(n, supply)))
        else:
            if got > min(n, supply):
                fail(4, "%s: UP TO %d gave %d, over the ceiling of %d"
                        % (mn, n, got, min(n, supply)))
        # 18: a starved map reports rather than fails, so the only thing to
        # check in the tree is that it inserted what it could.
        if supply < n and not cfg["replace_all"] and got != supply and cfg["exact"]:
            fail(18, "%s: only %d eligible targets and %d inserted" % (mn, supply, got))

    # --- 4's second property: not every map at N -------------------------
    if not cfg["exact"] and not cfg["replace_all"] and n >= 5:
        live = [lf for _p, lf, _nm, _c in BV.insertion_areas(vanilla, van)]
        got = [len(inserted.get(lf, [])) for lf in live if area_of(lf) in ticked_areas]
        if got and all(g == n for g in got):
            fail(4, "every one of the %d participating areas is exactly at N=%d - "
                    "UP TO is behaving as EXACTLY" % (len(got), n))

    # --- 10: the pool property -------------------------------------------
    used = {}
    for mn, rows in inserted.items():
        for ve, oe in rows:
            base = [(nn, t, m) for nn, t, m in identities
                    if m == oe["model"] and
                    (EL.zone_scaled_npc(mn, nn) == oe["npc"] or nn == oe["npc"]) and
                    (t == oe["think"] or cfg["keep_ai"])]
            if len(base) == 1:
                used[base[0]] = used.get(base[0], 0) + 1
    if used and len(used) > 1:
        enabled = [i for i in identities if i[2] in ticked_models]
        counts = [used.get(i, 0) for i in enabled]
        if max(counts) - min(counts) > 1:
            fail(10, "use counts differ by %d (max %d, min %d) over %d "
                     "unambiguously attributed insertions"
                     % (max(counts) - min(counts), max(counts), min(counts),
                        sum(counts)))

    # --- 12: spacing ------------------------------------------------------
    if cfg["spread"]:
        for mn, rows in inserted.items():
            if len(rows) < 2:
                continue
            pos = [tuple(e["pos"]) for e in targets.get(mn, [])]
            chosen = [tuple(ve["pos"]) for ve, _oe in rows]
            got = min(dist2(a, b) for i, a in enumerate(chosen) for b in chosen[i + 1:]) ** 0.5
            uni = []
            for seed in range(12):
                uni.append(min_pairwise(pos, choose(pos, len(rows), Draws(seed), exact=True)))
            expected = sum(uni) / len(uni)
            if got < expected:
                fail(12, "%s: minimum pairwise distance %.0f is below the uniform "
                         "draw's measured mean %.0f" % (mn, got, expected))

    return fails, inserted


def cmd_verify(vanilla, output, cfg):
    van = EL.load_base_maps(vanilla)
    out = {mn: EL.load_map(output, mn) for mn in EL.BASE_MAPS}
    fails, inserted = compare_trees(vanilla, van, out, cfg)
    n = cfg["count"]
    ticked_areas = set(cfg["areas"]) if cfg["areas"] else set(BV.INSERT_AREA_PREFIXES)
    ticked_models = set(cfg["bosses"]) if cfg["bosses"] else set(
        BV.insertion_models(vanilla, van))

    total = sum(len(v) for v in inserted.values())
    print("configuration: count=%d %s%s%s%s, %d areas, %d bosses"
          % (n, "EXACTLY" if cfg["exact"] else "UP TO",
             ", REPLACE ALL" if cfg["replace_all"] else "",
             ", SPREAD" if cfg["spread"] else "",
             ", REPLACED ENEMY'S AI" if cfg["keep_ai"] else ", OWN AI",
             len(ticked_areas), len(ticked_models)))
    print("%d inserted bosses across %d map files\n" % (total, len(inserted)))

    criteria = [
        (2, "boss identities appear only at ordinary placements in ticked areas"),
        (3, "EXACTLY N is an equality on min(N, eligible targets)"),
        (4, "UP TO N never exceeds the ceiling, and not every map is at N"),
        (5, "REPLACE ALL ENEMIES replaces every eligible target"),
        (6, "count 0 changes nothing"),
        (7, "an empty INSERTED BOSSES changes nothing"),
        (8, "an unticked area is not visited"),
        (9, "every arrival is a real insertion identity with a ticked model"),
        (10, "any two enabled identities' use counts differ by at most one"),
        (11, "BOSS BEHAVIOUR is exact in whichever state was used"),
        (12, "SPREAD BOSSES OUT beats the uniform draw"),
        (13, "no boss placement was replaced"),
        (14, "no insertion outside the fourteen areas"),
        (15, "nothing protected was replaced"),
        (16, "a scaled area's arrival carries that area's variant"),
        (17, "position, rotation and scale are unchanged"),
        (18, "a starved map inserts what it can"),
    ]
    for num, text in criteria:
        msgs = fails.get(num, [])
        print("  %-3s %-66s %s" % (num, text, "PASS" if not msgs else "FAIL"))
        for m in msgs[:6]:
            print("        " + m)
        if len(msgs) > 6:
            print("        ... and %d more" % (len(msgs) - 6))
    if total == 0 and not fails:
        print("\nNOTE: nothing was inserted. That is criteria 6/7/8 passing, or the")
        print("      pass did not run - check the configuration flags above.")
    return 1 if fails else 0


def parse_flags(argv):
    cfg = dict(count=1, exact=False, replace_all=False, spread=False, keep_ai=False,
               areas=(), bosses=(), skipped=(), caged_dogs=False)
    for a in argv:
        if a.startswith("--count="):
            cfg["count"] = int(a.split("=", 1)[1])
        elif a == "--exactly":
            cfg["exact"] = True
        elif a == "--replace-all":
            cfg["replace_all"] = True
        elif a == "--spread":
            cfg["spread"] = True
        elif a == "--keep-ai":
            cfg["keep_ai"] = True
        elif a == "--caged-dogs":
            cfg["caged_dogs"] = True
        elif a.startswith("--areas="):
            cfg["areas"] = tuple(x for x in a.split("=", 1)[1].split(",") if x)
        elif a.startswith("--bosses="):
            cfg["bosses"] = tuple(x for x in a.split("=", 1)[1].split(",") if x)
        elif a.startswith("--skipped="):
            cfg["skipped"] = tuple(x for x in a.split("=", 1)[1].split(",") if x)
        else:
            sys.exit("unknown flag %s" % a)
    return cfg


def main():
    if len(sys.argv) >= 3 and sys.argv[1] == "selftest":
        return cmd_selftest(sys.argv[2])
    if len(sys.argv) >= 4 and sys.argv[1] == "verify":
        return cmd_verify(sys.argv[2], sys.argv[3], parse_flags(sys.argv[4:]))
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main())
