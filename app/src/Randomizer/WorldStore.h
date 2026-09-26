// WorldStore.h - worlds, revisions, world saves and safety backups, on disk.
//
// A WORLD is a named playthrough: a settings history plus the save data
// produced by playing it. One is active at a time, Vanilla is one of them, and
// activating one swaps randomizer files and save data together. This file owns
// everything that lives on disk for that; the transaction that swaps them is
// Game/WorldActivation, and the screens are UI/WorldsScreen.
//
// THE LAYOUT (worlds plan section 4.1):
//
//   /data/bbrandomizer/
//     SaveBackups/<saveTitle>_<dir>_<stamp>_<reason>/  manifest.txt + data/
//     Worlds/acct-<16 hex account id>/
//       worlds.cfg                    next_world_id only
//       vanilla/  world.cfg  save/{manifest.txt,data/}
//       w-0001/   world.cfg  rev-0001.cfg  rev-0002.cfg  save/{...}
//
// FOUR THINGS THIS FILE IS CAREFUL ABOUT, because getting any of them wrong
// destroys a playthrough rather than producing a wrong screen:
//
//   * ACCOUNT SCOPING IS STRUCTURAL. There is no call here that lists or
//     touches a world without an account id, because a WorldStore cannot be
//     constructed without one. Worlds belong to the account that created them
//     and are listed only for that account (spec worlds D12).
//   * A SAVE IS COPIED, NEVER LINKED (spec worlds D4). A save with two owners
//     silently changes under whichever world is not being activated.
//   * EVERY DIRECTORY THAT MATTERS IS BUILT UNDER ".partial" AND RENAMED INTO
//     PLACE. A reader therefore never has to ask whether a copy finished:
//     either the directory exists with a manifest in it, or it does not exist.
//     Free space cannot be measured on this platform (findings section 7), so
//     "it ran out of disk" is a case that has to be survivable rather than
//     preventable (spec worlds D24).
//   * A COPY IS VERIFIED BEFORE ANYTHING DEPENDS ON IT, by walking what landed
//     on disk and comparing it against its manifest - count, path, per-file
//     size and total. Never by trusting the copy's own return value.
//
// The save-data half of all of this is Platform/SaveData: this file never
// mounts anything and never names a save-data API. It hands SaveData a
// container to read and a directory to write, and copies plain files around
// /data afterwards.
#pragma once

#include "RandomizerDefaults.h"

#include "../Platform/SaveData.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace bbr {

// Vanilla is a world, not a special case (spec worlds D5): always present, no
// editable settings, no revisions, not deletable. Its id is the literal below
// rather than a "w-NNNN", so it can never collide with an allocated one.
extern const char* const kVanillaWorldId;
extern const char* const kVanillaWorldName;

// ---------------------------------------------------------------------------
// What a world is made of
// ---------------------------------------------------------------------------

// One world's settings at one point in time - what a revision file holds.
// RandomizerDefaults whole, rather than a subset, because it is the same
// struct the editor edits and the randomizer is handed: a recipe that dropped
// a field would generate a different world than the one that was approved.
struct WorldRecipe {
    uint32_t           seed = 0;
    RandomizerDefaults settings;
};

struct WorldRevision {
    std::string id;       // "rev-0001"
    WorldRecipe recipe;
};

struct World {
    // --- world.cfg ---
    std::string id;                   // "w-0001", or the literal "vanilla"
    std::string name;
    std::string created;              // "YYYY-MM-DD HH:MM:SS", local
    std::string lastPlayed;           // empty means never
    std::string lastPlayedRevision;   // the revision current when the save was
                                      // last written (spec worlds D3)
    uint64_t    accountId = 0;
    bool        isVanilla = false;

    // --- derived when the world is listed, not stored ---
    std::string currentRevision;      // the highest rev-NNNN; empty for Vanilla
    int         revisionCount = 0;
    bool        hasSave = false;      // save/manifest.txt is readable
    uint64_t    saveFiles  = 0;
    uint64_t    saveBytes  = 0;
    uint64_t    saveBlocks = 0;       // the CONTAINER the save came out of, so
                                      // a restore can check it fits (B25)
    std::string saveCaptured;
    std::string saveRevision;         // the revision stamped into its manifest
};

// True when `a` and `b` are the same recipe - the test that decides whether
// editing a world appends a revision (worlds plan P4). Defined as "they
// serialize identically", so a field added to RandomizerDefaults is covered
// without anyone remembering to extend this.
bool SameRecipe(const WorldRecipe& a, const WorldRecipe& b);

// A–Z, 0–9 and space, at most 16 characters, uppercased (worlds plan P10).
// Applied on the way in, not just in the editor: world.cfg is one key=value
// line per field, so a name carrying a newline would silently corrupt it.
std::string NormalizeWorldName(const std::string& name);

// ---------------------------------------------------------------------------
// Safety backups
//
// Timestamped, never overwritten, never auto-deleted (spec worlds section 2).
// World saves are working copies that get swapped; these are the recovery path
// when something goes wrong, and they are deliberately not pruned.
// ---------------------------------------------------------------------------

