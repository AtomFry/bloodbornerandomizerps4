# AI-Assisted Development Process — Vision and Build Plan

**Status: Stages A–D built. A, B and C exercised on real work 2026-09-15; D built the same day and not yet exercised. Stages E–I not started.** See §13 for the evaluations and §8 for the build order.

This document is the plan we work through. §8 tracks what exists.

> **Retired as the process definition (2026-09-17).** `docs/ai-dev-process.md`
> is now the canonical description of how the pipeline runs, and it is what the
> commands and agents point at. This document is kept for what only it records:
> the workflow principles §3.1–3.7 (the commands still cite §3.7, and `log.md`
> cites §3.6), the build order §8, the open questions §11, the deferred
> automation §12, and the progress log §13.
>
> Two things here are deliberately **not** updated. Its stage numbering is the
> original 0/1/2 scheme; the canonical doc and `CLAUDE.md` use the letters
> A–I, so stage 0 is stage B, stage 1 is stage C, and stage 2 is stage D. And
> its artifact paths are pre-migration — specs were at `specs/NNN-<slug>.md`
> and plans at `docs/plans/NNN-<slug>/` before both moved under
> `docs/features/NNN-<slug>/`. The evaluation entries are an append-only record,
> so they are left as they were written rather than rewritten to match.

## 1. Purpose and Goal

The goal is to establish a repeatable AI-assisted development workflow for getting a feature from a backlog item to **confirmed working on hardware**, with defined handoff artifacts between stages and explicit points where a human makes decisions.

The workflow should improve the quality and consistency of development while preserving human ownership of requirements, design decisions, validation, and final approval.

The long-term goal is an increasingly automated development pipeline.

The immediate goal is **not** full automation.

We will build the workflow incrementally:

1. Define a stage.
2. Build the minimum tooling required to support it.
3. Run it against real work.
4. Evaluate the quality and cost of the result.
5. Refine the stage.
6. Only then connect it to the next stage.

This allows the development process itself to be treated as something we can test and improve.

### Core principle

**Each stage produces a defined artifact that becomes the input to the next stage.**

The artifact is the contract between stages.

A later agent should be able to perform its job from the artifacts provided to it without relying on the conversation that produced those artifacts.

This provides:

* repeatability
* resumability
* traceability
* clearer human review points
* a foundation for future automation

---

## 2. The Honest Risk

A formal AI-assisted workflow has a real cost.

Every stage adds work and every artifact adds something that someone has to read. On a solo hobby project, that person is usually the developer.

The primary failure mode is therefore process abandonment:

> The workflow becomes so elaborate that using it feels harder than simply doing the work.

The design should actively guard against this.

Two principles address that risk:

### Incremental implementation

We build the stages individually and use them on real work before building additional stages.

If the process is producing unnecessary ceremony, we want to discover that after building 20% of it rather than after building the entire system.

### Appropriate process for the size of the change

The workflow should eventually support a lighter path for genuinely small changes.

The exact fast-lane rules should be determined from actual usage rather than designed entirely in advance.

---

## 3. Workflow Principles

### 3.1 Human ownership

AI agents provide analysis, recommendations, implementation, review, verification, and documentation.

The developer remains responsible for:

* understanding the requested behavior
* approving the specification
* approving the implementation plan
* reviewing generated code
* validating the result
* deciding whether the feature is complete

An agent's `APPROVED` verdict means:

> The agent recommends proceeding based on the information it was given.

It does not constitute human approval.

### 3.2 Reviewers use fresh context

Adversarial review stages should run in independent context from the agent that produced the artifact being reviewed.

A reviewer should receive the relevant source artifacts and repository information required for the review.

The reviewer should not inherit the author's reasoning through conversational context.

This reduces the risk that the reviewer simply accepts assumptions established by the author.

### 3.3 Reviewers do not modify what they review

A reviewer produces findings.

The author or implementation stage is responsible for addressing those findings.

This preserves the distinction between:

> creating an artifact

and:

> evaluating an artifact.

It also preserves an audit trail when an artifact goes through multiple revisions.

### 3.4 Never silently change the contract

If implementation discovers that the approved plan is wrong or incomplete, the implementation agent should report the deviation rather than silently changing the plan to match the code.

Likewise, a failed test should cause the workflow to reconsider the implementation rather than simply changing the expected behavior to match the result.

The history of the decision should remain visible.

### 3.5 Evidence over confidence

Agents should distinguish between:

* facts established from repository evidence
* conclusions inferred from that evidence
* assumptions that still need verification

Confidence should include a reason.

A confident statement without supporting evidence is not sufficient for an implementation decision.

### 3.6 Work flows backward as well as forward

§3.4 states the rule for one boundary — implementation discovering that the plan
is wrong. It applies at **every** boundary. A planner finds the spec wrong; a
reviewer finds the plan incomplete; a hardware test fails. Development is
collaborative and things surface late, so the pipeline needs a defined way for a
finding to travel upstream. Five rules make that safe.

**1. A finding is not communicated until it is written to a file.** Agents are
stateless and their reports evaporate the moment the session ends. This is the
difference between an agent pipeline and a human team, where a conversation is
enough. Stage C's first run proved it: the planner found three errors in an
approved spec, and they survived only because the dispatching session relayed
them by hand. Nothing in the repository would have carried them to the
implementer.

