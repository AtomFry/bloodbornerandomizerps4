// cstdlib before anything pulling <random>/<cmath>, or the toolchain's libc++
// fails on ::abs - same ordering constraint as BossRandomizer.cpp.
#include <cstdlib>

#include "TrickWeaponGrant.h"

#include "CharaInitRows.h"
#include "TrickWeaponTable.h"

#include "../Msb/BinUtil.h"
#include "../Platform/Log.h"

namespace bbr {

namespace {

// CHARACTER_INIT_PARAM, from tools/param_offsets.py against the real paramdef.
// The row's shape is in CharaInitRows.h; this is the ONE field this feature
// writes, and the only one it ever wrote after the milestone-3 probe settled
// which of the two candidate routes the game reads (see the header).
//
// equip_Wep_Right is -1 on all 1700 rows of the shipped param, so nothing in the
// data said the game reads it; the console said it does, and equips it. Its
// three siblings (equip_Subwep_Right, equip_Wep_Left, equip_Subwep_Left) and the
// four *_GenId fields just past the requirement bytes are deliberately LEFT AT
// -1: the grant works without them, and writing a field on a guess is how a
// route that works stops being explicable.
const size_t kEquipWepRight = 16; // equip_Wep_Right, s32

// Spec 037 D3. The component-wise minimum of the ten origins' four starting
// stats, measured from CharaInitParam rather than chosen: strength 9, skill 9,
// bloodtinge 5, arcane 6. Every origin meets it, including Waste of Skin at
// level 4, so whatever is granted can be swung at once - which matters because
// 39 of the 78 rows are wieldable by NO origin as the game ships them.
//
// The reduction persists on that weapon's ten upgrade tiers for the whole run.
// That side effect is accepted deliberately.
const WeaponRequirements kGrantProfile = { 9, 9, 5, 6 };

} // namespace

const char* TrickWeaponName(int32_t weaponId) {
    const std::array<ModelPoolEntry, kTrickWeaponCount>& table = TrickWeaponTable();
    for (int i = 0; i < kTrickWeaponCount; i++) {
        if (strtol(table[(size_t)i].model, nullptr, 10) == weaponId) {
            return table[(size_t)i].displayName;
        }
    }
    return nullptr;
}

bool GrantTrickWeapon(std::vector<uint8_t>& plain,
                      const ParamMember& charaInitParam,
                      const ParamMember& weaponParam,
                      const TrickWeaponSelection& selection,
                      std::mt19937& rng,
                      WeaponRequirementWriter& reqWriter,
                      TrickWeaponGrantResult& result,
                      std::string* error) {
    // Step 1: the candidate list. The table holds the weapon id as a decimal
    // string in `model` because that is the field the picker keys and logs on;
    // there is deliberately no second array of ids to fall out of step with it.
    const std::array<ModelPoolEntry, kTrickWeaponCount>& table = TrickWeaponTable();
    std::vector<int32_t> candidates;
    for (int i = 0; i < kTrickWeaponCount; i++) {
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

    // Step 2: ONE draw, and this pass is the last RNG consumer of the run, so
    // turning the setting on cannot move any other roll.
    std::uniform_int_distribution<int> pick(0, (int)candidates.size() - 1);
    const int32_t weapon = candidates[(size_t)pick(rng)];
    result.weaponGranted = weapon;

    // Step 3: the requirement profile, as the highest-ranked owner. Where the
    // coffin pass drew the same weapon, this is the profile that stands - and it
    // stands whichever of the two passes ran first (spec 037 D4).
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

        // One s32 over another, and NOTHING ELSE in the row: every other byte
        // is left exactly as the game shipped it, including the starting
        // Hunter's Mark, the four clothing entries, and the ten item slots -
        // which is what keeps START WITH HUNTER TOOLS' write in the same rows
        // intact whichever feature runs first.
        WriteI32LE(plain, row->dataOffset + kEquipWepRight, weapon);

        result.rowsChanged++;
    }

    const char* name = TrickWeaponName(weapon);
    Log(("trick weapon: granted " + std::to_string(weapon) + " (" +
         std::string(name != nullptr ? name : "UNKNOWN") + ") to " +
         std::to_string(result.rowsChanged) + " origin rows, drawn from " +
         std::to_string(candidates.size()) + " ticked, " +
         std::to_string(result.reqRowsWritten) + " requirement rows rewritten, " +
         std::to_string(result.rowsMissing) + " rows missing").c_str());

    return true;
}

} // namespace bbr
