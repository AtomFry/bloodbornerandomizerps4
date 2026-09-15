#include "ModelPicker.h"

#include "../Platform/Log.h"
#include "../Platform/Renderer.h"

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

void ModelPicker::MovePage(int direction, int count) {
    cursor_ += direction * VisibleRowCount(kPickerLayout);
    if (cursor_ < 0) cursor_ = 0;
    if (cursor_ > count - 1) cursor_ = count - 1;
}

bool ModelPicker::Update(const ButtonEdges& input, const ModelPoolEntry* table, int count,
                         bool* enabled) {
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
    if (input.r1 || input.right) MovePage(1, count);
    if (input.l1 || input.left)  MovePage(-1, count);

    scroll_ = ScrollToShow(scroll_, cursor_, count, VisibleRowCount(kPickerLayout));

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

void ModelPicker::Draw(Renderer& renderer, const char* heading, const ModelPoolEntry* table,
                       int count, const bool* enabled) {
    renderer.Clear(20, 24, 28);

    int on = 0;
    for (int i = 0; i < count; i++) if (enabled[i]) on++;

    if (pending_ != Pending::None) {
        DrawConfirm(renderer, count, on);
        return;
    }

    DrawCenteredLabel(renderer, 120, heading, kTitleScale, Palette::Heading);

    int visible = VisibleRowCount(kPickerLayout);
    int page    = cursor_ / visible + 1;
    int pages   = (count + visible - 1) / visible;
    std::string countText = std::to_string(on) + " OF " + std::to_string(count) +
                            "     PAGE " + std::to_string(page) + " OF " +
                            std::to_string(pages);
    DrawCenteredLabel(renderer, 185, countText.c_str(), kCountScale, Palette::Text);

    int width = NameFieldWidth(table, count);
    std::vector<std::string> items;
    items.reserve((size_t)count);
    for (int i = 0; i < count; i++) {
        items.push_back(PadTo(RowLabel(table[i]), width) + "   " +
                        PadTo(enabled[i] ? "YES" : "NO", 3));
    }

    DrawScrollableList(renderer, kPickerLayout, items, cursor_, scroll_, kItemScale,
                       Palette::Text, Palette::Selected);

    DrawCenteredLabel(renderer, kScreenHeight - 130,
                      "UP DOWN MOVE   L1 R1 PAGE   X TOGGLE", kFooterScale, Palette::Dim);
    DrawCenteredLabel(renderer, kScreenHeight - 80,
                      "SQUARE ALL   TRIANGLE NONE   O BACK", kFooterScale, Palette::Dim);
}

// A full-screen prompt rather than a panel over the list: Renderer has no
// filled-rectangle call (Clear, DrawText, TextWidth is the whole API), and
// drawing over the list without one would leave rows showing through the text.
// Adding FillRect to the Platform layer for one modal is not worth it - and a
// question that takes over the screen is harder to answer without reading.
void ModelPicker::DrawConfirm(Renderer& renderer, int count, int enabledCount) {
    bool enabling = (pending_ == Pending::EnableAll);

    std::string line1 = std::string(enabling ? "ENABLE" : "DISABLE") + " ALL " +
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
