#include "SaveProbeScreen.h"

#include "Controls.h"

#include "../Platform/Log.h"
#include "../Platform/SaveDataProbe.h"

namespace bbr {

namespace {
const int kTitleScale  = 5;
const int kBodyScale   = 3;
const int kFooterScale = 3;

// The progress log's band and density - this is the same kind of output and
// there is no reason for it to look different.
const ListLayout kLogLayout = { 300, 70, 920, 50 };
} // namespace

SaveProbeScreen::SaveProbeScreen(const std::string& titleId)
    : titleId_(titleId) {
    lines_.push_back("SAVE DATA PROBE - TEMPORARY DIAGNOSTIC");
    lines_.push_back("");
    lines_.push_back("TARGET TITLE " + titleId_);
    lines_.push_back("");
    lines_.push_back("PRESS X TO RUN THE PROBE");
    lines_.push_back("PRESS O TO GO BACK");
    lines_.push_back("");
    lines_.push_back("THIS MOUNTS SAVE DATA READ ONLY AND LISTS IT.");
    lines_.push_back("NOTHING IS WRITTEN, DELETED OR MOVED.");
}

void SaveProbeScreen::Update(const ButtonEdges& input) {
    int visible = VisibleRowCount(kLogLayout);

    if (input.cross && !hasRun_) {
        hasRun_ = true;
        lines_.clear();
        Log("saveprobe: X pressed - running probe");

        // Runs to completion inside one frame. It is a handful of syscalls and
        // a small directory, so there is nothing to step across frames the way
        // the randomizer job is - and if it hangs or dies, that is itself the
        // finding, recorded in live.log line by line.
        ProbeSaveData(titleId_, lines_);

        // Show the end, which is where the answer is.
        scroll_ = ClampScroll((int)lines_.size(), (int)lines_.size(), visible);
        return;
    }

    if (input.up)   scroll_--;
    if (input.down) scroll_++;
    scroll_ = ClampScroll(scroll_, (int)lines_.size(), visible);

    if (input.circle) requestedScreen_ = ScreenId::Menu;
}

void SaveProbeScreen::Draw(Renderer& renderer) {
    renderer.Clear(20, 24, 28);

    DrawCenteredLabel(renderer, 140, "SAVE DATA PROBE", kTitleScale, Palette::Heading);

    int visible = VisibleRowCount(kLogLayout);
    int offset  = ClampScroll(scroll_, (int)lines_.size(), visible);

    for (int row = 0; row < visible; row++) {
        int index = offset + row;
        if (index >= (int)lines_.size()) break;

        const std::string& line = lines_[index];

        // Failures in a wall of near-identical lines are the thing worth
        // seeing, so they are coloured rather than left to be read carefully.
        Color color = Palette::Text;
        if (line.find("FAILED") != std::string::npos ||
            line.find("CANNOT") != std::string::npos) {
            color = Palette::Bad;
        } else if (line.find(" OK") != std::string::npos ||
                   line.find("MOUNTED AT") != std::string::npos) {
            color = Palette::Good;
        }

        DrawLabelLeft(renderer, 120, kLogLayout.firstY + row * kLogLayout.spacing,
                      line.c_str(), kBodyScale, color);
    }

    DrawPaneScrollHints(renderer, kLogLayout, 120, 1680,
                        (int)lines_.size(), offset, visible);

    DrawCenteredLabel(renderer, kScreenHeight - 100,
                      hasRun_ ? "UP DOWN SCROLL" : "X RUN PROBE   UP DOWN SCROLL",
                      kFooterScale, Palette::Dim);
    DrawCenteredLabel(renderer, kScreenHeight - 50, "O BACK", kFooterScale, Palette::Dim);
}

} // namespace bbr
