#include "ModelPicker.h"

#include "../Platform/Log.h"
#include "../Platform/Renderer.h"

#include <cstring>
#include <string>
#include <vector>

namespace bbr {

namespace {
const int kTitleScale  = 5;
const int kCountScale  = 3;
const int kItemScale   = 3;
const int kFooterScale = 3;

// 12 visible rows, so seven pages of the 82-row enemy list and two of the
// 17-row boss list. Checked by ui_scroll_verify.py: every row and both
// MORE ABOVE / MORE BELOW hints clear the count line above and the footer.
const ListLayout kPickerLayout = { 280, 52, 900, 52 };

// The same list moved down to clear an instruction line, which costs one
// visible row: (900 - 332) / 52 + 1 = 11. The heading and the count line are
// deliberately NOT moved - keeping their constants shared and untouched is
// what guarantees the two shipped pickers still render pixel for pixel.
// MORE ABOVE then sits at 332 - 52 = 280, whose ink starts 10px below the
// instruction line's ink. Both bands are asserted by ui_scroll_verify.py.
const ListLayout kPickerLayoutWithInstruction = { 332, 52, 900, 52 };

const int kInstructionY = 235;

// An instruction line is what distinguishes the two layouts, so the choice
// lives in one function rather than at each of the five use sites.
const ListLayout& LayoutFor(const PickerStrings& strings) {
    return strings.instruction ? kPickerLayoutWithInstruction : kPickerLayout;
}

// The band the rows are aligned within: label left at kRowX, flag right at
// kRowFlagRight. 1080 px wide and centred on the 1920 surface, so the block
// still sits where the centred list used to.
//
// Sized from the widest row any of the three tables can produce - the boss
// list's "C4520 LADY MARIA OF THE ASTRAL CLOCKTOWER" at 791 px - plus the
// widest flag word, "SKIPPED" at 127 px, leaving 162 px between them.
// tools/settings_ui_verify.py case 9 asserts the band against the real tables,
// so a regenerated table that outgrows it fails there, not silently here.
const int kRowX         = 420;
const int kRowFlagRight = 1500;
const int kRowW         = kRowFlagRight - kRowX;

// --- U4 pass 2: the picker's band and its frame ----------------------------
//
// THE ROW BOX IS THIS PICKER'S OWN, and not the -10/64 the three categorised
// screens share. Those rows sit on a 76px pitch; these sit on 52, so a 64px
// band would run into its neighbour. At +2/48 the band holds the row's whole
// ink box - scale 3 puts ink 9..44 below the draw y - with 7px above it, 6
// below, and 4px of ground between one band and the next.
const int kPickerBarOffsetY = 2;
const int kPickerBarHeight  = 48;

// THIS PICKER GETS NO PANEL FRAME, and the reason is arithmetic rather than
// taste. It was tried, and the numbers refuse it.
//
// A frame that encloses the list has to enclose its scroll hints too - a hint
// is part of the list it is about - so its top edge must sit between the count
// line's ink, which ends at 229, and MORE ABOVE's, which starts at 237. That is
// an EIGHT PIXEL window for a 2px line that needs clearance on both sides, and
// a first attempt at 236 drew the frame straight through the hint.
//
// Dropping the hints outside the frame does not rescue it: that puts the top
// edge in the 10px between MORE ABOVE's ink and the first row's band, and the
// two layouts disagree about where that is - kPickerLayoutWithInstruction
// starts 52px lower and its own hint ends at 324.
//
// The vertical budget here is simply spoken for: heading, count line, optional
// instruction, two hints, eleven or twelve rows and two footer lines in 1080px.
// Buying room means moving constants that are deliberately pinned so the two
// shipped pickers keep rendering pixel for pixel, which is a layout change and
// not a styling pass. The rows get the band and the separators; the panel
// belongs to a later item that is allowed to move the furniture.

std::string RowLabel(const ModelPoolEntry& m, bool showRowId) {
    // Without the id column the label IS the display name - no leading space
    // and no uppercasing pass, because there is nothing there to uppercase.
    // The trick-weapon list is the one that asks for this: its ids are
    // nine-digit param ids and all 78 of its names are distinct, so the column
    // would cost width and tell the player nothing (feature 037).
    if (!showRowId) return std::string(m.displayName);

    std::string label = std::string(m.model) + " " + m.displayName;
    // Uppercase the model id so it matches the name beside it; the table
    // stores it lowercase because that is how the game data spells it.
    for (size_t k = 0; k < 5 && k < label.size(); k++) {
        if (label[k] >= 'a' && label[k] <= 'z') label[k] = (char)(label[k] - 'a' + 'A');
    }
    return label;
}

} // namespace

void ModelPicker::Reset() {
    cursor_ = 0;
    scroll_ = 0;
    pending_ = Pending::None;
}

void ModelPicker::MovePage(int direction, int count, const PickerStrings& strings) {
    cursor_ += direction * VisibleRowCount(LayoutFor(strings));
    if (cursor_ < 0) cursor_ = 0;
    if (cursor_ > count - 1) cursor_ = count - 1;
}

bool ModelPicker::Update(const ButtonEdges& input, const PickerStrings& strings,
                         const ModelPoolEntry* table, int count, bool* enabled) {
    if (pending_ != Pending::None) {
        if (input.cross) {
            bool on = (pending_ == Pending::EnableAll);
            for (int i = 0; i < count; i++) enabled[i] = on;
            Log(on ? "model picker: all enabled" : "model picker: all disabled");
            pending_ = Pending::None;
        } else if (input.circle) {
            Log("model picker: select-all/none cancelled");
            pending_ = Pending::None;
        }
        return false; // O here cancels the prompt, it does not leave the picker
    }

    cursor_ = NavigateVertical(cursor_, count, input);

    // L1/R1 are the intended paging buttons; d-pad left/right does the same
    // thing as a fallback, because the d-pad arrives through a different code
    // path (SDL_JOYHATMOTION) that is already proven on hardware.
    if (input.r1 || input.right) MovePage(1, count, strings);
    if (input.l1 || input.left)  MovePage(-1, count, strings);

    scroll_ = ScrollToShow(scroll_, cursor_, count, VisibleRowCount(LayoutFor(strings)));

    if (input.cross && cursor_ >= 0 && cursor_ < count) {
        enabled[cursor_] = !enabled[cursor_];
        Log((std::string("model picker: ") + table[cursor_].model +
             (enabled[cursor_] ? " = YES" : " = NO")).c_str());
    }

    if (input.square)   pending_ = Pending::EnableAll;
    if (input.triangle) pending_ = Pending::DisableAll;

    if (input.circle) {
        int on = 0;
        for (int i = 0; i < count; i++) if (enabled[i]) on++;
        Log((std::string("model picker: closed with ") + std::to_string(on) + " of " +
             std::to_string(count) + " enabled").c_str());
        return true;
    }
    return false;
}

// Does NOT clear: the host draws the background before calling this, so the
// picker can be drawn over a dimmed parent screen instead of replacing it.
// Both remaining call sites therefore start with their own Clear.
void ModelPicker::Draw(Renderer& renderer, const PickerStrings& strings,
                       const ModelPoolEntry* table, int count, const bool* enabled) {
    int on = 0;
    for (int i = 0; i < count; i++) if (enabled[i]) on++;

    DrawCenteredLabel(renderer, 120, strings.heading, kTitleScale, Palette::Heading);

    const ListLayout& layout = LayoutFor(strings);
    int visible = VisibleRowCount(layout);
    int page    = cursor_ / visible + 1;
    int pages   = (count + visible - 1) / visible;
    std::string countText = std::to_string(on) + " OF " + std::to_string(count) +
                            "     PAGE " + std::to_string(page) + " OF " +
                            std::to_string(pages);
    DrawCenteredLabel(renderer, 185, countText.c_str(), kCountScale, Palette::Text);

    // Below the count line rather than above it, so the reader has the list's
    // name, then its size, then what ticking a row actually does - and so the
    // heading constants stay shared with the two pickers that have no
    // instruction line.
    if (strings.instruction) {
        DrawCenteredLabel(renderer, kInstructionY, strings.instruction, kCountScale,
                          Palette::Text);
    }

    // Drawn a row at a time, label left and flag right, rather than assembled
    // into one padded string and centred. Space padding only lines columns up
    // in a monospace font; the atlas font is proportional, so the old approach
    // put the flags in a ragged column that moved with each name's width.
    int offset = ClampScroll(scroll_, count, visible);

    for (int row = 0; row < visible; row++) {
        int index = offset + row;
        if (index >= count) break;

        int   y     = layout.firstY + row * layout.spacing;
        bool  focused = (index == cursor_);
        Color color = focused ? Palette::Selected : Palette::Text;

        // U4 pass 2. This list had NO band at all - the cursor was a colour
        // change and nothing else, which on an 82-row list at scale 3 is the
        // hardest cursor in the app to find. It gets the same band every other
        // list has, in this picker's own row box.
        bool lastDrawn = (row == visible - 1) || (index + 1 >= count);
        if (!lastDrawn) {
            DrawRowSeparator(renderer, kRowX, y + kPickerBarOffsetY + kPickerBarHeight,
                             kRowW);
        }
        if (focused) {
            DrawSelectionBand(renderer, kRowX, y + kPickerBarOffsetY, kRowW,
                              kPickerBarHeight);
        }

        DrawLabelLeft(renderer, kRowX, y,
                      RowLabel(table[index], strings.showRowId).c_str(),
                      kItemScale, color);
        DrawLabelRight(renderer, kRowFlagRight, y,
                       enabled[index] ? strings.flagOn : strings.flagOff, kItemScale, color);
    }

    DrawPaneScrollHints(renderer, layout, kRowX, kRowW, count, offset, visible);

    // U4 pass 4. L1/R1 are labelled shoulder buttons rather than symbols, so
    // they stay as a plain prompt - inventing a glyph for a button that has
    // its name printed on it would be worse than the words.
    const ButtonPrompt moveRow[] = {
        { kBtnDpad,  "MOVE" },
        { nullptr,   "L1 R1 PAGE" },
        { kBtnCross, "TOGGLE" },
    };
    DrawPromptRow(renderer, kScreenHeight - 130, moveRow, 3, kFooterScale);

    // The second line IS a prompt row - three buttons, and only the two verbs
    // differ between the three pickers. It was one flat string reading
    // "SQUARE ALL   TRIANGLE NONE   O BACK", which named the buttons in words
    // beside a footer that had already stopped doing that everywhere else.
    const ButtonPrompt verbRow[] = {
        { kBtnSquare,   strings.allVerb },
        { kBtnTriangle, strings.noneVerb },
        { kBtnCircle,   "BACK" },
    };
    DrawPromptRow(renderer, kScreenHeight - 80, verbRow, 3, kFooterScale);

    // The prompt goes last, over the list it is asking about, so the counts
    // it quotes can be read against the rows they came from.
    if (pending_ != Pending::None) DrawConfirm(renderer, strings, count, on);
}

// An overlay over the still-drawn list rather than a screen that replaces it:
// the question is about the list, so losing sight of it to answer was always
// the wrong trade. It was a full-screen prompt only because Renderer had no
// way to fill a rectangle. Now it does, and the dim is what keeps the prompt
// readable against the rows underneath.
//
// FillRectBlend is the one SDL entry point in this screenful that has never
// run on a PS4. If the list behind the prompt is not visibly dimmed, or is
// invisible, alpha blending is unavailable here - see the plan's 4.3.
void ModelPicker::DrawConfirm(Renderer& renderer, const PickerStrings& strings, int count,
                              int enabledCount) {
    renderer.FillRectBlend(0, 0, kScreenWidth, kScreenHeight, 0, 0, 0, kPromptAlpha);

    bool enabling = (pending_ == Pending::EnableAll);

    // The verbs carry their own "ALL"/"NONE", because a skip list's two
    // actions are not "enable all" and "disable all" and reusing that wording
    // under a heading reading ENEMIES SKIPPED is exactly the ambiguity the
    // instruction line exists to remove.
    std::string line1 = std::string(enabling ? strings.verbAll : strings.verbNone) + " " +
                        std::to_string(count);
    // Naming what is about to be lost makes this a real question rather than a
    // reflex "yes": the current count is the number the user spent time on.
    std::string line2 = std::string("THIS REPLACES YOUR CURRENT ") +
                        std::to_string(enabledCount) + " OF " + std::to_string(count);

    DrawCenteredLabel(renderer, 400, line1.c_str(), kTitleScale, Palette::Heading);
    DrawCenteredLabel(renderer, 520, line2.c_str(), kCountScale, Palette::Text);
    DrawCenteredLabel(renderer, 640, "X CONFIRM        O CANCEL", kCountScale, Palette::Selected);
}

} // namespace bbr
