# Plan Evidence — Randomizer Settings UI

**Plan:** `docs/features/randomizer-settings-ui/plan.md`

**Spec:** `docs/features/randomizer-settings-ui/spec.md`

---

> **This document is the investigation behind the plan, not instructions.** It is
> revisable. There is no length budget.

---

## E1. Reference trace

The spec says there is no reference-tool equivalent. That is correct in shape and
wrong in one useful detail.

**Shape — confirmed.** `reference/Randomizer/MainWindowComponents/MainWindow.xaml`
is 221 lines holding **51 `CheckBox` elements**, every one absolutely positioned
with a `Margin="x,y,0,0"` on a single `Grid`. There is no `TabControl`, no
`TabItem` and no `GroupBox` anywhere in the file. Settings are grouped only by
which x-margin column they sit in (10, 110, 189, 290). There is nothing here for a
controller-driven screen to borrow, and nothing that constrains the ordering of
this port's passes — this feature touches no pass.

```
$ grep -c "CheckBox" reference/Randomizer/MainWindowComponents/MainWindow.xaml
51
$ grep -c "TabControl\|TabItem\|GroupBox" reference/Randomizer/MainWindowComponents/MainWindow.xaml
0
```

**The detail the spec did not record: the reference tool already has per-setting
help text.** Every checkbox carries a `ToolTip` attribute, and several of spec
Appendix A's drafts are recognisably paraphrases of them:

| Reference `Content` | Reference `ToolTip` |
| ------------------- | ------------------- |
| Randomize Enemies | Randomize enemies in all maps, excluding chalice maps. |
| Randomize Bosses | Randomize bosses in all maps, excluding chalice maps. |
| Enemies | Customize the enemies that will appear in all maps. |
| Bosses | Customize the bosses that will appear in all maps. |
| Randomize Workshop Tools | Randomize the workshop tools in the normal maps. Blood gem tool and rune tool. |
| Randomize Enemy Drops | Randomize items that drop from enemies. |
| Unchanged Bell Maidens | Keep Bell Maidens from being changed, keeping the generators working. |

Extracted with:

```
python - <<'EOF'
import re
t = open('reference/Randomizer/MainWindowComponents/MainWindow.xaml', encoding='utf-8').read()
for m in re.finditer(r'<CheckBox ToolTip="([^"]*)"[^>]*?Content="([^"]*)"', t):
    print("%-44s | %s" % (m.group(2).strip(), m.group(1)))
EOF
```

This **supports** spec Appendix A rather than contradicting it: the reference
tooltips are terser, say nothing about defaults, and carry the chalice caveats
that are out of scope here. Appendix A is the better text and is what the plan
wires in. The finding matters only as confirmation that per-setting help is
reference behaviour, not an invention — which is relevant because `CLAUDE.md` §7
makes reference behaviour the default.

**Where the trace contradicts the spec:** nowhere. The spec's §2 acceptance
target is a presentation change with byte-identical output, and nothing in the
reference bears on it.

---

## E2. What exists in the port

### E2.1 The row-index problem, measured

`EnableWizardScreen.cpp` lines 22–69 declare 21 `const int k*Row` constants plus
`kSaveDataRowCount`. `SetupDefaultsScreen.h` lines 38–91 declare the same 21 as
`static const int`. Each set is shadowed by an `items` vector — three in total:
`DrawSaveData` (line 840), `DrawConfirm` (line 956), `DrawList` — whose element
positions must equal the constants.

The source comments state the hazard explicitly and repeatedly: *"These indices
are also the POSITION of each entry … an entry added to either list out of order
compiles, passes `ui_scroll_verify.py` and mislabels every row below it."* Three
successive features (032, 033, 018) each appended last for exactly this reason,
and each recorded that it deliberately accepted a worse row placement to avoid
renumbering.

The duplication cost, counted: `UpdateSaveData` runs from line 199 to line 388 —
**190 lines** — of which lines 204–288 are the Left/Right chain (16 branches) and
lines 290–377 the `X` chain (20 branches, 16 of them the same toggles written a
second time). The spec §4.2 estimate of "roughly 180 lines for 16 toggles" is
confirmed.

Spec §10 (9.2) deletes the second chain outright, and the descriptor table
collapses the first to one `AdjustSetting` call. What remains is a `switch` on
`SettingKind` for the three drill-ins.

### E2.2 Navigation state

`GoToStep()` (line 153) is three lines: set `step_`, `selected_ = 0`,
`scrollOffset_ = 0`. Every transition goes through it, which is why
`ReturnFromPicker(row)` (line 453) exists — it calls `GoToStep(Step::SaveData)`
and then immediately undoes both resets. `UpdateEditSeed` does the same thing by
hand twice (lines 438–439, 444–445). `UpdateConfirm`'s `O` path (line 500) does
not, which is the position loss spec §3 describes.

So B8 is not implemented by adding restoration; it is implemented by **deleting
the reset**. Once each step owns its own cursor, `ReturnFromPicker` and the two
hand-written restores in `UpdateEditSeed` all become dead code.

