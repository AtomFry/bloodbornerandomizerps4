# Plan Evidence — Startup Screen

**Plan:** `docs/features/startup-screen/plan.md`

**Spec:** `docs/features/startup-screen/spec.md`

---

> **This document is the investigation behind the plan, not instructions.**
> It is revisable. There is no length budget.

---

## E1. Reference trace

There is none, and the spec is right about why (§3). `reference/` is a WPF
desktop application. It has no console save container to read, no
`activation.journal`, no first-run capture and no account-scoped world store,
so it has no launch sequence and no launch screen. Searching `reference/` for a
splash, a progress window or a startup dialog finds nothing: the only two XAML
files in the tool are `reference/Randomizer/App.xaml` and
`reference/Randomizer/Window1.xaml`, and the second is the randomizer's whole
UI rather than a prelude to it.

Consequently **`CLAUDE.md` §7's "match the reference before improving it" has
nothing to bind here**, and there is no reference quirk to preserve. The
behavioural authority for this item is the spec alone, and the *port's* current
startup path — traced below — is the thing being changed rather than a thing to
match.

---

## E2. What exists in the port

Every line reference is to the working tree of 2026-09-26.

### E2.1 The frame loop

`Application::Run` (`app/src/Application.cpp:76-99`) is strictly serial:

```
current->Update(...); current->Draw(...); platform.Present();
```

confirming spec §4.4. A screen cannot present from inside `Update`. State set
during `Update` N is on screen after `Present` N, i.e. before `Update` N+1. This
is the whole basis of the plan's `stageDrawn_` gate.

`WorldsSession worldsSession` is declared at `Application.cpp:64` and passed to
every `WorldsScreen`, so it is the only startup state that survives a tab
switch — confirming the spec appendix. Nothing in this plan needs to add a field
to it: `stagesDone_` and `problem_` are per-launch *and* per-screen, and the
only launch on which the screen is rebuilt mid-startup is none.

`Run` also does not return when `Platform::Init` fails (`:48-53`) — confirmed,
and out of scope by spec §10 D11.

### E2.2 The current startup path

* Constructor (`WorldsScreen.cpp:211-229`): `ResolveUser()`, then a `WorldStore`
  if valid. If `!session_.startupDone && user_.valid` it enters
  `Mode::Reconciling` and says `STARTING UP`; **otherwise** it logs the user
  error (if any), enters `Mode::Browse` and calls `Refresh()`.
* `UpdateStartup` (`:390-450`) runs the two jobs, one `Step()` per `Update`.
* `FinishStartup` (`:383-388`): `ProbeContainer()`, `startupDone = true`,
  `Mode::Browse`, `Refresh()` — all inside one `Update`.
* `DrawStartup` (`:699-745`): the wordmark at `kStartupTitleY` scale 5,
  `STARTING UP`/`FIRST RUN` at `kStartupSubY` scale 4, the scrolling log at
  `kProgressLayout`, a live `StatusText()` line while `startupBusy_`, the scroll
  hints, and the two footer lines while `startupWait_`.

The two booleans the spec wants removed are `startupBusy_` and `startupWait_`
(`WorldsScreen.h:172-173`). Between them they encode: nothing running; a job
running; finished-and-quiet; finished-and-worth-reading. They cannot distinguish
a failed capture from a swept staging tree — both produce `startupWait_ = true`
and an identical `X CONTINUE`.

Note that the wordmark, `STARTING UP`, `FIRST RUN`, `UP DOWN SCROLL` and
`X CONTINUE` are **inline literals** in `DrawStartup`, not named constants, and
are therefore the only strings on this screen `settings_ui_verify.py` cannot
see. Spec §7's "named string constants" requirement is a change from the status
quo, not a restatement of it.

### E2.3 `WorldReconcileJob`

`app/src/Game/WorldActivation.cpp:1174-1311`. Internal `stage`: 0 sweep, 1 read
journal, 2 act, 3 resume, 4 done. `Step()` does at most one of those per call
and returns, so stages 0 and 1 are one frame each on every launch.

