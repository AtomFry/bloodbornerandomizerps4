---
description: Re-run the planner on an existing plan with your feedback
argument-hint: <row number> <feedback, in quotes>
---

Refine the implementation plan for backlog row **$1** using the developer's
feedback.

`$ARGUMENTS` is the row number followed by the feedback. Everything after the
row number is the feedback — treat it as the developer's words, not a summary
to improve on.

This implements `docs/plans/ai-dev-process-vision.md` §3.7. **You do not edit the plan
yourself.** You re-dispatch the agent that owns it.

1. **Find the plan** at `docs/features/<padded>-<slug>/plan.md`, padding `$1` to
   three digits. If there is none, say so and stop; `/plan $1` creates one.

2. **Read its current status**, and note it. What happens at step 6 depends on
   whether it was `APPROVED` before this refinement.

   If it reads `IMPLEMENTED` or `DONE` it describes shipped work. **Stop and
   ask**, because rewriting it destroys the record of what was actually built —
   a deviation belongs in its §10, not in a rewrite of §3.

   If it reads `STALE`, say which upstream decision invalidated it and confirm
   the developer wants a refinement rather than a re-plan. A plan built against a
   changed spec usually needs `/plan`, not `/refine-plan`.

3. **Check the spec is still `APPROVED`** and read its §10. A plan cannot be
   refined into contradicting the decisions it implements. If the feedback asks
   for something the spec forbids, that is a **spec** change — stop, say so, and
   point the developer at `/refine-spec $1`. Do not resolve it here.

4. **Read `docs/features/<padded>-<slug>/log.md`** if it exists, and surface any
   outstanding findings aimed at this plan that the feedback does not cover.

5. **Dispatch the `planner` subagent** with: the plan path, the evidence path,
   the spec path, the developer's feedback verbatim, any outstanding `log.md`
   findings, and an instruction to report **which of its changes altered an
   approved decision** as against which were corrections or clarifications.

   **Tell it to revise in place.** This is the instruction that matters most in
   this command, and the one whose absence makes plans grow until an
   implementer cannot use them. A refinement *changes what the plan says*. It
   does not:

   - annotate a section with what it used to say, or mark one "resolved",
     "corrected" or "superseded"
   - add a paragraph explaining which review finding prompted which change
   - add a per-finding response list to §10 or anywhere else
   - explain in the status block what this pass answered

   A plan that argues with its own history forces the implementer to classify
   every paragraph before using it, and that cost compounds with each review
   cycle — two or three rounds of an otherwise healthy refinement loop are
   enough to bury the contract. The superseded reasoning goes to
   `plan-evidence.md` where it is still revisable; the account of this pass goes
   to `log.md` at step 9, which already records it in full. Neither belongs in
   `plan.md` §1–§7.

   Where the review's finding was itself about structure — a §6
   `NEEDS RESTRUCTURING` assessment — the response is the move it named: text
   relocated to `plan-evidence.md`, cut in favour of `log.md`, or a milestone
   split. Nothing is added.

6. **Put any new §8 questions to the developer**, record them in §9, and
   reconcile the body exactly as `/plan` steps 5-7 do, including step 7b's
   budget and genre check. The reconciliation pass matters more here than on a
   first run: a refinement changes one part of a document whose other parts were
   written against the old shape, and it is the pass where accumulated history
   is easiest to spot and cheapest to move.

7. **Classify the refinement and set the status** (§3.6, §3.7):

   - **No approved decision changed** — the plan keeps `APPROVED`. Record the
     amendment in `log.md` at step 9, **not** in the plan's §10: that section is
     for deviations discovered during implementation, and a stage C refinement
     is not one. `git` and `log.md` between them already show what changed.
   - **An approved decision changed** — status drops to
     `QUESTIONS ANSWERED — awaiting developer approval`, and the developer must
     re-approve before implementation starts.

   If the plan was not `APPROVED` to begin with, set the status as `/plan` step 8
   would.

8. **Propagate staleness** when an approved decision changed: mark
   `plan-review.md`, `implementation-report.md`, `code-review.md` and
   `test-report.md` `STALE` if they exist, each with the decision that
   invalidated it. Never delete them. Report what you marked.

   Marking an artifact `STALE` is a one-line status change on that artifact. It
   is not a reason to add anything to `plan.md`.

9. **Append to `log.md`**: the date, that stage C refined the plan, the feedback,
   the classification, and what was marked `STALE`. Append only.

Then stop. Do not implement, do not create a branch, do not write code.

Report: what changed, the classification and why, whether approval survived,
the contract's line count before and after, what was marked `STALE`, and
anything the planner found wrong in the spec — which goes to `log.md`, never
into the spec itself.

If the contract grew during a refinement whose purpose was to answer findings,
say so explicitly. A contract that grows every cycle is the signal that this
command's step 5 is not being followed.
