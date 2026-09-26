#include "FileIo.h"

#include <orbis/libkernel.h>

#include <stdlib.h>

namespace bbr {

namespace {

// Same BSD open(2) flag values Game/AfrManager.cpp already proves correct on
// hardware - musl's <fcntl.h> values are Linux's and don't match FreeBSD's.
// Full table in docs/ps4-homebrew-findings.md.
const int kBsdRdonly    = 0x0000;
const int kBsdWronly    = 0x0001;
const int kBsdCreat     = 0x0200;
const int kBsdTrunc     = 0x0400;
const int kBsdDirectory = 0x00020000;

bool IsDir(const std::string& path) {
    OrbisKernelStat st;
    if (sceKernelStat(path.c_str(), &st) != 0) return false;
    return ((unsigned)st.st_mode & 0xF000u) == 0x4000u;
}

// BSD dirent layout and open(2) flags, hardware-proved for sceKernelGetdents
// on this (FreeBSD-derived) kernel - musl's <dirent.h> and <fcntl.h> carry
// Linux's values and do not match. The table and the reasoning are in
// docs/ps4-homebrew-findings.md section 1, which is the single source for
// it; several files carry their own copy rather than share a header that
// every layer would have to depend on.
struct BsdDirent {
    uint32_t d_fileno;
    uint16_t d_reclen;
    uint8_t  d_type;
    uint8_t  d_namlen;
    char     d_name[256];
};

// Entry names directly under `path` (files and subdirectories), skipping
// "." and "..". Empty on any error (including `path` not being a directory).
std::vector<std::string> ListDirNames(const std::string& path) {
    std::vector<std::string> names;

    int fd = sceKernelOpen(path.c_str(), kBsdRdonly | kBsdDirectory, 0777);
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

} // namespace

bool ReadWholeFile(const std::string& path, std::vector<uint8_t>& out) {
    int fd = sceKernelOpen(path.c_str(), kBsdRdonly, 0);
    if (fd < 0) return false;

    // Deliberately not sized via sceKernelStat()'s st_size: on real hardware
    // that field came back wrong (88 for a genuine 44KB file) even though
    // st_mode - used elsewhere in Game/AfrManager.cpp - is fine, meaning the
    // OrbisKernelStat/struct-stat layout this SDK ships doesn't fully match
    // the real kernel ABI. Reading in growing chunks until a short read
    // sidesteps that struct entirely; a short read reliably means EOF for a
    // regular local file (unlike a pipe/socket), so this is safe here.
    out.clear();
    const size_t kChunk = 65536;
    for (;;) {
        size_t oldSize = out.size();
        out.resize(oldSize + kChunk);
        ssize_t n = (ssize_t)sceKernelRead(fd, out.data() + oldSize, kChunk);
        if (n < 0) {
            sceKernelClose(fd);
            out.clear();
            return false;
        }
        out.resize(oldSize + (size_t)n);
        if ((size_t)n < kChunk) break;
    }
    sceKernelClose(fd);
    return true;
}

bool WriteWholeFile(const std::string& path, const std::vector<uint8_t>& data) {
    int fd = sceKernelOpen(path.c_str(), kBsdWronly | kBsdCreat | kBsdTrunc, 0777);
    if (fd < 0) return false;

    size_t total = 0;
    while (total < data.size()) {
        ssize_t n = (ssize_t)sceKernelWrite(fd, data.data() + total, data.size() - total);
        if (n <= 0) break;
        total += (size_t)n;
    }
    sceKernelClose(fd);
    return total == data.size();
}

bool MakeDirsRecursive(const std::string& path) {
    std::string prefix;
    for (size_t i = 0; i < path.size(); i++) {
        prefix.push_back(path[i]);
        bool isLast = (i + 1 == path.size());
        if (path[i] == '/' || isLast) {
            if (prefix.size() > 1 && !IsDir(prefix)) {
                if (sceKernelMkdir(prefix.c_str(), 0777) != 0 && !IsDir(prefix)) return false;
            }
        }
    }
    return true;
}

namespace {
bool HasSuffix(const std::string& s, const std::string& suffix) {
    return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}
} // namespace

bool CopyDirRecursive(const std::string& srcDir, const std::string& dstDir) {
    if (!IsDir(srcDir)) return false;
    if (!MakeDirsRecursive(dstDir)) return false;

    bool ok = true;
    for (const std::string& name : ListDirNames(srcDir)) {
        // .bak files are the reference tool's own backup artifacts, not
        // real game data - skip them so they don't end up in AFR output.
        if (HasSuffix(name, ".bak")) continue;

        std::string srcPath = srcDir + "/" + name;
        std::string dstPath = dstDir + "/" + name;

        if (IsDir(srcPath)) {
            if (!CopyDirRecursive(srcPath, dstPath)) ok = false;
        } else {
            std::vector<uint8_t> data;
            if (!ReadWholeFile(srcPath, data) || !WriteWholeFile(dstPath, data)) ok = false;
        }
    }
    return ok;
}

} // namespace bbr
