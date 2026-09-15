#include "GameInfo.h"

#include <orbis/libkernel.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

namespace bbr {

namespace {

const char* kAfrRoot = "/data/GoldHEN/AFR";

// BSD open(2) flag values and on-disk dirent layout, proved correct for
// sceKernelGetdents on this kernel (FreeBSD-derived; musl's <dirent.h> and
// <fcntl.h> values are Linux's and don't match). See
// docs/ps4-homebrew-findings.md §1 for both tables and why the dirent struct
// is declared locally rather than taken from an SDK typedef.
const int kBsdRdonly    = 0x0000;
const int kBsdDirectory = 0x00020000;

struct BsdDirent {
    uint32_t d_fileno;
    uint16_t d_reclen;
    uint8_t  d_type;
    uint8_t  d_namlen;
    char     d_name[256];
};

const unsigned kModeIfmt  = 0xF000;
const unsigned kModeIfreg = 0x8000;

bool StatMode(const std::string& path, unsigned* modeOut, long long* sizeOut) {
    OrbisKernelStat st;
    if (sceKernelStat(path.c_str(), &st) != 0) return false;
    *modeOut = (unsigned)st.st_mode;
    if (sizeOut) *sizeOut = (long long)st.st_size;
    return true;
}

// Directory entry names under /data/GoldHEN/AFR/, skipping "." and "..".
// Not a general-purpose listing utility - just enough for this one path,
// same "prove concrete first" approach the rest of this project's Game/
// layer uses.
std::vector<std::string> ListAfrRootNames() {
    std::vector<std::string> names;

    int fd = sceKernelOpen(kAfrRoot, kBsdRdonly | kBsdDirectory, 0777);
    if (fd < 0) return names;

    const int kBufSize = 64 * 1024;
    char* buf = (char*)malloc(kBufSize);
    if (!buf) { sceKernelClose(fd); return names; }

    for (;;) {
        int nread = sceKernelGetdents(fd, buf, kBufSize);
        if (nread <= 0) break;

        int pos = 0;
        while (pos < nread) {
            BsdDirent* de = (BsdDirent*)(buf + pos);
            if (de->d_reclen == 0) break;

            std::string name(de->d_name, de->d_namlen);
            if (name != "." && name != "..") names.push_back(name);

            pos += de->d_reclen;
        }
    }

    free(buf);
    sceKernelClose(fd);
    return names;
}

// Content fingerprint: does <afrRoot>/<titleId>/dvdroot_ps4/event/
// common.emevd.dcx exist and start with the DCX magic - a file already read
// and hashed successfully out of AFR on hardware. Checking this instead of
// the folder's name is what lets a retitled/repackaged install (or a second
// one alongside it) get detected correctly.
bool LooksLikeBloodborne(const std::string& titleId) {
    std::string path = std::string(kAfrRoot) + "/" + titleId + "/dvdroot_ps4/event/common.emevd.dcx";

    unsigned mode = 0;
    long long size = 0;
    if (!StatMode(path, &mode, &size)) return false;
    if ((mode & kModeIfmt) != kModeIfreg) return false;
    if (size < 4) return false;

    int fd = sceKernelOpen(path.c_str(), kBsdRdonly, 0777);
    if (fd < 0) return false;
    unsigned char head[4] = {0};
    int n = sceKernelRead(fd, head, sizeof(head));
    sceKernelClose(fd);
    if (n != 4) return false;

    return head[0] == 'D' && head[1] == 'C' && head[2] == 'X' && head[3] == '\0';
}

struct KnownTitle { const char* id; const char* description; };

// Optional "pretty name" lookup only - never used to decide whether a
// title is detected. Ported from the old whitelist; kept purely as a
// label for the six officially-issued SKUs, if one happens to match.
const KnownTitle kKnownTitles[] = {
    { "CUSA00900", "USA" },
    { "CUSA00207", "Australia" },
    { "CUSA00208", "United Kingdom" },
    { "CUSA01363", "Asia" },
    { "CUSA03014", "Japan / The Old Hunters Edition" },
    { "CUSA03173", "Europe / GOTY" },
};
const int kKnownTitleCount = sizeof(kKnownTitles) / sizeof(kKnownTitles[0]);

std::string DescriptionFor(const std::string& titleId) {
    for (int i = 0; i < kKnownTitleCount; i++)
        if (titleId == kKnownTitles[i].id) return kKnownTitles[i].description;
    return "";
}

} // namespace

std::vector<TitleInfo> GameInfo::DetectAll() {
    std::vector<TitleInfo> found;

    for (const std::string& name : ListAfrRootNames()) {
        if (!LooksLikeBloodborne(name)) continue;
        found.push_back(TitleInfo{ name, DescriptionFor(name) });
    }

    return found;
}

} // namespace bbr
