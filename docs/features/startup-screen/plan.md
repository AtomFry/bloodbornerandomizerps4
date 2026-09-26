# Plan — Startup Screen

**Status: APPROVED** — approved by the developer 2026-09-26.

**Spec:** `docs/features/startup-screen/spec.md` — APPROVED 2026-09-26

**Evidence:** `docs/features/startup-screen/plan-evidence.md` — the measurements,
traces and rejected alternatives behind this plan

**Plan review:** `docs/features/startup-screen/plan-review.md` — added during review

---

> **This document is the implementation contract.** §1–§7 are what the
> implementer reads, in order. §8–§10 are the record.

---

## 1. Objective

Replace the scrolling technical log the app shows while it launches with a
loading screen carrying a five-step progress bar, and show an error screen —
one sentence, the log, a prompt — only when one of four named startup failures
occurs. Underneath, replace the two booleans that encode the end of startup
with one explicit outcome, and run each blocking unit of startup work on the
frame *after* the one that shows its step, so the bar is never behind the work.

---

## 2. Approved behaviour

| # | Behaviour | Source |
| - | --------- | ------ |
| B1 | The startup screen has exactly two states: a loading state and an error state | spec §10 D1 |
| B2 | The loading state draws the wordmark `BLOODBORNE RANDOMIZER`, a rule, a progress bar and the literal word `LOADING`, and no other text, at every point | spec §10 D2, §2 |
| B3 | The bar has five equal steps, one per stage of spec §4.7, advances when a stage completes, never retreats, and does not animate between steps | spec §10 D3, §7 |
| B4 | A stage this launch does not need counts as completed immediately and advances the bar | spec §5, §7 |
| B5 | The bar shows the step whose work is running while that work runs — in particular during the container read | spec §2, §8 AC3 |
| B6 | A healthy launch has no hold, no prompt and no minimum display time; the worlds list replaces the loading screen directly | spec §10 D4 |
| B7 | A first run does not hold or prompt either | spec §10 D5 |
| B8 | Exactly four conditions enter the error state: reconcile failed; journal phase not understood; first-run capture failed; no signed-in player | spec §2, §4.1 |
| B9 | Each of the four shows its own one-line, screen-owned sentence; the job's own wording and any return code stay in the log below it | spec §2, §7 |
| B10 | The prompt is `X CONTINUE` for the first three and `O EXIT` for the fourth | spec §10 D10 |
| B11 | The error state is the only state that holds, the only one that prompts and the only one that draws the log; its log drawing and scrolling are today's | spec §10 D6 |
| B12 | Every other startup situation — swept debris, a journal correctly dealt with, a first run that worked, a first run with nothing to capture, no Bloodborne save data — is silent and goes to the log only | spec §2 |
| B13 | The log keeps accumulating on every launch even though only the error state draws it, and every line still reaches `live.log` | spec §4.5, §6 |
| B14 | No signed-in player routes to the error state instead of dropping to a list that looks working and refuses later | spec §4.2, §6 |
| B15 | Returning to `WORLDS` from `DEFAULTS` shows the list immediately, with no loading state and no repeated startup work | spec §2, §7 |

---

## 3. Constraints and invariants

### 3.1 Must be preserved

* **No file operation is added, removed or reordered.** The same sweeps,
  reconciliation, capture, container read and list build, in the same order —
  breaking this puts save data at risk, which is the one thing this item is not
  allowed to touch.
* **The startup sequence runs once per launch, not once per tab visit.**
  `WorldsSession::startupDone` stays the guard; re-running it would re-sweep and
  re-probe every time the player presses Left.
* **Every line the startup sequence says still reaches `live.log` with the same
  text.** The on-screen log stops being drawn on healthy launches; it does not
  stop being written.
* **Layering:** this screen already reaches `WorldStore`, `AfrManager` and
  `savedata` and must keep doing so through those APIs only — no raw AFR paths
  and no new filesystem calls in `UI/`.
* **Only the error state consumes input.** No input handling anywhere in the
  loading state.
* **The error state's log band keeps today's geometry** — `kProgressLayout`, a
  heading at y 200 at scale 4, a two-line footer at `kScreenHeight - 130` — so
  `ui_scroll_verify.py`'s `Worlds startup` entry stays valid unchanged.
