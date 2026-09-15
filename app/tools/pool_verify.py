#!/usr/bin/env python3
"""Validating the enemy and boss pickers: baked tables, and a run's output.

Two failures this is built to catch, both silent:

  table   EnemyPoolTable.h must BE the engine's pool. If an exclusion rule
          changes and the table is not regenerated, every saved selection
          silently remaps onto the wrong creatures - the config string is
          positional, so a shifted table is a shifted meaning. Nothing else
          in the system would notice.

  filter  The feature's actual contract: no placement anywhere was changed
          to a model the user disabled. Checkable from the output tree alone,
          which is the only thing that matters once a run has happened.

Usage:
    python pool_verify.py table    enemy|boss|both <vanilla_dvdroot>
    python pool_verify.py filter   <vanilla_dvdroot> <output_dvdroot> <c2630,c4040,...>
    python pool_verify.py selftest <vanilla_dvdroot>
"""

import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from enemy_lookup import (BASE_MAPS, load_map, enemies, exclusion_reason,  # noqa: E402
                          engine_pool, engine_pool_models)
from boss_verify import boss_pool_models, build_pool, load_maps  # noqa: E402
from names import model_name  # noqa: E402

# Each picker's baked table, and the function that recomputes it from the
# vanilla tree. Keeping them side by side is the point: the check is that
# the two agree.
TABLES = {
    "enemy": ("EnemyPoolTable.h", "kEnemyPoolModelCount", engine_pool_models),
    "boss":  ("BossPoolTable.h",  "kBossPoolModelCount",  boss_pool_models),
}

SRC = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "src", "Randomizer")


def baked_table(kind="enemy"):
    """The (model, displayName, weight) rows of a table, in file order."""
    text = open(os.path.join(SRC, TABLES[kind][0]), encoding="utf-8").read()
    body = text.split("kTable = {{", 1)[1].split("}};", 1)[0]
    return re.findall(r'\{\s*"([^"]+)",\s*"([^"]*)",\s*(-?\d+)\s*\}', body)


def cmd_table(kind, root):
    header, count_const, models_fn = TABLES[kind]
    rows = baked_table(kind)
    want = models_fn(root)
    failures = []

    if len(rows) != len(want):
        failures.append("table has %d rows, engine pool has %d models" % (len(rows), len(want)))
    else:
        for i, ((mdl, name, _w), expect) in enumerate(zip(rows, want)):
            if mdl != expect:
                failures.append("row %d is %s, engine pool has %s (ORDER MISMATCH - "
                                "every saved selection is now wrong)" % (i, mdl, expect))

    # The count is also baked as a constant the C++ uses to size an array.
    text = open(os.path.join(SRC, header), encoding="utf-8").read()
    m = re.search(count_const + r"\s*=\s*(\d+)", text)
    if not m:
        failures.append("%s not found" % count_const)
    elif int(m.group(1)) != len(rows):
        failures.append("%s is %s but the table has %d rows"
                        % (count_const, m.group(1), len(rows)))

    for mdl, name, _w in rows:
        if not name:
            failures.append("%s has an empty display name" % mdl)

    if failures:
        print("FAIL")
        for f in failures:
            print("  " + f)
        print("\nRegenerate with: python gen_pool_table.py %s <vanilla_dvdroot>" % kind)
        return 1

    print("PASS: %s matches the %s pool - %d models" % (header, kind, len(rows)))
    return 0


def cmd_filter(vanilla, output, included_csv):
    included = {m.strip() for m in included_csv.split(",") if m.strip()}
    unknown = included - {m for m, _n, _w in baked_table()}
    if unknown:
        print("FAIL: not models in the table: %s" % sorted(unknown))
        return 1

    # A placement counts as "changed" when its (npc, think, model) differs from
    # the vanilla placement at the same index - the same comparison
    # enemy_lookup.py diff makes.
    violations, changed, checked = [], 0, 0
    for mapname in BASE_MAPS:
        van = load_map(vanilla, mapname)
        out = load_map(output, mapname)
        if van is None or out is None:
            continue
        v = list(enemies(van))
        o = list(enemies(out))
        if len(v) != len(o):
            print("FAIL: %s has %d placements in vanilla, %d in output"
                  % (mapname, len(v), len(o)))
            return 1
        for a, b in zip(v, o):
            checked += 1
            if (a["npc"], a["think"], a["model"]) == (b["npc"], b["think"], b["model"]):
                continue
            changed += 1
            if b["model"] not in included:
                violations.append("%s %s -> %s (%s) which is NOT in the selection"
                                  % (mapname, a["name"], b["model"],
                                     model_name(b["model"]) or "?"))

    if violations:
        print("FAIL: %d placement(s) used an excluded model" % len(violations))
        for v in violations[:20]:
            print("  " + v)
        if len(violations) > 20:
            print("  ... and %d more" % (len(violations) - 20))
        return 1

    print("PASS: %d placements checked, %d changed, all within the %d selected model(s)"
          % (checked, changed, len(included)))
    return 0


