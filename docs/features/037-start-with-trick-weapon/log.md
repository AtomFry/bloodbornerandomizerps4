# Feature 037 — log

Append-only. Every stage adds; none rewrites. This is the only place the
history of this feature's artifacts lives — `spec.md` and `plan.md` state what
is true now, and neither narrates how it got there.

---

## 2026-09-27 — stage B, the spec

Written by the `spec-author` subagent from backlog row 37, which had been filed
the same day out of a feasibility conversation.

**Six questions were put to the developer; all six were answered.** Four were
asked first as the ones that changed the work — the pool's scope, the
requirement profile, whether an inventory-only outcome is acceptable, and how
stage C should be scoped. Two more followed to clear the remaining blocking
question so `/plan` was not left waiting on it.

They became D1–D6 in spec §10. **Two were taken against the spec's own
recommendation**, and both are marked as such in §10 with the developer's
reason:

* **D2** — the spec recommended granting the base version only. The developer
  chose to give every version its own picker row, so a player can pick Uncanny
  or Lost deliberately. This tripled the picker from 26 rows to 78.
* **D6** — the spec recommended hardware-testing row 34 first, since it writes
  the same origin rows and would have split this feature's unknown in two for
  free. The developer chose to plan the whole feature immediately with the probe
  as milestone 1.

A seventh decision — excluding the version-8 and version-9 rows — was settled by
investigation rather than referred to the developer, and is recorded in §10 on
that basis.

**Five factual corrections the investigation made to the backlog row**, all of
which had been written into it by this session and were wrong:

| The row said | The data says |
| --- | --- |
| 37 weapons | 47 distinct player weapons; `id/1000000` merges distinct weapons |
| No hand field findable | `rightHandEquipable` / `leftHandEquipable` bits at offset 256 split all 47 cleanly |
| No weapon names in the repo | every name is in `data/vanilla/.../msg/engus/item.msgbnd.dcx` |
| `7010000` is Lost, `7020000` Uncanny | inverted — 7010000 is Uncanny |
| Precedence against row 5 is an open question | decided as D4 |

The backlog row was corrected on all five points.

**One arithmetic correction during the revision.** After D2 the orchestrating
session measured the picker at 80 rows; the subagent measured 78 and was right.
The 80 counted `12080000` and `38090000` as fourth versions of their weapons.
They are not: both are unnamed, and the exclusion rule the spec settles on is
**obtainability** — of the 84 right-hand tier-0 rows, exactly 78 appear in a
shop or an item lot and the six excluded appear in neither. Verified
independently before the correction was accepted.

Status ended at `QUESTIONS ANSWERED — awaiting developer approval`, then
**`APPROVED` by the developer the same day**.

---

## 2026-09-27 — stage C, the plan

Written by the `planner` subagent against the approved spec.

**The run was interrupted by a session limit** after `plan.md` was written and
before `plan-evidence.md`. The agent was resumed with its context intact and
wrote the evidence file, then trimmed `plan.md` toward the contract budget.

**Three questions were put to the developer, all about how to build it rather
than what to build. All three were answered with the planner's recommendation**,
and are recorded as D1–D3 in plan §9:

* **D1** — M3's probe build writes **both** candidate routes, a different weapon
  on each, so one hardware cycle settles which one the game honours. An
  ambiguous failure is re-probed one write at a time.
* **D2** — the readiness summary is **left alone**. A world whose only setting
  is a ticked weapon still reads `0 OF 15 ON`, because that number is documented
  as a statement about toggles and this setting has none.
* **D3** — the 22 origin rows are **carved into a shared header**, changing no
  value and no byte of row 34's output, so the eventual narrowing stays a
  one-place edit.

**Reconciliation found nothing to change in §1–§7.** All three answers matched
the planner's own recommendations, so the body had already been written to them:
§4.5 already said the probe writes both routes, the carve-out already ran
through §4.4, §5 and §6, and D2 correctly appeared nowhere, since leaving the
summary alone means no instruction. The only edits were emptying §8, recording
§9, and the status line. The two `§8` references surviving in the body were
checked and point at the **spec's** §8, which exists.

