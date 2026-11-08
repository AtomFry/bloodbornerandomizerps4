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
    python pool_verify.py table    enemy|boss|skip|both <vanilla_dvdroot>
    python pool_verify.py filter   <vanilla_dvdroot> <output_dvdroot> <c2630,c4040,...>
    python pool_verify.py skipped  <vanilla_dvdroot> <output_dvdroot> <c1050,c1051> [--starved]
    python pool_verify.py selftest <vanilla_dvdroot>
"""

import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from enemy_lookup import (BASE_MAPS, load_map, load_base_maps, enemies,  # noqa: E402
                          exclusion_reason, engine_pool, engine_pool_models,
                          is_m28_forced, randomizes, zone_scaled_npc,
                          overwritable_models, overwritable_counts,
                          overwritable_placements, SKIP_MAIDENS, M28_FORCED)
from boss_verify import boss_pool_models, build_pool, load_maps  # noqa: E402
from gen_pool_table import RENDERABLE  # noqa: E402
from enemy_lookup import _parse_int_array, _scaling_maps  # noqa: E402
from names import model_name  # noqa: E402

# Each picker's baked table, and the function that recomputes it from the
# vanilla tree. Keeping them side by side is the point: the check is that
# the two agree.
TABLES = {
    "enemy": ("EnemyPoolTable.h", "kEnemyPoolModelCount", engine_pool_models),
    "boss":  ("BossPoolTable.h",  "kBossPoolModelCount",  boss_pool_models),
    # The skip list is the other side of the pool - what can be REPLACED
    # rather than what can REPLACE - so it has its own recompute function and
    # its own frozen order. Same check, same failure mode: a regenerated
    # order silently remaps every saved selection.
    "skip":  ("EnemySkipTable.h", "kEnemySkipModelCount", overwritable_models),
}

SRC = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "src", "Randomizer")
UI_SRC = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "src", "UI")

# Font8x8.cpp draws an unrenderable character as a full-width BLANK column
# rather than as nothing, so a message can be well inside every width budget
# and still show the player an empty line. 71 characters fit a line at scale 3
# (kAdvance 9 x scale 3 = 27px into 1920); the failure prefix takes 29 of them.
LINE_CHARS = 71
FAIL_PREFIX_CHARS = 29
FAIL_MESSAGE_CHARS = LINE_CHARS - FAIL_PREFIX_CHARS  # 42


def named_strings(path):
    """Every `const char* const kName = "...";` in a C++ source file.

    Parsing the strings out of the source is the point: retyping them into
    the test would pin the test's copy and let the shipped one drift, which
    is exactly the failure mode - an unrenderable message looks fine in a
    diff and blank on the TV."""
    text = open(path, encoding="utf-8").read()
    return dict(re.findall(
        r'const char\* const (\w+)\s*=\s*"((?:[^"\\]|\\.)*)"\s*;', text))


def renderable(s):
    """Mirror of Font8x8.cpp's FindGlyph: every character has a glyph."""
    return all(c in RENDERABLE for c in s)

# Parsed directly, following starting_weapons_verify.py's precedent. CLAUDE.md
# section 1 fixes this path in place for exactly this reason.
REF_RANDOMIZE_CS = os.path.join(
    os.path.dirname(os.path.abspath(__file__)), "..", "..",
    "reference", "Randomizer", "MainWindowComponents", "RandomizeFunctions.cs")


def all_placements(root):
    """Every enemy placement in the 24 base maps, as (mapname, enemy dict)."""
    out = []
    for mapname in BASE_MAPS:
        m = load_map(root, mapname)
        if m is None:
            continue
        for e in enemies(m):
            out.append((mapname, e))
    return out


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


def _frozen(mapname, a, b):
    """Is this placement untouched by the enemy pass?

    NOT plain equality with vanilla, and that distinction is the whole point.
    The stat pass (BossParamScaling) runs AFTER the enemy pass and rewrites
    NPCParamID on 1,369 of the 2,269 overwritable placements - a frozen one
    included. Demanding equality would fail on a correct implementation,
    which is exactly the bug the retired `maidens` command shipped with: of
    the 42 maidens it expected to be byte-identical, 26 sit in stat-scaled
    maps, so it would have reported 16 and failed.

    Frozen therefore means: same model, same think, and an npc that is either
    unchanged or the zone-tuned variant of the vanilla value for that map."""
    if b["model"] != a["model"] or b["think"] != a["think"]:
        return False
    return b["npc"] in (a["npc"], zone_scaled_npc(mapname, a["npc"]))


