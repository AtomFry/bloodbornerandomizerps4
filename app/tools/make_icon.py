#!/usr/bin/env python3
"""Generate sce_sys/icon0.png - the PS4 home-screen tile.

A plain 512x512 PNG: PS4 blue background, "BLOODBORNE" / "RANDOMIZER"
stamped on in white using the exact same glyph bitmaps as the in-app font
(src/Platform/Font8x8.cpp), so the tile and the app's own UI look like the
same typeface. Stdlib only - no Pillow, no downloads.

    python3 tools/make_icon.py
"""

import struct
import sys
import zlib
from pathlib import Path

SIZE = 512

BG = (18, 66, 145)     # PS4-blue
FG = (240, 242, 245)   # near-white text

# Same 8x8 glyph bitmaps as src/Platform/Font8x8.cpp - only the letters
# BLOODBORNE / RANDOMIZER need. MSB = leftmost pixel, one byte per row.
GLYPHS = {
    ' ': (0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00),
    'A': (0x18, 0x3C, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x00),
    'B': (0x7C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x7C, 0x00),
    'D': (0x78, 0x6C, 0x66, 0x66, 0x66, 0x6C, 0x78, 0x00),
    'E': (0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x7E, 0x00),
    'I': (0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x00),
    'L': (0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7E, 0x00),
    'M': (0x63, 0x77, 0x7F, 0x6B, 0x63, 0x63, 0x63, 0x00),
    'N': (0x63, 0x73, 0x7B, 0x6F, 0x67, 0x63, 0x63, 0x00),
    'O': (0x3C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00),
    'R': (0x7C, 0x66, 0x66, 0x7C, 0x6C, 0x66, 0x63, 0x00),
    'Z': (0x7E, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x7E, 0x00),
}

LINE1 = "BLOODBORNE"
LINE2 = "RANDOMIZER"
SCALE = 6          # each glyph pixel becomes a SCALE x SCALE block
ADVANCE = 8 * SCALE  # tight, no inter-letter gap - fine for a one-off logo
LINE_GAP = 24


def png_chunk(tag: bytes, data: bytes) -> bytes:
    return (
        struct.pack(">I", len(data))
        + tag
        + data
        + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)
    )


def draw_text(pixels, x0, y0, text, scale, color):
    cx = x0
    for ch in text:
        rows = GLYPHS[ch]
        for row in range(8):
            bits = rows[row]
            for col in range(8):
                if not (bits & (0x80 >> col)):
                    continue
                for dy in range(scale):
                    py = y0 + row * scale + dy
                    for dx in range(scale):
                        px = cx + col * scale + dx
                        pixels[py][px] = color
        cx += ADVANCE


def build_png(size: int) -> bytes:
    pixels = [[BG] * size for _ in range(size)]

    line1_w = len(LINE1) * ADVANCE
    line2_w = len(LINE2) * ADVANCE
    text_h = 8 * SCALE
    block_h = text_h * 2 + LINE_GAP
    top = (size - block_h) // 2

    draw_text(pixels, (size - line1_w) // 2, top, LINE1, SCALE, FG)
    draw_text(pixels, (size - line2_w) // 2, top + text_h + LINE_GAP, LINE2, SCALE, FG)

    rows = bytearray()
    for y in range(size):
        rows.append(0)  # filter type 0 (None) for this scanline
        for x in range(size):
            r, g, b = pixels[y][x]
            rows.append(r)
            rows.append(g)
            rows.append(b)

    ihdr = struct.pack(">IIBBBBB", size, size, 8, 2, 0, 0, 0)  # 8-bit truecolour
    return (
        b"\x89PNG\r\n\x1a\n"
        + png_chunk(b"IHDR", ihdr)
        + png_chunk(b"IDAT", zlib.compress(bytes(rows), 9))
        + png_chunk(b"IEND", b"")
    )


def main() -> int:
    out = Path(__file__).resolve().parent.parent / "sce_sys" / "icon0.png"
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(build_png(SIZE))
    print(f"wrote {out} ({out.stat().st_size} bytes, {SIZE}x{SIZE})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
