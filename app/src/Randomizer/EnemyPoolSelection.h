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
#pragma once

#include "BossPoolTable.h"
#include "EnemyPoolTable.h"
#include "ModelPoolSelection.h"

namespace bbr {

typedef ModelPoolSelection<kEnemyPoolModelCount> EnemyPoolSelection;
typedef ModelPoolSelection<kBossPoolModelCount>  BossPoolSelection;

} // namespace bbr
