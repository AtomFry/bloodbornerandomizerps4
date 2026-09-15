# AI-Assisted Development Process — vision and build plan

**Status: stage A complete (`CLAUDE.md` written). Stages B–I not started.**
This document is the plan we work through; §8 tracks what exists.

The goal is a repeatable, named pipeline for getting a feature from "a row in
the spec" to "confirmed working on hardware", with defined handoff artifacts
between stages and explicit points where a human decides. Today that pipeline
exists, but only in your head and in the shape of the `docs/plans/*.md` files
that happen to have accumulated. This makes it explicit, gives each stage a
trigger phrase, and writes down the standing rules so they stop having to be
re-explained every session.

---

## 1. Read this part first: the honest risk

A five-agent adversarial pipeline is a real cost. Every stage is a document
someone has to read, and on a solo hobby project the person reading all of them
is you. The failure mode is not that the process doesn't work — it is that it
works, produces four documents for a twenty-line change, and you quietly stop
using it by week three.

Two things in this design are there specifically to prevent that:

- **A fast lane** (§7). Not every change earns the full pipeline. The process
  says out loud when to skip most of it.
- **Staged build** (§8). We build the two stages that carry the most value —
  Plan and Plan Review — and use them on real work before building anything
  else. If the overhead is wrong, we find out after building 20% of this, not
  100%.

The parts of your outline with the clearest payoff are **plan**, **adversarial
plan review**, and **the spec/plan/implementation-report paper trail**. Those
catch expensive mistakes before code exists. The parts with the least clear
payoff on *this* repo are the automated testing agent and the documentation
agent, for reasons in §2. They are still in the plan, but last.

---

## 2. What changed from your outline, and why

Your outline was written against your day job — Azure DevOps work items, D365,
SQL, `origin/sprint`, draft PRs. None of that exists here. These are the
substantive corrections, not just find-and-replace.

