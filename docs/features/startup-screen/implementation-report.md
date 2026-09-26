# Implementation Report — Startup Screen — milestone 1

**Status: AWAITING HARDWARE TEST**

**Plan:** `docs/features/startup-screen/plan.md` — milestone 1, "the outcome and
the error state"

**Spec:** `docs/features/startup-screen/spec.md`

**Implemented:** 2026-09-26

---

## 1. What was built

Startup now ends in an explicit outcome instead of two booleans. `StartupProblem`
names the four failures spec §4.1 enumerates; a file-scope table in
`WorldsScreen.cpp` maps each to the one sentence the screen says and the one
prompt it offers; and `Mode::Problem` is the only state that holds, prompts or
draws a prompt. Every quiet path — a swept partial, a discarded staging tree, an
activation correctly finished, a first run that worked — now falls straight
through to the list with no hold. The loading display is still milestone 1's
placeholder: the old technical log, as §7 says it should be.

| # | Planned change | Status | Where |
| - | -------------- | ------ | ----- |
| 1 | `StartupProblem` in `WorldsScreen.h`; the sentence/prompt table and its two lookups in `WorldsScreen.cpp` | done | `app/src/UI/WorldsScreen.h:62-71`; `app/src/UI/WorldsScreen.cpp:163-229` |
| 2 | `Mode` becomes `{ Loading, Problem, Browse, ConfirmDelete }`; `Update` and `Draw` dispatch on four modes | done | `app/src/UI/WorldsScreen.h:110`; `app/src/UI/WorldsScreen.cpp:461-466, 812-817` |
| 3 | Delete `startupBusy_` and `startupWait_`; add `problem_` | done — no reference to either boolean survives anywhere under `app/` | `app/src/UI/WorldsScreen.h:201-205` |
| 4 | Rewrite `UpdateStartup`'s outcome handling | done | `app/src/UI/WorldsScreen.cpp:476-545` |
| 5 | Route the no-signed-in-player branch to the error state, `Say`ing `user_.error` | done — `Refresh()` is no longer called on that branch | `app/src/UI/WorldsScreen.cpp:282-305`, `:318-324` |
| 6 | `UpdateProblem` and `DrawProblem`; rename the two Y constants | done — `DrawLoading` is milestone 2's, see §2 | `app/src/UI/WorldsScreen.cpp:97-98, 548-574, 865-901` |
| 7 | Extend `settings_ui_verify.py` with milestone 1's cases | done — 7 new cases, 89/89 passing (was 82) | `app/tools/settings_ui_verify.py:555-576, 1410-1473` |

### How the four outcomes are reached

* **`JournalNotUnderstood`** — the reconcile finished `!ok` with
  `hadJournal && action == ReconcileAction::Nothing`. Classified from those two
  fields and never from `ReconcileResult::error`, which is empty on exactly this
  path (hazard §3.3 row 3).
* **`ReconcileFailed`** — the reconcile finished `!ok` any other way.
* **`CaptureFailed`** — `FirstRunCaptureResult::ok` is false. Branching on `ok`
  alone, never on `note`, which is set on three *success* paths (hazard row 4).
* **`NoSignedInPlayer`** — the constructor, when `savedata::ResolveUser()` returns
  invalid.

### Invariants checked

* **No file operation added, removed or reordered.** The sweeps, reconcile,
  first-run check, capture, container read and list build run in exactly the
  order they did. The only timing change is that `FirstRunCaptureJob` is
  *constructed* one `Update` earlier than before (§3); its constructor touches no
  files — it assigns `user` and `startUs` into a fresh `State`.
* **Once per launch.** `session_.startupDone` is still the guard and is still set
  only by `FinishStartup()`.
* **Every `Say` still reaches `live.log` with the same text.** The only change to
  what is said is the no-signed-in-player branch, which moved from
  `Log("worlds: " + user_.error)` to `Say(user_.error)` — `Say` logs
  `"worlds: " + line`, so the `live.log` text is byte-identical and the line now
  also appears on the on-screen log, which is what §4.5 asks for.
