# Bloodborne Enemy Randomizer — instructions for Claude

Read this before doing anything in this repo. It defines the project context, working rules, architecture, verification expectations, and important constraints for working safely in this repository.

## 1. What this repo holds

```
app/                    the PS4 randomizer — the active project
  src/                    Platform, UI, Game, Randomizer, Msb, Param
  tools/                  Python verifiers, table generators, fixtures
docs/                   specs, plans, findings, and decisions — see §8
  features/               one folder per pipeline work item
  plans/                  flat plans predating the pipeline — frozen
data/                   GITIGNORED game trees — see §9
reference/              the Windows tool — read-only behavioral reference
  BloodborneRandomizer.sln
  Randomizer/             WPF app containing the Bloodborne randomizer logic
  SoulsFormats/            FromSoftware file-format library
```

The repo contains two codebases with different roles.

**`app/` — the real project.** A native C++ homebrew application using the OpenOrbis toolchain and SDL2. It runs on an already-jailbroken (GoldHEN) PS4 and performs Bloodborne randomization directly on the console. This is where active development happens.

**`reference/` — the Windows reference tool.** The original C# WPF randomizer. It is a read-only behavioral reference, not an active development target. Use it to understand intended randomizer behavior when implementing or investigating the PS4 version.

When the two implementations disagree, treat the Windows tool as the behavioral reference unless the difference is required by the PS4 implementation.

### Important constraints

* `RootNamespace` remains `MSB_Test`. This is internal C# configuration and is intentionally unchanged.
* `app/tools/starting_weapons_verify.py` parses `reference/Randomizer/MainWindowComponents/RandomizeFunctions.cs` directly. Do not move or rename that reference path without updating the verifier.
* Do not modify the `reference/` codebase unless explicitly requested.
* This is homebrew development, not jailbreak or exploit development. The PS4 is already modded and the app requires only ordinary homebrew filesystem access.
* If a task appears to require privilege escalation, sandbox escape, jailbreak development, or kernel-level work, stop and explain the dependency rather than expanding the project into that area.

---

## 2. Build

From the repository root:

```bash
cd app
make
```

Produces `IV0000-BBRD00001_00-BBRANDOMIZERMAIN.pkg`.

See `docs/build.md` for build requirements, toolchain constraints, installation, logging, and troubleshooting.


## 3. How things get proven

There is no host C++ compiler or emulator. The OpenOrbis cross-toolchain is the only C++ build environment, so runtime behavior must ultimately be verified on the PS4.

Use three verification layers:

1. **Cross-compile** — catches C++ compilation and linking errors. It does not prove runtime behavior.
2. **Python verification** — scripts under `app/tools/` mirror randomizer rules against real game data. Passing a verifier supports the algorithm's behavior but does not prove the C++ implementation matches it.
3. **Hardware testing** — the PS4 is the final authority for runtime behavior. Claude cannot perform this test. A clean build and passing Python verification mean **ready for hardware testing**, not complete.

Use `data/vanilla/dvdroot_ps4` as the trusted vanilla baseline and `data/runs/` for saved randomized output from hardware tests.

See `docs/testing.md` for verification commands, available verifiers, test methodology, and known pitfalls.

## 4. Working agreement

* **Milestones describe implementation structure. Gates describe when a human stops to test. They are independent decisions.** A milestone is a meaningful piece of functionality becoming complete — it is not, by itself, a reason to stop. A gate is a deliberate pause for hardware validation, and it is chosen separately, justified, and approved by the developer.

* **Every plan states an execution strategy, and approving the plan approves it.** Near the top of `plan.md`: the milestone structure, the execution mode (continuous or gated), where the human test gates are, and what verification runs between milestones. The default is **continuous execution with a single hardware test at the end**.

* **Never split work to create a test checkpoint.** Choose the decomposition that produces the cleanest implementation. Where functionality divides naturally, document the pieces even when they will be built continuously and tested together. Scaffolding, transitional states and seams that exist only to make an intermediate milestone testable are a defect — they deform the code for a test that usually does not happen.

* **Verify aggressively between milestones even when not stopping.** Build the `.pkg` and run the applicable automated checks after each milestone. That localises a failure to a milestone at almost no cost. Stopping for a *human* test is the expensive part, and it is what gates are for.

* **This is a learning project.** Explain what each milestone accomplishes and the underlying mechanisms involved — sandboxing, PKG structure, AFR files, map formats — as part of the work.

* **Do not create commits or branches without asking.** Leave the working tree for the user to manage.

## 5. Known traps

