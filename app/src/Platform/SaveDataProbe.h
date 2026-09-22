// SaveDataProbe.h - TEMPORARY DIAGNOSTIC.
//
// THE DELETE LIST, for when this scaffolding goes (worlds plan milestone 6):
// this file, SaveDataProbe.cpp, UI/SaveProbeScreen.{h,cpp},
// ScreenId::SaveDataProbe and the "SAVE DATA PROBE (TEST)" main-menu row.
// -lSceSaveData STAYS in the Makefile - Platform/SaveData takes it over.
//
// THE QUESTION: can this app get a readable handle on Bloodborne's save data?
//
// Nothing in the randomizer touches save data today - feature
// randomizer-settings-ui milestone 1 removed the simulated backup/replace
// handling outright, precisely so the real thing could be designed from a clean
// slate. This probe is the first step of that design: find out what is actually
// reachable before specifying anything.
//
// HOW APOLLO SAVE TOOL DOES IT, and what this mirrors: PS4 save data is not
// readable as a plain directory. It lives encrypted under the user's home, and
// the supported way in is the official SDK path -
//
//     sceUserServiceGetInitialUser   which user's saves
//     sceSaveDataInitialize3         start the service
//     sceSaveDataDirNameSearch       enumerate a title's save directories
//     sceSaveDataMount (RDONLY)      mount one, yielding /savedataN
//     ordinary file I/O on that path
//     sceSaveDataUmount
//
// The mount is what does the work: the returned path is a decrypted view that
// ordinary sceKernelOpen/Getdents can walk. That is ordinary homebrew
// filesystem access - no privilege escalation, no sandbox escape, no kernel
// work - and it is the same door Apollo goes through to see a save at all.
// (Apollo does more on top: re-encrypting for transfer between consoles. That
// is a different problem and is not what this probe is for.)
//
// THE RISK THIS CARRIES: libSceSaveData is a library this app has never linked.
// docs/ps4-homebrew-findings.md section 7 records that each new library is a
// chance to repeat the sceAppInstUtilInitialize failure - a bad NID that makes
// the runtime loader reject the whole binary BEFORE main() runs, with no output
// anywhere. If the app stops launching after this change, that is the cause,
// and the fix is to drop -lSceSaveData from the Makefile and rebuild.
#pragma once

#include <string>
#include <vector>

