// RandomizerSettings.h - plain configuration data. No SDL2, no UI, no
// filesystem - just what the randomizer would eventually be configured with.
// M4 only builds the data + a screen to edit it; nothing here does anything
// yet (RandomizerEngine doesn't exist until the actual randomizer does).
#pragma once

namespace bbr {

struct RandomizerSettings {
    enum class Difficulty { Easy, Normal, Hard };

    bool randomizeEnemies = true;
    bool randomizeBosses  = true;
    Difficulty difficulty = Difficulty::Normal;
};

} // namespace bbr