def cmd_skipped(vanilla, output, models_csv, starved=False):
    """The ENEMIES SKIPPED contract, checked from an output tree.

    Supersedes `maidens`, which checked UNCHANGED BELL MAIDENS - a setting
    feature 032 D1 deleted. This is the same contract expressed against the
    general list, with the frozen definition corrected (see _frozen).
    """
    skipped = {m.strip() for m in models_csv.split(",") if m.strip()}
    unknown = skipped - {m for m, _n, _w in baked_table("skip")}
    if unknown:
        print("FAIL: not models in EnemySkipTable.h: %s" % sorted(unknown))
        return 1
    maidens_ticked = bool(skipped & set(SKIP_MAIDENS))

    moved, not_frozen, became, eligible, changed_eligible = 0, [], [], 0, 0
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
            reference_excluded = bool(exclusion_reason(a["name"]))
            is_skipped_model = any(p in a["name"] for p in skipped)

            # --- assertion 1: a skipped creature's own placements are frozen
            if is_skipped_model and not reference_excluded:
                if is_m28_forced(mapname, a["name"]):
                    # The reference's Yahar'gul override is unconditional and
                    # outranks the skip list, so these twelve are ALLOWED to
                    # move. With the maidens ticked the fallback pool may also
                    # redraw them AS maidens, so nothing is asserted about them
                    # here - but they are still eligible placements and still
                    # count toward the rate below, which is why this exempts
                    # them from assertion 1 only rather than skipping the
                    # placement outright.
                    moved += 1
                elif not _frozen(mapname, a, b):
                    not_frozen.append("%s %s: %s/%s/%s -> %s/%s/%s"
                                      % (mapname, a["name"], a["model"], a["think"],
                                         a["npc"], b["model"], b["think"], b["npc"]))

            # --- assertion 2: no ELIGIBLE placement CHANGED to a skipped model
            #
            # Stated as a diff over the 2,269 overwritable placements, not as
            # "no placement anywhere uses a skipped model". The literal form
            # is FALSE on a correct implementation: six of the 85 rows also
            # own reference-excluded placements that stay put when ticked
            # (c1060, c2090, c2100, c2120, c2500, c2710). A check that fails
            # on the happy path teaches its reader to ignore it.
            #
            # Boss-written placements are out of scope: the boss pass writes
            # only into arenas the fixed list already protects, and c2090 /
            # c2710 can still arrive there from the boss pool, which this list
            # does not filter. See plan 032 section 5.10.
            if not reference_excluded:
                eligible += 1
                if (a["npc"], a["think"], a["model"]) != (b["npc"], b["think"], b["model"]):
                    changed_eligible += 1
                if b["model"] in skipped and a["model"] not in skipped:
                    became.append("%s %s -> %s" % (mapname, a["name"], b["model"]))

    rate = (100.0 * changed_eligible / eligible) if eligible else 0.0
    print("skipped models        : %s" % ", ".join(sorted(skipped)))
    print("eligible placements   : %d, of which %d changed (%.1f%%)"
          % (eligible, changed_eligible, rate))
    print("forced m28 placements : %d skipped-model placements exempted" % moved)
    if maidens_ticked:
        print("                        (the chime maiden rows are ticked, so the")
        print("                         reference's unconditional Yahar'gul override")
        print("                         applies and those twelve may legitimately move)")

    # assertion 3 is a RATE, reported not asserted: skipping removes
    # placements from the roll stream, so a skipped run and an unskipped run
    # are different worlds and only the proportion is comparable. Compare this
    # figure against the same run with nothing ticked. See plan section 5.7.
    print("change rate           : %.1f%% - compare against the unskipped run,"
          " not against a fixed number" % rate)

    failures = []
    if not_frozen:
        failures.append("%d placement(s) of a skipped model were not frozen" % len(not_frozen))
        for x in not_frozen[:20]:
            print("  NOT FROZEN: " + x)
    if became and not starved:
        failures.append("%d eligible placement(s) became a skipped model" % len(became))
        for x in became[:20]:
            print("  BECAME SKIPPED: " + x)
    if became and starved:
        print("--starved: %d eligible placements became a skipped model, which is"
              " EXPECTED here" % len(became))
        print("           (spec 032 D4: everything selected was also skipped, so the")
        print("            pool half yielded and the skipped models were drawn from)")

    if failures:
        print("FAIL: " + "; ".join(failures))
        return 1
    print("PASS: every skipped creature stayed put%s"
          % ("" if starved else ", and none arrived anywhere new"))
    return 0


