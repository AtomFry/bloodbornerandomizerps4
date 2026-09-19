# AI-Assisted Development Process

## 1. Purpose

This document defines the AI-assisted development process being built for the Bloodborne PS4 Randomizer.

The goal is not to automate development as quickly as possible. The goal is to build a development workflow that:

* keeps requirements explicit before implementation
* separates investigation, planning, implementation, and review
* gives the developer approval gates at important decisions
* produces an evidence trail for what was investigated and why
* catches incorrect assumptions before they become code
* can be exercised and improved incrementally

The process is itself being developed as a project. Each stage is built, exercised, evaluated, and revised before depending on it for later stages.

The target pipeline is:

**spec → plan → adversarial plan review → implement → code review → verify → hardware test → document**

Only stages explicitly marked as available are operational. The later stages described in this document are the intended design, not available tooling.

---

## 2. Design principles

### 2.1 Investigation before implementation

The AI should investigate the existing code and data before proposing implementation details.

The process should prefer evidence from:

* the existing repository
* the Windows reference implementation
* project documentation
* real game data
* existing verification tools
* hardware findings

over assumptions based on names, conventions, or what seems likely.

### 2.2 Plans are reviewable artifacts

A plan should be understandable and reviewable independently of the conversation that produced it.

A plan should explain:

* what the feature does
* what existing behavior it depends on
* where changes are expected
* how the implementation will work
* how the result will be verified
* known risks and unanswered questions

### 2.3 Approval is an explicit gate

The developer owns approval decisions.

AI-generated artifacts can be drafted, reviewed, and revised, but the process should not treat an artifact as approved merely because the AI believes it is correct.

Important gates include:

**SPEC DRAFTED → SPEC REVIEWED → SPEC APPROVED**

and later:

**PLAN DRAFTED → PLAN REVIEWED → PLAN APPROVED**

An implementation stage must not silently bypass an approval gate.

### 2.4 Adversarial review

Review stages should actively search for incorrect assumptions rather than merely summarize or agree with the preceding artifact.

The reviewer should ask:

* What evidence supports this?
* What might be missing?
* What assumptions are unverified?
* Does the proposed behavior actually match the reference?
* Are there edge cases?
* Does the verification strategy prove the intended behavior?
* Could the proposed implementation affect unrelated behavior?

### 2.5 Incremental implementation

The process should favor small, testable changes.

A successful stage should produce a concrete artifact or capability that can be exercised before the next stage is added.

The process itself should not become a large collection of untested automation.

---

## 3. Pipeline overview

| Stage | Purpose                              | Status                           |
| ----- | ------------------------------------ | -------------------------------- |
| A     | Repository context and working rules | **Available**                    |
| B     | Create a feature specification       | **Available and exercised**      |
| C     | Create an implementation contract    | **Available and exercised**      |
| D     | Adversarial plan review              | **Available and exercised**      |
| E     | Implementation                       | **Available, not yet exercised** |
| F     | Code review                          | Planned                          |
| G     | Verification                         | Planned                          |
| H     | Hardware test                        | Planned                          |
| I     | Documentation and completion         | Planned                          |

The stages are intentionally separated.

A later stage should consume the artifact produced by the preceding stage rather than reconstructing the previous stage's reasoning from conversation history.

### 3.1 Artifacts are separated by audience, not only by stage

A stage C plan has two readers with incompatible needs. A reviewer and a
developer need the evidence — how a number was measured, what else was tried,
why this design. An implementation agent needs none of it, and pays for all of
it: every paragraph that is not an instruction is a paragraph it must first
classify as background before it can act.

So stage C produces two documents, and the split is by audience:

* **`plan.md` — the implementation contract.** What to do, in order, with what
  must not change and how each milestone is verified. Stage E reads this.
* **`plan-evidence.md` — the investigation.** Traces, measurements, rejected
  alternatives, risk analysis. Stage D verifies against it; stage E consults it
  only when a question needs it.

`log.md` is the third: the append-only history of how the artifacts got to their
current state. It already records every review and refinement, which is why no
artifact needs to narrate its own history — and why none of them may.

The failure this prevents is specific and was observed on feature 032 before the
split existed: an adversarial review loop working correctly, each cycle adding a
correction and a response, until the implementation-facing content was a
minority of a document nobody could hand to an implementer.

---

## 4. Stage A — Repository context

**Status: Available**

Stage A is the repository context provided by `CLAUDE.md` and the supporting documentation.

Its purpose is to give Claude the persistent project knowledge required to work safely in the repository.

This includes:

* repository structure
* build instructions
* verification philosophy
* working agreement
* known traps
* architecture boundaries
* standing design rules
* documentation conventions
* Git rules
* development-process rules

`CLAUDE.md` should remain concise. Detailed knowledge belongs in the appropriate `docs/` document.

Stage A is not a workflow command. It is the context on which the later workflow operates.

---

## 5. Stage B — Specification

**Status: Available and exercised**

Stage B converts a backlog item into a structured feature specification.

### Command

`/spec NNN`

`NNN` is the backlog row number without zero padding.

For example:

`/spec 16`

