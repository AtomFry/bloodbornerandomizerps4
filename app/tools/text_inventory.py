#!/usr/bin/env python3
"""Dump every on-screen string of the WORLDS and DEFAULTS screens for review.

Not a verifier - it asserts nothing and can never fail a build. It exists so a
copy pass has one document to mark up instead of five source files, and so the
result is the text that actually ships rather than a transcription of it.

It reuses settings_ui_verify.py's parsers, so anything that file can already
measure, this one reports identically. A string that moves or is renamed shows
up here as a KeyError rather than as silently stale prose.

    python tools/text_inventory.py [output.md]

Writes docs/screen-text-inventory.md by default. It writes the file itself
rather than printing, because this console's stdout is cp1252 and the inventory
is full of em dashes.
"""

import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import settings_ui_verify as V  # noqa: E402

UI = V.UI


def chars(s):
    return len(s)


def words(s):
    return len(s.split())


def lines_at(s, width_px=440, scale=3):
    """How many lines this wraps to in a 440px help column at scale 3."""
    try:
        return len(V.wrap(s, scale, width_px))
    except Exception:
        return None


def row(label, text, note=""):
    n = lines_at(text)
    stat = "%d chars, %d words" % (chars(text), words(text))
    if n is not None:
        stat += ", **%d lines**" % n
    out = ["| %s | %s |" % (label, stat)]
    if note:
        out.append("| | *%s* |" % note)
    return out


def block(text):
    """A quoted block the developer can edit in place."""
    return "> " + text.replace("\n", " ").strip()


def parse_worlds_strings():
    src = V.strip_comments(V.read(os.path.join(UI, "WorldsScreen.cpp")))
    out = {}
    for m in re.finditer(r'const char\* const (\w+)\s*=\s*((?:\s*"[^"]*")+)\s*;', src):
        out[m.group(1)] = "".join(re.findall(r'"([^"]*)"', m.group(2)))
    return out


