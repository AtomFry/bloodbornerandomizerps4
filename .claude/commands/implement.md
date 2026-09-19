---
description: Stage E — build one milestone of an approved plan, verify it, and stop
argument-hint: <row number> [milestone number]
---

Implement **one milestone** of the plan for backlog row **$1**.

`$ARGUMENTS` is the row number, optionally followed by a milestone number. With
no milestone given, take the first one not yet implemented.

This is stage E of the pipeline in `docs/ai-dev-process.md`. Run it like this:

1. **Find the plan** at `docs/features/<padded>-<slug>/plan.md`, padding `$1` to
   three digits. If there is none, say so and stop — `/plan $1` comes first.

2. **Check the plan's status line.** Implementation needs a plan the developer
   has approved, because §4 of `CLAUDE.md` makes approval their gate, not yours.

   - `APPROVED` → proceed.
   - `QUESTIONS ANSWERED — awaiting developer approval` → **stop and ask.** This
     is the normal end state of `/plan`, and it is one step short of
     implementable. Offer to walk them through the plan; do not set the approval
     yourself.
   - `DRAFT`, `QUESTIONS OPEN` → stop and say what is outstanding.
   - `STALE` → say which upstream decision invalidated it; usually the answer is
     `/plan $1`.
   - `IMPLEMENTED` or `DONE` → say what is already built and ask what is wanted.

3. **Check §8 is empty.** An open implementation question is a fork, and an
   agent that guesses which way it goes builds the milestone twice. If §8 has
   anything live in it, put it to the developer with `AskUserQuestion` and
   record the answer in §9 before dispatching.

4. **Work out which milestone, and that it is the next one.** Read §7 and the
   feature's `log.md`. Milestones on this project end at a hardware test that
   only the developer can run, so a milestone whose predecessor is built but not
   yet tested is **not** ready — starting it chains two milestones, which
   `CLAUDE.md` §4 exists to prevent. Say so and ask rather than proceeding.

   Check the milestone's own preconditions too. A plan may require something to
   exist before the first edit — a captured baseline tree, a recorded seed. If
   one is outstanding, stop: several of them cannot be satisfied once the code
   has changed.

5. **Check the working tree.** Report what is already modified before anything
   is built on top of it, so the developer can tell this milestone's changes
   from what was already there. Do not stash, reset, or clean anything.

6. **Dispatch the `implementer` subagent** with: the plan path, the evidence
   path, the spec path, the `log.md` path, and the milestone number. Tell it
   which path is the contract.

   Do not summarise the plan for it and do not tell it how you would build the
   feature. The contract is written to be sufficient on its own, and a summary
   from you both adds a second source of truth and hides whether the contract
   actually was sufficient — which is worth knowing.

7. **Read the report and spot-check it.** An implementation report is a claim,
   the same as a plan's numbers are. Before relaying it:

   - Confirm the build actually produced the `.pkg`, and that the verification
     commands it lists were run with the results it reports.
   - Read the diff against the plan's §5 file table. A file changed that the
     plan did not name is a deviation whether or not the report calls it one.
   - Check every §3.1 invariant the milestone could have broken.

   Say which of these you confirmed.

8. **Check the plan was not edited.** The implementer may write §10 and nothing
   else. If §1–§7 changed, that destroys stage F's ability to compare the
   implementation against what was approved — revert that part and report it.

9. **Append to `log.md`.** One entry: the date, that stage E implemented
   milestone N, whether it completed or hit a stop condition, the deviations,
   the decisions the plan left open, the verification run, and what is now
   awaiting hardware test. Append only.

10. **Hand off the hardware test.** Give the developer the §6 procedure as steps
    they can follow: what to enable, what to do, what to look for, what a
    failure looks like. This is the point of the stop.

Then stop. Do not start the next milestone, do not create a branch, and do not
commit — `CLAUDE.md` §9 leaves the working tree to the developer. A clean build
and green mirrors mean **ready for hardware testing**, never done (`CLAUDE.md`
§3).

If the implementer hit a stop condition, that is a successful run of this
command. Report where it stopped, what state the tree is in, and what decision
is needed — and do not work around it yourself.

Report: which milestone was built, whether it completed, the files changed, every
deviation and open decision taken, the verification results and what was not
run, what you independently confirmed, and the hardware test the developer must
now run.