* **Only the error state consumes input.** `UpdateStartup` takes `input` and
  `(void)`s it explicitly.
* **The log band did not move.** `kProblemTitleY`/`kProblemSentenceY` are the old
  `kStartupTitleY`/`kStartupSubY` values unchanged (120, 200), `kProgressLayout`
  is untouched, and the footer is still `kScreenHeight - 130` / `- 80`.
  `ui_scroll_verify.py` passes unedited.
* **`DrawConfirmDelete` still uses `kStartupTitleScale` and `kStartupSubScale`.**
  Neither constant was deleted.

---

## 2. Deviations from the plan

All four are in the plan's §10.

**D1 — milestone 1 keeps `DrawStartup` and `FinishStartup`.** §5's milestone-1
row says to replace both declarations with `DrawLoading`, `DrawProblem` and
`UpdateProblem`. §7's milestone-1 step 2 says the opposite in as many words —
"keeping `DrawStartup` as the loading drawing for now" — and its step 4 routes
quiet paths through `FinishStartup()`, while §7's milestone-2 steps 3 and 5
delete `FinishStartup` and add `DrawLoading`. §7 was followed, because it is the
milestone ordering, it is internally consistent, and §5's table collapses both
milestones into one row. `DrawProblem` and `UpdateProblem` are in; `DrawLoading`
and the deletion of `FinishStartup` are milestone 2's.

**D2 — `kLoadingWord` is deferred to milestone 2.** §5 lists it among milestone
1's named strings, but nothing in milestone 1 draws it, and an unused file-scope
`const char* const` produces
`warning: unused variable 'kLoadingWord' [-Wunused-const-variable]` under the
build's `-Wall`. It was the only warning the milestone produced; the build was
run both with and without it to confirm that. It arrives with `DrawLoading`,
which is where §6's case for it also lives. Every other string §5 assigns to
milestone 1 is in, including `kWordmark`, which `DrawStartup` now uses in place
of its inline literal.

**D3 — one private helper the plan does not list.** `EnterProblem(StartupProblem)`
does exactly §4.4's three statements. There are four call sites.

**D4 — the first-run capture is set up on the reconcile's failing paths too.**
See §3.

---

## 3. Decisions the plan left open

**Which stage `UpdateStartup` is in, with `Mode::Reconciling` and
`Mode::FirstRunCapture` gone.** Those two modes were the discriminator. The plan
replaces them with `stagesDone_`, which is milestone 2. Milestone 1 reads the
stage off the jobs instead: the reconcile phase is `!capture_`, and `capture_`
exists only once the reconcile is over. The consequence is that
`FirstRunCaptureJob` is now *constructed* at the end of the reconcile's last
`Update` rather than at the start of the capture's first one. Its constructor
does no I/O, and its first `Step()` still runs on the frame after, so no file
operation moved. The alternative — introducing `stagesDone_` early — is milestone
2's work.

**Whether a failed reconcile still runs the first-run capture.** It does, and it
is set up before the outcome is classified, so a launch that fails both shows
`ReconcileFailed` first and `CaptureFailed` after the player continues — P8's
"once per failure, in stage order". This is also what preserves the file-op
invariant: today's code reaches the capture whatever the reconcile's `ok` was.

**What `X` does in the error state on this milestone.** §4.4 has it resume the
stage machine, which does not exist yet; §7's milestone-2 step 4 explicitly
defers that. Here `X` clears `problem_`, sets `mode_ = Mode::Loading`, and calls
`FinishStartup()` *only if there is no capture left to do* — so continuing from a
failed reconcile on a first run drops into the capture rather than skipping it.

**What `DrawStartup`'s subtitle says now that no mode names the job.** It reads
`capture_ ? "FIRST RUN" : "STARTING UP"` — the same two strings on the same two
occasions as before. The whole function is deleted in milestone 2.

