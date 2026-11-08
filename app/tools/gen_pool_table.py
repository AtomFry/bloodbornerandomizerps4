#!/usr/bin/env python3
"""Generate the baked model tables: EnemyPoolTable.h, BossPoolTable.h,
EnemySkipTable.h, InsertedBossTable.h, InsertAreaTable.h.

The PS4 app must not parse Characters.json or rebuild a pool just to draw a
menu, so each list is baked exactly like ModelSizeTable.h and NpcScalingTable.h.

THE ORDER IS LOAD-BEARING. defaults.cfg stores each selection as one character
per table row (see ModelPoolSelection.h), so regenerating a table with a
different order silently remaps everyone's saved selection onto the wrong
creatures. The length check in the loader catches a changed COUNT; it cannot
catch a changed order. If the order ever has to change, change the config key
name at the same time.

Usage:
    python gen_pool_table.py enemy|boss|skip|insertboss|area|both
                             <vanilla_dvdroot> [--check]

`both` stays enemy+boss, so every existing invocation means what it did.

`insertboss` and `area` are feature 013's two tables. The first is NOT
BossPoolTable.h with different rows - it is a different list, harvested by
different rules (boss_verify.insertion_list), and it gets its own config key
for that reason. The second is not creatures at all: its `model` column holds a
map-name PREFIX, following CagedDogList.h's precedent, and its display name
comes from MapNames.json rather than Characters.json.

--check exits non-zero if the file on disk differs from what would be
generated, without writing anything - for the *_pool_verify.py table checks.
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from enemy_lookup import (engine_pool, engine_pool_models,  # noqa: E402
                          overwritable_models, overwritable_counts)
from boss_verify import (build_pool, load_maps, boss_pool_models,  # noqa: E402
                         insertion_list, insertion_models, insertion_areas)
from names import model_name  # noqa: E402

SRC = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "src", "Randomizer")

# Font8x8.cpp renders A-Z, 0-9, space and - since the enemy picker - ' ( ) - ,
# and, since feature 040's armour names, / as well. Anything else comes out as a
# blank gap, so a name containing one is a bug to catch here rather than a
# mystery on the TV. This set mirrors Font8x8.cpp's kGlyphs and must be grown
# with it, never instead of it: adding a character here without the glyph turns
# the check green and leaves the hole on the TV.
RENDERABLE = set("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 '()-,/")


# Names this project owns, consulted BEFORE Characters.json. Each entry needs
# a reason a reader can check, because overriding the community data set is
# exactly the kind of edit that looks like a mistake later.
#
#   c1130 - Characters.json calls it "Labyrinth Ritekeeper", but
#           docs/enemy-exclusion-history.md:118-122 identifies c1130_0000 - the
#           only retail-loaded one of its two placements - as the Oedon Chapel
#           dweller: ThinkParamID 0, no combat AI, DemonsFanatic in the
#           external data sheet. Settled by the developer as spec 032 D6.
#
# Applied for every Kind, which is safe and worth stating: c1130 occurs zero
# times in EnemyPoolTable.h and zero times in BossPoolTable.h, so those two
# files cannot change because of anything in here.
NAME_OVERRIDES = {
    "c1130": ("OEDON CHAPEL DWELLER", "spec 032 D6 - see NAME_OVERRIDES"),
}


def display_name(model):
    if model in NAME_OVERRIDES:
        return NAME_OVERRIDES[model][0]
    name = (model_name(model) or model).upper()
    bad = sorted({c for c in name if c not in RENDERABLE})
    if bad:
        raise SystemExit(
            "%s: name %r contains characters the 8x8 font cannot render: %r\n"
            "Add the glyphs to Font8x8.cpp or sanitize the name here."
            % (model, name, bad))
    return name


class Kind(object):
    # name_fn turns a `model` column value into its display name. It is a
    # parameter rather than always display_name() because the area table's
    # column holds a map-name prefix, which Characters.json knows nothing about.
    def __init__(self, key, header, func, count_const, models_fn, weights_fn, blurb,
                 name_fn=None, weight_blurb=None):
        self.key = key
        self.header = header
        self.func = func
        self.count_const = count_const
        self.models_fn = models_fn
        self.weights_fn = weights_fn
        self.blurb = blurb
        self.name_fn = name_fn or display_name
        self.weight_blurb = weight_blurb


def enemy_weights(root):
    w = {}
    for _npc, _think, mdl in engine_pool(root):
        w[mdl] = w.get(mdl, 0) + 1
    return w


def boss_weights(root):
    pool, _refill, _ook = build_pool(load_maps(root))
    w = {}
    for _npc, _think, mdl in pool:
        if mdl:
            w[mdl] = w.get(mdl, 0) + 1
    return w


def insertboss_weights(root):
    """How many distinct identities that model contributes to the insertion
    list - a real draw weight here, because the insertion pool is drained BY
    IDENTITY and refilled from the full list (plan 013 B17)."""
    w = {}
    for _npc, _think, mdl in insertion_list(root):
        if mdl:
            w[mdl] = w.get(mdl, 0) + 1
    return w


def area_models(root):
    return [prefix for prefix, _lf, _name, _n in insertion_areas(root)]


def area_weights(root):
    return {prefix: n for prefix, _lf, _name, n in insertion_areas(root)}


# prefix -> display name, filled by build() before it asks for any row, because
# the name comes from the area's live map file and not from the prefix itself.
_AREA_NAMES = {}


def area_display_name(prefix):
    name = _AREA_NAMES[prefix]
    bad = sorted({c for c in name if c not in RENDERABLE})
    if bad:
        raise SystemExit(
            "%s: name %r contains characters the 8x8 font cannot render: %r"
            % (prefix, name, bad))
    return name


def skip_weights(root):
    """NOT a draw weight - nothing ever draws from the skip table. This is how
    many placements a ticked row protects, which is the only number that means
    anything for a list you cannot be replaced FROM."""
    return overwritable_counts(root)


KINDS = {
    "enemy": Kind(
        "enemy", "EnemyPoolTable.h", "EnemyPoolTable", "kEnemyPoolModelCount",
        engine_pool_models, enemy_weights,
        ["// Every model that can appear in the ENEMY randomizer's candidate pool,",
         "// which is NOT the same as every enemy model in the game - placements on",
         "// the exclusion list, with no think/npc param, or filtered by one of the",
         "// two special cases in EnemyRandomizer.cpp never contribute."]),
    "boss": Kind(
        "boss", "BossPoolTable.h", "BossPoolTable", "kBossPoolModelCount",
        boss_pool_models, boss_weights,
        ["// Every model that can appear in the BOSS randomizer's candidate pool,",
         "// per docs/plans/boss-randomization.md section 4.2 - the boss-name list,",
         "// minus the eligibility rejects, minus the narrower insert-reject list.",
         "//",
         "// Note this pool is DRAINED and refilled as bosses are assigned, so a",
         "// selection of one model means every boss arena gets that model."]),
    "skip": Kind(
        "skip", "EnemySkipTable.h", "EnemySkipTable", "kEnemySkipModelCount",
        overwritable_models, skip_weights,
        ["// Every model the ENEMY randomizer is allowed to OVERWRITE - the other",
         "// side of the pool. A row ticked in ENEMIES SKIPPED leaves that creature",
         "// out of the run in both directions: its own placements are never",
         "// rewritten, and it never arrives as a replacement. See feature 032.",
         "//",
         "// This is NOT EnemyPoolTable.h with extra rows and must never be derived",
         "// from it. The two answer different questions - what can be REPLACED",
         "// versus what can REPLACE - and they carry different per-row numbers.",
         "// The 82 pool models are a strict subset of these 85; the three extras",
         "// are c1130, c2121 (both ThinkParamID <= 1, so neither can enter the",
         "// pool) and c2561 (dropped by an explicit name test in the pool loop).",
         "//",
         "// `poolEntries` therefore counts PLACEMENTS PROTECTED, not draw weight:",
         "// nothing draws from this table. The column sums to 2269."]),
    "insertboss": Kind(
        "insertboss", "InsertedBossTable.h", "InsertedBossTable",
        "kInsertedBossModelCount", insertion_models, insertboss_weights,
        ["// Every boss model INSERTED BOSSES can write over an ordinary enemy",
         "// placement (feature 013). This is NOT BossPoolTable.h: the reference",
         "// harvests insertBossesEnemy at RandomizeFunctions.cs:1657, which is",
         "// BEFORE three of the de-eligibility tests the boss pool applies, and it",
         "// applies neither the 15-pattern insert-reject list nor the per-map npc",
         "// dedupe. Measured: 32 identities across these 22 models, against the",
         "// boss pool's 18 across 17. Moon Presence (c5400) is removed, as the",
         "// reference removes it.",
         "//",
         "// Its own table and its own config key on purpose (plan 013 D-B):",
         "// BOSSES INCLUDED and this are separate questions about separate",
         "// passes, and sharing either would tie them together forever."]),
    "area": Kind(
        "area", "InsertAreaTable.h", "InsertAreaTable", "kInsertAreaCount",
        area_models, area_weights,
        ["// The fourteen explorable areas AREAS INCLUDED can tick for feature",
         "// 013's boss-insertion pass - the fourteen if (boss<Area>) blocks of",
         "// StartFunctions.cs:1038-1101, which gate that pass and nothing else.",
         "//",
         "// `model` IS A MAP-NAME PREFIX, not a creature, following",
         "// CagedDogList.h's precedent. Each prefix covers every file of its area,",
         "// one to three of them, because the pass visits the unused variants too,",
         "// exactly as the reference does; keying on a live file name alone would",
         "// silently drop 8 of the 22 files. The fourteen prefixes are mutually",
         "// exclusive, cover all 22 area files, and match neither m21 file - so",
         "// the Hunter's Dream and the Abandoned Old Workshop can never be an",
         "// insertion area.",
         "//",
         "// Names are from tools/data/MapNames.json, taken from each area's LIVE",
         "// map file."],
        name_fn=area_display_name,
        weight_blurb=[
            "// `poolEntries` is how many eligible insertion targets that area's LIVE",
            "// map file holds - the placements a boss could be written over, which",
            "// is what caps a map file's insertion count. Not a draw weight; nothing",
            "// draws from this table. The column sums to 1381."]),
}


def build(kind, root):
    models = kind.models_fn(root)
    weights = kind.weights_fn(root)
    if kind.key == "area":
        _AREA_NAMES.clear()
        _AREA_NAMES.update({p: n for p, _lf, n, _w in insertion_areas(root)})
    rows = [(m, kind.name_fn(m), weights.get(m, 0)) for m in models]
    width = max(len(n) for _m, n, _w in rows)
    # The model column is padded too, for the area table's uneven prefixes. All
    # three creature tables have 5-character ids, so their rows are unchanged.
    mwidth = max(len(m) for m, _n, _w in rows) + 3

    L = []
    L.append("// %s - GENERATED by tools/gen_pool_table.py %s." % (kind.header, kind.key))
    L.append("// Do not edit by hand; regenerate and re-run the matching verifier.")
    L.append("//")
    L.extend(kind.blurb)
    L.append("//")
    L.append("// ORDER IS LOAD-BEARING: RandomizerDefaults stores the picker's selection")
    L.append("// as one character per row, positionally. Sorted by display name, then by")
    L.append("// model id to break ties - see the generator's header comment.")
    if kind.key != "area":
        L.append("//")
        L.append("// Names are from tools/data/Characters.json (Smithbox, MIT) and are the")
        L.append("// community's labels, not From Software's - treated the same way every")
        L.append("// other name in this project is: useful, not authoritative.")
    if kind.weight_blurb:
        L.append("//")
        L.extend(kind.weight_blurb)
    elif kind.key != "skip":
        L.append("//")
        L.append("// `poolEntries` is how many distinct stat/AI variants that model")
        L.append("// contributes, i.e. its relative draw weight. Unused by the app today;")
        L.append("// baked because it is free here and docs/deferred-ideas.md §2 needs it.")
    L.append("#pragma once")
    L.append("")
    L.append("#include <array>")
    L.append("")
    L.append('#include "ModelPoolSelection.h"')
    L.append("")
    L.append("namespace bbr {")
    L.append("")
    L.append("// Kept as a function-local static for the same reason as")
    L.append("// EnemyExclusionList(): header-only, no translation-unit duplication.")
    L.append("inline const std::array<ModelPoolEntry, %d>& %s() {" % (len(rows), kind.func))
    L.append("    static const std::array<ModelPoolEntry, %d> kTable = {{" % len(rows))
    for mdl, name, w in rows:
        row = '        { %-*s %-*s %3d },' % (mwidth, '"%s",' % mdl,
                                              width + 3, '"%s",' % name, w)
        if mdl in NAME_OVERRIDES:
            row += "  // name is OURS, not Characters.json: %s" % NAME_OVERRIDES[mdl][1]
        L.append(row)
    L.append("    }};")
    L.append("    return kTable;")
    L.append("}")
    L.append("")
    L.append("const int %s = %d;" % (kind.count_const, len(rows)))
    L.append("")
    L.append("} // namespace bbr")
    L.append("")
    return "\n".join(L)


def run(kind, root, check):
    text = build(kind, root)
    out = os.path.join(SRC, kind.header)
    if check:
        if not os.path.exists(out):
            print("FAIL: %s does not exist" % out)
            return 1
        with open(out, encoding="utf-8", newline="") as f:
            on_disk = f.read()
        if on_disk.replace("\r\n", "\n") != text:
            print("FAIL: %s is stale - regenerate it" % kind.header)
            return 1
        print("PASS: %s matches the vanilla tree" % kind.header)
        return 0
    with open(out, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
    print("wrote %s (%d rows)" % (kind.header, text.count('\n        { "')))
    return 0


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    check = "--check" in sys.argv[1:]
    if len(args) < 2 or args[0] not in ("enemy", "boss", "skip", "insertboss",
                                       "area", "both"):
        print(__doc__)
        return 2
    # `both` stays enemy+boss on purpose: it is what every existing script and
    # doc means by it, and silently widening it would regenerate a third file.
    kinds = ["enemy", "boss"] if args[0] == "both" else [args[0]]
    rc = 0
    for k in kinds:
        rc |= run(KINDS[k], args[1], check)
    return rc


if __name__ == "__main__":
    sys.exit(main())
