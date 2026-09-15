# Bloodborne Enemy Randomizer — working notes for Claude

Read this before doing anything in this repo. It is the standing context: what
the project is, how it builds, how things get proven, and the traps that have
already cost real time.

---

## 1. What this repo holds

```
app/                    the PS4 randomizer — the real project, where work happens
  src/                    Platform, UI, Game, Randomizer, Msb, Param
  tools/                  Python verifiers, table generators, tools/data/ fixtures
docs/                   specs, findings, and plans/ — see §8
data/                   GITIGNORED, game trees — see §9
reference/              the Windows tool, read-only behavioural reference
  BloodborneRandomizer.sln
  Randomizer/             the WPF app (all Bloodborne logic)
  SoulsFormats/           FromSoftware file-format library
```

Two codebases with very different roles.

**`app/` — the real project.** A native C++ homebrew application (OpenOrbis
toolchain, SDL2) that runs on the user's already-jailbroken (GoldHEN) PS4 and
randomizes Bloodborne directly on the console. This is where essentially all
active work happens.

**`reference/` — the Windows reference tool.** The original C# WPF randomizer.
It is the **behavioural reference, not a thing being improved** — nothing in
here should be modified. When the PS4 port and this tool disagree, the default
answer is that the port is wrong (see §7). Read it to learn what the port should
do.

> Two things here are load-bearing, not just documentation. `RootNamespace` is
> still `MSB_Test`; that is internal to C# and deliberately left alone. And
> `app/tools/starting_weapons_verify.py` parses
> `reference/Randomizer/MainWindowComponents/RandomizeFunctions.cs` directly, so
> that path cannot move.

This is **homebrew development, not jailbreak or exploit work.** The console is
already modded; the app needs nothing beyond ordinary homebrew filesystem
access. If something appears to require privilege escalation or a sandbox
escape, stop and explain the dependency rather than working around it. Standing
rule: don't widen scope into jailbreak, exploit, or kernel territory.

---

## 2. Build

```bash
cd app
make
```

Produces `IV0000-BBRD00001_00-BBRANDOMIZERMAIN.pkg`. Requires `OO_PS4_TOOLCHAIN`
pointing at the OpenOrbis install; the Makefile errors out if it is unset.

Non-obvious build facts, all hard-won and all documented in the Makefile:

- **Compile with the SDK's clang, link with lld 18.1.8 specifically**
  (`C:/Users/Fry/LLVM18-portable/bin`). Later lld versions emit an `.oelf` the
  console's loader rejects at launch. Confirmed on hardware.
- **`gnu++17`, not `c++17`.** libc++'s `<atomic>` needs POSIX declarations that
  strict `-std=c++17` hides.
- **After any class or struct layout change, do a full clean rebuild**
  (`rm -rf src/x64`). A stale partial rebuild once produced a real
  heap-corruption SIGSEGV on hardware. This is not theoretical.

Install by copying the `.pkg` to `/data/pkg/` over FTP and installing via
Settings → Debug Settings → Package Installer. Live log, one line per event,
crash-resilient: `/data/bbrandomizer/live.log`.

---

## 3. How things get proven

**There is no host C++ compiler and no emulator.** No MSVC, no working MinGW —
only the OpenOrbis cross-toolchain. So there are no C++ unit tests, and nothing
in this environment can run the application. That shapes everything below.

Verification has exactly three layers:

**(a) The cross-compile.** A full clean build catches type, syntax, and link
errors. That is all it catches.

**(b) Python mirrors — this project's substitute for unit tests.** Scripts in
`app/tools/*_verify.py` re-implement the algorithm's rules in
Python and check them against real game data. Every one has a `selftest`
subcommand:

```bash
python tools/pool_verify.py selftest ../data/vanilla/dvdroot_ps4
```

`ls app/tools/*_verify.py` lists the ones that exist. Reuse their parsing rather
than duplicating it — `treasure_verify` reuses `boss_verify`'s MSBB reader,
which is the pattern to follow.

**State the limitation honestly whenever one of these is cited as evidence: a
Python mirror pins the rules, not the C++ implementation of them.** The two can
drift. That is a known, accepted weakness of this setup, not something to paper
over.

**(c) The hardware test — the user, holding the PS4.** This is the only thing
that proves a feature works, and Claude can never do it. Building cleanly and
passing selftests means "ready for hardware test", never "done". Say it that way.

