# Proposal — a loading screen, not a log

**Status: PROPOSAL, shape agreed with the developer 2026-09-26.** Ready to
become `spec.md` via `/spec` on request. This file stays as the record of what
was proposed and why.

**Scope:** the screen between launch and the `WORLDS` rail. Nothing about the
randomizer, the editor, or activation's own progress log.

---

## 1. The problem

The startup screen was built while startup was the thing being debugged, so it
shows what a developer needed then: every line, as it happens. A normal boot on
the reference console shows this and nothing else:

```
STARTING UP
SWEPT 0 PARTIAL BACKUP(S), 0 PARTIAL WORLD SAVE(S)
NO ACTIVATION WAS INTERRUPTED
SAVE DATA CUSA00207 SPRJ0005  1136 BLOCKS
```

Three of those four lines say *nothing happened*, in vocabulary that means
something only to the person who wrote the code. A game does not tell you it
checked for a corrupt save and found none.

**The framing that settled the design.** Someone stumbles on this, thinks
"Bloodborne randomizer for PS4, neat", installs it and runs it. They expect to
see a randomizer. Every decision below follows from that: on a healthy console
the startup screen should be a thing you barely notice, and it should never ask
them to do anything.

---

## 2. The screen

### 2.1 Loading — every boot, including the first

```
                                                          1920 × 1080


                    BLOODBORNE RANDOMIZER                 scale 6, Heading
                 ─────────────────────────                1px rule, Dim

                 ▓▓▓▓▓▓▓▓▓▓▓▓▓░░░░░░░░░░░░                bar, 720 × 6
                          LOADING                         scale 3, Dim


```

Four elements, none of which move except the bar. The text is the literal word
`LOADING` throughout — it does not name the phase, because naming the phase is
the thing we are removing.

**The bar advances one step per stage and does not animate between steps.**
Smoothness is explicitly not a goal. A bar that jumps 2/5 → 3/5 → done is
honest about what it knows and costs nothing to build.

**No hold, no prompt, no minimum display time.** When the work is done the rail
replaces this screen. On a healthy console that is well under a second of
visible screen; on a first run it is however long the capture takes, and the
player sees a loading screen doing loading, which is what they expect.

### 2.2 Startup problem — only on an unexpected error

```
                    STARTUP PROBLEM                       scale 5, Bad
                 ─────────────────────────

          <the one-sentence reason, in Bad>               scale 3

          <the log, exactly as it is drawn today>         scale 3, Text

                      X  CONTINUE                         scale 3, Dim
```

The only state that holds, the only state that shows the log, and the only
state with a prompt. It reuses the existing log drawing verbatim — this is
deliberately not a new design.

**What counts as an error**, and nothing else does:

| Condition | Where |
| --------- | ----- |
| `ReconcileResult::ok == false` | reconciliation failed, or hit a journal phase no version of this app wrote |
| `FirstRunCaptureResult::error` is non-empty | the capture failed or rolled back |
| `User::valid == false` | no signed-in player, or no account id for them — see §4.4 |

Everything else — swept `.partial` directories, a journal found and correctly
reconciled at any phase, a first run that worked — is **silent**. It goes to the
log and nowhere else.

### 2.3 What this removes, deliberately

| Removed | Why it is safe to remove |
| ------- | ------------------------ |
| The hold after a non-quiet reconcile | Swept partials and a discarded staging tree touched nothing the player owns. A journal reconciled at phases 5–7 *succeeded*, and the rail it hands to already marks the resulting world `ACTIVE`. The hold was written while the transaction was unproven; it passed hardware on 2026-09-25 |
| The hold after first-run capture | Nothing is destroyed, the live save is untouched, and `VANILLA` appears on the rail holding a save — the outcome is visible where the player would look. **Developer decision, 2026-09-26:** a new user expects to see the randomizer, not to acknowledge a message about internals they have no context for |
| Per-phase status text | The text is always `LOADING` |
| The scrolling log on a healthy boot | It is in `live.log` either way |

---

## 3. What has to change underneath