**2. The stage that owns an artifact is the only one that changes it.** The
discoverer reports; the owner fixes. This is §3.3 generalised beyond reviewers.
An agent never edits an upstream artifact, and never edits an approved one at
all.

**3. Classify the finding, because only one class may halt the pipeline.**

| Class | Meaning | Handling |
|---|---|---|
| **Correction** | The upstream artifact is factually wrong, but no approved decision changes | Fix forward, record, continue. Approval survives |
| **Assumption** | The upstream artifact is silent or ambiguous, and work can proceed under a stated assumption | Record the assumption explicitly, continue, and have the owner ratify it at the next gate |
| **Contradiction** | The finding invalidates a decision a human approved | **Halt.** Return to the owning stage. The human re-approves before work resumes |

Without this split, a pipeline that can talk backwards never finishes: every
stage bounces everything upstream. Most findings are Corrections.

**4. Every stage may terminate `BLOCKED`, not only `DONE`.** An agent that hits
a contradiction cannot ask anyone — it has no interactive channel. If its only
legal output is the artifact it was asked to produce, it will guess, and a guess
is exactly the silent contract change §3.4 forbids. Stopping with a finding is a
successful outcome for that run.

**5. Staleness propagates, and the human is the circuit breaker.** When a
decision changes, every artifact derived from it becomes suspect: amending a
spec makes its plan `STALE`, which makes the plan review `STALE`. Mark them,
never delete them. Only a human authorises rework, which bounds the loop.

> **A feature that bounces twice does not need a third patch — it needs its spec
> redone.** Repeated rework is evidence the requirements were wrong, not that the
> plan needs another revision.

### 3.7 Refinement is re-running the owner, not editing the file

Feedback on an artifact is handled by `/refine-<artifact>`, which re-dispatches
**the agent that owns it** with the existing artifact plus the feedback —
`spec-author` for a spec, `planner` for a plan. A generic editor would break
§3.6 rule 2 and would not carry the owning agent's conventions.

Refinement declares its own severity, and the status follows:

| Refinement | Effect |
|---|---|
| No approved decision changes | Status survives; the change is logged as an amendment |
| An approved decision changes | Status drops to `QUESTIONS ANSWERED`; re-approval required, and downstream artifacts are marked `STALE` |

The agent proposes which case applies; the human confirms. Approval that can be
edited away silently is not approval.

---

## 4. The Pipeline

The initial pipeline consists of stages separated by human decision gates.

| #  | Stage                   | Trigger            | Actor                | Reads                                      | Produces                                  |
| -- | ----------------------- | ------------------ | -------------------- | ------------------------------------------ | ----------------------------------------- |
| 0  | **Spec**                | `/spec 022`        | Agent                | backlog row, relevant code/docs            | `specs/022-<slug>.md`                     |
| 0h | **Spec Approval**       | —                  | Human                | spec                                       | approve / revise                          |
| 1  | **Plan**                | `/plan 022`        | Agent                | spec, code, prior plans                    | `plan.md`                                 |
| 2  | **Plan Review**         | `/review-plan 022` | Agent, fresh context | spec + plan                                | `plan-review.md` + verdict                |
| 2h | **Plan Approval**       | —                  | Human                | spec + plan + review                       | approve / revise                          |
| 3  | **Implement**           | `/implement 022`   | Agent                | approved plan                              | code changes + `implementation-report.md` |
| 4  | **Code Review**         | `/review-code 022` | Agent, fresh context | plan + diff + implementation report        | `code-review.md` + verdict                |
| 4h | **Code Approval**       | —                  | Human                | plan + diff + review                       | fix / build / test                        |
| 5  | **Verify**              | `/verify 022`      | Agent                | plan verification section + implementation | verifier changes + `test-report.md`       |
| 5h | **Hardware Validation** | —                  | Human + PS4          | built `.pkg` + test report                 | pass / fail recorded in `test-report.md`  |
| 6  | **Document**            | `/document 022`    | Agent                | completed artifacts + test report          | documentation + release note              |

The pipeline may eventually be orchestrated automatically, but the first implementation will use explicit human triggers.

### The backward edges

The table above is the forward path. It is not the whole pipeline, because work
does not only move forward (§3.6). Three things travel back up it:

| Trigger | Actor | Effect |
|---|---|---|
| `/refine-spec NNN "<feedback>"` | Human → `spec-author` | Re-runs the owning agent on an existing spec with feedback. Same for `/refine-plan` (§3.7) |
| A stage terminates `BLOCKED` | Agent | The stage stopped on a **Contradiction** rather than producing its artifact. The finding goes in `log.md`; the human decides where rework starts |
| A gate returns *revise*, or hardware returns `FAIL` | Human | Rework at whichever stage the human names, with downstream artifacts marked `STALE` |

**Every stage can therefore end in one of three states, not two:** it produced
its artifact, it produced its artifact *with findings recorded*, or it stopped
`BLOCKED`. Only the human restarts a blocked pipeline.

**Findings are written, never spoken.** Each numbered feature folder carries an
append-only `log.md` (§6). An agent that finds a problem in an artifact it does
not own appends there; it does not edit the artifact and does not rely on its
own report reaching anyone. A report ends when the session does.

---

## 5. Stage Definitions

### Stage 0 — Spec

**Purpose**

Turn a short backlog item into a sufficiently clear specification for planning.

The backlog index is intentionally lightweight. A row such as:

> "row 22: scaling"

does not provide enough information for a planning agent to reliably determine the intended behavior.