// SaveBackups/<saveTitle>_<dir>_<stamp>_<reason>, as a full path.
std::string SafetyBackupPath(const std::string& titleId, const std::string& dirName,
                             const std::string& reason);

// Removes every SaveBackups/*.partial. Run at startup, before anything is
// listed: a .partial is the debris of a copy that did not finish, and the one
// state that must never be mistaken for a backup is a directory that looks
// like one (spec worlds D24).
int SweepPartialBackups();

struct SafetyBackupResult {
    bool        ok = false;      // copied AND verified
    std::string error;
    std::string path;            // the final directory, set only when ok
    uint64_t    files = 0;
    uint64_t    bytes = 0;
    uint64_t    elapsedMs = 0;
    savedata::Manifest manifest;
};

// Backs the live container up and then VERIFIES the result against its own
// manifest before reporting success. The verification is a separate walk of
// what landed on disk, not a restatement of what the copy thought it wrote,
// which is the only version of it worth having - and it happens here rather
// than at the call site so that "took a safety backup" and "has a usable
// safety backup" cannot come apart.
//
// Stepped one file at a time, like every long operation in this app: the frame
// loop is strictly serial, so a ~27 MB copy run inside one Update() is a
// screen frozen for a second or more.
class SafetyBackupJob {
public:
    SafetyBackupJob(const savedata::User& user, const std::string& titleId,
                    const std::string& dirName, const std::string& reason,
                    const std::string& world, const std::string& revision);
    ~SafetyBackupJob();

    SafetyBackupJob(const SafetyBackupJob&) = delete;
    SafetyBackupJob& operator=(const SafetyBackupJob&) = delete;

    void  Step();
    bool  Done() const;
    std::string StatusText() const;
    float Progress() const;
    const SafetyBackupResult& Result() const;

private:
    struct State;
    std::unique_ptr<State> s_;
};

// ---------------------------------------------------------------------------
// Storing a save into a world
// ---------------------------------------------------------------------------

struct StoreSaveResult {
    bool        ok = false;
    std::string error;
    uint64_t    files = 0;
    uint64_t    bytes = 0;
    uint64_t    elapsedMs = 0;
    savedata::Manifest manifest;
};

// Copies a VERIFIED backup directory into a world's save/, disk to disk, one
// file per Step(). A copy, never a link (spec worlds D4).
//
// The source is re-verified first: this job is the one place a world's save
// comes from, so it refuses to file anything that does not already match its
// own manifest. The copy is built in save.partial/ and renamed into place over
// an existing save via save.old/, so a world never holds a half-written save
// and the previous one survives until the new one is complete.
class StoreSaveJob {
public:
    StoreSaveJob(const std::string& srcBackupDir, const std::string& worldSaveDir,
                 const std::string& worldId, const std::string& revisionId);
    ~StoreSaveJob();

    StoreSaveJob(const StoreSaveJob&) = delete;
    StoreSaveJob& operator=(const StoreSaveJob&) = delete;

    void  Step();
    bool  Done() const;
    std::string StatusText() const;
    float Progress() const;
    const StoreSaveResult& Result() const;

private:
    struct State;
    std::unique_ptr<State> s_;
};

// ---------------------------------------------------------------------------
// The store itself
// ---------------------------------------------------------------------------

class WorldStore {
public:
    // `accountId` is sceUserServiceGetNpAccountId for the signed-in player,
    // which hardware showed is exactly the ACCOUNT_ID inside that player's own
    // save (findings section 8.5). Read it through savedata::ResolveUser.
    explicit WorldStore(uint64_t accountId);

    uint64_t    AccountId() const { return accountId_; }
    std::string AccountDir() const { return accountDir_; }

    // The first row of the plan's section 4.2 table: no account directory
    // means the randomizer has never been used by this player, which is what
    // first-run capture keys off. Deliberately not a stored flag - a flag
    // drifts from the disk it claims to describe.
    bool AccountDirExists() const;

    // Creates Worlds/acct-<id>/, worlds.cfg and Vanilla. Only
    // FirstRunCaptureJob should call this: creating the account directory is
    // what ENDS the first run, and doing it before the live save has been
    // captured is how that save would be lost.
    bool CreateAccount(std::string& error);

    // Removes the whole account directory. The one caller is
    // FirstRunCaptureJob rolling itself back after a failed capture, so that
    // the next run tries again instead of finding a first run already spent.
    void DestroyAccount();

    // --- worlds ---

    // The player's worlds, most recently played first, then never-played by
    // creation date, newest first. Vanilla is NOT in this list: the rail shows
    // it in a fixed position of its own, above them (B2).
    std::vector<World> List() const;

    bool Load(const std::string& worldId, World& out) const;
    bool Exists(const std::string& worldId) const;

