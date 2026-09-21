// cstdlib before anything pulling <random>/<cmath>, or the toolchain's libc++
// fails on ::abs - same ordering constraint as BossRandomizer.cpp.
#include <cstdlib>

#include "EnableWizardScreen.h"

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
const int kRailRow0Y    = 230;
const int kRailRuleY    = 296;
const int kRailFirstY   = 330;   // same band and same pitch as the pane rows
const int kRailPitch    = 76;
const int kPaneHeadingY = 220;
const int kHelpTitleY   = 220;
const int kHelpPitch    = 52;
const int kHelpRuleY    = 330;
const int kHelpBodyY    = 356;
const int kHelpTitleMaxLines = 2;
const int kHelpBodyMaxLines  = 11;
const int kFooterY      = 1000;

const int kRuleThickness = 2;

// FINISH sits below the six categories, separated from them by its own rule -
// the mirror of what the SEED row above them gets. Setup Defaults has neither,
// because it has no commit path.
const int kRailRule2Y = 778;
const int kFinishY    = 806;

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

// No OPTIONS on this screen: OPTIONS commits, and it commits on Confirm only
// (spec section 10, 9.2). The way to Confirm is the FINISH rail row.
const char* const kFooterLine =
    "UP DOWN MOVE   LEFT RIGHT CHANGE   X SELECT   O BACK";

// Confirm's list is still a flat review list, so it keeps the band the flat
// settings list used: from under the sub-heading down to just above where the
// "MORE BELOW" hint has to clear the footer.
const ListLayout kSettingsLayout = { 420, 90, 870, 60 };

// The progress log is denser and starts higher. One slot is given up to the
// live status line while the run is going, so the log itself shows one fewer
// line then than it does once the run has finished.
const ListLayout kProgressLayout = { 300, 70, 920, 50 };
} // namespace

EnableWizardScreen::EnableWizardScreen(RandomizerDefaults& defaults)
    : defaults_(defaults),
      run_(defaults),
      seed_(defaults.lastSeed) {
    // 0 means defaults.cfg has never carried a seed, so roll one immediately -
    // the row should never come up blank or showing a meaningless zero.
    if (seed_ == 0) RollNewSeed();
}

// No cursor or scroll reset here any more. It used to zero both
// unconditionally, which is exactly what "returning restores the position the
// player left" forbids (spec section 2): every step now owns its own cursor
// state and nothing clears it on the way past.
void EnableWizardScreen::GoToStep(Step step) {
    step_ = step;
}

