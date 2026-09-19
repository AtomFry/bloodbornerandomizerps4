---
name: grill-me
description: Structured interrogation of an unclear request before specifying or planning it. Use when a backlog row, feature request, or change description does not yet pin down the intended behaviour well enough to plan against — typically at stage B (spec) and stage C (plan) of the development pipeline.
---

# Grill Me

Turn a vague ask into a settled one, without wasting the developer's time on
questions the repository can already answer.

## The one rule that matters

**Exhaust repository evidence before asking a single question.**

A question the repo answers is worse than no question: it costs the developer
attention, and it teaches them that answering you is a chore. On this project
most "unknowns" are already written down — in the reference tool, in the
backlog, in a prior plan, or in the game data itself.

Ask only what evidence genuinely cannot settle: intent, preference, scope, and
risk tolerance.

## Investigate first, in this order

1. **The backlog row** — `docs/randomization-feature-spec.md`. Rows carry the
   feature number, the reference flag name, what it does, status, and often a
   cost estimate. That is more than it looks like.
2. **The reference tool** — `reference/Randomizer/`. This is the behavioural
   authority (see `CLAUDE.md` §1 and §7). `BooleanHandler.cs` is the real
   settings list; the XAML labels are incomplete and sometimes misleading. If
   the question is "what should this do?", the default answer is "what the
   reference does".
3. **Prior plans** — `docs/plans/`. Several features are neighbours of one
   already built, and the earlier plan usually records the decision you are
   about to re-ask. `pickers.md` and `workshop-tools.md` are the style and
   depth reference.
4. **The port's own code** — `app/src/`. What already exists, what can be
   reused, and what the platform layer can actually do.
5. **The game data** — `data/vanilla/dvdroot_ps4`, via the Python tools in
   `app/tools/`. Counts, pool sizes, and param rows are measurable. Measure
   them rather than estimating; the house style requires real numbers.

Only after these should a question reach the developer.

## What is actually worth asking

Questions that evidence cannot settle, roughly in order of how often they bite:

- **Scope boundary.** Which backlog rows does this cover, and which
  deliberately not? Neighbouring rows often look like one feature.
- **Fidelity versus improvement.** The standing preference is fidelity to the
  reference, and several "obvious fixes" have made things worse
  (`CLAUDE.md` §7). If you want to deviate, ask — and say what it costs.
- **Behaviour the bytes do not reveal.** Knowing what a flag writes is not
  knowing what it does. Never infer a setting's meaning or polarity from
  decoding alone (`CLAUDE.md` §5).
- **UI shape.** Where a setting lives, what it is called, whether it needs a
  sub-screen. Remember the 8x8 font is uppercase A–Z and digits only — no
  punctuation, no lowercase, unknown glyphs render blank and silent.
- **Risk tolerance.** Anything that could make a run unwinnable, corrupt a
  save, or cost a hardware test cycle deserves an explicit decision.
- **What the hardware test should look at.** The developer runs it; only they
  can, and they need to know what would count as failure.

## How to ask

Batch the questions. Number them. For each one give:

- the question, in plain language
- what you already established, so it is clear you did the reading
- **a recommended answer and why** — a question with no recommendation pushes
  the whole decision back onto the developer

`docs/plans/starting-weapons.md` §4 is the worked example of this format on
this project. Read it rather than inventing a new shape.

Prefer a small number of consequential questions over a thorough list. Four
good questions get answered; fourteen get skimmed.

## Stop when

- every remaining unknown is a genuine preference or risk decision, and
- each has a recommendation attached, and
- nothing on the list could have been settled by reading the repo.

If a question is blocking — proceeding under either answer would waste real
work — say so explicitly and separate it from the rest.

## Anti-patterns

- Asking what the reference tool does instead of reading it.
- Asking for a number that `app/tools/` could measure.
- Asking permission to follow a standing rule that `CLAUDE.md` already states.
- Re-opening a settled decision: chalice dungeons are out of scope, and
  `docs/deferred-ideas.md` lists ideas already declined on purpose. Check it
  before proposing a feature.
- Presenting questions without recommendations.
- Continuing to ask once the answers stop changing what gets built.
