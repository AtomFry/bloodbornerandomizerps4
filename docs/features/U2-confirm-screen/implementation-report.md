# U2 — Condense the activation confirmation — implementation report

**Taken outside the pipeline at the developer's request**, exactly as U1 was:
no `spec.md`, no `plan.md`, built directly from its row in `docs/ui-backlog.md`.
This report is written after the fact and is the only artifact. The convention
is `docs/features/README.md`'s, and it is stated here because a thin folder is
otherwise indistinguishable from one where the stages were forgotten.

**Status: BUILT — not hardware-tested.** Clean cross-compile, `.pkg` produced,
`settings_ui_verify.py` 106/106 with fourteen new U2 cases, every other verifier
green. Per `CLAUDE.md` §3 that is *ready for hardware testing*, not done.

**Date:** 2026-09-26

---

## 1. The decision

Four options were put to the developer. The chosen one was **D, with no
expansion**:

> The five activation facts get the top of the screen at full size — those are the
> consequential ones and they're what "do you want to continue?" is actually
> about. The settings collapse to one summary line.

The "no expansion" half matters more than it sounds. It resolves the tension the
row itself recorded — *keep all the information* versus *never scroll* — decisively
in favour of never scrolling. The per-setting list is **gone from this screen**,
not hidden behind a drill-in.

**Why that is defensible.** The recipe is the screen the player just finished
editing one step back; the `WORLDS` rail states the same count against the row;
and every value reaches `live.log` when the run starts. What the confirmation
owes the player is the **consequence**, and the settings list was burying it.

## 2. What the screen was, and why it read as a dump

One flat scrolling review of **27 rows** — eight statement rows followed by all 19
settings — in a six-row band, at scale 4, with its own scroll offset and a
`UP DOWN SCROLL` hint. Its two footers sat 30px lower than every other screen in
the app purely to win a sixth visible row.

The complaint was not its length. It was its **flatness**: the irreversible
save-data consequence had exactly the same visual weight as `EASY ROM   NO`.

## 3. What it is now

Two fixed blocks, eight rows between them, no list and no scroll:

```text
                    WORLD EDITOR                 scale 5, Heading
                 CONFIRM ACTIVATION              scale 4, state line
        <the save action, or the refusal>        scale 3, wrapped, <=2 lines

   DEACTIVATING                      MIDNIGHT    <- tier 1, scale 4
   OUTGOING SAVE        BACKED UP AND SAVED TO IT
   ACTIVATING                      BLOOD MOON
   INCOMING SAVE                 KEEP EXISTING
   HOW LONG                     ABOUT A MINUTE
   ----------------------------------------------  the tier rule
   SEED                            1234567890    <- tier 2, scale 3
   TARGET                           CUSA03173
   SETTINGS                          12 OF 15 ON

                OPTIONS ACTIVATE   O BACK        one footer line
```

**Tier 1 — what is about to happen.** The five B10 facts, at the item scale.
Always five rows.

**Tier 2 — what this world is.** Seed, target, and the settings as a **count**, at
the row scale, under a rule. Always three rows. Quieter on purpose: the whole
point of two tiers is that these are not the same kind of fact as the five above.

**Label left, value right, in one centred block** — the way the settings pane
opposite them already is, rather than centred `LABEL   VALUE` strings. A column
of values that starts at a different x on every row is a block the eye cannot
scan, which was half of why the old list read as a log.

### 3.1 Three things deliberately removed

| Removed | Why |
| --- | --- |
| All 19 setting rows | §1 — the developer's decision, no expansion |
| The `NAME` head row | It said the same thing `ACTIVATING` says one row below it. The only difference was that `NAME` read this screen's working copy and `ACTIVATING` reads the plan — stating it twice was the clearest single example of the density this item is about |
| The `UP DOWN SCROLL` footer line | The screen cannot scroll. A hint for a control that does nothing is worse than no hint |

`kSettingsLayout`, `kConfirmGap`, `confirmScroll_`, `ConfirmItems()` and
`ConfirmHeadRows()` all went with them. `kSettingsLayout` had exactly one user —
Confirm — and the settings pane has its own geometry, shared with Setup Defaults.

### 3.2 The settings count is the rail's sentence, not a new one

`EnabledToggleCount(run_)` over `ToggleCount()`, rendered `n OF m ON` — character
for character what the `WORLDS` rail already shows against the row. Two screens
stating one fact two ways is how a player learns to trust neither.

`SaveChoice` stays excluded from that count, as `SettingsModel` already has it:
the save policy is stated in full in tier 1, where it belongs, and counting it
here as though it were a randomization toggle would both double-state it and make
the count mean two things at once.

## 4. What the verifier caught

Two real defects, both found by the new cases rather than by reading:

**4.1 The block was too narrow at 1200px.** `OUTGOING SAVE` +
`BACKED UP - NO WORLD TO SAVE IT TO` measures **1190px** at scale 4, which left
**10px** between label and value — a row that reads as one run-on string. The block
is 1400px (x=260..1660) and every tier row is held to a **40px minimum gap**.

