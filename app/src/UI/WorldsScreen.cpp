#include "WorldsScreen.h"

#include "Controls.h"
#include "SettingsModel.h"

#include "../Platform/Log.h"

#include <stdio.h>

#include <string>
#include <vector>

namespace bbr {

namespace {

// --- text scales -----------------------------------------------------------
const int kHeadingScale  = 4;   // the details pane's heading - the row's name
const int kRowScale      = 3;   // rail rows, detail rows, help
const int kFooterScale   = 3;
const int kStartupTitleScale = 5;
const int kStartupSubScale   = 4;
const int kProgressScale     = 3;

// --- the three columns -----------------------------------------------------
//
// The SAME split the two categorised screens use - 60 + 580 + 20 + 2 + 18 +
// 700 + 20 + 2 + 18 + 440 + 60 = 1920 - so switching tabs moves the content
// and not the furniture. settings_ui_verify.py compares these against the
// other screens' copies and fails if any of them drifts.
const int kRailX          = 60;
const int kRailW          = 580;
const int kPaneX          = 680;
const int kPaneW          = 700;
const int kPaneValueRight = 1380;
const int kHelpX          = 1420;
const int kHelpW          = 440;

// --- vertical furniture ----------------------------------------------------
//
// The tab strip and the heading beneath it come from Controls.h, which both
// tabbed screens draw from. Everything below the header rule is this screen's.
const int kStateY       = 140;   // the derived state, right-aligned, beside
                                 // the heading - the wizard's TARGET readout
                                 // in the same band
const int kStateRight   = 1860;
const int kHeaderRuleY  = 196;
const int kColumnRuleY  = 210;
const int kColumnRuleH  = 750;
const int kPaneHeadingY = 220;
const int kHelpTitleY   = 220;
const int kHelpPitch    = 52;
const int kHelpRuleY    = 330;
const int kHelpBodyY    = 356;
const int kHelpTitleMaxLines = 2;
const int kHelpBodyMaxLines  = 11;
const int kFooterY      = 1000;

const int kRuleThickness = 2;

// The focus bar, as on every other rail in the app.
const int kBarOffsetY = -10;
const int kBarHeight  = 64;

const Color kRuleColor = { 64, 72, 82 };

// The rail: one row per world, in the same band and at the same pitch as the
// settings pane opposite it. It scrolls, because the number of worlds is the
// player's business and not a layout constant.
const ListLayout kRailLayout = { 330, 76, 880, 52 };

// The details pane. Denser than a settings pane - these are short readouts
// and wrapped sentences rather than editable rows - so it fits eleven lines
// where the settings pane fits eight. ui_scroll_verify.py checks that the
// worst case this screen can build still fits, because nothing here has a
// cursor: a details pane that scrolled would be one the player cannot scroll.
const ListLayout kDetailLayout = { 330, 52, 880, 44 };

// The active marker: a swatch in Palette::Good at the rail row's right edge
// (worlds plan section 4.5). Not a glyph - the 8x8 fallback has no symbol for
// it, and a word there would collide with a 16-character world name.
//
// Nothing here clips the row's label: the clear space between the widest name
// a player can type and the swatch is a REQUIREMENT, asserted by
// settings_ui_verify.py against the atlas, in the same way the picker's flag
// column is. A 16-character name is the cap the plan set (P10) precisely so
// this band can be checked rather than defended at draw time.
const int kSwatchW       = 16;
const int kSwatchH       = 32;
const int kSwatchOffsetY = 12;   // relative to the row's draw y; at scale 3 the
                                 // row's ink spans 9..44, so the swatch sits
                                 // squarely against it

// The loading state: a wordmark, a rule, a five-cell bar and one word, all
// centred in the same 1000px block so the rule and the bar share an edge
// (plan section 4.6). There is no bar primitive anywhere in this app and this
// change does not add one - the cells are FillRects, exactly as every rule and
// swatch on every other screen is.
const int kLoadWordmarkY = 390;   // drawn at kStartupTitleScale
const int kLoadRuleY     = 480;
const int kLoadBarY      = 530;
const int kLoadBarH      = 28;
const int kLoadWordY     = 610;   // drawn at kStartupSubScale
const int kLoadBlockX    = 460;   // (1920 - kLoadBlockW) / 2
const int kLoadBlockW    = 1000;  // shared by the rule and the bar
const int kLoadCellGap   = 20;
const int kLoadCellW     = 184;   // (kLoadBlockW - 4 * kLoadCellGap) / 5
// Five EQUAL steps, one per stage of the startup sequence (spec 4.7), and the
// denominator stagesDone_ counts to. settings_ui_verify.py asserts the
// arithmetic above divides exactly: a bar whose cells were rounded would drift
// off the block's right edge.
const int kLoadStageCount = 5;

// The startup problem state: the wizard's progress-log geometry, unmoved -
// ui_scroll_verify.py's "Worlds startup" entry is these numbers, and the
// error state reusing them exactly is why that entry needs no edit (plan P7).
const int          kProblemTitleY    = 120;
const int          kProblemSentenceY = 200;
const ListLayout   kProgressLayout = { 300, 70, 920, 50 };

const char* const kFooterLine =
    "UP DOWN MOVE   LEFT RIGHT TABS   X SELECT   TRIANGLE DELETE   O EXIT";

// --- the strings this screen owns -----------------------------------------
//
// Named rather than inlined so settings_ui_verify.py can parse them out and
// measure every one against the column it is drawn into.

const char* const kRowNewWorld = "+ NEW WORLD";

const char* const kHelpNewWorldTitle = "+ NEW WORLD";
const char* const kHelpNewWorldBody =
    "Create a new world using your default settings.";

const char* const kHelpVanillaTitle = "VANILLA";
const char* const kHelpVanillaBody =
    "Play Bloodborne with the original game files and save data.";

const char* const kHelpWorldTitle = "WORLD";
const char* const kHelpWorldBody =
    "A saved randomizer setup and its save data.";

const char* const kNoteNewWorld =
    "STARTS FROM YOUR DEFAULT SETTINGS";

const char* const kNoteVanilla =
    "NO SETTINGS - CANNOT BE DELETED";

const char* const kNoteNoContainer =
    "RUN BLOODBORNE ONCE BEFORE ACTIVATING THIS WORLD";

const char* const kNoteContainerTooSmall =
    "THIS WORLD'S SAVE IS TOO LARGE FOR THIS CONSOLE";

// --- the delete confirmation (B17) ----------------------------------------
//
// Centred on the whole screen rather than drawn into the details pane, so they
// are measured against 1920 and not against kPaneW - which is why they are
// named with their own prefix and not kNote.
const char* const kDeleteTitle = "DELETE WORLD";
const char* const kDeleteLine1 =
    "REMOVES THIS WORLD AND ALL ITS REVISIONS";
const char* const kDeleteLine2 =
    "ITS SAVE DATA IS KEPT";
const char* const kDeleteFooter = "X DELETE   O CANCEL";
const char* const kDeleteRefusedFooter = "O BACK";
const char* const kDeleteKeptLine =
    "SAVE DATA KEPT IN SAVEBACKUPS";

const char* const kRefuseVanilla =
    "VANILLA CANNOT BE DELETED";
const char* const kRefuseActive =
    "ACTIVE WORLDS CANNOT BE DELETED";

// --- the startup screen's own words ---------------------------------------
//
// Centred on the whole screen, so all of these are measured against 1920
// rather than against a column.

const char* const kWordmark = "BLOODBORNE RANDOMIZER";

// The only word the loading state says. It never names the stage: which job
// is running is internals the player has no context for, and a phase name
// that flickers past is worse than a bar that does not (spec 10 D2).
const char* const kLoadingWord = "LOADING";

const char* const kProblemTitle = "STARTUP PROBLEM";

// One sentence per outcome, said by the SCREEN. The job's own wording and
// every return code stay in the log drawn beneath it (B9) - savedata::User's
// error, in particular, carries a raw hex code that has no business being the
// headline.
const char* const kProblemReconcileFailed =
    "AN INTERRUPTED ACTIVATION COULD NOT BE FINISHED";
const char* const kProblemJournalNotUnderstood =
    "AN INTERRUPTED ACTIVATION COULD NOT BE READ - IT WAS LEFT ALONE";
const char* const kProblemCaptureFailed =
    "YOUR SAVE COULD NOT BE COPIED INTO VANILLA";
const char* const kProblemNoSignedInPlayer =
    "NO PLAYER SIGNED IN";

const char* const kProblemScrollHint = "UP DOWN SCROLL";

// X for the three that leave a usable app behind; O for the one that does
// not. Worlds are scoped to the account, so with nobody signed in every
// action eventually refuses and offering to continue would be a dead end
// dressed up as a choice (spec section 10).
const char* const kPromptContinue = "X CONTINUE";
const char* const kPromptExit     = "O EXIT";

// The outcome table (plan P6). One row per non-None StartupProblem: what the
// screen says, and what it offers. A table rather than a switch so
// settings_ui_verify.py can parse the mapping out of the source and assert it
// is TOTAL - an outcome with no row here would be an error screen with a
// blank sentence and no way off it, which is the one thing an error screen
// cannot be.
struct ProblemRow {
    StartupProblem problem;
    const char*    sentence;
    const char*    prompt;
};

const ProblemRow kProblemTable[] = {
    { StartupProblem::ReconcileFailed,      kProblemReconcileFailed,      kPromptContinue },
    { StartupProblem::JournalNotUnderstood, kProblemJournalNotUnderstood, kPromptContinue },
    { StartupProblem::CaptureFailed,        kProblemCaptureFailed,        kPromptContinue },
    { StartupProblem::NoSignedInPlayer,     kProblemNoSignedInPlayer,     kPromptExit },
};

const int kProblemRowCount = (int)(sizeof(kProblemTable) / sizeof(kProblemTable[0]));

const char* ProblemSentence(StartupProblem problem) {
    for (int i = 0; i < kProblemRowCount; i++) {
        if (kProblemTable[i].problem == problem) return kProblemTable[i].sentence;
    }
    return "";
}

const char* ProblemPrompt(StartupProblem problem) {
    for (int i = 0; i < kProblemRowCount; i++) {
        if (kProblemTable[i].problem == problem) return kProblemTable[i].prompt;
    }
    return "";
}

// --- small formatters ------------------------------------------------------

std::string U64(uint64_t v) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%llu", (unsigned long long)v);
    return std::string(buf);
}

// Whole units, never a decimal point: the 8x8 fallback font has no '.' glyph
// and advances the cursor for a character it cannot draw, so on that path
// "27.4 MB" would read as "27 4 MB".
std::string SizeText(uint64_t bytes) {
    if (bytes >= 1024ull * 1024ull) return U64(bytes / (1024ull * 1024ull)) + " MB";
    if (bytes >= 1024ull)           return U64(bytes / 1024ull) + " KB";
    return U64(bytes) + " BYTES";
}

std::string SeedText(uint32_t seed) {
    std::string digits = U64(seed);
    if (digits.size() >= 10) return digits;
    return std::string(10 - digits.size(), '0') + digits;
}

// "rev-0003" -> 3. Anything else -> 0, which reads as "no revision".
int RevisionNumber(const std::string& id) {
    size_t dash = id.rfind('-');
    if (dash == std::string::npos) return 0;
    int n = 0;
    for (size_t i = dash + 1; i < id.size(); i++) {
        if (id[i] < '0' || id[i] > '9') return 0;
        n = n * 10 + (id[i] - '0');
    }
    return n;
}

const char* StateReadout(AfrActiveState state) {
    switch (state) {
        case AfrActiveState::FirstRun:  return "FIRST RUN";
        case AfrActiveState::Vanilla:   return "VANILLA";
        case AfrActiveState::World:     return "";      // the world's name is used
        case AfrActiveState::Unmanaged: return "UNMANAGED";
    }
    return "";
}

} // namespace

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

