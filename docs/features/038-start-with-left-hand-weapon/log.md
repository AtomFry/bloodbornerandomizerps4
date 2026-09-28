# Feature 038 — log

Append-only. `implementation-report.md` states what was built; this states how
it got here.

---

## 2026-09-27 — stages B, C and D deliberately skipped

The developer asked for row 38 to be implemented directly from its backlog row.
No spec, no plan, no plan review. The convention for such items is recorded in
`docs/features/README.md`: the folder holds an `implementation-report.md`
written after the fact and nothing else, and says so at the top, because a
folder that merely looks thin is indistinguishable from one where the stages
were forgotten.

**What made that defensible here, and it would not generalise.** Row 38 is row
37's mirror — the same grant into `equip_Wep_Left` at offset 24 instead of
`equip_Wep_Right` at 16, in the same 22 origin rows, through the same requirement
writer, picker, config encoding and engine pass. Row 37 had just been
hardware-proven, so the one thing a spec and plan existed to de-risk — whether
the game reads that row family at all — was already answered. Almost every design
decision was inherited rather than taken.

**Two decisions were put to the developer before any code was written**, because
neither was inherited:

* **Commit row 37 first**, so the two features' diffs stay separable. Done —
  `8b194ce`.
* **All 14 left-hand weapons are in scope**, not firearms only. The measured set
  is 11 firearms plus Loch Shield, Hunter's Torch and Fist of Gratia; excluding
  the last three would have meant a hand-maintained exclusion list contradicting
  what the data says.

## 2026-09-27 — stage E, built in one pass

**14 rows, derived not authored.** 18 tier-0 rows carry `leftHandEquipable`; 16
are named; 14 are obtainable. The same obtainability rule row 37 uses — present
in a shop or an item lot — produces exactly those 14 and correctly drops Wooden
Shield and Torch, which are named but obtainable nowhere.

**Flat, unlike row 37.** No firearm has an Uncanny or Lost version, so there is
no version dimension and no equivalent of row 37's D2. Asserted rather than
assumed.

**The one thing the brief did not anticipate.** Row 37's two membership tests
agree, so `gen_weapon_table.py` treats disagreement as proof the derivation is
broken and refuses to write. Row 38's disagree — 16 named against 14 obtainable —
so the left-hand rule is a conjunction with `obtainable ⊆ named` asserted, stated
in its own generator rather than by loosening row 37's guard into a flag. That is
the main reason the implementation sits beside row 37's files rather than inside
them.

**Decisions taken during implementation**, all in the report: the label had to
become `START WITH A LEFT WEAPON` because `...LEFT-HAND WEAPON` measures 731px
against a 700px pane budget; **Loch Shield has no upgrade tiers**, the only such
row in either table, so row 37's "all ten tiers" assertion was deliberately not
copied; and tolerance flags were added to three existing verifiers so a combined
hardware run stays verifiable.

**Verification, re-run independently by the orchestrating session:** clean
`make clean && make`, zero errors or warnings, 7,143,424-byte `.pkg`;
`left_hand_weapons_verify` 60/60; `trick_weapons_verify` 56/56;
`hunter_tools_verify` 25/25; `starting_weapons_verify` 21/21; `pool_verify`
95/95; `settings_ui_verify`, `ui_scroll_verify` and `worlds_verify` all passing.
Row 37's output is unchanged — `TrickWeaponTable.h` byte-identical, no
pre-existing case in any tool changing state.

## 2026-09-27 — hardware test passed. Row 38 is DONE.

The developer reports the feature tested and working on the console. The backlog
row is **DONE**.

**Left open, and recorded so it is not lost:** the starting Quicksilver Bullet
count was not changed, which matters because 11 of the 14 rows are firearms and a
granted gun with no bullets is a weapon you cannot fire. Nobody has decided
whether that should change.
