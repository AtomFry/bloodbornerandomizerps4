# Feature 016 — log

Append-only. Every stage adds entries; **no stage edits or deletes one.** A
finding that later proves wrong gets a new entry saying so, appended below with
the original left intact. Entries are in chronological order, oldest first.

See `docs/plans/ai-dev-process-vision.md` §3.6 for why this file exists and
§6 for what an entry contains; `docs/ai-dev-process.md` §13 is the current
summary. Classes are **Correction**, **Assumption**, **Contradiction**.

---

## 2026-09-15 — stage 1 (plan) — 3 findings against `specs/016-unchanged-bell-maidens.md`

Raised by the `planner` subagent during the first `/plan 16` run, while the spec
was `APPROVED`. All three are **Corrections**: none changes what the feature
does, so none invalidated the approval. All three were verified independently by
the dispatching session before being recorded here.

**None of them has been applied to the spec yet.** The spec still contains all
three errors. They are carried in `plan.md` §6 so the implementer is not misled;
applying them to the spec needs `/refine-spec 16`.

### 1. Correction — §8's flag-off assertion cannot be checked, and the spec says it can

Spec §8 says that with the flag **off**, the six forced Yahar'gul placements
"must now differ from vanilla *regardless of the Yahar'gul zone chance*", and
lists it as "checkable from an output tree alone".

That is false for the flag-off case. With the flag off, `c1050` is still in the
replacement pool (weight 4 of 333), so a forced placement can legitimately be
redrawn *as a chime maiden* by chance. A passing check therefore proves nothing,
and a failing one is not necessarily a defect.

The flag-**on** assertion is sound and deterministic, because `c1050` is not in
the flag-on pool: all 12 forced placements must differ from vanilla.

This matters to the implementer, not just the reader — it changes what the
verifier is allowed to assert. `plan.md` §6.2 is written to the corrected form.

### 2. Correction — §8 cites a selftest that does not exist

Spec §8 names `enemy_lookup.py`'s `selftest`. `app/tools/enemy_lookup.py` has no
`selftest` subcommand — verified, zero occurrences of the string in the file. Its
subcommands are `frozen`, `diff`, `compare`, `pool`.

`plan.md` §6.2 puts every new automated assertion in `pool_verify.py selftest`,
which already has the harness.

### 3. Correction — §10.1 reads as a far larger change than it is

Spec §10.1 says building the m28 override means those six placements "stop
obeying the Yahar'gul zone chance", without stating the zone chance. A reader
reasonably infers most seeds change.

