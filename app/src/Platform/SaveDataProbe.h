// SaveDataProbe.h - TEMPORARY DIAGNOSTIC. Delete this file, SaveDataProbe.cpp,
// UI/SaveProbeScreen.{h,cpp}, ScreenId::SaveDataProbe, the menu row and the
// -lSceSaveData in the Makefile when the question below has been answered.
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

} // namespace bbr