**What replaces `startupBusy_` in `DrawStartup`.** `bool busy = reconcile_ ||
capture_`, which is what the boolean meant. `DrawProblem` needs no equivalent: no
job is running there, so it draws the full `VisibleRowCount(kProgressLayout)`
rows and no live status line, which is the case `ui_scroll_verify.py` already
models (hazard §3.3 row 6).

**Case numbering in `settings_ui_verify.py`.** The four totality/mapping cases
are a new group, numbered `10:` the way the picker band was added as `9:`. The
three width cases are `5:`, the file's existing group for "everything fits",
which is where the delete-confirmation strings also sit.

---

## 4. Verification run

| Check | Command | Result |
| ----- | ------- | ------ |
| Build | `cd app && make` | `IV0000-BBRD00001_00-BBRANDOMIZERMAIN.pkg` built, **no warnings** |
| UI strings and geometry | `python app/tools/settings_ui_verify.py` | **89/89 passing** (82 before; 7 new cases, 0 failures). Widest problem sentence 1685 px of 1920 at scale 4 — exactly the plan's predicted figure |
| Scroll bands | `python app/tools/ui_scroll_verify.py` | passing, **file unedited** |
| Worlds rules | `python app/tools/worlds_verify.py` | 97/97 passing, file unedited |
| The other ten mirrors | `boss`, `caged_dogs`, `drops`, `easy_modes`, `hunter_tools`, `mergo_darkness`, `pool`, `starting_weapons`, `treasure` (`selftest ../data/vanilla/dvdroot_ps4`); `itemdata` (`roundtrip ...`); `font_atlas` | all passing — run to confirm nothing outside this screen moved |

The seven new `settings_ui_verify.py` cases:

```
  10: the outcome table has one row for each of the 4 non-None StartupProblem outcomes ok
  10: every sentence and prompt the table names is a measurable named string ok
  10: all 4 outcome sentences are distinct and non-empty                   ok
  10: every prompt is kPromptContinue or kPromptExit, and only NoSignedInPlayer exits ok
  5: the 4 startup problem sentences fit the screen at scale 4 (widest 1685 px of 1920) ok
  5: the startup problem title fits the screen at scale 5 (513 px)         ok
  5: the scroll hint and both prompts fit the screen at scale 3 (widest 284 px) ok
```

Nothing failed, and nothing in the plan's §6 for this milestone was skipped.

---

## 5. What this does not prove

Per `CLAUDE.md` §3, a clean cross-compile and green mirrors mean **ready for
hardware testing** and nothing more. Specifically unproven here:

* That any of the four error states is ever *entered*. The mirrors read the
  table out of the source; no tool available here runs `WorldReconcileJob` or
  `FirstRunCaptureJob`, so the classification of `!ok` into
  `JournalNotUnderstood` versus `ReconcileFailed` rests entirely on reading
  `WorldActivation.cpp`.
* That a healthy launch no longer holds. The hold was `startupWait_`, which is
  deleted, but "the list appears without a prompt" is a runtime observation.
* That the no-signed-in-player path reaches the error state at all — it needs a
  console with nobody signed in.
* That `X` on a failed reconcile during a first run lands in the capture rather
  than skipping it. This is the one control-flow change with no mirror.
* Frame ordering and timing of any kind. That is milestone 2's substance and is
  unobservable here (`plan-evidence.md` §E7).

---

## 6. Hardware test handoff

Install the `.pkg` and read `live.log` alongside each run.

**1. Healthy launch.** Launch normally on a console with an account signed in and
worlds already present.
*Look for:* the old technical log (expected on this milestone), then the worlds
list, with **no prompt and no hold of any kind**. A `X CONTINUE` on a healthy
launch is a milestone-1 failure.

**2. `DEFAULTS` and back.** Press Left, then Right.
*Look for:* the list immediately, no startup log, and no new `worlds:` lines in
`live.log`.

