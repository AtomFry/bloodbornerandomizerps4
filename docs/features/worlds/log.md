# Log — Worlds

Append-only cross-stage record. Every stage adds; none rewrites. No other
artifact in this folder narrates its own history — that is what this file is for.

---

## 2026-09-21 — Stage C, implementation planning

**Wrote** `plan.md` (the implementation contract, §1–§7) and `plan-evidence.md`
(the investigation), against `spec.md` at status **APPROVED**.

**Investigation.** There was no reference trace to make: the Windows tool has no
save-data, profile or configuration-store code at all, so the feature has no
reference behaviour to match and the only constraint inherited from it is output
parity. Apollo Save Tool's restore sequence was used as prior art instead, with
three departures recorded in evidence §E1.1. The survey covered the temporary
save-data probe, `AfrManager`, the `randomizer-settings-ui` settings chain,
`FileIo`, the resumable-job pattern, and the four Python verifiers that would be
affected. Measurements were taken from the hardware backup in `data/Save Backups/`
and from `data/vanilla/dvdroot_ps4`, each with its command, in evidence §E4.

**Shape.** Seven milestones, M0–M6. M0 is the developer-specified hardware probe;
M1–M3 build the platform half and are hardware-tested through the probe screen
promoted to an operations harness; M4–M6 build the UI and retire the Enable and
Disable wizards, `MenuScreen`, `PlaceholderScreen` and the probe scaffolding.

**Four questions were put to the developer in §8**, and two of them were spec gaps
rather than planning questions:

| # | Question | Outcome |
| - | -------- | ------- |
| Q1 | Should `START FRESH` revert to `KEEP EXISTING` after one activation? | **Spec gap.** Answered more strongly than recommended: `SAVE DATA` always defaults to `KEEP EXISTING` and `START FRESH` is a one-shot the player chooses deliberately each time. Amended the spec as **D17**; plan **P12** |
| Q2 | How does the player switch tabs, given that Left/Right already changes a value? | Neither option offered. The premise was wrong — `randomizer-settings-ui` spec §2 and both `UpdateRail` implementations leave Left/Right unused on the rail — so tabs switch on Left/Right **while focus is on the rail**. Amended the spec as **D19**; plan **P13** |
| Q3 | How are worlds scoped when the user has no PSN account id? | Not a developer question: milestone 0's identity results answer it. Recorded as plan **P14** with the recommended answer kept as the fallback |
| Q4 | Should a restore fail or continue when a file inside the mount refuses to be written? | Not a developer question: milestone 0 step 5's per-file results answer it. Recorded as plan **P15** with the recommended answer kept as the fallback |

**A third spec gap was raised in the stage C report rather than in §8**: on a
console whose randomizer output predates this feature — randomizer files present
with no manifest — first-run capture still puts the live save into Vanilla. The
developer accepted the plan's handling and amended the spec as **D18**; plan
**P16**.

**A fourth ambiguity was raised and resolved in the plan**: spec §2's `SAVE` table
describes `KEEP EXISTING` only as adopting the live save, which cannot be right for
a world that already has its own. The developer promoted the plan's reading —
restore this world's own save when it has one, adopt the live save when it does
not, never overwrite a world's stored save with another world's — and the
confirmation screen must say which applies.

**The contract's length was raised as a deviation.** §1–§7 runs to 576 lines
against the ~400-line guidance. The planner checked for the usual cause — leaked
evidence or history — and found none; the overage is seven milestones' step lists.
The planner offered to split it into a platform plan and a UI plan. The developer
chose to keep one contract and to record the overage as accepted rather than trim
it. Plan **P17**.

**Reconciliation pass**, same day, after the answers:

* `plan.md` §2 gained B12a, B30, B31 and B32; no existing behaviour was renumbered.
* §3.3's `START FRESH` hazard became a closed hazard pointing at phase 7.
* §4.3's phase-6 table was rewritten to the decided `KEEP EXISTING` semantics, and
  phase 7 gained the `START FRESH` revert.
* §4.5 gained the tab binding and lost two justification clauses in a genre pass.
* Milestones 0–6 were updated: M0's gate now names the P14 and P15 answers it
  produces, M1 and M2 consume them, M3 implements the revert inside the
  transaction, M4 gains the tab-switching step, M5 pins `KEEP EXISTING` as the
  default, and M6 surfaces the revert and states the confirmation wording.
* §6's H7 and H10 pass conditions were extended to cover the revert and the tab
  binding.
* §8 was emptied; §9 gained P12–P17.
* `plan-evidence.md`: §E5.4 and §E5.7 were rewritten as decided behaviour rather
  than open risks, §E5.3 now cites D18, a new §E2.8 records the rail-input evidence
  behind D19 and corrects the planner's wrong premise, §E3 gained four rejected
  alternatives, and §E7 gained the account-id unknown.

**Status** set to `QUESTIONS ANSWERED — awaiting developer approval`. The developer
answered from a summary and has not yet read the reconciled plan.

**Not done, deliberately:** no production code, no branch, no commit;
`docs/features/README.md`, `app/UI_BLUEPRINT.md`,
`docs/randomization-feature-spec.md` and `docs/ps4-homebrew-findings.md` untouched.

**Open item carried forward, not blocking:** safety backups accumulate at about
27 MB per activation and are never pruned, per spec §7.1 and §2. A retention
policy is a behavioural decision and has no spec entry yet. Evidence §E5.5.

---

## 2026-09-21 — Stage E, milestone 0

Milestone 0, "hardware probe", **completed**. No stop condition fired. Milestone 1
not started. Built on a clean tree — the developer committed everything through
`0716d70` before this ran, so a rollback point exists.

Three files changed, all scaffolding: `Platform/SaveDataProbe.{h,cpp}` and
`UI/SaveProbeScreen.cpp`. No `ScreenId`, no `Makefile`, no `LIBS` change, and
none of milestone 1+'s production files.

**Six deviations**, recorded in `plan.md` §10. The two that matter: the guard's
verdict prints after the search rather than before it, because it compares
against the search results and cannot logically precede them; and the write probe
is a third button (`TRIANGLE`) rather than replacing `SQUARE`, because `SQUARE`
produces the backup that step 5 restores from and must stay separately runnable.

**Safety invariants confirmed independently by the dispatching session**, not
taken on report — these are what stand between a diagnostic and a destroyed
playthrough:

* Four save-data mounts exist. Three are `RDONLY`, including every mount of the
  live directory. The single `RDWR|CREATE2` names `kScratchDirName` — the
  compile-time constant — and never a discovered or live directory name.
* The guard arms only when the scratch name matches none of the discovered
  directories, and `scratchAllowed()` re-checks three conditions at **every**
  write call site rather than trusting one evaluation. Its own comment gives the
  reason: "a guard evaluated far from the call site is a guard that a later edit
  walks past."
* A leftover scratch directory trips the guard and keeps it tripped. Deliberate —
  the guard cannot distinguish a leftover from a collision, and recovery is the
  console's Saved Data Management.

**`statvfs` is a stub that cannot fill the caller's struct.** The implementer
found this statically; the dispatching session disassembled
`libc.a(statvfs.lo)` independently and confirms it:

```asm
statvfs:  subq $0x928,%rsp ; movq %rsp,%rsi ; callq statfs
          sarl $0x1f,%eax  ; addq $0x928,%rsp ; retq
```

The caller's buffer pointer (`%rsi`, argument 2) is overwritten with a local
stack address before the call, and there is no copy-out. The caller's struct is
never written. This is the same class of SDK/kernel ABI mismatch as
`sceKernelStat`'s `st_size` returning 88 for a 44 KB file, and it means §4.4's
"if milestone 0 shows its fields are sane" branch is already decided against.

Step 3 was implemented as written rather than substituting `statfs` — which does
fill a caller buffer, with FreeBSD's 488-byte struct. That substitution is a plan
question, not an implementation one. It is **not** the plan's statvfs-unusable
stop condition, which additionally requires the fallback capacity probe to have
failed, and that is milestone 1's work. Step 3 also dumps 128 raw bytes into a
zeroed 1 KB arena, so an oversized kernel write shows up as a visible dump rather
than a smashed stack.

**Verification, re-run independently:** `settings_ui_verify.py` 40/40;
`ui_scroll_verify.py` PASSED; `pool_verify.py selftest` 88/88; `make clean &&
make` with no warnings on the changed objects. `plan.md` §1–§7 unchanged at 599
lines with only §10 appended.

**Not run:** `worlds_verify.py` (does not exist until M1), the rest of the parity
suite (no randomizer, settings or table code touched), and H1–H3, which are the
hardware run itself.

**Now awaiting the hardware probe.** Its output settles **P14** (account scoping)
and **P15** (which files a restore writes), and its steps 5 and 7 are the gates on
the entire feature. The developer records the results in
`technical-findings.md` §7.

---

## 2026-09-22 — Milestone 0 hardware results

The probe ran on a real PS4 over 2026-09-21/22. Full evidence is in
`technical-findings.md`, rewritten from it. **Every mechanic the Worlds feature
needs is now proven**, and two of the plan's open decisions are settled.

**Answered:** read + backup (byte-exact, game-safe); `RDWR` mount of an existing
container (granted, harmless); overwriting files (13/13); **creating** files;
**unlinking** files; `sceKernelRename` on a directory; identity via
`GetForegroundUser` + `GetNpAccountId`.

**P14 settled** — `sceUserServiceGetNpAccountId` returns exactly the
`ACCOUNT_ID` stored in the save's own `param.sfo`, so ownership is checkable
without parsing the save.

**P15 settled** — the restore file set is *everything except `sce_sys`*. All four
`sce_sys` entries refuse with `0x8002000D` (EACCES). This costs nothing: they
hold `ACCOUNT_ID`, `TITLE_ID`, `SAVEDATA_BLOCKS` and the icon, all already
correct for the container being written into. Apollo needs `patch_sfo` only
because it moves saves between accounts; we do not. Narrower than the planner's
fallback, which had assumed `param.sfo` would be writable.

**START FRESH needs no new API.** Unlinking `userdata*` and `backup*` while
leaving `sce_sys` produced a fresh game, and a later restore brought the saves
back. `sceSaveDataDelete` was never called and is not required — the plan's
step 7 was gated on a `CREATE2` mount that never succeeded, so it was
unreachable dead code throughout.

That restore is itself the answer to the last unknown: the `userdata` files no
longer existed, so restoring them was **file creation**, not overwriting. Every
earlier run reported `CREATED 0`.

**`CREATE2` is refused and disabled.** `0x809F0008`, `REQUIRED BLOCKS 0`. Apollo
creates containers by writing a PFS image and a sealedkey under `/user/home` and
registering them in `/system_data`'s `savedata.db`; the probe's own directory map
shows neither path exists in this sandbox.

**An incident worth carrying forward.** After the run in which `CREATE2` failed,
Bloodborne hung on a black screen and recovered only when the save was deleted
from the console's Saved Data Management. The competing explanation — swapping
AFR content under an existing save — was ruled out by the developer, who has done
that many times without incident.

The uncomfortable part: the failed mount named `BBRRESTORETEST` and never named
`SPRJ0005`, yet `SPRJ0005` is what broke. **Naming a different directory did not
contain the damage**, so the scratch-name guard is not the isolation it was
designed to be. Two working assumptions died there — that a failed operation is a
no-op, and that directory naming scopes the blast radius. Both are recorded as
traps.

**`statvfs` cannot report free space.** Its libc implementation overwrites the
caller's buffer pointer with a local before calling `statfs` and never copies
back; confirmed statically by disassembly and on hardware (all fields zero,
`/user` returning `0xFFFFFFFF`). Spec **D13**'s capacity refusal needs a
different mechanism — `statfs` is the obvious candidate and is untested.

**Spec changes this forces**, none of which are plan tweaks:

* `START FRESH` is unlink-based, not delete-and-recreate.
* The app can never create a save container, so a world's save can only be
  written into one the game has already made.
* `sce_sys` is never written.
* D13's capacity check needs a working measurement first.

Milestone 0's gate is met. Milestone 1 should not start until the spec carries
these.

---

## 2026-09-22 — the findings become the technical doc, and milestone 0 leaves the plan

Three artifacts were carrying the same platform knowledge in three registers: the
spec explained the mechanisms, the plan restated the procedures inside its
milestone steps, and `technical-findings.md` held the raw evidence. A stage E
agent reading all three would read the `sce_sys` rule four times and the unlink
rule three.

The developer's instruction settled it: the research becomes one reference the
planning and implementation agents are pointed at, and neither of the other two
documents repeats it.

