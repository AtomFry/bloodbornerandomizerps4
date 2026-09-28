// cstdlib before anything pulling <random>/<cmath>, or the toolchain's libc++
// fails on ::abs - same ordering constraint as BossRandomizer.cpp.
#include <cstdlib>

#include "LeftHandWeaponGrant.h"

#include "CharaInitRows.h"
#include "LeftHandWeaponTable.h"

#include "../Msb/BinUtil.h"
#include "../Platform/Log.h"

namespace bbr {

namespace {

// CHARACTER_INIT_PARAM, from tools/param_offsets.py against the real paramdef.
// The row's shape is in CharaInitRows.h; this is the ONE field this feature
// writes.
//
// equip_Wep_Left sits eight bytes past equip_Wep_Right, the field feature 037's
// hardware test proved the game reads and equips (see LeftHandWeaponGrant.h).
// Both are -1 on all 1700 rows of the shipped param, so the data says nothing
// about either; the console said the right-hand one is read. That the left-hand
// sibling behaves the same way is an inference, and it is what this feature's
// hardware test is for.
//
// equip_Subwep_Left and the four *_GenId fields just past the requirement bytes
// are deliberately LEFT AT -1: the right-hand grant worked without its own
// siblings, and writing a field on a guess is how a route that works stops being
// explicable.
const size_t kEquipWepLeft = 24; // equip_Wep_Left, s32

// The same profile feature 037 grants, and the same reason: the component-wise
// minimum of the ten origins' four starting stats, measured from CharaInitParam
// rather than chosen - strength 9, skill 9, bloodtinge 5, arcane 6. Every origin
// meets it, including Waste of Skin at level 4.
//
// It matters more on this hand than the other. The Cannon wants 30 strength, the
// Gatling Gun 28, the Church Cannon 27 strength and 16 bloodtinge, Evelyn 18
// bloodtinge - none of which any origin meets, so without this most of the
// picker would grant a weapon the character cannot fire.
//
// The reduction persists on that weapon's upgrade tiers for the whole run. That
// side effect is accepted deliberately, exactly as it is for feature 037.
const WeaponRequirements kGrantProfile = { 9, 9, 5, 6 };

} // namespace

const char* LeftHandWeaponName(int32_t weaponId) {
    const std::array<ModelPoolEntry, kLeftHandWeaponCount>& table = LeftHandWeaponTable();
    for (int i = 0; i < kLeftHandWeaponCount; i++) {
        if (strtol(table[(size_t)i].model, nullptr, 10) == weaponId) {
            return table[(size_t)i].displayName;
        }
    }
    return nullptr;
}

bool GrantLeftHandWeapon(std::vector<uint8_t>& plain,
                         const ParamMember& charaInitParam,
                         const ParamMember& weaponParam,
                         const LeftHandWeaponSelection& selection,
                         std::mt19937& rng,
                         WeaponRequirementWriter& reqWriter,
                         LeftHandWeaponGrantResult& result,
                         std::string* error) {
    // Step 1: the candidate list. The table holds the weapon id as a decimal
    // string in `model` because that is the field the picker keys and logs on;
    // there is deliberately no second array of ids to fall out of step with it.
    const std::array<ModelPoolEntry, kLeftHandWeaponCount>& table = LeftHandWeaponTable();
    std::vector<int32_t> candidates;
    for (int i = 0; i < kLeftHandWeaponCount; i++) {
        if (selection.enabled[i]) {
            candidates.push_back((int32_t)strtol(table[(size_t)i].model, nullptr, 10));
        }
    }

    // Nothing ticked: write nothing, draw nothing. Returning BEFORE the RNG is
    // touched is what makes turning the setting on with an empty selection
    // identical to leaving it off, roll for roll.
    if (candidates.empty()) return true;

    std::vector<ParamRow> charaRows;
    if (!ParseParamRows(plain, charaInitParam, charaRows, error)) return false;
    std::vector<ParamRow> weaponRows;
    if (!ParseParamRows(plain, weaponParam, weaponRows, error)) return false;

    // Step 2: ONE draw, from this table alone. The right-hand grant has already
    // drawn from its own; the two are independent, and because the two passes are
    // the last RNG consumers of the run neither can move an earlier roll.
    std::uniform_int_distribution<int> pick(0, (int)candidates.size() - 1);
    const int32_t weapon = candidates[(size_t)pick(rng)];
    result.weaponGranted = weapon;

    // Step 3: the requirement profile, as the highest-ranked owner. Where the
    // coffin pass drew the same weapon - RANDOMIZE STARTING GUNS puts firearms
    // into the Dream - this is the profile that stands, and it stands whichever
    // of the passes ran first (spec 037 D4, inherited).
    //
    // Eleven rows for thirteen of the 14. The Loch Shield has no upgrade tiers
    // at all, so it is one row; the writer skips a missing tier rather than
    // failing, which is what makes that unremarkable.
    result.reqRowsWritten = reqWriter.Apply(plain, weaponRows, weapon, kGrantProfile,
                                            ReqOwner::Grant);

    // Step 4: the write, over every origin row.
    for (int32_t id : kOriginRows) {
        const ParamRow* row = FindRow(charaRows, id);
        if (row == nullptr) {
            // Not fatal on its own: a row missing from one block still leaves
            // the other block written. Reported so a wholesale mismatch - a
            // different game version, say - is visible in the log rather than
            // looking like a silent success.
            result.rowsMissing++;
            continue;
        }

        // ParseParamRows only proves the row STARTS inside the member. The
        // whole 300-byte row is checked rather than just the four bytes written
        // below: the row is the unit the param format guarantees, and a row that
        // does not fit means the archive is not the layout every offset here was
        // measured against.
        if (row->dataOffset + kRowBytes > plain.size()) {
            if (error) {
                *error = "CharaInitParam row " + std::to_string(id) +
                         " runs past the end of the archive";
            }
            return false;
        }

        // One s32 over another, and NOTHING ELSE in the row: every other byte is
        // left exactly as the game shipped it, including the starting Hunter's
        // Mark, the four clothing entries, the ten item slots that START WITH
        // HUNTER TOOLS writes, and equip_Wep_Right, which START WITH A TRICK
        // WEAPON writes eight bytes earlier. All three features can be on at
        // once and none of them can take another's bytes.
        WriteI32LE(plain, row->dataOffset + kEquipWepLeft, weapon);

        result.rowsChanged++;
    }

    const char* name = LeftHandWeaponName(weapon);
    Log(("left-hand weapon: granted " + std::to_string(weapon) + " (" +
         std::string(name != nullptr ? name : "UNKNOWN") + ") to " +
         std::to_string(result.rowsChanged) + " origin rows, drawn from " +
         std::to_string(candidates.size()) + " ticked, " +
         std::to_string(result.reqRowsWritten) + " requirement rows rewritten, " +
         std::to_string(result.rowsMissing) + " rows missing").c_str());

    return true;
}

} // namespace bbr
