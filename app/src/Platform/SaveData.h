// SaveData.h - the app's one door to PS4 save data.
//
// Everything above this file - the world store, the activation transaction,
// the screens - goes through these calls and never touches libSceSaveData
// itself. No orbis type appears below, on purpose: this header is included
// from Randomizer/ and Game/ code that must not learn what a mount is.
//
// WHAT THE PLATFORM ACTUALLY PERMITS, and how we know: every sequence here was
// executed on a real PS4 before it was written down. The operational reference
// is docs/features/worlds/technical-findings.md section 8, and its four rules
// bind every function in this file:
//
//   * Never set CREATE2, never call sceSaveDataDelete, never write under
//     sce_sys. The first two are not needed and the first is implicated in
//     corrupting a save (findings section 6); the third cannot succeed - the
//     four sce_sys entries refuse with EACCES, and they already hold values
//     correct for the container being written into.
//   * Bracket every container write with a read of the container's file list
//     and total, before and after. A match is not proof, but it dates the
//     damage, which is the one thing the findings section 6 incident lacked.
//   * A failed operation is NOT a no-op, and naming a different directory does
//     not bound the damage.
//   * Check the container is large enough before writing. SAVEDATA_BLOCKS
//     lives in the unwritable param.sfo, so a container's size is a fact to
//     check against, never a thing to set.
//
// TWO TITLE IDS, AND THEY ARE NOT THE SAME VALUE. The AFR title is the
// BLOODBORNE TITLE ID setting (CUSA03173 on the reference console). The
// save-data title is discovered by searching (CUSA00207 there - Bloodborne's
// regional SKUs share one save container through INSTALL_DIR_SAVEDATA).
// Nothing in this file is ever handed the AFR title, and nothing in it
// derives one from the other. Feeding the AFR title to a save search returns
// zero hits on a console that plainly has saves, and looks exactly like a
// broken mechanism - it already cost one investigation.
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace bbr {
namespace savedata {

// ---------------------------------------------------------------------------
// Identity
// ---------------------------------------------------------------------------

// Who the save data belongs to. Every search, mount and world listing in the
// app is scoped to this one user - the app never reads another user's save
// data and never lists another user's worlds.
struct User {
    bool        valid     = false;
    int32_t     userId    = -1;
    uint64_t    accountId = 0;   // matches ACCOUNT_ID inside the save's own
                                 // param.sfo (findings 8.5), so ownership is
                                 // checkable without parsing a save
    std::string userName;
    std::string error;           // a sentence, set only when !valid
};

// findings 8.1's first two calls. The FOREGROUND user, with no fallback to the
// initial user: the plan's refusal table refuses when there is no foreground
// user or no account id for it, and an app that quietly acted for a different
// signed-in player than the one holding the pad would be writing a save into
// the wrong account's world.
User ResolveUser();

// ---------------------------------------------------------------------------
// Discovery
// ---------------------------------------------------------------------------

// One line of the findings 8.1 sweep, kept so a refusal can show its working.
struct TitleSweepEntry {
    std::string titleId;
    std::string label;      // region/edition, for the report
    int         hits = 0;   // directories the search returned
    int         rc   = 0;   // the search's own return code
};

struct SaveTitle {
    bool                     found = false;  // exactly one title had hits
    std::string              titleId;
    std::vector<std::string> dirNames;       // that title's directories
    int                      titlesWithHits = 0;
    std::vector<TitleSweepEntry> swept;
    std::string              error;          // a sentence, set when !found
};

// Sweeps the six officially-issued Bloodborne SKUs and accepts an answer only
// when exactly one of them has save data. Zero is "nothing to work with" and
// more than one is a question for the player, not a guess - both refuse.
//
// Does NOT sweep the configured AFR title: see this header's opening note.
SaveTitle DiscoverSaveTitle(const User& user);

struct SearchResult {
    bool                     ok = false;
    std::vector<std::string> dirNames;
    int                      rc = 0;
    std::string              error;
};

// The raw search for one title. cond.key and cond.order are left at their
// defaults, so the ORDER of the result is not specified: callers select by
// name or by there being exactly one, never by index.
SearchResult FindSaveDirs(const User& user, const std::string& titleId);

// ---------------------------------------------------------------------------
// Reading a container
// ---------------------------------------------------------------------------

struct FileEntry {
    std::string rel;     // relative to the container root, '/' separated
    uint64_t    bytes = 0;
};

struct Container {
    bool                   exists = false;  // the read-only mount succeeded
    std::vector<FileEntry> files;           // sorted by rel
    uint64_t               bytes  = 0;      // sum of the above
    uint64_t               blocks = 0;      // the mount's OWN figure, which is
                                            // the size a restore must fit into
    int                    walkErrors = 0;
    int                    rc = 0;          // the mount's return code
    std::string            error;
};

// Mounts read-only, walks, unmounts. This is also THE BRACKETING READ: it is
// what is called before and after anything that writes a container.
//
// Sizes every file by reading it through to a short read, never by st_size -
// which returns 88 for a genuine 44 KB file on this kernel - and recurses on
// d_type == 4, because a directory opened read-only returns directory data
// rather than failing, which is how an earlier walk reported sce_sys as a
// 32,768-byte "file" and silently lost 10 MB of a 36 MB save.
Container ReadContainer(const User& user, const std::string& titleId,
                        const std::string& dirName);

// True for a root-level userdata* or backup* entry - the files the GAME wrote,
// as opposed to the four sce_sys entries the system owns. A container holding
// none of these exists but has nothing in it worth capturing.
bool IsGameSaveFile(const std::string& rel);

// ---------------------------------------------------------------------------
// Emptying a container - this is START FRESH
// ---------------------------------------------------------------------------

struct EmptyResult {
    bool        ok = false;
    int         removed = 0;
    int         refused = 0;
    int         kept    = 0;   // sce_sys, and anything not at the root
    Container   before;
    Container   after;
    std::string error;
};

// findings 8.3: mount RDWR (never CREATE2), unlink every ROOT-LEVEL userdata*
// and backup*, leave sce_sys, unmount. The container itself is never
// destroyed - this app cannot create one, so destroying one would be
// unrecoverable without the game.
//
// Bracketed: `before` and `after` are full reads of the container.
EmptyResult EmptyContainer(const User& user, const std::string& titleId,
                           const std::string& dirName);

// ---------------------------------------------------------------------------
// Manifests
// ---------------------------------------------------------------------------

// A backup directory is
//
//     <name>/manifest.txt
//     <name>/data/<every file of the container, same relative paths>
//
// and the manifest is what makes it a backup rather than a directory of
// files. It is written LAST and the directory is renamed into place after
// it, so a reader never has to ask whether a copy finished: either the
// directory exists with a manifest in it, or it does not exist.
//
// Verification is file count, relative paths, per-file sizes and the total -
// exactly what the spec defines it as. No checksums: that would exceed the
// approved definition and cost a second full read of 27 MB.
struct Manifest {
    std::string title_id;
    std::string dir_name;
    uint64_t    account_id = 0;
    uint64_t    blocks     = 0;   // the CONTAINER's size, not the copy's
    uint64_t    files      = 0;
    uint64_t    bytes      = 0;
    std::string captured;         // "YYYY-MM-DD HH:MM:SS", local
    std::string world;            // world id; empty until milestone 2
    std::string revision;         // revision id; empty until milestone 2
    std::vector<FileEntry> entries;
};

std::string FormatManifest(const Manifest& m);
bool ParseManifest(const std::string& text, Manifest& out, std::string& error);

bool WriteManifestFile(const std::string& path, const Manifest& m);
bool ReadManifestFile(const std::string& path, Manifest& out, std::string& error);

// Walks <backupDir>/data and compares it against <backupDir>/manifest.txt in
// count, path set, per-file size and total. This is the check that must pass
// before a live save is touched - never after.
bool VerifyBackup(const std::string& backupDir, std::string& error,
                  Manifest* outManifest = nullptr);

// ---------------------------------------------------------------------------
// Backup and restore, as resumable jobs
// ---------------------------------------------------------------------------

// A ~27 MB copy is far too long to spend inside one Screen::Update(): the
// frame loop is strictly serial, so nothing can be drawn while Update is
// running. Both jobs therefore take the Step()/Done()/StatusText()/Progress()
// shape EnemyRandomizerJob established, one file per step, so the caller can
// present a frame - and a progress line - between files.

struct BackupResult {
    bool        ok = false;
    std::string error;
    std::string path;        // the final directory, set only when ok
    uint64_t    files = 0;
    uint64_t    bytes = 0;
    uint64_t    elapsedMs = 0;
    Manifest    manifest;
};

class BackupJob {
public:
    // Copies the container into `destDir` - a full path ending in the backup's
    // final name. The copy goes into `destDir` + ".partial" and is renamed on
    // success, so a run that dies partway leaves a .partial directory that no
    // reader will mistake for a backup.
    //
    // `world` and `revision` are recorded in the manifest and may be empty.
    BackupJob(const User& user, const std::string& titleId,
              const std::string& dirName, const std::string& destDir,
              const std::string& world, const std::string& revision);
    ~BackupJob();

    BackupJob(const BackupJob&) = delete;
    BackupJob& operator=(const BackupJob&) = delete;

    void  Step();
    bool  Done() const;

    // What the step that just ran did - one file, named, with its byte count.
    // A caller that records this after every Step() gets one line per file.
    std::string StatusText() const;
    float Progress() const;
    const BackupResult& Result() const;

private:
    struct State;
    std::unique_ptr<State> s_;
};

struct RestoreResult {
    bool        ok = false;
    std::string error;
    uint64_t    files = 0;       // written, so excluding sce_sys
    uint64_t    bytes = 0;
    uint64_t    removed = 0;     // emptied out of the way first
    uint64_t    skipped = 0;     // sce_sys entries, never attempted
    uint64_t    elapsedMs = 0;
    Container   before;
    Container   after;
};

class RestoreJob {
public:
    // Restores <srcDir>/data into the container, in the findings 8.4 order:
    // verify the source against its manifest, read the container, refuse on
    // any mismatch BEFORE writing a byte, mount RDWR, EMPTY the container,
    // write everything except sce_sys, unmount, and read the container back to
    // confirm it against the manifest.
    //
    // Emptying first is mandatory and not obvious: backup* files accumulate
    // with play time - three on a fresh save, eleven on a played one - so
    // writing a three-backup save over an eleven-backup container would leave
    // eight files of the previous playthrough mixed into the restored one.
    RestoreJob(const User& user, const std::string& titleId,
               const std::string& dirName, const std::string& srcDir);
    ~RestoreJob();

    RestoreJob(const RestoreJob&) = delete;
    RestoreJob& operator=(const RestoreJob&) = delete;

    void  Step();
    bool  Done() const;
    std::string StatusText() const;
    float Progress() const;
    const RestoreResult& Result() const;

private:
    struct State;
    std::unique_ptr<State> s_;
};

} // namespace savedata
} // namespace bbr