**`technical-findings.md` gained §8, "The four operations, as sequences"** — find
the save, back up, empty, restore, plus confirming ownership and the rules that
bind all four. It is written as recipes rather than as a narrative of what was
discovered, because that is what an implementer needs, and each carries the
measured result it came from: `REMOVED 11 / REFUSED 0`, ~15 MB in ~1,009 ms, 26
files at 26,949,914 bytes. The older sections renumbered around it — traps to §9,
save format to §10, still-unknown to §11.

**Milestone 0 came out of `plan.md`.** It was complete, hardware-run, and written
up in three other places; carrying its eight steps in an implementation contract
meant a stage E agent reading forty lines of work that will never be done again.
§1–§7 went from 597 to 549 lines. Milestones 1–6 keep their numbers — other
documents cite them, and the gap at 0 explains itself. H1–H3 left the hardware
table with it; the run now starts at H4 and every H-number a milestone cites
still resolves. The planner's own flag on this: milestone 1's six-clause
invariant list collapsed to a citation of findings §8.6, which shortens an
implementer's leash if they never open that file — mitigated by §3.1 still
spelling the same four rules out in the section every implementer reads.

**The spec's §4 was rewritten to consequences only.** It had grown mechanism —
PFS images and sealedkeys, EACCES, `statvfs` being a libc stub — none of which
changes what the feature does. What survives is what the results allow and
forbid: the feature is possible; only the game can create a container, so
activation must detect a console where Bloodborne has never run; a container is
emptied, never destroyed; the save-data title must be discovered; capacity cannot
be checked in advance; a failure is not a no-op. §8's first two hardware tests
are struck as already proven, pointing at findings §8.3 and §8.4 rather than at a
milestone that no longer exists.

Nothing about the feature's behaviour changed in this pass. The spec's status is
unchanged and the plan is still awaiting the developer's re-approval, with Q5 and
Q6 open in §8.

---

## 2026-09-22 — Q5 and Q6 answered, plan approved

The plan's two open questions were both spec gaps rather than plan choices — they
asked what the feature *does*, not how to build it — so the answers landed in the
spec as binding decisions and the plan records them only as provenance.

**D26 — a container with no save files.** A player can delete their Bloodborne
save through the console's own Saved Data Management, leaving the container
present but holding nothing. Activation then has nothing to back up, and the
tempting behaviour — capture whatever is there into the outgoing world — would
overwrite that world's good stored save with an empty one, in the single case
where the safety backup that normally covers such a replacement cannot be taken.
So phases 2 and 3 are skipped and the outgoing world's save is left exactly as it
was. The confirmation screen says so before the player commits. The contract
already behaved this way; what it gained is the sentence about telling the player.

**D27 — the AFR title when several are installed.** `GameInfo::DetectAll` found
three on the reference console while save data lived under a fourth. D25 already
said the `BLOODBORNE TITLE ID` setting names the AFR target; D27 closes what it
left open — the setting decides and nothing else does, and a configured title
that is not among the detected installs refuses at phase 1 with the detected
titles listed. The failure being avoided is a silent one: an AFR tree written for
a title that is not installed produces a game that launches unmodified with no
error anywhere, which this project has already been caught by once.

Recorded as **B36** and **B37** in the plan's behaviour table and **P24**/**P25**
in its decisions. A numbering slip caught before it settled: the first draft
cited them as B34 and B35, which were already `START FRESH` and the `sce_sys`
rule — two live behaviours that would have been silently mis-cited from the
refusal table. Checked afterwards by matching every `B` and `D` citation in
§1–§7 against its definition; 37 of 37 and 27 of 27 resolve.

**The plan is APPROVED**, 2026-09-22. Milestone 1 can start.

---

## 2026-09-22 — stage E implemented milestone 1

**Completed.** No stop condition fired. Milestone 1, "the save-data service", is
built and awaiting hardware test H4.

The run was interrupted once: the implementer was killed mid-milestone by an API
rate limit, having written `Platform/SaveData.{h,cpp}` and begun the harness but
with no verifier, no build and no report. It was resumed from its own transcript
and told to re-read what it had written rather than trust its recollection of
it. Nothing was discarded and nothing was rebuilt from scratch; the interruption
left no trace in the result beyond this paragraph.

**Files.** `Platform/SaveData.{h,cpp}` new — identity, save-title discovery,
`ReadContainer`, `EmptyContainer`, manifest and verifier, `BackupJob`,
`RestoreJob`. `Platform/SaveDataProbe.{h,cpp}` and `UI/SaveProbeScreen.{h,cpp}`
rewritten from milestone 0's diagnostic into a 394-line harness holding no orbis
call at all. `app/tools/worlds_verify.py` new. Every file is in plan §5's table
for milestone 1; none outside it was touched.

**One deviation**, recorded in plan §10. Step 7 asks that `SaveDataProbe.cpp`
end with no mount call of its own; milestone 0's write probe was deleted rather
than repointed, because its `CREATE2` mount, `sceSaveDataDelete`, `statvfs` dump
and scratch guard are exactly what §3.1 forbids the service to have. What it
established is already in `technical-findings.md` §§1–7. The cost is that those
diagnostics cannot be re-run from this build.

**Decisions the contract left open** are set out in `implementation-report.md`
§3. The ones most likely to be argued at stage F: the `bbr::savedata` namespace,
the app's first nested one; `ResolveUser` taking the foreground user with no
fallback to the initial user; the six-SKU discovery sweep not searching the
configured AFR title; `BackupJob` walking names only and taking sizes from the
copy, leaving `ReadContainer` as the bracketing read; verification as a separate
`VerifyBackup` call rather than part of the backup; a restore confirming only
the written set; `RestoreJob` carrying §4.4's restore refusals itself so the
milestone is testable alone; and `Platform/SaveData.cpp` not including
`Randomizer/FileIo.h`, to keep `Platform/` off `Randomizer/`. Report §3 counts
fifteen of these and plan §10 lists thirteen — a tally discrepancy, not a
disagreement about any one decision.

