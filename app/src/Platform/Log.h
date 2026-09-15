// Log.h - crash-resilient logging, split out of Platform.h so anything that
// just wants to log (Input, Screens) doesn't need the whole Platform class.
#pragma once

namespace bbr {

// Opens, writes one line, fsyncs, closes, every call - no buffering to lose
// if the process dies mid-run, which is the normal case while bringing
// something up. Writes to /data/bbrandomizer/live.log (falls back to /data/).
void Log(const char* text);

} // namespace bbr
