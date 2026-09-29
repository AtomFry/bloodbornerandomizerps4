# Feature pipeline — index

One folder per work item that has entered the formal pipeline
(`docs/ai-dev-process.md`). The folder holds every artifact for that item: the
spec, the plan, the reviews, and the append-only `log.md`.

| # | Feature | Spec status | Folder | Spec | Plan | Plan review |
|---|---|---|---|---|---|---|
| 16 | Unchanged Bell Maidens | **RETIRED** | [016-unchanged-bell-maidens/](016-unchanged-bell-maidens/) | [spec.md](016-unchanged-bell-maidens/spec.md) | [plan.md](016-unchanged-bell-maidens/plan.md) — never implemented | [plan-review.md](016-unchanged-bell-maidens/plan-review.md) |

> Row 16 shipped once and was then **retired by row 32** on 2026-09-19 (spec 032 D1); its
> effect survives as two ticks on `ENEMIES SKIPPED`. The folder is kept as the record.
> The backlog is authoritative and marks the row **OUT**.

| 18 | Easy Shadows (covers rows 19–21: Easy Rom, Easy Failures, Easy Emissary) | **APPROVED** | [018-easy-shadows/](018-easy-shadows/) | [spec.md](018-easy-shadows/spec.md) | [plan.md](018-easy-shadows/plan.md) — questions answered, awaiting approval | — |
| 24 | Melee Movesets (covers row 25, Gun Movesets) | **QUESTIONS OPEN** | [024-melee-movesets/](024-melee-movesets/) | [spec.md](024-melee-movesets/spec.md) | — | — |
| 27 | No Team Type (ships as `ENEMIES ON SAME TEAM`) | **APPROVED** | [027-no-team-type/](027-no-team-type/) | [spec.md](027-no-team-type/spec.md) — **APPROVED** 2026-09-28; three decisions, **D3 taken against the spec's own recommendation** | [plan.md](027-no-team-type/plan.md) — **APPROVED** 2026-09-28; two milestones, **no gates**, one hardware test at the end; [plan-evidence.md](027-no-team-type/plan-evidence.md), [log.md](027-no-team-type/log.md) | **BUILT** — both milestones 2026-09-28, stage D skipped; [implementation-report.md](027-no-team-type/implementation-report.md). One deviation (§6 names `worlds_verify.py selftest`, which does not run the checks it credits). **Awaiting hardware** |
| 32 | Bypassed Enemies (`ENEMIES SKIPPED`; supersedes row 16) | **APPROVED** | [032-bypassed-enemies/](032-bypassed-enemies/) | [spec.md](032-bypassed-enemies/spec.md) — refined 2026-09-18, [log.md](032-bypassed-enemies/log.md) | [plan.md](032-bypassed-enemies/plan.md) — **both milestones implemented, hardware-tested and closed 2026-09-19** | [plan-review.md](032-bypassed-enemies/plan-review.md) — CHANGES REQUESTED, answered by the 2026-09-19 refinement |
| 33 | Protect Caged Dogs (`DO NOT RANDOMIZE CAGED DOGS`) — Central Yharnam **and the Forbidden Woods** | **APPROVED** | [033-protect-caged-dogs/](033-protect-caged-dogs/) | [spec.md](033-protect-caged-dogs/spec.md) — refined 2026-09-19, widened to the Forbidden Woods cages; D1–D8 unchanged, D9–D10 added. **D10 needs amending — see plan §9 P11** | [plan.md](033-protect-caged-dogs/plan.md) — questions answered, awaiting approval; [plan-evidence.md](033-protect-caged-dogs/plan-evidence.md) | — |
| 34 | Start With Hunter Tools (`START WITH HUNTER TOOLS`) | **n/a — skipped** | [034-start-with-hunter-tools/](034-start-with-hunter-tools/) | — | — | — |
| 11 | Randomize Shop Items (armour and consumables; the other two buckets of row 5's reference function) | **APPROVED** | [011-randomize-shop-items/](011-randomize-shop-items/) | [spec.md](011-randomize-shop-items/spec.md) — **APPROVED** 2026-09-28; four decisions recorded, all taking the spec's own recommendation | [plan.md](011-randomize-shop-items/plan.md) — **APPROVED** 2026-09-28; three milestones, **no gates**, one hardware test at the end; [plan-evidence.md](011-randomize-shop-items/plan-evidence.md), [log.md](011-randomize-shop-items/log.md) | — |
| 13 | Bosses Can Replace Enemies (covers row 17, the fourteen per-area flags) | **APPROVED** | [013-bosses-replace-enemies/](013-bosses-replace-enemies/) | [spec.md](013-bosses-replace-enemies/spec.md) — **APPROVED** 2026-09-28; **eight settings**, fifteen decisions (D-A–D-O), two superseded in place; **D-L and D-N were both answered against the options offered** and reshaped the mechanism; [log.md](013-bosses-replace-enemies/log.md) | [plan.md](013-bosses-replace-enemies/plan.md) — questions answered 2026-09-28, awaiting approval; **five milestones**, continuous through M1–M4, **required gate before M5**; **corrects the approved spec’s 29 identities to 32**; [plan-evidence.md](013-bosses-replace-enemies/plan-evidence.md) | — |
| 38 | Start With A Left Weapon (14 rows; `equip_Wep_Left`) | **n/a — skipped** | [038-start-with-left-hand-weapon/](038-start-with-left-hand-weapon/) | — | — | **DONE** — hardware-tested 2026-09-27; [implementation-report.md](038-start-with-left-hand-weapon/implementation-report.md), [log.md](038-start-with-left-hand-weapon/log.md) |
| 37 | Start With A Trick Weapon (a 78-row version-level picker; grants at character creation) | **APPROVED** | [037-start-with-trick-weapon/](037-start-with-trick-weapon/) | [spec.md](037-start-with-trick-weapon/spec.md) — **APPROVED** 2026-09-27; six decisions recorded, two taken against the spec's own recommendation (D2, D6) | [plan.md](037-start-with-trick-weapon/plan.md) — **APPROVED** 2026-09-27; **all four milestones built** the same day, the M3 gate answered on hardware (route A); [plan-evidence.md](037-start-with-trick-weapon/plan-evidence.md), [log.md](037-start-with-trick-weapon/log.md) | **skipped** at the developer's request — [implementation-report.md](037-start-with-trick-weapon/implementation-report.md), **awaiting the rest of spec §8's hardware list** |
| — | Font atlas (replaces the 8×8 bitmap font with EB Garamond) | **n/a — skipped** | [font-atlas/](font-atlas/) | — | — | — |
| — | Worlds (named playthroughs with their own save data; retires the Enable/Disable wizards) | **DONE** | [worlds/](worlds/) | [spec.md](worlds/spec.md) — re-approved 2026-09-22 with D20–D25 from milestone 0; D13 superseded, amended with D17–D19; evidence in [technical-findings.md](worlds/technical-findings.md) | [plan.md](worlds/plan.md) — **DONE**, approved 2026-09-22, six milestones built 2026-09-22..24; [plan-evidence.md](worlds/plan-evidence.md), [log.md](worlds/log.md) | Hardware test **passed in full 2026-09-25** — [hardware-test-plan.md](worlds/hardware-test-plan.md) |
| — | Startup screen — replace the streaming log with a loading screen | **APPROVED** | [startup-screen/](startup-screen/) | [spec.md](startup-screen/spec.md) — **APPROVED** 2026-09-26, three questions answered the same day; [proposal.md](startup-screen/proposal.md) is the agreed shape it was written from | [plan.md](startup-screen/plan.md) — **APPROVED** 2026-09-26, both milestones implemented the same day; [plan-evidence.md](startup-screen/plan-evidence.md), [log.md](startup-screen/log.md) | **AWAITING HARDWARE TEST** — [hardware-test-plan.md](startup-screen/hardware-test-plan.md), [implementation-report.md](startup-screen/implementation-report.md) |
| U1 | Activation screens — replace the streaming progress log with a loading screen | **n/a — skipped** | [U1-activation-screens/](U1-activation-screens/) | — | — | **BUILT** 2026-09-26, awaiting hardware test — [implementation-report.md](U1-activation-screens/implementation-report.md) |
| U2 | Confirm screen — two fixed tiers replacing the 27-row scrolling review | **n/a — skipped** | [U2-confirm-screen/](U2-confirm-screen/) | — | — | **BUILT** 2026-09-26, awaiting hardware test — [implementation-report.md](U2-confirm-screen/implementation-report.md) |
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

Five items are marked this way: row 34, row 38, `font-atlas/`,
`U1-activation-screens/` and `U2-confirm-screen/`. U1 and U2 are the first folders to use the `U<n>-`
prefix from `docs/ui-backlog.md`.

`font-atlas/`, `randomizer-settings-ui/`, `worlds/` and `startup-screen/` are the
four folders with no `NNN-` prefix — every other folder is numbered after a row in
`docs/randomization-feature-spec.md`, and none of these four had a row to be named
after, because that backlog covers randomization settings only. Of the four, only
`font-atlas/` is skipped; the other three went through the normal spec → plan →
review → implementation stages.

**That threshold was reached at three, and the backlog now exists:**
`docs/ui-backlog.md`. New UI and platform items get a `U`-prefixed row there and
a folder named `U<n>-<slug>/`. The four existing unnumbered folders keep their
names — renaming them would break every inbound link for no gain — and are listed
in that backlog's §1 as the shipped rows U0a–U0d.

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
- `docs/ui-backlog.md` — the UI and platform backlog; rows U1 onward
- `docs/plans/` — flat plans predating the pipeline; frozen, still useful as reference and house style
- `docs/plans/ai-dev-process-vision.md` — the retired build plan: workflow principles §3, open questions §11, progress log §13