`data/runs/` holds saved randomized output trees from past hardware runs, for
diffing. `data/vanilla/dvdroot_ps4` is the trusted vanilla baseline.

**Methodology rule that has already been got wrong once: comparing two
randomized runs is not enough.** Without vanilla as a third input you cannot
distinguish "frozen by exclusion" from "skipped by zone chance in both runs"
from "coincidentally redrew the same model". A first pass that skipped this
reported 221 phantom anomalies.

---

## 4. Working agreement

**Pace in small, explicit milestones.** Explain what the milestone does and the
PS4/OpenOrbis concept behind it, make the smallest change that reaches it, build
the `.pkg`, and stop. Wait for the hardware test and explicit approval before
starting the next substantial piece. Do not chain milestones. Do not refactor
ahead of the ask.

**This is a learning project, not just a delivery.** Explaining the mechanism —
sandboxing, PKG structure, AFR, the map format — is part of the work, not
padding around it.

**Never create commits or branches without asking.** Leave the working tree and
let the user decide.

---

## 5. Traps that have already bitten

**Heredoc backslash mangling — hit four separate times.** Writing C/C++ source
through a `python - <<'PYEOF'` heredoc in the Bash tool eats one level of
backslash escaping before Python sees it. This has silently produced a real NUL
byte in `EnableWizardScreen.cpp` (grep then reported the file as binary) and
silently dropped newline escapes from `printf` format strings in
`RandomizerDefaultsStore.cpp` three times.

> **Rule: never write a C string or char literal containing a backslash escape
> through a heredoc.** Use the Edit tool. If a bulk scripted edit is genuinely
> unavoidable, operate on bytes and build the escape from `bytes([92])` rather
> than typing a backslash anywhere in the script.

**The 8x8 font is uppercase A–Z and digits only.** No punctuation, no arrows,
no lowercase. `FindGlyph` renders anything unknown as **blank**, silently. This
is why scroll hints read `MORE BELOW` rather than using arrows, and why any
progress line containing a comma quietly loses it. Check every user-facing
string against this before adding it.

**A `.bak` file in a "vanilla" tree means the live files are modified.** Any
folder the reference Windows tool has ever run against has inverted polarity:
the live file is that run's randomized *output*, and the `.bak` sibling holds
the real vanilla bytes. This contaminated the vanilla source once and produced a
day of chasing a code bug that did not exist.

**Don't call a value "corrupt" from an incomplete check.** The 900,000,000+
range `NPCParamID`s in the reference tool's output are real hand-curated scaling
data (`NPCScalingFile.txt`), not garbage. They were called corruption after
checking map placements but never checking whether the values existed as valid
param rows.

**`OrbisKernelStat`'s `st_size` is unreliable on this SDK.** It returned 88 for a
genuine 44KB file on real hardware, which is why `ReadWholeFile` never sizes off
it. The header's layout does not necessarily match the real kernel ABI
field-for-field — treat any `OrbisKernelStat` field this project has not already
hardware-validated with suspicion. `st_mode` has always been correct.

**Byte decoding never proves game behaviour.** Knowing what a flag writes is not
knowing what it does. Hardware-test before renaming a setting or inverting its
polarity.

---

## 6. Domain orientation

**AFR is a file-redirect overlay.** Files under
`/data/GoldHEN/AFR/<titleId>/dvdroot_ps4/...` shadow the matching installed game
file; everything else loads from the real install. That describes the
*mechanism* correctly.

**But a sparse AFR tree is not sufficient in practice.** The reference tool's
README requires `chr`, `event`, `map`, `param`, `script`, and `sfx` all present,
and hardware testing confirmed it. So the randomizer mirrors all six folders
from the vanilla source into AFR *before* overwriting the files it actually
changes. Defer to the README and hardware testing over any theoretical model of
how AFR "should" behave.

**There is no vanilla game data reachable by the app.** Vanilla files live
inside the game's encrypted install, unreachable from homebrew. The user FTPs a
real vanilla `dvdroot_ps4` onto the console once, to
`/data/bbrandomizer/VanillaSource/dvdroot_ps4`. The randomizer reads from there
and writes to AFR. **Never bundle vanilla data into the `.pkg`** — that is about
package size and build iteration speed, and it remains correctly avoided.