**Verification, re-run independently by the dispatching session, not taken from
the report.** `make clean && make` produced
`IV0000-BBRD00001_00-BBRANDOMIZERMAIN.pkg` at 7,143,424 bytes with `eboot.bin`
at 3,707,760, matching the report's figures. `worlds_verify.py` 29/29 manifest
and 33/33 source invariants; `settings_ui_verify.py` 40/40;
`ui_scroll_verify.py` all passed; `pool_verify.py selftest` 88/88. Checked by
hand rather than through the verifier: `CREATE2` and `sceSaveDataDelete` appear
in `src/` only in comments saying they are never used; no `sceSaveData` or
`sceUserService` call exists outside `Platform/`; `st_size` appears only in a
comment disclaiming it; `LIBS` is unchanged; containers are mounted by
`dirName`, never by index; `EmptyContainer` brackets its write with
`ReadContainer` before and after; the two modified `UI/` files contain no SDL
reference. Plan §1–§7 verified untouched by diff — §10 is the only change.

**Not run.** H4, which is hardware. The full output-parity suite: no file under
`Randomizer/`, `Game/` or the three pool tables was modified, so B28 holds by
construction, with `pool_verify selftest` run as the cheap proxy. And
`worlds_verify.py verify <dir>`, which needs a manifest-bearing backup that does
not exist until H4 produces the first one.

**A pre-existing invariant found already false.** §3.1 requires
`/data/GoldHEN/AFR/...` in exactly one file. It is in three —
`Game/AfrManager.cpp`, `Game/GameInfo.cpp:13` and `UI/EnableWizardScreen.cpp:494`
— all predating this milestone, none touched by it. The implementer reported two;
the third is in `UI/`, which also makes it a layering point for milestone 4.
`AfrManager` is milestone 2's file and `EnableWizardScreen` milestone 5's, so
both are reachable without extra scope.

**Awaiting hardware test H4** — backup, empty, restore, back up again through
the production service, the two backups matching file for file outside
`sce_sys`. Milestone 2 does not start until it passes.

---

## 2026-09-22 — developer directed milestones 2–4 to be built before their hardware gates

Stage E raised the gate: milestone 1 is built but H4 is unrun, and plan §7's
standing rule is that no milestone begins before its predecessor's hardware test.
Building 2, 3 and 4 in one run chains past three gates — H4, H5 and H6–H9 — and
`CLAUDE.md` §4 exists to prevent exactly that.

The developer was asked and chose to proceed anyway, in full knowledge of the
risk: if the milestone 1 save-data service turns out to be lossy on hardware, it
will be found with world directories written, a Vanilla save captured and an
activation transaction built on top of it, and the debugging surface will be
three milestones wide rather than one.

Recorded here because the plan's milestone ordering is a safety property, not a
scheduling convenience, and stage F should not have to infer from the commit
history why three milestones arrived untested. H4, H5 and H6–H9 remain owed and
are not superseded by this decision; they are deferred, and every milestone below
stays AWAITING HARDWARE TEST until they are run.

Each milestone was still implemented as its own stage E run against its own
section of the contract, with its own report and its own spot-check.

---

## 2026-09-22 — stage E implemented milestone 2

**Completed.** No stop condition fired. Milestone 2, "the world store", is built
and owes hardware test H5. H4 is still owed from milestone 1.

**Files.** `Randomizer/WorldStore.{h,cpp}` new — account-scoped worlds,
revisions, world saves, safety backups, `SafetyBackupJob`, `StoreSaveJob`,
`FirstRunCaptureJob` and the `*.partial` sweeps. `RandomizerDefaults.h` gains
`startFreshSave`. `RandomizerDefaultsStore.{h,cpp}` has the serializer and parser
extracted as `FormatSettings` / `ApplySettingKey` / `ApplySettingsText`, now
shared by `defaults.cfg` and revision files, plus `start_fresh_save`.
`Game/AfrManager.{h,cpp}` gains `AfrManifest` read/write and `DeriveActive`, the
§4.2 table. `UI/SaveProbeScreen.{h,cpp}` gains three harness operations.
`app/tools/worlds_verify.py` and `app/tools/pool_verify.py` extended.
`Platform/SaveData.{h,cpp}` and `Platform/SaveDataProbe.{h,cpp}` untouched.

**Three deviations**, all recorded in plan §10. `AfrManager` got the manifest and
derivation only — §5's row also lists staging, swap and remove-tree, which §7
step 6 does not and which have no caller until §4.3 phases 4–5; deferred to
milestone 3. `pool_verify.py` was edited here rather than at milestone 5, because
its exact-equality worst-case `defaults.cfg` size case necessarily breaks the
moment `start_fresh_save` is added — 616 becomes 635, split so the 584-byte
settings block is checked against the `char buf[1024]` that holds it. And the
milestone-2 harness went into `UI/SaveProbeScreen` rather than
`Platform/SaveDataProbe`, because it drives `Randomizer/WorldStore` and
`Game/AfrManager`, which `Platform/` may not include without a layer cycle.

**Decisions the contract left open**: eighteen, in `implementation-report.md` §3.
The ones most likely to be argued at stage F are `WorldStore` as a class with no
default constructor, making account scoping structural; recipe identity defined
as serializer equality rather than field comparison; a world's save swapped in
through `save.partial` → `save.old` → rename, verified at both ends; first-run
capture refusing without creating anything on ambiguous save discovery, and
rolling the account folder back if the capture fails, so a console never spends
its one first run; and `DeriveActive` as a pure function over two booleans, so
`Game/` stays independent of `Randomizer/`.

**Verification, re-run independently by the dispatching session.** `make clean &&
make` produced the `.pkg` at 7,143,424 bytes. `worlds_verify.py` 140/140
(29 manifest, 54 worlds, 57 source invariants). `pool_verify.py` 89/89.
`ui_scroll_verify.py` passed. The full output-parity suite passed by exit code:
`boss`, `treasure`, `drops`, `starting_weapons`, `caged_dogs`, `easy_modes`,
`hunter_tools`, `mergo_darkness` all clean, and `itemdata_verify.py roundtrip`
reports the archive byte-identical across the round trip.

**One verifier is deliberately red.** `settings_ui_verify.py` is 39/40: check 1,
"all 16 bool fields appear in exactly one entry", fails because `startFreshSave`
has no `SettingsModel` entry until milestone 5 step 1, whose done-condition is
literally that this verifier passes. It will still be red at milestone 4's gate,
where §6 also lists it. Flagged to the developer as a decision milestone 4 may
need to absorb; **not** resolved by exempting the field.