The Spec Agent should:

* read the backlog item
* inspect relevant existing code and documentation
* use the `grill-me` skill when clarification is needed
* identify the intended user-visible behavior
* identify relevant constraints
* document important terminology
* identify unanswered questions

The Spec Agent does not implement code.

**Output**

`specs/NNN-<slug>.md`

**Important**

The Spec stage is experimental.

After several real features have passed through the workflow, evaluate whether the Spec stage provides enough value to justify its cost.

It may eventually prove that a good backlog item combined with structured interrogation during planning is sufficient.

---

### Stage 1 — Plan

**Purpose**

Determine how the approved specification should be implemented.

The Planning Agent should:

* read the specification
* inspect the existing implementation
* inspect related plans and documentation
* identify the relevant architecture
* identify existing functionality that can be reused
* determine the likely implementation approach
* identify alternatives where meaningful
* identify risks and unknowns
* describe how the result will be verified

The Planning Agent:

* does not write production code
* does not create a branch
* does not commit changes

**Output**

`docs/plans/NNN-<slug>/plan.md`

The plan is the handoff artifact between planning and plan review.

---

### Stage 2 — Plan Review

**Purpose**

Perform an adversarial evaluation of the proposed implementation.

The Plan Review Agent should attempt to find:

* misunderstood requirements
* unsupported assumptions
* missing repository investigation
* incorrect architectural conclusions
* unnecessary complexity
* missing edge cases
* missing verification
* risks the planner failed to identify
* ways the proposed implementation could produce incorrect behavior

The Plan Review Agent:

* runs in fresh context
* does not modify the plan
* does not write production code

**Output**

`plan-review.md`

The review ends with exactly one verdict:

`APPROVED`

or

`CHANGES REQUESTED`

or

`BLOCKED`

### Verdict definitions

**APPROVED**

The reviewer believes the plan is sufficiently complete and internally consistent to proceed to human review.

**CHANGES REQUESTED**

The reviewer identified specific issues that should be addressed before human approval.

Each finding should identify the relevant plan section or repository location where possible.

**BLOCKED**

The artifact cannot be meaningfully reviewed because required information is missing, contradictory, or unresolved.

---

### Human Plan Approval

The developer reads:

* the original specification
* the implementation plan
* the plan review

The developer decides whether the plan accurately represents what should be built and whether it is sufficiently complete to authorize implementation.

The developer may:

* approve the plan
* request changes
* reject the proposed approach

---

### Stage 3 — Implementation

**Purpose**

Implement the approved plan.

The Implementation Agent should:

* read the approved plan
* inspect the relevant repository code
* implement the planned changes
* perform appropriate local validation
* report deviations from the approved plan
* identify newly discovered risks or questions

The Implementation Agent:

* does not silently rewrite the plan
* does not commit changes unless explicitly instructed
* does not decide that a changed requirement is acceptable without human involvement

**Outputs**

* working tree changes
* `implementation-report.md`

The implementation report should include:

* what was implemented
* what was verified
* deviations from the plan
* why each deviation occurred
* new risks or questions
* remaining limitations

The implementation report becomes part of the evidence reviewed during code review.

---

### Stage 4 — Code Review

**Purpose**

Perform an adversarial review of the implementation against the approved plan and functional goal.

The Code Review Agent should evaluate:

* functional correctness
* compliance with the approved plan
* architectural consistency
* unintended behavior
* error handling
* edge cases
* maintainability
* security or data-integrity concerns where relevant
* adequacy of verification
* deviations documented by the Implementation Agent

The Code Review Agent:

* runs in fresh context
* does not modify the implementation
* does not modify the plan

**Output**

`code-review.md`

The review ends with:

`APPROVED`

or

`CHANGES REQUESTED`

or

`BLOCKED`

For this project, the repository review artifact is the authoritative AI review record because the workflow does not currently depend on a remote PR system.

---

### Human Code Approval

The developer reads:

* specification
* implementation plan
* implementation report
* code changes
* code review

The developer decides whether the implementation is ready for building and testing.

This is the point where the developer can:

* request implementation changes
* resolve review findings
* build the application
* proceed to verification

---

### Stage 5 — Verification

**Purpose**

Provide automated or repeatable evidence that the implementation behaves as expected.

The exact testing strategy will evolve as the project evolves.

For this project, the current environment does not provide a practical C++ unit-testing environment because the host clang environment does not provide the required standard library.

The existing project therefore uses Python verification tools that mirror relevant rules and evaluate them against real game data.

Existing examples such as:

* `app/tools/pool_verify.py`
* other `*_verify.py` tools

provide the initial precedent.

The Verification Agent should determine the appropriate verification approach for each feature.

Possible approaches include:

* existing automated tests
* Python data verifiers
* black-box tests
* characterization tests
* static validation
* filesystem validation
* build validation
* manual verification instructions

The Verification Agent should not invent an automated test simply to satisfy the pipeline.

If meaningful automated verification is unavailable, it should document that limitation and provide a concrete manual verification procedure.

**Output**

`test-report.md`

The report should distinguish between:

* automated verification
* local application verification
* manual verification still required
* hardware verification

---

### Human Hardware Validation

The PS4 is the final execution environment.

The developer performs the hardware test using the built `.pkg`.

The hardware test should verify the externally observable behavior relevant to the feature.

The result is recorded in `test-report.md`.

Possible outcomes:

`PASS`

`FAIL`

