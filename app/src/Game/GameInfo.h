// GameInfo.h - which Bloodborne installation(s) are present, detected by
// inspecting AFR content rather than matching a fixed title-ID whitelist.
//
// Why not a whitelist: a user can dump, retitle, and reinstall Bloodborne
// under a title ID Sony never issued (this project's own test console runs
// a second copy under a custom ID specifically to run a second mod
// alongside the original), and more than one such install can legitimately
// exist side by side. A hardcoded list of six regional SKUs can't express
// either of those, so detection instead asks "does this AFR folder contain
// something that is really Bloodborne's data" - see DetectAll().
#pragma once

#include <string>
#include <vector>

namespace bbr {

struct TitleInfo {
    std::string titleId;     // the AFR folder's name - may not be a real Sony-issued ID
    std::string description; // region/edition label, only if titleId matches a known official SKU
};

class GameInfo {
public:
    // Lists /data/GoldHEN/AFR/ and, for every entry, checks for
    // <entry>/dvdroot_ps4/event/common.emevd.dcx with the DCX magic header.
    // That combination is real, on-disk evidence the folder holds
    // Bloodborne data - not a guess based on the folder's name. Returns one
    // TitleInfo per match; can legitimately return more than one (see the
    // note above) or zero (nothing installed, or nothing AFR-seeded yet).
    static std::vector<TitleInfo> DetectAll();
};

} // namespace bbr
