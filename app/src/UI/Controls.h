// Controls.h - the reusable drawing/input pieces every screen was
// hand-rolling its own copy of: near-identical "centered label", "list with
// a highlighted selection" and "move selection on up/down" code, repeated
// across the menu and status screens the worlds feature has since retired.
// This is that logic, written once.
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
//
// U4 PASS 1 - THE WHOLE PALETTE TURNED WARM. It was built cool: a blue-grey
// ground, a cyan-blue heading, blue-grey dim text. The reference the developer
// is aiming at - the game's own inventory screen - has NO BLUE IN IT ANYWHERE.
// Its ground is a warm near-black, its headings are gold, its body text is
// parchment, and what it dims, it dims toward brown rather than toward slate.
//
// This is the cheapest large change available to this app: six constants, and
// every screen restyles at once, because nothing outside this namespace names a
// colour any more.
//
// TWO THINGS DELIBERATELY KEPT. `Selected`'s gold was already right - it is the
// colour the reference uses for the text of a highlighted row - and `Mark`'s red
// was already lifted off the game. Both are unchanged, and the rest of the
// palette moved TOWARD them rather than the other way round.
//
// COLOUR LIES ON A MONITOR. None of these values is verifiable off-console: the
// verifiers check geometry and fit, and a television at three metres is the only
// place a palette is actually judged. Expect to adjust after the hardware test,
// and treat the numbers as a starting point rather than as a decision.
namespace Palette {
    // The ground every screen clears to, and the reason `Clear(20, 24, 28)` no
    // longer appears in any screen: it was written out thirteen times, so a
    // palette change had to find all thirteen. DrawGround() is the one caller.
    constexpr Color Ground   = { 18, 16, 15 };

    // Gold, not blue. This one constant is most of the change.
    constexpr Color Heading  = { 198, 160, 86  };
    // Parchment rather than cool white.
    constexpr Color Text     = { 214, 202, 178 };
    constexpr Color Selected = { 225, 175, 70  };
    // Both warmed toward the ground so they sit in the same world as the rest.
    // Good keeps enough green to read as "this succeeded" and no more.
    constexpr Color Good     = { 140, 178, 120 };
    constexpr Color Bad      = { 200, 88,  76  };
    // Dimmed toward brown, not toward slate. Labels are drawn in this on both
    // Confirm tiers, so it carries more text than its name suggests.
    constexpr Color Dim      = { 138, 124, 104 };
    // The bar drawn BEHIND the focused row of a rail or a pane, with the row's
    // own text in Selected on top of it. A dark tint of Selected rather than a
    // new hue, so the highlight reads as the same colour the text does. Both
    // categorised screens draw it, which is why it is here and not in one of
    // them; the exact value is a TV judgement and may want adjusting after the
    // hardware test.
    constexpr Color SelectedBar = { 56, 44, 18 };
    // U4 PASS 2 - the band is three tones, not one flat fill. The reference
    // lights its selected row from the top and falls away down the row, which is
    // what stops a highlight reading as a coloured rectangle pasted behind the
    // text. `Lit` is the upper half, `SelectedBar` the lower; `Edge` is the
    // bright rule closing it top and bottom, and is the part that actually makes
    // it look drawn rather than filled.
    constexpr Color SelectedBarLit = { 78, 62, 26 };
    constexpr Color SelectedEdge   = { 122, 96, 40 };
    // The active-world marker. Deliberately NOT Good: that green is this
    // app's confirmation colour - "the operation succeeded" - and on a screen
    // whose every other hue is gold, parchment and steel, a saturated green
    // block read as a status LED bolted onto the menu. This is the game's own
    // blood red, lifted off the painted original enough to hold at scale 3 on
    // a TV.
    constexpr Color Mark = { 210, 76, 66 };

    // Furniture: rules, panel frames and separators. Was { 64, 72, 82 } - a
    // blue-grey - and was the fourth copy of a constant three screens each
    // declared for themselves. Dimmer than any text, because a rule that reads
    // as loud as a label is furniture competing with content.
    constexpr Color Rule     = { 64, 58, 48 };
    // The inner line of a double frame, and the quietest thing on screen.
    constexpr Color RuleSoft = { 42, 38, 32 };

    // U4 PASS 4 - the four face buttons, in the console's own colours. These
    // are the one place blue is allowed back onto a screen whose whole palette
    // was turned warm, and the reason is that they are not decoration: the
    // colour IS the identifier, on the pad in the player's hands and in the
    // game's own menus. Muted toward the ground so they sit in this palette
    // rather than glowing out of it.
    constexpr Color BtnCross    = { 108, 146, 206 };   // blue
    constexpr Color BtnCircle   = { 206, 86,  80  };   // red
    constexpr Color BtnTriangle = { 112, 184, 144 };   // green
    constexpr Color BtnSquare   = { 198, 120, 178 };   // pink
    // The d-pad is not colour-coded on the pad, so it takes the label's own
    // colour rather than inventing a fifth.
    constexpr Color BtnDpad     = { 150, 140, 124 };
}

// The Hunter's Mark, drawn as TEXT. app/tools/mark_glyph.py bakes it into the
// font atlas at codepoint 127, so it blits, tints and scales through exactly
// the same path as every letter: DrawLabel* place it, TextWidth measures it,
// and Palette colours it. Font8x8.cpp carries a crude 8x8 version for the
// fallback path, so it is never the one thing that silently fails to draw.
//
// Keep it alone in its own literal. Concatenated with a string starting in a
// hex digit, the  escape would swallow those digits and produce a
// different character - the standard's rule, and a silent one.
constexpr const char* kActiveMark = "";

