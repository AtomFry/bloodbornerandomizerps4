// EnemyPoolSelection.h - the enemy and boss picker selection types.
//
// Both are the same thing pointed at different baked tables, so the logic
// lives once in ModelPoolSelection.h and this only binds the sizes. The file
// keeps its original name because every include of it still wants exactly
// this.
//
// Each type is paired with one table and the pairing is positional - a
// selection encoded against EnemyPoolTable() is meaningless against
// BossPoolTable(). Pass the matching table to IsModelEnabled; the
// *_pool_verify.py table checks are what keep each table honest.
//
// EnemySkipSelection is the odd one out and its second template argument is
// the whole reason: it constructs to NOTHING selected, and an unknown model
// reads as NOT skipped. Ticking a row there means "leave this creature
// alone", so every fail-safe has to point the opposite way from the two
// pickers above - see ModelPoolSelection.h and feature 032.
//
// TrickWeaponSelection is not creatures at all - it is the 78 right-hand trick
// weapon versions START WITH A TRICK WEAPON can grant (feature 037). It shares
// this machinery because the question is the same shape (which rows of a baked
// table are ticked) and the table rows are the same struct, with the weapon id
// in `model`. Its `false` is the same fail-safe as the skip list, pointing the
// same way: nothing ticked grants nothing, so a fresh struct, an absent key and
// a wrong-length value all mean "the run the app made before this existed".
//
// LeftHandWeaponSelection is the same thing for the other hand - the 14
// left-hand weapons START WITH A LEFT-HAND WEAPON can grant (feature 038). Same
// machinery, same fail-safe, a different table. The two are independent
// selections and the two tables are DIFFERENT SIZES, which is exactly why each
// type is bound to one table here: a 14-character line decoded against the
// 78-row table, or the reverse, is rejected by the length guard rather than
// silently remapped.
//
// InsertedBossSelection and InsertAreaSelection are feature 013's two, and both
// are all-ticked by default - the polarity of the two pickers above rather than
// of the skip list - because all-ticked is what the run does when the master
// toggle BOSSES CAN REPLACE ENEMIES is off, which is nothing at all. So a
// defaults.cfg written before either key existed reads as every area and every
// boss enabled with the pass switched off, and that is today's run exactly.
//
// InsertedBossSelection is NOT BossPoolSelection with a different size. The two
// tables are harvested by different rules over different reject lists and are
// 22 and 17 rows; a 22-character line decoded against the 17-row table is
// rejected by the length guard, which is the same reason every other type here
// names exactly one table.
//
// The four armour selections are feature 040's, and they are FOUR rather than
// one because a player wears four pieces and each slot is drawn independently.
// Same machinery again, same fail-safe as the two weapon lists - nothing ticked
// grants nothing, so a fresh struct, an absent key and a wrong-length value all
// leave that slot's vanilla garment alone - and four different sizes, 33 / 34 /
// 26 / 31. The sizes differing is the same protection it is everywhere else
// here: a line pasted into the wrong key is rejected by the length guard rather
// than remapped onto another slot's pieces. Only the gloves and legwear counts
// are close enough for that to be a real risk, and 26 against 31 still differ.
#pragma once

#include "ArmourTables.h"
#include "BossPoolTable.h"
#include "EnemyPoolTable.h"
#include "EnemySkipTable.h"
#include "InsertAreaTable.h"
#include "InsertedBossTable.h"
#include "LeftHandWeaponTable.h"
#include "ModelPoolSelection.h"
#include "TrickWeaponTable.h"

namespace bbr {

typedef ModelPoolSelection<kEnemyPoolModelCount> EnemyPoolSelection;
typedef ModelPoolSelection<kBossPoolModelCount>  BossPoolSelection;
typedef ModelPoolSelection<kEnemySkipModelCount, false> EnemySkipSelection;
typedef ModelPoolSelection<kTrickWeaponCount, false> TrickWeaponSelection;
typedef ModelPoolSelection<kLeftHandWeaponCount, false> LeftHandWeaponSelection;
typedef ModelPoolSelection<kInsertedBossModelCount> InsertedBossSelection;
typedef ModelPoolSelection<kInsertAreaCount>       InsertAreaSelection;
typedef ModelPoolSelection<kHeadwearCount, false>   HeadwearSelection;
typedef ModelPoolSelection<kBodyArmourCount, false> BodyArmourSelection;
typedef ModelPoolSelection<kGlovesCount, false>     GlovesSelection;
typedef ModelPoolSelection<kLegwearCount, false>    LegwearSelection;

} // namespace bbr