**Contract budget.** §1–§7 came to 556 lines against a nominal 400. Measured
against this repo's own plans — caged dogs 419, startup screen 404, worlds 553,
randomizer settings UI 605 — that is mid-range for a feature of this size, and
it was accepted rather than trimmed further. A genre pass found §1–§7 to be
instructions, constraints and verification throughout.

Status ended at `QUESTIONS ANSWERED — awaiting developer approval`.

---

## 2026-09-27 — stage E, milestones 1-3

Built by the `implementer` subagent. The plan was approved the same day and
**stage D (plan review) was deliberately skipped at the developer's request**.

**All three milestones completed. No stop condition fired.** The run stopped at
the Required gate after M3, which is where the Execution Strategy puts it; M4
was not begun, because its content is selected by the probe's result.

**What was built.** M1 the derived 78-row table and its verifier; M2 the setting,
its picker and its config key; M3 the grant, the shared requirement writer, and
the dual-route probe build.

**Nine deviations, all recorded in plan §10.** Six are small and were taken
inside the contract's latitude. **Three are things the plan named that were not
done**, and they are listed here because a deviation that is an omission is the
kind that goes unnoticed:

1. `RandomizerDefaultsStore.cpp`'s `FormatSettings` comment still reads "Worst
   case today is 636 bytes". It was already wrong before this feature (635) and
   is now 737. §5 scopes that file to the config key alone.
2. `settings_ui_verify.py` case 9's picker alignment band still measures only the
   three creature tables — its regex keys on `"c\d+"`, which no weapon row
   matches. Evidence M7 measured the widest weapon row at 653px against the
   band's 791px, so nothing is at risk, but it is unmeasured *there*.
   `ui_scroll_verify.py` does cover the band.
3. The settings-UI spec's Appendix A help table has no row for the new setting.
   §5 scoped that edit to §7.1, and nothing pins help text against Appendix A.

**Decisions the contract left open, and how they were taken** — eight, all in the
implementation report. The two worth reading: the FMG's "1408 entries" turned out
to be 1408 *ids* of which 1245 carry text, and the exclusion count is **seven,
not the spec's six** (85 candidates, not 84), which evidence M3 had already
corrected. The verifier asserts 85/78 and *reports* the seven rather than
hardcoding a count.

**Verification, re-run independently by the orchestrating session** rather than
taken from the report: clean `make clean && make` from scratch with **zero errors
or warnings** and a 7,143,424-byte `.pkg`; `trick_weapons_verify` 50/50;
`pool_verify` 94/94; `settings_ui_verify` 113/113; `worlds_verify` 97/97;
`hunter_tools_verify` 19/19 and `starting_weapons_verify` 17/17, both unchanged,
which is what says rows 34 and 5 still produce what they did; `ui_scroll_verify`
and `font_atlas_verify` PASSED; `gen_weapon_table --check` PASS.

**Also confirmed independently:** every changed file is named in plan §5 — zero
unnamed files; `plan.md` §1-§7 is untouched at 487 lines, so only §10 was
written; the config figures land exactly where the plan predicted (686-byte
settings block, 737-byte worst-case `defaults.cfg`); the grant runs **last** in
`StepItemData` (line 1098, after starting weapons at 1052 and hunter tools at
1073), which is the invariant that keeps every other roll where it was.

**Not verified, and not verifiable here:** `trick_weapons_verify.py verify` and
its two `--granted` forms need a real granted output tree, which does not exist
until the app has run on the console. `data/runs/` holds only pre-feature runs.

**Awaiting hardware.** The probe is the gate: whether Bloodborne grants a weapon
written into `CharaInitParam.equip_Wep_Right` at all. M4 deletes the losing route
once the console answers.

---

## 2026-09-27 — the probe, on hardware. Route A works.

The Required gate after M3 is answered, and it is the good answer.

**What was tested.** Two rows ticked — `AMYGDALAN ARM` and `CHIKAGE` — everything
else off, a new character started.

**What happened.** The character spawned in the clinic **holding the Amygdalan
Arm**. So `CharaInitParam.equip_Wep_Right` **is** read at character creation and
route A is the live route.

That settles the central unknown this feature has carried since the spec: the
field is empty on all 1700 shipped rows and Bloodborne never uses it anywhere,
which is why nothing offline could establish that the engine still honours it.
It does.

