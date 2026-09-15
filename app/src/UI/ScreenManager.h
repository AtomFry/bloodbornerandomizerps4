// ScreenManager.h - owns "the current screen". Only one at a time for now;
// M2 has exactly one Screen (HelloScreen), so this is intentionally trivial.
// It's still its own class because M3 (main menu) is the first milestone
// that needs SetScreen() to actually switch to something else.
#pragma once

#include "Screen.h"

#include <memory>

namespace bbr {

class ScreenManager {
public:
    void SetScreen(std::unique_ptr<Screen> screen) { current_ = std::move(screen); }
    Screen* Current() { return current_.get(); }

private:
    std::unique_ptr<Screen> current_;
};

} // namespace bbr
