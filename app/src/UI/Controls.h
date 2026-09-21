// Controls.h - the reusable drawing/input pieces every screen was
// hand-rolling its own copy of (MenuScreen, OptionsScreen, StatusScreen all
// had near-identical "centered label", "list with a highlighted selection",
// and "move selection on up/down" code). This is that logic, written once.
//
// Deliberately small: a label, a list, and a navigation helper. Not a full
// widget/layout system - grow it only when a real screen needs something
// these three don't cover (see the roadmap's UI-3 note).
#pragma once

#include "../Platform/Input.h"
#include "../Platform/Renderer.h"

#include <string>
#include <vector>

namespace bbr {

struct Color { unsigned char r, g, b; };

// The palette every screen was independently repeating as raw RGB triples.
namespace Palette {
    constexpr Color Heading  = { 130, 185, 255 };
    constexpr Color Text     = { 214, 220, 228 };
    constexpr Color Selected = { 225, 175, 70  };
    constexpr Color Good     = { 90,  200, 140 };
    constexpr Color Bad      = { 235, 95,  115 };
    constexpr Color Dim      = { 130, 140, 152 };
    // The bar drawn BEHIND the focused row of a rail or a pane, with the row's
    // own text in Selected on top of it. A dark tint of Selected rather than a
    // new hue, so the highlight reads as the same colour the text does. Both
    // categorised screens draw it, which is why it is here and not in one of
    // them; the exact value is a TV judgement and may want adjusting after the
    // hardware test.
    constexpr Color SelectedBar = { 56, 44, 18 };
}

// How hard an overlay dims whatever is behind it - black at this alpha of 255.
// A pixel behind an overlay survives at (255 - alpha) / 255 of its brightness.
//
// There are two of these because the two overlays want opposite things.
//
// kScrimAlpha covers a whole screen with a different screen - a picker over the
// settings screen. What is behind is context, not information: the parent must
// read as a dim shape, and its TEXT must not compete with the picker's. At 235
// the parent keeps 7.8% of its brightness, which puts its body text near
// (17,17,18) against a (2,2,2) ground - present, but far too dark to read as
// words. It was 190 (25.5% surviving, text at (55,56,58)) and that was legible
// enough to make the picker hard to read.
constexpr unsigned char kScrimAlpha = 235;

// kPromptAlpha dims a list so a prompt ABOUT that list can sit over it. Here
// what is behind is the information the prompt is asking about - "SKIP ALL"
// means nothing without the rows it would skip - so this one stays light
// enough to read. Do not unify these two: the prompt would go black.
constexpr unsigned char kPromptAlpha = 190;

// One line of text, horizontally centered on screen.
void DrawCenteredLabel(Renderer& renderer, int y, const char* text, int scale, Color color);

// A vertical list of items, one per line, all centered; `selected` is drawn
// in `selectedColor`, everything else in `textColor`.
void DrawMenuList(Renderer& renderer, int firstY, int spacing,
                   const std::vector<std::string>& items, int selected, int scale,
                   Color textColor, Color selectedColor);

// Moves `selected` up/down by one on input.up/input.down, wrapping at the
// ends. Returns `selected` unchanged if neither fired this frame.
int NavigateVertical(int selected, int itemCount, const ButtonEdges& input);

// ---------------------------------------------------------------------------
// Scrolling lists
//
// DrawMenuList above draws every item unconditionally, which is fine while a
// list is short enough to fit. Once it isn't, rows land on top of the footer
// or off the bottom of the screen entirely - and because NavigateVertical
// still walks through them, you end up toggling a setting you cannot see.
// These pieces put a window over the list instead.
//
// The math is deliberately separate from the drawing: the progress log needs
// the same scrolling but colors its lines individually and appends a live
// status line, so it shares the window calculation and draws itself.
// ---------------------------------------------------------------------------

struct ListLayout {
    int firstY;       // y of the topmost visible row
    int spacing;      // px between rows
    int bottomLimit;  // no row may be drawn at or below this y
    // Gap between the list and its "MORE ..." hint. Per-layout rather than one
    // file-scope constant: the feasible gap depends on the item scale and on
    // the furniture above and below, and ui_scroll_verify.py's corrected
    // ink-box model shows no single value clears every screen.
    int hintGap;
};

// How many rows of `layout` actually fit. Never less than 1, so a caller can
// divide by it safely even if someone hands it a nonsensical band.
int VisibleRowCount(const ListLayout& layout);

// Clamps `offset` to a window that exists: never negative, never scrolled
// past the point where the last item sits on the bottom row.
int ClampScroll(int offset, int itemCount, int visible);

// The smallest change to `offset` that brings `selected` back into view, so
// the list only moves when the selection would otherwise leave the window.
int ScrollToShow(int offset, int selected, int itemCount, int visible);

// "MORE ABOVE" / "MORE BELOW", drawn only on the side that has more. The
// 8x8 font is uppercase and digits only - it has no arrow glyphs, and an
// unknown character renders blank - so these have to be words.
void DrawScrollHints(Renderer& renderer, const ListLayout& layout, int itemCount,
                      int offset, int visible);

// DrawMenuList's scrolling equivalent: draws the `visible` items starting at
// `offset`, highlights `selected` (an index into the FULL list, not the
// window), and adds the hints. Pass selected = -1 for a list with no cursor.
void DrawScrollableList(Renderer& renderer, const ListLayout& layout,
                         const std::vector<std::string>& items, int selected,
                         int offset, int scale, Color textColor, Color selectedColor);

// ---------------------------------------------------------------------------
// Panes
//
// Everything above centres on the whole screen, which is right for a screen
// that IS one list and wrong for the categorised screens, where three columns
// each own a band of x. These take an explicit x - and, where they centre, an
// explicit width - so nothing here has to know how wide the screen is.
// ---------------------------------------------------------------------------

// The settings pane's scroll window, shared by both categorised screens: 8
// rows of 76px from y 330, the last at 862. Asserted by ui_scroll_verify.py
// and settings_ui_verify.py. No category holds more than 4 settings today and
// none holds more than 7 with the whole backlog, so this never scrolls yet -
// it is windowed anyway, because a pane that silently truncates is the failure
// mode being removed.
inline constexpr ListLayout kPaneLayout = { 330, 76, 880, 52 };

// One line of text with its left edge at x.
void DrawLabelLeft(Renderer& renderer, int x, int y, const char* text, int scale,
                    Color color);

// One line of text with its RIGHT edge at rightX - the settings pane's value
// column, so YES/NO and "82 OF 82" line up against the same edge whatever they
// measure.
void DrawLabelRight(Renderer& renderer, int rightX, int y, const char* text, int scale,
                     Color color);

// DrawScrollHints for a pane: the same two words, centred in the band
// [x, x + width) instead of on the screen.
void DrawPaneScrollHints(Renderer& renderer, const ListLayout& layout, int x, int width,
                          int itemCount, int offset, int visible);

// Greedy word wrap at `maxWidth`, measured with Renderer::TextWidth - never
// from character counts, which stopped predicting width when the proportional
// atlas replaced the fixed 8x8 cell. Breaks on spaces only; a single word
// wider than maxWidth gets its own line and overhangs, which
// settings_ui_verify.py checks cannot happen to any string the app draws.
std::vector<std::string> WrapText(Renderer& renderer, const char* text, int scale,
                                   int maxWidth);

} // namespace bbr