**A second thing the test happened to prove.** Two rows were ticked and exactly
one weapon arrived, so the many-ticked-means-random path works — the draw, the
candidate list built from the selection, and the single-weapon result. A
one-ticked test would not have shown that.

Route B's outcome was not reported either way; it does not change M4, because
D5 prefers in-hand and route A delivered it.

**Consequence for M4:** delete route B and the probe constant, keep route A.

---

## 2026-09-27 — stage E, milestone 4. Plan 037 fully built.

Built by the `implementer` subagent once the gate was answered. **Completed; no
stop condition fired.** All four milestones of plan 037 are now implemented.

**The branch taken.** The probe put the weapon in the character's hand, which is
row 1 of plan §6's outcome table: keep route A, delete route B, and no wording
change, because nothing user-facing had promised the losing route — the help
already read "Start holding one of the trick weapons you tick here".

**Deleted:** `kProbeInventoryWeapon` (`22000000`), the inventory-slot write, two
result counters that could henceforth only report 0, the log line's probe
clause, and the mirror's route-B branch. **Kept:** route A entire — one
`WriteI32LE` of the drawn weapon into `equip_Wep_Right` per origin row, the
single last-in-run draw, and the `ReqOwner::Grant` 9/9/5/6 profile through the
shared writer. `CharaInitRows.h` was deliberately **not** touched: row 34 still
uses all four slot helpers.

**Seven deviations, recorded in plan §10.** The two worth reading:

* **The two route-shape verifier cases became four rather than being deleted.**
  They now assert that route A writes the drawn weapon, that there is exactly one
  `WriteI32LE`, that none of route B's five symbols survives, and that **no
  7-or-8-digit decimal literal** appears anywhere in the pass — chosen over
  naming the one deleted constant, so that a *different* re-introduced weapon id
  also fails.
* **`verify`'s G3 narrowing costs the combined-run case, accepted deliberately.**
  The deleted write and row 34's write are both item-slot writes, so "reject the
  deleted write" and "tolerate row 34" cannot both hold in one predicate.
  `trick_weapons_verify.py verify` now describes a grant-only tree; the combined
  direction is covered by `hunter_tools_verify.py verify … --granted <id>`.

**Verification, re-run independently by the orchestrating session:** clean
`make clean && make` from scratch, zero errors or warnings, 7,143,424-byte
`.pkg`; `trick_weapons_verify` **53/53** (was 50/50, −2 +5, no pre-existing case
changing state); `hunter_tools_verify` 19/19 and `starting_weapons_verify` 17/17,
both unchanged; `pool_verify` 94/94; `settings_ui_verify`, `ui_scroll_verify`,
`worlds_verify`, `font_atlas_verify` all passing; `gen_weapon_table --check`
PASS.

**Also confirmed independently:** none of route B's five symbols survives in the
grant, which now performs exactly one write, to `equip_Wep_Right` at offset 16;
`plan.md` §1-§7 is still 487 lines, untouched, with §10 grown to 16 rows; and
`log.md` was not edited by the implementer.

**Still not verifiable here:** `verify` and the two `--granted` forms need a real
granted output tree, and `data/runs/` still holds only pre-feature runs.

**The outstanding hardware question, and it is not a formality.** The Amygdalan
Arm arrived *in hand*, which is not the same as *wieldable*: as shipped it
requires arcane 15, and what makes it usable at level 4 is the requirement
rewrite to 9/9/5/6. If that rewrite silently failed, the weapon would still
appear in hand and would not swing. Nothing tested so far distinguishes those two
states. Spec §8's list is outstanding, and three of its checks fail silently:
that a Lost/Uncanny row grants *that version*, that `LOGARIUS' WHEEL` and
`KOS PARASITE` are usable without levelling, and that the granted weapon can
actually be swung and transformed.

---

## 2026-09-27 — hardware test passed. Row 37 is DONE.

The developer reports the feature tested and working on the console, beyond the
probe and the Amygdalan Arm swing already recorded above. The backlog row is
**DONE** — `CLAUDE.md` §3's bar, implemented and confirmed on hardware.

Nothing in the code changed for this entry; it records the status transition and
the fact that the console, not a verifier, is what moved it.
