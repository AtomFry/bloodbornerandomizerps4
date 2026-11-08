// ArmourGrant.h - START WITH HEADWEAR / BODY ARMOUR / GLOVES / LEGWEAR. Dresses
// a new character in up to four pieces of armour chosen at character creation.
//
// FOUR SETTINGS, ONE PASS. The four pickers are four separate settings a player
// ticks separately, but they are one pass because they draw together: that is
// what makes the draws independent of one another, and it is the whole reason
// this is not four copies of TrickWeaponGrant.cpp. See the four-draw rule below.
//
// What the player sees: in vanilla a new character wakes in Iosefka's Clinic
// wearing whatever its origin gives it - the four Yharnam Hunter garments for the
// 2000 block, the same three with a bare head for the 3000 block. Tick nothing in
// a picker and that slot keeps its vanilla garment. Tick one and every new
// character wears it. Tick several and one is drawn for the run.
//
// THE FOUR DRAWS ARE INDEPENDENT, AND NEITHER HALF OF THE MECHANISM IS OPTIONAL.
// Spec 040 B6: changing what is ticked in one slot must not change which piece a
// fixed seed draws in another. Two things are required for that and this file
// does both:
//
//   1. EXACTLY FOUR raw rng() words are consumed whenever the feature does
//      anything at all - one per slot, in slot order, whether or not that slot
//      has anything ticked. A slot with nothing ticked DISCARDS its word. Drawing
//      only for the ticked slots would mean an empty headwear picker shifting
//      every later slot's word along by one.
//
//   2. Each index is `word % candidates.size()`, NOT a
//      std::uniform_int_distribution. A distribution consumes a VARIABLE number
//      of raw words as the candidate count changes, which puts the coupling back
//      in through the other door: ticking one more body-armour row would then
//      move the gloves draw.
//
// The cost of (2) is a modulo bias, and it is accepted deliberately: mt19937
// yields 2^32 values and the largest candidate list here is 34, so the bias is
// about one part in 10^8 on a cosmetic choice. Determinism across slots is worth
// far more than that here, and it is the property the spec states.
//
// This pass runs LAST in StepItemData, after both weapon grants, so its four
// draws cannot move any earlier roll in the run. That is the same reasoning the
// two weapon grants use, extended by one: with this here, the run's RNG consumers
// are, in order, everything else, the right-hand grant, the left-hand grant, and
// then these four words.
//
// NOTHING TICKED ANYWHERE IS FREE. With all four selections empty the pass
// returns before touching the RNG and before parsing anything, so turning all
// four settings on with nothing ticked leaves a run identical roll for roll and
// byte for byte.
//
// NO REQUIREMENTS ARE INVOLVED, unlike either weapon grant. The armour row has no
// requirement fields at all - starting_gear_verify.py A7 asserts zero proper*
// fields on EQUIP_PARAM_PROTECTOR_ST against four on the weapon row - so there is
// nothing to lower, WeaponRequirements.h is not included here, and no equip-weight
// allowance is touched. A heavy piece is simply worn.
//
// What it edits: equip_Helm / equip_Armer / equip_Gaunt / equip_Leg in the 22
// player-origin rows of CharaInitParam.param, and only the ones whose picker had
// something ticked. The archive is never rebuilt: every write is one s32 over
// another inside an existing row. Every other byte of the row is left exactly as
// the game shipped it - the other clothing fields, the two weapon fields, the ten
// item slots and the four *_GenId fields, which stay at -1 for the same reason
// both weapon grants leave their own siblings alone.
//
// AN ARMOUR ID GOES ONLY IN A CLOTHING FIELD, and only in its own slot's. The
// pieces are drawn from four tables that are already partitioned by
// protectorCategory, and starting_gear_verify.py A8 establishes that
// id/1000 % 10 equals that category on every wearable row - so a piece in the
// wrong field is checkable from the id alone, and G6 checks it.
//
// This has NO reference-tool counterpart, for the same reason features 037 and
// 038 have none: the string CharaInitParam does not occur anywhere in
// reference/Randomizer/. The Windows tool never touches the character-creation
// template.
#pragma once

#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include "EnemyPoolSelection.h"

#include "../Param/ParamBnd.h"

namespace bbr {

struct ArmourGrantResult {
    // The piece granted per slot, in the frozen slot order - head, body, gloves,
    // legwear - or 0 where that slot had nothing ticked and was left alone.
    // Indexed by kEquipClothing's index, which is protectorCategory.
    //
    // The id rather than a bool because the UI names the piece on the progress
    // line, and because "granted something" and "granted THIS" are different
    // claims when a picker holds 34 rows.
    int32_t granted[4] = { 0, 0, 0, 0 };
    int rowsChanged = 0;   // origin rows that received at least one piece
    int fieldsWritten = 0; // clothing fields written across all of those rows
    int rowsMissing = 0;   // targets absent from the real param
    int drawsMade = 0;     // raw rng() words consumed: 0 or exactly 4, never else
};

// Draws one piece per ticked slot and writes each into every origin row's own
// clothing field, in place inside `plain` (the decompressed archive).
//
// DRAWS EXACTLY FOUR WORDS, OR NONE. Four selections with nothing ticked between
// them return immediately having written nothing and drawn nothing. Anything
// ticked draws four - one per slot, in slot order - and a slot with nothing
// ticked discards its word rather than skipping it. That constant draw count is
// what makes the four slots independent (B6); see the header comment above.
//
// Returns false only on a structural problem, with `error` set; partial edits are
// possible in that case, so the caller should treat a failure as fatal to the run
// rather than continuing.
bool GrantArmour(std::vector<uint8_t>& plain,
                 const ParamMember& charaInitParam,
                 const HeadwearSelection& headwear,
                 const BodyArmourSelection& bodyArmour,
                 const GlovesSelection& gloves,
                 const LegwearSelection& legwear,
                 std::mt19937& rng,
                 ArmourGrantResult& result,
                 std::string* error);

// The display name of a granted piece, for the progress line, or nullptr if the
// id is not in that slot's table. `slot` is 0..3 in the frozen slot order. Lives
// here rather than in the UI so no screen file learns how an armour id is spelled
// or which table a slot uses.
const char* ArmourPieceName(int slot, int32_t armourId);

} // namespace bbr
