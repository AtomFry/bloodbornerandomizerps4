#!/usr/bin/env python3
"""FMG (FromSoftware message file) reader - the game's own display text.

This runs on the PC, never on the PS4. Its only consumer today is
gen_weapon_table.py, which needs the name Bloodborne itself gives a weapon id
so a picker row cannot disagree with what the inventory says. Composing
"UNCANNY " + a base name gives the wrong string for four of the 26 trick
weapons (see docs/features/037-start-with-trick-weapon/plan-evidence.md M5), so
the names are read and never assembled.

Bloodborne's FMGs are the WIDE (64-bit) variant, version 2:

    0x00  u8   big-endian flag (0 here)
    0x01  u8   bom/pad
    0x02  u8   version           2 = wide
    0x04  s32  file size         == the container member's size exactly
    0x08  s32  1
    0x0C  s32  group count
    0x10  s32  string count
    0x14  s32  255
    0x18  s64  string-offset-table address == 0x28 + 16 * group_count
    0x28       group table, 16 bytes per group: s32 offset_index,
               s32 first_id, s32 last_id, s32 pad
               ... then the string offset table, 8 bytes each, UTF-16LE
               strings. Offset 0 means "this id has no string".

Both of those equalities are exact on the shipped files, so they are checked
rather than assumed - a layout mistake would otherwise read as a plausible-
looking wrong name.

NOTE ON PRINTING. The Windows console here is cp1252. Printing a weapon name,
or the Japanese member name an FMG lives under, raises UnicodeEncodeError that
reads like a parse failure and is not (docs/known-traps.md). Callers write
files or ASCII; this module prints nothing.

Usage (diagnostic only):
    PYTHONIOENCODING=utf-8 python fmg.py <vanilla_dvdroot> [id ...]
"""

import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from boss_verify import read_dcx  # noqa: E402
from param_offsets import bnd4_members  # noqa: E402

# msg/engus/item.msgbnd.dcx member names are Japanese. Spelled as escapes so
# this file stays ASCII and greps cleanly on a cp1252 console.
WEAPON_NAME_FMG = "武器名.fmg"         # weapon names
WEAPON_DESC_FMG = "武器説明.fmg"   # weapon descriptions

ITEM_MSGBND = os.path.join("msg", "engus", "item.msgbnd.dcx")

GROUP_TABLE_OFFSET = 0x28
GROUP_ENTRY_SIZE = 16
OFFSET_ENTRY_SIZE = 8


class FmgError(Exception):
    pass


def parse_fmg(buf, member_size=None):
    """Returns {id: string}. Ids whose offset is 0 carry no string and are
    omitted. `member_size`, when given, is checked against the header's own
    file-size field - one of the two framing equalities."""
    version = buf[0x02]
    if version != 2:
        raise FmgError("FMG version %d is not the wide variant this reads" % version)
    if buf[0x00] != 0:
        raise FmgError("FMG claims big-endian; Bloodborne's are little-endian")

    file_size = struct.unpack_from("<i", buf, 0x04)[0]
    group_count = struct.unpack_from("<i", buf, 0x0C)[0]
    string_count = struct.unpack_from("<i", buf, 0x10)[0]
    offset_table = struct.unpack_from("<q", buf, 0x18)[0]

    # Framing check 1: the header's file size is the member's size.
    if member_size is not None and file_size != member_size:
        raise FmgError("FMG file size %d != container member size %d"
                       % (file_size, member_size))
    # Framing check 2: the string offset table starts exactly after the groups.
    expect = GROUP_TABLE_OFFSET + GROUP_ENTRY_SIZE * group_count
    if offset_table != expect:
        raise FmgError("FMG offset table at %d, expected %d (0x28 + 16 * %d)"
                       % (offset_table, expect, group_count))

    out = {}
    for g in range(group_count):
        e = GROUP_TABLE_OFFSET + g * GROUP_ENTRY_SIZE
        index, first_id, last_id = struct.unpack_from("<iii", buf, e)
        for n, mid in enumerate(range(first_id, last_id + 1)):
            o = offset_table + (index + n) * OFFSET_ENTRY_SIZE
            str_off = struct.unpack_from("<q", buf, o)[0]
            if str_off == 0:
                continue  # no string for this id; not the same as an empty one
            end = str_off
            while buf[end:end + 2] != b"\x00\x00":
                end += 2
            out[mid] = buf[str_off:end].decode("utf-16-le")
    if string_count != group_span(buf, group_count):
        raise FmgError("FMG string count %d does not match the group spans"
                       % string_count)
    return out


def group_span(buf, group_count):
    """Total ids covered by the group table - equals the header's string count
    on the shipped files."""
    total = 0
    for g in range(group_count):
        e = GROUP_TABLE_OFFSET + g * GROUP_ENTRY_SIZE
        _index, first_id, last_id = struct.unpack_from("<iii", buf, e)
        total += last_id - first_id + 1
    return total


def load_fmg(root, member_name, container=ITEM_MSGBND):
    """Reads one FMG member out of a msgbnd container. Returns
    (entries, id_count) where id_count is every id the groups cover, including
    the ones with no string."""
    buf = read_dcx(os.path.join(root, container))
    for name, off, size in bnd4_members(buf):
        if name == member_name:
            data = buf[off:off + size]
            entries = parse_fmg(data, member_size=size)
            count = group_span(data, struct.unpack_from("<i", data, 0x0C)[0])
            return entries, count
    raise FmgError("member not found in %s" % container)


def weapon_names(root):
    """{weapon id: name as the game spells it}. Ids with no string are absent."""
    entries, _count = load_fmg(root, WEAPON_NAME_FMG)
    return entries


def weapon_descriptions(root):
    entries, _count = load_fmg(root, WEAPON_DESC_FMG)
    return entries


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    root = sys.argv[1]
    entries, count = load_fmg(root, WEAPON_NAME_FMG)
    print("weapon-name FMG: %d ids, %d with a string" % (count, len(entries)))
    for a in sys.argv[2:]:
        wid = int(a)
        name = entries.get(wid)
        print("  %-10d %s" % (wid, "<none>" if name is None else name.upper()))
    return 0


if __name__ == "__main__":
    sys.exit(main())
