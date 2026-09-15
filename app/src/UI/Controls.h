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
}

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

} // namespace bbr
