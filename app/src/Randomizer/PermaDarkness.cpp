#include "PermaDarkness.h"

#include "../Msb/BinUtil.h"

namespace bbr {

namespace {
const int64_t kTargetEventId = 6548972;
const size_t kEventEntrySize = 48; // Bloodborne/64-bit EMEVD.Event: see EMEVD.cs's Event(BinaryReaderEx)
const size_t kInstructionEntrySize = 32; // Bloodborne/64-bit EMEVD.Instruction: see Instruction.cs
} // namespace

bool ApplyPermaDarkness(std::vector<uint8_t>& plain, bool permaDarknessOn) {
    if (plain.size() < 16) return false;
    if (!(plain[0] == 'E' && plain[1] == 'V' && plain[2] == 'D' && plain[3] == 0)) return false;
    bool is64 = (plain[5] == 0xFF); // matches br.AssertSByte(0, -1) == -1
    if (!is64) return false; // only the Bloodborne EMEVD format is handled here

    size_t pos = 16;
    auto readVarint64 = [&]() -> int64_t {
        int64_t v = ReadI64LE(plain, pos);
        pos += 8;
        return v;
    };

    int64_t eventCount = readVarint64();
    int64_t offsetsEvents = readVarint64();
    readVarint64(); // instruction count - unused
    int64_t offsetsInstructions = readVarint64();
    readVarint64(); // unknown struct count
    readVarint64(); // unknown struct offset
    readVarint64(); // layer count
    readVarint64(); // offsets.Layers - unused
    readVarint64(); // parameter count
    readVarint64(); // offsets.Parameters - unused
    readVarint64(); // linked file count
    readVarint64(); // offsets.LinkedFiles - unused
    readVarint64(); // argument data length
    int64_t offsetsArguments = readVarint64();
    readVarint64(); // strings length
    readVarint64(); // offsets.Strings - unused

    if (eventCount < 0 || offsetsEvents < 0) return false;

    for (int64_t i = 0; i < eventCount; i++) {
        size_t base = (size_t)offsetsEvents + (size_t)i * kEventEntrySize;
        if (base + kEventEntrySize > plain.size()) return false;
        int64_t eventId = ReadI64LE(plain, base);
        if (eventId != kTargetEventId) continue;

        int64_t instructionsOffset = ReadI64LE(plain, base + 16);
        size_t instrBase = (size_t)offsetsInstructions + (size_t)instructionsOffset;
        if (instrBase + kInstructionEntrySize > plain.size()) return false;

        int32_t argsOffset = ReadI32LE(plain, instrBase + 16);
        size_t argDataPos = (size_t)offsetsArguments + (size_t)argsOffset;
        if (argDataPos + 9 > plain.size()) return false;

        if (permaDarknessOn) {
            plain[argDataPos + 4] = 254;
            plain[argDataPos + 5] = 21;
            plain[argDataPos + 6] = 0;
            plain[argDataPos + 8] = 1;
        } else {
            plain[argDataPos + 4] = 159;
            plain[argDataPos + 5] = 134;
            plain[argDataPos + 6] = 1;
            plain[argDataPos + 8] = 0;
        }
        return true;
    }

    return false;
}

} // namespace bbr