WorldsScreen::WorldsScreen(RandomizerDefaults& defaults, WorldsSession& session)
    : defaults_(defaults), session_(session) {
    user_ = savedata::ResolveUser();
    if (user_.valid) store_.reset(new WorldStore(user_.accountId));

    // The two startup jobs run once per launch, not once per visit. Coming
    // back from the DEFAULTS tab must not re-sweep, re-reconcile and re-ask
    // whether this is a first run - the answer has not changed and the log
    // would be in the player's way every time.
    if (!session_.startupDone && user_.valid) {
        mode_ = Mode::Loading;
        Say("STARTING UP");
    } else if (!user_.valid) {
        // B14: a list built with no account looks like it is working and then
        // refuses every action, because worlds are scoped to the account.
        // Said rather than logged, so the raw return code savedata::User
        // carries is on the log the error state draws as well as in live.log.
        Say(user_.error);
        EnterProblem(StartupProblem::NoSignedInPlayer);
    } else {
        mode_ = Mode::Browse;
        Refresh();
    }
}

WorldsScreen::~WorldsScreen() = default;

void WorldsScreen::Say(const std::string& line) {
    lines_.push_back(line);
    Log(("worlds: " + line).c_str());
}

void WorldsScreen::AddLines(const std::vector<std::string>& lines) {
    for (size_t i = 0; i < lines.size(); i++) Say(lines[i]);
}

