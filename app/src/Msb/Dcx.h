// Dcx.h - the DCX_DFLT_10000_44_9 container Bloodborne's map files use
// (confirmed via SoulsFormats/SoulsFormats/Formats/DCX.cs and validated
// byte-exact against real vanilla/randomized m21_01_00_00.msb.dcx files with
// a Python zlib prototype before any of this was written). Plain zlib
// payload inside a fixed-layout, big-endian header - no Oodle/Kraken.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace bbr {

// Decompresses a .msb.dcx file's raw bytes into the plain MSBB payload.
// Returns false if the magic/header doesn't match this exact DCX variant, or
// if the inner zlib stream fails to inflate. When it returns false and
// outError is non-null, *outError names which specific check failed -
// useful on a real console where there's no debugger to step through this.
bool DcxDecompress(const std::vector<uint8_t>& dcxBytes, std::vector<uint8_t>& outPlain,
                    std::string* outError = nullptr);

// Wraps a plain MSBB payload back into a DCX_DFLT_10000_44_9 container,
// byte-layout-compatible with what the game reads (the inner zlib stream
// itself is stored/uncompressed - see Zlib.h for why that's fine).
std::vector<uint8_t> DcxCompress(const std::vector<uint8_t>& plain);

} // namespace bbr
