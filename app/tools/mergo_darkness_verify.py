#!/usr/bin/env python3
"""Checking which lighting state a randomized tree is in.

The setting's whole effect is four bytes inside a zlib-compressed file, so
there is no way to tell a correct run from a broken one by looking at the
output tree. This decodes event 6548972 out of event/common.emevd.dcx and
reports which of the two known states it is in.

    ENABLE MERGO DARKNESS = YES  -> (10000, 5630, 1)   vanilla bytes  -> DARK
    ENABLE MERGO DARKNESS = NO   -> (10000, 99999, 0)  poked          -> LIT

Note which way round that is. The VANILLA bytes give a permanently dark world
and the POKED bytes give the normal game - measured on console in both
directions, and the opposite of what the event decode suggests. The mechanism
is unexplained; see docs/plans/mergo-darkness.md section 2.7.

SpEffect 5630 is a marker the event puts on the player and that Nightmare of
Mensis reads; 99999 does not exist in SpEffectParam, so referencing it makes
the instruction a no-op. What the game then DOES with either state is the part
that is not understood - do not infer it from here.

The structural assertions matter as much as the values: if a future change to
DcxCompress or to the C++ EMEVD walk starts poking the wrong offset, that
shows up here as "instruction 0 is not SetSpEffect" rather than as a silently
wrong game.

Usage:
    python mergo_darkness_verify.py show     <dvdroot>
    python mergo_darkness_verify.py verify   <dvdroot> dark|lit
    python mergo_darkness_verify.py selftest
"""

import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from boss_verify import read_dcx  # noqa: E402

# --- What we expect to find ------------------------------------------------

TARGET_EVENT = 6548972

# (character, speffect, third arg). The third argument moves with the poke and
# is checked too - the reference tool writes both, so a run that changed only
# the SpEffect id would be a partial write worth catching.
DARK_ARGS = (10000, 5630, 1)   # vanilla bytes, untouched -> dark world
LIT_ARGS  = (10000, 99999, 0)  # poked -> normal lit game

SETSPEFFECT_BANK = 2004
SETSPEFFECT_ID   = 8

# EMEVD layout, mirroring src/Randomizer/PermaDarkness.cpp, which in turn
# mirrors SoulsFormats' EMEVD.Read(). Bloodborne is the 64-bit variant, so
# every header field is an 8-byte little-endian value after a 16-byte prefix.
HEADER_PREFIX  = 16
EVENT_SIZE     = 48
INSTR_SIZE     = 32


class EmevdError(Exception):
    pass


def parse_event(plain, target=TARGET_EVENT):
    """Returns the list of (bank, id, args_bytes) for `target`'s instructions."""
    if len(plain) < HEADER_PREFIX:
        raise EmevdError("file is too small to be an EMEVD")
    if plain[:4] != b"EVD\x00":
        raise EmevdError("bad EMEVD magic %r" % plain[:4])
    if plain[5] != 0xFF:
        raise EmevdError("not the 64-bit (Bloodborne) EMEVD format")

    pos = HEADER_PREFIX

    def varint():
        nonlocal pos
        v = struct.unpack_from("<q", plain, pos)[0]
        pos += 8
        return v

    event_count = varint()
    events_off  = varint()
    varint()                      # instruction count
    instrs_off  = varint()
    varint(); varint()            # unknown struct count / offset
    varint(); varint()            # layer count / offset
    varint(); varint()            # parameter count / offset
    varint(); varint()            # linked file count / offset
    varint()                      # argument data length
    args_off    = varint()

    for i in range(event_count):
        base = events_off + i * EVENT_SIZE
        if base + EVENT_SIZE > len(plain):
            raise EmevdError("event table runs past end of file")
        event_id, instr_count, instr_off = struct.unpack_from("<qqq", plain, base)[:3]
        if event_id != target:
            continue
        out = []
        for j in range(instr_count):
            ib = instrs_off + instr_off + j * INSTR_SIZE
            if ib + INSTR_SIZE > len(plain):
                raise EmevdError("instruction table runs past end of file")
            bank, idx = struct.unpack_from("<ii", plain, ib)
            arg_len   = struct.unpack_from("<q", plain, ib + 8)[0]
            arg_off   = struct.unpack_from("<i", plain, ib + 16)[0]
            start     = args_off + arg_off
            out.append((bank, idx, plain[start:start + arg_len]))
        return out

    raise EmevdError("event %d not found" % target)