def cmd_selftest(root):
    """Pins the properties that matter, including in the failing direction."""
    cases = []

    # Same checks over both tables - the pickers are one implementation, so an
    # invariant that holds for one and not the other is a bug either way.
    for kind in ("enemy", "boss", "skip"):
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
    for kind in ("enemy", "boss", "skip"):
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

    # --- ENEMIES SKIPPED (feature 032, retargeted from feature 016) --------
    #
    # These were the UNCHANGED BELL MAIDENS cases. D1 deleted that setting;
    # ticking its two live rows here reproduces it, so the measurements are
    # carried over unchanged - 317 entries, 80 models, 16 lost weights, 54
    # maiden placements - now expressed against the user's list instead of a
    # checkbox. If any of them moves, the two mechanisms are not equivalent.
    #
    # Two separable halves, and these cases keep them separable:
    #   the SKIP, which the player controls, and
    #   the m28 OVERRIDE, which is unconditional and outranks it.
    MAIDENS = SKIP_MAIDENS
    pool_on = engine_pool(root, skipped=MAIDENS)
    models_off = engine_pool_models(root)
    models_on = engine_pool_models(root, skipped=MAIDENS)
    placements = all_placements(root)

    cases.append(("032: nothing-skipped pool is unchanged at 333 entries, 82 models",
                  len(pool) == 333 and len(models_off) == 82))
    cases.append(("032: maidens-skipped pool is 317 entries, 80 models",
                  len(pool_on) == 317 and len(models_on) == 80))
    cases.append(("032: the two models lost are exactly c1050 and c1051",
                  set(models_off) - set(models_on) == {"c1050", "c1051"}))
    lost_weight = sum(int(w) for m, _n, w in rows if m in ("c1050", "c1051"))
    cases.append(("032: 333 - 317 == 16 == the baked weights of those rows",
                  len(pool) - len(pool_on) == 16 == lost_weight))
    # Plan section 5.3: regenerating the table against the flag-on pool would
    # silently remap all 82 saved selection characters. Asserted, not assumed.
    cases.append(("032: EnemyPoolTable.h is still 82 rows and is NOT the skipped pool",
                  len(rows) == 82 and [m for m, _n, _w in rows] != models_on))

    maidens = [(mn, e) for mn, e in placements
               if any(p in e["name"] for p in MAIDENS)]
    cases.append(("032: all 54 maiden placements match with the two rows ticked",
                  len(maidens) == 54 and
                  all(exclusion_reason(e["name"], MAIDENS) for _mn, e in maidens)))
    cases.append(("032: none of them match with nothing ticked",
                  not any(exclusion_reason(e["name"]) for _mn, e in maidens)))
    # The pattern is gone with D1; the fact it recorded is not. If a future
    # data set ever contains c1055, that should be noticed rather than
    # silently changing what the two maiden rows cover.
    cases.append(("032: c1055 still matches zero placements - it was always inert",
                  not any("c1055" in e["name"] for _mn, e in placements)))
    cases.append(("032: ...and BellMaidenExclusionList is gone from the header",
                  "BellMaidenExclusionList" not in
                  open(os.path.join(SRC, "EnemyExclusionList.h"), encoding="utf-8").read()))

    forced = [(mn, e) for mn, e in placements if is_m28_forced(mn, e["name"])]
    cases.append(("032: the six forced names are 12 placements, all in m28",
                  len(M28_FORCED) == 6 and len(forced) == 12 and
                  all("m28" in mn for mn, _e in forced)))
    cases.append(("032: all 12 forced placements are NPCParamID 105810",
                  all(e["npc"] == 105810 for _mn, e in forced)))
    cases.append(("032: no forced name appears outside m28",
                  not any(any(p in e["name"] for p in M28_FORCED)
                          for mn, e in placements if "m28" not in mn)))
    cases.append(("032: all six are skip-matched, so the override does real work",
                  all(exclusion_reason(e["name"], MAIDENS) for _mn, e in forced)))

    ref_cs = open(REF_RANDOMIZE_CS, encoding="utf-8", errors="replace").read()
    cases.append(("032: the six names appear verbatim in the reference source",
                  all(('"%s"' % p) in ref_cs for p in M28_FORCED)))

    # The placement decision itself (review finding 2.3). Nothing above models
    # the four lines in StepWriteMap that decide whether a placement is
    # retargeted; these do, in both directions, at every possible roll.
    fmap, fname = forced[0][0], forced[0][1]["name"]
    cases.append(("032: a forced placement randomizes at EVERY roll when skipped",
                  all(randomizes(fmap, fname, r, 100, skipped=MAIDENS) for r in range(101))))
    cases.append(("032: ...and at every roll when not skipped too",
                  all(randomizes(fmap, fname, r, 100) for r in range(101))))
    # roll 100 vs zoneChance 100 is the 1-in-101 skip the override now bypasses.
    other = next(e["name"] for mn, e in maidens
                 if "m28" in mn and not is_m28_forced(mn, e["name"]))
    cases.append(("032: a non-forced m28 maiden never randomizes when skipped",
                  not any(randomizes("m28_00_00_00", other, r, 100, skipped=MAIDENS)
                          for r in range(101))))
    cases.append(("032: ...but does when not skipped, except roll 100",
                  [r for r in range(101)
                   if not randomizes("m28_00_00_00", other, r, 100)] == [100]))
    cases.append(("032: the override is map-scoped - forced names are inert outside m28",
                  not is_m28_forced("m24_01_00_00", fname)))

    # --- Feature 032 milestone 1: the pool decision ------------------------
    #
    # StepBuildPool used to have one empty-pool message and one outcome. It
    # now has three causes and two outcomes, and these pin the measurements
    # the separation rests on. The starved shape is reachable TODAY through
    # UNCHANGED BELL MAIDENS, which is why milestone 1 was independently
    # testable; milestone 2 deleted that flag and these now use the list.
    loaded = load_base_maps(root)
    maiden_sel = set(SKIP_MAIDENS)

    starved = engine_pool(root, skipped=maiden_sel, included=maiden_sel, maps=loaded)
    cases.append(("032: maidens selected AND skipped starves the pool",
                  starved == []))
    fallback = engine_pool(root, included=maiden_sel, maps=loaded)
    cases.append(("032: ...and the fallback pool for it is 16 entries",
                  len(fallback) == 16 == lost_weight))

    # Why a starved pool has exactly one cause: ticking a model always
    # removes contributions, because every drawable model owns placements.
    own = {e["model"] for _mn, e in placements
           if e["model"] and not exclusion_reason(e["name"])}
    baked_models = {m for m, _n, _w in rows}
    cases.append(("032: every baked pool model owns overwritable placements",
                  baked_models <= own))

    # ...and the fallback can only be emptied by an unreadable tree, never by
    # a selection. Sampled over every single-row selection, which is the
    # narrowest one the picker can commit.
    cases.append(("032: no single-model selection can empty the fallback pool",
                  all(engine_pool(root, included={m}, maps=loaded)
                      for m in sorted(baked_models))))

    # Every string this milestone puts on screen, parsed out of the source.
    # The character assertion is the one that matters: a lowercase message
    # passes any length budget and draws as blank columns.
    eng_strings = named_strings(os.path.join(SRC, "EnemyRandomizer.cpp"))
    ui_strings = named_strings(os.path.join(UI_SRC, "WorldEditorScreen.cpp"))
    fail_msgs = {k: v for k, v in eng_strings.items() if k.startswith("kFail")}
    prefix = ui_strings.get("kEnemyFailPrefix", "")
    ui_lines = [ui_strings.get(k, None) for k in
                ("kPoolFellBackLine1", "kPoolFellBackLine2",
                 "kNothingRandomizedLine")]

    cases.append(("032: all six new display strings were found in the source",
                  len(fail_msgs) == 2 and bool(prefix) and
                  all(v is not None for v in ui_lines)))
    cases.append(("032: every new display string is renderable by Font8x8",
                  all(renderable(s) for s in
                      list(fail_msgs.values()) + [prefix] + ui_lines)))
    cases.append(("032: each failure message fits the 42 chars the prefix leaves",
                  len(prefix) == FAIL_PREFIX_CHARS and
                  all(len(m) <= FAIL_MESSAGE_CHARS for m in fail_msgs.values()) and
                  all(len(prefix) + len(m) <= LINE_CHARS
                      for m in fail_msgs.values())))
    cases.append(("032: each new progress line fits the 71-character line",
                  all(len(s) <= LINE_CHARS for s in ui_lines)))
    # The predicate must be able to fail, or the four cases above are vacuous.
    cases.append(("032: ...and the renderability check rejects lowercase and ':'",
                  not renderable("enemy pool is empty") and
                  not renderable("ENEMY RANDOMIZATION FAILED: ")))
    # A starved selection and a broken installation must not read alike.
    cases.append(("032: the two failure messages are different strings",
                  len(set(fail_msgs.values())) == 2))

    # --- 040: every WHOLE-LITERAL progress line, against the same two budgets --
    #
    # The four constants above are the ones the worlds feature froze as named
    # strings. Most progress lines are not: they are built inline, often from a
    # count, and the ones that are a single literal end to end can be measured
    # exactly. Those are checked here, which is what brings feature 040's two new
    # lines - STARTING WITH 20 BLOOD VIALS AND 20 BULLETS, and one WEARING <NAME>
    # per granted slot - under the same 71-character and renderability budgets the
    # rest of the log is held to.
    #
    # A lowercase line passes any length budget and draws as blank columns on the
    # fallback path, which is why the character check is the one that matters.
    editor_src = open(os.path.join(UI_SRC, "WorldEditorScreen.cpp"),
                      encoding="utf-8").read()
    whole_literals = re.findall(r'AddProgressLine\(\s*"((?:[^"\\]|\\.)*)"\s*\)',
                                editor_src)
    cases.append(("040: the progress log has whole-literal lines to measure",
                  len(whole_literals) >= 5))
    cases.append(("040: every whole-literal progress line is renderable and fits "
                  "the 71-character line",
                  all(renderable(s) and len(s) <= LINE_CHARS
                      for s in whole_literals)))
    # Feature 040's other new line is BUILT, not a whole literal: one
    # "WEARING <NAME>" per granted slot, where the name comes from the engine. Its
    # worst case is measurable even so, because the names are in a generated
    # header - so the widest of the 124 is the line to check, against the same 71
    # characters.
    #
    # Deliberately NOT generalised to every AddProgressLine fragment in the file.
    # One shipped fragment - "STARTING CHOICES: " on the confirm block - carries a
    # colon, which the FALLBACK glyph table does not hold; that predates this
    # feature, is out of its scope, and a sweeping case here would read as feature
    # 040 having broken it.
    armour_src = open(os.path.join(SRC, "ArmourTables.h"), encoding="utf-8").read()
    armour_names = re.findall(r'\{\s*"\d+",\s*"([^"]*)",\s*\d+\s*\}', armour_src)
    wearing = [f for f in
               re.findall(r'"((?:[^"\\]|\\.)*)"', editor_src) if "WEARING" in f]
    cases.append(("040: the widest WEARING line fits the 71-character line",
                  len(armour_names) == 124 and bool(wearing) and
                  all(renderable(f) for f in wearing) and
                  all(renderable(n) for n in armour_names) and
                  max(len(f) for f in wearing) +
                  max(len(n) for n in armour_names) <= LINE_CHARS))

    # --- Feature 032 milestone 2: the table, the polarity, the two halves --
    skip_rows = baked_table("skip")
    skip_models = [m for m, _n, _w in skip_rows]
    counts = overwritable_counts(root, loaded)

    cases.append(("032: the skip table is 85 rows and matches kEnemySkipModelCount",
                  len(skip_rows) == 85))
    cases.append(("032: its weights sum to 2269 overwritable placements",
                  sum(int(w) for _m, _n, w in skip_rows) == 2269 == sum(counts.values())))
    cases.append(("032: the 85 are a strict superset of the 82 pool models",
                  set(baked_models) < set(skip_models)))
    cases.append(("032: the three extras are exactly c1130, c2121, c2561",
                  set(skip_models) - set(baked_models) == {"c1130", "c2121", "c2561"}))
    cases.append(("032: every skip row has a renderable display name",
                  all(nm and renderable(nm) for _m, nm, _w in skip_rows)))
    # D3 and D6 are opposite decisions and must not drift into each other:
    # c2561 gets NO invented name, c1130 gets a deliberate override.
    cases.append(("032: c2561 falls back to C2561 - no invented name (D3)",
                  dict((m, n) for m, n, _w in skip_rows)["c2561"] == "C2561"))
    skip_src = open(os.path.join(SRC, "EnemySkipTable.h"), encoding="utf-8").read()
    cases.append(("032: c1130 reads OEDON CHAPEL DWELLER, marked as ours (D6)",
                  dict((m, n) for m, n, _w in skip_rows)["c1130"] == "OEDON CHAPEL DWELLER"
                  and "name is OURS" in
                  [ln for ln in skip_src.split("\n") if '"c1130"' in ln][0]))
    cases.append(("032: ...and c1130 is in NEITHER shipped pool table",
                  "c1130" not in {m for m, _n, _w in baked_table("enemy")} and
                  "c1130" not in {m for m, _n, _w in baked_table("boss")}))

    # --- the polarity, mirrored in BOTH directions -------------------------
    # ModelPoolSelection<N, DefaultSelected>. The skip type's three fail-safes
    # are the mirror image of the pickers', and a call site that inverted one
    # of them would look perfectly reasonable - hence both directions here.
    def sel(n, default):
        return [default] * n

    def decode(text, n, default):
        if len(text) != n:
            return sel(n, default)      # untouched -> the constructed state
        return [c != "0" for c in text]

    cases.append(("032: a fresh skip selection is 85 zeros, a pool one 82 ones",
                  sel(85, False) == [False] * 85 and sel(82, True) == [True] * 82))
    cases.append(("032: wrong-length, empty and absent skip values skip NOTHING",
                  not any(decode("0101", 85, False)) and
                  not any(decode("", 85, False)) and
                  not any(sel(85, False))))
    cases.append(("032: ...while the same three leave the pool ALL enabled",
                  all(decode("0101", 82, True)) and all(decode("", 82, True)) and
                  all(sel(82, True))))
    poolsel_src = open(os.path.join(SRC, "ModelPoolSelection.h"), encoding="utf-8").read()
    cases.append(("032: the unknown-model fallback is the polarity, not a literal",
                  "return DefaultSelected;" in poolsel_src and
                  "template <int N, bool DefaultSelected = true>" in poolsel_src))
    cases.append(("032: EnemySkipSelection is declared with DefaultSelected=false",
                  "ModelPoolSelection<kEnemySkipModelCount, false>" in
                  open(os.path.join(SRC, "EnemyPoolSelection.h"), encoding="utf-8").read()))

    # --- the zone-scaling mirror (the corrected `frozen` definition) -------
    tracked = _parse_int_array("NpcScalingTable.h", "NpcParamsTable()")
    scaling = _parse_int_array("NpcScalingTable.h", "NpcScalingTable()")
    seen = {}
    for v in scaling:
        seen[v] = seen.get(v, 0) + 1
    cases.append(("032: 1171 tracked values, each appearing exactly ONCE in 26591",
                  len(tracked) == 1171 and len(scaling) == 26591 and
                  all(seen.get(v) == 1 for v in tracked)))
    reached = sum(1 for mn, e in overwritable_placements(root, loaded)
                  if zone_scaled_npc(mn, e["npc"]) != e["npc"])
    cases.append(("032: the stat pass reaches 1369 of the 2269 - spec F6 exactly",
                  reached == 1369))
    # Why plain equality is the WRONG frozen test, pinned so it cannot regress.
    scaled_maidens = sum(1 for mn, e in placements
                         if any(p in e["name"] for p in MAIDENS)
                         and not is_m28_forced(mn, e["name"])
                         and zone_scaled_npc(mn, e["npc"]) != e["npc"])
    cases.append(("032: 26 of the 42 non-forced maidens sit in stat-scaled maps",
                  scaled_maidens == 26))

    # No chaining, over the REACHABLE value set rather than the vanilla maps:
    # a scaled map can hold any NPCParamID the pool can place, so the vanilla
    # form of this check does not cover the values the mirror will meet.
    reachable = {e["npc"] for _mn, e in placements if e["npc"] > 1}
    reachable |= {npc for npc, _t, _m in engine_pool(root, maps=loaded)}
    zones = sorted({z for z in _scaling_maps().values() if z})
    pos = {}
    for j, x in enumerate(scaling):
        if x in seen and seen[x] == 1:
            pos.setdefault(x, j)
    chain_free = True
    for z in zones:
        single = {v: scaling[pos[v] + z] for v in tracked
                  if v in pos and pos[v] + z < len(scaling)}
        for v in reachable:
            once = single.get(v, v)
            if single.get(once, once) != once and once in single:
                chain_free = False
    cases.append(("032: no sequential chaining over the %d reachable values, 15 zones"
                  % len(reachable), chain_free))

    # --- the picker's strings ---------------------------------------------
    picker = named_strings(os.path.join(UI_SRC, "ModelPicker.h"))
    picker_src = open(os.path.join(UI_SRC, "ModelPicker.h"), encoding="utf-8").read()
    skip_strings = picker_src.split("kEnemiesSkippedStrings", 1)[1].split("};", 1)[0]
    skip_words = re.findall(r'"((?:[^"\\]|\\.)*)"', skip_strings)
    # Eight, one per PickerStrings field: heading, instruction, the two flag
    # words, the two confirm verbs, and the two footer VERBS.
    #
    # It was seven while the footer was one flat string reading "SQUARE SKIP ALL
    # TRIANGLE SKIP NONE   O BACK". U4 pass 4 made that line a prompt row, so
    # the buttons are fixed in ModelPicker.cpp and only the two verbs are the
    # picker's own - BACK went with the string, because it is the same word on
    # all three pickers.
    cases.append(("032: the skipped picker declares all eight of its strings",
                  len(skip_words) == 8))
    cases.append(("032: every picker string is renderable and fits the line",
                  all(renderable(w) and len(w) <= LINE_CHARS for w in skip_words)))
    cases.append(("032: the instruction line is the 42-character F13 wording",
                  "SELECT ENEMIES THAT WILL NOT BE RANDOMIZED" in skip_words and
                  len("SELECT ENEMIES THAT WILL NOT BE RANDOMIZED") == 42))
    cases.append(("032: the flag column reads SKIPPED / - , not YES / NO (P13)",
                  "SKIPPED" in skip_words and "-" in skip_words and
                  "YES" not in skip_words))
    # The widest row the new list can draw, against the 71-character line.
    widest = max(len(m) + 1 + len(n) for m, n, _w in skip_rows)
    cases.append(("032: the widest skip row is 39 + 3 + 7 = 49 characters",
                  widest == 39 and widest + 3 + len("SKIPPED") == 49))

    # --- the config line ---------------------------------------------------
    store = open(os.path.join(SRC, "RandomizerDefaultsStore.cpp"), encoding="utf-8").read()
    bufs = [int(x) for x in re.findall(r"char buf\[(\d+)\]", store)]
    # The buffer that matters is FormatSettings' own, not the largest one in the
    # file: LoadRandomizerDefaults' char buf[4096] is a READ buffer, so
    # max(bufs) silently made the fit check below a comparison against that
    # instead. Feature 013 is the first change where the difference decides the
    # answer - 1,038 bytes fits 4,096 and does not fit 1,024 - so the format
    # buffer is now parsed from inside the function it belongs to.
    format_body = store.split("std::string FormatSettings", 1)[1].split("\n}", 1)[0]
    # The semicolon matters: the narrating comment above the declaration quotes
    # "char buf[1024]" and "char buf[4096]" as prose, and a regex without it
    # reads the history instead of the code.
    format_buf = int(re.search(r"char buf\[(\d+)\];", format_body).group(1))
    # Worst case: every fixed line at its longest, plus all three selections.
    #
    # The SETTINGS BLOCK is the part FormatSettings() builds in char buf[1024],
    # and it is the part shared with a world's rev-NNNN.cfg. defaults.cfg wraps
    # it in two more lines of its own, which are built separately and are not
    # in that buffer - so the buffer check below is against the block, and the
    # total is what the whole file costs on disk.
    # The five picker selection lines are one character per table row, so their
    # lengths are a property of the GENERATED HEADERS and must be read from them.
    # They were hardcoded here until 2026-09-29, when correcting
    # gen_weapon_table.py's LOT_CATEGORY_WEAPON took LeftHandWeaponTable.h from
    # 14 rows to 16 and this arithmetic did not notice: the exact-equality
    # assertion below went on passing against a stale figure, which is precisely
    # the failure it exists to prevent.
    def _rows(header):
        src = open(os.path.join(SRC, header), encoding="utf-8").read()
        return int(re.search(r"std::array<ModelPoolEntry,\s*(\d+)>", src).group(1))

    n_boss  = _rows("BossPoolTable.h")
    n_enemy = _rows("EnemyPoolTable.h")
    n_skip  = _rows("EnemySkipTable.h")
    n_trick = _rows("TrickWeaponTable.h")
    n_left  = _rows("LeftHandWeaponTable.h")

    # Feature 040's four are all in ONE generated header, so they cannot be read
    # by _rows() above - it takes the first std::array in the file and there are
    # four. Read by count constant instead, which is the same thing the C++
    # template argument is.
    def _count(header, const):
        src = open(os.path.join(SRC, header), encoding="utf-8").read()
        return int(re.search(r"const int %s = (\d+);" % const, src).group(1))

    n_head    = _count("ArmourTables.h", "kHeadwearCount")
    n_body    = _count("ArmourTables.h", "kBodyArmourCount")
    n_gloves  = _count("ArmourTables.h", "kGlovesCount")
    n_legwear = _count("ArmourTables.h", "kLegwearCount")

    settings_block = (len("randomize_enemies=1") + 1 +
                      len("randomize_bosses=1") + 1 + len("randomize_treasure=1") + 1 +
                      len("randomize_workshop_tools=1") + 1 + len("randomize_enemy_drops=1") + 1 +
                      len("randomize_starting_weapons=1") + 1 + len("randomize_starting_guns=1") + 1 +
                      len("randomize_shop_weapons=1") + 1 +
                      len("randomize_shop_armour=1") + 1 +
                      len("randomize_shop_items=1") + 1 +
                      len("enable_mergo_darkness=1") + 1 +
                      len("no_team_type=1") + 1 +
                      len("do_not_randomize_caged_dogs=1") + 1 +
                      len("start_with_hunter_tools=1") + 1 +
                      len("start_with_supplies=1") + 1 +
                      len("easy_shadows=1") + 1 + len("easy_rom=1") + 1 +
                      len("easy_failures=1") + 1 + len("easy_emissary=1") + 1 +
                      len("start_fresh_save=1") + 1 +
                      len("bosses_included=") + n_boss + 1 +
                      len("enemies_included=") + n_enemy + 1 +
                      len("enemies_skipped=") + n_skip + 1 +
                      len("trick_weapons_included=") + n_trick + 1 +
                      len("left_hand_weapons_included=") + n_left + 1 +
                      # Feature 040's four picker lines, 217 bytes together.
                      # Measured from the headers rather than written out, for
                      # the same reason the five lines above are.
                      len("armour_head_included=") + n_head + 1 +
                      len("armour_body_included=") + n_body + 1 +
                      len("armour_gloves_included=") + n_gloves + 1 +
                      len("armour_legwear_included=") + n_legwear + 1 +
                      # Feature 013's eight, 248 bytes. The count key is
                      # measured at its widest value, 15, because that is what
                      # a worst case means for an integer setting.
                      len("bosses_can_replace_enemies=1") + 1 +
                      len("boss_replacements_exact=1") + 1 +
                      len("boss_replacements_per_map=15") + 1 +
                      len("replace_all_enemies=1") + 1 +
                      len("space_out_inserted_bosses=1") + 1 +
                      len("inserted_boss_keeps_enemy_ai=1") + 1 +
                      len("inserted_bosses_included=") + 22 + 1 +
                      len("inserted_boss_areas=") + 14 + 1)
    worst = (len("bloodborne_title_id=CUSA00000") + 1 +
             settings_block +
             len("last_seed=4294967295") + 1)
    # 555 until feature 033 added the caged-dogs key, which is 30 bytes with
    # its newline; 585 until START WITH HUNTER TOOLS added 26 more; 611 until
    # feature 018's four easy-mode keys added 58 (15 + 11 + 16 + 16), making
    # 669; 616 once the randomizer-settings-ui spec §4.8 removed
    # backup_existing_save (23 with its newline) and
    # replace_save_default_is_new (30); 635 once the worlds feature added
    # start_fresh_save (19); 737 once feature 037 added
    # trick_weapons_included, which is 102 bytes - the 23-character key, a
    # 78-character selection line and its newline. The settings block went 584
    # -> 686 with it. Then 779 once feature 038 added
    # left_hand_weapons_included, which is 42 bytes - the 26-character key, a
    # 14-character selection line and its newline - taking the settings block to
    # 728 and leaving 296 bytes of char buf[1024] spare. Those 102 and 42 are why
    # neither key name can be renamed without redoing this arithmetic. Then 794
    # once feature 027 added no_team_type, which is 15 bytes - the 12-character
    # key, its "=1" and its newline - taking the settings block to 743 and
    # leaving 281 bytes of char buf[1024] spare. Then 841 once feature 011 added
    # randomize_shop_armour and randomize_shop_items, which are 24 and 23 bytes -
    # each the key, its "=1" and its newline - taking the settings block to 790
    # and leaving 234 bytes of char buf[1024] spare.
    #
    # Then 1,089 once feature 013 added its eight keys, which are 248 bytes
    # together (29 + 26 + 29 + 22 + 28 + 31 + 48 + 35), taking the settings
    # block to 1,038 - FOURTEEN BYTES OVER char buf[1024], which snprintf would
    # have clamped, dropping the tail of the last key in silence. That is why
    # feature 013 grows the buffer to 2,048 (spec 013 D-I): the growth is a
    # prerequisite of the feature, not a tidy-up, and 1,038 leaves 1,010 spare.
    #
    # The 790 this rebases from is itself the sum of rows 27 (+15) and 11 (+47)
    # landing after plan 013 was written; the plan's own 976 figure was measured
    # against the 728-byte block that predated both, and 728 + 15 + 47 + 248 is
    # 1,038 either way. Whoever lands next rebases the same way.
    #
    # Then 1,091 / a 1,040-byte block, because correcting gen_weapon_table.py's
    # LOT_CATEGORY_WEAPON took LeftHandWeaponTable.h from 14 rows to 16 and every
    # picker line is one character per table row. That is the two bytes the five
    # lengths above are now READ for rather than written out.
    #
    # Then 1,330 / a 1,279-byte block once feature 040 added its five keys, which
    # are 239 bytes together: 55 + 56 + 50 + 56 for the four armour pickers (each
    # the key, the '=', its 33 / 34 / 26 / 31 character selection line and a
    # newline) and 22 for start_with_supplies=1. That leaves 769 bytes of char
    # buf[2048] spare. Those five lengths are why none of the five key names can
    # be renamed without redoing this arithmetic.
    #
    # This is an exact equality on purpose: it fails the
    # moment a key is added or removed without the buffer being thought about.
    cases.append(("worst-case defaults.cfg is 1330 bytes", worst == 1330))
    cases.append(("the 1279-byte settings block fits FormatSettings' char buf[2048]",
                  settings_block == 1279 and format_buf == 2048 and
                  settings_block < format_buf))
    # The save-data removal, checked the way the two cases below check a key
    # that must be PRESENT: on the quoted key literal in the load chain, the
    # "key=%d" fragment of the save format string, and the struct field the
    # argument list would name. Prose naming the retired keys - the tolerance
    # comment does, deliberately - therefore cannot make this pass or fail.
    cases.append(("save data: neither retired key survives in load or save",
                  '"backup_existing_save"' not in store and
                  '"replace_save_default_is_new"' not in store and
                  "backup_existing_save=%d" not in store and
                  "replace_save_default_is_new=%d" not in store and
                  "backupExistingSaveData" not in store and
                  "replaceSaveDefaultIsNew" not in store))
    cases.append(("032: unchanged_bell_maidens is gone from load AND save (D1)",
                  "unchanged_bell_maidens" not in store.replace(
                      "// Dropping unchanged_bell_maidens here is genuinely free: the loader above", "")))
    cases.append(("032: enemies_skipped is in both load and save",
                  store.count("enemies_skipped") >= 2))
    # A key written but never read loads as off forever, and a key read but
    # never written is forgotten on save - both compile and both look fine.
    # A key written but never read loads as off forever, and a key read but
    # never written is forgotten on save - the same trap as the caged-dogs
    # case below.
    cases.append(("start_with_hunter_tools is in both load and save",
                  store.count('"start_with_hunter_tools"') == 1 and
                  "start_with_hunter_tools=%d" in store and
                  "defaults.startWithHunterTools ? 1 : 0" in store))
    cases.append(("033: do_not_randomize_caged_dogs is in both load and save",
                  store.count('"do_not_randomize_caged_dogs"') == 1 and
                  "do_not_randomize_caged_dogs=%d" in store))
    # Feature 027. Same trap, same shape: written but never read loads as off
    # forever, read but never written is forgotten on save.
    cases.append(("027: no_team_type is in both load and save",
                  store.count('"no_team_type"') == 1 and
                  "no_team_type=%d" in store and
                  "defaults.noTeamType ? 1 : 0" in store))
    # Feature 011, one case per key and the same trap again: a key written but
    # never read loads as off forever, and a key read but never written is
    # forgotten on save. Two separate cases rather than one, because a single
    # case that checked "either key" would pass on a build that forgot one.
    cases.append(("011: randomize_shop_armour is in both load and save",
                  store.count('"randomize_shop_armour"') == 1 and
                  "randomize_shop_armour=%d" in store and
                  "defaults.randomizeShopArmour ? 1 : 0" in store))
    cases.append(("011: randomize_shop_items is in both load and save",
                  store.count('"randomize_shop_items"') == 1 and
                  "randomize_shop_items=%d" in store and
                  "defaults.randomizeShopItems ? 1 : 0" in store))
    # Same trap, one shape different: a selection key is written as %s from
    # Encode(), so the save side is checked on the format fragment AND on the
    # Encode() call, not on a "=%d".
    cases.append(("037: trick_weapons_included is in both load and save",
                  store.count('"trick_weapons_included"') == 1 and
                  "trick_weapons_included=%s" in store and
                  "defaults.trickWeapons.Encode().c_str()" in store))
    # Feature 013, eight keys and the same trap eight more times: written but
    # never read loads as off forever, read but never written is forgotten on
    # save. The scalar keys are checked on the format fragment AND on the
    # argument that feeds it, because "%d" alone would pass with the wrong field
    # behind it; the two selections are checked on Decode/Encode instead, since
    # that is how a selection crosses the same boundary.
    for key, arg in (("bosses_can_replace_enemies", "defaults.bossesCanReplaceEnemies ? 1 : 0"),
                     ("boss_replacements_exact", "defaults.bossReplacementsExact ? 1 : 0"),
                     ("boss_replacements_per_map", "defaults.bossReplacementsPerMap"),
                     ("replace_all_enemies", "defaults.replaceAllEnemies ? 1 : 0"),
                     ("space_out_inserted_bosses", "defaults.spaceOutInsertedBosses ? 1 : 0"),
                     ("inserted_boss_keeps_enemy_ai", "defaults.insertedBossKeepsEnemyAi ? 1 : 0")):
        cases.append(("013: %s is in both load and save" % key,
                      store.count('"%s"' % key) == 1 and
                      "%s=%%d" % key in store and arg in store))
    cases.append(("013: inserted_bosses_included is in both load and save",
                  store.count('"inserted_bosses_included"') == 1 and
                  "inserted_bosses_included=%s" in store and
                  "out.insertedBosses.Decode(value)" in store and
                  "defaults.insertedBosses.Encode().c_str()" in store))
    cases.append(("013: inserted_boss_areas is in both load and save",
                  store.count('"inserted_boss_areas"') == 1 and
                  "inserted_boss_areas=%s" in store and
                  "out.bossAreas.Decode(value)" in store and
                  "defaults.bossAreas.Encode().c_str()" in store))
    # The count is the app's first integer setting, and a hand-edited config is
    # the one path that can present a value the UI cannot reach, so the loader
    # clamps as well as AdjustSetting.
    cases.append(("013: the count key is clamped to 0-15 on load, not just in the UI",
                  "if (n < 0) n = 0;" in store and "if (n > 15) n = 15;" in store))
    # Feature 040, five more of the same trap. Four separate cases for the four
    # picker keys rather than one that checks "any of them": a single case would
    # pass on a build that wired the gloves key to the legwear selection, which is
    # the mistake four near-identical chains actually make.
    for key, field in (("armour_head_included", "headwear"),
                       ("armour_body_included", "bodyArmour"),
                       ("armour_gloves_included", "gloves"),
                       ("armour_legwear_included", "legwear")):
        cases.append(("040: %s is in both load and save, on %s" % (key, field),
                      store.count('"%s"' % key) == 1 and
                      "%s=%%s" % key in store and
                      "out.%s.Decode(value)" % field in store and
                      "defaults.%s.Encode().c_str()" % field in store))
    cases.append(("040: start_with_supplies is in both load and save",
                  store.count('"start_with_supplies"') == 1 and
                  "start_with_supplies=%d" in store and
                  "defaults.startWithSupplies ? 1 : 0" in store))
    cases.append(("038: left_hand_weapons_included is in both load and save",
                  store.count('"left_hand_weapons_included"') == 1 and
                  "left_hand_weapons_included=%s" in store and
                  "defaults.leftHandWeapons.Encode().c_str()" in store))

    # --- 037: PickerStrings::showRowId, set explicitly at every site ---------
    #
    # PickerStrings is AGGREGATE-INITIALISED, so a site that omits the trailing
    # field gets `false` silently and loses its id column - the three shipped
    # pickers would stop drawing "C4520" beside the name and nothing would fail
    # to compile. Counting the initialisers is the only check that catches it.
    picker_defs = re.findall(
        r"inline constexpr PickerStrings (\w+) = \{(.*?)\};", picker_src, re.S)
    cases.append(("037: eleven PickerStrings are declared",
                  len(picker_defs) == 11))
    row_ids = {name: re.findall(r"\b(true|false)\b", body)
               for name, body in picker_defs}
    cases.append(("037: every PickerStrings sets showRowId explicitly",
                  all(len(v) == 1 for v in row_ids.values())))
    cases.append(("037: the three creature pickers keep their id column, the "
                  "two weapon pickers have none",
                  row_ids.get("kEnemiesIncludedStrings") == ["true"] and
                  row_ids.get("kBossesIncludedStrings") == ["true"] and
                  row_ids.get("kEnemiesSkippedStrings") == ["true"] and
                  row_ids.get("kTrickWeaponsStrings") == ["false"] and
                  row_ids.get("kLeftHandWeaponsStrings") == ["false"]))
    # Feature 013's two. INSERTED BOSSES keeps the id column for the same reason
    # the three creature lists do; AREAS INCLUDED has none (plan 013 D3) because
    # a map-name prefix means nothing to a player and all fourteen names differ.
    cases.append(("013: the inserted-boss picker keeps its id column and the "
                  "area picker has none",
                  row_ids.get("kInsertedBossesStrings") == ["true"] and
                  row_ids.get("kBossAreasStrings") == ["false"]))
    # Feature 040's four. No id column on any of them: every name is the game's
    # own and a six-digit param id means nothing to a player. Checked one by one
    # rather than with all(), so a single slot that silently dropped the field
    # names itself.
    for name in ("kHeadwearStrings", "kBodyArmourStrings", "kGlovesStrings",
                 "kLegwearStrings"):
        cases.append(("040: %s has no id column" % name,
                      row_ids.get(name) == ["false"]))
    picker_cpp = open(os.path.join(UI_SRC, "ModelPicker.cpp"), encoding="utf-8").read()
    cases.append(("037: RowLabel honours showRowId and is called with it",
                  "RowLabel(const ModelPoolEntry& m, bool showRowId)" in picker_cpp and
                  "if (!showRowId) return std::string(m.displayName);" in picker_cpp and
                  "RowLabel(table[index], strings.showRowId)" in picker_cpp))

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
    a = [x for x in a if x != "--starved"] if a[0] != "skipped" else a
    if a[0] == "table" and len(a) == 3:
        if a[1] == "both":
            return cmd_table("enemy", a[2]) | cmd_table("boss", a[2])
        if a[1] in TABLES:
            return cmd_table(a[1], a[2])
        print(__doc__)
        return 2
    if a[0] == "filter" and len(a) == 4:
        return cmd_filter(a[1], a[2], a[3])
    if a[0] == "skipped" and len(a) in (4, 5):
        return cmd_skipped(a[1], a[2], a[3], "--starved" in a)
    if a[0] == "selftest" and len(a) == 2:
        return cmd_selftest(a[1])
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main())
