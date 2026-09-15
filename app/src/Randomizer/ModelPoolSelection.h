// ModelPoolSelection.h - which creatures are allowed into a randomizer's
// candidate pool. Shared by the enemy picker and the boss picker, which behave
// identically and differ only in which baked table they are pointed at.
//
// WHAT UNTICKING MEANS. An unticked creature is never used as a REPLACEMENT.
// It does not protect that creature's own placements, which are still
// randomized like any other - see docs/plans/pickers.md section 1.
// This matches the reference tool's oopsAll/excludeEnemies semantics.
//
// All-enabled is the default and is exactly the behaviour that existed before
// the pickers, so a config written before they existed, or one whose value is
// the wrong length, reads as "everything on" and changes nothing.
#pragma once

#include <string>

namespace bbr {

// One row of a generated pool table (EnemyPoolTable.h, BossPoolTable.h).
struct ModelPoolEntry {
    const char* model;       // "c2630"
    const char* displayName; // uppercase, renderable by Font8x8.cpp
    int         poolEntries; // draw weight; see the generator's header comment
};

// N is the row count of the table this selection is paired with. The table
// itself is passed to IsModelEnabled rather than stored, so this stays a
// plain value type that RandomizerDefaults can copy freely (the Setup
// Defaults screen keeps a whole working copy of it).
template <int N>
struct ModelPoolSelection {
    bool enabled[N];

    ModelPoolSelection() { EnableAll(); }

    void EnableAll()  { for (int i = 0; i < N; i++) enabled[i] = true; }
    void DisableAll() { for (int i = 0; i < N; i++) enabled[i] = false; }
    void Toggle(int i) { if (i >= 0 && i < N) enabled[i] = !enabled[i]; }

    int CountEnabled() const {
        int n = 0;
        for (int i = 0; i < N; i++) if (enabled[i]) n++;
        return n;
    }
    bool AllEnabled() const  { return CountEnabled() == N; }
    bool NoneEnabled() const { return CountEnabled() == 0; }
    static int Count() { return N; }

    // The randomizers' only entry point: is this model allowed in the pool?
    // Linear over a short table of short strings, called once per placement
    // during one pass - not worth a map, and a map would need building per run.
    bool IsModelEnabled(const std::string& model, const ModelPoolEntry* table) const {
        for (int i = 0; i < N; i++) {
            if (model == table[i].model) return enabled[i];
        }
        // Not in the table at all, which means the table is stale relative to
        // the pool rules. Silently dropping the model would quietly shrink the
        // pool, so allow it through - that is the behaviour these features had
        // before they existed. The *_pool_verify.py table check is what is
        // supposed to catch this.
        return true;
    }

    // "0110111..." - one character per table row, positional. Deliberately not
    // a bitmask: readable in a text config, debuggable by eye, and still only
    // N bytes.
    std::string Encode() const {
        std::string s;
        s.reserve(N);
        for (int i = 0; i < N; i++) s += enabled[i] ? '1' : '0';
        return s;
    }

    // Returns false and leaves the selection untouched unless the string is
    // exactly the right length. Length is the only guard available against a
    // regenerated table: it catches a changed COUNT and cannot catch a changed
    // ORDER, which is why the generators' order is frozen.
    bool Decode(const std::string& s) {
        if ((int)s.size() != N) return false;
        for (int i = 0; i < N; i++) enabled[i] = (s[(size_t)i] != '0');
        return true;
    }
};

} // namespace bbr