def _arg_data_pos(plain):
    """Byte offset of event 6548972's first instruction's argument data.

    Mirrors PermaDarkness.cpp's own walk so the selftest pokes exactly where
    the C++ does, rather than trusting parse_event's copy of the bytes.
    """
    pos = HEADER_PREFIX
    vals = []
    for _ in range(16):
        vals.append(struct.unpack_from("<q", plain, pos)[0])
        pos += 8
    events_off, instrs_off, args_off = vals[1], vals[3], vals[13]
    for i in range(vals[0]):
        base = events_off + i * EVENT_SIZE
        event_id, _ic, instr_off = struct.unpack_from("<qqq", plain, base)[:3]
        if event_id != TARGET_EVENT:
            continue
        ib = instrs_off + instr_off
        return args_off + struct.unpack_from("<i", plain, ib + 16)[0]
    raise EmevdError("event %d not found" % TARGET_EVENT)


def read_state(dvdroot):
    """Returns (args_tuple, instructions). Raises EmevdError on anything odd."""
    path = os.path.join(dvdroot, "event", "common.emevd.dcx")
    if not os.path.exists(path):
        raise EmevdError("%s does not exist" % path)
    instrs = parse_event(read_dcx(path))

    if len(instrs) != 2:
        raise EmevdError("event %d has %d instructions, expected 2"
                         % (TARGET_EVENT, len(instrs)))
    bank, idx, args = instrs[0]
    if (bank, idx) != (SETSPEFFECT_BANK, SETSPEFFECT_ID):
        raise EmevdError("instruction 0 is %d[%02d], expected %d[%02d] SetSpEffect"
                         % (bank, idx, SETSPEFFECT_BANK, SETSPEFFECT_ID))
    if len(args) != 12:
        raise EmevdError("instruction 0 has %d argument bytes, expected 12" % len(args))

    return struct.unpack_from("<iii", args, 0), instrs


def describe(args):
    if args == DARK_ARGS:
        return "VANILLA BYTES - world runs DARK (ENABLE MERGO DARKNESS = YES)"
    if args == LIT_ARGS:
        return "POKED - world runs LIT, the normal game (ENABLE MERGO DARKNESS = NO)"
    return "UNRECOGNISED - neither known state"


# --- Commands --------------------------------------------------------------

def cmd_show(dvdroot):
    try:
        args, instrs = read_state(dvdroot)
    except EmevdError as e:
        print("ERROR: %s" % e)
        return 2
    print("event %d in %s" % (TARGET_EVENT, os.path.join(dvdroot, "event", "common.emevd.dcx")))
    for j, (bank, idx, raw) in enumerate(instrs):
        ints = [struct.unpack_from("<i", raw, k)[0] for k in range(0, len(raw) - 3, 4)]
        print("  [%d] %d[%02d] args=%s  bytes=%s"
              % (j, bank, idx, ints, " ".join("%02x" % b for b in raw)))
    print()
    print("  state: %s" % describe(args))
    return 0


def cmd_verify(dvdroot, expected):
    # Named outcome rather than a flag: every short flag for this ("--disabled",
    # "--off") reads ambiguously now that the poke and the outcome are inverted.
    if expected not in ("dark", "lit"):
        print("expected outcome must be 'dark' or 'lit', got %r" % expected)
        return 2
    want = DARK_ARGS if expected == "dark" else LIT_ARGS
    label = expected.upper()
    try:
        args, _ = read_state(dvdroot)
    except EmevdError as e:
        print("FAIL: %s" % e)
        return 1
    if args != want:
        print("FAIL: expected %s %s, found %s" % (label, want, args))
        print("      %s" % describe(args))
        return 1
    print("PASS: %s" % describe(args))
    return 0


