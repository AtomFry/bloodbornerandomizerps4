# Feature NNN — <Name>

**Status: DRAFT**

**Reference:** `reference/.../SomeFile.cs` — or **New feature**

**Plan:** `docs/features/NNN-<slug>/plan.md` — added after the spec is approved

---

## 1. What does this feature do?

Explain the feature in plain language.

Someone familiar with Bloodborne but unfamiliar with the code should be able to understand what the feature changes and why it exists.

---

## 2. What does the player experience?

Describe what the player sees or experiences — one row per setting, in the game's own vocabulary, in terms a player could confirm by looking at the game.

| Setting | Off | On |
| ------- | --- | -- |
| **NAME** | What happens today | What happens instead |

State outcomes, not mechanism. No param IDs, model IDs, map filenames, function names or field names belong in this section.

Where a setting removes or changes some of a group, **say how many are left**, not only how many change. That number is a fact to measure, not to infer.

Include anything important about when the feature takes effect, what happens during a run, and what the player gets when this setting is combined with a related one.

---

## 3. What does the existing randomizer do?

Describe how the reference version handles this feature.

Include the relevant file, method, or game data.

If the reference has unusual, buggy, or disabled behaviour, document it here.

If this is a new feature with no reference implementation, say so.

---

## 4. What do we know?

Record the evidence for the claims made in §1 and §2, so a reader can check them.

Examples:

* How many things a setting affects, and **how many it leaves alone**
* What the affected creatures, items or places actually are
* Relevant maps or enemies, named as the game names them
* Pool sizes
* Important exclusions
* Existing randomizer behaviour the player would notice

Mechanism belongs here only where it explains a player-visible outcome — "the rule matches every model in the family, which is why none are left" belongs; the value it writes does not. Evidence that supports a decision in the plan rather than a claim in §2 belongs in the appendix.

Label every claim as a fact with its source, an inference with its reasoning, or an assumption with a way to verify it.

---

## 5. Terminology

Define project-specific terms that could cause confusion.

If there is nothing worth defining, write:

*No additional terminology needed.*

---

## 6. Scope

### In scope

What this feature includes.

### Out of scope

What this feature deliberately does not include.

Mention related features that should be handled separately when that helps prevent the work from growing beyond this feature.

Chalice dungeons remain out of scope.

---

## 7. Constraints and decisions

List the things that must be respected while implementing the feature.

Examples:

* Save safety
* AFR structure
* Reference behaviour
* UI limitations
* Known game-data restrictions
* Features that must remain unchanged

Record important decisions here once they have been made.

---

## 8. How will we know it works?

Describe what must be observably true for the feature to be correct — acceptance criteria, not test design. Which verifier asserts it, and how, belongs to the stages that own building and testing.

### Automated testing

Describe what can be verified without running the game.

Mention an existing test or tool if one already covers the feature.

If a new test is needed, say what it needs to prove.

### Hardware testing

Describe what needs to be checked on the PS4.

Include the expected result and the most important failure cases.

---

## 9. Open questions

List decisions that still need to be made.

For each question, include:

* The question
* What we currently know
* A suggested answer, when there is a reasonable one
* Whether the decision affects the implementation enough that we should resolve it before planning

Remove questions from this section once they are answered.

---

## 10. Decisions

Record decisions made while developing the spec.

| Date       | Decision |
| ---------- | -------- |
| YYYY-MM-DD | ...      |

These decisions become part of the agreed behaviour for the feature.

---

## Appendix — research notes for stage C

*Not part of the specification, and not covered by the developer's approval. These are pointers from the spec investigation to save stage C a search. Verify anything here before relying on it.*

Pointers, not conclusions: file and symbol and one line on why it matters, never the values found. Keep it short — if it runs past a page, it is a plan, which is a different stage.

**Where the behaviour lives**

* `path/to/file` — symbol, and why it matters

**Worth checking early**

* Things that are not obvious from the code and would cost stage C a search

**Dead ends already walked**

* Places that looked promising and were not, so nobody repeats them

---

<!--

A spec describes WHAT the feature should do and WHAT must be true.

The implementation details belong in the plan:

- Files and classes to change
- Code structure
- Implementation approach
- Detailed testing approach
- Step-by-step work

Keep those decisions in the plan so the implementation can be worked out after the behaviour is understood.

-->