Exits, all through `State::Finish(why, ok)`:

| Where | `ok` | Meaning |
| ----- | ---- | ------- |
| `:1245` | true | no journal |
| `:1267` | **false** | `ActionForPhase` returned `Nothing` — a phase the app does not write |
| `:1274-1279` | true | staging discarded, or nothing had changed yet |
| `:1284` | **false** | no signed-in player while resuming |
| `:1307` | true | the interrupted activation was finished |
| `:1310` | **false** | the interrupted activation could not be finished |

`ActionForPhase` (`:334-347`) maps 1–4 to `DiscardStaging`, 5 to `RestoreTree`,
6 to `ResumeSaveSwap`, 7 to `FinishCommit`, and everything else to `Nothing`.
This confirms spec §4.1 exactly, including that the third `ok == false` site is
unreachable from this screen (`WorldsScreen.cpp:220` only constructs the job
when `user_.valid`).

`ReconcileResult::error` is assigned at `:1309` only — the resumed-activation
path. On the unknown-phase path it stays empty. Confirmed; this is why the plan
classifies by `hadJournal` and `action`.

The job exposes `Done`, `TakeLines`, `StatusText`, `Progress` and `Result`. It
does **not** expose its stage. `Progress()` (`:1217-1222`) returns `0.1f * stage`
below stage 4 — a stage number is recoverable from it, but only by dividing a
float by a magic constant.

### E2.4 `FirstRunCaptureJob`

`app/src/Randomizer/WorldStore.cpp:1041-1199`. Phases 0 look, 1 create, 2 back
up, 3 file it, 4 done. `State::Fail` sets `ok = false` **and** `error`
(`:1066-1073`); `State::Rollback` calls `Fail` (`:1076-1081`); `State::Finish`
sets `ok = true` and never touches `error`. So `!ok` ⟺ `error` non-empty,
exactly as spec §4.1 says. `note` is set only on `Finish` paths
(`:1113-1146`) — three success cases, none of them a failure.

No `TakeLines`. Its only progress channel is `StatusText()`, which the new
loading state does not draw — so the capture is, as the spec says, a stage the
bar sits on with nothing else changing.

### E2.5 `ProbeContainer` and `Refresh`

`ProbeContainer` (`WorldsScreen.cpp:247-273`) guards on
`session_.containerKnown || !user_.valid`, then `DiscoverSaveTitle` (six
candidate SKUs) and `ReadContainer`. `containerKnown` is per launch and never
persisted — confirming spec §4.3's correction of the proposal: the probe runs on
every launch, not only the first.

Its result is read only by `CannotActivate` (`:276-282`). Nothing else in
`app/src` reads `containerExists`, `containerBlocks`, `saveTitleId` or
`saveDirName` — confirmed by grep across `app/src`. Spec §10 D9 keeps it on the
startup path regardless.

`Refresh` (`:284-338`) does: `store_->Load(vanilla)`, `store_->List()`,
`AfrManager::Check` (which creates and unlinks `bbrandomizer_write_test.tmp`),
`ReadManifest`, `DeriveActive`, then one `CurrentRevision` read per world. Tens
of small file operations, no container mount, unmeasured.

### E2.6 The present-then-block precedent

`WorldEditorScreen::OpenConfirm` (`:396-401`) sets `planPending_` during
`Update` N; `UpdateConfirm` (`:846-861`) consumes it and does the blocking
`PlanActivation` on `Update` N+1. The comment at `:391-395` states the reasoning
in the same terms the spec uses. The pattern transfers, with one wrinkle the
editor does not have: this screen's *first* stage is entered in the constructor,
before any `Update`, so a flag set at entry and consumed on the next `Update`
would do the sweep on frame 0 — before anything had been drawn. That is why the
plan gates on "has been drawn" (set in `Draw`) rather than on "one `Update` has
passed". `DrawStartup` already writes `logScroll_` at `:718`, so a `Draw` that
writes screen state is not a new shape in this file.

