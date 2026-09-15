# Seed Entry — Windows Behavior and Proposed PS4 Design

**Status: DONE — implemented and hardware-confirmed** (spec row 6).
Sections 1-5 are the agreed plan; section 6 records what was built.

Resolves deferred UI item #2 (`docs/deferred-ideas.md`). This is infrastructure,
not a new randomization feature: it makes runs reproducible and shareable, and it
makes the determinism steps in our own boss and treasure specs (V1/V7) performable
for the first time — they were written as if they already were, which was my error.

---

## 1. Determinism audit — does a seed even reproduce a run today?

Worth establishing before building a UI that promises it. **Audited, and the answer
is yes.** Evidence:

| Risk | Finding |
|---|---|
| More than one RNG stream | **One.** A single `std::mt19937 rng` (`EnemyRandomizer.cpp:251`), constructed from the seed at `:204` and passed by reference to every pass — boss, treasure, drops, starting weapons. |
| Another entropy source | **None.** `time(nullptr)` appears exactly once in the whole tree, at `EnableWizardScreen.cpp:306`, which is the seed itself. No `random_device`, no `srand`, no `chrono`. |
| Unordered container iteration | **One `unordered_map`** (`EnemyRandomizer.cpp:547`, `enemyModelIndex`) — used only for `find()` at `:607`, never iterated. Everything else is `std::set`, which is ordered. |
| Map processing order | Fixed tables throughout: `kBaseMaps`, `BossMapOrder()`, `TreasureMapOrder()`. No directory scan feeds the randomizer — `sceKernelGetdents` is used only by `GameInfo` title detection and the file mirror, neither of which consumes RNG. |

**Two caveats that must be documented in the UI's wake, or they'll be reported as
bugs:**

- **A seed only reproduces a run for the same set of toggles.** Turning a feature
  on or off changes how many draws happen and in what order, so the same seed with
  different settings is a different run. This is inherent to a single shared
  stream and is also how the reference behaves.
- **Seeds are not portable between this tool and the Windows one.** We use
  `std::mt19937`; the reference uses .NET `System.Random`. Same seed, different
  sequence. Bit-exact RNG matching was deferred long ago and stays deferred.

## 2. What the Windows tool does

Added in commit 257c819. A text box, and `UIComponents.cs:149-155`:

```csharp
var seedString = SeedTextbox.Text.Length >= 10 ? SeedTextbox.Text.Substring(0, 10) : SeedTextbox.Text;
if (!int.TryParse(seedString, out seed))
{
    seed = new Random().Next();
    SeedTextbox.Text = seed.ToString();
}
```

- Numbers only, first 10 characters.
- Unparseable or blank → a random seed is generated **and written back into the
  box**, so the user can always see what was used.
- The seed is also shown on completion: `MessageBox.Show($"Finished.  Seed: {seed}.")`.
- It seeds **two** streams from the same value, `universalRand` and `keyitemRand`
  (`StartFunctions.cs:34,38`). We have one, because key-item randomization isn't
  implemented here.

## 3. PS4 design (agreed)

**There is no "random vs fixed" mode.** The seed is always a concrete, visible
ten-digit value that the user can re-roll or edit. That is simpler than the
mode flag I first proposed and it removes the reference's worst behavior — a
silent fallback that quietly substitutes a value you never see until afterwards.

One new row in the Enable Randomizer wizard, above the randomizer toggles because
it applies to all of them:

```
BACKUP EXISTING SAVE   NO
REPLACE SAVE           NEW SAVE DATA
SEED                   0001234567          <- new
RANDOMIZE ENEMIES      YES
...
```

- **Left/right** rolls a **new random seed**, and can be pressed repeatedly —
  each press produces a different value. This follows the existing convention
  that left/right changes the highlighted row's value.
- **X** opens a digit editor, exactly the pattern the title-ID row already uses:
  10 positions, left/right moves the cursor, up/down cycles 0-9. There is no
  keyboard on this console; the editor is proven and `CycleDigit` already exists.
- The value is **zero-padded to ten digits** on display, so what the row shows and
  what the editor shows are the same thing.

The wizard list goes from 9 rows to 10. **That needs no layout work** — scrolling
landed first, and this is the first thing to benefit from it.

### Persistence