* **`DrawConfirmDelete` keeps using `kStartupTitleScale` and
  `kStartupSubScale`.** Those two constants are shared; do not delete them with
  the old startup drawing.

### 3.2 Out of scope

* The duplicated `DiscoverSaveTitle` / `ReadContainer` on a first run.
* `AfrManager::Check` writing `bbrandomizer_write_test.tmp` during `Refresh`.
* The duplicated `reconcile:` / `worlds:` lines in `live.log`.
* `live.log` being wiped by `Platform::Init` every launch.
* `Application::Run` not returning after `Platform::Init` fails. Do not touch
  `Application.cpp` at all.
* Any change to what `WorldReconcileJob` or `FirstRunCaptureJob` *do*. The only
  permitted change to either is the one read-only accessor in §5.
* The worlds list, the editor, the settings screens, chalice dungeons.

### 3.3 Hazards

| Hazard | What to do | Evidence |
| ------ | ---------- | -------- |
| The bar freezes a step behind the work because a stage's work runs in the same `Update` that entered the stage | Apply the `stageDrawn_` gate of §4.3 to **every** stage, including the first, which is entered in the constructor | §E5.1 |
| The reconcile job's two bar steps are driven off the wrong job state, so step 2 never shows or shows on a healthy launch | Drive the step-1/step-2 boundary off the new `ActingOnJournal()` accessor, not off `Progress()` or a line count | §E5.2 |
| `ReconcileResult::error` is empty on one of the two reachable failure paths, so a sentence built from it would be blank | Choose the sentence from `hadJournal` and `action`, never from `error` | §E5.3 |
| `FirstRunCaptureResult::note` is set on success paths and reads like a failure | Branch on `result.ok` only | §E5.4 |
| The container read blocks one `Update` for an unmeasured duration (~2 s inferred, never timed) and has never been shown not to trip a watchdog | Change nothing about it; report the observed duration from the hardware test (§6) | §E5.5, §E7 |
| Removing `startupBusy_` changes how many log rows the error state draws | Intended: the error state always draws the full `VisibleRowCount(kProgressLayout)` rows, which is the case `ui_scroll_verify.py` already models | §E5.6 |
| A second failure after the player continues from the first | The machine re-enters the error state with the second sentence, which is P8: one screen per failure, in stage order | §E5.7 |

---

## 4. Implementation approach

> Chosen: one screen-owned five-stage state machine, a draw-gated advance, and
> an explicit `StartupProblem` outcome. Alternatives considered and why they
> were rejected: `plan-evidence.md` §E3.

### 4.1 The two modes

`WorldsScreen::Mode` becomes `{ Loading, Problem, Browse, ConfirmDelete }`.
`Reconciling` and `FirstRunCapture` are removed: which job is running is a
stage, not a mode, and nothing on screen names it any more.

### 4.2 The outcome

`enum class StartupProblem { None, ReconcileFailed, JournalNotUnderstood,
CaptureFailed, NoSignedInPlayer };`, declared at `namespace bbr` scope in
`WorldsScreen.h` and held as `StartupProblem problem_`. It replaces
`startupBusy_` and `startupWait_`, both of which are deleted.

The mapping from outcome to what the screen draws is a **file-scope table** in
`WorldsScreen.cpp`'s anonymous namespace — one row per non-`None` outcome, each
row `{ StartupProblem, sentence, prompt }` — with two small lookups over it. A
table rather than a `switch` so `settings_ui_verify.py` can parse the mapping
and assert it is total (§6).

### 4.3 The stage machine

Five stages, indexed 0–4, exactly the five of spec §4.7:

| Index | Stage | Work |
| ----- | ----- | ---- |
| 0 | find out | `WorldReconcileJob` sweeping and reading the journal |
| 1 | act | `WorldReconcileJob` acting on the journal, including a multi-frame resume |
| 2 | capture | `FirstRunCaptureJob` |
| 3 | read the container | `ProbeContainer()` |
| 4 | build the list | `Refresh()` |

State: `int stagesDone_` (0–5) and `bool stageDrawn_`. The **running** stage is
`stagesDone_` while `stagesDone_ < 5`.

* Any change to `stagesDone_` sets `stageDrawn_ = false`.
* `DrawLoading` sets `stageDrawn_ = true` as its last statement.
* `UpdateStartup` returns immediately while `!stageDrawn_`.

