# Feature pipeline — index

One folder per work item that has entered the formal pipeline
(`docs/ai-dev-process.md`). The folder holds every artifact for that item: the
spec, the plan, the reviews, and the append-only `log.md`.

| # | Feature | Spec status | Folder | Spec | Plan | Plan review |
|---|---|---|---|---|---|---|
| 16 | Unchanged Bell Maidens | **APPROVED** | [016-unchanged-bell-maidens/](016-unchanged-bell-maidens/) | [spec.md](016-unchanged-bell-maidens/spec.md) | [plan.md](016-unchanged-bell-maidens/plan.md) — questions answered, awaiting approval | [plan-review.md](016-unchanged-bell-maidens/plan-review.md) |
| 18 | Easy Shadows (covers rows 19–21: Easy Rom, Easy Failures, Easy Emissary) | **APPROVED** | [018-easy-shadows/](018-easy-shadows/) | [spec.md](018-easy-shadows/spec.md) | [plan.md](018-easy-shadows/plan.md) — questions answered, awaiting approval | — |
| 24 | Melee Movesets (covers row 25, Gun Movesets) | **QUESTIONS OPEN** | [024-melee-movesets/](024-melee-movesets/) | [spec.md](024-melee-movesets/spec.md) | — | — |
| 32 | Bypassed Enemies (`ENEMIES SKIPPED`; supersedes row 16) | **APPROVED** | [032-bypassed-enemies/](032-bypassed-enemies/) | [spec.md](032-bypassed-enemies/spec.md) — refined 2026-09-18, [log.md](032-bypassed-enemies/log.md) | [plan.md](032-bypassed-enemies/plan.md) — **both milestones implemented, hardware-tested and closed 2026-09-19** | [plan-review.md](032-bypassed-enemies/plan-review.md) — CHANGES REQUESTED, answered by the 2026-09-19 refinement |
| 33 | Protect Caged Dogs (`DO NOT RANDOMIZE CAGED DOGS`) — Central Yharnam **and the Forbidden Woods** | **APPROVED** | [033-protect-caged-dogs/](033-protect-caged-dogs/) | [spec.md](033-protect-caged-dogs/spec.md) — refined 2026-09-19, widened to the Forbidden Woods cages; D1–D8 unchanged, D9–D10 added. **D10 needs amending — see plan §9 P11** | [plan.md](033-protect-caged-dogs/plan.md) — questions answered, awaiting approval; [plan-evidence.md](033-protect-caged-dogs/plan-evidence.md) | — |
| 34 | Start With Hunter Tools (`START WITH HUNTER TOOLS`) | **n/a — skipped** | [034-start-with-hunter-tools/](034-start-with-hunter-tools/) | — | — | — |
| — | Font atlas (replaces the 8×8 bitmap font with EB Garamond) | **n/a — skipped** | [font-atlas/](font-atlas/) | — | — | — |
| — | Randomizer Settings UI (six-category Settings screen; also re-models Setup Defaults and removes the save-data handling) | **APPROVED** | [randomizer-settings-ui/](randomizer-settings-ui/) | [spec.md](randomizer-settings-ui/spec.md) — approved 2026-09-20 | [plan.md](randomizer-settings-ui/plan.md) — **approved 2026-09-20; all four milestones implemented, none hardware tested**; [plan-evidence.md](randomizer-settings-ui/plan-evidence.md), [log.md](randomizer-settings-ui/log.md) | — |

## What this index is, and is not

This tracks **the state of the artifacts**, not the state of the feature.

`docs/randomization-feature-spec.md` remains the master inventory and status
board — whether a feature is TODO, READY, DONE, or OUT lives there and only
there. Holding the same status in two files guarantees one of them goes stale,
which is the failure `CLAUDE.md` §8 warns about. When in doubt, the backlog
table is the authority on the feature; this index is the authority on the
documents.

## Spec status values

