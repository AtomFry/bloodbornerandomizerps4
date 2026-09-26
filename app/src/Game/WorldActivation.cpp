#include "WorldActivation.h"

#include "../Platform/Log.h"
#include "../Randomizer/RandomizerDefaultsStore.h"

#include <orbis/libkernel.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

namespace bbr {

const char* const kSavePolicyKeep  = "keep-existing";
const char* const kSavePolicyFresh = "start-fresh";

namespace {

// Beside defaults.cfg and the worlds folder. Deliberately NOT inside AFR,
// which an activation rewrites wholesale and would take the journal with it -
// the one file that has to outlive the thing it describes.
const char* const kJournalPath = "/data/bbrandomizer/activation.journal";

// A one-time manual FTP drop point: nothing on the PS4 ships a vanilla
// dvdroot_ps4 copy (AFR is a redirect/overlay over the real installed game,
// not a full tree), so the randomizer needs its own read-only vanilla source
// to randomize FROM. Named here as well as in the Enable wizard until
// milestone 6 retires that screen; the two are the same string on purpose.
const char* const kVanillaSourceDir = "/data/bbrandomizer/VanillaSource/dvdroot_ps4";

// Same BSD open(2) flag values proven correct for sceKernelOpen everywhere
// else in this project - musl's <fcntl.h> values are Linux's and do not match
// this FreeBSD-derived kernel. Full table in docs/ps4-homebrew-findings.md.
const int kBsdRdonly = 0x0000;
const int kBsdWronly = 0x0001;
const int kBsdCreat  = 0x0200;
const int kBsdTrunc  = 0x0400;

const unsigned kModeIfmt  = 0xF000;
const unsigned kModeIfdir = 0x4000;

std::string U64Text(uint64_t value) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%llu", (unsigned long long)value);
    return std::string(buf);
}

std::string NowStamp() {
    time_t now = time(nullptr);
    struct tm* lt = localtime(&now);
    char buf[32];
    if (!lt || strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", lt) == 0) {
        return "(UNKNOWN)";
    }
    return std::string(buf);
}

uint64_t NowUs() { return sceKernelGetProcessTime(); }

bool IsDir(const std::string& path) {
    OrbisKernelStat st;
    if (sceKernelStat(path.c_str(), &st) != 0) return false;
    return ((unsigned)st.st_mode & kModeIfmt) == kModeIfdir;
}

bool ReadTextFile(const std::string& path, std::string& out) {
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

// fsync before close, always. The journal's entire value is that it is on the
// platter before the phase it names begins; a journal still in a cache when
// the power goes is a journal that describes the wrong phase.
bool WriteTextFileSynced(const std::string& path, const std::string& text) {
    int fd = sceKernelOpen(path.c_str(), kBsdWronly | kBsdCreat | kBsdTrunc, 0777);
    if (fd < 0) return false;
    int w = sceKernelWrite(fd, text.data(), text.size());
    sceKernelFsync(fd);
    sceKernelClose(fd);
    return w == (int)text.size();
}

void ApplyJournalLine(const std::string& key, const std::string& value,
                      ActivationJournal& out) {
    if      (key == "from_world")    out.fromWorld   = value;
    else if (key == "to_world")      out.toWorld     = value;
    else if (key == "revision")      out.revision    = value;
    else if (key == "save_policy")   out.savePolicy  = value;
    else if (key == "save_title_id") out.saveTitleId = value;
    else if (key == "save_dir_name") out.saveDirName = value;
    else if (key == "safety_backup") out.safetyBackup = value;
    else if (key == "phase")         out.phase       = atoi(value.c_str());
    // Unknown keys ignored, the same tolerance defaults.cfg has.
}

} // namespace

// ---------------------------------------------------------------------------
// Refusals
// ---------------------------------------------------------------------------

const char* RefusalReasonName(RefusalReason reason) {
    switch (reason) {
        case RefusalReason::None:                 return "NONE";
        case RefusalReason::NoUser:               return "USER";
        case RefusalReason::NotThisAccount:       return "OWNERSHIP";
        case RefusalReason::SaveTitle:            return "SAVE TITLE";
        case RefusalReason::SaveDirectory:        return "SAVE DIRECTORY";
        case RefusalReason::NoContainer:          return "CONTAINER";
        case RefusalReason::ContainerTooSmall:    return "CONTAINER SIZE";
        case RefusalReason::StoredSaveUnverified: return "MANIFEST";
        case RefusalReason::VanillaSourceMissing: return "SOURCE";
        case RefusalReason::AfrNotWritable:       return "AFR";
        case RefusalReason::EmptySelection:       return "SELECTION";
    }
    return "?";
}

namespace {

Refusal Refuse(RefusalReason reason, const std::string& sentence) {
    Refusal out;
    out.reason   = reason;
    out.sentence = sentence;
    return out;
}

} // namespace

Refusal CheckActivation(const ActivationFacts& f) {
    // Section 4.4, top to bottom. The order is the order of the table, and it
    // matters: the first row that fires is what the player is told, so the
    // cheap structural problems are reported before the expensive ones.

    // --- User ---
    if (!f.userValid || f.accountId == 0) {
        return Refuse(RefusalReason::NoUser,
                      f.userError.empty() ? "NO SIGNED-IN PLAYER" : f.userError);
    }

    // --- Ownership (B22) ---
    if (!f.worldExists) {
        return Refuse(RefusalReason::NotThisAccount,
                      "NO SUCH WORLD FOR THIS ACCOUNT");
    }
    if (f.worldAccountId != f.accountId) {
        return Refuse(RefusalReason::NotThisAccount,
                      "THIS WORLD BELONGS TO ANOTHER ACCOUNT");
    }

    // --- Save title (B29) - never the AFR title, which is a separate value ---
    if (f.saveTitlesWithHits == 0) {
        return Refuse(RefusalReason::SaveTitle,
                      "RUN BLOODBORNE ONCE BEFORE ACTIVATING THIS WORLD");
    }
    if (f.saveTitlesWithHits > 1) {
        return Refuse(RefusalReason::SaveTitle,
                      "MORE THAN ONE BLOODBORNE SAVE TITLE WAS FOUND");
    }

    // --- Save directory - never selected by index (findings section 9) ---
    if (f.saveDirCount == 0) {
        return Refuse(RefusalReason::SaveDirectory,
                      "NO SAVE DIRECTORY WAS FOUND FOR THIS TITLE");
    }
    if (f.saveDirCount > 1) {
        return Refuse(RefusalReason::SaveDirectory,
                      "MORE THAN ONE SAVE DIRECTORY WAS FOUND FOR THIS TITLE");
    }

    // --- Container (B33) - the app can never create one ---
    if (f.worldHasStoredSave && !f.containerExists) {
        return Refuse(RefusalReason::NoContainer,
                      "RUN BLOODBORNE ONCE BEFORE ACTIVATING THIS WORLD");
    }

    // --- Container size (B25) ---
    if (f.worldHasStoredSave && f.storedSaveBlocks > f.containerBlocks) {
        return Refuse(RefusalReason::ContainerTooSmall,
                      "THIS WORLD'S SAVE NEEDS " + U64Text(f.storedSaveBlocks) +
                      " BLOCKS AND THE CONTAINER HAS " + U64Text(f.containerBlocks));
    }

    // --- Manifest - walked, never taken on trust ---
    if (f.worldHasStoredSave && !f.storedSaveVerifies) {
        return Refuse(RefusalReason::StoredSaveUnverified,
                      "THIS WORLD'S SAVE DOES NOT MATCH ITS MANIFEST - " +
                      (f.storedSaveError.empty() ? std::string("NO REASON GIVEN")
                                                 : f.storedSaveError));
    }

    // --- Source - Vanilla generates nothing, so it needs none ---
    if (!f.worldIsVanilla && !f.vanillaSourcePresent) {
        return Refuse(RefusalReason::VanillaSourceMissing,
                      "THE VANILLA SOURCE IS MISSING - COPY DVDROOT_PS4 IN OVER "
                      "FTP FIRST");
    }

    // --- AFR. The title id itself is NOT checked against the installed
    // titles: it is used exactly as entered (plan P26). All that is asked is
    // whether the folder it names can be written to ---
    if (!f.afrWritable) {
        return Refuse(RefusalReason::AfrNotWritable,
                      "THE AFR FOLDER FOR " + f.afrTitleId + " CANNOT BE WRITTEN TO");
    }

    // --- the two guards carried over from the Enable wizard ---
    if (!f.worldIsVanilla && f.randomizeEnemies && !f.anyEnemySelected) {
        return Refuse(RefusalReason::EmptySelection,
                      "NO ENEMIES SELECTED - SELECT AT LEAST ONE OR TURN OFF "
                      "RANDOMIZE ENEMIES");
    }
    if (!f.worldIsVanilla && f.randomizeBosses && !f.anyBossSelected) {
        return Refuse(RefusalReason::EmptySelection,
                      "NO BOSSES SELECTED - SELECT AT LEAST ONE OR TURN OFF "
                      "RANDOMIZE BOSSES");
    }

    return Refusal();
}

// ---------------------------------------------------------------------------
// Phase 6's table
// ---------------------------------------------------------------------------

const char* SaveActionName(SaveAction action) {
    switch (action) {
        case SaveAction::Nothing:     return "NOTHING";
        case SaveAction::RestoreOwn:  return "RESTORE THIS WORLD'S SAVE";
        case SaveAction::AdoptLive:   return "ADOPT THE LIVE SAVE";
        case SaveAction::StartFresh:  return "START FRESH";
        case SaveAction::NoContainer: return "NO CONTAINER";
    }
    return "?";
}

const char* SaveActionSentence(SaveAction action) {
    switch (action) {
        case SaveAction::Nothing:
            return "UNCHANGED - THIS WORLD IS ALREADY ACTIVE";
        case SaveAction::RestoreOwn:
            return "THIS WORLD'S SAVE IS RESTORED";
        case SaveAction::AdoptLive:
            return "ADOPTS YOUR CURRENT SAVE - THIS WORLD HAS NONE";
        case SaveAction::StartFresh:
            return "BACKED UP, THEN A NEW PLAYTHROUGH BEGINS";
        case SaveAction::NoContainer:
            return "BLOODBORNE HAS NOT MADE A SAVE YET - IT WILL AT NEXT LAUNCH";
    }
    return "?";
}

// ---------------------------------------------------------------------------
// The journal
// ---------------------------------------------------------------------------

std::string ActivationJournalPath() { return kJournalPath; }

std::string FormatActivationJournal(const ActivationJournal& j) {
    char phase[32];
    snprintf(phase, sizeof(phase), "%d", j.phase);

    return std::string("from_world=") + j.fromWorld + "\n" +
           "to_world=" + j.toWorld + "\n" +
           "revision=" + j.revision + "\n" +
           "save_policy=" + j.savePolicy + "\n" +
           "save_title_id=" + j.saveTitleId + "\n" +
           "save_dir_name=" + j.saveDirName + "\n" +
           "safety_backup=" + j.safetyBackup + "\n" +
           "phase=" + phase + "\n";
}

bool ParseActivationJournal(const std::string& text, ActivationJournal& out) {
    out = ActivationJournal();

    bool sawTo = false;
    size_t pos = 0;
    while (pos <= text.size()) {
        size_t nl = text.find('\n', pos);
        std::string line = text.substr(pos, nl == std::string::npos
                                                ? std::string::npos : nl - pos);
        if (!line.empty() && line[line.size() - 1] == '\r') line.resize(line.size() - 1);

        size_t eq = line.find('=');
        if (eq != std::string::npos) {
            std::string key = line.substr(0, eq);
            if (key == "to_world") sawTo = true;
            ApplyJournalLine(key, line.substr(eq + 1), out);
        }

        if (nl == std::string::npos) break;
        pos = nl + 1;
    }

    // A journal naming no destination describes no transaction, which is the
    // one thing it exists to do. Treated as absent rather than as an empty
    // activation of nothing.
    if (!sawTo || out.toWorld.empty()) return false;
    out.present = true;
    return true;
}

bool ReadActivationJournal(ActivationJournal& out) {
    out = ActivationJournal();

    std::string text;
    if (!ReadTextFile(kJournalPath, text)) return false;
    return ParseActivationJournal(text, out);
}

bool WriteActivationJournal(const ActivationJournal& journal) {
    return WriteTextFileSynced(kJournalPath, FormatActivationJournal(journal));
}

void DeleteActivationJournal() { sceKernelUnlink(kJournalPath); }

ReconcileAction ActionForPhase(int phase) {
    // Exactly one action per phase, and every phase this app can write is
    // here. A phase number from nowhere leaves the disk alone: guessing what
    // a journal means is how a good save gets overwritten by a stale one.
    switch (phase) {
        case 1:
        case 2:
        case 3:
        case 4: return ReconcileAction::DiscardStaging;
        case 5: return ReconcileAction::RestoreTree;
        case 6: return ReconcileAction::ResumeSaveSwap;
        case 7: return ReconcileAction::FinishCommit;
        default: return ReconcileAction::Nothing;
    }
}

const char* ReconcileActionName(ReconcileAction action) {
    switch (action) {
        case ReconcileAction::Nothing:        return "NOTHING";
        case ReconcileAction::DiscardStaging: return "DISCARD THE STAGED TREE";
        case ReconcileAction::RestoreTree:    return "PUT THE OLD TREE BACK";
        case ReconcileAction::ResumeSaveSwap: return "RE-RUN THE SAVE SWAP";
        case ReconcileAction::FinishCommit:   return "FINISH THE COMMIT";
    }
    return "?";
}

// ---------------------------------------------------------------------------
// WorldActivationJob
// ---------------------------------------------------------------------------

namespace {

// Sub-stages within a phase. The phase number is what the journal carries and
// what reconciliation reads; these never leave this file.
enum {
    kSubStart = 0,
    kSubWork  = 1,
    kSubDone  = 2,
};

const int kPhaseFinished = 8;

} // namespace

struct WorldActivationJob::State {
    savedata::User user;
    std::string    afrTitleId;
    std::string    toWorldId;

