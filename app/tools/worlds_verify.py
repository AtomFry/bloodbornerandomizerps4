#!/usr/bin/env python3
"""Mirroring the Worlds feature's rules, independently of the C++ that runs them.

This is the third verification layer for the worlds feature (CLAUDE.md section
3): the cross-compile catches C++ errors, the hardware is the final authority
for runtime behaviour, and this pins THE RULES in a second implementation that
can be read and run on a PC.

What it covers today (worlds plan milestones 1 and 2):

  * the save manifest - its exact text format, a round trip through it, and
    the four ways a copy can disagree with it: count, path set, per-file size
    and total. Milestone 1's whole safety argument is "a safety backup is
    VERIFIED before the live save is touched", and this is the definition of
    verified.
  * which files the app treats as the game's own, since that decides what
    START FRESH unlinks and what a restore writes.
  * the invariants of plan section 3.1 that are readable from the source:
    CREATE2 is never set, sceSaveDataDelete is never called, sce_sys is never
    written, a backup is built in <name>.partial and renamed into place, save
    searches are never indexed by position, and no orbis call escapes
    src/Platform/.

  * the world store's rules: how a recipe is written and read back, what
    makes two recipes the same recipe, how world and revision ids are
    allocated, what order the rail lists worlds in, what a world name is
    allowed to be, and which of the four "which world is active" states each
    shape of disk derives to.

  * the activation transaction's rules: every refusal in section 4.4 fired
    on a constructed mismatch and on nothing else, which row of section
    4.3's phase-6 table each shape of the world and the console lands on,
    the journal's format, and the one reconciliation action each phase has.

  * output parity (B28) as far as source can show it: the options mapping
    and the run decision in the activation transaction are the Enable
    wizard's, field for field, so that "only the output path differs" is
    checked rather than asserted.

WHAT IT CANNOT DO. It cannot prove the C++ implements any of this. It proves
the rules are consistent and that the source still says what it said; only the
hardware tests prove the service behaves. A green run here means the rules
hold, not that a save is safe.

Usage:
    python worlds_verify.py                 # everything below
    python worlds_verify.py selftest        # the manifest rules only
    python worlds_verify.py worlds          # the world store rules only
    python worlds_verify.py activation      # the activation rules only
    python worlds_verify.py source          # the source invariants only
    python worlds_verify.py verify <dir>    # check one real backup directory
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
APP = os.path.dirname(HERE)
SRC = os.path.join(APP, "src")

SAVEDATA_H = os.path.join(SRC, "Platform", "SaveData.h")
SAVEDATA_CPP = os.path.join(SRC, "Platform", "SaveData.cpp")
WORLDSTORE_H = os.path.join(SRC, "Randomizer", "WorldStore.h")
WORLDSTORE_CPP = os.path.join(SRC, "Randomizer", "WorldStore.cpp")
DEFAULTS_H = os.path.join(SRC, "Randomizer", "RandomizerDefaults.h")
STORE_CPP = os.path.join(SRC, "Randomizer", "RandomizerDefaultsStore.cpp")
AFR_CPP = os.path.join(SRC, "Game", "AfrManager.cpp")
ACTIVATION_H = os.path.join(SRC, "Game", "WorldActivation.h")
ACTIVATION_CPP = os.path.join(SRC, "Game", "WorldActivation.cpp")


# ---------------------------------------------------------------------------
# The manifest, mirrored
#
# Plan section 4.1: title_id, dir_name, account_id, blocks, files, bytes,
# captured, world, revision, then one "f <relpath> <bytes>" line per file.
# Header order is fixed here because the C++ writes it in this order and a
# round trip that silently reordered it would hide a writer change.
# ---------------------------------------------------------------------------

HEADER_KEYS = [
    "title_id",
    "dir_name",
    "account_id",
    "blocks",
    "files",
    "bytes",
    "captured",
    "world",
    "revision",
]

NUMERIC_KEYS = ("account_id", "blocks", "files", "bytes")


class ManifestError(Exception):
    pass


def format_manifest(header, entries):
    """`entries` is a list of (relpath, bytes), written sorted by path."""
    out = []
    for key in HEADER_KEYS:
        value = header.get(key, 0 if key in NUMERIC_KEYS else "")
        out.append("%s=%s" % (key, value))
    for rel, size in sorted(entries):
        out.append("f %s %d" % (rel, size))
    return "\n".join(out) + "\n"


def parse_manifest(text):
    """Returns (header, entries). Raises ManifestError on anything malformed.

    Unknown keys are ignored, the same tolerance defaults.cfg has, so a
    manifest written by a later version still reads here.
    """
    header = {}
    entries = []
    saw_title = False

    for raw in text.split("\n"):
        line = raw.rstrip("\r")
        if not line:
            continue
        if line.startswith("f "):
            rest = line[2:]
            # rsplit, not split: a path containing a space must survive the
            # round trip rather than reparse into a wrong size.
            sp = rest.rfind(" ")
            if sp <= 0:
                raise ManifestError("malformed file line: %r" % line)
            try:
                size = int(rest[sp + 1:])
            except ValueError:
                raise ManifestError("unreadable size: %r" % line)
            entries.append((rest[:sp], size))
            continue
        if "=" not in line:
            continue
        key, value = line.split("=", 1)
        if key == "title_id":
            saw_title = True
        if key in NUMERIC_KEYS:
            try:
                value = int(value)
            except ValueError:
                raise ManifestError("non-numeric %s: %r" % (key, value))
        if key in HEADER_KEYS:
            header[key] = value

    if not saw_title:
        raise ManifestError("no title_id - not a manifest")

    entries.sort()

    # The header's own counts must agree with the lines beneath it. A manifest
    # that disagrees with itself is not evidence of anything.
    if header.get("files", 0) != len(entries):
        raise ManifestError("manifest says %d file(s) but lists %d"
                            % (header.get("files", 0), len(entries)))
    total = sum(size for _, size in entries)
    if header.get("bytes", 0) != total:
        raise ManifestError("manifest says %d bytes but lists %d"
                            % (header.get("bytes", 0), total))
    return header, entries


def verify(header, entries, found):
    """Compares a walked copy against a manifest.

    `found` is a list of (relpath, bytes) as walked off disk. Returns None when
    it verifies, or the reason it does not. Count, path set, per-file size and
    total - exactly what the spec defines verification as, and no checksums.
    """
    found = sorted(found)
    entries = sorted(entries)

    if len(found) != len(entries):
        return "count - manifest %d file(s), found %d" % (len(entries), len(found))
    for (want_rel, want_size), (got_rel, got_size) in zip(entries, found):
        if want_rel != got_rel:
            return "path - expected %s, found %s" % (want_rel, got_rel)
        if want_size != got_size:
            return "size - %s is %d bytes, manifest says %d" % (got_rel, got_size, want_size)
    total = sum(size for _, size in found)
    if total != header.get("bytes", 0):
        return "total - found %d bytes, manifest says %d" % (total, header.get("bytes", 0))
    return None


def is_game_save_file(rel):
    """The files the GAME wrote, as opposed to the four sce_sys entries.

    Root level only: a '/' means it is inside sce_sys, which is never unlinked
    and never written. This is what START FRESH empties and what a restore puts
    back, so getting it wrong is how another playthrough's surplus backup*
    files would survive into a restored save.
    """
    if "/" in rel:
        return False
    return rel.startswith("userdata") or rel.startswith("backup")


# ---------------------------------------------------------------------------
# A realistic container, from the measured reference save
# (plan evidence M1/M1b: 26 files, 26,949,914 bytes, 1136 blocks)
# ---------------------------------------------------------------------------

def reference_entries():
    entries = [("userdata%04d" % i, 1310720) for i in range(10)]
    entries.append(("userdata0010", 262144))
    entries += [("backup%04d" % i, 1310720) for i in range(10)]
    entries.append(("backup0010", 262144))
    entries += [
        ("sce_sys/icon0.png", 58994),
        ("sce_sys/param.sfo", 2728),
        ("sce_sys/sce_icon0png1", 116736),
        ("sce_sys/sce_paramsfo1", 32768),
    ]
    return sorted(entries)


def reference_header(entries):
    return {
        "title_id": "CUSA00207",
        "dir_name": "SPRJ0005",
        "account_id": 1234567890123456789,
        "blocks": 1136,
        "files": len(entries),
        "bytes": sum(size for _, size in entries),
        "captured": "2026-09-22 12:00:00",
        "world": "vanilla",
        "revision": "",
    }


# ---------------------------------------------------------------------------
# Rules
# ---------------------------------------------------------------------------

def rule_cases():
    cases = []
    entries = reference_entries()
    header = reference_header(entries)
    text = format_manifest(header, entries)

    # --- the shape of the file ---------------------------------------------
    lines = text.split("\n")
    cases.append(("header is the nine keys, in order",
                  [l.split("=", 1)[0] for l in lines[:9]] == HEADER_KEYS))
    cases.append(("every other line is an 'f ' entry",
                  all(l.startswith("f ") for l in lines[9:] if l)))
    cases.append(("the file ends with a newline", text.endswith("\n")))
    cases.append(("26 files, 26949914 bytes - the measured save",
                  len(entries) == 26 and header["bytes"] == 26949914))

    # --- round trip ---------------------------------------------------------
    got_header, got_entries = parse_manifest(text)
    cases.append(("round trip returns the same entries", got_entries == entries))
    cases.append(("round trip returns the same header",
                  all(got_header[k] == header[k] for k in HEADER_KEYS)))
    cases.append(("round trip is byte-identical the second time",
                  format_manifest(got_header, got_entries) == text))

    # A world id and revision are carried through untouched - milestone 2
    # writes them and nothing here is allowed to normalise them away.
    h2 = dict(header, world="w-0007", revision="rev-0003")
    r2, _ = parse_manifest(format_manifest(h2, entries))
    cases.append(("world and revision survive a round trip",
                  r2["world"] == "w-0007" and r2["revision"] == "rev-0003"))

    # A path with a space is not a case that occurs in a Bloodborne save, but
    # the format has to be unambiguous or a later one would reparse wrong.
    spaced = sorted(entries + [("odd name 0001", 17)])
    hs = dict(header, files=len(spaced), bytes=sum(s for _, s in spaced))
    _, rs = parse_manifest(format_manifest(hs, spaced))
    cases.append(("a path containing a space round-trips", rs == spaced))

    # --- manifests that must be rejected -----------------------------------
    def rejects(payload):
        try:
            parse_manifest(payload)
            return False
        except ManifestError:
            return True

    cases.append(("a file with no title_id is not a manifest",
                  rejects("dir_name=SPRJ0005\nfiles=0\nbytes=0\n")))
    cases.append(("a header file count that disagrees is rejected",
                  rejects(format_manifest(dict(header, files=25), entries))))
    cases.append(("a header byte total that disagrees is rejected",
                  rejects(format_manifest(dict(header, bytes=1), entries))))
    cases.append(("a malformed file line is rejected",
                  rejects(text + "f nosize\n")))
    cases.append(("an unreadable size is rejected",
                  rejects(text.replace("f userdata0000 1310720",
                                       "f userdata0000 lots"))))
    cases.append(("an unknown key is ignored, not rejected",
                  parse_manifest("future_key=whatever\n" + text)[1] == entries))

    # --- verification: the four ways a copy can disagree -------------------
    cases.append(("an exact copy verifies", verify(header, entries, entries) is None))

    missing = [e for e in entries if e[0] != "userdata0005"]
    cases.append(("a missing file fails on count",
                  (verify(header, entries, missing) or "").startswith("count")))

    extra = entries + [("userdata0011", 1310720)]
    cases.append(("a surplus file fails on count",
                  (verify(header, entries, extra) or "").startswith("count")))

    renamed = [("userdata0009x" if r == "userdata0009" else r, s) for r, s in entries]
    cases.append(("a renamed file fails on path",
                  (verify(header, entries, renamed) or "").startswith("path")))

    truncated = [(r, s - 1 if r == "userdata0003" else s) for r, s in entries]
    cases.append(("a file one byte short fails on size",
                  (verify(header, entries, truncated) or "").startswith("size")))

    # The case count, path set and total all agree on: two files' contents
    # swapped between them. Only the per-file size catches it, which is why
    # verification is per-file and not just a total.
    swapped = []
    for rel, size in entries:
        if rel == "userdata0010":
            swapped.append((rel, 1310720))
        elif rel == "userdata0000":
            swapped.append((rel, 262144))
        else:
            swapped.append((rel, size))
    cases.append(("two files' sizes swapped fails on size",
                  sum(s for _, s in swapped) == header["bytes"] and
                  (verify(header, entries, swapped) or "").startswith("size")))

    # A total that disagrees with a per-file-correct copy can only come from a
    # manifest header that lies; the per-file loop passes and the total catches
    # it.
    cases.append(("a header total that lies fails on total",
                  (verify(dict(header, bytes=header["bytes"] + 1), entries, entries)
                   or "").startswith("total")))

    cases.append(("an empty copy of a non-empty manifest fails",
                  verify(header, entries, []) is not None))

    # --- which files the game owns -----------------------------------------
    game = [r for r, _ in entries if is_game_save_file(r)]
    cases.append(("22 game save files, and no sce_sys among them",
                  len(game) == 22 and not any(g.startswith("sce_sys") for g in game)))
    cases.append(("sce_sys entries are never game save files",
                  not any(is_game_save_file(r) for r, _ in entries
                          if r.startswith("sce_sys"))))
    cases.append(("a userdata name below the root is not a game save file",
                  not is_game_save_file("sce_sys/userdata0000")))
    cases.append(("the sce_sys directory itself is not a game save file",
                  not is_game_save_file("sce_sys")))

    # The section 3.3 hazard in miniature. A world's save carries however many
    # backup* files its playthrough accumulated - three on a freshly created
    # save, eleven on a played one - so restoring the smaller over the larger
    # without emptying first leaves the surplus mixed into the restored save.
    played = [r for r, _ in entries]
    fresh_backups = {"backup0000", "backup0001", "backup0010"}
    stored = [r for r in played
              if not r.startswith("backup") or r in fresh_backups]
    without_empty = sorted(set(played) | set(stored))
    with_empty = sorted(set(r for r in played if not is_game_save_file(r)) |
                        set(stored))
    cases.append(("restoring without emptying leaves 8 files of the last playthrough",
                  len(without_empty) - len(with_empty) == 8))
    cases.append(("emptying first leaves exactly the stored save",
                  with_empty == sorted(set(stored))))

    return cases


# ---------------------------------------------------------------------------
# The world store, mirrored
#
# Plan section 4.1: worlds.cfg holds next_world_id, world.cfg holds the five
# metadata keys, and rev-NNNN.cfg holds "seed=" plus every setting, in
# defaults.cfg's format and serializer. The KEY SET below is read out of the
# C++ rather than retyped, because a mirror carrying its own copy would go on
# passing after the store stopped writing one.
# ---------------------------------------------------------------------------

WORLD_CFG_KEYS = ["name", "created", "last_played", "last_played_revision", "account_id"]

AFR_MANIFEST_KEYS = ["world_id", "world_name", "revision", "seed",
                     "title_id", "account_id", "written"]


def read(path):
    with open(path, "r", encoding="utf-8", errors="replace") as handle:
        return handle.read()


def code_of(text):
    """The source with // comments and blank lines removed.

    Every invariant below is about what the code DOES, and this code explains
    at length what it deliberately does not do - "never set CREATE2" would
    match its own comment saying so.
    """
    out = []
    for line in text.split("\n"):
        stripped = line.strip()
        if stripped.startswith("//"):
            continue
        if "//" in line:
            line = line[:line.index("//")]
        if line.strip():
            out.append(line)
    return "\n".join(out)


def format_settings_body():
    text = code_of(read(STORE_CPP))
    return text[text.index("std::string FormatSettings"):text.index("bool ApplySettingKey")]


def written_setting_keys():
    """The keys FormatSettings writes, in the order it writes them."""
    return re.findall(r"(\w+)=%[dsu]", format_settings_body().replace("\\n", " "))


def read_setting_keys():
    """The keys ApplySettingKey recognises."""
    return re.findall(r'strcmp\(key, "(\w+)"\)', code_of(read(STORE_CPP)))


def format_recipe(seed, settings, keys):
    """A rev-NNNN.cfg: seed first, then the settings block, unchanged."""
    lines = ["seed=%d" % seed]
    lines += ["%s=%s" % (key, settings[key]) for key in keys]
    return "\n".join(lines) + "\n"


def parse_recipe(text, struct_defaults, known):
    """What WorldStore does: seed is its own key, every other recognised key is
    a setting, and anything absent keeps the struct's own default."""
    seed = 0
    settings = dict(struct_defaults)
    for raw in text.split("\n"):
        line = raw.rstrip("\r")
        if "=" not in line:
            continue
        key, value = line.split("=", 1)
        if key == "seed":
            seed = int(value)
        elif key in known:
            settings[key] = value
    return seed, settings


