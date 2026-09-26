#include "AfrManager.h"

#include "../Platform/Log.h"

#include <orbis/libkernel.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vector>

namespace bbr {

namespace {

const unsigned kModeIfmt  = 0xF000;
const unsigned kModeIfdir = 0x4000;

const char* const kManifestName = ".bbrandomizer_manifest";

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
const int kBsdRdonly = 0x0000;
const int kBsdWronly = 0x0001;
const int kBsdCreat  = 0x0200;
const int kBsdTrunc  = 0x0400;

// THE ONE PLACE THIS STRING EXISTS. Every path below is built from it, and
// nothing above this file is ever handed a raw AFR path.
std::string AfrRoot(const std::string& titleId) {
    return "/data/GoldHEN/AFR/" + titleId;
}

std::string DvdRoot(const std::string& titleId) {
    return AfrRoot(titleId) + "/dvdroot_ps4";
}

std::string ManifestPath(const std::string& titleId) {
    return DvdRoot(titleId) + "/" + kManifestName;
}

// The two names the swap moves trees between. Both sit BESIDE dvdroot_ps4
// inside the title's AFR folder rather than somewhere else on /data, because
// a rename is only atomic - and only cheap - within one filesystem, and a
// 6 GB copy across partitions is not a swap.
std::string StagingRoot(const std::string& titleId) {
    return AfrRoot(titleId) + "/dvdroot_ps4.staging";
}

std::string OldRoot(const std::string& titleId) {
    return AfrRoot(titleId) + "/dvdroot_ps4.old";
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

const int     kBsdDirectory = 0x00020000;
const uint8_t kDtDir        = 4;

struct DirEntryName {
    std::string name;
    bool        isDir = false;
};

std::vector<DirEntryName> ListDir(const std::string& path) {
    std::vector<DirEntryName> names;

    int fd = sceKernelOpen(path.c_str(), kBsdRdonly | kBsdDirectory, 0777);
    if (fd < 0) return names;

    char* buf = (char*)malloc(64 * 1024);
    if (!buf) { sceKernelClose(fd); return names; }

    for (;;) {
        int nread = sceKernelGetdents(fd, buf, 64 * 1024);
        if (nread <= 0) break;

        int pos = 0;
        while (pos < nread) {
            BsdDirent* de = (BsdDirent*)(buf + pos);
            if (de->d_reclen == 0) break;

            std::string name(de->d_name, de->d_namlen);
            if (name != "." && name != "..") {
                DirEntryName entry;
                entry.name  = name;
                entry.isDir = (de->d_type == kDtDir);
                names.push_back(entry);
            }
            pos += de->d_reclen;
        }
    }

    free(buf);
    sceKernelClose(fd);
    return names;
}

// Removes an ordinary directory tree. Only ever aimed at a dvdroot_ps4.old or
// a dvdroot_ps4.staging under this title's AFR folder - never at a save
// container, which is emptied through Platform/SaveData and never destroyed.
void RemoveTree(const std::string& path) {
    std::vector<DirEntryName> entries = ListDir(path);
    for (size_t i = 0; i < entries.size(); i++) {
        std::string child = path + "/" + entries[i].name;
        if (entries[i].isDir) RemoveTree(child);
        else                  sceKernelUnlink(child.c_str());
    }
    sceKernelRmdir(path.c_str());
}

std::string Hex(int value) {
    char buf[32];
    snprintf(buf, sizeof(buf), "0x%08X", (unsigned)value);
    return std::string(buf);
}

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

bool ReadSmallFile(const std::string& path, std::string& out) {
    out.clear();
    int fd = sceKernelOpen(path.c_str(), kBsdRdonly, 0);
    if (fd < 0) return false;

    char buf[2048];
    bool ok = true;
    for (;;) {
        int n = sceKernelRead(fd, buf, sizeof(buf));
        if (n < 0) { ok = false; break; }
        if (n > 0) {
            if (out.size() + (size_t)n > 16 * 1024) { ok = false; break; }
            out.append(buf, (size_t)n);
        }
        if ((size_t)n < sizeof(buf)) break;
    }
    sceKernelClose(fd);
    return ok;
}

bool WriteSmallFile(const std::string& path, const std::string& text) {
    int fd = sceKernelOpen(path.c_str(), kBsdWronly | kBsdCreat | kBsdTrunc, 0777);
    if (fd < 0) return false;
    int w = sceKernelWrite(fd, text.data(), text.size());
    sceKernelFsync(fd);
    sceKernelClose(fd);
    return w == (int)text.size();
}

} // namespace

AfrStatus AfrManager::Check(const std::string& titleId) {
    AfrStatus status;
    std::string afrRoot = AfrRoot(titleId);

    status.rootExists = IsDir(afrRoot);
    if (!status.rootExists) return status;

    status.writable = CanWrite(afrRoot);
    status.seeded   = IsDir(DvdRoot(titleId));
    if (status.seeded) {
        status.randomized = Exists(ManifestPath(titleId));
    }
    return status;
}

// ---------------------------------------------------------------------------
// .bbrandomizer_manifest
// ---------------------------------------------------------------------------

std::string AfrManager::FormatManifest(const AfrManifest& m) {
    char seed[32];
    snprintf(seed, sizeof(seed), "%u", (unsigned)m.seed);
    char account[32];
    snprintf(account, sizeof(account), "%llu", (unsigned long long)m.accountId);

    return std::string("world_id=") + m.worldId + "\n" +
           "world_name=" + m.worldName + "\n" +
           "revision=" + m.revision + "\n" +
           "seed=" + seed + "\n" +
           "title_id=" + m.titleId + "\n" +
           "account_id=" + account + "\n" +
           "written=" + m.written + "\n";
}

bool AfrManager::ParseManifest(const std::string& text, AfrManifest& out) {
    out = AfrManifest();

    bool sawWorld = false;
    size_t pos = 0;
    while (pos <= text.size()) {
        size_t nl = text.find('\n', pos);
        std::string line = text.substr(pos, nl == std::string::npos
                                                ? std::string::npos : nl - pos);
        if (!line.empty() && line[line.size() - 1] == '\r') line.resize(line.size() - 1);

        size_t eq = line.find('=');
        if (eq != std::string::npos) {
            std::string key = line.substr(0, eq);
            std::string val = line.substr(eq + 1);
            if      (key == "world_id")   { out.worldId = val; sawWorld = true; }
            else if (key == "world_name")   out.worldName = val;
            else if (key == "revision")     out.revision = val;
            else if (key == "seed")         out.seed = (uint32_t)strtoul(val.c_str(), nullptr, 10);
            else if (key == "title_id")     out.titleId = val;
            else if (key == "account_id")   out.accountId = strtoull(val.c_str(), nullptr, 10);
            else if (key == "written")      out.written = val;
            // Unknown keys ignored, the same tolerance defaults.cfg has.
        }

        if (nl == std::string::npos) break;
        pos = nl + 1;
    }

    // A manifest with no world_id names nothing, which is the one thing this
    // file exists to do. Treated as absent rather than as an empty world.
    if (!sawWorld || out.worldId.empty()) return false;
    out.present = true;
    return true;
}

bool AfrManager::ReadManifest(const std::string& titleId, AfrManifest& out) {
    out = AfrManifest();

    std::string text;
    if (!ReadSmallFile(ManifestPath(titleId), text)) return false;
    return ParseManifest(text, out);
}

bool AfrManager::WriteManifest(const std::string& titleId, const AfrManifest& manifest) {
    return WriteSmallFile(ManifestPath(titleId), FormatManifest(manifest));
}

// ---------------------------------------------------------------------------
// Which world is active
// ---------------------------------------------------------------------------

AfrActiveWorld AfrManager::DeriveActive(const AfrStatus& status,
                                        const AfrManifest& manifest,
                                        bool accountDirExists,
                                        bool worldIsKnown) {
    AfrActiveWorld out;

    // Checked first, and before anything about AFR is considered: a player
    // with no worlds folder has never used the randomizer, and the very first
    // thing that must happen for them is their live save being captured into
    // Vanilla - not a claim about which world is active.
    if (!accountDirExists) {
        out.state  = AfrActiveState::FirstRun;
        out.reason = "THE RANDOMIZER HAS NOT BEEN USED ON THIS ACCOUNT";
        return out;
    }

    if (manifest.present) {
        if (worldIsKnown) {
            out.state     = AfrActiveState::World;
            out.worldId   = manifest.worldId;
            out.worldName = manifest.worldName;
            out.revision  = manifest.revision;
            out.reason    = "THE GAME IS RUNNING THIS WORLD'S FILES";
            return out;
        }
        out.state  = AfrActiveState::Unmanaged;
        out.reason = "THE ACTIVE FILES NAME A WORLD THAT IS NOT HERE";
        return out;
    }

    // Seeded with no manifest. Randomizer files are present that this app did
    // not write, so saying "unmodified" would be a lie and saying "world X" a
    // guess. Every console that ran a build before this feature lands here.
    if (status.seeded) {
        out.state  = AfrActiveState::Unmanaged;
        out.reason = "RANDOMIZER FILES ARE PRESENT THAT THIS APP DID NOT WRITE";
        return out;
    }

    out.state  = AfrActiveState::Vanilla;
    out.reason = "NO RANDOMIZER FILES - THE GAME RUNS AS IT SHIPPED";
    return out;
}

// ---------------------------------------------------------------------------
// The activation swap
// ---------------------------------------------------------------------------

std::string AfrManager::StagingDvdroot(const std::string& titleId) {
    return StagingRoot(titleId);
}

bool AfrManager::WriteManifestAt(const std::string& dvdrootPath,
                                 const AfrManifest& manifest) {
    return WriteSmallFile(dvdrootPath + "/" + kManifestName, FormatManifest(manifest));
}

bool AfrManager::Swap(const std::string& titleId, bool useStaging, std::string& error) {
    error.clear();

    std::string live    = DvdRoot(titleId);
    std::string staging = StagingRoot(titleId);
    std::string old     = OldRoot(titleId);

    if (useStaging && !IsDir(staging)) {
        error = "NO STAGED TREE TO SWAP IN";
        return false;
    }

    // Debris from an interrupted run that reconciliation did not reach. It is
    // the previous occupant of dvdroot_ps4 and nothing depends on it once a
    // live tree exists, so it goes before the rename rather than colliding
    // with it.
    if (IsDir(old) && IsDir(live)) RemoveTree(old);

    bool hadLive = IsDir(live);
    if (hadLive) {
        int rc = sceKernelRename(live.c_str(), old.c_str());
        if (rc < 0) { error = "COULD NOT SET THE LIVE TREE ASIDE " + Hex(rc); return false; }
        Log(("afr: " + live + " -> dvdroot_ps4.old").c_str());
    }

    if (useStaging) {
        int rc = sceKernelRename(staging.c_str(), live.c_str());
        if (rc < 0) {
            // Put the console back exactly as it was. The staged tree stays
            // where it is: reconciliation removes it, and a half-swapped AFR
            // folder is the one state this rename exists to avoid.
            if (hadLive) sceKernelRename(old.c_str(), live.c_str());
            error = "COULD NOT SWAP THE NEW TREE IN " + Hex(rc);
            return false;
        }
        Log(("afr: dvdroot_ps4.staging -> " + live).c_str());
    }

    if (hadLive) RemoveTree(old);
    return true;
}

bool AfrManager::StagingExists(const std::string& titleId) {
    return IsDir(StagingRoot(titleId));
}

void AfrManager::RemoveStaging(const std::string& titleId) {
    if (!IsDir(StagingRoot(titleId))) return;
    Log(("afr: removing " + StagingRoot(titleId)).c_str());
    RemoveTree(StagingRoot(titleId));
}

bool AfrManager::RollBackSwap(const std::string& titleId, bool& outRestored) {
    outRestored = false;

    std::string live = DvdRoot(titleId);
    std::string old  = OldRoot(titleId);

    if (!IsDir(old)) return true;        // the swap completed, or never began
    if (IsDir(live)) {
        // Both names exist: the second rename landed and only the cleanup was
        // lost. Finish the cleanup.
        RemoveTree(old);
        return true;
    }

    int rc = sceKernelRename(old.c_str(), live.c_str());
    if (rc < 0) return false;
    outRestored = true;
    Log(("afr: dvdroot_ps4.old -> " + live).c_str());
    return true;
}

} // namespace bbr
