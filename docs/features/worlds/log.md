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