`Screen.h` states the flat `ScreenId` enum is deliberate; `ScreenManager` holds
exactly one screen and `Application.cpp` line 74 replaces it wholesale on a
transition (`screens.SetScreen(MakeScreen(requested, defaults))`). Converting a
step into a `ScreenId` would therefore destroy every per-run toggle. Confirmed by
reading the switch in `MakeScreen`: every branch is a fresh `make_unique`.

### E2.3 `ModelPicker` as the overlay candidate

`ModelPicker::Draw` (ModelPicker.cpp line 126) opens with
`renderer.Clear(20, 24, 28)`; `DrawConfirm` (line 182) draws over that cleared
screen. Its header comment states the reason plainly:

> *A full-screen prompt rather than a panel over the list: Renderer has no
> filled-rectangle call (Clear, DrawText, TextWidth is the whole API), and drawing
> over the list without one would leave rows showing through the text.*

That comment is the exact gap this feature closes, and it identifies the
select-all/none prompt as the cheapest existing place to prove the primitive.

Call sites that must gain their own `Clear`: `EnableWizardScreen::DrawEnemyPicker`
/ `DrawSkipPicker` / `DrawBossPicker` (lines 935, 940, 945) and the three branches
of `SetupDefaultsScreen::Draw`. Six sites, one `Clear` each.

`ModelPicker` is already parameterised by `PickerStrings` and takes its table,
count and flag array from the host, so the settings model can supply all four
from a `SettingDef` with no change to the component's interface.

### E2.4 The settings chain, and why one struct serves both screens

`SetupDefaultsScreen` already holds `RandomizerDefaults working_` and edits it
directly (`SetupDefaultsScreen.h` line 106). `EnableWizardScreen` holds the same
information shredded into 15 `bool` members, three selection members, a
`std::string titleId_` and a `uint32_t seed_` (EnableWizardScreen.h lines 99–130),
copied field by field in a 24-line constructor initialiser list.

So the "one model, two screens" requirement of spec Appendix B is satisfied by
making the wizard hold what Setup Defaults already holds. What differs between the
hosts is then not the *shape* of the data but what happens at exit:

| | Enable wizard | Setup Defaults |
| | ------------- | -------------- |
| Edits | a per-run copy | a working copy of the persisted defaults |
| `OPTIONS` | commits a run (Confirm only) | `defaults_ = working_; SaveRandomizerDefaults(...)` |
| Writes back | **`lastSeed` only** (`StartCommit` lines 577–578) | everything |
| Extra item | the seed | `BLOODBORNE TITLE ID` |

"Editable here but not there" therefore needs no expression in the descriptor
table at all — every one of the 18 settings is editable on both screens, and the
two host-specific items (seed, title ID) occupy the same place in each: rail row 0
(§9 D1). This is simpler than spec Appendix B anticipated.

The risk this creates is one line long: `defaults_ = run_` in the wizard would
silently persist every per-run toggle and break spec §4.7. Hence the §3.1
invariant.

### E2.5 Reusable drawing pieces

`Controls.h` exposes `ListLayout`, `VisibleRowCount`, `ClampScroll`,
`ScrollToShow`, `NavigateVertical`, `DrawScrollHints`, `DrawScrollableList`,
`DrawCenteredLabel`, `DrawMenuList` and `Palette`. The spec's claim (§4.4) that
the scroll windowing maths is layout-agnostic is confirmed by reading
`ClampScroll` and `ScrollToShow`: neither takes a pixel.

What is **not** reusable is every drawing entry point. `DrawCenteredLabel`
computes `x = (kScreenWidth - width) / 2` and `DrawScrollableList` and
`DrawScrollHints` both call it, so all three centre on the whole screen, not on a
pane. Three panes need left-aligned and right-aligned draws and a pane-local hint.
Hence the three additions in plan §4.

`Renderer` is exactly `Init`, `Shutdown`, `Clear`, `Present`, `DrawText`,
`TextWidth` (Renderer.h). Spec §4.3 confirmed verbatim.

`FontAtlas.h` already exposes `FontAtlasLineHeight(scale)` — *"Screens that space
rows by hand can use this instead of assuming 8 * scale"* — and **no screen calls
it**, because `Renderer` does not forward it. That is the whole of
`Renderer::LineHeight`.

### E2.6 Verifier coupling, checked before planning any rename

`pool_verify.py` reads UI and randomizer source in three places. These are read
off the file, not inferred:

* **line 533** — `named_strings(UI_SRC/"EnableWizardScreen.cpp")`, matching
  `const char\* const (\w+)\s*=\s*"…";`. It requires `kEnemyFailPrefix`,
  `kPoolFellBackLine1`, `kPoolFellBackLine2` and `kNothingRandomizedLine` to exist
  under those names, in that file, in that form.
* **lines 661–663** — `ModelPicker.h`, split on the literal
  `kEnemiesSkippedStrings` and then on the next `};`, taking every quoted string in
  between. Exactly seven are expected, and `"YES" not in skip_words` is asserted. A
  comment containing a quoted string inside that block would break the count.
