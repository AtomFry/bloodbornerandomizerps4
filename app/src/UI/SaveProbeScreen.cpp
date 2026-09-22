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
    lines_.push_back("X        PROBE - MOUNT READ ONLY AND LIST");
    lines_.push_back("SQUARE   PROBE AND BACK UP");
    lines_.push_back("TRIANGLE WRITE PROBE - RESTORE VERIFY DELETE");
    lines_.push_back("O        GO BACK");
    lines_.push_back("");
    lines_.push_back("TRIANGLE WRITES ONLY INTO A SCRATCH SAVE");
    lines_.push_back("DIRECTORY OF ITS OWN AND DELETES IT AGAIN.");
    lines_.push_back("");
    lines_.push_back("ITS LAST STEP MOUNTS THE LIVE SAVE READ-WRITE");
    lines_.push_back("TO SEE IF THAT IS EVEN PERMITTED. IT OPENS NO");
    lines_.push_back("FILE AND UNMOUNTS AT ONCE - BUT IT IS THE ONE");
    lines_.push_back("STEP THAT TOUCHES THE REAL SAVE.");
    lines_.push_back("");
    lines_.push_back("TAKE A BACKUP WITH SQUARE FIRST - THAT IS");
    lines_.push_back("WHAT THE WRITE PROBE RESTORES, AND YOUR");
    lines_.push_back("RECOVERY IF THE LAST STEP GOES WRONG.");
    lines_.push_back("");
    lines_.push_back("R1       DELETE USERDATA FILES - DESTRUCTIVE");
    lines_.push_back("L1       DELETE BACKUP FILES  - DESTRUCTIVE");
    lines_.push_back("SCE_SYS IS NEVER TOUCHED BY EITHER. BACK UP FIRST.");
}

void SaveProbeScreen::Update(const ButtonEdges& input) {
    int visible = VisibleRowCount(kLogLayout);

    if ((input.cross || input.square) && !hasRun_) {
        hasRun_ = true;
        didBackup_ = input.square;
        lines_.clear();
        Log(didBackup_ ? "saveprobe: SQUARE pressed - probe + backup"
                       : "saveprobe: X pressed - probe only");

        // Runs to completion inside one frame. It is a handful of syscalls and
        // a small directory, so there is nothing to step across frames the way
        // the randomizer job is - and if it hangs or dies, that is itself the
        // finding, recorded in live.log line by line.
        ProbeSaveData(titleId_, lines_, didBackup_);

        // Show the end, which is where the answer is.
        scroll_ = ClampScroll((int)lines_.size(), (int)lines_.size(), visible);
        return;
    }

    // A separate button from X and SQUARE on purpose: this is the only path in
    // the app that writes save data, and it should not be reachable by
    // mistyping the read-only one. Same one-frame shape - it copies ~27MB
    // twice, so expect the frame to stall for as long as steps 5 and 6 report.
    if (input.triangle && !hasRun_) {
        hasRun_ = true;
        lines_.clear();
        Log("saveprobe: TRIANGLE pressed - write probe");

        ProbeSaveDataWrite(titleId_, lines_);

        scroll_ = ClampScroll((int)lines_.size(), (int)lines_.size(), visible);
        return;
    }

    // Shoulder buttons, not face buttons: these destroy save contents and
    // should not sit next to the ones someone presses to look around.
    if ((input.r1 || input.l1) && !hasRun_) {
        hasRun_ = true;
        std::string prefix = input.r1 ? "userdata" : "backup";
        lines_.clear();
        Log(("saveprobe: DESTRUCTIVE delete-" + prefix + " probe").c_str());

        ProbeDeleteByPrefix(titleId_, prefix, lines_);

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
            line.find("CANNOT") != std::string::npos ||
            line.find("MISMATCH") != std::string::npos ||
            line.find("TRIPPED") != std::string::npos ||
            line.find("REFUSED") != std::string::npos) {
            color = Palette::Bad;
        } else if (line.find(" OK") != std::string::npos ||
                   line.find("MOUNTED AT") != std::string::npos ||
                   line.find("VERDICT MATCH") != std::string::npos ||
                   line.find("  ARMED") != std::string::npos) {
            color = Palette::Good;
        }

        DrawLabelLeft(renderer, 120, kLogLayout.firstY + row * kLogLayout.spacing,
                      line.c_str(), kBodyScale, color);
    }

    DrawPaneScrollHints(renderer, kLogLayout, 120, 1680,
                        (int)lines_.size(), offset, visible);

    DrawCenteredLabel(renderer, kScreenHeight - 100,
                      hasRun_ ? "UP DOWN SCROLL"
                              : "X PROBE  SQUARE BACKUP  TRIANGLE WRITE  R1 DEL USERDATA  L1 DEL BACKUP",
                      kFooterScale, Palette::Dim);
    DrawCenteredLabel(renderer, kScreenHeight - 50, "O BACK", kFooterScale, Palette::Dim);
}

} // namespace bbr
