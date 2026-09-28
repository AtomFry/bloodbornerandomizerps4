# UI and platform backlog — inventory and status

The running list of work items that are **not randomization settings**: screens,
navigation, presentation, packaging and platform behaviour. It is the sibling of
`randomization-feature-spec.md`, which stays authoritative for anything derived
from the reference tool's settings list.

**Why this file exists.** `docs/features/README.md` has been recording that
platform/UI items have no backlog row to be numbered after, and that three such
folders (`font-atlas/`, `randomizer-settings-ui/`, `worlds/`) had crossed the
stated threshold for giving them one. This is that backlog. Existing unnumbered
folders keep their names; new items get a `U`-prefixed row.

## Status legend

Same meanings as the randomization backlog, so the two read alike:

| Label | Meaning |
|---|---|
| **DONE** | Implemented, on hardware, confirmed working |
| **BUILT** | Implemented and green off-console, **not yet hardware-confirmed** |
| **TODO** | Not implemented |
| **IDEA** | Captured, not yet specified — no commitment that it ships |
| **OUT** | Deliberately out of scope |

---

## 1. Shipped

| # | Item | What it did | Status | Folder |
|---|---|---|---|---|
| U0a | Font atlas | Replaced the 8×8 bitmap font with an EB Garamond atlas | **DONE** | [features/font-atlas/](features/font-atlas/) |
| U0b | Randomizer Settings UI | The six-category settings screen; re-modelled Setup Defaults | **BUILT** — none of the four milestones hardware-tested | [features/randomizer-settings-ui/](features/randomizer-settings-ui/) |
| U0c | Worlds | Named playthroughs with their own save data; retired the Enable/Disable wizards | **DONE** — hardware test passed in full 2026-09-25 | [features/worlds/](features/worlds/) |
| U0d | Startup screen | Replaced the streaming boot log with a loading screen | **BUILT** — awaiting hardware test | [features/startup-screen/](features/startup-screen/) |

U0d is the **precedent for U1 and U2 below**: same complaint (a screen that
reads as a log), same intended fix (state what is happening, not every step
taken). Read its spec before specifying either.

---

## 2. The screens that read as logs — both done (0 open)

| # | Item | What it does | Status |
|---|---|---|---|
| U1 | **Simplify the activation screens** | Replaced the streaming per-phase log with the startup screen's loading shape, and success now hands straight to the `WORLDS` tab instead of a result screen | **BUILT** 2026-09-26 — **not yet hardware-tested**. See [features/U1-activation-screens/](features/U1-activation-screens/) |
| U2 | **Condense the activation confirmation** | Replaced the 27-row scrolling review with two fixed tiers — five rows of what is about to happen, three of what this world is — that never scroll | **BUILT** 2026-09-26 — **not yet hardware-tested**. See [features/U2-confirm-screen/](features/U2-confirm-screen/) |

Both were built directly from these rows, without a spec or a plan, at the
developer's request. Each folder holds an `implementation-report.md` and nothing
else.

**U1's shaping decision.** Full startup-screen parity over a condensed result
screen: a successful activation draws nothing and returns to the rail, and the
generation's counts live in `live.log` alone. `Step::Problem` is the only state
that holds, and it keeps the log, because a refusal and a failure are the one
case where the detail is what the player is there for. The three-line closing
flourish (`A HUNTER MUST HUNT`) had nowhere left to live and was removed — if it
is wanted back, the `WORLDS` rail is the place for it.

**U2's shaping decision.** Of the four options put to the developer, the chosen
one was the two-tier split **with no expansion** — so the per-setting list is
gone from that screen entirely rather than collapsed behind a drill-in. The
tension the row recorded ("keep all the information" versus "never scroll") was
therefore resolved in favour of never scrolling: the screen states the
consequence, and the recipe is a count.

**The startup bar was changed to match the activation bar** in the same pass
(2026-09-26), replacing its five discrete cells with one continuous fill. Two
loading screens in one session that disagree about what a bar looks like is a
seam the player can see, and removing that seam is why U1 copied the startup
geometry to the pixel in the first place. The five-stage machine behind it is
unchanged, so the fill advances in fifths.

### U2 — the tension to resolve in the spec

The developer's two requirements pull against each other: *keep all the
information* and *never scroll*. At the current row count those cannot both hold
literally, so the spec has to choose how the information compresses. Candidates,
none chosen:

- **Non-default only.** List the settings that differ from vanilla defaults and
  summarise the rest as a count. Makes the common case short and the unusual
  case honest.
