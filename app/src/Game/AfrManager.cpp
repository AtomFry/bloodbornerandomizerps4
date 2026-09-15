#include "AfrManager.h"

#include <orbis/libkernel.h>

namespace bbr {

namespace {

const unsigned kModeIfmt  = 0xF000;
const unsigned kModeIfdir = 0x4000;

bool IsDir(const std::string& path) {
    OrbisKernelStat st;
    if (sceKernelStat(path.c_str(), &st) != 0) return false;
    return ((unsigned)st.st_mode & kModeIfmt) == kModeIfdir;
}

bool Exists(const std::string& path) {
    OrbisKernelStat st;
    return sceKernelStat(path.c_str(), &st) == 0;
}

// BSD open(2) flag values, proved correct for sceKernelOpen on this kernel -
// musl's <fcntl.h> values are Linux's and don't match FreeBSD's. Full table
// in docs/ps4-homebrew-findings.md.
const int kBsdWronly = 0x0001;
const int kBsdCreat  = 0x0200;
const int kBsdTrunc  = 0x0400;

// Full CRUD against a scratch file in AFR is hardware-proven to work with no
// sandbox escape (docs/ps4-homebrew-findings.md §1) - this is that same
// create+delete probe, trimmed to a yes/no answer.
bool CanWrite(const std::string& afrRoot) {
    std::string marker = afrRoot + "/bbrandomizer_write_test.tmp";
    int fd = sceKernelOpen(marker.c_str(), kBsdWronly | kBsdCreat | kBsdTrunc, 0777);
    if (fd < 0) return false;
    sceKernelClose(fd);
    sceKernelUnlink(marker.c_str());
    return true;
}

} // namespace

AfrStatus AfrManager::Check(const std::string& titleId) {
    AfrStatus status;
    std::string afrRoot = "/data/GoldHEN/AFR/" + titleId;

    status.rootExists = IsDir(afrRoot);
    if (!status.rootExists) return status;

    status.writable = CanWrite(afrRoot);
    status.seeded   = IsDir(afrRoot + "/dvdroot_ps4");
    if (status.seeded) {
        status.randomized = Exists(afrRoot + "/dvdroot_ps4/.bbrandomizer_manifest");
    }
    return status;
}

} // namespace bbr