### E2.7 The verifiers

`app/tools/settings_ui_verify.py` (1483 lines, 82 cases, all passing). Relevant
helpers: `parse_geometry(filename)` — every file-local `const int kFoo = N;` plus
`kRuleColor`; `parse_named_strings(filename)` — every
`const char* const kFoo = "...";`, concatenated literals joined;
`parse_list_layout`; `width(text, scale)`, `ink_top`, `ink_bottom`, `wrap` — all
measured against the real font atlas, never character counts. Its worlds pass
selects strings by prefix: `kNote*`, `kHelp*`, `kDelete*`/`kRefuse*`, plus
`kFooterLine` and `kRowNewWorld`. The plan's new prefixes — `kWordmark`,
`kLoadingWord`, `kProblem*`, `kPrompt*` — collide with none of those, so no
existing case changes meaning.

`app/tools/ui_scroll_verify.py` (361 lines, all passing) carries one row per
scrolling band. `("Worlds startup", 300, 70, 920, 3, 50, 18, 200, 4,
SCREEN_H - 130, 2)` is the startup log: `kProgressLayout`, scale 3, 18 rows
worst case, a heading at y 200 scale 4 above it, a two-line footer at
`SCREEN_H - 130`. Its comment already says the worst case it models is the
*finished* state — no live status line — which is precisely the error state.
Keeping the error state on those numbers is what lets this file stay untouched;
that is why "editing it to pass" is a stop condition in the plan.

`app/tools/worlds_verify.py` is in the tree and passing. It pins the worlds
rules — `DeriveActive`, the store's layout, the activation transaction — none of
which this item touches, so it is a regression run only.

---

## E3. Alternatives considered

| Approach | Why rejected |
| -------- | ------------ |
| Keep `startupBusy_`/`startupWait_` and add a third boolean for "failed" | This is the defect the spec names. Three booleans encode eight states, of which four are nonsense, and the failure reason still has nowhere to live |
| Derive the reconcile bar step from `WorldReconcileJob::Progress()` (`0.1f * stage`) | Recovers an integer from a float by dividing by an undocumented constant. Any change to `Progress`'s curve — a purely cosmetic change elsewhere — would silently move the bar |
| Derive it from `Result().hadJournal` before `Done()` | Works today and needs no header change, but it reads a "result" mid-flight, which nothing in the codebase documents as legal. The one-line accessor is cheaper to keep correct |
| Expose `int Stage()` on `WorldReconcileJob` | Publishes an internal counter the screen would then have to re-interpret. `ActingOnJournal()` names the one boundary the bar actually needs |
| One bar step per reconcile *job* stage (seven steps total) | Contradicts spec §4.7 and D3. The split inside the job is "sweep / read" and "decide / do", which is not a distinction a player can act on |
| Gate only the container read, per the literal reading of spec §7 | Leaves the sweep running before frame 0 (a black first frame) and leaves `Refresh` running in the same `Update` as the probe. The uniform gate costs one frame per stage and removes a whole class of "why is the bar behind" bugs |
| Run `Refresh()` in the same `Update` as `ProbeContainer()`, so the fifth step is never drawn | Spec §4.4 accepts that the fifth step need not be *observed at full*, which the uniform gate also satisfies (the frame after `Refresh` draws the list). But it would put unmeasured blocking work behind a bar showing the previous step, which is the exact failure §7 forbids |
| A single filled bar `stagesDone_ / 5` wide | Reads as a continuous measure and invites smoothing, which D3 rules out. It also cannot show *which* step is running, only how many are done — so B5 would be satisfied only by inference from the fill edge |
| Add a progress-bar control to `Controls.h` | `Controls.h`'s own comment says to grow it only when a real screen needs something it does not cover. One screen with one bar is not that; a second user of it is |
| Move the container read off the startup path | Closed by spec §10 D9 |
| Hold on the error screen and run the remaining stages there when `X` is pressed | Puts the ~2 s container read behind a screen with no bar, reintroducing exactly the freeze the item exists to remove |
| Collect failures and show one combined error screen at the end | Delays the report of a reconcile failure until after a first-run capture has run on a console whose activation state is already suspect. Closed by P8 |
| Put the sentence/prompt mapping in a `switch` | A `switch` over a private nested enum cannot be reached from the anonymous namespace, and a `switch` is much harder for the verifier to assert *total*. A table at file scope solves both |