| Your outline | Here | Why |
|---|---|---|
| `PLAN AB#1234`, read item from ADO | `/plan 022`, read `specs/022-<slug>.md` | No ADO. Specs become numbered files in the repo, per your call. |
| `PLAN\[item number]\` folder | `docs/plans/NNN-slug/` folder | Reuses the directory the repo already uses, so existing links in the spec table don't break. |
| Branch from `origin/sprint` | Branch from `master`, or no branch at all | There is no `sprint`. And you have a standing rule that I don't create commits without asking — so the Implementation Agent leaves a working tree, and *you* decide whether it becomes a branch or a commit. |
| Draft PR is the handoff artifact | `implementation-report.md` + the working diff | There is no PR host in this workflow. The report is what the PR description would have been, and it lives next to the plan it came from. |
| Code review in PR comments vs a file | Settled: a file, `code-review.md` | Follows directly from the line above. Your "on the fence" question resolves itself once there's no PR. |
| Testing Agent — Playwright? black-box? | Python verifier mirrors under `app/tools/`, run against `data/vanilla/dvdroot_ps4` | This is what testing already *is* here. The host clang ships no standard library, so C++ unit tests are not available; the project's standing substitute is a Python script that mirrors the rule and checks it against real game data. `pool_verify.py` and its siblings are the existing precedent. |
| Documentation Agent builds demos, screenshots, video | Release-note paragraph, `docs/user-guide.md` update, spec status row update | There is no emulator. Nothing in this loop can see the game running — only your PS4 can, and only you are holding it. Screen capture has to come from you. |
| SQL JIT skill | dropped | Not applicable. |
| — | **New: a Spec stage** (§3, stage 0) | Genuine gap. Your backlog rows are one-liners like "row 22: scaling". Something has to turn that into a spec worth planning against, and it isn't the Plan Agent's job. |
| — | **New: the hardware test is a pipeline stage** | It is the only way anything reaches DONE in this project, it can only be done by you, and it is the single most common reason work sits half-finished. It deserves to be a named stage with a named artifact, not an afterthought. |

One more correction worth stating plainly: **`grill-me` does not exist.** It is
not installed at the user or project level here. We author it (§6).

---

## 3. The pipeline

Ten stages. Five are agents, four are you, one is the console.

| # | Stage | Trigger | Actor | Reads | Produces |
|---|---|---|---|---|---|
| 0 | **Spec** | `/spec 022` | agent | backlog row in `docs/randomization-feature-spec.md`, relevant code | `specs/022-<slug>.md` |
| 0h | Spec approval | — | **you** | the spec | go / reshape |
| 1 | **Plan** | `/plan 022` | agent | the spec, the code, prior plans | `docs/plans/022-<slug>/plan.md` |
| 2 | **Plan review** | `/review-plan 022` | agent (fresh context) | the spec + the plan | `plan-review.md`, verdict: APPROVED or CHANGES REQUESTED |
| 2h | Plan approval | — | **you** | spec, plan, review | go / revise |
| 3 | **Implement** | `/implement 022` | agent | the plan only | code changes + `implementation-report.md` |
| 4 | **Code review** | `/review-code 022` | agent (fresh context) | plan + diff | `code-review.md`, verdict |
| 4h | Code approval | — | **you** | plan, diff, review | fix / build |
| 5 | **Verify** | `/verify 022` | agent | the plan's verification section | a `tools/*_verify.py` change + selftest results in `test-report.md` |
| 5h | **Hardware test** | — | **you + the PS4** | the built `.pkg` | pass/fail recorded in `test-report.md` |
| 6 | **Document** | `/document 022` | agent | all of the above | user-guide edit, spec status row, release note |

### The two rules that make the adversarial stages work

**Reviewers run in fresh context.** A reviewer that helped write the plan will
approve the plan. Stages 2 and 4 run as subagents with their own context window,
given the artifact and the spec and nothing else. That is the entire reason
they're subagents rather than just "now review it".

**Each stage reads its input artifact, not the conversation.** The Implementation
Agent reads `plan.md` — not the discussion that produced it. If the plan is
ambiguous, that's a plan defect, and it should surface as one rather than being
papered over by context the next stage happens to still have. This is also what
makes it possible to resume a half-finished item weeks later.

### Verdicts

Reviewer output ends with exactly one of:

- `APPROVED` — proceed to the next stage.
- `CHANGES REQUESTED` — numbered, specific, each tied to a plan section or a
  file and line.
- `BLOCKED` — the input artifact is not reviewable (missing, contradicts the
  spec, depends on an unanswered question).

A reviewer never edits the thing it reviews. That's the author's job on the next
pass, and keeping it that way is what preserves the adversarial framing.

---

## 4. Where the artifacts live

```
specs/
  README.md                       index: number, name, status, link
  022-per-zone-scaling.md         one spec per numbered feature
  024-melee-movesets.md

docs/plans/
  022-per-zone-scaling/           one folder per item, created by /plan
    plan.md
    plan-review.md
    implementation-report.md
    code-review.md
    test-report.md

  boss-randomization.md           pre-process plans, flat (see below)
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

Two deliberate choices:

**The pre-process plans stay flat.** On 2026-09-14 every plan was consolidated
into `docs/plans/` and the `-plan` / `-spec` / `-findings` suffixes were dropped,
since the directory already says what they are. They keep their descriptive names
rather than being retrofitted with feature numbers, because several span more
than one spec row — `pickers.md` covers 9 and 10, `param-features.md` covers 4
and 5, `boss-and-treasure-findings.md` covers 2 and 3 — and numbering them would
be false precision. New items created by `/plan` use the numbered folder layout
above. A directory and files coexist fine in the same parent.

The flat ones remain the style reference the plan template is derived from.

**`docs/randomization-feature-spec.md` becomes the backlog index, not the spec.**
It already is one — it is a 20-row status table with a suggested order. What it
is *not* is a spec you can plan against; a single table row is not enough for
stage 1 to do good work. So it keeps its job as the master inventory and status
board, and `specs/NNN-*.md` holds the per-feature detail. Stage 6 writes the
status back to the table, which keeps the two in sync automatically rather than
by discipline.

---

## 5. How this maps onto Claude Code's actual mechanics

Worth getting straight before we build, because the four concepts are easy to
conflate and they do different jobs:

| Mechanism | File | Loaded | Use it for |
|---|---|---|---|
| **Project memory** | `CLAUDE.md` | always, every session | Standing facts and rules that apply to *all* work: build commands, the heredoc hazard, "don't commit without asking", the house doc style. |
| **Slash command** | `.claude/commands/plan.md` | when you type `/plan` | The trigger. Thin — it names the item, points at the agent, passes the number. This is where `PLAN AB#1234` lands. |
| **Subagent** | `.claude/agents/planner.md` | when dispatched | The actor, with its own context window and its own tool permissions. This is what makes reviewers independent, and what lets you give the reviewer read-only tools so it *cannot* edit the plan. |
| **Skill** | `.claude/skills/<name>/SKILL.md` | when relevant to the task | Shared procedure used by more than one agent: how to interrogate an ask, how to write a verifier, the project's doc style. Keeps the agent files short. |

The shape that falls out: **commands are triggers, agents are actors, skills are
shared knowledge, CLAUDE.md is the standing law.** Your instinct in the original
message — CLAUDE.md first, then agents, then skills, then a playbook — is right,
with one adjustment: skills and agents get built together per stage rather than
as two separate phases, because an agent without its skill is half a stage.

**Name collision to avoid:** `/code-review` is already a built-in skill in this
environment (diff review for bugs and cleanups). Our stage-4 command is
`/review-code` so the two don't shadow each other — and the built-in stays
available as a second, independent opinion on the same diff.

---

## 6. Inventory of what gets built

**Root**
- `CLAUDE.md` — build commands, the verifier convention, the heredoc/backslash
  hazard, no-commits-without-asking, the hardware-test gate, house doc style,
  and a short map of this pipeline.

**Commands** (`.claude/commands/`) — `spec`, `plan`, `review-plan`, `implement`,
`review-code`, `verify`, `document`. Each takes a feature number.

**Agents** (`.claude/agents/`)
- `spec-author` — turns a backlog row into a spec.
- `planner` — the plan. Explicitly forbidden from writing code or creating branches.
- `plan-reviewer` — read-only tools. Cannot edit the plan even by accident.
- `implementer` — reads `plan.md`, writes code, writes the report. Does not commit.
- `code-reviewer` — read-only tools.
- `verifier` — writes and runs the Python mirror.
- `documenter` — user guide, spec status, release note.

**Skills** (`.claude/skills/`)
- `grill-me` — structured interrogation of an ask before planning. Authored from
  scratch; used by `spec-author` and `planner`. The one that most directly
  prevents wasted work.

**Cut on 2026-09-15: `bb-project-facts`, `bb-verifier`, `bb-doc-style`.** All
three were originally specified as distillations of material that `CLAUDE.md`
already carries — and, worse, each would have been a prose *description* of
something that already has a working exemplar in the repo:

| Proposed skill | Already in | Exemplar it would have described |
|---|---|---|
| `bb-project-facts` | `CLAUDE.md` §6 + §8 | `docs/ps4-homebrew-findings.md` |
| `bb-verifier` | `CLAUDE.md` §3 | the existing `app/tools/*_verify.py` |
| `bb-doc-style` | `CLAUDE.md` §8 | `workshop-tools.md`, `pickers.md` |

A copy drifts from its original, and the copy is the one that misleads — which is
the failure `CLAUDE.md` §8 already warns about for status lines. Agents that need
this material get a pointer (*"read `CLAUDE.md` §6 and §8 first"*), not a
duplicate. `grill-me` survives the same test: it is the only skill in the
original inventory with no counterpart anywhere in the repo.

**This rests on an unverified assumption** — that project `CLAUDE.md` is loaded
into subagent context. See §9 question 5; it gates stages B and D.

**Templates** (`docs/plans/_templates/`) — `plan.md`, `plan-review.md`,
`implementation-report.md`, `code-review.md`, `test-report.md`, `spec.md`.
Derived from `workshop-tools.md` and `pickers.md`, which already have
the right sections.

### The plan template's required sections

From your outline, with two additions:

1. Functional summary of the ask, in plain terms
2. Technical implementation summary
3. **Confidence**, stated as a level with the reason for it
4. Explicit list of code changes — for each: what, why it serves the goal,
   alternatives considered where relevant
5. **How this gets verified** — which verifier, which selftest, and what the
   hardware test is looking for *(new)*
6. **What this cannot break, and what it might** — the blast radius *(new)*
7. Risks, concerns, open questions — may be empty

Section 5 is new because on this project a plan that doesn't say how it will be
proven is not finished; the hardware test is expensive and the person running it
needs to know what to look at. Section 6 is new because the existing plans keep
reaching for it anyway — `workshop-tools.md` has an "it cannot make a run
unwinnable" paragraph that is exactly this, and it's the most useful paragraph
in the document.

---

## 7. Two lanes

**Full lane** — all ten stages. For anything that touches the randomizer engine,
changes a saved config format, adds a UI screen, or alters eligibility rules.
Anything where being wrong costs a hardware test cycle or corrupts a save.

**Fast lane** — `/plan` (short form) → implement → your review → hardware test.
No separate spec, no adversarial review passes. For: a new settings row wired to
existing engine code, a copy change, a tools/ script fix, a doc edit.

The rule of thumb: **if the plan would be shorter than the review of it, use the
fast lane.** The `/plan` command takes a `--fast` flag that produces the short
form and says in the document that it took the fast lane, so the record is
honest about which path a change came down.

---

## 8. Build order

Each stage is independently useful. We stop and use each one on real work before
building the next — that's the point of staging it, and it matches how you want
this project paced generally.

| Stage | What gets built | Proven by |
|---|---|---|
| **A** ✅ | `CLAUDE.md` only — **done** | A fresh session behaves correctly without re-explaining the build, the heredoc hazard, or the commit rule. *Still to be proven in practice; see §11.* |
| **B** | `specs/` + index + spec template + `/spec` + `spec-author` + `grill-me` | Run `/spec` on a real backlog row. Compare to how you'd have written it. |
| **C** | `/plan` + `planner` + plan template | Run `/plan` on that spec. Compare to `workshop-tools.md`. |
| **D** | `/review-plan` + `plan-reviewer` + review template | Point it at an existing finished plan whose outcome you already know. Does it find the things that actually went wrong? |
| **E** | `/implement` + `implementer` + implementation-report template | First full run through A–E on one real feature. |
| **F** | `/review-code` + `code-reviewer` | |
| **G** | `/verify` + `verifier` | |
| **H** | `/document` + `documenter` | |
| **I** | Orchestration: `/feature 022` runs the chain with the human gates in the right places | Only after A–H have survived contact with two or three real features. |

Stage D's test is the good one and worth doing properly: run the reviewer
against a plan you already know the outcome of. `pickers.md` lists five
deviations that were discovered during implementation — a plan reviewer worth
having should predict at least a couple of them from the plan text alone. If it
predicts none, the reviewer prompt needs work before we build anything on top
of it.

---

## 9. Open questions

1. **Does the fast lane get its own artifacts at all?** Currently it produces a
   short plan and nothing else. Possibly it should produce nothing but the
   implementation report, so the record exists but the ceremony doesn't.
2. **What happens when a hardware test fails?** The pipeline has no reverse
   gear yet. Probably: append to `test-report.md`, re-enter at stage 3 with the
   failure as input, and never silently amend the plan to match what got built.
3. **Does the spec stage survive?** It may turn out that a good backlog row plus
   `grill-me` at the start of planning is enough, and `specs/` is a layer of
   indirection. We'll know after stage B.
4. **Model choice per stage.** Reviewers arguably want the strongest model and
   the documenter doesn't. Deferred until there's usage to judge from.
5. **Does a subagent inherit project `CLAUDE.md`?** *(blocking — settle before
   stage B builds an agent, and certainly before stage D.)* The §6 decision to
   cut the three `bb-*` skills assumes it does. If it doesn't, stages 2 and 4 —
   which run in fresh context **by design** — would be reviewing PS4 randomizer
   plans with no idea what AFR is, and the skills question reopens. Settle it
   empirically, not by argument: spawn a throwaway agent and ask it what
   `CLAUDE.md` says about why there is no vanilla game data reachable by the app.
   It either knows or it doesn't. Note that even a "no" does not restore the
   skills — the fix would be a pointer line in each agent file, not a copy.

---

## 10. Deferred — your TODOs, kept

These are all from your outline and all still wanted, just not first:

- **Automatic plan ↔ plan-review ping-pong** until the reviewer approves, with a
  bounded iteration count and the whole exchange preserved. Mechanically
  straightforward once both agents exist and their output formats are stable —
  the `CHANGES REQUESTED` verdict is already the loop condition. Build it at
  stage I, not before, because iterating an unstable template just multiplies
  the instability.
- **Automatic implement ↔ code-review coordination.** Same shape, same timing.
  Riskier than the plan loop, because two agents agreeing that code is good is
  not the same as the code being good, and here nothing between them can run the
  program. Wants a hard iteration cap and a rule that it stops for a human on
  any disagreement it can't settle from the plan.
- **Auto-trigger documentation on a passed test.** Cheap once stage H exists.
- **Demo capture.** Not automatable from inside this loop. The realistic version
  is a `/document` step that tells you exactly which three screens to capture on
  the PS4 and writes the annotations around them once you drop the files in.

---

## 11. Progress log

**Stage A — `CLAUDE.md` — done.** Ten sections: what the repo holds, build,
how things get proven, working agreement, traps, domain orientation, standing
design preferences, documents, git, and a pointer back to this plan. Content was
distilled from the repo, the existing plans, and machine-local session memory,
which was the judgement call taken: durable project facts and hazards now live
in the committed file, and session memory keeps the long-form archive.

**How to tell whether it actually worked:** start a fresh session and ask for
something ordinary — a small change to a UI string, say. If it reaches for the
right build command, checks the string against the uppercase-only 8x8 font, and
stops before committing, the file is doing its job. If any of that has to be
re-explained, the file is missing something and should be fixed before stage B
builds on it.

**Two loose ends surfaced while writing it** — `app/` and `docs/` being entirely
untracked, and `/data/` needing a `.gitignore` entry. **Both are now closed:**
commit `785cefe` tracked the port, the docs, and `CLAUDE.md`, and `/data/` is
ignored at the bottom of `.gitignore`.

**Stage A revision — 2026-09-15.** `CLAUDE.md` was audited for bloat against its
own stated purpose in §5 of this document: *standing facts and rules that apply
to all work*. Twelve findings, 321 → 301 lines — 60 lines out, 40 rewritten back
in, so the density improved more than the length dropped. The test applied to
each candidate was: does it apply to all work, is there a rule attached, and
would deleting it change what an agent does?

The useful distinction that came out of it, worth reusing: **evidence that makes
a rule stick** (keep — "hit four separate times" is why the heredoc rule survives
contact with a confident agent) versus **history with no rule attached** (cut —
two deleted diagnostic rigs, a directory rename, a tally of past corrections).
§5 traps and §6 domain orientation were left nearly untouched for that reason.

One finding was not bloat but a live error: §9 asserted the PS4 port was
uncommitted, five commits after it was committed. A stale fact in an
always-loaded file gets acted on, which is the strongest argument in this
document for keeping the inventory in §8 honest.

**Stage A's original test still has not run.** The audit improved the file; it
did not prove it works. That test is unchanged: a fresh session asked for
something ordinary should reach for the right build command, check the string
against the uppercase-only 8x8 font, and stop before committing.

**Next: stage B** — `specs/`, the spec template and index, `/spec`,
`spec-author`, and `grill-me`. Settle §9 question 5 first.