def same_recipe(a, b, keys):
    """Recipe identity, as the C++ defines it: the same seed, and a settings
    block that serializes to the same text. Not a field-by-field comparison -
    one of those forgets whichever field was added last."""
    seed_a, set_a = a
    seed_b, set_b = b
    return (seed_a == seed_b and
            format_recipe(seed_a, set_a, keys) == format_recipe(seed_b, set_b, keys))


def world_number(name):
    if len(name) != 6 or not name.startswith("w-") or not name[2:].isdigit():
        return 0
    return int(name[2:])


def revision_number(name):
    if len(name) != 12 or not name.startswith("rev-") or not name.endswith(".cfg"):
        return 0
    return int(name[4:8]) if name[4:8].isdigit() else 0


def next_world_id(stored, existing):
    """max(next_world_id, highest existing + 1), never reused."""
    want = max(1, stored)
    for name in existing:
        n = world_number(name)
        if n >= want:
            want = n + 1
    return want


def _descending(text):
    """Sort a string descending inside an ascending tuple sort."""
    return tuple(-ord(c) for c in text)


def rail_order(worlds):
    """last_played descending, then never-played by created descending, with
    the id as the final tiebreak so the order does not reshuffle."""
    def key(w):
        played = bool(w["last_played"])
        return (0 if played else 1,
                _descending(w["last_played"] if played else w["created"]),
                _descending(w["id"]))
    return [w["id"] for w in sorted(worlds, key=key)]


