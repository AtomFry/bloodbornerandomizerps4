// WorldsScreen.h - the WORLDS tab of the main screen: the list of this
// account's worlds, what each one is, and which one the game is currently
// running (worlds B1-B4, B20, B22, B32).
//
// THE RAIL IS ALWAYS "+ NEW WORLD", "VANILLA", THEN THE PLAYER'S WORLDS, most
// recently played first (B2). The first two rows are there whether or not any
// world exists, so the screen is never empty and the two things a new player
// can do are always the first two things on it.
//
// WHICH WORLD IS ACTIVE IS DERIVED, NEVER STORED. AfrManager::DeriveActive
// reads it back off the disk every time from the marker inside dvdroot_ps4 and
// the account's worlds folder (worlds plan section 4.2). A console carrying
// randomizer files this app did not write is UNMANAGED, and no row is marked
// ACTIVE - saying "unmodified" there would be a lie and naming a world a guess
// (B32).
//
// FOUR MODES, one Screen. Loading is every launch: the startup sweep and any
// interrupted activation (plan section 4.3), and the capture of a pre-existing
// live save into Vanilla (B26), all run under it. WHICH JOB IS RUNNING IS NOT
// A MODE - nothing on screen names it. Problem is the one state that holds,
// the one that prompts and the one that draws the log, and it is entered only
// by the four StartupProblem outcomes below. Browse is the tabbed
// three-column screen. They are modes rather than ScreenIds for the same
// reason every other multi-step screen in this app uses modes: Application
// rebuilds screens on every switch, so a real drill-in would throw this
// screen's state away on the way back.
//
// ConfirmDelete is the fourth. TRIANGLE on a world asks before destroying
// anything, names the world, and says its save is kept as a safety backup
// (B17) - and it REFUSES outright on Vanilla, which is never deletable (B20),
// and on the active world, which the game is running right now. The refusal
// uses the same mode rather than a silent no-op: a button that does nothing is
// indistinguishable from a button that is broken.
#pragma once

#include "Screen.h"

#include "../Game/AfrManager.h"
#include "../Game/WorldActivation.h"
#include "../Platform/SaveData.h"
#include "../Randomizer/RandomizerDefaults.h"
#include "../Randomizer/WorldStore.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace bbr {

// How startup ended, explicitly. Two booleans used to encode this between
// them, and they could not tell a failed save restore from a swept temporary
// directory - both looked identical to the player (startup-screen spec
// section 1).
//
// These four are the COMPLETE set of failures the startup sequence can detect
// and report (spec section 4.1), and the set is fixed at four: a fifth needs
// a spec change, not a fifth enumerator. Every non-None outcome has a row in
// WorldsScreen.cpp's kProblemTable giving the sentence the screen says and
// the prompt it offers, and settings_ui_verify.py asserts that mapping is
// total.
enum class StartupProblem {
    None,                    // startup got through; nothing is shown

    ReconcileFailed,         // an interrupted activation could not be finished
    JournalNotUnderstood,    // a journal at a phase no version of this app
                             // writes, so nothing was done to it
    CaptureFailed,           // the first run could not file the live save
                             // into Vanilla
    NoSignedInPlayer,        // no foreground user, or no account for them
};

// What survives a tab switch.
//
// Application rebuilds a screen every time one is switched to, so anything
// this screen learns the expensive way has to be held somewhere that outlives
// it - and Application is where app-wide state lives (Application.h).
//
// The expensive thing is the save container. savedata::ReadContainer sizes
// every file by reading it through to a short read, because st_size lies on
// this kernel (findings section 9); at the measured ~15 MB/s that is close to
// two seconds on a full Bloodborne save. Once per launch, while the startup
// progress log is already on screen, is fine. Once per tab switch is a screen
// that appears to hang every time the player presses Left.
struct WorldsSession {
    bool startupDone = false;   // the reconcile and first-run jobs have run

    bool        containerKnown  = false;  // the probe below has been done
    bool        containerExists = false;
    uint64_t    containerBlocks = 0;
    std::string saveTitleId;              // discovered, never the AFR title
    std::string saveDirName;
};

class WorldsScreen : public Screen {
public:
    // `defaults` is Application's canonical copy: the source a new world is
    // pre-filled from, and where the BLOODBORNE TITLE ID setting - the AFR
    // title, which is never the save-data title (B29) - comes from.
    WorldsScreen(RandomizerDefaults& defaults, WorldsSession& session);
    ~WorldsScreen();

    void Update(const ButtonEdges& input) override;
    void Draw(Renderer& renderer) override;
    bool WantsExit() const override { return wantsExit_; }
    ScreenId    RequestedScreen() const override { return requestedScreen_; }
    std::string RequestedWorldId() const override { return requestedWorldId_; }

private:
    enum class Mode { Loading, Problem, Browse, ConfirmDelete };

    // One line of the details pane. A `note` row is a wrapped sentence drawn
    // across the whole pane; every other row is a label on the left and its
    // value right-aligned, exactly like a settings row.
    struct DetailRow {
        std::string label;
        std::string value;
        bool        note = false;
    };