def cmd_selftest(root):
    """Pins the properties that matter, including in the failing direction."""
    cases = []

    # Same checks over both tables - the pickers are one implementation, so an
    # invariant that holds for one and not the other is a bug either way.
    for kind in ("enemy", "boss"):
        rows = baked_table(kind)
        models = TABLES[kind][2](root)
        cases.append(("%s: baked table matches the engine pool" % kind,
                      [m for m, _n, _w in rows] == models))
        cases.append(("%s: order is stable across two builds" % kind,
                      TABLES[kind][2](root) == models))
        cases.append(("%s: every model has a display name" % kind,
                      all(nm for _m, nm, _w in rows)))
        cases.append(("%s: no duplicate model ids" % kind,
                      len({m for m, _n, _w in rows}) == len(rows)))
        cases.append(("%s: every weight is at least 1" % kind,
                      all(int(w) >= 1 for _m, _n, w in rows)))

    rows = baked_table("enemy")
    pool = engine_pool(root)
    cases.append(("enemy: weights sum to the pool size",
                  sum(int(w) for _m, _n, w in rows) == len(pool)))
    # The two pools share two MODELS but no IDENTITIES. Blood Starved Beast and
    # Father Gascoigne each exist twice in the game with different NpcParam and
    # ThinkParam - a boss version and a non-boss one - and the reference's
    # boss-name list separates them by placement suffix (c2710_0000 is the
    # boss, c2710_0001 the NPC). So the pickers control genuinely different
    # creatures that happen to share a model.
    #
    # Measured, not assumed: an early version of this file asserted the model
    # lists were disjoint and failed on first run.
    overlap = ({m for m, _n, _w in baked_table("boss")} &
               {m for m, _n, _w in rows})
    cases.append(("the two pools share exactly the models c2090 and c2710",
                  overlap == {"c2090", "c2710"}))
    enemy_ids = set(engine_pool(root))
    boss_ids = set(build_pool(load_maps(root))[0])
    cases.append(("...but share no identity - each side has its own variant",
                  not (enemy_ids & boss_ids)))

    # The three engine rules cmd_pool used to get wrong (plan section 2.2).
    cases.append(("c2561 is excluded from the pool",
                  not any(m == "c2561" for _n, _t, m in pool)))
    names_seen = set()
    for mapname in BASE_MAPS:
        mm = load_map(root, mapname)
        if mm:
            for e in enemies(mm):
                names_seen.add(e["name"])
    cases.append(("c2561 placements DO exist, so the exclusion is real",
                  any("c2561" in n for n in names_seen)))

    # A wrong-length config string must be rejected rather than misapplied -
    # this is the only guard against a regenerated table.
    for kind in ("enemy", "boss"):
        header, const, _fn = TABLES[kind]
        cases.append(("%s: table size matches the C++ constant" % kind,
                      ("%s = %d" % (const, len(baked_table(kind)))) in
                      open(os.path.join(SRC, header), encoding="utf-8").read()))

    # Duplicate display names exist, which is exactly why rows show the id too.
    dupes = len(rows) - len({n for _m, n, _w in rows})
    cases.append(("duplicate names exist, so rows must show the model id", dupes > 0))

    # --- EnemyPoolSelection.h semantics, MIRRORED --------------------------
    # There is no host C++ toolchain here (clang exists but ships no standard
    # library), so this re-implements Encode/Decode/IsModelEnabled in Python
    # and asserts the properties instead - the same substitute
    # ui_scroll_verify.py uses for the C++ scroll arithmetic. It catches the
    # rules drifting; it cannot catch the C++ failing to implement them.
    n = len(rows)

    def encode(enabled):
        return "".join("1" if e else "0" for e in enabled)

    def decode(text, current):
        if len(text) != n:          # the only guard against a regenerated table
            return None
        return [c != "0" for c in text]

    all_on = [True] * n
    cases.append(("mirror: default all-on encodes to all ones",
                  encode(all_on) == "1" * n))
    cases.append(("mirror: round trip is lossless",
                  decode(encode([i % 3 == 0 for i in range(n)]), all_on)
                  == [i % 3 == 0 for i in range(n)]))
    cases.append(("mirror: short string rejected", decode("0101", all_on) is None))
    cases.append(("mirror: long string rejected", decode("0" * (n + 1), all_on) is None))
    cases.append(("mirror: empty string rejected", decode("", all_on) is None))
    cases.append(("mirror: encoded length equals the table size",
                  len(encode(all_on)) == n))

    # The config line has to fit RandomizerDefaultsStore's buffer.
    store = open(os.path.join(SRC, "RandomizerDefaultsStore.cpp"), encoding="utf-8").read()
    n = len(baked_table("enemy")) + len(baked_table("boss"))
    m2 = re.findall(r"char buf\[(\d+)\]", store)
    cases.append(("both *_included lines fit the config buffer",
                  bool(m2) and len("enemies_included=bosses_included=") + n + 2
                  < max(int(x) for x in m2)))

    failures = 0
    for name, ok in cases:
        print("  %-52s %s" % (name, "ok" if ok else "FAILED"))
        if not ok:
            failures += 1
    print("%d/%d passing" % (len(cases) - failures, len(cases)))
    return 1 if failures else 0


def main():
    a = sys.argv[1:]
    if not a:
        print(__doc__)
        return 2
    if a[0] == "table" and len(a) == 3:
        if a[1] == "both":
            return cmd_table("enemy", a[2]) | cmd_table("boss", a[2])
        if a[1] in TABLES:
            return cmd_table(a[1], a[2])
        print(__doc__)
        return 2
    if a[0] == "filter" and len(a) == 4:
        return cmd_filter(a[1], a[2], a[3])
    if a[0] == "selftest" and len(a) == 2:
        return cmd_selftest(a[1])
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main())