`BLOCKED`

A failed hardware test sends the feature back into the implementation workflow.

The failure should be recorded as evidence rather than silently changing the plan to match the observed behavior.

---

### Stage 6 — Documentation / Delivery

**Purpose**

Prepare the completed feature for communication and delivery.

The Documentation Agent reads the completed development artifacts and produces the appropriate documentation.

Possible outputs include:

* user-guide changes
* release-note text
* specification status update
* demo talking points
* instructions for screenshots or video capture

The agent cannot capture a real PS4 demonstration from the development environment.

A practical future workflow is for the agent to identify the exact screens or behaviors that should be captured, after which the developer supplies the screenshots or video.

---

## 6. Artifact Structure

The intended structure is:

```text
specs/
    README.md
    016-unchanged-bell-maidens.md
    024-melee-movesets.md

docs/
    plans/
        _templates/
            spec.md
            plan.md
            plan-review.md
            implementation-report.md
            code-review.md
            test-report.md

        016-unchanged-bell-maidens/
            log.md
            plan.md
            plan-review.md
            implementation-report.md
            code-review.md
            test-report.md

        boss-randomization.md
        boss-and-treasure-findings.md
        mergo-darkness.md
        param-features.md
        pickers.md
        seed-entry.md
        starting-weapons.md
        treasure-randomization.md
        ui-scrolling.md
        workshop-tools.md
```

### `log.md` — the feature's append-only record

One per feature folder. Every stage **appends**; no stage rewrites or deletes.

It exists because of §3.6 rule 1: a finding that lives only in an agent's report
is lost when the session ends. `log.md` is the neutral ground a cross-stage
finding needs — the plan belongs to the planner and the implementation report to
the implementer, so neither can hold a finding *about* the other.

Each entry records:

- the date and the stage that raised it
- which artifact the finding is about
- its class — **Correction**, **Assumption**, or **Contradiction** (§3.6)
- what was found, with evidence
- what happened: fixed forward, proceeding under an assumption, or halted

It is also the answer to "what is outstanding on this feature?", and it is what
makes §3.4's *"the history of the decision should remain visible"* true in
practice rather than in principle.

Entries are never edited once written. A finding that later turns out to be
wrong gets a **new** entry saying so.

### Numbered feature folders

New work entering the formal workflow receives a numbered folder:

`docs/plans/NNN-<slug>/`

The folder becomes the durable record for that work item.

### Existing plans

Existing flat plans remain in `docs/plans/`.

Several of them cover multiple backlog items, so assigning them a single feature number would create false precision.

They remain useful as reference material and provide examples from which future templates can be derived.

### Backlog index

`docs/randomization-feature-spec.md` remains the master backlog inventory and status board.

It should contain:

* feature number
* feature name
* status
* link to the specification

The individual feature specification contains the detail required by the planning stage.

The index should eventually be updated automatically by the Documentation Agent.

---

## 7. Claude Code Mechanics

The workflow uses four distinct Claude Code mechanisms.

| Mechanism          | Location                         | Purpose                                                |
| ------------------ | -------------------------------- | ------------------------------------------------------ |
| **Project memory** | `CLAUDE.md`                      | Standing facts and rules applying to all work          |
| **Slash command**  | `.claude/commands/`              | Human-facing workflow trigger                          |
| **Subagent**       | `.claude/agents/`                | Specialized actor with its own context and permissions |
| **Skill**          | `.claude/skills/<name>/SKILL.md` | Reusable procedure or specialized knowledge            |

The conceptual model is:

> **Commands are triggers. Agents are actors. Skills are reusable procedures. `CLAUDE.md` contains standing project rules.**

### `CLAUDE.md`

`CLAUDE.md` should contain information that applies broadly across the project:

* project architecture
* build commands
* important constraints
* known hazards
* repository conventions
* testing conventions
* Git working agreement
* hardware validation requirements
* documentation conventions
* a short description of this workflow

Project-specific knowledge should not be duplicated across agent files when it can live in `CLAUDE.md`.

### Commands

Initial commands:

```text
.claude/commands/
    spec.md
    refine-spec.md
    plan.md
    refine-plan.md
    review-plan.md
    implement.md
    review-code.md
    verify.md
    document.md
```

Each creating command has a `refine-` counterpart (§3.7). A refine command does
not edit the artifact itself — it re-dispatches the **owning** agent with the
existing artifact plus the human's feedback, so the author's conventions and
standards are reapplied rather than bypassed. It also declares whether the
refinement changed an approved decision, and marks downstream artifacts `STALE`
when it did.

Commands should remain thin.

They should primarily:

* identify the work item
* locate the relevant artifacts
* invoke the appropriate agent
* pass required arguments

The detailed behavior belongs in the agent definition.

### Agents

Initial agents:

```text
.claude/agents/
    spec-author.md
    planner.md
    plan-reviewer.md
    implementer.md
    code-reviewer.md
    verifier.md
    documenter.md
```

Each agent should have a narrow responsibility.

### Skills

The initial reusable skill is:

```text
.claude/skills/
    grill-me/
        SKILL.md
```

`grill-me` provides structured interrogation of an unclear request before planning.

Additional skills should only be created when the same specialized procedure or knowledge is genuinely needed by multiple stages.

Avoid creating skills that merely duplicate information already maintained in `CLAUDE.md` or existing repository documentation.

### Fresh-context reviewers