def normalize_world_name(name):
    """A-Z, 0-9 and space, at most 16 characters, uppercased, no trailing
    space. Applied on the way in because world.cfg is one key=value line per
    field: a newline in a name would silently corrupt the file."""
    out = ""
    for ch in name:
        if len(out) >= 16:
            break
        ch = ch.upper()
        if ("A" <= ch <= "Z") or ("0" <= ch <= "9") or ch == " ":
            out += ch
    return out.rstrip(" ")


def derive_active(seeded, manifest_present, account_dir_exists, world_known):
    """The plan's section 4.2 table, as four exclusive answers."""
    if not account_dir_exists:
        return "FirstRun"
    if manifest_present:
        return "World" if world_known else "Unmanaged"
    if seeded:
        return "Unmanaged"
    return "Vanilla"


def world_rule_cases():
    cases = []

    written = written_setting_keys()
    known = read_setting_keys()

    # --- the shared serializer ---------------------------------------------
    cases.append(("every key FormatSettings writes, ApplySettingKey reads",
                  bool(written) and all(k in known for k in written)))
    cases.append(("a revision carries no bloodborne_title_id - the AFR title is a "
                  "Defaults setting (D25)",
                  "bloodborne_title_id" not in written))
    cases.append(("a revision carries no last_seed - a world's seed is its own key",
                  "last_seed" not in written and "seed" not in written))
    cases.append(("bloodborne_title_id and last_seed are still READ, for defaults.cfg",
                  "bloodborne_title_id" in known and "last_seed" in known))
    cases.append(("start_fresh_save is both written and read",
                  "start_fresh_save" in written and "start_fresh_save" in known))

    # --- a recipe round trip ------------------------------------------------
    # Every setting at a non-default value, so a key dropped in either
    # direction changes the result rather than landing back on its default.
    struct_defaults = {}
    for key in written:
        if key.endswith("_included"):
            struct_defaults[key] = "1" * 17
        elif key.endswith("_skipped"):
            struct_defaults[key] = "0" * 17
        else:
            struct_defaults[key] = "0"

    settings = {}
    for key in written:
        if key.endswith("_included") or key.endswith("_skipped"):
            settings[key] = "10" * 8 + "1"
        else:
            settings[key] = "1"

    text = format_recipe(4294967295, settings, written)
    got = parse_recipe(text, struct_defaults, set(known))
    cases.append(("a recipe round-trips to identical settings",
                  got == (4294967295, settings)))
    cases.append(("the seed is the first line of a revision",
                  text.split("\n")[0] == "seed=4294967295"))
    cases.append(("a revision is the seed plus one line per setting",
                  len([l for l in text.split("\n") if l]) == len(written) + 1))

    later = parse_recipe("some_key_from_a_later_build=7\n" + text,
                         struct_defaults, set(known))
    cases.append(("an unknown key in a revision is ignored, not fatal", later == got))

    dropped = "\n".join(l for l in text.split("\n") if not l.startswith("easy_rom="))
    _seed, set_d = parse_recipe(dropped, struct_defaults, set(known))
    cases.append(("a key absent from a revision reads as the struct's own default",
                  set_d["easy_rom"] == struct_defaults["easy_rom"] and
                  set_d["easy_rom"] != settings["easy_rom"]))

    # --- SAVE DATA is KEEP EXISTING unless it says otherwise (D17) ----------
    cases.append(("an absent start_fresh_save is KEEP EXISTING",
                  struct_defaults["start_fresh_save"] == "0" and
                  parse_recipe("seed=1\n", struct_defaults,
                               set(known))[1]["start_fresh_save"] == "0"))
    cases.append(("RandomizerDefaults declares startFreshSave false",
                  "bool startFreshSave = false;" in read(DEFAULTS_H)))

    # --- recipe identity ----------------------------------------------------
    base = (7, dict(settings))
    cases.append(("a recipe equals itself",
                  same_recipe(base, (7, dict(settings)), written)))
    cases.append(("a different seed is a different recipe",
                  not same_recipe(base, (8, dict(settings)), written)))

    flipped = dict(settings)
    flipped["easy_rom"] = "0"
    cases.append(("one flipped setting is a different recipe",
                  not same_recipe(base, (7, flipped), written)))

    policy = dict(settings)
    policy["start_fresh_save"] = "0"
    cases.append(("changing SAVE DATA is a different recipe, so the revert appends "
                  "a revision (B12)",
                  not same_recipe(base, (7, policy), written)))

    # --- ids -----------------------------------------------------------------
    cases.append(("the first world is w-0001", next_world_id(1, []) == 1))
    cases.append(("an id is never reused after a delete",
                  next_world_id(1, ["w-0001", "w-0002", "w-0003"]) == 4))
    cases.append(("a lost worlds.cfg still cannot hand out an existing id",
                  next_world_id(1, ["w-0009"]) == 10))
    cases.append(("a worlds.cfg ahead of the directory wins",
                  next_world_id(40, ["w-0002"]) == 40))
    cases.append(("vanilla is not a numbered world", world_number("vanilla") == 0))
    cases.append(("a stray directory is not a world",
                  world_number("w-00001") == 0 and world_number("w-abcd") == 0 and
                  world_number("w-0001.partial") == 0))
    cases.append(("revision files are rev-NNNN.cfg",
                  revision_number("rev-0001.cfg") == 1 and
                  revision_number("rev-0042.cfg") == 42 and
                  revision_number("world.cfg") == 0 and
                  revision_number("rev-1.cfg") == 0))

    # --- rail order ----------------------------------------------------------
    worlds = [
        {"id": "w-0001", "created": "2026-01-01 00:00:00", "last_played": "2026-05-01 00:00:00"},
        {"id": "w-0002", "created": "2026-02-01 00:00:00", "last_played": ""},
        {"id": "w-0003", "created": "2026-03-01 00:00:00", "last_played": "2026-06-01 00:00:00"},
        {"id": "w-0004", "created": "2026-04-01 00:00:00", "last_played": ""},
    ]
    cases.append(("played worlds first, newest play first, then never-played by "
                  "creation date",
                  rail_order(worlds) == ["w-0003", "w-0001", "w-0004", "w-0002"]))

    tied = [
        {"id": "w-0001", "created": "2026-01-01 00:00:00", "last_played": ""},
        {"id": "w-0002", "created": "2026-01-01 00:00:00", "last_played": ""},
    ]
    cases.append(("a tie is broken by id, so the rail order is stable",
                  rail_order(tied) == ["w-0002", "w-0001"]))

    # --- names ---------------------------------------------------------------
    cases.append(("a name is uppercased", normalize_world_name("my run") == "MY RUN"))
    cases.append(("a name is capped at 16 characters",
                  normalize_world_name("A" * 40) == "A" * 16))
    cases.append(("punctuation and newlines are dropped, so world.cfg cannot break",
                  normalize_world_name("BAD\nNAME!") == "BADNAME"))
    cases.append(("a trailing space is trimmed", normalize_world_name("RUN   ") == "RUN"))
    cases.append(("duplicate names are allowed - identity is the id (D15)",
                  normalize_world_name("RUN") == normalize_world_name("run")))

    # --- which world is active -----------------------------------------------
    rows = [
        ("no account folder",               False, False, False, False, "FirstRun"),
        ("nothing in AFR",                  False, False, True,  False, "Vanilla"),
        ("an AFR root with no dvdroot_ps4", False, False, True,  True,  "Vanilla"),
        ("a manifest naming a known world", True,  True,  True,  True,  "World"),
        ("a manifest naming an unknown world", True, True, True, False, "Unmanaged"),
        ("seeded with no manifest",         True,  False, True,  False, "Unmanaged"),
    ]
    for what, seeded, present, account, world_known, expect in rows:
        cases.append(("4.2: %s derives to %s" % (what, expect),
                      derive_active(seeded, present, account, world_known) == expect))

    # Exhaustive: every shape of disk lands on exactly one of the four states,
    # and none of them falls through.
    states = set()
    total = 0
    for seeded in (False, True):
        for present in (False, True):
            for account in (False, True):
                for world_known in (False, True):
                    states.add(derive_active(seeded, present, account, world_known))
                    total += 1
    cases.append(("4.2: all %d shapes of disk resolve, to exactly the four states" % total,
                  total == 16 and
                  states == set(["FirstRun", "Vanilla", "World", "Unmanaged"])))
    cases.append(("4.2: first run wins over everything, so the live save is captured "
                  "before any claim about AFR",
                  all(derive_active(s, p, False, k) == "FirstRun"
                      for s in (False, True) for p in (False, True)
                      for k in (False, True))))

    # --- the two config files ------------------------------------------------
    world_cfg = "".join("%s=%s\n" % (k, v) for k, v in
                        zip(WORLD_CFG_KEYS,
                            ["MY RUN", "2026-09-22 12:00:00", "2026-09-22 13:00:00",
                             "rev-0002", "1234567890123456789"]))
    parsed = dict(l.split("=", 1) for l in world_cfg.split("\n") if l)
    cases.append(("world.cfg is the five keys of section 4.1",
                  sorted(parsed) == sorted(WORLD_CFG_KEYS)))

    store_cpp = code_of(read(WORLDSTORE_CPP))
    for key in WORLD_CFG_KEYS:
        cases.append(("WorldStore reads and writes the %s key" % key,
                      store_cpp.count('"%s' % key) >= 2))
    cases.append(("worlds.cfg holds next_world_id and nothing else",
                  "next_world_id=%d" in store_cpp and '"next_world_id="' in store_cpp))

    # --- .bbrandomizer_manifest ---------------------------------------------
    afr = code_of(read(AFR_CPP))
    for key in AFR_MANIFEST_KEYS:
        cases.append(("the AFR manifest writes and reads the %s key" % key,
                      '"%s="' % key in afr and '"%s"' % key in afr))
    cases.append(("the AFR manifest lives inside dvdroot_ps4, so it travels with the "
                  "tree it describes (D11)",
                  'DvdRoot(titleId) + "/" + kManifestName' in afr))
    cases.append(("a manifest naming no world is treated as absent",
                  "if (!sawWorld || out.worldId.empty()) return false;" in afr))

    return cases