    // Allocates an id, writes world.cfg and rev-0001.cfg. Ids are "w-NNNN",
    // one above the highest ever used, and are never reused - a rename never
    // changes one (spec worlds D15).
    bool Create(const std::string& name, const WorldRecipe& recipe,
                World& out, std::string& error);

    // Rewrites world.cfg and appends NO revision: a name is metadata, not part
    // of the recipe (worlds plan P4).
    bool Rename(const std::string& worldId, const std::string& name,
                std::string& error);

    // Records which revision was current when this world's save was last
    // written, and when (spec worlds D3).
    bool SetLastPlayed(const std::string& worldId, const std::string& revisionId,
                       std::string& error);

    // Keeps the world's save as a safety backup, then removes the world
    // (spec worlds D9). Refuses Vanilla, which is never deletable (D16).
    //
    // Does NOT check whether the world is active: only a screen knows which
    // one that is, and B17's "the active world cannot be deleted" is enforced
    // there.
    bool Delete(const std::string& worldId, std::string& error,
                std::string* outKeptBackupPath = nullptr);

    // --- revisions ---

    // Newest first, which is the order HISTORY shows them in (spec worlds
    // D14). Empty for Vanilla, which has no revisions (D16).
    std::vector<WorldRevision> Revisions(const std::string& worldId) const;

    bool LoadRevision(const std::string& worldId, const std::string& revisionId,
                      WorldRevision& out) const;
    bool CurrentRevision(const std::string& worldId, WorldRevision& out) const;

    // Appends a revision ONLY when `recipe` differs from the current one, and
    // reports which revision is current afterwards either way. Settings are
    // never overwritten and history is append-only: making an earlier revision
    // current again is itself an append, so nothing is ever lost (D2).
    bool AppendRevision(const std::string& worldId, const WorldRecipe& recipe,
                        std::string& outRevisionId, bool& outAppended,
                        std::string& error);

    // --- world saves ---

    // Where this world's save lives. Handed to savedata::RestoreJob as a
    // source, and to StoreSaveJob as a destination; this is the only path
    // shape either of them is given.
    std::string SaveDir(const std::string& worldId) const;

    bool HasStoredSave(const std::string& worldId) const;
    bool StoredSaveManifest(const std::string& worldId, savedata::Manifest& out,
                            std::string& error) const;

    // Walks the stored save and compares it against its manifest. This is what
    // must pass before a world's save is allowed anywhere near a container.
    bool VerifyStoredSave(const std::string& worldId, std::string& error,
                          savedata::Manifest* outManifest = nullptr) const;

    // Removes this account's *.partial and *.old world-save debris, the same
    // sweep SweepPartialBackups() does for SaveBackups.
    int SweepPartialWorldSaves() const;

private:
    std::string WorldDir(const std::string& worldId) const;
    bool ReadWorldCfg(const std::string& worldId, World& out) const;
    bool WriteWorldCfg(const World& world) const;
    int  NextWorldId() const;
    bool WriteNextWorldId(int next) const;

    uint64_t    accountId_;
    std::string accountDir_;
};

// ---------------------------------------------------------------------------
// First-run capture
// ---------------------------------------------------------------------------

struct FirstRunCaptureResult {
    bool        ok = false;
    bool        alreadyDone = false;  // the account directory was already there
    bool        createdVanilla = false;
    bool        capturedSave = false;
    std::string backupPath;           // the safety backup, when one was taken
    std::string note;                 // why nothing was captured, when nothing was
    std::string error;
    uint64_t    files = 0;
    uint64_t    bytes = 0;
    uint64_t    elapsedMs = 0;
};

// B26: on first run the existing live save is captured into Vanilla before
// anything else happens. A fresh install already has a real playthrough on the
// console; if the first thing the player does is create a world, that save must
// already belong to Vanilla.
//
// It runs at most once per account, and does nothing at all afterwards -
// "already done" is simply "the account directory exists".
//
// NOTHING IS CAPTURED, and that is not a failure, when: the player has no
// Bloodborne save data, no container exists yet, or the container exists but
// holds no userdata*/backup* files. The last of those is spec worlds D26: an
// empty capture over a good stored save would be destroying it, and the backup
// that normally covers such a replacement is precisely the one that cannot be
// taken.
//
// It REFUSES, without creating anything, when the save-data title or directory
// is ambiguous. That is deliberately not the same as "nothing to capture": an
// ambiguous console would otherwise spend its one first run and lose the
// chance to file the live save into Vanilla.
class FirstRunCaptureJob {
public:
    explicit FirstRunCaptureJob(const savedata::User& user);
    ~FirstRunCaptureJob();

    FirstRunCaptureJob(const FirstRunCaptureJob&) = delete;
    FirstRunCaptureJob& operator=(const FirstRunCaptureJob&) = delete;

    void  Step();
    bool  Done() const;
    std::string StatusText() const;
    float Progress() const;
    const FirstRunCaptureResult& Result() const;

private:
    struct State;
    std::unique_ptr<State> s_;
};

} // namespace bbr