namespace bbr {

// Runs the sequence above, appending one line per step - including failures,
// with their error codes - to `outLines`. Every line also goes to live.log, so
// the result survives a crash mid-probe.
//
// `configuredTitleId` is what Setup Defaults is pointing at. It is searched,
// but it is NOT trusted to be the right answer: the first run of this probe
// searched only that one, found nothing, and looked like a failure of the
// mechanism when in fact the mechanism was fine and the console's saves simply
// belonged to a different SKU. So every officially-issued Bloodborne title is
// swept as well, and each is reported separately.
//
// Save data is keyed strictly by title ID. There is no redirection between one
// title's saves and another's - two title IDs means two unrelated containers,
// even for the same game.
//
// Reports what it finds rather than judging it: a mount that fails is a result,
// not an error to swallow. Never throws, never exits, and unmounts whatever it
// mounted before returning.
// `doBackup` false: mount read-only and report. True: additionally copy the
// save out to /data/bbrandomizer/SaveBackups/<title>_<dir>_<timestamp>/.
//
// The mount stays READ-ONLY either way. A backup only ever writes into this
// app's own folder, never into save data - restoring is a separate, riskier
// operation and is deliberately not implemented here.
void ProbeSaveData(const std::string& configuredTitleId,
                   std::vector<std::string>& outLines,
                   bool doBackup);

// THE SECOND QUESTION, and the one the worlds feature is gated on: does
// RESTORING work, does DELETING work, who is the player, and can free space be
// measured? ProbeSaveData() above answers only "can we read it".
//
// This is worlds plan milestone 0. Steps, in the plan's order:
//
//   1  the compile-time scratch save-directory name, and a guard that aborts
//      every save-data write path if that name ever equals a directory the
//      search actually returned
//   2  identity: GetInitialUser / GetForegroundUser / GetLoginUserIdList /
//      GetUserName / GetNpAccountId, printed beside the ACCOUNT_ID read out of
//      the live save's own param.sfo
//   3  capacity: statvfs("/data") and statvfs("/user"), every field raw
//   4  rename: sceKernelRename a scratch directory under /data/bbrandomizer
//   5  restore: mount the SCRATCH directory RDWR|CREATE2 and copy a local
//      backup into it, reporting every file's write result
//   6  verify: re-mount the scratch directory RDONLY and walk it
//   7  delete: sceSaveDataDelete the scratch directory and re-search
//   8  steps 5 and 6 timed in milliseconds
//   9  mount the EXISTING live container RDWR, write nothing, unmount
//
// WHAT KEEPS A REAL PLAYTHROUGH SAFE. Every write - the CREATE2 mount, the
// copy, the delete - names kScratchDirName, a compile-time constant, and each
// of those call sites re-checks the guard immediately before firing. Nothing
// here touches a privileged path.
//
// STEP 9 IS THE ONE EXCEPTION, and it is deliberate. Step 5 established that
// CREATE2 is refused: this app cannot bring a save container into existence,
// because that is done by writing a PFS image and a sealedkey under /user/home
// and registering the result in /system_data's savedata.db, and neither path
// exists in this sandbox. Creating and writing are separate permissions, so
// whether an ALREADY EXISTING container accepts RDWR is a different question -
// and answering it decides whether restore is possible at all.
//
// The only existing container is the live save, so step 9 mounts it RDWR. It
// passes no CREATE2, opens no file, walks nothing, and unmounts immediately on
// both paths. The residual risk is that mounting read-write and unmounting may
// rewrite container metadata even with no file writes; that cannot be excluded
// from outside, which is why the step runs last and why a verified backup is a
// precondition for running the write probe at all.
//
// Steps 5-7 need a local backup to restore FROM: press SQUARE first to take
// one. The newest backup whose name begins with the live save's title id is
// the one used, and which one was chosen is printed.
//
// Same reporting contract as above: a failure is a result, printed with its
// error code, not swallowed. Never throws, and unmounts whatever it mounted.
void ProbeSaveDataWrite(const std::string& configuredTitleId,
                        std::vector<std::string>& outLines);

// DESTRUCTIVE. Unlinks every userdata* file inside the live save's mount and
// leaves everything else - backup*, sce_sys - alone. Its own entry point on its
// own button so it cannot fire as a side effect of the write probe.
//
// The question: Bloodborne keeps characters in the userdata files, so does
// removing them leave the game with no characters, the way a fresh container
// would? If yes, START FRESH needs no sceSaveDataDelete and no blank-save
// template - it is a delete of eleven files through a mount we already know we
// can write to.
//
// TWO THINGS THIS ALSO TESTS, both previously unknown:
//
//   - Whether sceKernelUnlink is permitted inside a save mount at all. Writing
//     to an existing file is proven; removing one is not.
//   - Whether a later restore can CREATE files. Every write in step 11 was an
//     overwrite (CREATED 0). After this, the userdata files are gone, so
//     restoring them is a creation - and if creation is refused, the only route
//     back is to let the game rebuild the container.
//
// `prefix` selects which root-level files go - "userdata" or "backup". Only
// files at the mount root are considered, so sce_sys is never touched whatever
// is passed.
//
// The two are separate buttons so they can be tested independently. Deleting
// userdata alone answers whether the game rebuilds from its own backups;
// deleting backups as well answers what it does with nothing to fall back on.
void ProbeDeleteByPrefix(const std::string& configuredTitleId,
                         const std::string& prefix,
                         std::vector<std::string>& outLines);

} // namespace bbr