**3. Journal at a phase the app writes.** Hand-write an `activation.journal` at a
phase in 1–7 and launch.
*Look for:* no prompt, and the resulting world marked `ACTIVE`. This is a silent
path now.

**4. Journal at a phase the app does not write** (e.g. 9). Hand-write it and
launch.
*Look for:* `STARTUP PROBLEM` in red, then
`AN INTERRUPTED ACTIVATION WAS NOT UNDERSTOOD AND WAS LEFT ALONE`, the log
beneath it, `UP DOWN SCROLL` and `X CONTINUE`. Up/Down must scroll the log. The
journal must still be on disk afterwards. `X` must go to the list.
*Failure looks like:* a blank sentence, `O EXIT` instead of `X CONTINUE`, or the
journal gone.

**5. No signed-in player.** Sign out on the console and launch.
*Look for:* `STARTUP PROBLEM`, `NO SIGNED-IN PLAYER`, the raw return code visible
in the log beneath it, and `O EXIT` with **no** `X CONTINUE`. `O` exits; `X` must
do nothing.
*Failure looks like:* the worlds list appearing, which is the behaviour this
replaces.

**6. First run** (optional on this milestone, and the slowest). A real save, no
worlds folder.
*Look for:* no prompt after the capture, and a list with `VANILLA` holding the
save.

The timing measurements the plan asks for — how long the bar sits on its fourth
step, and on step 3 during a capture — belong to milestone 2; there is no bar
yet.

---

## 7. Stop point

**Completion gate, not a stop condition.** All seven of §7's milestone-1 changes
are done, the `.pkg` builds warning-free, all three verifiers the plan names pass
(plus the other eleven), and the milestone is handed over for hardware testing.
Milestone 2 has not been started.

One thing noticed and deliberately not fixed, for whoever takes milestone 2:
§4.3's "Finishing the reconcile" sets `stagesDone_ = 2` on the `!ok` path and
only checks `!store_->AccountDirExists()` inside the `ok` branch. Read literally,
that means continuing from a failed reconcile on a console that is *not* a first
run would re-enter stage 2 and construct a `FirstRunCaptureJob` that today's code
never constructs — a file operation added, against §3.1. Milestone 1 sidesteps it
by deciding `firstRun` before classifying the outcome. Milestone 2 will need the
same care when it turns that branch into stage indices.

---

# Implementation Report — Startup Screen — milestone 2

**Status: AWAITING HARDWARE TEST** — and with it the whole feature, which is
code-complete at this milestone. The developer chose to test the two milestones
together, so §6 below is the handoff for **the feature**, not for this milestone
alone.

**Plan:** `docs/features/startup-screen/plan.md` — milestone 2, "the loading
state and the five-stage bar"

**Spec:** `docs/features/startup-screen/spec.md`

**Implemented:** 2026-09-26

---

## 1. What was built

The technical log is gone from the healthy path. A launch now shows the
wordmark, a rule, a five-cell bar and the word `LOADING`, and nothing else at
any point; the log is still written, still accumulated, and still drawn — but
only by the error state.

Underneath it is the five-stage machine of plan §4.3. `stagesDone_` is the
running stage, `stageDrawn_` says whether that stage has had a frame of its own,
and `UpdateStartup` does nothing until it has. That is the whole
present-then-block rule, and it applies to all five stages including the first,
which the constructor enters before any frame exists. `FinishStartup` is gone:
the container read and the list build are now stages 3 and 4, each with its own
drawn frame, so the bar is on its fourth cell *while* the container is being
read rather than after it.