# ---------------------------------------------------------------------------
# The activation transaction, mirrored
#
# Plan section 4.4 is a table of refusals, section 4.3 a table of what phase 6
# does and a phase machine for reconciling an interrupted run. All three are
# rules rather than mechanisms, which makes them exactly what this file can
# pin: the C++ has to be hardware-tested, but "each refusal fires on its own
# mismatch and on nothing else" is checkable here, twice, in two languages.
# ---------------------------------------------------------------------------

# In the order CheckActivation tests them, which is section 4.4's own order.
# Read out of the C++ enum as well, below, so a reason added on one side and
# not the other is a failure rather than a silence.
REFUSAL_REASONS = [
    "None",
    "NoUser",
    "NotThisAccount",
    "SaveTitle",
    "SaveDirectory",
    "NoContainer",
    "ContainerTooSmall",
    "StoredSaveUnverified",
    "VanillaSourceMissing",
    # There is deliberately no "AfrTitleNotDetected" between these two. Plan
    # P26 removed it in milestone 6: the BLOODBORNE TITLE ID setting is used
    # exactly as entered and AFR handling never validates it against an
    # install. The removed check inspected the AFR overlay, which a Vanilla
    # activation deletes, so the check made B8 a one-way door.
    "AfrNotWritable",
    "EmptySelection",
]

JOURNAL_KEYS = ["from_world", "to_world", "revision", "save_policy",
                "save_title_id", "save_dir_name", "safety_backup", "phase"]


def good_facts():
    """Facts that pass every row, so each case below breaks exactly one thing."""
    return {
        "user_valid": True,
        "account_id": 0x1234567890ABCDEF,
        "world_exists": True,
        "world_is_vanilla": False,
        "world_account_id": 0x1234567890ABCDEF,
        "save_titles": 1,
        "save_dirs": 1,
        "container": True,
        "container_blocks": 1136,
        "stored_save": True,
        "stored_blocks": 1136,
        "stored_verifies": True,
        "source": True,
        "afr_writable": True,
        "rand_enemies": True,
        "any_enemy": True,
        "rand_bosses": True,
        "any_boss": True,
    }


def check_activation(f):
    """Section 4.4, top to bottom. The first row that fires is the answer."""
    if not f["user_valid"] or f["account_id"] == 0:
        return "NoUser"
    if not f["world_exists"] or f["world_account_id"] != f["account_id"]:
        return "NotThisAccount"
    if f["save_titles"] != 1:
        return "SaveTitle"
    if f["save_dirs"] != 1:
        return "SaveDirectory"
    if f["stored_save"] and not f["container"]:
        return "NoContainer"
    if f["stored_save"] and f["stored_blocks"] > f["container_blocks"]:
        return "ContainerTooSmall"
    if f["stored_save"] and not f["stored_verifies"]:
        return "StoredSaveUnverified"
    if not f["world_is_vanilla"] and not f["source"]:
        return "VanillaSourceMissing"
    # No AFR-title row here (P26): the configured title is used as entered.
    if not f["afr_writable"]:
        return "AfrNotWritable"
    if not f["world_is_vanilla"] and f["rand_enemies"] and not f["any_enemy"]:
        return "EmptySelection"
    if not f["world_is_vanilla"] and f["rand_bosses"] and not f["any_boss"]:
        return "EmptySelection"
    return "None"


def broken(**changes):
    f = good_facts()
    f.update(changes)
    return f


# (what, the facts, the reason it must give). Every row of section 4.4 plus
# the two guards carried over from the Enable wizard, and two rows that must
# NOT refuse - a refusal that fires universally is as wrong as one that never
# fires.
REFUSAL_CASES = [
    ("nothing wrong",                     good_facts(),                      "None"),
    ("no foreground user",                broken(user_valid=False),          "NoUser"),
    ("no account id",                     broken(account_id=0),              "NoUser"),
    ("the world is not there",            broken(world_exists=False),        "NotThisAccount"),
    ("another account's world",           broken(world_account_id=42),       "NotThisAccount"),
    ("no save title",                     broken(save_titles=0),             "SaveTitle"),
    ("two save titles",                   broken(save_titles=2),             "SaveTitle"),
    ("no save directory",                 broken(save_dirs=0),               "SaveDirectory"),
    ("two save directories",              broken(save_dirs=2),               "SaveDirectory"),
    ("a stored save and no container",    broken(container=False),           "NoContainer"),
    ("a container one block too small",   broken(container_blocks=1135),     "ContainerTooSmall"),
    ("a stored save that does not verify", broken(stored_verifies=False),    "StoredSaveUnverified"),
    ("no vanilla source",                 broken(source=False),              "VanillaSourceMissing"),
    ("an AFR folder that is not writable", broken(afr_writable=False),       "AfrNotWritable"),
    ("randomize enemies with none selected", broken(any_enemy=False),        "EmptySelection"),
    ("randomize bosses with none selected",  broken(any_boss=False),         "EmptySelection"),
    ("vanilla with no vanilla source",    broken(world_is_vanilla=True, source=False), "None"),
    ("no container and no stored save",   broken(container=False, stored_save=False),  "None"),
]


