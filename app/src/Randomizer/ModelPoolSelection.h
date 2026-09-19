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
//
// POLARITY IS A TEMPLATE PARAMETER, NOT A CONVENTION. Feature 032's
// ENEMIES SKIPPED is the mirror image of the two pickers above: a ticked row
// means "leave this creature alone", so it must construct to NOTHING ticked,
// an unknown model must read as NOT ticked, and a stale saved value must leave
// NOTHING ticked. Those are the same three fail-safes as here, pointing the
// other way.
//
// The alternative - reuse ModelPoolSelection<85> and have every call site read
// "enabled" as "skipped" - was rejected deliberately: it constructs to
// EVERYTHING skipped, which freezes the whole game, and each of the three
// fail-safes would then depend on a caller remembering to invert. Making the
// default a parameter puts the polarity in the type, where it cannot be
// forgotten. DefaultSelected defaults to true, so ModelPoolSelection<82> and
// ModelPoolSelection<17> spell and behave exactly as they always have.
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
//
// DefaultSelected is the polarity described above: the state a fresh object
// starts in, and the answer IsModelEnabled gives for a model that is not in
// the table at all.
template <int N, bool DefaultSelected = true>
struct ModelPoolSelection {
    bool enabled[N];

    ModelPoolSelection() {
        if (DefaultSelected) EnableAll(); else DisableAll();
    }

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
        // the pool rules. Both answers here are "do what this feature did
        // before it existed", which is why the fallback is the polarity and
        // not a literal: for an inclusion list that means allowing the model
        // through rather than quietly shrinking the pool, and for a skip list
        // it means not freezing a creature the user never saw a row for.
        // The *_pool_verify.py table check is what is supposed to catch this.
        return DefaultSelected;
    }

    // Reads correctly at a skip-list call site. Same test, same table; the
    // name is the whole point, because "IsModelEnabled" at a site deciding
    // whether to LEAVE A CREATURE ALONE invites exactly the inversion bug the
    // polarity parameter exists to prevent.
    bool IsModelSkipped(const std::string& model, const ModelPoolEntry* table) const {
        return IsModelEnabled(model, table);
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
