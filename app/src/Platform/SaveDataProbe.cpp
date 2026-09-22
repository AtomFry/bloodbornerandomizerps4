#include "SaveDataProbe.h"

#include "Log.h"

#include <orbis/SaveData.h>
#include <orbis/UserService.h>
#include <orbis/libkernel.h>

#include <sys/statvfs.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <functional>
#include <vector>

namespace bbr {

namespace {

// Same BSD values FileIo.cpp proved on this kernel. Duplicated rather than
// shared because this file is temporary and FileIo's copies are file-local; a
// probe that drags a refactor in with it is a probe that is hard to delete.
const int kBsdRdonly    = 0x0000;
const int kBsdWronly    = 0x0001;
const int kBsdCreat     = 0x0200;
const int kBsdTrunc     = 0x0400;
const int kBsdDirectory = 0x00020000;

// Where a backup lands. Beside defaults.cfg, on the partition this app
// already writes to - deliberately NOT inside AFR, which the randomizer
// rewrites wholesale and would take the backups with it.
const char* const kBackupRoot = "/data/bbrandomizer/SaveBackups";

struct BsdDirent {
    uint32_t d_fileno;
    uint16_t d_reclen;
    uint8_t  d_type;
    uint8_t  d_namlen;
    char     d_name[256];
};

// BSD d_type values. Only DIR matters here, but a directory opened and read as
// though it were a file returns directory data rather than failing, which is
// exactly how the first version of this probe reported sce_sys as a 32768-byte
// "file" and lost everything inside it.
const uint8_t kDtDir = 4;

// Save directory names are at most 32 bytes and are NOT guaranteed to be
// null-terminated when they use the whole field.
std::string Bounded(const char* data, size_t maxLen) {
    size_t n = 0;
    while (n < maxLen && data[n] != '\0') n++;
    return std::string(data, n);
}

std::string Hex(int value) {
    char buf[32];
    snprintf(buf, sizeof(buf), "0x%08X", (unsigned)value);
    return std::string(buf);
}

// mtime is the save's last-written time. Rendered local, because the question
// it answers - "is this the save I was just playing?" - is asked in the time
// the player's clock shows, not UTC.
std::string WhenText(time_t t) {
    if (t <= 0) return "(NOT SET)";
    struct tm* lt = localtime(&t);
    if (!lt) return "(UNREADABLE " + std::to_string((long long)t) + ")";
    char buf[64];
    if (strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", lt) == 0) {
        return "(UNFORMATTABLE " + std::to_string((long long)t) + ")";
    }
    return std::string(buf);
}

std::string SizeText(uint64_t blocks) {
    // Blocks are the unit the API speaks in; bytes are the unit a person
    // sizing a copy operation needs.
    uint64_t bytes = blocks * (uint64_t)ORBIS_SAVE_DATA_BLOCK_SIZE;
    char buf[96];
    snprintf(buf, sizeof(buf), "%llu BLOCKS (%llu KB)",
             (unsigned long long)blocks, (unsigned long long)(bytes / 1024));
    return std::string(buf);
}

// The metadata strings are UTF-8 and may hold anything the game wrote. The
// atlas draws printable ASCII, so anything else is shown as '?' rather than
// silently dropped - a wrong character is a visible result, a missing one is
// a misleading one.
std::string Printable(const char* data, size_t maxLen) {
    std::string out;
    for (size_t i = 0; i < maxLen && data[i] != '\0'; i++) {
        unsigned char c = (unsigned char)data[i];
        out += (c >= 32 && c <= 126) ? (char)c : '?';
    }
    return out;
}

// Reads one string value out of a PS4 param.sfo. Format: "\0PSF" magic, then
// a 20-byte header giving the key-table and data-table offsets and the entry
// count, then `count` 16-byte index entries.
//
// Returns empty on any malformed input rather than trusting offsets from a file
// this app does not own.
std::string SfoString(const unsigned char* buf, size_t len, const char* wantKey) {
    if (len < 0x14 || memcmp(buf, "\x00PSF", 4) != 0) return std::string();

    uint32_t keyTable, dataTable, count;
    memcpy(&keyTable,  buf + 0x08, 4);
    memcpy(&dataTable, buf + 0x0C, 4);
    memcpy(&count,     buf + 0x10, 4);
    if (count > 1024 || keyTable >= len || dataTable >= len) return std::string();

    for (uint32_t i = 0; i < count; i++) {
        size_t e = 0x14 + (size_t)i * 16;
        if (e + 16 > len) break;

        uint16_t keyOff, fmt;
        uint32_t valLen, dataOff;
        memcpy(&keyOff,  buf + e + 0, 2);
        memcpy(&fmt,     buf + e + 2, 2);
        memcpy(&valLen,  buf + e + 4, 4);
        memcpy(&dataOff, buf + e + 12, 4);

        size_t kPos = (size_t)keyTable + keyOff;
        if (kPos >= len) continue;

        size_t kEnd = kPos;
        while (kEnd < len && buf[kEnd] != 0) kEnd++;
        std::string key((const char*)buf + kPos, kEnd - kPos);
        if (key != wantKey) continue;

        if (fmt != 0x0204) return std::string();   // not a UTF-8 string entry
        size_t vPos = (size_t)dataTable + dataOff;
        if (vPos >= len || vPos + valLen > len) return std::string();
        return Bounded((const char*)buf + vPos, valLen);
    }
    return std::string();
}

// Where an installed game's param.sfo might be readable from inside this
// app's sandbox. None of these is guaranteed: architecture.md records that the
// encrypted game installation is NOT accessible, which is the whole reason the
// vanilla tree has to be supplied separately. param.sfo is metadata the system
// reads unencrypted, so it is worth asking - but only by trying an ordinary
// read-only open. Nothing here escalates privilege, and a refusal is a result.
const char* const kSfoPathFormats[] = {
    "/user/app/%s/sce_sys/param.sfo",
    "/user/appmeta/%s/param.sfo",
    "/system_data/priv/appmeta/%s/param.sfo",
    "/mnt/sandbox/%s_000/app0/sce_sys/param.sfo",
    "/user/appmeta/external/%s/param.sfo",
};
const int kSfoPathCount = sizeof(kSfoPathFormats) / sizeof(kSfoPathFormats[0]);

// The six officially-issued Bloodborne SKUs, same list as Game/GameInfo.cpp.
// Duplicated rather than shared: this file is temporary and GameInfo's copy is
// file-local, and a probe that drags a refactor in with it is hard to delete.
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

// --- milestone 0: the write probe ------------------------------------------

// THE SCRATCH DIRECTORY. Every save-data write this probe performs names this
// and nothing else. It is a compile-time constant so that no runtime value,
// no search result and no off-by-one can ever aim a CREATE2 mount or a delete
// at a directory the game actually uses - Bloodborne's is SPRJ0005, and this
// probe never types that.
//
// It is created under the SAVE-DATA title (the one the search found saves
// under, CUSA00207 on the console under test), because that is exactly the
// cross-title CREATE2 path the worlds feature needs to prove.
//
// Save directory names are at most ORBIS_SAVE_DATA_DIRNAME_DATA_MAXSIZE bytes.
const char* const kScratchDirName = "BBRRESTORETEST";

// This app's own folder on /data. The rename step works entirely inside it.
const char* const kBbrRoot = "/data/bbrandomizer";

// Reads one value out of a param.sfo WITHOUT caring what format it is in.
//
// SfoString() above only understands format 0x0204, the UTF-8 string entries.
// The three values this probe needs - ACCOUNT_ID, SAVEDATA_BLOCKS and the
// PARAMS blob - are all format 0x0004, which SfoString() returns empty for.
// This is that raw-bytes sibling: it hands back a pointer into the buffer and
// a length, and lets the caller decide what the bytes mean.
//
// Returns nullptr on any malformed input rather than trusting offsets from a
// file this app does not own.
const unsigned char* SfoRaw(const unsigned char* buf, size_t len,
                            const char* wantKey,
                            uint16_t* outFmt, uint32_t* outLen) {
    if (len < 0x14 || memcmp(buf, "\x00PSF", 4) != 0) return nullptr;

    uint32_t keyTable, dataTable, count;
    memcpy(&keyTable,  buf + 0x08, 4);
    memcpy(&dataTable, buf + 0x0C, 4);
    memcpy(&count,     buf + 0x10, 4);
    if (count > 1024 || keyTable >= len || dataTable >= len) return nullptr;

    for (uint32_t i = 0; i < count; i++) {
        size_t e = 0x14 + (size_t)i * 16;
        if (e + 16 > len) break;

        uint16_t keyOff, fmt;
        uint32_t valLen, dataOff;
        memcpy(&keyOff,  buf + e + 0, 2);
        memcpy(&fmt,     buf + e + 2, 2);
        memcpy(&valLen,  buf + e + 4, 4);
        memcpy(&dataOff, buf + e + 12, 4);

        size_t kPos = (size_t)keyTable + keyOff;
        if (kPos >= len) continue;

        size_t kEnd = kPos;
        while (kEnd < len && buf[kEnd] != 0) kEnd++;
        std::string key((const char*)buf + kPos, kEnd - kPos);
        if (key != wantKey) continue;

        size_t vPos = (size_t)dataTable + dataOff;
        if (vPos >= len || vPos + valLen > len) return nullptr;
        if (outFmt) *outFmt = fmt;
        if (outLen) *outLen = valLen;
        return buf + vPos;
    }
    return nullptr;
}

// A format-0x0004 integer, little-endian, up to 8 bytes wide. ACCOUNT_ID is
// 8 bytes; SAVEDATA_BLOCKS is 8 bytes and is the `blocks` figure a CREATE2
// mount needs, readable from a stored backup without mounting anything.
bool SfoU64(const unsigned char* buf, size_t len, const char* wantKey,
            uint64_t* out) {
    uint16_t fmt = 0;
    uint32_t vlen = 0;
    const unsigned char* p = SfoRaw(buf, len, wantKey, &fmt, &vlen);
    if (!p || vlen == 0 || vlen > 8) return false;

    uint64_t value = 0;
    for (uint32_t i = 0; i < vlen; i++) value |= (uint64_t)p[i] << (8 * i);
    *out = value;
    return true;
}

std::string Hex64(uint64_t value) {
    char buf[32];
    snprintf(buf, sizeof(buf), "0x%016llX", (unsigned long long)value);
    return std::string(buf);
}

// Reads a whole file by chunked reads to a short read. NOT sceKernelStat: its
// st_size returned 88 for a genuine 44KB file on this kernel (FileIo.cpp).
bool ReadWholeFile(const std::string& path, std::vector<unsigned char>& out,
                   size_t cap) {
    out.clear();
    int fd = sceKernelOpen(path.c_str(), kBsdRdonly, 0);
    if (fd < 0) return false;

    const size_t kChunk = 65536;
    std::vector<unsigned char> chunk(kChunk);
    bool ok = true;
    for (;;) {
        int n = sceKernelRead(fd, chunk.data(), kChunk);
        if (n < 0) { ok = false; break; }
        if (n > 0) {
            if (out.size() + (size_t)n > cap) { ok = false; break; }
            out.insert(out.end(), chunk.begin(), chunk.begin() + n);
        }
        if ((size_t)n < kChunk) break;
    }
    sceKernelClose(fd);
    return ok;
}

// One file found by a tree walk: where it sits relative to the walk root, and
// how many bytes actually came out of it.
struct TreeEntry {
    std::string rel;
    uint64_t    bytes;
};

// Walks a directory tree, sizing every file by reading it through to a short
// read. Two traps are designed around here, both from technical-findings.md:
// st_size is unusable on this kernel, and a directory opened read-only returns
// directory data instead of failing - which is how an earlier probe reported
// sce_sys as a 32768-byte "file" and silently lost 10MB of a 36MB save. Hence
// d_type == 4 and recursion.
// Mounts a save RDONLY, walks it, unmounts. Used to bracket a risky operation
// so "did that change anything?" is answered by comparing two readings rather
// than by launching the game and watching for a black screen.
//
// Always RDONLY: this reads what is there, it never participates in the
// operation being measured.
bool SnapshotLiveSave(const std::string& title, const std::string& dir,
                      int32_t userId,
                      const std::function<void(const std::string&)>& say,
                      std::vector<TreeEntry>& out, uint64_t& bytes, int& errors);

struct TreeWalk {
    static void Go(const std::string& root, const std::string& rel,
                   std::vector<TreeEntry>& out, int& errors, int depth) {
        if (depth > 4) { errors++; return; }

        std::string path = rel.empty() ? root : root + "/" + rel;
        int dfd = sceKernelOpen(path.c_str(), kBsdRdonly | kBsdDirectory, 0777);
        if (dfd < 0) { errors++; return; }

        const int kBufSize = 64 * 1024;
        char* buf = (char*)malloc(kBufSize);
        if (!buf) { sceKernelClose(dfd); errors++; return; }

        // Names are collected before recursing: reusing one directory fd while
        // opening others underneath it is asking for trouble.
        std::vector<std::string> subdirs;

        for (;;) {
            int nread = sceKernelGetdents(dfd, buf, kBufSize);
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
                            const size_t kChunk = 65536;
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

                            TreeEntry entry;
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
            Go(root, subdirs[i], out, errors, depth + 1);
        }
    }
};

// mkdir every directory component of `rel` under `root`. mkdir does not create
// parents, and a save has sce_sys/ inside it.
void MakeParentDirs(const std::string& root, const std::string& rel) {
    size_t start = 0;
    for (;;) {
        size_t slash = rel.find('/', start);
        if (slash == std::string::npos) break;
        sceKernelMkdir((root + "/" + rel.substr(0, slash)).c_str(), 0777);
        start = slash + 1;
    }
}

// Milliseconds, for step 8. The raw microsecond figure is printed alongside so
// that a wrong unit is visible rather than assumed - the same reflex st_size
// earned the hard way.
uint64_t NowUs() { return sceKernelGetProcessTime(); }

bool SnapshotLiveSave(const std::string& title, const std::string& dir,
                      int32_t userId,
                      const std::function<void(const std::string&)>& say,
                      std::vector<TreeEntry>& out, uint64_t& bytes, int& errors) {
    out.clear();
    bytes  = 0;
    errors = 0;

    OrbisSaveDataMount m;
    memset(&m, 0, sizeof(m));
    m.userId      = userId;
    m.titleId     = title.c_str();
    m.dirName     = dir.c_str();
    m.fingerprint = nullptr;
    m.blocks      = 0;
    m.mountMode   = ORBIS_SAVE_DATA_MOUNT_MODE_RDONLY;

    OrbisSaveDataMountResult r;
    memset(&r, 0, sizeof(r));

    int rc = sceSaveDataMount(&m, &r);
    if (rc < 0) {
        say("    RDONLY MOUNT FAILED " + Hex(rc));
        return false;
    }

    std::string path = Bounded(r.mountPathName, sizeof(r.mountPathName));
    TreeWalk::Go(path, "", out, errors, 0);
    for (size_t i = 0; i < out.size(); i++) bytes += out[i].bytes;

    OrbisSaveDataMountPoint mp;
    memset(&mp, 0, sizeof(mp));
    memcpy(mp.data, r.mountPathName, sizeof(mp.data));
    int un = sceSaveDataUmount(&mp);
    if (un < 0) say("    UNMOUNT " + Hex(un));

    return true;
}

} // namespace

void ProbeSaveData(const std::string& configuredTitleId,
                   std::vector<std::string>& outLines,
                   bool doBackup) {
    // One helper so every line lands in both places - the screen the developer
    // is looking at, and live.log, which survives the app dying mid-probe.
    auto say = [&outLines](const std::string& line) {
        outLines.push_back(line);
        Log(("saveprobe: " + line).c_str());
    };

    say("PROBE START - CONFIGURED TITLE " + configuredTitleId);

    // If this line appears at all, libSceSaveData resolved its NIDs and the
    // binary loaded. That is itself the first result worth having.
    say("LIBSCESAVEDATA LINKED AND RUNNING");

    int32_t userId = -1;
    int rc = sceUserServiceInitialize(nullptr);
    say("USERSERVICEINITIALIZE " + Hex(rc));

    rc = sceUserServiceGetInitialUser(&userId);
    if (rc < 0) {
        say("GETINITIALUSER FAILED " + Hex(rc) + " - CANNOT CONTINUE");
        return;
    }
    say("INITIAL USER ID " + std::to_string(userId));

    rc = sceSaveDataInitialize3(0);
    say("SAVEDATAINITIALIZE3 " + Hex(rc));

    // --- sweep every candidate title ---------------------------------------
    //
    // The configured title first, then the six official SKUs. A console's saves
    // belong to whichever SKU actually wrote them, which need not be the one
    // Setup Defaults points at - and that mismatch is invisible from a single
    // search returning zero hits.
    std::vector<std::string> titles;
    std::vector<std::string> labels;
    if (!configuredTitleId.empty()) {
        titles.push_back(configuredTitleId);
        labels.push_back("CONFIGURED");
    }
    for (int i = 0; i < kCandidateCount; i++) {
        bool already = false;
        for (size_t k = 0; k < titles.size(); k++)
            if (titles[k] == kCandidates[i].id) already = true;
        if (already) continue;
        titles.push_back(kCandidates[i].id);
        labels.push_back(kCandidates[i].label);
    }

    const int kMaxDirs = 16;
    OrbisSaveDataDirName foundDirs[kMaxDirs];
    std::string foundTitle;
    unsigned    foundCount = 0;

    // --- can we read the game's own param.sfo? -----------------------------
    //
    // INSTALL_DIR_SAVEDATA in a game's param.sfo is the AUTHORITATIVE answer to
    // "which title id owns this game's saves" - it is exactly why Bloodborne's
    // saves live under CUSA00207 while the game is CUSA03173. If it is readable
    // the app can stop guessing; if it is not, the search below still finds the
    // answer empirically, and this block costs one failed open per path.
    say("");
    say("LOOKING FOR THE GAME PARAM.SFO");
    bool sfoFound = false;
    for (int i = 0; i < kSfoPathCount && !sfoFound; i++) {
        char path[256];
        snprintf(path, sizeof(path), kSfoPathFormats[i], configuredTitleId.c_str());

        int sfd = sceKernelOpen(path, kBsdRdonly, 0);
        if (sfd < 0) {
            say(std::string("  ") + path + " - " + Hex(sfd));
            continue;
        }

        const size_t kCap = 64 * 1024;
        unsigned char* sbuf = (unsigned char*)malloc(kCap);
        int got = sbuf ? sceKernelRead(sfd, sbuf, kCap) : -1;
        sceKernelClose(sfd);

        if (got > 0) {
            sfoFound = true;
            say(std::string("  ") + path + " - READ " + std::to_string(got) + " BYTES OK");

            std::string installDir = SfoString(sbuf, (size_t)got, "INSTALL_DIR_SAVEDATA");
            std::string sfoTitle   = SfoString(sbuf, (size_t)got, "TITLE_ID");
            if (!sfoTitle.empty())   say("    TITLE_ID             " + sfoTitle);
            if (!installDir.empty()) {
                say("    INSTALL_DIR_SAVEDATA " + installDir);
                say("    ^ AUTHORITATIVE SAVE TITLE - NO SEARCHING NEEDED");
            } else {
                say("    INSTALL_DIR_SAVEDATA (ABSENT - SAVES USE TITLE_ID)");
            }
        }
        if (sbuf) free(sbuf);
    }
    if (!sfoFound) {
        say("  NOT AT ANY GUESSED PATH - MAPPING WHAT IS VISIBLE");

        // Every guess came back 0x80020002, which is ENOENT (errno 2), NOT
        // EACCES (which would end 0x0D). We were not refused - those paths do
        // not exist here. So the useful question is no longer "can we read it"
        // but "what does this sandbox actually expose", and that is answered by
        // listing roots rather than guessing leaf paths.
        //
        // Read-only opens and directory reads throughout. Nothing here attempts
        // to escalate privilege or leave the sandbox; a refusal is a result.
        const char* const kRoots[] = {
            "/", "/user", "/user/app", "/user/appmeta", "/user/home",
            "/system_data", "/mnt", "/mnt/sandbox", "/app0", "/data",
        };
        const int kRootCount = sizeof(kRoots) / sizeof(kRoots[0]);

        for (int r = 0; r < kRootCount; r++) {
            int rfd = sceKernelOpen(kRoots[r], kBsdRdonly | kBsdDirectory, 0777);
            if (rfd < 0) {
                say(std::string("  ") + kRoots[r] + " - " + Hex(rfd) +
                    (((unsigned)rfd & 0xFF) == 0x02 ? " (NOT THERE)" : " (REFUSED)"));
                continue;
            }

            const int kBufSize = 16 * 1024;
            char* rbuf = (char*)malloc(kBufSize);
            int   shown = 0;
            std::string names;

            if (rbuf) {
                for (;;) {
                    int nread = sceKernelGetdents(rfd, rbuf, kBufSize);
                    if (nread <= 0) break;
                    int pos = 0;
                    while (pos < nread) {
                        BsdDirent* de = (BsdDirent*)(rbuf + pos);
                        if (de->d_reclen == 0) break;
                        std::string nm(de->d_name, de->d_namlen);
                        if (nm != "." && nm != "..") {
                            if (shown < 12) {
                                if (!names.empty()) names += " ";
                                names += nm;
                            }
                            shown++;
                        }
                        pos += de->d_reclen;
                    }
                }
                free(rbuf);
            }
            sceKernelClose(rfd);

            say(std::string("  ") + kRoots[r] + " - OPEN OK, " +
                std::to_string(shown) + " ENTRY(S)");
            if (!names.empty()) {
                say("      " + names + (shown > 12 ? " ..." : ""));
            }
        }

        say("  FALLING BACK TO SEARCHING");
    }

    say("");
    say("SEARCHING " + std::to_string(titles.size()) + " TITLE(S)");

    for (size_t t = 0; t < titles.size(); t++) {
        OrbisSaveDataTitleId searchTitle;
        memset(&searchTitle, 0, sizeof(searchTitle));
        strncpy(searchTitle.data, titles[t].c_str(), sizeof(searchTitle.data) - 1);

        OrbisSaveDataDirNameSearchCond cond;
        memset(&cond, 0, sizeof(cond));
        cond.userId  = userId;
        cond.titleId = &searchTitle;

        OrbisSaveDataDirName dirNames[kMaxDirs];
        memset(dirNames, 0, sizeof(dirNames));

        // params and infos are what make this more than a list of names. The
        // search fills them WITHOUT mounting anything, so the save's title,
        // last-written time and size are readable even if the mount below is
        // refused. This is the metadata Apollo shows in its own list view.
        OrbisSaveDataParam      params[kMaxDirs];
        OrbisSaveDataSearchInfo infos[kMaxDirs];
        memset(params, 0, sizeof(params));
        memset(infos, 0, sizeof(infos));

        OrbisSaveDataDirNameSearchResult result;
        memset(&result, 0, sizeof(result));
        result.dirNames    = dirNames;
        result.dirNamesNum = kMaxDirs;
        result.params      = params;
        result.infos       = infos;
        result.setNum      = kMaxDirs;

        rc = sceSaveDataDirNameSearch(&cond, &result);

        std::string tag = titles[t] + " (" + labels[t] + ")";
        if (rc < 0) {
            say("  " + tag + " - SEARCH FAILED " + Hex(rc));
            continue;
        }
        if (result.hitNum == 0) {
            say("  " + tag + " - NONE");
            continue;
        }

        say("  " + tag + " - " + std::to_string(result.hitNum) + " SAVE DIR(S) OK");
        unsigned shown = result.hitNum < (unsigned)kMaxDirs ? result.hitNum
                                                            : (unsigned)kMaxDirs;
        // setNum is how many params/infos sets actually came back, which need
        // not equal hitNum - read only what was filled.
        unsigned metaCount = result.setNum < shown ? result.setNum : shown;

        for (unsigned i = 0; i < shown; i++) {
            say("      DIR " + Bounded(dirNames[i].data, sizeof(dirNames[i].data)));
            if (i >= metaCount) {
                say("        (NO METADATA RETURNED)");
                continue;
            }
            say("        SAVED   " + WhenText(params[i].mtime));
            say("        SIZE    " + SizeText(infos[i].blocks));

            std::string title    = Printable(params[i].title, sizeof(params[i].title));
            std::string subtitle = Printable(params[i].subtitle, sizeof(params[i].subtitle));
            std::string details  = Printable(params[i].details, sizeof(params[i].details));

            if (!title.empty())    say("        TITLE   " + title);
            if (!subtitle.empty()) say("        SUB     " + subtitle);
            if (!details.empty()) {
                // details is up to 1024 bytes; one screen line is plenty to
                // see whether it carries anything useful.
                if (details.size() > 90) details = details.substr(0, 90) + "...";
                say("        DETAIL  " + details);
            }
            if (params[i].userParam != 0) {
                say("        USERPARAM " + std::to_string((unsigned)params[i].userParam));
            }
        }

        // Keep the first title that actually has saves; that is what gets
        // mounted below.
        if (foundTitle.empty()) {
            memcpy(foundDirs, dirNames, sizeof(dirNames));
            foundTitle = titles[t];
            foundCount = shown;
        }
    }

    if (foundTitle.empty()) {
        say("");
        say("NO SAVE DATA UNDER ANY KNOWN BLOODBORNE TITLE");
        say("A SAVE MADE BY A REPACKAGED TITLE ID WOULD NOT APPEAR HERE -");
        say("SAVE DATA IS KEYED BY TITLE ID AND IS NEVER REDIRECTED.");
        say("PROBE END - NOTHING MOUNTED");
        return;
    }

    (void)foundCount;

    // --- mount the first title that had saves, read-only -------------------
    //
    // RDONLY deliberately. This probe answers "can we see it", and a
    // read-write mount of a real save is not something to do by accident while
    // finding out.
    say("");
    say("MOUNTING " + foundTitle);

    // sceSaveDataMount, NOT sceSaveDataMount2. The difference decides whether
    // this probe can work at all: OrbisSaveDataMount2 has no titleId field, so
    // it can only ever mount the CALLING app's own save data - this homebrew's,
    // which does not exist. Only the original OrbisSaveDataMount carries
    // `const char *titleId`, and reaching Bloodborne's saves from here means
    // naming Bloodborne.
    std::string dirName = Bounded(foundDirs[0].data, sizeof(foundDirs[0].data));

    OrbisSaveDataMount mount;
    memset(&mount, 0, sizeof(mount));
    mount.userId      = userId;
    mount.titleId     = foundTitle.c_str();
    mount.dirName     = dirName.c_str();
    mount.fingerprint = nullptr;   // only needed for cross-console transfer
    mount.blocks      = 0;
    mount.mountMode   = ORBIS_SAVE_DATA_MOUNT_MODE_RDONLY;

    OrbisSaveDataMountResult mountResult;
    memset(&mountResult, 0, sizeof(mountResult));

    say("  DIR " + dirName);

    rc = sceSaveDataMount(&mount, &mountResult);
    if (rc < 0) {
        // Mounting ANOTHER title's save data from this app may simply be
        // refused - homebrew is not the game that owns it. If so, this code is
        // the finding: it decides whether copy/restore can work from inside
        // this app at all, or whether the design has to go a different way.
        // The search above succeeding while this fails would be the clearest
        // possible statement of where the boundary is.
        say("MOUNT FAILED " + Hex(rc));
        say("SEARCH COULD SEE IT BUT MOUNT WAS REFUSED - THIS IS THE ANSWER");
        say("PROBE END - NOTHING MOUNTED");
        return;
    }

    std::string mountPath = Bounded(mountResult.mountPathName,
                                    sizeof(mountResult.mountPathName));
    say("MOUNTED AT " + mountPath);

    // How big the mounted save actually is, from the mount rather than the
    // search - a second, independent reading of the same number.
    OrbisSaveDataMountPoint infoPoint;
    memset(&infoPoint, 0, sizeof(infoPoint));
    memcpy(infoPoint.data, mountResult.mountPathName, sizeof(infoPoint.data));

    OrbisSaveDataMountInfo mountInfo;
    memset(&mountInfo, 0, sizeof(mountInfo));
    sceSaveDataGetMountInfo(&infoPoint, &mountInfo);
    say("MOUNT SIZE " + SizeText(mountInfo.blocks));

    // --- walk it with ordinary file I/O ------------------------------------
    //
    // The whole point: if this lists files and reads bytes, the mount is a
    // normal readable directory and copying it out is a file-copy problem,
    // not a crypto one.
    // Recursive, and directory-aware via d_type. A flat listing under-reports:
    // sce_sys is a directory, and the first version of this probe read it as a
    // file, got one block back and silently lost everything inside it - 10MB of
    // the mount's own reported 36MB went unaccounted for.
    uint64_t grandTotal = 0;
    int      fileCount  = 0;
    int      dirCount   = 0;

    // A save is a handful of directories deep at most; the cap is here so a
    // surprise cannot spin forever on hardware.
    struct Walk {
        // dst empty = read-only probe. dst set = copy as we go: the same
        // pass that proves every byte is readable is the pass that writes it
        // out, so a backup cannot report success over a file never read.
        static void Go(const std::string& path, const std::string& dst,
                       const std::string& indent, int depth,
                       uint64_t& total, int& files, int& dirs, int& errors,
                       const std::function<void(const std::string&)>& say) {
            if (!dst.empty()) sceKernelMkdir(dst.c_str(), 0777);
            if (depth > 4) {
                say(indent + "(DEPTH LIMIT)");
                errors++;
                return;
            }

            int dfd = sceKernelOpen(path.c_str(), kBsdRdonly | kBsdDirectory, 0777);
            if (dfd < 0) {
                say(indent + "OPEN DIR FAILED " + Hex(dfd));
                errors++;
                return;
            }

            const int kBufSize = 64 * 1024;
            char* buf = (char*)malloc(kBufSize);
            if (!buf) {
                sceKernelClose(dfd);
                say(indent + "OUT OF MEMORY");
                errors++;
                return;
            }

            // Names are collected before recursing: reusing one directory fd
            // while opening others underneath it is asking for trouble.
            std::vector<std::string> subdirs;
            std::vector<std::string> subdests;

            for (;;) {
                int nread = sceKernelGetdents(dfd, buf, kBufSize);
                if (nread <= 0) break;

                int pos = 0;
                while (pos < nread) {
                    BsdDirent* de = (BsdDirent*)(buf + pos);
                    if (de->d_reclen == 0) break;

                    std::string name(de->d_name, de->d_namlen);
                    if (name != "." && name != "..") {
                        std::string full = path + "/" + name;

                        if (de->d_type == kDtDir) {
                            dirs++;
                            say(indent + "DIR  " + name + "/");
                            subdirs.push_back(full);
                            subdests.push_back(dst.empty() ? "" : dst + "/" + name);
                        } else {
                            files++;
                            // Read through, counting. sceKernelStat's st_size is
                            // unusable here - on real hardware it returned 88 for
                            // a 44KB file (see Randomizer/FileIo.cpp).
                            int ffd = sceKernelOpen(full.c_str(), kBsdRdonly, 0);
                            if (ffd < 0) {
                                say(indent + "FILE " + name + " - OPEN FAILED " + Hex(ffd));
                                errors++;
                            } else {
                                int ofd = -1;
                                if (!dst.empty()) {
                                    std::string outPath = dst + "/" + name;
                                    ofd = sceKernelOpen(outPath.c_str(),
                                                        kBsdWronly | kBsdCreat | kBsdTrunc,
                                                        0777);
                                    if (ofd < 0) {
                                        say(indent + "FILE " + name +
                                            " - DEST OPEN FAILED " + Hex(ofd));
                                        sceKernelClose(ffd);
                                        errors++;
                                        pos += de->d_reclen;
                                        continue;
                                    }
                                }

                                const size_t kChunk = 65536;
                                char*    chunk = (char*)malloc(kChunk);
                                uint64_t got   = 0;
                                bool     bad   = false;

                                if (!chunk) {
                                    bad = true;
                                } else {
                                    for (;;) {
                                        int n = sceKernelRead(ffd, chunk, kChunk);
                                        if (n < 0) { bad = true; break; }
                                        if (n > 0 && ofd >= 0) {
                                            // A short write is a failure, not
                                            // something to retry blindly: a
                                            // backup that silently truncates is
                                            // worse than one that says it could
                                            // not finish.
                                            int w = sceKernelWrite(ofd, chunk, n);
                                            if (w != n) { bad = true; break; }
                                        }
                                        got += (uint64_t)n;
                                        if ((size_t)n < kChunk) break;
                                    }
                                    free(chunk);
                                }
                                sceKernelClose(ffd);
                                if (ofd >= 0) sceKernelClose(ofd);

                                total += got;
                                if (bad) errors++;
                                say(indent + "FILE " + name + " - " +
                                    std::to_string((unsigned long long)got) +
                                    (bad ? " BYTE(S) THEN ERROR"
                                         : (dst.empty() ? " BYTES READ OK"
                                                        : " BYTES COPIED")));
                            }
                        }
                    }
                    pos += de->d_reclen;
                }
            }

            free(buf);
            sceKernelClose(dfd);

            for (size_t i = 0; i < subdirs.size(); i++) {
                Go(subdirs[i], subdests[i], indent + "  ", depth + 1,
                   total, files, dirs, errors, say);
            }
        }
    };

    // Apollo names its copies "<userid>_<titleid>_<dirname>". The same idea
    // with a timestamp instead of the user id, because this app backs up one
    // console's saves repeatedly and wants each run kept, not overwritten.
    std::string backupDir;
    if (doBackup) {
        char stamp[32];
        time_t now = time(nullptr);
        struct tm* lt = localtime(&now);
        if (lt && strftime(stamp, sizeof(stamp), "%Y%m%d-%H%M%S", lt) != 0) {
            // Nothing below /data is created for us, so every level has to be
            // made in turn - mkdir does not create parents.
            sceKernelMkdir("/data/bbrandomizer", 0777);
            sceKernelMkdir(kBackupRoot, 0777);
            backupDir = std::string(kBackupRoot) + "/" + foundTitle + "_" +
                        dirName + "_" + stamp;
            say("");
            say("BACKING UP TO " + backupDir);
        } else {
            say("COULD NOT BUILD A TIMESTAMP - BACKUP SKIPPED");
        }
    }

    int errorCount = 0;
    Walk::Go(mountPath, backupDir, "  ", 0,
             grandTotal, fileCount, dirCount, errorCount, say);

    say("TOTAL " + std::to_string(fileCount) + " FILE(S) IN " +
        std::to_string(dirCount + 1) + " DIR(S), " +
        std::to_string((unsigned long long)(grandTotal / 1024)) + " KB");

    // Reconciles against the mount's own figure. A flat listing used to
    // under-report by 10MB because sce_sys is a directory; this line is what
    // makes that kind of gap visible instead of plausible.
    say("MOUNT REPORTED " + SizeText(mountInfo.blocks));

    if (!backupDir.empty()) {
        if (errorCount == 0) {
            say("BACKUP COMPLETE - " + std::to_string(errorCount) + " ERROR(S)");
        } else {
            say("BACKUP INCOMPLETE - " + std::to_string(errorCount) +
                " ERROR(S) - DO NOT TRUST THIS COPY");
        }
    } else if (errorCount != 0) {
        say("READ ERRORS " + std::to_string(errorCount));
    } 

    // --- always unmount ----------------------------------------------------
    OrbisSaveDataMountPoint umount;
    memset(&umount, 0, sizeof(umount));
    memcpy(umount.data, mountResult.mountPathName, sizeof(umount.data));

    rc = sceSaveDataUmount(&umount);
    say("UNMOUNT " + Hex(rc));

    say("PROBE END");
}


void ProbeSaveDataWrite(const std::string& configuredTitleId,
                        std::vector<std::string>& outLines) {
    // Same reporting contract as ProbeSaveData: every line goes to the screen
    // and to live.log, so a probe that dies mid-write still leaves its trail -
    // which on a write probe is the whole point.
    auto say = [&outLines](const std::string& line) {
        outLines.push_back(line);
        Log(("savewrite: " + line).c_str());
    };

    say("WRITE PROBE START - CONFIGURED TITLE " + configuredTitleId);
    say("LIBSCESAVEDATA LINKED AND RUNNING");
    say("");

    // --- STEP 1 (part one): the scratch name -------------------------------
    say("STEP 1 SCRATCH NAME");
    say(std::string("  SCRATCH SAVE DIR ") + kScratchDirName);
    say("  EVERY SAVE-DATA WRITE BELOW NAMES THIS AND NOTHING ELSE");
    say("  GUARD VERDICT PRINTS AFTER THE SEARCH - IT NEEDS THE RESULTS");

    // --- STEP 2: who is the player -----------------------------------------
    say("");
    say("STEP 2 IDENTITY");

    int rc = sceUserServiceInitialize(nullptr);
    say("  USERSERVICEINITIALIZE " + Hex(rc));

    int32_t initialUser = ORBIS_USER_SERVICE_USER_ID_INVALID;
    int initRc = sceUserServiceGetInitialUser(&initialUser);
    say("  GETINITIALUSER    " + Hex(initRc) + " ID " + std::to_string(initialUser));

    int32_t foregroundUser = ORBIS_USER_SERVICE_USER_ID_INVALID;
    int fgRc = sceUserServiceGetForegroundUser(&foregroundUser);
    say("  GETFOREGROUNDUSER " + Hex(fgRc) + " ID " + std::to_string(foregroundUser));

    OrbisUserServiceLoginUserIdList loginList;
    memset(&loginList, 0, sizeof(loginList));
    int listRc = sceUserServiceGetLoginUserIdList(&loginList);
    std::string loginIds;
    for (int i = 0; i < ORBIS_USER_SERVICE_MAX_LOGIN_USERS; i++) {
        if (!loginIds.empty()) loginIds += " ";
        loginIds += std::to_string(loginList.userId[i]);
    }
    say("  GETLOGINUSERIDLIST " + Hex(listRc) + " IDS " + loginIds);

    // Which id everything below is scoped to. technical-findings.md 1.2 says
    // prefer the foreground user - "initial user" is not necessarily whoever
    // launched the app. Falling back to the initial user keeps the probe
    // runnable and says so, rather than refusing the way production code will.
    int32_t userId = ORBIS_USER_SERVICE_USER_ID_INVALID;
    if (fgRc >= 0 && foregroundUser != ORBIS_USER_SERVICE_USER_ID_INVALID) {
        userId = foregroundUser;
        say("  USING USER ID " + std::to_string(userId) + " (FOREGROUND)");
    } else if (initRc >= 0 && initialUser != ORBIS_USER_SERVICE_USER_ID_INVALID) {
        userId = initialUser;
        say("  USING USER ID " + std::to_string(userId) + " (INITIAL - FOREGROUND UNAVAILABLE)");
    } else {
        say("  NO USABLE USER ID - CANNOT CONTINUE");
        say("WRITE PROBE END");
        return;
    }

    char nameBuf[64];
    memset(nameBuf, 0, sizeof(nameBuf));
    int nameRc = sceUserServiceGetUserName(userId, nameBuf, sizeof(nameBuf));
    say("  GETUSERNAME       " + Hex(nameRc) + " " + Printable(nameBuf, sizeof(nameBuf)));

    uint64_t npAccountId = 0;
    int npRc = sceUserServiceGetNpAccountId(userId, &npAccountId);
    say("  GETNPACCOUNTID    " + Hex(npRc) + " " + Hex64(npAccountId));

    rc = sceSaveDataInitialize3(0);
    say("  SAVEDATAINITIALIZE3 " + Hex(rc));

    // --- STEP 2 (continued): find the live save ----------------------------
    //
    // The configured title first, then the six official SKUs, exactly as
    // ProbeSaveData does - a console's saves belong to whichever SKU wrote
    // them, which need not be the one Setup Defaults points at.
    std::vector<std::string> titles;
    std::vector<std::string> labels;
    if (!configuredTitleId.empty()) {
        titles.push_back(configuredTitleId);
        labels.push_back("CONFIGURED");
    }
    for (int i = 0; i < kCandidateCount; i++) {
        bool already = false;
        for (size_t k = 0; k < titles.size(); k++)
            if (titles[k] == kCandidates[i].id) already = true;
        if (already) continue;
        titles.push_back(kCandidates[i].id);
        labels.push_back(kCandidates[i].label);
    }

    // One search helper, heap-backed. params/infos are 1280 and 48 bytes per
    // slot, which is more than belongs on a UI thread's stack, and the search
    // is run twice (sweep, then again after the delete).
    const unsigned kMaxDirs = 16;
    std::vector<OrbisSaveDataDirName>     dirNames(kMaxDirs);
    std::vector<OrbisSaveDataParam>       params(kMaxDirs);
    std::vector<OrbisSaveDataSearchInfo>  infos(kMaxDirs);

    auto searchDirs = [&](const std::string& title,
                          std::vector<std::string>& outDirs,
                          unsigned& hitNum) -> int {
        outDirs.clear();
        hitNum = 0;

        OrbisSaveDataTitleId searchTitle;
        memset(&searchTitle, 0, sizeof(searchTitle));
        strncpy(searchTitle.data, title.c_str(), sizeof(searchTitle.data) - 1);

        // cond.key and cond.order are left at their defaults, so the ORDER of
        // the returned names is not guaranteed. Nothing below reads meaning
        // from a position: the live directory is only accepted when there is
        // exactly one, and the scratch directory is looked for BY NAME.
        OrbisSaveDataDirNameSearchCond cond;
        memset(&cond, 0, sizeof(cond));
        cond.userId  = userId;
        cond.titleId = &searchTitle;

        memset(dirNames.data(), 0, kMaxDirs * sizeof(OrbisSaveDataDirName));
        memset(params.data(),   0, kMaxDirs * sizeof(OrbisSaveDataParam));
        memset(infos.data(),    0, kMaxDirs * sizeof(OrbisSaveDataSearchInfo));

        OrbisSaveDataDirNameSearchResult res;
        memset(&res, 0, sizeof(res));
        res.dirNames    = dirNames.data();
        res.dirNamesNum = kMaxDirs;
        res.params      = params.data();
        res.infos       = infos.data();
        res.setNum      = kMaxDirs;

        int r = sceSaveDataDirNameSearch(&cond, &res);
        if (r < 0) return r;

        hitNum = res.hitNum;
        unsigned shown = res.hitNum < kMaxDirs ? res.hitNum : kMaxDirs;
        for (unsigned i = 0; i < shown; i++) {
            outDirs.push_back(Bounded(dirNames[i].data, sizeof(dirNames[i].data)));
        }
        return r;
    };

    say("");
    say("STEP 2 SEARCH - " + std::to_string(titles.size()) + " TITLE(S)");

    std::vector<std::string> discoveredDirs;   // every name any search returned
    std::string liveTitle;
    std::string liveDir;
    bool containerFound = false;
    bool tooManyDirs    = false;

    for (size_t t = 0; t < titles.size(); t++) {
        std::vector<std::string> hits;
        unsigned hitNum = 0;
        int srcRc = searchDirs(titles[t], hits, hitNum);

        std::string tag = titles[t] + " (" + labels[t] + ")";
        if (srcRc < 0) {
            say("  " + tag + " - SEARCH FAILED " + Hex(srcRc));
            continue;
        }
        if (hitNum == 0) {
            say("  " + tag + " - NONE");
            continue;
        }

        say("  " + tag + " - " + std::to_string(hitNum) + " SAVE DIR(S)");
        for (size_t i = 0; i < hits.size(); i++) {
            say("      DIR " + hits[i]);
            discoveredDirs.push_back(hits[i]);
        }

        // The first title that has any saves is the container this console
        // uses. A second directory under it is the plan's stop condition.
        if (!containerFound) {
            containerFound = true;
            liveTitle      = titles[t];
            if (hitNum == 1 && hits.size() == 1) {
                liveDir = hits[0];   // the only one there is, not "the first"
            } else {
                tooManyDirs = true;
                say("      MORE THAN ONE SAVE DIRECTORY UNDER THIS TITLE");
                say("      NOT PROCEEDING - THE PLAN STOPS HERE ON PURPOSE");
            }
        }
    }

    if (!containerFound) {
        say("  NO SAVE DATA UNDER ANY KNOWN BLOODBORNE TITLE");
    } else if (!tooManyDirs) {
        say("  LIVE SAVE " + liveTitle + " / " + liveDir);
    }

    // --- STEP 1 (part two): the guard --------------------------------------
    //
    // The one thing standing between this probe and a destroyed 40-hour
    // playthrough. If the compile-time scratch name equals ANY directory the
    // search returned, every write path below refuses.
    //
    // A scratch directory left behind by a previous failed run also trips
    // this, and that is deliberate: the guard cannot tell a leftover from a
    // collision, and guessing is not what a guard is for. The recovery is the
    // console's own Settings > Application Saved Data Management.
    say("");
    say("STEP 1 SCRATCH GUARD");
    bool guardArmed = true;
    for (size_t i = 0; i < discoveredDirs.size(); i++) {
        if (discoveredDirs[i] == kScratchDirName) guardArmed = false;
    }
    if (guardArmed) {
        say(std::string("  ARMED - ") + kScratchDirName + " MATCHES NONE OF " +
            std::to_string(discoveredDirs.size()) + " DISCOVERED DIR(S)");
    } else {
        say(std::string("  TRIPPED - ") + kScratchDirName +
            " MATCHES A DISCOVERED DIRECTORY");
        say("  ALL WRITE STEPS WILL ABORT");
        say("  IF THIS IS A LEFTOVER FROM AN EARLIER RUN, REMOVE IT FROM");
        say("  SETTINGS > APPLICATION SAVED DATA MANAGEMENT AND RUN AGAIN");
    }

    // Re-checked immediately before every save-data mount and delete, rather
    // than trusted once: a guard evaluated far from the call site is a guard
    // that a later edit walks past.
    auto scratchAllowed = [&](const char* what) -> bool {
        if (!guardArmed) {
            say(std::string("  ") + what + " ABORTED - SCRATCH GUARD TRIPPED");
            return false;
        }
        if (liveDir == kScratchDirName) {
            say(std::string("  ") + what + " ABORTED - SCRATCH NAME IS THE LIVE DIRECTORY");
            return false;
        }
        for (size_t i = 0; i < discoveredDirs.size(); i++) {
            if (discoveredDirs[i] == kScratchDirName) {
                say(std::string("  ") + what + " ABORTED - SCRATCH NAME IS A DISCOVERED DIRECTORY");
                return false;
            }
        }
        return true;
    };

    // --- STEP 2 (continued): the live save's own ACCOUNT_ID ----------------
    //
    // RDONLY. This is the only time the live directory is mounted at all, and
    // it is never mounted any other way anywhere in this file.
    say("");
    say("STEP 2 LIVE SAVE ACCOUNT_ID");
    if (!containerFound || tooManyDirs) {
        say("  SKIPPED - NO SINGLE LIVE SAVE DIRECTORY TO READ");
    } else {
        OrbisSaveDataMount liveMount;
        memset(&liveMount, 0, sizeof(liveMount));
        liveMount.userId      = userId;
        liveMount.titleId     = liveTitle.c_str();
        liveMount.dirName     = liveDir.c_str();
        liveMount.fingerprint = nullptr;
        liveMount.blocks      = 0;
        liveMount.mountMode   = ORBIS_SAVE_DATA_MOUNT_MODE_RDONLY;

        OrbisSaveDataMountResult liveResult;
        memset(&liveResult, 0, sizeof(liveResult));

        rc = sceSaveDataMount(&liveMount, &liveResult);
        if (rc < 0) {
            say("  MOUNT RDONLY FAILED " + Hex(rc));
        } else {
            std::string livePath = Bounded(liveResult.mountPathName,
                                           sizeof(liveResult.mountPathName));
            say("  MOUNTED RDONLY AT " + livePath);

            std::vector<unsigned char> sfo;
            if (!ReadWholeFile(livePath + "/sce_sys/param.sfo", sfo, 1024 * 1024)) {
                say("  COULD NOT READ sce_sys/param.sfo");
            } else {
                say("  PARAM.SFO " + std::to_string((unsigned long long)sfo.size()) + " BYTES");
                uint64_t sfoAccount = 0;
                if (!SfoU64(sfo.data(), sfo.size(), "ACCOUNT_ID", &sfoAccount)) {
                    say("  ACCOUNT_ID NOT READABLE FROM PARAM.SFO");
                } else {
                    say("  PARAM.SFO ACCOUNT_ID " + Hex64(sfoAccount));
                    say("  GETNPACCOUNTID       " + Hex64(npAccountId));
                    say(sfoAccount == npAccountId
                            ? "  MATCH - NP ACCOUNT ID IS USABLE FOR OWNERSHIP"
                            : "  MISMATCH - NP ACCOUNT ID IS NOT THE SAVE OWNER KEY");
                }
            }

            OrbisSaveDataMountPoint liveUmount;
            memset(&liveUmount, 0, sizeof(liveUmount));
            memcpy(liveUmount.data, liveResult.mountPathName, sizeof(liveUmount.data));
            rc = sceSaveDataUmount(&liveUmount);
            say("  UNMOUNT " + Hex(rc));
        }
    }

    // --- STEP 3: capacity ---------------------------------------------------
    //
    // Printed raw and NOT interpreted. sys/statvfs.h declares musl's LINUX
    // struct statvfs, and orbis/LibcInternal.h declares the symbol as a bare
    // void statvfs() - the SDK has no opinion about the layout. That is the
    // same class of mismatch that makes OrbisKernelStat::st_size return 88 for
    // a 44KB file, so the named fields below are musl's opinion, and the hex
    // words after them are what actually landed in memory.
    say("");
    say("STEP 3 CAPACITY");
    const char* const kVfsPaths[] = { "/data", "/user" };
    for (int p = 0; p < 2; p++) {
        // A 1KB zeroed arena rather than a bare struct statvfs: if the kernel
        // writes a larger struct than musl declares, a local would be a stack
        // smash. Over-sizing turns that into a visible hex dump instead.
        unsigned char arena[1024];
        memset(arena, 0, sizeof(arena));
        struct statvfs* vfs = (struct statvfs*)arena;

        int vrc = statvfs(kVfsPaths[p], vfs);
        say(std::string("  ") + kVfsPaths[p] + " RC " + Hex(vrc));
        say("    F_BSIZE   " + std::to_string((unsigned long long)vfs->f_bsize));
        say("    F_FRSIZE  " + std::to_string((unsigned long long)vfs->f_frsize));
        say("    F_BLOCKS  " + std::to_string((unsigned long long)vfs->f_blocks));
        say("    F_BFREE   " + std::to_string((unsigned long long)vfs->f_bfree));
        say("    F_BAVAIL  " + std::to_string((unsigned long long)vfs->f_bavail));
        say("    F_FILES   " + std::to_string((unsigned long long)vfs->f_files));
        say("    F_FFREE   " + std::to_string((unsigned long long)vfs->f_ffree));
        say("    F_FAVAIL  " + std::to_string((unsigned long long)vfs->f_favail));
        say("    F_FSID    " + std::to_string((unsigned long long)vfs->f_fsid));
        say("    F_FLAG    " + std::to_string((unsigned long long)vfs->f_flag));
        say("    F_NAMEMAX " + std::to_string((unsigned long long)vfs->f_namemax));

        for (int w = 0; w < 16; w += 2) {
            uint64_t a = 0, b = 0;
            memcpy(&a, arena + (size_t)w * 8, 8);
            memcpy(&b, arena + (size_t)(w + 1) * 8, 8);
            char line[96];
            snprintf(line, sizeof(line), "    +%03d  %016llX %016llX",
                     w * 8, (unsigned long long)a, (unsigned long long)b);
            say(line);
        }
    }

    // --- STEP 4: rename -----------------------------------------------------
    //
    // Entirely inside this app's own /data folder - no save data is involved.
    // Activation swaps the AFR tree by rename (plan 4.3 phase 5), so whether
    // a non-empty directory renames atomically here is worth knowing before
    // that design is built on it.
    say("");
    say("STEP 4 RENAME");
    {
        sceKernelMkdir(kBbrRoot, 0777);
        std::string srcDir  = std::string(kBbrRoot) + "/probe_rename_a";
        std::string dstDir  = std::string(kBbrRoot) + "/probe_rename_b";
        std::string srcFile = srcDir + "/probe.txt";
        std::string dstFile = dstDir + "/probe.txt";

        // Clear anything a previous interrupted run left behind.
        sceKernelUnlink(srcFile.c_str());
        sceKernelRmdir(srcDir.c_str());
        sceKernelUnlink(dstFile.c_str());
        sceKernelRmdir(dstDir.c_str());

        int mrc = sceKernelMkdir(srcDir.c_str(), 0777);
        say("  MKDIR " + srcDir + " " + Hex(mrc));

        const char* payload = "BBRANDOMIZER RENAME PROBE";
        size_t payloadLen = strlen(payload);
        int wfd = sceKernelOpen(srcFile.c_str(), kBsdWronly | kBsdCreat | kBsdTrunc, 0777);
        if (wfd < 0) {
            say("  CREATE probe.txt FAILED " + Hex(wfd));
        } else {
            int w = sceKernelWrite(wfd, payload, payloadLen);
            sceKernelClose(wfd);
            say("  WROTE " + std::to_string(w) + " OF " +
                std::to_string((unsigned long long)payloadLen) + " BYTE(S)");
        }

        int rrc = sceKernelRename(srcDir.c_str(), dstDir.c_str());
        say("  SCEKERNELRENAME " + Hex(rrc));

        std::vector<unsigned char> back;
        bool readBack = ReadWholeFile(dstFile, back, 4096);
        if (readBack && back.size() == payloadLen &&
            memcmp(back.data(), payload, payloadLen) == 0) {
            say("  FILE MOVED WITH THE DIRECTORY - CONTENT MATCHES");
        } else {
            say("  FILE DID NOT ARRIVE INTACT - READ " +
                std::to_string((unsigned long long)back.size()) + " BYTE(S)");
        }

        int oldFd = sceKernelOpen(srcDir.c_str(), kBsdRdonly | kBsdDirectory, 0777);
        if (oldFd < 0) {
            say("  OLD PATH GONE " + Hex(oldFd));
        } else {
            sceKernelClose(oldFd);
            say("  OLD PATH STILL OPENS - RENAME DID NOT MOVE IT");
        }

        int urc = sceKernelUnlink(dstFile.c_str());
        int drc = sceKernelRmdir(dstDir.c_str());
        say("  CLEANUP UNLINK " + Hex(urc) + " RMDIR " + Hex(drc));
    }

    // --- STEP 5: DISABLED ---------------------------------------------------
    //
    // This step attempted an RDWR|CREATE2 mount of a scratch directory. It is
    // switched off, for two independent reasons.
    //
    // It is ANSWERED. The mount was refused 0x809F0008 with REQUIRED BLOCKS 0.
    // Apollo brings a container into existence by writing a PFS image and a
    // sealedkey under /user/home and registering the result in /system_data's
    // savedata.db. The probe's own directory map shows neither path exists in
    // this sandbox, so this app cannot create save data and no retry changes
    // that.
    //
    // It is also IMPLICATED. After the run in which it failed, Bloodborne hung
    // on a black screen and only recovered when the save was deleted from the
    // console's own Saved Data Management. The developer has since confirmed
    // that swapping AFR content under an existing save - the obvious competing
    // explanation - works reliably and has never caused this. One anomaly, one
    // novel operation, the same window.
    //
    // THE UNCOMFORTABLE PART: the failed mount named BBRRESTORETEST and never
    // named SPRJ0005, yet SPRJ0005 is what stopped working. Naming a different
    // directory did NOT contain the damage, so the scratch-name guard is not
    // the isolation it was designed to be. It still prevents us writing to the
    // live directory; it does not prevent a failed create from disturbing the
    // title's save registry.
    //
    // Causation is not proven - it is one correlation. But a step that cannot
    // succeed and may corrupt a playthrough has no remaining upside, and
    // leaving it live would re-run it on every press.
    const bool kAttemptCreate2 = false;

    say("");
    say("STEP 5 RESTORE INTO THE SCRATCH DIRECTORY");
    if (!kAttemptCreate2) {
        say("  DISABLED - RDWR|CREATE2 WAS REFUSED 0x809F0008 AND THIS APP");
        say("  CANNOT CREATE SAVE DATA FROM THIS SANDBOX. THE RUN THAT");
        say("  ATTEMPTED IT PRECEDED A SAVE THAT WOULD NOT LOAD, SO IT IS");
        say("  NOT BEING RETRIED. SEE STEP 9 FOR THE QUESTION THAT REMAINS.");
    }

    std::vector<TreeEntry> source;       // what the backup holds
    std::string chosenBackupPath;        // ...and where it lives, for step 11
    std::string scratchMountPath;
    bool     restoreDone   = false;
    uint64_t restoreUs     = 0;
    uint64_t verifyUs      = 0;
    uint64_t sourceBytes   = 0;

    if (!containerFound) {
        say("  SKIPPED - NO LIVE SAVE FOUND, SO NO CONTAINER TITLE TO CREATE UNDER");
    } else if (tooManyDirs) {
        say("  SKIPPED - MORE THAN ONE SAVE DIRECTORY UNDER " + liveTitle);
    } else {
        // Which local backup to restore FROM. Names are
        // <title>_<dir>_<YYYYmmdd-HHMMSS>, so the newest is the greatest
        // string among those for this container title. Printed either way.
        std::string prefix = liveTitle + "_";
        std::string chosen;
        int candidates = 0;

        int bfd = sceKernelOpen(kBackupRoot, kBsdRdonly | kBsdDirectory, 0777);
        if (bfd < 0) {
            say(std::string("  ") + kBackupRoot + " - " + Hex(bfd));
        } else {
            const int kBufSize = 64 * 1024;
            char* buf = (char*)malloc(kBufSize);
            if (buf) {
                for (;;) {
                    int nread = sceKernelGetdents(bfd, buf, kBufSize);
                    if (nread <= 0) break;
                    int pos = 0;
                    while (pos < nread) {
                        BsdDirent* de = (BsdDirent*)(buf + pos);
                        if (de->d_reclen == 0) break;
                        std::string name(de->d_name, de->d_namlen);
                        if (de->d_type == kDtDir && name.size() > prefix.size() &&
                            name.compare(0, prefix.size(), prefix) == 0) {
                            candidates++;
                            if (name > chosen) chosen = name;
                        }
                        pos += de->d_reclen;
                    }
                }
                free(buf);
            }
            sceKernelClose(bfd);
        }

        if (chosen.empty()) {
            say("  NO LOCAL BACKUP FOR " + liveTitle + " UNDER " + kBackupRoot);
            say("  PRESS SQUARE ON THIS SCREEN FIRST TO TAKE ONE");
        } else {
            std::string backupDir = std::string(kBackupRoot) + "/" + chosen;
            chosenBackupPath = backupDir;
            say("  " + std::to_string(candidates) + " CANDIDATE(S) - USING " + chosen);

            // The container size a CREATE2 mount needs, read from the backup's
            // own param.sfo. Never guessed: restoring into a container smaller
            // than the save fails partway, and a half-written save is worse
            // than none.
            std::vector<unsigned char> sfo;
            uint64_t blocks = 0;
            bool haveBlocks = false;
            if (ReadWholeFile(backupDir + "/sce_sys/param.sfo", sfo, 1024 * 1024)) {
                haveBlocks = SfoU64(sfo.data(), sfo.size(), "SAVEDATA_BLOCKS", &blocks);
            }
            if (!haveBlocks) {
                say("  COULD NOT READ SAVEDATA_BLOCKS - NOT GUESSING A CONTAINER SIZE");
            } else {
                say("  SAVEDATA_BLOCKS " + SizeText(blocks));

                int srcErrors = 0;
                TreeWalk::Go(backupDir, "", source, srcErrors, 0);
                for (size_t i = 0; i < source.size(); i++) sourceBytes += source[i].bytes;
                say("  SOURCE " + std::to_string(source.size()) + " FILE(S), " +
                    std::to_string((unsigned long long)sourceBytes) + " BYTES, " +
                    std::to_string(srcErrors) + " READ ERROR(S)");

                if (source.empty() || srcErrors != 0) {
                    say("  NOT RESTORING FROM A BACKUP THAT DID NOT READ CLEANLY");
                } else if (kAttemptCreate2 && scratchAllowed("RDWR|CREATE2 MOUNT")) {
                    OrbisSaveDataMount sm;
                    memset(&sm, 0, sizeof(sm));
                    sm.userId      = userId;
                    sm.titleId     = liveTitle.c_str();
                    sm.dirName     = kScratchDirName;   // the constant, always
                    sm.fingerprint = nullptr;
                    sm.blocks      = blocks;
                    sm.mountMode   = ORBIS_SAVE_DATA_MOUNT_MODE_RDWR |
                                     ORBIS_SAVE_DATA_MOUNT_MODE_CREATE2;

                    OrbisSaveDataMountResult smr;
                    memset(&smr, 0, sizeof(smr));

                    say(std::string("  MOUNTING ") + liveTitle + " / " + kScratchDirName +
                        " RDWR|CREATE2");

                    uint64_t t0 = NowUs();
                    rc = sceSaveDataMount(&sm, &smr);
                    if (rc < 0) {
                        say("  MOUNT FAILED " + Hex(rc) + " - REQUIRED BLOCKS " +
                            std::to_string((unsigned long long)smr.requiredBlocks));
                    } else {
                        scratchMountPath = Bounded(smr.mountPathName,
                                                   sizeof(smr.mountPathName));
                        say("  MOUNTED AT " + scratchMountPath);

                        int written = 0, refused = 0, failed = 0;
                        uint64_t totalWritten = 0;

                        for (size_t i = 0; i < source.size(); i++) {
                            const TreeEntry& e = source[i];
                            MakeParentDirs(scratchMountPath, e.rel);

                            std::string from = backupDir + "/" + e.rel;
                            std::string to   = scratchMountPath + "/" + e.rel;

                            int ifd = sceKernelOpen(from.c_str(), kBsdRdonly, 0);
                            if (ifd < 0) {
                                say("    " + e.rel + " - SOURCE OPEN FAILED " + Hex(ifd));
                                failed++;
                                continue;
                            }
                            int ofd = sceKernelOpen(to.c_str(),
                                                    kBsdWronly | kBsdCreat | kBsdTrunc,
                                                    0777);
                            if (ofd < 0) {
                                sceKernelClose(ifd);
                                say("    " + e.rel + " - DEST OPEN REFUSED " + Hex(ofd));
                                refused++;
                                continue;
                            }

                            const size_t kChunk = 65536;
                            char* chunk = (char*)malloc(kChunk);
                            uint64_t wrote = 0;
                            int err = 0;
                            bool shortWrite = false;
                            if (!chunk) {
                                shortWrite = true;
                            } else {
                                for (;;) {
                                    int n = sceKernelRead(ifd, chunk, kChunk);
                                    if (n < 0) { err = n; break; }
                                    if (n > 0) {
                                        int wn = sceKernelWrite(ofd, chunk, n);
                                        if (wn != n) {
                                            if (wn < 0) err = wn; else shortWrite = true;
                                            break;
                                        }
                                        wrote += (uint64_t)wn;
                                    }
                                    if ((size_t)n < kChunk) break;
                                }
                                free(chunk);
                            }
                            sceKernelClose(ifd);
                            sceKernelClose(ofd);

                            if (err == 0 && !shortWrite && wrote == e.bytes) {
                                written++;
                                totalWritten += wrote;
                                say("    " + e.rel + " - " +
                                    std::to_string((unsigned long long)wrote) +
                                    " BYTES WRITTEN OK");
                            } else {
                                failed++;
                                say("    " + e.rel + " - " +
                                    std::to_string((unsigned long long)wrote) + " OF " +
                                    std::to_string((unsigned long long)e.bytes) +
                                    (err != 0 ? (" BYTES THEN " + Hex(err))
                                              : " BYTES THEN SHORT WRITE"));
                            }
                        }

                        say("  WROTE " + std::to_string(written) + " OF " +
                            std::to_string(source.size()) + " FILE(S), " +
                            std::to_string((unsigned long long)totalWritten) + " BYTES");
                        say("  DEST OPEN REFUSED " + std::to_string(refused) +
                            ", FAILED " + std::to_string(failed));

                        OrbisSaveDataMountPoint sump;
                        memset(&sump, 0, sizeof(sump));
                        memcpy(sump.data, smr.mountPathName, sizeof(sump.data));
                        rc = sceSaveDataUmount(&sump);
                        say("  UNMOUNT " + Hex(rc));

                        restoreUs   = NowUs() - t0;
                        restoreDone = true;
                        say("  STEP 5 ELAPSED " +
                            std::to_string((unsigned long long)(restoreUs / 1000)) +
                            " MS (" + std::to_string((unsigned long long)restoreUs) +
                            " US RAW)");
                    }
                }
            }
        }
    }

    // --- STEP 6: verify -----------------------------------------------------
    say("");
    say("STEP 6 VERIFY");
    if (!restoreDone) {
        say("  SKIPPED - STEP 5 DID NOT COMPLETE");
    } else if (scratchAllowed("RDONLY VERIFY MOUNT")) {
        OrbisSaveDataMount vm;
        memset(&vm, 0, sizeof(vm));
        vm.userId      = userId;
        vm.titleId     = liveTitle.c_str();
        vm.dirName     = kScratchDirName;
        vm.fingerprint = nullptr;
        vm.blocks      = 0;
        vm.mountMode   = ORBIS_SAVE_DATA_MOUNT_MODE_RDONLY;

        OrbisSaveDataMountResult vmr;
        memset(&vmr, 0, sizeof(vmr));

        uint64_t t0 = NowUs();
        rc = sceSaveDataMount(&vm, &vmr);
        if (rc < 0) {
            say("  RE-MOUNT RDONLY FAILED " + Hex(rc));
        } else {
            std::string vpath = Bounded(vmr.mountPathName, sizeof(vmr.mountPathName));
            say("  RE-MOUNTED AT " + vpath);

            OrbisSaveDataMountPoint infoPoint;
            memset(&infoPoint, 0, sizeof(infoPoint));
            memcpy(infoPoint.data, vmr.mountPathName, sizeof(infoPoint.data));
            OrbisSaveDataMountInfo mountInfo;
            memset(&mountInfo, 0, sizeof(mountInfo));
            sceSaveDataGetMountInfo(&infoPoint, &mountInfo);

            std::vector<TreeEntry> dest;
            int destErrors = 0;
            TreeWalk::Go(vpath, "", dest, destErrors, 0);

            uint64_t destBytes = 0;
            for (size_t i = 0; i < dest.size(); i++) {
                destBytes += dest[i].bytes;
                say("    " + dest[i].rel + " - " +
                    std::to_string((unsigned long long)dest[i].bytes) + " BYTES");
            }
            say("  WALKED " + std::to_string(dest.size()) + " FILE(S), " +
                std::to_string((unsigned long long)destBytes) + " BYTES, " +
                std::to_string(destErrors) + " ERROR(S)");
            say("  SOURCE " + std::to_string(source.size()) + " FILE(S), " +
                std::to_string((unsigned long long)sourceBytes) + " BYTES");

            // Always reconcile the walked total against the mount's own
            // figure: a flat listing once under-reported by 10MB and looked
            // entirely plausible doing it.
            say("  MOUNT REPORTS " + SizeText(mountInfo.blocks));

            int missing = 0, wrongSize = 0;
            for (size_t i = 0; i < source.size(); i++) {
                bool found = false;
                for (size_t j = 0; j < dest.size(); j++) {
                    if (dest[j].rel != source[i].rel) continue;
                    found = true;
                    if (dest[j].bytes != source[i].bytes) {
                        wrongSize++;
                        say("    SIZE DIFFERS " + source[i].rel + " - " +
                            std::to_string((unsigned long long)source[i].bytes) + " -> " +
                            std::to_string((unsigned long long)dest[j].bytes));
                    }
                    break;
                }
                if (!found) {
                    missing++;
                    say("    MISSING " + source[i].rel);
                }
            }
            int extra = 0;
            for (size_t j = 0; j < dest.size(); j++) {
                bool found = false;
                for (size_t i = 0; i < source.size(); i++) {
                    if (source[i].rel == dest[j].rel) { found = true; break; }
                }
                if (!found) { extra++; say("    EXTRA " + dest[j].rel); }
            }

            if (destErrors == 0 && missing == 0 && wrongSize == 0 && extra == 0 &&
                destBytes == sourceBytes && dest.size() == source.size()) {
                say("  VERDICT MATCH - EVERY FILE AND EVERY BYTE ACCOUNTED FOR");
            } else {
                say("  VERDICT MISMATCH - MISSING " + std::to_string(missing) +
                    ", WRONG SIZE " + std::to_string(wrongSize) +
                    ", EXTRA " + std::to_string(extra));
            }

            OrbisSaveDataMountPoint vump;
            memset(&vump, 0, sizeof(vump));
            memcpy(vump.data, vmr.mountPathName, sizeof(vump.data));
            rc = sceSaveDataUmount(&vump);
            say("  UNMOUNT " + Hex(rc));

            verifyUs = NowUs() - t0;
            say("  STEP 6 ELAPSED " +
                std::to_string((unsigned long long)(verifyUs / 1000)) + " MS (" +
                std::to_string((unsigned long long)verifyUs) + " US RAW)");
        }
    }

    // --- STEP 7: delete -----------------------------------------------------
    say("");
    say("STEP 7 DELETE");
    if (!restoreDone) {
        say("  SKIPPED - NOTHING WAS CREATED TO DELETE");
    } else if (scratchAllowed("SCESAVEDATADELETE")) {
        OrbisSaveDataTitleId delTitle;
        memset(&delTitle, 0, sizeof(delTitle));
        strncpy(delTitle.data, liveTitle.c_str(), sizeof(delTitle.data) - 1);

        OrbisSaveDataDirName delDir;
        memset(&delDir, 0, sizeof(delDir));
        strncpy(delDir.data, kScratchDirName, sizeof(delDir.data) - 1);

        OrbisSaveDataDelete del;
        memset(&del, 0, sizeof(del));
        del.userId  = userId;
        del.titleId = &delTitle;
        del.dirName = &delDir;

        rc = sceSaveDataDelete(&del);
        say(std::string("  SCESAVEDATADELETE ") + liveTitle + " / " +
            kScratchDirName + " " + Hex(rc));

        std::vector<std::string> after;
        unsigned afterHits = 0;
        int searchRc = searchDirs(liveTitle, after, afterHits);
        if (searchRc < 0) {
            say("  RE-SEARCH FAILED " + Hex(searchRc));
        } else {
            bool scratchStillThere = false;
            bool liveStillThere    = false;
            for (size_t i = 0; i < after.size(); i++) {
                say("      DIR " + after[i]);
                if (after[i] == kScratchDirName) scratchStillThere = true;
                if (after[i] == liveDir)         liveStillThere = true;
            }
            say(scratchStillThere ? "  SCRATCH DIRECTORY STILL PRESENT - DELETE DID NOT TAKE"
                                  : "  SCRATCH DIRECTORY GONE");
            say(liveStillThere ? "  LIVE DIRECTORY " + liveDir + " STILL PRESENT"
                               : "  LIVE DIRECTORY " + liveDir + " NOT FOUND - INVESTIGATE");
        }
    }

    // --- STEP 8: timings ----------------------------------------------------
    say("");
    say("STEP 8 TIMING");
    say("  STEP 5 RESTORE " +
        std::to_string((unsigned long long)(restoreUs / 1000)) + " MS");
    say("  STEP 6 VERIFY  " +
        std::to_string((unsigned long long)(verifyUs / 1000)) + " MS");

    // --- STEP 9: is RDWR granted on an EXISTING container? ------------------
    //
    // Step 5 established that CREATE2 is refused: this app cannot bring a save
    // container into existence, because Apollo does that by writing a PFS image
    // and a sealedkey under /user/home and registering the result in
    // /system_data's savedata.db - neither of which exists in this sandbox.
    //
    // That is a narrower failure than "restore does not work". Creating and
    // writing are separate permissions, and only creating has been tested. If
    // an ALREADY EXISTING container can be mounted RDWR, restore is still
    // possible in a different shape: never create, only overwrite what the game
    // itself made.
    //
    // The only existing container on this console is the live save, so this
    // step mounts it read-write. THIS IS THE ONE PLACE THE PROBE DEPARTS FROM
    // "the live directory is only ever mounted RDONLY", and it is deliberate,
    // developer-authorised, and bounded as tightly as the question allows:
    //
    //   - CREATE2 is NOT passed. Only an existing container can satisfy this.
    //   - No file is opened, walked, read or written while it is mounted.
    //   - It is unmounted immediately, on both the success and failure paths.
    //
    // The residual risk is that mounting read-write and unmounting may itself
    // rewrite container metadata even with no file writes. That cannot be
    // excluded from outside, which is why this runs last and why a verified
    // backup is a precondition.
    say("");
    say("STEP 9 RDWR MOUNT OF THE EXISTING LIVE CONTAINER");

    if (liveTitle.empty() || liveDir.empty()) {
        say("  SKIPPED - NO LIVE SAVE WAS IDENTIFIED");
    } else {
        say(std::string("  TARGET ") + liveTitle + " / " + liveDir);
        say("  NO CREATE2. NO FILE IS OPENED. MOUNT, REPORT, UNMOUNT.");

        // Bracketed: read the container's whole contents immediately before and
        // immediately after, and compare. Without this, "did that hurt?" can
        // only be answered by launching the game and watching for a black
        // screen - which is how the last incident became an ambiguity instead
        // of a finding.
        //
        // A match is not proof that nothing changed: the previous damage was
        // not visible in the files at all, and may have been registry state we
        // cannot see from here. A MISMATCH, though, is conclusive.
        std::vector<TreeEntry> before, after;
        uint64_t beforeBytes = 0, afterBytes = 0;
        int      beforeErrs  = 0, afterErrs  = 0;

        say("  READING CONTAINER BEFORE THE RDWR MOUNT");
        bool haveBefore = SnapshotLiveSave(liveTitle, liveDir, userId, say,
                                           before, beforeBytes, beforeErrs);
        if (!haveBefore) {
            say("  COULD NOT READ IT - NOT PROCEEDING");
            say("");
            say("WRITE PROBE END");
            return;
        }
        say("  BEFORE " + std::to_string(before.size()) + " FILE(S), " +
            std::to_string((unsigned long long)beforeBytes) + " BYTES, " +
            std::to_string(beforeErrs) + " ERROR(S)");

        OrbisSaveDataMount rw;
        memset(&rw, 0, sizeof(rw));
        rw.userId      = userId;
        rw.titleId     = liveTitle.c_str();
        rw.dirName     = liveDir.c_str();
        rw.fingerprint = nullptr;
        rw.blocks      = 0;
        rw.mountMode   = ORBIS_SAVE_DATA_MOUNT_MODE_RDWR;

        OrbisSaveDataMountResult rwr;
        memset(&rwr, 0, sizeof(rwr));

        int rwRc = sceSaveDataMount(&rw, &rwr);
        if (rwRc < 0) {
            say("  RDWR MOUNT REFUSED " + Hex(rwRc));
            say("  RESTORE HAS NO ROUTE FROM THIS SANDBOX - THIS IS THE ANSWER");
        } else {
            std::string rwPath = Bounded(rwr.mountPathName, sizeof(rwr.mountPathName));
            say("  RDWR MOUNT GRANTED AT " + rwPath);
            say("  RESTORE IS POSSIBLE BY OVERWRITING AN EXISTING CONTAINER");

            OrbisSaveDataMountPoint rwPoint;
            memset(&rwPoint, 0, sizeof(rwPoint));
            memcpy(rwPoint.data, rwr.mountPathName, sizeof(rwPoint.data));

            int rwUn = sceSaveDataUmount(&rwPoint);
            say("  UNMOUNT " + Hex(rwUn));
            if (rwUn < 0) {
                say("  UNMOUNT FAILED - DO NOT LAUNCH THE GAME, CHECK THE SAVE FIRST");
            }
        }

        // --- the other half of the bracket ---------------------------------
        say("  READING CONTAINER AFTER THE RDWR MOUNT");
        if (!SnapshotLiveSave(liveTitle, liveDir, userId, say,
                              after, afterBytes, afterErrs)) {
            say("  COULD NOT READ IT BACK - THAT IS ITSELF A RESULT");
            say("  THE CONTAINER WAS READABLE MOMENTS AGO AND IS NOT NOW");
        } else {
            say("  AFTER  " + std::to_string(after.size()) + " FILE(S), " +
                std::to_string((unsigned long long)afterBytes) + " BYTES, " +
                std::to_string(afterErrs) + " ERROR(S)");

            bool same = (before.size() == after.size()) &&
                        (beforeBytes == afterBytes);
            if (same) {
                for (size_t i = 0; i < before.size() && same; i++) {
                    bool found = false;
                    for (size_t k = 0; k < after.size(); k++) {
                        if (after[k].rel == before[i].rel) {
                            found = true;
                            if (after[k].bytes != before[i].bytes) same = false;
                            break;
                        }
                    }
                    if (!found) same = false;
                }
            }

            if (same) {
                say("  MATCH - NO FILE CHANGED NAME, COUNT OR SIZE");
                say("  THIS DOES NOT PROVE THE CONTAINER IS UNHARMED. THE LAST");
                say("  INCIDENT LEFT NO FILE-LEVEL TRACE EITHER. LAUNCH THE");
                say("  GAME TO FIND OUT.");
            } else {
                say("  MISMATCH - THE RDWR MOUNT CHANGED THE CONTAINER");
                say("  BEFORE " + std::to_string(before.size()) + " FILE(S) / " +
                    std::to_string((unsigned long long)beforeBytes) + " BYTES");
                say("  AFTER  " + std::to_string(after.size()) + " FILE(S) / " +
                    std::to_string((unsigned long long)afterBytes) + " BYTES");
            }
        }
    }

    // --- STEP 10: can bytes actually be written through an RDWR mount? ------
    //
    // Step 9 proved the mount is GRANTED and harmless. It did not write a
    // single byte, so whether writing is permitted is still unknown - and that
    // is the whole of restore.
    //
    // This writes one file's EXISTING CONTENT BACK OVER ITSELF. Same bytes,
    // same length, so a success changes nothing at all:
    //
    //   - O_TRUNC is deliberately NOT used. The length is identical, so there
    //     is nothing to truncate, and a write that dies halfway leaves the
    //     remaining bytes exactly as they were - which is to say, correct.
    //   - sce_sys is excluded. Metadata and icons are the parts most likely to
    //     be checksummed or regenerated, and the smallest game file is enough
    //     to answer the question.
    //   - The content is verified by reading it back before unmounting.
    //
    // This is the smallest operation that still answers "can we restore?".
    bool writesPermitted = false;

    say("");
    say("STEP 10 WRITE ONE FILE BACK OVER ITSELF");

    if (liveTitle.empty() || liveDir.empty()) {
        say("  SKIPPED - NO LIVE SAVE WAS IDENTIFIED");
    } else {
        std::vector<TreeEntry> pre;
        uint64_t preBytes = 0;
        int      preErrs  = 0;

        say("  READING CONTAINER BEFORE");
        if (!SnapshotLiveSave(liveTitle, liveDir, userId, say, pre, preBytes, preErrs)) {
            say("  COULD NOT READ IT - NOT PROCEEDING");
        } else {
            // Smallest file outside sce_sys. Smallest because a short write is
            // the failure mode, and less to write is less to go wrong.
            std::string target;
            uint64_t    targetBytes = 0;
            for (size_t i = 0; i < pre.size(); i++) {
                if (pre[i].rel.compare(0, 7, "sce_sys") == 0) continue;
                if (target.empty() || pre[i].bytes < targetBytes) {
                    target      = pre[i].rel;
                    targetBytes = pre[i].bytes;
                }
            }

            if (target.empty()) {
                say("  NO NON-SCE_SYS FILE TO TEST WITH");
            } else {
                say("  TARGET " + target + " (" +
                    std::to_string((unsigned long long)targetBytes) + " BYTES)");

                OrbisSaveDataMount wm;
                memset(&wm, 0, sizeof(wm));
                wm.userId      = userId;
                wm.titleId     = liveTitle.c_str();
                wm.dirName     = liveDir.c_str();
                wm.fingerprint = nullptr;
                wm.blocks      = 0;
                wm.mountMode   = ORBIS_SAVE_DATA_MOUNT_MODE_RDWR;

                OrbisSaveDataMountResult wmr;
                memset(&wmr, 0, sizeof(wmr));

                int wrc = sceSaveDataMount(&wm, &wmr);
                if (wrc < 0) {
                    say("  RDWR MOUNT REFUSED " + Hex(wrc));
                } else {
                    std::string wpath = Bounded(wmr.mountPathName,
                                                sizeof(wmr.mountPathName));
                    std::string full = wpath + "/" + target;
                    say("  MOUNTED AT " + wpath);

                    std::vector<unsigned char> original;
                    bool readOk = ReadWholeFile(full, original, (size_t)targetBytes + 4096);
                    say("  READ " + std::to_string((unsigned long long)original.size()) +
                        " BYTE(S) " + (readOk ? "OK" : "FAILED"));

                    if (readOk && !original.empty()) {
                        // No O_TRUNC - see the note above.
                        int wfd = sceKernelOpen(full.c_str(), kBsdWronly, 0);
                        if (wfd < 0) {
                            say("  OPEN FOR WRITE REFUSED " + Hex(wfd));
                            say("  RESTORE IS NOT POSSIBLE - THIS IS THE ANSWER");
                        } else {
                            int wrote = sceKernelWrite(wfd, original.data(),
                                                       original.size());
                            sceKernelClose(wfd);
                            say("  WROTE " + std::to_string(wrote) + " OF " +
                                std::to_string((unsigned long long)original.size()) +
                                " BYTE(S)");

                            std::vector<unsigned char> back;
                            if (!ReadWholeFile(full, back, (size_t)targetBytes + 4096)) {
                                say("  READ BACK FAILED");
                            } else if (back.size() != original.size()) {
                                say("  READ BACK SIZE CHANGED - " +
                                    std::to_string((unsigned long long)back.size()));
                            } else if (memcmp(back.data(), original.data(),
                                              back.size()) != 0) {
                                say("  READ BACK CONTENT DIFFERS - WRITE CORRUPTED IT");
                            } else {
                                say("  READ BACK IDENTICAL - THE WRITE LANDED");
                                writesPermitted = true;
                            }
                        }
                    }

                    OrbisSaveDataMountPoint wp;
                    memset(&wp, 0, sizeof(wp));
                    memcpy(wp.data, wmr.mountPathName, sizeof(wp.data));
                    int wun = sceSaveDataUmount(&wp);
                    say("  UNMOUNT " + Hex(wun));
                    if (wun < 0) {
                        say("  UNMOUNT FAILED - DO NOT LAUNCH THE GAME YET");
                    }
                }

                std::vector<TreeEntry> post;
                uint64_t postBytes = 0;
                int      postErrs  = 0;
                say("  READING CONTAINER AFTER");
                if (!SnapshotLiveSave(liveTitle, liveDir, userId, say,
                                      post, postBytes, postErrs)) {
                    say("  COULD NOT READ IT BACK - THAT IS ITSELF A RESULT");
                } else {
                    say("  BEFORE " + std::to_string(pre.size()) + " FILE(S), " +
                        std::to_string((unsigned long long)preBytes) + " BYTES");
                    say("  AFTER  " + std::to_string(post.size()) + " FILE(S), " +
                        std::to_string((unsigned long long)postBytes) + " BYTES");
                    say((pre.size() == post.size() && preBytes == postBytes)
                            ? "  MATCH - LAUNCH THE GAME TO CONFIRM IT STILL LOADS"
                            : "  MISMATCH - THE WRITE CHANGED THE CONTAINER");
                }
            }
        }
    }

    // --- STEP 11: the real thing - restore every file from a backup ---------
    //
    // Step 10 answers a permission question with one file and identical bytes.
    // That is not restore. Restore writes a whole save, and has failure modes
    // one file cannot show:
    //
    //   - sce_sys entries may refuse where game files accept. Two of them
    //     (sce_paramsfo1, sce_icon0png1) are system-generated.
    //   - A backup may hold files the container does not, so restore needs to
    //     CREATE inside the mount, not only overwrite. Different permission,
    //     reported separately below.
    //   - Unmount has to re-seal after many writes, not one.
    //
    // Gated on step 10 because if plain overwriting is refused there is nothing
    // here worth attempting, and the gate keeps attribution clean: a failure in
    // step 11 is a failure of SCALE, not of permission.
    say("");
    say("STEP 11 RESTORE EVERY FILE FROM THE BACKUP");

    if (!writesPermitted) {
        say("  SKIPPED - STEP 10 DID NOT PROVE WRITES ARE PERMITTED");
    } else if (chosenBackupPath.empty() || source.empty()) {
        say("  SKIPPED - NO USABLE BACKUP WAS FOUND IN STEP 5");
    } else {
        say("  FROM " + chosenBackupPath);
        say("  " + std::to_string(source.size()) + " FILE(S), " +
            std::to_string((unsigned long long)sourceBytes) + " BYTES");

        std::vector<TreeEntry> pre2;
        uint64_t pre2Bytes = 0;
        int      pre2Errs  = 0;
        say("  READING CONTAINER BEFORE");

        if (!SnapshotLiveSave(liveTitle, liveDir, userId, say,
                              pre2, pre2Bytes, pre2Errs)) {
            say("  COULD NOT READ IT - NOT PROCEEDING");
        } else {
            OrbisSaveDataMount rm;
            memset(&rm, 0, sizeof(rm));
            rm.userId      = userId;
            rm.titleId     = liveTitle.c_str();
            rm.dirName     = liveDir.c_str();
            rm.fingerprint = nullptr;
            rm.blocks      = 0;
            rm.mountMode   = ORBIS_SAVE_DATA_MOUNT_MODE_RDWR;

            OrbisSaveDataMountResult rmr;
            memset(&rmr, 0, sizeof(rmr));

            uint64_t t0 = NowUs();
            int rrc = sceSaveDataMount(&rm, &rmr);
            if (rrc < 0) {
                say("  RDWR MOUNT REFUSED " + Hex(rrc));
            } else {
                std::string rpath = Bounded(rmr.mountPathName,
                                            sizeof(rmr.mountPathName));
                say("  MOUNTED AT " + rpath);

                int wrote = 0, created = 0, overwrote = 0, refused = 0, shortW = 0;

                for (size_t i = 0; i < source.size(); i++) {
                    const TreeEntry& e = source[i];
                    std::string from = chosenBackupPath + "/" + e.rel;
                    std::string to   = rpath + "/" + e.rel;

                    // Did it already exist? That is the create-vs-overwrite
                    // distinction, and it is the one this step exists to learn.
                    bool existed = false;
                    for (size_t k = 0; k < pre2.size(); k++) {
                        if (pre2[k].rel == e.rel) { existed = true; break; }
                    }

                    std::vector<unsigned char> data;
                    if (!ReadWholeFile(from, data, (size_t)e.bytes + 4096)) {
                        say("    " + e.rel + " - SOURCE READ FAILED");
                        refused++;
                        continue;
                    }

                    MakeParentDirs(rpath, e.rel);

                    int fd = sceKernelOpen(to.c_str(),
                                           kBsdWronly | kBsdCreat | kBsdTrunc, 0777);
                    if (fd < 0) {
                        say("    " + e.rel + " - OPEN REFUSED " + Hex(fd) +
                            (existed ? " (OVERWRITE)" : " (CREATE)"));
                        refused++;
                        continue;
                    }

                    int n = data.empty() ? 0
                          : sceKernelWrite(fd, data.data(), data.size());
                    sceKernelClose(fd);

                    if (n != (int)data.size()) {
                        say("    " + e.rel + " - SHORT WRITE " +
                            std::to_string(n) + " OF " +
                            std::to_string((unsigned long long)data.size()));
                        shortW++;
                        continue;
                    }

                    wrote++;
                    if (existed) overwrote++; else created++;
                }

                say("  WROTE " + std::to_string(wrote) + " OF " +
                    std::to_string(source.size()) + " FILE(S)");
                say("  OVERWROTE " + std::to_string(overwrote) +
                    "  CREATED " + std::to_string(created) +
                    "  REFUSED " + std::to_string(refused) +
                    "  SHORT " + std::to_string(shortW));

                OrbisSaveDataMountPoint rp;
                memset(&rp, 0, sizeof(rp));
                memcpy(rp.data, rmr.mountPathName, sizeof(rp.data));
                int run = sceSaveDataUmount(&rp);
                say("  UNMOUNT " + Hex(run));
                if (run < 0) {
                    say("  UNMOUNT FAILED AFTER WRITING - DO NOT LAUNCH THE GAME YET");
                }
                say("  TOOK " + std::to_string((unsigned long long)((NowUs() - t0) / 1000)) +
                    " MS");
            }

            std::vector<TreeEntry> post2;
            uint64_t post2Bytes = 0;
            int      post2Errs  = 0;
            say("  READING CONTAINER AFTER");
            if (!SnapshotLiveSave(liveTitle, liveDir, userId, say,
                                  post2, post2Bytes, post2Errs)) {
                say("  COULD NOT READ IT BACK - THAT IS ITSELF A RESULT");
            } else {
                say("  BEFORE " + std::to_string(pre2.size()) + " FILE(S), " +
                    std::to_string((unsigned long long)pre2Bytes) + " BYTES");
                say("  AFTER  " + std::to_string(post2.size()) + " FILE(S), " +
                    std::to_string((unsigned long long)post2Bytes) + " BYTES");
                say("  SOURCE " + std::to_string(source.size()) + " FILE(S), " +
                    std::to_string((unsigned long long)sourceBytes) + " BYTES");
                say((post2.size() == source.size() && post2Bytes == sourceBytes)
                        ? "  CONTAINER NOW MATCHES THE BACKUP - LAUNCH THE GAME"
                        : "  CONTAINER DOES NOT MATCH THE BACKUP - SEE THE COUNTS");
            }
        }
    }

    say("");
    say("WRITE PROBE END");
}

void ProbeDeleteByPrefix(const std::string& configuredTitleId,
                         const std::string& prefix,
                         std::vector<std::string>& outLines) {
    auto say = [&outLines](const std::string& line) {
        outLines.push_back(line);
        Log(("savedel: " + line).c_str());
    };

    say("DELETE " + prefix + "* PROBE - DESTRUCTIVE");
    say("CONFIGURED TITLE " + configuredTitleId);
    say("");

    // --- identity ----------------------------------------------------------
    int32_t userId = -1;
    sceUserServiceInitialize(nullptr);
    if (sceUserServiceGetForegroundUser(&userId) < 0 &&
        sceUserServiceGetInitialUser(&userId) < 0) {
        say("NO USER - CANNOT CONTINUE");
        return;
    }
    say("USER ID " + std::to_string(userId));
    sceSaveDataInitialize3(0);

    // --- find the live save ------------------------------------------------
    std::string liveTitle, liveDir;
    for (int i = 0; i < kCandidateCount && liveTitle.empty(); i++) {
        OrbisSaveDataTitleId st;
        memset(&st, 0, sizeof(st));
        strncpy(st.data, kCandidates[i].id, sizeof(st.data) - 1);

        OrbisSaveDataDirNameSearchCond cond;
        memset(&cond, 0, sizeof(cond));
        cond.userId  = userId;
        cond.titleId = &st;

        OrbisSaveDataDirName dirs[16];
        memset(dirs, 0, sizeof(dirs));

        OrbisSaveDataDirNameSearchResult res;
        memset(&res, 0, sizeof(res));
        res.dirNames    = dirs;
        res.dirNamesNum = 16;

        if (sceSaveDataDirNameSearch(&cond, &res) < 0) continue;
        if (res.hitNum != 1) continue;          // never index into ambiguity

        liveTitle = kCandidates[i].id;
        liveDir   = Bounded(dirs[0].data, sizeof(dirs[0].data));
    }

    if (liveTitle.empty()) {
        say("NO LIVE SAVE FOUND - NOTHING TO DO");
        return;
    }
    say("LIVE SAVE " + liveTitle + " / " + liveDir);

    // --- bracket: before ---------------------------------------------------
    std::vector<TreeEntry> before;
    uint64_t beforeBytes = 0;
    int      beforeErrs  = 0;
    say("READING CONTAINER BEFORE");
    if (!SnapshotLiveSave(liveTitle, liveDir, userId, say,
                          before, beforeBytes, beforeErrs)) {
        say("COULD NOT READ IT - NOT PROCEEDING");
        return;
    }
    say("  BEFORE " + std::to_string(before.size()) + " FILE(S), " +
        std::to_string((unsigned long long)beforeBytes) + " BYTES");

    // --- unlink every userdata* at the mount root --------------------------
    OrbisSaveDataMount m;
    memset(&m, 0, sizeof(m));
    m.userId      = userId;
    m.titleId     = liveTitle.c_str();
    m.dirName     = liveDir.c_str();
    m.fingerprint = nullptr;
    m.blocks      = 0;
    m.mountMode   = ORBIS_SAVE_DATA_MOUNT_MODE_RDWR;

    OrbisSaveDataMountResult mr;
    memset(&mr, 0, sizeof(mr));

    int rc = sceSaveDataMount(&m, &mr);
    if (rc < 0) {
        say("RDWR MOUNT REFUSED " + Hex(rc));
        return;
    }

    std::string path = Bounded(mr.mountPathName, sizeof(mr.mountPathName));
    say("MOUNTED AT " + path);

    int removed = 0, refused = 0, kept = 0;
    for (size_t i = 0; i < before.size(); i++) {
        const std::string& rel = before[i].rel;

        // Root level only. A '/' means it is inside sce_sys, which stays
        // whatever prefix was asked for.
        bool match = rel.size() >= prefix.size() &&
                     rel.compare(0, prefix.size(), prefix) == 0 &&
                     rel.find('/') == std::string::npos;
        if (!match) { kept++; continue; }

        int urc = sceKernelUnlink((path + "/" + rel).c_str());
        if (urc < 0) {
            say("  " + rel + " - UNLINK REFUSED " + Hex(urc));
            refused++;
        } else {
            say("  " + rel + " - REMOVED");
            removed++;
        }
    }
    say("REMOVED " + std::to_string(removed) +
        "  REFUSED " + std::to_string(refused) +
        "  LEFT ALONE " + std::to_string(kept));

    OrbisSaveDataMountPoint mp;
    memset(&mp, 0, sizeof(mp));
    memcpy(mp.data, mr.mountPathName, sizeof(mp.data));
    int un = sceSaveDataUmount(&mp);
    say("UNMOUNT " + Hex(un));
    if (un < 0) say("UNMOUNT FAILED - DO NOT LAUNCH THE GAME YET");

    // --- bracket: after ----------------------------------------------------
    std::vector<TreeEntry> after;
    uint64_t afterBytes = 0;
    int      afterErrs  = 0;
    say("READING CONTAINER AFTER");
    if (!SnapshotLiveSave(liveTitle, liveDir, userId, say,
                          after, afterBytes, afterErrs)) {
        say("COULD NOT READ IT BACK - THAT IS ITSELF A RESULT");
    } else {
        say("  AFTER " + std::to_string(after.size()) + " FILE(S), " +
            std::to_string((unsigned long long)afterBytes) + " BYTES");
        for (size_t i = 0; i < after.size(); i++) say("    " + after[i].rel);
    }

    say("");
    if (removed > 0) {
        say("LAUNCH THE GAME AND SEE WHAT SURVIVED.");
        if (prefix == "userdata") {
            say("IF THERE ARE NO CHARACTERS, START FRESH NEEDS NO");
            say("SCESAVEDATADELETE AND NO BLANK SAVE TEMPLATE.");
            say("IF CHARACTERS SURVIVE, THE GAME REBUILT THEM FROM THE");
            say("BACKUP FILES - DELETE THOSE WITH L1 AND TRY AGAIN.");
        } else {
            say("THE GAME NOW HAS NOTHING TO FALL BACK ON.");
        }
        say("TO RECOVER - RESTORE A BACKUP WITH TRIANGLE. THAT NOW");
        say("REQUIRES CREATING FILES, WHICH HAS NEVER BEEN TESTED. IF IT");
        say("IS REFUSED, DELETE THE SAVE IN SETTINGS, LET THE GAME");
        say("REBUILD THE CONTAINER, THEN RESTORE.");
    }
    say("DELETE " + prefix + "* PROBE END");
}

} // namespace bbr