| Status | Meaning |
|---|---|
| **DRAFT** | Being written; not ready to read |
| **QUESTIONS OPEN** | Drafted, but §9 has questions the developer must answer |
| **QUESTIONS ANSWERED** — awaiting approval | Every §9 question is answered and recorded in §10, but the developer answered from a summary rather than from the spec itself. The normal end state of a `/spec` run |
| **APPROVED** | The developer has read it and approved it; ready for `/plan` |
| **SUPERSEDED** | Replaced — say by what, and leave the file in place |

`APPROVED` is human gate 0h. An agent never sets it on its own behalf; it
records that the developer gave it.

**n/a — skipped** is not a pipeline status. It marks an item the developer
deliberately took outside the pipeline, asking for implementation without a
spec or a plan. Such a folder holds an `implementation-report.md` written after
the fact and nothing else. The report says so at the top, because a folder that
merely looks thin is indistinguishable from one where the stages were forgotten.

Two items are marked this way: row 34, and `font-atlas/`.

`font-atlas/` and `randomizer-settings-ui/` are the folders with no `NNN-`
prefix — every other folder is numbered after a row in
`docs/randomization-feature-spec.md`, and platform/UI work has no row to be named
after. `randomizer-settings-ui/` is **not** skipped: it is going through the
normal spec → plan → review → implementation stages, it simply has no backlog row
to be numbered after. Two unnumbered folders is the point at which a separate
platform backlog starts to look justified; a third would settle it.

## Layout and naming

```text
docs/features/NNN-<slug>/
    log.md                    append-only cross-stage record; every stage adds, none rewrites
    spec.md                   stage B
    plan.md                   stage C — the implementation contract, §1–§7
    plan-evidence.md          stage C — traces, measurements, rejected alternatives
    plan-review.md            stage D
    implementation-report.md  stage E — one per milestone
```

Each file has one audience. `plan.md` is what a stage E implementation agent
reads and is sufficient on its own; `plan-evidence.md` is what a reviewer or a
developer reads to check that the contract is right; `log.md` is how both got
here. No artifact narrates its own history — `log.md` already does, which is why
a plan must not repeat which review finding changed which section.

Features 16, 18 and 32 predate the stage C split and keep their single combined
`plan.md`. Plans written from 2026-09-19 onward use the two-file layout.

`NNN` is the backlog row number zero-padded to three digits and `<slug>` is the
feature name lowercased and hyphenated. Row 24, "Melee Movesets", becomes
`024-melee-movesets/`.

The number comes from the backlog row so the two cross-reference without a
lookup table. Where one item covers several adjacent rows — the backlog's §11
groups features that share machinery on purpose — number it after the lowest row
it covers and name the rest in the spec's §6 Scope.

Files are named for the stage that produces them rather than for the feature, so
`docs/features/*/spec.md` reaches every spec and `docs/features/*/plan.md` every
plan.

## History

Specs used to live at `specs/NNN-<slug>.md` and plans at
`docs/plans/NNN-<slug>/`, which split each item's artifacts across two trees.
They were consolidated here on 2026-09-17.

Stage numbering changed in the same pass: the pipeline stages are lettered A–I
(`CLAUDE.md` §10 and `docs/ai-dev-process.md`), so what older documents call
stage 0, 1 and 2 are stages B, C and D. Artifacts written before the migration
keep their original wording; only the tooling was relabelled.

## Related

- `docs/ai-dev-process.md` — the canonical process definition; §3.1 is the artifact split, §5 stage B, §7 stage C, §8 stage D, §9 stage E, §13 the artifact layout
- `docs/features/_templates/spec.md` — the template every spec starts from
- `docs/features/_templates/plan.md` — the implementation contract, and its 400-line budget
- `docs/plans/` — flat plans predating the pipeline; frozen, still useful as reference and house style
- `docs/plans/ai-dev-process-vision.md` — the retired build plan: workflow principles §3, open questions §11, progress log §13
