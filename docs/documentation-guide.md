# Documentation Guide

This directory contains the project's specifications, findings, design decisions, implementation plans, and user documentation.

## Authoritative documents

### `randomization-feature-spec.md`

The master randomization feature backlog and status board.

This is the authority for feature completion status. When a plan and the feature table disagree, treat the table as the most recently checked status.

### `ps4-homebrew-findings.md`

Hardware-confirmed platform knowledge, including:

* sandbox and filesystem behavior
* toolchain constraints
* packaging
* SDL2
* error decoding
* approaches that were tested and ruled out

Check this document before investigating platform-level behavior. Hardware findings may contradict assumptions based on SDK headers or documentation.

### `windows-randomizer-technical-review.md`

Technical reference for the Windows randomizer, including its architecture, algorithms, and known issues.

### `deferred-ideas.md`

Ideas that have been deliberately recorded but are not authorized work.

Check this before proposing related features or changes.

### `enemy-exclusion-history.md`

History of the removed `EnemyExclusionListExtra` behavior. Use this when considering individual enemy exclusions rather than restoring the old list wholesale.

### `design-decisions.md`

Standing design decisions, compatibility rules, and deliberate behavioral choices.

### `user-guide.md`

User-facing documentation describing behavior that currently ships.

## Feature folders

`docs/features/NNN-<slug>/` holds every artifact for one work item that has gone through the formal pipeline: `spec.md`, `plan.md`, `plan-review.md`, and the append-only `log.md`. `docs/features/README.md` indexes them and `docs/features/_templates/` holds the templates each stage starts from.

`docs/ai-dev-process.md` describes the AI-assisted development workflow itself, and its §13 defines this layout. `docs/plans/ai-dev-process-vision.md` is the retired build plan behind it.

## Legacy plans

`docs/plans/` contains one implementation plan per feature increment, from before the pipeline existed. Several cover multiple backlog items at once.

The directory is **frozen** — new pipeline work gets a feature folder instead — but the files remain the house-style reference. `workshop-tools.md` and `pickers.md` are the examples worth reading.

### Plan structure

Plans should begin with a plain-language explanation of what the feature does, followed by numbered technical sections.

Prefer measured values and observed facts over estimates.

After implementation, include a **Deviations from the plan as written** section describing meaningful differences between the plan and the resulting implementation.

### Status

Status lines are authoritative project information and must remain current.

When a feature is implemented, update both:

1. the plan's status
2. the corresponding row in `randomization-feature-spec.md`

Do this in the same pass.

Use precise intermediate states where appropriate. For example:

`Implemented, builds clean, awaiting hardware test`

Do not mark work as complete merely because it builds or passes non-hardware verification.

## Other documentation

For UI implementation work, `docs/user-guide.md` and `docs/features/worlds/spec.md` §2
describe the shipping screens. `docs/plans/ui-blueprint-wizards.md` (formerly
`app/UI_BLUEPRINT.md`) is frozen history — the wizards it specifies were retired on
2026-09-24 — and is kept only for its button-meaning scheme and the
"Defaults != Profile" distinction.