That is the whole present-then-block rule, and it covers the first stage too:
the constructor enters stage 0 before any frame exists, so frame 0 draws the
loading screen and frame 1 does the sweep. `DrawLoading` writing screen state is
the same shape as `DrawStartup` already writing `logScroll_` today.

Transitions, all inside `UpdateStartup`:

* **Stage 0** — create `reconcile_` on first entry; `Step()`;
  `AddLines(TakeLines())`. If `Done()`, finish the reconcile (below). Otherwise
  if `ActingOnJournal()`, set `stagesDone_ = 1`.
* **Stage 1** — `Step()`; `AddLines(TakeLines())`; if `Done()`, finish the
  reconcile.
* **Finishing the reconcile** — read `Result()`, reset `reconcile_`, set
  `stagesDone_ = 2` (a failed stage is still over, so the bar still advances),
  then: if `!result.ok`, enter the error state with `JournalNotUnderstood` when
  `result.hadJournal && result.action == ReconcileAction::Nothing`, and with
  `ReconcileFailed` otherwise. If `ok`, and `store_` exists and
  `!store_->AccountDirExists()`, say today's two first-run lines and stay at
  stage 2; otherwise set `stagesDone_ = 3`.
* **Stage 2** — create `capture_` on first entry; `Step()`; when `Done()`, say
  today's result lines unchanged, reset `capture_`, set `stagesDone_ = 3`, and
  enter the error state with `CaptureFailed` if `!result.ok`.
* **Stage 3** — `ProbeContainer()`; set `stagesDone_ = 4`.
* **Stage 4** — `Refresh()`; set `stagesDone_ = 5`; `session_.startupDone = true`;
  `mode_ = Mode::Browse`.

`FinishStartup()` is deleted; stages 3 and 4 are what it used to do, split so
each gets its own drawn frame.

### 4.4 Entering and leaving the error state

Entering sets `problem_`, `mode_ = Mode::Problem` and `logFollowTail_ = true`.
The lines explaining the failure are already in `lines_` by then.

`UpdateProblem`: up/down scroll the log exactly as `UpdateStartup` does today.
For the three continuable outcomes, `X` sets `mode_ = Mode::Loading`,
`problem_ = StartupProblem::None` and `stageDrawn_ = false`, and the machine
carries on from `stagesDone_` — so the container read is again covered by the
loading screen rather than freezing the error screen. For `NoSignedInPlayer`,
`O` sets `wantsExit_ = true` and nothing else responds. No state honours an
input it does not draw a prompt for.

### 4.5 The constructor

Unchanged except for the no-signed-in-player branch: instead of `Log(...)` then
`Refresh()` then `Mode::Browse`, it `Say`s `user_.error` — the same `live.log`
text, now also on the on-screen log — and enters the error state with
`NoSignedInPlayer`. `stagesDone_` stays 0 and startup never runs. The
`startupDone` tab-return branch is otherwise untouched (B15).

### 4.6 Drawing

`DrawStartup` splits into `DrawLoading` and `DrawProblem`.

`DrawLoading`: clear; the wordmark centred at `kLoadWordmarkY` at
`kStartupTitleScale` in `Palette::Heading`; a `kRuleThickness` rule at
`kLoadRuleY` spanning `[kLoadBlockX, kLoadBlockX + kLoadBlockW)` in
`kRuleColor`; five bar cells at `kLoadBarY`, height `kLoadBarH`, each
`kLoadCellW` wide and `kLoadCellGap` apart across the same block — cells below
`stagesDone_` in `Palette::Good`, the cell at `stagesDone_` in
`Palette::Selected`, the rest in `kRuleColor`; `kLoadingWord` centred at
`kLoadWordY` at `kStartupSubScale` in `Palette::Text`. Nothing else, ever.

The loading geometry, as `const int` in the anonymous namespace so
`parse_geometry` picks it up:

| Constant | Value | |
| -------- | ----: | - |
| `kLoadWordmarkY` | 390 | wordmark draw y, at `kStartupTitleScale` (5) |
| `kLoadRuleY` | 480 | |
| `kLoadBarY` | 530 | |
| `kLoadBarH` | 28 | |
| `kLoadWordY` | 610 | `LOADING` draw y, at `kStartupSubScale` (4) |
| `kLoadBlockX` | 460 | `(1920 - kLoadBlockW) / 2` |
| `kLoadBlockW` | 1000 | shared by the rule and the bar |
| `kLoadCellGap` | 20 | |
| `kLoadCellW` | 184 | `(kLoadBlockW - 4 * kLoadCellGap) / 5` |
| `kLoadStageCount` | 5 | also the denominator of `stagesDone_` |

