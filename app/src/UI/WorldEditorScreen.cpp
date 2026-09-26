// cstdlib before anything pulling <random>/<cmath>, or the toolchain's libc++
// fails on ::abs - same ordering constraint as BossRandomizer.cpp.
#include <cstdlib>

#include "WorldEditorScreen.h"

#include "Controls.h"

#include "../Platform/Log.h"
#include "../Randomizer/EnemyRandomizer.h"
#include "../Randomizer/RandomizerDefaultsStore.h"

#include <cstring>
#include <ctime>
#include <random>
#include <string>
#include <vector>

namespace bbr {

namespace {
// THE ROW CONSTANTS ARE GONE. A block of kSomethingRow indices used to live
// here, and each one had to equal the POSITION of its entry in BOTH the
// DrawSaveData and DrawConfirm `items` vectors - three parallel lists, where a
// row inserted anywhere but the end silently mislabelled everything below it.
// Settings now come from SettingsModel.h, whose declaration order IS the
// display order, and Confirm is generated from that same table rather than
// carrying a second hand-written copy of it.

// The four strings feature 032 puts on the commit screen, named rather than
// inlined so pool_verify.py selftest case 6 can parse them out of this file
// and assert what they have to satisfy.
//
// The atlas is the live text path and covers printable ASCII (32..126), so
// these strings have no character-set problem there. The 42-character limit
// below - A-Z, 0-9, space and ' ( ) - , with no lowercase and no ':' - is
// Font8x8.cpp's, and now binds only the FALLBACK path, which is what
// pool_verify.py's renderable() still checks. It matters because
// DrawText8x8 advances the cursor for a character it cannot render, so on
// that path an unrenderable one is a full-width BLANK column rather than a
// missing one: a line can be well inside every width budget and still show
// the player nothing. Hence the prefix's ' - ', already this screen's
// separator elsewhere.
//
// Width is NOT predictable from character counts - the atlas is proportional,
// so measure with Renderer::TextWidth(). pool_verify.py's 71-character budget
// is likewise the fallback's fixed-advance arithmetic (9px x scale 3 into
// 1920), kept because it is a conservative bound: the longest of these four
// is 54 characters and 1002px at scale 3, well inside the screen either way.
// The prefix takes 29 of those characters, leaving 42 for whatever
// EnemyRandomizer's Fail() put in result.error - which is why those two
// messages are named constants over there as well.
const char* const kEnemyFailPrefix = "ENEMY RANDOMIZATION FAILED - ";
const char* const kPoolFellBackLine1 = "ALL SELECTED ENEMIES WERE ALSO SKIPPED";
const char* const kPoolFellBackLine2 = "SKIPPED ENEMIES WERE USED AS REPLACEMENTS FOR THIS RUN";
const char* const kNothingRandomizedLine = "NO ENEMIES WERE RANDOMIZED - EVERY ENEMY WAS SKIPPED";

// --- text scales -----------------------------------------------------------
const int kTitleScale    = 5;
// No kHeaderScale: the header's only remaining item, the target readout, is
// drawn at kRowScale. The seed that used the larger scale now lives in the pane.
const int kHeadingScale  = 4;
const int kItemScale     = 4;   // Confirm's list, which is still a flat review list
const int kRowScale      = 3;   // the categorised screen's rail, pane and help
const int kFooterScale   = 3;
const int kProgressScale = 3;

// How many category rows the rail draws, repeated here only so the two rail
// constants below can be arithmetic rather than two more magic numbers. The
// authority is WorldEditorScreen::kCategoryCount, which this is asserted
// against at the bottom of this block.
const int kCategoryRows = 7;

// --- the categorised screen's geometry -------------------------------------
//
// The SAME numbers SetupDefaultsScreen.cpp carries, deliberately duplicated:
// plan §4.4 calls this "one geometry, shared by the wizard's Settings step and
// SetupDefaultsScreen", but §5 assigns no shared layout file to either
// milestone, and every other screen in this app carries its own constants.
// settings_ui_verify.py parses BOTH files and fails if any of these disagrees
// with its twin, so the two cannot drift silently.
//
// 60 + 580 + 20 + 2 + 18 + 700 + 20 + 2 + 18 + 440 + 60 = 1920. The rail is
// 580 wide because it has to hold BLOODBORNE TITLE ID   CUSA03173 at scale 3
// on the other screen (552px), which is what sized the whole split. Every
// number below is asserted by tools/settings_ui_verify.py against the atlas's
// own advances - none of them is a character count.
const int kRailX          = 60;
const int kRailW          = 580;
const int kPaneX          = 680;
const int kPaneW          = 700;
const int kPaneValueRight = 1380; // values are right-aligned to this edge
const int kHelpX          = 1420;
const int kHelpW          = 440;

// --- vertical furniture ----------------------------------------------------
const int kTitleY       = 36;
const int kHeaderY      = 118;
const int kHeaderRuleY  = 196;
const int kColumnRuleY  = 210;
const int kColumnRuleH  = 750;
// The rail is TEN rows deep here - NAME, SEED, seven categories, HISTORY -
// where Setup Defaults' is seven, so it no longer shares that screen's 330/76
// category band and cannot: ten rows at 76px would put the last one's ink at
// 958, two pixels inside the column rule and under its own focus bar. The
// pitch is 70 instead, which lands the whole rail on one uniform grid from
// kRailRow0Y with both rules sitting in a gap, and ends the column's ink at
// 900. settings_ui_verify.py pins these four against this file alone;
// everything that decides where the three COLUMNS are is still shared and
// still compared.
const int kRailRow0Y    = 230;   // NAME
const int kRailPitch    = 70;
const int kRailRow1Y    = 300;   // SEED,           kRailRow0Y + kRailPitch
const int kRailRuleY    = 358;
const int kRailFirstY   = 370;   // first category, kRailRow1Y + kRailPitch
const int kPaneHeadingY = 220;
const int kHelpTitleY   = 220;
const int kHelpPitch    = 52;
const int kHelpRuleY    = 330;
const int kHelpBodyY    = 356;
const int kHelpTitleMaxLines = 2;
const int kHelpBodyMaxLines  = 11;
const int kFooterY      = 1000;

const int kRuleThickness = 2;

// HISTORY sits below the category block, separated from it by its own rule -
// the mirror of what the NAME and SEED rows above it get. Setup Defaults has
// neither, because it has no history and no commit path. Both rules sit 14px
// under the ink above them and 19px over the ink below, which is the same gap
// at both ends of the block.
const int kRailRule2Y = 848;
const int kHistoryY   = 860;     // kRailFirstY + kCategoryRows * kRailPitch

// The header's only remaining readout, right-aligned to this edge. Decorative
// and never a focus target (plan 9 D1). The seed used to sit beside it and now
// lives in the pane; the target stays because the wizard writes into
// /data/GoldHEN/AFR/<titleId>/, and which title that is belongs on screen
// throughout rather than only on Confirm.
const int kHeaderTargetRight = 1860;

// The focus bar sits behind the focused row. At pitch 76 with scale-3 text,
// (rowY - 10, height 64) contains the row's whole ink box and still leaves
// 12px to the next row's bar, so two adjacent bars never touch.
const int kBarOffsetY = -10;
const int kBarHeight  = 64;

// Rules and separators are 2px FillRects, not SDL_RenderDrawLine - one fewer
// SDL entry point to prove on hardware (plan P4). Dimmer than Palette::Dim,
// which is text: a rule that reads as loud as a label is furniture competing
// with content.
const Color kRuleColor = { 64, 72, 82 };

// OPTIONS is on this screen now. It writes the world - creating it on the
// first press - and opens Confirm, which is where the activation is
// committed from. It is the same button Setup Defaults saves with, and it
// works from either region, because saving is not an action of whichever
// row happens to be highlighted.
//
// The footer says SAVE AND ACTIVATE because since milestone 6 OPTIONS does
// both: it writes the world (appending a revision only if the recipe
// changed) and then opens the activation confirmation. It said only SAVE
// while Confirm still committed a plain randomizer run.
const char* const kFooterLine =
    "UP DOWN MOVE   LEFT RIGHT CHANGE   X SELECT   O BACK   OPTIONS SAVE AND ACTIVATE";

// The screen's own title. Named rather than inlined because three modes draw
// it and settings_ui_verify.py measures it.
const char* const kScreenTitle = "WORLD EDITOR";

// Confirm's list is still a flat review list, so it keeps the band the flat
// settings list used: from under the sub-heading down to just above where the
// "MORE BELOW" hint has to clear the footer.
//
// Milestone 6 moved it down and tightened its pitch: the B10 statement needs
// two wrapped rows above the list, and at 420/90 the list's MORE ABOVE hint
// sat inside the second of them. 470/80 keeps the same six visible rows with
// the band cleared - ui_scroll_verify.py asserts both ends.
const ListLayout kSettingsLayout = { 470, 80, 870, 60 };

// --- Confirm, which is now the ACTIVATION confirmation (B10) ---------------
//
// The B10 statement is eight label/value rows at the head of that same list -
// the three that were already there plus the five activation adds - rather
// than a block of its own above it. Two reasons: the band between the state
// line and the list is two scale-3 rows deep and the statement is five rows,
// and a statement that scrolls with the recipe it is about is one list to
// read instead of two.
const char* const kConfirmGap = "   ";   // what every row puts between the two

// What OPTIONS on the Settings step did to the world on disk is still stated
// here, on the state line, and so is the reason an activation is refused. The
// refusal's own SENTENCE is too long for scale 4, so it is wrapped underneath
// at the row scale instead - which is also where the plain-words version of
// the phase-6 row goes when there is no refusal.
const int kConfirmStateY         = 250;
const int kConfirmSentenceY      = 306;
const int kConfirmSentencePitch  = 46;
const int kConfirmSentenceW      = 1800;
const int kConfirmSentenceMax    = 2;

const char* const kConfirmHeading    = "CONFIRM ACTIVATION";
const char* const kConfirmChecking   = "CHECKING";
const char* const kCannotActivate    = "CANNOT ACTIVATE";
const char* const kNoOutgoingWorld   = "NOTHING - NO WORLD IS ACTIVE";

// The five activation rows' labels, named so settings_ui_verify.py measures
// the widest row rather than guessing which one it is.
const char* const kRowDeactivating  = "DEACTIVATING";
const char* const kRowOutgoingSave  = "OUTGOING SAVE";
const char* const kRowActivating    = "ACTIVATING";
const char* const kRowIncomingSave  = "INCOMING SAVE";
const char* const kRowHowLong       = "HOW LONG";

// Where the outgoing world's save goes (B10). Four cases, and the two that say
// nothing happens are as important as the two that say something does: a
// player who is told their save was filed away when it was not has been
// misled about the one thing this screen exists to promise.
const char* const kOutgoingNoContainer = "NO CONTAINER - NOTHING TO BACK UP";
const char* const kOutgoingEmpty       = "LEFT ALONE - CONTAINER IS EMPTY";
const char* const kOutgoingNowhere     = "BACKED UP - FILED NOWHERE";
const char* const kOutgoingFiled       = "BACKED UP AND FILED INTO IT";

// Roughly how long it takes (B10), and deliberately no more precise than this.
// The arithmetic: a ~27 MB save at the measured ~15 MB/s is ~2 s a pass, so a
// safety backup and its capture are ~8 s together and a restore is ~4 s; a
// full generation is 10-20 s. The worst case is therefore around half a
// minute, and a number would be false precision on a console whose disk speed
// nobody has measured twice.
const char* const kDurationUnknown = "A MOMENT";
const char* const kDurationShort   = "A FEW SECONDS";
const char* const kDurationMedium  = "UNDER A MINUTE";
const char* const kDurationLong    = "ABOUT A MINUTE";

// The two lines Confirm ends on, and the two the activation can end on.
const char* const kConfirmFooterHint = "UP DOWN SCROLL";
const char* const kConfirmFooterGo   = "OPTIONS ACTIVATE   O BACK";
const char* const kConfirmFooterBack = "O BACK";

// The progress log is denser and starts higher. One slot is given up to the
// live status line while the run is going, so the log itself shows one fewer
// line then than it does once the run has finished.
const ListLayout kProgressLayout = { 300, 70, 920, 50 };

// HISTORY is a full-screen list with a cursor, unlike Confirm, so it runs at
// the row scale rather than the item scale: a revision line carries three
// columns of text and has to stay one line however many revisions there are.
const ListLayout kHistoryLayout = { 340, 60, 820, 50 };

// What a brand new world is called before the player names it. Not blank: a
// world with no name lists as its own id ("W-0001") on the worlds rail, which
// reads as a bug rather than as an invitation. It is a perfectly ordinary
// name and duplicates are permitted (B21), so two unnamed worlds are two
// worlds called WORLD and not an error.
const char* const kNewWorldName = "WORLD";

// The characters NormalizeWorldName lets through (worlds plan P10), in the
// order up/down cycles them. Space is first so that holding up from a blank
// name walks the alphabet rather than the digits.
const char* const kNameAlphabet = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
const int kNameAlphabetLen = 37;

// The class's kSeedDigits, repeated for the file-local formatter above because
// a private static member is not reachable from this namespace. The two are
// asserted equal in the constructor.
const int kSeedDigitsForText = 10;

// Zero-padded to kSeedDigits, so the rail row, the digit editor and the
// history list all show the same ten characters for the same number.
std::string SeedText(uint32_t seed) {
    std::string digits = std::to_string(seed);
    if (digits.size() >= (size_t)kSeedDigitsForText) return digits;
    return std::string((size_t)kSeedDigitsForText - digits.size(), '0') + digits;
}

char CycleNameChar(char c, int dir) {
    int index = 0;
    for (int i = 0; i < kNameAlphabetLen; i++) {
        if (kNameAlphabet[i] == c) { index = i; break; }
    }
    index = (index + dir + kNameAlphabetLen) % kNameAlphabetLen;
    return kNameAlphabet[index];
}
} // namespace

WorldEditorScreen::WorldEditorScreen(RandomizerDefaults& defaults,
                                     const std::string& worldId)
    : isVanilla_(worldId == kVanillaWorldId),
      defaults_(defaults),
      run_(defaults),
      worldId_(worldId),
      isNewWorld_(worldId.empty()),
      seed_(defaults.lastSeed) {
    // The rail's own geometry is arithmetic on a category count written twice -
    // once as this class's list and once in the anonymous namespace above.
    // Here is where the two are made to agree.
    static_assert(kCategoryRows == kCategoryCount,
                  "the rail geometry and the category list disagree");
    static_assert(kSeedDigitsForText == kSeedDigits,
                  "the seed formatter and the seed editor disagree");

    // 0 means defaults.cfg has never carried a seed, so roll one immediately -
    // the row should never come up blank or showing a meaningless zero.
    if (seed_ == 0) RollNewSeed();

    // Worlds are account-scoped, structurally (WorldStore.h). Without a
    // signed-in user there is no store, and the editor can still be read and
    // navigated but cannot write anything - which is what storeError_ says on
    // Confirm.
    user_ = savedata::ResolveUser();
    if (user_.valid) {
        store_.reset(new WorldStore(user_.accountId));
    } else {
        storeError_ = user_.error;
        Log(("world editor: " + user_.error).c_str());
    }

    if (isNewWorld_) {
        // B6: a new world is pre-filled from the DEFAULTS tab, which run_
        // already is. SAVE DATA is the one field that is NOT a default in that
        // sense - it is always KEEP EXISTING (B12), whatever defaults.cfg
        // happens to carry - so it is forced here rather than inherited.
        run_.startFreshSave = false;
        name_ = kNewWorldName;
        Log("world editor: new world, pre-filled from the DEFAULTS tab");
    } else {
        LoadWorld();
    }

    // B8/B20: Vanilla is a world with no editable settings and no revisions,
    // so there is no Settings step for it to open on. X on the worlds rail's
    // VANILLA row routes here and lands directly on the same confirmation
    // every other world gets - milestone 6 step 3.
    if (isVanilla_) {
        Log("world editor: VANILLA - straight to the activation confirmation");
        OpenConfirm();
    }
}

WorldEditorScreen::~WorldEditorScreen() = default;

// B7: an existing world opens on its CURRENT revision - the recipe that would
// be regenerated if it were activated right now - not on the Defaults tab and
// not on the revision its save was written under.
void WorldEditorScreen::LoadWorld() {
    if (!store_) return;

    World world;
    if (!store_->Load(worldId_, world)) {
        storeError_ = "WORLD " + worldId_ + " COULD NOT BE READ";
        Log(("world editor: " + storeError_).c_str());
        return;
    }
    name_       = world.name;
    storedName_ = world.name;

    // Vanilla has no revisions and needs none: activating it REMOVES the
    // randomizer's files rather than generating any (B8, B20). Asking for a
    // current revision here would report a missing one as an error.
    if (isVanilla_) {
        Log("world editor: opened VANILLA - no revisions, nothing to edit");
        return;
    }

    WorldRevision current;
    if (store_->CurrentRevision(worldId_, current)) {
        // The AFR title is a DEFAULTS-tab setting and is deliberately not
        // stored in a revision (spec worlds D25), so it survives this
        // assignment from the copy run_ was built with.
        std::string titleId = run_.bloodborneTitleId;
        run_ = current.recipe.settings;
        run_.bloodborneTitleId = titleId;
        seed_ = current.recipe.seed;
        Log(("world editor: opened " + worldId_ + " on " + current.id).c_str());
    } else {
        storeError_ = "WORLD " + worldId_ + " HAS NO REVISION";
        Log(("world editor: " + storeError_).c_str());
    }
}

// No cursor or scroll reset here any more. It used to zero both
// unconditionally, which is exactly what "returning restores the position the
// player left" forbids (spec section 2): every step now owns its own cursor
// state and nothing clears it on the way past.
void WorldEditorScreen::GoToStep(Step step) {
    step_ = step;
}

std::string WorldEditorScreen::SeedDisplayText() const { return SeedText(seed_); }

// OPTIONS's one destination. The world is written first, so the confirmation
// describes what is on disk rather than what is in this screen's memory; then
// the plan is marked as wanted, and UpdateConfirm builds it on the first frame
// AFTER this one - phase 1 reads the whole save container, and a screen that
// has not been drawn yet cannot say it is working.
void WorldEditorScreen::OpenConfirm() {
    if (!isVanilla_) SaveWorld();
    confirmScroll_ = 0;   // the only place Confirm's scroll is zeroed
    plan_        = ActivationPlan();
    planReady_   = false;
    planPending_ = true;
    GoToStep(Step::Confirm);
}

// Returns a value without storing it anywhere. Split out from RollNewSeed so
// the seed editor can roll into its working buffer without committing: SQUARE
// there has to stay undoable with O, and a roll that assigned seed_ directly
// would have made it permanent the moment it was pressed.
//
// NOT time(nullptr) directly: two presses inside the same second would hand
// back the same value and look broken. One generator seeded from the clock,
// drawn from repeatedly, gives a different value every press.
uint32_t NextSeedValue() {
    static std::mt19937 roller((uint32_t)time(nullptr));
    return (uint32_t)roller();
}

void WorldEditorScreen::RollNewSeed() {
    seed_ = NextSeedValue();
    Log(("world editor: rolled new seed " + std::to_string(seed_)).c_str());
}

const SettingDef& WorldEditorScreen::SelectedSetting() const {
    return SettingInCategory(Category(), listCursor_[lastCategory_]);
}

std::string WorldEditorScreen::NameDisplayText() const {
    return name_.empty() ? std::string("NOT SET") : name_;
}

void WorldEditorScreen::Update(const ButtonEdges& input) {
    switch (step_) {
        case Step::Settings:      UpdateSettings(input); break;
        case Step::EditName:      UpdateEditName(input); break;
        case Step::EditSeed:      UpdateEditSeed(input); break;
        case Step::Picker:        UpdatePicker(input); break;
        case Step::History:       UpdateHistory(input); break;
        case Step::Confirm:       UpdateConfirm(input); break;
        case Step::Progress:      UpdateProgress(input); break;
    }
}

void WorldEditorScreen::UpdateSettings(const ButtonEdges& input) {
    // OPTIONS writes the world from either region, exactly as Setup Defaults
    // saves from either region: it is not an action of whichever row is
    // highlighted, and there is no FINISH row to put it on any more.
    if (input.options) {
        OpenConfirm();
        return;
    }

    if (focus_ == Focus::Rail) UpdateRail(input);
    else                       UpdatePane(input);
}

void WorldEditorScreen::UpdateRail(const ButtonEdges& input) {
    railCursor_ = NavigateVertical(railCursor_, kRailItemCount, input);

    // Landing on a category is what changes which settings the pane shows;
    // landing on NAME, SEED or HISTORY deliberately does not, so the pane
    // never blanks (plan P13).
    if (railCursor_ >= kFirstCatRow && railCursor_ < kHistoryRow) {
        lastCategory_ = railCursor_ - kFirstCatRow;
    }

    // Left/Right do nothing anywhere on the rail. They used to roll a new seed
    // on the seed row, which made an irreversible change to the run from a
    // direction press, with no confirmation and no undo. The seed is now only
    // ever changed inside its own editor, reached with X. This screen is not
    // one of the two tabs either, so there is nothing for them to switch to.

    if (input.cross) {
        if (railCursor_ == kNameRow) {
            OpenNameEditor();
        } else if (railCursor_ == kSeedRow) {
            OpenSeedEditor();
        } else if (railCursor_ == kHistoryRow) {
            LoadHistory();
            GoToStep(Step::History);
        } else {
            focus_ = Focus::List;
        }
    }

    if (input.circle) {
        // Back to the rail this editor was opened from. Anything not written
        // by an OPTIONS press is discarded, which is the same bargain Setup
        // Defaults makes and is said out loud for the same reason.
        Log("world editor: closed - anything unsaved is discarded");
        requestedScreen_ = ScreenId::Worlds;
    }
}

void WorldEditorScreen::UpdatePane(const ButtonEdges& input) {
    SettingCategory category = Category();
    int count = CategorySize(category);
    if (count <= 0) {          // an empty category cannot happen today and is
        focus_ = Focus::Rail;  // a model bug if it ever does - do not trap the
        return;                // player in a pane with nothing in it
    }

    int& cursor = listCursor_[lastCategory_];
    cursor = NavigateVertical(cursor, count, input);
    listScroll_[lastCategory_] = ScrollToShow(listScroll_[lastCategory_], cursor, count,
                                              VisibleRowCount(kPaneLayout));

    const SettingDef& def = SettingInCategory(category, cursor);

    // Left/Right are the ONLY way a setting changes, and AdjustSetting is the
    // only thing that writes one.
    if (input.left || input.right) {
        AdjustSetting(def, run_, input.right ? 1 : -1);
        Log((std::string("world editor: ") + def.label + " = " +
             SettingValueText(def, run_)).c_str());
    }

    // X advances: into the picker on a drill-in, and nowhere at all on a
    // toggle. It must not toggle - see the header.
    if (input.cross && IsDrillIn(def)) {
        picker_.Reset();
        pickerSetting_ = &def;
        Log((std::string("world editor: opening picker for ") + def.label).c_str());
        GoToStep(Step::Picker);
    }

    if (input.circle) focus_ = Focus::Rail;
}

void WorldEditorScreen::OpenNameEditor() {
    // Space-padded to the full width so every one of the sixteen slots is a
    // character the cursor can sit on. NormalizeWorldName trims the trailing
    // spaces back off on the way out, so "ABC" edited and accepted unchanged
    // is still "ABC".
    for (int i = 0; i < kNameLen; i++) {
        nameBuf_[i] = i < (int)name_.size() ? name_[(size_t)i] : ' ';
    }
    nameBuf_[kNameLen] = '\0';
    nameCursor_ = 0;
    Log("world editor: opening name editor");
    GoToStep(Step::EditName);
}

void WorldEditorScreen::UpdateEditName(const ButtonEdges& input) {
    // The same control scheme as the seed and title-ID editors: left/right
    // walks the cursor, up/down cycles the character under it. One scheme for
    // all three, so none of them has to be learned separately.
    if (input.left)  nameCursor_ = (nameCursor_ + kNameLen - 1) % kNameLen;
    if (input.right) nameCursor_ = (nameCursor_ + 1) % kNameLen;

    if (input.up || input.down) {
        int dir = input.up ? 1 : -1;
        nameBuf_[nameCursor_] = CycleNameChar(nameBuf_[nameCursor_], dir);
    }

    if (input.cross) {
        // Normalised here as well as in the store: the rail, the details pane
        // and this screen's own heading all show name_ before anything is
        // written, and they must show what WOULD be written rather than what
        // was typed.
        name_ = NormalizeWorldName(std::string(nameBuf_, kNameLen));
        Log(("world editor: name set to " + NameDisplayText()).c_str());
        GoToStep(Step::Settings);
    }

    if (input.circle) {
        Log("world editor: name edit cancelled - unchanged");
        GoToStep(Step::Settings);
    }
}

void WorldEditorScreen::OpenSeedEditor() {
    std::string padded = SeedDisplayText();
    memcpy(seedBuf_, padded.c_str(), kSeedDigits);
    seedBuf_[kSeedDigits] = '\0';
    seedCursor_ = 0;
    Log("world editor: opening seed editor");
    GoToStep(Step::EditSeed);
}

void WorldEditorScreen::UpdateEditSeed(const ButtonEdges& input) {
    // Same control scheme as the title-ID editor: left/right walks the cursor,
    // up/down cycles the digit under it.
    if (input.left)  seedCursor_ = (seedCursor_ + kSeedDigits - 1) % kSeedDigits;
    if (input.right) seedCursor_ = (seedCursor_ + 1) % kSeedDigits;

    if (input.up || input.down) {
        int dir = input.up ? 1 : -1;
        int digit = ((seedBuf_[seedCursor_] - '0') + dir + 10) % 10;
        seedBuf_[seedCursor_] = (char)('0' + digit);
    }

    // SQUARE rolls a random seed into the editor rather than committing one.
    // It lands in the buffer like a typed digit would, so X still confirms it
    // and O still discards it - seed_ is untouched until X. Rolling is never
    // itself the irreversible step.
    if (input.square) {
        uint32_t rolled = NextSeedValue();
        std::string digits = std::to_string(rolled);
        std::string padded = digits.size() >= (size_t)kSeedDigits
                                 ? digits
                                 : std::string((size_t)kSeedDigits - digits.size(), '0') + digits;
        memcpy(seedBuf_, padded.c_str(), kSeedDigits);
        seedBuf_[kSeedDigits] = '\0';
        Log(("world editor: rolled " + std::to_string(rolled) + " into the seed editor").c_str());
    }

    if (input.cross) {
        // Ten digits can express more than a uint32 holds, so clamp rather
        // than wrap - a silently wrapped seed would be a seed you can't retype.
        unsigned long long value = strtoull(seedBuf_, nullptr, 10);
        if (value > 0xFFFFFFFFull) {
            value = 0xFFFFFFFFull;
            Log("world editor: seed clamped to 4294967295");
        }
        seed_ = (uint32_t)value;
        Log(("world editor: seed set to " + std::to_string(seed_)).c_str());
        GoToStep(Step::Settings);
    }

    if (input.circle) {
        Log("world editor: seed edit cancelled - unchanged");
        GoToStep(Step::Settings);
    }
}

// Straight back to the row it was opened from: nothing cleared the pane's
// cursor while the picker was up, so nothing has to restore it. This is what
// ReturnFromPicker used to exist for, and why it no longer does.
void WorldEditorScreen::UpdatePicker(const ButtonEdges& input) {
    if (!pickerSetting_) {          // cannot happen - Step::Picker is only
        GoToStep(Step::Settings);   // entered with the setting set
        return;
    }
    const SettingDef& def = *pickerSetting_;
    if (picker_.Update(input, SelectionStrings(def), SelectionTable(def),
                       SelectionCount(def), SelectionFlags(def, run_))) {
        pickerSetting_ = nullptr;
        GoToStep(Step::Settings);
    }
}

// ---------------------------------------------------------------------------
// HISTORY (spec worlds D2, D14, B14, B16)
// ---------------------------------------------------------------------------

void WorldEditorScreen::LoadHistory() {
    revisions_.clear();
    historyCursor_ = 0;
    historyScroll_ = 0;
    if (!store_ || worldId_.empty()) return;   // a world not yet written has none
    revisions_ = store_->Revisions(worldId_);  // newest first (D14)
    Log(("world editor: HISTORY - " + std::to_string(revisions_.size()) +
         " revisions").c_str());
}

namespace {
// "rev-0003" -> 3, for display. Anything else -> 0, which reads as "no
// revision" - the same parse the worlds screen makes of the same ids.
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

// How many settings differ between two recipes. Every setting, through the
// model, so a field added to RandomizerDefaults is counted without anyone
// remembering to extend this - the same reason SameRecipe is defined as "they
// serialize identically" rather than as a hand-written operator==.
//
// The pool kinds are compared flag by flag rather than by their "N OF M" text,
// because two selections of the same SIZE are still two different pools.
int CountChangedSettings(const RandomizerDefaults& a, const RandomizerDefaults& b) {
    RandomizerDefaults left  = a;   // SelectionFlags needs a writable struct;
    RandomizerDefaults right = b;   // neither copy is modified
    int changed = 0;
    for (int i = 0; i < SettingCount(); i++) {
        const SettingDef& def = SettingAt(i);
        if (def.flag) {
            if ((left.*(def.flag)) != (right.*(def.flag))) changed++;
            continue;
        }
        const bool* lf = SelectionFlags(def, left);
        const bool* rf = SelectionFlags(def, right);
        int count = SelectionCount(def);
        if (!lf || !rf) continue;
        for (int j = 0; j < count; j++) {
            if (lf[j] != rf[j]) { changed++; break; }
        }
    }
    return changed;
}
} // namespace

// Newest first, each line carrying the revision's number, its seed, and how
// many settings it changed from the revision before it. The OLDEST revision
// changed nothing from anything - it is where the world started - so it says
// so rather than reporting a count against a baseline that does not exist.
std::vector<std::string> WorldEditorScreen::HistoryItems() const {
    std::vector<std::string> items;
    for (size_t i = 0; i < revisions_.size(); i++) {
        const WorldRevision& rev = revisions_[i];
        std::string line = "REVISION " + std::to_string(RevisionNumber(rev.id)) +
                           "   " + SeedText(rev.recipe.seed);
        if (i + 1 < revisions_.size()) {
            int changed = CountChangedSettings(rev.recipe.settings,
                                               revisions_[i + 1].recipe.settings);
            line += "   " + std::to_string(changed) + " CHANGED";
        } else {
            line += "   CREATED";
        }
        items.push_back(line);
    }
    return items;
}

void WorldEditorScreen::UpdateHistory(const ButtonEdges& input) {
    int count = (int)revisions_.size();
    if (count > 0) {
        historyCursor_ = NavigateVertical(historyCursor_, count, input);
        historyScroll_ = ScrollToShow(historyScroll_, historyCursor_, count,
                                      VisibleRowCount(kHistoryLayout));
    }

    // D2: making an earlier revision current again is itself an APPEND, so
    // nothing is ever lost and the list grows rather than rewinds. It happens
    // here rather than on the next OPTIONS press because "selecting one makes
    // it current" is what the row does - and because an append is the one edit
    // on this screen that cannot lose anything if the player backs out of it:
    // the revision they came from is still in the list, one row down.
    // store_ is guarded here rather than at the top of the function: circle is
    // handled at the bottom, so an early return would swallow it and leave the
    // player stuck in HISTORY. count > 0 already implies a store - LoadHistory()
    // returns early without one - but that invariant is two hops away, and this
    // is the line that would crash if it ever stopped holding.
    if (input.cross && count > 0 && store_) {
        WorldRecipe chosen = revisions_[(size_t)historyCursor_].recipe;
        std::string chosenId = revisions_[(size_t)historyCursor_].id;

        std::string titleId = run_.bloodborneTitleId;   // D25: not a per-world value
        run_ = chosen.settings;
        run_.bloodborneTitleId = titleId;
        seed_ = chosen.seed;

        std::string revisionId;
        std::string error;
        bool appended = false;
        if (store_->AppendRevision(worldId_, chosen, revisionId, appended, error)) {
            saveNote_ = appended ? chosenId + " IS NOW " + revisionId
                                 : chosenId + " IS ALREADY CURRENT";
        } else {
            storeError_ = error;
            saveNote_ = "COULD NOT MAKE " + chosenId + " CURRENT";
        }
        Log(("world editor: " + saveNote_).c_str());
        LoadHistory();   // the list just grew, and this screen is what shows it
        GoToStep(Step::Settings);
        return;
    }

    if (input.circle) {
        Log("world editor: leaving HISTORY");
        GoToStep(Step::Settings);
    }
}

// ---------------------------------------------------------------------------
// Writing the world (worlds plan section 7, milestone 5 step 5)
// ---------------------------------------------------------------------------

WorldRecipe WorldEditorScreen::CurrentRecipe() const {
    WorldRecipe recipe;
    recipe.seed = seed_;
    recipe.settings = run_;
    return recipe;
}

// Creates the world on the first press and updates it on every one after.
// A RENAME APPENDS NOTHING and a settings change appends one revision (P4) -
// both of those are WorldStore's rules, not this screen's; what is here is the
// order they are applied in and the sentence the player is shown afterwards.
void WorldEditorScreen::SaveWorld() {
    if (!store_) {
        if (storeError_.empty()) storeError_ = "NO SIGNED IN USER";
        Log(("world editor: cannot save - " + storeError_).c_str());
        return;
    }

    std::string error;
    if (worldId_.empty()) {
        World created;
        if (!store_->Create(name_, CurrentRecipe(), created, error)) {
            storeError_ = error;
            Log(("world editor: create failed - " + error).c_str());
            return;
        }
        worldId_    = created.id;
        name_       = created.name;     // as NormalizeWorldName left it
        storedName_ = created.name;
        isNewWorld_ = false;
        storeError_.clear();
        saveNote_ = "CREATED " + created.id;
        Log(("world editor: created " + created.id).c_str());
        return;
    }

    // The rename first, so the sentence below can report the revision outcome
    // without the two getting tangled: they are independent, and a rename
    // genuinely is not a recipe change.
    bool renamed = false;
    if (NormalizeWorldName(name_) != storedName_) {
        if (!store_->Rename(worldId_, name_, error)) {
            storeError_ = error;
            Log(("world editor: rename failed - " + error).c_str());
            return;
        }
        name_       = NormalizeWorldName(name_);
        storedName_ = name_;
        renamed     = true;
    }

    std::string revisionId;
    bool appended = false;
    if (!store_->AppendRevision(worldId_, CurrentRecipe(), revisionId, appended,
                                error)) {
        storeError_ = error;
        Log(("world editor: could not write a revision - " + error).c_str());
        return;
    }
    storeError_.clear();

    // Said as what happened rather than as "saved": the one thing a player has
    // to be able to tell apart is a rename, which records no history, from a
    // settings change, which does (B14).
    if (renamed && appended)       saveNote_ = "RENAMED AND APPENDED " + revisionId;
    else if (appended)             saveNote_ = "APPENDED " + revisionId;
    else if (renamed)              saveNote_ = "RENAMED - NO NEW REVISION";
    else                           saveNote_ = "NO CHANGES - STILL " + revisionId;
    Log(("world editor: " + worldId_ + " - " + saveNote_).c_str());
}

void WorldEditorScreen::UpdateConfirm(const ButtonEdges& input) {
    // Phase 1, on the first frame after Confirm was opened and never in the
    // draw path. PlanActivation reads the whole save container and walks the
    // incoming world's stored save (WorldActivation.h) - a second or two of
    // blocking work in a frame loop that cannot draw while it runs - so the
    // screen draws CHECKING first and does the work on the frame after. It
    // writes nothing, whatever it decides.
    if (planPending_) {
        planPending_ = false;
        plan_      = PlanActivation(user_, run_.bloodborneTitleId, worldId_);
        planReady_ = true;
        if (plan_.refusal.Refused()) {
            Log(("world editor: activation refused - " +
                 std::string(RefusalReasonName(plan_.refusal.reason)) + " - " +
                 plan_.refusal.sentence).c_str());
        } else {
            Log(("world editor: activation planned - " +
                 std::string(SaveActionName(plan_.saveAction))).c_str());
        }
        return;
    }

    // No cursor here - this is a review list - so up/down move the window
    // itself, which is the only way to read the settings that don't fit.
    int visible = VisibleRowCount(kSettingsLayout);
    int count   = (int)ConfirmItems().size();
    if (input.up)   confirmScroll_--;
    if (input.down) confirmScroll_++;
    confirmScroll_ = ClampScroll(confirmScroll_, count, visible);

    // OPTIONS activates, and only when there is something to activate: a
    // refusal has already said nothing will be written, and a world the store
    // could not write is a world whose stored recipe is not the one on this
    // screen - activating it would generate something the player never saw.
    if (input.options && planReady_ && plan_.ok && storeError_.empty()) {
        Log("world editor: activation confirmed");
        // Switch steps BEFORE any work happens: the activation is stepped from
        // UpdateProgress across many frames, so the progress log has to
        // already be the thing being drawn.
        StartCommit();
        GoToStep(Step::Progress);
        return;
    }

    if (input.circle) {
        if (isVanilla_) {
            // Vanilla never had a Settings step to go back to (B20).
            Log("world editor: VANILLA activation cancelled");
            requestedScreen_ = ScreenId::Worlds;
            return;
        }
        // Back to the settings screen exactly as it was left - the rail cursor
        // and lastCategory_ are where the player left them, because nothing
        // cleared either (spec section 2).
        Log("world editor: activation cancelled - back to settings");
        GoToStep(Step::Settings);
    }
}

void WorldEditorScreen::UpdateProgress(const ButtonEdges& input) {
    // One coarse unit of activation work per frame. The frame loop is serial
    // (Update -> Draw -> Present), so doing the whole transaction in one call
    // would leave nothing on screen until it finished - stepping it here is
    // what makes the progress display possible at all.
    //
    // The job says what each phase did; this screen only relays it. Its lines
    // are pushed straight into the log rather than through AddProgressLine,
    // because the job has already written every one of them to live.log under
    // its own prefix and logging them twice makes the one record that survives
    // a hardware test harder to read.
    if (job_) {
        job_->Step();
        std::vector<std::string> lines = job_->TakeLines();
        for (size_t i = 0; i < lines.size(); i++) progressLines_.push_back(lines[i]);
        if (job_->Done()) FinishCommit();
        return; // input is ignored while the activation is still running
    }

    // The run is over, so the log is static and can be read back: up/down
    // scroll it, which also unpins it from the tail.
    if (input.up) {
        progressFollowTail_ = false;
        progressScroll_--;
    }
    if (input.down) {
        progressFollowTail_ = false;
        progressScroll_++;
    }

    // O is the only other thing that does anything here - this is a result
    // screen, not a step with its own choices.
    if (input.circle) {
        Log("world editor: returning to the WORLDS tab");
        requestedScreen_ = ScreenId::Worlds;
    }
}

// The vanilla source path and the AFR output path are BOTH gone from this
// file. Game/WorldActivation owns the first and Game/AfrManager the second,
// which is what plan section 3.1 asks for: /data/GoldHEN/AFR appears in one
// place, and this screen no longer knows where a tree comes from or goes.

void WorldEditorScreen::AddProgressLine(const std::string& line) {
    progressLines_.push_back(line);
    Log(("world editor: " + line).c_str());
}

void WorldEditorScreen::StartCommit() {
    progressLines_.clear();
    completionLineStart_ = (std::size_t)-1;
    commitFinished_ = false;
    progressScroll_ = 0;
    progressFollowTail_ = true;
    job_.reset();

    // Vanilla has no seed and no recipe: activating it deletes a tree (B8,
    // B20), so nothing below this point applies to it.
    if (isVanilla_) {
        AddProgressLine("ACTIVATING VANILLA - THE RANDOMIZER FILES ARE REMOVED");
        job_.reset(new WorldActivationJob(user_, run_.bloodborneTitleId, worldId_));
        return;
    }

    // The seed is always chosen on the settings screen - rolled or typed - so
    // this reports it rather than generating one. It is the seed the world's
    // current revision now carries, because OPTIONS wrote that revision before
    // this screen was reached.
    uint32_t seed = seed_;
    AddProgressLine("USING SEED " + std::to_string(seed));

    // Remembered so the next NEW world opens showing it. Only lastSeed is
    // written back; this world's own settings live in run_ and in its
    // revision, and are deliberately never assigned over defaults_.
    defaults_.lastSeed = seed;
    SaveRandomizerDefaults(defaults_);

    // The two empty-pool guards used to be here, refusing the commit before it
    // could half-write a tree. They are now phase 1 refusals in
    // Game/WorldActivation - RefusalReason::EmptySelection - which is checked
    // before ANY phase writes anything, including the safety backup, and which
    // the confirmation screen has therefore already shown the player. They are
    // the same two conditions with the same two sentences, moved to where
    // every other refusal lives.

    // The SKIPPING lines, unchanged, for the recipe about to be generated:
    // plan section 3.1 keeps them, and this is still the screen that shows
    // them. The || chain they belong to now lives beside the generation it
    // decides, in Game/WorldActivation's phase 4, where worlds_verify.py pins
    // it field for field.
    if (!run_.randomizeEnemies) AddProgressLine("SKIPPING ENEMY RANDOMIZATION");
    if (!run_.randomizeBosses)  AddProgressLine("SKIPPING BOSS RANDOMIZATION");
    if (!run_.randomizeTreasure) AddProgressLine("SKIPPING TREASURE RANDOMIZATION");
    if (!run_.randomizeEnemyDrops) AddProgressLine("SKIPPING ENEMY DROP RANDOMIZATION");
    if (!run_.randomizeStartingWeapons) AddProgressLine("SKIPPING STARTING WEAPON RANDOMIZATION");
    if (!run_.randomizeStartingGuns) AddProgressLine("SKIPPING STARTING GUN RANDOMIZATION");
    if (!run_.randomizeShopWeapons) AddProgressLine("SKIPPING SHOP WEAPON RANDOMIZATION");

    // THE OPTIONS MAPPING AND THE RUN DECISION ARE GONE FROM THIS FILE, and
    // that is milestone 6's whole point about parity: they now exist once, in
    // Game/WorldActivation's phase 4, built out of the world's CURRENT
    // REVISION rather than out of this screen's working copy. The tree an
    // activation produces is therefore the tree the recipe on disk describes,
    // whatever this screen happens to be showing, and worlds_verify.py pins
    // that one copy field for field against the mapping and the || chain the
    // Enable wizard used to carry here (B28).
    //
    // The output path went with them. This screen no longer names
    // /data/GoldHEN/AFR or the vanilla source: activation stages into
    // AfrManager::StagingDvdroot and swaps the tree in by rename, which is the
    // half of the transaction that makes it interruptible at all.
    job_.reset(new WorldActivationJob(user_, run_.bloodborneTitleId, worldId_));
}

void WorldEditorScreen::FinishCommit() {
    bool failed  = false;
    bool refused = false;

    if (job_) {
        const ActivationResult& activation = job_->Result();
        failed  = !activation.ok;
        refused = activation.refusal.Refused();

        // The job has already said what every phase did, refusal and failure
        // included - those lines arrived through TakeLines. What is added here
        // is the GENERATION's own report, which no other caller has ever shown
        // and which is the only place several settings say anything at all.
        //
        // It is reported against run_, this screen's copy of the recipe, which
        // is what OPTIONS wrote into the world's current revision immediately
        // before the activation read it back - and OPTIONS is refused when the
        // store could not write, so the two cannot come apart.
        const EnemyRandomizerResult& result = activation.generation;
        if (activation.generated) {
            if (run_.randomizeEnemies) {
                // D5 again: extend this line rather than add a SKIPPING one.
                // No parentheses - the 8x8 font has no punctuation glyphs.
                std::string line = "RANDOMIZED " + std::to_string(result.enemiesRandomized) +
                                   " ENEMIES ACROSS " + std::to_string(result.mapsProcessed) +
                                   " MAPS";
                AddProgressLine(line);

                // Feature 032 D4. Everything the run was allowed to draw was
                // also something it was told to leave alone, so the pool half
                // of that instruction yielded. Said in two short lines rather
                // than one long one, matching the shape StartCommit already
                // uses for NO ENEMIES SELECTED. Without it the run silently
                // contradicts the setting.
                if (result.poolFellBack) {
                    AddProgressLine(kPoolFellBackLine1);
                    AddProgressLine(kPoolFellBackLine2);
                }
                // The every-row-skipped case: the run succeeded and genuinely
                // changed nothing, which a bare success line would not say.
                if (result.enemiesRandomized == 0) {
                    AddProgressLine(kNothingRandomizedLine);
                }
            }
            if (run_.randomizeBosses) {
                AddProgressLine("RANDOMIZED " + std::to_string(result.bossesRandomized) +
                                " BOSS PLACEMENTS");
            }
            if (run_.randomizeTreasure) {
                // D5: report workshop-tools state by extending this line rather
                // than a separate SKIPPING line, which would fire even when
                // treasure randomization was never going to run. No parentheses
                // - the 8x8 font has no punctuation glyphs (Font8x8.cpp), so
                // they'd render as blank gaps.
                std::string line = "RANDOMIZED " + std::to_string(result.treasuresRandomized) +
                                    " TREASURE PICKUPS";
                if (run_.randomizeWorkshopTools) line += " WORKSHOP TOOLS INCLUDED";
                AddProgressLine(line);
            }
            if (run_.randomizeStartingWeapons || run_.randomizeStartingGuns) {
                AddProgressLine("STARTING CHOICES: " +
                                std::to_string(result.startingMeleeChanged) + " WEAPONS, " +
                                std::to_string(result.startingGunsChanged) + " GUNS");
            }
            if (run_.randomizeShopWeapons) {
                AddProgressLine("RANDOMIZED " + std::to_string(result.shopWeaponsChanged) +
                                " SHOP WEAPONS");
            }
            if (run_.randomizeEnemyDrops) {
                AddProgressLine("RANDOMIZED " + std::to_string(result.dropsRandomized) +
                                " ENEMY DROPS");
            }
            // Reported only when it did something. There is no SKIPPING
            // counterpart: NO means the app deliberately left the file alone,
            // which is not a skipped step worth a line (D5).
            // Reported only when it is ON, because ON is the unusual
            // outcome: the whole game runs dark. OFF is the normal game.
            if (run_.enableMergoDarkness) {
                AddProgressLine("MERGO DARKNESS ENABLED - THE WORLD WILL BE DARK");
            }
            // Reported as a state rather than a count, deliberately: the
            // honest count covers both blocks of origin rows (HunterTools.cpp)
            // and a player who picks one origin would read 22 as a defect. No
            // SKIPPING counterpart, for the same reason enableMergoDarkness
            // has none - NO means the app left the file alone.
            if (run_.startWithHunterTools) {
                AddProgressLine("STARTING WITH BOTH HUNTER WORKSHOP TOOLS");
            }
            // One line per ENABLED easy setting, each carrying its count
            // (plan 018 P2). No SKIPPING counterparts, for the same reason
            // the two features above have none. The counts are fixed - 2 /
            // 60 / 3 / 14 - so a 0 or a wrong number here means a pattern
            // list or a map name is wrong. EASY ROM reads 60 and EASY
            // EMISSARY 14 because this port writes both map variants of
            // those two areas.
            if (run_.easyShadows) {
                AddProgressLine("EASY SHADOWS REPLACED " +
                                std::to_string(result.easyCounts.shadows) + " PLACEMENTS");
            }
            if (run_.easyRom) {
                AddProgressLine("EASY ROM REPLACED " +
                                std::to_string(result.easyCounts.rom) + " PLACEMENTS");
            }
            if (run_.easyFailures) {
                AddProgressLine("EASY FAILURES REPLACED " +
                                std::to_string(result.easyCounts.failures) + " PLACEMENTS");
            }
            if (run_.easyEmissary) {
                AddProgressLine("EASY EMISSARY REPLACED " +
                                std::to_string(result.easyCounts.emissary) + " PLACEMENTS");
            }
            // Only when it did something - an all-enabled pool is the default
            // and saying so every run is noise.
            if (run_.randomizeEnemies && !run_.enemiesIncluded.AllEnabled()) {
                AddProgressLine("ENEMY POOL LIMITED TO " +
                                std::to_string(run_.enemiesIncluded.CountEnabled()) + " OF " +
                                std::to_string(kEnemyPoolModelCount) + " ENEMIES");
            }
            // Only when it did something: nothing skipped is the default and
            // saying so every run is noise, exactly like the pool line above.
            if (run_.randomizeEnemies && run_.enemiesSkipped.CountEnabled() > 0) {
                AddProgressLine(std::to_string(run_.enemiesSkipped.CountEnabled()) + " OF " +
                                std::to_string(kEnemySkipModelCount) +
                                " ENEMIES SKIPPED");
            }
            if (run_.randomizeBosses && !run_.bossesIncluded.AllEnabled()) {
                AddProgressLine("BOSS POOL LIMITED TO " +
                                std::to_string(run_.bossesIncluded.CountEnabled()) + " OF " +
                                std::to_string(kBossPoolModelCount) + " BOSSES");
            }
            // The item-data archive is rewritten once for whichever param
            // features ran, so report it once rather than per feature.
            if (run_.randomizeEnemyDrops || run_.randomizeStartingWeapons ||
                run_.randomizeStartingGuns || run_.randomizeShopWeapons ||
                run_.startWithHunterTools) {
                AddProgressLine("ITEM DATA " + std::to_string(result.itemDataMembers) +
                                " ENTRIES, WROTE " +
                                std::to_string(result.itemDataWrittenBytes / 1048576) + " MB");
            }
        } else if (!result.error.empty()) {
            // Phase 4 reached the randomizer and it failed. The job's own
            // FAILED line named the phase; this one names the reason, in the
            // engine's words.
            AddProgressLine(kEnemyFailPrefix + result.error);
        }

        // Phase 7 reverts a one-shot START FRESH by appending a revision that
        // puts it back to KEEP EXISTING (B12). Said out loud, because the
        // player chose START FRESH deliberately and has to be able to see that
        // the choice was spent rather than kept.
        if (activation.revertedSavePolicy) {
            AddProgressLine("SAVE DATA IS BACK TO KEEP EXISTING - " +
                            activation.revertRevisionId);
        }

        // Re-read what the store now holds, so this screen's copy matches the
        // world that was just activated - the reverted SAVE DATA above most of
        // all. Nothing navigates back into Settings from here today, but a
        // screen holding a recipe that is no longer the world's is a bug
        // waiting for the first edit that does.
        if (activation.ok && !isVanilla_) LoadWorld();

        job_.reset();
    }

    // The closing flourish reads as "it worked, go play" - showing it after a
    // refusal or a failure would be actively misleading, so it is gated on
    // neither having happened above. A refusal is called out separately from a
    // failure because the two mean opposite things about the console: a
    // refusal changed nothing at all, and a failure stopped partway.
    completionLineStart_ = progressLines_.size();
    if (refused) {
        AddProgressLine("ACTIVATION REFUSED - NOTHING WAS CHANGED");
    } else if (failed) {
        AddProgressLine("ACTIVATION FAILED - CHECK THE LOG FOR DETAILS");
    } else {
        AddProgressLine("WHAT ARE YOU STILL DOING HERE");
        AddProgressLine("ENOUGH TREMBLING IN YOUR BOOTS");
        AddProgressLine("A HUNTER MUST HUNT");
    }

    commitFinished_ = true;
}

// ---------------------------------------------------------------------------
// Draw
// ---------------------------------------------------------------------------

void WorldEditorScreen::Draw(Renderer& renderer) {
    switch (step_) {
        case Step::Settings:      DrawSettings(renderer); break;
        case Step::EditName:      DrawEditName(renderer); break;
        case Step::EditSeed:      DrawEditSeed(renderer); break;
        case Step::Picker:
            // The picker draws over the screen it was opened from rather than
            // replacing it: the dimmed parent is what keeps the list's context
            // visible, as Bloodborne's own Origin picker does. FillRectBlend
            // is the one SDL entry point here that has never run on a PS4 - if
            // the screen behind is not dimmed but gone, see the plan's §4.3.
            DrawSettings(renderer);
            if (pickerSetting_) {
                const SettingDef& def = *pickerSetting_;
                renderer.FillRectBlend(0, 0, kScreenWidth, kScreenHeight, 0, 0, 0,
                                       kScrimAlpha);
                picker_.Draw(renderer, SelectionStrings(def), SelectionTable(def),
                             SelectionCount(def), SelectionFlags(def, run_));
            }
            break;
        case Step::History:       DrawHistory(renderer); break;
        case Step::Confirm:       DrawConfirm(renderer); break;
        case Step::Progress:      DrawProgress(renderer); break;
    }
}

void WorldEditorScreen::DrawSettings(Renderer& renderer) {
    renderer.Clear(20, 24, 28);

    DrawCenteredLabel(renderer, kTitleY, kScreenTitle, kTitleScale, Palette::Heading);

    // The header band. Decorative and never a focus target (plan 9 D1). It
    // carries only the target: the seed lives in the pane, and showing it here
    // as well put the same ten digits on screen twice at once. The seed is
    // stated again on Confirm, which is where it matters - immediately before
    // the run is committed.
    std::string target = std::string("TARGET  ") + run_.bloodborneTitleId;
    DrawLabelRight(renderer, kHeaderTargetRight, kHeaderY, target.c_str(), kRowScale,
                   Palette::Dim);

    renderer.FillRect(kRailX, kHeaderRuleY, 1800, kRuleThickness,
                      kRuleColor.r, kRuleColor.g, kRuleColor.b);
    renderer.FillRect(kPaneX - 20, kColumnRuleY, kRuleThickness, kColumnRuleH,
                      kRuleColor.r, kRuleColor.g, kRuleColor.b);
    renderer.FillRect(kHelpX - 20, kColumnRuleY, kRuleThickness, kColumnRuleH,
                      kRuleColor.r, kRuleColor.g, kRuleColor.b);

    DrawRail(renderer);
    DrawPane(renderer);
    DrawHelp(renderer);

    DrawCenteredLabel(renderer, kFooterY, kFooterLine, kFooterScale, Palette::Dim);
}

void WorldEditorScreen::DrawRailRow(Renderer& renderer, int y, const char* text,
                                     bool focused, bool current) {
    if (focused) {
        renderer.FillRect(kRailX, y + kBarOffsetY, kRailW, kBarHeight,
                          Palette::SelectedBar.r, Palette::SelectedBar.g,
                          Palette::SelectedBar.b);
    }
    // The category the pane is showing stays in the selected colour without
    // the bar whenever the cursor is elsewhere - in the pane, or on SEED or
    // FINISH - so it is still obvious which category those settings belong to,
    // and still obvious that the cursor has moved on.
    Color color = (focused || current) ? Palette::Selected : Palette::Text;
    DrawLabelLeft(renderer, kRailX, y, text, kRowScale, color);
}

void WorldEditorScreen::DrawRail(Renderer& renderer) {
    bool railHasFocus = (focus_ == Focus::Rail);

    // Two bare navigation labels, like the category rows below them. Their
    // values live in the pane, which is directly to their right and would
    // otherwise be repeating them a few hundred pixels away.
    DrawRailRow(renderer, kRailRow0Y, "NAME",
                railHasFocus && railCursor_ == kNameRow, false);
    DrawRailRow(renderer, kRailRow1Y, "SEED",
                railHasFocus && railCursor_ == kSeedRow, false);

    // NAME and SEED sit outside the category block, separated from it by their
    // own rule - the same treatment HISTORY gets below the categories.
    renderer.FillRect(kRailX, kRailRuleY, kRailW, kRuleThickness,
                      kRuleColor.r, kRuleColor.g, kRuleColor.b);

    for (int i = 0; i < kCategoryCount; i++) {
        DrawRailRow(renderer, kRailFirstY + i * kRailPitch,
                    CategoryLabel(kCategories[i]),
                    railHasFocus && railCursor_ == kFirstCatRow + i,
                    i == lastCategory_);
    }

    renderer.FillRect(kRailX, kRailRule2Y, kRailW, kRuleThickness,
                      kRuleColor.r, kRuleColor.g, kRuleColor.b);

    DrawRailRow(renderer, kHistoryY, "HISTORY",
                railHasFocus && railCursor_ == kHistoryRow, false);
}

void WorldEditorScreen::DrawPane(Renderer& renderer) {
    // The NAME and SEED rail rows show their values here rather than beside
    // themselves, so the pane is not left showing a category the cursor has
    // moved away from. They are the two rail rows whose content is not a
    // settings list; HISTORY still keeps the last category on screen, because
    // its own content is a whole mode rather than a readout and blanking the
    // middle would lose the context its help is about.
    if (focus_ == Focus::Rail && railCursor_ == kNameRow) {
        DrawLabelLeft(renderer, kPaneX, kPaneHeadingY, "NAME", kHeadingScale,
                      Palette::Heading);
        DrawLabelLeft(renderer, kPaneX, kPaneLayout.firstY, "WORLD NAME", kRowScale,
                      Palette::Text);
        DrawLabelRight(renderer, kPaneValueRight, kPaneLayout.firstY,
                       NameDisplayText().c_str(), kRowScale, Palette::Text);
        // What the last OPTIONS press did, where the player can see it without
        // leaving the screen (B14): a rename records no revision and a settings
        // change does, and the difference is invisible otherwise.
        if (!saveNote_.empty()) {
            DrawLabelLeft(renderer, kPaneX,
                          kPaneLayout.firstY + kPaneLayout.spacing,
                          saveNote_.c_str(), kRowScale, Palette::Dim);
        }
        return;
    }

    if (focus_ == Focus::Rail && railCursor_ == kSeedRow) {
        DrawLabelLeft(renderer, kPaneX, kPaneHeadingY, "SEED", kHeadingScale,
                      Palette::Heading);
        DrawLabelLeft(renderer, kPaneX, kPaneLayout.firstY, "RANDOMIZER SEED", kRowScale,
                      Palette::Text);
        DrawLabelRight(renderer, kPaneValueRight, kPaneLayout.firstY,
                       SeedDisplayText().c_str(), kRowScale, Palette::Text);
        return;
    }

    SettingCategory category = Category();

    DrawLabelLeft(renderer, kPaneX, kPaneHeadingY, CategoryLabel(category), kHeadingScale,
                  Palette::Heading);

    int count   = CategorySize(category);
    int visible = VisibleRowCount(kPaneLayout);
    int offset  = ClampScroll(listScroll_[lastCategory_], count, visible);

    for (int row = 0; row < visible; row++) {
        int index = offset + row;
        if (index >= count) break;

        const SettingDef& def = SettingInCategory(category, index);
        int  y       = kPaneLayout.firstY + row * kPaneLayout.spacing;
        bool focused = (focus_ == Focus::List && index == listCursor_[lastCategory_]);

        if (focused) {
            renderer.FillRect(kPaneX, y + kBarOffsetY, kPaneW, kBarHeight,
                              Palette::SelectedBar.r, Palette::SelectedBar.g,
                              Palette::SelectedBar.b);
        }
        Color color = focused ? Palette::Selected : Palette::Text;
        DrawLabelLeft(renderer, kPaneX, y, def.label, kRowScale, color);
        DrawLabelRight(renderer, kPaneValueRight, y,
                       SettingValueText(def, run_).c_str(), kRowScale, color);
    }

    DrawPaneScrollHints(renderer, kPaneLayout, kPaneX, kPaneW, count, offset, visible);
}

void WorldEditorScreen::DrawHelp(Renderer& renderer) {
    const char* title;
    std::vector<std::string> bodyLines;

    // Focus can only be in the pane while the rail cursor is on a category, so
    // these three are the whole of the non-category case.
    if (railCursor_ == kNameRow) {
        title = "NAME";
        bodyLines = WrapText(renderer, NameHelp(), kRowScale, kHelpW);
    } else if (railCursor_ == kSeedRow) {
        title = "SEED";
        bodyLines = WrapText(renderer, SeedHelp(), kRowScale, kHelpW);
    } else if (railCursor_ == kHistoryRow) {
        title = "HISTORY";
        bodyLines = WrapText(renderer, HistoryHelp(), kRowScale, kHelpW);
    } else {
        const SettingDef& def = SelectedSetting();
        title = def.label;
        bodyLines = WrapText(renderer, def.help, kRowScale, kHelpW);
    }

    // Wrapped every frame rather than cached: the wrap is a few hundred
    // advance lookups for one string, against the several hundred glyph blits
    // the same frame already costs, and a cache here would be the only thing
    // in the draw path needing invalidation.
    std::vector<std::string> titleLines = WrapText(renderer, title, kRowScale, kHelpW);
    for (int i = 0; i < (int)titleLines.size() && i < kHelpTitleMaxLines; i++) {
        DrawLabelLeft(renderer, kHelpX, kHelpTitleY + i * kHelpPitch,
                      titleLines[(size_t)i].c_str(), kRowScale, Palette::Selected);
    }

    renderer.FillRect(kHelpX, kHelpRuleY, kHelpW, kRuleThickness,
                      kRuleColor.r, kRuleColor.g, kRuleColor.b);

    for (int i = 0; i < (int)bodyLines.size() && i < kHelpBodyMaxLines; i++) {
        DrawLabelLeft(renderer, kHelpX, kHelpBodyY + i * kHelpPitch,
                      bodyLines[(size_t)i].c_str(), kRowScale, Palette::Text);
    }
}

// The same shape as DrawEditSeed and DrawEditTitleId: one full-screen row of
// characters with the one under the cursor highlighted. The buffer is always
// exactly kNameLen characters, space-padded, so a name being typed from the
// left does not jump about as it grows.
void WorldEditorScreen::DrawEditName(Renderer& renderer) {
    renderer.Clear(20, 24, 28);

    DrawCenteredLabel(renderer, 300, "WORLD NAME", kTitleScale, Palette::Heading);

    const int kEditScale = 6;
    int totalWidth = renderer.TextWidth(nameBuf_, kEditScale);
    int startX     = (kScreenWidth - totalWidth) / 2;
    int y          = 500;

    // A space has an advance and no ink, so an empty slot still moves the pen
    // and still gets a cursor position - but nothing would mark it. The
    // underscore bar below the cursor is what makes a blank slot visible.
    char single[2] = { 0, 0 };
    int pen = startX;
    for (int i = 0; i < kNameLen; i++) {
        single[0] = nameBuf_[i];
        Color c = (i == nameCursor_) ? Palette::Selected : Palette::Text;
        renderer.DrawText(pen, y, single, kEditScale, c.r, c.g, c.b);
        int advance = renderer.TextWidth(single, kEditScale);
        if (i == nameCursor_) {
            renderer.FillRect(pen, y + 100, advance, 4,
                              Palette::Selected.r, Palette::Selected.g,
                              Palette::Selected.b);
        }
        pen += advance;
    }

    DrawCenteredLabel(renderer, kScreenHeight - 130,
                      "LEFT RIGHT SELECT   UP DOWN CHANGE", kFooterScale, Palette::Dim);
    DrawCenteredLabel(renderer, kScreenHeight - 80, "X ACCEPT   O CANCEL",
                      kFooterScale, Palette::Dim);
}

void WorldEditorScreen::DrawEditSeed(Renderer& renderer) {
    renderer.Clear(20, 24, 28);

    DrawCenteredLabel(renderer, 300, "RANDOMIZER SEED", kTitleScale, Palette::Heading);

    // Drawn a character at a time so the one under the cursor can be
    // highlighted - same approach as DrawEditTitleId.
    const int kEditScale = 6;
    int totalWidth = renderer.TextWidth(seedBuf_, kEditScale);
    int startX     = (kScreenWidth - totalWidth) / 2;
    int y          = 500;

    // Advance by each character's own width rather than a single "A" width:
    // the font is proportional, so a fixed per-character step would drift and
    // would not add up to totalWidth (which is what startX centres on).
    int pen = startX;
    for (int i = 0; i < kSeedDigits; i++) {
        char single[2] = { seedBuf_[i], '\0' };
        Color c = (i == seedCursor_) ? Palette::Selected : Palette::Text;
        renderer.DrawText(pen, y, single, kEditScale, c.r, c.g, c.b);
        pen += renderer.TextWidth(single, kEditScale);
    }

    DrawCenteredLabel(renderer, kScreenHeight - 130,
                      "LEFT RIGHT SELECT   UP DOWN CHANGE   SQUARE RANDOM",
                      kFooterScale, Palette::Dim);
    DrawCenteredLabel(renderer, kScreenHeight - 80, "X ACCEPT   O CANCEL",
                      kFooterScale, Palette::Dim);
}

// Roughly how long the activation takes (B10). Three phrases, chosen from
// which phases will actually run - see the constants above for the arithmetic.
const char* WorldEditorScreen::DurationText() const {
    if (!planReady_) return kDurationUnknown;
    // Phase 4 is the expensive one and Vanilla skips it, because activating
    // Vanilla removes a tree rather than building one.
    if (!plan_.toVanilla) return kDurationLong;
    bool copying = plan_.containerExists && plan_.containerHasGameFiles;
    bool moving  = (plan_.saveAction == SaveAction::RestoreOwn ||
                    plan_.saveAction == SaveAction::AdoptLive);
    return (copying || moving) ? kDurationMedium : kDurationShort;
}

namespace {
// Where the outgoing world's save goes (B10), in the order the transaction
// decides it: no container at all, a container with nothing in it, a backup
// with no world to file it into, and the ordinary case.
const char* OutgoingSaveText(const ActivationPlan& p, bool ready) {
    if (!ready)                   return "-";
    if (!p.containerExists)       return kOutgoingNoContainer;
    if (!p.containerHasGameFiles) return kOutgoingEmpty;
    if (p.fromWorldId.empty())    return kOutgoingNowhere;
    return kOutgoingFiled;
}
} // namespace

// The B10 statement: eight rows, always eight, whatever the plan says. A row
// that appears only sometimes is a list whose geometry has to be measured
// twice and a screen whose shape moves under the player.
std::vector<std::string> WorldEditorScreen::ConfirmHeadRows() const {
    std::vector<std::string> rows;
    // The three that were here before activation was: what this world IS.
    rows.push_back(std::string("NAME") + kConfirmGap + NameDisplayText());
    rows.push_back(std::string("SEED") + kConfirmGap +
                   (isVanilla_ ? std::string("NONE") : SeedDisplayText()));
    rows.push_back(std::string("TARGET") + kConfirmGap + run_.bloodborneTitleId);
    // The five activation adds: what is deactivated, where its save goes, what
    // is activated, what happens to the incoming save, and how long it takes.
    rows.push_back(std::string(kRowDeactivating) + kConfirmGap +
                   (plan_.fromWorldId.empty() ? std::string(kNoOutgoingWorld)
                                              : plan_.fromWorldName));
    // A refused activation does nothing to either save, so neither row states
    // one: they say "-" rather than describing a copy that will not happen.
    bool willRun = planReady_ && plan_.ok;
    rows.push_back(std::string(kRowOutgoingSave) + kConfirmGap +
                   OutgoingSaveText(plan_, willRun));
    rows.push_back(std::string(kRowActivating) + kConfirmGap +
                   (plan_.toWorldName.empty() ? NameDisplayText() : plan_.toWorldName));
    rows.push_back(std::string(kRowIncomingSave) + kConfirmGap +
                   (willRun ? SaveActionName(plan_.saveAction) : "-"));
    rows.push_back(std::string(kRowHowLong) + kConfirmGap + DurationText());
    return rows;
}

// The statement, then the recipe it is about. There is no second hardcoded
// copy of the settings list anywhere in this file - that parallel structure,
// and the row constants it had to agree with, is what feature 034 removed.
//
// Vanilla contributes no settings rows at all: it has no recipe (B20), and
// showing the DEFAULTS tab's settings beside VANILLA would be stating a
// randomization that is about to be deleted.
std::vector<std::string> WorldEditorScreen::ConfirmItems() const {
    std::vector<std::string> items = ConfirmHeadRows();
    if (isVanilla_) return items;
    for (int i = 0; i < SettingCount(); i++) {
        const SettingDef& def = SettingAt(i);
        items.push_back(std::string(def.label) + kConfirmGap +
                        SettingValueText(def, run_));
    }
    return items;
}

void WorldEditorScreen::DrawConfirm(Renderer& renderer) {
    renderer.Clear(20, 24, 28);

    DrawCenteredLabel(renderer, 140, kScreenTitle, kTitleScale, Palette::Heading);

    // The state line. Three things can own it, in this order: the store could
    // not write the world (so what is on disk is not what is on this screen),
    // the activation is refused (so nothing will be written), or the OPTIONS
    // press that opened this screen did something worth naming - a rename
    // records no revision and a settings change does, and the difference is
    // invisible otherwise (B14).
    bool refused = planReady_ && plan_.refusal.Refused();
    std::string state;
    Color stateColor = Palette::Text;
    if (!storeError_.empty()) {
        state      = storeError_;
        stateColor = Palette::Bad;
    } else if (refused) {
        state      = std::string(kCannotActivate) + " - " +
                     RefusalReasonName(plan_.refusal.reason);
        stateColor = Palette::Bad;
    } else if (!planReady_) {
        state = kConfirmChecking;
    } else if (!saveNote_.empty()) {
        state = saveNote_;
    } else {
        state = kConfirmHeading;
    }
    DrawCenteredLabel(renderer, kConfirmStateY, state.c_str(), kItemScale, stateColor);

    // Underneath it, in the player's words: which row of the phase-6 table
    // applies, or the refusal's own sentence. Wrapped at the row scale because
    // both run well past what scale 4 fits across 1920.
    if (planReady_) {
        const char* sentence = refused ? plan_.refusal.sentence.c_str()
                                       : SaveActionSentence(plan_.saveAction);
        std::vector<std::string> lines =
            WrapText(renderer, sentence, kRowScale, kConfirmSentenceW);
        for (int i = 0; i < (int)lines.size() && i < kConfirmSentenceMax; i++) {
            DrawCenteredLabel(renderer, kConfirmSentenceY + i * kConfirmSentencePitch,
                              lines[(size_t)i].c_str(), kRowScale,
                              refused ? Palette::Bad : Palette::Dim);
        }
    }

    std::vector<std::string> items = ConfirmItems();
    DrawScrollableList(renderer, kSettingsLayout, items, -1, confirmScroll_, kItemScale,
                       Palette::Text, Palette::Selected);

    // 30px lower than every other footer in the app, and deliberately so:
    // under the atlas's real ink box no hint gap can clear both this list's
    // last row at y=870 and a footer at kScreenHeight - 130. Moving the pair
    // down is what keeps six visible rows instead of five - see
    // ui_scroll_verify.py, which asserts both ends.
    //
    // OPTIONS is offered only when it would do something. A footer that
    // advertises a button which silently declines is worse than one that does
    // not mention it.
    bool canActivate = planReady_ && plan_.ok && storeError_.empty();
    DrawCenteredLabel(renderer, kScreenHeight - 100, kConfirmFooterHint, kFooterScale,
                      Palette::Dim);
    DrawCenteredLabel(renderer, kScreenHeight - 50,
                      canActivate ? kConfirmFooterGo : kConfirmFooterBack,
                      kFooterScale, Palette::Dim);
}

// The revision list (B16): newest first, with a cursor, because selecting one
// is what makes it current again. A world that has never been written has no
// revisions and says so rather than drawing an empty band.
void WorldEditorScreen::DrawHistory(Renderer& renderer) {
    renderer.Clear(20, 24, 28);

    DrawCenteredLabel(renderer, 140, kScreenTitle, kTitleScale, Palette::Heading);
    DrawCenteredLabel(renderer, 240, "HISTORY", kItemScale, Palette::Text);

    std::vector<std::string> items = HistoryItems();
    if (items.empty()) {
        DrawCenteredLabel(renderer, kHistoryLayout.firstY,
                          "THIS WORLD HAS NOT BEEN SAVED YET", kRowScale, Palette::Dim);
        DrawCenteredLabel(renderer, kScreenHeight - 80, "O BACK", kFooterScale,
                          Palette::Dim);
        return;
    }

    int visible = VisibleRowCount(kHistoryLayout);
    int offset  = ClampScroll(historyScroll_, (int)items.size(), visible);
    DrawScrollableList(renderer, kHistoryLayout, items, historyCursor_, offset,
                       kRowScale, Palette::Text, Palette::Selected);

    DrawCenteredLabel(renderer, kScreenHeight - 130, "UP DOWN MOVE", kFooterScale,
                      Palette::Dim);
    DrawCenteredLabel(renderer, kScreenHeight - 80, "X MAKE CURRENT   O BACK",
                      kFooterScale, Palette::Dim);
}

void WorldEditorScreen::DrawProgress(Renderer& renderer) {
    renderer.Clear(20, 24, 28);

    DrawCenteredLabel(renderer, 120, kScreenTitle, kTitleScale, Palette::Heading);
    DrawCenteredLabel(renderer, 200, "PROGRESS", kItemScale, Palette::Text);

    // The log scrolls, so it draws a window rather than everything. While the
    // run is going the bottom slot belongs to the live status line, which is
    // why the log gets one fewer row then than it does afterwards.
    int slots     = VisibleRowCount(kProgressLayout);
    int logRows   = job_ ? slots - 1 : slots;
    if (logRows < 1) logRows = 1;
    int lineCount = (int)progressLines_.size();

    // Pinned to the tail until the user scrolls away from it. Writing the
    // resolved offset back means a later up/down starts from what's actually
    // on screen rather than from a stale value.
    int offset = progressFollowTail_ ? lineCount - logRows : progressScroll_;
    offset = ClampScroll(offset, lineCount, logRows);
    progressScroll_ = offset;

    // The trailing completion message is colored distinctly to read as
    // "done", not as another status line.
    int drawn = 0;
    for (int row = 0; row < logRows; row++) {
        int index = offset + row;
        if (index >= lineCount) break;
        Color c = ((std::size_t)index >= completionLineStart_) ? Palette::Good : Palette::Text;
        DrawCenteredLabel(renderer, kProgressLayout.firstY + row * kProgressLayout.spacing,
                          progressLines_[(std::size_t)index].c_str(), kProgressScale, c);
        drawn++;
    }

    // While the randomizer is running this is the only line that changes,
    // and it's the whole point of the screen - what's happening right now.
    // It sits directly under the last log line, inside the reserved slot.
    if (job_) {
        std::string live = job_->StatusText() + "  " +
                           std::to_string((int)(job_->Progress() * 100.0f + 0.5f)) + "%";
        DrawCenteredLabel(renderer, kProgressLayout.firstY + drawn * kProgressLayout.spacing,
                          live.c_str(), kProgressScale, Palette::Heading);
    }

    DrawScrollHints(renderer, kProgressLayout, lineCount, offset, logRows);

    if (commitFinished_) {
        DrawCenteredLabel(renderer, kScreenHeight - 130, "UP DOWN SCROLL", kFooterScale,
                          Palette::Dim);
        DrawCenteredLabel(renderer, kScreenHeight - 80, "O RETURN TO WORLDS", kFooterScale,
                          Palette::Dim);
    }
}

} // namespace bbr