**A §3.1 layering point the report did not raise.** §3.1 requires orbis calls
only in `Platform/`. `WorldStore.cpp` is new code under `Randomizer/` making
roughly forty `sceKernel*` calls. It is consistent with what was already there —
`Randomizer/FileIo.cpp`, `Randomizer/RandomizerDefaultsStore.cpp`,
`Game/GameInfo.cpp` and `Game/AfrManager.cpp` all do the same and all predate
this feature — and milestone 1 deliberately kept `Platform/SaveData.cpp` off
`Randomizer/FileIo.h` to avoid a layer cycle, which leaves a new `Randomizer/`
file no in-contract way to touch the filesystem. So the invariant as written
cannot hold for `WorldStore` as the plan positions it. Recorded for stage F as a
contract inconsistency rather than an implementer error; `sceSaveData` and
`sceUserService` remain confined to `Platform/`, which is the part that governs
save data.

**Awaiting hardware:** H5, and H4 still outstanding from milestone 1.

---

## 2026-09-22 — stage E implemented milestone 3

**Completed.** No stop condition fired. Milestone 3, "the activation transaction",
is built and owes H6–H9. H4 and H5 remain owed from milestones 1 and 2.

**Files.** `Game/WorldActivation.{h,cpp}` new, 1332 lines in the implementation —
`CheckActivation` as a pure function, `PlanActivation`, the seven phases, the
journal, `ActionForPhase` and reconciliation. `Game/AfrManager.{h,cpp}` gains the
staging path, `WriteManifestAt`, `Swap`, `RemoveStaging` and `RollBackSwap` that
milestone 2 deferred. `UI/SaveProbeScreen.{h,cpp}` gains two harness operations.
`worlds_verify.py` gains an activation section and 28 more source invariants.

**Three deviations**, all in plan §10. A thirteenth refusal, `EmptySelection`,
not in §4.4 — the Enable wizard's existing `NO ENEMIES/BOSSES SELECTED` guards
moved into phase 1, because §3.1 requires the run decision unchanged and without
them an activation could reach `RandIndex(rng, 0)`, undefined behaviour by
`BossRandomizer.cpp`'s own header, *after* the safety backup and capture had run.
Phase 4 generates for every non-Vanilla world including one with every setting
off, because §4.3 skips it only for Vanilla while §3.1 also asks the `||` chain
be kept, and both cannot hold: a world with no tree has nowhere for
`.bbrandomizer_manifest`, so §4.2 would derive it as Vanilla and B4 would be
false. And `PlanActivation` is public so the harness, and milestone 6's
confirmation screen, can get phase 1's verdict without a job that stops half-open.

**A contradiction in the approved behaviour table, raised by the implementer and
not resolved by it.** B37 (D27) refuses an AFR title that `GameInfo::DetectAll`
did not find. `DetectAll` proves a title installed by finding
`/data/GoldHEN/AFR/<title>/dvdroot_ps4/event/common.emevd.dcx` — a file that
exists only because the randomizer wrote it, AFR being an overlay. B8 removes
that tree when Vanilla is activated. So activating Vanilla makes `DetectAll`
stop reporting the title, and §4.4's AFR-title row then refuses every later
activation for it. **H11's "activate Vanilla, return" cannot pass as written**,
nor can a first activation on a console whose AFR folder was never seeded. B37
was implemented exactly as written rather than reinterpreted; the three rejected
alternatives are in `implementation-report.md` §2.4. This needs the developer's
decision before milestone 6, and it is a spec question — B37 and B8 are both
binding — not a plan choice.

**Decisions the contract left open**: twelve, in `implementation-report.md` §3.
The most load-bearing is §3.6 — §4.3's phase-5 reconciliation rule ("if `.old`
exists and `dvdroot_ps4` does not, rename it back, else the swap completed") is
wrong in two of the four ways phase 5 can be interrupted, and continuing to phase
6 after a rollback produces exactly the silent incoherence §7.1 forbids. What
distinguishes the cases is whether the staged tree survives; the implementation
asks that, redoes the swap if it did not complete, then continues at phase 6.
Also §3.11: phase 1 is not literally write-free, because `AfrManager::Check`
creates and unlinks a scratch probe file, the only way to answer §4.4's
writability row.

**Verification, re-run independently by the dispatching session.** `make clean &&
make` produced the `.pkg` at 7,143,424 bytes. `worlds_verify.py` 221/221
(29 manifest, 54 worlds, 54 activation, 84 source invariants). The full parity
suite passed by exit code — `boss`, `treasure`, `drops`, `starting_weapons`,
`caged_dogs`, `easy_modes`, `hunter_tools`, `mergo_darkness`, `pool`,
`ui_scroll`, and `itemdata roundtrip`. `settings_ui_verify.py` remains 39/40,
milestone 2's known deferral, unchanged.

**Output parity checked by hand, not taken from the verifier.** The
`EnemyRandomizerOptions` field sets in `WorldActivation.cpp` and
`EnableWizardScreen.cpp` were extracted and diffed: 14 direct assignments plus
four `options.easyModes.*`, eighteen in all, identical in both and drawn from
identically-named members. The run decision is the same thirteen identifiers in
the same order, with `randomizeWorkshopTools` and `doNotRandomizeCagedDogs`
correctly still absent from it. `WorldActivation.cpp` names no AFR path itself
and goes through `AfrManager`.

**One report claim corrected.** The milestone 3 report calls `AfrManager` "still
the only file naming `/data/GoldHEN/AFR`". It is not, and was not before this
milestone: `Game/GameInfo.cpp` and `UI/EnableWizardScreen.cpp` name it too. What
is true, and is what the claim was reaching for, is that milestone 3 added no
fourth.

**Awaiting hardware:** H6–H9, with H4 and H5 still outstanding.

---

## 2026-09-23 — stage E implemented milestone 4

**Completed.** No stop condition fired. Milestone 4, "the worlds screen", is built
and owes H10's navigation-and-listing half. H4, H5 and H6–H9 remain owed.

The run was interrupted by an API rate limit during the optional parity suite and
resumed from its own transcript; the implementer re-read every file it had
written, confirmed none was mid-edit, and redid the clean build from scratch.
This is the second such interruption in this feature — milestone 1 had the first.