    std::unique_ptr<WorldStore> store;

    int phase = 1;
    int sub   = kSubStart;

    bool resumed = false;
    ActivationJournal journal;

    std::unique_ptr<SafetyBackupJob>      backup;
    std::unique_ptr<StoreSaveJob>         capture;
    std::unique_ptr<EnemyRandomizerJob>   generate;
    std::unique_ptr<savedata::RestoreJob> restore;
    std::unique_ptr<StoreSaveJob>         adopt;
    std::unique_ptr<savedata::RestoreJob> recover;

    std::vector<std::string> pending;
    std::string status = "STARTING";
    uint64_t    startUs = 0;

    ActivationResult result;

    void Say(const std::string& line) {
        pending.push_back(line);
        Log(("activation: " + line).c_str());
    }

    void Finish(const std::string& why) {
        result.ok           = true;
        result.elapsedMs    = (NowUs() - startUs) / 1000;
        result.reachedPhase = phase;
        status              = why;
        Say(why);
        phase = kPhaseFinished;
    }

    // A failure leaves the journal exactly where it is, on purpose. The next
    // launch reconciles it; swallowing it here would turn a recoverable
    // interruption into a console nobody can reason about.
    void Fail(const std::string& why) {
        result.ok           = false;
        result.error        = why;
        result.reachedPhase = phase;
        result.elapsedMs    = (NowUs() - startUs) / 1000;
        status              = why;
        Say("FAILED - " + why);
        phase = kPhaseFinished;
    }

