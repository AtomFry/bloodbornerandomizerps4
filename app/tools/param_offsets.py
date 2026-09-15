#!/usr/bin/env python3
"""Offline PARAM inspection: turn cell indices into field names and byte offsets.

This runs on the PC, never on the PS4. Its whole purpose is to let the PS4 app
avoid shipping a PARAMDEF parser: the app only ever needs a byte offset within a
fixed-size row, and that offset can be computed once here and baked in as a
constant (the same approach already used for ModelSizeTable.h / NpcScalingTable.h).

Cell index == paramdef field index, one-to-one, including padding (dummy8) and
bit fields - confirmed from SoulsFormats PARAM.Row.ReadCells, which allocates
`new Cell[paramdef.Fields.Count]` and fills `cells[i]` per field.

Usage:
    python param_offsets.py fields  <vanilla_dvdroot> <PARAM_TYPE> [cell ...]
    python param_offsets.py rows    <vanilla_dvdroot> <param file name>
    python param_offsets.py upgrade <vanilla_dvdroot>
"""

import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from boss_verify import read_dcx  # noqa: E402

SEP = chr(92)

# --- BND4 (flat container; all Bloodborne param members are uncompressed) ---

BND4_HEADER_SIZE = 64
BND4_FILE_HEADER_SIZE = 36


def bnd4_members(buf):
    """Yields (name, data_offset, size). Layout per SoulsFormats BND4/
    BinderFileHeader with format 0x74: 4-byte offsets, 8-byte sizes, ids, names."""
    count = struct.unpack_from("<i", buf, 12)[0]
    for k in range(count):
        e = BND4_HEADER_SIZE + k * BND4_FILE_HEADER_SIZE
        size = struct.unpack_from("<q", buf, e + 16)[0]
        offset = struct.unpack_from("<I", buf, e + 24)[0]
        name_offset = struct.unpack_from("<I", buf, e + 32)[0]
        end = name_offset
        while buf[end:end + 2] != b"\x00\x00":
            end += 2
        name = buf[name_offset:end].decode("utf-16-le").replace(SEP, "/").split("/")[-1]
        yield name, offset, size


# --- PARAMDEF --------------------------------------------------------------

VALUE_SIZE = {"s8": 1, "u8": 1, "dummy8": 1, "s16": 2, "u16": 2,
              "s32": 4, "u32": 4, "f32": 4, "fixstr": 1, "fixstrW": 2}
BIT_TYPES = {"u8", "u16", "u32", "dummy8"}
BIT_LIMIT = {"u8": 8, "u16": 16, "u32": 32}


def _fixstr(buf, off, size):
    return buf[off:off + size].split(b"\x00")[0].decode("ascii", "replace")


def parse_paramdef(buf):
    field_count = struct.unpack_from("<h", buf, 8)[0]
    field_size = struct.unpack_from("<h", buf, 10)[0]
    param_type = _fixstr(buf, 12, 0x20)
    unicode_names = buf[45] != 0
    version = struct.unpack_from("<h", buf, 46)[0]
    base = 56 if version >= 201 else 48

    fields = []
    for i in range(field_count):
        f = base + i * field_size
        disp_len = 0x40
        dtype = _fixstr(buf, f + disp_len, 8)
        byte_count = struct.unpack_from("<i", buf, f + disp_len + 36)[0]
        desc_size = 8 if version >= 201 else 4
        itype_off = f + disp_len + 40 + desc_size
        internal_name = ""
        if version >= 102:
            internal_name = _fixstr(buf, itype_off + 0x20, 0x20).strip()
        bit_size = -1
        if ":" in internal_name:
            internal_name, _, bits = internal_name.partition(":")
            try:
                bit_size = int(bits)
            except ValueError:
                bit_size = -1
        vsize = VALUE_SIZE.get(dtype, 1)
        fields.append({
            "index": i,
            "name": internal_name or _fixstr(buf, f, disp_len),
            "type": dtype,
            "array_len": byte_count // vsize if vsize else byte_count,
            "bit_size": bit_size,
        })
    return {"param_type": param_type, "version": version,
            "unicode": unicode_names, "fields": fields}