def cmd_selftest(dvdroot=None):
    """Pins both directions of the setting, on synthetic payloads.

    Needs no game files: it builds a minimal EMEVD around the one event this
    tool reads, which is enough to prove the decoder finds the right bytes and
    that the two states are told apart rather than both passing.
    """
    def build(speffect, third, bank=SETSPEFFECT_BANK, idx=SETSPEFFECT_ID,
              arg_len=12, instr_count=2):
        args_blob = struct.pack("<iii", 10000, speffect, third)
        events_off = HEADER_PREFIX + 16 * 8
        instrs_off = events_off + EVENT_SIZE
        args_start = instrs_off + INSTR_SIZE * instr_count

        buf = bytearray(b"EVD\x00" + bytes([0, 0xFF, 1, 0]) + b"\x00" * 8)
        for v in (1, events_off, instr_count, instrs_off, 0, 0, 0, 0,
                  0, 0, 0, 0, len(args_blob), args_start, 0, 0):
            buf += struct.pack("<q", v)
        ev = bytearray(EVENT_SIZE)
        struct.pack_into("<qqq", ev, 0, TARGET_EVENT, instr_count, 0)
        buf += ev
        for j in range(instr_count):
            ins = bytearray(INSTR_SIZE)
            struct.pack_into("<ii", ins, 0, bank if j == 0 else 1000, idx if j == 0 else 4)
            struct.pack_into("<q", ins, 8, arg_len if j == 0 else 0)
            struct.pack_into("<i", ins, 16, 0)
            buf += ins
        buf += args_blob
        return bytes(buf)

    cases = []

    vanilla  = parse_event(build(5630, 1))
    disabled = parse_event(build(99999, 0))
    cases.append(("vanilla bytes decode to the DARK state",
                  struct.unpack_from("<iii", vanilla[0][2], 0) == DARK_ARGS))
    cases.append(("poked bytes decode to the LIT state",
                  struct.unpack_from("<iii", disabled[0][2], 0) == LIT_ARGS))
    cases.append(("the two states are not equal",
                  DARK_ARGS != LIT_ARGS))

    # A partial write - SpEffect changed but the third argument left behind -
    # must not be mistaken for either state. This is the specific bug the
    # third-argument check exists to catch.
    partial = struct.unpack_from("<iii", parse_event(build(99999, 1))[0][2], 0)
    cases.append(("partial write matches neither state",
                  partial not in (DARK_ARGS, LIT_ARGS)))

    def raises(payload):
        try:
            parse_event(payload)
            return False
        except EmevdError:
            return True

    cases.append(("bad magic is rejected", raises(b"XXXX" + b"\x00" * 200)))
    cases.append(("32-bit EMEVD is rejected",
                  raises(b"EVD\x00" + bytes([0, 0x00, 1, 0]) + b"\x00" * 200)))

    # --- both directions against the REAL file, if one was given ----------
    # ApplyPermaDarkness's true branch was dead code until the app started
    # writing both states, so this exercises it: poke the real payload each
    # way and read the result back through the same decoder the tool uses.
    if dvdroot:
        import os as _os
        path = _os.path.join(dvdroot, "event", "common.emevd.dcx")
        if _os.path.exists(path):
            plain = bytearray(read_dcx(path))
            pos = _arg_data_pos(plain)

            def poked(on):
                b = bytearray(plain)
                vals = (254, 21, 0, 1) if on else (159, 134, 1, 0)
                b[pos + 4], b[pos + 5], b[pos + 6], b[pos + 8] = vals
                return struct.unpack_from("<iii", parse_event(bytes(b))[0][2], 0)

            cases.append(("real file, permaDarknessOn=true  -> DARK state",
                          poked(True) == DARK_ARGS))
            cases.append(("real file, permaDarknessOn=false -> LIT state",
                          poked(False) == LIT_ARGS))
            cases.append(("true branch is a no-op on an untouched vanilla file",
                          poked(True) == struct.unpack_from(
                              "<iii", parse_event(bytes(plain))[0][2], 0)))

    failures = 0
    for name, ok in cases:
        print("  %-42s %s" % (name, "ok" if ok else "FAILED"))
        if not ok:
            failures += 1
    print("%d/%d passing" % (len(cases) - failures, len(cases)))
    return 1 if failures else 0


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 2
    mode = argv[1]
    if mode == "selftest":
        return cmd_selftest(argv[2] if len(argv) > 2 else None)
    if len(argv) < 3:
        print(__doc__)
        return 2
    dvdroot = argv[2]
    if mode == "show":
        return cmd_show(dvdroot)
    if mode == "verify":
        return cmd_verify(dvdroot, argv[3] if len(argv) > 3 else "")
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv))
