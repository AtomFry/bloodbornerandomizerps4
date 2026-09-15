// Msbb.h - Bloodborne's MSBB map-layout format (SoulsFormats:
// SoulsFormats/Formats/MSB/MSBB/*.cs, all read in full before this was
// written). Only Part.Enemy (NPCParamID/ThinkParamID/ModelName, via its
// modelIndex) is ever mutated by the randomizer - nothing else in a map file
// needs to change. That drives the whole design here:
//
// Every entry in every section (Models/Events/Regions/Parts) is kept as a
// raw captured byte blob, never decoded into a C++ struct. This is safe
// because every offset SoulsFormats writes inside one entry (name offset,
// type-data offset, gparam offset, ...) is stored relative to that entry's
// own start position - so copying an entry's bytes verbatim to a new
// position in the output file, unmodified, reproduces a perfectly valid
// entry no matter where it lands. The only offsets that are NOT
// entry-relative are the section-level framing (nameOffset/entryOffsets/
// nextParamOffset, all absolute file positions per MSBBB.cs's Section<T>),
// which this file fully recomputes on every write - see Parse()/Serialize().
//
// Two fixed-offset fields get poked in place, still without decoding
// anything else in the blob:
//   - Part's common header: Type (u32 @ entry+0x14) and modelIndex
//     (i32 @ entry+0x1C) - identical layout for every Part subtype, see
//     PartsParam.cs's Part(BinaryReaderEx) constructor.
//   - Part.Enemy's type data (only entries with Type==2): ThinkParamID and
//     NPCParamID, at typeDataOffset+8 and +12, where typeDataOffset is the
//     i64 read from entry+0xB8. See Part.Enemy's ReadTypeData in
//     PartsParam.cs.
// Poking these never changes an entry's total byte length (all are
// fixed-size int fields), so nothing else in the blob needs to shift.
//
// Cross-references between entries (Object/Enemy.CollisionName, Event
// part/point refs, Generator spawn refs, ...) are stored as plain indices
// into a section's entry list, resolved by POSITION, not by name, at load
// time. Since this parser never reorders, inserts into the middle of, or
// removes entries from Events/Regions/Parts, every such index stays valid
// automatically - there is no name<->index resolution pass here at all
// (unlike SoulsFormats' GetNames()/GetIndices()), because there's nothing
// to fix up.
//
// The one place new entries ARE added is ModelParam.Enemies: the
// randomizer's model-list merge (see EnemyRandomizer.cpp) appends synthetic
// Model.Enemy entries for any enemy model name a map doesn't already have.
// New entries only ever get appended at a section's end, so existing
// indices (into Models, referenced by Part.modelIndex) never shift either.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace bbr {

struct MsbbSection {
    int32_t unk1 = 3;
    // Raw captured bytes for each entry, in the exact order they were read
    // (== the order every index-based cross-reference in the file assumes -
    // see file header comment). Never reordered, never resorted by type.
    std::vector<std::vector<uint8_t>> entries;
};

class MsbbFile {
public:
    // Parses a decompressed MSBB payload (i.e. already through
    // DcxDecompress). Returns false if the header doesn't match the
    // expected MSBB layout.
    bool Parse(const std::vector<uint8_t>& plain);

    // Serializes back to a plain MSBB payload (still needs DcxCompress to
    // become a .msb.dcx file). Section-level framing is fully recomputed;
    // every entry's own bytes are copied verbatim from what Parse() read
    // (or, for freshly-appended model entries, from BuildEnemyModelEntry()).
    std::vector<uint8_t> Serialize() const;

    MsbbSection models;
    MsbbSection events;
    MsbbSection regions;
    MsbbSection parts;
};

// PartsType discriminant values (PartsParam.cs's internal PartsType enum).
namespace PartsType {
const uint32_t kMapPiece = 0x0;
const uint32_t kObject   = 0x1;
const uint32_t kEnemy    = 0x2;
} // namespace PartsType

// ModelType discriminant values (ModelParam.cs's internal ModelType enum).
namespace ModelType {
const uint32_t kEnemy = 2;
} // namespace ModelType

namespace part_fields {

uint32_t GetType(const std::vector<uint8_t>& blob);
int32_t  GetModelIndex(const std::vector<uint8_t>& blob);
void     SetModelIndex(std::vector<uint8_t>& blob, int32_t v);
std::string GetName(const std::vector<uint8_t>& blob);

// Lives in the Part's BASE data, not its type data: the i64 at entry+0xB0 is
// baseDataOffset and EntityID is the first i32 there (PartsParam.cs's
// Part(BinaryReaderEx) reads baseDataOffset at that position, then seeks to it
// and reads EntityID first). Read-only: boss randomization skips placements
// with EntityID == -1, and nothing needs to write it.
int32_t GetEntityID(const std::vector<uint8_t>& blob);

// Valid only when GetType(blob) == PartsType::kEnemy.
int32_t GetEnemyThinkParamID(const std::vector<uint8_t>& blob);
int32_t GetEnemyNPCParamID(const std::vector<uint8_t>& blob);
int32_t GetEnemyTalkID(const std::vector<uint8_t>& blob);
void    SetEnemyThinkParamID(std::vector<uint8_t>& blob, int32_t v);
void    SetEnemyNPCParamID(std::vector<uint8_t>& blob, int32_t v);

} // namespace part_fields

// EventType discriminant values (EventParam.cs's internal EventType enum).
namespace EventType {
const uint32_t kTreasure = 0x4;
} // namespace EventType

namespace event_fields {

// Event common header (EventParam.cs's Event(BinaryReaderEx)): nameOffset @
// 0x00, EventID @ 0x08, Type @ 0x0C, baseDataOffset @ 0x18, typeDataOffset @
// 0x20. Only the type tag and the Treasure item lot are needed here.
uint32_t GetType(const std::vector<uint8_t>& blob);

// Valid only when GetType(blob) == EventType::kTreasure. ItemLot1 sits at
// typeDataOffset + 0x10 (Treasure.Read: two zeroed int32s, partIndex2, another
// zeroed int32, then ItemLot1/2/3 - corroborated by its own UnkT1C field
// landing at +0x1C). ItemLot2/ItemLot3 are deliberately not exposed: the
// reference randomizer only ever touches ItemLot1.
int32_t GetTreasureItemLot1(const std::vector<uint8_t>& blob);
void    SetTreasureItemLot1(std::vector<uint8_t>& blob, int32_t v);

} // namespace event_fields

namespace model_fields {

uint32_t GetType(const std::vector<uint8_t>& blob);
std::string GetName(const std::vector<uint8_t>& blob);

// Synthesizes a brand-new Model.Enemy entry blob for a model not already
// present in some map's Models section (see ModelParam.cs's Model base
// class: every subtype shares one fixed 40-byte header + two padded UTF-16
// strings, no subtype-specific fields at all).
std::vector<uint8_t> BuildEnemyModelEntry(const std::string& name);

} // namespace model_fields

} // namespace bbr