- **Counts with drill-in.** One line per category (`ENEMIES — 4 of 9 on`), with
  the full list still reachable by opening the category.
- **Grouped ticks.** A compact grid of on/off marks rather than a label/value
  row each.

Whichever wins, the screen must still state the things that are not settings and
cannot be summarised away: which world, which seed, whether the save is kept or
started fresh, and any refusal or store error (`storeError_`).

---

## 3. Open — the look (1)

| # | Item | What it does | Status | Cost |
|---|---|---|---|---|
| U4 | **Bloodborne menu styling** | Restyle every screen toward the game's own menus — warm near-black ground, gold headings, framed panels, banded selection, hairline row separators, a vignette, button glyphs. Not a layout change: the geometry U1, U2 and the settings screens settled stays put | **BUILT** 2026-09-27, all four passes — **not hardware-tested** | Medium, and spread across every screen |

**Reference:** the in-game inventory screen (three framed columns, gold headings,
parchment body text, banded selection, hairline separators, button glyphs in the
footer). Provided by the developer 2026-09-26.

**The observation this rests on.** The two hardest things to fake are already in
place: the atlas is **EB Garamond**, a serif, and the palette already carries gold
(`Selected`), parchment (`Text`) and blood red (`Mark`). What is wrong is mostly
*hue* — `Heading` is a bright cyan-blue and the ground is a cool blue-grey, and
there is no blue anywhere in the reference.

**Planned as passes, because none of it can be verified off-console.** The
verifiers check geometry and fit; whether a screen feels like Bloodborne is a
television judgement, and colour in particular lies on a monitor.

| Pass | What | Status |
|---|---|---|
| 1 | **Palette + frames** — the warm ground, gold headings, and framed panels with corner ticks | **BUILT** 2026-09-26 |
| 2 | **Selection band + hairline row separators** — and the picker's first selection highlight | **BUILT** 2026-09-27 |
| 3 | **Vignette** — as a graded ground rather than a blend | **BUILT** 2026-09-27 |
| 4 | **Footer button glyphs** — five baked into the atlas, footers become prompt rows | **BUILT** 2026-09-27 |

All four passes are built and **none is hardware-tested**. That is the whole of
what U4 set out to do; the texture question below stays open and unattempted.

**Pass 2 as built.** `DrawSelectionBand` replaced five flat `FillRect` highlights
across three screens with a three-tone band closed by a bright edge, and
`DrawRowSeparator` puts a 1px hairline between rows of both rails, both settings
panes and the picker. The picker gained a selection band it never had — its
cursor was a colour change and nothing else, which on 82 rows at scale 3 was the
hardest cursor in the app to find.

**Pass 3 is a graded ground, not a blend over the frame — and that is a
performance decision with a measured reason.** This app draws through
`SDL_CreateSoftwareRenderer`, so every pixel is CPU work and `FillRectBlend` is
read-modify-write. Blending a 280px margin on four edges is **1,366,400 blended
pixels on top of** the 2,073,600-pixel opaque clear: 66% more fill every frame,
in the more expensive mode. Drawing the ground as fourteen concentric rings
instead costs **exactly what the clear cost**, because the rings tile the screen
and no pixel is written twice — and that tiling is re-derived by a verifier case
on every run, because the whole cost argument depends on it.

What it gives up: an opaque ground darkens the **background** only, so text and
frames near the edges are not dimmed the way a blended vignette would dim them.
On this app that is the better end of the trade — what sits near the edges is the
tab strip, the headings and the footer prompts, and those are meant to stay
readable.

**Pass 4 baked five glyphs into the font atlas** — the four face buttons and a
d-pad, at codepoints 128–132, drawn by `app/tools/button_glyphs.py` the way the
Hunter's Mark is drawn by `mark_glyph.py`. No C++ changed to reach them:
`FindGlyph` already took an `unsigned char` and `DrawText` already reinterpreted
its text as unsigned.

Every footer in the app became a **prompt row** — `DrawPromptRow` draws each
glyph in the button's own colour and each label in `Dim`. That colour is the
reason the row exists at all: one `DrawText` call carries one colour, so a
footer built as a single string can only ever be monochrome, and the letters
`X` and `O` that used to stand in for the glyphs looked like nothing on the pad.
Nineteen rows across four screens, all measured against `DrawPromptRow`'s own
arithmetic by the verifier.

Three things that fell out of it, all now pinned:

- **The buttons are excluded from the text ink box**, as the mark already was.
  They happen to sit inside it and change nothing today — which is exactly why
  it is written down rather than left to luck. Without it, resizing a glyph
  would silently move every row clearance in the app.
