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
| E     | Implementation                       | **Available and exercised**      |
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

**Status: Available and exercised** — the worlds feature's six milestones and the startup screen's two, 2026-09-22 to 2026-09-26.

The implementation stage consumes an approved plan and builds the milestones its **Execution Strategy** calls for.

### Structure and gates are two decisions

**Milestones describe implementation structure. Gates describe when a human stops to test. They are independent.** (`CLAUDE.md` §4.)

A milestone answers *what meaningful piece of functionality becomes complete here?* — never *what can we make independently testable here?* Splitting work to create a test checkpoint produces scaffolding nobody uses and seams that deform the code.

Every plan carries an **Execution Strategy** block near the top, and approving the plan approves it:

```
Structure                  4 functional milestones
Execution                  Continuous
Human test gates           Final only
Intermediate verification  Build + automated checks after each milestone
```

| Setting | Options | Default |
| --- | --- | --- |
| Structure | single implementation, N functional milestones, N phases with sub-work | whatever produces the cleanest implementation |
| Execution | continuous, or gated after named milestones | continuous |
| Human test gates | final only, after selected milestones, after every milestone | final only |
| Intermediate verification | which checks run at each boundary | build plus the applicable automated checks |

Gates are classified, and the classification is written into the plan:

* **Required** — data could be corrupted or destroyed; or a later step would make diagnosing a failure here substantially harder; or the next milestone depends on confirming something build and static verification cannot establish.
* **Optional** — testing here gives useful confidence but nothing depends on it. The developer decides at approval.
* **No gate** — the default. The feature is completed and validated at the end.

**Continuous execution still verifies at every boundary.** The implementer builds the `.pkg` and runs the applicable checks after each milestone, and a failure ends the run there. That localises a failure to one milestone at almost no cost; only the *human* test cycle is expensive, and that is what a gate spends.

### Why this replaced "every milestone stops"

The original rule made every milestone a hardware-test gate, and the project paid for it twice.

The **worlds** feature built an operations harness — `Platform/SaveDataProbe` and `UI/SaveProbeScreen`, around 400 lines — specifically so milestones 1–3 could be hardware-tested, maintained it across three milestones, then deleted it in milestone 6 before it was ever used for that purpose. Two hardware tests were permanently lost with it.

The **startup screen**'s milestone 1 deliberately kept `DrawStartup` and `FinishStartup` alive so milestone 2 would be distinguishable on a television. That one shaping decision produced a §5/§7 contradiction in the plan, a deferred constant with a compiler-warning workaround, and a latent trap punted to the next milestone instead of solved once — three of the six recorded deviations.

In both cases the developer tested once, at the end, as they had said they would.

### Command

`/implement NNN [milestone | list | range | all]`

With nothing given, follow the plan's Execution Strategy: under continuous execution, every milestone up to the next gate, or all of them when there is none.

### Agent

`.claude/agents/implementer.md`

### What it does

1. Implement only the approved scope of the milestones in its work order.
2. Follow the repository working agreement.
3. Build the `.pkg`.
4. Build and run the plan's §6 automated verification **after each milestone**, including ones it does not stop at.
5. Produce `implementation-report.md`.
6. Continue to the next milestone, or stop where the Execution Strategy places a gate and hand off the hardware test.

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

**Status: Planned as a stage. Practised by hand.**

The PS4 is the final runtime authority (`CLAUDE.md` section 3), and no agent can
reach one, so this stage will never be automated the way B–E are. What it would
formalise is the handoff: the procedure the developer executes, and where the
result is recorded.

The shape that practice has settled into, from the worlds feature:

* every milestone's `implementation-report.md` ends with a hardware handoff —
  what to do, what to expect, what a failure looks like;
* an item with tests that need sequencing across milestones gets one
  `hardware-test-plan.md` instead, ordered so each test is safe before the next;
* the result is appended to `log.md` with the numbers checked independently
  rather than taken from the tester's summary, and the feature's status lines
  are updated only then.

**One lesson is worth carrying into the stage when it is built.** The worlds
feature built a probe harness specifically so milestones 1–3 could be hardware
tested, then deleted it in milestone 6 — before it had ever been used, because
testing was deferred to the end. Two tests were lost with it. A milestone's
test scaffolding has to outlive the milestone that tests through it.

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
        033-protect-caged-dogs/
            log.md                     append-only history, every stage
            spec.md                    stage B
            plan.md                    stage C — the implementation contract
            plan-evidence.md           stage C — the investigation behind it
            plan-review.md             stage D
            implementation-report.md   stage E, one per milestone
            hardware-test-plan.md      optional — see below
            technical-findings.md      optional — see below
```

Two further files appear when an item needs them, and neither is produced by a
stage:

* **`hardware-test-plan.md`** — one ordered procedure covering a whole feature,
  written when an item has more hardware tests than a single milestone handoff
  can sensibly carry. `worlds/` has the only one so far. A per-milestone
  handoff inside `implementation-report.md` remains the default; reach for this
  only when the tests need sequencing across milestones.
* **`technical-findings.md`** — hardware facts established by a probe built for
  this item specifically. Anything of lasting platform value belongs in
  `docs/ps4-homebrew-findings.md` instead, and should be promoted there.

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