The resulting specification lands in a zero-padded feature folder:

`docs/features/016-<slug>/spec.md`

### Agent

`.claude/agents/spec-author.md`

### Supporting skill

`.claude/skills/grill-me/SKILL.md`

The spec-author investigates the requested feature, asks questions where requirements are unclear, and produces the specification artifact.

The specification establishes **what the feature should do** without prematurely becoming an implementation plan.

### Specification location

`docs/features/NNN-<slug>/spec.md`

Specifications are indexed by:

`docs/features/README.md`

### Approval

A specification is not considered approved merely because it was generated.

The developer explicitly approves the specification before it can be consumed by Stage C.

---

## 6. Stage B evaluation

Stage B has been exercised end to end against backlog row 16.

The exercise demonstrated that the process can take a backlog item through investigation and specification rather than relying on an informal conversation.

Four defects were discovered during the exercise and corrected.

This is an important part of the process design: exercising the workflow is itself a test of the workflow.

The evaluation should therefore record:

* what was attempted
* what worked
* what failed
* what was changed
* what evidence demonstrated the correction

The detailed evaluation belongs in this document rather than in `CLAUDE.md`.

---

## 7. Stage C — Implementation planning

**Status: Available and exercised**

Stage C converts an **APPROVED** specification into an implementation contract
and the evidence behind it.

### Command

`/plan NNN`

The command consumes the approved specification associated with the backlog row.

For example:

`/plan 16`

### Agent

`.claude/agents/planner.md`

The planner investigates the repository and reference implementation as necessary and produces an implementation plan based on the approved specification.

The planner's mandate is **investigate at full depth, write the instructions at minimum sufficient depth**. Depth of investigation and length of instruction are separate dials, and the process turns them in opposite directions.

The planner should not redefine the feature simply because implementation investigation reveals a different approach.

If the approved specification appears incorrect or incomplete, that should become an explicit issue requiring specification revision rather than being silently changed in the plan.

### Plan location

New pipeline plans use:

`docs/features/NNN-<slug>/plan.md` — the contract
`docs/features/NNN-<slug>/plan-evidence.md` — the investigation

Older plans stored directly under `docs/plans/` predate this pipeline and remain where they are. That directory is frozen: pipeline work gets a feature folder under `docs/features/` instead.

### Plan contents

`plan.md` §1–§7 is the contract, and it is what stage E reads:

1. Objective
2. Approved behavior, each item traced to the spec
3. Constraints, exclusions and hazards
4. Implementation approach, stated as decisions
5. Files and changes
6. Verification
7. Milestones and stop conditions

§8–§10 are the record: open questions, decisions, and deviations found during
implementation.

`plan-evidence.md` carries the reference trace, the survey of the port, rejected
alternatives, every measurement with the command that produced it, the risk
analysis, and what could not be established.

Between them the two documents contain enough evidence that another developer
can review the proposed implementation without repeating the investigation —
while the contract alone remains sufficient to implement from.

### The contract budget

`plan.md` §1–§7 should come to roughly **400 lines**.

The budget is a diagnostic, not a quality target. Length is not the defect;
mixed genres are. A contract well over budget almost always contains a
measurement, a rejected alternative, or an account of how a decision was
reached — material that belongs in `plan-evidence.md` or is already in `log.md`.
The remedy is to move it, or to split a milestone that is doing too much. It is
never to cut a step the implementer needs.

Three rules keep the contract usable across refinement cycles:

* Every paragraph in §1–§7 is an instruction, a constraint, or a verification.
* Decisions are stated, not argued; the argument is one pointer away.
* No artifact narrates its own history. `log.md` does that.

The third is the one that decays first, because each individual violation looks
like diligence.

---

## 8. Stage D — Adversarial plan review

**Status: Available and exercised**

Stage D reviews an implementation plan independently of the planner that created it.

### Command

`/review-plan NNN`

### Agent

`.claude/agents/plan-reviewer.md`

The purpose is to find problems before implementation begins.

The review challenges:

* requirements coverage
* technical assumptions
* reference behavior
* affected code
* edge cases
* compatibility concerns
* verification adequacy
* unexplained deviations
* unnecessary scope

### Two independent judgements

The review ends in a **verdict** — `APPROVED`, `CHANGES REQUESTED` or `BLOCKED` — and, separately, an **implementation handoff assessment** — `READY` or `NEEDS RESTRUCTURING`.

They are independent because a plan can be right and unusable. The verdict judges whether the design is correct. The handoff assessment judges whether an implementation agent could execute `plan.md` §1–§7 without the planning conversation and without reading the evidence file, across seven dimensions: scope, sequence, decisions, dependencies, verification, separation and handoff.

`APPROVED` with `NEEDS RESTRUCTURING` is a real and useful outcome: the design stands and the contract needs rearranging. It is a far cheaper fix than a re-plan.

### Restructuring findings are moves, not additions

A handoff finding names what to move where, what to cut, or where a milestone splits. It never asks the plan to add an explanation, a justification, or a record of the review.