- **`font_atlas_verify.py` re-derives all five glyphs** from `button_glyphs.py`
  and compares them to the bake. The failure it is really there for is editing
  the drawing and forgetting to re-run the generator; confirmed to catch it
  (40 mismatches on a deliberate 0.72 → 0.80 change).
- **Escapes, not raw bytes.** `kBtnCross` and friends are written `""` in
  their own literals: 0x80 and up are not valid UTF-8 alone, and a source file
  carrying them raw is at the mercy of how a compiler reads it.

**Amended after the first hardware test, 2026-09-27.** Two things the console
showed that a mockup could not:

- **The three picker footers were still prose** — `SQUARE ALL   TRIANGLE NONE
  O BACK` as a flat string, naming buttons in words on the one screen where
  every other footer had stopped doing that. They are prompt rows now, and
  `PickerStrings` carries the two VERBS rather than a whole line, since the
  three buttons are the same on all three pickers and only the wording differs
  (`ALL` / `SKIP ALL`).
- **Left/right had no glyph at all.** There was one d-pad glyph, used wherever a
  prompt meant *any* direction, and every horizontal prompt was plain text — so
  `UP DOWN CHANGE` got a symbol and `LEFT RIGHT CHANGE` did not, in the same
  footer. There are now **three** d-pads: all arms lit, up/down lit, left/right
  lit. The unlit arms are baked at partial coverage, so they tint to a darker
  shade of the same colour — two brightnesses inside one glyph, one draw call,
  no change to `DrawText`. The words `LEFT RIGHT` and `UP DOWN` are gone from
  the labels, because the glyph now says which half of the pad and the text was
  saying it twice.

The d-pad arms are drawn much thicker than a face button's symbol (`DPAD_W`
0.185 against `SYM_W` 0.080). A thin cross reads as a maths symbol rather than a
pad, and the axis variants need the area: at the symbol width there was not
enough of either arm for the lit/unlit contrast to register at 17px.

**The pad is four arms around a hub, not two crossing bars** — a small hole at
the centre, so what the glyph says is *there are four directions to press*
rather than *here is one plus-shaped button*. It also sharpens the axis
variants: with the hub open, a lit pair reads as two separate lit arms instead
of one bar running through the middle. They are drawn as rectangles rather than
strokes for it, because a stroke's rounded cap fills the hub back in from both
sides — and square inner ends are what a real pad has anyway.

**L1/R1 keep their words.** They are labelled shoulder buttons rather than
symbols, and inventing a glyph for a button with its name printed on it would be
worse than the words. The picker's second footer line stays prose for the same
reason — its two halves are not button prompts.

**The picker is deliberately NOT framed, and the measurement is kept as a
verifier case.** A frame enclosing the list must enclose its scroll hints, so its
top edge belongs between the count line's ink (ends at 229) and `MORE ABOVE`'s
(starts at 237) — an **eight-pixel window** for a 2px line needing clearance on
both sides. A first attempt at 236 drew the frame straight through the hint.
Dropping the hints outside the frame does not rescue it either: the two picker
layouts disagree about where that gap is. Buying the room means moving constants
pinned so the shipped pickers render pixel for pixel, which is a layout change
rather than a styling pass. The case fails if that window ever opens, which is
when framing the picker becomes worth revisiting.

**Out of reach without a platform change, and deliberately not attempted.** The
parchment grain and painted page edges in the reference are bitmap textures.
`Renderer` has no texture path at all — it is `FillRect`, `FillRectBlend` and baked
atlas glyphs — so adding one is a Platform-layer change rather than a styling pass.
Revisit only after pass 1–3 have been seen on a television, and only if they are
not enough.

## 4. Open — new screens wanted by randomization rows (1)

| # | Item | What it does | Status | Cost |
|---|---|---|---|---|
| U3 | **Per-map enemy pool sub-screen** | The front-end for randomization row 35. Reuses the `ModelPicker` component behind the three existing drill-ins, but adds a map dimension: pick a map, then pick that map's pool. Needs a per-map override state distinct from "inherits the global list", or the screen cannot show the difference | **IDEA** — blocked on row 35's spec | Medium. The picker exists; the map axis, the override state and its config encoding do not |

Randomization row 17 (per-zone boss toggles) also wants a sub-screen, and the
randomization backlog has said for a while that it does. If U3 and row 17 are
built near each other, they should share one map/zone-axis component rather than
growing two.

---

## 5. Open — packaging and distribution (1)

