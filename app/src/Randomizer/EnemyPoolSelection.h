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
#pragma once

#include "BossPoolTable.h"
#include "EnemyPoolTable.h"
#include "EnemySkipTable.h"
#include "ModelPoolSelection.h"

namespace bbr {

typedef ModelPoolSelection<kEnemyPoolModelCount> EnemyPoolSelection;
typedef ModelPoolSelection<kBossPoolModelCount>  BossPoolSelection;
typedef ModelPoolSelection<kEnemySkipModelCount, false> EnemySkipSelection;

} // namespace bbr
