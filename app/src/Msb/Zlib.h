// Zlib.h - the standard 2-byte-header + deflate-stream + adler32-trailer
// container (RFC 1950), which is exactly what a DCX_DFLT_10000_44_9 payload
// wraps (confirmed byte-exact against real Bloodborne map files via a Python
// zlib.decompress() prototype before any of this was written).
//
// Decompression is real: it's Mark Adler's public-domain puff.c (vendored
// verbatim as puff.cpp/puff.h - see that file's header), the canonical
// minimal INFLATE-only reference decoder, driven here with the destination
// size already known from the DCX header so its no-output "scanning mode"
// is never needed.
//
// Compression is deliberately NOT a real deflate encoder: the PS4 toolchain
// exposes only sceZlibInflate (decompression, chunked/async, no compress
// call at all - see orbis/Zlib.h), and hand-rolling a correct Huffman/LZ77
// encoder is a lot of surface area for zero behavioral benefit here, since
// DEFLATE's "stored" block type (BTYPE=00) is a fully spec-legal way to wrap
// arbitrary bytes with zero compression. Any compliant inflate - including
// whatever the game engine itself uses to load these files, since it already
// has to decode genuinely-compressed FromSoftware files - decodes stored
// blocks correctly. The only cost is file size (no compression), which is
// irrelevant here (map files are ~150-300KB either way).
#pragma once

#include <cstdint>
#include <vector>

namespace bbr {

// Decompresses a standard zlib stream (2-byte header + deflate + adler32
// trailer) into `out`, which must already be sized to the known-good
// uncompressed length (the DCX header always states this). Returns false on
// any format error.
bool ZlibInflate(const uint8_t* src, size_t srcLen, std::vector<uint8_t>& out);

// Wraps `data` as a valid zlib stream using only stored (uncompressed)
// deflate blocks - see header comment above for why. Always succeeds.
std::vector<uint8_t> ZlibDeflateStored(const uint8_t* data, size_t len);

} // namespace bbr