| # | Planned change | Status | Where |
| - | -------------- | ------ | ----- |
| 1 | `bool ActingOnJournal() const` on `WorldReconcileJob` | done — `return s_->stage >= 2;`; the job's stages 0–1 are the sweep and the journal read, 2–3 the action and the resume | `app/src/Game/WorldActivation.h:431-440`; `app/src/Game/WorldActivation.cpp:1208-1212` |
| 2 | `stagesDone_`, `stageDrawn_`, and one helper every assignment goes through | done — `SetStagesDone` is the only writer of `stagesDone_` anywhere, and it clears `stageDrawn_` | `app/src/UI/WorldsScreen.h:152-157, 207-230`; `app/src/UI/WorldsScreen.cpp:491-497` |
| 3 | Restructure `UpdateStartup` into five stages; delete `FinishStartup` | done — the probe and `Refresh` are separate stages, no stage's work is reachable while `!stageDrawn_`, and no reference to `FinishStartup` survives under `app/` | `app/src/UI/WorldsScreen.cpp:499-636` |
| 4 | `X` in the error state resumes the machine | done — it clears `problem_`, sets `Mode::Loading` and `stageDrawn_ = false`, and nothing else; the machine carries on from `stagesDone_` under the loading screen | `app/src/UI/WorldsScreen.cpp:650-665` |
| 5 | The loading geometry constants and `DrawLoading`, ending in `stageDrawn_ = true` | done — four elements, no log, no input, no fifth thing | `app/src/UI/WorldsScreen.cpp:94-112, 186-192, 915-949` |
| 6 | Extend `settings_ui_verify.py` with milestone 2's cases | done — 5 new cases, **94/94 passing** (89 before) | `app/tools/settings_ui_verify.py:1481-1536` |

### The five stages, and what runs on which frame

| `stagesDone_` | Work | Healthy launch |
| ------------- | ---- | -------------- |
| 0 | `WorldReconcileJob` sweeping and reading the journal | two `Step()`s |
| 1 | the same job acting on the journal, including a multi-frame resume | skipped — `ActingOnJournal()` never becomes true before `Done()` |
| 2 | `FirstRunCaptureJob` | skipped, because `firstRun_` is false |
| 3 | `ProbeContainer()` | the slow one |
| 4 | `Refresh()` | one frame |
| 5 | — | `session_.startupDone`, `Mode::Browse` |

The stage-0/stage-1 boundary is driven off `ActingOnJournal()` and nothing else
— not `Progress()`, which is a fraction, and not the line count, which depends
on what the journal said (hazard §3.3 row 2). `Done()` is tested before it
everywhere it is used, so the job's finished stage 4 also reading `>= 2` is a
case nothing sees.

### The trap milestone 1 flagged, and how it is handled

§4.3 sets `stagesDone_ = 2` on the reconcile's `!ok` path but asks
`!store_->AccountDirExists()` only inside the `ok` branch. Continuing from a
failed reconcile therefore enters stage 2 on **every** console, including one
that is not on a first run — and stage 2 constructing a `FirstRunCaptureJob`
there would be a file operation this app has never performed, which §3.1 forbids
outright.

It is handled by **remembering the answer rather than re-asking it**.
`firstRun_` is set once, at the reconcile's finish, before the outcome is
classified — exactly where milestone 1 computed its local `firstRun`. Stage 2's
first statement is then:

```cpp
if (!firstRun_) {
    SetStagesDone(3);
    return;
}
```

so a launch with nothing to capture completes the stage the moment it is entered
— which is also B4's "a stage this launch does not need counts as completed
immediately and advances the bar" — and the capture job is constructed on
exactly the launches that constructed it before.

Re-asking `AccountDirExists()` at stage 2 was rejected for two reasons: it is a
second file operation where the plan permits none, and after a successful
capture the answer is a different one, so a machine that re-entered stage 2
would read it wrongly. `firstRun_` is the deviation recorded in plan §10.

### Invariants checked

* **No file operation added, removed or reordered.** `SweepPartialBackups` →
  `SweepPartialWorldSaves` → `ReadActivationJournal` → the journal's action →
  `FirstRunCaptureJob` (first runs only) → `DiscoverSaveTitle` → `ReadContainer`
  → `Refresh`'s manifest and world reads, in that order, on exactly the launches
  that did them before. What changed is which frame each runs on. The one thing
  milestone 1 had moved a frame earlier — the `FirstRunCaptureJob`
  *constructor*, which touches no files — moved back to where §4.3 puts it.