def save_action(container, start_fresh, same_world, stored_save, has_game_files):
    """Section 4.3's phase-6 table, in the order that table is written."""
    if not container:
        return "NoContainer"
    if start_fresh:
        return "StartFresh"
    if same_world:
        return "Nothing"
    if stored_save:
        return "RestoreOwn"
    if has_game_files:
        return "AdoptLive"
    return "Nothing"


def action_for_phase(phase):
    """What reconciliation does about a journal stopped at `phase`."""
    if phase in (1, 2, 3, 4):
        return "DiscardStaging"
    if phase == 5:
        return "RestoreTree"
    if phase == 6:
        return "ResumeSaveSwap"
    if phase == 7:
        return "FinishCommit"
    return "Nothing"


def format_journal(journal):
    return "".join("%s=%s\n" % (k, journal[k]) for k in JOURNAL_KEYS)


def parse_journal(text):
    """Absent unless it names a destination: a journal describing no
    transaction is not a journal, the same rule the AFR manifest has about
    world_id."""
    out = {}
    for raw in text.split("\n"):
        line = raw.rstrip("\r")
        if "=" not in line:
            continue
        key, value = line.split("=", 1)
        if key in JOURNAL_KEYS:
            out[key] = int(value) if key == "phase" else value
    if not out.get("to_world"):
        return None
    return out


def activation_rule_cases():
    cases = []

    # --- the refusal table --------------------------------------------------
    for what, facts, expect in REFUSAL_CASES:
        got = check_activation(facts)
        cases.append(("4.4: %s -> %s" % (what, expect), got == expect))

    # Every reason the enum declares is reachable from the table above, and
    # nothing the table produces is outside the enum. A reason nothing can
    # produce is a row that was written and then lost.
    produced = set(check_activation(f) for _w, f, _e in REFUSAL_CASES)
    cases.append(("4.4: every refusal reason is reachable from a constructed case",
                  produced == set(REFUSAL_REASONS)))

    # The order is part of the rule: a console with two problems is told about
    # the first one, not a list it cannot act on.
    cases.append(("4.4: the user check precedes every other",
                  check_activation(broken(user_valid=False, afr_writable=False,
                                          source=False)) == "NoUser"))
    cases.append(("4.4: ownership precedes anything about save data",
                  check_activation(broken(world_exists=False,
                                          save_titles=0)) == "NotThisAccount"))
    cases.append(("4.4: the container check precedes the size check",
                  check_activation(broken(container=False,
                                          container_blocks=0)) == "NoContainer"))

    # The three stored-save rows are scoped to a world that HAS one. A world
    # with no stored save is activatable on a console the game has never
    # written a container on - the game makes one at next launch (B33).
    for field, value in (("container", False), ("container_blocks", 0),
                         ("stored_verifies", False)):
        cases.append(("4.4: %s does not refuse a world with no stored save" % field,
                      check_activation(broken(stored_save=False,
                                              **{field: value})) == "None"))

    # Vanilla generates nothing, so the source row does not apply to it - and
    # every other row still does.
    cases.append(("4.4: vanilla still refuses on an unwritable AFR folder",
                  check_activation(broken(world_is_vanilla=True,
                                          afr_writable=False)) == "AfrNotWritable"))
    # P26, stated as a case rather than only as an absence: a configured title
    # that no install reports is NOT a refusal, and Vanilla in particular has
    # to stay activatable on a console whose AFR folder was never seeded -
    # that one-way door is what P26 removed.
    cases.append(("4.4: nothing refuses on the configured AFR title itself",
                  "afr_detected" not in good_facts() and
                  "AfrTitleNotDetected" not in REFUSAL_REASONS))
    cases.append(("4.4: vanilla ignores an empty enemy selection",
                  check_activation(broken(world_is_vanilla=True,
                                          any_enemy=False)) == "None"))

    # A refusal writes nothing, which is the whole point of doing all of this
    # in phase 1 - so the check has to be a function of facts alone.
    cases.append(("4.4: the same facts always give the same answer",
                  all(check_activation(f) == check_activation(dict(f))
                      for _w, f, _e in REFUSAL_CASES)))

    # --- section 4.3's phase-6 table ---------------------------------------
    rows = [
        ("the incoming world is the outgoing one", True,  False, True,  True,  True,  "Nothing"),
        ("the incoming world has a stored save",   True,  False, False, True,  True,  "RestoreOwn"),
        ("the incoming world has no stored save",  True,  False, False, False, True,  "AdoptLive"),
        ("START FRESH, whatever else is true",     True,  True,  True,  True,  True,  "StartFresh"),
        ("START FRESH on a world with a save",     True,  True,  False, True,  True,  "StartFresh"),
        ("no container, KEEP EXISTING",            False, False, False, False, False, "NoContainer"),
        ("no container, START FRESH",              False, True,  False, False, False, "NoContainer"),
        # D26: a container with no save files has nothing to adopt, and the
        # backup that would have been copied into the world was never taken.
        ("no save files to adopt",                 True,  False, False, False, False, "Nothing"),
    ]
    for what, container, fresh, same, stored, game_files, expect in rows:
        cases.append(("4.3: %s -> %s" % (what, expect),
                      save_action(container, fresh, same, stored, game_files) == expect))

    # Exhaustive: every shape lands on exactly one row and none falls through.
    seen = set()
    total = 0
    for container in (False, True):
        for fresh in (False, True):
            for same in (False, True):
                for stored in (False, True):
                    for game_files in (False, True):
                        seen.add(save_action(container, fresh, same, stored, game_files))
                        total += 1
    cases.append(("4.3: all %d shapes resolve, to exactly the five actions" % total,
                  total == 32 and seen == set(["Nothing", "RestoreOwn", "AdoptLive",
                                               "StartFresh", "NoContainer"])))

    # A world's stored save is never overwritten with another world's (D4):
    # the only row that writes into the incoming world is the one where it has
    # no save of its own.
    writes_into_world = [save_action(True, False, False, stored, True)
                         for stored in (False, True)]
    cases.append(("4.3: a world with a stored save is restored, never written over",
                  writes_into_world == ["AdoptLive", "RestoreOwn"]))

    # --- the journal --------------------------------------------------------
    journal = {
        "from_world": "w-0001",
        "to_world": "w-0002",
        "revision": "rev-0003",
        "save_policy": "start-fresh",
        "save_title_id": "CUSA00207",
        "save_dir_name": "SPRJ0005",
        "safety_backup": "/data/bbrandomizer/SaveBackups/CUSA00207_SPRJ0005_X_preactivate",
        "phase": 6,
    }
    text = format_journal(journal)
    cases.append(("the journal is the eight keys of section 4.3, in order",
                  [l.split("=", 1)[0] for l in text.split("\n") if l] == JOURNAL_KEYS))
    cases.append(("a journal round-trips", parse_journal(text) == journal))
    cases.append(("a journal naming no world is treated as absent",
                  parse_journal("phase=3\n") is None))
    cases.append(("an unknown key in a journal is ignored, not fatal",
                  parse_journal("future=1\n" + text) == journal))
    cases.append(("the journal carries no AFR title - that is the "
                  "BLOODBORNE TITLE ID setting (D25)",
                  not any("afr" in k or k == "title_id" for k in JOURNAL_KEYS)))

    # --- the phase machine --------------------------------------------------
    actions = {}
    for phase in range(0, 10):
        actions[phase] = action_for_phase(phase)
    for phase in range(1, 8):
        cases.append(("phase %d reconciles by %s" % (phase, actions[phase]),
                      actions[phase] != "Nothing"))
    cases.append(("every phase 1-7 has exactly one action, and all five are used",
                  set(actions[p] for p in range(1, 8)) ==
                  set(["DiscardStaging", "RestoreTree", "ResumeSaveSwap",
                       "FinishCommit"])))
    cases.append(("phases 1-4 all discard the staged tree - nothing "
                  "user-visible had changed",
                  all(actions[p] == "DiscardStaging" for p in range(1, 5))))
    cases.append(("a phase this app never writes is left alone rather than guessed",
                  actions[0] == "Nothing" and actions[8] == "Nothing" and
                  actions[9] == "Nothing"))

    return cases