`DrawProblem`: today's `DrawStartup` with `kProblemTitle` in `Palette::Bad` at
`kProblemTitleY`, the outcome's sentence at `kProblemSentenceY`, the log loop
with `logRows = VisibleRowCount(kProgressLayout)` and no live status line, the
scroll hints, then `kProblemScrollHint` and the outcome's prompt as today's two
footer lines.

### What this reuses

| Existing code or tool | How it is used | Change needed |
| --------------------- | -------------- | ------------- |
| `app/src/UI/WorldsScreen.{h,cpp}` | the whole feature lives here | rewritten in the parts named in §5 |
| `app/src/UI/Controls.h` — `DrawCenteredLabel`, `ListLayout`, `VisibleRowCount`, `ClampScroll`, `DrawScrollHints`, `Palette` | the error state's log and every label | none |
| `Renderer::FillRect` | the rule and the five bar cells; there is no bar primitive in this app and this change does not add one to `Controls` | none |
| `app/src/UI/WorldEditorScreen.cpp` `UpdateConfirm` / `OpenConfirm` | the present-then-block pattern §4.3 generalises | none |
| `app/src/Game/WorldActivation.{h,cpp}` — `WorldReconcileJob` | stages 0 and 1 | one read-only accessor |
| `app/src/Randomizer/WorldStore.{h,cpp}` — `FirstRunCaptureJob` | stage 2 | none |
| `app/tools/settings_ui_verify.py` | measures every named string and the loading stack | extended |
| `app/tools/ui_scroll_verify.py` | already covers the error state's log band | none — it must keep passing unchanged |

---

## 5. Files and changes

| File | Change | Milestone |
| ---- | ------ | --------- |
| `app/src/UI/WorldsScreen.h` | `StartupProblem` enum at namespace scope; `Mode` becomes `{ Loading, Problem, Browse, ConfirmDelete }`; drop `startupBusy_` and `startupWait_`, add `problem_`; replace the `DrawStartup`/`FinishStartup` declarations with `DrawLoading`, `DrawProblem`, `UpdateProblem` | 1 |
| `app/src/UI/WorldsScreen.h` | add `int stagesDone_` and `bool stageDrawn_` | 2 |
| `app/src/UI/WorldsScreen.cpp` | named strings `kWordmark`, `kLoadingWord`, `kProblemTitle`, the four `kProblem*` sentences, `kProblemScrollHint`, `kPromptContinue`, `kPromptExit`; the outcome table and its two lookups | 1 |
| `app/src/UI/WorldsScreen.cpp` | the constructor's no-user branch; `UpdateStartup`'s outcome handling; `UpdateProblem`; `DrawProblem`; the `Update`/`Draw` dispatch | 1 |
| `app/src/UI/WorldsScreen.cpp` | rename `kStartupTitleY`/`kStartupSubY` to `kProblemTitleY`/`kProblemSentenceY`, values unchanged | 1 |
| `app/src/UI/WorldsScreen.cpp` | the loading geometry constants (§6 lists what the verifier pins); the stage machine of §4.3; `DrawLoading`; delete `FinishStartup` | 2 |
| `app/src/Game/WorldActivation.h` | declare `bool ActingOnJournal() const;` on `WorldReconcileJob` — true once the job has stopped finding out and started acting | 2 |
| `app/src/Game/WorldActivation.cpp` | define it as `return s_->stage >= 2;` | 2 |
| `app/tools/settings_ui_verify.py` | new cases for the error state's strings and the outcome table | 1 |
| `app/tools/settings_ui_verify.py` | new cases for the loading stack and the bar arithmetic | 2 |

No generated tables, no build-system change, no clean rebuild required, and
nothing touches `defaults.cfg`, the worlds manifest or any saved data.

---

## 6. Verification

### Build

`cd app && make` produces `IV0000-BBRD00001_00-BBRANDOMIZERMAIN.pkg`.
Incremental is sufficient; nothing generated changes.

### Automated

