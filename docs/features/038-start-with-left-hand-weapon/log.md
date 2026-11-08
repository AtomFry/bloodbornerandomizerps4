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

---

## 2026-09-29 — the table was wrong, and is now 16 rows

Found by the `implementer` while building row 40, which stopped on a stop
condition rather than proceeding. **Row 38 shipped with two rows missing**, and
the cause was a constant in a file it only borrows.

`gen_left_hand_table.py` imports `obtainable_ids()` from `gen_weapon_table.py`,
where `LOT_CATEGORY_WEAPON` read **1**. Measured: `ItemLotParam` category **0** is
weapons (63 weapon ids against 3 protector) and category **1** is armour (99
against 2). So the lot half of the obtainability test was searching armour lots.

**Row 37 was unaffected and row 38 was not**, for a reason worth keeping: for the
right hand, obtainability is a cross-check and all 78 rows are in a shop listing,
so the OR never needed the lot half. For the left hand **obtainability GOVERNS** —
that is the whole reason `gen_left_hand_table.py` exists — and the **Torch**
(20100000) and the **Wooden Shield** (19000000) are in no shop at all. With the
wrong category they failed both halves and were excluded. Both are ordinary
left-hand items a player picks up in vanilla.

**What changed.** `LOT_CATEGORY_WEAPON` 1 → 0; `LeftHandWeaponTable.h`
regenerated at 16 rows; `left_hand_weapons_verify.py` `EXPECTED_ROWS` 14 → 16 and
`EXPECTED_TIERLESS` 1 → 3 (both new rows are tierless, and neither is upgradeable
in vanilla); both generators' docstrings corrected. `TrickWeaponTable.h` is
byte-identical either way, which `gen_weapon_table.py --check` asserts.

**One case became a stronger claim rather than a looser one.** L4 used to assert
that the name and obtainability tests **disagree** and that every disagreement is
excluded, because requiring agreement would have been false about the data. After
the correction `named - obtainable` is **empty**: every named candidate is
obtainable. The case now asserts the agreement, and the two excluded ids
(6180000, 20090000) are excluded on the name test alone.

**A saved selection consequence, accepted by the developer.** The
`left_hand_weapons_included` line is one character per row, so it goes 14 → 16 and
the length guard rejects a stored 14-character line, reading it as nothing ticked.
That is the fail-safe direction and the designed behaviour for a changed count.
The config key was deliberately **not** renamed.

**And a third verifier hole, of the same family as the two this feature already
turned up.** `pool_verify.py` computed the config block from **hardcoded** picker
row counts — `+ 14` for the left hand among them — so the table growing to 16 did
not move its arithmetic and its exact-equality assertion went on passing against a
stale figure. That is precisely the failure the assertion exists to prevent. The
five picker lengths are now **read from the generated headers**
(`std::array<ModelPoolEntry, N>`), so they cannot drift again, and the totals are
rebased to **1040 / 1091**. Verified: the derived counts are 17 / 82 / 85 / 78 / 16,
matching the headers exactly.

**Verification.** Clean `make`, zero errors or warnings, `.pkg` produced.
`left_hand_weapons_verify` **60/60**, `trick_weapons_verify` 56/56,
`pool_verify` 108/108, `settings_ui_verify` 113/113, `insert_bosses_verify` 56/56,
`worlds_verify` 29/29, `hunter_tools_verify` 29/29, `starting_weapons_verify`
45/45, `team_type_verify` 20/20, `drops_verify` 16/16, `ui_scroll_verify` and
`font_atlas_verify` passing, and all three generators `--check` PASS.

**Worth one hardware confirmation when convenient**, though it is not a gate: row
38 is hardware-proven at 14 rows, and the two new rows have never been granted.
Ticking **TORCH** or **WOODEN SHIELD** and starting a character would confirm the
two additions behave like the other fourteen.