    void Refused(const Refusal& refusal) {
        result.ok              = false;
        result.refusal         = refusal;
        result.plan.refusal    = refusal;
        result.plan.ok         = false;
        result.reachedPhase    = 1;
        result.elapsedMs       = (NowUs() - startUs) / 1000;
        status                 = refusal.sentence;
        Say("REFUSED - " + std::string(RefusalReasonName(refusal.reason)));
        Say(refusal.sentence);
        phase = kPhaseFinished;
    }

    // The journal names the phase ABOUT TO RUN and is fsync'd before it runs.
    bool Advance(int nextPhase) {
        journal.phase = nextPhase;
        if (!WriteActivationJournal(journal)) {
            Fail("COULD NOT WRITE THE ACTIVATION JOURNAL");
            return false;
        }
        phase = nextPhase;
        sub   = kSubStart;
        return true;
    }
};

WorldActivationJob::WorldActivationJob(const savedata::User& user,
                                       const std::string& afrTitleId,
                                       const std::string& toWorldId)
    : s_(new State()) {
    s_->user       = user;
    s_->afrTitleId = afrTitleId;
    s_->toWorldId  = toWorldId;
    s_->startUs    = NowUs();
    if (user.valid) s_->store.reset(new WorldStore(user.accountId));
}

WorldActivationJob::WorldActivationJob(const savedata::User& user,
                                       const std::string& afrTitleId,
                                       const ActivationJournal& journal)
    : s_(new State()) {
    s_->user       = user;
    s_->afrTitleId = afrTitleId;
    s_->toWorldId  = journal.toWorld;
    s_->startUs    = NowUs();
    s_->resumed    = true;
    s_->journal    = journal;
    if (user.valid) s_->store.reset(new WorldStore(user.accountId));

    // Everything phase 1 would have discovered is already in the journal, and
    // the journal wins: a console that has changed underneath must still
    // finish the transaction it started rather than start a different one.
    ActivationPlan& p = s_->result.plan;
    p.ok          = true;
    p.fromWorldId = journal.fromWorld;
    p.toWorldId   = journal.toWorld;
    p.revisionId  = journal.revision;
    p.toVanilla   = (journal.toWorld == kVanillaWorldId);
    p.sameWorld   = (!journal.fromWorld.empty() && journal.fromWorld == journal.toWorld);
    p.afrTitleId  = afrTitleId;
    p.saveTitleId = journal.saveTitleId;
    p.saveDirName = journal.saveDirName;
    p.startFresh  = (journal.savePolicy == kSavePolicyFresh);

    s_->result.safetyBackupPath = journal.safetyBackup;
    s_->result.tookSafetyBackup = !journal.safetyBackup.empty();

    if (s_->store) {
        World world;
        p.toWorldName = (s_->store->Load(p.toWorldId, world) && !world.name.empty())
                            ? world.name : p.toWorldId;
        World from;
        if (!p.fromWorldId.empty()) {
            p.fromWorldName = (s_->store->Load(p.fromWorldId, from) && !from.name.empty())
                                  ? from.name : p.fromWorldId;
        }
    }

    s_->phase = journal.phase;
    s_->sub   = kSubStart;
}

WorldActivationJob::~WorldActivationJob() = default;

bool WorldActivationJob::Done() const { return s_->phase >= kPhaseFinished; }
std::string WorldActivationJob::StatusText() const { return s_->status; }
int WorldActivationJob::Phase() const { return s_->phase; }
const ActivationResult& WorldActivationJob::Result() const { return s_->result; }

std::vector<std::string> WorldActivationJob::TakeLines() {
    std::vector<std::string> out;
    out.swap(s_->pending);
    return out;
}

float WorldActivationJob::Progress() const {
    State& s = *s_;
    switch (s.phase) {
        case 1: return 0.02f;
        case 2: return 0.05f + (s.backup   ? s.backup->Progress()   * 0.25f : 0.0f);
        case 3: return 0.30f + (s.capture  ? s.capture->Progress()  * 0.15f : 0.0f);
        case 4: return 0.45f + (s.generate ? s.generate->Progress() * 0.35f : 0.0f);
        case 5: return 0.82f;
        case 6: return 0.85f + (s.restore  ? s.restore->Progress()  * 0.13f
                             : (s.adopt    ? s.adopt->Progress()    * 0.13f : 0.0f));
        case 7: return 0.99f;
        default: return 1.0f;
    }
}

namespace {

// Phase 1, the gathering half. Every read here is a read - nothing in this
// function writes, which is what makes "a refused activation changed nothing"
// a claim rather than a hope. Takes the four things it needs rather than the
// job's State, so it stays a plain function over plain data.
void GatherFacts(const savedata::User& user, const std::string& afrTitleId,
                 const std::string& toWorldId, WorldStore& store,
                 ActivationPlan& plan);

} // namespace

ActivationPlan PlanActivation(const savedata::User& user,
                              const std::string& afrTitleId,
                              const std::string& toWorldId) {
    ActivationPlan plan;

    if (!user.valid || user.accountId == 0) {
        plan.facts.userValid = false;
        plan.facts.userError = user.error;
        plan.refusal = CheckActivation(plan.facts);
        return plan;
    }

    WorldStore store(user.accountId);
    GatherFacts(user, afrTitleId, toWorldId, store, plan);

    // Walked, not taken on trust, and only when there is something to walk:
    // this is the check that must pass before a world's save is allowed
    // anywhere near a container.
    if (plan.facts.worldHasStoredSave) {
        std::string error;
        plan.facts.storedSaveVerifies = store.VerifyStoredSave(toWorldId, error, nullptr);
        plan.facts.storedSaveError    = error;
    }

    plan.refusal = CheckActivation(plan.facts);
    if (plan.refusal.Refused()) return plan;

    // Which row of section 4.3's phase-6 table applies. Ordered exactly as
    // that table is: no container wins over everything, then START FRESH,
    // then "this world is already the active one".
    if (!plan.containerExists)              plan.saveAction = SaveAction::NoContainer;
    else if (plan.startFresh)               plan.saveAction = SaveAction::StartFresh;
    else if (plan.sameWorld)                plan.saveAction = SaveAction::Nothing;
    else if (plan.facts.worldHasStoredSave) plan.saveAction = SaveAction::RestoreOwn;
    else if (plan.containerHasGameFiles)    plan.saveAction = SaveAction::AdoptLive;
    else                                    plan.saveAction = SaveAction::Nothing;

    plan.ok = true;
    return plan;
}

void WorldActivationJob::Step() {
    State& s = *s_;
    if (s.phase >= kPhaseFinished) return;

    switch (s.phase) {

    // ======================================================================
    // Phase 1 - check. Nothing is written.
    // ======================================================================
    case 1: {
        // Said one step before the work, so the screen has a frame in which to
        // draw it: gathering the facts reads the whole container and walks the
        // incoming world's stored save, which is a second or two of blocking
        // work in a frame loop that cannot draw while it runs.
        if (s.sub == kSubStart) {
            s.Say("CHECKING");
            s.status = "CHECKING";
            s.sub = kSubWork;
            return;
        }

        s.result.plan = PlanActivation(s.user, s.afrTitleId, s.toWorldId);

        ActivationPlan& p = s.result.plan;
        if (p.refusal.Refused()) { s.Refused(p.refusal); return; }

        s.Say("ACTIVATING " + p.toWorldName + " - " + p.toWorldId +
              (p.revisionId.empty() ? std::string() : " " + p.revisionId));
        s.Say("DEACTIVATING " + (p.fromWorldId.empty()
                                     ? std::string("NOTHING - NO WORLD IS ACTIVE")
                                     : p.fromWorldName + " - " + p.fromWorldId));
        s.Say("SAVE DATA " + std::string(SaveActionName(p.saveAction)));
        s.Say(SaveActionSentence(p.saveAction));

        // The journal starts here, because phase 2 is the first phase that
        // writes anything.
        s.journal = ActivationJournal();
        s.journal.fromWorld   = p.fromWorldId;
        s.journal.toWorld     = p.toWorldId;
        s.journal.revision    = p.revisionId;
        s.journal.savePolicy  = p.startFresh ? kSavePolicyFresh : kSavePolicyKeep;
        s.journal.saveTitleId = p.saveTitleId;
        s.journal.saveDirName = p.saveDirName;

        s.Advance(2);
        return;
    }

    // ======================================================================
    // Phase 2 - the safety backup. Any failure aborts everything (D24).
    // ======================================================================
    case 2: {
        ActivationPlan& p = s.result.plan;

        if (s.sub == kSubStart) {
            // Skipped when there is no container, or when the container holds
            // no save files at all - there is nothing to back up, and an empty
            // capture over a world's good stored save would be destroying it
            // in the one case where the backup that normally covers such a
            // replacement cannot be taken (spec worlds D26).
            if (!p.containerExists || !p.containerHasGameFiles) {
                s.Say(p.containerExists
                          ? "NO SAVE FILES IN THE CONTAINER - NOTHING IS BACKED UP"
                          : "NO SAVE CONTAINER - NOTHING IS BACKED UP");
                if (!p.fromWorldId.empty()) {
                    s.Say("THE OUTGOING WORLD'S STORED SAVE IS LEFT EXACTLY AS IT WAS");
                }
                s.Advance(4);
                return;
            }

            s.Say("BACKING UP THE LIVE SAVE");
            s.backup.reset(new SafetyBackupJob(s.user, p.saveTitleId, p.saveDirName,
                                               "preactivate", p.fromWorldId,
                                               p.fromRevision));
            s.sub = kSubWork;
            return;
        }

        s.backup->Step();
        s.status = s.backup->StatusText();
        if (!s.backup->Done()) return;

        const SafetyBackupResult& r = s.backup->Result();
        if (!r.ok) {
            // A backup that did not complete is never treated as one (D24).
            s.Fail("THE SAFETY BACKUP FAILED - " + r.error);
            return;
        }

        s.result.tookSafetyBackup = true;
        s.result.safetyBackupPath = r.path;
        s.result.backupFiles      = r.files;
        s.result.backupBytes      = r.bytes;
        s.Say("SAFETY BACKUP VERIFIED - " + U64Text(r.files) + " FILE(S), " +
              U64Text(r.bytes) + " BYTES");
        s.Say(r.path);

        s.journal.safetyBackup = r.path;
        s.Advance(3);
        return;
    }

    // ======================================================================
    // Phase 3 - capture that backup into the outgoing world
    // ======================================================================
    case 3: {
        ActivationPlan& p = s.result.plan;

        if (s.sub == kSubStart) {
            if (p.fromWorldId.empty()) {
                s.Say("NO OUTGOING WORLD - THE BACKUP IS KEPT BUT FILED NOWHERE");
                s.Advance(4);
                return;
            }
            s.Say("FILING THE BACKUP INTO " + p.fromWorldName);
            s.capture.reset(new StoreSaveJob(s.result.safetyBackupPath,
                                             s.store->SaveDir(p.fromWorldId),
                                             p.fromWorldId, p.fromRevision));
            s.sub = kSubWork;
            return;
        }

        s.capture->Step();
        s.status = s.capture->StatusText();
        if (!s.capture->Done()) return;

        const StoreSaveResult& r = s.capture->Result();
        if (!r.ok) { s.Fail("COULD NOT FILE THE BACKUP - " + r.error); return; }

        // When the save was last written, and which revision was current then
        // (spec worlds D3). This is also what the rail sorts on.
        std::string error;
        s.store->SetLastPlayed(p.fromWorldId, p.fromRevision, error);

        s.result.capturedOutgoing = true;
        s.result.capturedFiles    = r.files;
        s.Say(p.fromWorldName + " NOW HOLDS " + U64Text(r.files) + " FILE(S), " +
              U64Text(r.bytes) + " BYTES");

        s.Advance(4);
        return;
    }

    // ======================================================================
    // Phase 4 - generate into the staging tree and stamp it
    // ======================================================================
    case 4: {
        ActivationPlan& p = s.result.plan;

        if (s.sub == kSubStart) {
            if (p.toVanilla) {
                s.Say("VANILLA - NOTHING IS GENERATED");
                s.Advance(5);
                return;
            }

            WorldRevision revision;
            if (!s.store->CurrentRevision(p.toWorldId, revision)) {
                s.Fail("COULD NOT READ " + p.toWorldId + "'S SETTINGS");
                return;
            }
            const RandomizerDefaults& run = revision.recipe.settings;

            // Field for field, in the order the Enable wizard maps them, and
            // for the same reasons its comments give. The ONLY difference
            // between this run and the one that screen makes is where the tree
            // lands: same seed, same options, same tree (B28).
            EnemyRandomizerOptions options;
            options.randomizeEnemies = run.randomizeEnemies;
            options.randomizeBosses = run.randomizeBosses;
            options.randomizeTreasure = run.randomizeTreasure;
            options.randomizeWorkshopTools = run.randomizeWorkshopTools;
            options.randomizeEnemyDrops = run.randomizeEnemyDrops;
            options.randomizeStartingWeapons = run.randomizeStartingWeapons;
            options.randomizeStartingGuns = run.randomizeStartingGuns;
            options.randomizeShopWeapons = run.randomizeShopWeapons;
            options.enableMergoDarkness = run.enableMergoDarkness;
            options.doNotRandomizeCagedDogs = run.doNotRandomizeCagedDogs;
            options.startWithHunterTools = run.startWithHunterTools;
            options.easyModes.shadows = run.easyShadows;
            options.easyModes.rom = run.easyRom;
            options.easyModes.failures = run.easyFailures;
            options.easyModes.emissary = run.easyEmissary;
            options.enemiesIncluded = run.enemiesIncluded;
            options.bossesIncluded = run.bossesIncluded;
            options.enemiesSkipped = run.enemiesSkipped;

            // The Enable wizard's run decision, kept verbatim, and reported
            // here rather than used to skip the run: a world with nothing
            // switched on is still a world, and it needs a real tree to carry
            // .bbrandomizer_manifest - which is the only record of which world
            // is active (D11). With every option off the job mirrors the
            // vanilla tree unchanged, so that world plays exactly as the game
            // shipped while still being a world the app can see.
            bool anythingOn =
                run.randomizeEnemies || run.randomizeBosses || run.randomizeTreasure ||
                run.randomizeEnemyDrops || run.randomizeStartingWeapons ||
                run.randomizeStartingGuns || run.randomizeShopWeapons ||
                run.enableMergoDarkness || run.startWithHunterTools ||
                run.easyShadows || run.easyRom || run.easyFailures || run.easyEmissary;
            if (!anythingOn) {
                s.Say("NO RANDOMIZER SETTINGS ARE ON - THE TREE IS A PLAIN MIRROR");
            }

            // Debris from an interrupted run is not a starting point: half of
            // one world's tree under half of another's is exactly the silently
            // incoherent state this transaction exists to prevent.
            AfrManager::RemoveStaging(p.afrTitleId);

            s.Say("GENERATING WITH SEED " + U64Text(revision.recipe.seed));
            s.generate.reset(new EnemyRandomizerJob(
                kVanillaSourceDir, AfrManager::StagingDvdroot(p.afrTitleId),
                revision.recipe.seed, options));
            s.sub = kSubWork;
            return;
        }

        s.generate->Step();
        s.status = s.generate->StatusText();
        if (!s.generate->Done()) return;

        s.result.generation = s.generate->Result();
        if (!s.result.generation.success) {
            AfrManager::RemoveStaging(p.afrTitleId);
            s.Fail("GENERATION FAILED - " + s.result.generation.error);
            return;
        }
        s.result.generated = true;
        s.Say("GENERATED " + U64Text((uint64_t)s.result.generation.mapsProcessed) +
              " MAP(S)");

        // The marker goes into the STAGED tree, before the swap, so it arrives
        // with the content it describes and cannot exist without it (D11).
        AfrManifest marker;
        marker.worldId   = p.toWorldId;
        marker.worldName = p.toWorldName;
        marker.revision  = p.revisionId;
        marker.seed      = p.seed;
        marker.titleId   = p.afrTitleId;
        marker.accountId = s.user.accountId;
        marker.written   = NowStamp();
        if (!AfrManager::WriteManifestAt(AfrManager::StagingDvdroot(p.afrTitleId),
                                         marker)) {
            AfrManager::RemoveStaging(p.afrTitleId);
            s.Fail("COULD NOT STAMP THE STAGED TREE");
            return;
        }

        s.Advance(5);
        return;
    }

    // ======================================================================
    // Phase 5 - swap the files
    // ======================================================================
    case 5: {
        ActivationPlan& p = s.result.plan;

        // On a resume, work out whether the rename pair completed before
        // redoing it. The .old tree alone does not answer that - it is present
        // both when the second rename has not happened and when only the
        // cleanup was lost. WHAT ANSWERS IT IS THE STAGED TREE: phase 4 does
        // not advance until the tree is complete and stamped, and phase 5's
        // second rename is what consumes it. A staged tree still sitting there
        // means the swap did not finish. Vanilla stages nothing, so for
        // Vanilla the question is simply whether the live tree is gone.
        if (s.resumed && s.sub == kSubStart) {
            bool restoredTree = false;
            if (!AfrManager::RollBackSwap(p.afrTitleId, restoredTree)) {
                s.Fail("COULD NOT PUT THE PREVIOUS TREE BACK");
                return;
            }
            if (restoredTree) {
                s.result.treeRolledBack = true;
                s.Say("THE PREVIOUS TREE WAS PUT BACK");
            }

            bool complete = p.toVanilla ? !AfrManager::Check(p.afrTitleId).seeded
                                        : !AfrManager::StagingExists(p.afrTitleId);
            if (complete) {
                s.Say("THE FILE SWAP HAD ALREADY COMPLETED");
                s.result.swapped = true;
                s.Advance(6);
                return;
            }

            s.Say("THE FILE SWAP DID NOT COMPLETE - DOING IT AGAIN");
            s.sub = kSubWork;
            return;
        }

        std::string error;
        if (!AfrManager::Swap(p.afrTitleId, !p.toVanilla, error)) {
            AfrManager::RemoveStaging(p.afrTitleId);
            s.Fail(error);
            return;
        }
        s.result.swapped = true;
        s.Say(p.toVanilla ? "THE RANDOMIZER'S FILES ARE REMOVED"
                          : "THE NEW TREE IS LIVE");

        s.Advance(6);
        return;
    }

    // ======================================================================
    // Phase 6 - swap the save, per section 4.3's table
    // ======================================================================
    case 6: {
        ActivationPlan& p = s.result.plan;

        if (s.sub == kSubStart) {
            // A resumed run recomputes the row from the journal's own data
            // rather than trusting a plan it never built.
            if (s.resumed) {
                p.saveAction = SaveAction::Nothing;
                if (p.startFresh) {
                    p.saveAction = SaveAction::StartFresh;
                } else if (!p.sameWorld && s.store &&
                           s.store->HasStoredSave(p.toWorldId)) {
                    p.saveAction = SaveAction::RestoreOwn;
                } else if (!p.sameWorld && !s.result.safetyBackupPath.empty()) {
                    p.saveAction = SaveAction::AdoptLive;
                }
                s.Say("RESUMING THE SAVE SWAP - " +
                      std::string(SaveActionName(p.saveAction)));
            }

            switch (p.saveAction) {
            case SaveAction::RestoreOwn:
                s.Say("PUTTING " + p.toWorldName + "'S SAVE BACK");
                s.restore.reset(new savedata::RestoreJob(s.user, p.saveTitleId,
                                                         p.saveDirName,
                                                         s.store->SaveDir(p.toWorldId)));
                s.sub = kSubWork;
                return;

            case SaveAction::AdoptLive:
                s.Say(p.toWorldName + " HAS NO SAVE - ADOPTING THE LIVE ONE");
                s.adopt.reset(new StoreSaveJob(s.result.safetyBackupPath,
                                               s.store->SaveDir(p.toWorldId),
                                               p.toWorldId, p.revisionId));
                s.sub = kSubWork;
                return;

            case SaveAction::StartFresh: {
                // findings 8.3. The container is EMPTIED, never destroyed -
                // this app cannot create one, so destroying it would be
                // unrecoverable without the game.
                s.Say("EMPTYING THE SAVE CONTAINER");
                savedata::EmptyResult empty =
                    savedata::EmptyContainer(s.user, p.saveTitleId, p.saveDirName);
                if (!empty.ok) { s.Fail("COULD NOT EMPTY THE CONTAINER - " + empty.error); return; }
                if (empty.refused > 0) {
                    s.Fail(U64Text((uint64_t)empty.refused) + " FILE(S) REFUSED UNLINK");
                    return;
                }
                s.result.emptiedFiles = (uint64_t)empty.removed;
                s.Say("REMOVED " + U64Text((uint64_t)empty.removed) + " FILE(S), LEFT " +
                      U64Text((uint64_t)empty.kept) + " ALONE");
                s.Advance(7);
                return;
            }

            case SaveAction::NoContainer:
                s.Say("NO SAVE CONTAINER - BLOODBORNE MAKES ONE AT NEXT LAUNCH");
                s.Advance(7);
                return;

            case SaveAction::Nothing:
            default:
                s.Say("THE SAVE ON THE CONSOLE IS ALREADY THE RIGHT ONE");
                s.Advance(7);
                return;
            }
        }

        if (s.restore) {
            s.restore->Step();
            s.status = s.restore->StatusText();
            if (!s.restore->Done()) return;

            const savedata::RestoreResult& r = s.restore->Result();
            if (!r.ok) {
                s.restore.reset();
                if (s.resumed) { s.sub = kSubDone; return; }   // recover below
                s.Fail("THE RESTORE FAILED - " + r.error);
                return;
            }
            s.result.restoredFiles = r.files;
            s.Say("RESTORED " + U64Text(r.files) + " FILE(S), " + U64Text(r.bytes) +
                  " BYTES, AFTER REMOVING " + U64Text(r.removed));
            s.Advance(7);
            return;
        }

        if (s.adopt) {
            s.adopt->Step();
            s.status = s.adopt->StatusText();
            if (!s.adopt->Done()) return;

            const StoreSaveResult& r = s.adopt->Result();
            if (!r.ok) { s.Fail("COULD NOT ADOPT THE LIVE SAVE - " + r.error); return; }

            std::string error;
            s.store->SetLastPlayed(p.toWorldId, p.revisionId, error);
            s.result.adoptedFiles = r.files;
            s.Say(p.toWorldName + " ADOPTED " + U64Text(r.files) + " FILE(S)");
            s.Advance(7);
            return;
        }

        // Only reachable on a resume whose restore could not complete. Section
        // 4.3: put the named safety backup back, and report.
        if (s.sub == kSubDone && !s.recover) {
            if (s.result.safetyBackupPath.empty()) {
                s.Fail("THE SAVE SWAP COULD NOT BE RE-RUN AND THERE IS NO BACKUP TO PUT BACK");
                return;
            }
            s.Say("THE SAVE SWAP COULD NOT BE RE-RUN - PUTTING THE SAFETY BACKUP BACK");
            s.recover.reset(new savedata::RestoreJob(s.user, p.saveTitleId, p.saveDirName,
                                                     s.result.safetyBackupPath));
            return;
        }

        if (!s.recover) { s.Fail("THE SAVE SWAP LOST TRACK OF ITSELF"); return; }

        s.recover->Step();
        s.status = s.recover->StatusText();
        if (!s.recover->Done()) return;

        const savedata::RestoreResult& rr = s.recover->Result();
        if (!rr.ok) { s.Fail("THE SAFETY BACKUP COULD NOT BE PUT BACK - " + rr.error); return; }
        s.result.recoveredFromBackup = true;
        s.Say("THE SAFETY BACKUP IS BACK ON THE CONSOLE - " + U64Text(rr.files) +
              " FILE(S)");
        s.Fail("THE SAVE SWAP COULD NOT BE COMPLETED - THE SAVE WAS PUT BACK");
        return;
    }

    // ======================================================================
    // Phase 7 - commit
    // ======================================================================
    case 7: {
        ActivationPlan& p = s.result.plan;

        // START FRESH applies to ONE activation and then reverts, recorded as
        // a revision (B12). A stored one would make the world decline to load
        // its own save on every future activation - the silently incoherent
        // failure the whole feature exists to prevent.
        if (p.startFresh && !p.toVanilla && s.store) {
            WorldRevision current;
            if (s.store->CurrentRevision(p.toWorldId, current)) {
                WorldRecipe reverted = current.recipe;
                reverted.settings.startFreshSave = false;

                std::string revId, error;
                bool appended = false;
                if (s.store->AppendRevision(p.toWorldId, reverted, revId, appended, error)) {
                    s.result.revertedSavePolicy = appended;
                    s.result.revertRevisionId   = revId;
                    s.Say("SAVE DATA IS BACK ON KEEP EXISTING - " + revId);
                } else {
                    s.Say("COULD NOT REVERT SAVE DATA - " + error);
                }
            }
        }

        DeleteActivationJournal();
        s.Finish(p.toWorldName + " IS ACTIVE");
        return;
    }

    default:
        s.Fail("UNKNOWN ACTIVATION PHASE");
        return;
    }
}

namespace {

void GatherFacts(const savedata::User& user, const std::string& afrTitleId,
                 const std::string& toWorldId, WorldStore& store,
                 ActivationPlan& p) {
    ActivationFacts& f = p.facts;

    f.userValid = user.valid;
    f.userError = user.error;
    f.accountId = user.accountId;

    p.afrTitleId  = afrTitleId;
    p.toWorldId   = toWorldId;
    p.toVanilla   = (toWorldId == kVanillaWorldId);
    f.worldIsVanilla = p.toVanilla;
    f.afrTitleId  = afrTitleId;

    // --- the incoming world ---
    World world;
    if (store.Load(toWorldId, world)) {
        f.worldExists     = true;
        f.worldAccountId  = world.accountId;
        p.toWorldName     = world.name.empty() ? world.id : world.name;
        p.revisionId      = world.currentRevision;

        f.worldHasStoredSave = world.hasSave;
        f.storedSaveBlocks   = world.saveBlocks;

        WorldRevision revision;
        if (!p.toVanilla && store.CurrentRevision(toWorldId, revision)) {
            p.seed          = revision.recipe.seed;
            p.startFresh    = revision.recipe.settings.startFreshSave;
            f.randomizeEnemies = revision.recipe.settings.randomizeEnemies;
            f.randomizeBosses  = revision.recipe.settings.randomizeBosses;
            f.anyEnemySelected = !revision.recipe.settings.enemiesIncluded.NoneEnabled();
            f.anyBossSelected  = !revision.recipe.settings.bossesIncluded.NoneEnabled();
        }
    }

    // --- what is active now, derived from disk rather than a stored flag ---
    AfrStatus   status = AfrManager::Check(afrTitleId);
    AfrManifest manifest;
    AfrManager::ReadManifest(afrTitleId, manifest);
    bool known = manifest.present && store.Exists(manifest.worldId);

    AfrActiveWorld active = AfrManager::DeriveActive(status, manifest,
                                                     store.AccountDirExists(), known);
    if (active.state == AfrActiveState::World) {
        p.fromWorldId  = active.worldId;
        p.fromRevision = active.revision;

        World from;
        p.fromWorldName = (store.Load(active.worldId, from) && !from.name.empty())
                              ? from.name : active.worldId;
    } else if (active.state == AfrActiveState::Vanilla &&
               store.Exists(kVanillaWorldId)) {
        // Vanilla is a world like any other, so deactivating it files the live
        // save into it exactly as deactivating a randomized world would.
        p.fromWorldId   = kVanillaWorldId;
        p.fromWorldName = kVanillaWorldName;
        p.fromRevision  = "";
    }
    p.sameWorld = (!p.fromWorldId.empty() && p.fromWorldId == p.toWorldId);

    f.afrWritable = status.writable;

    f.vanillaSourcePresent = IsDir(kVanillaSourceDir);

    // --- save data. The save title is DISCOVERED and is not the AFR title ---
    savedata::SaveTitle title = savedata::DiscoverSaveTitle(user);
    f.saveTitlesWithHits = title.titlesWithHits;
    if (title.found) {
        f.saveDirCount = (int)title.dirNames.size();
        p.saveTitleId  = title.titleId;
        if (title.dirNames.size() == 1) {
            p.saveDirName = title.dirNames[0];

            savedata::Container container =
                savedata::ReadContainer(user, p.saveTitleId, p.saveDirName);
            p.containerExists  = container.exists;
            p.containerBlocks  = container.blocks;
            f.containerExists  = container.exists;
            f.containerBlocks  = container.blocks;
            for (size_t i = 0; i < container.files.size(); i++) {
                if (savedata::IsGameSaveFile(container.files[i].rel)) {
                    p.containerHasGameFiles = true;
                    break;
                }
            }
        }
    }
}

} // namespace

// ---------------------------------------------------------------------------
// WorldReconcileJob
// ---------------------------------------------------------------------------

struct WorldReconcileJob::State {
    savedata::User user;
    std::string    afrTitleId;