| Check | Command | Asserts |
| ----- | ------- | ------- |
| UI strings and geometry | `python app/tools/settings_ui_verify.py` | all existing cases still pass, plus the new cases below |
| Scroll bands | `python app/tools/ui_scroll_verify.py` | unchanged and still passing — the error state's log band did not move |
| Worlds rules | `python app/tools/worlds_verify.py` | unchanged and still passing — no startup *work* changed |

New cases, milestone 1:

* each of the four `kProblem*` sentences fits 1920 px at `kStartupSubScale`
  (widest today 1685 px), and `kProblemTitle` fits at `kStartupTitleScale`
* the outcome table has one row for every enumerator of `StartupProblem` except
  `None`, every sentence is distinct, every prompt is `kPromptContinue` or
  `kPromptExit`, and `NoSignedInPlayer` is the **only** row using `kPromptExit`
* `kProblemScrollHint` and both prompts fit the screen at `kFooterScale`

New cases, milestone 2:

* `kWordmark` fits `kLoadBlockW` at `kStartupTitleScale`, and `kLoadingWord`
  fits it at `kStartupSubScale`
* `kLoadBlockX * 2 + kLoadBlockW == 1920` — the block is centred
* `kLoadBlockW - 4 * kLoadCellGap` is divisible by 5 and equals
  `5 * kLoadCellW` — five *equal* steps, with no rounding drift
* the loading stack is in order and ink-clear under the same ink-box rule the
  other screens use — wordmark ink, rule, bar, `LOADING` ink — and the whole
  stack lies within 1080

These verifiers pin the rules — that strings fit and regions do not overlap —
**not** the C++ that draws them. Nothing available here can observe the bar, the
timing or the frame ordering; see `plan-evidence.md` §E7.

### Hardware

The developer runs these; the implementer cannot.

| Test | Pass condition |
| ---- | -------------- |
| Healthy launch | The loading screen from the first frame, the bar stepping, then the list — no prompt, no hold, no black frame. **Report how long the bar sits on its fourth step**: that is spec §4.3's unverified ~2 s and the only measurement this item can obtain |
| First run, real save, no worlds folder | The loading screen throughout, no prompt, then a list with `VANILLA` holding the save. The bar sits on step 3 for the whole capture; report its duration |
| Hand-written `activation.journal` at a phase the app writes (1–7) | No prompt, and the resulting world marked `ACTIVE`. Whether step 2 is visible at all is the observation wanted |
| Hand-written journal at a phase the app does not write | The error state, `AN INTERRUPTED ACTIVATION WAS NOT UNDERSTOOD AND WAS LEFT ALONE`, the log beneath it, `X CONTINUE`; the journal still on disk; X returns to the loading screen and then the list |
| No signed-in player | The error state, `NO SIGNED-IN PLAYER`, `O EXIT` and no `X CONTINUE`; the log shows the raw return code; O exits |
| `DEFAULTS` and back | The list immediately, no loading screen, no repeated work |
| Watch for | the bar freezing a step behind the work; a black screen during the container read; the list appearing before the capture finished; any prompt on a healthy launch |

---

## 7. Milestones and stop conditions

### Milestone 1 — the outcome and the error state

**Goal.** Startup decides its outcome explicitly, the four failures show their
own sentence and prompt, and nothing else holds or prompts.

**Changes**, in order:

1. Add `StartupProblem` to `WorldsScreen.h`, and the sentence/prompt table and
   its two lookups to `WorldsScreen.cpp` — done when both lookups return a
   non-empty string for all four outcomes and the file compiles.
2. Rename `Mode::Reconciling`/`Mode::FirstRunCapture` to `Mode::Loading` and add
   `Mode::Problem`, keeping `DrawStartup` as the loading drawing for now — done
   when `Update` and `Draw` dispatch on the four modes.
3. Delete `startupBusy_` and `startupWait_`; add `problem_` — done when nothing
   references either boolean.
4. Rewrite `UpdateStartup`'s outcome handling: a reconcile failure classified by
   `hadJournal` and `action` per §4.3, a capture failure by `result.ok`, and
   every quiet path falling straight through to `FinishStartup()` with no hold —
   done when no successful path can reach `Mode::Problem` and no failing path
   can reach `Mode::Browse` without passing through it.
