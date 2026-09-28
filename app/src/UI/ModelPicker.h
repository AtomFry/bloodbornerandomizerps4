// ModelPicker.h - the shared drill-in list. Drives the enemy picker (82 rows),
// the boss picker (17), ENEMIES SKIPPED (85), START WITH A TRICK WEAPON (78)
// and START WITH A LEFT WEAPON (14), which behave identically; the host
// supplies the table, the count and the flags.
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
    // The second footer line's two VERBS. Not a whole line any more: it is a
    // prompt row now, so the glyphs are fixed - SQUARE for all, TRIANGLE for
    // none, CIRCLE for back - and only the words differ between pickers. BACK
    // is not here because it is the same word on all three.
    const char* allVerb;      // what SQUARE does - "ALL", "SKIP ALL"
    const char* noneVerb;     // what TRIANGLE does - "NONE", "SKIP NONE"
    // Whether the row draws the table's `model` field before the name. The
    // three creature lists need it: "C1130" beside OEDON CHAPEL DWELLER is how
    // a reader tells two rows with the same community name apart, and two of
    // them genuinely share one. The trick-weapon list does not - all 78 names
    // are distinct, and the id there is a nine-digit param id that means
    // nothing to a player.
    //
    // THIS STRUCT IS AGGREGATE-INITIALISED, so a site that forgets this field
    // gets `false` silently and drops its id column. Every site sets it
    // explicitly, and pool_verify.py counts them.
    bool showRowId;
};

// The three lists' vocabulary, defined once because both hosting screens draw
// the same picker and a second copy is a second thing to forget to change.
//
// The atlas draws printable ASCII (32..126, plus the mark at 127) and is
// proportional, so what a
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
    "ALL", "NONE",
    true,
};

inline constexpr PickerStrings kBossesIncludedStrings = {
    "BOSSES INCLUDED",
    nullptr,
    "YES", "NO",
    "ENABLE ALL", "DISABLE ALL",
    "ALL", "NONE",
    true,
};

// YES/NO under a heading reading ENEMIES SKIPPED is genuinely ambiguous -
// YES to being skipped, or YES to being included? - so this list says
// SKIPPED, and the instruction line says what the whole screen does.
inline constexpr PickerStrings kEnemiesSkippedStrings = {
    "ENEMIES SKIPPED",
    "SELECT ENEMIES THAT WILL NOT BE RANDOMIZED",
    "SKIPPED", "-",
    "SKIP ALL", "SKIP NONE",
    "SKIP ALL", "SKIP NONE",
    true,
};

// The fourth list, and the only one that is not creatures: the 78 right-hand
// trick weapon versions a new character can be granted (feature 037). YES/NO is
// unambiguous here - YES means "this weapon may be the one you start with" -
// and the instruction line carries the none/one/many rule, which is the part no
// heading can say. The instruction line also puts the list on the 11-row
// layout, so 78 rows is 8 pages.
inline constexpr PickerStrings kTrickWeaponsStrings = {
    "START WITH A TRICK WEAPON",
    "TICK ANY NUMBER - ONE IS DRAWN FOR THE RUN",
    "YES", "NO",
    "ENABLE ALL", "DISABLE ALL",
    "ALL", "NONE",
    false,
};

// The fifth list: the 14 left-hand weapons a new character can be granted
// (feature 038) - 11 firearms plus the Loch Shield, the Hunter's Torch and the
// Fist of Gratia. Word for word the strings above with its own heading, because
// it is the same setting for the other hand and reading differently would
// suggest it behaves differently. 14 rows on the 11-row instruction layout is
// 2 pages.
inline constexpr PickerStrings kLeftHandWeaponsStrings = {
    "START WITH A LEFT WEAPON",
    "TICK ANY NUMBER - ONE IS DRAWN FOR THE RUN",
    "YES", "NO",
    "ENABLE ALL", "DISABLE ALL",
    "ALL", "NONE",
    false,
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