void WorldsScreen::EnterProblem(StartupProblem problem) {
    problem_ = problem;
    mode_ = Mode::Problem;
    // The lines explaining the failure are the last thing in the log, so the
    // state opens on them however far the player had scrolled.
    logFollowTail_ = true;
}

// ---------------------------------------------------------------------------
// Reading the disk
// ---------------------------------------------------------------------------

// The one expensive read this screen does, and the reason WorldsSession
// exists. Done while the startup log is on screen, and never again.
void WorldsScreen::ProbeContainer() {
    if (session_.containerKnown || !user_.valid) return;

    savedata::SaveTitle title = savedata::DiscoverSaveTitle(user_);
    if (!title.found) {
        // Not an error here. The refusal that matters is activation's, which
        // does this properly in phase 1; all this costs is the two B25/B33
        // sentences, which are about a container nothing has found.
        Say("SAVE DATA - " + title.error);
        session_.containerKnown = true;
        return;
    }
    // By name, never by index (findings section 9). DiscoverSaveTitle only
    // reports found with at least one directory, and more than one is
    // activation's refusal to make, not this screen's.
    session_.saveTitleId = title.titleId;
    session_.saveDirName = title.dirNames.empty() ? std::string() : title.dirNames[0];

    savedata::Container container =
        savedata::ReadContainer(user_, session_.saveTitleId, session_.saveDirName);
    session_.containerExists = container.exists;
    session_.containerBlocks = container.blocks;
    session_.containerKnown  = true;

    Say("SAVE DATA " + session_.saveTitleId + " " + session_.saveDirName +
        (container.exists ? "  " + U64(container.blocks) + " BLOCKS"
                          : "  NO CONTAINER"));
}

std::string WorldsScreen::CannotActivate(const World& world) const {
    if (!world.hasSave) return std::string();            // nothing to restore
    if (!session_.containerKnown) return std::string();  // nothing known to judge on
    if (!session_.containerExists) return kNoteNoContainer;
    if (world.saveBlocks > session_.containerBlocks) return kNoteContainerTooSmall;
    return std::string();
}

void WorldsScreen::Refresh() {
    worlds_.clear();
    rowCache_.clear();
    hasVanilla_ = false;
    vanilla_ = World();
    active_ = AfrActiveWorld();

    if (!store_) return;

    hasVanilla_ = store_->Load(kVanillaWorldId, vanilla_);
    worlds_ = store_->List();

    // Section 4.2, derived fresh from the disk every time this screen is
    // built. The AFR title is the BLOODBORNE TITLE ID setting and nothing
    // else (B29, B37).
    AfrStatus   status = AfrManager::Check(defaults_.bloodborneTitleId);
    AfrManifest manifest;
    AfrManager::ReadManifest(defaults_.bloodborneTitleId, manifest);
    bool known = manifest.present && store_->Exists(manifest.worldId);
    active_ = AfrManager::DeriveActive(status, manifest, store_->AccountDirExists(), known);

    // One cache entry per rail row, in rail order.
    int count = RailCount();
    rowCache_.resize((size_t)count);
    for (int i = 0; i < count; i++) {
        RowCache& row = rowCache_[(size_t)i];
        row.seed     = "-";
        row.settings = "-";
        row.revision = "-";

        if (i == 0) {   // + NEW WORLD, described by the Defaults tab
            row.settings = U64((uint64_t)EnabledToggleCount(defaults_)) + " OF " +
                           U64((uint64_t)ToggleCount()) + " ON";
            continue;
        }
        const World* world = RowWorld(i);
        if (!world) continue;
        row.cannot = CannotActivate(*world);
        if (world->isVanilla) continue;   // no seed, no settings, no revisions

        WorldRevision current;
        if (store_->CurrentRevision(world->id, current)) {
            row.seed     = SeedText(current.recipe.seed);
            row.settings = U64((uint64_t)EnabledToggleCount(current.recipe.settings)) +
                           " OF " + U64((uint64_t)ToggleCount()) + " ON";
        }
        int number = RevisionNumber(world->currentRevision);
        if (number > 0) {
            row.revision = U64((uint64_t)number) + " OF " +
                           U64((uint64_t)world->revisionCount);
        }
    }

    if (railCursor_ >= count) railCursor_ = count > 0 ? count - 1 : 0;
}