5. Route the constructor's no-signed-in-player branch to the error state with
   `NoSignedInPlayer`, `Say`ing `user_.error` — done when `Refresh()` is no
   longer called on that branch.
6. Add `UpdateProblem` and `DrawProblem` per §4.4 and §4.6, renaming the two Y
   constants — done when the error state draws title, sentence, log and the
   right prompt, and `X`/`O` behave per B10.
7. Extend `settings_ui_verify.py` with milestone 1's cases — done when it
   reports its new total with 0 failures.

**Invariants** this milestone must not break: no file operation added, removed
or reordered; every `Say` still reaches `live.log` with the same text;
once-per-launch startup; only the error state consumes input; the log band's
geometry is unmoved.

**Verification:** the build; all three verifiers; the hardware tests for the two
journal cases and no-signed-in-player, plus a healthy launch to confirm no hold.
On this milestone a healthy launch still shows the old technical log — that is
expected, and is what milestone 2 removes.

**Completion gate.** All of the above pass, the `.pkg` builds, and the milestone
is handed to the developer for hardware testing. Do not begin milestone 2.

### Milestone 2 — the loading state and the five-stage bar

**Goal.** A healthy launch shows a wordmark, a rule, a stepping bar and the word
`LOADING`, and every blocking unit of work runs on the frame after the one that
showed its step.

**Changes**, in order:

1. Add `bool ActingOnJournal() const` to `WorldReconcileJob` — done when it
   returns true for a journal the app acts on and false before the journal has
   been read.
2. Add `stagesDone_` and `stageDrawn_`, and route every assignment to
   `stagesDone_` through one helper that clears `stageDrawn_` — done when no
   other code writes `stagesDone_`.
3. Restructure `UpdateStartup` into the five stages of §4.3 and delete
   `FinishStartup` — done when the probe and `Refresh` are separate stages and
   no stage's work is reachable while `!stageDrawn_`.
4. Make `X` in the error state resume the machine rather than call the old
   finish path — done when continuing from a failed reconcile lands on the
   loading screen showing step 3 or later, and then the list.
5. Add the loading geometry constants and `DrawLoading`, whose last statement is
   `stageDrawn_ = true` — done when the loading state draws exactly four
   elements and nothing else.
6. Extend `settings_ui_verify.py` with milestone 2's cases — done when it
   reports its new total with 0 failures.

**Invariants** this milestone must not break: all of milestone 1's, plus — the
bar never retreats; a skipped stage advances it; no blocking work runs before
its stage has been drawn; the error state's log band is still unmoved;
`WorldReconcileJob`'s behaviour is unchanged by the accessor.

**Verification:** the build; all three verifiers; the full hardware table of §6.

**Completion gate.** All of the above pass, the `.pkg` builds, and the milestone
is handed to the developer for hardware testing.

### Stop conditions

Halt and report rather than deciding, if any of these occur:

* the build fails in a way the plan did not anticipate
* a verification check fails and the cause is not an obvious implementation slip
* `ui_scroll_verify.py` needs editing to pass — the log band was supposed not to
  move, so an edit there means the design drifted
* an implementation decision would contradict the spec or §2
* a fifth condition that should enter the error state is discovered; B8 fixes
  the set at four
* the stage machine needs more than the one read-only accessor of §5 from either
  job, or needs a job's `Step()` or result semantics changed
* `ProbeContainer()` or `Refresh()` turns out to need a different call site to
  keep the bar honest
* the change needs a file not listed in §5
* a §3.1 invariant cannot be preserved

---

## 8. Open questions

*Empty.* The one question this plan raised was a spec gap and was answered by
the developer on 2026-09-26; it is **P8** in §9 and is now binding in the spec
as well. The heading is kept so the gap between §7 and §9 cannot be mistaken
for an editing accident.

---

## 9. Decisions

