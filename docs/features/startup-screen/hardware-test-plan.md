# Hardware Test Plan — Startup Screen

**Status: READY TO EXECUTE** — written 2026-09-26, after milestone 2.

**Covers:** both milestones as one build, which is how the developer chose to
test them.

**Package:** `app/IV0000-BBRD00001_00-BBRANDOMIZERMAIN.pkg`, built 2026-09-26,
no warnings.

**Nothing in this feature has run on a PS4.** Both milestones were built before
any hardware test, at the developer's direction. Two milestones' changes sit
under every step, so a failure is not narrowed the way one-at-a-time testing
would narrow it.

---

## What you are actually checking

Off-console verification can only prove that strings fit and regions do not
overlap. **Everything this feature is about is unprovable from here** — whether
the bar appears at all, whether it steps in time with the work, whether the
screen is drawn before the two-second read rather than after it, and how long
any of it takes. That is the whole point of this pass.

**Three numbers to write down**, because the spec labels all three as inferences
and no measurement of any exists:

| # | What to time | Why it matters |
| - | ------------ | -------------- |
| T1 | How long the bar sits on its **fourth cell** on a healthy launch | The container read. Inferred at ~2 s from 15 MB/s applied to a 27 MB save, never timed |
| T2 | How long the bar sits on its **third cell** on a first run | The capture. That path has never run on hardware at all |
| T3 | How long the **fifth cell** takes | `Refresh()`. Unmeasured, tens of small file reads |

---

## Part 0 — Before you start

1. **FTP copy of your save.** Use PS4Explorer to pull
   `/user/home/134cf1a5/savedata/CUSA00207/` somewhere FTP can reach, as in the
   worlds test pass. Nothing here should touch save data, but Part D
   deliberately provokes reconciliation.
2. **Copy `/data/bbrandomizer/`** — you will be hand-editing
   `activation.journal` in Parts D and E, and you want the original back.
3. Note that **`live.log` is wiped on every launch**, so pull it *after* each
   part you care about and before relaunching.
4. Install the `.pkg`.

---

## Part A — A healthy launch *(the case that matters)*

### Do
Launch the app on a console that already has worlds.

### Expect
* The loading screen appears **immediately** — wordmark `BLOODBORNE RANDOMIZER`,
  a rule beneath it, a five-cell bar, and the word `LOADING`.
* Cells light left to right. The running cell is a different colour from the
  completed ones.
* **No prompt, no hold.** The worlds list replaces the screen on its own.
* Nothing on screen but the wordmark, the rule, the bar and `LOADING` — no
  phase names, no log, no counts.

### Measure
**T1 — time the fourth cell.** A stopwatch or a phone video is fine; one
decimal place is plenty. This is the number the whole design was sized around.

### Failure looks like
* **A black screen, or the old technical log, before the loading screen.** The
  loading screen must be the first thing drawn.
* **The bar freezing on a cell while the app is clearly still working**, or
  jumping two cells at once — that is the bar lagging the work, which is the
  defect milestone 2 exists to remove.
* **Any word other than the wordmark and `LOADING`.**
* **Any prompt at all.**

---

## Part B — Tab switching does not re-run startup

### Do
From the worlds list, Left to `DEFAULTS`, then Right back to `WORLDS`.

### Expect
The list, immediately, both ways. **No loading screen, no bar, no pause.**

### Failure looks like
The loading screen reappearing — that would mean `WorldsSession::startupDone` is
no longer guarding the sequence, and every tab switch is re-sweeping and
re-probing.

---

## Part C — First run

### Do
Move `/data/bbrandomizer/Worlds/` aside over FTP (rename, do not delete), then
launch.

### Expect
The same loading screen. The bar reaches its **third** cell and stays there
while the capture runs — tens of seconds is expected. Then it finishes and the
list appears with `VANILLA` holding a save. **No prompt at any point**, which is
the change you asked for.

### Measure
**T2 — time the third cell.**

### Then check over FTP
`Worlds/acct-4a17f1993020e604/vanilla/save/manifest.txt` exists and its `files`
and `bytes` match your live save. A `SaveBackups/…_firstrun/` directory exists.
Your live save is unchanged.

### Failure looks like
A prompt; the list appearing before the capture has finished; the bar sitting on
the wrong cell; or anything under `SaveBackups/` left with a `.partial` suffix.

---

## Part D — An interrupted activation that reconciles cleanly

### Do
Hand-write `/data/bbrandomizer/activation.journal` with a `phase` the app does
write — any of 1 to 7 — and plausible `from_world` / `to_world` values, then
launch.

### Expect
**Silent.** The loading screen, the bar, the list. No error screen and no
prompt, because reconciliation succeeded — this is the hold you deliberately
removed.

### Watch for
Whether the **second cell** is ever visibly lit. It covers acting on the
journal, and on a fast path it may flash by; note what you see either way.

### Then check
The journal is gone, and the resulting world is marked `ACTIVE` on the list.

---

## Part E — An interrupted activation the app cannot understand

### Do
Hand-write the journal with `phase=9`, a value no version of the app writes,
then launch.

### Expect
* The **error screen**: `STARTUP PROBLEM` in red, the sentence
  `AN INTERRUPTED ACTIVATION WAS NOT UNDERSTOOD AND WAS LEFT ALONE`, the log
  beneath it, and `X CONTINUE`.
* Press `X`. **It must return to the loading screen** — the fourth cell
  running — and then to the list. It must not freeze, and it must not jump
  straight to the list.

### Then check
**The journal is still on disk.** The app was explicit that it left it alone;
if it is gone, the app deleted something it said it did not.

---

## Part F — Nobody signed in

### Do
Sign out of the PS4 account, or launch with no user in the foreground.

### Expect
The error screen with `NO SIGNED-IN PLAYER`, and **`O EXIT` — with no
`X CONTINUE`**. This is the one condition of four that offers no continue,
because there is nothing to continue to.

### Failure looks like
`X CONTINUE` appearing here; or the old behaviour, which was to skip startup
silently and drop you on a list showing two inert rows with no explanation.

---

## Part G — Two failures on one launch *(optional, awkward to provoke)*

Only if you can arrange it: a `phase=9` journal **and** a moved-aside `Worlds/`
directory, so the reconcile fails and the capture then also fails.

**Expect two error screens in stage order**, each with its own sentence, `X`
moving from the first to the second. That is decision P8, and it is the only
case that exercises it.

Skip this if it is more trouble than it is worth; it is rare and nothing else
depends on it.

---

## The one thing that could go wrong in a new way

`docs/features/worlds/implementation-report.md` records, still unverified, that
**two seconds of blocking work inside one `Update()` does not trip a
watchdog**. This feature neither lengthens nor moves that call — but it is now
the fourth cell of a visible bar, so if the console ever did object, this is the
build where you would see it.

**Symptom:** the app dies, or the screen goes black, while the bar sits on its
fourth cell. If that happens, it is not the loading screen's bug — it is that
exposure finally becoming visible, and it needs its own investigation.

---

## Result sheet

| Part | Test | Result | Time |
| ---- | ---- | ------ | ---- |
| A | Healthy launch, bar, no prompt | | T1 = |
| B | Tab switch does not re-run startup | | |
| C | First run, no prompt, save captured | | T2 = |
| — | `Refresh()` / fifth cell | | T3 = |
| D | Journal at a written phase — silent | | |
| E | Journal at an unwritten phase — error, `X` resumes | | |
| F | Signed out — `O EXIT`, no `X CONTINUE` | | |
| G | Two failures, two screens *(optional)* | | |
