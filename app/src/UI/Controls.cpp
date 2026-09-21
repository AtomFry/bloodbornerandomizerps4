#include "Controls.h"

namespace bbr {

void DrawCenteredLabel(Renderer& renderer, int y, const char* text, int scale, Color color) {
    int x = (kScreenWidth - renderer.TextWidth(text, scale)) / 2;
    renderer.DrawText(x, y, text, scale, color.r, color.g, color.b);
}

void DrawMenuList(Renderer& renderer, int firstY, int spacing,
                   const std::vector<std::string>& items, int selected, int scale,
                   Color textColor, Color selectedColor) {
    for (size_t i = 0; i < items.size(); i++) {
        Color c = (static_cast<int>(i) == selected) ? selectedColor : textColor;
        DrawCenteredLabel(renderer, firstY + static_cast<int>(i) * spacing, items[i].c_str(), scale, c);
    }
}

int NavigateVertical(int selected, int itemCount, const ButtonEdges& input) {
    if (input.up)   return (selected + itemCount - 1) % itemCount;
    if (input.down) return (selected + 1) % itemCount;
    return selected;
}

namespace {
// The hint's own scale. The gap that used to live beside it is now
// ListLayout::hintGap - see the struct.
const int kHintScale = 3;
} // namespace

int VisibleRowCount(const ListLayout& layout) {
    if (layout.spacing <= 0) return 1;
    int rows = (layout.bottomLimit - layout.firstY) / layout.spacing + 1;
    return rows < 1 ? 1 : rows;
}

int ClampScroll(int offset, int itemCount, int visible) {
    int maxOffset = itemCount - visible;
    if (maxOffset < 0) maxOffset = 0;
    if (offset > maxOffset) offset = maxOffset;
    if (offset < 0) offset = 0;
    return offset;
}

int ScrollToShow(int offset, int selected, int itemCount, int visible) {
    if (selected < offset) offset = selected;                 // scrolled off the top
    if (selected >= offset + visible) offset = selected - visible + 1;  // off the bottom
    return ClampScroll(offset, itemCount, visible);
}

void DrawScrollHints(Renderer& renderer, const ListLayout& layout, int itemCount,
                      int offset, int visible) {
    if (offset > 0) {
        DrawCenteredLabel(renderer, layout.firstY - layout.hintGap, "MORE ABOVE",
                          kHintScale, Palette::Dim);
    }
    if (offset + visible < itemCount) {
        int lastRowY = layout.firstY + (visible - 1) * layout.spacing;
        DrawCenteredLabel(renderer, lastRowY + layout.hintGap, "MORE BELOW",
                          kHintScale, Palette::Dim);
    }
}

void DrawScrollableList(Renderer& renderer, const ListLayout& layout,
                         const std::vector<std::string>& items, int selected,
                         int offset, int scale, Color textColor, Color selectedColor) {
    int count   = static_cast<int>(items.size());
    int visible = VisibleRowCount(layout);
    offset = ClampScroll(offset, count, visible);

    for (int row = 0; row < visible; row++) {
        int index = offset + row;
        if (index >= count) break;
        Color c = (index == selected) ? selectedColor : textColor;
        DrawCenteredLabel(renderer, layout.firstY + row * layout.spacing,
                          items[static_cast<size_t>(index)].c_str(), scale, c);
    }

    DrawScrollHints(renderer, layout, count, offset, visible);
}

void DrawLabelLeft(Renderer& renderer, int x, int y, const char* text, int scale,
                    Color color) {
    renderer.DrawText(x, y, text, scale, color.r, color.g, color.b);
}

void DrawLabelRight(Renderer& renderer, int rightX, int y, const char* text, int scale,
                     Color color) {
    int x = rightX - renderer.TextWidth(text, scale);
    renderer.DrawText(x, y, text, scale, color.r, color.g, color.b);
}

void DrawPaneScrollHints(Renderer& renderer, const ListLayout& layout, int x, int width,
                          int itemCount, int offset, int visible) {
    if (offset > 0) {
        const char* text = "MORE ABOVE";
        int tx = x + (width - renderer.TextWidth(text, kHintScale)) / 2;
        DrawLabelLeft(renderer, tx, layout.firstY - layout.hintGap, text, kHintScale,
                      Palette::Dim);
    }
    if (offset + visible < itemCount) {
        const char* text = "MORE BELOW";
        int tx = x + (width - renderer.TextWidth(text, kHintScale)) / 2;
        int lastRowY = layout.firstY + (visible - 1) * layout.spacing;
        DrawLabelLeft(renderer, tx, lastRowY + layout.hintGap, text, kHintScale,
                      Palette::Dim);
    }
}

std::vector<std::string> WrapText(Renderer& renderer, const char* text, int scale,
                                   int maxWidth) {
    std::vector<std::string> lines;
    if (!text) return lines;

    std::string line;
    std::string source(text);
    size_t i = 0;
    while (i < source.size()) {
        // One word, plus the single space that will precede it if it is not
        // first on the line.
        size_t end = source.find(' ', i);
        if (end == std::string::npos) end = source.size();
        std::string word = source.substr(i, end - i);
        i = end;
        while (i < source.size() && source[i] == ' ') i++;
        if (word.empty()) continue;

        if (line.empty()) {
            line = word;
            continue;
        }
        std::string candidate = line + " " + word;
        if (renderer.TextWidth(candidate.c_str(), scale) <= maxWidth) {
            line = candidate;
        } else {
            lines.push_back(line);
            line = word;
        }
    }
    if (!line.empty()) lines.push_back(line);
    return lines;
}

} // namespace bbr
