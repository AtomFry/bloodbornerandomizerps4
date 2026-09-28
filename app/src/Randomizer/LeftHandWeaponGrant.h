// LeftHandWeaponGrant.h - START WITH A LEFT-HAND WEAPON. Puts one of the 14
// left-hand weapons into a new character's possession at character creation,
// with its requirements lowered so it can be used immediately.
//
// THIS IS START WITH A TRICK WEAPON FOR THE OTHER HAND. Everything structural
// here is feature 037's, shared rather than copied: the 22 player-origin rows
// (CharaInitRows.h), the owner-ranked requirement writer (WeaponRequirements.h),
// the picker machinery, the config encoding and the same 9/9/5/6 profile. The
// only differences are which field is written - equip_Wep_Left, s32 at offset
// 24, eight bytes past the field 037 writes at 16 - and which table of weapons
// is offered.
//
// What the player sees: in vanilla a new character wakes in Iosefka's Clinic
// empty-handed. With a weapon ticked here the character wakes with it in the
// LEFT hand. Tick nothing and nothing is granted; tick one and every new
// character gets it; tick several and one is drawn for the run. Independent of
// the right-hand grant: both may be on, and each draws its own weapon.
//
// NO PROBE WAS NEEDED, AND THAT IS WHY THIS FEATURE HAS NO GATE. Feature 037's
// milestone-3 build wrote two candidate routes and the hardware test on
// 2026-09-27 answered: a new character spawned in Iosefka's Clinic HOLDING the
// weapon written into equip_Wep_Right of these same 22 rows. So the game reads
// this row family at character creation and honours an equip field in it. What
// the console has NOT been asked is whether equip_Wep_Left specifically behaves
// the same way as its sibling; that is the one thing this feature's hardware
// test settles, and until it does, the feature is BUILT and not DONE.
//
// What it edits: equip_Wep_Left in the 22 player-origin rows of
// CharaInitParam.param, plus the four requirement bytes of the granted weapon
// and its upgrade tiers through the run's shared WeaponRequirementWriter. The
// archive is never rebuilt: every write is one value over another inside an
// existing row. equip_Subwep_Left and the four *_GenId fields stay at -1
// deliberately, for the same reason feature 037 leaves its own siblings alone:
// the right-hand grant worked without them, and writing a field on a guess is
// how a route that works stops being explicable.
//
// WHY THE REQUIREMENTS HAVE TO COME DOWN, and why it matters more here than on
// the right hand: this list is the heavy end of the weapon param. The Cannon
// wants 30 strength, the Gatling Gun 28, the Church Cannon 27 strength and 16
// bloodtinge, Evelyn 18 bloodtinge. No origin meets any of those, so without the
// profile most of the picker would grant a weapon the character is holding and
// cannot fire.
//
// ONE OF THE 14 HAS NO UPGRADE TIERS. The Loch Shield (19100000) is the only row
// in either hand's table that ships with a base row and nothing else - no +1..+10
// - so the profile covers one row for it and eleven for the other thirteen.
// WeaponRequirementWriter skips a missing tier rather than treating it as an
// error, which is what makes that a non-event here.
//
// This has NO reference-tool counterpart, for the same reason feature 037 has
// none: the string CharaInitParam does not occur anywhere in
// reference/Randomizer/. The Windows tool never touches the character-creation
// template. RANDOMIZE STARTING GUNS, which this port already ships, changes what
// the Hunter's Dream and the Bath Messengers OFFER, never what the character
// holds.
#pragma once

#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include "EnemyPoolSelection.h"
#include "WeaponRequirements.h"

#include "../Param/ParamBnd.h"

namespace bbr {

struct LeftHandWeaponGrantResult {
    int32_t weaponGranted = 0;  // the drawn weapon id; 0 when nothing was ticked
    int rowsChanged = 0;        // origin rows that received the grant
    int reqRowsWritten = 0;     // base row + upgrade tiers given the profile
    int rowsMissing = 0;        // targets absent from the real param
};

// Draws one weapon from the ticked rows and writes it into every origin row's
// left-hand equip field, in place inside `plain` (the decompressed archive).
//
// DRAWS EXACTLY ONCE, AND ONLY WHEN SOMETHING IS TICKED. An empty selection
// returns immediately having written nothing and drawn nothing, so turning the
// setting on with nothing ticked leaves the run identical roll for roll.
//
// DRAWS INDEPENDENTLY OF THE RIGHT-HAND GRANT. The caller runs this after it, so
// the two passes are the last two RNG consumers of the run and neither can move
// any earlier roll. Adding this feature does shift nothing before it; it draws
// from its own table and never consults the right hand's choice.
//
// Returns false only on a structural problem, with `error` set; partial edits
// are possible in that case, so the caller should treat a failure as fatal to
// the run rather than continuing.
bool GrantLeftHandWeapon(std::vector<uint8_t>& plain,
                         const ParamMember& charaInitParam,
                         const ParamMember& weaponParam,
                         const LeftHandWeaponSelection& selection,
                         std::mt19937& rng,
                         WeaponRequirementWriter& reqWriter,
                         LeftHandWeaponGrantResult& result,
                         std::string* error);

// The display name of a granted weapon id, for the progress line, or nullptr if
// the id is not in the table. Lives here rather than in the UI so no screen file
// learns how a weapon id is spelled.
const char* LeftHandWeaponName(int32_t weaponId);

} // namespace bbr
