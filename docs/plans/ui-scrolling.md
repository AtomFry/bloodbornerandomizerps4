# UI list scrolling

**Status: DONE — implemented and hardware-confirmed 2026-09-13.**

Originally filed as a deferred UI complaint about the progress screen. Promoted
to real work when it turned out two *interactive* screens had grown the same
defect, which was worse.

---

## 1. The defect

The trigger was the three starting-weapon toggles: they pushed *Setup Defaults*
to 10 rows and the *Enable Randomizer* settings and confirm lists to 9, and
`DrawMenuList` drew every row unconditionally.

On a 1080-tall screen that put `RANDOMIZE STARTING WEAPONS` on top of the
footer, clipped `RANDOMIZE STARTING GUNS` at the bottom edge, and pushed
`RANDOMIZE SHOP WEAPONS` entirely off-screen — **while `NavigateVertical` still
walked through all of them, so a setting could be toggled with no visible
feedback.** That is worse than the original progress-log complaint, because
those screens are interactive.

**Not a crash risk, then or now.** The blitter goes through
`SDL_RenderFillRect`, which clips to the render target, so the off-screen rows
were discarded safely.

### The original progress-screen report

Reported from hardware 2026-09-13: with enemy drops enabled, the closing message
overlapped earlier log lines instead of appearing below them.

`EnableWizardScreen::DrawProgress` laid every line out at a fixed pitch with no
bound — lines start at `y = 300`, advance `70` per line, screen is `1080` tall,
and the `O RETURN TO MENU` footer sits at `kScreenHeight - 80` = `1000`. That
leaves room for **exactly 10 lines** before text collides with the footer, and
nothing clamped, scrolled, or paginated past that.

The line count had grown as features landed. Worst case with all four
randomizers enabled:

| Source | Lines |
|---|---|
| backup choice | 1 |
| replace-save choice | 1 |
| seed | 1 |
| enemy result | 1 |
| boss result | 1 |
| treasure result | 1 |
| enemy drops result | **2** (count + item-data size) |
| completion flourish | 3 |
| **total** | **11** |

Eleven lines needs `y` up to 1070, past the footer. Even an all-*disabled* run
hit exactly 10 (three setup lines + four "SKIPPING …" lines + three flourish
lines) — right at the boundary, which is why this only became visible when enemy
drops added its second line.

---

## 2. What was built

Scrolling as **one shared control** in `UI/Controls.h/.cpp`, not three
per-screen patches:

- `ListLayout { firstY, spacing, bottomLimit }` and `VisibleRowCount()` derive
  capacity from the space a screen actually has, instead of hardcoding a count
  that silently rots as rows are added.
- `ScrollToShow()` moves the window by the minimum needed to keep the cursor
  visible; `ClampScroll()` makes an out-of-range window unrepresentable.
- `DrawScrollableList()` for cursor lists; `DrawScrollHints()` alone for the
  progress log, which colours lines individually and appends a live status line,
  so it shares the arithmetic and draws itself.
- Hints read `MORE ABOVE` / `MORE BELOW`. **The 8x8 font is uppercase and digits
  only** — no arrow glyphs, and `FindGlyph` returns blank for anything unknown,
  so an arrow would have rendered as nothing at all.

**Behaviour:** the progress log pins to the tail while the run is going (you want
the newest line) and up/down scroll it back once the run finishes. The confirm
list has no cursor, so up/down move its window directly.

**Capacity now:** Setup Defaults 7 of 10 rows, both wizard lists 6 of 9, progress
log 9 lines finished / 8 plus the live line while running. Setup Defaults gained
a row by starting at `y=300` instead of `340`, using dead space the heading was
not occupying.

`DrawMenuList` was deliberately kept for `MenuScreen` and `SelectReplace`, which
still fit.

---

## 3. Verification

`tools/ui_scroll_verify.py` mirrors the C++ arithmetic — the standing substitute
where no host C++ compiler exists — and asserts, per screen:

- every visible row and both hints clear the heading, the footer and the screen
  edge;
- walking the cursor twice around a wrapping list always leaves it visible;
- free scrolling can never leave the list;
- the last item is reachable;
- the running log always shows its newest line.

All pass. Three stale counts in the script needed correcting as part of this
work as well as the new screen: both settings lists are 13 rows now, and the
progress log's worst case is 15 lines rather than 14 (the pool-limited line).

---

## 4. Remaining lever

`spacing` is one constant per screen in the `ListLayout`. Dropping it from 90 to
~70 would fit most of both lists without scrolling at all.

**Deliberately not done** — it changes the look of every settings screen, and
scrolling is what makes the lists correct regardless of how many rows they grow
to.
