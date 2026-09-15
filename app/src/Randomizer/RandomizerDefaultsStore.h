// RandomizerDefaultsStore.h - loads/saves RandomizerDefaults to
// /data/bbrandomizer/defaults.cfg. Plain "key=value" text, one line per
// field, same FTP-inspectable style as this project's other on-disk files
// (live.log). Not a generic settings framework - just enough to persist
// the handful of fields RandomizerDefaults actually has right now.
#pragma once

#include "RandomizerDefaults.h"

namespace bbr {

// Missing file, or a missing/unparseable key within it, falls back to
// RandomizerDefaults' own in-struct defaults - this is what makes a fresh
// install come up with "backup existing save = YES" per the spec.
RandomizerDefaults LoadRandomizerDefaults();

// Creates /data/bbrandomizer/ if needed and overwrites the file.
void SaveRandomizerDefaults(const RandomizerDefaults& defaults);

} // namespace bbr
