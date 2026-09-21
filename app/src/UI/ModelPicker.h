// ModelPicker.h - the shared drill-in creature list. Drives the enemy picker
// (82 rows), the boss picker (17) and ENEMIES SKIPPED (85), which behave
// identically; the host supplies the table, the count and the flags.
//
// The first two are INCLUSION lists - ticking a row lets that creature be
// used as a replacement. The third is the opposite: ticking a row leaves the
// creature out of the run entirely. The component cannot tell them apart and
// must not try, so every user-visible word it draws comes from the host as
// PickerStrings. Feature 032 added that rather than re-wording the component,
// because the two shipped pickers must render exactly as they do today.
//
// NOT a Screen. Application.cpp constructs screens fresh on every switch, so
// a real drill-in would destroy the Enable wizard's in-progress toggles on the
// way back - the same reason SetupDefaultsScreen's title-ID editor is an
// internal mode rather than its own ScreenId. This is a component both hosting
// screens own and delegate to while their own mode says so.
//
// The host owns the selection; this only edits what it is handed.
//
// Layout is denser than the settings screens (item scale 3, 52px spacing, 12
// visible rows instead of 6): 82 rows at the settings screens' size would be
// fourteen pages. Scale 3 is already proven here - it is what the progress log
// uses. Geometry is asserted by tools/ui_scroll_verify.py.
//
// A picker with an instruction line gets 11 visible rows instead of 12: the
// list starts lower to make room, and the heading and count line do NOT move.
// That is spec 032 F13's option B, chosen so the two shipped pickers keep
// their exact pixels and so MORE ABOVE keeps clear of the instruction line
// above it (the 27px-vs-14px figures that used to be quoted here were from
// the old 8 * scale height model and no longer describe anything).
// 85 rows is 8 pages at 11 and at 12 alike, so the lost row costs nothing.
#pragma once

#include "Controls.h"

#include "../Randomizer/EnemyPoolSelection.h"

namespace bbr {

class Renderer;

// Every word the picker draws that depends on what the list MEANS. Passed to
// both Update and Draw rather than stored, for two reasons: Update needs it
// too (the visible row count depends on whether there is an instruction
// line), and a required parameter makes a missed call site a compile error
// instead of a picker that silently draws the wrong vocabulary.
struct PickerStrings {
    const char* heading;      // "ENEMIES SKIPPED"
    const char* instruction;  // second line under the count, or nullptr
    const char* flagOn;       // flag column when the row is ticked
    const char* flagOff;      // ...and when it is not
    const char* verbAll;      // confirm prompt, e.g. "SKIP ALL"
    const char* verbNone;     // ...e.g. "SKIP NONE"
    const char* footer;       // the second footer line, naming both verbs
};

// The three lists' vocabulary, defined once because both hosting screens draw
// the same picker and a second copy is a second thing to forget to change.
//
// The atlas draws printable ASCII (32..126) and is proportional, so what a
// string actually costs is Renderer::TextWidth(), never its length. The two
// budgets pool_verify.py's selftest still asserts - renderable by
// Font8x8.cpp (A-Z, 0-9, space and ' ( ) - , and nothing else) and 71
// characters at scale 3 - pin the FALLBACK path, where the glyph table is
// smaller and every advance is fixed. They stay because that path is what
// draws if FontAtlasInit fails. These definitions are therefore still
// declared one per line and not assembled at runtime, so the selftest can
// parse them.
inline constexpr PickerStrings kEnemiesIncludedStrings = {
    "ENEMIES INCLUDED",
    nullptr,
    "YES", "NO",
    "ENABLE ALL", "DISABLE ALL",
    "SQUARE ALL   TRIANGLE NONE   O BACK",
};

inline constexpr PickerStrings kBossesIncludedStrings = {
    "BOSSES INCLUDED",
    nullptr,
    "YES", "NO",
    "ENABLE ALL", "DISABLE ALL",
    "SQUARE ALL   TRIANGLE NONE   O BACK",
};

// YES/NO under a heading reading ENEMIES SKIPPED is genuinely ambiguous -
// YES to being skipped, or YES to being included? - so this list says
// SKIPPED, and the instruction line says what the whole screen does.
inline constexpr PickerStrings kEnemiesSkippedStrings = {
    "ENEMIES SKIPPED",
    "SELECT ENEMIES THAT WILL NOT BE RANDOMIZED",
    "SKIPPED", "-",
    "SKIP ALL", "SKIP NONE",
    "SQUARE SKIP ALL   TRIANGLE SKIP NONE   O BACK",
};

class ModelPicker {
public:
    // Call when entering the mode, so the cursor starts at the top rather than
    // wherever it was left last time.
    void Reset();

    // Returns true once the user is finished and the host should leave the
    // mode. `enabled` is the selection's flag array, edited in place.
    bool Update(const ButtonEdges& input, const PickerStrings& strings,
                const ModelPoolEntry* table, int count, bool* enabled);

    void Draw(Renderer& renderer, const PickerStrings& strings,
              const ModelPoolEntry* table, int count, const bool* enabled);

private:
    // Select-all / select-none wipe a hand-built list with one press and there
    // is no undo, so both confirm first. A third mode rather than a separate
    // screen, for the same reason the picker itself is not one.
    enum class Pending { None, EnableAll, DisableAll };

    void MovePage(int direction, int count, const PickerStrings& strings);
    void DrawConfirm(Renderer& renderer, const PickerStrings& strings, int count,
                     int enabledCount);

    int     cursor_ = 0;
    int     scroll_ = 0;
    Pending pending_ = Pending::None;
};

} // namespace bbr
