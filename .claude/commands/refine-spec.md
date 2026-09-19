---
description: Re-run the spec author on an existing spec with your feedback
argument-hint: <row number> <feedback, in quotes>
---

Refine the specification for backlog row **$1** using the developer's feedback.

`$ARGUMENTS` is the row number followed by the feedback. Everything after the
row number is the feedback — treat it as the developer's words, not a summary
to improve on.

This implements `docs/plans/ai-dev-process-vision.md` §3.7. **You do not edit the spec
yourself.** You re-dispatch the agent that owns it, so its conventions and
standards are reapplied rather than bypassed.

1. **Find the spec** at `docs/features/<padded>-<slug>/spec.md`. Pad `$1` to three
   digits — `docs/features/24*/` does not match
   `docs/features/024-melee-movesets/`. If there is none, say so and stop;
   `/spec $1` is the stage that creates one.

2. **Read the spec's current status**, and note it. What happens at step 6
   depends on whether it was `APPROVED` before this refinement.

3. **Read `docs/features/<padded>-<slug>/log.md`** if the folder exists. Findings
   raised by later stages live there, and a refinement is the moment to act on
   the ones aimed at this spec. Bring any that the feedback does not already
   cover to the developer's attention before dispatching.

4. **Dispatch the `spec-author` subagent** with: the spec path, the developer's
   feedback verbatim, any outstanding `log.md` findings about this spec, and an
   instruction to report **which of its changes altered an approved decision**
   as against which were corrections or clarifications.

5. **Put any new §9 questions to the developer** exactly as `/spec` step 4 does,
   then record them in §10 and reconcile the body as `/spec` steps 5-6 do. A
   refinement can raise new questions; it usually should not raise many.

6. **Classify the refinement and set the status accordingly** (§3.6, §3.7). This
   is the step that keeps approval meaningful:

   - **No approved decision changed** — the spec keeps `APPROVED`. Add the change
     to the spec's *Amendments after approval* section with the date, what
     changed, and that it was a correction.
   - **An approved decision changed** — status drops to
     `QUESTIONS ANSWERED — awaiting developer approval`. The developer must
     re-approve. Say so plainly rather than assuming the earlier approval carries
     over; it does not.

   If the spec was not `APPROVED` to begin with, there is no contract to break:
   set the status the way `/spec` step 7 would.

7. **Propagate staleness** when an approved decision changed. Any
   `docs/features/<padded>-<slug>/plan.md` and anything derived from it were built
   against the old decision:

   - Mark each downstream artifact's status `STALE`, with a line saying which
     decision invalidated it. **Do not delete or rewrite them** — §3.6 rule 5.
   - Report exactly what you marked, so the developer can override.

8. **Append to `log.md`** — create it from the feature folder if absent. One
   entry: the date, that stage B refined the spec, the feedback that prompted it,
   the classification, and what was marked `STALE`. Append only; never edit an
   existing entry.

9. **Update `docs/features/README.md`** so the row matches the new status.

Then stop. Do not plan, do not implement, do not write code.

Report: what changed in the spec, the classification and why, whether approval
survived, what was marked `STALE`, and any new questions.
