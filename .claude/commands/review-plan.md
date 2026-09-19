---
description: Stage D — adversarially review an implementation plan before you read it
argument-hint: <backlog row number, e.g. 16>
---

Review the implementation plan for backlog row **$1**.

This is stage D of the pipeline in `docs/ai-dev-process.md`. Run it like
this:

1. **Find the plan** at `docs/features/<padded>-<slug>/plan.md`, padding `$1` to
   three digits. If there is none, say so and stop — `/plan $1` comes first.

2. **Check what state it is in.**

   - `DRAFT` or `QUESTIONS OPEN` → **stop.** Open questions are forks; reviewing
     a plan that has not decided its own approach wastes the pass. Say what is
     outstanding.
   - `STALE` → say which upstream decision invalidated it and ask whether to
     review anyway. Usually the answer is `/plan $1` instead.
   - `IMPLEMENTED` or `DONE` → this describes shipped work. Ask what the developer
     actually wants; a retrospective review is a legitimate request but a
     different one.
   - `QUESTIONS ANSWERED` or `APPROVED` → proceed.

3. **Do not read the plan yourself first.** §3.2 exists so the reviewer forms an
   independent judgement, and a summary from you in the dispatch prompt would
   hand it your reading of the plan. Pass paths, not opinions.

4. **Dispatch the `plan-reviewer` subagent** with: the plan path, the evidence
   path (`plan-evidence.md`), the spec path, the feature's `log.md` path if it
   exists, and nothing else about their contents. Do not tell it what you think
   of the plan, which findings you expect, or what the planner told you. Its
   independence is the entire value of the stage.

   Say which path is the contract and which is the evidence, and nothing more —
   it reads the contract first, alone, on purpose.

5. **Read the review, and spot-check its blocking findings** before relaying
   them. A reviewer can be wrong, and a false blocking finding costs the
   developer a refine cycle. Verify against the repository the same way you would
   verify a planner's claim. Say which ones you confirmed.

   **Check §6's findings are moves and cuts, not additions.** A restructuring
   finding that asks the plan to explain something, justify something, or record
   this review is malformed — acting on it would grow the contract, which is the
   opposite of what §6 is for. Say so when you see one rather than passing it
   through to `/refine-plan`.

6. **Report to the developer**, and do not act on the findings yourself. The
   verdict is a recommendation (§3.1); what happens next is theirs:

   - `APPROVED` → they read spec, plan and review, then approve or not.
   - `CHANGES REQUESTED` → they decide whether to `/refine-plan $1` with the
     findings as feedback, accept the plan as-is, or change the spec.
   - `BLOCKED` → say exactly what would unblock it.

   **Relay the §6 handoff assessment separately from the verdict.** They are
   independent, and the combination changes what the developer should do:

   - `APPROVED` + `READY` → the plan can go to `/implement $1`.
   - `APPROVED` + `NEEDS RESTRUCTURING` → the design is right and the contract
     needs rearranging first. That is a `/refine-plan $1` carrying only the §6
     moves — a cheaper fix than a re-plan, and worth saying so.
   - `CHANGES REQUESTED` + either → the findings come first.

7. **Append to `log.md`** — create it if absent. One entry: the date, that stage
   D reviewed the plan, the verdict, the handoff assessment, the blocking
   findings, which you verified, and any finding aimed at the **spec** rather
   than the plan. Append only.

   This entry is the record of the review. Nothing about this review is written
   into `plan.md` — not now, and not by the refinement that answers it.

8. **Do not change the plan's status.** A review is not an approval and does not
   move an artifact's state. Only the developer does that.

Then stop. Do not refine the plan, do not implement, do not write code.

Report: the verdict, the handoff assessment, every blocking finding in one line
each, every §6 move or cut in one line each, which you independently confirmed,
what the reviewer could not check, and what you recommend the developer do
next.
