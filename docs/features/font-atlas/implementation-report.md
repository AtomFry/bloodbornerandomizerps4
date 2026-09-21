# Font atlas — implementation report

**This item was deliberately taken outside the pipeline.** There is no spec and
no plan. The developer chose this route on 2026-09-20, the same way row 34
(`START WITH HUNTER TOOLS`) was handled, and this report was written after the
fact. A folder that merely looks thin is indistinguishable from one where the
stages were forgotten, so this paragraph exists to say which it is.

The folder is not numbered. Every other folder is `NNN-<slug>` after a backlog
row in `docs/randomization-feature-spec.md`; this is platform work, not a
randomization setting, so it has no row to be named after.

**Status:** implemented, cross-compiled, Python-verified. **Not yet hardware
tested** — per `CLAUDE.md` §3 that means *ready for hardware testing*, not done.

---

## 1. What this changes

Text was drawn by `src/Platform/Font8x8.cpp`: 42 hand-authored 8×8 one-bit
glyphs — uppercase A–Z, digits, five punctuation marks — scaled by integer
nearest-neighbour, one `SDL_RenderFillRect` per lit pixel. A capital `M` at
scale 3 was up to 64 draw calls producing 3×3 blocks.

Text is now drawn from EB Garamond, rasterised on the PC at author time and
baked into a generated header. All of printable ASCII (32–126), anti-aliased,
proportionally spaced, with real baselines and descenders.

See the "Why this is not a new library" note in §4 — this adds nothing to
`LIBS` and therefore carries none of the load-time NID risk that
`docs/ps4-homebrew-findings.md` §7 warns about.

## 2. Files

| File | Role |
|---|---|
| `app/tools/fonts/EBGaramond-Variable.ttf` | source typeface, SIL OFL 1.1 |
| `app/tools/fonts/OFL.txt` | its license — must travel with any redistribution |
| `app/tools/gen_font_atlas.py` | new — rasterises the TTF, emits the baked header |
| `app/tools/font_atlas_verify.py` | new — layer-2 verifier (see §5) |
| `app/src/Platform/FontAtlasData.h` | new, **generated** — 854 KB, do not hand-edit |
| `app/src/Platform/FontAtlas.h` / `.cpp` | new — runtime: build textures, draw, measure |
| `app/src/Platform/Renderer.cpp` | routes `DrawText`/`TextWidth` to the atlas |
| `app/src/UI/EnableWizardScreen.cpp` | seed editor caret — monospace fix |
| `app/src/UI/SetupDefaultsScreen.cpp` | title-ID editor caret — monospace fix |

No `Makefile` change: `SRC_CPP` uses `find`, so `FontAtlas.cpp` is picked up
automatically.

## 3. How the data is shaped

Four sizes are baked, one per text scale the UI actually passes — 3, 4, 5 and 6,
found by grepping the `kScale*` constants under `src/UI/`.

**The scale→pixel mapping is 11×scale, not 8×scale.** This is the one thing in
this change that is easy to get wrong, and it was got wrong first. `Font8x8`'s
glyphs filled their 8-pixel box, so a capital stood `8 * scale` px tall. In a
real typeface the em is the *design* size, not the cap height — EB Garamond's
capitals are ~0.71 em. Baking at `8 * scale` therefore rendered every capital at
71% of its former height, and the average advance at 54% of its former width: a
legibility regression on a TV that nobody asked for, invisible in a diff and
invisible to the compiler.

`11 * scale` restores it. Measured cap heights against the old font:

| scale | em px | old cap h | new cap h | line height | vs old |
|---|---|---|---|---|---|
| 3 | 33 | 24 | 24 | 44 | 100% |
| 4 | 44 | 32 | 30 | 59 | 94% |
| 5 | 55 | 40 | 39 | 73 | 98% |
| 6 | 66 | 48 | 46 | 87 | 96% |

Line height stays inside the tightest row spacing the UI uses — 52 px in the
enemy picker at scale 3, against a 44 px line — so no list needed re-spacing, and
`ui_scroll_verify.py` still passes unchanged.

Per glyph the generator stores position in the atlas, ink width/height,
`bearingX`, `bearingY` and `advance`. The bearings and the per-glyph advance are
the whole point: `Font8x8` had a single fixed `kAdvance = 9`, so `I` occupied
exactly as much space as `W` and nothing had a baseline.

Coverage bytes are stored **tightly** — each glyph's `w*h` ink box, concatenated
— not as the padded atlas rectangle, which is a ~44% saving for identical pixels.
`FontAtlas.cpp` expands those runs into one texture per size at startup, placing
each at its baked `atlasX`/`atlasY`. Total baked coverage is 301 KB across the
four sizes; `FontAtlas.o` compiles to 321 KB.

RLE would save roughly a third more but adds a decode step. Not taken; revisit
only if the header size becomes a problem.

### Build trap

