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
#pragma once

#include "BossPoolTable.h"
#include "EnemyPoolTable.h"
#include "EnemySkipTable.h"
#include "LeftHandWeaponTable.h"
#include "ModelPoolSelection.h"
#include "TrickWeaponTable.h"

namespace bbr {

typedef ModelPoolSelection<kEnemyPoolModelCount> EnemyPoolSelection;
typedef ModelPoolSelection<kBossPoolModelCount>  BossPoolSelection;
typedef ModelPoolSelection<kEnemySkipModelCount, false> EnemySkipSelection;
typedef ModelPoolSelection<kTrickWeaponCount, false> TrickWeaponSelection;
typedef ModelPoolSelection<kLeftHandWeaponCount, false> LeftHandWeaponSelection;

} // namespace bbr