// --- U4 pass 4: the button glyphs ------------------------------------------
//
// Codepoints 128..132, baked by app/tools/button_glyphs.py. Written as HEX
// ESCAPES rather than as raw bytes: 0x80 and up are not valid UTF-8 on their
// own, and a source file carrying them raw is at the mercy of how a compiler
// decides to read it. The mark at 0x7f is plain ASCII and has no such problem.
//
// The same warning the mark carries applies with more force here. Keep each
// alone in its own literal - concatenated with a string starting in a hex
// digit, the escape swallows those digits and silently produces a different
// character. "\x80" "ABC" is fine; "\x80ABC" is the glyph at 0x80ABC.
constexpr const char* kBtnCross    = "\x80";
constexpr const char* kBtnCircle   = "\x81";
constexpr const char* kBtnTriangle = "\x82";
constexpr const char* kBtnSquare   = "\x83";
constexpr const char* kBtnDpad     = "\x84";

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

// ---------------------------------------------------------------------------
// The main screen's tab strip
//
// Two text tabs across the top, the active one boxed, with the active tab's
// name as a heading beneath (worlds B1). Both tabbed screens draw it from
// here rather than each carrying its own copy: the two would otherwise be one
// unnoticed edit away from disagreeing about their own names, their order, or
// where the frame sits - and a tab strip that moves between tabs reads as the
// screen jumping rather than as the content changing.
//
// There is no box around the INACTIVE tab. One box means "you are here"; two
// boxes with different fills means the player has to know which fill wins.
// ---------------------------------------------------------------------------

constexpr int kTabCount    = 2;
constexpr int kTabWorlds   = 0;
constexpr int kTabDefaults = 1;

// Geometry. Measured by settings_ui_verify.py against the atlas's own
// advances, like every other number on these screens - none of them is a
// character count.
constexpr int kTabScale        = 4;
constexpr int kTabX            = 60;   // the same left edge as the rail below
constexpr int kTabY            = 40;
constexpr int kTabPadX         = 24;   // clear space inside the box, each side
constexpr int kTabGap          = 20;   // between one box and the next
constexpr int kTabBoxOffsetY   = -10;  // the box, relative to the label's draw y
constexpr int kTabBoxHeight    = 78;
constexpr int kTabBorder       = 3;
constexpr int kTabHeadingY     = 118;  // the active tab's name, beneath the strip
constexpr int kTabHeadingScale = 5;

// "WORLDS" / "DEFAULTS", by index. The strings live here so one verifier case
// covers both screens' tab labels.
const char* TabLabel(int index);

// The strip, with `active` boxed. Does not draw the heading - a screen draws
// its own, because Setup Defaults' heading is its tab's name and the worlds
// screen's carries a readout beside it.
void DrawTabs(Renderer& renderer, int active);

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

// --- U4 pass 1: the ground, and the frames drawn on it ---------------------

// Clears the screen to Palette::Ground. EVERY screen starts with this and none
// of them clears for itself any more - thirteen hand-written Clear(20, 24, 28)
// calls were thirteen places a palette change had to find.
//
// It is a function rather than a constant because pass 3 puts the vignette here,
// where it will apply to every screen at once without any of them being touched
// again.
void DrawGround(Renderer& renderer);

// A panel frame: a rectangle drawn as TWO lines - Palette::Rule outside,
// Palette::RuleSoft one pixel in - with short ticks stepping in at each corner.
//
// This is the detail that most says "FromSoftware menu" in the reference, and it
// is entirely FillRect work, so nothing here is a new SDL entry point. It draws
// an OUTLINE and never fills, so it can be laid over content that is already
// drawn, and it takes a rect rather than reading any screen's constants -
// panels are defined by the columns they enclose, which every screen already
// knows the geometry of.
void DrawPanelFrame(Renderer& renderer, int x, int y, int w, int h);

// --- U4 pass 2: the selected row, and the lines between rows ---------------

// The band drawn BEHIND the focused row of any list, with the row's own text in
// Palette::Selected on top of it. Replaces the flat FillRect that five call
// sites across three screens each wrote out for themselves.
//
// Takes the row's whole box - the same (x, y + kBarOffsetY, w, kBarHeight) those
// five passed - so no caller's geometry changes and nothing here has to know
// what a row is.
void DrawSelectionBand(Renderer& renderer, int x, int y, int w, int h);

// A hairline between two rows of a list, in Palette::RuleSoft. ONE pixel, not
// the two every other rule in this app uses: a separator as heavy as a rule
// competes with the frame around the column it sits in, and the reference's are
// barely there.
//
// Draw it between consecutive rows and never after the last one - a line under
// the final row reads as the bottom of a box that has no top.
void DrawRowSeparator(Renderer& renderer, int x, int y, int w);

// One "<button> WHAT IT DOES" pair of a footer.
//
// `glyph` is one of the kBtn* constants, or nullptr for a prompt that has no
// button - a bare note sharing the row. `label` is what pressing it does.
struct ButtonPrompt {
    const char* glyph;
    const char* label;
};

// Draws a row of prompts, centred on the screen at `y`.
//
// The glyph goes down in its OWN colour and the label in Palette::Dim, which
// is why this exists at all: one DrawText call carries one colour, so a footer
// built as a single string can only ever be monochrome. Every footer in the
// app used to be exactly that - "X SELECT   O BACK" as plain text, with the
// letters X and O standing in for glyphs they do not look like.
void DrawPromptRow(Renderer& renderer, int y, const ButtonPrompt* prompts, int count,
                   int scale);

// The colour a given kBtn* glyph is drawn in. Exposed because a screen that
// draws a prompt outside a row - the seed editor's SQUARE RANDOM, say - should
// not have to repeat the mapping.
Color ButtonColor(const char* glyph);

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