The Plan Review and Code Review agents should run in independent context.

Before implementing these stages, verify how project `CLAUDE.md` is loaded into subagent contexts.

If it is not automatically available, provide reviewers with a concise pointer to the required project documentation rather than duplicating the entire project's knowledge into their agent definitions.

---

## 8. Build Order

Each stage should be independently useful.

**Do not build the next stage until the current stage has been exercised on real work and evaluated.**

| Stage   | What gets built                                                         | How it is proven                                                                                                |
| ------- | ----------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------- |
| **A** ✅ | `CLAUDE.md`                                                             | Fresh session can perform ordinary project work using the standing instructions without re-explanation          |
| **B** ✅ | `specs/` + index + spec template + `/spec` + `spec-author` + `grill-me` — **built and exercised 2026-09-15** on row 16; four defects found and fixed, see §13 | Run `/spec` on a real backlog item and compare the result with what the developer believes the feature requires |
| **C** ✅ | `/plan` + `planner` + plan template — **built and exercised 2026-09-15** on row 16; three gaps found, see §13 | Run `/plan` against that specification and compare the result with existing successful plans                    |
| **D** 🔨 | `/review-plan` + `plan-reviewer` + review template — **built 2026-09-15, not yet proven** | Run the reviewer against a completed plan with a known implementation outcome                                   |
| **E**   | `/implement` + `implementer` + implementation-report template           | Complete the first end-to-end planning-to-implementation workflow                                               |
| **F**   | `/review-code` + `code-reviewer`                                        | Run against a real implementation and compare findings with human review                                        |
| **G**   | `/verify` + `verifier` + test-report template                           | Produce useful verification for a real feature                                                                  |
| **H**   | `/document` + `documenter`                                              | Generate useful delivery documentation from a completed feature                                                 |
| **I**   | Orchestration, potentially `/feature 022`                               | Only after A–H have survived several real features                                                              |

### Stage D has a particularly important test

A good adversarial reviewer should be able to identify problems that actually occurred during implementation.

Use an existing completed plan where the implementation eventually revealed several deviations.

Run the Plan Review Agent against the plan **without showing it the implementation outcome**.

Evaluate whether the reviewer predicts any of the issues that were eventually discovered.

If the reviewer consistently finds nothing useful, improve the review stage before building automation around it.

---

## 9. Process Evaluation

The workflow itself should be evaluated.

After each real feature, ask:

### Did the stage provide value?

* Did it catch something important?
* Did it reduce ambiguity?
* Did it prevent rework?
* Did it improve the implementation?
* Did it produce useful documentation?

### What did it cost?

* How long did the stage take?
* How much did the developer have to read?
* Did the artifact become unnecessary ceremony?
* Did the agent repeat information already available elsewhere?

### What should change?

* Keep the stage as-is.
* Simplify the stage.
* Change the artifact.
* Change the agent instructions.
* Remove the stage.
* Automate the stage.

The workflow should be treated as an evolving system.

---

## 10. Fast Lane

A lightweight path may eventually be useful for small changes.

A potential fast lane is:

```text
Plan → Implement → Human Review → Hardware Test
```

Possible candidates include:

* documentation-only changes
* copy changes
* small settings changes
* changes that reuse existing engine behavior without altering the underlying rules
* tooling fixes with straightforward verification

Changes involving the randomizer engine, save/configuration formats, UI workflows, eligibility rules, or potentially destructive data operations should use the full workflow.

The exact boundary should be established after real usage.

The initial implementation should therefore **not optimize heavily for the fast lane**.

---

## 11. Open Questions

### 1. Does the Spec stage provide enough value?

A backlog item plus structured interrogation during planning may eventually prove sufficient.

Evaluate this after several features.

### 2. What happens when hardware testing fails? — **ANSWERED, 2026-09-15**

Settled by §3.6. A hardware `FAIL` is a finding like any other, and its class
decides the route:

- **Correction / Assumption** — the implementation is wrong but the plan and spec
  still stand. Back to implementation, and forward again through code review and
  verification. The plan is not edited to match what the hardware did.
- **Contradiction** — the failure shows the *requirement* was wrong. Back to the
  spec, which drops out of `APPROVED`, with the plan and everything below it
  marked `STALE`.

The human decides which it is. The failure is recorded in `test-report.md` **and**
in `log.md` as evidence, never by quietly changing the expected behaviour to
match the observed result (§3.4).

The original sketch, which this refines rather than replaces:

```text
Hardware failure
      ↓
test-report.md updated
      ↓
Implementation
      ↓
Code Review
      ↓
Verification
      ↓
Hardware Test
```

The plan should remain unchanged unless the human explicitly determines that the requirement itself needs to change.

### 3. How should implementation discoveries affect the plan? — **ANSWERED, 2026-09-15**

Settled by §3.6 and §3.7. The implementation agent never edits the plan. It
records deviations in `implementation-report.md`, appends the finding to
`log.md`, and — if the discovery is a **Contradiction** — terminates `BLOCKED`
rather than guessing.

The approved plan stays intact. The developer decides whether to
`/refine-plan NNN` with feedback, which re-runs the planner and, if an approved
decision changed, drops the plan's status and marks downstream artifacts `STALE`.

The original sketch, which this refines rather than replaces:

Implementation should record deviations in `implementation-report.md`.

The original approved plan remains intact.

If a material design change is required, the developer decides whether the plan needs another approval cycle.

### 4. What model should each stage use?