std::string EnableWizardScreen::SeedDisplayText() const {
    std::string digits = std::to_string(seed_);
    // A uint32 is at most 10 digits so this can't currently underflow, but the
    // subtraction is unsigned - guard it rather than depend on the type.
    if (digits.size() >= (size_t)kSeedDigits) return digits;
    // Zero-padded so the row and the digit editor show the same thing.
    return std::string((size_t)kSeedDigits - digits.size(), '0') + digits;
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

void EnableWizardScreen::RollNewSeed() {
    seed_ = NextSeedValue();
    Log(("enable wizard: rolled new seed " + std::to_string(seed_)).c_str());
}

const SettingDef& EnableWizardScreen::SelectedSetting() const {
    return SettingInCategory(Category(), listCursor_[lastCategory_]);
}

void EnableWizardScreen::Update(const ButtonEdges& input) {
    switch (step_) {
        case Step::Settings:      UpdateSettings(input); break;
        case Step::EditSeed:      UpdateEditSeed(input); break;
        case Step::Picker:        UpdatePicker(input); break;
        case Step::Confirm:       UpdateConfirm(input); break;
        case Step::Progress:      UpdateProgress(input); break;
    }
}

void EnableWizardScreen::UpdateSettings(const ButtonEdges& input) {
    if (focus_ == Focus::Rail) UpdateRail(input);
    else                       UpdatePane(input);
}

void EnableWizardScreen::UpdateRail(const ButtonEdges& input) {
    railCursor_ = NavigateVertical(railCursor_, kRailItemCount, input);

    // Landing on a category is what changes which settings the pane shows;
    // landing on SEED or FINISH deliberately does not, so the pane never
    // blanks (plan P13).
    if (railCursor_ >= 1 && railCursor_ <= kCategoryCount) {
        lastCategory_ = railCursor_ - 1;
    }

    // Left/Right do nothing anywhere on the rail. They used to roll a new seed
    // on row 0, which made an irreversible change to the run from a direction
    // press, with no confirmation and no undo. The seed is now only ever
    // changed inside its own editor, reached with X.

    if (input.cross) {
        if (railCursor_ == 0) {
            OpenSeedEditor();
        } else if (railCursor_ == kFinishRow) {
            Log("enable wizard: FINISH - opening confirm screen");
            confirmScroll_ = 0; // the only place Confirm's scroll is zeroed
            GoToStep(Step::Confirm);
        } else {
            focus_ = Focus::List;
        }
    }

    if (input.circle) {
        Log("enable wizard: cancelled at first step - no changes made");
        requestedScreen_ = ScreenId::Menu;
    }
}

void EnableWizardScreen::UpdatePane(const ButtonEdges& input) {
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
        Log((std::string("enable wizard: ") + def.label + " = " +
             SettingValueText(def, run_)).c_str());
    }

    // X advances: into the picker on a drill-in, and nowhere at all on a
    // toggle. It must not toggle - see the header.
    if (input.cross && IsDrillIn(def)) {
        picker_.Reset();
        pickerSetting_ = &def;
        Log((std::string("enable wizard: opening picker for ") + def.label).c_str());
        GoToStep(Step::Picker);
    }

    if (input.circle) focus_ = Focus::Rail;
}

void EnableWizardScreen::OpenSeedEditor() {
    std::string padded = SeedDisplayText();
    memcpy(seedBuf_, padded.c_str(), kSeedDigits);
    seedBuf_[kSeedDigits] = '\0';
    seedCursor_ = 0;
    Log("enable wizard: opening seed editor");
    GoToStep(Step::EditSeed);
}

void EnableWizardScreen::UpdateEditSeed(const ButtonEdges& input) {
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
        Log(("enable wizard: rolled " + std::to_string(rolled) + " into the seed editor").c_str());
    }

    if (input.cross) {
        // Ten digits can express more than a uint32 holds, so clamp rather
        // than wrap - a silently wrapped seed would be a seed you can't retype.
        unsigned long long value = strtoull(seedBuf_, nullptr, 10);
        if (value > 0xFFFFFFFFull) {
            value = 0xFFFFFFFFull;
            Log("enable wizard: seed clamped to 4294967295");
        }
        seed_ = (uint32_t)value;
        Log(("enable wizard: seed set to " + std::to_string(seed_)).c_str());
        GoToStep(Step::Settings);
    }

    if (input.circle) {
        Log("enable wizard: seed edit cancelled - unchanged");
        GoToStep(Step::Settings);
    }
}