Three changes. Only the first touches anything outside `WorldsScreen`.

### 3.1 `ProbeContainer` must run *after* the frame that shows its step

`FinishStartup()` calls `ProbeContainer()` synchronously
([WorldsScreen.cpp:383](../../../app/src/UI/WorldsScreen.cpp)), and that is a
six-SKU `DiscoverSaveTitle` sweep plus a mount, full walk and unmount — **about
two seconds on a 27 MB save**, per milestone 4's measurement. No frame is drawn
for its duration.

This is the entire visible wait. Everything else on a healthy console is
near-instant, which is why a per-phase *weighted* bar is unnecessary: one step
dominates and the rest are free.

It does **not** need to become a stepped job. It needs the ordering milestone 6
used for the activation confirmation's phase 1 — present the frame showing that
step, *then* block. The bar then sits honestly at that step for two seconds
rather than freezing at the previous one.

### 3.2 A step counter for the bar

A fixed five-stage list, each stage either run or skipped, each advancing the
bar on completion. A fixed denominator keeps the bar monotonic even though two
stages usually do not run:

| # | Stage | Normal boot |
| - | ----- | ----------- |
| 1 | Sweep debris, read the journal | runs, instant |
| 2 | Act on the journal / resume | skipped |
| 3 | First-run capture | skipped |
| 4 | Read the save container | runs, **~2s** |
| 5 | Build the world list | runs, fast |

### 3.3 An outcome, replacing two booleans

`quiet` and `startupWait_` currently encode four situations between them, and
cannot tell a failed save restore from a deleted temp directory — both set
`startupWait_ = true` and draw the identical white log. Replace with an explicit
outcome of `Ok` or `Failed`, where `Failed` carries the sentence to show.

**This is the substantive correctness fix in this item**, independent of how the
screen looks.

---

## 4. Notes, decisions and things found on the way

### 4.1 The log is wiped every launch — accepted

`Platform::Init` unlinks `live.log` at startup
([Platform.cpp:36](../../../app/src/Platform/Platform.cpp)), so the log only
ever covers the current boot. Raised and **accepted as-is by the developer,
2026-09-26**. Recorded here so it is not re-raised: if a player reports "it did
something odd last time", that evidence is gone by the time they relaunch.

### 4.2 First run does the expensive work twice — separate item

`FirstRunCaptureJob` phase 0 calls `DiscoverSaveTitle` and `ReadContainer`;
`FinishStartup` then calls `ProbeContainer`, which does **both again**. Twelve
save-data searches and two full container walks on the slowest launch there is.
`WorldsSession` exists to cache exactly this and the capture job does not
populate it.

**Not part of this item.** It is a pure performance win with no design content,
it roughly halves the first-run wait, and it should be its own small change.

### 4.3 `AfrManager::Check` writes during startup

`Refresh()` calls it, and it probes writability by creating and unlinking a
scratch file. Harmless and already known (milestone 3 report §3.11). Noted only
so that "startup writes nothing" is not assumed.

### 4.4 No signed-in player currently degrades silently

`WorldsScreen`'s constructor skips startup entirely when `user_.valid` is false
and drops straight to `Browse` with a single log line. The rail then lists
nothing and the player is given no reason. This proposal routes it to the error
screen, which is a behaviour change worth calling out rather than smuggling in.

---

## 5. Verification

**Automated**, the usual mirror — `settings_ui_verify.py` can assert that the
wordmark, `LOADING`, the rule and the bar fit their band at their scales, that
the error screen's strings fit, and that nothing overlaps. It cannot judge any
of this.

**Hardware.** Everything that matters here is how it looks on a television at
three metres, and how long the bar sits at stage 4. One test: launch on a
healthy console and confirm the screen is unremarkable; launch with a
hand-written `activation.journal` and confirm it is still unremarkable; force an
error and confirm the problem screen appears with a readable reason.

---

## 6. Size

**Small.** One screen's draw code in two states, a five-stage counter, one call
moved behind a frame, and one enum replacing two booleans. No randomizer code,
no save-data code, no new files.
