// RandomizerDefaultsStore.h - loads/saves RandomizerDefaults to
// /data/bbrandomizer/defaults.cfg, and owns the "key=value" serializer that
// file shares with a world's revision files. Plain text, one line per field,
// same FTP-inspectable style as this project's other on-disk files
// (live.log). Not a generic settings framework - just enough to persist
// the handful of fields RandomizerDefaults actually has right now.
//
// TWO FILES, ONE FORMAT. defaults.cfg is what a NEW world starts from;
// Worlds/acct-<id>/w-NNNN/rev-NNNN.cfg is one world's settings at one point
// in time (worlds plan §4.1). They carry the same settings block, written and
// read by the same three functions below, so a setting can never round-trip
// through one file and not the other. What differs is only what each wraps it
// in: defaults.cfg adds bloodborne_title_id and last_seed, a revision adds
// seed. bloodborne_title_id deliberately does NOT go into a revision - the
// AFR title is a Defaults-tab setting, not a per-world one (spec worlds D25).
#pragma once

#include "RandomizerDefaults.h"

#include <string>

namespace bbr {

// Missing file, or a missing/unparseable key within it, falls back to
// RandomizerDefaults' own in-struct defaults - this is what makes a fresh
// install come up with every randomizer setting off and SAVE DATA on
// KEEP EXISTING.
RandomizerDefaults LoadRandomizerDefaults();

// Creates /data/bbrandomizer/ if needed and overwrites the file.
void SaveRandomizerDefaults(const RandomizerDefaults& defaults);

// --- the shared settings block ---------------------------------------------

// Every randomizer setting as "key=value" lines, in a fixed order, ending in a
// newline. Neither bloodborne_title_id nor any seed is included: each caller
// writes its own wrapper (see this header's opening note).
//
// This doubles as recipe IDENTITY. Two recipes are the same recipe when this
// returns the same text for both, which is what decides whether editing a
// world appends a revision (worlds plan P4) - a comparison that cannot forget
// a field the way a hand-written operator== can.
std::string FormatSettings(const RandomizerDefaults& defaults);

// Applies one "key=value" pair, returning false for a key it does not
// recognise. Callers ignore that: an unknown key is skipped, which is what
// lets a defaults.cfg written by an older or newer build still load, and what
// lets a revision file omit the keys that are not its business.
//
// const char*, not std::string, because the strcmp chain below it is what
// settings_ui_verify.py reads to mirror this store's key set.
bool ApplySettingKey(const char* key, const char* value, RandomizerDefaults& out);

// Applies every "key=value" line in `text`. Absent keys leave `out` alone, so
// `out` arrives already carrying whatever default should stand.
void ApplySettingsText(const std::string& text, RandomizerDefaults& out);

} // namespace bbr