**Files.** `UI/WorldsScreen.{h,cpp}` new — rail, details pane, help pane, the
active marker, and the `Browse`, `Reconciling` and `FirstRunCapture` modes, plus
`WorldsSession`, the cross-switch state `Application` owns. `UI/Controls.{h,cpp}`
gain the tab-strip geometry, `TabLabel` and `DrawTabs`. `UI/Screen.h` gains
`ScreenId::Worlds`, `Defaults`, `WorldEditor` and `RequestedWorldId()`.
`UI/SetupDefaultsScreen.{h,cpp}` gain the tab strip, the `DEFAULTS` heading, a
Defaults-only category list and a focus-dependent footer. `Application.cpp` takes
the world id and session into the screen factory and opens on `WORLDS`.
`UI/MenuScreen.cpp` changes one token. `settings_ui_verify.py` +21 cases,
`ui_scroll_verify.py` +3 screens.

**Two deviations**, both in plan §10. `ScreenId::SetupDefaults` was renamed to
`Defaults` rather than a second id added, since keeping both would mean two ids
for one screen; the cost is one token in `UI/MenuScreen.cpp`, a file §5 assigns
to milestone 6. And `WorldsScreen` has three modes rather than §4.5's four:
`ConfirmDelete` belongs to milestone 5 step 7, the step that defines deleting, so
**milestone 5 step 7 will have to edit `UI/WorldsScreen.{h,cpp}`** — the same
§5-table-versus-§7 gap milestone 2 hit with `AfrManager`.

**Decisions the contract left open**: eleven, in `implementation-report.md` §3.
The load-bearing four: the details pane is label/value rows plus at most one note
chosen by precedence, since the pane has no cursor and the worst case must fit
the band, measured at 10 of 11 rows; `UNMANAGED` and `FIRST RUN` are stated in
the header band rather than against a row, because B4 forbids marking any row in
those states; the save container is read once per launch and kept in
`WorldsSession` rather than re-read per visit, because `ReadContainer` is about
two seconds on a 27 MB save and a tab switch must not freeze; and `O` exits from
`WORLDS` while `MenuScreen` is reached through `DEFAULTS`'s `O`, which keeps the
`SAVE DATA PROBE (TEST)` harness reachable — load-bearing while H4, H5 and H6–H9
are all still unrun.

**Verification, re-run independently by the dispatching session.** `make clean &&
make` produced the `.pkg` at 7,143,424 bytes with zero warnings.
`worlds_verify.py`, `ui_scroll_verify.py` and `font_atlas_verify.py` all pass,
and the full parity suite passed by exit code — `pool`, `boss`, `treasure`,
`drops`, `starting_weapons`, `caged_dogs`, `easy_modes`, `hunter_tools`,
`mergo_darkness`, and `itemdata roundtrip` byte-identical. Layering checked by
hand: `UI/WorldsScreen.{h,cpp}` contain no SDL reference, no orbis call and no
AFR path literal, and neither do the other modified `UI/` files. The AFR literal
is still the same three pre-existing `.cpp` files; milestone 4 added no fourth.

**`settings_ui_verify.py` closes milestone 4's gate red, at 60/61.** All 21 new
cases pass; the single failure is the carried-over check 1, `all 16 bool fields
appear in exactly one entry`, missing `startFreshSave`. The implementer was
instructed not to exempt the field and not to pull milestone 5 step 1 forward,
and did neither. §6 lists this verifier for milestone 4, so milestone 4 ends with
a known-red check by deliberate choice rather than by oversight: the field's
`SettingsModel` entry is milestone 5 step 1, whose own done-condition is that
this verifier passes. Put to the developer as a decision for milestone 5, not
resolved here.

**Still open from milestone 3, and untouched here:** the B37/B8 contradiction.
Nothing milestone 4 builds activates anything, so it did not constrain this work.

**Awaiting hardware:** H10's navigation-and-listing half, with H4, H5 and H6–H9
all still outstanding. Note that on a console with no worlds folder the app now
opens straight into first-run capture, which means **launching this build is
itself H5** — it is no longer possible to run H4 alone from a cold install
without passing through it.

**Precision on that, checked in the code rather than taken from the report.** The
milestone 4 report describes first-run capture as waiting for `X`. It does not.
`WorldsScreen.cpp:396–410` enters `Mode::FirstRunCapture` and constructs and
steps `FirstRunCaptureJob` unconditionally when the account directory is absent;
the `X` is the acknowledgement *after* the job completes. So on a console with no
worlds folder, capture begins on its own as soon as the app opens. The developer
cannot install this build and run H4 in isolation first — H5 will already have
happened. Anyone sequencing the outstanding hardware tests needs to know that.

---

## 2026-09-24 — a developer decision, then stage E implemented milestones 5 and 6

**One decision taken before dispatching.** The B37/B8 contradiction milestone 3
raised, open since 2026-09-22 and recorded as blocking milestone 6, was put to
the developer and answered. Recorded as **P26** in plan §9, superseding the
second sentence of P25/D27: the `BLOODBORNE TITLE ID` setting alone decides the
AFR title and is used as entered — AFR handling never infers or validates it.
The developer's reasoning was that the title id is a setting the user enters, so
there is nothing to infer. The §4.4 `AFR title` refusal and
`RefusalReason::AfrTitleNotDetected` come out. `GameInfo::DetectAll` inspects the
AFR overlay, which a Vanilla activation deletes, so it could never have been
evidence that a title is installed. Save-title discovery is a different question
and is unaffected — it keeps inspecting `param.sfo`.

**Both milestones were built in one session at the developer's explicit
instruction** — "worlds 5 and 6 both and then I'll test". This departs from
`CLAUDE.md` §4 and from §7's "do not begin the next milestone before that", and
the departure is the developer's call, made in advance and stated to both
implementer agents so neither treated the missing hardware results as a stop
condition. The cost is recorded here rather than discovered later: **no hardware
test has been run on any part of this feature.** H4, H5, H6–H9, H10 and H11 are
all outstanding, and when one fails it will have six milestones' changes beneath
it rather than one.

---

### Milestone 5 — the world editor

**Completed.** No stop condition fired.