---

## E4. Measurements

All text widths are from the real font atlas through `settings_ui_verify.py`'s
own `width()`, i.e. the same measurement the app's `Renderer::TextWidth` makes.

Command used for every row of the first table:

```
cd app/tools && python -c "
import settings_ui_verify as v
v.load_atlas()
print(v.width('<text>', <scale>))"
```

| Quantity | Value | Source |
| -------- | ----: | ------ |
| `BLOODBORNE RANDOMIZER` at scale 5 | 760 px | atlas |
| `LOADING` at scale 4 | 206 px | atlas |
| `STARTUP PROBLEM` at scale 5 | 513 px | atlas |
| `AN INTERRUPTED ACTIVATION COULD NOT BE FINISHED` at scale 4 | 1224 px | atlas |
| `AN INTERRUPTED ACTIVATION WAS NOT UNDERSTOOD AND WAS LEFT ALONE` at scale 4 | **1685 px** — the widest of the four, 88 % of the 1920 surface | atlas |
| `THE EXISTING SAVE COULD NOT BE FILED INTO VANILLA` at scale 4 | 1209 px | atlas |
| `NO SIGNED-IN PLAYER` at scale 4 | 472 px | atlas |
| `UP DOWN SCROLL` at scale 3 | 284 px | atlas |
| `X CONTINUE` at scale 3 | 207 px | atlas |
| `O EXIT` at scale 3 | 107 px | atlas |
| ink box at scale 5 | y+16 … y+73 | `v.ink_top/ink_bottom` |
| ink box at scale 4 | y+14 … y+58 | as above |
| ink box at scale 3 | y+9 … y+44 | as above |

The four sentences all fit one line on 1920 at scale 4 with the widest leaving
117 px of margin each side. This is why the plan does not wrap them and the spec
can call the sentence "one line".

**The worst-case startup log line**, measured the same way, so the error state's
log cannot overflow:

| Line | Width at scale 3 |
| ---- | ---: |
| `NO SIGNED-IN PLAYER - GETFOREGROUNDUSER 0X80960003` | 921 px |
| `NO ACCOUNT ID FOR THE SIGNED-IN PLAYER - 0X80960003` | 902 px |
| `THE JOURNAL'S PHASE IS NOT ONE THIS APP WRITES - LEFT ALONE` | 1031 px |
| `SWEPT 0 PARTIAL BACKUP(S), 0 PARTIAL WORLD SAVE(S)` | 872 px |

All comfortably inside 1920. The log is centred on the whole screen, so its
budget is the surface.

**The loading stack**, derived rather than measured, with the arithmetic shown
so the reviewer can re-derive it:

| Element | Draw y | Ink band | Note |
| ------- | -----: | -------- | ---- |
| wordmark, scale 5 | 390 | 406 … 463 | 760 px wide, centred |
| rule, 2 px | 480 | 480 … 482 | x 460 … 1460 |
| bar, 28 px | 530 | 530 … 558 | same x band |
| `LOADING`, scale 4 | 610 | 624 … 668 | 206 px wide, centred |

Stack ink spans 406 … 668, whose midpoint is 537 against a screen midpoint of
540 — the block reads as vertically centred without needing a magic offset.
Every gap is positive, so the ink-box overlap case in the plan's §6 passes by
construction and then keeps passing if someone edits a constant.

Bar arithmetic: `kLoadBlockW - 4 * kLoadCellGap = 1000 - 80 = 920`, and
`920 / 5 = 184` exactly. Chosen so the five steps are *equal* with no remainder
pixel — a bar whose last cell is 1 px wider than the others is a bar that looks
like it is still moving. `kLoadBlockX = (1920 - 1000) / 2 = 460`.

