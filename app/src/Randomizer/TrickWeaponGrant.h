// TrickWeaponGrant.h - START WITH A TRICK WEAPON. Puts one of the 78 right-hand
// trick weapon versions into a new character's possession at character creation,
// with its requirements lowered so it can be used immediately.
//
// What the player sees: in vanilla a new character wakes in Iosefka's Clinic
// empty-handed and the first weapon is the Hunter's Dream coffin choice. With a
// weapon ticked here the character wakes HOLDING it, equipped in the right
// hand - confirmed on hardware, see below. Tick nothing and nothing is granted;
// tick one and every new character gets it; tick several and one is drawn for
// the run.
//
// This has NO reference-tool counterpart. The string CharaInitParam does not
// occur anywhere in reference/Randomizer/ - the Windows tool never touches the
// character-creation template. The nearest thing it does is randomize the
// weapons ON SALE, which this port already ships as RANDOMIZE STARTING WEAPONS
// and which changes what the Dream offers, never what the character holds. So
// there is no reference behaviour to match and no quirk to preserve.
//
// What it edits: the 22 player-origin rows of CharaInitParam.param, the same
// rows START WITH HUNTER TOOLS writes (CharaInitRows.h holds the list once), plus
// the four requirement bytes of the granted weapon and its ten upgrade tiers
// through the run's shared WeaponRequirementWriter. The archive is never
// rebuilt: every write is one value over another inside an existing row.
//
// HARDWARE-CONFIRMED, 2026-09-27, AND THE WHOLE FEATURE RODE ON IT. Nothing
// observed offline could show that Bloodborne reads a weapon written into
// equip_Wep_Right: that field, and its three siblings, are -1 on all 1700 rows
// of the shipped param - player origins and NPC templates alike - so the route
// this feature needs is the one the game's own data never exercises. What IS
// exercised is the neighbouring clothing fields (set on 1557 rows, which is
// where the starting Black Hood comes from) and the ordinary inventory slots,
// which carry weapon ids on three shipped rows.
//
// So the milestone-3 build wrote BOTH candidate routes, with a different weapon
// on each, so one hardware cycle could tell them apart: equip_Wep_Right = the
// drawn weapon, and a first-free inventory slot = a fixed Threaded Cane. Two
// rows were ticked and a new character was started: the character spawned in
// Iosefka's Clinic HOLDING the drawn weapon. equip_Wep_Right is therefore read
// at character creation, and it EQUIPS rather than merely granting - which is
// the outcome spec 037 D5 preferred.
//
// The inventory route and its fixed probe weapon were deleted with that answer.
// This file now writes exactly ONE field per origin row. equip_Wep_Right_GenId
// and the three sibling equip_* fields stay at -1 deliberately: nothing in the
// data says they are a required companion to this field, the probe granted the
// weapon without them, and writing a field on a guess is how a route that works
// stops being explicable.
#pragma once

#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include "EnemyPoolSelection.h"
#include "WeaponRequirements.h"

#include "../Param/ParamBnd.h"

namespace bbr {

struct TrickWeaponGrantResult {
    int32_t weaponGranted = 0;  // the drawn weapon id; 0 when nothing was ticked
    int rowsChanged = 0;        // origin rows that received the grant
    int reqRowsWritten = 0;     // base row + upgrade tiers given the profile
    int rowsMissing = 0;        // targets absent from the real param
};

// Draws one weapon from the ticked rows and writes it into every origin row, in
// place inside `plain` (the decompressed archive).
//
// DRAWS EXACTLY ONCE, AND ONLY WHEN SOMETHING IS TICKED. An empty selection
// returns immediately having written nothing and drawn nothing, so turning the
// setting on with nothing ticked leaves the run identical roll for roll. The
// caller runs this LAST of the param passes for the same reason: one draw at the
// very end of the run cannot move any other roll.
//
// Returns false only on a structural problem, with `error` set; partial edits
// are possible in that case, so the caller should treat a failure as fatal to
// the run rather than continuing.
bool GrantTrickWeapon(std::vector<uint8_t>& plain,
                      const ParamMember& charaInitParam,
                      const ParamMember& weaponParam,
                      const TrickWeaponSelection& selection,
                      std::mt19937& rng,
                      WeaponRequirementWriter& reqWriter,
                      TrickWeaponGrantResult& result,
                      std::string* error);

// The display name of a granted weapon id, for the progress line, or nullptr if
// the id is not in the table. Lives here rather than in the UI so no screen file
// learns how a weapon id is spelled.
const char* TrickWeaponName(int32_t weaponId);

} // namespace bbr
