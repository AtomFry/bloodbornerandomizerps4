// WorldActivation.h - activating a world: the one operation that changes what
// the game will do.
//
// Activation does three things AS ONE TRANSACTION (spec worlds section 2):
// the outgoing world's save is backed up to that world, the incoming world's
// randomizer files are generated and made current, and the incoming world's
// save becomes live. They switch together or not at all - a world's save
// running on another world's randomization is not corrupt, it is SILENTLY
// INCOHERENT, which is worse, because nothing reports it and the player finds
// out hours later.
//
// THE SEVEN PHASES (worlds plan section 4.3):
//
//   1  CHECK      every refusal in section 4.4. Nothing is written.
//   2  BACKUP     the live save into SaveBackups/..._preactivate, verified.
//                 ANY failure aborts the whole transaction (spec worlds D24).
//   3  CAPTURE    that verified backup into the OUTGOING world's save/.
//   4  GENERATE   EnemyRandomizerJob into dvdroot_ps4.staging, then stamp it.
//   5  SWAP FILES rename the staged tree into place.
//   6  SWAP SAVE  restore, adopt, empty or nothing - the section 4.3 table.
//   7  COMMIT     revert a one-shot START FRESH, then delete the journal.
//
// Phases 2 and 3 are SKIPPED TOGETHER when the container does not exist or
// holds no save files: capturing emptiness over a world's good stored save
// would be destroying it, in the one case where the safety backup that
// normally covers such a replacement cannot be taken (spec worlds D26).
//
// WHY A JOURNAL. The console can lose power at any instant, and the operation
// spans two unrelated stores - an AFR tree on /data and a save container the
// kernel owns. A file naming the phase about to run, fsync'd before the phase
// runs, is what turns "the app died somewhere in the middle" into a specific,
// recoverable question at next launch. Reconcile() answers it.
//
// WHY IT IS A JOB. A full activation moves tens of megabytes of save data and
// regenerates a multi-gigabyte tree. Application.cpp's frame loop is strictly
// serial, so anything done inside one Update() is a frozen screen for its
// whole duration. Step() does one unit and returns, exactly like
// EnemyRandomizerJob, whose shape this follows.
#pragma once

#include "AfrManager.h"

#include "../Platform/SaveData.h"
#include "../Randomizer/EnemyRandomizer.h"
#include "../Randomizer/WorldStore.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace bbr {

// ---------------------------------------------------------------------------
// Refusals - worlds plan section 4.4, all checked in phase 1, before any write
// ---------------------------------------------------------------------------

// "Refuse rather than guess" (spec worlds section 7.1). Every one of these is
// decided from facts gathered without writing anything, so a refused
// activation has changed nothing at all - which is the only claim worth making
// about an operation that can destroy a playthrough.
enum class RefusalReason {
    None = 0,

    // No foreground user, or no account id for one.
    NoUser,

    // The incoming world belongs to another account, or is not there at all
    // (spec worlds D12).
    NotThisAccount,

    // The findings section 8.1 sweep found no save title, or more than one.
    // Never the AFR title: the two are independent values (spec worlds D25).
    SaveTitle,

    // More than one save directory for the resolved title, or none where one
    // is required.
    SaveDirectory,

    // The incoming world has a stored save and there is no container to put it
    // in. The app cannot create one - only the game can (spec worlds D21).
    NoContainer,

    // The incoming world's stored save needs more blocks than the container
    // has. SAVEDATA_BLOCKS is in the unwritable param.sfo, so the container's
    // size is a fact to check against (spec worlds D23).
    ContainerTooSmall,

    // The incoming world's stored save does not match its own manifest.
    StoredSaveUnverified,

    // VanillaSource/dvdroot_ps4 is missing, so there is nothing to randomize
    // FROM. Not checked when the incoming world is Vanilla, which generates
    // nothing.
    VanillaSourceMissing,