def main():
    entries, cat_labels, rail_help, categories, _ = V.parse_model()
    w = parse_worlds_strings()

    out = []
    p = out.append
    p("# Screen text inventory — WORLDS and DEFAULTS")
    p("")
    p("**Generated** by `app/tools/text_inventory.py` from the source. Do not")
    p("hand-edit the strings here expecting them to ship — mark this file up, and")
    p("the edits get applied to the source and the file regenerated.")
    p("")
    p("Counts are characters, words, and **how many lines the string wraps to in")
    p("the 440px help column at scale 3**. The help pane allows **11 lines**; the")
    p("title above it allows 2. Line count is the number that matters — a long")
    p("string that still fits in 3 lines reads fine, and a short one that spills")
    p("to 12 is truncated on screen.")
    p("")
    p("---")
    p("")
    p("## How the three columns work")
    p("")
    p("Both screens share one layout: a **rail** on the left, a **pane** in the")
    p("middle, a **help column** on the right.")
    p("")
    p("| Column | WORLDS | DEFAULTS |")
    p("| --- | --- | --- |")
    p("| **Rail** | `+ NEW WORLD`, `VANILLA`, then the player's worlds | `TITLE ID`, then six category names |")
    p("| **Pane** | label/value detail rows for the highlighted world, then at most one note | the settings in the highlighted category, each a label and a value |")
    p("| **Help** | a title and a body, three variants by row type | the highlighted setting's own label and help text |")
    p("")
    p("**There is no per-category help.** On DEFAULTS the help column always shows")
    p("a *setting's* help — the one under the cursor — except on rail row 0, where")
    p("it shows the title-ID help. So every category's worth of help text is the")
    p("settings' help text, listed below.")
    p("")
    p("---")
    p("")

    # ------------------------------------------------------------------ WORLDS
    p("# 1. The WORLDS screen")
    p("")
    p("## 1.1 Rail rows")
    p("")
    p("| Row | Text | Source |")
    p("| --- | --- | --- |")
    p("| Row 0 | `%s` | fixed |" % w["kRowNewWorld"])
    p("| Row 1 | `VANILLA` | the world's name |")
    p("| Row 2+ | the world's name, up to 16 chars of A–Z, 0–9, space | player-entered |")
    p("")

    p("## 1.2 Help column — three variants")
    p("")
    for title_key, body_key, when in [
        ("kHelpNewWorldTitle", "kHelpNewWorldBody", "cursor on `+ NEW WORLD`"),
        ("kHelpVanillaTitle", "kHelpVanillaBody", "cursor on `VANILLA`"),
        ("kHelpWorldTitle", "kHelpWorldBody", "cursor on any other world"),
    ]:
        body = w[body_key]
        p("### %s — *%s*" % (w[title_key], when))
        p("")
        p("**Title:** `%s`" % w[title_key])
        p("")
        p("**Body** — %d chars, %d words, **%d lines of 11**"
          % (chars(body), words(body), lines_at(body)))
        p("")
        p(block(body))
        p("")

    p("## 1.3 Pane — detail row labels")
    p("")
    p("Label/value pairs. The labels are fixed; the values are computed.")
    p("")
    p("| Row shown for | Labels, in order |")
    p("| --- | --- |")
    p("| `+ NEW WORLD` | `SETTINGS` |")
    p("| `VANILLA` | `STATUS`, `SAVE DATA`, `LAST PLAYED`, `CREATED` |")
    p("| a world | `STATUS`, `SEED`, `SETTINGS`, `REVISION`, `SAVE DATA`, `SAVED ON`, `LAST PLAYED` |")
    p("")
    p("| Value | Form |")
    p("| --- | --- |")
    p("| `STATUS` | `ACTIVE` or `NOT ACTIVE` |")
    p("| `SAVE DATA` | e.g. `14.5 MB IN 17 FILES`, or `NONE` |")
    p("| `LAST PLAYED` | a timestamp, or `NEVER` |")
    p("| `SETTINGS` | e.g. `3 OF 16 ON` |")
    p("| `SAVED ON` | e.g. `REVISION 2`, or `-` |")
    p("")

    p("## 1.4 Pane — notes")
    p("")
    p("At most one note is shown beneath the detail rows: the row's own")
    p("explanation, or why the world cannot be activated.")
    p("")
    for key, when in [
        ("kNoteNewWorld", "shown on `+ NEW WORLD`"),
        ("kNoteVanilla", "shown on `VANILLA`"),
        ("kNoteNoContainer", "shown when Bloodborne has never made save data"),
        ("kNoteContainerTooSmall", "shown when the world's save will not fit"),
    ]:
        t = w[key]
        p("**%s** — *%s* — %d chars, %d words" % (key, when, chars(t), words(t)))
        p("")
        p(block(t))
        p("")

    p("## 1.5 Delete confirmation")
    p("")
    for key in ("kDeleteTitle", "kDeleteLine1", "kDeleteLine2", "kDeleteKeptLine",
                "kDeleteFooter", "kDeleteRefusedFooter",
                "kRefuseVanilla", "kRefuseActive"):
        t = w[key]
        p("* **%s** (%d chars) — `%s`" % (key, chars(t), t))
    p("")

    p("## 1.6 Startup screen")
    p("")
    for key in ("kWordmark", "kLoadingWord", "kProblemTitle",
                "kProblemReconcileFailed", "kProblemJournalNotUnderstood",
                "kProblemCaptureFailed", "kProblemNoSignedInPlayer",
                "kProblemScrollHint", "kPromptContinue", "kPromptExit"):
        t = w[key]
        p("* **%s** (%d chars) — `%s`" % (key, chars(t), t))
    p("")
    p("---")
    p("")

    # ---------------------------------------------------------------- DEFAULTS
    p("# 2. The DEFAULTS screen")
    p("")
    p("## 2.1 Rail rows")
    p("")
    p("| Row | Text |")
    p("| --- | --- |")
    p("| Row 0 | `TITLE ID` |")
    for i, lab in enumerate(cat_labels):
        note = "  ← **never shown on DEFAULTS**, editor only" if lab == "SAVE" else ""
        p("| Row %d | `%s`%s |" % (i + 1, lab, note))
    p("")

    p("## 2.2 Title ID")
    p("")
    t = rail_help["kTitleIdHelp"]
    p("**Rail label:** `TITLE ID`  **Help title:** `BLOODBORNE TITLE ID`  "
      "**Value:** the id, or `NOT SET`")
    p("")
    p("**Help** — %d chars, %d words, **%d lines of 11**"
      % (chars(t), words(t), lines_at(t)))
    p("")
    p(block(t))
    p("")
    p("---")
    p("")

    # ---------------------------------------------------------------- SETTINGS
    p("# 3. Every setting")
    p("")
    p("Each appears on DEFAULTS (except `SAVE DATA`) and again in the world")
    p("editor. Rail label, pane value, and help body are all listed.")
    p("")

    by_cat = {}
    for e in entries:
        by_cat.setdefault(e["category"], []).append(e)

    value_form = {
        "Toggle": "`YES` / `NO`",
        "SaveChoice": "`KEEP EXISTING` / `START FRESH`",
        "EnemyPool": "`n OF 82`",
        "EnemySkip": "`n OF 85`",
        "BossPool": "`n OF 17`",
    }

    for ci, cat in enumerate(categories):
        if cat not in by_cat:
            continue
        label = cat_labels[ci] if ci < len(cat_labels) else cat
        p("## 3.%d  %s" % (ci + 1, label))
        if label == "SAVE":
            p("")
            p("*Editor only. Deliberately absent from DEFAULTS — it is a per-world")
            p("question, not a preference.*")
        p("")
        for e in by_cat[cat]:
            h = e["help"]
            n = lines_at(h)
            p("### %s" % e["label"])
            p("")
            p("| | |")
            p("| --- | --- |")
            p("| **Rail label** | `%s` (%d chars) |" % (e["label"], chars(e["label"])))
            p("| **Pane value** | %s |" % value_form.get(e["kind"], e["kind"]))
            p("| **Help title** | `%s` — same as the label |" % e["label"])
            p("| **Help length** | %d chars, %d words, **%d lines of 11** |"
              % (chars(h), words(h), n))
            p("")
            p(block(h))
            p("")

    # ------------------------------------------------------------------ EDITOR
    p("---")
    p("")
    p("# 4. World editor only")
    p("")
    p("Three rail rows the DEFAULTS tab does not have.")
    p("")
    for key, label in [("kNameHelp", "NAME"), ("kSeedHelp", "SEED"),
                       ("kHistoryHelp", "HISTORY")]:
        t = rail_help[key]
        p("### %s" % label)
        p("")
        p("**Help** — %d chars, %d words, **%d lines of 11**"
          % (chars(t), words(t), lines_at(t)))
        p("")
        p(block(t))
        p("")

    # ------------------------------------------------------------------ LONGEST
    p("---")
    p("")
    p("# 5. Longest first")
    p("")
    p("The help strings ranked by how many lines they occupy, since that is what")
    p("makes a column look dense. Eleven is the cap.")
    p("")
    p("| Lines | Words | Setting |")
    p("| --- | --- | --- |")
    ranked = [(lines_at(e["help"]), words(e["help"]), e["label"]) for e in entries]
    ranked += [(lines_at(rail_help[k]), words(rail_help[k]), n) for k, n in
               [("kTitleIdHelp", "TITLE ID"), ("kNameHelp", "NAME"),
                ("kSeedHelp", "SEED"), ("kHistoryHelp", "HISTORY")]]
    ranked += [(lines_at(w[b]), words(w[b]), "WORLDS help — " + w[t]) for t, b in
               [("kHelpNewWorldTitle", "kHelpNewWorldBody"),
                ("kHelpVanillaTitle", "kHelpVanillaBody"),
                ("kHelpWorldTitle", "kHelpWorldBody")]]
    for n, wd, lab in sorted(ranked, reverse=True):
        p("| %d | %d | %s |" % (n, wd, lab))
    p("")


    dest = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", "..",
        "docs", "screen-text-inventory.md")
    dest = os.path.normpath(dest)
    with open(dest, "w", encoding="utf-8", newline="") as f:
        f.write("\n".join(out) + "\n")
    print("wrote %s (%d lines)" % (dest, len(out) + 1))


if __name__ == "__main__":
    main()
