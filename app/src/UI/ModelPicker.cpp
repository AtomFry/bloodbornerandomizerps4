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
const ListLayout kPickerLayout = { 280, 52, 900 };

// The same list moved down to clear an instruction line, which costs one
// visible row: (900 - 332) / 52 + 1 = 11. The heading and the count line are
// deliberately NOT moved - keeping their constants shared and untouched is
// what guarantees the two shipped pickers still render pixel for pixel.
// MORE ABOVE then sits at 332 - 46 = 286, comfortably below the instruction
// line's 259 bottom. Both bands are asserted by ui_scroll_verify.py.
const ListLayout kPickerLayoutWithInstruction = { 332, 52, 900 };

const int kInstructionY = 235;

// An instruction line is what distinguishes the two layouts, so the choice
// lives in one function rather than at each of the five use sites.
const ListLayout& LayoutFor(const PickerStrings& strings) {
    return strings.instruction ? kPickerLayoutWithInstruction : kPickerLayout;
}

std::string PadTo(const std::string& s, int width) {
    std::string out = s;
    while ((int)out.size() < width) out += ' ';
    return out;
}

std::string RowLabel(const ModelPoolEntry& m) {
    std::string label = std::string(m.model) + " " + m.displayName;
    // Uppercase the model id so it matches the name beside it; the table
    // stores it lowercase because that is how the game data spells it.
    for (size_t k = 0; k < 5 && k < label.size(); k++) {
        if (label[k] >= 'a' && label[k] <= 'z') label[k] = (char)(label[k] - 'a' + 'A');
    }
    return label;
}

// Measured from the table rather than baked as a constant: the enemy list's
// longest label is 39 characters and the boss list's is 41, and a shared
// constant would silently clip the day a regenerated table grows past it.
// Every row is then padded to the same width so all rows render the same
// pixel width - the lists are drawn centred, so equal widths are what puts
// the YES/NO into a fixed column without a left-aligned draw path.
int NameFieldWidth(const ModelPoolEntry* table, int count) {
    int w = 0;
    for (int i = 0; i < count; i++) {
        int len = (int)RowLabel(table[i]).size();
        if (len > w) w = len;
    }
    return w;
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

void ModelPicker::Draw(Renderer& renderer, const PickerStrings& strings,
                       const ModelPoolEntry* table, int count, const bool* enabled) {
    renderer.Clear(20, 24, 28);

    int on = 0;
    for (int i = 0; i < count; i++) if (enabled[i]) on++;

    if (pending_ != Pending::None) {
        DrawConfirm(renderer, strings, count, on);
        return;
    }

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

    int width = NameFieldWidth(table, count);
    // The flag column is as wide as the longer of the two words, so the rows
    // stay a fixed width whichever vocabulary the host passed.
    int flagWidth = (int)strlen(strings.flagOn);
    if ((int)strlen(strings.flagOff) > flagWidth) flagWidth = (int)strlen(strings.flagOff);
    std::vector<std::string> items;
    items.reserve((size_t)count);
    for (int i = 0; i < count; i++) {
        items.push_back(PadTo(RowLabel(table[i]), width) + "   " +
                        PadTo(enabled[i] ? strings.flagOn : strings.flagOff, flagWidth));
    }

    DrawScrollableList(renderer, layout, items, cursor_, scroll_, kItemScale,
                       Palette::Text, Palette::Selected);

    DrawCenteredLabel(renderer, kScreenHeight - 130,
                      "UP DOWN MOVE   L1 R1 PAGE   X TOGGLE", kFooterScale, Palette::Dim);
    DrawCenteredLabel(renderer, kScreenHeight - 80, strings.footer, kFooterScale,
                      Palette::Dim);
}

// A full-screen prompt rather than a panel over the list: Renderer has no
// filled-rectangle call (Clear, DrawText, TextWidth is the whole API), and
// drawing over the list without one would leave rows showing through the text.
// Adding FillRect to the Platform layer for one modal is not worth it - and a
// question that takes over the screen is harder to answer without reading.
void ModelPicker::DrawConfirm(Renderer& renderer, const PickerStrings& strings, int count,
                              int enabledCount) {
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
