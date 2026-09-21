#include "SaveDataProbe.h"

#include "Log.h"

#include <orbis/SaveData.h>
#include <orbis/UserService.h>
#include <orbis/libkernel.h>

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

} // namespace bbr
