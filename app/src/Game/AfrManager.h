// AfrManager.h - status of the AFR (alternate file redirect) tree for one
// title. UI code only ever sees this struct, never the raw
// /data/GoldHEN/AFR path - that string exists in exactly one place, this
// class's .cpp.
#pragma once

#include <string>

namespace bbr {

struct AfrStatus {
    bool rootExists  = false; // /data/GoldHEN/AFR/<titleId> exists
    bool writable    = false; // could create+delete a marker file there
    bool seeded      = false; // .../dvdroot_ps4 subfolder exists (vanilla
                               // game data has been copied in over FTP)

    // Whether .../dvdroot_ps4/.bbrandomizer_manifest exists. Deliberately
    // inside dvdroot_ps4, not somewhere else in the title's AFR folder:
    // the eventual transaction model stages/activates dvdroot_ps4 as one
    // atomic unit (rename staged -> active), so a marker living inside it
    // travels and disappears with the exact content it describes. There is
    // no separate app-level "is it on" flag to fall out of sync - nothing
    // writes this file yet (no randomizer engine exists), so this is
    // always false today, honestly.
    bool randomized  = false;
};

class AfrManager {
public:
    // Only meaningful if the caller already knows titleId is a real,
    // detected title (see GameInfo::DetectAll) - this class doesn't know or
    // care which titles are "real Bloodborne", just paths.
    static AfrStatus Check(const std::string& titleId);
};

} // namespace bbr