This constraint is what makes the dimension safe. Without it, a review that flags "the contract is too big" produces a refinement that adds an account of the flag — growing the artifact it was meant to shrink. Stage C's refinement command carries the matching rule: **revise in place.**

`NEEDS RESTRUCTURING` is never a comment on how much the plan knows. A lean contract beside a thousand-line evidence file is exactly right.

### Iteration

A review finding results in one of:

* plan accepted
* plan revised and reviewed again
* specification identified as incorrect or incomplete

The review should have a bounded iteration mechanism so that an unresolved disagreement does not create an uncontrolled loop.

Implementation requires an explicitly approved plan.

---

## 9. Stage E — Implementation

**Status: Available, not yet exercised**

The implementation stage consumes an approved plan and builds **one milestone** of it.

### Command

`/implement NNN [milestone]`

With no milestone given, the first one not yet implemented.

### Agent

`.claude/agents/implementer.md`

### What it does

1. Implement only the approved scope of one milestone.
2. Follow the repository working agreement.
3. Build the `.pkg`.
4. Run the plan's §6 automated verification.
5. Produce `implementation-report.md`.
6. Stop at the milestone's completion gate, and hand off the hardware test.

It does not create a branch or a commit — `CLAUDE.md` §9 leaves the working tree to the developer.

### What it reads

`plan.md` §1–§7, and the spec's §10 so it can recognise a conflict.

`plan-evidence.md` is a reference it consults on a specific question, not reading it does before starting. That distinction is the entire reason stage C produces two documents.

§8–§10 of the plan and `log.md` are not instructions. §10 is where the implementer's own deviations go, and it is the only part of the plan stage E may write.

### Stopping is a successful outcome

The plan's §7 carries explicit stop conditions — a build failing in a way the plan did not anticipate, a verification failure with no obvious cause, a decision that would contradict the spec, a required behavior that cannot be implemented as described, a file the plan did not name, an invariant that cannot be preserved.

The implementation agent should not silently redesign the feature when it encounters a problem with the plan. Halting early with a clear account of where and why is the designed behavior, not a failure of the run.

If the plan is materially wrong, the process returns to the planning stage.

---

## 10. Stage F — Code review

**Status: Planned**

The code review stage will compare the implementation against:

* the approved specification
* the approved implementation plan
* repository conventions
* relevant reference behavior
* applicable verification requirements

The review should identify both implementation defects and cases where the implementation has drifted from the approved design.

A passing code review does not replace runtime verification or hardware testing.

---

## 11. Stage G — Verification

**Status: Planned**

Verification will use the strongest applicable evidence available for the feature.

Depending on the feature, this may include:

* successful OpenOrbis compilation
* Python verification tools
* deterministic fixtures
* byte-level comparisons
* vanilla-versus-randomized comparisons
* saved output inspection
* regression checks

Passing a verifier does not automatically prove that the C++ implementation behaves correctly at runtime.

The existing verification philosophy in `docs/testing.md` remains authoritative for the distinction between build verification, Python verification, and hardware testing.

---

## 12. Stage H — Hardware testing

**Status: Planned**

The PS4 is the final runtime authority

---

## 13. Artifact layout

Every artifact belonging to one work item lives in one folder.

```text
docs/
    features/
        README.md                     the pipeline index
        _templates/
            spec.md  spec-review.md
            plan.md  plan-evidence.md  plan-review.md
            implementation-report.md
        016-unchanged-bell-maidens/
            log.md                     append-only history, every stage
            spec.md                    stage B
            plan.md                    stage C — the implementation contract
            plan-evidence.md           stage C — the investigation behind it
            plan-review.md             stage D
            implementation-report.md   stage E, one per milestone
```

Each file has one audience and one genre. `plan.md` holds instructions,
`plan-evidence.md` holds the reasoning that justifies them, and `log.md` holds
the chronology of how both arrived. Material in the wrong file is a defect even
when it is correct — that is what stage D's handoff assessment checks.

The folder is named `NNN-<slug>`, where `NNN` is the backlog row number from
`docs/randomization-feature-spec.md` zero-padded to three digits, and `<slug>`
is the feature name lowercased and hyphenated. Files inside are named for the
stage that produces them, so `docs/features/*/spec.md` reaches every spec.

Where one item covers several adjacent backlog rows, number the folder after the
lowest row it covers and name the rest in the spec's §6 Scope.

### `log.md` — the feature's append-only record

One per feature folder. Every stage appends; no stage rewrites or deletes.

It is the neutral ground a cross-stage finding needs: the plan belongs to the
planner and the implementation report to the implementer, so neither can hold a
finding *about* the other. It is also the answer to "what is outstanding on this
feature?"

Entries are never edited once written. A finding that later turns out to be
wrong gets a **new** entry saying so.

`docs/plans/ai-dev-process-vision.md` §6 carries the fuller rationale, including
the per-entry fields and the artifact types the later stages will add.

### Legacy plans

`docs/plans/` holds flat plan files that predate this pipeline. Several cover
multiple backlog items, so assigning each a single feature number would create
false precision. They remain useful as reference material and as house-style
examples, but the directory is frozen — new pipeline work gets a feature folder.