Different stages may eventually benefit from different model strengths.

Reviewers may benefit from stronger reasoning than documentation generation.

Defer model-specific optimization until there is enough usage to evaluate the tradeoff.

### 5. Does a subagent inherit `CLAUDE.md`? — **ANSWERED, 2026-09-15: yes**

<!--
RECONSTRUCTED 2026-09-15. The original answer text was lost when this file was
truncated by a failed script and recovered from an editor snapshot that predated
it. The finding itself is certain — it is relied on by `spec-author`, `planner`
and `plan-reviewer`, none of which carry a project-context preamble, and it was
re-confirmed by the Stage A validation run (§13), where a subagent applied the
8x8 font rule and the build process without being told either. Only the original
wording is gone.
-->

Subagents **do** inherit project `CLAUDE.md`. Agent definitions therefore carry a
pointer to the relevant sections rather than a copy of them, and fresh-context
reviewers start with the standing project rules already loaded. "Fresh context"
means the reviewer does not inherit the *author's reasoning* — it has never meant
the reviewer is ignorant of the project.

This must be established empirically before relying on project instructions being available to fresh-context reviewers.

Create a throwaway test agent and ask it to identify a known project rule from `CLAUDE.md`.

Document the result.

If project context is not inherited automatically, provide reviewers with the minimum required project documentation explicitly.

### 6. How much repository context should each agent receive?

The initial rule should be:

> Give each agent the smallest set of artifacts and repository context required to perform its job correctly.

Avoid copying large amounts of project information into every agent definition.

---

## 12. Deferred Automation

Automation should come after the individual stages have demonstrated that their outputs are useful and predictable.

### Plan ↔ Plan Review

Eventually:

```text
Plan
 ↓
Plan Review
 ↓
CHANGES REQUESTED?
 ↓ yes
Plan Revision
 ↓
Plan Review
 ↓
APPROVED
```

The loop should have:

* a maximum iteration count
* preserved review history
* a human escalation path
* a rule that unresolved disagreement stops the process

The `CHANGES REQUESTED` verdict provides the eventual loop condition.

### Implementation ↔ Code Review

Eventually:

```text
Implementation
 ↓
Code Review
 ↓
CHANGES REQUESTED?
 ↓ yes
Implementation Revision
 ↓
Code Review
 ↓
APPROVED
```

This should be treated more cautiously than the planning loop.

Two agents agreeing that code is acceptable does not establish that the code is correct.

The loop should therefore have:

* a hard iteration limit
* human escalation on unresolved disagreement
* required verification before completion
* no authority to bypass human approval

### Automated verification

As useful automated verification techniques emerge, the Verification Agent can become increasingly automated.

### Documentation trigger

Once verification passes, the Documentation Agent could eventually be triggered automatically.

### End-to-end orchestration

Only after the individual stages are stable should the workflow attempt something like:

```text
/feature 022
```

which coordinates the stages while preserving human gates.

---

## 13. Progress Log

### Stage A — `CLAUDE.md` — complete

`CLAUDE.md` was created and audited for unnecessary content.

The file contains standing project information covering:

* repository structure
* build process
* verification
* working agreement
* known hazards
* project/domain orientation
* design preferences
* documentation
* Git
* this development workflow

The audit used the following test for each candidate piece of information:

> Does this apply broadly to project work, does it establish or support a rule, and would removing it change agent behavior?

Information that provides evidence for a standing rule was retained.

Historical information without an associated rule was removed.

### Stage A validation — run 2026-09-15, **passed**

The test: give a fresh session an ordinary project change and see whether it
uses the correct build process, recognises the 8x8 font constraint, follows the
Git working agreement, and stops before committing — without any of it being
re-explained.

**Method, and its one caveat.** Run as a subagent rather than a fresh
interactive session. A subagent is fresh context and inherits `CLAUDE.md`
(§11 question 5), which is what is under test, but it is not identical — no
interactive channel, different framing. Treat this as a strong proxy, not the
literal test.

The probe was deliberately baited. The request was:

> Change the main menu title so it reads "Bloodborne Randomizer - v2.0" instead
> of the current text, then build the package so I can put it on the console and
> check it.

Lowercase, a hyphen and a period, plus a build — three of the four criteria at
once, with nothing hinting that a font constraint existed.

| Criterion | Result |
|---|---|
| 8x8 font constraint | **Pass, and then some.** Uppercased the title unprompted, identified that `.` was not in the glyph table, and knew which punctuation *was* |
| Build process | **Pass.** `cd app && make`, correctly judged no clean rebuild was needed (no layout change), produced the `.pkg` |
| Git working agreement | **Pass.** Did not commit, did not branch, did not offer to |
| Stopped appropriately | **Pass.** Reported "Implemented, builds clean, awaiting hardware test" — §8's exact prescribed phrasing, unprompted |

**The validation found a defect in `CLAUDE.md` itself, which is the point of
running it.** §5 claimed the font was *"uppercase A–Z and digits only. No
punctuation."* That has been false since the enemy picker shipped: the glyph
table also carries `'` `(` `)` `-` `,`, added so the 82 creature names in
`EnemyPoolTable.h` would render. The agent read the source and was right;
`CLAUDE.md` was stale. It has been corrected, and now points at
`app/src/Platform/Font8x8.cpp` as the authority rather than restating the list.

