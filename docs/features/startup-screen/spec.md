# Feature — Startup Screen

**Status: APPROVED** — approved by the developer 2026-09-26.

**Reference:** New feature. No reference-tool equivalent — the Windows tool is a
desktop application with no launch sequence to show.

**Plan:** `docs/features/startup-screen/plan.md` — added after this spec is
approved.

**Folder naming:** unnumbered, like `docs/features/worlds/`,
`docs/features/font-atlas/` and `docs/features/randomizer-settings-ui/`.
`docs/randomization-feature-spec.md` inventories randomizer *settings*; this is
UI work and has no backlog row.

**Origin:** `docs/features/startup-screen/proposal.md`, whose §2 shape the
developer agreed on 2026-09-26. Those agreements are recorded in §10. Every
factual claim the proposal made has been re-checked against the code for this
spec; where it was wrong, §4 says so.

---

## 1. What does this feature do?

It replaces the scrolling startup log the app shows while it is launching with a
loading screen, and keeps the log for the one case where the player needs it.

The app does a short sequence of work before it can show the player their
worlds: it clears up any debris a previous launch left behind, finishes any
world activation that was interrupted, files a pre-existing Bloodborne save into
Vanilla the first time it runs, reads the console's save container, and builds
the list. Today it narrates all of that on screen, line by line, and on some
paths it stops and waits for the player to press a button before going on.

That display was built while startup was the thing being debugged. On a healthy
console every line it prints says that nothing happened, in words that mean
something only to the person who wrote the code. Someone who has just installed
a Bloodborne randomizer expects to see a randomizer, not a report on its
internals.

So the startup screen becomes two things and nothing else: a **loading state**
that every launch shows and nobody needs to read, and an **error state** that
appears only when something actually went wrong, says what in one sentence,
shows the log, and waits.

Underneath the screen there is one correctness change. The app currently decides
what to show at the end of startup from two booleans that, between them, cannot
tell a failed save restore from a deleted temporary directory: both look
identical to the player. The feature replaces them with an explicit outcome that
is either "fine" or "failed, for this reason".

---

## 2. What does the player experience?

### The two states

| State | When it is shown | What the player sees | What it asks of them |
| ----- | ---------------- | -------------------- | -------------------- |
| **Loading** | Every launch, from the first frame, including the first run | The wordmark `BLOODBORNE RANDOMIZER`, a rule under it, a progress bar, and the word `LOADING` | Nothing. It never waits for input |
| **Startup problem** | Only on the failures listed below | `STARTUP PROBLEM`, one sentence saying what happened, the startup log as it is drawn today, and a prompt | It holds until the player answers the prompt |

The loading state shows the literal word `LOADING` throughout. It does not name
the phase it is in; naming the phase is the thing being removed. Nothing on the
screen moves except the bar.

**The bar steps, it does not animate.** It has five equal steps, one per stage of
the startup sequence, and it advances when a stage completes. A stage that does
not need to run on this launch counts as completed immediately, so the bar only
ever moves forward and the sequence always ends at full. There is no smoothing
between steps and none is wanted: a bar that jumps is honest about what it
knows.

**No hold, no prompt, no minimum display time on a healthy launch.** When the
work is done the worlds list replaces the loading screen directly.

**No hold on a first run either.** The first launch is the one where the app
files the player's existing Bloodborne save into the world called `VANILLA`, and
today it stops to tell them so. It will not. Nothing is destroyed, the save the
game is using is untouched, and the outcome is visible where a player would look
for it: `VANILLA` is on the list, holding a save. A new user should see the
randomizer, not acknowledge a message about internals they have no context for.

### How long the loading state is visible

On a healthy console, roughly two seconds, almost all of it one stage — reading
the console's save container (§4.3). Everything else is fast enough to pass in a
frame or two. On a first run it is longer, because the app also copies the
existing save twice — once to a safety backup, once into `VANILLA` — and the bar
sits at its third step for the whole of that with nothing else changing on
screen. That is accepted: a loading screen doing loading is what a player
expects on the slowest launch they will ever have.

The bar must be showing the step whose work is running while that work runs. It
must never freeze one step behind the work, which is what happens today (§4.4).

### What puts the app into the startup problem state

Four conditions, and nothing else:

| What happened | The sentence shown |
| ------------- | ------------------ |
| An interrupted activation was found, and finishing it failed | `AN INTERRUPTED ACTIVATION COULD NOT BE FINISHED` |
| An interrupted activation was found, recorded at a point no version of this app writes, so nothing was done to it | `AN INTERRUPTED ACTIVATION WAS NOT UNDERSTOOD AND WAS LEFT ALONE` |
| The first run could not file the existing save into `VANILLA` | `THE EXISTING SAVE COULD NOT BE FILED INTO VANILLA` |
| Nobody is signed in on the console, or the signed-in player has no account | `NO SIGNED-IN PLAYER` |

The sentence is the screen's, not the job's; the job's own wording and every
technical detail are in the log drawn beneath it, and in `live.log`. §4.1 shows
that these four are the complete set of failures the startup code can report,
and §4.2 that the fourth is a behaviour change.

**The prompt is `X CONTINUE` for the first three and `O EXIT` for the fourth**
(§10). The first three leave an app the player can still use — the worlds they
have are listed, and `DEFAULTS` works. The fourth does not: worlds are scoped to
the account, so with nobody signed in every action eventually refuses, and
offering to continue would be a dead end dressed up as a choice. Signing in on
the console is the real next step and the screen says so.

Everything else is silent and goes to the log only: partial backups or partial
world saves swept away, an interrupted activation found and correctly dealt with
at any point the app does write, a first run that worked, a first run that found
nothing to capture, and a console where Bloodborne has never made save data.

### What is removed

| Removed | Why it is safe to remove |
| ------- | ------------------------ |
| The hold after a startup that swept debris or dealt with an interrupted activation | Swept partials and a discarded staging tree touched nothing the player owns, and an interrupted activation that was successfully finished leaves the resulting world marked `ACTIVE` on the list it hands to. The hold was written while that transaction was unproven; it passed hardware testing on 2026-09-25 |
| The hold after the first-run capture | See above — the outcome is on the list |
| The running commentary and the per-phase status line | The text is `LOADING` |
| The scrolling log on a healthy launch | It is in `live.log` either way |

### What is unchanged

Returning to the `WORLDS` tab from the `DEFAULTS` tab does not show the loading
screen, and does not redo any startup work. That is already true and must stay
true. The error state's log keeps today's scrolling behaviour.

---

## 3. What does the existing randomizer do?

Nothing to borrow. The Windows reference tool is a WPF desktop application: it
has no console save data to reconcile, no interrupted-activation journal, no
first-run capture, and therefore no launch sequence worth showing. There is no
reference behaviour to match or deviate from, and no reference quirk to
preserve.

What the *port* does today is described in §4, because it is the thing being
changed.

---

## 4. What do we know?

Every reference below is to the working tree as of 2026-09-26.

### 4.1 The four error conditions, enumerated against the result structs

**Fact.** The startup sequence is driven from `WorldsScreen::UpdateStartup`
(`app/src/UI/WorldsScreen.cpp:394-450`), which runs two jobs and inspects two
result structs. Reading every place those structs are given a failure value:

* `ReconcileResult::ok` (`app/src/Game/WorldActivation.h:394`) is set only by
  `WorldReconcileJob::State::Finish`, at five call sites in
  `app/src/Game/WorldActivation.cpp:1240-1313`. Three set it true — no journal
  found, a staged tree discarded, an interrupted activation finished. Two set it
  false: a journal whose recorded phase maps to no action (`ActionForPhase`'s
  `default`, `WorldActivation.cpp:334-348`; the app writes phases 1 to 7), and a
  resumed activation that itself failed.
* A third `ok == false` site exists — no signed-in player when a journal needs
  resuming (`WorldActivation.cpp:1276`) — but it is **unreachable from this
  screen**, because `WorldsScreen`'s constructor only starts the job when the
  user is valid (`WorldsScreen.cpp:220`). It becomes reachable only if the job is
  ever run from somewhere else.
* `FirstRunCaptureResult::error` (`app/src/Randomizer/WorldStore.h:337`) is set
  only by `State::Fail`, which also sets `ok = false`; `State::Rollback` calls
  `Fail` (`WorldStore.cpp:1055-1080`). So "error non-empty" and "not ok" are the
  same condition. The successful paths set `note` instead, which is not a
  failure: no Bloodborne save data, no container yet, or a container holding no
  save files (`WorldStore.cpp:1113-1146`).