    // NOTE: there is deliberately NO "the AFR title was not detected" refusal
    // here. D27's second sentence asked for one and plan P26 removed it: the
    // BLOODBORNE TITLE ID setting is used exactly as the player entered it,
    // and AFR handling never infers or validates it. The check it removed
    // inspected the AFR overlay, which a Vanilla activation deletes, so it
    // could never have been evidence that a title is installed - it made B8 a
    // one-way door. Save-title discovery is a different question and still
    // inspects param.sfo. See docs/ps4-homebrew-findings.md section 6.

    // The AFR root for that title is not writable.
    AfrNotWritable,

    // Carried over from the Enable wizard rather than added here: a run with
    // RANDOMIZE ENEMIES or RANDOMIZE BOSSES on and nothing selected cannot
    // produce a tree. The enemy case fails after most of the tree is written;
    // the boss case is worse, drawing RandIndex(rng, 0) - undefined behaviour
    // rather than a clean failure (see BossRandomizer.cpp). Refusing up front
    // is what the wizard already does, and phase 1 is where this feature does
    // its refusing.
    EmptySelection,
};

struct Refusal {
    RefusalReason reason = RefusalReason::None;
    std::string   sentence;   // one line, for the details pane and the log

    bool Refused() const { return reason != RefusalReason::None; }
};

const char* RefusalReasonName(RefusalReason reason);

// Everything phase 1 decides on, gathered first and judged second. Split this
// way so the judgement is a pure function: the harness and the Python mirror
// can fire every row of section 4.4 on a constructed mismatch without a
// console, exactly as AfrManager::DeriveActive is tested.
struct ActivationFacts {
    // --- who ---
    bool        userValid = false;
    std::string userError;
    uint64_t    accountId = 0;

    // --- the incoming world ---
    bool     worldExists    = false;
    bool     worldIsVanilla = false;
    uint64_t worldAccountId = 0;

    // --- save data ---
    int  saveTitlesWithHits = 0;   // findings 8.1: exactly one is an answer
    int  saveDirCount       = 0;   // directories under the resolved title

    bool     containerExists = false;
    uint64_t containerBlocks = 0;

    bool        worldHasStoredSave   = false;
    uint64_t    storedSaveBlocks     = 0;
    bool        storedSaveVerifies   = true;   // walked, not assumed
    std::string storedSaveError;

    // --- the tree ---
    bool        vanillaSourcePresent = false;
    std::string afrTitleId;                    // the BLOODBORNE TITLE ID setting,
                                               // used as entered and never
                                               // validated against an install
                                               // (plan P26)
    bool        afrWritable          = false;

    // --- the recipe, for the two carried-over guards ---
    bool randomizeEnemies  = false;
    bool anyEnemySelected  = true;
    bool randomizeBosses   = false;
    bool anyBossSelected   = true;
};

// Section 4.4, in its own order. The first row that fires is the one reported:
// a console with three problems should be told about the first one and then
// asked again, not given a list it cannot act on.
Refusal CheckActivation(const ActivationFacts& facts);

// ---------------------------------------------------------------------------
// What phase 6 will do - worlds plan section 4.3's table
// ---------------------------------------------------------------------------

// KEEP EXISTING means two different things and the confirmation screen has to
// say which (spec worlds section 2): a world with a stored save gets its OWN
// save back, and a world with none adopts the live one. It never means
// overwriting one world's stored save with another's, which is the aliasing
// D4 forbids.
enum class SaveAction {
    // The incoming world is the outgoing one: phases 2 and 3 already captured
    // it, so there is nothing to swap.
    Nothing,

    // Restore this world's own stored save, per findings 8.4.
    RestoreOwn,

    // The world has no stored save: leave the container alone and copy the
    // verified backup into the world - a copy, never a link (spec worlds D4).
    AdoptLive,

    // Empty the container, per findings 8.3. Nothing is restored and the
    // incoming world's stored save, if it has one, is left untouched.
    StartFresh,

    // No container exists. A world with a stored save was already refused in
    // phase 1; otherwise the game creates the container at next launch.
    NoContainer,
};

const char* SaveActionName(SaveAction action);

// What the confirmation screen says about this row, in the player's words.
const char* SaveActionSentence(SaveAction action);

