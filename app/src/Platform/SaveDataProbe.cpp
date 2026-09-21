#include "SaveDataProbe.h"

#include "Log.h"

#include <orbis/SaveData.h>
#include <orbis/UserService.h>
#include <orbis/libkernel.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <vector>

namespace bbr {

namespace {

// Same BSD values FileIo.cpp proved on this kernel. Duplicated rather than
// shared because this file is temporary and FileIo's copies are file-local; a
// probe that drags a refactor in with it is a probe that is hard to delete.
const int kBsdRdonly    = 0x0000;
const int kBsdDirectory = 0x00020000;

struct BsdDirent {
    uint32_t d_fileno;
    uint16_t d_reclen;
    uint8_t  d_type;
    uint8_t  d_namlen;
    char     d_name[256];
};

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
                   std::vector<std::string>& outLines) {
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
    int fd = sceKernelOpen(mountPath.c_str(), kBsdRdonly | kBsdDirectory, 0777);
    if (fd < 0) {
        say("OPEN MOUNT DIR FAILED " + Hex(fd));
    } else {
        const int kBufSize = 64 * 1024;
        char* buf = (char*)malloc(kBufSize);
        int   files = 0;

        if (!buf) {
            say("OUT OF MEMORY LISTING MOUNT");
        } else {
            for (;;) {
                int nread = sceKernelGetdents(fd, buf, kBufSize);
                if (nread <= 0) break;

                int pos = 0;
                while (pos < nread) {
                    BsdDirent* de = (BsdDirent*)(buf + pos);
                    if (de->d_reclen == 0) break;

                    std::string name(de->d_name, de->d_namlen);
                    if (name != "." && name != "..") {
                        files++;

                        // Read the file THROUGH, counting bytes, rather than
                        // sampling the first few. Two reasons: it proves the
                        // entire file is reachable, which is precisely what a
                        // copy needs to know; and sceKernelStat's st_size is
                        // not usable here - on real hardware it returned 88 for
                        // a 44KB file, because this SDK's stat struct does not
                        // match the kernel ABI (see Randomizer/FileIo.cpp). A
                        // short read is a reliable EOF for a regular file.
                        std::string full = mountPath + "/" + name;
                        int ffd = sceKernelOpen(full.c_str(), kBsdRdonly, 0);
                        if (ffd < 0) {
                            say("  FILE " + name + " - OPEN FAILED " + Hex(ffd));
                        } else {
                            const size_t kChunk = 65536;
                            char*     chunk = (char*)malloc(kChunk);
                            uint64_t  total = 0;
                            bool      readError = false;

                            if (!chunk) {
                                readError = true;
                            } else {
                                for (;;) {
                                    int n = sceKernelRead(ffd, chunk, kChunk);
                                    if (n < 0) { readError = true; break; }
                                    total += (uint64_t)n;
                                    if ((size_t)n < kChunk) break;   // EOF
                                }
                                free(chunk);
                            }
                            sceKernelClose(ffd);

                            if (readError) {
                                say("  FILE " + name + " - READ ERROR AFTER " +
                                    std::to_string((unsigned long long)total) + " BYTE(S)");
                            } else {
                                say("  FILE " + name + " - " +
                                    std::to_string((unsigned long long)total) +
                                    " BYTES READ OK");
                            }
                        }
                    }
                    pos += de->d_reclen;
                }
            }
            free(buf);
        }

        sceKernelClose(fd);
        say("LISTED " + std::to_string(files) + " ENTRY(S) AT MOUNT ROOT");
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