**Existing verifier totals**, so a regression is visible:

| Command | Result on 2026-09-26 |
| ------- | -------------------- |
| `python app/tools/settings_ui_verify.py` | `82/82 passing` |
| `python app/tools/ui_scroll_verify.py` | `all geometry and scroll properties PASSED` |
| `python app/tools/worlds_verify.py` | `97/97 passing`, `all checks passing` |

**Not measured, and the plan says so wherever it leans on them:** the container
probe's ~2 s, the first-run total, and `Refresh()`'s cost. See §E7.

---

## E5. Risk analysis

### E5.1 The bar lagging the work

The whole point of the item. On a healthy launch today, the last frame drawn
before `FinishStartup` is the one in which the reconcile job finished; the probe
then blocks inside the same `Update`, so the player stares at a stale frame for
the probe's whole duration. With five stages, the same failure would occur at
every boundary rather than only once.

The `stageDrawn_` gate makes the failure structurally impossible rather than
individually avoided: work is unreachable until the stage has been through
`Draw`. The cost is one frame (~16 ms at 60 Hz) per stage boundary, five
boundaries, ~83 ms added to a ~2 s launch — under 5 %, and only on the launch
path.

Residual risk: someone later adds a sixth stage and forgets the gate. Bounded by
routing every `stagesDone_` write through one helper (milestone 2 step 2) and by
the stop condition about needing a different call site.

### E5.2 Driving the reconcile bar steps off the wrong signal

The reconcile job's four internal stages map to two bar steps, and the mapping
is not derivable from `Done()` alone. `ActingOnJournal()` returns
`s_->stage >= 2`, which is true from the moment the journal has been read and an
action chosen, and stays true through the resume and through `Done`. The plan's
stage-0 body tests `Done()` *first*, so a job that finishes in the same `Step`
that set stage 2 (the `Nothing` and `DiscardStaging` actions, which `Finish`
immediately) correctly completes both bar steps at once rather than parking on
step 2.

Consequence of getting it wrong: on a healthy launch the bar would either skip
step 1 entirely or sit on step 2 with nothing to do. Cosmetic, but it is the one
thing the hardware test for the healthy launch is looking at.

### E5.3 `ReconcileResult::error` is empty on one failure path

Assigned at `WorldActivation.cpp:1309` only. A sentence built from it would be
blank on the unknown-phase path — which is the *more* likely of the two to be
hit, because it is what a journal from a future or corrupt build produces. The
spec's own §4.1 caught this; the plan classifies from `hadJournal` and `action`,
both of which are set before either failure is reachable. The reason text is not
lost: `State::Finish` `Say`s it, so it is in `lines_` and drawn under the
sentence.

### E5.4 `note` is not an error

`FirstRunCaptureResult::note` carries the three benign "nothing to capture"
outcomes. Treating it as a failure would put the error state in front of every
console that has never run Bloodborne — a first-launch experience of an error
screen saying nothing is wrong. Branching on `result.ok` avoids it; the spec
lists this as a dead end already walked, and the trace at §E2.4 confirms `!ok`
and non-empty `error` are the same condition.

### E5.5 The container read's duration and the watchdog

`docs/features/worlds/implementation-report.md:1346-1349` lists the ~2 s figure
as unverified, and `:1354-1355` lists "two seconds of blocking work inside one
`Update()` does not trip a watchdog" as unverified as well — nothing in this app
has blocked that long in a frame before.

This item **does not change that exposure**: it neither lengthens nor shortens
the blocking call, and the spec forbids moving it. What it changes is what the
player sees while it happens. If the console does object, it will object
identically with or without this change, and the loading screen makes the
symptom easier to describe. The plan's only response is to make the hardware
test report the duration, because this is the first launch on which the figure
can be observed at all.

### E5.6 The error state's log row count

