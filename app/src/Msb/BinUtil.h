// BinUtil.h - little/big-endian byte-buffer read/write/append helpers used by
// Dcx.cpp and Msbb.cpp. Header-only, no allocation beyond the caller's own
// std::vector<uint8_t> buffer.
//
// DCX container headers are big-endian (confirmed against real Bloodborne
// map files via a Python zlib prototype before any of this was written).
// Everything inside the decompressed MSBB payload is little-endian (per
// MSBB's own header flags: isBigEndian=false). Hence both BE and LE readers.
#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace bbr {

inline uint32_t ReadU32BE(const std::vector<uint8_t>& b, size_t off) {
    return (uint32_t(b[off]) << 24) | (uint32_t(b[off + 1]) << 16) |
           (uint32_t(b[off + 2]) << 8) | uint32_t(b[off + 3]);
}

inline void AppendU32BE(std::vector<uint8_t>& b, uint32_t v) {
    b.push_back((uint8_t)(v >> 24));
    b.push_back((uint8_t)(v >> 16));
    b.push_back((uint8_t)(v >> 8));
    b.push_back((uint8_t)(v));
}

inline uint16_t ReadU16LE(const std::vector<uint8_t>& b, size_t off) {
    return (uint16_t)((uint16_t)b[off] | ((uint16_t)b[off + 1] << 8));
}

inline uint32_t ReadU32LE(const std::vector<uint8_t>& b, size_t off) {
    return uint32_t(b[off]) | (uint32_t(b[off + 1]) << 8) |
           (uint32_t(b[off + 2]) << 16) | (uint32_t(b[off + 3]) << 24);
}

inline int32_t ReadI32LE(const std::vector<uint8_t>& b, size_t off) {
    return (int32_t)ReadU32LE(b, off);
}

inline void WriteU32LE(std::vector<uint8_t>& b, size_t off, uint32_t v) {
    b[off]     = (uint8_t)(v);
    b[off + 1] = (uint8_t)(v >> 8);
    b[off + 2] = (uint8_t)(v >> 16);
    b[off + 3] = (uint8_t)(v >> 24);
}

inline void WriteI32LE(std::vector<uint8_t>& b, size_t off, int32_t v) {
    WriteU32LE(b, off, (uint32_t)v);
}

inline uint64_t ReadU64LE(const std::vector<uint8_t>& b, size_t off) {
    uint64_t lo = ReadU32LE(b, off);
    uint64_t hi = ReadU32LE(b, off + 4);
    return lo | (hi << 32);
}

inline int64_t ReadI64LE(const std::vector<uint8_t>& b, size_t off) {
    return (int64_t)ReadU64LE(b, off);
}

inline void WriteU64LE(std::vector<uint8_t>& b, size_t off, uint64_t v) {
    WriteU32LE(b, off, (uint32_t)(v & 0xFFFFFFFFu));
    WriteU32LE(b, off + 4, (uint32_t)(v >> 32));
}

inline void WriteI64LE(std::vector<uint8_t>& b, size_t off, int64_t v) {
    WriteU64LE(b, off, (uint64_t)v);
}

inline void AppendU32LE(std::vector<uint8_t>& b, uint32_t v) {
    b.push_back((uint8_t)(v));
    b.push_back((uint8_t)(v >> 8));
    b.push_back((uint8_t)(v >> 16));
    b.push_back((uint8_t)(v >> 24));
}

inline void AppendI32LE(std::vector<uint8_t>& b, int32_t v) { AppendU32LE(b, (uint32_t)v); }

inline void AppendU64LE(std::vector<uint8_t>& b, uint64_t v) {
    AppendU32LE(b, (uint32_t)(v & 0xFFFFFFFFu));
    AppendU32LE(b, (uint32_t)(v >> 32));
}

inline void AppendI64LE(std::vector<uint8_t>& b, int64_t v) { AppendU64LE(b, (uint64_t)v); }

// Reads a null-terminated UTF-16LE string at an absolute byte offset and
// returns it re-encoded as UTF-8. Bloodborne map/entry names are ASCII in
// practice (model names, part names), so a byte-truncating decode (high byte
// dropped) is sufficient here - full UTF-8 multibyte encoding is not needed
// for round-tripping ASCII identifiers, only for display purposes we don't
// need for opaque entries.
inline std::string ReadUtf16AsAsciiAt(const std::vector<uint8_t>& b, size_t off) {
    std::string out;
    size_t i = off;
    while (i + 1 < b.size()) {
        uint16_t ch = (uint16_t)b[i] | ((uint16_t)b[i + 1] << 8);
        if (ch == 0) break;
        out.push_back((char)(ch & 0xFF));
        i += 2;
    }
    return out;
}

// Appends an ASCII string as null-terminated UTF-16LE (mirrors the decode
// above - every name this tool writes back is one it already read as ASCII,
// or a plain ASCII literal like a model name "c1234").
inline void AppendAsciiAsUtf16(std::vector<uint8_t>& b, const std::string& s) {
    for (char c : s) {
        b.push_back((uint8_t)c);
        b.push_back(0);
    }
    b.push_back(0);
    b.push_back(0);
}

inline void AppendPad(std::vector<uint8_t>& b, size_t align) {
    while (b.size() % align != 0) b.push_back(0);
}

inline void AppendBytes(std::vector<uint8_t>& b, const uint8_t* data, size_t len) {
    b.insert(b.end(), data, data + len);
}

} // namespace bbr