**Files.** `UI/SettingsModel.{h,cpp}` gain `SettingCategory::Save`,
`SettingKind::SaveChoice` and the `SAVE DATA` entry. `UI/EnableWizardScreen.{h,cpp}`
renamed via `git mv` to `UI/WorldEditorScreen.{h,cpp}` and extended with the `NAME`
row, the `SAVE` category, `HISTORY`, the name editor and revision writing.
`UI/WorldsScreen.{h,cpp}` gain the fourth mode, `ConfirmDelete`.
`Application.cpp` passes the requested world id in. `Randomizer/EasyModes.h`,
`Randomizer/EnemyRandomizer.cpp` and `UI/SetupDefaultsScreen.h` carried the old
file name in prose. `settings_ui_verify.py` 61 → 71 cases; `ui_scroll_verify.py`,
`pool_verify.py` and `worlds_verify.py` follow the rename.

**Four deviations**, all in plan §10. The rename touched six verifier parses, not
§3.3's three, and that hazard row's line numbers had moved. The editor's rail left
`SHARED_GEOMETRY` — ten rows do not fit Defaults' band, so it runs its own 70px
grid and its rows no longer align horizontally with the pane rows. `FINISH` and
its readiness summary are gone, since §4.5's rail has none, and the summary moved
into Confirm's first three rows. And `Application.cpp` and `UI/WorldsScreen.{h,cpp}`
were edited though §5 assigns them to milestones 4 and 6 — the `WorldsScreen` case
was flagged in advance by milestone 4.

**Decisions the contract left open**: twelve, in `implementation-report.md` §3.
The load-bearing one is that **a new world's `SAVE DATA` is forced to
`KEEP EXISTING` rather than inherited from `defaults.cfg`** — B12 read as beating
B6, on the grounds that `start_fresh_save` is on disk and FTP-editable and a file
must not be able to arm a destructive default. Also: selecting a revision in
`HISTORY` appends immediately rather than on the next `OPTIONS`; `SaveChoice` is a
second bool-backed kind excluded from `ToggleCount()`; a new world is called
`WORLD`.

**Verification, re-run independently by the dispatching session.** The `.pkg`
built at 7,143,424 bytes. `settings_ui_verify.py` **71/71** — milestone 4's
carried-over red check 1 is genuinely green, with `startFreshSave` covered rather
than exempted. `ui_scroll_verify.py` and `worlds_verify.py` 84/84 pass. The full
parity suite passes by `selftest` against `data/vanilla/dvdroot_ps4`: `pool` 89/89,
`boss`, `treasure`, `drops`, `starting_weapons`, `caged_dogs`, `easy_modes`,
`hunter_tools`, `mergo_darkness`, `font_atlas`.

**Invariants checked by hand.** The pool tables are untouched in git. No real SDL
or orbis reference in `UI/` — the grep hits are comments. The AFR literal is still
the pre-existing set, no new file. And `X` cannot toggle `SAVE DATA` at the model
layer, not merely by convention: `SettingsModel.cpp`'s `IsDrillIn()` excludes
`SaveChoice`.

**`ui_scroll_verify.py` caught one real error in passing** — the first
`kHistoryLayout` band put `MORE BELOW`'s ink at 974 against a footer at 959.

---

### Milestone 6 — activation from the UI, and retirement

**Completed.** No stop condition fired. The feature is code-complete.

**Files.** `Game/WorldActivation.{h,cpp}` lose the AFR-title refusal, the
`DetectAll` sweep and the `GameInfo.h` include (P26).
`UI/WorldEditorScreen.{h,cpp}` run `WorldActivationJob` instead of
`EnemyRandomizerJob`, and Confirm becomes the B10 activation confirmation; the
options mapping, the `||` run-decision chain, the output path and the
vanilla-source path are gone from the editor. `UI/WorldsScreen.cpp` routes `X` on
`VANILLA` to the editor. `UI/Screen.h` is down to `None`, `Worlds`, `Defaults`,
`WorldEditor`. `UI/SetupDefaultsScreen.cpp` and `Application.cpp` follow.
**Deleted:** `UI/MenuScreen.{h,cpp}`, `UI/PlaceholderScreen.{h,cpp}`,
`Platform/SaveDataProbe.{h,cpp}`, `UI/SaveProbeScreen.{h,cpp}`.

**Six deviations**, all in plan §10. P26 was implemented here, which §7's step
list predates, and it meant editing `Game/WorldActivation.{h,cpp}` and
`worlds_verify.py`, files §5 assigns to milestones 1–3. `ScreenId::Menu` had to go
with `MenuScreen` though step 5 names only the two wizard ids.
`UI/SetupDefaultsScreen.cpp` held the last two `ScreenId::Menu` transitions. Both
UI verifiers were edited here; Confirm's head rows went 3 → 8 and `kSettingsLayout`
moved to `{470, 80, 870, 60}` after `ui_scroll_verify.py` caught `MORE ABOVE`
drawing inside the second sentence row — neither verifier was relaxed. And
**`worlds_verify.py`'s output-parity check lost its comparand**: it compared the
activation's mapping against the wizard's copy, which step 2 deleted, so the
wizard's 18-pair mapping and 13-field chain are now frozen in the verifier with
two new cases asserting the editor carries no second copy.

**Decisions the contract left open**: fifteen, in `implementation-report.md` §3.
The load-bearing ones: Vanilla routes through the editor opening on Confirm rather
than a fifth `WorldsScreen` mode; the coarse duration is three phrases derived
from the measured ~15 MB/s and the 10–20s generation figure; `OPTIONS` is gated on
`storeError_` as well as the refusal, which keeps the editor's reported recipe and
the regenerated revision from diverging; and phase 1 runs on Confirm's first frame
rather than on the `OPTIONS` press, so `CHECKING` is drawn before the blocking
read.

**Verification, re-run independently by the dispatching session.** A clean rebuild
produced the `.pkg` with no warnings. `worlds_verify.py` (29+54+54+97),
`settings_ui_verify.py` 81/81 and `ui_scroll_verify.py` all pass, and the full
parity suite is green — `pool` 89/89, `boss`, `treasure`, `drops`,
`starting_weapons`, `caged_dogs`, `easy_modes`, `hunter_tools`, `mergo_darkness`,
`font_atlas`, `itemdata roundtrip`.

**Invariants checked by hand.** `grep -r SaveDataProbe app/src` is empty of code —
two prose comments in `UI/Controls.h` and `UI/WorldEditorScreen.h` name
`MenuScreen` and `EnableWizardScreen` historically, and nothing references a
deleted screen. `-lSceSaveData` is still the only save-data library in `LIBS`. No
real SDL or orbis reference in `UI/`. The AFR literal is now in four files rather
than five, `UI/WorldEditorScreen.cpp` being the pre-existing UI one.

