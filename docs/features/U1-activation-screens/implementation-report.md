# U1 — Simplify the activation screens — implementation report

**This item was taken outside the pipeline at the developer's request.** There is
no `spec.md` and no `plan.md`: the developer asked for U1 to be implemented
directly from its row in `docs/ui-backlog.md`. This report is written after the
fact and is the only artifact, which is the convention `docs/features/README.md`
records for such items (row 34 and `font-atlas/` are the precedents). A folder
that merely looks thin is indistinguishable from one where the stages were
forgotten, so it says so here.

**Status: BUILT — not hardware-tested.** Clean cross-compile, `.pkg` produced,
all four UI verifiers green. Per `CLAUDE.md` §3 that means *ready for hardware
testing*, not done. Everything this item changed is about how a screen looks on a
television, which is the one thing no verifier can judge.

**Date:** 2026-09-26

---

## 1. What was asked for

UI backlog row U1, in the developer's own framing: the activation screens feel
"overbearing on how much text and information — it's almost like a log
information on the screen", and should be "more like the startup screen".

The startup screen had just made exactly this change for boot
(`docs/features/startup-screen/`), so it was the shape to copy rather than a new
design.

## 2. The one decision that shaped it

The running screen was never in question — it becomes a loading screen. What the
**finished** state should do was a real fork, and it was put to the developer
before any code was written:

| Option | Chosen |
| --- | --- |
| Condense the result counts into a fixed, non-scrolling summary grid | no |
| **Go straight back to the `WORLDS` rail; counts to `live.log` only** | **yes** |
| One `ACTIVATED` line with no numbers | no |

So a successful activation now draws **no screen of its own**. It is the startup
screen handing to the rail, one step later in the same session.

**Why that is safe, and it is the same argument the startup screen made.** The
rail the player lands on already marks the world `ACTIVE` — the same fact, stated
where they would look for it. The dismissal bought nothing.

**What it costs, stated plainly:** the three-line closing flourish
(`WHAT ARE YOU STILL DOING HERE` / `ENOUGH TREMBLING IN YOUR BOOTS` /
`A HUNTER MUST HUNT`) had nowhere left to live and is gone. It was the one piece
of character on the screen. If it is wanted back, the `WORLDS` rail is where it
belongs, not a screen that exists only to hold it.

## 3. What the screens do now

### 3.1 `Step::Progress` — the loading state

Four elements, and nothing else at any point in the seven-phase transaction:

```text
              ACTIVATING              scale 5, Heading
        -------------------           2px rule, kRuleColor
        #########..........           bar, 1000 x 28
             MIDNIGHT                 scale 4, Text — the world's name
```

No log, no phase name, no percentage, no prompt, and **no input at all**. The
transaction has no phase it would be safe to abandon between, so a button that
appeared to offer a cancel would be lying. The old screen already ignored input
here for that reason.

The geometry is `WorldsScreen`'s `kLoad*` values **to the pixel**, and
`settings_ui_verify.py` now compares the two sets rather than trusting that they
were copied — see §5.

**The one deliberate departure: the bar is a continuous fill, not the startup
screen's five discrete cells.** The reason is the work behind it, not taste.
Startup's five stages are coarse and all but one are instant, so a cell per stage
tells the whole truth and a smooth bar would be inventing motion it could not
justify. An activation is the opposite — `WorldActivationJob::Progress()` is a
real 0..1 across all seven phases *with* sub-phase progress inside the three long
ones, and phase 4 alone runs 10–20 seconds. Seven cells would sit motionless
through most of the wait, which is the one thing a loading screen must not do: a
still bar with no other feedback reads as a hang, and the log this removes is
what used to disprove that.

The world's name is the one element startup does not need and this screen does.
An activation replaces whatever is live, so "which one is this" is a fact worth
stating — and it is a fact, not a phase.

### 3.2 `Step::Problem` — the only state that holds

New step, and the mirror of `WorldsScreen`'s own `Mode::Problem`, on the same
band with the same numbers. Two titles, because the two outcomes mean opposite
things about the console:

| Title | Sentence | Means |
| --- | --- | --- |
| `ACTIVATION REFUSED` | the refusal's own designed sentence | nothing was written at all |
| `ACTIVATION FAILED` | fixed: `THIS WORLD WAS NOT ACTIVATED - THE LOG BELOW SAYS WHERE IT STOPPED` | stopped partway; reconciliation finishes or rolls back at next launch |

Beneath it, the log — every line the job emitted plus this screen's report of what
the generation did — scrollable, tail-followed, `O RETURN TO WORLDS`.

This is where "failure still has to say what failed and why" lands, and it is the
one case where the detail earns its place.

## 4. The two corrections found on the way

Neither was in the row as written; both are in the shipped code.

