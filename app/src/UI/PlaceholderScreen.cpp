#include "PlaceholderScreen.h"

#include "Controls.h"

#include "../Platform/Log.h"

namespace bbr {

namespace {
const int kTitleScale  = 5;
const int kBodyScale   = 4;
const int kFooterScale = 3;
} // namespace

void PlaceholderScreen::Update(const ButtonEdges& input) {
    if (input.circle) {
        Log("placeholder: O pressed - returning to menu");
        requestedScreen_ = ScreenId::Menu;
    }
}

void PlaceholderScreen::Draw(Renderer& renderer) {
    renderer.Clear(20, 24, 28);

    DrawCenteredLabel(renderer, 400, heading_.c_str(), kTitleScale, Palette::Heading);
    DrawCenteredLabel(renderer, 520, "NOT BUILT YET", kBodyScale, Palette::Text);

    DrawCenteredLabel(renderer, kScreenHeight - 80, "O RETURN", kFooterScale, Palette::Dim);
}

} // namespace bbr