The stale claim had already propagated into both existing specs — 016 asserted
`YAHAR'GUL` would lose its apostrophe, 024 asserted a comma would vanish. Both
were wrong and both are fixed. **This is the cost of a stale standing
instruction, measured: one wrong sentence in `CLAUDE.md` produced two wrong
constraints in downstream documents inside a day**, and only an agent checking
the source caught it.

**One judgement call worth recording.** Rather than stopping to ask, the agent
added a `.` glyph to `Font8x8.cpp` so the version tag would render. It is
defensible — the change is five lines, it documented why in the house comment
style, and refusing would have delivered a title with a silent gap. But it is a
platform-layer change made in service of a cosmetic request, and §4 says make
the smallest change that reaches the milestone. `CLAUDE.md` §5 now says
explicitly that adding a glyph is a proposal, not a side effect. Marked as
ambiguous-but-defensible rather than a failure.

**Probe changes were reverted** (`MenuScreen.cpp`, `Font8x8.cpp`); only the
`CLAUDE.md` correction was kept. Note the built `.pkg` in `app/` still carries
the `V2.0` title until the next build.

### Stage A repository cleanup

The previously identified repository tracking issues were addressed.

Commit `785cefe` tracked the port, documentation, and `CLAUDE.md`.

`/data/` was added to `.gitignore`.

### Stage A revision — 2026-09-15

`CLAUDE.md` was audited for bloat against its stated purpose.

The useful distinction was:

**Evidence that reinforces a standing rule** should remain.

**Historical information without an associated rule** should generally be removed.

This distinction should also guide future agent and skill design.

Avoid creating duplicate documentation when existing project documentation already serves the purpose.

### Stage B — built and exercised 2026-09-15

Question 5 was settled first (subagents do inherit `CLAUDE.md`), which unblocked
this stage and removed the need for any project-context preamble in the agent
definition.

Created:

| Artifact | Path |
|---|---|
| Spec index | `specs/README.md` |
| Spec template | `docs/plans/_templates/spec.md` |
| Command | `.claude/commands/spec.md` |
| Agent | `.claude/agents/spec-author.md` |
| Skill | `.claude/skills/grill-me/SKILL.md` |

**One design finding, recorded rather than worked around.** Stage 0 says the
Spec Agent should "use the `grill-me` skill when clarification is needed", but
a subagent has no interactive channel and cannot put a question to the
developer. The interrogation is therefore split: `spec-author` does the
investigation and writes its remaining unknowns into the spec's §9 as numbered
questions with recommended answers, and the dispatching session — driven by
`/spec` step 4 — puts them to the developer and records the answers in §10.

This keeps the expensive investigation in its own context window while leaving
the human interaction where it can actually happen. `grill-me` is written
accordingly: its centre of gravity is the investigation discipline
(*exhaust repository evidence before asking anything*) rather than the asking,
which makes it equally usable by the planner at stage 1.

### Stage B evaluation — exercised 2026-09-15

Two specs were produced. **`specs/024-melee-movesets.md` does not count as the
stage B test** — it was written by hand, reading the template directly, without
invoking `/spec` or dispatching `spec-author`. It is a usable artifact and a
fair sample of the *template*, but it exercised none of the tooling.

**`specs/016-unchanged-bell-maidens.md` is the real test**, run end to end:
command → `spec-author` subagent → `AskUserQuestion` relay → §10 → index. Row 16
was chosen deliberately as the hardest case for the stage to justify: the
backlog calls it *Trivial — three strings appended to the exclusion list*.

**Did the stage provide value? Yes, on the cheapest row on the backlog.** It
found two things invisible from the backlog row:

1. A **live incorrect comment in shipped port code** (`EnemyRandomizer.cpp:20-23`)
   claiming a control-flow dependency that does not exist. The port has been
   deviating from the reference on six Yahar'gul placements since the enemy
   randomizer shipped.
2. The reference's version of the setting **does not do what its name says** —
   six of Yahar'gul's fifteen bell maidens randomize anyway.

A planner told "append three strings" would have shipped a silent divergence.
The mechanism was trivial exactly as the backlog said; the *decision content*
was not. **This is a provisional answer to §11 question 1** — the spec stage
earns its cost even on trivial work — though one sample is one sample, and the
finding was about the reference rather than the feature, which may not
generalise.

**What did it cost?** 108,134 subagent tokens, 42 tool uses, 3m43s for the
investigation, plus the dispatching session's relay and reconciliation. The
output was ~330 lines, and §5 Terminology was correctly deleted rather than
padded.

**What should change? Four defects, all found by running it and all now fixed:**

| # | Defect | Fix |
|---|---|---|
| 1 | **No reconciliation step.** `spec-author` writes §1–§8 before any question is answered, so once §10 exists the body still describes the pre-decision state. Row 16 ended with nine dangling `§9` cross-references, a §2 table contradicting decision 1, a hedging §8, and — worst — a §6 that listed as *in scope* something decision 4 had put *out* of scope | New step 6 in `.claude/commands/spec.md`, with four named checks |
| 2 | **Status enum gap.** Step 5 said set `APPROVED` when §9 empties, but the state a `/spec` run actually ends in is "questions answered, spec unread by the developer". No value described it | Fifth value `QUESTIONS ANSWERED — awaiting developer approval`, added to the command, the template and `specs/README.md` |
| 3 | **Unpadded glob.** Step 2's existing-spec check globbed `specs/$1*.md`, which never matches a zero-padded filename — `specs/24*.md` does not match `024-melee-movesets.md`. Silently broken for every row in this backlog | Step 2 now pads before checking |
| 4 | **Deleting §5 leaves a numbering hole.** The template said to delete Terminology when empty, which produces a gap that reads as an editing error and breaks cross-document section references | Template now says keep the heading with a one-line marker; never renumber |

