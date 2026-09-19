---
description: Stage B — turn a backlog row into a specification worth planning against
argument-hint: <backlog row number, e.g. 24>
---

Write the specification for backlog row **$1**.

This is stage B of the pipeline in `docs/ai-dev-process.md`.

Run it like this:

1. **Locate the row.** Find row `$1` in `docs/randomization-feature-spec.md`.

   If there is no such row, say so and stop — do not invent a feature.

   If the row is already marked **DONE** or **OUT**, say so and ask whether to continue before doing any work. Do not dispatch the subagent until the developer chooses to continue.

2. **Check for an existing spec.** Feature folders zero-pad `NNN` to three digits.

   Check the padded form, not the bare argument. Pad `$1` to three digits and check `docs/features/<padded>-*/spec.md`, or list `docs/features/` and match the row number however it is written.

   If an existing spec is found, read it enough to determine whether this is an existing draft that should be revised. Ask the developer whether to revise that spec rather than starting a new one.

   **Do not dispatch `spec-author` until this decision has been made.**

3. **Dispatch the `spec-author` subagent.** Give it the row number and the row's text.

   The subagent investigates and drafts the specification in its own context so the investigation does not crowd this session.

   The subagent must produce only the spec artifact. It must not update `docs/features/README.md`.

4. **Review the subagent's result.**

   Read the generated spec and its reported §9 questions.

   Confirm that the spec contains the required sections from `docs/features/_templates/spec.md`, that its status is consistent with §9, and that the questions are genuine unresolved developer decisions rather than facts that could have been established through further investigation.

   If the subagent reports something it could not establish, determine whether additional repository investigation can settle it before presenting it to the developer. Do not turn an investigable fact into a developer question.

5. **Put §9 questions to the developer.**

   The subagent cannot ask them directly.

   Use `AskUserQuestion`, carrying each question's evidence-based recommended answer through as the first option so the recommendation is visible.

   Ask blocking questions first.

   If there are more than four questions, ask the consequential ones and leave the remaining questions in §9.

6. **Record the answers in §10 Decisions taken.**

   Record each developer decision with the date.

   Move each answered question out of §9.

   If unanswered questions remain, leave them in §9 and keep the status `QUESTIONS OPEN`.

   Delete §9 entirely once it is empty.

7. **Reconcile the document with the decisions.**

   The subagent drafted §1–§9 before receiving the developer's answers. Once §10 exists, parts of the body may describe a state that is no longer true.

   Check all of the following:

   * **Dangling cross-references.** Run `grep -n "§9" <spec>`. Every remaining hit must point to a section that still exists. Repoint references at §10 or remove them when they are no longer needed.
   * **Stale body text.** Re-read §2 (what the player experiences) and §8 (how we will know it works) against §10. Replace conditional language such as "whichever way §9.N goes" with the decision that was actually made. Also check §1, §4, §6, and §7.
   * **Numbering holes.** The template may instruct the author to delete §5 Terminology when there is nothing to define. Do not renumber later sections. Keep the heading and give it one line saying it is deliberately empty so the gap cannot be mistaken for an editing accident.
   * **Contradictions created by decisions.** A decision may invalidate an assumption in §4 or a boundary in §6. Fix the affected statement where it sits.
   * **Decision authority.** Do not introduce implementation decisions while reconciling the document. Only incorporate what the developer actually decided and make the existing analysis consistent with it.

   Do not redo the subagent's investigation during reconciliation unless a contradiction requires verification.

8. **Set the status line.**

   Use exactly one of these statuses:

   * `QUESTIONS OPEN` — §9 still contains unanswered questions.
   * `QUESTIONS ANSWERED — awaiting developer approval` — §9 is empty, but the developer has not read and approved the resulting spec.
   * `APPROVED` — §9 is empty and the developer has explicitly read and approved the resulting spec.

   The normal successful end state of a `/spec` run is `QUESTIONS ANSWERED — awaiting developer approval`.

   Approval belongs to the developer. Never infer approval from the absence of objections or from answers to the questions alone.

9. **Update `docs/features/README.md`.**

   Update the row for this spec so it matches the final status line.

   This is the only stage that owns the final index update.

10. **Stop.**

Do not plan the feature.

Do not write production code.

Do not create a branch or commit.

`/plan $1` is the next stage, and it is the developer's decision whether to run it.

Report:

* the spec path
* the backlog row(s) covered
* the questions asked
* what the developer decided
* what step 7 reconciled
* anything the subagent could not establish
* the final spec status
