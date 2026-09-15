#!/usr/bin/env python3
"""Refresh the Smithbox alias data in tools/data/.

Kept as a script rather than a submodule: Smithbox is a ~650 MB repository and
we want two small JSON files from it. See data/README.md.
"""

import json
import os
import sys
import urllib.request

BASE = ("https://raw.githubusercontent.com/vawser/Smithbox/main/"
        "src/Smithbox.Data/Assets/Aliases/BB/")
LICENSE_URL = "https://raw.githubusercontent.com/vawser/Smithbox/main/LICENSE"
FILES = ["Characters.json", "MapNames.json"]
DATA = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data")


def fetch(url, dest, validate_json):
    data = urllib.request.urlopen(url, timeout=60).read()
    if validate_json:
        entries = json.loads(data)
        if not isinstance(entries, list) or not entries:
            raise ValueError("expected a non-empty JSON array")
        missing = [k for k in ("ID", "Name") if k not in entries[0]]
        if missing:
            raise ValueError("entries are missing field(s): %s" % ", ".join(missing))
        count = len(entries)
    else:
        count = None
    with open(dest, "wb") as f:
        f.write(data)
    return len(data), count


def main():
    os.makedirs(DATA, exist_ok=True)
    failed = False

    for name in FILES:
        dest = os.path.join(DATA, name)
        try:
            size, count = fetch(BASE + name, dest, validate_json=True)
            print("  %-18s %8d bytes, %d entries" % (name, size, count))
        except Exception as e:
            print("  %-18s FAILED: %s" % (name, e))
            failed = True

    try:
        size, _ = fetch(LICENSE_URL, os.path.join(DATA, "Smithbox-LICENSE.txt"), False)
        print("  %-18s %8d bytes" % ("Smithbox-LICENSE.txt", size))
    except Exception as e:
        print("  %-18s FAILED: %s" % ("Smithbox-LICENSE.txt", e))
        failed = True

    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
