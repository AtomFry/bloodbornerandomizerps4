// WeaponRequirements.h - the four stat requirements on an EquipParamWeapon row,
// and the one writer both features that lower them go through.
//
// TWO FEATURES LOWER REQUIREMENTS, AND THEY CAN COLLIDE. RANDOMIZE STARTING
// WEAPONS gives whatever lands in a Hunter's Dream coffin the profile that slot
// has always carried, so the choice is usable at level 4. START WITH A TRICK
// WEAPON gives the weapon it grants the origin minimum, so the granted weapon is
// usable at once. Tick a weapon that the coffin pass also happens to draw and
// both want to write the same four bytes of the same eleven rows.
//
// WHY A WRITER AND NOT AN ORDERING. Spec 037 D4 says the GRANT wins, and says it
// must be an explicit rule in the code rather than a consequence of which of two
// calls runs second. So this records an owner rank per weapon id and applies a
// profile only when the incoming rank is at least as high as the one already
// recorded. Grant outranks CoffinSlot, so:
//
//   coffin then grant -> the grant's profile is applied over it
//   grant then coffin -> the coffin's profile is REFUSED
//
// and the bytes are the same either way. Rearranging StepItemData cannot change
// the outcome, which is the whole point.
//
// ">= the recorded rank", not "> it", so a second write at the SAME rank still
// lands: the coffin pass writes five different slots, and two of them drawing
// the same weapon must not leave the second one unwritten.
//
// SINGLE BYTES. All four requirements are u8. A four-byte write at 237 silently
// corrupts the three neighbours - the same trap CharaInitRows.h documents for
// the item counts, and the reason these are written one byte at a time.
#pragma once

#include <cstdint>
#include <vector>

#include "../Param/ParamBnd.h"

namespace bbr {

// Field offsets within an EQUIP_PARAM_WEAPON_ST row, from
// tools/param_offsets.py against the real paramdef. properMagic is BLOODTINGE
// and properFaith is ARCANE - confirmed independently by the order of
// baseStr/baseDex/baseMag/baseFai in CharaInitParam, which is what makes the
// origin minimum land on 9/9/5/6.
const size_t kProperStrength = 237;
const size_t kProperAgility  = 238;
const size_t kProperMagic    = 239;
const size_t kProperFaith    = 240;

// A weapon's ten upgrade variants are its id + 100n. Verified against the real
// EquipParamWeapon: every tier exists for all five vanilla starters and for all
// 78 grantable trick weapon versions, so a profile always covers eleven rows.
const int kUpgradeTiers = 10;
const int32_t kUpgradeStride = 100;

// The four values as one thing, so a profile is passed rather than four bytes
// in an order a caller can get wrong.
struct WeaponRequirements {
    uint8_t strength, agility, magic, faith;
};

// Who is asking. Higher wins. Deliberately an explicit rank rather than a bool:
// adding a third feature that lowers requirements means adding a rank here, not
// reasoning about call order again.
enum class ReqOwner { CoffinSlot = 0, Grant = 1 };

inline const ParamRow* FindWeaponRow(const std::vector<ParamRow>& rows, int32_t id) {
    for (const ParamRow& r : rows) {
        if (r.id == id) return &r;
    }
    return nullptr;
}

// Created once per run, in StepItemData, and shared by every pass that lowers a
// requirement. Holding it per pass would put the precedence rule back into call
// order, which is exactly what D4 forbids.
class WeaponRequirementWriter {
public:
    // Applies `req` to `weaponId` and each of its ten upgrade tiers, unless a
    // higher-ranked owner has already claimed that weapon. Returns the number
    // of rows written - 0 when the write was refused, or when the weapon is
    // absent from the param. A missing tier is skipped rather than an error.
    int Apply(std::vector<uint8_t>& plain, const std::vector<ParamRow>& weaponRows,
              int32_t weaponId, const WeaponRequirements& req, ReqOwner owner) {
        Claim* existing = Find(weaponId);
        if (existing != nullptr && (int)owner < (int)existing->owner) return 0;

        int written = 0;
        for (int tier = 0; tier <= kUpgradeTiers; tier++) {
            const ParamRow* row = FindWeaponRow(weaponRows, weaponId + tier * kUpgradeStride);
            if (row == nullptr) continue;
            plain[row->dataOffset + kProperStrength] = req.strength;
            plain[row->dataOffset + kProperAgility]  = req.agility;
            plain[row->dataOffset + kProperMagic]    = req.magic;
            plain[row->dataOffset + kProperFaith]    = req.faith;
            written++;
        }

        if (existing != nullptr) existing->owner = owner;
        else claims_.push_back({ weaponId, owner });
        return written;
    }

private:
    struct Claim {
        int32_t  weaponId;
        ReqOwner owner;
    };

    Claim* Find(int32_t weaponId) {
        for (Claim& c : claims_) {
            if (c.weaponId == weaponId) return &c;
        }
        return nullptr;
    }

    // At most six entries on any run - five coffin slots and one grant - so a
    // linear scan over a vector is the whole data structure.
    std::vector<Claim> claims_;
};

} // namespace bbr
