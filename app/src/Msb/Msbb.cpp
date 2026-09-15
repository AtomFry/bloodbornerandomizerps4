#include "Msbb.h"

#include "BinUtil.h"

#include <algorithm>

namespace bbr {

namespace {

const size_t kMsbbHeaderSize = 16;

// Reads one section's generic framing (Section<T>.Read in MSBBB.cs) and
// slices out each entry's raw bytes. entryOffsets/nameOffset/nextParamOffset
// are all absolute file positions - confirmed directly from MSBBB.cs (plain
// `br.Position = offset` seeks, no per-section base adjustment).
bool ReadSection(const std::vector<uint8_t>& b, size_t& pos, MsbbSection& section,
                  const char* expectedTypeName) {
    if (pos + 8 > b.size()) return false;
    section.unk1 = ReadI32LE(b, pos);
    int32_t offsetCount = ReadI32LE(b, pos + 4);
    pos += 8;

    if (offsetCount < 1) return false;
    int64_t nameOffset = ReadI64LE(b, pos);
    pos += 8;

    std::vector<int64_t> entryOffsets;
    entryOffsets.reserve(offsetCount - 1);
    for (int32_t i = 0; i < offsetCount - 1; i++) {
        entryOffsets.push_back(ReadI64LE(b, pos));
        pos += 8;
    }
    int64_t nextParamOffset = ReadI64LE(b, pos);
    pos += 8;

    std::string typeName = ReadUtf16AsAsciiAt(b, (size_t)nameOffset);
    if (typeName != expectedTypeName) return false;

    // nextParamOffset is 0 for the last section (Parts) - meaning "end of
    // file", not literally file position 0. Substitute a real end-of-data
    // sentinel so the boundary sort below works uniformly for every section.
    int64_t endSentinel = (nextParamOffset == 0) ? (int64_t)b.size() : nextParamOffset;

    std::vector<int64_t> sortedBoundaries = entryOffsets;
    sortedBoundaries.push_back(endSentinel);
    std::sort(sortedBoundaries.begin(), sortedBoundaries.end());

    section.entries.clear();
    section.entries.reserve(entryOffsets.size());
    for (int64_t off : entryOffsets) {
        auto it = std::upper_bound(sortedBoundaries.begin(), sortedBoundaries.end(), off);
        int64_t end = (it != sortedBoundaries.end()) ? *it : (int64_t)b.size();
        if (off < 0 || end < off || (size_t)end > b.size()) return false;
        section.entries.emplace_back(b.begin() + off, b.begin() + end);
    }

    pos = (size_t)nextParamOffset;
    return true;
}

// Writes one section's generic framing + entries, exactly mirroring
// Section<T>.Write in MSBBB.cs, but patching offsets directly by remembered
// buffer position instead of SoulsFormats' named-reservation mechanism -
// there's no need for that indirection when this file controls the whole
// serialize in one pass. Returns the absolute position of this section's
// own NextParamOffset field, which the caller patches once it knows where
// the next section starts (or with 0, for the last section) - matching
// MSBBB.Write's `bw.FillInt64("NextParamOffset", ...)` calls between
// sections.
size_t WriteSection(std::vector<uint8_t>& out, const MsbbSection& section,
                     const char* typeName) {
    AppendI32LE(out, section.unk1);
    AppendI32LE(out, (int32_t)(section.entries.size() + 1));

    size_t nameOffsetFieldPos = out.size();
    AppendI64LE(out, 0);

    std::vector<size_t> entryOffsetFieldPos;
    entryOffsetFieldPos.reserve(section.entries.size());
    for (size_t i = 0; i < section.entries.size(); i++) {
        entryOffsetFieldPos.push_back(out.size());
        AppendI64LE(out, 0);
    }

    size_t nextParamOffsetFieldPos = out.size();
    AppendI64LE(out, 0);

    WriteI64LE(out, nameOffsetFieldPos, (int64_t)out.size());
    AppendAsciiAsUtf16(out, typeName);
    AppendPad(out, 8);

    for (size_t i = 0; i < section.entries.size(); i++) {
        WriteI64LE(out, entryOffsetFieldPos[i], (int64_t)out.size());
        AppendBytes(out, section.entries[i].data(), section.entries[i].size());
    }

    return nextParamOffsetFieldPos;
}

} // namespace

bool MsbbFile::Parse(const std::vector<uint8_t>& b) {
    if (b.size() < kMsbbHeaderSize) return false;
    if (!(b[0] == 'M' && b[1] == 'S' && b[2] == 'B' && b[3] == ' ')) return false;
    if (ReadI32LE(b, 4) != 1)    return false;
    if (ReadI32LE(b, 8) != 0x10) return false;
    // b[12]=isBigEndian(false), b[13]=isBitBigEndian(false), b[14]=textEncoding(1),
    // b[15]=is64BitOffset(0xFF) - fixed constants for every real Bloodborne map,
    // not validated here since nothing downstream depends on them varying.

    size_t pos = kMsbbHeaderSize;
    if (!ReadSection(b, pos, models, "MODEL_PARAM_ST"))  return false;
    if (!ReadSection(b, pos, events, "EVENT_PARAM_ST"))   return false;
    if (!ReadSection(b, pos, regions, "POINT_PARAM_ST"))  return false;
    if (!ReadSection(b, pos, parts, "PARTS_PARAM_ST"))    return false;
    return pos == 0;
}

std::vector<uint8_t> MsbbFile::Serialize() const {
    std::vector<uint8_t> out;
    out.push_back('M'); out.push_back('S'); out.push_back('B'); out.push_back(' ');
    AppendI32LE(out, 1);
    AppendI32LE(out, 0x10);
    out.push_back(0); out.push_back(0); out.push_back(1); out.push_back((uint8_t)0xFF);

    size_t modelsNextPos = WriteSection(out, models, "MODEL_PARAM_ST");
    WriteI64LE(out, modelsNextPos, (int64_t)out.size());

    size_t eventsNextPos = WriteSection(out, events, "EVENT_PARAM_ST");
    WriteI64LE(out, eventsNextPos, (int64_t)out.size());

    size_t regionsNextPos = WriteSection(out, regions, "POINT_PARAM_ST");
    WriteI64LE(out, regionsNextPos, (int64_t)out.size());

    size_t partsNextPos = WriteSection(out, parts, "PARTS_PARAM_ST");
    WriteI64LE(out, partsNextPos, 0);

    return out;
}

namespace part_fields {

uint32_t GetType(const std::vector<uint8_t>& blob) { return ReadU32LE(blob, 0x14); }
int32_t  GetModelIndex(const std::vector<uint8_t>& blob) { return ReadI32LE(blob, 0x1C); }
void     SetModelIndex(std::vector<uint8_t>& blob, int32_t v) { WriteI32LE(blob, 0x1C, v); }

std::string GetName(const std::vector<uint8_t>& blob) {
    int64_t nameOffset = ReadI64LE(blob, 0x08);
    return ReadUtf16AsAsciiAt(blob, (size_t)nameOffset);
}

int32_t GetEntityID(const std::vector<uint8_t>& blob) {
    int64_t baseDataOffset = ReadI64LE(blob, 0xB0);
    return ReadI32LE(blob, (size_t)baseDataOffset);
}

int32_t GetEnemyThinkParamID(const std::vector<uint8_t>& blob) {
    int64_t typeDataOffset = ReadI64LE(blob, 0xB8);
    return ReadI32LE(blob, (size_t)typeDataOffset + 8);
}

int32_t GetEnemyNPCParamID(const std::vector<uint8_t>& blob) {
    int64_t typeDataOffset = ReadI64LE(blob, 0xB8);
    return ReadI32LE(blob, (size_t)typeDataOffset + 12);
}

int32_t GetEnemyTalkID(const std::vector<uint8_t>& blob) {
    int64_t typeDataOffset = ReadI64LE(blob, 0xB8);
    return ReadI32LE(blob, (size_t)typeDataOffset + 16);
}

void SetEnemyThinkParamID(std::vector<uint8_t>& blob, int32_t v) {
    int64_t typeDataOffset = ReadI64LE(blob, 0xB8);
    WriteI32LE(blob, (size_t)typeDataOffset + 8, v);
}

void SetEnemyNPCParamID(std::vector<uint8_t>& blob, int32_t v) {
    int64_t typeDataOffset = ReadI64LE(blob, 0xB8);
    WriteI32LE(blob, (size_t)typeDataOffset + 12, v);
}

} // namespace part_fields

namespace event_fields {

uint32_t GetType(const std::vector<uint8_t>& blob) { return ReadU32LE(blob, 0x0C); }

int32_t GetTreasureItemLot1(const std::vector<uint8_t>& blob) {
    int64_t typeDataOffset = ReadI64LE(blob, 0x20);
    return ReadI32LE(blob, (size_t)typeDataOffset + 0x10);
}

void SetTreasureItemLot1(std::vector<uint8_t>& blob, int32_t v) {
    int64_t typeDataOffset = ReadI64LE(blob, 0x20);
    WriteI32LE(blob, (size_t)typeDataOffset + 0x10, v);
}

} // namespace event_fields

namespace model_fields {

uint32_t GetType(const std::vector<uint8_t>& blob) { return ReadU32LE(blob, 0x08); }

std::string GetName(const std::vector<uint8_t>& blob) {
    int64_t nameOffset = ReadI64LE(blob, 0x00);
    return ReadUtf16AsAsciiAt(blob, (size_t)nameOffset);
}

std::vector<uint8_t> BuildEnemyModelEntry(const std::string& name) {
    // Mirrors Model.Write in ModelParam.cs: a fixed 40-byte (0x28) header -
    // identical layout for every Model subtype, only the Type tag differs -
    // followed by the name string, the (always-empty, for a synthesized
    // entry) placeholder string, padded to 8 bytes.
    std::vector<uint8_t> blob;
    AppendI64LE(blob, 0x28);              // nameOffset: right after the header
    AppendU32LE(blob, ModelType::kEnemy);
    AppendI32LE(blob, 0);                 // ID - discarded on read
    size_t placeholderOffsetPos = blob.size();
    AppendI64LE(blob, 0);                 // placeholderOffset - patched below
    AppendI32LE(blob, 0);                 // InstanceCount - see Msbb.h note: not recomputed in v1
    AppendI32LE(blob, 0);
    AppendI32LE(blob, 0);
    AppendI32LE(blob, 0);

    AppendAsciiAsUtf16(blob, name);
    WriteI64LE(blob, placeholderOffsetPos, (int64_t)blob.size());
    AppendAsciiAsUtf16(blob, "");
    AppendPad(blob, 8);
    return blob;
}

} // namespace model_fields

} // namespace bbr