    std::unique_ptr<WorldActivationJob> resume;

    int         stage = 0;   // 0 sweep, 1 read, 2 act, 3 resume, 4 done
    std::string status = "STARTING";
    std::vector<std::string> pending;
    ReconcileResult result;

    void Say(const std::string& line) {
        pending.push_back(line);
        Log(("reconcile: " + line).c_str());
    }

    void Finish(const std::string& why, bool ok) {
        result.ok = ok;
        status    = why;
        Say(why);
        stage = 4;
    }
};

WorldReconcileJob::WorldReconcileJob(const savedata::User& user,
                                     const std::string& afrTitleId)
    : s_(new State()) {
    s_->user       = user;
    s_->afrTitleId = afrTitleId;
}

WorldReconcileJob::~WorldReconcileJob() = default;

bool WorldReconcileJob::Done() const { return s_->stage >= 4; }

// Stages 0 and 1 are the sweep and the journal read; 2 and 3 are the action
// and the resumed activation. Done() is checked first everywhere this is
// used, so the finished stage 4 reading true here is not a case anything sees.
bool WorldReconcileJob::ActingOnJournal() const { return s_->stage >= 2; }
std::string WorldReconcileJob::StatusText() const { return s_->status; }
const ReconcileResult& WorldReconcileJob::Result() const { return s_->result; }

std::vector<std::string> WorldReconcileJob::TakeLines() {
    std::vector<std::string> out;
    out.swap(s_->pending);
    return out;
}

float WorldReconcileJob::Progress() const {
    State& s = *s_;
    if (s.stage >= 4) return 1.0f;
    if (s.resume) return 0.2f + s.resume->Progress() * 0.8f;
    return 0.1f * (float)s.stage;
}

void WorldReconcileJob::Step() {
    State& s = *s_;
    if (s.stage >= 4) return;

    // --- the sweep, always, before anything is listed --------------------
    if (s.stage == 0) {
        s.result.sweptPartialBackups = SweepPartialBackups();
        if (s.user.valid) {
            WorldStore store(s.user.accountId);
            s.result.sweptWorldSaves = store.SweepPartialWorldSaves();
        }
        s.Say("SWEPT " + U64Text((uint64_t)s.result.sweptPartialBackups) +
              " PARTIAL BACKUP(S), " +
              U64Text((uint64_t)s.result.sweptWorldSaves) + " PARTIAL WORLD SAVE(S)");
        s.stage = 1;
        return;
    }

    // --- the journal ------------------------------------------------------
    if (s.stage == 1) {
        if (!ReadActivationJournal(s.result.journal)) {
            s.Finish("NO ACTIVATION WAS INTERRUPTED", true);
            return;
        }
        s.result.hadJournal = true;
        s.result.action     = ActionForPhase(s.result.journal.phase);

        s.Say("AN ACTIVATION WAS INTERRUPTED AT PHASE " +
              U64Text((uint64_t)s.result.journal.phase));
        s.Say("FROM " + (s.result.journal.fromWorld.empty()
                             ? std::string("NOTHING") : s.result.journal.fromWorld) +
              " TO " + s.result.journal.toWorld);
        s.Say("ACTION " + std::string(ReconcileActionName(s.result.action)));
        s.stage = 2;
        return;
    }

    // --- the action -------------------------------------------------------
    if (s.stage == 2) {
        switch (s.result.action) {
        case ReconcileAction::Nothing:
            // A phase number no version of this app wrote. The journal is left
            // on disk to be looked at rather than acted on or deleted.
            s.Finish("THE JOURNAL'S PHASE IS NOT ONE THIS APP WRITES - LEFT ALONE", false);
            return;

        case ReconcileAction::DiscardStaging:
            s.result.removedStaging = AfrManager::StagingExists(s.afrTitleId);
            AfrManager::RemoveStaging(s.afrTitleId);
            DeleteActivationJournal();
            s.Finish(s.result.removedStaging
                         ? "THE STAGED TREE WAS DISCARDED - NOTHING ELSE CHANGED"
                         : "NOTHING HAD BEEN CHANGED YET",
                     true);
            return;

        case ReconcileAction::RestoreTree:
        case ReconcileAction::ResumeSaveSwap:
        case ReconcileAction::FinishCommit:
            if (!s.user.valid) {
                s.Finish("NO PLAYER SIGNED IN", false);
                return;
            }
            s.resume.reset(new WorldActivationJob(s.user, s.afrTitleId, s.result.journal));
            s.result.resumed = true;
            s.stage = 3;
            return;
        }
        return;
    }

    // --- finishing the interrupted transaction ----------------------------
    s.resume->Step();
    s.status = s.resume->StatusText();

    std::vector<std::string> fresh = s.resume->TakeLines();
    for (size_t i = 0; i < fresh.size(); i++) s.Say("  " + fresh[i]);

    if (!s.resume->Done()) return;

    s.result.activation   = s.resume->Result();
    s.result.restoredTree = s.result.activation.treeRolledBack;
    if (s.result.activation.ok) {
        s.Finish("THE INTERRUPTED ACTIVATION WAS FINISHED", true);
    } else {
        s.result.error = s.result.activation.error;
        s.Finish("THE INTERRUPTED ACTIVATION COULD NOT BE FINISHED", false);
    }
}

} // namespace bbr
