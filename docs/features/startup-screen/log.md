# Log — Startup Screen

Append-only. The history of how this item's artifacts got here; no artifact
narrates its own.

---

## 2026-09-26 — the item was raised, and its shape agreed

Raised by the developer after the worlds feature shipped and the startup screen
stopped being the thing under development: it still shows a streaming log of
technical lines on every launch, which reads as a debug build rather than an
app.

**No backlog row, correctly.** `docs/randomization-feature-spec.md` inventories
randomizer *settings*; this is UI work. The folder is `startup-screen/`, not
`NNN-<slug>`, matching `worlds/`, `font-atlas/` and `randomizer-settings-ui/`.

`proposal.md` was written from a code trace and refined across several rounds
with the developer before stage B ran. The shape it settled on: two states, a
loading state that is wordmark / rule / bar / the literal word `LOADING`, and an
error state that is the only one that holds, prompts or shows the log.

**The framing that decided it**, in the developer's words: someone stumbles on
this, thinks "Bloodborne randomizer for PS4, neat", installs it and runs it.
They expect to see a randomizer. That is what removed the first-run hold — a new
user has no context for a message about internals, so it cannot inform them,
only delay them.

**Three findings came out of the trace** and were recorded rather than acted on:
`live.log` is unlinked on every launch, so it only ever covers the current boot
(accepted as-is); a first run does `DiscoverSaveTitle` and `ReadContainer`
twice, which is its own item; and `AfrManager::Check` writes a scratch probe
file during `Refresh`, so "startup writes nothing" is not true.

---

## 2026-09-26 — stage B wrote the specification

`spec.md`, from `proposal.md` plus an independent trace. The seven agreed
decisions went straight to §10 dated the same day rather than being reopened.

**Three questions were put to the developer**, all answered as recommended:

* **The save-container read stays on the startup path.** It is the entire
  visible wait and its result has one consumer — the two "cannot activate"
  notes in the details pane — so moving it was a real option. Keeping it means
  the list is correct the moment it appears, and it makes the ~2 s the thing the
  loading screen exists to cover rather than an embarrassment.
* **With no signed-in player the error state offers `O EXIT`, not `X CONTINUE`**
  — the one condition of four that does. There is nothing to continue to.
* **`Application::Run` not returning after `Platform::Init` fails is out of
  scope** and is raised as its own small change.

**The spec corrected five errors in the proposal**, four of them the dispatching
session's own, each verified in the source before being accepted:
`ReconcileResult::error` is empty on the unknown-phase path, so a sentence built
from it would draw blank; three error conditions are really four; "the rail
lists nothing" with no user is wrong — it lists two inert rows, which is worse
than empty because it looks like it works; the proposal's 1px rule contradicts
`kRuleThickness = 2`; and the proposal contradicted itself on timing, §2.1
saying "well under a second" against §3.1's ~2 s.

**Three numbers could not be established** and are labelled inferences in the
spec: the ~2 s probe figure (15 MB/s from `technical-findings.md` applied to a
27 MB save, never timed), the first-run duration (that path has never run on
hardware at all), and `Refresh()`'s cost.

Status went to `QUESTIONS ANSWERED — awaiting developer approval`, then
**APPROVED** by the developer the same day.

---

## 2026-09-26 — stage C wrote the plan

`plan.md` (§1–§7 ≈ 404 lines, inside the ~400 guide) and `plan-evidence.md`.

**Two milestones**, both gated on hardware because nothing off-console can
observe the three things this feature is about — the bar, the frame ordering and
the timing:

1. **The outcome and the error state.** The `StartupProblem` enum and its
   sentence/prompt table, `startupBusy_`/`startupWait_` deleted, the four
   failures classified and routed, the no-signed-in-player case moved out of the
   constructor, `DrawProblem` on today's log geometry. Healthy launches still
   show the old log, which is what makes milestone 2 distinguishable on a TV.
2. **The loading state and the five-stage bar.** One read-only accessor on
   `WorldReconcileJob`, the stage machine with its draw-gated advance,
   `FinishStartup` deleted and split across stages 3 and 4, `DrawLoading`.

**One question, raised as a spec gap rather than a plan choice** and answered by
the developer: **when two stages fail on one launch, the error state is shown
once per failure, in stage order.** The four conditions are not mutually
exclusive — a failed reconcile followed by a failed capture is reachable — and
today's code sidesteps it by running the capture before it ever holds. Recorded
as plan **P8** and, because it is behaviour, also added to **spec §10**, so the
spec stays the authority.

**The planner found nothing wrong in the spec.** Every §4 "Fact" bearing on the
plan was re-verified: the five `State::Finish` sites, `ActionForPhase`'s
default, `!ok ⟺ error` for the capture, `containerKnown` being per-launch, and
`CannotActivate` being the probe's only consumer.

**One plan decision worth noting (P2).** The present-then-block gate is applied
uniformly to all five stages rather than only to the container read, because
stage 1 is entered in the *constructor* — the `WorldEditorScreen` pattern the
spec points at would still have run the sweep before frame 0.

**Step 7 reconciled four things.** The status line; §8 collapsed to a one-line
empty heading with the question moved to §9 as P8; a §3.3 hazard row that
deferred to "§8 Q1" now states the decision; and an obsolete stop condition —
"§8 Q1 is still unanswered when step 4 of milestone 1 is reached" — was removed,
since leaving it would have halted the implementer on a question already
answered. In `plan-evidence.md`, §E5.7's "raised as §8 Q1" became the answer,
and the rejected combined-error-screen alternative is now marked closed by P8.