**The `Makefile` has no header dependencies.** `$(INTDIR)/%.o: $(PROJDIR)/%.cpp`
lists no headers, so regenerating `FontAtlasData.h` does **not** cause anything
to rebuild — `make` reports success and silently packages the previous atlas.
This was hit during this work: the first `.pkg` after changing `SCALES` still
contained the old sizes.

After running `gen_font_atlas.py`, always:

```bash
touch app/src/Platform/FontAtlas.cpp && make
```

Confirm it took by checking the size of the compiled object, since the `.pkg`
size is block-aligned and will not visibly change.

## 4. Runtime notes

**Why this is not a new library.** FreeType runs on the PC, inside PIL, in
`gen_font_atlas.py`. The PS4 side only blits pixels, using SDL2 calls already
present in the linked `libSDL2.a` (verified against the archive before writing
any code): `SDL_CreateRGBSurfaceWithFormatFrom`, `SDL_CreateTextureFromSurface`,
`SDL_RenderCopy`, `SDL_SetTextureColorMod`, `SDL_SetTextureBlendMode`. `LIBS` in
the `Makefile` is untouched.

**Colour.** The atlas is white with alpha carrying the coverage, so
`SDL_SetTextureColorMod` tints one atlas to every `Palette` colour. No
per-colour duplication.

**Contract preserved.** `FontAtlasDrawText` takes the text's **top-left**, exactly
as `DrawText8x8` did, and computes the baseline internally as `y + ascent`. That
is why no screen's layout maths had to change.

**Fallback retained.** `Font8x8` is still compiled and still wired up. If
`FontAtlasInit` fails, `Renderer` silently uses the 8×8 path, so a failure costs
legibility rather than the whole app. Reverting is one condition in
`Renderer.cpp`. Once hardware confirms the atlas, `Font8x8` can be deleted.

**Draw calls went down, not up.** One `RenderCopy` per character replaces up to
64 `FillRect`s. On a software renderer that is a net win.

## 5. Verification

`python app/tools/font_atlas_verify.py` — **PASS**.

Two independent checks, because a clean cross-compile proves `FontAtlas.cpp`
links and proves nothing about whether glyphs land in the right place. Baseline
and bearing errors compile perfectly and look wrong only on a TV.

1. **Metrics.** Every baked number for all 95 glyphs × 4 sizes is re-derived
   straight from the TTF and compared. 380 glyph entries, no mismatches.
2. **Layout.** The C++ draw loop's arithmetic is replayed in Python and the
   composed result compared against PIL's own rendering of the same string —
   5 samples × 4 sizes, within a 1 px rounding tolerance.

One verifier quirk worth recording: PIL reports the space character's mask as
`(5, 0)` — width but zero height. The generator normalises anything with a zero
dimension to `(0, 0)` plus an advance, and the runtime skips the blit either
way, so the verifier normalises before comparing. The layout check passing on
samples containing spaces is what confirms the space *advance* is correct.

**Build:** `make` from `app/` completes clean, no warnings.
`IV0000-BBRD00001_00-BBRANDOMIZERMAIN.pkg`, 7.1 MB.

## 6. What hardware testing must confirm

1. **The app still launches.** The binary grew by ~301 KB of `.rodata`. Nothing
   suggests this is a problem; it is simply the first thing to look at.
2. **Text renders at all**, and `live.log` says `FontAtlasInit ok` rather than
   `FontAtlasInit: texture build failed, falling back to 8x8`. The fallback is
   silent by design, so the log is the only way to tell which path ran.
3. **Alpha blending works on the software renderer.** `SDL_BLENDMODE_BLEND` has
   never been exercised in this app. Anti-aliased edges depend on it. If it is
   unsupported, glyphs will look hard-edged or boxed rather than absent.
4. **Legibility at TV viewing distance.** Cap heights now match the old font to
   within 6%, so this should be no worse than what shipped — but "matches the
   old font" was itself only ever checked on a monitor. If it reads too small,
   change the `SCALES` table in `gen_font_atlas.py` and re-run — no C++ change
   needed. Mind the build trap above when you do.
5. **The two caret editors.** Seed entry and title-ID entry now step by each
   character's own width. Check the highlight sits under the right character
   across the whole field.
6. **Redraw cost.** Menus redraw every frame today. One `RenderCopy` per glyph
   should be cheaper than the old per-pixel fills, but this is a CPU rasteriser
   and it is worth watching for sluggishness.

## 7. Follow-ups deliberately not done

* **`docs/known-traps.md` §"8x8 font limitations"** is now wrong — the
  "uppercase and digits only, unknown characters render blank silently" trap
  does not apply to the atlas path. Left for after hardware confirmation, since
  until then the fallback can still be the active path.
* **Screens still use uppercase-only strings** as a convention inherited from
  the old font's limits. Lowercase is available everywhere now; restyling the
  copy is separate work.
* **`Font8x8.cpp` not deleted** — see §4.
* **Renderer primitives** (`FillRect`, `DrawLine`, blend mode) for panels,
  dividers and selection bars are the natural next milestone. They were the
  other half of the earlier analysis and are deliberately not in this change.
