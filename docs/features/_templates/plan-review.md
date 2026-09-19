# Plan Review NNN — <Name>

**Verdict: CHANGES REQUESTED**

**Implementation handoff: NEEDS RESTRUCTURING**

**Plan:** `docs/features/NNN-<slug>/plan.md`

**Evidence:** `docs/features/NNN-<slug>/plan-evidence.md`

**Spec:** `docs/features/NNN-<slug>/spec.md`

**Reviewed:** YYYY-MM-DD

---

## 1. Summary

Briefly explain the verdict and the main reasons for it.

A reader should be able to understand the overall result without reading the full review.

---

## 2. Findings

List problems or gaps in the plan, starting with the most important.

For each finding, include:

### Finding 1 — <Short description>

**Severity:** Blocking / Should fix / Worth considering

**Where:** Plan section, file, or code location

**Problem:** What is wrong or missing.

**Why it matters:** What could happen if it is left as-is.

**Recommendation:** What should be changed or investigated.

**Confidence:** High / Medium / Low, with an explanation when uncertain.

### Finding 2 — <Short description>

...

### Severity

**Blocking** — The plan could produce incorrect behaviour, damage data or saves, break existing functionality, or cannot be implemented as written.

**Should fix** — The plan has a meaningful gap that could cause wasted work, a failed test, or an incomplete implementation.

**Worth considering** — A useful improvement that does not create a significant problem if left unchanged.

Focus on findings that have a real consequence. Avoid style preferences or alternative approaches that do not materially improve the result.

---

## 3. What looks good

Identify the important parts of the plan that are well supported.

Call out decisions that were investigated thoroughly, reuse of existing code that makes sense, and verification that adequately covers the feature.

---

## 4. What could not be verified

List anything that could not be confirmed during the review.

Examples:

* Behaviour that requires PS4 testing
* Measurements that could not be reproduced
* Game data that needs further investigation
* Repository areas that were not examined

---

## 5. Numbers and evidence

Check important measurements and factual claims in the plan.

| Claim   | Plan says | Verified value | Result    |
| ------- | --------: | -------------: | --------- |
| Example |       100 |            100 | Confirmed |

If a number cannot be reproduced or appears incorrect, make it a finding in Section 2.

---

## 6. Implementation handoff

The plan can be correct and still be unusable by an implementer. This section
judges the contract (`plan.md` §1--§7) as a working document, separately from
whether its design is right.

Answer each in one or two sentences.

| Dimension | Question | Assessment |
| --------- | -------- | ---------- |
| **Scope** | Is the change bounded to one coherent pass, with unrelated improvements explicitly excluded in §3.2? | |
| **Sequence** | Are the §7 changes ordered, and is each step independently understandable? | |
| **Decisions** | Is every behaviour-affecting decision settled, with none left hiding inside an implementation instruction? | |
| **Dependencies** | Are prerequisite changes and their ordering identified? | |
| **Verification** | Can the implementer tell whether each milestone succeeded, from §6 and the §7 completion gates alone? | |
| **Separation** | Is the contract free of investigation, review history and superseded reasoning -- is that material in `plan-evidence.md` and `log.md` instead? | |
| **Handoff** | Could a fresh implementation agent execute §1--§7 without the planning conversation, and without reading the evidence file? | |

### Conclusion

**Implementation handoff: READY** or **NEEDS RESTRUCTURING**.

`NEEDS RESTRUCTURING` is not a comment on how much the plan knows. It means the
implementation-facing part is too large, out of order, mixed with material that
is not an instruction, or missing something the implementer needs. Say which.

### Restructuring findings are moves, not additions

A finding in this section must name **what to move where**, or **what to cut**.
It must never ask the plan to add an explanation, a justification, or a record
of this review -- that is the failure mode this section exists to catch, and a
reviewer who asks for it causes the defect they were checking for.

State each one in that form:

* Move `plan.md` §N's ... into `plan-evidence.md` §E4 -- it is a measurement, not an instruction
* Cut `plan.md` §N's account of ... -- `log.md` already carries it
* §7 milestone 2 does too much; split it at ... so each half builds and verifies

If the contract is missing something the implementer genuinely needs, say what
and where it goes -- and keep it to what changes the implementer's actions.

---

<!--

This review evaluates the plan. It does not rewrite or modify it.

The planner addresses findings and updates the plan separately.

-->
