// PlaceholderScreen.h - a screen that shows a heading and "NOT BUILT YET",
// returning to the menu on Circle. Stands in for a wizard destination that
// exists in navigation but has no real content yet - see UI_BLUEPRINT.md
// for what each one will eventually become. One reusable class instead of
// three near-identical ones, same reasoning as Controls.h.
#pragma once

#include "Screen.h"

#include <string>

namespace bbr {

class PlaceholderScreen : public Screen {
public:
    explicit PlaceholderScreen(std::string heading) : heading_(std::move(heading)) {}

    void Update(const ButtonEdges& input) override;
    void Draw(Renderer& renderer) override;
    ScreenId RequestedScreen() const override { return requestedScreen_; }

private:
    std::string heading_;
    ScreenId requestedScreen_ = ScreenId::None;
};

} // namespace bbr