Measured: Yahar'gul's chance is **100** (`EnemyRandomizer.cpp:114-115`, matching
`FieldContainer.cs`'s `YahargulChance = 100`) and `RandInt` is inclusive at both
ends, so the skip fires on exactly **1 roll in 101**. Across 12 forced
placements, P(at least one differs) = 1 − (100/101)¹² ≈ **11%** of seeds.

Both the chance value and the inclusivity were verified in source by the
dispatching session.

---

## 2026-09-15 — stage 1 (plan) — decision 3 reversed a shipped safety choice

Not a finding against an upstream artifact — recorded because it changes
already-shipped behaviour and the reason must stay visible (§3.4).

`plan.md` §9.3 removes the up-front `Fail("enemy pool is empty ...")` at
`EnemyRandomizer.cpp:407-409` and guards the draw site instead, matching the
reference, which has no up-front check and simply skips at
`RandomizeFunctions.cs:322`.

This reverses a deliberate decision made in `pickers.md` D12 and documented
in-code at `:401-406` with the words **"Do not relax this"**. The hazard behind
that comment is real and survives the change: the port's draw is
`pool[RandInt(0, size-1)]`, and `RandInt(0, -1)` is undefined behaviour. The
reference is safe only because C# throws rather than corrupting memory.

**Every site that indexes `pool` must be guarded, not just `StepBuildPool`.**
Removing the up-front check without doing so trades a clean error message for
memory corruption.

Developer decision, 2026-09-15, in answer to the `/plan 16` questions.

---

## 2026-09-15 — stage 1 (plan) — milestone decision taken on the developer's behalf

The developer answered "do what the Windows version does" to the question of
whether the m28 override should be its own milestone. That does not resolve it:
the Windows tool is one desktop application with no milestone structure, so
there is nothing to copy. The question is about this project's build cadence,
not the randomizer's behaviour.

Taken as **one milestone** on the reasoning in `plan.md` §7 — a stop must end in
something the developer can hardware-test, and the override built alone has no UI
to exercise and no single-seed observable effect. Flagged to the developer as a
call made on their behalf; reversible at no cost until implementation starts.

---

## 2026-09-15 — stage 0 (spec) — `/refine-spec 16` applied the three Corrections

No developer feedback was supplied; this run was driven entirely by the three
findings above. All three are now applied to
`specs/016-unchanged-bell-maidens.md`.

**Classification: Correction. Approval survived.** The spec's four §10 decisions
stand verbatim — verified by the dispatching session, not merely reported by the
agent. Status stays `APPROVED`, and **nothing was marked `STALE`**:
`plan.md` was built against decisions that did not change.

What changed in the spec:

- **§8** now splits the m28 override assertion into flag-on (deterministic, so
  assert it) and flag-off (**explicitly do not assert it**). Measured: with the
  flag off a forced placement redraws as a chime maiden 1.2% of the time, 13.5%
  across the 12, which is what makes that check blind.
- **§8** moves every automated assertion to `pool_verify.py selftest` and
  describes `enemy_lookup.py` as the by-eye inspection tool it actually is.
- **§10.1** gains the measured size: chance 100, `RandInt` inclusive both ends,
  so 1 roll in 101, and ≈11.3% of flag-off seeds differ — meaning ≈89% are
  byte-identical to today. The decision itself is untouched.
- **New §11 Amendments after approval** records all of it.

Reconciliation caught four statements elsewhere that the corrections falsified —
§2's "Off = exactly today's behaviour", the 6-vs-12 placement discrepancy, §3's
unquantified "small and probably harmless", and §7's defaults line.

Two measured additions the agent made beyond the findings, both grounded:

- The 1-in-101 off-by-one **exists in the reference too**
  (`RandomizeFunctions.cs:190`), so it is a faithfully ported quirk and nothing
  here asks for it to be fixed (`CLAUDE.md` §7).
- *Inference:* on the ≈11% of seeds where it fires, divergence is not local — one
  `std::mt19937` seeded once means one extra draw shifts every draw after it.

## 2026-09-15 — correction to this log — finding 2 above misquotes the spec

Finding 2 in the first entry says *"Spec §8 names `enemy_lookup.py`'s
`selftest`"*. **That is wrong.** The string `selftest` never appeared in the
spec. What §8 actually said was that `enemy_lookup.py frozen`/`diff` were one of
"two existing mirrors" covering the feature.

The substance of the finding was correct and worth fixing — `enemy_lookup.py`
cannot carry an assertion, having no selftest and no pass/fail at all. Only the
quotation was wrong. A refiner working from the log alone would have searched the
spec for a string that was not there.

Raised by the `spec-author` agent during the refine run, which recorded the
discrepancy in the spec's §11 rather than silently writing the log's version into
the spec — the correct call.

The entry above is left as written, per this file's append-only rule.

---

## 2026-09-15 — stage 2 (plan review) — verdict CHANGES REQUESTED

First run of `/review-plan`. Review at `plan-review.md`. One blocking finding,
four should-fix, one worth-considering. The plan's status was NOT changed — a
review is not an approval.

### Blocking (2.1) — verified independently by the dispatching session

Removing the up-front `Fail("enemy pool is empty ...")` (plan §9.3) deletes the
**only hard failure the enemy path has for an unusable `VanillaSource`**.

Confirmed in source: `StepMirror` logs `"warning - incomplete mirror"` and
continues (`EnemyRandomizer.cpp:310-312`); `StepReadMap` logs
`"skipping missing map"` and returns (`:335-337`). Neither fails the run. So with
a wrong or half-transferred vanilla tree, no map loads, the pool is empty, and
today the `Fail` is what makes that visible. Without it the run reports
**success** having randomized nothing — `RANDOMIZED 0 ENEMIES ACROSS 0 MAPS`.

This matters for the decision history: the developer's answer "match Windows
properly" (logged above) was given about **the enemy-picker route to an empty
pool**. The bad-VanillaSource route was not in front of them. The finding does
not overturn that decision; it shows the decision was taken on partial
information, which is the developer's to revisit.

### Should-fix, in one line each

- **2.2** §3.2's placement-loop shape carries no empty-pool guard, and §4 row (e)
  says only "guard every site that indexes `pool`". The reference's equivalent is
  a single per-placement gate at the write (`RandomizeFunctions.cs:322`), not
  five draw-site guards. Failure mode is `RandInt(0, -1)` UB.
- **2.3** §5.1 claims the flag-off change is "verified by the Python mirror
  pinning the rule", but none of §6.2's ten selftest cases models the placement
  decision at all.
- **2.4** §6.3's `enemy_lookup.py frozen --bell` will not print what the plan
  says. Verified: `cmd_frozen` prints only `totals["reference"]` and
  `totals["ours"]` (`enemy_lookup.py:202-203`), and §4's tool row does not list a
  change to it.
- **2.5** Row index 4 is positional in three parallel arrays consumed by
  `Controls.cpp`'s `DrawMenuList`; §4 says "insert at index 4" only for the
  header constants.
- **2.6** (worth considering) `docs/randomization-feature-spec.md:134` still says
  "Trivial. Three strings appended" and is on the explicitly-not-changed list,
  against `CLAUDE.md` §8's status-line rule. May be stage 6's job; the plan does
  not say.

### Numbers: all reproduced

The reviewer re-derived every measured figure in the plan from
`data/vanilla/dvdroot_ps4` and all agreed — 2877 enemy parts, 608 excluded, 54
maiden placements, pool 333/82 → 317/80, `c1055` zero across 43 files, the 12
forced placements all `NPCParamID 105810`, zone chance 100, 11.3%.

Two reference line numbers drift by one or two (exclusion test at `:28` not
`:26`; override block `289-320` not `288-320`). No consequence.

---

## 2026-09-15 — stage 2 (plan review) — finding against a code comment, not the plan

Raised by the reviewer while verifying the plan's rejection of the `changeData`
refactor. Recorded here because it would otherwise exist only in a subagent
report and die with the session (§3.6 rule 1).

**The reference's `cc` flag cannot change any outcome in a base map.** An enemy
matching `nonoList` sets `changeData = false` *and* `cc = false` in the same
iteration, so the zone block's `else { if (cc) changeData = true; }` can only
ever re-set a value that is already `true`.

The port's file-header comment at `EnemyRandomizer.cpp:24-30` describes `cc` as
forcing "every later enemy in that same map to skip its zone-chance roll and
randomize unconditionally" — the opposite of what it does.

**The port's behaviour is correct either way** (it does not reproduce `cc`); only
the stated reasoning is wrong. This sits in the same comment block as the
known-wrong m28 bullet that spec §10.4 reserves for separate work, so it belongs
in that same backlog entry.

Class: **Correction**, against code comments rather than against the plan or the
spec. Nothing downstream is invalidated.

---

## 2026-09-16 — stage 3 (implement) — §9.3 deferred in answer to blocking finding 2.1

The plan-review verdict was CHANGES REQUESTED on one blocking finding: removing
the up-front `Fail("enemy pool is empty ...")` would delete the only hard error
the enemy path has for an unusable `VanillaSource`.

Put to the developer before implementation started, with three options —
split the two causes (fail on "no maps read", skip on "pool empty"), take §9.3
as written, or defer it. **Decision: defer §9.3 out of row 16**, 2026-09-16.

Class: **Contradiction** — decision 3 of 2026-09-15 is reversed, on information
that was not in front of the developer when they made it. The log entry of
2026-09-15 recording decision 3 stands as written, per this file's append-only
rule; this entry supersedes it.

Consequences carried into `plan.md` §10.1:

- `EnemyRandomizer.cpp`'s `Fail` and its "Do not relax this" comment are
  unchanged. Plan §4 row (e) was not applied.
- **Review finding 2.2 is moot** — no replacement guard exists to specify, and
  `RandInt(0, -1)` stays unreachable.
- **§5.2's half-built-tree risk stands.** Flag on + only the two `CHIME MAIDEN`
  rows ticked still fails after the mirror phase. `NoneEnabled()` does not catch
  it, because the selection is not empty.
- **§6.5 step 8 inverts**: the expected result is now the clean empty-pool
  failure message, not a completed no-op run. Still worth running, to confirm it
  is the message and not a crash.

Matching the reference here is now its own open item rather than a passenger on
row 16. The comment at the `Fail` was extended to record the second route to an
empty pool that this feature creates, and to name where the decision lives.

### Should-fix findings: 2.3, 2.4, 2.5 addressed; 2.6 not

- **2.3** — `enemy_lookup.py` gained `randomizes()`, a line-for-line mirror of
  `StepWriteMap`'s four-line placement decision, and `pool_verify.py selftest`
  gained five cases driving it across all 101 rolls in both flag states. §5.1's
  claim that the mirror pins the rule is now true.
- **2.4** — `cmd_frozen`'s summary prints the bell total and a grand total.
  Measured: 608 without `--bell`, 662 with. The plan's arithmetic is printed
  rather than inferred.
- **2.5** — the new entry was inserted at position 4 in all three parallel item
  vectors, and both screen files carry a comment stating that the row constants
  *are* the item positions.
- **2.6** — `docs/randomization-feature-spec.md:134` still reads "Trivial. Three
  strings appended". Left to the documentation stage, recorded as stale.

### Result

Clean rebuild with zero warnings, `.pkg` built. `pool_verify.py selftest`
43/43 including 18 new `bell:` cases; `table both` PASS at 82/17;
`ui_scroll_verify.py` PASS at 15 rows. Every measured figure in plan §6.2
reproduced against `data/vanilla/dvdroot_ps4`.

**Ready for hardware test, not done.**

---

## 2026-09-17 — documentation migration (not a stage finding)

Recorded here because this file's own path changed and the entries above cite
paths that no longer resolve.

This feature's artifacts moved from two trees into one folder:

| Was | Now |
|---|---|
| `specs/016-unchanged-bell-maidens.md` | `docs/features/016-unchanged-bell-maidens/spec.md` |
| `docs/plans/016-unchanged-bell-maidens/plan.md` | `docs/features/016-unchanged-bell-maidens/plan.md` |
| `docs/plans/016-unchanged-bell-maidens/plan-review.md` | `docs/features/016-unchanged-bell-maidens/plan-review.md` |
| `docs/plans/016-unchanged-bell-maidens/log.md` | `docs/features/016-unchanged-bell-maidens/log.md` |

Pipeline stages were relabelled from 0/1/2 to the letters B/C/D used by
`CLAUDE.md` §10 and `docs/ai-dev-process.md`. The entries above say "stage 1"
and "stage 2"; read those as stage C and stage D.

Per the append-only rule, the entries above were **not** rewritten. Only this
file's header pointer was repointed, because it named a document that has been
retired to `docs/plans/ai-dev-process-vision.md`.

No finding, no decision, no change to the plan or the spec.