* **`AccountDirExists()` is called the same number of times**, at the same point
  in the sequence, on every path. That is why `firstRun_` exists.
* **The startup sequence runs once per launch, not once per tab visit.**
  `session_.startupDone` is still the guard, still set in exactly one place —
  stage 4, the last thing the machine does — and the constructor's
  `startupDone` branch is untouched (B15).
* **Every line still reaches `live.log` with the same text.** No `Say` was
  added, removed or reworded in this milestone. The two first-run lines are said
  at the reconcile's finish, as §4.3 specifies and as milestone 1 said them; the
  capture's result lines are byte-identical and are said at the same point
  relative to the job finishing. The log is simply no longer *drawn* on the
  healthy path (B13).
* **Only the error state consumes input.** `UpdateStartup` still `(void)`s its
  argument as its first statement, and `DrawLoading` draws no prompt and reads
  no input.
* **The error state's log band is unmoved.** `kProgressLayout`,
  `kProblemTitleY`, `kProblemSentenceY` and the two footer lines are untouched;
  every new constant is a `kLoad*` drawn only by the loading state.
  `ui_scroll_verify.py` passes **unedited**, as §6 requires.
* **`WorldReconcileJob`'s behaviour is unchanged by the accessor.** It is one
  `const` line returning an existing field; nothing else in
  `WorldActivation.{h,cpp}` changed, and `worlds_verify.py` passes unedited.
* **The bar never retreats.** `stagesDone_` is written only by `SetStagesDone`,
  and every call site passes a value greater than the current one.
* **`DrawConfirmDelete` still uses `kStartupTitleScale` and
  `kStartupSubScale`.** Both survive; `DrawLoading` uses them too.

---

## 2. Deviations from the plan

Both are recorded in the plan's §10, and both come out of the trap above.

**D5 — `bool firstRun_` added to `WorldsScreen`.** §5's milestone-2 row lists
`stagesDone_` and `stageDrawn_` only. See "the trap" in §1 for why a third piece
of state is unavoidable given where §4.3 asks the first-run question and where it
sets `stagesDone_ = 2`.

