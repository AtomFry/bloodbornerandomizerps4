#include "Dcx.h"

#include "BinUtil.h"
#include "Zlib.h"

#include <cstdio>

namespace bbr {

namespace {
const size_t kHeaderSize = 76; // through the end of the compressedHeaderLength field

std::string HexDumpRange(const std::vector<uint8_t>& b, size_t off, size_t n) {
    std::string out;
    char tmp[4];
    for (size_t i = off; i < b.size() && i < off + n; i++) {
        snprintf(tmp, sizeof(tmp), "%02X ", b[i]);
        out += tmp;
    }
    return out;
}

bool Fail(std::string* outError, const std::string& msg) {
    if (outError) *outError = msg;
    return false;
}
} // namespace

bool DcxDecompress(const std::vector<uint8_t>& b, std::vector<uint8_t>& outPlain,
                    std::string* outError) {
    if (b.size() < kHeaderSize) {
        return Fail(outError, "file too small to be a DCX (" + std::to_string(b.size()) + " bytes)");
    }
    if (!(b[0] == 'D' && b[1] == 'C' && b[2] == 'X' && b[3] == 0)) {
        return Fail(outError, "bad DCX magic - first 16 bytes: " + HexDumpRange(b, 0, 16));
    }
    if (ReadU32BE(b, 4) != 0x10000) return Fail(outError, "unexpected header field @4 (not DCX_DFLT_10000_44_9?)");
    if (ReadU32BE(b, 8) != 0x18)    return Fail(outError, "unexpected header field @8 (not DCX_DFLT_10000_44_9?)");
    if (ReadU32BE(b, 12) != 0x24)   return Fail(outError, "unexpected header field @12 (not DCX_DFLT_10000_44_9?)");
    if (ReadU32BE(b, 16) != 0x44)   return Fail(outError, "unexpected header field @16 (not DCX_DFLT_10000_44_9?)");
    if (ReadU32BE(b, 20) != 0x4C)   return Fail(outError, "unexpected header field @20 (not DCX_DFLT_10000_44_9?)");
    if (!(b[24] == 'D' && b[25] == 'C' && b[26] == 'S' && b[27] == 0)) {
        return Fail(outError, "missing DCS marker @24");
    }

    uint32_t uncompressedSize = ReadU32BE(b, 28);
    uint32_t compressedSize   = ReadU32BE(b, 32);

    if (!(b[36] == 'D' && b[37] == 'C' && b[38] == 'P' && b[39] == 0)) {
        return Fail(outError, "missing DCP marker @36");
    }
    if (!(b[40] == 'D' && b[41] == 'F' && b[42] == 'L' && b[43] == 'T')) {
        return Fail(outError, "compression method is not DFLT @40 - bytes: " + HexDumpRange(b, 40, 4));
    }
    if (!(b[68] == 'D' && b[69] == 'C' && b[70] == 'A' && b[71] == 0)) {
        return Fail(outError, "missing DCA marker @68");
    }
    if (ReadU32BE(b, 72) != 8) return Fail(outError, "unexpected compressedHeaderLength @72");

    if (b.size() < kHeaderSize + compressedSize) {
        return Fail(outError, "file too short for declared compressedSize (have " +
                                   std::to_string(b.size()) + ", need " +
                                   std::to_string(kHeaderSize + compressedSize) + ")");
    }

    outPlain.assign(uncompressedSize, 0);
    if (!ZlibInflate(b.data() + kHeaderSize, compressedSize, outPlain)) {
        return Fail(outError, "zlib inflate failed (uncompressedSize=" + std::to_string(uncompressedSize) +
                                   " compressedSize=" + std::to_string(compressedSize) + ")");
    }
    return true;
}

std::vector<uint8_t> DcxCompress(const std::vector<uint8_t>& plain) {
    std::vector<uint8_t> zlibStream = ZlibDeflateStored(plain.data(), plain.size());

    std::vector<uint8_t> out;
    out.reserve(kHeaderSize + zlibStream.size());

    out.push_back('D'); out.push_back('C'); out.push_back('X'); out.push_back(0);
    AppendU32BE(out, 0x10000);
    AppendU32BE(out, 0x18);
    AppendU32BE(out, 0x24);
    AppendU32BE(out, 0x44);
    AppendU32BE(out, 0x4C);
    out.push_back('D'); out.push_back('C'); out.push_back('S'); out.push_back(0);
    AppendU32BE(out, (uint32_t)plain.size());
    AppendU32BE(out, (uint32_t)zlibStream.size());
    out.push_back('D'); out.push_back('C'); out.push_back('P'); out.push_back(0);
    out.push_back('D'); out.push_back('F'); out.push_back('L'); out.push_back('T');
    AppendU32BE(out, 0x20);
    AppendU32BE(out, 0x9000000);
    AppendU32BE(out, 0);
    AppendU32BE(out, 0);
    AppendU32BE(out, 0);
    AppendU32BE(out, 0x00010100);
    out.push_back('D'); out.push_back('C'); out.push_back('A'); out.push_back(0);
    AppendU32BE(out, 8);

    out.insert(out.end(), zlibStream.begin(), zlibStream.end());
    return out;
}

} // namespace bbr
