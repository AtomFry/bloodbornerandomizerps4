#include "WorldStore.h"

#include "RandomizerDefaultsStore.h"

#include "../Platform/Log.h"

#include <orbis/libkernel.h>

#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

namespace bbr {

const char* const kVanillaWorldId   = "vanilla";
const char* const kVanillaWorldName = "VANILLA";

namespace {

// Beside defaults.cfg, on the partition this app already writes to -
// deliberately NOT inside AFR, which activation rewrites wholesale and would
// take the worlds and the backups with it.
const char* const kWorldsRoot      = "/data/bbrandomizer/Worlds";
const char* const kSaveBackupsRoot = "/data/bbrandomizer/SaveBackups";

// Same BSD open(2) flag values proven correct for sceKernelOpen elsewhere in
// this project (Platform.cpp, Game/AfrManager.cpp, Randomizer/FileIo.cpp) -
// musl's <fcntl.h> values are Linux's and don't match this FreeBSD-derived
// kernel. Full table in docs/ps4-homebrew-findings.md.
const int kBsdRdonly    = 0x0000;
const int kBsdWronly    = 0x0001;
const int kBsdCreat     = 0x0200;
const int kBsdTrunc     = 0x0400;
const int kBsdDirectory = 0x00020000;

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

const uint8_t kDtDir = 4;
const size_t  kChunk = 65536;

const unsigned kModeIfmt  = 0xF000;
const unsigned kModeIfdir = 0x4000;

std::string U64Text(uint64_t value) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%llu", (unsigned long long)value);
    return std::string(buf);
}

std::string Hex(int value) {
    char buf[32];
    snprintf(buf, sizeof(buf), "0x%08X", (unsigned)value);
    return std::string(buf);
}

// "YYYY-MM-DD HH:MM:SS" - what world.cfg and a manifest record, and what the
// rail sorts on. Fixed-width and most-significant-first on purpose: sorting it
// as text is sorting it as time, so no date parsing exists anywhere in here.
std::string NowStamp() {
    time_t now = time(nullptr);
    struct tm* lt = localtime(&now);
    char buf[32];
    if (!lt || strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", lt) == 0) {
        return "(UNKNOWN)";
    }
    return std::string(buf);
}

// "YYYYMMDD-HHMMSS" - the same instant, in a form that can go in a directory
// name.
std::string PathStamp() {
    time_t now = time(nullptr);
    struct tm* lt = localtime(&now);
    char buf[32];
    if (!lt || strftime(buf, sizeof(buf), "%Y%m%d-%H%M%S", lt) == 0) return "NOSTAMP";
    return std::string(buf);
}

uint64_t NowUs() { return sceKernelGetProcessTime(); }

bool IsDir(const std::string& path) {
    OrbisKernelStat st;
    if (sceKernelStat(path.c_str(), &st) != 0) return false;
    return ((unsigned)st.st_mode & kModeIfmt) == kModeIfdir;
}

bool FileExists(const std::string& path) {
    int fd = sceKernelOpen(path.c_str(), kBsdRdonly, 0);
    if (fd < 0) return false;
    sceKernelClose(fd);
    return true;
}

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

void MakeParentDirs(const std::string& root, const std::string& rel) {
    size_t start = 0;
    for (;;) {
        size_t slash = rel.find('/', start);
        if (slash == std::string::npos) break;
        sceKernelMkdir((root + "/" + rel.substr(0, slash)).c_str(), 0777);
        start = slash + 1;
    }
}

struct DirEntryName {
    std::string name;
    bool        isDir = false;
};

// Entry names directly under `path`, skipping "." and "..". Empty on any error,
// including `path` not being a directory.
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

// Removes an ordinary directory tree under /data. Only ever aimed at this
// app's own directories - a world, a .partial, a .old. Never at a save
// container: those are emptied through Platform/SaveData and never destroyed,
// because this app cannot create one.
void RemoveTree(const std::string& path) {
    std::vector<DirEntryName> entries = ListDir(path);
    for (size_t i = 0; i < entries.size(); i++) {
        std::string child = path + "/" + entries[i].name;
        if (entries[i].isDir) RemoveTree(child);
        else                  sceKernelUnlink(child.c_str());
    }
    sceKernelRmdir(path.c_str());
}

// Reads a small text file whole. Capped, because everything read through here
// is a config of a few hundred bytes and a runaway file is a bug, not input.
bool ReadTextFile(const std::string& path, std::string& out) {
    out.clear();
    int fd = sceKernelOpen(path.c_str(), kBsdRdonly, 0);
    if (fd < 0) return false;

    char buf[4096];
    bool ok = true;
    for (;;) {
        int n = sceKernelRead(fd, buf, sizeof(buf));
        if (n < 0) { ok = false; break; }
        if (n > 0) {
            if (out.size() + (size_t)n > 64 * 1024) { ok = false; break; }
            out.append(buf, (size_t)n);
        }
        if ((size_t)n < sizeof(buf)) break;
    }
    sceKernelClose(fd);
    return ok;
}

bool WriteTextFile(const std::string& path, const std::string& text) {
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
    // fsync before close: a world.cfg or a revision that exists but is empty
    // after a power cut is worse than one that is not there at all.
    sceKernelFsync(fd);
    sceKernelClose(fd);
    return ok;
}

// Copies one file, reading through in chunks to a short read. NOT by st_size,
// which returned 88 for a genuine 44 KB file on this kernel (findings 9). A
// short write fails the copy rather than being retried blindly.
bool CopyFileThrough(const std::string& src, const std::string& dst,
                     uint64_t* outBytes, std::string& error) {
    *outBytes = 0;

    int in = sceKernelOpen(src.c_str(), kBsdRdonly, 0);
    if (in < 0) { error = "OPEN FAILED " + Hex(in); return false; }

    int out = sceKernelOpen(dst.c_str(), kBsdWronly | kBsdCreat | kBsdTrunc, 0777);
    if (out < 0) {
        sceKernelClose(in);
        error = "DEST OPEN REFUSED " + Hex(out);
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
        if (n < 0) { ok = false; error = "READ FAILED " + Hex(n); break; }
        if (n > 0) {
            int w = sceKernelWrite(out, buf, n);
            if (w != n) {
                ok = false;
                error = "SHORT WRITE " + U64Text((uint64_t)(w < 0 ? 0 : w)) +
                        " OF " + U64Text((uint64_t)n);
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

// --- the config files ------------------------------------------------------

// Every config in this feature is the same "key=value" text defaults.cfg is,
// for the same reason: it can be read over FTP with any text editor when
// something has gone wrong on a console that cannot be debugged.
void ForEachConfigLine(const std::string& text,
                       void (*fn)(const std::string&, const std::string&, void*),
                       void* ctx) {
    size_t pos = 0;
    while (pos <= text.size()) {
        size_t nl = text.find('\n', pos);
        std::string line = text.substr(pos, nl == std::string::npos
                                                ? std::string::npos : nl - pos);
        if (!line.empty() && line[line.size() - 1] == '\r') line.resize(line.size() - 1);

        size_t eq = line.find('=');
        if (eq != std::string::npos) fn(line.substr(0, eq), line.substr(eq + 1), ctx);

        if (nl == std::string::npos) break;
        pos = nl + 1;
    }
}

void ApplyWorldCfgLine(const std::string& key, const std::string& value, void* ctx) {
    World& w = *(World*)ctx;
    if      (key == "name")                 w.name = value;
    else if (key == "created")              w.created = value;
    else if (key == "last_played")          w.lastPlayed = value;
    else if (key == "last_played_revision") w.lastPlayedRevision = value;
    else if (key == "account_id")           w.accountId = strtoull(value.c_str(), nullptr, 10);
    // Unknown keys ignored, the same tolerance defaults.cfg has.
}

std::string FormatWorldCfg(const World& w) {
    return "name=" + w.name + "\n" +
           "created=" + w.created + "\n" +
           "last_played=" + w.lastPlayed + "\n" +
           "last_played_revision=" + w.lastPlayedRevision + "\n" +
           "account_id=" + U64Text(w.accountId) + "\n";
}

struct RevisionParse {
    WorldRecipe* recipe;
};

void ApplyRevisionLine(const std::string& key, const std::string& value, void* ctx) {
    RevisionParse& p = *(RevisionParse*)ctx;
    if (key == "seed") {
        // strtoul, not atoi: a seed can exceed INT_MAX.
        p.recipe->seed = (uint32_t)strtoul(value.c_str(), nullptr, 10);
        return;
    }
    // Everything else is a setting, read by the same chain defaults.cfg uses,
    // so a setting cannot round-trip through one file and not the other.
    ApplySettingKey(key.c_str(), value.c_str(), p.recipe->settings);
}

std::string FormatRevision(const WorldRecipe& recipe) {
    char head[32];
    int hn = snprintf(head, sizeof(head), "seed=%u\n", (unsigned)recipe.seed);
    if (hn < 0) return std::string();
    if ((size_t)hn > sizeof(head)) hn = (int)sizeof(head);
    return std::string(head, (size_t)hn) + FormatSettings(recipe.settings);
}

std::string NumberedId(const char* prefix, int n) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%s%04d", prefix, n);
    return std::string(buf);
}

// "rev-0007" -> 7, and anything that is not that shape -> 0.
int RevisionNumber(const std::string& fileName) {
    if (fileName.size() != 12) return 0;                    // rev-NNNN.cfg
    if (fileName.compare(0, 4, "rev-") != 0) return 0;
    if (fileName.compare(8, 4, ".cfg") != 0) return 0;
    for (int i = 4; i < 8; i++) {
        if (fileName[i] < '0' || fileName[i] > '9') return 0;
    }
    return atoi(fileName.substr(4, 4).c_str());
}

// "w-0007" -> 7, and anything that is not that shape -> 0. Vanilla's id is the
// literal "vanilla" and deliberately never matches.
int WorldNumber(const std::string& dirName) {
    if (dirName.size() != 6) return 0;
    if (dirName.compare(0, 2, "w-") != 0) return 0;
    for (int i = 2; i < 6; i++) {
        if (dirName[i] < '0' || dirName[i] > '9') return 0;
    }
    return atoi(dirName.substr(2, 4).c_str());
}

bool HasSuffix(const std::string& s, const char* suffix) {
    size_t n = strlen(suffix);
    return s.size() >= n && s.compare(s.size() - n, n, suffix) == 0;
}

// Rail order: last_played descending, then never-played by created
// descending. The id is the final tiebreak purely so the order is stable -
// std::sort is not, and a list that reshuffles between frames is unusable.
bool ByRailOrder(const World& a, const World& b) {
    bool ap = !a.lastPlayed.empty();
    bool bp = !b.lastPlayed.empty();
    if (ap != bp) return ap;
    const std::string& ka = ap ? a.lastPlayed : a.created;
    const std::string& kb = bp ? b.lastPlayed : b.created;
    if (ka != kb) return ka > kb;
    return a.id > b.id;
}

} // namespace

// ---------------------------------------------------------------------------
// Recipes and names
// ---------------------------------------------------------------------------

bool SameRecipe(const WorldRecipe& a, const WorldRecipe& b) {
    return a.seed == b.seed && FormatSettings(a.settings) == FormatSettings(b.settings);
}

std::string NormalizeWorldName(const std::string& name) {
    std::string out;
    for (size_t i = 0; i < name.size() && out.size() < 16; i++) {
        char c = name[i];
        if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
        bool ok = (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' ';
        if (ok) out.push_back(c);
    }
    // Trailing spaces would be invisible in the rail and would make two names
    // that look identical compare differently.
    while (!out.empty() && out[out.size() - 1] == ' ') out.resize(out.size() - 1);
    return out;
}

// ---------------------------------------------------------------------------
// Safety backups
// ---------------------------------------------------------------------------

std::string SafetyBackupPath(const std::string& titleId, const std::string& dirName,
                             const std::string& reason) {
    return std::string(kSaveBackupsRoot) + "/" + titleId + "_" + dirName + "_" +
           PathStamp() + "_" + reason;
}

int SweepPartialBackups() {
    int swept = 0;
    std::vector<DirEntryName> entries = ListDir(kSaveBackupsRoot);
    for (size_t i = 0; i < entries.size(); i++) {
        if (!entries[i].isDir) continue;
        if (!HasSuffix(entries[i].name, ".partial")) continue;
        RemoveTree(std::string(kSaveBackupsRoot) + "/" + entries[i].name);
        Log(("worldstore: swept " + entries[i].name).c_str());
        swept++;
    }
    return swept;
}

struct SafetyBackupJob::State {
    std::unique_ptr<savedata::BackupJob> backup;
    std::string destDir;
    int         phase = 0;   // 0 copying, 1 verifying, 2 done
    uint64_t    startUs = 0;
    std::string status = "STARTING";
    SafetyBackupResult result;

    void Fail(const std::string& why) {
        result.ok    = false;
        result.error = why;
        status       = why;
        Log(("worldstore: safety backup failed - " + why).c_str());
        phase = 2;
    }
};

SafetyBackupJob::SafetyBackupJob(const savedata::User& user, const std::string& titleId,
                                 const std::string& dirName, const std::string& reason,
                                 const std::string& world, const std::string& revision)
    : s_(new State()) {
    s_->startUs = NowUs();
    s_->destDir = SafetyBackupPath(titleId, dirName, reason);
    MakeDirs(kSaveBackupsRoot);
    s_->backup.reset(new savedata::BackupJob(user, titleId, dirName, s_->destDir,
                                             world, revision));
}

SafetyBackupJob::~SafetyBackupJob() = default;

bool SafetyBackupJob::Done() const { return s_->phase >= 2; }
std::string SafetyBackupJob::StatusText() const { return s_->status; }
const SafetyBackupResult& SafetyBackupJob::Result() const { return s_->result; }

float SafetyBackupJob::Progress() const {
    if (s_->phase >= 2) return 1.0f;
    if (!s_->backup) return 0.0f;
    return s_->backup->Progress() * 0.9f;
}

void SafetyBackupJob::Step() {
    State& s = *s_;
    if (s.phase >= 2) return;

    if (s.phase == 0) {
        s.backup->Step();
        s.status = s.backup->StatusText();
        if (!s.backup->Done()) return;

        const savedata::BackupResult& r = s.backup->Result();
        if (!r.ok) { s.Fail(r.error); return; }
        s.result.files = r.files;
        s.result.bytes = r.bytes;
        s.phase = 1;
        return;
    }

    // A backup nothing has checked is not a backup. This walks what actually
    // landed on disk and compares it against the manifest beside it - count,
    // path, per-file size and total - rather than believing the copy.
    std::string error;
    savedata::Manifest manifest;
    if (!savedata::VerifyBackup(s.destDir, error, &manifest)) {
        s.Fail("VERIFY FAILED - " + error);
        return;
    }

    s.result.ok        = true;
    s.result.path      = s.destDir;
    s.result.manifest  = manifest;
    s.result.elapsedMs = (NowUs() - s.startUs) / 1000;
    s.status = "SAFETY BACKUP VERIFIED - " + U64Text(manifest.files) + " FILE(S), " +
               U64Text(manifest.bytes) + " BYTES";
    s.phase = 2;
}

// ---------------------------------------------------------------------------
// Storing a save into a world
// ---------------------------------------------------------------------------

struct StoreSaveJob::State {
    std::string srcDir;
    std::string destDir;
    std::string partialDir;
    std::string oldDir;
    std::string worldId;
    std::string revisionId;

    std::vector<savedata::FileEntry> files;
    size_t      next = 0;
    int         phase = 0;   // 0 prepare, 1 copying, 2 swap, 3 verify, 4 done
    uint64_t    startUs = 0;
    std::string status = "STARTING";
    StoreSaveResult result;

    void Fail(const std::string& why) {
        result.ok    = false;
        result.error = why;
        status       = why;
        Log(("worldstore: store save failed - " + why).c_str());
        phase = 4;
    }
};

StoreSaveJob::StoreSaveJob(const std::string& srcBackupDir, const std::string& worldSaveDir,
                           const std::string& worldId, const std::string& revisionId)
    : s_(new State()) {
    s_->srcDir     = srcBackupDir;
    s_->destDir    = worldSaveDir;
    s_->partialDir = worldSaveDir + ".partial";
    s_->oldDir     = worldSaveDir + ".old";
    s_->worldId    = worldId;
    s_->revisionId = revisionId;
    s_->startUs    = NowUs();
}

StoreSaveJob::~StoreSaveJob() = default;

bool StoreSaveJob::Done() const { return s_->phase >= 4; }
std::string StoreSaveJob::StatusText() const { return s_->status; }
const StoreSaveResult& StoreSaveJob::Result() const { return s_->result; }

float StoreSaveJob::Progress() const {
    if (s_->phase >= 4) return 1.0f;
    if (s_->files.empty()) return 0.0f;
    return (float)s_->next / (float)(s_->files.size() + 2);
}

void StoreSaveJob::Step() {
    State& s = *s_;
    if (s.phase >= 4) return;

    if (s.phase == 0) {
        // The source is re-verified here, not taken on trust: this job is the
        // one way a save gets into a world, so nothing that already disagrees
        // with its own manifest is allowed to become a world's save.
        std::string error;
        if (!savedata::VerifyBackup(s.srcDir, error, &s.result.manifest)) {
            s.Fail("SOURCE VERIFY FAILED - " + error);
            return;
        }

        // A .partial or .old left by an earlier run is debris, not a starting
        // point - copying into one would mix two saves together.
        RemoveTree(s.partialDir);
        RemoveTree(s.oldDir);
        MakeDirs(s.partialDir + "/data");

        s.files = s.result.manifest.entries;
        if (s.files.empty()) { s.Fail("THE SOURCE HOLDS NO FILES"); return; }

        s.status = "COPYING " + U64Text((uint64_t)s.files.size()) + " FILE(S)";
        s.phase  = 1;
        return;
    }

    if (s.phase == 1) {
        const savedata::FileEntry& e = s.files[s.next];
        MakeParentDirs(s.partialDir + "/data", e.rel);

        uint64_t bytes = 0;
        std::string error;
        if (!CopyFileThrough(s.srcDir + "/data/" + e.rel,
                             s.partialDir + "/data/" + e.rel, &bytes, error)) {
            s.Fail(e.rel + " - " + error);
            return;
        }
        if (bytes != e.bytes) {
            s.Fail(e.rel + " - COPIED " + U64Text(bytes) + " OF " + U64Text(e.bytes) +
                   " BYTES");
            return;
        }

        s.result.files++;
        s.result.bytes += bytes;
        s.status = e.rel + " - " + U64Text(bytes) + " BYTES COPIED";
        s.next++;
        if (s.next >= s.files.size()) s.phase = 2;
        return;
    }

    if (s.phase == 2) {
        // The manifest is written LAST and the directory renamed after it, so
        // the only two states a reader can see are "no save" and "a complete
        // save". The world's previous save goes to .old first and is removed
        // only once the new one is in place, so a failed rename leaves the old
        // one recoverable rather than nothing at all.
        savedata::Manifest m = s.result.manifest;
        m.world    = s.worldId;
        m.revision = s.revisionId;
        if (!savedata::WriteManifestFile(s.partialDir + "/manifest.txt", m)) {
            s.Fail("COULD NOT WRITE THE MANIFEST");
            return;
        }

        bool hadOld = IsDir(s.destDir);
        if (hadOld) {
            int rc = sceKernelRename(s.destDir.c_str(), s.oldDir.c_str());
            if (rc < 0) { s.Fail("COULD NOT SET THE OLD SAVE ASIDE " + Hex(rc)); return; }
        }

        int rc = sceKernelRename(s.partialDir.c_str(), s.destDir.c_str());
        if (rc < 0) {
            // Put the world back exactly as it was before failing.
            if (hadOld) sceKernelRename(s.oldDir.c_str(), s.destDir.c_str());
            s.Fail("RENAME INTO PLACE FAILED " + Hex(rc));
            return;
        }
        if (hadOld) RemoveTree(s.oldDir);

        s.result.manifest = m;
        s.status = "STORED - " + U64Text(s.result.files) + " FILE(S)";
        s.phase  = 3;
        return;
    }

    // phase 3: confirm what is now on disk, as a walk of the destination.
    std::string error;
    if (!savedata::VerifyBackup(s.destDir, error, nullptr)) {
        s.Fail("STORED SAVE VERIFY FAILED - " + error);
        return;
    }

    s.result.ok        = true;
    s.result.elapsedMs = (NowUs() - s.startUs) / 1000;
    s.status = "SAVE STORED AND VERIFIED - " + U64Text(s.result.files) + " FILE(S), " +
               U64Text(s.result.bytes) + " BYTES";
    s.phase = 4;
}

// ---------------------------------------------------------------------------
// WorldStore
// ---------------------------------------------------------------------------

WorldStore::WorldStore(uint64_t accountId) : accountId_(accountId) {
    char buf[32];
    snprintf(buf, sizeof(buf), "acct-%016llx", (unsigned long long)accountId);
    accountDir_ = std::string(kWorldsRoot) + "/" + buf;
}

std::string WorldStore::WorldDir(const std::string& worldId) const {
    return accountDir_ + "/" + worldId;
}

std::string WorldStore::SaveDir(const std::string& worldId) const {
    return WorldDir(worldId) + "/save";
}

bool WorldStore::AccountDirExists() const { return IsDir(accountDir_); }

bool WorldStore::CreateAccount(std::string& error) {
    error.clear();
    if (AccountDirExists()) { error = "THE ACCOUNT ALREADY HAS WORLDS"; return false; }

    MakeDirs(accountDir_);
    if (!AccountDirExists()) { error = "COULD NOT CREATE " + accountDir_; return false; }

    if (!WriteNextWorldId(1)) { error = "COULD NOT WRITE WORLDS.CFG"; return false; }

    World vanilla;
    vanilla.id        = kVanillaWorldId;
    vanilla.name      = kVanillaWorldName;
    vanilla.created   = NowStamp();
    vanilla.accountId = accountId_;
    vanilla.isVanilla = true;
    MakeDirs(WorldDir(vanilla.id));
    if (!WriteWorldCfg(vanilla)) { error = "COULD NOT WRITE VANILLA"; return false; }

    Log(("worldstore: created " + accountDir_).c_str());
    return true;
}

void WorldStore::DestroyAccount() {
    Log(("worldstore: removing " + accountDir_).c_str());
    RemoveTree(accountDir_);
}

int WorldStore::NextWorldId() const {
    int next = 1;

    std::string text;
    if (ReadTextFile(accountDir_ + "/worlds.cfg", text)) {
        size_t at = text.find("next_world_id=");
        if (at != std::string::npos) {
            int stored = atoi(text.c_str() + at + strlen("next_world_id="));
            if (stored > next) next = stored;
        }
    }

    // max(next_world_id, highest existing + 1). The file is the record, but the
    // directory is the truth: an id is never reused, so a worlds.cfg that lost
    // a write must not hand out one that already exists.
    std::vector<DirEntryName> entries = ListDir(accountDir_);
    for (size_t i = 0; i < entries.size(); i++) {
        int n = WorldNumber(entries[i].name);
        if (n >= next) next = n + 1;
    }
    return next;
}

bool WorldStore::WriteNextWorldId(int next) const {
    char buf[64];
    snprintf(buf, sizeof(buf), "next_world_id=%d\n", next);
    return WriteTextFile(accountDir_ + "/worlds.cfg", buf);
}

bool WorldStore::ReadWorldCfg(const std::string& worldId, World& out) const {
    std::string text;
    if (!ReadTextFile(WorldDir(worldId) + "/world.cfg", text)) return false;

    out = World();
    out.id        = worldId;
    out.isVanilla = (worldId == kVanillaWorldId);
    ForEachConfigLine(text, ApplyWorldCfgLine, &out);
    return true;
}

bool WorldStore::WriteWorldCfg(const World& world) const {
    return WriteTextFile(WorldDir(world.id) + "/world.cfg", FormatWorldCfg(world));
}

bool WorldStore::Exists(const std::string& worldId) const {
    return IsDir(WorldDir(worldId)) && FileExists(WorldDir(worldId) + "/world.cfg");
}

bool WorldStore::Load(const std::string& worldId, World& out) const {
    if (!ReadWorldCfg(worldId, out)) return false;

    // Everything below is derived from the disk every time it is asked for,
    // never cached in world.cfg: a count or a size that drifts from what is
    // beside it is worse than one that costs a directory listing.
    if (!out.isVanilla) {
        std::vector<DirEntryName> entries = ListDir(WorldDir(worldId));
        int highest = 0;
        for (size_t i = 0; i < entries.size(); i++) {
            int n = RevisionNumber(entries[i].name);
            if (n <= 0) continue;
            out.revisionCount++;
            if (n > highest) highest = n;
        }
        if (highest > 0) out.currentRevision = NumberedId("rev-", highest);
    }

    savedata::Manifest m;
    std::string error;
    if (savedata::ReadManifestFile(SaveDir(worldId) + "/manifest.txt", m, error)) {
        out.hasSave      = true;
        out.saveFiles    = m.files;
        out.saveBytes    = m.bytes;
        out.saveBlocks   = m.blocks;
        out.saveCaptured = m.captured;
        out.saveRevision = m.revision;
    }
    return true;
}

std::vector<World> WorldStore::List() const {
    std::vector<World> worlds;

    std::vector<DirEntryName> entries = ListDir(accountDir_);
    for (size_t i = 0; i < entries.size(); i++) {
        if (!entries[i].isDir) continue;
        if (WorldNumber(entries[i].name) <= 0) continue;   // vanilla, debris, .partial

        World w;
        if (Load(entries[i].name, w)) worlds.push_back(w);
    }

    std::sort(worlds.begin(), worlds.end(), ByRailOrder);
    return worlds;
}

bool WorldStore::Create(const std::string& name, const WorldRecipe& recipe,
                        World& out, std::string& error) {
    error.clear();
    if (!AccountDirExists()) { error = "THIS ACCOUNT HAS NO WORLDS FOLDER YET"; return false; }

    std::string id = NumberedId("w-", NextWorldId());
    if (IsDir(WorldDir(id))) { error = "WORLD ID " + id + " ALREADY EXISTS"; return false; }

    out = World();
    out.id        = id;
    out.name      = NormalizeWorldName(name);
    out.created   = NowStamp();
    out.accountId = accountId_;

    MakeDirs(WorldDir(id));
    if (!WriteWorldCfg(out)) { error = "COULD NOT WRITE WORLD.CFG"; return false; }

    std::string revId = NumberedId("rev-", 1);
    if (!WriteTextFile(WorldDir(id) + "/" + revId + ".cfg", FormatRevision(recipe))) {
        error = "COULD NOT WRITE " + revId;
        return false;
    }
    out.currentRevision = revId;
    out.revisionCount   = 1;

    // Written after the world exists: an id handed out and then not used is
    // simply skipped, which is harmless, whereas one reused is not.
    WriteNextWorldId(WorldNumber(id) + 1);

    Log(("worldstore: created world " + id).c_str());
    return true;
}

bool WorldStore::Rename(const std::string& worldId, const std::string& name,
                        std::string& error) {
    error.clear();
    if (worldId == kVanillaWorldId) { error = "VANILLA CANNOT BE RENAMED"; return false; }

    World w;
    if (!ReadWorldCfg(worldId, w)) { error = "NO SUCH WORLD - " + worldId; return false; }

    w.name = NormalizeWorldName(name);
    if (!WriteWorldCfg(w)) { error = "COULD NOT WRITE WORLD.CFG"; return false; }
    return true;
}

bool WorldStore::SetLastPlayed(const std::string& worldId, const std::string& revisionId,
                               std::string& error) {
    error.clear();

    World w;
    if (!ReadWorldCfg(worldId, w)) { error = "NO SUCH WORLD - " + worldId; return false; }

    w.lastPlayed         = NowStamp();
    w.lastPlayedRevision = revisionId;
    if (!WriteWorldCfg(w)) { error = "COULD NOT WRITE WORLD.CFG"; return false; }
    return true;
}

bool WorldStore::Delete(const std::string& worldId, std::string& error,
                        std::string* outKeptBackupPath) {
    error.clear();
    if (outKeptBackupPath) outKeptBackupPath->clear();
    if (worldId == kVanillaWorldId) { error = "VANILLA CANNOT BE DELETED"; return false; }
    if (!Exists(worldId)) { error = "NO SUCH WORLD - " + worldId; return false; }

    // The save is kept as a safety backup, always (spec worlds D9). It already
    // has a backup's exact shape - manifest.txt beside data/ - so keeping it is
    // a rename, not a copy, and costs nothing.
    if (IsDir(SaveDir(worldId))) {
        savedata::Manifest m;
        std::string readError;
        std::string title = "UNKNOWN";
        std::string dir   = "UNKNOWN";
        if (savedata::ReadManifestFile(SaveDir(worldId) + "/manifest.txt", m, readError)) {
            title = m.title_id;
            dir   = m.dir_name;
        }

        std::string kept = SafetyBackupPath(title, dir, "deleted-" + worldId);
        MakeDirs(kSaveBackupsRoot);
        int rc = sceKernelRename(SaveDir(worldId).c_str(), kept.c_str());
        if (rc < 0) {
            // Refuse rather than destroy: the whole point of deleting is that
            // the save survives it.
            error = "COULD NOT KEEP THE SAVE " + Hex(rc);
            return false;
        }
        if (outKeptBackupPath) *outKeptBackupPath = kept;
        Log(("worldstore: kept " + worldId + "'s save as " + kept).c_str());
    }

    RemoveTree(WorldDir(worldId));
    Log(("worldstore: deleted world " + worldId).c_str());
    return true;
}

// --- revisions -------------------------------------------------------------

bool WorldStore::LoadRevision(const std::string& worldId, const std::string& revisionId,
                              WorldRevision& out) const {
    std::string text;
    if (!ReadTextFile(WorldDir(worldId) + "/" + revisionId + ".cfg", text)) return false;

    out = WorldRevision();
    out.id = revisionId;

    // The struct's own defaults stand for any key the file does not carry,
    // which is the same rule defaults.cfg has and the reason an older
    // revision still loads after a setting is added.
    RevisionParse parse;
    parse.recipe = &out.recipe;
    ForEachConfigLine(text, ApplyRevisionLine, &parse);
    return true;
}

std::vector<WorldRevision> WorldStore::Revisions(const std::string& worldId) const {
    std::vector<WorldRevision> out;
    if (worldId == kVanillaWorldId) return out;   // Vanilla has no revisions (D16)

    std::vector<int> numbers;
    std::vector<DirEntryName> entries = ListDir(WorldDir(worldId));
    for (size_t i = 0; i < entries.size(); i++) {
        int n = RevisionNumber(entries[i].name);
        if (n > 0) numbers.push_back(n);
    }
    std::sort(numbers.begin(), numbers.end());

    // Newest first - the order HISTORY shows them in (spec worlds D14).
    for (size_t i = numbers.size(); i > 0; i--) {
        WorldRevision rev;
        if (LoadRevision(worldId, NumberedId("rev-", numbers[i - 1]), rev)) {
            out.push_back(rev);
        }
    }
    return out;
}

bool WorldStore::CurrentRevision(const std::string& worldId, WorldRevision& out) const {
    World w;
    if (!Load(worldId, w)) return false;
    if (w.currentRevision.empty()) return false;
    return LoadRevision(worldId, w.currentRevision, out);
}

bool WorldStore::AppendRevision(const std::string& worldId, const WorldRecipe& recipe,
                                std::string& outRevisionId, bool& outAppended,
                                std::string& error) {
    error.clear();
    outRevisionId.clear();
    outAppended = false;

    if (worldId == kVanillaWorldId) {
        error = "VANILLA HAS NO SETTINGS";     // spec worlds D16
        return false;
    }
    if (!Exists(worldId)) { error = "NO SUCH WORLD - " + worldId; return false; }

    World w;
    if (!Load(worldId, w)) { error = "COULD NOT READ " + worldId; return false; }

    // Only when the recipe DIFFERS (worlds plan P4). A rename, or an edit the
    // player backed out of, leaves the history alone - otherwise HISTORY fills
    // with revisions that change nothing and the ones that matter are lost in
    // them.
    if (!w.currentRevision.empty()) {
        WorldRevision current;
        if (LoadRevision(worldId, w.currentRevision, current) &&
            SameRecipe(current.recipe, recipe)) {
            outRevisionId = w.currentRevision;
            return true;
        }
    }

    int highest = 0;
    std::vector<DirEntryName> entries = ListDir(WorldDir(worldId));
    for (size_t i = 0; i < entries.size(); i++) {
        int n = RevisionNumber(entries[i].name);
        if (n > highest) highest = n;
    }

    std::string revId = NumberedId("rev-", highest + 1);
    if (!WriteTextFile(WorldDir(worldId) + "/" + revId + ".cfg", FormatRevision(recipe))) {
        error = "COULD NOT WRITE " + revId;
        return false;
    }

    outRevisionId = revId;
    outAppended   = true;
    Log(("worldstore: " + worldId + " appended " + revId).c_str());
    return true;
}

// --- world saves -----------------------------------------------------------

bool WorldStore::HasStoredSave(const std::string& worldId) const {
    return FileExists(SaveDir(worldId) + "/manifest.txt");
}

bool WorldStore::StoredSaveManifest(const std::string& worldId, savedata::Manifest& out,
                                    std::string& error) const {
    return savedata::ReadManifestFile(SaveDir(worldId) + "/manifest.txt", out, error);
}

bool WorldStore::VerifyStoredSave(const std::string& worldId, std::string& error,
                                  savedata::Manifest* outManifest) const {
    if (!HasStoredSave(worldId)) {
        error = worldId + " HAS NO STORED SAVE";
        return false;
    }
    return savedata::VerifyBackup(SaveDir(worldId), error, outManifest);
}

int WorldStore::SweepPartialWorldSaves() const {
    int swept = 0;
    std::vector<DirEntryName> entries = ListDir(accountDir_);
    for (size_t i = 0; i < entries.size(); i++) {
        if (!entries[i].isDir) continue;

        const char* debris[] = { "/save.partial", "/save.old" };
        for (int d = 0; d < 2; d++) {
            std::string path = accountDir_ + "/" + entries[i].name + debris[d];
            if (!IsDir(path)) continue;
            RemoveTree(path);
            Log(("worldstore: swept " + path).c_str());
            swept++;
        }
    }
    return swept;
}

// ---------------------------------------------------------------------------
// First-run capture
// ---------------------------------------------------------------------------

struct FirstRunCaptureJob::State {
    savedata::User user;
    std::unique_ptr<WorldStore> store;
    std::unique_ptr<SafetyBackupJob> backup;
    std::unique_ptr<StoreSaveJob>    storeSave;

    std::string liveTitle;
    std::string liveDir;
    bool        capture = false;

    int         phase = 0;   // 0 look, 1 create, 2 back up, 3 file it, 4 done
    uint64_t    startUs = 0;
    std::string status = "STARTING";
    FirstRunCaptureResult result;

    void Finish(const std::string& why) {
        result.ok        = true;
        result.elapsedMs = (NowUs() - startUs) / 1000;
        status           = why;
        phase            = 4;
    }

    void Fail(const std::string& why) {
        result.ok    = false;
        result.error = why;
        status       = why;
        Log(("worldstore: first-run capture failed - " + why).c_str());
        phase = 4;
    }

    // Undo everything this job made, so the NEXT run gets a first run too.
    // The safety backup it may already have taken stays where it is: those are
    // never deleted, and one that exists without a world to belong to is still
    // the player's save.
    void Rollback(const std::string& why) {
        if (store) store->DestroyAccount();
        result.createdVanilla = false;
        Fail(why);
    }
};

FirstRunCaptureJob::FirstRunCaptureJob(const savedata::User& user) : s_(new State()) {
    s_->user    = user;
    s_->startUs = NowUs();
}

FirstRunCaptureJob::~FirstRunCaptureJob() = default;

bool FirstRunCaptureJob::Done() const { return s_->phase >= 4; }
std::string FirstRunCaptureJob::StatusText() const { return s_->status; }
const FirstRunCaptureResult& FirstRunCaptureJob::Result() const { return s_->result; }

float FirstRunCaptureJob::Progress() const {
    State& s = *s_;
    if (s.phase >= 4) return 1.0f;
    if (s.phase <= 1) return 0.05f;
    if (s.phase == 2) return s.backup ? 0.05f + s.backup->Progress() * 0.45f : 0.05f;
    return s.storeSave ? 0.5f + s.storeSave->Progress() * 0.5f : 0.5f;
}

void FirstRunCaptureJob::Step() {
    State& s = *s_;
    if (s.phase >= 4) return;

    if (s.phase == 0) {
        if (!s.user.valid) { s.Fail(s.user.error); return; }
        s.store.reset(new WorldStore(s.user.accountId));

        if (s.store->AccountDirExists()) {
            s.result.alreadyDone = true;
            s.Finish("THIS PLAYER ALREADY HAS WORLDS - NOTHING TO DO");
            return;
        }

        // Look before creating anything. An ambiguous console must not spend
        // its one first run: refusing leaves the account directory absent, so
        // the next launch tries again, while creating Vanilla without the save
        // would file the player's playthrough nowhere, permanently.
        savedata::SaveTitle title = savedata::DiscoverSaveTitle(s.user);
        if (!title.found) {
            if (title.titlesWithHits > 1) { s.Fail(title.error); return; }
            s.result.note = "NO BLOODBORNE SAVE DATA FOR THIS PLAYER";
        } else if (title.dirNames.size() != 1) {
            s.Fail("MORE THAN ONE SAVE DIRECTORY UNDER " + title.titleId);
            return;
        } else {
            s.liveTitle = title.titleId;
            s.liveDir   = title.dirNames[0];

            savedata::Container c =
                savedata::ReadContainer(s.user, s.liveTitle, s.liveDir);
            if (!c.exists) {
                s.result.note = "NO SAVE CONTAINER YET - BLOODBORNE HAS NOT RUN";
            } else {
                bool anyGameFile = false;
                for (size_t i = 0; i < c.files.size(); i++) {
                    if (savedata::IsGameSaveFile(c.files[i].rel)) { anyGameFile = true; break; }
                }
                // spec worlds D26: a container holding no save files means
                // there is nothing to capture, and capturing emptiness over a
                // world's save would be destroying it.
                if (!anyGameFile) s.result.note = "THE CONTAINER HOLDS NO SAVE FILES";
                else              s.capture     = true;
            }
        }

        s.status = s.capture ? "CAPTURING THE LIVE SAVE INTO VANILLA" : s.result.note;
        s.phase  = 1;
        return;
    }

    if (s.phase == 1) {
        std::string error;
        if (!s.store->CreateAccount(error)) { s.Fail(error); return; }
        s.result.createdVanilla = true;

        if (!s.capture) { s.Finish("VANILLA CREATED - " + s.result.note); return; }

        s.backup.reset(new SafetyBackupJob(s.user, s.liveTitle, s.liveDir, "firstrun",
                                           kVanillaWorldId, ""));
        s.phase = 2;
        return;
    }

    if (s.phase == 2) {
        s.backup->Step();
        s.status = s.backup->StatusText();
        if (!s.backup->Done()) return;

        const SafetyBackupResult& r = s.backup->Result();
        if (!r.ok) { s.Rollback(r.error); return; }
        s.result.backupPath = r.path;

        s.storeSave.reset(new StoreSaveJob(r.path, s.store->SaveDir(kVanillaWorldId),
                                           kVanillaWorldId, ""));
        s.phase = 3;
        return;
    }

    s.storeSave->Step();
    s.status = s.storeSave->StatusText();
    if (!s.storeSave->Done()) return;

    const StoreSaveResult& r = s.storeSave->Result();
    if (!r.ok) { s.Rollback(r.error); return; }

    std::string error;
    // Vanilla has no revisions, so the revision it was last played on is
    // empty - the field still records WHEN, which is what the rail sorts on.
    s.store->SetLastPlayed(kVanillaWorldId, "", error);

    s.result.capturedSave = true;
    s.result.files        = r.files;
    s.result.bytes        = r.bytes;
    s.Finish("VANILLA HOLDS THE LIVE SAVE - " + U64Text(r.files) + " FILE(S), " +
             U64Text(r.bytes) + " BYTES");
}

} // namespace bbr