**The harness deletion changes how the outstanding hardware tests must be run.**
H4 and H6–H9 were written against `Platform/SaveDataProbe` and `UI/SaveProbeScreen`,
which step 6 deleted. Under the planned sequencing they would have been run at
milestones 1 and 3 while the harness still existed; testing after milestone 6
instead means they have to be re-expressed through the production UI. Two lose
coverage outright: **H4's byte-for-byte backup/restore comparison** has no UI
equivalent and can only be checked over FTP after the fact, and **H9's twelve
synthetic refusal triggers** are gone — only the few reachable naturally (no save
title, wrong account, absent container) can now be provoked. The consolidated,
ordered hardware procedure written for the developer accounts for both.

**Noticed and deliberately not fixed:** `Game/GameInfo.{h,cpp}` now has no caller
at all, P26 having removed the last one, and deleting it needs a file §5 does not
list. `UI/Controls.h`'s opening comment still names `MenuScreen`. The editor's
Settings footer still reads `OPTIONS SAVE`. `app/UI_BLUEPRINT.md` still describes
the wizards and the main menu. And `make clean` no longer removes the objects of
deleted sources, because `OBJS` comes from a live `find` — harmless, but
surprising, and it is why milestone 6's rebuild was `rm -rf app/src/x64 && make`.

**Awaiting hardware: the whole feature.** H4, H5, H6–H9, H10 and H11.

---

## 2026-09-25 — hardware test passed in full

**The developer ran `hardware-test-plan.md` end to end and reported every part
passing.** This is the first time any of the worlds feature has run on a PS4,
and it covers all six milestones at once. The feature is **DONE**.

Console: account `acct-4a17f1993020e604`, save title `CUSA00207`, save directory
`SPRJ0005`, 17 files / 15,153,434 bytes / 1136 blocks.

**Evidence captured off the console**, at the developer's paths under
`data/Worlds Testing/` (gitignored):

* `0.1 - Take a manual FTP copy of your save` — the live save, pulled with the
  PS4Explorer homebrew app rather than FTP, because `/user/home/…/savedata/` is
  not reachable over FTP directly. Worth knowing for any future save test.
* `0.2 - Take a copy of the randomizer's data directory too` — `/data/bbrandomizer`,
  490 MB by now, having accumulated across the whole build.

`_old_Worlds` was the pre-existing worlds directory, renamed aside rather than
deleted so Part A ran as a genuine first run.

### Four numbers checked rather than taken on trust

| Claim | Check | Result |
| ----- | ----- | ------ |
| The account directory is the account id | `5339001523510765060` → `0x4a17f1993020e604`, and the directory is `acct-4a17f1993020e604` | **exact** |
| The manifest's file count is real | 17 `f` lines, manifest says `files=17` | **exact** |
| The manifest's total is real | the 17 sizes sum to 15,153,434, manifest says `bytes=15153434` | **exact** |
| Vanilla's save is the first-run backup | `vanilla/save/manifest.txt` and `SaveBackups/…_firstrun/manifest.txt` are identical, `captured` included | **identical** |

**P14 is now hardware-confirmed twice over.** Milestone 0's probe showed
`sceUserServiceGetNpAccountId` returning the `ACCOUNT_ID` in the save's own
`param.sfo`; this run shows the directory the world store actually created being
that same id in hex.

### Three things the run established that the plan did not predict

1. **D25/P23 is not a theoretical distinction on this console — it is the live
   configuration.** `BLOODBORNE TITLE ID` is `CUSA03173` and the AFR tree written
   under it is the one the game reads, while the save data lives under
   `CUSA00207` with directory `SPRJ0005`. The AFR title and the save title are
   genuinely different values on the reference console, and the sweep resolved
   the save title correctly without being told it. Had either been derived from
   the other, this console would have failed. This belongs in
   `docs/ps4-homebrew-findings.md` as hardware-confirmed platform knowledge.

2. **The first activation filed its safety backup nowhere, and that is correct.**
   The log reads `DEACTIVATING NOTHING - NO WORLD IS ACTIVE` then
   `NO OUTGOING WORLD - THE BACKUP IS KEPT BUT FILED NOWHERE`. The AFR tree was
   already seeded by earlier builds and carried no `.bbrandomizer_manifest`, so
   §4.2's fourth row applied — **UNMANAGED**, no row `ACTIVE` — rather than
   "Vanilla active". P2 called this case and D18 accepted it; this is it
   happening. Nothing was lost: Part A had already captured the live save into
   Vanilla, and the `_preactivate` backup is on disk regardless.

3. **`blocks` in a manifest is the container's reported size, not the save's
   footprint.** `blocks=1136` against 15,153,434 bytes of data — 1136 × 32 KiB is
   37 MB, which is the container, not the contents. So §4.4's container-size
   refusal compares the capture-time container against the target container
   rather than a requirement against a capacity. That is conservative and correct
   for the same-console case this feature is built for, and it would also refuse
   correctly when restoring onto a smaller container. It would *not* catch a
   large save being restored into a container that is big enough overall but too
   full. Noted rather than changed — no behaviour here is wrong, but the field's
   meaning is not what its name suggests and a future reader will assume
   otherwise.

### What this does and does not close

**Closes:** H5, H10 in both halves, H6, H7, H11, H8 and the reachable part of H9.
Every `.pkg` from milestones 1–6 is now hardware-validated as one build.

**Does not close**, and is recorded in the test plan's "Coverage this plan cannot
reach": **H4's byte-for-byte backup/restore round trip** and **H9's ten synthetic
refusal rows**. Both were harness tests, and milestone 6 deleted the harness
before the harness was ever used, because testing was deferred to the end. The
substitutes — an FTP comparison of Vanilla's stored save, and the three or four
naturally reachable refusals — are weaker and were run as such.

**The sequencing lesson, recorded because it nearly cost something.** Building
milestones 5 and 6 before testing 1–4 was the developer's explicit call and it
worked: everything passed. What it did cost is that the operations harness
milestone 3 built specifically so that milestones 1–3 could be hardware-tested
was deleted, in milestone 6, without ever having served that purpose. Two tests
died with it. A feature that plans a scaffold for testing intermediate milestones
should either test them or keep the scaffold until it has.