    // Per-rail-row values that cost a file read to work out - a world's
    // current revision, and the container check behind B25/B33. Built once by
    // Refresh() and index-aligned with the rail, because Details() runs in the
    // draw path and a draw path that opens files is a frame rate that depends
    // on how many worlds the player has.
    struct RowCache {
        std::string seed;        // zero-padded, or "-"
        std::string settings;    // "12 OF 15 ON", or "-"
        std::string revision;    // "3 OF 5", or "-"
        std::string cannot;      // the B25/B33 sentence, or empty
    };

    void UpdateStartup(const ButtonEdges& input);
    void UpdateProblem(const ButtonEdges& input);
    void UpdateRail(const ButtonEdges& input);
    void UpdateConfirmDelete(const ButtonEdges& input);

    // Enters ConfirmDelete for the rail row under the cursor, with
    // deleteRefusal_ set when that row cannot be deleted at all.
    void AskDelete();

    void DrawLoading(Renderer& renderer);
    void DrawProblem(Renderer& renderer);
    void DrawBrowse(Renderer& renderer);
    void DrawConfirmDelete(Renderer& renderer);
    void DrawRail(Renderer& renderer);
    void DrawDetails(Renderer& renderer);
    void DrawHelp(Renderer& renderer);

    // Enters the one state that holds: sets problem_, the mode and the log's
    // follow-tail. The lines explaining the failure are already in lines_.
    void EnterProblem(StartupProblem problem);

    // The one place stagesDone_ is written. Every advance clears stageDrawn_,
    // which is the whole of the present-then-block rule: the new step is on
    // screen before the work it names runs (plan section 4.3, P2).
    void SetStagesDone(int stages);

    void ProbeContainer();
    void Refresh();
    void AddLines(const std::vector<std::string>& lines);
    void Say(const std::string& line);

    // The rail, as indices: 0 is + NEW WORLD, 1 is VANILLA, and 2.. are
    // worlds_ in order. Nothing outside these four addresses a row by number.
    int          RailCount() const;
    std::string  RowLabel(int index) const;
    const World* RowWorld(int index) const;   // nullptr for + NEW WORLD
    bool         RowIsActive(int index) const;

    // Empty when the world can be activated. Non-empty is the sentence the
    // details pane states instead (worlds plan section 4.5, B25, B33).
    std::string CannotActivate(const World& world) const;

    std::vector<DetailRow> Details(int index) const;
    const char* HelpTitle(int index) const;
    const char* HelpBody(int index) const;

    RandomizerDefaults& defaults_;
    WorldsSession&      session_;

    savedata::User              user_;
    std::unique_ptr<WorldStore> store_;

    // The rail's contents, re-read from disk by Refresh(). Vanilla is held
    // separately because it is a fixed row rather than one of the listed
    // worlds (B2).
    World              vanilla_;
    bool               hasVanilla_ = false;
    std::vector<World>    worlds_;
    std::vector<RowCache> rowCache_;   // one per rail row, in rail order

    // Section 4.2's derivation, and the manifest it was derived from.
    AfrActiveWorld active_;

    Mode mode_ = Mode::Browse;

    // The two startup jobs. Non-null only while their own mode is running.
    std::unique_ptr<WorldReconcileJob>  reconcile_;
    std::unique_ptr<FirstRunCaptureJob> capture_;

    // The startup log, drawn exactly as the wizard draws its progress log.
    std::vector<std::string> lines_;
    int  logScroll_ = 0;
    bool logFollowTail_ = true;

    // How startup ended. None everywhere except in Mode::Problem, which is
    // the only state that reads it.
    StartupProblem problem_ = StartupProblem::None;

    // The five-stage machine behind the loading bar (plan section 4.3): 0 find
    // out, 1 act, 2 capture, 3 read the container, 4 build the list. The
    // RUNNING stage is stagesDone_ while it is below kLoadStageCount; 5 means
    // startup is over. It never retreats - a stage that FAILED is still over,
    // so continuing from the error state resumes at the one after it (P3).
    int  stagesDone_ = 0;

    // Whether the running stage has had a frame of its own. UpdateStartup does
    // nothing until it has, so the bar can never sit a step behind the work it
    // is describing. The constructor enters stage 0 before any frame exists,
    // which is why this starts false.
    bool stageDrawn_ = false;

    // Whether this launch has a live save to capture into Vanilla - decided
    // ONCE, when the reconcile finishes, because the capture itself creates
    // the account directory the question is asked of. Stage 2 reads it rather
    // than re-asking the disk, which also keeps a CONTINUE from a failed
    // reconcile from constructing a capture job on a console that is not on
    // its first run.
    bool firstRun_ = false;

    int railCursor_ = 0;
    int railScroll_ = 0;

    // The ConfirmDelete mode's subject. `deleteRefusal_` empty means the
    // deletion is offered; non-empty means the screen states that sentence and
    // O is the only way out.
    std::string deleteWorldId_;
    std::string deleteWorldName_;
    std::string deleteRefusal_;
    std::string deleteResult_;    // what Delete() did, stated before returning
    std::string deleteKeptPath_;  // where its save went, when one was kept

    bool        wantsExit_ = false;
    ScreenId    requestedScreen_ = ScreenId::None;
    std::string requestedWorldId_;
};

} // namespace bbr