def field_offsets(defn):
    """Replays SoulsFormats' ReadCells byte-advance to get each cell's offset."""
    out = []
    pos = 0
    bit_offset = -1
    bit_type = "u8"
    for f in defn["fields"]:
        t = f["type"]
        is_bit = t in BIT_TYPES and f["bit_size"] != -1
        if not is_bit:
            bit_offset = -1
            start = pos
            if t in ("fixstr",):
                pos += f["array_len"]
            elif t in ("fixstrW",):
                pos += f["array_len"] * 2
            elif t == "dummy8":
                pos += f["array_len"]
            else:
                pos += VALUE_SIZE.get(t, 4)
            out.append((f, start, pos - start))
        else:
            new_bit_type = "u8" if t == "dummy8" else t
            limit = BIT_LIMIT[new_bit_type]
            if bit_offset == -1 or new_bit_type != bit_type or bit_offset + f["bit_size"] > limit:
                bit_offset = 0
                bit_type = new_bit_type
                start = pos
                pos += VALUE_SIZE[new_bit_type]
            else:
                start = pos - VALUE_SIZE[bit_type]
            out.append((f, start, 0))  # width 0 marks a packed bit field
            bit_offset += f["bit_size"]
    return out, pos


def load_defs(root):
    buf = read_dcx(os.path.join(root, "paramdef", "paramdef.paramdefbnd.dcx"))
    defs = {}
    for name, off, size in bnd4_members(buf):
        d = parse_paramdef(buf[off:off + size])
        defs[d["param_type"]] = d
    return defs


def load_param(root, member_name):
    buf = read_dcx(os.path.join(root, "param", "gameparam", "gameparam.parambnd.dcx"))
    for name, off, size in bnd4_members(buf):
        if name == member_name:
            return buf[off:off + size]
    return None


def param_rows(p):
    """Yields (row_id, data_offset). Descriptors start at 0x40, 24-byte stride."""
    row_count = struct.unpack_from("<H", p, 0x0A)[0]
    for r in range(row_count):
        o = 0x40 + r * 24
        yield struct.unpack_from("<i", p, o)[0], struct.unpack_from("<q", p, o + 8)[0]


# --- Commands --------------------------------------------------------------

def cmd_fields(root, param_type, cells):
    defs = load_defs(root)
    if param_type not in defs:
        print("unknown param type %r; available include: %s"
              % (param_type, ", ".join(sorted(defs)[:8])))
        return 2
    d = defs[param_type]
    offs, row_size = field_offsets(d)
    print("%s  version=%d  fields=%d  computed row size=%d bytes"
          % (param_type, d["version"], len(d["fields"]), row_size))
    print()
    if not cells:
        cells = range(min(20, len(offs)))
    print("  %-6s %-34s %-8s %-8s %s" % ("cell", "field", "type", "offset", "note"))
    for c in cells:
        c = int(c)
        if c >= len(offs):
            print("  %-6d OUT OF RANGE" % c)
            continue
        f, start, width = offs[c]
        note = "packed bits (%d)" % f["bit_size"] if width == 0 else ""
        print("  %-6d %-34s %-8s %-8d %s" % (c, f["name"], f["type"], start, note))
    return 0


def cmd_rows(root, member):
    p = load_param(root, member)
    if p is None:
        print("member %r not found" % member)
        return 2
    ptype = _fixstr(p, 0x0C, 0x20)
    rows = list(param_rows(p))
    offs = [o for _, o in rows]
    strides = {offs[i + 1] - offs[i] for i in range(min(len(offs) - 1, 200))}
    print("%s: paramType=%s rows=%d  row stride(s) seen=%s"
          % (member, ptype, len(rows), sorted(strides)))
    print("  first ids: %s" % [rid for rid, _ in rows[:10]])
    return 0


def cmd_upgrade(root):
    """Open question #4: is the reference's '+100 per upgrade tier' real?"""
    p = load_param(root, "EquipParamWeapon.param")
    ids = {rid for rid, _ in param_rows(p)}
    print("EquipParamWeapon rows: %d" % len(ids))
    starters = [7000000, 5000000, 22000000, 14000000, 6000000]
    print()
    print("  checking base + 100*n for n in 1..10 (the reference's assumption):")
    all_ok = True
    for base in starters:
        present = [n for n in range(1, 11) if base + 100 * n in ids]
        ok = len(present) == 10
        all_ok = all_ok and ok
        print("    %-10d base_present=%-5s tiers_found=%2d/10  %s"
              % (base, base in ids, len(present), "OK" if ok else "MISMATCH"))
    print()
    print("  verdict: %s" % ("+100 stride holds for all sampled starters"
                             if all_ok else "stride does NOT hold - see above"))
    return 0 if all_ok else 1


def main():
    if len(sys.argv) >= 4 and sys.argv[1] == "fields":
        return cmd_fields(sys.argv[2], sys.argv[3], sys.argv[4:])
    if len(sys.argv) >= 4 and sys.argv[1] == "rows":
        return cmd_rows(sys.argv[2], sys.argv[3])
    if len(sys.argv) >= 3 and sys.argv[1] == "upgrade":
        return cmd_upgrade(sys.argv[2])
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main())