| # | Date | Decision | Taken by |
| - | ---- | -------- | -------- |
| P1 | 2026-09-26 | Five stages, indexed 0–4, exactly spec §4.7's; the reconcile job's four internal stages map onto bar steps 1 and 2 through one new read-only accessor | planner |
| P2 | 2026-09-26 | The present-then-block rule is applied uniformly to all five stages, including the first (entered in the constructor) and the list build, rather than only to the container read | planner |
| P3 | 2026-09-26 | A stage that failed still counts as completed, so the bar advances past it and continuing from the error state resumes at the next stage | planner |
| P4 | 2026-09-26 | Continuing from the error state returns to the loading state rather than running the remaining stages inside the error screen's `Update` | planner |
| P5 | 2026-09-26 | The bar is five discrete cells with a gap, not one filled bar, and the running cell is drawn in `Palette::Selected` so B5 is visible rather than merely implied | planner |
| P6 | 2026-09-26 | The outcome-to-sentence-and-prompt mapping is a parseable table rather than a `switch`, so a verifier can assert it is total | planner |
| P7 | 2026-09-26 | The error state reuses today's title, subtitle, log and footer geometry exactly, so `ui_scroll_verify.py` needs no edit | planner |
| P8 | 2026-09-26 | **Each failing stage holds on its own error screen, in stage order**, with its own sentence and its own prompt. A failure is an ordinary stage outcome, so the stage machine needs no compound case and the bar stays monotonic. Collecting failures and showing one of them would hide a real failure behind another. Raised by this plan as a spec gap, not a plan choice; also recorded in spec §10 | developer |

---

## 10. Changes during implementation

| Date | Change | Reason |
| ---- | ------ | ------ |
| 2026-09-26 | **Milestone 1 keeps `DrawStartup` and `FinishStartup`.** §5's milestone-1 row says to replace both declarations with `DrawLoading`/`DrawProblem`/`UpdateProblem`; §7's milestone-1 step 2 says to keep `DrawStartup` as the loading drawing "for now" and step 4 routes quiet paths through `FinishStartup()`, and §7's milestone-2 steps 3 and 5 delete `FinishStartup` and add `DrawLoading`. §7 was followed. `DrawProblem` and `UpdateProblem` are in; `DrawLoading` and the deletion of `FinishStartup` are milestone 2 | §5 and §7 disagree about which milestone owns the two symbols; §7 is the milestone ordering and is internally consistent, §5's row collapses both milestones into one line |
| 2026-09-26 | **`kLoadingWord` is deferred to milestone 2** with `DrawLoading`, the only thing that draws it. Every other string §5 lists for milestone 1 — `kWordmark`, `kProblemTitle`, the four sentences, `kProblemScrollHint`, both prompts — is in | An unused file-scope `const char* const` is a `-Wunused-const-variable` warning under the build's `-Wall`, and it was the only warning the milestone produced. Verified by building with it and without it |
| 2026-09-26 | **`void EnterProblem(StartupProblem)` added to `WorldsScreen`,** a private helper §5 does not list, doing exactly §4.4's three statements | Four call sites enter the error state (the constructor and three in `UpdateStartup`). Three repeated statements at four sites is the drift §4.4 describes in one place |
| 2026-09-26 | **The first-run capture is set up on the reconcile's failing paths as well as its succeeding one,** so a reconcile failure followed by a first-run capture failure shows two error screens in stage order | P1. Today's code reached the capture whatever the reconcile's `ok` was, so this is also the invariant that no file operation is reordered |
| 2026-09-26 | **Milestone 2 adds `bool firstRun_` to `WorldsScreen`,** a member §5 does not list beside `stagesDone_` and `stageDrawn_`. It holds the answer to `store_ && !store_->AccountDirExists()`, asked once when the reconcile finishes, and stage 2 reads it instead of re-asking the disk | §4.3 asks the first-run question only inside the reconcile's `ok` branch but sets `stagesDone_ = 2` on the `!ok` branch as well, so continuing from a failed reconcile enters stage 2 on **every** console. Without a remembered answer, stage 2 would either construct a `FirstRunCaptureJob` on a console that is not on its first run — a file operation added, against §3.1 — or have to re-ask `AccountDirExists()`, which is a second file operation and, after a capture, a different answer. Flagged by milestone 1's report as the trap this milestone owns |
| 2026-09-26 | **The `FirstRunCaptureJob` is constructed in stage 2 rather than at the reconcile's finish,** which is where milestone 1 put it (row 4 above). §4.3 assigns the construction to stage 2's first entry; the two first-run log lines stay at the finish, where §4.3 puts them. Nothing observable changed: the lines are said in the same order on the same launches, and the job's first `Step()` still runs after them | §4.3, now that the stage machine exists to hold the construction. Milestone 1 constructed it early only because it had no stage index to enter |