Today `logRows = startupBusy_ ? slots - 1 : slots` — one slot is reserved for
the live status line. With `startupBusy_` gone and no live line, the error state
always draws `slots` rows, which is 18 for `kProgressLayout` and is exactly the
worst case `ui_scroll_verify.py`'s `Worlds startup` row already models. So this
is a simplification that *reduces* the gap between the model and the code. The
risk is the opposite one: an implementer keeping the `- 1` "to be safe" would
make the verifier's model wrong in the other direction.

### E5.7 Two failures on one launch

Reachable: a journal at an unknown phase on a console that has never run this
app for this account, so the capture also runs and can also fail. Today only
one screen appears, after both. Under the plan each failure holds in turn. The
risk of the recommended answer is a player pressing `X` twice; the risk of the
alternative is a reconcile failure being invisible behind a capture failure.
Raised as a spec gap because it is a visible behaviour the approved spec did
not settle. **Answered by the developer 2026-09-26** — one screen per failure,
in stage order — and now binding in both spec §10 and plan §9 (P8).

### E5.8 The no-signed-in-player path changes when `Refresh` runs

Today the invalid-user branch calls `Refresh()` in the constructor; `Refresh`
returns at `:291` because `store_` is null, so it does almost nothing — but it
is not literally nothing, and it is called before anything is drawn. Under the
plan it is not called at all on that branch, so `worlds_`, `vanilla_` and
`active_` keep their default-constructed values. That is safe because the error
state never draws the rail, `O EXIT` is the only exit, and `RailCount()`'s
value is never consulted. Checked: nothing in `DrawProblem` touches
`rowCache_` or `worlds_`.

### E5.9 Log content must not change

`Say` is the only writer of both the on-screen log and (via `Log`) the
`worlds:` lines in `live.log`. The plan changes exactly one call: the invalid
user's message moves from `Log("worlds: " + user_.error)` to
`Say(user_.error)`, which produces the byte-identical `live.log` line and adds
the on-screen one. Every other `Say` — `STARTING UP`, the job lines relayed
through `AddLines`, the blank line, `FIRST RUN - CAPTURING...`, the capture's
result lines, `SAVE DATA ...` — is untouched, so acceptance criterion 7 holds by
construction rather than by inspection.

### E5.10 Renaming two geometry constants

`kStartupTitleY` and `kStartupSubY` are read only by `DrawStartup`.
`kStartupTitleScale` and `kStartupSubScale` are read by `DrawStartup` **and**
`DrawConfirmDelete` (`:663-696`). Renaming only the two Y constants is safe;
deleting the two scale constants is not, which is why the plan's §3.1 names
them. `ui_scroll_verify.py` hard-codes the numbers 200 and 4 rather than the
constant names, so it is indifferent to the rename and would only notice a
change of value.

---

## E6. What the spec's appendix claimed