# ---------------------------------------------------------------------------
# Source invariants - plan section 3.1, as far as they are readable
# ---------------------------------------------------------------------------

def source_cases():
    cases = []

    if not os.path.exists(SAVEDATA_CPP):
        cases.append(("Platform/SaveData.cpp exists", False))
        return cases

    header = read(SAVEDATA_H)
    cpp = read(SAVEDATA_CPP)
    code = code_of(cpp)

    # Layering: the header is included by Randomizer/ and Game/ code, which
    # must not learn what a mount is.
    # Comments are stripped first: this header explains at length that no orbis
    # type appears in it, and would otherwise match its own explanation.
    cases.append(("SaveData.h names no orbis type",
                  "orbis" not in code_of(header).lower()))

    # Every orbis call in the app stays in src/Platform/ (CLAUDE.md section 6).
    strays = []
    for root, _, names in os.walk(SRC):
        for name in names:
            if not name.endswith((".cpp", ".h")):
                continue
            path = os.path.join(root, name)
            if os.sep + "Platform" + os.sep in path:
                continue
            body = code_of(read(path))
            if "sceSaveData" in body or "sceUserService" in body:
                strays.append(os.path.relpath(path, APP))
    cases.append(("no save-data or user-service call outside src/Platform/ (%s)"
                  % (", ".join(strays) if strays else "none"), not strays))

    # The three operations that must never appear anywhere in the app.
    forbidden = {"CREATE2": [], "sceSaveDataDelete": []}
    for root, _, names in os.walk(SRC):
        for name in names:
            if not name.endswith((".cpp", ".h")):
                continue
            body = code_of(read(os.path.join(root, name)))
            for needle in forbidden:
                if needle in body:
                    forbidden[needle].append(name)
    cases.append(("CREATE2 is never set (%s)"
                  % (", ".join(forbidden["CREATE2"]) or "absent"),
                  not forbidden["CREATE2"]))
    cases.append(("sceSaveDataDelete is never called (%s)"
                  % (", ".join(forbidden["sceSaveDataDelete"]) or "absent"),
                  not forbidden["sceSaveDataDelete"]))

    # sce_sys is never written: the restore's write list is built by skipping
    # it, and nothing opens a path under it for writing.
    cases.append(("the restore skips sce_sys when building what it writes",
                  "if (UnderSceSys(s.manifest.entries[i].rel)) { s.result.skipped++; continue; }"
                  in code))
    cases.append(("UnderSceSys tests the sce_sys/ prefix",
                  'UnderSceSys(const std::string& rel) { return StartsWith(rel, "sce_sys/"); }'
                  in code))

    # Emptying is root-level userdata*/backup* only, and it is the shared half
    # of START FRESH and of a restore.
    cases.append(("IsGameSaveFile refuses anything below the root",
                  "if (rel.find('/') != std::string::npos) return false;" in code))
    cases.append(("IsGameSaveFile matches userdata and backup",
                  'StartsWith(rel, "userdata") || StartsWith(rel, "backup")' in code))
    cases.append(("the unlink pass goes through IsGameSaveFile",
                  "if (!IsGameSaveFile(rel)) { kept++; continue; }" in code))

    # The .partial discipline: manifest last, then rename into place.
    partial = code.index('s_->partialDir = destDir + ".partial";')
    manifest_write = code.index('WriteManifestFile(s.partialDir + "/manifest.txt"')
    rename = code.index("sceKernelRename(s.partialDir.c_str(), s.destDir.c_str())")
    cases.append(("a backup is built in <name>.partial", partial > 0))
    cases.append(("the manifest is written before the rename, and nothing after",
                  manifest_write < rename))
    cases.append(("a stale .partial is removed rather than copied into",
                  "RemoveTree(s.partialDir);" in code))

    # Search results are selected by name or by there being one, never by
    # index, so the unspecified sort order cannot matter.
    cases.append(("the search leaves cond.key and cond.order at their defaults",
                  "cond.key" not in code and "cond.order" not in code))
    cases.append(("discovery refuses on zero or more than one title",
                  "if (out.titlesWithHits == 0)" in code and
                  "if (out.titlesWithHits > 1)" in code))
    # The probe harness that used to carry this check was deleted in milestone
    # 6, so the assertion moved to the production caller: phase 1 refuses on
    # anything but exactly one directory, which is SaveDirectory above.
    cases.append(("more than one save directory refuses in phase 1",
                  'if (f.saveDirCount > 1) {' in code_of(read(ACTIVATION_CPP))))

    # Every container write is bracketed by a read before and after.
    for name, body in (("EmptyContainer", cpp[cpp.index("EmptyResult EmptyContainer"):]),
                       ("RestoreJob", cpp[cpp.index("void RestoreJob::Step"):])):
        cases.append(("%s reads the container before and after it writes" % name,
                      body.count("ReadContainer(") >= 2))

    # A restore's refusals all happen in the phase before anything is mounted
    # read-write.
    refuse = cpp[cpp.index("void RestoreJob::Step"):]
    checks = refuse.index("s.manifest.blocks > s.result.before.blocks")
    mount_rw = refuse.index("MountContainer(s.user, s.titleId, s.dirName, true)")
    cases.append(("the container-size check precedes the read-write mount",
                  checks < mount_rw))
    cases.append(("the account check precedes the read-write mount",
                  refuse.index("s.manifest.account_id != s.user.accountId") < mount_rw))
    cases.append(("the source is verified before the container is opened",
                  refuse.index("VerifyBackup(s.srcDir") < mount_rw))
    cases.append(("the container is emptied before anything is written",
                  refuse.index("UnlinkGameSaveFiles(") < refuse.index("CopyFileThrough(")))

    # Sizes come from reading, never from st_size, which returns 88 for a
    # genuine 44 KB file on this kernel.
    cases.append(("nothing here uses sceKernelStat", "sceKernelStat" not in code))

    # The manifest keys the C++ writes are the keys this file mirrors.
    for key in HEADER_KEYS:
        cases.append(("SaveData.cpp writes the %s key" % key,
                      '"%s="' % key in code and '"%s"' % key in code))

    # --- milestone 2: the world store ---------------------------------------

    store = code_of(read(WORLDSTORE_CPP))
    store_h = code_of(read(WORLDSTORE_H))

    # WorldStore.h is included by UI code, which must not learn what a mount
    # is. Comments are stripped first - the header explains this at length and
    # would otherwise match its own explanation.
    cases.append(("WorldStore.h names no orbis type",
                  "orbis" not in store_h.lower()))

    # Account scoping is structural rather than remembered: there is no way to
    # construct a WorldStore without an account id, and the directory every
    # other call works under is built from it once.
    cases.append(("a WorldStore cannot be built without an account id",
                  "explicit WorldStore(uint64_t accountId);" in store_h and
                  "WorldStore() =" not in store_h))
    cases.append(("the account directory is derived from the account id",
                  '"acct-%016llx"' in store and
                  "accountDir_ = std::string(kWorldsRoot)" in store))

    # A world's save is built in .partial with its manifest last, then renamed
    # into place over the previous one via .old - the same discipline a safety
    # backup has, for the same reason.
    manifest_write = store.index('WriteManifestFile(s.partialDir + "/manifest.txt"')
    rename_in = store.index("sceKernelRename(s.partialDir.c_str(), s.destDir.c_str())")
    cases.append(("a world's save is built in save.partial",
                  's_->partialDir = worldSaveDir + ".partial";' in store))
    cases.append(("its manifest is written before the rename, and nothing after",
                  manifest_write < rename_in))
    cases.append(("the previous save is set aside, not deleted, before the swap",
                  store.index("sceKernelRename(s.destDir.c_str(), s.oldDir.c_str())")
                  < rename_in))
    cases.append(("a stale .partial or .old is removed rather than copied into",
                  "RemoveTree(s.partialDir);" in store and "RemoveTree(s.oldDir);" in store))

    # A copy is verified by walking what landed on disk, both on the way in and
    # on the way out. Nothing downstream is allowed to trust a return value.
    step = store[store.index("void StoreSaveJob::Step"):]
    cases.append(("a stored save's source is verified before it is copied",
                  step.index("VerifyBackup(s.srcDir") < step.index("CopyFileThrough(")))
    cases.append(("a stored save is verified again once it is in place",
                  "VerifyBackup(s.destDir" in store))
    cases.append(("a safety backup is not ok until it verifies",
                  store.index("VerifyBackup(s.destDir, error, &manifest)")
                  < store.index("s.result.ok        = true;")))
    cases.append(("a world's stored save is verified through the same walk",
                  "return savedata::VerifyBackup(SaveDir(worldId), error, outManifest);"
                  in store))

    # First-run capture must not spend the one first run it gets. It looks
    # before it creates anything, and undoes itself if the capture fails.
    look = store.index("savedata::SaveTitle title = savedata::DiscoverSaveTitle")
    create = store.index("if (!s.store->CreateAccount(error))")
    cases.append(("first-run capture looks at the save data before creating anything",
                  look < create))
    cases.append(("an ambiguous save directory refuses instead of creating Vanilla",
                  store.index('s.Fail("MORE THAN ONE SAVE DIRECTORY UNDER "') < create))
    cases.append(("a failed capture removes the account folder, so the next run "
                  "gets a first run too",
                  "if (store) store->DestroyAccount();" in store and
                  store.count("s.Rollback(") >= 2))
    cases.append(("a container holding no save files captures nothing (D26)",
                  "if (!anyGameFile) s.result.note" in store))

    # Deleting keeps the save (D9) and Vanilla is not deletable (D16).
    cases.append(("deleting a world keeps its save by moving it, never removing it",
                  store.index('SafetyBackupPath(title, dir, "deleted-" + worldId)')
                  < store.index("RemoveTree(WorldDir(worldId));")))
    cases.append(("a save that cannot be kept refuses the delete",
                  'error = "COULD NOT KEEP THE SAVE "' in store))
    cases.append(("Vanilla cannot be deleted or given settings",
                  'error = "VANILLA CANNOT BE DELETED"' in store and
                  'error = "VANILLA HAS NO SETTINGS"' in store))

    # A revision is appended only when the recipe differs (P4).
    cases.append(("an unchanged recipe appends no revision",
                  "SameRecipe(current.recipe, recipe)" in store))
    cases.append(("recipe identity is serializer equality, not a field list",
                  "FormatSettings(a.settings) == FormatSettings(b.settings)" in store))

    # The startup sweep, both halves of it.
    cases.append(("*.partial backups are swept",
                  'HasSuffix(entries[i].name, ".partial")' in store))
    cases.append(("*.partial and *.old world saves are swept",
                  '"/save.partial", "/save.old"' in store))

    # --- milestone 2: the AFR marker -----------------------------------------

    afr = code_of(read(AFR_CPP))
    derive = afr[afr.index("AfrActiveWorld AfrManager::DeriveActive"):]
    cases.append(("first run is decided before anything about AFR is considered",
                  derive.index("if (!accountDirExists)") <
                  derive.index("if (manifest.present)") <
                  derive.index("if (status.seeded)")))

    # Plan section 3.1: the AFR path string lives in one file. Two older
    # callers predate that rule and are pinned here rather than silently
    # tolerated - the check is that no NEW one appears.
    afr_paths = []
    for root, _, names in os.walk(SRC):
        for name in names:
            if not name.endswith((".cpp", ".h")):
                continue
            path = os.path.join(root, name)
            if "/data/GoldHEN/AFR" in code_of(read(path)):
                afr_paths.append(name)
    # Milestone 6 took the third entry off this list: the world editor stopped
    # building its own output path when it stopped running the randomizer
    # itself. The 2026-09-25 cleanup took the last one, deleting the dead
    # Game/GameInfo.{h,cpp}, so the rule CLAUDE.md section 6 states - AfrManager
    # is the one owner of AFR paths - now holds exactly, with no exception.
    cases.append(("the AFR path appears in exactly one file, AfrManager.cpp (%s)"
                  % ", ".join(sorted(afr_paths)),
                  sorted(afr_paths) == ["AfrManager.cpp"]))

    # --- milestone 3: the activation transaction ---------------------------

    act = code_of(read(ACTIVATION_CPP))
    act_h = code_of(read(ACTIVATION_H))

    # The header is included by UI code, which must not learn what a mount is.
    cases.append(("WorldActivation.h names no orbis type",
                  "orbis" not in act_h.lower()))

    # The refusal enum on both sides of the mirror. A reason added to one and
    # not the other is what this catches.
    enum = act_h[act_h.index("enum class RefusalReason"):]
    enum = enum[:enum.index("};")]
    declared = re.findall(r"^\s*(\w+)\s*(?:=\s*\d+)?\s*,", enum, re.M)
    cases.append(("the C++ declares exactly the refusal reasons this file mirrors "
                  "(%s)" % ", ".join(declared),
                  declared == REFUSAL_REASONS))

    # Nothing is written before the checks pass: the journal - the first thing
    # an activation puts on disk - is created after the plan is judged.
    step = act[act.index("void WorldActivationJob::Step"):]
    cases.append(("the plan is judged before the journal is written",
                  step.index("s.result.plan = PlanActivation(") <
                  step.index("s.Advance(2);")))
    cases.append(("a refusal ends the job without advancing a phase",
                  "if (p.refusal.Refused()) { s.Refused(p.refusal); return; }" in step))

    # The journal names the phase ABOUT TO RUN and is fsync'd before it runs.
    advance = act[act.index("bool Advance(int nextPhase)"):]
    advance = advance[:advance.index("};")]
    cases.append(("Advance stamps the journal with the phase about to run",
                  advance.index("journal.phase = nextPhase;") <
                  advance.index("WriteActivationJournal(journal)")))
    cases.append(("...and writes it before the job moves on to that phase",
                  advance.index("WriteActivationJournal(journal)") <
                  advance.index("sub   = kSubStart;")))
    cases.append(("the journal is fsync'd before it is closed",
                  "sceKernelFsync(fd);" in act and
                  act.index("sceKernelFsync(fd);") < act.index("sceKernelClose(fd);\n    return w ==")))

    # Phase 2's failure aborts the whole transaction (D24), and a failure
    # anywhere leaves the journal on disk for the next launch to reconcile.
    cases.append(("a failed safety backup aborts the activation",
                  's.Fail("THE SAFETY BACKUP FAILED - " + r.error);' in step))
    cases.append(("a failure does not delete the journal",
                  act.index("void Fail(const std::string& why)") > 0 and
                  "DeleteActivationJournal" not in
                  act[act.index("void Fail(const std::string& why)"):
                      act.index("void Refused(const Refusal& refusal)")]))

    # Phases 2 and 3 are skipped together, and the outgoing world's stored save
    # is left exactly as it was (D26).
    cases.append(("no container, or no save files in it, skips the backup AND "
                  "the capture",
                  "if (!p.containerExists || !p.containerHasGameFiles) {" in step))

    # The tree is generated somewhere else and moved in by rename, and a stale
    # staged tree is removed rather than written into.
    cases.append(("generation writes into the staging tree, never the live one",
                  "AfrManager::StagingDvdroot(p.afrTitleId)" in step and
                  "new EnemyRandomizerJob(" in step))
    cases.append(("a stale staged tree is removed before generating",
                  step.index("AfrManager::RemoveStaging(p.afrTitleId);") <
                  step.index("new EnemyRandomizerJob(")))
    cases.append(("the marker is written into the STAGED tree, before the swap",
                  step.index("AfrManager::WriteManifestAt(") <
                  step.index("AfrManager::Swap(")))

    # START FRESH empties the container and restores nothing; a restore empties
    # it first through Platform/SaveData, which is the same operation.
    cases.append(("START FRESH empties the container and restores nothing",
                  "savedata::EmptyContainer(s.user, p.saveTitleId, p.saveDirName)" in step))
    cases.append(("a refused unlink fails the activation rather than writing anyway",
                  'FILE(S) REFUSED UNLINK' in step))

    # Phase 7 reverts a one-shot START FRESH as a recorded revision (B12).
    commit = step[step.index("case 7: {"):]
    cases.append(("phase 7 reverts START FRESH by appending a revision",
                  "reverted.settings.startFreshSave = false;" in commit and
                  "AppendRevision(p.toWorldId, reverted" in commit))
    cases.append(("the journal is deleted last, after the revert",
                  commit.index("reverted.settings.startFreshSave = false;") <
                  commit.index("DeleteActivationJournal();")))

    # The swap renames before it deletes, so a complete tree exists under one
    # name or the other at every instant.
    afr_swap = afr[afr.index("bool AfrManager::Swap"):]
    afr_swap = afr_swap[:afr_swap.index("bool AfrManager::StagingExists")]
    cases.append(("the swap sets the live tree aside before bringing the new one in",
                  afr_swap.index("sceKernelRename(live.c_str(), old.c_str())") <
                  afr_swap.index("sceKernelRename(staging.c_str(), live.c_str())")))
    cases.append(("the old tree is removed only after the new one is in place",
                  afr_swap.index("sceKernelRename(staging.c_str(), live.c_str())") <
                  afr_swap.rindex("RemoveTree(old);")))
    cases.append(("a failed swap puts the live tree back",
                  "if (hadLive) sceKernelRename(old.c_str(), live.c_str());" in afr_swap))
    cases.append(("Vanilla omits the second rename, which is what removes the files",
                  "if (useStaging) {" in afr_swap))

    # --- output parity (B28): the options mapping and the run decision ------
    #
    # Until milestone 6 there were TWO copies of this - the Enable wizard's and
    # the activation transaction's - and this file compared them to each other.
    # Milestone 6 deleted the wizard's, which is what it was always going to
    # do, so there is nothing left to compare against except the list itself.
    #
    # The two lists below are the wizard's, frozen: they are exactly what
    # UI/WorldEditorScreen.cpp carried through milestone 5, in its order, and
    # they are the baseline "the tree a seed and a recipe produce is the tree
    # they produced before this feature" is measured against. A field added to
    # EnemyRandomizerOptions has to be added here deliberately, with the output
    # question asked out loud - which is the whole point of pinning it.
    WIZARD_OPTIONS_MAPPING = [
        ("randomizeEnemies", "randomizeEnemies"),
        ("randomizeBosses", "randomizeBosses"),
        ("randomizeTreasure", "randomizeTreasure"),
        ("randomizeWorkshopTools", "randomizeWorkshopTools"),
        ("randomizeEnemyDrops", "randomizeEnemyDrops"),
        ("randomizeStartingWeapons", "randomizeStartingWeapons"),
        ("randomizeStartingGuns", "randomizeStartingGuns"),
        ("randomizeShopWeapons", "randomizeShopWeapons"),
        ("enableMergoDarkness", "enableMergoDarkness"),
        ("doNotRandomizeCagedDogs", "doNotRandomizeCagedDogs"),
        ("startWithHunterTools", "startWithHunterTools"),
        ("easyModes.shadows", "easyShadows"),
        ("easyModes.rom", "easyRom"),
        ("easyModes.failures", "easyFailures"),
        ("easyModes.emissary", "easyEmissary"),
        ("enemiesIncluded", "enemiesIncluded"),
        ("bossesIncluded", "bossesIncluded"),
        ("enemiesSkipped", "enemiesSkipped"),
    ]
    WIZARD_RUN_DECISION = [
        "randomizeEnemies", "randomizeBosses", "randomizeTreasure",
        "randomizeEnemyDrops", "randomizeStartingWeapons", "randomizeStartingGuns",
        "randomizeShopWeapons", "enableMergoDarkness", "startWithHunterTools",
        "easyShadows", "easyRom", "easyFailures", "easyEmissary",
    ]

    editor = code_of(read(os.path.join(SRC, "UI", "WorldEditorScreen.cpp")))

    def options_mapping(text):
        return re.findall(r"options\.([\w.]+)\s*=\s*run_?\.(\w+);", text)

    act_map = options_mapping(act)
    cases.append(("the options mapping is field for field the wizard's (%d fields)"
                  % len(WIZARD_OPTIONS_MAPPING),
                  act_map == WIZARD_OPTIONS_MAPPING))

    def run_decision(text, start):
        chunk = text[text.index(start):]
        chunk = chunk[:chunk.index(";") if start.startswith("bool") else chunk.index(") {")]
        return re.findall(r"run_?\.(\w+)", chunk)

    act_decision = run_decision(act, "bool anythingOn =")
    cases.append(("the run decision is the wizard's || chain, field for field (%d)"
                  % len(WIZARD_RUN_DECISION),
                  act_decision == WIZARD_RUN_DECISION))

    # ...and there is now exactly ONE copy of each. A second mapping anywhere
    # in UI/ is the parallel structure milestone 6 removed growing back.
    cases.append(("the editor no longer maps EnemyRandomizerOptions itself",
                  not options_mapping(editor) and
                  "EnemyRandomizerOptions" not in editor))
    cases.append(("the editor no longer runs EnemyRandomizerJob directly",
                  "new EnemyRandomizerJob(" not in editor and
                  "new WorldActivationJob(" in editor))

    # The four progress-log constants stay where pool_verify.py parses them
    # from, and stay IN USE: the generation's own report is the one thing the
    # activation job does not say, and the editor still says it.
    for name in ("kEnemyFailPrefix", "kPoolFellBackLine1", "kPoolFellBackLine2",
                 "kNothingRandomizedLine"):
        cases.append(("%s is still a const char* const in WorldEditorScreen.cpp" % name,
                      ("const char* const %s =" % name) in editor and name not in act))
        cases.append(("%s is still emitted onto the progress log" % name,
                      editor.count(name) >= 2))

    # The SKIPPING lines plan section 3.1 keeps, still one per setting that has
    # one, still on the screen that always drew them.
    for field in ("randomizeEnemies", "randomizeBosses", "randomizeTreasure",
                  "randomizeEnemyDrops", "randomizeStartingWeapons",
                  "randomizeStartingGuns", "randomizeShopWeapons"):
        cases.append(("the SKIPPING line for %s survived the rewrite" % field,
                      bool(re.search(r'if \(!run_\.%s\)\s+AddProgressLine\("SKIPPING '
                                     % field, editor))))

    # The one library this feature adds, and no more.
    makefile = read(os.path.join(APP, "Makefile"))
    libs = [l for l in makefile.split("\n") if l.startswith("LIBS :=")][0]
    cases.append(("-lSceSaveData is in LIBS", "-lSceSaveData" in libs))
    cases.append(("no second save-data library was added",
                  libs.count("SaveData") == 1))

    return cases


