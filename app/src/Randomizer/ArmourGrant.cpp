// cstdlib before anything pulling <random>/<cmath>, or the toolchain's libc++
// fails on ::abs - same ordering constraint as BossRandomizer.cpp.
#include <cstdlib>

#include "ArmourGrant.h"

#include "ArmourTables.h"
#include "CharaInitRows.h"

#include "../Msb/BinUtil.h"
#include "../Platform/Log.h"

namespace bbr {

namespace {

// One slot's table and its row count, so the four slots can be walked as a loop
// rather than as four near-identical blocks. Built per call because a
// std::array's accessor returns a reference to a function-local static and there
// is nothing to cache.
struct SlotTable {
    const ModelPoolEntry* rows;
    int count;
    const bool* enabled;
};

// The candidate list for one slot: the ticked rows' ids, in table order. The
// table holds the armour id as a decimal string in `model` because that is the
// field the picker keys and logs on; there is deliberately no second array of ids
// to fall out of step with it.
std::vector<int32_t> Candidates(const SlotTable& slot) {
    std::vector<int32_t> out;
    for (int i = 0; i < slot.count; i++) {
        if (slot.enabled[i]) {
            out.push_back((int32_t)strtol(slot.rows[(size_t)i].model, nullptr, 10));
        }
    }
    return out;
}

SlotTable TableFor(int slot,
                   const HeadwearSelection& headwear,
                   const BodyArmourSelection& bodyArmour,
                   const GlovesSelection& gloves,
                   const LegwearSelection& legwear) {
    switch (slot) {
        case 0: return { HeadwearTable().data(),   kHeadwearCount,   headwear.enabled };
        case 1: return { BodyArmourTable().data(), kBodyArmourCount, bodyArmour.enabled };
        case 2: return { GlovesTable().data(),     kGlovesCount,     gloves.enabled };
        default: break;
    }
    return { LegwearTable().data(), kLegwearCount, legwear.enabled };
}

} // namespace

const char* ArmourPieceName(int slot, int32_t armourId) {
    if (slot < 0 || slot >= kClothingSlots) return nullptr;
    const ModelPoolEntry* rows = nullptr;
    int count = 0;
    switch (slot) {
        case 0: rows = HeadwearTable().data();   count = kHeadwearCount;   break;
        case 1: rows = BodyArmourTable().data(); count = kBodyArmourCount; break;
        case 2: rows = GlovesTable().data();     count = kGlovesCount;     break;
        default: rows = LegwearTable().data();   count = kLegwearCount;    break;
    }
    for (int i = 0; i < count; i++) {
        if (strtol(rows[(size_t)i].model, nullptr, 10) == armourId) {
            return rows[(size_t)i].displayName;
        }
    }
    return nullptr;
}

bool GrantArmour(std::vector<uint8_t>& plain,
                 const ParamMember& charaInitParam,
                 const HeadwearSelection& headwear,
                 const BodyArmourSelection& bodyArmour,
                 const GlovesSelection& gloves,
                 const LegwearSelection& legwear,
                 std::mt19937& rng,
                 ArmourGrantResult& result,
                 std::string* error) {
    // Step 1: four candidate lists, in the frozen slot order.
    std::vector<int32_t> candidates[kClothingSlots];
    bool any = false;
    for (int slot = 0; slot < kClothingSlots; slot++) {
        candidates[slot] = Candidates(
            TableFor(slot, headwear, bodyArmour, gloves, legwear));
        if (!candidates[slot].empty()) any = true;
    }

    // Step 2: nothing ticked in any of the four. Write nothing, draw nothing,
    // parse nothing. Returning BEFORE the RNG is touched is what makes turning
    // all four settings on with empty selections identical to leaving them off,
    // roll for roll.
    if (!any) return true;

    std::vector<ParamRow> charaRows;
    if (!ParseParamRows(plain, charaInitParam, charaRows, error)) return false;

    // Step 3: EXACTLY FOUR RAW WORDS, one per slot, in slot order, whether or not
    // that slot has anything ticked. This loop is the mechanism spec 040 B6 rests
    // on and neither half of it is stylistic:
    //
    //   the COUNT is constant - a slot with an empty list still calls rng() and
    //   throws the word away, so emptying one picker cannot shift another's word
    //   along the stream;
    //
    //   the INDEX is `word % k` and not a std::uniform_int_distribution, because a
    //   distribution consumes a variable number of raw words as k changes, which
    //   would reintroduce exactly the coupling the constant count removes.
    //
    // The modulo bias this accepts is about one part in 10^8 (2^32 against at most
    // 34 candidates) on a cosmetic choice; see ArmourGrant.h.
    for (int slot = 0; slot < kClothingSlots; slot++) {
        const uint32_t word = (uint32_t)rng();
        result.drawsMade++;
        if (candidates[slot].empty()) continue; // the word is discarded on purpose
        const size_t k = candidates[slot].size();
        result.granted[slot] = candidates[slot][(size_t)(word % (uint32_t)k)];
    }

    // Step 4: the write, over every origin row.
    for (int32_t id : kOriginRows) {
        const ParamRow* row = FindRow(charaRows, id);
        if (row == nullptr) {
            // Not fatal on its own: a row missing from one block still leaves the
            // other block written. Reported so a wholesale mismatch - a different
            // game version, say - is visible in the log rather than looking like a
            // silent success.
            result.rowsMissing++;
            continue;
        }

        // ParseParamRows only proves the row STARTS inside the member. The whole
        // 300-byte row is checked rather than just the bytes written below: the row
        // is the unit the param format guarantees, and a row that does not fit
        // means the archive is not the layout every offset here was measured
        // against.
        if (row->dataOffset + kRowBytes > plain.size()) {
            if (error) {
                *error = "CharaInitParam row " + std::to_string(id) +
                         " runs past the end of the archive";
            }
            return false;
        }

        bool changed = false;
        for (int slot = 0; slot < kClothingSlots; slot++) {
            // A slot with nothing ticked is LEFT ALONE, not written with a
            // sentinel. Unlike the two weapon fields, all four clothing fields
            // hold a real garment on the shipped rows, so writing anything here
            // would undress the character rather than leave it as the game
            // intended.
            if (result.granted[slot] == 0) continue;

            // One s32 over another, and NOTHING ELSE in the row: every other byte
            // is left exactly as the game shipped it, including the three
            // untouched clothing fields, the two weapon fields that START WITH A
            // TRICK WEAPON and START WITH A LEFT WEAPON write, the ten item slots
            // that START WITH HUNTER TOOLS and START WITH SUPPLIES write, and the
            // four *_GenId fields. All six writers can be on at once and none of
            // them can take another's bytes.
            WriteI32LE(plain, row->dataOffset + kEquipClothing[slot],
                       result.granted[slot]);
            result.fieldsWritten++;
            changed = true;
        }
        if (changed) result.rowsChanged++;
    }

    std::string what;
    for (int slot = 0; slot < kClothingSlots; slot++) {
        if (slot > 0) what += ", ";
        if (result.granted[slot] == 0) {
            what += "-";
            continue;
        }
        const char* name = ArmourPieceName(slot, result.granted[slot]);
        what += std::to_string(result.granted[slot]) + " (" +
                std::string(name != nullptr ? name : "UNKNOWN") + ")";
    }
    Log(("armour: granted head/body/gloves/legwear = " + what + " to " +
         std::to_string(result.rowsChanged) + " origin rows, " +
         std::to_string(result.fieldsWritten) + " fields written, " +
         std::to_string(result.drawsMade) + " draws, " +
         std::to_string(result.rowsMissing) + " rows missing").c_str());

    return true;
}

} // namespace bbr
