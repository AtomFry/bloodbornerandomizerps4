# Implementation Report NNN — <Name> — milestone <N>

**Status: AWAITING HARDWARE TEST**

**Plan:** `docs/features/NNN-<slug>/plan.md` — milestone <N>

**Spec:** `docs/features/NNN-<slug>/spec.md`

**Implemented:** YYYY-MM-DD

---

## 1. What was built

One short paragraph, then the plan's §7 change list with what actually happened
to each.

| # | Planned change | Status | Where |
| - | -------------- | ------ | ----- |
| 1 | ... | done | `app/src/...:120-148` |
| 2 | ... | done differently — see §3 | `app/src/...` |

---

## 2. Deviations from the plan

Anything built differently from §1–§7, or not built at all.

For each: what the plan said, what was done, and why. If nothing deviated, say
so in one line — that is the expected outcome.

Every entry here must also be recorded in the plan's §10.

---

## 3. Decisions the plan left open

Choices made during implementation that the plan did not settle. Each with the
options and why this one was taken.

These are the most likely places for the code review to disagree. Do not bury
them.

---

## 4. Verification run

| Check | Command | Result |
| ----- | ------- | ------ |
| Build | `cd app && make` | `.pkg` built |
| ... | `python app/tools/....py selftest` | 43 passed |

Paste the failure, not a summary of it, for anything that did not pass.

---

## 5. What this does not prove

The build and the mirrors do not establish runtime behaviour (`CLAUDE.md` §3).
Say plainly what still rests on the PS4.

---

## 6. Hardware test handoff

From the plan's §6 Hardware section, as a procedure the developer can follow.

1. What to enable
2. What to do
3. What to look for
4. What a failure looks like

---

## 7. Stop point

Which stop condition or completion gate ended this pass, and what the next
milestone is.

---

<!--

One report per milestone. A milestone that stopped early still gets a report
saying where it stopped and why.

-->
