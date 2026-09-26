# Plan NNN — <Name>

**Status: DRAFT**

**Spec:** `docs/features/NNN-<slug>/spec.md`

**Evidence:** `docs/features/NNN-<slug>/plan-evidence.md` — the measurements,
traces and rejected alternatives behind this plan

**Plan review:** `docs/features/NNN-<slug>/plan-review.md` — added during review

---

## Execution Strategy

*Approving this plan approves this strategy. `CLAUDE.md` §4: milestones describe
implementation structure; gates describe when a human stops to test.*

| | |
| --- | --- |
| **Structure** | <single implementation \| N functional milestones \| N phases with sub-work> |
| **Execution** | <continuous \| gated after M<n>> |
| **Human test gates** | <final only \| after M<n> and M<m>> |
| **Intermediate verification** | <what builds and runs after each milestone> |

<One line per gate, if any: which milestone, **Required** or **Optional**, and
what risk it materially reduces. Omit the paragraph entirely when there are no
gates — "final only" needs no justification, it is the default.>

---

> **This document is the implementation contract.**
>
> §1–§7 are what the implementer reads, in order, and they are sufficient to
> start work. Everything an implementer must *do* is here; nothing here is
> history.
>
> §8–§10 are the record, not instructions. `plan-evidence.md` holds the
> investigation. `log.md` holds the stage-by-stage history — how this plan was
> reviewed and refined belongs there and must not be narrated here.
>
> **Budget: §1–§7 together should fit in about 400 lines.** If they do not, the
> surplus is almost always evidence or history that belongs in the other two
> files. Length is not the defect; mixing genres is.

---

## 1. Objective

One short paragraph: what this change accomplishes, in behavioural terms.

No background, no history, no justification. A reader should know what they are
building after this paragraph.

---

## 2. Approved behaviour

Only settled behaviour, taken from the spec. Each item traces to the spec
section or decision that settled it.

| # | Behaviour | Source |
| - | --------- | ------ |
| B1 | ... | spec §10 D1 |
| B2 | ... | spec §2 |

Nothing in this section is argued. If a behaviour still needs a decision, it
belongs in §8, not here — and the plan is not ready to implement.

---

## 3. Constraints and invariants

### 3.1 Must be preserved

What the implementation must not change. Each one line, with the consequence of
breaking it.

* Existing X behaviour is unchanged — ... would silently alter shipped output
* RNG sequence is unchanged when ... — ... breaks seed reproducibility
* Layering: UI never touches raw AFR paths; the randomizer core never touches SDL2

### 3.2 Out of scope

What an implementer might reasonably think belongs here and does not. Be
explicit — an unstated exclusion is an invitation.

* ...

### 3.3 Hazards

Things that could go wrong during implementation, one line each, with what to do
about it. Detailed analysis goes in `plan-evidence.md`; this section carries only
what changes the implementer's behaviour.

| Hazard | What to do | Evidence |
| ------ | ---------- | -------- |
| ... | ... | `plan-evidence.md` §E5.1 |

---

## 4. Implementation approach

The design, stated as decisions rather than as reasoning.

Describe the pieces and how they fit. Where an alternative was considered and
rejected, say which approach was chosen in one line and point at the evidence
file — do not reproduce the comparison here.

> Chosen: <approach>. Alternatives considered and why they were rejected:
> `plan-evidence.md` §E3.

### What this reuses

| Existing code or tool | How it is used | Change needed |
| --------------------- | -------------- | ------------- |
| `app/src/...` | ... | extend / reuse as-is / none |

The survey of everything that exists nearby belongs in `plan-evidence.md` §E2.
This table is only what this change actually touches or depends on.

---

## 5. Files and changes

| File | Change | Milestone |
| ---- | ------ | --------- |
| `app/src/...` | ... | 1 |

Include new files, modified files, generated files and tables, build or
configuration changes, and saved-configuration changes.

Call out anything that requires a clean rebuild or could invalidate existing
saved data.

---

## 6. Verification

Only the checks the implementer must actually run, and what each one proves.

### Build

What must build, and whether a clean rebuild is required.

### Automated

| Check | Command | Asserts |
| ----- | ------- | ------- |
| ... | `python app/tools/....py selftest` | ... |

For a new or extended Python mirror, say what its self-test asserts. State
plainly that a mirror pins the rules and not the C++ implementation of them.

### Hardware

What the developer must test on the PS4, and what the pass condition is. The
implementer cannot run this — it is the handoff, not a step.

---

## 7. Milestones and stop conditions

Each milestone is **a meaningful piece of functionality becoming complete** —
not a test checkpoint. Whether execution pauses after one is the Execution
Strategy's business, above, not the milestone's. One milestone is a fine answer,
and so is five.

Never introduce a seam, a transitional state or scaffolding whose only purpose
is making an intermediate milestone independently testable (`CLAUDE.md` §4).

### Milestone 1 — <name>

**Goal.** One sentence.

**Changes**, in order:

1. ... — done when ...
2. ... — done when ...

**Invariants** this milestone must not break: <reference §3.1 items by name>

**Verification:** <the §6 checks that apply here>

**On completion.** The `.pkg` builds and the checks above pass. Then **continue
to the next milestone** unless the Execution Strategy places a gate here, in
which case stop and hand off the hardware test.

### Milestone 2 — <name>

...

### Stop conditions

Halt and report rather than deciding, if any of these occur:

* the build fails in a way the plan did not anticipate
* a verification check fails and the cause is not an obvious implementation slip
* an implementation decision would contradict the spec or §2
* a required behaviour cannot be implemented as described
* the change needs a file not listed in §5
* a §3.1 invariant cannot be preserved

---

## 8. Open questions

Implementation questions still needing a developer decision. Each with what is
known, a recommended answer and its reason, and whether it is blocking.

Questions about what the feature should *do* belong in the spec instead.

Remove a question once it is answered and record the answer in §9.

---

## 9. Decisions

| # | Date | Decision | Taken by |
| - | ---- | -------- | -------- |
| P1 | YYYY-MM-DD | ... | planner / developer |

Decisions only — one line each. The reasoning behind a decision goes in
`plan-evidence.md`; the history of how it was reached goes in `log.md`.

---

## 10. Changes during implementation

Keep empty until implementation begins. Stage C refinements do **not** go
here — they are edits to §1–§7, recorded in `log.md`.

| Date | Change | Reason |
| ---- | ------ | ------ |
| YYYY-MM-DD | ... | ... |

---

<!--

The spec describes WHAT the feature should do.
This plan describes WHAT THE IMPLEMENTER DOES, and how it is verified.
plan-evidence.md describes WHY the plan says what it says.
log.md describes HOW the plan got here.

Keep production code out of the plan.

-->