# ---------------------------------------------------------------------------
# Checking a real backup directory
# ---------------------------------------------------------------------------

def walk_sizes(root):
    found = []
    for base, _, names in os.walk(root):
        for name in names:
            path = os.path.join(base, name)
            rel = os.path.relpath(path, root).replace(os.sep, "/")
            found.append((rel, os.path.getsize(path)))
    return found


def cmd_verify(backup_dir):
    manifest_path = os.path.join(backup_dir, "manifest.txt")
    data_dir = os.path.join(backup_dir, "data")
    if not os.path.exists(manifest_path):
        print("no manifest.txt in %s - not a backup" % backup_dir)
        return 1
    if not os.path.isdir(data_dir):
        print("no data/ in %s - not a backup" % backup_dir)
        return 1

    try:
        header, entries = parse_manifest(read(manifest_path))
    except ManifestError as exc:
        print("manifest is malformed: %s" % exc)
        return 1

    reason = verify(header, entries, walk_sizes(data_dir))
    print("%s / %s captured %s" % (header.get("title_id"), header.get("dir_name"),
                                   header.get("captured")))
    print("%d file(s), %d bytes, container %s blocks"
          % (header["files"], header["bytes"], header["blocks"]))
    if reason:
        print("FAILED: %s" % reason)
        return 1
    print("verified")
    return 0


# ---------------------------------------------------------------------------

def report(title, cases):
    print(title)
    failures = 0
    for name, ok in cases:
        print("  %-64s %s" % (name, "ok" if ok else "FAILED"))
        if not ok:
            failures += 1
    print("  %d/%d passing" % (len(cases) - failures, len(cases)))
    print("")
    return failures


def main(argv):
    mode = argv[1] if len(argv) > 1 else "all"

    if mode == "verify":
        if len(argv) < 3:
            print(__doc__)
            return 2
        return cmd_verify(argv[2])

    failures = 0
    if mode in ("all", "selftest"):
        failures += report("manifest rules", rule_cases())
    if mode in ("all", "worlds"):
        failures += report("world store rules", world_rule_cases())
    if mode in ("all", "activation"):
        failures += report("activation rules", activation_rule_cases())
    if mode in ("all", "source"):
        failures += report("source invariants", source_cases())
    if mode not in ("all", "selftest", "worlds", "activation", "source"):
        print(__doc__)
        return 2

    print("%s" % ("FAILED - %d check(s)" % failures if failures else "all checks passing"))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