// ---------------------------------------------------------------------------
// The plan phase 1 produces
// ---------------------------------------------------------------------------

struct ActivationPlan {
    bool    ok = false;
    Refusal refusal;

    // Empty when nothing is active - a first run, or an UNMANAGED tree. There
    // is then nothing to capture INTO, though the safety backup is still taken.
    std::string fromWorldId;
    std::string fromWorldName;
    std::string fromRevision;

    std::string toWorldId;
    std::string toWorldName;
    std::string revisionId;      // the incoming world's current revision
    uint32_t    seed = 0;
    bool        toVanilla = false;
    bool        sameWorld = false;

    std::string afrTitleId;      // the BLOODBORNE TITLE ID setting
    std::string saveTitleId;     // discovered, and a different value (D25)
    std::string saveDirName;

    bool     containerExists       = false;
    bool     containerHasGameFiles = false;   // decides phases 2 and 3 (D26)
    uint64_t containerBlocks       = 0;

    bool       startFresh = false;
    SaveAction saveAction = SaveAction::Nothing;

    ActivationFacts facts;
};

// Phase 1, whole, and NOTHING ELSE: it gathers the facts, walks the incoming
// world's stored save, judges them and works out which row of the phase-6
// table applies - and writes nothing at all. The job runs it as its first
// phase, the confirmation screen builds its sentences from it (B10), and the
// harness prints it, all from the one implementation.
//
// Costly: it reads the whole save container and walks the stored save, so it
// is a second or two of blocking work. Not something to call every frame.
ActivationPlan PlanActivation(const savedata::User& user,
                              const std::string& afrTitleId,
                              const std::string& toWorldId);

// ---------------------------------------------------------------------------
// The journal
// ---------------------------------------------------------------------------

// key=value, like every other file this app writes, so it can be read over FTP
// on a console that cannot be debugged - and hand-written there, which is how
// the reconciliation path is exercised without staging a real power cut.
struct ActivationJournal {
    bool        present = false;
    std::string fromWorld;
    std::string toWorld;
    std::string revision;
    std::string savePolicy;     // kSavePolicyKeep or kSavePolicyFresh
    std::string saveTitleId;
    std::string saveDirName;
    std::string safetyBackup;   // empty until phase 2 has produced one
    int         phase = 0;      // the phase ABOUT TO RUN, fsync'd before it does
};

extern const char* const kSavePolicyKeep;
extern const char* const kSavePolicyFresh;

std::string ActivationJournalPath();
std::string FormatActivationJournal(const ActivationJournal& journal);
bool ParseActivationJournal(const std::string& text, ActivationJournal& out);
bool ReadActivationJournal(ActivationJournal& out);
bool WriteActivationJournal(const ActivationJournal& journal);
void DeleteActivationJournal();

// What reconciliation does about a journal stopped at a given phase. Exactly
// one action per phase and no phase unhandled - the point of the table is that
// "we do not know what state the console is in" is never an answer.
enum class ReconcileAction {
    // No journal, or a phase number no version of this app ever wrote. The
    // disk is left alone and the journal is reported rather than acted on.
    Nothing,

    // Phases 1-4. Nothing user-visible changed: the staged tree is work in
    // progress and the journal goes with it. A safety backup or a capture that
    // did complete is kept - those only ever add.
    DiscardStaging,

    // Phase 5. If dvdroot_ps4.old exists and dvdroot_ps4 does not, the first
    // rename landed and the second did not: put it back. Then continue at
    // phase 6.
    RestoreTree,

    // Phase 6. Re-run the save swap from the journal's own data, and if that
    // cannot complete, restore the named safety backup and report.
    ResumeSaveSwap,

    // Phase 7. The files and the save are already in place; all that is left
    // is reverting a one-shot START FRESH and deleting the journal.
    FinishCommit,
};

ReconcileAction ActionForPhase(int phase);
const char* ReconcileActionName(ReconcileAction action);

// ---------------------------------------------------------------------------
// Results
// ---------------------------------------------------------------------------