**Layering** (the user's own architecture): `Application` → `Platform`
(SDL2/Input/Renderer), `Game` (GameInfo/AfrManager), `UI`
(Screen/ScreenManager/Controls), `Randomizer` (Settings/Engine/Progress), `Msb`
(map format), `Param`. **UI never touches raw AFR paths; the randomizer core
never touches SDL2.** Keep it that way.

**MSB map entries are opaque byte blobs on purpose.** Every offset inside an
entry is relative to that entry's own start, so blobs relocate verbatim. Only
`Part.Enemy` (poking ThinkParamID/NPCParamID/modelIndex at fixed offsets) and
`Model` get any field-level access. Validated by a Python re-implementation
achieving a byte-exact round trip before any C++ was trusted.

**DCX writes use stored (uncompressed) DEFLATE blocks.** The toolchain's zlib is
decompress-only. Stored blocks are spec-legal and any compliant inflate reads
them. Decompression uses vendored `puff.c`.

---

## 7. Standing design preferences

**Fidelity to the reference tool wins over improvement.** Several "obvious
fixes" turned out to be deviations that made things worse — `EnemyExclusionListExtra`
froze 119 placements before it was removed. The port now matches the reference's
exclusions exactly; re-add individual entries from
`docs/enemy-exclusion-history.md` with a stated reason rather than restoring the
list. A similar parked idea (applying the `ThinkParamID <= 1` test
symmetrically) is deliberately left unimplemented for the same reason.

**Chalice dungeons are out of scope**, by decision. Three settings are affected.
Don't reopen it.

**Reproduce the reference's quirks deliberately, and say so.** `m21_00_00_00` is
never randomized (a free in-game control group) and `m21_01_00_00` is listed
twice; both are reproduced on purpose and encoded as invariants in
`treasure_verify.py`.

---

## 8. Documents

**`docs/` — reference and status:**

- `randomization-feature-spec.md` — the master backlog and status board. ~20
  settings remain, with a suggested implementation order in §11. **This is the
  authority on what is done**; if a plan and this table disagree, the table is
  what was checked most recently.
- `ps4-homebrew-findings.md` — hardware-confirmed platform knowledge: sandbox
  and filesystem, toolchain pins, packaging, SDL2, error decoding, and dead ends
  not worth retrying. Check here first for anything platform-level; several
  entries contradict what the SDK headers imply.
- `windows-randomizer-technical-review.md` — the reference tool's architecture,
  algorithms, and bug catalogue.
- `deferred-ideas.md` — ideas recorded but explicitly **not** authorized. Check
  before proposing a feature; it may already have been deferred on purpose.
- `enemy-exclusion-history.md` — the removed `EnemyExclusionListExtra`, kept so
  entries can be re-added with justification rather than restored wholesale.
- `user-guide.md` — user-facing; covers what ships today only.

**`docs/plans/` — one document per feature increment.** Flat, descriptive names,
no suffix. `workshop-tools.md` and `pickers.md` are the style reference.
`ai-dev-process.md` is the plan for the development pipeline itself (§10).

**House style for plans:** plain-language "what this does" section first, then
numbered technical sections, measured numbers rather than estimates, and an
explicit **deviations from the plan as written** section once implemented.

**Status lines are load-bearing — keep them true.** A stale one actively
misleads: a plan that still says "no code written" for shipped work will send the
next reader off to rebuild it, and four of them said exactly that. When a feature
lands, update its plan's status line *and* the spec table row in the same pass.

"Implemented, builds clean, awaiting hardware test" is a real and common status —
use it rather than rounding up to done.

Also `app/UI_BLUEPRINT.md`.

---

## 9. Git

`app/`, `docs/`, and `CLAUDE.md` have been tracked since `785cefe` — the PS4
port is committed and has history behind it.

**`/data/` is gitignored and must stay that way** — ~1.7 GB of game trees that
are neither source nor ours to redistribute. `app/.gitignore` separately covers
build output (`src/x64/`, `*.pkg`, `eboot.bin`, `pkg.gp4`, `*.log`).

---

## 10. The development process

A formalized pipeline — spec → plan → adversarial plan review → implement →
code review → verify → hardware test → document — is being built out in stages.
The vision and build order are in
[docs/plans/ai-dev-process.md](docs/plans/ai-dev-process.md).

**Status: stage A (this file). No commands, agents, or skills exist yet.** Until
they do, this section describes intent, not available tooling. Update it as each
stage lands.
