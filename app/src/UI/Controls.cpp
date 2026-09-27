#include "Controls.h"

namespace bbr {

namespace {
const char* const kTabLabels[kTabCount] = { "WORLDS", "DEFAULTS" };

// The box's own colour. The same value the rules use, one step down from
// Palette::Dim: a frame drawn as loud as its label is furniture competing with
// content. Repeated here rather than shared with the screens' kRuleColor,
// which is file-local to each of them.
const Color kTabBoxColor = { 64, 72, 82 };
} // namespace

const char* TabLabel(int index) {
    if (index < 0 || index >= kTabCount) return "";
    return kTabLabels[index];
}

void DrawTabs(Renderer& renderer, int active) {
    int x = kTabX;
    for (int i = 0; i < kTabCount; i++) {
        const char* label = kTabLabels[i];
        int labelW = renderer.TextWidth(label, kTabScale);
        int boxW   = labelW + kTabPadX * 2;

        if (i == active) {
            // Four FillRects rather than an outlined rect: FillRect is the one
            // rectangle entry point this app has proven on hardware, and the
            // rules elsewhere are drawn the same way.
            int boxY = kTabY + kTabBoxOffsetY;
            renderer.FillRect(x, boxY, boxW, kTabBorder,
                              kTabBoxColor.r, kTabBoxColor.g, kTabBoxColor.b);
            renderer.FillRect(x, boxY + kTabBoxHeight - kTabBorder, boxW, kTabBorder,
                              kTabBoxColor.r, kTabBoxColor.g, kTabBoxColor.b);
            renderer.FillRect(x, boxY, kTabBorder, kTabBoxHeight,
                              kTabBoxColor.r, kTabBoxColor.g, kTabBoxColor.b);
            renderer.FillRect(x + boxW - kTabBorder, boxY, kTabBorder, kTabBoxHeight,
                              kTabBoxColor.r, kTabBoxColor.g, kTabBoxColor.b);
        }

        Color color = (i == active) ? Palette::Selected : Palette::Dim;
        renderer.DrawText(x + kTabPadX, kTabY, label, kTabScale, color.r, color.g, color.b);
        x += boxW + kTabGap;
    }
}

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

// --- U4 pass 1 -------------------------------------------------------------

namespace {
// --- U4 pass 3: the vignette -----------------------------------------------
//
// THE GROUND IS GRADED, NOT DARKENED AFTERWARDS, and that is a performance
// decision with a measured reason rather than a stylistic one.
//
// The obvious way to vignette is to blend black over the finished frame. This
// app draws through SDL_CreateSoftwareRenderer - every pixel is CPU work - and
// FillRectBlend is read-modify-write. Covering a 280px margin on four edges is
// 1,366,400 blended pixels ON TOP of the 2,073,600-pixel opaque clear it would
// no longer replace: 66% more fill every frame, in the more expensive mode, on
// a console whose menu already redraws in full each time.
//
// Drawing the ground as concentric RINGS instead costs exactly what the clear
// cost, because each pixel is still written precisely once - the rings tile the
// screen and never overlap. The gradient is free.
//
// WHAT IT GIVES UP, stated plainly: an opaque ground darkens the BACKGROUND
// only, so text and frames near the edges are not dimmed the way a blended
// vignette would dim them. On this app that is arguably the better end of the
// trade - what sits near the edges is the tab strip, the headings and the
// footer prompts, and those are meant to stay readable.
const int kVignetteMargin = 280;   // how far in the darkening reaches
const int kVignetteSteps  = 14;    // 20px per ring; must divide the margin
const int kVignetteFloor  = 40;    // the outermost ring, as a percentage of Ground

// Ring `i` of `kVignetteSteps`, counted inward from the screen edge. Integer
// throughout: the floor is a percentage so nothing here needs floating point.
Color VignetteTone(int i) {
    const Color& g = Palette::Ground;
    // 0 at the outer edge, 100 at the inner one, sampled at the ring's middle
    // so the outermost ring is not fully at the floor and the innermost is not
    // fully at the ground - which would band visibly at both ends.
    int t = (200 * i + 100) / (2 * kVignetteSteps);
    int pct = kVignetteFloor + (100 - kVignetteFloor) * t / 100;
    Color out;
    out.r = (unsigned char)(g.r * pct / 100);
    out.g = (unsigned char)(g.g * pct / 100);
    out.b = (unsigned char)(g.b * pct / 100);
    return out;
}
} // namespace

void DrawGround(Renderer& renderer) {
    const int step = kVignetteMargin / kVignetteSteps;

    // The middle, at full ground. Everything outside it is a ring.
    renderer.FillRect(kVignetteMargin, kVignetteMargin,
                      kScreenWidth - 2 * kVignetteMargin,
                      kScreenHeight - 2 * kVignetteMargin,
                      Palette::Ground.r, Palette::Ground.g, Palette::Ground.b);

    // Each ring is four strips. The left and right strips stop short of the
    // top and bottom ones rather than running the full height, so no pixel is
    // written twice - which is the whole basis of the cost argument above.
    for (int i = 0; i < kVignetteSteps; i++) {
        Color c = VignetteTone(i);
        int inset = i * step;
        int w = kScreenWidth - 2 * inset;
        int h = kScreenHeight - 2 * inset;

        renderer.FillRect(inset, inset, w, step, c.r, c.g, c.b);
        renderer.FillRect(inset, inset + h - step, w, step, c.r, c.g, c.b);
        renderer.FillRect(inset, inset + step, step, h - 2 * step, c.r, c.g, c.b);
        renderer.FillRect(inset + w - step, inset + step, step, h - 2 * step,
                          c.r, c.g, c.b);
    }
}

namespace {
// How far a corner tick runs along each edge, and how far the inner line sits
// inside the outer one. Both are deliberately small: at 1px in and 28px long
// the frame reads as a drawn border at three metres rather than as two
// rectangles, which is what it looks like when the gap is opened up.
const int kFrameInset   = 3;
const int kFrameTickLen = 28;
const int kFrameLine    = 2;   // matches every rule in the app
} // namespace

void DrawPanelFrame(Renderer& renderer, int x, int y, int w, int h) {
    const Color& o = Palette::Rule;
    const Color& s = Palette::RuleSoft;

    // The outer rectangle, as four fills. Corners overlap, which costs nothing
    // and avoids four off-by-one cases at the ends of each run.
    renderer.FillRect(x, y, w, kFrameLine, o.r, o.g, o.b);                    // top
    renderer.FillRect(x, y + h - kFrameLine, w, kFrameLine, o.r, o.g, o.b);   // bottom
    renderer.FillRect(x, y, kFrameLine, h, o.r, o.g, o.b);                    // left
    renderer.FillRect(x + w - kFrameLine, y, kFrameLine, h, o.r, o.g, o.b);   // right

    // The inner line is NOT a second rectangle - it is eight short runs, one
    // stepping in from each corner along each edge. A full inner rectangle
    // reads as a double border all the way round, which the reference does not
    // do; the ticks are what make a corner look drawn rather than mitred.
    int ix = x + kFrameInset;
    int iy = y + kFrameInset;
    int iw = w - 2 * kFrameInset;
    int ih = h - 2 * kFrameInset;
    if (iw <= 2 * kFrameTickLen || ih <= 2 * kFrameTickLen) return;

    // top-left and top-right, along the top edge
    renderer.FillRect(ix, iy, kFrameTickLen, kFrameLine, s.r, s.g, s.b);
    renderer.FillRect(ix + iw - kFrameTickLen, iy, kFrameTickLen, kFrameLine, s.r, s.g, s.b);
    // the same two along the bottom
    renderer.FillRect(ix, iy + ih - kFrameLine, kFrameTickLen, kFrameLine, s.r, s.g, s.b);
    renderer.FillRect(ix + iw - kFrameTickLen, iy + ih - kFrameLine, kFrameTickLen,
                      kFrameLine, s.r, s.g, s.b);
    // and four down the sides
    renderer.FillRect(ix, iy, kFrameLine, kFrameTickLen, s.r, s.g, s.b);
    renderer.FillRect(ix, iy + ih - kFrameTickLen, kFrameLine, kFrameTickLen, s.r, s.g, s.b);
    renderer.FillRect(ix + iw - kFrameLine, iy, kFrameLine, kFrameTickLen, s.r, s.g, s.b);
    renderer.FillRect(ix + iw - kFrameLine, iy + ih - kFrameTickLen, kFrameLine,
                      kFrameTickLen, s.r, s.g, s.b);
}

// --- U4 pass 2 -------------------------------------------------------------

namespace {
// The bright rule closing the band top and bottom. Two pixels, matching every
// other rule in the app - this one is meant to be seen.
const int kBandEdge = 2;
} // namespace

void DrawSelectionBand(Renderer& renderer, int x, int y, int w, int h) {
    if (w <= 0 || h <= 0) return;

    const Color& base = Palette::SelectedBar;
    const Color& lit  = Palette::SelectedBarLit;
    const Color& edge = Palette::SelectedEdge;

    // Lower half first, then the lit upper half over it. Two fills rather than a
    // gradient: there is no gradient primitive here, and at these two tones a
    // television reads the split as a falloff rather than as a seam.
    renderer.FillRect(x, y, w, h, base.r, base.g, base.b);
    renderer.FillRect(x, y, w, h / 2, lit.r, lit.g, lit.b);

    // The edges. A band closed top and bottom reads as drawn; the same band
    // without them reads as a rectangle behind the text, which is what the flat
    // fill this replaces looked like.
    if (h >= 2 * kBandEdge) {
        renderer.FillRect(x, y, w, kBandEdge, edge.r, edge.g, edge.b);
        renderer.FillRect(x, y + h - kBandEdge, w, kBandEdge, edge.r, edge.g, edge.b);
    }
}

namespace {
// The gap between a glyph and its own label, and between one prompt and the
// next. The second is much larger on purpose: what separates "CROSS SELECT"
// from "CIRCLE BACK" has to be unmistakably wider than what joins a glyph to
// its word, or the row reads as one run of alternating symbols and nouns.
const int kPromptGlyphGap = 10;
const int kPromptGap      = 46;
} // namespace

Color ButtonColor(const char* glyph) {
    if (glyph == nullptr || glyph[0] == '\0') return Palette::Dim;
    switch ((unsigned char)glyph[0]) {
        case 0x80: return Palette::BtnCross;
        case 0x81: return Palette::BtnCircle;
        case 0x82: return Palette::BtnTriangle;
        case 0x83: return Palette::BtnSquare;
        case 0x84:                       // the pad, all four arms lit
        case 0x85:                       // up/down lit
        case 0x86: return Palette::BtnDpad;   // left/right lit
        default:   return Palette::Dim;
    }
}

void DrawPromptRow(Renderer& renderer, int y, const ButtonPrompt* prompts, int count,
                   int scale) {
    if (prompts == nullptr || count <= 0) return;

    // Measured first, then drawn from the left edge of the measured row, so
    // the row is centred as a WHOLE. Centring each prompt independently would
    // not be centring at all, and measuring with TextWidth rather than by
    // character count is the same reason every other block in this app does:
    // the atlas is proportional.
    int total = 0;
    for (int i = 0; i < count; i++) {
        if (prompts[i].glyph) {
            total += renderer.TextWidth(prompts[i].glyph, scale) + kPromptGlyphGap;
        }
        total += renderer.TextWidth(prompts[i].label, scale);
        if (i + 1 < count) total += kPromptGap;
    }

    int x = (kScreenWidth - total) / 2;
    for (int i = 0; i < count; i++) {
        if (prompts[i].glyph) {
            Color c = ButtonColor(prompts[i].glyph);
            DrawLabelLeft(renderer, x, y, prompts[i].glyph, scale, c);
            x += renderer.TextWidth(prompts[i].glyph, scale) + kPromptGlyphGap;
        }
        DrawLabelLeft(renderer, x, y, prompts[i].label, scale, Palette::Dim);
        x += renderer.TextWidth(prompts[i].label, scale);
        if (i + 1 < count) x += kPromptGap;
    }
}

void DrawRowSeparator(Renderer& renderer, int x, int y, int w) {
    if (w <= 0) return;
    const Color& c = Palette::RuleSoft;
    renderer.FillRect(x, y, w, 1, c.r, c.g, c.b);
}

} // namespace bbr