| # | Item | What it does | Status | Cost |
|---|---|---|---|---|
| U5 | **Bundle the vanilla data in the PKG** | Ships the vanilla source tree inside the randomizer's own package, so installing the app is the whole setup and the manual FTP step disappears. **Deliberately held until the public release build** — it adds ~78 MB to every build and install, which is not worth paying on every feature | **DEFERRED — decided 2026-09-27**, do at release | Low-to-medium, and almost all of it is build plumbing |

**The decision, and why it is a deferral rather than an idea.** The shape is
settled: the data goes in the app's own package and the app reads it from
`/app0` directly. What is deferred is only *when* — shipping it now would put
78 MB into every rebuild-install cycle during development for no gain, since the
developer already has the tree in place. Pick it up when a public release build
is being prepared, and treat it as part of that work rather than as an
independent feature.

**Why it matters more than its size suggests.** The reason this port exists is
that the Windows reference tool works well and is genuinely fun, but you have to
remember where the files go and how to drive it, and it is not well documented.
*Install it, run it, it works* is the product. A setup that ends with "now FTP
78 MB into this exact path" is the one place the port still fails that, so
turnkey install is the goal U5 serves — not package hygiene.

**What it needs.** The app has never read `/app0` at all: the font atlas, the
glyphs and every table are baked into the binary, so there is no existing read
path to extend. `/app0` is our own package mount, readable by our own process
(`docs/ps4-homebrew-findings.md` §1), so nothing about it is unproven in
principle — but reading a large bundled tree from it has not been done here.

**Only three subtrees are actually read** (`EnemyRandomizer.cpp`):
`map/mapstudio/*.msb.dcx`, `event/common.emevd.dcx` and
`param/gameparam/gameparam.parambnd.dcx`. Bundling only those may cut the 78 MB
substantially, and the tree that ships should be that set rather than a whole
`dvdroot_ps4` copy.

**Open questions for the release work:**

- Whether the randomizer reads `/app0` as the vanilla source directly, or keeps
  `/data/bbrandomizer/VanillaSource/` as an override that wins when present. The
  override is worth keeping for development — it is how a modified tree gets
  tested without a rebuild — and it is also the escape hatch if a regional SKU
  ever needs different data.
- What the refusal text becomes. `THE VANILLA SOURCE IS MISSING - COPY
  DVDROOT_PS4 IN OVER FTP` should become unreachable in a release build; decide
  whether it stays as a development-only path or goes.
- Whether the release PKG is built from a `data/` tree that is gitignored and
  absent on a clean checkout — the build must still succeed without it, or
  every ordinary build breaks.
- Distribution: the release PKG would contain game files. That is the
  developer's own dump and the packaging step is local; publishing is a separate
  question and this row takes no position on it.

### Rejected: a companion data PKG

The first shape considered was a second PKG carrying the data, installed once,
so app updates stayed small. **Rejected on 2026-09-27** because it is not
turnkey: an installed PKG's payload lands in its own encrypted container, which
only that title can read, so the companion would have to be an app the user
launches once to copy its tree into `/data` — install, launch, wait, optionally
uninstall. That is fewer steps than FTP but it is still a documented procedure,
and the whole point is to have none.

### Closed: reading the vanilla files from the installed game

Not possible from this app, and already probed on hardware —
`docs/ps4-homebrew-findings.md` §1: Bloodborne's files sit in an encrypted PFS
image mounted only inside its own sandbox, and nine candidate content roots plus
a `dvdroot_ps4` hunt found nothing with the game stopped and suspended.

Game dumpers get at them one of two ways, and neither is available here: by
reading the game's sandbox mount while the game itself is running — the PS4 runs
one title at a time, so this app is not up when that mount exists — or by
mounting and decrypting the PFS image with a kernel payload. Both are privilege
escalation, which `CLAUDE.md` §1 puts outside this project. GoldHEN's own AFR
plugin does not read the installed files either; it hooks the game's file opens
at runtime. **Do not re-open this.**

---

## 6. Related

- `randomization-feature-spec.md` — the randomization settings backlog; rows 35
  and 36 are the two new randomization ideas captured alongside U1–U3
- `features/README.md` — artifact index; authority on document state, not feature state
- `screen-text-inventory.md` — generated per-string character, word and wrapped-line
  counts for the WORLDS and DEFAULTS screens. The measuring tool for U1 and U2;
  extend it to the Confirm and Progress steps as part of that work
- `deferred-ideas.md` — ideas deliberately declined; check before proposing more
- `plans/ui-blueprint-wizards.md` — **frozen and retired**, describes the Enable/Disable
  wizards removed on 2026-09-24. Not current UI