* **lines 686–706** — `RandomizerDefaultsStore.cpp`, for `char buf[(\d+)]`, for
  the presence of key names, and for the worst-case config size. Line 686 begins
  the sum with `len("backup_existing_save=1") + 1`, and line 706 asserts
  `worst == 669` as an **exact equality**.

The spec's §4.5 warning is therefore accurate but **understated**: the coupling is
not only to string literals in `EnableWizardScreen.cpp`. It also binds
`ModelPicker.h` and `RandomizerDefaultsStore.cpp`, and two cases in the same
selftest are source-text assertions that milestone 1 must move (§E4 M6).

Nothing in `pool_verify.py` or `ui_scroll_verify.py` references the two save-data
rows by label, so the row deletions themselves break no case directly — only the
config-size arithmetic does.

---

## E3. Alternatives considered

| Approach | Why rejected |
| -------- | ------------ |
| Keep integer row indices and add a `category` array beside them | This is the parallel-structure problem with a third parallel structure added. It fails the moment a category gains a setting anywhere but last, which is the operation the feature exists to allow |
| A `std::function`/lambda getter-setter per setting | Costs a heap allocation and an indirect call per row on a CPU rasteriser drawing three panes a frame, and makes the table non-`constexpr`. Pointer-to-member is one deref and the table stays in `.rodata` |
| A virtual `Setting` base class with one subclass per setting | 18 classes and 18 vtables to express one bool. The project's house style is static tables (`EnemyPoolTable.h`, `CagedDogList.h`, `NpcScalingTable.h`) |
| Put the model under `app/src/Randomizer/` | Categories, rail labels, help text and `PickerStrings` are presentation. Putting them in the randomizer core inverts the layering rule in `CLAUDE.md` §6 and would make the engine depend on `ModelPicker.h` |
| Give each host its own descriptor table | Reintroduces exactly the duplication being removed; the two tables would differ only in the two host items, which are not settings |
| Express "editable here but not there" as a per-entry flag | Measured against the code, no setting is editable on one screen and not the other (§E2.4). A flag with only one value is a flag that gets set wrong |
| Make the seed a seventh rail category | Spec §10 keeps the seed out of the categories. It is a rail *row*, above the categories and separated from them by a rule, exactly as `FINISH` is below them |
| Make the header band itself the first focus position, with the seed edited there | Proposed by this plan and **rejected by the developer** (§9 D1). A focus target that lives outside the column the cursor otherwise walks is a second navigation idiom for one item; a rail row is the same idiom as everything else. The cost is 60 px of rail width, since the rail row must then carry `BLOODBORNE TITLE ID   CUSA03173` |
| Put the seed row's value only in the header, leaving the rail row reading `SEED` | Would have kept the rail at 520 px, but Left/Right feedback would then happen 100 px above the cursor. The value goes on the row; the header keeps its own copy so the seed stays visible from any category, which is what "persistent header" means |
| Convert the wizard's steps into `ScreenId`s now that there are more of them | `Application.cpp` rebuilds the screen on every switch; the wizard's in-progress state would be destroyed. Spec §3 and §7.3 forbid it, and `Screen.h`'s own comment explains why the flat enum is deliberate |
| Implement the dim with `SDL_SetTextureBlendMode` on a stretched 1×1 texture, reusing the exact calls `FontAtlas` makes | Plausible, and the fallback if the draw-blend path fails — but it needs a texture created, owned and destroyed in `Renderer`, and a scaled blended blit is a *different* SDL software path from an unscaled one, so it is not the free ride it looks like. `SDL_SetRenderDrawBlendMode` is three lines and no state |
| Add `SDL_RenderDrawLine` for separators | A second unproven SDL entry point for something a 2 px `FillRect` already does. `docs/ps4-homebrew-findings.md` §7's "link fewer, call fewer" logic applies |
| Keep `kHintGap` a single constant and adjust it | No single value satisfies every screen under the corrected model — see §E4 M2. The constraint is per-screen because the item scale and the surrounding furniture differ per screen |
| Weaken the geometry model to caps-only ink so today's constants pass | The atlas covers printable ASCII and the help text is the first mixed-case prose in the app. A caps-only model would pass today and be wrong the first time a descender is drawn |
| Do the categorised screen first and the save-data removal later | The removal renumbers 19 constants across three `items` vectors. Doing it while those vectors still exist is one mechanical pass; doing it after would be two |
| One milestone for the whole categorised UI | A generated model, a new Renderer API, an overlay change, a focus stack and two screen rewrites is not one implementation pass, and only the last of them is testable |

---

## E4. Measurements

All measurements are reproducible from the repository as it stands on
2026-09-20. The atlas helper referred to below is:

