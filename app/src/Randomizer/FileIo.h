// FileIo.h - plain whole-file read/write and recursive mkdir, built on the
// same raw sceKernel* calls + BSD flag values already proven correct on
// hardware in Game/AfrManager.cpp (full CRUD against AFR with no sandbox
// escape needed - docs/ps4-homebrew-findings.md §1).
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace bbr {

bool ReadWholeFile(const std::string& path, std::vector<uint8_t>& out);
bool WriteWholeFile(const std::string& path, const std::vector<uint8_t>& data);

// Creates every path component that doesn't already exist (like `mkdir -p`).
bool MakeDirsRecursive(const std::string& path);

// Copies every file and subdirectory under srcDir into dstDir, verbatim,
// creating directories as needed. Returns false if srcDir doesn't exist or
// any individual file fails to copy (best-effort otherwise - continues
// past a single bad entry rather than aborting the whole tree).
bool CopyDirRecursive(const std::string& srcDir, const std::string& dstDir);

} // namespace bbr