**4.1 The refusal sentence is quoted from the job, not from `plan_`.** The first
draft read `plan_.refusal.sentence` — the copy `Step::Confirm` built a frame
earlier. Phase 1 re-checks everything that screen showed, and if the two ever
disagree, the refusal that actually stopped the transaction is the one to quote.
`FinishCommit` now captures `activation.refusal.sentence` inside the block that
has the result.

**4.2 The sentence must wrap, and unwrapped it would have run off the screen.**
Four refusal sentences are **built at runtime** — they append a vanilla path, a
setting's label or a save directory name — so the longest one on screen is not the
longest one in `WorldActivation.cpp`, and no character budget measured off that
file would bound it. `Step::Confirm` already learned this and wraps its copy to
two lines at 1800px; `Step::Problem` now does the same. Drawing it with a bare
`DrawCenteredLabel`, as the first draft did, would have pushed a refusal about a
long path off both edges of the television — and no verifier would have caught it,
because every string in the source fits.

**One enum replaced two booleans**, which is the correction the startup screen
made for the same reason: `refused` and `failed` as a pair let a caller claim
both, and they mean opposite things. `Outcome { None, Refused, Failed }`.

## 5. Verification

| Layer | Result |
| --- | --- |
| Cross-compile | clean, no warnings; `.pkg` produced |
| `settings_ui_verify.py` | **97/97**, including two new U1 cases |
| `ui_scroll_verify.py` | all geometry and scroll properties PASSED |
| `worlds_verify.py` | 97/97 — it parses this screen's `SKIPPING` lines, which are untouched |
| `pool_verify.py selftest` | 89/89 — it parses four of this screen's strings |
| `font_atlas_verify.py` | PASS |
| Hardware | **not run** — see §6 |

**Two cases added to `settings_ui_verify.py`:**

- `U1: the activation loading screen's 7 geometry constants equal the startup
  screen's` — the `kAct*` / `kLoad*` twin comparison. Two files draw the same four
  elements at two moments of one session; a player who can tell them apart is
  looking at a bug, so the numbers are compared rather than merely copied. The bar
  is deliberately **not** in that list (§3.1), so `kLoadCellW` and `kLoadCellGap`
  have no twin and the two bars share only their band.
- `U1: the loading block is centred on the screen` — the one property the twin
  comparison cannot catch, since both files could be wrong together.

**One stale assertion removed from `ui_scroll_verify.py`.** Its `Progress (run)`
block proved that the live status line could never collide with the footer. There
is no live status line any more, and no list at all while the job runs, so the
block asserted something that no longer exists — it passed, and it was checking a
screen that was gone. The band it used is now reached only by `Step::Problem` and
is relabelled `Activation log`; the loading state is pinned by the two
`settings_ui_verify.py` cases instead.

## 6. What hardware has to answer

Everything here is how it looks at three metres, plus one behavioural question
the verifiers cannot reach:

1. **A healthy activation of a real world.** The bar should move continuously and
   never appear to stall — phase 4 is the one to watch. Landing back on the rail
   should feel like an ending, not like the app dropped you.
2. **Does losing the result screen feel like losing something?** This is the
   decision in §2, and the console is where it is actually judged.
3. **`VANILLA`.** Activation is a tree deletion and is fast; confirm the loading
   screen is not a flash of nothing.
4. **A refusal.** Easiest to force: turn on `RANDOMIZE ENEMIES` with an empty
   `ENEMIES INCLUDED`. Confirm the title reads `ACTIVATION REFUSED`, the sentence
   is legible, and the log beneath it is scrollable. **Check a refusal carrying a
   long runtime path wraps to two lines and stays on screen** — §4.2 is reasoned,
   not observed.
5. **A failure.** Harder to force honestly; if one occurs, confirm the title says
   `FAILED` rather than `REFUSED` and that the next launch reconciles.

## 7. Files changed

| File | What |
| --- | --- |
| `app/src/UI/WorldEditorScreen.h` | `Step::Problem`, `Outcome`, two new draw/update decls; `commitFinished_` and `completionLineStart_` removed |
| `app/src/UI/WorldEditorScreen.cpp` | `DrawProgress` rewritten as the loading state; `DrawProblem` and `UpdateProblem` added; `UpdateProgress` reduced to stepping the job; `FinishCommit` routes the three endings; the `kAct*` constants |
| `app/tools/settings_ui_verify.py` | two U1 cases |
| `app/tools/ui_scroll_verify.py` | the stale running-state block replaced; band relabelled |
| `docs/user-guide.md` | the loading screen, the two problem screens, and where the counts went |
| `docs/ui-backlog.md` | U1 → **BUILT** |

`Game/WorldActivation` was **not touched**. The seven-phase transaction, its
journal, its refusals and its reconciliation are exactly as they were — U1 is
presentation, and the row said so.