// Straight back to the row it was opened from: nothing cleared the pane's
// cursor while the picker was up, so nothing has to restore it. This is what
// ReturnFromPicker used to exist for, and why it no longer does.
void EnableWizardScreen::UpdatePicker(const ButtonEdges& input) {
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

void EnableWizardScreen::UpdateConfirm(const ButtonEdges& input) {
    // No cursor here - this is a review list - so up/down move the window
    // itself, which is the only way to read the settings that don't fit.
    int visible = VisibleRowCount(kSettingsLayout);
    int count   = SettingCount() + 1; // the seed row, then every setting
    if (input.up)   confirmScroll_--;
    if (input.down) confirmScroll_++;
    confirmScroll_ = ClampScroll(confirmScroll_, count, visible);

    if (input.options) {
        Log("enable wizard: commit confirmed - running commit");
        // Switch screens BEFORE any work happens: the randomizer is stepped
        // from UpdateProgress across many frames, so the progress screen has
        // to already be the thing being drawn.
        StartCommit();
        GoToStep(Step::Progress);
    }

    if (input.circle) {
        // Back to the settings screen exactly as it was left - the rail cursor
        // is still on FINISH and lastCategory_ is still the category the
        // player was in, because nothing cleared either (spec section 2).
        Log("enable wizard: confirm cancelled - back to settings");
        GoToStep(Step::Settings);
    }
}

void EnableWizardScreen::UpdateProgress(const ButtonEdges& input) {
    // One coarse unit of randomizer work per frame. The frame loop is
    // serial (Update -> Draw -> Present), so doing the whole 10-20s run in
    // one call would leave nothing on screen until it finished - stepping
    // it here is what makes the progress display possible at all.
    if (job_) {
        job_->Step();
        if (job_->Done()) FinishCommit();
        return; // input is ignored while the commit is still running
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
        Log("enable wizard: returning to menu");
        requestedScreen_ = ScreenId::Menu;
    }
}

namespace {
// A one-time manual FTP drop point: nothing on the PS4 ships a vanilla
// dvdroot_ps4 copy (AFR is a redirect/overlay over the real installed game,
// not a full tree - see UI_BLUEPRINT.md/project memory), so the randomizer
// needs its own read-only vanilla source to randomize FROM. The user seeds
// this folder by hand, once, over FTP.
const char* kVanillaSourceDir = "/data/bbrandomizer/VanillaSource/dvdroot_ps4";
} // namespace

void EnableWizardScreen::AddProgressLine(const std::string& line) {
    progressLines_.push_back(line);
    Log(("enable wizard: " + line).c_str());
}

void EnableWizardScreen::StartCommit() {
    progressLines_.clear();
    completionLineStart_ = (std::size_t)-1;
    commitFinished_ = false;
    job_.reset();

    // The seed is now always chosen on the settings screen - rolled or typed -
    // so this reports it rather than generating one.
    uint32_t seed = seed_;
    AddProgressLine("USING SEED " + std::to_string(seed));

    // Remembered so the next run opens showing it: "do that again" becomes
    // open-the-wizard-and-commit. Only lastSeed is written back; the wizard's
    // own per-run settings live in run_ and are deliberately never assigned
    // over defaults_, as they never have been.
    defaults_.lastSeed = seed;
    SaveRandomizerDefaults(defaults_);

    // D12/D8: an empty pool would be caught by StepBuildPool, but only after
    // the mirror phase has already copied most of the game into the AFR folder
    // - so the user would watch it fail and be left with a half-built tree.
    // Refuse up front instead, and say what to do about it.
    if (run_.randomizeEnemies && run_.enemiesIncluded.NoneEnabled()) {
        AddProgressLine("NO ENEMIES SELECTED");
        AddProgressLine("SELECT AT LEAST ONE ENEMY OR TURN OFF RANDOMIZE ENEMIES");
        completionLineStart_ = progressLines_.size();
        AddProgressLine("COMMIT CANCELLED - NOTHING WAS WRITTEN");
        commitFinished_ = true;
        return;
    }

    // The boss equivalent, and it guards a nastier failure: the boss pool is
    // drained as arenas are assigned and refilled from a model-distinct copy
    // when it empties. With no bosses selected BOTH are empty, so the refill
    // never helps and the next draw calls RandIndex(rng, 0) -> a
    // uniform_int_distribution(0, -1), which is undefined behaviour rather
    // than a clean failure. BossRandomizer.cpp's own header warns about this.
    if (run_.randomizeBosses && run_.bossesIncluded.NoneEnabled()) {
        AddProgressLine("NO BOSSES SELECTED");
        AddProgressLine("SELECT AT LEAST ONE BOSS OR TURN OFF RANDOMIZE BOSSES");
        completionLineStart_ = progressLines_.size();
        AddProgressLine("COMMIT CANCELLED - NOTHING WAS WRITTEN");
        commitFinished_ = true;
        return;
    }

    if (!run_.randomizeEnemies) AddProgressLine("SKIPPING ENEMY RANDOMIZATION");
    if (!run_.randomizeBosses)  AddProgressLine("SKIPPING BOSS RANDOMIZATION");
    if (!run_.randomizeTreasure) AddProgressLine("SKIPPING TREASURE RANDOMIZATION");
    if (!run_.randomizeEnemyDrops) AddProgressLine("SKIPPING ENEMY DROP RANDOMIZATION");
    if (!run_.randomizeStartingWeapons) AddProgressLine("SKIPPING STARTING WEAPON RANDOMIZATION");
    if (!run_.randomizeStartingGuns) AddProgressLine("SKIPPING STARTING GUN RANDOMIZATION");
    if (!run_.randomizeShopWeapons) AddProgressLine("SKIPPING SHOP WEAPON RANDOMIZATION");

    // enableMergoDarkness IS in this list, unlike randomizeWorkshopTools
    // below: it is a complete change on its own rather than a modifier on
    // another feature, so turning it on alone must still build a tree (D4 in
    // docs/plans/mergo-darkness.md). There is no asymmetry to worry
    // about because the setting is named for its action - YES is the only
    // state that writes anything, and NO asks for nothing to be done.
    // The four easy-mode settings are in this list for the same reason
    // enableMergoDarkness is: each one writes real map files on its own and
    // needs no other feature to mean anything, so ticking one alone must
    // start a run. They are deliberately NOT gated on randomizeEnemies.
    if (run_.randomizeEnemies || run_.randomizeBosses || run_.randomizeTreasure ||
        run_.randomizeEnemyDrops || run_.randomizeStartingWeapons ||
        run_.randomizeStartingGuns || run_.randomizeShopWeapons ||
        run_.enableMergoDarkness || run_.startWithHunterTools ||
        run_.easyShadows || run_.easyRom || run_.easyFailures || run_.easyEmissary) {
        std::string outputDir = "/data/GoldHEN/AFR/" + run_.bloodborneTitleId + "/dvdroot_ps4";
        EnemyRandomizerOptions options;
        options.randomizeEnemies = run_.randomizeEnemies;
        options.randomizeBosses = run_.randomizeBosses;
        options.randomizeTreasure = run_.randomizeTreasure;
        // Meaningless without randomizeTreasure (D3), so it deliberately does
        // NOT appear in the big || above - ticking it alone must not start a
        // run that does nothing.
        // Meaningless without randomizeEnemies for the same reason (D3), so it
        // is likewise absent from the big || above.
        options.randomizeWorkshopTools = run_.randomizeWorkshopTools;
        options.randomizeEnemyDrops = run_.randomizeEnemyDrops;
        options.randomizeStartingWeapons = run_.randomizeStartingWeapons;
        options.randomizeStartingGuns = run_.randomizeStartingGuns;
        options.randomizeShopWeapons = run_.randomizeShopWeapons;
        options.enableMergoDarkness = run_.enableMergoDarkness;
        // Meaningless without randomizeEnemies (spec 033 B10), exactly like
        // randomizeWorkshopTools above, so it too is absent from the big ||:
        // ticking it alone must not start a run that does nothing. It also
        // gets no SKIPPING line - NO is simply the run this app already made.
        options.doNotRandomizeCagedDogs = run_.doNotRandomizeCagedDogs;
        // IS in the big || above, unlike the two modifiers either side of it:
        // this one changes the game on its own - it needs no other feature to
        // mean anything - so ticking it alone must build a tree, exactly like
        // enableMergoDarkness.
        options.startWithHunterTools = run_.startWithHunterTools;
        // Four independent settings on one struct, none of them a
        // randomizer and none of them a modifier on another feature - they
        // apply whether or not anything else above is on, and they are the
        // last writer of the placements they touch. See EasyModes.h.
        options.easyModes.shadows = run_.easyShadows;
        options.easyModes.rom = run_.easyRom;
        options.easyModes.failures = run_.easyFailures;
        options.easyModes.emissary = run_.easyEmissary;
        options.enemiesIncluded = run_.enemiesIncluded;
        options.bossesIncluded = run_.bossesIncluded;
        options.enemiesSkipped = run_.enemiesSkipped;
        job_.reset(new EnemyRandomizerJob(kVanillaSourceDir, outputDir, seed, options));
    } else {
        FinishCommit();
    }
}

void EnableWizardScreen::FinishCommit() {
    bool failed = false;

    if (job_) {
        const EnemyRandomizerResult& result = job_->Result();
        if (result.success) {
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
        } else {
            AddProgressLine(kEnemyFailPrefix + result.error);
            failed = true;
        }
        job_.reset();
    }

    // The closing flourish reads as "it worked, go play" - showing it after
    // a failure would be actively misleading, so it's gated on nothing
    // having failed above.
    completionLineStart_ = progressLines_.size();
    if (!failed) {
        AddProgressLine("WHAT ARE YOU STILL DOING HERE");
        AddProgressLine("ENOUGH TREMBLING IN YOUR BOOTS");
        AddProgressLine("A HUNTER MUST HUNT");
    } else {
        AddProgressLine("COMMIT FAILED - CHECK THE LOG FOR DETAILS");
    }

    commitFinished_ = true;
}

// ---------------------------------------------------------------------------
// Draw
// ---------------------------------------------------------------------------

void EnableWizardScreen::Draw(Renderer& renderer) {
    switch (step_) {
        case Step::Settings:      DrawSettings(renderer); break;
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
        case Step::Confirm:       DrawConfirm(renderer); break;
        case Step::Progress:      DrawProgress(renderer); break;
    }
}

void EnableWizardScreen::DrawSettings(Renderer& renderer) {
    renderer.Clear(20, 24, 28);

    DrawCenteredLabel(renderer, kTitleY, "ENABLE RANDOMIZER", kTitleScale, Palette::Heading);

    // The header band. Decorative and never a focus target (plan 9 D1). It
    // carries only the target now: the seed moved into the pane, and showing
    // it here as well put the same ten digits on screen three times at once.
    // The seed is still stated on FINISH's readiness summary, which is where
    // it matters - immediately before the run is committed.
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

void EnableWizardScreen::DrawRailRow(Renderer& renderer, int y, const char* text,
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

void EnableWizardScreen::DrawRail(Renderer& renderer) {
    bool railHasFocus = (focus_ == Focus::Rail);

    // A bare navigation label, like the six category rows below it. The value
    // lives in the pane, which is directly to its right and would otherwise be
    // repeating it a few hundred pixels away.
    DrawRailRow(renderer, kRailRow0Y, "SEED", railHasFocus && railCursor_ == 0, false);

    // SEED sits outside the category block, separated from it by its own rule
    // - the same treatment FINISH gets below the categories.
    renderer.FillRect(kRailX, kRailRuleY, kRailW, kRuleThickness,
                      kRuleColor.r, kRuleColor.g, kRuleColor.b);

    for (int i = 0; i < kCategoryCount; i++) {
        SettingCategory category = (SettingCategory)i;
        DrawRailRow(renderer, kRailFirstY + i * kRailPitch, CategoryLabel(category),
                    railHasFocus && railCursor_ == i + 1,
                    i == lastCategory_);
    }

    renderer.FillRect(kRailX, kRailRule2Y, kRailW, kRuleThickness,
                      kRuleColor.r, kRuleColor.g, kRuleColor.b);

    DrawRailRow(renderer, kFinishY, "FINISH",
                railHasFocus && railCursor_ == kFinishRow, false);
}

void EnableWizardScreen::DrawPane(Renderer& renderer) {
    // The SEED rail row shows its value here rather than beside itself, so the
    // pane is not left showing a category the cursor has moved away from. It
    // is the one rail row whose content is not a settings list; FINISH still
    // keeps the last category on screen, because its summary is in the help
    // pane and blanking the middle would lose the context the summary is about.
    if (focus_ == Focus::Rail && railCursor_ == 0) {
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

void EnableWizardScreen::DrawHelp(Renderer& renderer) {
    const char* title;
    std::vector<std::string> bodyLines;

    if (focus_ == Focus::Rail && railCursor_ == kFinishRow) {
        // The readiness summary (spec §2, B9): what committing would do, said
        // in three lines, with FINISH's own help under it. Each line is wrapped
        // like any other, so a long one folds rather than running off the pane.
        title = "FINISH";
        const char* summary[3];
        std::string seedLine   = std::string("SEED  ") + SeedDisplayText();
        std::string targetLine = std::string("TARGET  ") + run_.bloodborneTitleId;
        std::string countLine  = std::to_string(EnabledToggleCount(run_)) + " OF " +
                                 std::to_string(ToggleCount()) + " SETTINGS ENABLED";
        summary[0] = seedLine.c_str();
        summary[1] = targetLine.c_str();
        summary[2] = countLine.c_str();
        for (int i = 0; i < 3; i++) {
            std::vector<std::string> wrapped = WrapText(renderer, summary[i], kRowScale,
                                                        kHelpW);
            for (size_t j = 0; j < wrapped.size(); j++) bodyLines.push_back(wrapped[j]);
        }
        bodyLines.push_back(std::string()); // one blank line before the prose
        std::vector<std::string> help = WrapText(renderer, FinishHelp(), kRowScale, kHelpW);
        for (size_t j = 0; j < help.size(); j++) bodyLines.push_back(help[j]);
    } else if (railCursor_ == 0) {
        // Focus can only be in the pane while the rail cursor is on a
        // category, so this is the whole of the "rail row 0" case.
        title = "SEED";
        bodyLines = WrapText(renderer, SeedHelp(), kRowScale, kHelpW);
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

void EnableWizardScreen::DrawEditSeed(Renderer& renderer) {
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

// The one list of settings, built from the model. There is no second
// hardcoded copy of it anywhere in this file - that parallel structure, and
// the row constants it had to agree with, is what this feature removed.
std::vector<std::string> EnableWizardScreen::ConfirmItems() const {
    std::vector<std::string> items;
    items.push_back(std::string("SEED   ") + SeedDisplayText());
    for (int i = 0; i < SettingCount(); i++) {
        const SettingDef& def = SettingAt(i);
        items.push_back(std::string(def.label) + "   " + SettingValueText(def, run_));
    }
    return items;
}

void EnableWizardScreen::DrawConfirm(Renderer& renderer) {
    renderer.Clear(20, 24, 28);

    DrawCenteredLabel(renderer, 140, "ENABLE RANDOMIZER", kTitleScale, Palette::Heading);
    DrawCenteredLabel(renderer, 260, "CONFIRM", kItemScale, Palette::Text);

    std::vector<std::string> items = ConfirmItems();
    DrawScrollableList(renderer, kSettingsLayout, items, -1, confirmScroll_, kItemScale,
                       Palette::Text, Palette::Selected);

    // 30px lower than every other footer in the app, and deliberately so:
    // under the atlas's real ink box no hint gap can clear both this list's
    // last row at y=870 and a footer at kScreenHeight - 130. Moving the pair
    // down is what keeps six visible rows instead of five - see
    // ui_scroll_verify.py, which asserts both ends.
    DrawCenteredLabel(renderer, kScreenHeight - 100, "UP DOWN SCROLL", kFooterScale, Palette::Dim);
    DrawCenteredLabel(renderer, kScreenHeight - 50, "OPTIONS COMMIT   O BACK", kFooterScale, Palette::Dim);
}

void EnableWizardScreen::DrawProgress(Renderer& renderer) {
    renderer.Clear(20, 24, 28);

    DrawCenteredLabel(renderer, 120, "ENABLE RANDOMIZER", kTitleScale, Palette::Heading);
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
        DrawCenteredLabel(renderer, kScreenHeight - 80, "O RETURN TO MENU", kFooterScale,
                          Palette::Dim);
    }
}

} // namespace bbr
