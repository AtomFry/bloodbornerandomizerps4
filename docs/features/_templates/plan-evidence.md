# Plan Evidence NNN — <Name>

**Plan:** `docs/features/NNN-<slug>/plan.md`

**Spec:** `docs/features/NNN-<slug>/spec.md`

---

> **This document is the investigation behind the plan, not instructions.**
>
> The implementer does not read it to start work. They consult it when an
> implementation question needs context the contract deliberately left out —
> "why this number", "was the other approach considered", "how was that
> measured".
>
> It is **revisable**, unlike `log.md`. When a measurement is corrected, correct
> it here and say what it replaces; the chronology of who found the error lives
> in `log.md`.
>
> There is no length budget. Depth here is the point.

---

## E1. Reference trace

What the Windows tool in `reference/` actually does: the functions, the call
sites, the fields written, and where the behaviour sits relative to other
passes. Ordering between passes is behaviour, not preference.

Cite file and line. Say where the trace confirmed the spec and where it did not.

---

## E2. What exists in the port

The full survey of neighbouring code, settings chains, shared helpers, generated
tables, verification tools, and prior plans.

For anything that could be reused with a small change, record whether it should
be reused, extended, or left alone, and why. The plan's §4 carries only the
conclusion.

---

## E3. Alternatives considered

| Approach | Why rejected |
| -------- | ------------ |
| ... | ... |

Include approaches that look obvious and are wrong — an unrecorded rejected
alternative gets re-proposed at every later stage.

---

## E4. Measurements

Every number the plan depends on, with how it was obtained and against what.

| Quantity | Value | How measured | Source |
| -------- | ----: | ------------ | ------ |
| ... | ... | `python app/tools/....py ...` | `data/vanilla/dvdroot_ps4` |

Measured numbers are the house style. A number nobody can reproduce is a defect,
so record the command, not just the result.

Where a measurement was corrected, show the corrected value and note what it
replaces and why the first one was wrong.

---

## E5. Risk analysis

The full reasoning behind each hazard listed in the plan's §3.3, including the
ones judged low enough not to change the implementation.

### E5.1 <Hazard>

What could happen, how likely, what bounds it, and what the plan does about it.

---

## E6. What the spec's appendix claimed

If the spec carried research notes for stage C, record what they claimed and
what the trace found. The appendix is a lead, not a finding.

---

## E7. Anything that could not be established

What was not measurable without the console, what was left unverified, and where
the plan is resting on an assumption rather than a measurement.

---

<!--

Nothing in this file is an instruction. If something here tells the implementer
what to do, it belongs in plan.md.

Nothing in this file is process history. If something here records who said what
and when, it belongs in log.md.

-->
