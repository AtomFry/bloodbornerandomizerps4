#include "SetupDefaultsScreen.h"

#include "Controls.h"

#include "../Platform/Log.h"
#include "../Randomizer/RandomizerDefaultsStore.h"

#include <string.h>

#include <string>
#include <vector>

namespace bbr {

namespace {
// --- text scales -----------------------------------------------------------
//
// No kTitleScale. This screen's title row is the tab strip now: Controls.h's
// kTabScale draws the two tabs and kTabHeadingScale the active one's name
// beneath them (worlds B1). The wizard, which is not tabbed, keeps its own
// title and its own constants - which is why settings_ui_verify.py no longer
// compares the two screens' title geometry, and pins the wizard's on its own.
const int kHeadingScale = 4;
const int kRowScale     = 3;
const int kFooterScale  = 3;
// The title-ID editor is a full-screen mode of its own, outside the tab frame,
// and keeps the heading scale the screen title used to have.
const int kEditTitleScale = 5;
const int kEditScale      = 6;

// --- the three columns -----------------------------------------------------
//
// 60 + 580 + 20 + 2 + 18 + 700 + 20 + 2 + 18 + 440 + 60 = 1920. The rail is
// 580 wide because it has to hold BLOODBORNE TITLE ID   CUSA03173 at scale 3
// (552px), which is the widest thing on this screen and what sizes the whole
// split. Every number below is asserted by tools/settings_ui_verify.py against
// the atlas's own advances - none of them is a character count.
const int kRailX          = 60;
const int kRailW          = 580;
const int kPaneX          = 680;
const int kPaneW          = 700;
const int kPaneValueRight = 1380; // values are right-aligned to this edge
const int kHelpX          = 1420;
const int kHelpW          = 440;

// --- vertical furniture ----------------------------------------------------
// No kTitleY: see the scales above. The tab strip owns the top of the screen
// and Controls.h owns its geometry, so both tabbed screens draw the frame in
// the same place by construction rather than by agreement.
// No kHeaderY: this screen's header band carries nothing since the title ID
// moved into the pane. The rule below it stays, because it still separates the
// screen title from the three columns.
const int kHeaderRuleY  = 196;
const int kColumnRuleY  = 210;
const int kColumnRuleH  = 750;
const int kRailRow0Y    = 230;
const int kRailRuleY    = 296;
const int kRailFirstY   = 330;   // same band and same pitch as the pane rows
const int kRailPitch    = 76;
const int kPaneHeadingY = 220;
const int kHelpTitleY   = 220;
const int kHelpPitch    = 52;
const int kHelpRuleY    = 330;
const int kHelpBodyY    = 356;
const int kHelpTitleMaxLines = 2;
const int kHelpBodyMaxLines  = 11;
const int kFooterY      = 1000;

const int kRuleThickness = 2;

// The focus bar sits behind the focused row. At pitch 76 with scale-3 text,
// (rowY - 10, height 64) contains the row's whole ink box and still leaves
// 12px to the next row's bar, so two adjacent bars never touch.
const int kBarOffsetY = -10;
const int kBarHeight  = 64;

// Rules and separators are 2px FillRects, not SDL_RenderDrawLine - one fewer
// SDL entry point to prove on hardware (plan P4). Dimmer than Palette::Dim,
// which is text: a rule that reads as loud as a label is furniture competing
// with content.
const Color kRuleColor = { 64, 72, 82 };

const char* const kFooterLine =
    "UP DOWN MOVE   LEFT RIGHT CHANGE   X SELECT   O BACK   OPTIONS SAVE";

// Left/Right mean two different things on this screen and always have: a
// value change in the pane, and - since the tabs landed - a tab switch on the
// rail (B31, spec worlds D19). Nothing is re-bound; the rail simply stopped
// throwing both inputs away.
const char* const kRailFooterLine =
    "UP DOWN MOVE   LEFT RIGHT TABS   X SELECT   O BACK   OPTIONS SAVE";

char CycleLetter(char c, int dir) {
    int idx = ((c - 'A') + dir + 26) % 26;
    return (char)('A' + idx);
}

char CycleDigit(char c, int dir) {
    int idx = ((c - '0') + dir + 10) % 10;
    return (char)('0' + idx);
}
} // namespace

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void SetupDefaultsScreen::Update(const ButtonEdges& input) {
    if (mode_ == Mode::Picker && pickerSetting_) {
        const SettingDef& def = *pickerSetting_;
        if (picker_.Update(input, SelectionStrings(def), SelectionTable(def),
                           SelectionCount(def), SelectionFlags(def, working_))) {
            // Straight back to the row it was opened from: nothing cleared the
            // pane's cursor while the picker was up, so nothing has to restore
            // it.
            mode_ = Mode::Settings;
            pickerSetting_ = nullptr;
        }
        return;
    }
    if (mode_ == Mode::EditTitleId) {
        UpdateEditTitleId(input);
        return;
    }
    UpdateSettings(input);
}

void SetupDefaultsScreen::UpdateSettings(const ButtonEdges& input) {
    // OPTIONS saves from either region, as it always has: saving is not an
    // action of whichever row is highlighted.
    if (input.options) {
        defaults_ = working_;
        SaveRandomizerDefaults(defaults_);
        Log("defaults: saved");
        requestedScreen_ = ScreenId::Worlds;
        return;
    }

    if (focus_ == Focus::Rail) UpdateRail(input);
    else                       UpdatePane(input);
}

void SetupDefaultsScreen::UpdateRail(const ButtonEdges& input) {
    railCursor_ = NavigateVertical(railCursor_, kRailItemCount, input);

    // Landing on a category is what changes which settings the pane shows;
    // landing on row 0 deliberately does not, so the pane never blanks.
    if (railCursor_ >= 1) lastCategory_ = railCursor_ - 1;

    // Left/Right switch tabs while focus is on the rail (B31). They were
    // already doing nothing here - row 0's value is only ever changed inside
    // its own editor, and a category has no value - so this is the one place
    // the tab switch could go without a button gaining a second meaning.
    //
    // The pane keeps its own Left/Right, which still changes a value.
    if (input.left || input.right) {
        // Edits are local until OPTIONS writes them (see the header), so
        // leaving discards them - exactly as O does, and said out loud for
        // the same reason.
        Log("defaults: switching to the WORLDS tab - unsaved changes discarded");
        requestedScreen_ = ScreenId::Worlds;
        return;
    }

    if (input.cross) {
        if (railCursor_ == 0) OpenTitleIdEditor();
        else                  focus_ = Focus::List;
    }

    if (input.circle) {
        // O leaves the DEFAULTS tab for the WORLDS tab, which is the only
        // other screen there is: the main menu it used to return to went with
        // the wizards it listed (worlds B27, milestone 6 step 5). Exiting the
        // app is O on the WORLDS rail, one press further on.
        Log("defaults: cancelled - no changes saved");
        requestedScreen_ = ScreenId::Worlds;
    }
}

void SetupDefaultsScreen::UpdatePane(const ButtonEdges& input) {
    SettingCategory category = Category();
    int count = CategorySize(category);
    if (count <= 0) {          // an empty category cannot happen today and is
        focus_ = Focus::Rail;  // a model bug if it ever does - do not trap the
        return;                // player in a pane with nothing in it
    }

    int& cursor = listCursor_[lastCategory_];
    cursor = NavigateVertical(cursor, count, input);
    listScroll_[lastCategory_] = ScrollToShow(listScroll_[lastCategory_], cursor, count,
                                              VisibleRowCount(kPaneLayout));

    const SettingDef& def = SettingInCategory(category, cursor);

    // Left/Right are the ONLY way a setting changes, and AdjustSetting is the
    // only thing that writes one.
    if (input.left || input.right) {
        AdjustSetting(def, working_, input.right ? 1 : -1);
        Log((std::string("defaults: ") + def.label + " = " +
             SettingValueText(def, working_)).c_str());
    }

    // X advances: into the picker on a drill-in, and nowhere at all on a
    // toggle. It must not toggle - see the header.
    if (input.cross && IsDrillIn(def)) {
        picker_.Reset();
        pickerSetting_ = &def;
        mode_ = Mode::Picker;
    }

    if (input.circle) focus_ = Focus::Rail;
}

void SetupDefaultsScreen::OpenTitleIdEditor() {
    // Seed the editor from whatever's already set, if it's the right shape;
    // otherwise leave editBuf_ at its current/default value untouched.
    const std::string& src = working_.bloodborneTitleId;
    if ((int)src.size() == kTitleIdLen) {
        memcpy(editBuf_, src.c_str(), kTitleIdLen);
        editBuf_[kTitleIdLen] = '\0';
    }
    cursor_ = 0;
    mode_ = Mode::EditTitleId;
}

void SetupDefaultsScreen::UpdateEditTitleId(const ButtonEdges& input) {
    // Left/right moves which character is being edited (that's the actual
    // left-to-right motion); up/down changes the highlighted character -
    // swapped from the first pass per testing feedback.
    if (input.left)  cursor_ = (cursor_ + kTitleIdLen - 1) % kTitleIdLen;
    if (input.right) cursor_ = (cursor_ + 1) % kTitleIdLen;

    if (input.up || input.down) {
        int dir = input.up ? 1 : -1;
        if (cursor_ < 4) editBuf_[cursor_] = CycleLetter(editBuf_[cursor_], dir);
        else             editBuf_[cursor_] = CycleDigit(editBuf_[cursor_], dir);
    }

    if (input.cross) {
        working_.bloodborneTitleId = editBuf_;
        Log((std::string("defaults: title id set to ") + editBuf_).c_str());
        mode_ = Mode::Settings;
    }

    if (input.circle) {
        Log("defaults: title id edit cancelled");
        mode_ = Mode::Settings;
    }
}

// ---------------------------------------------------------------------------
// Draw
// ---------------------------------------------------------------------------

std::string SetupDefaultsScreen::TitleIdDisplay() const {
    return working_.bloodborneTitleId.empty() ? std::string("NOT SET")
                                              : working_.bloodborneTitleId;
}

const SettingDef& SetupDefaultsScreen::SelectedSetting() const {
    return SettingInCategory(Category(), listCursor_[lastCategory_]);
}

void SetupDefaultsScreen::Draw(Renderer& renderer) {
    if (mode_ == Mode::EditTitleId) {
        DrawEditTitleId(renderer);
        return;
    }

    DrawSettings(renderer);

    // The picker draws over the screen it was opened from rather than
    // replacing it: the dimmed parent is what keeps the list's context
    // visible, as Bloodborne's own Origin picker does. FillRectBlend is the
    // one SDL entry point here that has never run on a PS4 - if the screen
    // behind is not dimmed but gone, see the plan's 4.3.
    if (mode_ == Mode::Picker && pickerSetting_) {
        const SettingDef& def = *pickerSetting_;
        renderer.FillRectBlend(0, 0, kScreenWidth, kScreenHeight, 0, 0, 0, kScrimAlpha);
        picker_.Draw(renderer, SelectionStrings(def), SelectionTable(def),
                     SelectionCount(def), SelectionFlags(def, working_));
    }
}

void SetupDefaultsScreen::DrawSettings(Renderer& renderer) {
    renderer.Clear(20, 24, 28);

    // The tab strip, and the active tab's name as the heading beneath it
    // (B1). The screen title that used to sit here said SETUP DEFAULTS; the
    // tab says what this is now, and saying it twice would be furniture.
    DrawTabs(renderer, kTabDefaults);
    DrawLabelLeft(renderer, kTabX, kTabHeadingY, TabLabel(kTabDefaults),
                  kTabHeadingScale, Palette::Heading);

    // No header readout. The title ID is shown in the pane when its rail row is
    // selected, the same way the wizard shows the seed - repeating the value
    // beside its own row and again in a header put it on screen three times.
    renderer.FillRect(kRailX, kHeaderRuleY, 1800, kRuleThickness,
                      kRuleColor.r, kRuleColor.g, kRuleColor.b);
    renderer.FillRect(kPaneX - 20, kColumnRuleY, kRuleThickness, kColumnRuleH,
                      kRuleColor.r, kRuleColor.g, kRuleColor.b);
    renderer.FillRect(kHelpX - 20, kColumnRuleY, kRuleThickness, kColumnRuleH,
                      kRuleColor.r, kRuleColor.g, kRuleColor.b);

    DrawRail(renderer);
    DrawPane(renderer);
    DrawHelp(renderer);

    DrawCenteredLabel(renderer, kFooterY,
                      focus_ == Focus::Rail ? kRailFooterLine : kFooterLine,
                      kFooterScale, Palette::Dim);
}

void SetupDefaultsScreen::DrawRailRow(Renderer& renderer, int y, const char* text,
                                      bool focused, bool current) {
    if (focused) {
        renderer.FillRect(kRailX, y + kBarOffsetY, kRailW, kBarHeight,
                          Palette::SelectedBar.r, Palette::SelectedBar.g,
                          Palette::SelectedBar.b);
    }
    // The category the pane is showing stays in the selected colour without
    // the bar whenever the cursor is elsewhere - in the pane, or on row 0 - so
    // it is still obvious which category those settings belong to, and still
    // obvious that the cursor has moved on.
    Color color = (focused || current) ? Palette::Selected : Palette::Text;
    DrawLabelLeft(renderer, kRailX, y, text, kRowScale, color);
}

void SetupDefaultsScreen::DrawRail(Renderer& renderer) {
    bool railHasFocus = (focus_ == Focus::Rail);

    // A bare navigation label, like the six category rows below it; the value
    // is shown in the pane, immediately to its right.
    DrawRailRow(renderer, kRailRow0Y, "BLOODBORNE TITLE ID", railHasFocus && railCursor_ == 0,
                false);

    // Row 0 sits outside the category block, separated from it by its own rule
    // - the same treatment FINISH gets below the categories on the wizard.
    renderer.FillRect(kRailX, kRailRuleY, kRailW, kRuleThickness,
                      kRuleColor.r, kRuleColor.g, kRuleColor.b);

    for (int i = 0; i < kCategoryCount; i++) {
        SettingCategory category = kCategories[i];
        DrawRailRow(renderer, kRailFirstY + i * kRailPitch, CategoryLabel(category),
                    railHasFocus && railCursor_ == i + 1,
                    i == lastCategory_);
    }
}

void SetupDefaultsScreen::DrawPane(Renderer& renderer) {
    // Row 0 shows its value here rather than beside itself, so the pane is not
    // left showing a category the cursor has moved away from - the same rule
    // the wizard applies to SEED.
    if (focus_ == Focus::Rail && railCursor_ == 0) {
        DrawLabelLeft(renderer, kPaneX, kPaneHeadingY, "BLOODBORNE TITLE ID", kHeadingScale,
                      Palette::Heading);
        DrawLabelLeft(renderer, kPaneX, kPaneLayout.firstY, "TITLE ID", kRowScale,
                      Palette::Text);
        DrawLabelRight(renderer, kPaneValueRight, kPaneLayout.firstY,
                       TitleIdDisplay().c_str(), kRowScale, Palette::Text);
        return;
    }

    SettingCategory category = Category();

    DrawLabelLeft(renderer, kPaneX, kPaneHeadingY, CategoryLabel(category), kHeadingScale,
                  Palette::Heading);

    int count   = CategorySize(category);
    int visible = VisibleRowCount(kPaneLayout);
    int offset  = ClampScroll(listScroll_[lastCategory_], count, visible);

    for (int row = 0; row < visible; row++) {
        int index = offset + row;
        if (index >= count) break;

        const SettingDef& def = SettingInCategory(category, index);
        int  y       = kPaneLayout.firstY + row * kPaneLayout.spacing;
        bool focused = (focus_ == Focus::List && index == listCursor_[lastCategory_]);

        if (focused) {
            renderer.FillRect(kPaneX, y + kBarOffsetY, kPaneW, kBarHeight,
                              Palette::SelectedBar.r, Palette::SelectedBar.g,
                              Palette::SelectedBar.b);
        }
        Color color = focused ? Palette::Selected : Palette::Text;
        DrawLabelLeft(renderer, kPaneX, y, def.label, kRowScale, color);
        DrawLabelRight(renderer, kPaneValueRight, y,
                       SettingValueText(def, working_).c_str(), kRowScale, color);
    }

    DrawPaneScrollHints(renderer, kPaneLayout, kPaneX, kPaneW, count, offset, visible);
}

void SetupDefaultsScreen::DrawHelp(Renderer& renderer) {
    const char* title;
    const char* body;
    if (railCursor_ == 0) {
        // Focus can only be in the pane while the rail cursor is on a
        // category, so this is the whole of the "rail row 0" case.
        title = "BLOODBORNE TITLE ID";
        body  = TitleIdHelp();
    } else {
        const SettingDef& def = SelectedSetting();
        title = def.label;
        body  = def.help;
    }

    // Wrapped every frame rather than cached: the wrap is a few hundred
    // advance lookups for one string, against the several hundred glyph blits
    // the same frame already costs, and a cache here would be the only thing
    // in the draw path needing invalidation.
    std::vector<std::string> titleLines = WrapText(renderer, title, kRowScale, kHelpW);
    for (int i = 0; i < (int)titleLines.size() && i < kHelpTitleMaxLines; i++) {
        DrawLabelLeft(renderer, kHelpX, kHelpTitleY + i * kHelpPitch,
                      titleLines[(size_t)i].c_str(), kRowScale, Palette::Selected);
    }

    renderer.FillRect(kHelpX, kHelpRuleY, kHelpW, kRuleThickness,
                      kRuleColor.r, kRuleColor.g, kRuleColor.b);

    std::vector<std::string> bodyLines = WrapText(renderer, body, kRowScale, kHelpW);
    for (int i = 0; i < (int)bodyLines.size() && i < kHelpBodyMaxLines; i++) {
        DrawLabelLeft(renderer, kHelpX, kHelpBodyY + i * kHelpPitch,
                      bodyLines[(size_t)i].c_str(), kRowScale, Palette::Text);
    }
}

void SetupDefaultsScreen::DrawEditTitleId(Renderer& renderer) {
    renderer.Clear(20, 24, 28);

    DrawCenteredLabel(renderer, 300, "BLOODBORNE TITLE ID", kEditTitleScale,
                      Palette::Heading);

    int totalWidth = renderer.TextWidth(editBuf_, kEditScale);
    int startX     = (kScreenWidth - totalWidth) / 2;
    int y          = 460;

    // Proportional font: step by the character actually drawn, not by a fixed
    // "A" width, so the row stays centred on totalWidth (see DrawEditSeed).
    char single[2] = { 0, 0 };
    int pen = startX;
    for (int i = 0; i < kTitleIdLen; i++) {
        single[0] = editBuf_[i];
        Color c = (i == cursor_) ? Palette::Selected : Palette::Text;
        renderer.DrawText(pen, y, single, kEditScale, c.r, c.g, c.b);
        pen += renderer.TextWidth(single, kEditScale);
    }

    DrawCenteredLabel(renderer, kScreenHeight - 130, "LEFT RIGHT SELECT   UP DOWN CHANGE", kFooterScale, Palette::Dim);
    DrawCenteredLabel(renderer, kScreenHeight - 80, "X SAVE   O CANCEL", kFooterScale, Palette::Dim);
}

} // namespace bbr