`RandomizerDefaults` gains `lastSeed` (`last_seed` in `defaults.cfg`), and the
**wizard writes it back at commit**, so the next run opens showing the seed the
last run used. Reproducing the previous run becomes: open the wizard, commit.

`0` means "never set" — on first ever run the wizard rolls a value immediately, so
the field is never blank.

This makes `EnableWizardScreen` the first screen other than Setup Defaults to
write `defaults.cfg`. Its constructor takes a non-const `RandomizerDefaults&`
accordingly, and it persists **only** `lastSeed` — the wizard's own toggle states
stay per-run and are still not written back, exactly as today.

The seed is deliberately **not** editable from Setup Defaults. It is a remembered
run artifact, not a preference, and one editor for it is enough.

### Re-rolling

Re-rolling must not use `time(nullptr)` directly: two presses within the same
second would produce the same value and look broken. A `std::mt19937` seeded once
from the clock supplies successive values instead.

### Range

Ours is a `uint32_t`; the reference parses a signed `int`. Ten digits allows values
above `UINT32_MAX`, so entry is **clamped to 4294967295** on accept rather than
wrapping. Ten digits is kept for familiarity with the reference's box.

### Progress line

Today it always reads `GENERATING NEW RANDOMIZER SEED <n>`, which is no longer
accurate for a value the user chose or kept. It becomes `USING SEED <n>`.

## 4. Verification plan

**This feature validates itself, and it retires a test step we could never run.**

Offline, before hardware:
- Extend `tools/ui_scroll_verify.py` for the wizard's 10th row (geometry only).
- No randomization logic changes, so no new property validator is needed. The
  existing boss/treasure/drops/starting-weapon validators must all still pass
  unchanged against a run.

On hardware — **the real acceptance test, and the first true determinism check
this project has ever been able to perform**:

1. Edit the seed to `1234567890`, all features on. Keep the AFR tree.
2. Wipe AFR, reopen the wizard — it must come up showing `1234567890` from
   `defaults.cfg`. Commit again with identical toggles.
3. **The two trees must be byte-identical.** That is the whole claim.
4. Change one toggle, same seed → output must differ (proves the seed isn't
   being ignored, and demonstrates caveat 1 above).
5. Press left/right on the seed row several times → a different value each press,
   including twice in quick succession (guards the `time(nullptr)` trap in §3).
6. Two runs with different rolled seeds → different outputs.

Step 3 is the one that matters. If it fails, the determinism audit in §1 missed a
source and the honest thing is to find it before shipping the UI, not to ship a
seed field that doesn't reproduce.

## 5. Out of scope

Seed sharing/import by any means other than typing; matching the reference's RNG
sequence; a "recently used seeds" list.

---

## 6. As built

**Changed:** `RandomizerDefaults.h` (+`lastSeed`), `RandomizerDefaultsStore.cpp`
(`last_seed`, read with `strtoul` not `atoi` since a seed can exceed `INT_MAX`),
`UI/EnableWizardScreen.h/.cpp`, and one comment in `Application.cpp` that claimed
the wizard only reads defaults.

**Wizard:** new `SEED` row at index 2; `kSaveDataRowCount` 9 -> 10. New
`Step::EditSeed` with `UpdateEditSeed`/`DrawEditSeed`, modelled on the title-ID
editor. Left/right rolls, X edits, entry clamped to 4294967295 on accept.
`StartCommit` now emits `USING SEED <n>` and persists `lastSeed`.

**Verified offline:**
- Clean rebuild from a wiped `src/x64`, no warnings.
- `tools/ui_scroll_verify.py` passes with the wizard at 10 rows (6 visible),
  so the new row is reachable and the hints still clear heading and footer.
- Determinism audit in section 1 - the property the feature depends on.

**Two implementation traps worth recording:**

1. **Re-rolling must not call `time(nullptr)`.** Two presses inside the same
   second would return the same value and read as a broken button. A single
   `std::mt19937` seeded once from the clock supplies successive values.
2. **`<random>` pulls `<cmath>`**, so `#include <cstdlib>` has to come first in
   this file or the toolchain's libc++ fails on `::abs` - the same constraint
   already documented in `BossRandomizer.cpp`.

**Not done, deliberately:** the seed is not shown or editable in Setup Defaults.
It is a remembered run artifact rather than a preference, and one editor is enough.
