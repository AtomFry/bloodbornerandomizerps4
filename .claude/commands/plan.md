---
description: Stage C — turn an approved spec into an implementation contract worth reviewing
argument-hint: <backlog row number, e.g. 16>
---

Write the implementation plan for backlog row **$1**.

This is stage C of the pipeline in `docs/ai-dev-process.md`. Run it like
this:

1. **Find the spec** at `docs/features/<padded>-<slug>/spec.md`. Feature folders
   zero-pad `NNN` to three digits, so pad `$1` before looking —
   `docs/features/24*/` does not match `docs/features/024-melee-movesets/`. If
   there is no spec for this row, say so and stop: `/spec $1` is the stage that
   comes first, and planning without one means inventing the requirements.

2. **Check the spec's status line.** It must read `APPROVED`.

   - `QUESTIONS OPEN` or `QUESTIONS ANSWERED — awaiting developer approval` →
     **stop and tell the developer what is outstanding.** Offer to walk them
     through it, but do not plan against unapproved decisions and do not set the
     approval yourself.
   - `SUPERSEDED` → say what replaced it and stop.

3. **Check for an existing plan** at `docs/features/<padded>-<slug>/plan.md`. If one
   exists, read it and ask whether to revise it rather than starting a new one.
   Check its status first — a plan marked `IMPLEMENTED` or `DONE` describes
   shipped work, and rewriting it would destroy the record.

4. **Dispatch the `planner` subagent** with the row number and the spec path. It
   investigates and drafts; it runs in its own context so the investigation does
   not crowd this session.

   It returns **two** files: `plan.md`, the implementation contract that stage E
   executes, and `plan-evidence.md`, the investigation that supports it. Both
   belong to stage C and both go to the reviewer.

5. **Put its §8 questions to the developer.** The subagent cannot ask them
   itself. Use `AskUserQuestion`, carrying each question's recommended answer
   through as the first option so the recommendation is visible. Ask blocking
   questions first. If there are more than four, ask the consequential ones and
   leave the rest in §8.

   If the planner reports a **spec gap** — a question about what the feature
   should do rather than how to build it — do not answer it here. Tell the
   developer it belongs in the spec, and let them decide whether to amend the
   spec before the plan goes further.

6. **Record the answers in §9 Decisions taken**, with the date. Move each
   answered question out of §8.

   **Keep the §8 heading with one line saying it is empty.** Do not delete the
   section — step 7 forbids renumbering, and every other artifact cites §9 and
   §10 by number. One line is the whole of it: an empty section that explains
   which questions it used to hold, when they were answered and by whom is
   history, and `log.md` has all of it already.

7. **Reconcile the rest of the document with the decisions.** The subagent wrote
   §1–§7 before it had any answers, so once §9 exists, parts of the body describe
   a state that is no longer true. This is the step that is easiest to skip and
   that leaves the most misleading artifacts behind — a plan that argues with
   itself sends the implementer in two directions. Check all four:

   - **Dangling cross-references.** Run `grep -n "§8" <plan>`. Every hit now
     points at a section that no longer exists. Repoint each at §9, or drop it.
   - **Stale body text.** Re-read §4 (approach), §5 (files and changes) and §7
     (milestones) against §9 — those are where a decision does the most damage.
     Any surviving "whichever way §8.N goes" has to become what was decided.
     Where a decision invalidated something in `plan-evidence.md` too — a
     measurement taken for the rejected branch, an alternative no longer live —
     correct it there as well.
   - **Numbering holes.** Never renumber sections; other documents and this
     command cite them by number. Where a section has nothing to say, keep the
     heading and give it one line saying so.
   - **Contradictions the decisions created.** A decision can invalidate a
     hazard in §3.3, a verification step in §6, or a milestone boundary in §7.
     Fix it where it sits.

   **Edit in place.** Reconciliation changes what a section says; it never adds
   a note explaining that the section changed, and it never appends a record of
   this pass to §10. §10 is for deviations found during *implementation*. The
   history of this run goes to `log.md` at step 10, and nowhere else.

   Do not rewrite the planner's analysis while doing this. This step only makes
   the documents consistent with what was decided.

7b. **Check the contract is still a contract.** Two quick passes over
   `plan.md`, which the reconciliation may have degraded:

   - **Budget.** `§1`–`§7` should come to roughly 400 lines. Well over, and
     something has leaked in — look for a measurement, a rejected alternative,
     or an account of how a decision was reached, and move it to
     `plan-evidence.md` or drop it in favour of what `log.md` already records.
   - **Genre.** Every paragraph in §1–§7 is an instruction, a constraint, or a
     verification. Anything that is none of the three does not change what an
     implementer does, and belongs in the other two files.

   This is the step that keeps the contract usable across refinement cycles. It
   is cheap now and expensive later.

8. **Set the status line.** Three cases, not two:

   - §8 still has open questions → `QUESTIONS OPEN`.
   - §8 is empty, but the developer answered from your summary rather than from
     the document → `QUESTIONS ANSWERED — awaiting developer approval`. **This
     is the normal end state of a `/plan` run.**
   - §8 is empty and the developer has read the plan and approved it →
     `APPROVED`. Theirs to give, never yours or the subagent's.

9. **Update `docs/features/README.md`** so this feature's Plan column points at
   the new plan and its evidence file.

10. **Append to `log.md`** — create it if absent. One entry: the date, that
    stage C wrote the plan, the questions asked and what was decided, and what
    step 7 reconciled. Append only. This is the sole record of how the plan got
    here; the plan itself must not carry it.

Then stop. Do not implement the feature, do not create a branch, and do not
write code — stage D is the plan review, and it is the developer's call whether
to run it.

Report: both file paths and the contract's line count, the milestone breakdown,
what the questions were, what was decided, what step 7 reconciled, anything the
planner found wrong in the spec, and any risk serious enough to weigh before
approval.