| Claim | Verdict |
| ----- | ------- |
| The whole item is `UpdateStartup`, `FinishStartup`, `ProbeContainer`, `DrawStartup`, `Say`/`AddLines` and the `Mode` enum | **Confirmed**, with one addition: the constructor's invalid-user branch (`:217-226`) also changes, and `WorldActivation.{h,cpp}` gains one accessor |
| `WorldEditorScreen::UpdateConfirm`'s pending flag is the pattern to copy | **Confirmed, with a wrinkle.** The editor's flag is set during an `Update`; this screen's first stage is entered in the *constructor*, so a consume-on-next-`Update` flag would still run the sweep before frame 0. The plan gates on "has been drawn" instead (§E2.6) |
| `Application.cpp` fixes the `Update`/`Draw`/`Present` order and owns `WorldsSession` | **Confirmed** (`:76-99`, `:64`) |
| Every reconcile outcome is decided in `Step`, `Finish` and `ActionForPhase` | **Confirmed**, and the five `Finish` sites are enumerated at §E2.3 |
| `FirstRunCaptureJob`'s only failure channel is `Fail`/`Rollback` | **Confirmed** (`WorldStore.cpp:1066-1081`) |
| Both jobs expose a `Progress()` the screen ignores | **Confirmed**, and the plan continues to ignore both — see §E3 for why `Progress()` is the wrong source for the bar |
| `Controls.h` has no bar primitive; `Renderer::FillRect` is what rules are built from | **Confirmed** (`Renderer.h:33`). The plan does not add one |
| `settings_ui_verify.py`'s `parse_named_strings`/`parse_geometry`/`parse_list_layout` are how strings reach a width check | **Confirmed**, and the new string prefixes are collision-free (§E2.7) |
| *Check early:* can the reconcile job's two internal stages per bar step both complete without a frame between them? | **Answered: no for stages 0 and 1** — `Step` does one per call and returns. **Yes for stages 2 and 3 relative to `Done`**: the `Nothing` and `DiscardStaging` actions `Finish` inside the same `Step` that enters stage 2, so bar steps 1 and 2 can complete in one frame. The plan's stage-0 body tests `Done()` before `ActingOnJournal()` for exactly this |
| *Check early:* can anything other than `FinishStartup` reach `Mode::Browse`? | **Yes — the constructor**, on both the tab-return path and the invalid-user path (`:222-226`). The first is correct and stays; the second is the one the plan reroutes. No other assignment of `Mode::Browse` exists in the file |
| *Check early:* `WorldsSession` is the only state that survives a tab switch | **Confirmed**, and nothing this item introduces needs to |
| The worlds implementation report §5/§6 list what is unverified | **Confirmed**, and two entries bear directly on this plan (§E5.5) |
| *Dead end:* no shared progress-bar control | **Confirmed** |
| *Dead end:* no reference-tool startup screen | **Confirmed** (§E1) |
| *Dead end:* `ReconcileResult::error` is populated on only one failure path | **Confirmed** (`:1309`) |
| *Dead end:* `note` is not an error signal | **Confirmed** (§E2.4) |

Nothing in the appendix was found to be wrong. Two of its "check early" items
changed the design, which is what they were for.

Where the plan's trace touches spec §2, it agrees: the four error conditions are
the complete reachable set, and no fifth failure channel exists in the startup
sequence. `ProbeContainer` treats an undiscoverable save title as not an error
(`:250-257`) and `Refresh` has no error channel — both confirmed by reading
every return path.

---

## E7. Anything that could not be established

* **How long the container probe actually takes.** Spec §4.3's ~2 s is
  `technical-findings.md`'s 15 MB/s applied to a ~27 MB save, never timed. It is
  an inference, and the plan treats it as one: no milestone, verification step
  or geometry choice depends on the number. It appears only in the hardware
  table, as the thing to report.
* **How long a first run takes end to end.** That path has never run on hardware
  from a screen at all. The plan's consequence — the bar sits on step 3 for the
  whole capture — is the spec's accepted design either way, so no decision rests
  on the duration.
* **What `Refresh()` costs.** Unmeasured; "tens of small file reads plus one AFR
  write probe" is a reading of the code, not a timing. This is the one place the
  plan's uniform gate is a judgement rather than a deduction: if `Refresh` is
  genuinely instant, the fifth stage's extra frame is wasted; if it is not, the
  gate is what keeps the bar honest. One frame is cheap enough that the
  unmeasured case was resolved in favour of the gate.
* **Whether a ~2 s block inside one `Update` trips any console watchdog.** Never
  observed. Unchanged by this item (§E5.5).
* **Whether the loading state reads well on a TV.** Colours, the 28 px bar
  height, and whether `Palette::Selected` on the running cell is legible at
  viewing distance are hardware judgements. The verifier can prove only that
  nothing clips and nothing overlaps.
* **Whether the bar's step 2 is ever visible in practice.** On a journal at
  phases 1–4 the action completes in one `Step`, so step 2 may last a single
  frame. The hardware test asks for the observation rather than asserting an
  outcome.
* **The error state's four sentences have never been read by a player.** Their
  wording is the spec's and is not the plan's to revisit; the plan verifies only
  that they fit.
