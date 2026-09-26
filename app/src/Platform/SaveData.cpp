#include "SaveData.h"

#include "Log.h"

#include <orbis/SaveData.h>
#include <orbis/UserService.h>
#include <orbis/libkernel.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

namespace bbr {
namespace savedata {

namespace {

// BSD dirent layout and open(2) flags, hardware-proved for sceKernelGetdents
// on this (FreeBSD-derived) kernel - musl's <dirent.h> and <fcntl.h> carry
// Linux's values and do not match. The table and the reasoning are in
// docs/ps4-homebrew-findings.md section 1, which is the single source for
// it; several files carry their own copy rather than share a header that
// every layer would have to depend on.
const int kBsdRdonly    = 0x0000;
const int kBsdWronly    = 0x0001;
const int kBsdCreat     = 0x0200;
const int kBsdTrunc     = 0x0400;
const int kBsdDirectory = 0x00020000;

struct BsdDirent {
    uint32_t d_fileno;
    uint16_t d_reclen;
    uint8_t  d_type;
    uint8_t  d_namlen;
    char     d_name[256];
};

// The only d_type value that matters here. A directory opened read-only does
// not fail - it returns directory data - so telling one apart by its type is
// the only reliable way, and getting it wrong is what once reported sce_sys as
// a 32,768-byte "file" and lost 10 MB of a 36 MB save.
const uint8_t kDtDir = 4;

// A save is sce_sys plus a flat list of slot files; the cap is here so a
// surprise cannot spin forever on hardware.
const int kMaxDepth = 4;

const size_t kChunk = 65536;

// The six officially-issued Bloodborne SKUs. This is the SAVE-DATA sweep of
// technical-findings.md section 8.1 and has nothing to do with the AFR title,
// which comes from the BLOODBORNE TITLE ID setting and is used exactly as
// entered. The two are independent and genuinely differ on the reference
// console - AFR CUSA03173, save CUSA00207 - which is why this is its own
// list. Hardware-confirmed; see docs/ps4-homebrew-findings.md section 6.
struct Candidate { const char* id; const char* label; };
const Candidate kCandidates[] = {
    { "CUSA00900", "USA" },
    { "CUSA00207", "AUSTRALIA" },
    { "CUSA00208", "UNITED KINGDOM" },
    { "CUSA01363", "ASIA" },
    { "CUSA03014", "JAPAN / OLD HUNTERS" },
    { "CUSA03173", "EUROPE / GOTY" },
};
const int kCandidateCount = sizeof(kCandidates) / sizeof(kCandidates[0]);

// How many directories one search may return. A title has one save container
// in every case seen so far; the headroom exists so "more than one" is a
// result that can be reported rather than a truncation nobody notices.
const unsigned kMaxDirs = 16;

std::string Hex(int value) {
    char buf[32];
    snprintf(buf, sizeof(buf), "0x%08X", (unsigned)value);
    return std::string(buf);
}

std::string U64Text(uint64_t value) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%llu", (unsigned long long)value);
    return std::string(buf);
}

// Save directory names and mount paths are fixed-width fields and are NOT
// guaranteed to be null-terminated when they use the whole field.
std::string Bounded(const char* data, size_t maxLen) {
    size_t n = 0;
    while (n < maxLen && data[n] != '\0') n++;
    return std::string(data, n);
}

uint64_t NowUs() { return sceKernelGetProcessTime(); }

// sceSaveDataInitialize3 is cheap but wants calling once. The user service
// returns 0x80960003 ("already initialized" - SDL or the system got there
// first), which is benign and deliberately not treated as a failure.
bool EnsureInitialized() {
    static bool done = false;
    if (done) return true;
    sceUserServiceInitialize(nullptr);
    sceSaveDataInitialize3(0);
    done = true;
    return true;
}

// --- plain file helpers ----------------------------------------------------

// mkdir every component of `path`. mkdir does not create parents, and nothing
// below /data is created for us.
void MakeDirs(const std::string& path) {
    size_t start = 1; // skip the leading '/'
    for (;;) {
        size_t slash = path.find('/', start);
        if (slash == std::string::npos) break;
        sceKernelMkdir(path.substr(0, slash).c_str(), 0777);
        start = slash + 1;
    }
    sceKernelMkdir(path.c_str(), 0777);
}

// mkdir the directory components of `rel` underneath `root`, but not `rel`
// itself - `rel` is a file.
void MakeParentDirs(const std::string& root, const std::string& rel) {
    size_t start = 0;
    for (;;) {
        size_t slash = rel.find('/', start);
        if (slash == std::string::npos) break;
        sceKernelMkdir((root + "/" + rel.substr(0, slash)).c_str(), 0777);
        start = slash + 1;
    }
}

// Copies one file, reading through in chunks to a short read. NOT by st_size,
// which returned 88 for a genuine 44 KB file on this kernel. A short write is
// a failure rather than something to retry blindly - a copy that silently
// truncates is worse than one that says it could not finish.
bool CopyFileThrough(const std::string& src, const std::string& dst,
                     uint64_t* outBytes, std::string& error) {
    *outBytes = 0;

    int in = sceKernelOpen(src.c_str(), kBsdRdonly, 0);
    if (in < 0) { error = "OPEN FAILED " + Hex(in) + " " + src; return false; }

    int out = sceKernelOpen(dst.c_str(), kBsdWronly | kBsdCreat | kBsdTrunc, 0777);
    if (out < 0) {
        sceKernelClose(in);
        error = "DEST OPEN REFUSED " + Hex(out) + " " + dst;
        return false;
    }

    char* buf = (char*)malloc(kChunk);
    if (!buf) {
        sceKernelClose(in);
        sceKernelClose(out);
        error = "OUT OF MEMORY";
        return false;
    }

    bool ok = true;
    for (;;) {
        int n = sceKernelRead(in, buf, kChunk);
        if (n < 0) { ok = false; error = "READ FAILED " + Hex(n) + " " + src; break; }
        if (n > 0) {
            int w = sceKernelWrite(out, buf, n);
            if (w != n) {
                ok = false;
                error = "SHORT WRITE " + U64Text((uint64_t)(w < 0 ? 0 : w)) +
                        " OF " + U64Text((uint64_t)n) + " " + dst;
                break;
            }
            *outBytes += (uint64_t)n;
        }
        if ((size_t)n < kChunk) break;
    }

    free(buf);
    sceKernelClose(in);
    sceKernelClose(out);
    return ok;
}

bool ReadWholeFile(const std::string& path, std::string& out, size_t cap) {
    out.clear();
    int fd = sceKernelOpen(path.c_str(), kBsdRdonly, 0);
    if (fd < 0) return false;

    char* buf = (char*)malloc(kChunk);
    if (!buf) { sceKernelClose(fd); return false; }

    bool ok = true;
    for (;;) {
        int n = sceKernelRead(fd, buf, kChunk);
        if (n < 0) { ok = false; break; }
        if (n > 0) {
            if (out.size() + (size_t)n > cap) { ok = false; break; }
            out.append(buf, (size_t)n);
        }
        if ((size_t)n < kChunk) break;
    }

    free(buf);
    sceKernelClose(fd);
    return ok;
}

bool WriteWholeFile(const std::string& path, const std::string& text) {
    int fd = sceKernelOpen(path.c_str(), kBsdWronly | kBsdCreat | kBsdTrunc, 0777);
    if (fd < 0) return false;

    bool ok = true;
    size_t done = 0;
    while (done < text.size()) {
        size_t want = text.size() - done;
        if (want > kChunk) want = kChunk;
        int w = sceKernelWrite(fd, text.data() + done, want);
        if (w != (int)want) { ok = false; break; }
        done += want;
    }
    sceKernelClose(fd);
    return ok;
}

// --- directory walks -------------------------------------------------------

struct WalkedName {
    std::string rel;
    bool        isDir = false;
};

// Names only, no reading. Used where the caller is about to read every file
// anyway (a copy) and a sizing pass would double the work.
bool WalkNames(const std::string& root, const std::string& rel,
               std::vector<WalkedName>& out, int depth) {
    if (depth > kMaxDepth) return false;

    std::string path = rel.empty() ? root : root + "/" + rel;
    int dfd = sceKernelOpen(path.c_str(), kBsdRdonly | kBsdDirectory, 0777);
    if (dfd < 0) return false;

    char* buf = (char*)malloc(64 * 1024);
    if (!buf) { sceKernelClose(dfd); return false; }

    // Names are collected before recursing: reusing one directory fd while
    // opening others underneath it is asking for trouble.
    std::vector<std::string> subdirs;
    bool ok = true;

    for (;;) {
        int nread = sceKernelGetdents(dfd, buf, 64 * 1024);
        if (nread <= 0) break;

        int pos = 0;
        while (pos < nread) {
            BsdDirent* de = (BsdDirent*)(buf + pos);
            if (de->d_reclen == 0) break;

            std::string name(de->d_name, de->d_namlen);
            if (name != "." && name != "..") {
                WalkedName entry;
                entry.rel   = rel.empty() ? name : rel + "/" + name;
                entry.isDir = (de->d_type == kDtDir);
                out.push_back(entry);
                if (entry.isDir) subdirs.push_back(entry.rel);
            }
            pos += de->d_reclen;
        }
    }

    free(buf);
    sceKernelClose(dfd);

    for (size_t i = 0; i < subdirs.size(); i++) {
        if (!WalkNames(root, subdirs[i], out, depth + 1)) ok = false;
    }
    return ok;
}

// Names AND sizes, sizing by reading each file through to a short read. That
// also proves the whole file is readable, which is what makes this usable as
// the bracketing read.
void WalkSized(const std::string& root, const std::string& rel,
               std::vector<FileEntry>& out, int& errors, int depth) {
    if (depth > kMaxDepth) { errors++; return; }

    std::string path = rel.empty() ? root : root + "/" + rel;
    int dfd = sceKernelOpen(path.c_str(), kBsdRdonly | kBsdDirectory, 0777);
    if (dfd < 0) { errors++; return; }

    char* buf = (char*)malloc(64 * 1024);
    if (!buf) { sceKernelClose(dfd); errors++; return; }

    std::vector<std::string> subdirs;

    for (;;) {
        int nread = sceKernelGetdents(dfd, buf, 64 * 1024);
        if (nread <= 0) break;

        int pos = 0;
        while (pos < nread) {
            BsdDirent* de = (BsdDirent*)(buf + pos);
            if (de->d_reclen == 0) break;

            std::string name(de->d_name, de->d_namlen);
            if (name != "." && name != "..") {
                std::string childRel = rel.empty() ? name : rel + "/" + name;
                if (de->d_type == kDtDir) {
                    subdirs.push_back(childRel);
                } else {
                    int ffd = sceKernelOpen((root + "/" + childRel).c_str(),
                                            kBsdRdonly, 0);
                    if (ffd < 0) {
                        errors++;
                    } else {
                        char*    chunk = (char*)malloc(kChunk);
                        uint64_t got   = 0;
                        if (!chunk) {
                            errors++;
                        } else {
                            for (;;) {
                                int n = sceKernelRead(ffd, chunk, kChunk);
                                if (n < 0) { errors++; break; }
                                got += (uint64_t)n;
                                if ((size_t)n < kChunk) break;
                            }
                            free(chunk);
                        }
                        sceKernelClose(ffd);

                        FileEntry entry;
                        entry.rel   = childRel;
                        entry.bytes = got;
                        out.push_back(entry);
                    }
                }
            }
            pos += de->d_reclen;
        }
    }

    free(buf);
    sceKernelClose(dfd);

    for (size_t i = 0; i < subdirs.size(); i++) {
        WalkSized(root, subdirs[i], out, errors, depth + 1);
    }
}

bool ByRel(const FileEntry& a, const FileEntry& b) { return a.rel < b.rel; }

// Removes an ordinary directory tree under /data. Only ever aimed at this
// app's own ".partial" directories - never at a save mount, which is emptied
// through UnlinkGameSaveFiles below and never has its container destroyed.
void RemoveTree(const std::string& path) {
    std::vector<WalkedName> names;
    WalkNames(path, "", names, 0);

    // Files first, then directories deepest-first: rmdir needs an empty one.
    for (size_t i = 0; i < names.size(); i++) {
        if (!names[i].isDir) sceKernelUnlink((path + "/" + names[i].rel).c_str());
    }
    std::vector<std::string> dirs;
    for (size_t i = 0; i < names.size(); i++) {
        if (names[i].isDir) dirs.push_back(names[i].rel);
    }
    std::sort(dirs.begin(), dirs.end());
    for (size_t i = dirs.size(); i > 0; i--) {
        sceKernelRmdir((path + "/" + dirs[i - 1]).c_str());
    }
    sceKernelRmdir(path.c_str());
}

// --- mounting --------------------------------------------------------------

struct Mount {
    bool        ok = false;
    int         rc = 0;
    std::string path;
    uint64_t    blocks = 0;
    char        raw[16] = {}; // the mount point exactly as returned, for unmount
};

// sceSaveDataMount, NOT sceSaveDataMount2: Mount2 has no titleId field, so it
// can only mount the CALLING app's own save data. Reaching Bloodborne's saves
// from here means naming Bloodborne.
//
// CREATE2 is never passed. This app cannot bring a container into existence -
// that needs a PFS image and a sealedkey under /user/home plus a registration
// in /system_data's savedata.db, neither of which exists in this sandbox - and
// the one attempt at it is implicated in corrupting a live save.
Mount MountContainer(const User& user, const std::string& titleId,
                     const std::string& dirName, bool readWrite) {
    Mount m;
    memset(m.raw, 0, sizeof(m.raw));
    if (!EnsureInitialized()) return m;

    OrbisSaveDataMount req;
    memset(&req, 0, sizeof(req));
    req.userId      = user.userId;
    req.titleId     = titleId.c_str();
    req.dirName     = dirName.c_str();
    req.fingerprint = nullptr;   // only needed for cross-console transfer
    req.blocks      = 0;
    req.mountMode   = readWrite ? ORBIS_SAVE_DATA_MOUNT_MODE_RDWR
                                : ORBIS_SAVE_DATA_MOUNT_MODE_RDONLY;

    OrbisSaveDataMountResult res;
    memset(&res, 0, sizeof(res));

    m.rc = sceSaveDataMount(&req, &res);
    if (m.rc < 0) return m;

    memcpy(m.raw, res.mountPathName, sizeof(m.raw));
    m.path = Bounded(res.mountPathName, sizeof(res.mountPathName));

    OrbisSaveDataMountPoint point;
    memset(&point, 0, sizeof(point));
    memcpy(point.data, res.mountPathName, sizeof(point.data));

    OrbisSaveDataMountInfo info;
    memset(&info, 0, sizeof(info));
    sceSaveDataGetMountInfo(&point, &info);
    m.blocks = info.blocks;

    m.ok = true;
    return m;
}

int Unmount(const Mount& m) {
    OrbisSaveDataMountPoint point;
    memset(&point, 0, sizeof(point));
    memcpy(point.data, m.raw, sizeof(point.data));
    return sceSaveDataUmount(&point);
}

// findings 8.3, the shared half of START FRESH and of a restore's mandatory
// empty. Root-level userdata* and backup* only: a '/' in the relative path
// means it is inside sce_sys, which is never touched.
void UnlinkGameSaveFiles(const std::string& mountPath,
                         const std::vector<FileEntry>& contents,
                         int& removed, int& refused, int& kept) {
    removed = refused = kept = 0;
    for (size_t i = 0; i < contents.size(); i++) {
        const std::string& rel = contents[i].rel;
        if (!IsGameSaveFile(rel)) { kept++; continue; }

        int rc = sceKernelUnlink((mountPath + "/" + rel).c_str());
        if (rc < 0) refused++;
        else        removed++;
    }
}

bool StartsWith(const std::string& s, const char* prefix) {
    size_t n = strlen(prefix);
    return s.size() >= n && s.compare(0, n, prefix) == 0;
}

bool UnderSceSys(const std::string& rel) { return StartsWith(rel, "sce_sys/"); }

} // namespace

// ---------------------------------------------------------------------------
// Identity
// ---------------------------------------------------------------------------

User ResolveUser() {
    User u;
    EnsureInitialized();

    int32_t userId = ORBIS_USER_SERVICE_USER_ID_INVALID;
    int rc = sceUserServiceGetForegroundUser(&userId);
    if (rc < 0 || userId == ORBIS_USER_SERVICE_USER_ID_INVALID) {
        u.error = "NO SIGNED-IN PLAYER - GETFOREGROUNDUSER " + Hex(rc);
        return u;
    }
    u.userId = userId;

    uint64_t accountId = 0;
    rc = sceUserServiceGetNpAccountId(userId, &accountId);
    if (rc < 0 || accountId == 0) {
        u.error = "NO ACCOUNT ID FOR THE SIGNED-IN PLAYER - " + Hex(rc);
        return u;
    }
    u.accountId = accountId;

    char nameBuf[64];
    memset(nameBuf, 0, sizeof(nameBuf));
    if (sceUserServiceGetUserName(userId, nameBuf, sizeof(nameBuf)) >= 0) {
        u.userName = Bounded(nameBuf, sizeof(nameBuf));
    }

    u.valid = true;
    return u;
}

// ---------------------------------------------------------------------------
// Discovery
// ---------------------------------------------------------------------------

SearchResult FindSaveDirs(const User& user, const std::string& titleId) {
    SearchResult out;
    if (!user.valid) { out.error = "NO SIGNED-IN PLAYER"; return out; }
    EnsureInitialized();

    OrbisSaveDataTitleId searchTitle;
    memset(&searchTitle, 0, sizeof(searchTitle));
    strncpy(searchTitle.data, titleId.c_str(), sizeof(searchTitle.data) - 1);

    // cond.key and cond.order are left at their defaults, so the order of the
    // result is unspecified. Nothing here or above reads meaning from a
    // position: a directory is selected by name, or by there being one.
    OrbisSaveDataDirNameSearchCond cond;
    memset(&cond, 0, sizeof(cond));
    cond.userId  = user.userId;
    cond.titleId = &searchTitle;

    // params and infos are 1280 and 48 bytes per slot - more than belongs on a
    // UI thread's stack - and nothing here needs them, so only the names are
    // asked for.
    std::vector<OrbisSaveDataDirName> dirNames(kMaxDirs);
    memset(dirNames.data(), 0, kMaxDirs * sizeof(OrbisSaveDataDirName));

    OrbisSaveDataDirNameSearchResult res;
    memset(&res, 0, sizeof(res));
    res.dirNames    = dirNames.data();
    res.dirNamesNum = kMaxDirs;

    out.rc = sceSaveDataDirNameSearch(&cond, &res);
    if (out.rc < 0) {
        out.error = "SEARCH FAILED " + Hex(out.rc) + " FOR " + titleId;
        return out;
    }

    unsigned shown = res.hitNum < kMaxDirs ? res.hitNum : kMaxDirs;
    for (unsigned i = 0; i < shown; i++) {
        out.dirNames.push_back(Bounded(dirNames[i].data, sizeof(dirNames[i].data)));
    }
    std::sort(out.dirNames.begin(), out.dirNames.end());
    out.ok = true;
    return out;
}

SaveTitle DiscoverSaveTitle(const User& user) {
    SaveTitle out;
    if (!user.valid) { out.error = "NO SIGNED-IN PLAYER"; return out; }

    for (int i = 0; i < kCandidateCount; i++) {
        SearchResult r = FindSaveDirs(user, kCandidates[i].id);

        TitleSweepEntry entry;
        entry.titleId = kCandidates[i].id;
        entry.label   = kCandidates[i].label;
        entry.hits    = (int)r.dirNames.size();
        entry.rc      = r.rc;
        out.swept.push_back(entry);

        if (!r.ok || r.dirNames.empty()) continue;

        out.titlesWithHits++;
        if (out.titlesWithHits == 1) {
            out.titleId  = kCandidates[i].id;
            out.dirNames = r.dirNames;
        }
    }

    if (out.titlesWithHits == 0) {
        out.error = "NO BLOODBORNE SAVE DATA FOR THIS PLAYER";
        out.titleId.clear();
        out.dirNames.clear();
        return out;
    }
    if (out.titlesWithHits > 1) {
        out.error = "SAVE DATA UNDER " + U64Text((uint64_t)out.titlesWithHits) +
                    " BLOODBORNE TITLES - CANNOT CHOOSE ONE";
        out.titleId.clear();
        out.dirNames.clear();
        return out;
    }

    out.found = true;
    return out;
}

// ---------------------------------------------------------------------------
// Reading a container
// ---------------------------------------------------------------------------

bool IsGameSaveFile(const std::string& rel) {
    if (rel.find('/') != std::string::npos) return false;   // not at the root
    return StartsWith(rel, "userdata") || StartsWith(rel, "backup");
}

Container ReadContainer(const User& user, const std::string& titleId,
                        const std::string& dirName) {
    Container c;
    if (!user.valid) { c.error = "NO SIGNED-IN PLAYER"; return c; }

    Mount m = MountContainer(user, titleId, dirName, false);
    c.rc = m.rc;
    if (!m.ok) {
        c.error = "NO CONTAINER FOR " + titleId + " / " + dirName +
                  " - MOUNT " + Hex(m.rc);
        return c;
    }

    WalkSized(m.path, "", c.files, c.walkErrors, 0);
    std::sort(c.files.begin(), c.files.end(), ByRel);
    for (size_t i = 0; i < c.files.size(); i++) c.bytes += c.files[i].bytes;
    c.blocks = m.blocks;
    c.exists = true;

    int un = Unmount(m);
    if (un < 0) c.error = "UNMOUNT " + Hex(un);

    return c;
}

// ---------------------------------------------------------------------------
// Emptying a container
// ---------------------------------------------------------------------------

EmptyResult EmptyContainer(const User& user, const std::string& titleId,
                           const std::string& dirName) {
    EmptyResult out;

    // Bracket, before. Also the file list the unlink pass works from, so the
    // operation and the reading of it cannot disagree about what was there.
    out.before = ReadContainer(user, titleId, dirName);
    if (!out.before.exists) {
        out.error = out.before.error;
        return out;
    }

    Mount m = MountContainer(user, titleId, dirName, true);
    if (!m.ok) {
        out.error = "RDWR MOUNT REFUSED " + Hex(m.rc);
        return out;
    }

    UnlinkGameSaveFiles(m.path, out.before.files, out.removed, out.refused, out.kept);

    // The unmount is what re-seals the container. A failure here is reported
    // rather than swallowed: the container has been written to.
    int un = Unmount(m);

    // Bracket, after.
    out.after = ReadContainer(user, titleId, dirName);

    if (un < 0) {
        out.error = "UNMOUNT FAILED " + Hex(un) + " - DO NOT LAUNCH THE GAME YET";
        return out;
    }
    if (out.refused > 0) {
        out.error = U64Text((uint64_t)out.refused) + " FILE(S) REFUSED UNLINK";
        return out;
    }
    if (!out.after.exists) {
        out.error = "COULD NOT READ THE CONTAINER BACK";
        return out;
    }
    for (size_t i = 0; i < out.after.files.size(); i++) {
        if (IsGameSaveFile(out.after.files[i].rel)) {
            out.error = "GAME SAVE FILES REMAIN AFTER EMPTYING";
            return out;
        }
    }

    out.ok = true;
    return out;
}

// ---------------------------------------------------------------------------
// Manifests
// ---------------------------------------------------------------------------

std::string FormatManifest(const Manifest& m) {
    std::string out;
    out += "title_id=" + m.title_id + "\n";
    out += "dir_name=" + m.dir_name + "\n";
    out += "account_id=" + U64Text(m.account_id) + "\n";
    out += "blocks=" + U64Text(m.blocks) + "\n";
    out += "files=" + U64Text(m.files) + "\n";
    out += "bytes=" + U64Text(m.bytes) + "\n";
    out += "captured=" + m.captured + "\n";
    out += "world=" + m.world + "\n";
    out += "revision=" + m.revision + "\n";

    // One line per file, relative path then size. The size is read back from
    // the LAST space so that a path containing a space - none does today -
    // would survive a round trip rather than silently reparse wrong.
    for (size_t i = 0; i < m.entries.size(); i++) {
        out += "f " + m.entries[i].rel + " " + U64Text(m.entries[i].bytes) + "\n";
    }
    return out;
}

bool ParseManifest(const std::string& text, Manifest& out, std::string& error) {
    out = Manifest();
    error.clear();

    size_t pos = 0;
    bool sawTitle = false;
    while (pos <= text.size()) {
        size_t nl = text.find('\n', pos);
        std::string line = text.substr(pos, nl == std::string::npos
                                                ? std::string::npos : nl - pos);
        if (!line.empty() && line[line.size() - 1] == '\r') line.resize(line.size() - 1);

        if (!line.empty()) {
            if (line[0] == 'f' && line.size() > 2 && line[1] == ' ') {
                std::string rest = line.substr(2);
                size_t sp = rest.rfind(' ');
                if (sp == std::string::npos || sp == 0) {
                    error = "MALFORMED FILE LINE";
                    return false;
                }
                FileEntry e;
                e.rel   = rest.substr(0, sp);
                e.bytes = strtoull(rest.c_str() + sp + 1, nullptr, 10);
                out.entries.push_back(e);
            } else {
                size_t eq = line.find('=');
                if (eq != std::string::npos) {
                    std::string key = line.substr(0, eq);
                    std::string val = line.substr(eq + 1);
                    if      (key == "title_id")   { out.title_id = val; sawTitle = true; }
                    else if (key == "dir_name")   out.dir_name = val;
                    else if (key == "account_id") out.account_id = strtoull(val.c_str(), nullptr, 10);
                    else if (key == "blocks")     out.blocks = strtoull(val.c_str(), nullptr, 10);
                    else if (key == "files")      out.files  = strtoull(val.c_str(), nullptr, 10);
                    else if (key == "bytes")      out.bytes  = strtoull(val.c_str(), nullptr, 10);
                    else if (key == "captured")   out.captured = val;
                    else if (key == "world")      out.world = val;
                    else if (key == "revision")   out.revision = val;
                    // Unknown keys are ignored, the same tolerance
                    // defaults.cfg has, so a manifest written by a later
                    // version still reads here.
                }
            }
        }

        if (nl == std::string::npos) break;
        pos = nl + 1;
    }

    if (!sawTitle) { error = "NO title_id - NOT A MANIFEST"; return false; }

    std::sort(out.entries.begin(), out.entries.end(), ByRel);

    // The header's own counts must agree with the lines beneath it. A manifest
    // that disagrees with itself is not evidence of anything.
    uint64_t total = 0;
    for (size_t i = 0; i < out.entries.size(); i++) total += out.entries[i].bytes;
    if (out.files != (uint64_t)out.entries.size()) {
        error = "MANIFEST SAYS " + U64Text(out.files) + " FILE(S) BUT LISTS " +
                U64Text((uint64_t)out.entries.size());
        return false;
    }
    if (out.bytes != total) {
        error = "MANIFEST SAYS " + U64Text(out.bytes) + " BYTES BUT LISTS " +
                U64Text(total);
        return false;
    }
    return true;
}

bool WriteManifestFile(const std::string& path, const Manifest& m) {
    return WriteWholeFile(path, FormatManifest(m));
}

bool ReadManifestFile(const std::string& path, Manifest& out, std::string& error) {
    std::string text;
    if (!ReadWholeFile(path, text, 1024 * 1024)) {
        error = "COULD NOT READ " + path;
        return false;
    }
    return ParseManifest(text, out, error);
}

bool VerifyBackup(const std::string& backupDir, std::string& error,
                  Manifest* outManifest) {
    error.clear();

    Manifest m;
    if (!ReadManifestFile(backupDir + "/manifest.txt", m, error)) return false;
    if (outManifest) *outManifest = m;

    std::vector<FileEntry> found;
    int walkErrors = 0;
    WalkSized(backupDir + "/data", "", found, walkErrors, 0);
    std::sort(found.begin(), found.end(), ByRel);

    if (walkErrors > 0) {
        error = U64Text((uint64_t)walkErrors) + " FILE(S) COULD NOT BE READ";
        return false;
    }
    if (found.size() != m.entries.size()) {
        error = "COUNT - MANIFEST " + U64Text((uint64_t)m.entries.size()) +
                " FILE(S), FOUND " + U64Text((uint64_t)found.size());
        return false;
    }

    uint64_t total = 0;
    for (size_t i = 0; i < found.size(); i++) {
        if (found[i].rel != m.entries[i].rel) {
            error = "PATH - EXPECTED " + m.entries[i].rel + ", FOUND " + found[i].rel;
            return false;
        }
        if (found[i].bytes != m.entries[i].bytes) {
            error = "SIZE - " + found[i].rel + " IS " + U64Text(found[i].bytes) +
                    " BYTES, MANIFEST SAYS " + U64Text(m.entries[i].bytes);
            return false;
        }
        total += found[i].bytes;
    }
    if (total != m.bytes) {
        error = "TOTAL - FOUND " + U64Text(total) + " BYTES, MANIFEST SAYS " +
                U64Text(m.bytes);
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// BackupJob
// ---------------------------------------------------------------------------

namespace {

std::string NowStamp() {
    time_t now = time(nullptr);
    struct tm* lt = localtime(&now);
    char buf[32];
    if (!lt || strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", lt) == 0) {
        return "(UNKNOWN)";
    }
    return std::string(buf);
}

} // namespace

struct BackupJob::State {
    User        user;
    std::string titleId;
    std::string dirName;
    std::string destDir;
    std::string partialDir;
    std::string world;
    std::string revision;

    Mount       mount;
    bool        mounted = false;

    std::vector<std::string> files;   // relative paths still to copy
    size_t      next  = 0;
    int         phase = 0;            // 0 prepare, 1 copying, 2 finish, 3 done
    uint64_t    startUs = 0;
    std::string status = "STARTING";
    BackupResult result;

    void Fail(const std::string& why) {
        result.ok    = false;
        result.error = why;
        status       = why;
        Log(("savedata: backup failed - " + why).c_str());
        if (mounted) { Unmount(mount); mounted = false; }
        phase = 3;
    }
};

BackupJob::BackupJob(const User& user, const std::string& titleId,
                     const std::string& dirName, const std::string& destDir,
                     const std::string& world, const std::string& revision)
    : s_(new State()) {
    s_->user       = user;
    s_->titleId    = titleId;
    s_->dirName    = dirName;
    s_->destDir    = destDir;
    s_->partialDir = destDir + ".partial";
    s_->world      = world;
    s_->revision   = revision;
    s_->startUs    = NowUs();
}

BackupJob::~BackupJob() {
    // A job torn down mid-run must not leave a mount behind.
    if (s_ && s_->mounted) Unmount(s_->mount);
}

bool BackupJob::Done() const { return s_->phase >= 3; }
std::string BackupJob::StatusText() const { return s_->status; }
const BackupResult& BackupJob::Result() const { return s_->result; }

float BackupJob::Progress() const {
    if (s_->phase >= 3) return 1.0f;
    if (s_->files.empty()) return 0.0f;
    return (float)s_->next / (float)(s_->files.size() + 1);
}

void BackupJob::Step() {
    State& s = *s_;
    if (s.phase >= 3) return;

    if (s.phase == 0) {
        if (!s.user.valid) { s.Fail("NO SIGNED-IN PLAYER"); return; }

        // A .partial left by an earlier run is not a starting point - it is
        // debris, and copying into it would mix two containers together.
        RemoveTree(s.partialDir);
        MakeDirs(s.partialDir + "/data");

        s.mount = MountContainer(s.user, s.titleId, s.dirName, false);
        if (!s.mount.ok) {
            s.Fail("MOUNT RDONLY FAILED " + Hex(s.mount.rc));
            return;
        }
        s.mounted = true;
        s.result.manifest.blocks = s.mount.blocks;

        std::vector<WalkedName> names;
        WalkNames(s.mount.path, "", names, 0);
        for (size_t i = 0; i < names.size(); i++) {
            if (names[i].isDir) {
                sceKernelMkdir((s.partialDir + "/data/" + names[i].rel).c_str(), 0777);
            } else {
                s.files.push_back(names[i].rel);
            }
        }
        std::sort(s.files.begin(), s.files.end());

        if (s.files.empty()) {
            s.Fail("THE CONTAINER HOLDS NO FILES");
            return;
        }

        s.status = "READ CONTAINER - " + U64Text((uint64_t)s.files.size()) + " FILE(S)";
        s.phase  = 1;
        return;
    }

    if (s.phase == 1) {
        const std::string& rel = s.files[s.next];
        uint64_t bytes = 0;
        std::string err;
        if (!CopyFileThrough(s.mount.path + "/" + rel,
                             s.partialDir + "/data/" + rel, &bytes, err)) {
            s.Fail(rel + " - " + err);
            return;
        }

        FileEntry e;
        e.rel   = rel;
        e.bytes = bytes;
        s.result.manifest.entries.push_back(e);
        s.result.files++;
        s.result.bytes += bytes;

        s.status = rel + " - " + U64Text(bytes) + " BYTES COPIED";
        s.next++;
        if (s.next >= s.files.size()) s.phase = 2;
        return;
    }

    // phase 2: unmount, write the manifest LAST, then rename into place.
    int un = Unmount(s.mount);
    s.mounted = false;
    if (un < 0) {
        s.Fail("UNMOUNT FAILED " + Hex(un));
        return;
    }

    Manifest& m = s.result.manifest;
    m.title_id   = s.titleId;
    m.dir_name   = s.dirName;
    m.account_id = s.user.accountId;
    m.files      = s.result.files;
    m.bytes      = s.result.bytes;
    m.captured   = NowStamp();
    m.world      = s.world;
    m.revision   = s.revision;
    std::sort(m.entries.begin(), m.entries.end(), ByRel);

    if (!WriteManifestFile(s.partialDir + "/manifest.txt", m)) {
        s.Fail("COULD NOT WRITE THE MANIFEST");
        return;
    }

    int rc = sceKernelRename(s.partialDir.c_str(), s.destDir.c_str());
    if (rc < 0) {
        s.Fail("RENAME INTO PLACE FAILED " + Hex(rc));
        return;
    }

    s.result.ok        = true;
    s.result.path      = s.destDir;
    s.result.elapsedMs = (NowUs() - s.startUs) / 1000;
    s.status = "BACKUP COMPLETE - " + U64Text(s.result.files) + " FILE(S), " +
               U64Text(s.result.bytes) + " BYTES";
    s.phase = 3;
}

// ---------------------------------------------------------------------------
// RestoreJob
// ---------------------------------------------------------------------------

struct RestoreJob::State {
    User        user;
    std::string titleId;
    std::string dirName;
    std::string srcDir;

    Manifest    manifest;
    Mount       mount;
    bool        mounted = false;

    std::vector<FileEntry> toWrite;   // the manifest minus sce_sys
    size_t      next  = 0;
    int         phase = 0;  // 0 verify source, 1 read+refuse, 2 empty,
                            // 3 writing, 4 unmount, 5 confirm, 6 done
    uint64_t    startUs = 0;
    std::string status = "STARTING";
    RestoreResult result;

    void Fail(const std::string& why) {
        result.ok    = false;
        result.error = why;
        status       = why;
        Log(("savedata: restore failed - " + why).c_str());
        if (mounted) { Unmount(mount); mounted = false; }
        phase = 6;
    }
};

RestoreJob::RestoreJob(const User& user, const std::string& titleId,
                       const std::string& dirName, const std::string& srcDir)
    : s_(new State()) {
    s_->user    = user;
    s_->titleId = titleId;
    s_->dirName = dirName;
    s_->srcDir  = srcDir;
    s_->startUs = NowUs();
}

RestoreJob::~RestoreJob() {
    if (s_ && s_->mounted) Unmount(s_->mount);
}

bool RestoreJob::Done() const { return s_->phase >= 6; }
std::string RestoreJob::StatusText() const { return s_->status; }
const RestoreResult& RestoreJob::Result() const { return s_->result; }

float RestoreJob::Progress() const {
    if (s_->phase >= 6) return 1.0f;
    if (s_->toWrite.empty()) return 0.0f;
    return (float)s_->next / (float)(s_->toWrite.size() + 1);
}

void RestoreJob::Step() {
    State& s = *s_;
    if (s.phase >= 6) return;

    // --- phase 0: the source must verify before anything else happens ------
    if (s.phase == 0) {
        if (!s.user.valid) { s.Fail("NO SIGNED-IN PLAYER"); return; }

        std::string err;
        if (!VerifyBackup(s.srcDir, err, &s.manifest)) {
            s.Fail("SOURCE FAILS VERIFICATION - " + err);
            return;
        }
        s.status = "SOURCE VERIFIED - " + U64Text(s.manifest.files) + " FILE(S)";
        s.phase  = 1;
        return;
    }

    // --- phase 1: every refusal, before a byte is written ------------------
    if (s.phase == 1) {
        s.result.before = ReadContainer(s.user, s.titleId, s.dirName);
        if (!s.result.before.exists) {
            s.Fail("NO SAVE CONTAINER - RUN BLOODBORNE ONCE FIRST");
            return;
        }
        if (s.manifest.title_id != s.titleId) {
            s.Fail("SAVE IS FOR TITLE " + s.manifest.title_id + ", NOT " + s.titleId);
            return;
        }
        if (s.manifest.dir_name != s.dirName) {
            s.Fail("SAVE IS FOR DIRECTORY " + s.manifest.dir_name + ", NOT " + s.dirName);
            return;
        }
        if (s.manifest.account_id != 0 && s.manifest.account_id != s.user.accountId) {
            s.Fail("SAVE BELONGS TO ANOTHER ACCOUNT");
            return;
        }
        // SAVEDATA_BLOCKS lives in the unwritable param.sfo, so the container's
        // size is fixed and is a fact to check against rather than to set.
        if (s.manifest.blocks > s.result.before.blocks) {
            s.Fail("SAVE NEEDS " + U64Text(s.manifest.blocks) +
                   " BLOCKS, CONTAINER HAS " + U64Text(s.result.before.blocks));
            return;
        }

        for (size_t i = 0; i < s.manifest.entries.size(); i++) {
            if (UnderSceSys(s.manifest.entries[i].rel)) { s.result.skipped++; continue; }
            s.toWrite.push_back(s.manifest.entries[i]);
        }
        if (s.toWrite.empty()) {
            s.Fail("THE SAVE HOLDS NO FILES TO RESTORE");
            return;
        }

        s.status = "CONTAINER READ - " + U64Text((uint64_t)s.result.before.files.size()) +
                   " FILE(S), " + U64Text(s.result.before.bytes) + " BYTES";
        s.phase  = 2;
        return;
    }

    // --- phase 2: mount, and empty before writing --------------------------
    if (s.phase == 2) {
        s.mount = MountContainer(s.user, s.titleId, s.dirName, true);
        if (!s.mount.ok) {
            s.Fail("RDWR MOUNT REFUSED " + Hex(s.mount.rc));
            return;
        }
        s.mounted = true;

        int removed = 0, refused = 0, kept = 0;
        UnlinkGameSaveFiles(s.mount.path, s.result.before.files, removed, refused, kept);
        s.result.removed = (uint64_t)removed;
        if (refused > 0) {
            // Leaving another playthrough's surplus backup* files in place is
            // exactly the mixing this empty exists to prevent.
            s.Fail(U64Text((uint64_t)refused) + " FILE(S) REFUSED UNLINK - NOT WRITING");
            return;
        }

        s.status = "EMPTIED - " + U64Text((uint64_t)removed) + " FILE(S) REMOVED, " +
                   U64Text((uint64_t)kept) + " LEFT ALONE";
        s.phase  = 3;
        return;
    }

    // --- phase 3: one file per step, sce_sys never among them --------------
    if (s.phase == 3) {
        const FileEntry& e = s.toWrite[s.next];
        MakeParentDirs(s.mount.path, e.rel);

        uint64_t bytes = 0;
        std::string err;
        if (!CopyFileThrough(s.srcDir + "/data/" + e.rel,
                             s.mount.path + "/" + e.rel, &bytes, err)) {
            s.Fail(e.rel + " - " + err);
            return;
        }
        if (bytes != e.bytes) {
            s.Fail(e.rel + " - WROTE " + U64Text(bytes) + " OF " +
                   U64Text(e.bytes) + " BYTES");
            return;
        }

        s.result.files++;
        s.result.bytes += bytes;
        s.status = e.rel + " - " + U64Text(bytes) + " BYTES WRITTEN";

        s.next++;
        if (s.next >= s.toWrite.size()) s.phase = 4;
        return;
    }

    // --- phase 4: the unmount is what re-seals the container ---------------
    if (s.phase == 4) {
        int un = Unmount(s.mount);
        s.mounted = false;
        if (un < 0) {
            s.Fail("UNMOUNT FAILED " + Hex(un) + " - DO NOT LAUNCH THE GAME YET");
            return;
        }
        s.status = "UNMOUNTED";
        s.phase  = 5;
        return;
    }

    // --- phase 5: bracket, after - and confirm what was written ------------
    s.result.after = ReadContainer(s.user, s.titleId, s.dirName);
    if (!s.result.after.exists) {
        s.Fail("COULD NOT READ THE CONTAINER BACK");
        return;
    }

    // The WRITTEN set is what is checked: sce_sys was never written, so it is
    // reported rather than asserted against a manifest that may predate a
    // system-side change to it.
    std::vector<FileEntry> got;
    for (size_t i = 0; i < s.result.after.files.size(); i++) {
        if (!UnderSceSys(s.result.after.files[i].rel)) got.push_back(s.result.after.files[i]);
    }
    std::sort(got.begin(), got.end(), ByRel);

    if (got.size() != s.toWrite.size()) {
        s.Fail("AFTER - EXPECTED " + U64Text((uint64_t)s.toWrite.size()) +
               " FILE(S) OUTSIDE SCE_SYS, FOUND " + U64Text((uint64_t)got.size()));
        return;
    }
    for (size_t i = 0; i < got.size(); i++) {
        if (got[i].rel != s.toWrite[i].rel || got[i].bytes != s.toWrite[i].bytes) {
            s.Fail("AFTER - " + got[i].rel + " DOES NOT MATCH THE MANIFEST");
            return;
        }
    }

    s.result.ok        = true;
    s.result.elapsedMs = (NowUs() - s.startUs) / 1000;
    s.status = "RESTORE COMPLETE - " + U64Text(s.result.files) + " FILE(S), " +
               U64Text(s.result.bytes) + " BYTES";
    s.phase = 6;
}

} // namespace savedata
} // namespace bbr