// ---------------------------------------------------------------------------
// The rail, as indices
// ---------------------------------------------------------------------------

int WorldsScreen::RailCount() const {
    return 2 + (int)worlds_.size();   // + NEW WORLD, VANILLA, then the rest
}

std::string WorldsScreen::RowLabel(int index) const {
    if (index == 0) return kRowNewWorld;
    // Both fixed rows are present whatever is on disk (B2), so the Vanilla row
    // keeps its name even on a console whose worlds folder is not there yet.
    if (index == 1) return kVanillaWorldName;
    const World* world = RowWorld(index);
    if (!world) return std::string("?");
    if (world->name.empty()) return world->id;
    return world->name;
}

const World* WorldsScreen::RowWorld(int index) const {
    if (index == 1) return hasVanilla_ ? &vanilla_ : nullptr;
    int i = index - 2;
    if (i < 0 || i >= (int)worlds_.size()) return nullptr;
    return &worlds_[(size_t)i];
}

bool WorldsScreen::RowIsActive(int index) const {
    if (active_.state == AfrActiveState::Vanilla) return index == 1 && hasVanilla_;
    if (active_.state != AfrActiveState::World) return false;   // B4, B32
    const World* world = RowWorld(index);
    return world && world->id == active_.worldId;
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void WorldsScreen::Update(const ButtonEdges& input) {
    if (mode_ == Mode::Browse)             UpdateRail(input);
    else if (mode_ == Mode::ConfirmDelete) UpdateConfirmDelete(input);
    else if (mode_ == Mode::Problem)       UpdateProblem(input);
    else                                   UpdateStartup(input);
}

// The one place stagesDone_ is written. Clearing stageDrawn_ on every advance
// is the whole of the present-then-block rule: the step the bar now shows is
// on screen for a frame before the work it names runs (plan P2).
void WorldsScreen::SetStagesDone(int stages) {
    stagesDone_ = stages;
    stageDrawn_ = false;
}

// The five stages of plan 4.3, which are spec 4.7's five units of work:
//
//   0  find out            the reconcile job sweeping and reading the journal
//   1  act                 the reconcile job acting on what it found
//   2  capture             the first-run capture of the live save into Vanilla
//   3  read the container  ProbeContainer()
//   4  build the list      Refresh()
//
// One stage's work per frame at most, and none of it until the bar has shown
// that stage. Stages 1 and 2 are skipped on most launches; a skipped stage is
// still a completed one and still advances the bar (B4).
//
// NOTHING HERE CHANGES WHICH FILE OPERATIONS RUN OR IN WHAT ORDER. The sweep,
// the journal, the capture, the container read and the list build are the
// same calls in the same sequence they were when the log was on screen; what
// changed is which frame each one runs on.
void WorldsScreen::UpdateStartup(const ButtonEdges& input) {
    (void)input;   // B11: the loading state consumes no input, ever

    // Present, then block. The constructor enters stage 0 before any frame
    // exists, so this covers the first stage as well as the rest: frame 0
    // draws the bar, frame 1 does the sweep.
    if (!stageDrawn_) return;

    // --- stages 0 and 1: the reconcile ------------------------------------
    //
    // One job across two bar steps. The job runs four internal stages of its
    // own, and the split between finding out and acting is its to state
    // rather than this screen's to infer - hence the one read-only accessor
    // (P1). Neither Progress() nor the line count can tell them apart.
    if (stagesDone_ <= 1) {
        if (!reconcile_) {
            reconcile_.reset(new WorldReconcileJob(user_, defaults_.bloodborneTitleId));
        }
        reconcile_->Step();
        AddLines(reconcile_->TakeLines());

        if (!reconcile_->Done()) {
            if (stagesDone_ == 0 && reconcile_->ActingOnJournal()) SetStagesDone(1);
            return;
        }

        const ReconcileResult& result = reconcile_->Result();
        // ReconcileResult::error is populated ONLY on the resumed-activation
        // path; on the unknown-phase path it is empty, so the outcome is
        // classified from hadJournal and action and never from error.
        bool ok = result.ok;
        bool leftAlone = result.hadJournal && result.action == ReconcileAction::Nothing;
        // Asked HERE and nowhere else: the capture creates the very directory
        // this question is about, so after stage 2 the answer is a different
        // one. Held in firstRun_ for stage 2 to read.
        firstRun_ = store_ && !store_->AccountDirExists();
        reconcile_.reset();

        // A failed stage is still over, so the bar advances past it whatever
        // the outcome was (P3), and CONTINUE resumes at the stage after it.
        SetStagesDone(2);

        // B26: the live save is captured into Vanilla before anything else
        // happens, and "anything else" includes listing. Said now, whether or
        // not the reconcile failed - a reconcile failure is one screen and the
        // capture is the stage after it (P1), so neither swallows the other.
        if (firstRun_) {
            Say("");
            Say("FIRST RUN - CAPTURING THE LIVE SAVE INTO VANILLA");
        }

        if (!ok) {
            EnterProblem(leftAlone ? StartupProblem::JournalNotUnderstood
                                   : StartupProblem::ReconcileFailed);
            return;
        }
        // Everything else the reconcile can report - a swept partial, a
        // discarded staging tree, an activation correctly finished - is
        // silent and goes to the log only (B12).
        if (!firstRun_) SetStagesDone(3);
        return;
    }

    // --- stage 2: the first-run capture -----------------------------------
    if (stagesDone_ == 2) {
        // A launch with nothing to capture completes this stage the moment it
        // is entered (B4). This is also the guard that matters after a FAILED
        // reconcile: that path leaves the machine at stage 2 on every console,
        // and a capture job constructed on one that is not on its first run
        // would be a file operation this app has never performed (section
        // 3.1).
        if (!firstRun_) {
            SetStagesDone(3);
            return;
        }

        if (!capture_) capture_.reset(new FirstRunCaptureJob(user_));
        capture_->Step();
        if (!capture_->Done()) return;

        const FirstRunCaptureResult& result = capture_->Result();
        // `note` is set on the SUCCESS paths - no save data, no container, a
        // container holding no save files - so the outcome branches on ok
        // alone.
        bool ok = result.ok;
        if (!result.error.empty()) Say("FAILED - " + result.error);
        if (!result.note.empty())  Say(result.note);
        if (result.createdVanilla) Say("VANILLA CREATED");
        if (result.capturedSave) {
            Say("CAPTURED " + U64(result.files) + " FILE(S), " + SizeText(result.bytes));
            Say("BACKUP " + result.backupPath);
        }
        capture_.reset();
        SetStagesDone(3);

        if (!ok) EnterProblem(StartupProblem::CaptureFailed);
        // B7: a first run that worked does not hold. The outcome is visible
        // where a player would look for it - VANILLA is on the list, holding
        // a save.
        return;
    }

    // --- stage 3: the container read --------------------------------------
    //
    // The expensive one, and the reason the bar exists: it blocks a single
    // Update for as long as reading the whole container takes (spec 4.3). It
    // is unchanged and still here - only now the bar is already showing its
    // fourth step when it starts, rather than catching up afterwards.
    if (stagesDone_ == 3) {
        ProbeContainer();
        SetStagesDone(4);
        return;
    }

    // --- stage 4: the list -------------------------------------------------
    if (stagesDone_ == 4) {
        Refresh();
        SetStagesDone(5);
        session_.startupDone = true;
        mode_ = Mode::Browse;
    }
}

// The one state that holds. Up/Down scroll the log exactly as the startup log
// has always scrolled; the prompt is the outcome's own, and no state honours
// an input it does not draw a prompt for.
void WorldsScreen::UpdateProblem(const ButtonEdges& input) {
    if (input.up)   { logFollowTail_ = false; logScroll_--; }
    if (input.down) { logFollowTail_ = false; logScroll_++; }

    if (problem_ == StartupProblem::NoSignedInPlayer) {
        // There is nothing to continue to: every action is account-scoped and
        // would refuse. Signing in on the console is the real next step.
        if (input.circle) {
            Log("O pressed on the startup problem screen - exiting");
            wantsExit_ = true;
        }
        return;
    }

    if (input.cross) {
        problem_ = StartupProblem::None;
        mode_ = Mode::Loading;
        // A failed stage is still over, so stagesDone_ already points at the
        // one after it and the machine simply carries on - under the LOADING
        // screen, not this one (P4). The container read in particular has to
        // be covered by the bar rather than freezing the error screen the
        // player has just pressed X on.
        stageDrawn_ = false;
    }
}

void WorldsScreen::UpdateRail(const ButtonEdges& input) {
    int count = RailCount();
    if (count <= 0) return;

    railCursor_ = NavigateVertical(railCursor_, count, input);
    railScroll_ = ScrollToShow(railScroll_, railCursor_, count,
                               VisibleRowCount(kRailLayout));

    // Left/Right switch tabs while focus is on the rail (B31). This rail is
    // the whole screen - there is no settings pane here to take focus - so
    // the two inputs mean nothing else on it.
    if (input.left || input.right) {
        Log("worlds: switching to the DEFAULTS tab");
        requestedScreen_ = ScreenId::Defaults;
        return;
    }

    if (input.cross) {
        if (railCursor_ == 0) {
            Log("worlds: + NEW WORLD selected");
            requestedWorldId_.clear();          // empty means a new world (B6)
            requestedScreen_ = ScreenId::WorldEditor;
            return;
        }
        const World* world = RowWorld(railCursor_);
        if (!world) return;
        // B8: X on VANILLA activates Vanilla - the randomizer's files are
        // removed and the game runs unmodified. It routes to exactly the same
        // confirmation every other world does (milestone 6 step 3): the editor
        // opened on the id "vanilla", which has no settings to edit and
        // therefore opens straight on Confirm. One confirmation, one
        // transaction, one progress log - no second path that activates.
        Log(("worlds: " + world->id + " selected").c_str());
        requestedWorldId_ = world->id;          // B7
        requestedScreen_ = ScreenId::WorldEditor;
        return;
    }

    if (input.triangle) {
        AskDelete();
        return;
    }

    if (input.circle) {
        Log("O pressed on the worlds screen - exiting");
        wantsExit_ = true;
    }
}

// ---------------------------------------------------------------------------
// Deleting a world (B17, B20)
// ---------------------------------------------------------------------------

void WorldsScreen::AskDelete() {
    deleteWorldId_.clear();
    deleteWorldName_.clear();
    deleteRefusal_.clear();
    deleteResult_.clear();

    const World* world = RowWorld(railCursor_);
    if (railCursor_ == 0 || !world) {
        // + NEW WORLD is not a world and has nothing to destroy. Nothing is
        // said about it, because nothing was asked.
        return;
    }
    deleteWorldName_ = RowLabel(railCursor_);
    if (world->isVanilla) {
        deleteRefusal_ = kRefuseVanilla;
    } else if (RowIsActive(railCursor_)) {
        deleteRefusal_ = kRefuseActive;
    } else {
        deleteWorldId_ = world->id;
    }
    Log(("worlds: TRIANGLE on " + deleteWorldName_ +
         (deleteRefusal_.empty() ? " - confirming" : " - refused")).c_str());
    mode_ = Mode::ConfirmDelete;
}

void WorldsScreen::UpdateConfirmDelete(const ButtonEdges& input) {
    if (input.circle) {
        mode_ = Mode::Browse;
        // The rail is derived from the disk, so it has to be re-read once
        // something was actually removed: the row that was under the cursor is
        // gone. Refresh() clamps the cursor back into range itself.
        if (!deleteResult_.empty()) Refresh();
        return;
    }
    // O is the only way out of a refusal, and the only way out of the result.
    if (!deleteRefusal_.empty() || !deleteResult_.empty()) return;

    if (input.cross) {
        std::string error;
        std::string kept;
        if (store_ && store_->Delete(deleteWorldId_, error, &kept)) {
            deleteResult_ = "DELETED " + deleteWorldName_;
            deleteKeptPath_ = kept;
        } else {
            deleteResult_ = "COULD NOT DELETE - " + error;
        }
        Log(("worlds: " + deleteResult_).c_str());
    }
}

// ---------------------------------------------------------------------------
// The details pane's contents
// ---------------------------------------------------------------------------

std::vector<WorldsScreen::DetailRow> WorldsScreen::Details(int index) const {
    std::vector<DetailRow> rows;
    RowCache cache;
    if (index >= 0 && index < (int)rowCache_.size()) cache = rowCache_[(size_t)index];

    DetailRow row;
    const World* world = RowWorld(index);

    if (index == 0) {
        row.label = "SETTINGS"; row.value = cache.settings; rows.push_back(row);
    } else if (world && world->isVanilla) {
        row.label = "STATUS";
        row.value = RowIsActive(index) ? "ACTIVE" : "NOT ACTIVE";
        rows.push_back(row);

        row.label = "SAVE DATA";
        row.value = world->hasSave ? SizeText(world->saveBytes) + " IN " +
                                     U64(world->saveFiles) + " FILES"
                                   : std::string("NONE");
        rows.push_back(row);

        row.label = "LAST PLAYED";
        row.value = world->lastPlayed.empty() ? std::string("NEVER") : world->lastPlayed;
        rows.push_back(row);

        row.label = "CREATED"; row.value = world->created; rows.push_back(row);
    } else if (world) {
        row.label = "STATUS";
        row.value = RowIsActive(index) ? "ACTIVE" : "NOT ACTIVE";
        rows.push_back(row);

        row.label = "SEED";     row.value = cache.seed;     rows.push_back(row);
        row.label = "SETTINGS"; row.value = cache.settings; rows.push_back(row);
        row.label = "REVISION"; row.value = cache.revision; rows.push_back(row);

        row.label = "SAVE DATA";
        row.value = world->hasSave ? SizeText(world->saveBytes) + " IN " +
                                     U64(world->saveFiles) + " FILES"
                                   : std::string("NONE");
        rows.push_back(row);

        // B15: the save pairs with the WORLD, and the revision current when it
        // was last written is recorded and shown.
        row.label = "SAVED ON";
        {
            int number = RevisionNumber(world->lastPlayedRevision);
            row.value = number > 0 ? "REVISION " + U64((uint64_t)number)
                                   : std::string("-");
        }
        rows.push_back(row);

        row.label = "LAST PLAYED";
        row.value = world->lastPlayed.empty() ? std::string("NEVER") : world->lastPlayed;
        rows.push_back(row);
    }

    // The row's own explanation, where it has one.
    DetailRow note;
    note.note = true;
    if (index == 0)                     { note.value = kNoteNewWorld;  rows.push_back(note); }
    else if (world && world->isVanilla) { note.value = kNoteVanilla;   rows.push_back(note); }

    // Then exactly one of: why this world cannot be activated, or - when no
    // row is marked ACTIVE at all - why not. At most one, because the pane has
    // no cursor and a pane that overflows is one the player cannot scroll.
    if (!cache.cannot.empty()) {
        note.value = cache.cannot;
        rows.push_back(note);
    } else if (active_.state == AfrActiveState::Unmanaged ||
               active_.state == AfrActiveState::FirstRun) {
        note.value = active_.reason;
        rows.push_back(note);
    }

    return rows;
}

const char* WorldsScreen::HelpTitle(int index) const {
    if (index == 0) return kHelpNewWorldTitle;
    const World* world = RowWorld(index);
    if (world && world->isVanilla) return kHelpVanillaTitle;
    return kHelpWorldTitle;
}

const char* WorldsScreen::HelpBody(int index) const {
    if (index == 0) return kHelpNewWorldBody;
    const World* world = RowWorld(index);
    if (world && world->isVanilla) return kHelpVanillaBody;
    return kHelpWorldBody;
}

// ---------------------------------------------------------------------------
// Draw
// ---------------------------------------------------------------------------

void WorldsScreen::Draw(Renderer& renderer) {
    if (mode_ == Mode::Browse)             DrawBrowse(renderer);
    else if (mode_ == Mode::ConfirmDelete) DrawConfirmDelete(renderer);
    else if (mode_ == Mode::Problem)       DrawProblem(renderer);
    else                                   DrawLoading(renderer);
}

// Named, not a scrim over the rail: this is the one irreversible thing the
// WORLDS tab can do, and the screen behind it is not the context the question
// is about - the world's own name is.
void WorldsScreen::DrawConfirmDelete(Renderer& renderer) {
    renderer.Clear(20, 24, 28);

    DrawCenteredLabel(renderer, 200, kDeleteTitle, kStartupTitleScale,
                      Palette::Heading);
    DrawCenteredLabel(renderer, 320, deleteWorldName_.c_str(), kStartupSubScale,
                      Palette::Selected);

    if (!deleteRefusal_.empty()) {
        DrawCenteredLabel(renderer, 460, deleteRefusal_.c_str(), kRowScale,
                          Palette::Bad);
        DrawCenteredLabel(renderer, kScreenHeight - 80, kDeleteRefusedFooter,
                          kFooterScale, Palette::Dim);
        return;
    }

    // What it did, once it has done it. The world is off the rail by now, so
    // this is the only place that can say so.
    if (!deleteResult_.empty()) {
        DrawCenteredLabel(renderer, 460, deleteResult_.c_str(), kRowScale,
                          Palette::Good);
        if (!deleteKeptPath_.empty()) {
            DrawCenteredLabel(renderer, 520, kDeleteKeptLine, kRowScale,
                              Palette::Dim);
        }
        DrawCenteredLabel(renderer, kScreenHeight - 80, kDeleteRefusedFooter,
                          kFooterScale, Palette::Dim);
        return;
    }

    DrawCenteredLabel(renderer, 460, kDeleteLine1, kRowScale, Palette::Text);
    DrawCenteredLabel(renderer, 520, kDeleteLine2, kRowScale, Palette::Good);
    DrawCenteredLabel(renderer, kScreenHeight - 80, kDeleteFooter, kFooterScale,
                      Palette::Dim);
}

// The loading state: four elements and nothing else, at any point in startup
// (B2). No log, no phase name, no prompt and no input - a player who cannot
// act on what a line says is only being made to watch it.
void WorldsScreen::DrawLoading(Renderer& renderer) {
    renderer.Clear(20, 24, 28);

    DrawCenteredLabel(renderer, kLoadWordmarkY, kWordmark, kStartupTitleScale,
                      Palette::Heading);

    renderer.FillRect(kLoadBlockX, kLoadRuleY, kLoadBlockW, kRuleThickness,
                      kRuleColor.r, kRuleColor.g, kRuleColor.b);

    // Five discrete cells, not one filled bar: the cell being worked on is
    // drawn in Selected so that "the bar shows the step whose work is running"
    // is something the player can SEE rather than something the code merely
    // claims (B5, P5). Completed cells are Good; the rest are the rule's own
    // colour, so an untouched bar reads as furniture and not as an error.
    for (int i = 0; i < kLoadStageCount; i++) {
        Color color = kRuleColor;
        if (i < stagesDone_)       color = Palette::Good;
        else if (i == stagesDone_) color = Palette::Selected;
        renderer.FillRect(kLoadBlockX + i * (kLoadCellW + kLoadCellGap), kLoadBarY,
                          kLoadCellW, kLoadBarH, color.r, color.g, color.b);
    }

    DrawCenteredLabel(renderer, kLoadWordY, kLoadingWord, kStartupSubScale,
                      Palette::Text);

    // LAST, and deliberately a write from the draw path: the step now on the
    // bar has been presented, so UpdateStartup may do its work on the next
    // frame (section 4.3). Without this the bar would always be describing
    // work that had already finished. DrawStartup wrote logScroll_ from here
    // in exactly the same way.
    stageDrawn_ = true;
}

// The one state that draws the log, and the only one that says anything about
// what went wrong. One sentence, the log beneath it, and a prompt - the
// sentence is this screen's, and the job's own wording is in the log.
void WorldsScreen::DrawProblem(Renderer& renderer) {
    renderer.Clear(20, 24, 28);

    DrawCenteredLabel(renderer, kProblemTitleY, kProblemTitle,
                      kStartupTitleScale, Palette::Bad);
    DrawCenteredLabel(renderer, kProblemSentenceY, ProblemSentence(problem_),
                      kStartupSubScale, Palette::Text);

    // No job is running here, so there is no live status line and the band
    // draws its full complement of rows.
    int logRows   = VisibleRowCount(kProgressLayout);
    if (logRows < 1) logRows = 1;
    int lineCount = (int)lines_.size();

    int offset = logFollowTail_ ? lineCount - logRows : logScroll_;
    offset = ClampScroll(offset, lineCount, logRows);
    logScroll_ = offset;

    for (int r = 0; r < logRows; r++) {
        int index = offset + r;
        if (index >= lineCount) break;
        DrawCenteredLabel(renderer, kProgressLayout.firstY + r * kProgressLayout.spacing,
                          lines_[(size_t)index].c_str(), kProgressScale, Palette::Text);
    }

    DrawScrollHints(renderer, kProgressLayout, lineCount, offset, logRows);

    DrawCenteredLabel(renderer, kScreenHeight - 130, kProblemScrollHint, kFooterScale,
                      Palette::Dim);
    DrawCenteredLabel(renderer, kScreenHeight - 80, ProblemPrompt(problem_), kFooterScale,
                      Palette::Dim);
}

void WorldsScreen::DrawBrowse(Renderer& renderer) {
    renderer.Clear(20, 24, 28);

    DrawTabs(renderer, kTabWorlds);
    DrawLabelLeft(renderer, kTabX, kTabHeadingY, TabLabel(kTabWorlds), kTabHeadingScale,
                  Palette::Heading);

    // Which world the game is running right now, right-aligned in the header
    // band - the position the wizard puts its TARGET readout in. This is how
    // UNMANAGED and FIRST RUN are stated (B32): in both of them no row is
    // marked ACTIVE, so without this the screen would just be silent about it.
    {
        std::string what;
        if (active_.state == AfrActiveState::World) {
            what = active_.worldName.empty() ? active_.worldId : active_.worldName;
        } else {
            what = StateReadout(active_.state);
        }
        std::string readout = std::string("ACTIVE  ") + (what.empty() ? "NONE" : what);
        DrawLabelRight(renderer, kStateRight, kStateY, readout.c_str(), kRowScale,
                       Palette::Dim);
    }

    renderer.FillRect(kRailX, kHeaderRuleY, 1800, kRuleThickness,
                      kRuleColor.r, kRuleColor.g, kRuleColor.b);
    renderer.FillRect(kPaneX - 20, kColumnRuleY, kRuleThickness, kColumnRuleH,
                      kRuleColor.r, kRuleColor.g, kRuleColor.b);
    renderer.FillRect(kHelpX - 20, kColumnRuleY, kRuleThickness, kColumnRuleH,
                      kRuleColor.r, kRuleColor.g, kRuleColor.b);

    DrawRail(renderer);
    DrawDetails(renderer);
    DrawHelp(renderer);

    DrawCenteredLabel(renderer, kFooterY, kFooterLine, kFooterScale, Palette::Dim);
}

void WorldsScreen::DrawRail(Renderer& renderer) {
    int count   = RailCount();
    int visible = VisibleRowCount(kRailLayout);
    int offset  = ClampScroll(railScroll_, count, visible);

    for (int r = 0; r < visible; r++) {
        int index = offset + r;
        if (index >= count) break;
        int  y       = kRailLayout.firstY + r * kRailLayout.spacing;
        bool focused = (index == railCursor_);

        if (focused) {
            renderer.FillRect(kRailX, y + kBarOffsetY, kRailW, kBarHeight,
                              Palette::SelectedBar.r, Palette::SelectedBar.g,
                              Palette::SelectedBar.b);
        }
        Color color = focused ? Palette::Selected : Palette::Text;
        DrawLabelLeft(renderer, kRailX, y, RowLabel(index).c_str(), kRowScale, color);

        // Exactly one row carries this, and none does until first-run capture
        // has run (B4).
        if (RowIsActive(index)) {
            renderer.FillRect(kRailX + kRailW - kSwatchW, y + kSwatchOffsetY,
                              kSwatchW, kSwatchH,
                              Palette::Good.r, Palette::Good.g, Palette::Good.b);
        }
    }

    DrawPaneScrollHints(renderer, kRailLayout, kRailX, kRailW, count, offset, visible);
}

void WorldsScreen::DrawDetails(Renderer& renderer) {
    DrawLabelLeft(renderer, kPaneX, kPaneHeadingY, RowLabel(railCursor_).c_str(),
                  kHeadingScale, Palette::Heading);

    std::vector<DetailRow> rows = Details(railCursor_);

    // A note is a wrapped sentence, so the list drawn here is longer than the
    // list built above. Wrapped at draw time for the same reason the help pane
    // is: it is a few hundred advance lookups against the several hundred
    // glyph blits the same frame already costs, and a cache here would be the
    // only thing in the draw path needing invalidation.
    std::vector<DetailRow> drawnRows;
    for (size_t i = 0; i < rows.size(); i++) {
        if (!rows[i].note) { drawnRows.push_back(rows[i]); continue; }
        std::vector<std::string> wrapped =
            WrapText(renderer, rows[i].value.c_str(), kRowScale, kPaneW);
        for (size_t line = 0; line < wrapped.size(); line++) {
            DetailRow out;
            out.note  = true;
            out.value = wrapped[line];
            drawnRows.push_back(out);
        }
    }

    int visible = VisibleRowCount(kDetailLayout);
    for (int r = 0; r < visible && r < (int)drawnRows.size(); r++) {
        int y = kDetailLayout.firstY + r * kDetailLayout.spacing;
        const DetailRow& out = drawnRows[(size_t)r];
        if (out.note) {
            DrawLabelLeft(renderer, kPaneX, y, out.value.c_str(), kRowScale, Palette::Dim);
        } else {
            DrawLabelLeft(renderer, kPaneX, y, out.label.c_str(), kRowScale, Palette::Text);
            DrawLabelRight(renderer, kPaneValueRight, y, out.value.c_str(), kRowScale,
                           Palette::Text);
        }
    }
}

void WorldsScreen::DrawHelp(Renderer& renderer) {
    std::vector<std::string> titleLines =
        WrapText(renderer, HelpTitle(railCursor_), kRowScale, kHelpW);
    for (int i = 0; i < (int)titleLines.size() && i < kHelpTitleMaxLines; i++) {
        DrawLabelLeft(renderer, kHelpX, kHelpTitleY + i * kHelpPitch,
                      titleLines[(size_t)i].c_str(), kRowScale, Palette::Selected);
    }

    renderer.FillRect(kHelpX, kHelpRuleY, kHelpW, kRuleThickness,
                      kRuleColor.r, kRuleColor.g, kRuleColor.b);

    std::vector<std::string> bodyLines =
        WrapText(renderer, HelpBody(railCursor_), kRowScale, kHelpW);
    for (int i = 0; i < (int)bodyLines.size() && i < kHelpBodyMaxLines; i++) {
        DrawLabelLeft(renderer, kHelpX, kHelpBodyY + i * kHelpPitch,
                      bodyLines[(size_t)i].c_str(), kRowScale, Palette::Text);
    }
}

} // namespace bbr