This project has accumulated several non-obvious failure modes and misleading signals. Read `docs/known-traps.md` before investigating issues in unfamiliar areas or making changes involving file generation, fonts, game-data trees, PS4 filesystem APIs, or parameter interpretation.

## 6. Domain and architecture

The core concepts are AFR file redirection, the local vanilla source, MSB map data, Param data, and the PS4 application layers.

Keep the application boundaries intact:

* UI does not access raw AFR paths.
* Randomizer code does not depend on SDL2.
* Platform code owns PS4/filesystem/SDL2 concerns.
* `Msb` and `Param` handle their respective game-data formats.

See `docs/architecture.md` for AFR behavior, data flow, application layering, MSB handling, and DCX format details.


## 7. Standing design preferences

* **Match the reference tool before improving its behavior.** Do not add "obvious" behavioral changes unless explicitly requested and justified against the reference.

* **Chalice dungeons are out of scope.** Do not reopen this decision unless explicitly asked.

* **Preserve reference quirks deliberately.** Known quirks are part of compatibility and should remain unless explicitly changed. Document intentional quirks as invariants in the relevant verification tools.

See `docs/design-decisions.md` and relevant historical findings for the reasoning behind these decisions.

---

## 8. Documents

`docs/` contains project specifications, findings, decisions, plans, and user documentation.

Important sources:

* `docs/randomization-feature-spec.md` — authoritative feature/status table
* `docs/ps4-homebrew-findings.md` — hardware-confirmed platform knowledge
* `docs/windows-randomizer-technical-review.md` — reference-tool architecture and behavior
* `docs/deferred-ideas.md` — explicitly deferred, unauthorized ideas
* `docs/design-decisions.md` — standing design decisions and compatibility rules
* `docs/features/` — one folder per pipeline work item: spec, plan, reviews, log
* `docs/features/README.md` — the pipeline index and each item's spec status
* `docs/plans/` — flat implementation plans predating the pipeline; frozen
* `docs/user-guide.md` — current user-facing behavior

When sources disagree, use the document's stated authority and status rules in `docs/documentation-guide.md`.

The UI as it ships is described by `docs/user-guide.md` and by
`docs/features/worlds/spec.md` §2. The older `app/UI_BLUEPRINT.md` is frozen at
`docs/plans/ui-blueprint-wizards.md` — it describes the Enable/Disable wizards
and the main menu, all retired on 2026-09-24, and must not be used as current.

See `docs/README.md` for the full documentation map, plan conventions, and status requirements.

## 9. Git

* Do not commit anything under `/data/`. It contains local game data and must remain gitignored.
* Build output is also gitignored, including `src/x64/`, `*.pkg`, `eboot.bin`, `pkg.gp4`, and `*.log`.
* Do not create commits or branches unless explicitly asked.

See the repository `.gitignore` files for the authoritative ignore rules.

## 10. Development process

This repository is moving toward a formal AI-assisted development pipeline:

spec → plan → plan review → implement → code review → verify → hardware test → document

The pipeline is being built and exercised incrementally. **Only stages that are explicitly marked as available are operational.** Do not assume planned stages exist or use tooling that has not been built.

Current status:

* **Stage A — Repository context:** active; this `CLAUDE.md`
* **Stage B — Spec:** built and exercised
* **Stage C — Plan:** built and exercised
* **Stage D — Plan review:** built and exercised
* **Stage E — Implementation:** built and exercised — six milestones of the worlds
  feature, 2026-09-22 to 2026-09-24
* **Stages F–I:** planned, not yet implemented. Hardware testing has been done by
  hand (see `docs/features/worlds/hardware-test-plan.md` for the shape that took);
  stage H would formalise it

Specs and plans are tied to backlog rows in `docs/randomization-feature-spec.md`. Each work item gets one folder, `docs/features/NNN-<slug>/`, holding its `spec.md`, `plan.md`, `plan-evidence.md`, `plan-review.md`, `implementation-report.md` and append-only `log.md`.

Stage C produces two documents on purpose. `plan.md` §1–§7 is the **implementation contract** — what to do, what must not change, how it is verified — and it is what the stage E agent reads. `plan-evidence.md` holds the investigation that justifies it. `log.md` holds the history of how both got there, so no artifact narrates its own. Folder names use three-digit row numbers (`016-...`); commands receive the unpadded number (`/spec 16`).

See `docs/ai-dev-process.md` for the process definition and current stage details, and `docs/plans/ai-dev-process-vision.md` for the workflow principles, build history and evaluations behind it.