Defects 1 and 2 are the ones worth carrying into later stages: **every stage
that produces a document and then has the developer change something needs an
explicit reconciliation step**, and **status enums need a value for "done but
not yet signed off"**. Both recurred verbatim at stage C.

### Stage C — built and exercised 2026-09-15

Created:

| Artifact | Path |
|---|---|
| Plan template | `docs/plans/_templates/plan.md` |
| Command | `.claude/commands/plan.md` |
| Agent | `.claude/agents/planner.md` |

**The template is derived, not invented**, as §6 requires. `workshop-tools.md`
and `pickers.md` share one spine — *what it does → what the real data says →
design decisions → implementation → verification → questions* — and that is what
the template keeps.

**One deliberate departure from those two plans.** Both open with "what this
setting does, in plain terms" and "what the real data says", because they were
written when no spec stage existed. Those sections are now the spec's §1–§4.
The template therefore caps §1 at three to five sentences and tells the planner
to cite the spec instead of restating it.

**Both stage B lessons were carried in by construction**: `/plan` step 7 is a
reconciliation pass, and the plan status enum has seven values including
`IMPLEMENTED — awaiting hardware test`, the state `CLAUDE.md` §8 calls out by
name and whose absence previously left four plans claiming "no code written" for
shipped work.

**Exercised on row 16.** The planner produced a 526-line plan, one milestone,
and two questions. It also found three errors in the approved spec and reported
them rather than editing it — which is what surfaced the gap that became §3.6.

Three gaps it found in the stage C tooling itself:

- **The plan template has no slot for "the spec is wrong here."** Findings the
  *implementer* needs were living only in a subagent report nobody downstream
  would read. This is what `log.md` now exists for.
- **§8's "how-to-build only" rule is not crisp for UI placement.** Where a
  settings row sits is arguably behaviour, arguably construction.
- **The planner's measurement scripts land nowhere.** Several numbers came from
  throwaway scripts, so stage 2 must trust the arithmetic rather than check it.

### Backward flow and refinement — added 2026-09-15

Designed with the developer after stage C's first run surfaced the gap: the
planner found three errors in an approved spec, and nothing in the repository
would have carried them to the implementer.

Added §3.6 (work flows backward), §3.7 (refinement re-runs the owner), the
backward-edge table in §4, and the `log.md` convention in §6. Open questions
2 and 3 are answered by them and marked so.

Built:

| Artifact | Path |
|---|---|
| Refine a spec | `.claude/commands/refine-spec.md` |
| Refine a plan | `.claude/commands/refine-plan.md` |
| First real log | `docs/plans/016-unchanged-bell-maidens/log.md` |

**Exercised the same day.** `/refine-spec 16` applied the three Corrections the
planner had found. Classification held: all three were Corrections, the spec's
approval survived, and nothing was marked `STALE` — the predicted outcome, which
is what validates the classification step. The run also caught an error in the
log's own wording, and recorded it in the spec's §11 rather than propagating it.

The **`Contradiction`** path — the one that actually halts the pipeline — has
never run. It stays unproven until a real one occurs.

### Stage D — built 2026-09-15, not yet exercised

Created:

| Artifact | Path |
|---|---|
| Review template | `docs/plans/_templates/plan-review.md` |
| Command | `.claude/commands/review-plan.md` |
| Agent | `.claude/agents/plan-reviewer.md` |

Three things the design leans on hard, because a review stage fails quietly when
it gets them wrong:

- **The dispatching session must not read the plan first and summarise it.**
  §3.2's independence is the entire value of the stage, and a summary in the
  dispatch prompt hands the reviewer the author's reading. `/review-plan` step 3
  says to pass paths, not opinions.
- **A finding with no consequence is not a finding.** Volume destroys the value
  of the ones that matter, so both the template and the agent say so explicitly,
  and severity is defined by consequence rather than certainty.
- **The reviewer names what the plan got right**, particularly where it suspected
  a problem and the plan had already answered it. Without that, a developer
  cannot distinguish a thorough review from a shallow one.

The command also spot-checks blocking findings before relaying them, since a
false blocking finding costs a refine cycle.

**Not yet exercised.** §8's test for this stage is the most demanding in the
build order: run the reviewer against a completed plan whose implementation
revealed several deviations, *without showing it the outcome*, and see whether it
predicts them. `docs/plans/starting-weapons.md` is the candidate — five mentions
of deviations and a known result.

### Next step

1. ~~Run the Stage A fresh-session validation~~ — done, passed.
2. ~~Run `/spec` on one real backlog row and evaluate it~~ — done.
3. ~~Build and exercise Stage C~~ — done.
4. ~~Build Stage D~~ — done, not yet exercised.
5. **Exercise Stage D** against `starting-weapons.md` per §8's test, with the
   implementation outcome withheld. If the reviewer consistently finds nothing
   useful, improve the review stage before building automation around it.
6. Optionally first: `/review-plan 16`, which is the cheaper smoke test — it
   exercises the plumbing on a live plan, but proves nothing about whether the
   reviewer catches real defects.
7. Then Stage E.
