#!/usr/bin/env python3
"""Validation for the item-data (gameparam) round trip - increment 1.

The question this answers is narrow: can the PS4 app read the game's item-data
archive and write it back out without corrupting it?

  roundtrip - emulates the PS4 write path (src/Msb/Zlib.cpp's stored-block
              deflate + src/Msb/Dcx.cpp's DCX header) against the real vanilla
              file, then decompresses the result with a standards-compliant
              inflater and requires it to be byte-identical to the input. This
              is a PC-side proof that the bytes we intend to emit are valid at
              28 MB scale; it cannot prove the game accepts them.

  verify    - compares a REAL PS4-produced tree against vanilla: both
              gameparam files are decompressed and their payloads must be
              byte-identical, since increment 1 changes nothing inside.

Usage:
    python itemdata_verify.py roundtrip <vanilla_dvdroot>
    python itemdata_verify.py verify    <vanilla_dvdroot> <output_dvdroot>
"""

import os
import struct
import sys
import zlib

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from boss_verify import read_dcx  # noqa: E402
from param_offsets import bnd4_members  # noqa: E402

REL = os.path.join("param", "gameparam", "gameparam.parambnd.dcx")


def adler32(data):
    return zlib.adler32(data) & 0xFFFFFFFF


def deflate_stored(data):
    """Mirror of ZlibDeflateStored in src/Msb/Zlib.cpp."""
    out = bytearray([0x78, 0x01])
    n = len(data)
    off = 0
    while True:
        chunk = min(n - off, 65535)
        last = 1 if off + chunk >= n else 0
        out.append(last)
        out += struct.pack("<HH", chunk, (~chunk) & 0xFFFF)
        out += data[off:off + chunk]
        off += chunk
        if off >= n:
            break
    out += struct.pack(">I", adler32(data))
    return bytes(out)


def dcx_compress(plain):
    """Mirror of DcxCompress in src/Msb/Dcx.cpp."""
    z = deflate_stored(plain)
    out = bytearray(b"DCX\x00")
    for v in (0x10000, 0x18, 0x24, 0x44, 0x4C):
        out += struct.pack(">I", v)
    out += b"DCS\x00"
    out += struct.pack(">I", len(plain))
    out += struct.pack(">I", len(z))
    out += b"DCP\x00" + b"DFLT"
    for v in (0x20, 0x9000000, 0, 0, 0, 0x00010100):
        out += struct.pack(">I", v)
    out += b"DCA\x00"
    out += struct.pack(">I", 8)
    out += z
    return bytes(out)


def summarize(plain, label):
    members = list(bnd4_members(plain))
    print("  %s: %d bytes, %d archive entries" % (label, len(plain), len(members)))
    for want in ("NpcParam.param", "EquipParamWeapon.param", "ShopLineupParam.param"):
        hit = [m for m in members if m[0] == want]
        if hit:
            _, off, size = hit[0]
            print("      %-24s offset=%-9d size=%d" % (want, off, size))
        else:
            print("      %-24s MISSING" % want)
    return members


def cmd_roundtrip(vanilla_root):
    src = os.path.join(vanilla_root, REL)
    on_disk = os.path.getsize(src)
    plain = read_dcx(src)
    print("vanilla item-data archive")
    print("  on disk (compressed): %d bytes" % on_disk)
    members = summarize(plain, "decompressed")

    print()
    print("emulating the PS4 write path (stored-block deflate + DCX header)...")
    emitted = dcx_compress(plain)
    print("  would write: %d bytes (%.1f MB), %.1fx the vanilla file"
          % (len(emitted), len(emitted) / 1048576.0, len(emitted) / float(on_disk)))

    # The decisive check: a standards-compliant inflater must recover the input.
    body = emitted[76:]
    try:
        back = zlib.decompress(body)
    except Exception as e:
        print("  FAIL  emitted stream did not inflate: %s" % e)
        return 1

    ok = back == plain
    print("  %s  round trip byte-identical (%d bytes back)"
          % ("PASS" if ok else "FAIL", len(back)))
    if not ok:
        return 1

    # And the recovered payload must still parse as the same archive.
    members2 = list(bnd4_members(back))
    same = [m[0] for m in members] == [m[0] for m in members2]
    print("  %s  archive still lists the same %d entries after the round trip"
          % ("PASS" if same else "FAIL", len(members2)))

    blocks = (len(plain) + 65534) // 65535
    print()
    print("  note: %d stored deflate blocks at 65535 bytes each - the map files"
          % blocks)
    print("        already exercise multi-block output, this is the same path at scale")
    return 0 if (ok and same) else 1


def cmd_verify(vanilla_root, output_root):
    vpath = os.path.join(vanilla_root, REL)
    opath = os.path.join(output_root, REL)
    if not os.path.exists(opath):
        print("no item-data file in output tree: %s" % opath)
        print("(expected when REWRITE ITEM DATA was off - that is a valid result)")
        return 0

    vplain = read_dcx(vpath)
    oplain = read_dcx(opath)
    print("vanilla  on disk=%d decompressed=%d" % (os.path.getsize(vpath), len(vplain)))
    print("output   on disk=%d decompressed=%d" % (os.path.getsize(opath), len(oplain)))
    print()

    failures = []
    if len(vplain) != len(oplain):
        failures.append("payload length changed %d -> %d" % (len(vplain), len(oplain)))
    elif vplain != oplain:
        diffs = [i for i in range(0, len(vplain), 4096) if vplain[i:i + 4096] != oplain[i:i + 4096]]
        failures.append("payload differs (increment 1 must change nothing); "
                        "first differing 4K block at offset %d, %d blocks differ"
                        % (diffs[0] if diffs else -1, len(diffs)))

    vm = [m[0] for m in bnd4_members(vplain)]
    om = [m[0] for m in bnd4_members(oplain)]
    if vm != om:
        failures.append("archive entry list changed (%d -> %d entries)" % (len(vm), len(om)))

    if failures:
        print("FAILURES:")
        for f in failures:
            print("  " + f)
        return 1
    print("PASS  output item-data is byte-identical to vanilla after decompression")
    print("      (%d entries, %d bytes) - only the compression wrapper differs" % (len(om), len(oplain)))
    return 0


def main():
    if len(sys.argv) >= 3 and sys.argv[1] == "roundtrip":
        return cmd_roundtrip(sys.argv[2])
    if len(sys.argv) >= 4 and sys.argv[1] == "verify":
        return cmd_verify(sys.argv[2], sys.argv[3])
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main())