struct ActivationResult {
    bool        ok = false;
    Refusal     refusal;      // set when phase 1 refused; nothing was written
    std::string error;        // set when a later phase failed

    ActivationPlan plan;
    uint64_t       elapsedMs = 0;
    int            reachedPhase = 0;

    bool        tookSafetyBackup = false;
    std::string safetyBackupPath;
    uint64_t    backupFiles = 0;
    uint64_t    backupBytes = 0;

    bool     capturedOutgoing = false;
    uint64_t capturedFiles = 0;

    bool                  generated = false;
    EnemyRandomizerResult generation;

    bool     swapped = false;
    bool     treeRolledBack = false;   // a resume found phase 5 half-done
    uint64_t restoredFiles = 0;
    uint64_t adoptedFiles = 0;
    uint64_t emptiedFiles = 0;

    bool        revertedSavePolicy = false;   // phase 7, B12
    std::string revertRevisionId;

    // Set only by reconciliation, when a resumed phase 6 could not complete
    // and the named safety backup was put back instead.
    bool recoveredFromBackup = false;
};

class WorldActivationJob {
public:
    // A fresh activation of `toWorldId` - "vanilla" or a "w-NNNN".
    WorldActivationJob(const savedata::User& user, const std::string& afrTitleId,
                       const std::string& toWorldId);

    // Resuming an interrupted one. Starts at the journal's phase and takes its
    // save title, directory, policy and safety backup from the journal rather
    // than rediscovering them, so a console that has changed underneath still
    // finishes the transaction it started.
    WorldActivationJob(const savedata::User& user, const std::string& afrTitleId,
                       const ActivationJournal& journal);

    ~WorldActivationJob();

    WorldActivationJob(const WorldActivationJob&) = delete;
    WorldActivationJob& operator=(const WorldActivationJob&) = delete;

    void  Step();
    bool  Done() const;

    // One coarse line per thing that happened, drained by the caller. This is
    // what the editor's progress log shows - the same log the Enable wizard
    // already draws.
    std::vector<std::string> TakeLines();

    std::string StatusText() const;
    float Progress() const;
    int   Phase() const;
    const ActivationResult& Result() const;

private:
    struct State;
    std::unique_ptr<State> s_;
};

// ---------------------------------------------------------------------------
// Reconciliation - run at startup, before anything is listed
// ---------------------------------------------------------------------------

struct ReconcileResult {
    bool        ok = false;
    std::string error;

    int  sweptPartialBackups = 0;
    int  sweptWorldSaves     = 0;

    bool             hadJournal = false;
    ActivationJournal journal;
    ReconcileAction  action = ReconcileAction::Nothing;

    bool removedStaging = false;
    bool restoredTree   = false;
    bool resumed        = false;

    ActivationResult activation;   // meaningful when `resumed`
};

// Sweeps every SaveBackups/*.partial and this account's world-save debris,
// then reconciles any activation.journal. Safe and near-instant when there is
// no journal, which is the normal case - it is run every launch precisely so
// that the abnormal case cannot accumulate.
class WorldReconcileJob {
public:
    WorldReconcileJob(const savedata::User& user, const std::string& afrTitleId);
    ~WorldReconcileJob();

    WorldReconcileJob(const WorldReconcileJob&) = delete;
    WorldReconcileJob& operator=(const WorldReconcileJob&) = delete;

    void  Step();
    bool  Done() const;
    std::vector<std::string> TakeLines();
    std::string StatusText() const;
    float Progress() const;
    const ReconcileResult& Result() const;

    // True once the job has stopped FINDING OUT and started ACTING: the sweep
    // and the journal read are behind it and whatever the journal asked for is
    // either under way or over. False before the journal has been read.
    //
    // The startup screen's bar spends one step on each half (startup-screen
    // plan P1). The split cannot be read off Progress(), which is a fraction
    // and not a phase, nor off the line count, which depends on what the
    // journal said - so the job states it, read-only, and nothing about what
    // the job DOES changes.
    bool  ActingOnJournal() const;

private:
    struct State;
    std::unique_ptr<State> s_;
};

} // namespace bbr
