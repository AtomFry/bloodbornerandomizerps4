// PermaDarkness.h - C++ port of the reference tool's PermaDarknessFunction
// (MainWindow.xaml.cs). Opens event/common.emevd.dcx, finds event ID 6548972,
// and pokes 4 bytes in its first instruction's argument data. This is the
// only content change the reference tool ever makes outside
// map/mapstudio/*.msb.dcx in the "Randomize Enemies only" scenario (confirmed
// by a full audit of every function DoSomething() calls).
//
// WHICH BYTES MEAN WHAT (decoded from the vanilla tree - see
// docs/plans/mergo-darkness.md §2 for the full evidence):
//
//   Event 6548972 is a two-instruction loop referencing SpEffect 5630 on the
//   player (character 10000). 5630 carries no stats at all - it is a marker,
//   and Nightmare of Mensis (m26) reads it to choose a lighting preset and to
//   hand Mergo's Wet Nurse a matching state, 5631. Poking the instruction to
//   reference SpEffect 99999, which does not exist in SpEffectParam, is what
//   the reference tool does by default.
//
//   WHAT THE GAME ACTUALLY DOES WITH THEM IS NOT THIS. On console, the poked
//   file gives a normal lit game and the vanilla file gives a permanently
//   dark one from the first spawn - not an m26-local effect, and not a
//   "vanilla bytes mean vanilla behaviour" situation. The decode above
//   explains the file; it does not explain the game. See the plan's §2.7.
//
// CAREFUL WITH THE POLARITY, AND DO NOT REASON IT OUT FROM THE BYTES.
//
//   permaDarknessOn == true   -> writes the game's own shipped values
//                                -> a permanently DARK world
//   permaDarknessOn == false  -> writes the poke (SpEffect 99999)
//                                -> the normal, LIT game
//
// Yes, that means the values the game ships with are the abnormal ones here.
// It is measured on console in both directions and it contradicts what the
// decode above would suggest, so treat that decode as an account of which
// bytes mean what, NOT of what the game does with them. Why the shipped bytes
// behave differently under an AFR overlay than on an unmodified console is
// unexplained; see docs/plans/mergo-darkness.md section 2.7.
//
// BOTH branches are live. EnemyRandomizer.cpp's StepEmevd calls this on every
// run and passes the setting straight through, so the output file always holds
// one of these two known patterns rather than inheriting whatever the mirrored
// VanillaSource happened to contain. See that call site for why that matters.
//
// This is a minimal, targeted EMEVD reader - not a general parser. It reads
// just enough of the fixed Bloodborne-format EMEVD header (16-byte prefix +
// sixteen 8-byte varints, all little-endian - confirmed against
// SoulsFormats/Formats/EMEVD/EMEVD.cs's own Read()) to locate one event's
// first instruction's argument bytes, pokes 4 of them in place, and leaves
// everything else in the file untouched - the same "opaque blob, poke only
// what's needed" approach used for MSBB. Verified byte-for-byte against the
// real vanilla common.emevd.dcx before being trusted (see project notes).
#pragma once

#include <cstdint>
#include <vector>

namespace bbr {

// Mutates `plain` (an already-DCX-decompressed common.emevd.dcx payload) in
// place. Returns false if the file isn't the expected Bloodborne-format
// EMEVD or event 6548972 isn't found (plain is left untouched in that case).
//
// permaDarknessOn selects which of the two patterns to write: true = the
// shipped values = DARK, false = the poke = LIT. See the polarity note above;
// it is not the way round it looks.
bool ApplyPermaDarkness(std::vector<uint8_t>& plain, bool permaDarknessOn);

} // namespace bbr