```python
# atlas.py - parses the baked metrics out of the generated header
import re
src = open('app/src/Platform/FontAtlasData.h', encoding='utf-8').read()
sizes = re.search(r'const AtlasSize kSizes\[4\] = \{(.*?)\n\};', src, re.S).group(1)
META, ADV = {}, {}
for r in re.findall(r'\{([^}]*)\}', sizes):
    f = [x.strip() for x in r.split(',')]
    META[int(f[0])] = dict(ascent=int(f[2]), descent=int(f[3]), glyphs=f[6])
for sc, m in META.items():
    body = re.search(r'const AtlasGlyph %s\[\d+\] = \{(.*?)\n\};' % m['glyphs'], src, re.S).group(1)
    a, idx = {}, 32
    for line in body.split('\n'):
        mm = re.match(r'\s*\{([-0-9,]+)\},', line)
        if not mm: continue
        v = [int(x) for x in mm.group(1).split(',')]
        a[chr(idx)] = v          # dataOffset, atlasX, atlasY, w, h, bearingX, bearingY, advance
        idx += 1
    ADV[sc] = a
def width(s, scale): return sum(ADV[scale][c][7] for c in s if c in ADV[scale])
```

### M1 — the atlas's real vertical metrics

| Scale | ascent | descent | line box | ink top offset | ink bottom offset |
| ----: | -----: | ------: | -------: | -------------: | ----------------: |
| 3 | 34 | 10 | 44 | 9 | 44 |
| 4 | 45 | 14 | 59 | 14 | 58 |
| 5 | 56 | 17 | 73 | 16 | 73 |
| 6 | 67 | 20 | 87 | 20 | 86 |

`ink top offset = ascent - max(bearingY)` and
`ink bottom offset = ascent + max(height - bearingY)`, taken over all 95 glyphs
with a non-zero box. Since `DrawText` places the *top-left* at `y` and rides the
baseline at `y + ascent`, no glyph can draw above `y + inkTop` or below
`y + inkBottom`.

The line box (44/59/73/87) matches spec §4.6 exactly. The ink box is what the
corrected verifier should use, because it is tight at the bottom (within 1 px of
the line box) and 9–20 px looser at the top, which is where most of the
near-collisions on today's screens sit.

```
$ python - <<'EOF'   # uses atlas.py's parsing, extended to bearingY/height
... prints: scale 3 ascent 34 descent 10 lineH 44 maxBearingY 25 max(h-by) 10
            scale 4 ascent 45 descent 14 lineH 59 maxBearingY 31 max(h-by) 13
            scale 5 ascent 56 descent 17 lineH 73 maxBearingY 40 max(h-by) 17
            scale 6 ascent 67 descent 20 lineH 87 maxBearingY 47 max(h-by) 19
EOF
```

Caps-only (`A-Z 0-9 space - , ' ( )`) the bottom offsets are 43/56/70/83 — which
is why today's screens look acceptable while failing the honest model by a few
pixels. Every shipped string is uppercase; the help text will not be.

### M2 — what the corrected model does to today's geometry

Replaying each screen's real constants under the ink box, with the hint gap as a
free variable, the constraints are:

* `MORE ABOVE` ink bottom ≤ first row ink top → `gap ≥ inkBottom(3) - inkTop(item)`
* `MORE ABOVE` ink top ≥ heading ink bottom → `gap ≤ firstY + inkTop(3) - headBottom`
* `MORE BELOW` ink top ≥ last row ink bottom → `gap ≥ inkBottom(item) - inkTop(3)`
* `MORE BELOW` ink bottom ≤ footer ink top → `gap ≤ footerY + inkTop(3) - lastRowY - inkBottom(3)`

Results at the shipped `kHintGap = 46`:

| Screen | Verdict |
| ------ | ------- |
| Setup Defaults | `MORE BELOW` ink top 895 sits 3 px above the last row's ink bottom 898 |
| Wizard SaveData / Confirm | `MORE BELOW` ink top 925 vs last row ink bottom 928, **and** ink bottom 960 vs footer ink top 959 |
| Progress log | clear |
| Enemy / Boss / Skipped pickers | clear |

Sweeping a single global gap over 46–64 finds **no value that satisfies all seven
screens**: raising it fixes the wizard's overlap with its last row and pushes it
further into the footer; lowering it does the reverse. The constraint is per-screen
because the item scale (4 vs 3) and the furniture above and below differ. Hence
`ListLayout::hintGap`.

Solving per screen, with the wizard's footer left where it is, `kSettingsLayout`
has an **empty** feasible set: it needs `gap ≥ 57` to clear its own last row and
`gap ≤ 45` to clear the footer. The two ways out are shrinking the window
(`bottomLimit 870 → 780`, 6 visible rows → 5) or moving the footer. Moving the two
footer lines from `kScreenHeight - 130 / - 80` to `- 100 / - 50` gives
`gap ≤ 90`, keeps six rows, and leaves the lower footer line's ink bottom at 1074
of 1080. That is the chosen fix.

Final feasible values, all verified with ≥ 1 px clearance on every one of the four
constraints:

| Layout | firstY | spacing | bottom | item scale | hintGap | visible |
| ------ | -----: | ------: | -----: | ---------: | ------: | ------: |
| `kSettingsLayout` (footer moved) | 420 | 90 | 870 | 4 | 60 | 6 |
| `kListLayout` | 300 | 90 | 870 | 4 | 52 | 7 |
| `kProgressLayout` | 300 | 70 | 920 | 3 | 50 | 9 |
| `kPickerLayout` | 280 | 52 | 900 | 3 | 52 | 12 |
| `kPickerLayoutWithInstruction` | 332 | 52 | 900 | 3 | 52 | 11 |
| `kPaneLayout` (new) | 330 | 76 | 880 | 3 | 52 | 8 |

**An earlier attempt is recorded because it is the obvious wrong answer.** A
single `kHintGap = 36` does make every screen pass a *line-box* model, and it is
wrong: at 36 px the `MORE ABOVE` hint's own 44 px line box overlaps the first row
it sits above. The line-box model does not check hint-against-row, only
hint-against-furniture, which is why it looked like a fix. The corrected verifier
must assert hint-against-row in both directions, and the plan's milestone 2 step 7
says so.

### M3 — text widths, from the baked advances

The rail must hold `BLOODBORNE TITLE ID   CUSA03173` because §9 D1 makes the
title ID a rail row and §9 D2 keeps the shipped label. That row, not the widest
category, is what sizes the column.

| Quantity | Value | Budget |
| -------- | ----: | -----: |
| `BLOODBORNE TITLE ID   CUSA03173`, scale 3 | 552 px | 580 px rail — **the binding constraint** |
| `SEED   0123456789`, scale 3 (any seed; all digits share an advance) | 258 px | 580 px rail |
| `TITLE ID   NOT SET`, scale 3 — the empty-title-ID state | 295 px | 580 px rail |
| Widest category label, `WEAPONS & STARTING GEAR`, scale 3 | 458 px | 580 px rail |
| …spelled `WEAPONS AND STARTING GEAR` | 506 px | also fits 580 px |
| `FINISH`, scale 3 | 105 px | 580 px rail |
| Widest category heading, `WEAPONS & STARTING GEAR`, scale 4 | 614 px | 700 px pane |
| Widest setting label, `DO NOT RANDOMIZE CAGED DOGS`, scale 3 | 550 px | — |
| …plus a 40 px gap plus its value `NO` | 640 px | 700 px pane |
| Widest value string, `85 OF 85`, scale 3 | 118 px | — |
| Header readout `SEED  0123456789`, scale 4 | 340 px | x 60, 1800 px of band |
| Header readout `BLOODBORNE TITLE ID   CUSA03173`, scale 4 | 743 px | same |
| `TARGET  CUSA03173`, scale 3 | 315 px | right-aligned to x 1860 |
| Footer `UP DOWN MOVE   LEFT RIGHT CHANGE   X SELECT   O BACK`, scale 3 | 926 px | 1920 px |
| Longest single word in any help text, scale 3 | 196 px | 440 px help pane |
| Widest setting label wrapped as a help title at 440 px | ≤ 2 lines | 2 allowed |
| Appendix A help text, greedy-wrapped at 440 px, scale 3 | ≤ 10 lines | 11 allowed |
| The authored `BLOODBORNE TITLE ID` help, same | 8 lines | 11 allowed |
| …every help text at 460 px or wider | ≤ 9 lines | for reference |

The column split that satisfies all of these is
`60 + 580 + 20 + 2 + 18 + 700 + 20 + 2 + 18 + 440 + 60 = 1920`, giving rail at
x 60, column rules at x 660 and x 1400, pane at x 680 and help pane at x 1420.

**This replaces an earlier split of `60 + 520 + 40 + 700 + 40 + 500 + 60`.** That
one was derived while the seed and title ID were header items rather than rail
rows, so the rail only had to hold a 458 px category label. Once §9 D1 put
`BLOODBORNE TITLE ID   CUSA03173` in the column, 520 px was 32 px short. The
60 px the rail gains comes out of the help pane, which costs it one wrapped line
at worst (9 → 10 of an allowed 11).

