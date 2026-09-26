# Documentation map

Start here. `documentation-guide.md` explains the conventions these documents
follow and which one wins when two disagree.

## Layout

```text
docs/
    README.md                     this map
    documentation-guide.md        conventions, authority rules, status rules

    features/                     the formal pipeline — one folder per work item
        README.md                 the pipeline index
        _templates/               spec, plan and review templates
        NNN-<slug>/               log.md, spec.md, plan.md, plan-evidence.md,
                                  plan-review.md, implementation-report.md, and
                                  where an item needs them technical-findings.md
                                  and hardware-test-plan.md

    plans/                        flat plans predating the pipeline — frozen
        ai-dev-process-vision.md  the retired build plan for the pipeline itself
```

## Authoritative documents

| Document | Authority on |
|---|---|
| `randomization-feature-spec.md` | The feature backlog and completion status. Wins over any plan or index that disagrees |
| `ps4-homebrew-findings.md` | Hardware-confirmed platform behaviour — sandbox, filesystem, toolchain, packaging, SDL2. Beats assumptions drawn from SDK headers |
| `windows-randomizer-technical-review.md` | The reference tool's architecture, algorithms and known bugs |
| `design-decisions.md` | Standing design decisions and compatibility rules |
| `deferred-ideas.md` | Ideas deliberately declined. Check before proposing related work |
| `known-traps.md` | Non-obvious failure modes and misleading signals |
| `user-guide.md` | User-facing behaviour that currently ships |

## Working documents

| Document | What it is for |
|---|---|
| `architecture.md` | AFR behaviour, data flow, application layering, MSB and DCX handling |
| `build.md` | Toolchain, build requirements, installation, logging, troubleshooting |
| `testing.md` | The three verification layers and the verifiers under `app/tools/` |
| `ai-dev-process.md` | How the spec → plan → review pipeline runs, and which stages are operational |
| `enemy-exclusion-history.md` | Why `EnemyExclusionListExtra` was removed |

A feature folder is flat and permanent — items are never moved once created, and
completion is read from the backlog and from each `plan.md`'s status line, never
from where the folder sits. (A `Completed/` subfolder was tried and dropped on
2026-09-25: it broke every link into the two folders it held, and it conflated
*shipped* with *retired*.)

## Feature work

Anything that has entered the pipeline lives in `features/NNN-<slug>/`, numbered
by its row in `randomization-feature-spec.md`. `features/README.md` is the index
and records each item's spec status.

`plans/` holds the flat plans written before the pipeline existed. They are
still the best examples of house style, but nothing new is added there.

## Elsewhere in the repo

- `CLAUDE.md` — standing rules for working in this repository
- `docs/user-guide.md` and `docs/features/worlds/spec.md` §2 — the UI as it ships.
  The former `app/UI_BLUEPRINT.md` is frozen at `docs/plans/ui-blueprint-wizards.md`
- `.claude/commands/`, `.claude/agents/`, `.claude/skills/` — the pipeline tooling
