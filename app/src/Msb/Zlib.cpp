#include "Zlib.h"

#include "puff.h"

namespace bbr {

bool ZlibInflate(const uint8_t* src, size_t srcLen, std::vector<uint8_t>& out) {
    // RFC 1950: 2-byte header (CMF/FLG), then a raw deflate stream, then a
    // 4-byte big-endian Adler-32 trailer. puff() only wants the deflate
    // stream in between.
    if (srcLen < 6) return false;

    unsigned long destLen   = (unsigned long)out.size();
    unsigned long sourceLen = (unsigned long)(srcLen - 2 - 4);

    int ret = puff(out.empty() ? nullptr : out.data(), &destLen, src + 2, &sourceLen);
    return ret == 0;
}

namespace {

uint32_t Adler32(const uint8_t* data, size_t len) {
    const uint32_t kMod = 65521;
    uint32_t a = 1, b = 0;
    for (size_t i = 0; i < len; i++) {
        a = (a + data[i]) % kMod;
        b = (b + a) % kMod;
    }
    return (b << 16) | a;
}

} // namespace

std::vector<uint8_t> ZlibDeflateStored(const uint8_t* data, size_t len) {
    std::vector<uint8_t> out;
    out.reserve(len + len / 65535 * 5 + 11);

    // zlib header: CMF=0x78 (deflate, 32K window), FLG=0x01 (no/fastest
    // compression level; combined value is a multiple of 31 as required).
    out.push_back(0x78);
    out.push_back(0x01);

    const size_t kMaxBlock = 65535;
    size_t offset = 0;
    do {
        size_t chunk = len - offset;
        if (chunk > kMaxBlock) chunk = kMaxBlock;
        bool isLast = (offset + chunk >= len);

        out.push_back(isLast ? 0x01 : 0x00); // BFINAL | BTYPE=00, byte-aligned
        uint16_t l = (uint16_t)chunk;
        out.push_back((uint8_t)(l & 0xFF));
        out.push_back((uint8_t)(l >> 8));
        uint16_t nl = (uint16_t)(~l);
        out.push_back((uint8_t)(nl & 0xFF));
        out.push_back((uint8_t)(nl >> 8));

        out.insert(out.end(), data + offset, data + offset + chunk);
        offset += chunk;
    } while (offset < len);

    uint32_t adler = Adler32(data, len);
    out.push_back((uint8_t)(adler >> 24));
    out.push_back((uint8_t)(adler >> 16));
    out.push_back((uint8_t)(adler >> 8));
    out.push_back((uint8_t)(adler));

    return out;
}

} // namespace bbr
