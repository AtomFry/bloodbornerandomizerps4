// ParamBnd.h - locating data inside gameparam.parambnd.dcx.
//
// This is deliberately a LOCATOR, not a parser. The randomizer's eventual param
// edits (enemy drops, starting weapons) only ever overwrite one fixed-width
// integer with another inside an existing row - nothing is added, removed, or
// resized. So the file never needs to be rebuilt: find the byte range of the
// member you care about, poke bytes inside it, and re-emit the buffer as-is.
//
// That is why there is no PARAMDEF support here and no writer. Turning a
// paramdef "cell index" into a byte offset is done once on the PC by
// tools/param_offsets.py and baked in as a constant, the same way
// ModelSizeTable.h and NpcScalingTable.h were produced. See
// docs/plans/param-features.md.
//
// Format facts, measured from the real Bloodborne file rather than assumed:
//   - The .dcx wrapper is the same DCX_DFLT this port already handles, so
//     Msb/Dcx.h decompresses it unchanged (1.82 MB -> 28.3 MB).
//   - The payload is a BND4 flat container of 65 members.
//   - Every member is stored uncompressed (compressed size == uncompressed
//     size for all 65), so there is no nested DCX to unwrap.
//   - Header is 64 bytes; each file header is 36 bytes (format 0x74: 4-byte
//     data offsets, 8-byte sizes, ids and UTF-16 names present).
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace bbr {

struct ParamMember {
    std::string name;   // bare file name, e.g. "NpcParam.param"
    size_t offset = 0;  // byte offset of this member's data within the payload
    size_t size = 0;    // its length in bytes
};

// Parses just enough BND4 header to list members. `plain` is the decompressed
// payload. Returns false and sets `error` if the header doesn't look like the
// container we measured.
bool ParseParamBnd(const std::vector<uint8_t>& plain,
                   std::vector<ParamMember>& members,
                   std::string* error);

// Convenience lookup; returns nullptr when absent.
const ParamMember* FindParamMember(const std::vector<ParamMember>& members,
                                   const std::string& name);

// One row of a param file. `dataOffset` is absolute within the decompressed
// archive buffer, so a caller can poke a field with no further arithmetic.
struct ParamRow {
    int32_t id = 0;
    size_t dataOffset = 0;
};

// Reads the row table of one param member. Measured layout: row count is the
// u16 at +0x0A, descriptors start at +0x40 with a 24-byte stride of
// (id int32, 4 pad, dataOffset int64, nameOffset int64), and every row is the
// same size - so a field lives at a fixed offset inside each row.
//
// Validated on the real file: NPC_PARAM_ST 31398 rows / 388-byte stride,
// EQUIP_PARAM_WEAPON_ST 1090 / 316, SHOP_LINEUP_PARAM 1288 / 32, each stride
// matching the row size independently computed from the paramdef by
// tools/param_offsets.py.
bool ParseParamRows(const std::vector<uint8_t>& plain,
                    const ParamMember& member,
                    std::vector<ParamRow>& rows,
                    std::string* error);

} // namespace bbr
