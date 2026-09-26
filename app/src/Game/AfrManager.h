// AfrManager.h - status of the AFR (alternate file redirect) tree for one
// title, and the marker inside it that says which world produced it. UI code
// only ever sees these structs, never the raw /data/GoldHEN/AFR path - that
// string exists in exactly one place, this class's .cpp.
#pragma once

#include <cstdint>
#include <string>

namespace bbr {

struct AfrStatus {
    bool rootExists  = false; // /data/GoldHEN/AFR/<titleId> exists
    bool writable    = false; // could create+delete a marker file there
    bool seeded      = false; // .../dvdroot_ps4 subfolder exists (vanilla
                               // game data has been copied in over FTP)

    // Whether .../dvdroot_ps4/.bbrandomizer_manifest exists. Deliberately
    // inside dvdroot_ps4, not somewhere else in the title's AFR folder:
    // activation stages/activates dvdroot_ps4 as one atomic unit (rename
    // staged -> active), so a marker living inside it travels and disappears
    // with the exact content it describes. There is no separate app-level
    // "is it on" flag to fall out of sync.
    bool randomized  = false;
};

// The contents of .bbrandomizer_manifest - plain key=value, like every other
// file this app writes, so it can be read over FTP when a console cannot be
// debugged. This is the ONLY record of which world is active: spec worlds D11
// puts it here so the claim travels with the content it describes.
struct AfrManifest {
    bool        present = false;   // the file was there and parsed
    std::string worldId;
    std::string worldName;
    std::string revision;
    uint32_t    seed = 0;
    std::string titleId;           // the AFR title it was written for
    uint64_t    accountId = 0;     // the account that activated it
    std::string written;           // "YYYY-MM-DD HH:MM:SS", local
};

// Which row of the worlds plan's section 4.2 table the disk is in. There is no
// stored active flag anywhere: this is derived, every time, from what is
// actually on disk.
enum class AfrActiveState {
    // No world folder for this account. The randomizer has never been used by
    // this player, so no row shows ACTIVE and first-run capture runs.
    FirstRun,

    // No randomizer files, and no manifest claiming otherwise - the game runs
    // as it shipped, which is exactly what the Vanilla world is.
    Vanilla,

    // A manifest naming a world this account has. That world is active.
    World,

    // Randomizer files this app did not write, or a manifest naming a world
    // that is not there. No row shows ACTIVE, and the fix offered is to
    // activate something. Every console that ran a build before this feature
    // is in this state: the old Enable wizard wrote no manifest.
    Unmanaged,
};

struct AfrActiveWorld {
    AfrActiveState state = AfrActiveState::FirstRun;
    std::string    worldId;     // set only when state == World
    std::string    worldName;
    std::string    revision;
    std::string    reason;      // one sentence, for the details pane
};

class AfrManager {
public:
    // titleId comes from the BLOODBORNE TITLE ID setting and is used exactly
    // as the player entered it (worlds plan P26) - this class doesn't know or
    // care which titles are "real Bloodborne", just paths.
    static AfrStatus Check(const std::string& titleId);

    // The marker inside the live dvdroot_ps4 for `titleId`. Reading a file
    // that is not there is not an error: `out.present` stays false.
    static bool ReadManifest(const std::string& titleId, AfrManifest& out);
    static bool WriteManifest(const std::string& titleId, const AfrManifest& manifest);

    // The same format, without touching the disk - so the text can be built
    // and checked without an AFR tree, and so a staged tree can be stamped by
    // whatever is writing it.
    static std::string FormatManifest(const AfrManifest& manifest);
    static bool ParseManifest(const std::string& text, AfrManifest& out);

    // The section 4.2 derivation, as a pure function of four facts: what the
    // AFR tree looks like, what its manifest says, whether this account has a
    // worlds folder at all, and whether the world the manifest names still
    // exists. The last two come from Randomizer/WorldStore - passed in rather
    // than read here, so that nothing under Game/ has to know how worlds are
    // stored and this stays testable one row at a time.
    static AfrActiveWorld DeriveActive(const AfrStatus& status,
                                       const AfrManifest& manifest,
                                       bool accountDirExists,
                                       bool worldIsKnown);

    // --- the activation swap (worlds plan section 4.3, phases 4 and 5) -----
    //
    // A world's tree is GENERATED somewhere else and then moved into place by
    // rename, rather than written over the live one: a run that dies partway
    // through writing 6 GB directly into dvdroot_ps4 would leave the game
    // running half of one world and half of another, which is the silently
    // incoherent state the whole transaction exists to prevent.

    // Where a world's tree is built before it is swapped in. Handed to
    // EnemyRandomizerJob as its output directory, so it is a full path - but
    // it is still built HERE, because the AFR root string lives in this file
    // and nowhere else.
    static std::string StagingDvdroot(const std::string& titleId);

    // Stamps a tree that is not the live one - the staged tree, before it is
    // swapped in. `dvdrootPath` is what StagingDvdroot returned.
    static bool WriteManifestAt(const std::string& dvdrootPath,
                                const AfrManifest& manifest);

    // Phase 5. dvdroot_ps4 -> dvdroot_ps4.old, then (when `useStaging`)
    // dvdroot_ps4.staging -> dvdroot_ps4, then the .old tree is removed.
    // Vanilla passes useStaging = false and so omits the second rename,
    // which is what "the randomizer's files are removed" means (B8).
    //
    // Renames first and deletes last: at every instant between the two there
    // is a complete tree under one of the two names, so an interruption is
    // recoverable by RollBackSwap below rather than by regenerating.
    static bool Swap(const std::string& titleId, bool useStaging, std::string& error);

    // Reconciliation, for a journal that stopped at or before phase 4: the
    // staged tree is work in progress that nothing has depended on yet.
    static bool StagingExists(const std::string& titleId);
    static void RemoveStaging(const std::string& titleId);

    // Reconciliation, for a journal that stopped in phase 5. If dvdroot_ps4.old
    // is there and dvdroot_ps4 is not, the first rename happened and the
    // second did not: put it back. Otherwise the swap completed and there is
    // nothing to undo. `outRestored` says which of the two it was.
    static bool RollBackSwap(const std::string& titleId, bool& outRestored);
};

} // namespace bbr