`PROTECT CAGED DOGS` (spec §7.1's name) is 360 px at scale 3, 190 px narrower than
the shipped label. §9 D2 keeps the shipped label, so the pane budget is sized for
the wider one.

### M4 — the vertical layout, checked for overlap

Using the M1 ink boxes, every element of plan §4.4 in ascending order, with the
gap to the element above:

| Element | y | ink | clear of previous by |
| ------- | -: | --- | -------------------: |
| Screen title (scale 5) | 36 | 52–109 | — |
| Header readout (scale 4) | 118 | 132–176 | 23 px |
| Header rule | 196 | 196–198 | 20 px |
| Rail row 0 (scale 3) | 230 | 239–274 | 41 px |
| Rail rule 1 | 296 | 296–298 | 22 px |
| Rail categories (scale 3, pitch 76) | 330…710 | 339–754 | 41 px; 41 px between rows |
| Rail rule 2 | 778 | 778–780 | 24 px |
| `FINISH` (scale 3) | 806 | 815–850 | 35 px |
| Pane heading (scale 4) | 220 | 234–278 | 36 px below the header rule |
| `MORE ABOVE` (scale 3) | 278 | 287–322 | 9 px below the pane heading |
| Pane rows (scale 3, pitch 76) | 330…862 | 339–906 | 17 px below the hint |
| `MORE BELOW` (scale 3) | 914 | 923–958 | 17 px below the last row |
| Help title (scale 3, ≤ 2 lines) | 220, 272 | 229–316 | 22 px below the header rule |
| Help rule | 330 | 330–332 | 14 px |
| Help body (scale 3, ≤ 11 lines) | 356…876 | 365–920 | 24 px |
| Footer (scale 3) | 1000 | 1009–1044 | 51 px below `MORE BELOW`; 36 px of screen below |

No pair overlaps. The tightest margin is the 9 px between the pane heading and the
`MORE ABOVE` hint, which only appears when a category has more than 8 settings —
not possible today, and not possible with the whole backlog either (M5).

**What §9 D1's extra rail row cost, and what it did not.** The rail column gained
a row and a rule and grew from 6 focus positions to 8. Placing the categories at
y 330 rather than y 240 puts them on the same band and the same pitch as the pane
rows, which is why the column still ends at y 850 — 110 px inside its own column
rule (210…960) and 159 px above the footer's ink. Setup Defaults, with no
`FINISH` and no second rule, ends at y 754.

**The pane is untouched by this.** `kPaneLayout` is still `{330, 76, 880, 52}`,
still 8 visible rows, and every `hintGap` in M2's table is unchanged: the rail and
the pane are separate columns, and nothing in the rail's vertical budget feeds
into the pane's. The only knock-on from D1 is horizontal — the 60 px the rail
takes from the help pane (M3).

### M5 — how many settings a category can hold

From spec §7.1, counting the "current" and "backlog" columns:

| Category | Today | With the whole backlog |
| -------- | ----: | ---------------------: |
| Enemies | 4 | 7 |
| Bosses | 2 | 5 |
| Items & Treasure | 3 | 7 |
| Weapons & Starting Gear | 4 | 4 |
| Difficulty | 4 | 6 |
| World | 1 | 5 |
| **Total** | **18** | **34** |

The pane shows 8. Scrolling therefore never triggers today and triggers for no
category even once the backlog lands — but it is implemented anyway, because the
`ScrollToShow` machinery is already there and a pane that silently truncates is
the failure mode being removed.

18 settings plus the seed is 19 items, which matches spec Appendix A's "19 items"
and matches both screens' 21 → 19 row count after the two save-data rows go.

### M6 — the `defaults.cfg` budget and the verifier cases it moves

`pool_verify.py` (line ~684) rebuilds the worst-case config line by line and
asserts `worst == 669` **exactly**, plus `worst < max(bufs)` where `bufs` is
`char buf[1024]` parsed out of the store source. Removing the two keys removes
`backup_existing_save=1\n` (23 bytes) and `replace_save_default_is_new=1\n`
(30 bytes):

```
669 - 23 - 30 = 616
```

Verified by re-running the same arithmetic with the two entries removed. `616 <
1024`, so the buffer does not change and its comment's cited figure does.

Two further cases in the same selftest are source-text assertions that milestone 1
must keep in step:

* `"032: unchanged_bell_maidens is gone from load AND save (D1)"` strips the exact
  line `// Dropping unchanged_bell_maidens here is genuinely free: the loader
  above` before asserting the token is absent. Milestone 1 extends that comment to
  name the two new tolerated keys, so either the stripped line stays byte-identical
  or the case is updated with it.
* The picker-string cases split `ModelPicker.h` on `kEnemiesSkippedStrings` and
  count seven quoted strings. Milestone 2 corrects the comments *above* those
  blocks; it must not add a quoted string inside one.

### M7 — SDL symbols, checked in the archive before designing against them

```
$ llvm-nm --defined-only $OO_PS4_TOOLCHAIN/lib/libSDL2.a | grep -w <symbol>
```

| Symbol | Present |
| ------ | ------- |
| `SDL_RenderFillRect` | yes |
| `SDL_SetRenderDrawBlendMode` | yes |
| `SDL_RenderDrawLine` | yes (not used) |
| `SDL_SetTextureBlendMode` | yes (already used by `FontAtlas`) |
| `SDL_SetTextureAlphaMod` | yes |
| `SDL_RenderSetClipRect` | yes |

And the software backend's blended-fill implementations are compiled into the
archive, not stubbed:

```
$ llvm-nm --defined-only libSDL2.a | grep -i BlendFillRect
T SDL_BlendFillRect
t SDL_BlendFillRect_RGB555
t SDL_BlendFillRect_RGB565
t SDL_BlendFillRect_RGB888
t SDL_BlendFillRect_ARGB8888
t SDL_BlendFillRect_RGB
t SDL_BlendFillRect_RGBA
```

This reduces the alpha-blend risk from "will it link" to "does it behave", and it
is why the plan does not add a library or a texture. `LIBS` is untouched.

---

## E5. Risk analysis

### E5.1 Renumbering nineteen rows across three vectors

The highest-frequency failure mode in this file's history: three shipped features
each recorded that they appended last specifically to avoid it, and
`ui_scroll_verify.py` cannot catch it because it only models counts and pixels,
never labels.

Milestone 1 cannot avoid renumbering — it deletes rows 0 and 1 of the wizard and
rows 1 and 2 of Setup Defaults. What bounds the risk:

* it is the last time those constants exist at all; milestones 3 and 4 delete them;
* the mitigation is procedural rather than clever — rewrite each constant block
  top-to-bottom beside its `items` vector in one pass, then read the two side by
  side;
* the milestone-1 hardware test names it explicitly: *every label against the right
  value*. A mislabelled row is obvious on screen and invisible everywhere else.

An alternative considered and rejected: delete the two rows but leave the
remaining constants at their old values with two holes. It compiles, it needs no
renumbering, and it makes `kSaveDataRowCount` 21 with two unreachable indices —
which is exactly the "unreachable but present" state spec §10 (9.1b) forbids.

### E5.2 Alpha blending is unproven on this hardware

The dimmed overlay is the one requirement in the spec with no proven
implementation path. What is established:

* `SDL_SetRenderDrawBlendMode` and the software backend's `SDL_BlendFillRect_*`
  family are both in the linked archive (M7), so this is not a bad-NID risk of
  the `sceAppInstUtilInitialize` class — no new `.sprx` is loaded.
* The renderer is `SDL_CreateSoftwareRenderer` over the window surface
  (`Renderer::Init`), so the blend is a CPU path in code that is already linked and
  already running.
* `FontAtlas` already sets `SDL_BLENDMODE_BLEND` on its textures and
  `SDL_RenderCopy`s them, so a *texture* blend path is already exercised — but
  **the font atlas has not been hardware tested either**, so that is not evidence
  of anything yet.

What bounds it: milestone 2 exercises the exact call on an existing screen with
one small change, before any of the new UI is built, so the answer arrives at the
cheapest possible moment. If the answer is no, the fallback — an opaque
1 px-on/1 px-off scanline fill giving a fixed 50 % dim with no blending — costs
one `for` loop and no new SDL call, and is a visual change the developer should
approve rather than the implementer choose.

Cost, if it works: one full-screen blended fill is 2 073 600 pixels of read-modify-
write per frame on a CPU rasteriser, and it happens only while a picker is open.
The font-atlas report notes draw calls went *down* when the atlas landed, so there
is headroom, but frame pacing is on the hardware checklist for milestones 2 and 3
regardless.

### E5.3 The atlas fallback, if it ever fires

The font atlas is hardware-verified and good (§9 D4), so no milestone is gated on
it and every measurement in plan §4.4 rests on baked metrics that are known to be
the ones the console draws with. What remains is the fallback path, which is
silent by design.

`Renderer::DrawText` uses `Font8x8` whenever `FontAtlasReady()` is false, and
`FontAtlasInit` returns false only if a texture build fails — a runtime allocation
failure, logged as `FontAtlasInit: texture build failed, falling back to 8x8` and
otherwise invisible. `Font8x8`'s glyph table is 42 characters, with no lowercase,
no `&` and no `.`, and it advances the pen for a character it cannot render. Under
that path the help pane is a block of full-width blanks and two category labels
have a hole in them; the screen stays navigable and its central new feature does
not.

This does not change the design — it makes `live.log` part of every hardware
handoff, which is what plan §3.3 and the §7 stop conditions say.

A rejected mitigation: write the help text in uppercase so the fallback survives.
It would make the help pane shout at the player in the app's only body copy, and
the atlas exists precisely to stop the port writing for a 42-glyph font.


### E5.4 Output parity has no automated proof

Every Python mirror in `app/tools/` checks the *rules*. None of them can prove
that a settings screen assembles the same `EnemyRandomizerOptions` it did before,
because that assembly is C++ and there is no host compiler.

What the plan does instead:

* freezes the assembly as a §3.1 invariant — field by field, same values, same
  order, `StartCommit`'s big `||` untouched;
* keeps the wizard's option plumbing out of the model entirely: `AdjustSetting`
  writes a `RandomizerDefaults`, and `StartCommit` reads that struct exactly as it
  reads `defaults_` today;
* puts a byte-identical output diff in the milestone-4 hardware handoff, which is
  the only thing that can actually close B17.

The residual risk is a typo in the copy from 15 loose members to one struct — for
example assigning `options.randomizeStartingGuns = run_.randomizeStartingWeapons`.
That survives every verifier and shows up only in the diff. It is why milestone 4
is the milestone with the parity test attached.

### E5.5 Frame cost of three panes

Today's settings screen draws ~8 strings a frame. The new one draws, worst case:
1 title + 2 header + 7 rail + 1 heading + 8 rows × 2 (label + value) + 2 hints +
2 help title + 11 help body + 1 footer ≈ 43 strings, plus 5 filled rects. At scale
3 a help line is ~30 glyphs, so roughly 600 `SDL_RenderCopy` calls per frame
against roughly 200 today.

Unquantifiable without the console. Two things bound it: the font-atlas change
replaced up to 64 `FillRect`s per glyph with one `RenderCopy`, so the per-glyph
cost is already far lower than what shipped and ran acceptably; and nothing here
is in a time-critical loop — the randomizer's own work is stepped from
`UpdateProgress`, on a different screen. Frame pacing is on the hardware checklist
for milestones 3 and 4.

### E5.6 `DrawScrollHints`' signature change

Adding a field to `ListLayout` breaks every aggregate initialiser that omits it —
which is a compile error, not a silent one, and there are six. That is the desired
behaviour: a new layout that forgets to state its hint gap should not compile.

### E5.7 The help pane's wrapping is computed every frame

Greedy wrapping measures each word with `TextWidth`, which is a per-character
table lookup. Worst case is ~55 words for the longest help text, i.e. ~330
lookups, once per frame, only for the selected setting. Negligible beside the
~600 `RenderCopy`s. Caching was considered and rejected as premature — the input
changes only when the cursor moves, but the cache invalidation would be the only
complicated thing in the draw path.

---

## E6. What the spec's appendix claimed

Spec Appendix B carries eight research notes. Verified:

| Claim | Finding |
| ----- | ------- |
| The settings model is the load-bearing change; layout is mechanical once settings have stable identities | **Confirmed.** §E2.1: two parallel structures, 21 constants each, 190 lines of branch chain, and three features that each accepted a worse row placement rather than renumber |
| The `X`-no-longer-toggles decision deletes the entire second branch chain in `UpdateSaveData` | **Confirmed.** Lines 290–377, 20 branches, 16 of them duplicates of the Left/Right chain |
| `ModelPicker` is the closest existing thing to the overlay requirement; the gap is that it clears rather than compositing | **Confirmed**, and its own header comment names the missing `FillRect` as the reason (§E2.3) |
| `Controls.h`'s scroll windowing needs no changes | **Confirmed for the maths, wrong for the drawing.** `ClampScroll`/`ScrollToShow`/`VisibleRowCount` take no pixels and are reused unchanged; `DrawScrollableList` and `DrawScrollHints` centre on the whole screen and cannot draw into a pane (§E2.5). Three new draw helpers are needed |
| Renderer primitives needed: filled rect, line/separator, alpha-blended full-screen fill; all three in the linked SDL2, none exposed | **Confirmed present** (§E4 M7), but the line is **not needed** — a 2 px `FillRect` is a separator, and skipping it halves the number of unproven entry points |
| `pool_verify.py` parses string literals out of `EnableWizardScreen.cpp` — check it before moving or renaming any constant | **Confirmed and understated.** It also parses `ModelPicker.h` and `RandomizerDefaultsStore.cpp` (lines 661–663, 686–706), and two of its cases are source-text assertions milestone 1 must move — including the exact equality `worst == 669` at line 706, whose sum at line 686 starts with `backup_existing_save=1` (§E2.6, §E4 M6) |
| The model must express "this setting is editable here but not there" | **Not borne out.** All 18 settings are editable on both screens; the asymmetry is the two host items (seed, title ID), which occupy the same rail row on each screen, and what happens at exit. No per-entry flag is needed (§E2.4) |
| Both screens drop 21 → 19; sequence the removal as its own step first | **Confirmed and adopted** as milestone 1. It is also the moment the `pool_verify.py` config arithmetic has to move (§E4 M6), which is easier to do while the flat lists still exist |

Spec §4.6's two "stale font-era assumptions" were also checked directly:
`ui_scroll_verify.py`'s `glyph_h` is indeed `8 * scale`; the real line box is
44/59/73/87 (M1); and correcting it does **not** leave today's geometry passing —
two screens fail, which the spec did not predict (M2).

---

## E7. Anything that could not be established

* **Whether alpha blending works.** Symbol presence and backend compilation are
  established (M7); behaviour on firmware 9.00 with this SDK is not. Milestone 2
  exists to settle it.
* **Legibility at TV distance.** The help pane is the first body copy this app has
  drawn. Scale 3 is proven for list rows and progress lines, not for 10 lines of
  prose in a 440 px column. If it reads too small the remedy is scale 4, which
  costs the help text several more lines and may exceed the 11-line budget — in
  which case the pane band has to grow downward. Only the console can make that
  call.
* **Frame pacing with three panes and a blended overlay** (§E5.5).
* **Whether the seed reading in two places at once — the rail row under the cursor
  and the header readout above it — looks deliberate or redundant.** §9 D1 puts the
  value on the row for Left/Right feedback and keeps the header copy so the seed
  stays visible from any category. Which one a player actually looks at is not
  something the source can answer.
* **Whether category order on Confirm reads better than the flat order** (§9 D3).
  No way to establish this without a person looking at it.
* **The exact focus-highlight colour.** The plan says "a dark tint of
  `Palette::Selected`" and leaves the value to the implementer, because `Palette`
  has no such entry today and picking a number off a monitor for a TV has been
  wrong before on this project.
