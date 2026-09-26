#!/usr/bin/env python3
"""Generate docs/screen-text-inventory-proposed.md — the copy pass, for review.

Holds the PROPOSED replacement for every user-facing string, pairs each with the
string that ships today, and measures both the same way text_inventory.py does,
so "shorter" is a number and not a claim. Nothing here touches the source; the
proposals move into the app only once the developer confirms them.

    python tools/text_proposed.py [output.md]
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import settings_ui_verify as V  # noqa: E402
import text_inventory as INV  # noqa: E402


def lines(s):
    return len(V.wrap(s, 3, 440))


def words(s):
    return len(s.split())


# --------------------------------------------------------------------------
# The proposals. Key = the C++ identifier or setting label it replaces.
# --------------------------------------------------------------------------

SETTINGS = {
 "RANDOMIZE ENEMIES":
    "Randomizes enemy placements throughout the world. Bosses are unaffected.",
 "ENEMIES INCLUDED":
    "Choose which enemies can appear as replacements.",
 "ENEMIES SKIPPED":
    "Choose enemies that keep their original placements. They may still appear "
    "as replacements elsewhere.",
 "DO NOT RANDOMIZE CAGED DOGS":
    "Keeps the caged dogs of Central Yharnam and the Forbidden Woods in their "
    "original placements. Other enemies behave badly in these cages.",
 "RANDOMIZE BOSSES":
    "Randomizes boss placements throughout the world.",
 "BOSSES INCLUDED":
    "Choose which bosses can appear as replacements.",
 "RANDOMIZE TREASURE":
    "Randomizes items found throughout the world.",
 "RANDOMIZE WORKSHOP TOOLS":
    "Adds the Blood Gem and Rune Workshop Tools to the treasure pool. Requires "
    "Randomize Treasure.",
 "RANDOMIZE ENEMY DROPS":
    "Randomizes what enemies drop when killed.",
 "RANDOMIZE STARTING WEAPONS":
    "Randomizes the trick weapons offered in the Hunter's Dream.",
 "RANDOMIZE STARTING GUNS":
    "Randomizes the firearms offered in the Hunter's Dream.",
 "RANDOMIZE SHOP WEAPONS":
    "Randomizes the weapons sold by the Bath Messengers.",
 "START WITH HUNTER TOOLS":
    "Start with the Blood Gem and Rune Workshop Tools. Gems and runes can be "
    "fitted from the first area.",
 "EASY SHADOWS":
    "Replaces the duplicate Shadows of Yharnam with harmless larvae, so the "
    "fight is a duel.",
 "EASY ROM":
    "Replaces Rom's attendant spiders with harmless larvae.",
 "EASY FAILURES":
    "Replaces the duplicate Living Failures with harmless larvae.",
 "EASY EMISSARY":
    "Replaces the Celestial Emissary's lesser emissaries with harmless larvae.",
 "ENABLE MERGO DARKNESS":
    "Turns the Wet Nurse's arena darkness on for the whole game, from your "
    "first spawn.",
 "SAVE DATA":
    "Keep Existing restores this world's save, or uses your current save if it "
    "has none. Start Fresh begins a new playthrough, and applies to this "
    "activation only. Your save is always backed up first.",
}

RAIL = {
 "kTitleIdHelp":
    "The PS4 title ID of the Bloodborne installation the randomizer writes to. "
    "CUSA03173 is the Europe and Game of the Year release.",
 "kNameHelp":
    "What this world is called. Up to 16 characters. Renaming does not create a "
    "new revision.",
 "kSeedHelp":
    "Determines this world's randomization. The same seed and settings always "
    "produce the same world.",
 "kHistoryHelp":
    "Every set of settings this world has had, newest first. Choosing an older "
    "one makes it current, so the list only grows.",
}

WORLDS_HELP = {
 "kHelpNewWorldBody": "Create a new world using your default settings.",
 "kHelpVanillaBody":  "Play Bloodborne with the original game files and save data.",
 "kHelpWorldBody":    "A saved randomizer setup and its save data.",
}

NOTES = {
 "kNoteNewWorld":          "STARTS FROM YOUR DEFAULT SETTINGS",
 "kNoteVanilla":           "NO SETTINGS - CANNOT BE DELETED",
 "kNoteNoContainer":       "RUN BLOODBORNE ONCE BEFORE ACTIVATING THIS WORLD",
 "kNoteContainerTooSmall": "THIS WORLD'S SAVE IS TOO LARGE FOR THIS CONSOLE",
}

DELETE = {
 "kDeleteTitle":          "DELETE WORLD",
 "kDeleteLine1":          "REMOVES THIS WORLD AND ALL ITS REVISIONS",
 "kDeleteLine2":          "ITS SAVE DATA IS KEPT",
 "kDeleteKeptLine":       "SAVE DATA KEPT IN SAVEBACKUPS",
 "kDeleteFooter":         "X DELETE   O CANCEL",
 "kDeleteRefusedFooter":  "O BACK",
 "kRefuseVanilla":        "VANILLA CANNOT BE DELETED",
 "kRefuseActive":         "ACTIVE WORLDS CANNOT BE DELETED",
}

STARTUP = {
 "kWordmark":                    "BLOODBORNE RANDOMIZER",
 "kLoadingWord":                 "LOADING",
 "kProblemTitle":                "STARTUP PROBLEM",
 "kProblemReconcileFailed":      "AN INTERRUPTED ACTIVATION COULD NOT BE FINISHED",
 "kProblemJournalNotUnderstood": "AN INTERRUPTED ACTIVATION COULD NOT BE READ - IT WAS LEFT ALONE",
 "kProblemCaptureFailed":        "YOUR SAVE COULD NOT BE COPIED INTO VANILLA",
 "kProblemNoSignedInPlayer":     "NO PLAYER SIGNED IN",
 "kProblemScrollHint":           "UP DOWN SCROLL",
 "kPromptContinue":              "X CONTINUE",
 "kPromptExit":                  "O EXIT",
}

# Editor's activation confirmation - outside the original inventory, included
# because voice consistency is the point and these are the busiest sentences.
CONFIRM = [
 ("kConfirmHeading",      "CONFIRM ACTIVATION",                 "CONFIRM ACTIVATION"),
 ("kCannotActivate",      "CANNOT ACTIVATE",                    "CANNOT ACTIVATE"),
 ("kNoOutgoingWorld",     "NOTHING - NO WORLD IS ACTIVE",       "NOTHING - NO WORLD IS ACTIVE"),
 ("kOutgoingNoContainer", "NO CONTAINER - NOTHING TO BACK UP",  "NOTHING TO BACK UP"),
 ("kOutgoingEmpty",       "LEFT ALONE - CONTAINER IS EMPTY",    "NOTHING TO BACK UP - SAVE IS EMPTY"),
 ("kOutgoingNowhere",     "BACKED UP - FILED NOWHERE",          "BACKED UP - NO WORLD TO SAVE IT TO"),
 ("kOutgoingFiled",       "BACKED UP AND FILED INTO IT",        "BACKED UP AND SAVED TO IT"),
 ("(already active)",     "THIS WORLD IS ALREADY ACTIVE - ITS SAVE STAYS WHERE IT IS",
                          "UNCHANGED - THIS WORLD IS ALREADY ACTIVE"),
 ("(restore)",            "THIS WORLD'S OWN SAVE IS PUT BACK",  "THIS WORLD'S SAVE IS RESTORED"),
 ("(adopt)",              "THIS WORLD HAS NO SAVE YET - IT ADOPTS THE ONE ON THE CONSOLE",
                          "ADOPTS YOUR CURRENT SAVE - THIS WORLD HAS NONE"),
 ("(start fresh)",        "THE SAVE IS BACKED UP AND THE GAME STARTS A NEW PLAYTHROUGH",
                          "BACKED UP, THEN A NEW PLAYTHROUGH BEGINS"),
]

REFUSALS = [
 ("save title, zero",  "NO BLOODBORNE SAVE DATA FOR THIS PLAYER - RUN BLOODBORNE ONCE FIRST",
                       "RUN BLOODBORNE ONCE BEFORE ACTIVATING THIS WORLD"),
 ("ownership",         "THIS WORLD BELONGS TO ANOTHER ACCOUNT",
                       "THIS WORLD BELONGS TO ANOTHER ACCOUNT"),
 ("container absent",  "THIS WORLD HAS A SAVE AND THERE IS NO CONTAINER - RUN THE GAME ONCE",
                       "RUN BLOODBORNE ONCE BEFORE ACTIVATING THIS WORLD"),
 ("no signed-in user", "NO SIGNED-IN PLAYER - THE ACTIVATION CANNOT BE FINISHED",
                       "NO PLAYER SIGNED IN"),
]


def main():
    entries, cat_labels, rail_help, categories, _ = V.parse_model()
    w = INV.parse_worlds_strings()
    cur = {e["label"]: e["help"] for e in entries}

    out = []
    p = out.append

    p("# Screen text inventory — PROPOSED")
    p("")
    p("**For review. Nothing here has been applied to the source.** Confirm or")
    p("amend, and I will push the agreed wording into the app and regenerate")
    p("`screen-text-inventory.md` from it.")
    p("")
    p("Counts are **lines in the 440px help column at scale 3**, measured the same")
    p("way as the current inventory. The pane allows 11.")
    p("")
    p("---")
    p("")
    p("## The rules applied")
    p("")
    p("* **Lead with the effect.** \"Randomizes enemy placements.\"")
    p("* **Cut implementation detail.** \"drawn from the enemy pool\" is")
    p("  documentation, not UI.")
    p("* **Drop `Default: Off.` everywhere.** The value column two inches to the")
    p("  left already says `YES` or `NO`. That sentence cost a line on all sixteen")
    p("  toggles and told the player nothing they could not see.")
    p("* **Drop defensive framing.** \"Not a randomizer\", \"Different from…\",")
    p("  \"Independent of…\" — all explaining what a setting *is not*.")
    p("* **Keep real dependencies.** \"Requires Randomize Treasure\" changes what a")
    p("  player does; a paragraph on why does not.")
    p("* **Game vocabulary.** Hunter's Dream, Bath Messengers, Workshop Tools,")
    p("  Shadows of Yharnam.")
    p("* **Case convention is unchanged.** Help bodies are sentence case; rail")
    p("  labels, pane values, notes and footers stay uppercase — they are chrome")
    p("  and data, and they sit beside `YES`, `ACTIVE`, `14.5 MB IN 17 FILES`.")
    p("")
    p("---")
    p("")

    # ---------------------------------------------------------- flag
    p("## Where I disagree with the review — `ENABLE MERGO DARKNESS`")
    p("")
    p("The review flagged my wording as possibly reversed, and suggested")
    p("*\"Removes the Wet Nurse\'s darkness from Mergo\'s Loft.\"* **I checked the")
    p("code, and that suggestion would be wrong in both direction and scope.** It")
    p("reads the shipping help text as ground truth, and the shipping help text is")
    p("the thing that is wrong.")
    p("")
    p("`app/src/Randomizer/PermaDarkness.h` states it in capitals, and says it was")
    p("measured on console in both directions:")
    p("")
    p("```")
    p("permaDarknessOn == true   -> writes the game\'s own shipped values")
    p("                             -> a permanently DARK world")
    p("permaDarknessOn == false  -> writes the poke (SpEffect 99999)")
    p("                             -> the normal, LIT game")
    p("```")
    p("")
    p("The same header adds **\"not an m26-local effect\"** — the darkness is not")
    p("confined to Mergo\'s Loft; it runs from the first spawn. Backlog row 8 and")
    p("`docs/user-guide.md` agree with the code and with each other.")
    p("")
    p("So the shipping string is wrong twice: it has the direction backwards *and*")
    p("localises a global effect to one area. This is exactly the trap the file")
    p("warns about in its own comment — **\"CAREFUL WITH THE POLARITY, AND DO NOT")
    p("REASON IT OUT FROM THE BYTES\"** — because the decode of which byte means")
    p("what genuinely does suggest the opposite of what the console does.")
    p("")
    p("My revised wording keeps the direction and fixes the scope, and borrows the")
    p("user guide\'s framing, which is the clearest of the three:")
    p("")
    p("> Turns the Wet Nurse\'s arena darkness on for the whole game, from your")
    p("> first spawn.")
    p("")
    p("**No code, setting name or value has been touched.** If you would rather")
    p("confirm on console before changing this one string, it is the one item here")
    p("worth holding back — but leaving the current text means shipping a sentence")
    p("that tells the player the opposite of what will happen.")
    p("")
    p("---")
    p("")
    p("## The three other questions the review raised")
    p("")
    p("**`TITLE ID` — is \"four letters and five digits\" needed?** No, and it is")
    p("dropped. `SetupDefaultsScreen.cpp` hard-constrains the editor: positions")
    p("0-3 cycle letters, 4-8 cycle digits, over a fixed length. A player cannot")
    p("enter an invalid shape, so describing the shape is describing a rule the UI")
    p("already enforces. **\"PS4 title ID\" is restored** — that was a real loss.")
    p("")
    p("**`HISTORY` — is \"nothing is ever lost\" literally true?** Not quite, so it")
    p("is gone. Revisions are append-only within a world, but deleting a world")
    p("removes its folder and every revision in it (`WorldStore.cpp:898`). The")
    p("replacement — *\"so the list only grows\"* — is exactly true and makes the")
    p("same point.")
    p("")
    p("**Activation refusals — is the duplicated \"run Bloodborne once\" message")
    p("intentional?** Yes, and deliberately so. The two cases have different")
    p("causes (no save title found at all, versus a world with a stored save and")
    p("no container) but the player\'s action is identical, and the second can only")
    p("arise when the first is also true. One sentence for one remedy is better")
    p("than two phrasings of it, and it is the same sentence as the pane note in")
    p("§4, so the player meets one message in all three places.")
    p("")
    p("**`kOutgoingNowhere` / `kOutgoingFiled` vocabulary.** Caught correctly — my")
    p("first pass still said \"filed\" in one and \"saved\" in the other. Both now")
    p("use one pair: a save is **backed up** (to `SaveBackups`) and **saved to** a")
    p("world. \"Filed\" is gone everywhere.")
    p("")
    p("**Grammar convention, kept as the review described it.** A toggle says what")
    p("happens — *\"Randomizes…\"*. A picker says what the player is choosing —")
    p("*\"Choose which…\"*. That split is intentional and is now the rule.")
    p("")
    p("---")
    p("")

    # ---------------------------------------------------------- settings
    p("# 1. Settings")
    p("")
    by_cat = {}
    for e in entries:
        by_cat.setdefault(e["category"], []).append(e)

    for ci, cat in enumerate(categories):
        if cat not in by_cat:
            continue
        label = cat_labels[ci] if ci < len(cat_labels) else cat
        p("## %s" % label)
        p("")
        for e in by_cat[cat]:
            lab = e["label"]
            old, new = cur[lab], SETTINGS[lab]
            p("### %s" % lab)
            p("")
            p("| | Lines | Words |")
            p("| --- | --- | --- |")
            p("| Now | %d | %d |" % (lines(old), words(old)))
            p("| **Proposed** | **%d** | **%d** |" % (lines(new), words(new)))
            p("")
            p("*Now:* %s" % old)
            p("")
            p("**Proposed:** %s" % new)
            p("")

    # ---------------------------------------------------------- rail rows
    p("---")
    p("")
    p("# 2. Rail rows that are not settings")
    p("")
    for key, name in [("kTitleIdHelp", "TITLE ID — DEFAULTS tab"),
                      ("kNameHelp", "NAME — editor"),
                      ("kSeedHelp", "SEED — editor"),
                      ("kHistoryHelp", "HISTORY — editor")]:
        old, new = rail_help[key], RAIL[key]
        p("### %s" % name)
        p("")
        p("| | Lines | Words |")
        p("| --- | --- | --- |")
        p("| Now | %d | %d |" % (lines(old), words(old)))
        p("| **Proposed** | **%d** | **%d** |" % (lines(new), words(new)))
        p("")
        p("*Now:* %s" % old)
        p("")
        p("**Proposed:** %s" % new)
        p("")

    # ---------------------------------------------------------- worlds help
    p("---")
    p("")
    p("# 3. WORLDS screen — help column")
    p("")
    for key, title in [("kHelpNewWorldBody", "+ NEW WORLD"),
                       ("kHelpVanillaBody", "VANILLA"),
                       ("kHelpWorldBody", "WORLD")]:
        old, new = w[key], WORLDS_HELP[key]
        p("### %s" % title)
        p("")
        p("| | Lines | Words |")
        p("| --- | --- | --- |")
        p("| Now | %d | %d |" % (lines(old), words(old)))
        p("| **Proposed** | **%d** | **%d** |" % (lines(new), words(new)))
        p("")
        p("*Now:* %s" % old)
        p("")
        p("**Proposed:** %s" % new)
        p("")

    # ---------------------------------------------------------- short strings
    def table(heading, mapping, source):
        p("---")
        p("")
        p("# %s" % heading)
        p("")
        p("| String | Now | Proposed |")
        p("| --- | --- | --- |")
        for k, new in mapping.items():
            old = source.get(k, "—")
            mark = "" if old == new else " **←**"
            p("| `%s` | %s | %s%s |" % (k, old, new, mark))
        p("")

    table("4. WORLDS screen — pane notes", NOTES, w)
    table("5. Delete confirmation", DELETE, w)
    table("6. Startup screen", STARTUP, w)

    p("---")
    p("")
    p("# 7. Activation confirmation — world editor")
    p("")
    p("*Outside the original inventory. Included because consistent voice is the")
    p("point, and these are the sentences a player reads immediately before the")
    p("one irreversible thing the app does.*")
    p("")
    p("| String | Now | Proposed |")
    p("| --- | --- | --- |")
    for k, old, new in CONFIRM:
        mark = "" if old == new else " **←**"
        p("| `%s` | %s | %s%s |" % (k, old, new, mark))
    p("")

    p("---")
    p("")
    p("# 8. Activation refusals")
    p("")
    p("*Two of these say the same thing in different words today; the proposal")
    p("makes them one sentence, which is also the pane note in §4.*")
    p("")
    p("| Case | Now | Proposed |")
    p("| --- | --- | --- |")
    for k, old, new in REFUSALS:
        mark = "" if old == new else " **←**"
        p("| %s | %s | %s%s |" % (k, old, new, mark))
    p("")

    # ---------------------------------------------------------- totals
    p("---")
    p("")
    p("# 9. Before and after")
    p("")
    all_old = [cur[k] for k in SETTINGS] + [rail_help[k] for k in RAIL] + \
              [w[k] for k in WORLDS_HELP]
    all_new = list(SETTINGS.values()) + list(RAIL.values()) + list(WORLDS_HELP.values())
    p("| | Help strings | Total lines | Total words | Longest |")
    p("| --- | --- | --- | --- | --- |")
    p("| Now | %d | %d | %d | %d lines |"
      % (len(all_old), sum(lines(s) for s in all_old),
         sum(words(s) for s in all_old), max(lines(s) for s in all_old)))
    p("| **Proposed** | %d | **%d** | **%d** | **%d lines** |"
      % (len(all_new), sum(lines(s) for s in all_new),
         sum(words(s) for s in all_new), max(lines(s) for s in all_new)))
    p("")
    p("Against a ceiling of 11 lines, the longest is now 6 and most are 1 or 2.")
    p("Only `SAVE DATA` reaches 6, and it is the one setting that can destroy")
    p("something. `DO NOT RANDOMIZE CAGED DOGS` and `TITLE ID` are 5; everything")
    p("else is 4 or fewer.")
    p("")

    dest = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", "..",
        "docs", "screen-text-inventory-proposed.md")
    dest = os.path.normpath(dest)
    with open(dest, "w", encoding="utf-8", newline="") as f:
        f.write("\n".join(out) + "\n")
    print("wrote %s (%d lines)" % (dest, len(out) + 1))


if __name__ == "__main__":
    main()
