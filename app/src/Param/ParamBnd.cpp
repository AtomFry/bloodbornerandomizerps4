#include "ParamBnd.h"

#include "../Msb/BinUtil.h"

namespace bbr {

namespace {

const size_t kHeaderSize     = 64;
const size_t kFileHeaderSize = 36;

// Offsets within one 36-byte file header (SoulsFormats
// BinderFileHeader.ReadBinder4FileHeader with format 0x74).
const size_t kFhUncompressedSize = 16;
const size_t kFhDataOffset       = 24;
const size_t kFhNameOffset       = 32;

std::string Fail(std::string* error, const std::string& message) {
    if (error) *error = message;
    return message;
}

} // namespace

bool ParseParamBnd(const std::vector<uint8_t>& plain,
                   std::vector<ParamMember>& members,
                   std::string* error) {
    members.clear();

    if (plain.size() < kHeaderSize) {
        Fail(error, "param bnd too small (" + std::to_string(plain.size()) + " bytes)");
        return false;
    }
    if (!(plain[0] == 'B' && plain[1] == 'N' && plain[2] == 'D' && plain[3] == '4')) {
        Fail(error, "bad BND4 magic");
        return false;
    }

    int32_t fileCount = ReadI32LE(plain, 12);
    if (fileCount <= 0 || (size_t)fileCount > 4096) {
        Fail(error, "implausible BND4 file count " + std::to_string(fileCount));
        return false;
    }

    const size_t headersEnd = kHeaderSize + (size_t)fileCount * kFileHeaderSize;
    if (headersEnd > plain.size()) {
        Fail(error, "BND4 file headers run past end of buffer");
        return false;
    }

    members.reserve((size_t)fileCount);
    for (int32_t i = 0; i < fileCount; i++) {
        const size_t e = kHeaderSize + (size_t)i * kFileHeaderSize;

        int64_t size = ReadI64LE(plain, e + kFhUncompressedSize);
        uint32_t dataOffset = ReadU32LE(plain, e + kFhDataOffset);
        uint32_t nameOffset = ReadU32LE(plain, e + kFhNameOffset);

        if (size < 0 || (size_t)dataOffset + (size_t)size > plain.size()) {
            Fail(error, "BND4 member " + std::to_string(i) + " data range out of bounds");
            return false;
        }
        if ((size_t)nameOffset >= plain.size()) {
            Fail(error, "BND4 member " + std::to_string(i) + " name offset out of bounds");
            return false;
        }

        // Names are UTF-16 and stored as a full path; keep only the file name.
        std::string full = ReadUtf16AsAsciiAt(plain, (size_t)nameOffset);
        size_t slash = full.find_last_of("/\\");
        std::string bare = (slash == std::string::npos) ? full : full.substr(slash + 1);

        ParamMember m;
        m.name = bare;
        m.offset = (size_t)dataOffset;
        m.size = (size_t)size;
        members.push_back(m);
    }

    return true;
}

const ParamMember* FindParamMember(const std::vector<ParamMember>& members,
                                   const std::string& name) {
    for (const ParamMember& m : members) {
        if (m.name == name) return &m;
    }
    return nullptr;
}

bool ParseParamRows(const std::vector<uint8_t>& plain,
                    const ParamMember& member,
                    std::vector<ParamRow>& rows,
                    std::string* error) {
    rows.clear();

    const size_t kRowTableStart = 0x40;
    const size_t kRowDescStride = 24;
    const size_t kRowCountOffset = 0x0A;

    if (member.size < kRowTableStart) {
        Fail(error, member.name + ": too small to hold a row table");
        return false;
    }

    uint32_t rowCount = ReadU16LE(plain, member.offset + kRowCountOffset);
    if (rowCount == 0) {
        Fail(error, member.name + ": zero rows");
        return false;
    }

    const size_t tableBytes = (size_t)rowCount * kRowDescStride;
    if (kRowTableStart + tableBytes > member.size) {
        Fail(error, member.name + ": row table (" + std::to_string(rowCount) +
                        " rows) runs past the member");
        return false;
    }

    rows.reserve(rowCount);
    for (uint32_t r = 0; r < rowCount; r++) {
        const size_t d = member.offset + kRowTableStart + (size_t)r * kRowDescStride;

        ParamRow row;
        row.id = ReadI32LE(plain, d);
        int64_t rel = ReadI64LE(plain, d + 8);
        if (rel < 0 || (size_t)rel >= member.size) {
            Fail(error, member.name + ": row " + std::to_string(r) +
                            " data offset out of range");
            return false;
        }
        row.dataOffset = member.offset + (size_t)rel;
        rows.push_back(row);
    }

    return true;
}

} // namespace bbr