This is the failure a screen-width budget cannot see: 1190px fits 1920 easily.
Only measuring label-plus-value against *the block* finds it.

**4.2 The two mirrors disagreed about whether Confirm was a list.**
`settings_ui_verify.py` asks `ui_scroll_verify.py` for Confirm's row count so the
two files cannot drift. After removing the entry, the answer was still 27 — the
lookup was matching the **commented-out record** of the old entry, which
`ui_scroll_verify.py` keeps on purpose as documentation. `ui_scroll_count()` now
skips commented lines, and "there is no entry" is itself the assertion for
Confirm.

## 5. Verification

| Layer | Result |
| --- | --- |
| Cross-compile | clean, no warnings; `.pkg` produced |
| `settings_ui_verify.py` | **106/106**, fourteen of them new U2 cases |
| `ui_scroll_verify.py` | PASSED — the `Editor Confirm` entry is gone |
| `worlds_verify.py` | 97/97 |
| `pool_verify.py selftest` | 89/89 |
| `font_atlas_verify.py` | PASS |
| Hardware | **not run** — see §7 |

The fourteen cases cover: the row counts against the coded constants; both tiers'
widest label+value against the block with its minimum gap; the block's centring;
a regression guard that Confirm builds no per-setting rows and cannot scroll; that
the settings row uses the rail's own count; the five vertical gaps from the state
line down to the footer; and that each of the three blocks clears its own line
box.

**The regression guard is the one worth keeping.** U2's removal of the settings
list is a *decision*, not a consequence of the layout, and the failure mode is a
well-meaning future edit reinstating a per-setting loop into a screen whose
geometry has no room to scroll it. The case fails if `SettingValueText`,
`confirmScroll_` or `kConfirmGap` reappear.

## 6. The startup bar, changed in the same pass

Not part of U2, done alongside it at the developer's request: `WorldsScreen`'s
loading bar was **five discrete cells** and is now **one continuous fill**, matching
the activation bar U1 introduced.

Two loading screens in one session that disagree about what a bar looks like is a
seam the player can see — and removing that seam is why U1 copied the startup
screen's geometry to the pixel in the first place. `kLoadCellW` and `kLoadCellGap`
are gone; `kLoadBarMinFillW` is now twinned with `kActBarMinFillW` in the U1
geometry comparison, which checks eight constants instead of seven.

**The stage machine behind it is unchanged.** `stagesDone_` still counts the same
five stages, so the fill advances in fifths rather than smoothly. That is honest —
startup knows five things about its own progress and no more — and it is why the
denominator stays. The verifier case changed from "five equal cells fill the block
exactly" to "the five steps divide the 1000px block exactly", which is the same
arithmetic guarding the same edge.

What the cells bought was a visible distinction between the stage *running* and
the stages *done*. The fill keeps the claim that matters: its edge is the boundary
between finished and running work, because `SetStagesDone` only advances once a
stage is over and `stageDrawn_` holds the next stage back until the frame has been
presented.

## 7. What hardware has to answer

1. **Is the confirmation still enough to decide on?** This is the §1 decision, and
   the console is where it is judged. Specifically: does losing the settings list
   ever leave you unsure what you are about to activate?
2. **Does the two-tier split read as two tiers** at three metres, or does the rule
   just look like a divider between two equal lists? The scale difference (4 vs 3)
   and the `Dim` labels are doing that work.
3. **The widest rows.** Force `OUTGOING SAVE` to `BACKED UP - NO WORLD TO SAVE IT
   TO` (activate a world when none is active) and confirm §4.1's 1400px block
   actually looks right rather than merely fitting.
4. **The startup bar in fifths.** Confirm it does not read as stuck — one stage
   dominates the wait, so the bar sits at 3/5 for about two seconds.

## 8. Files changed

| File | What |
| --- | --- |
| `app/src/UI/WorldEditorScreen.h` | `ConfirmRow`; `ConfirmPlanRows`/`ConfirmWorldRows` replace `ConfirmItems`/`ConfirmHeadRows`; `confirmScroll_` removed |
| `app/src/UI/WorldEditorScreen.cpp` | `DrawConfirm` rewritten as two fixed tiers; the block and tier constants; `kSettingsLayout`, `kConfirmGap` and `kConfirmFooterHint` removed; `UpdateConfirm` loses its scroll handling |
| `app/src/UI/WorldsScreen.cpp` | the loading bar is one fill; `kLoadCellW`/`kLoadCellGap` removed, `kLoadBarMinFillW` added |
| `app/tools/settings_ui_verify.py` | fourteen U2 cases; the bar case rewritten; `kActBarMinFillW` twinned; `ui_scroll_count` skips comments |
| `app/tools/ui_scroll_verify.py` | the `Editor Confirm` entry removed, kept in a comment as the record |
| `docs/user-guide.md` | what the confirmation states, and that it does not list settings |
| `docs/ui-backlog.md` | U2 → **BUILT**; the startup-bar change recorded |

`Game/WorldActivation` was **not touched**. Every refusal, every phase and the
whole B10 statement are unchanged — U2 moved where five facts are drawn and deleted
nineteen rows, and nothing about what an activation *does* moved with them.