**Risks recorded before approval.** The three inferred numbers are load-bearing
for the design but not for any milestone, verification step or geometry choice —
they appear only in the hardware table as things to *report*. And the worlds
report's unverified assumption that two seconds of blocking work inside one
`Update()` does not trip a watchdog is unchanged by this item, which neither
lengthens nor moves that call — but the loading screen is the first thing that
will make the symptom visible if the console does object.

**APPROVED** by the developer the same day, who asked for implementation to
begin immediately.

---

## 2026-09-26 — stage E implemented both milestones

**Both were built in one session at the developer's explicit instruction** —
"let's implement the full thing, I don't want to test each milestone
separately". This departs from `CLAUDE.md` §4 and from §7's milestone gate, and
the departure is the developer's call, stated in advance to the milestone 2
agent so it did not treat the missing hardware results as a stop condition. The
cost, recorded here rather than discovered later: **no part of this feature has
run on a PS4**, and a failure will have both milestones beneath it.

### Milestone 1 — the outcome and the error state

**Completed.** `StartupProblem` at `bbr` scope, `Mode` becomes
`{ Loading, Problem, Browse, ConfirmDelete }`, `startupBusy_`/`startupWait_`
deleted, `problem_` added, the four failures classified and routed, the
no-signed-in-player case moved out of the constructor, `DrawProblem` on today's
log geometry, and `parse_startup_problems()` plus seven cases in
`settings_ui_verify.py`.

**Four deviations**, plan §10. The load-bearing one is that **§5 and §7
contradict each other** about which milestone owns `DrawStartup` and
`FinishStartup`: §5's milestone-1 row says replace both then, §7 milestone 1
step 2 says keep `DrawStartup` "for now" and §7 milestone 2 deletes them. §7 was
followed, correctly — §7 is the ordered contract and §5 is a summary table. The
dispatching session confirmed the contradiction is real and should have caught
it at stage C step 7b. Also: `kLoadingWord` deferred to milestone 2 because
clang warned on it unused; an `EnterProblem()` helper for §4.4's three
statements across four call sites; and the capture set up on the reconcile's
failing paths too, so a double failure shows two screens in stage order (P8).

**Verification, re-run independently.** `.pkg` with no warnings;
`settings_ui_verify.py` 89/89, up from 82; `ui_scroll_verify.py`,
`worlds_verify.py` 97/97 and `font_atlas_verify.py` all passing and unedited;
all ten parity mirrors passing.

**A trap was flagged and deliberately left for milestone 2.** §4.3 sets
`stagesDone_ = 2` on the `!ok` path but checks `AccountDirExists()` only inside
the `ok` branch, so read literally, continuing from a failed reconcile on a
console that is not a first run would construct a `FirstRunCaptureJob` that
today's code never constructs — a file operation added, which §3.1 forbids.

### Milestone 2 — the loading state and the five-stage bar

**Completed.** The feature is code-complete. The ten `kLoad*` constants and
`kLoadingWord`; `UpdateStartup` restructured into the five stages;
`FinishStartup` deleted and split across stages 3 and 4; `X` in the error state
resumes the machine rather than skipping to the list; `DrawStartup` replaced by
`DrawLoading`; `WorldReconcileJob::ActingOnJournal()` added, read-only, one
line; five more verifier cases.

**The trap was fixed by remembering rather than re-asking.** `firstRun_` is set
once at the reconcile's finish, where milestone 1 computed its local
equivalent, and stage 2 opens `if (!firstRun_) { SetStagesDone(3); return; }`.
Re-asking `AccountDirExists()` at stage 2 was rejected as a second file
operation whose answer differs after a capture.

**Two further deviations**, §10 rows 5 and 6: `bool firstRun_` is a member §5
does not list, and `FirstRunCaptureJob` is now constructed in stage 2 per §4.3
rather than at the reconcile's finish where milestone 1 put it. Nothing
observable changed.

**Verification, re-run independently by the dispatching session.** `.pkg` with
no warnings. `settings_ui_verify.py` **94/94**. `ui_scroll_verify.py`,
`worlds_verify.py` 97/97 and `font_atlas_verify.py` passing. **The ten parity
mirrors, which the implementer reported as not run** on the grounds that
nothing here touches randomizer rules, were run by the dispatching session and
all pass. Invariants checked by hand: `DrawStartup` and `FinishStartup` are
gone bar one historical comment; `kStartupTitleScale`/`kStartupSubScale`
survive for `DrawConfirmDelete` as §3.1 requires; no SDL or orbis reference in
`WorldsScreen`; `ui_scroll_verify.py`'s `Worlds startup` entry is untouched;
and `ActingOnJournal()` is `return s_->stage >= 2;` and nothing else.

### A numbering error the dispatching session made and fixed

The developer's spec-gap decision was inserted into §9 as **P1**, colliding
with the planner's P1; the first correction renumbered it **P7**, which
collided with the planner's P7 — the table ran to seven, not six. It is now
**P8**, placed last, with every reference in `plan.md`, `plan-evidence.md`,
`log.md` and `implementation-report.md` updated. IDs verified unique.

### Awaiting hardware

The whole feature. `hardware-test-plan.md` is the ordered procedure, seven
parts plus an optional eighth, and it carries **three timings to record** — the
container read, the first-run capture and `Refresh()` — because all three are
inferences in the spec and no measurement of any exists.

**One failure mode is new only in visibility.** The worlds report's unverified
"two seconds of blocking work inside one `Update()` does not trip a watchdog"
is unchanged by this item, but it is now the fourth cell of a visible bar. If
the app dies or blacks out there, that exposure has become real and needs its
own investigation rather than being read as a loading-screen bug.