**D6 — `FirstRunCaptureJob` is constructed in stage 2, not at the reconcile's
finish.** §4.3 says stage 2 creates it on first entry, and that is what is now
implemented; milestone 1 had constructed it a frame earlier because it had no
stage index to defer to (milestone 1's D4). Nothing observable changed — the two
first-run lines are said in the same place, on the same launches, before the
job's first `Step()`.

The §5/§7 disagreement over `DrawStartup` and `FinishStartup` (milestone 1's D1)
is now closed: §7 was followed, both symbols are gone, and `DrawLoading` is in.
`kLoadingWord` (milestone 1's D2) arrived with it and no longer warns.

---

## 3. Decisions the plan left open

**Stages 0 and 1 share one code block.** §4.3 describes them as two transitions
whose bodies differ by one line — stage 0 also tests `ActingOnJournal()`. They
are written as a single `if (stagesDone_ <= 1)` block with that test guarded by
`stagesDone_ == 0`, rather than as two blocks with five duplicated lines. The
guard matters: calling `SetStagesDone(1)` while already at stage 1 would clear
`stageDrawn_` every frame and halve the job's step rate.

**What a stage this launch does not need looks like on the bar.** §4.3 has the
reconcile's `ok` path jump straight from 2 to 3 when there is nothing to
capture, so on a healthy launch stage 2 is never displayed; the resumed path
through stage 2's own guard *does* display it, for one frame. Both satisfy B4.
The difference is one frame at 60 Hz on a path that only follows a failed
reconcile, and levelling it would have meant editing §4.3's `ok` branch.

**Cell colours.** §4.6 fixes them — `Palette::Good` below the running cell,
`Palette::Selected` on it, `kRuleColor` above — and that is what is coded. The
empty cells being the rule's own grey means an untouched bar reads as furniture
rather than as an error; that is a TV judgement, and is the kind of thing the
hardware test may want to adjust.

**The verifier asserts the word `LOADING` literally**, not just its width.
`kLoadingWord == "LOADING"` is part of the case, because B2 names the literal
word and a screen that said `PLEASE WAIT` would still fit the block.

**`stageDrawn_ = false` in `UpdateProblem` is written directly**, not through
`SetStagesDone`. §4.4 specifies exactly that, and §7's done-condition is that no
other code writes `stagesDone_` — which holds. Continuing does not advance the
bar; it re-shows the stage that is already current.

**Case numbering in `settings_ui_verify.py`.** The loading state's two width
cases are `5:` and its three geometry cases are `6:`, the file's existing groups
for "everything fits" and "nothing overlaps". No new group number was needed —
`10:` stays the error state's totality check.

---

## 4. Verification run

| Check | Command | Result |
| ----- | ------- | ------ |
| Build | `cd app && make` | `IV0000-BBRD00001_00-BBRANDOMIZERMAIN.pkg` built, **no warnings**. Incremental, which §6 says is sufficient — nothing generated changed |
| UI strings and geometry | `python app/tools/settings_ui_verify.py` | **94/94 passing** (89 before; 5 new cases, 0 failures) |
| Scroll bands | `python app/tools/ui_scroll_verify.py` | passing, **file unedited** |
| Worlds rules | `python app/tools/worlds_verify.py` | 97/97 passing, file unedited |
| Font atlas | `python app/tools/font_atlas_verify.py` | passing — the baked metrics the two new width cases measure from |

The five new `settings_ui_verify.py` cases, as the run printed them:

```
  5: the wordmark fits the loading block at scale 5 (760 px of 1000)       ok
  5: LOADING fits the loading block at scale 4 (206 px of 1000)            ok
  6: the loading block is centred (460 + 1000 + 460 == 1920)               ok
  6: the bar is 5 equal cells of 184 px filling the block exactly          ok
  6: the loading state's 4 elements are in order, clear of each other and inside the screen ok
```

**Not run:** the ten data-parity mirrors (`boss`, `caged_dogs`, `drops`,
`easy_modes`, `hunter_tools`, `itemdata`, `mergo_darkness`, `pool`,
`starting_weapons`, `treasure`). Milestone 1 ran them all; nothing in this
milestone touches randomizer rules, `Msb`, `Param` or game data — the changed
files are one UI screen, one read-only accessor on a job, and one verifier. They
are recorded here as **not run**, not as passed.

---

## 5. What this does not prove

Per `CLAUDE.md` §3, a clean cross-compile and green mirrors mean **ready for
hardware testing** and nothing more. Almost everything this milestone is about
is unobservable here (`plan-evidence.md` §E7):

* **That the bar is never behind the work.** The present-then-block rule is a
  claim about `Update`/`Draw`/`Present` ordering across frames, and no tool
  available here runs a frame.
* **That the bar steps at all**, that it never retreats, or that any particular
  cell is the one lit during the container read.
* **That the loading screen appears on the first frame** rather than after a
  black one.
* **That the container read does not trip a watchdog.**
  `docs/features/worlds/implementation-report.md` lists "two seconds of blocking
  work inside one `Update()` does not trip a watchdog" as unverified, and it
  still is. This milestone neither lengthens nor moves that call; what changes is
  that the screen in front of it now looks like a loading screen, so if the
  console does object, this is the first build where the symptom is unambiguous
  rather than hidden behind a log that had already stopped scrolling.
  Deliberately not addressed.
* **How long any stage takes.** Stage 3's duration is the one measurement this
  feature exists to obtain, and only the console can give it.
* **That any of the four error states is ever entered**, which was already
  milestone 1's caveat.

---

## 6. Hardware test handoff — the whole feature

Both milestones are in the tree and are tested together. Install the `.pkg` and
read `live.log` alongside each run.

**1. Healthy launch.** An account signed in, worlds already present.
*Look for:* the wordmark, the rule, the bar and `LOADING` **from the first
frame**; the bar stepping; then the worlds list — no prompt, no hold, no black
frame. The technical log must not appear at all.
***Measure:* how long the bar sits on its fourth cell.** That is the container
read — spec §4.3's inferred but never-timed ~2 s, and the only measurement this
feature can obtain. Note also any sign the console objects to it (a stutter, a
dropped frame, a forced quit).
*Failure looks like:* the bar reaching its last cell and the screen then sitting
there — that would mean the work is running a step ahead of the bar.

**2. `DEFAULTS` and back.** Press Left, then Right.
*Look for:* the list immediately. **No loading screen**, no bar, and no new
`worlds:` lines in `live.log`.

**3. First run.** A console with a real Bloodborne save and no worlds folder.
*Look for:* the loading screen throughout, no prompt, then a list with `VANILLA`
holding the save.
***Measure:* how long the bar sits on its third cell** — that is the capture.

**4. Journal at a phase the app writes** (1–7), hand-written, then launch.
*Look for:* no prompt, and the resulting world marked `ACTIVE`. The observation
wanted is **whether the second cell is ever visibly lit** — the reconcile may
pass through its acting stage in a single frame.

**5. Journal at a phase the app does not write** (e.g. 9).
*Look for:* `STARTUP PROBLEM` in red,
`AN INTERRUPTED ACTIVATION WAS NOT UNDERSTOOD AND WAS LEFT ALONE`, the log
beneath it, `UP DOWN SCROLL` and `X CONTINUE`. Up/Down must scroll the log, and
the journal must still be on disk afterwards.
***Then press `X`:*** the **loading screen** must come back, showing the fourth
cell as the running one, and then the list. It must **not** freeze on the error
screen during the container read, and it must not re-run the sweep.

**6. No signed-in player.** Sign out and launch.
*Look for:* `STARTUP PROBLEM`, `NO SIGNED-IN PLAYER`, the raw return code in the
log beneath it, `O EXIT` and **no** `X CONTINUE`. `O` exits; `X` does nothing.

**Watch for, throughout:** the bar freezing a cell behind the work; a black
screen during the container read; the list appearing before a capture finished;
any prompt on a healthy launch; any text on the loading screen other than
`BLOODBORNE RANDOMIZER` and `LOADING`.

**A judgement to make on a TV:** the empty cells are the rule's grey
(`64, 72, 82`) and the running cell is `Palette::Selected`. If the bar is hard
to read, that is a colour change and not a design change.

---

## 7. Stop point

**Completion gate, not a stop condition.** All six of §7's milestone-2 changes
are done, the `.pkg` builds warning-free, all three verifiers the plan names
pass — `ui_scroll_verify.py` and `worlds_verify.py` unedited, as §6 requires —
and the feature is handed over for hardware testing. No stop condition fired.

Noticed and deliberately not fixed:

* **The watchdog exposure on stage 3**, described in §5. §3.3 row 5 says to
  change nothing about the container read and report the observed duration
  instead, which is what the handoff asks for.
* **`Say("STARTING UP")` in the constructor** now writes a line no healthy
  launch will ever display. It still reaches `live.log`, which is what §3.1
  protects, so it was left exactly as it is.
* **`WorldReconcileJob::StatusText()`/`Progress()` and
  `FirstRunCaptureJob::StatusText()`/`Progress()` have lost their only caller**
  now that `DrawStartup` is gone. They are ordinary public API on the two jobs
  and nothing in scope asked for them to be removed.
* Everything §3.2 lists — the duplicated `DiscoverSaveTitle`/`ReadContainer` on
  a first run, `AfrManager::Check`'s scratch probe file, the duplicated
  `reconcile:` and `worlds:` lines in `live.log`, `live.log` being wiped each
  launch, and `Application::Run` not returning after `Platform::Init` fails — is
  untouched. `Application.cpp` was not opened.
