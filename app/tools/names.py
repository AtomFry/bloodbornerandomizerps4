#!/usr/bin/env python3
"""Model id -> creature name lookup, from the Smithbox alias data.

Reference data only. The PS4 app never reads this; it exists so the PC-side
tools can print "Huntsman (Transformed)" instead of "c2630". See data/README.md
for provenance, licensing and the accuracy caveat.

Usage as a tool:
    python names.py c2630 c1050 m24_01_00_01     # look ids up
    python names.py --search huntsman            # search by name
"""

import json
import os
import sys

DATA = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data")

_characters = None
_maps = None


def _load(filename):
    path = os.path.join(DATA, filename)
    try:
        with open(path, encoding="utf-8") as f:
            return {e["ID"]: e for e in json.load(f)}
    except (OSError, ValueError):
        return {}


def characters():
    global _characters
    if _characters is None:
        _characters = _load("Characters.json")
    return _characters


def maps():
    global _maps
    if _maps is None:
        _maps = _load("MapNames.json")
    return _maps


def model_name(model_id, default=None):
    """'c2630' -> 'Huntsman (Transformed)'. Falls back to the id itself."""
    e = characters().get(model_id)
    return e["Name"] if e else (default if default is not None else model_id)


def placement_name(placement, default=None):
    """'c2630_0000' -> the creature name, by taking the model prefix."""
    return model_name(placement.split("_")[0], default)


def map_name(map_id, default=None):
    e = maps().get(map_id)
    return e["Name"] if e else (default if default is not None else map_id)


def map_is_unused(map_id):
    """True for content Smithbox marks as never loaded by the retail game."""
    e = maps().get(map_id)
    return bool(e) and "unused" in [t.lower() for t in e.get("Tags", [])]


def available():
    return bool(characters())


def main():
    args = sys.argv[1:]
    if not args:
        print(__doc__)
        c, m = characters(), maps()
        print("loaded: %d character names, %d map names" % (len(c), len(m)))
        return 0

    if args[0] in ("--search", "-s"):
        needle = " ".join(args[1:]).lower()
        hits = [e for e in characters().values() if needle in e["Name"].lower()]
        if not hits:
            print("no character name matches %r" % needle)
            return 1
        for e in sorted(hits, key=lambda x: x["ID"]):
            print("  %-8s %s" % (e["ID"], e["Name"]))
        return 0

    for a in args:
        if a.startswith("m"):
            tag = " [unused]" if map_is_unused(a) else ""
            print("  %-14s %s%s" % (a, map_name(a), tag))
        else:
            print("  %-14s %s" % (a, placement_name(a)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