* `savedata::User::valid == false` (`app/src/Platform/SaveData.h:52-60`) has two
  causes, both in `ResolveUser` (`app/src/Platform/SaveData.cpp:467-495`): no
  foreground user, or no account id for the foreground user. In both cases
  `User::error` carries a sentence **with a raw hex return code appended** —
  which is why §2 specifies a screen-owned sentence and leaves the code to the
  log.

**Fact.** The proposal's three-condition table is therefore correct but coarse in
one place and misleading in another. `ReconcileResult::ok == false` is two
distinct situations that deserve different sentences, because one of them ("left
alone") changed nothing and the other may have left a half-finished activation.
And the implication that the reason can be read out of the result is wrong:
`ReconcileResult::error` is populated **only** on the resumed-activation path
(`WorldActivation.cpp:1310`). On the unknown-phase path it is empty and the
reason exists only in `StatusText()` and the log.

**Inference.** The four conditions in §2 are the complete set of failures the
current startup sequence can detect and report. Nothing else in the sequence
returns a failure at all: `ProbeContainer` explicitly treats an undiscoverable
save title as not an error (`WorldsScreen.cpp:250-257`), and `Refresh`
(`WorldsScreen.cpp:283-338`) has no error channel of any kind.

### 4.2 The no-signed-in-player path today

**Fact.** With no signed-in player, `WorldsScreen`'s constructor skips startup
entirely, writes one line to `live.log`, and drops straight to the worlds list
(`WorldsScreen.cpp:216-226`). `store_` is left null, so `Refresh` returns before
reading anything (`WorldsScreen.cpp:289`).

**Fact.** The resulting list is not empty, as the proposal states. `RailCount`
returns 2 whatever is on disk (`WorldsScreen.cpp:342`), so the player sees
`+ NEW WORLD` and `VANILLA` and nothing else, with no row marked `ACTIVE` and no
statement anywhere on the screen that the app could not identify them.

**Inference.** Every action on that list eventually refuses: activation planning
records the same invalid user (`WorldActivation.cpp:1082`). So the current
behaviour is a screen that looks like a working randomizer with no worlds, and
fails later without saying why. Routing it to the error state is the change, and
§10 settles what its prompt offers: `O EXIT`, because there is nothing to
continue to.

### 4.3 What the loading screen is actually waiting for

**Fact.** `FinishStartup` calls `ProbeContainer` synchronously
(`WorldsScreen.cpp:383`), which runs `savedata::DiscoverSaveTitle` — a sweep of
six candidate SKUs — then mounts the save container, walks it in full and
unmounts (`WorldsScreen.cpp:246-273`).

**Inference, not a measurement.** That is estimated at about two seconds, from
`technical-findings.md`'s measured ~15 MB/s applied to a ~27 MB save, because
`ReadContainer` sizes every file by reading it through to a short read — `st_size`
is unreliable on this kernel. The worlds implementation report lists this
estimate as explicitly unverified
(`docs/features/worlds/implementation-report.md:1346-1349`), along with the fact
that the first-run capture has never been run from a screen at all. **To
verify:** the first hardware launch, from the timing context around the
`SAVE DATA ... BLOCKS` line in `live.log`.

**Inference.** This makes the proposal's own §2.1 wrong where it says a healthy
launch is "well under a second of visible screen". The container probe runs on
every launch, not only the first — `WorldsSession::containerKnown` is per launch
and is not persisted (`WorldsScreen.h`, `WorldsScreen.cpp:248`) — so the loading
screen is visible for roughly the probe's duration every time. §2 states two
seconds for that reason.

**Fact.** The probe's result has exactly one consumer: `CannotActivate`, which
turns it into the two "cannot activate" sentences shown in the details pane
(`WorldsScreen.cpp:276-282`). Nothing else reads `containerExists`,
`containerBlocks`, `saveTitleId` or `saveDirName`. That made "do not do it during startup at
all" a real option, and §10 closes it: the read **stays**, so this ~2 s is
what the loading screen is for.

### 4.4 Why the bar would lag the work, and that the fix transfers

**Fact.** The frame loop is strictly serial: `Update`, then `Draw`, then
`Present`, once per iteration (`app/src/Application.cpp:78-84`). A screen cannot
present a frame from inside `Update`.

**Inference.** On a healthy launch the last frame drawn before the probe is the
frame in which the reconcile job finished; `FinishStartup` then blocks inside the
same `Update`. So whatever the bar shows at that instant is what the player
stares at for the probe's whole duration, one step behind the work being done.

**Fact.** The pattern the proposal points at does transfer, and the spec can rest
on it. `WorldEditorScreen::UpdateConfirm` sets a pending flag when the
confirmation opens and consumes it on the *next* `Update`
(`app/src/UI/WorldEditorScreen.cpp:846-860`, and the comment in `OpenConfirm` at
`:390-395`), precisely so that the frame saying `CHECKING` is presented before
the blocking work starts. Because `Draw` follows `Update` in the same iteration,
state set in `Update` N is on screen before `Update` N+1 runs. The same ordering
gives the bar an honest position during the probe.

**Inference.** One consequence to accept: the last visible bar position on a
healthy launch is the fourth step, not the fifth, because the frame after the
list is built draws the list. The bar need not be observed at full.

### 4.5 What the log holds, and where it goes

**Fact.** A healthy launch produces exactly four log lines, all through
`WorldsScreen::Say`, which appends to the on-screen log and writes to `live.log`
(`WorldsScreen.cpp:231-234`): `STARTING UP`; `SWEPT n PARTIAL BACKUP(S), n
PARTIAL WORLD SAVE(S)`; `NO ACTIVATION WAS INTERRUPTED`; and the
`SAVE DATA ... BLOCKS` line. Three of the four report that nothing happened.

**Fact.** The reconcile job's lines reach `live.log` **twice**: once from the
job's own `State::Say` under a `reconcile:` prefix
(`WorldActivation.cpp:1184-1187`), and again when `WorldsScreen::AddLines`
re-says them under a `worlds:` prefix. The world editor deliberately avoids this
for activation lines and says so (`WorldEditorScreen.cpp:906-916`). The startup
path never got the same treatment.

**Fact.** `FirstRunCaptureJob` has no `TakeLines`; its progress exists only as
`StatusText()`, and `WorldsScreen` synthesises log lines from the result once it
is done (`WorldsScreen.cpp:436-446`). So removing the live status line from the
screen removes the only per-file feedback a first run has, which is why §2 says
the bar sits still for the whole capture.

**Inference.** The on-screen log must therefore keep being accumulated on every
launch even though only the error state draws it: the error state is entered
after the lines that explain it have already been said.

### 4.6 Three facts about startup that must not be assumed away

**Fact.** `Platform::Init` unlinks `live.log` on every launch
(`app/src/Platform/Platform.cpp:34-37`), so the log only ever covers the current
boot. Accepted as-is (§10); recorded so it is not re-raised.

**Fact.** Startup writes to the AFR tree. `Refresh` calls `AfrManager::Check`,
which probes writability by creating and unlinking
`bbrandomizer_write_test.tmp` (`app/src/Game/AfrManager.cpp:145-152`,
`:185-198`). "Startup writes nothing" is not true and must not be claimed.

**Fact.** `Application::Run` does not return when `Platform::Init` fails: it
logs, calls `platform.Quit()`, and falls through into the frame loop
(`app/src/Application.cpp:48-53`). No startup screen of any design can appear on
that path, because there is no renderer. **Out of scope** by §10, and raised
as its own small change.

### 4.7 The five stages against the code

**Fact.** The sequence's real units of work, in order, are:

| Step | What runs | Where | On a healthy launch |
| ---- | --------- | ----- | ------------------- |
| 1 | Sweep partial backups and partial world saves; read the activation journal | `WorldActivation.cpp:1226-1258` (job stages 0 and 1, one per frame) | Runs, two frames |
| 2 | Act on the journal, including resuming an interrupted activation across many frames | `WorldActivation.cpp:1261-1313` (job stages 2 and 3) | Skipped |
| 3 | Capture the existing live save into `VANILLA` | `WorldStore.cpp:1101-1201` (five phases, multi-frame) | Skipped |
| 4 | Discover the save title and read the container | `WorldsScreen::ProbeContainer` | Runs, dominates (§4.3) |
| 5 | Build the worlds list | `WorldsScreen::Refresh` | Runs |

**Inference.** The proposal's five-stage model is the right one. Its stages 1 and
2 each merge two of the job's internal stages, which is correct at this altitude:
the merged pairs are "find out" and "act", and the split inside the job is not
something the bar should expose. Steps 2 and 3 are the only two that can be
skipped, and both are skipped on every launch after the first on a console where
nothing was interrupted.

**Inference.** Step 5's cost is small but not zero — a writability probe, a
manifest read, a directory listing, and one revision read per world — so it is
worth a step of its own rather than being folded into step 4. Not measured; it is
tens of small file reads and no container mount.

---

## 5. Terminology

* **Startup sequence** — the five steps of §4.7, run once per launch of the app,
  not once per visit to the `WORLDS` tab.
* **Loading state / startup problem state** — the two things this screen can be.
  They replace the four display situations the current two booleans encode.
* **Stage** — one of the five steps, and one step of the bar. A stage is
  *completed* when its work finishes and *skipped* when this launch does not need
  it; both advance the bar.
* **The log** — the lines the startup sequence accumulates. Drawn only by the
  error state; always written to `live.log`.
* **Interrupted activation** — an `activation.journal` left on disk by a launch
  that died mid-transaction. Reconciling it is step 1 and 2's job.

---

## 6. Scope

### In scope

* The screen shown between launch and the worlds list, in its two states.
* The five-stage bar, and which stage each unit of startup work advances.
* Running the container read after the frame that shows its stage, so the bar is
  never a step behind the work (§4.4).
* Replacing the two booleans that encode the end of startup with one outcome that
  is either fine or failed-with-a-reason.
* Routing the no-signed-in-player case to the error state (§4.2), where its
  prompt is `O EXIT` rather than `X CONTINUE` (§10).
* Keeping the log accumulating on every launch, and keeping the error state's log
  drawing and scrolling as they are today.

### Out of scope

* **The duplicated work on a first run.** `FirstRunCaptureJob` phase 0 calls
  `DiscoverSaveTitle` and `ReadContainer`, and `FinishStartup` then calls
  `ProbeContainer`, which does both again — twelve save-data searches and two
  container walks on the slowest launch the app has. It is a pure performance win
  with no design content and belongs in its own item. A planner must not pick it
  up here.
* **`AfrManager::Check` writing a scratch file during `Refresh`** (§4.6). Known,
  harmless, and not this item's business.
* **The duplicated `reconcile:` / `worlds:` lines in `live.log`** (§4.5). Noted so
  it is not mistaken for a bug introduced here.
* **`live.log` being wiped every launch** (§10).
* **`Application::Run` not returning after `Platform::Init` fails** (§4.6). A
  real defect, confirmed, and out of scope by §10 — no startup screen can
  appear on that path because there is no renderer. Raised as its own small
  change; a planner must not pick it up here.
* Activation's own progress log, the world editor, the settings screens, the
  randomizer and anything it writes.
* Chalice dungeons remain out of scope.

---

## 7. Constraints and decisions

* **Nothing in the startup sequence's work changes.** The same sweeps, the same
  reconciliation, the same capture, the same container read, in the same order.
  This item changes what is drawn and when it is drawn relative to the work, and
  how the outcome is represented — not what the app does to the disk.
* **Save safety is untouched.** No file operation is added, removed or reordered.
* **No blocking work may run before the frame that shows its stage has been
  presented** (§4.4). This applies to the container read, and to anything else
  later found to block.
* **The startup sequence still runs once per launch**, and returning from the
  `DEFAULTS` tab must not re-run it or re-show the loading screen.
* **The error state holds; nothing else does.** No state other than the error
  state may wait for input, and no state may have a minimum display time.
* **The bar is monotonic.** Five equal steps, fixed denominator, no step ever
  retreats, and a skipped stage advances it.
* **On-screen text is drawn from named string constants** in the screen's
  anonymous namespace, so the existing UI verifier can parse and measure them
  against the atlas — that is how `settings_ui_verify.py` reaches every other
  string on this screen.
* **Rules are 2px**, as everywhere else in this app; the proposal's "1px rule" is
  a sketch annotation, not a constraint (`kRuleThickness`,
  `SetupDefaultsScreen.cpp:77`).
* **Whole words and no decimal points** in anything drawn: the 8x8 fallback font
  has no `.` glyph and advances past characters it cannot draw.
* **The error sentence is the screen's**, one line, with the job's own wording and
  any return code left in the log beneath it (§4.1).

---

## 8. How will we know it works?

### Acceptance criteria

1. On a launch where nothing has gone wrong, the player sees only the loading
   state, and the worlds list replaces it with no input from them.
2. The loading state shows the wordmark, a rule, a bar and the word `LOADING`,
   and no other text, at every point in the sequence.
3. The bar never moves backwards, shows five equal steps, and is at the step
   whose work is currently running — in particular during the container read.
4. Each of the four conditions in §2 produces the startup problem state with its
   own sentence, the log beneath it, and a prompt; the app does not proceed until
   the player answers it.
5. None of the silent situations in §2 — swept debris, an interrupted activation
   correctly dealt with, a first run that worked, a first run with nothing to
   capture, a console with no Bloodborne save data — stops or prompts.
6. A first run reaches the worlds list with `VANILLA` present and holding the
   captured save, without asking the player anything.
7. Every line the startup sequence says still reaches `live.log`, whichever state
   the screen ends in.
8. Returning to `WORLDS` from `DEFAULTS` shows the list immediately, with no
   loading state and no repeated startup work.

### Automated testing

`settings_ui_verify.py` already parses this screen's named strings and geometry.
It should be extended to cover the new screen: that the wordmark, `LOADING`, the
error title, all four error sentences and the prompt fit the widths they are
drawn into at their scales, measured from the font atlas rather than character
counts, and that the loading state's vertical stack — wordmark, rule, bar, word —
does not overlap under the same ink-box rule the other screens use. The error
state's log band is unchanged and is already covered by `ui_scroll_verify.py`.

What a verifier cannot do is judge any of this; it can only prove that nothing is
clipped or overlapping.

The mapping from the four conditions of §2 to the sentences they show is worth
asserting somewhere that does not need a console, since §4.1 shows it is derived
from struct fields whose failure values are not all reachable the same way.

### Hardware testing

The PS4 is the authority for all of it.

* **Healthy launch.** The loading screen appears, the bar steps, the list
  replaces it. Expected: unremarkable, around two seconds. Report how long the
  bar sits on its fourth step — that is §4.3's unverified estimate.
* **First run**, on a console with a real Bloodborne save and no worlds folder.
  Expected: the loading screen throughout, no prompt, then a list with `VANILLA`
  holding the save. The failure that matters is any hold or message.
* **A hand-written `activation.journal` at a phase the app writes.** Expected:
  still unremarkable, no prompt, and the resulting world marked `ACTIVE`.
* **A hand-written journal at a phase the app does not write.** Expected: the
  startup problem state with the "not understood" sentence, and the journal still
  on disk afterwards.
* **No signed-in player.** Expected: the startup problem state, naming the
  reason and offering `O EXIT` — not `X CONTINUE` (§10).
* **Failure cases to watch for:** the bar freezing a step behind the work; a black
  screen during the container read; the list appearing before the capture has
  finished; any prompt at all on a healthy launch.

---

## 9. Open questions

*Deliberately empty.* All three questions this spec raised were answered by
the developer on 2026-09-26 and are recorded in §10 — the container read stays
on the startup path, no-signed-in-player offers `O EXIT`, and the
`Application::Run` defect is out of scope. The heading is kept so the gap
between §8 and §10 cannot be mistaken for an editing accident.

---

## 10. Decisions

| Date       | Decision |
| ---------- | -------- |
| 2026-09-26 | The startup screen has two states and no others: a loading state and an error state |
| 2026-09-26 | The loading state is the wordmark, a rule, a bar and the literal word `LOADING`; it never names the phase |
| 2026-09-26 | The bar steps once per stage and does not animate between steps; smoothness is explicitly not a goal |
| 2026-09-26 | A healthy launch has no hold, no prompt and no minimum display time; the worlds list replaces the loading screen directly |
| 2026-09-26 | A first run does not hold either. Someone who has just installed this expects to see the randomizer, not to acknowledge a message about internals they have no context for; nothing is destroyed and the outcome is visible on the list |
| 2026-09-26 | The error state is the only state that holds, the only one that prompts, and the only one that shows the log; it reuses today's log drawing rather than introducing a new design |
| 2026-09-26 | `live.log` being wiped by `Platform::Init` on every launch is accepted as-is and is not to be reopened |
| 2026-09-26 | The duplicated `DiscoverSaveTitle`/`ReadContainer` on a first run, and `AfrManager::Check` writing a scratch probe file during `Refresh`, are both out of scope for this item |
| 2026-09-26 | **The save-container read stays on the startup path.** Its result has one consumer — the two "cannot activate" notes in the details pane — but a list whose warnings appear after the fact, or change under the cursor, is worse than a loading screen that waits for them. This is where milestone 4 deliberately put it. The ~2s it costs is therefore the thing the bar exists to cover, and the five stages and the frame ordering are the substance of this item rather than incidental to it |
| 2026-09-26 | **With no signed-in player the error state offers `O EXIT`, not `X CONTINUE`** — the one condition of the four that does. Worlds are account-scoped and every action eventually refuses, so there is nothing to continue to; signing in on the console is the real next step. The other three conditions leave a usable app behind and keep `X CONTINUE` |
| 2026-09-26 | **When more than one startup stage fails on the same launch, the error state is shown once per failure, in stage order**, each with its own sentence and prompt. The four conditions above are not mutually exclusive — a failed reconcile followed by a failed first-run capture is reachable — and "one sentence" describes each screen, not each launch. Raised as a spec gap by the plan and answered here so the spec stays the authority on behaviour |
| 2026-09-26 | **`Application::Run` failing to return after `Platform::Init` fails is out of scope** and is raised as its own small change. It is a real defect — `Run` logs, calls `Quit()`, and falls into the frame loop with a dead platform — but no startup screen can appear on that path, because there is no renderer, and nothing in this feature makes it better or worse |

---

## Appendix — research notes for stage C

*Not part of the specification, and not covered by the developer's approval.
These are pointers from the spec investigation to save stage C a search. Verify
anything here before relying on it.*

**Where the behaviour lives**

* `app/src/UI/WorldsScreen.cpp` — `UpdateStartup`, `FinishStartup`,
  `ProbeContainer`, `DrawStartup`, `Say`/`AddLines`; the whole of this item's
  screen work is in these five plus the `Mode` enum in `WorldsScreen.h`.
* `app/src/UI/WorldEditorScreen.cpp` — `UpdateConfirm`'s pending-plan flag and
  `OpenConfirm`'s comment; the present-then-block pattern to copy, with the
  reasoning already written down.
* `app/src/Application.cpp` — `Run`'s frame loop; the one place that fixes
  `Update`/`Draw`/`Present` order, and where `WorldsSession` is owned.
* `app/src/Game/WorldActivation.cpp` — `WorldReconcileJob::Step`,
  `State::Finish`, `ActionForPhase`; every reconcile outcome is decided here.
* `app/src/Randomizer/WorldStore.cpp` — `FirstRunCaptureJob::Step` and
  `State::Fail`/`Rollback`/`Finish`; the capture's phases and its only failure
  channel. Both jobs expose a `Progress()` the screen currently ignores.
* `app/src/UI/Controls.h` — `Palette`, `DrawCenteredLabel`, `ListLayout`,
  `DrawScrollHints`; there is no bar primitive, and `Renderer::FillRect` is what
  every rule and swatch in this app is built from.
* `app/tools/settings_ui_verify.py` — `parse_named_strings`, `parse_geometry`,
  `parse_list_layout`; how strings and constants get out of `WorldsScreen.cpp`
  and into a width check.

**Worth checking early**

* Whether the reconcile job's two internal stages per bar step can both complete
  without a frame between them — the job steps once per `Update`, so the mapping
  from job stage to bar step is not one-to-one.
* Whether anything other than `FinishStartup` can reach `Mode::Browse`, since the
  new outcome has to be decided before that switch on every path.
* `WorldsSession` is the only state that survives a tab switch; anything the new
  outcome needs to outlive the screen has to live there.
* `docs/features/worlds/implementation-report.md` §5 and §6 list what the worlds
  milestones left unverified on hardware, including this screen's timing and the
  first-run path.

**Dead ends already walked**

* Looking for a shared progress-bar control to reuse: there is none anywhere in
  `app/src/UI`.
* Looking for a reference-tool startup screen: the WPF app has no equivalent.
* Trying to read a failure reason out of `ReconcileResult::error` for every failed
  reconcile: it is populated on only one of the two reachable failure paths
  (§4.1).
* Treating `FirstRunCaptureResult::note` as an error signal: it is set on success
  paths.
